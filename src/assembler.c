/*
 * assembler.c - Two-Pass Assembler Core Implementation
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#define _POSIX_C_SOURCE 200809L
#include "assembler.h"
#include "lexer.h"
#include "expr.h"
#include "opcodes.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <libgen.h>
#include <sys/stat.h>
#include <ctype.h>

/* Forward declarations for include handling */
static char *path_join(const char *dir, const char *file);
static char *get_directory(const char *filepath);
static char *read_file_content(const char *filename, long *size_out);

/* ========== Assembler Lifecycle ========== */

Assembler *assembler_create(void) {
    Assembler *as = calloc(1, sizeof(Assembler));
    if (!as) return NULL;

    as->memory = calloc(ASM_MEMORY_SIZE, 1);
    as->written = calloc(ASM_MEMORY_SIZE, 1);
    if (!as->memory || !as->written) {
        assembler_free(as);
        return NULL;
    }

    as->symbols = symbol_table_create(1024);
    if (!as->symbols) {
        assembler_free(as);
        return NULL;
    }

    as->scope = scope_create();
    as->anon_labels = anon_create();
    if (!as->scope || !as->anon_labels) {
        assembler_free(as);
        return NULL;
    }

    opcodes_init();

    as->pc = ASM_DEFAULT_ORG;
    as->org = ASM_DEFAULT_ORG;
    as->lowest_addr = 0xFFFF;
    as->highest_addr = 0;
    as->pass = 1;
    as->format = OUTPUT_PRG;
    as->fill_byte = 0x00;
    as->include_depth = 0;
    as->include_path_count = 0;
    as->cond_depth = 0;

    as->macros = macro_table_create(64);
    if (!as->macros) {
        assembler_free(as);
        return NULL;
    }
    as->macro_depth = 0;
    as->macro_unique_counter = 0;

    /* Initialize pseudo-PC state */
    as->real_pc = ASM_DEFAULT_ORG;
    as->in_pseudopc = 0;

    /* Default CPU type */
    as->cpu_type = CPU_6510;

    /* Initialize zone for local labels */
    as->current_zone = NULL;

    return as;
}

void assembler_free(Assembler *as) {
    if (!as) return;

    free(as->memory);
    free(as->written);
    free(as->current_zone);
    symbol_table_free(as->symbols);
    scope_free(as->scope);
    anon_free(as->anon_labels);
    macro_table_free(as->macros);

    /* Free assembled lines */
    for (int i = 0; i < as->line_count; i++) {
        statement_free(as->lines[i].stmt);
        free(as->lines[i].source_text);
        free(as->lines[i].zone);
    }
    free(as->lines);

    /* Free include stack entries */
    for (int i = 0; i < as->include_depth; i++) {
        free(as->include_stack[i].filename);
        free(as->include_stack[i].source);
    }

    /* Free include paths */
    for (int i = 0; i < as->include_path_count; i++) {
        free(as->include_paths[i]);
    }

    /* Free command-line defines */
    for (int i = 0; i < as->cmdline_define_count; i++) {
        free(as->cmdline_defines[i]);
    }

    free(as);
}

void assembler_reset(Assembler *as) {
    if (!as) return;

    memset(as->memory, 0, ASM_MEMORY_SIZE);
    memset(as->written, 0, ASM_MEMORY_SIZE);

    symbol_table_free(as->symbols);
    as->symbols = symbol_table_create(1024);

    scope_free(as->scope);
    as->scope = scope_create();

    /* Reset current zone for local labels */
    free(as->current_zone);
    as->current_zone = NULL;

    anon_clear(as->anon_labels);

    for (int i = 0; i < as->line_count; i++) {
        statement_free(as->lines[i].stmt);
        free(as->lines[i].source_text);
        free(as->lines[i].zone);
    }
    free(as->lines);
    as->lines = NULL;
    as->line_count = 0;
    as->line_capacity = 0;

    as->pc = ASM_DEFAULT_ORG;
    as->real_pc = ASM_DEFAULT_ORG;
    as->org = ASM_DEFAULT_ORG;
    as->lowest_addr = 0xFFFF;
    as->highest_addr = 0;
    as->in_pseudopc = 0;
    as->pass = 1;
    as->errors = 0;
    as->warnings = 0;

    /* Clear include stack (keep include_paths) */
    for (int i = 0; i < as->include_depth; i++) {
        free(as->include_stack[i].filename);
        free(as->include_stack[i].source);
    }
    as->include_depth = 0;

    /* Clear conditional stack */
    as->cond_depth = 0;

    /* Re-apply command-line defined symbols */
    for (int i = 0; i < as->cmdline_define_count; i++) {
        const char *definition = as->cmdline_defines[i];
        if (!definition) continue;

        /* Parse the definition again */
        char *def = strdup(definition);
        if (!def) continue;

        char *eq = strchr(def, '=');
        int32_t value = 1;

        if (eq) {
            *eq = '\0';
            const char *val_str = eq + 1;
            if (val_str[0] == '$') {
                value = (int32_t)strtol(val_str + 1, NULL, 16);
            } else if (val_str[0] == '%') {
                value = (int32_t)strtol(val_str + 1, NULL, 2);
            } else if (val_str[0] == '0' && (val_str[1] == 'x' || val_str[1] == 'X')) {
                value = (int32_t)strtol(val_str + 2, NULL, 16);
            } else {
                value = (int32_t)strtol(val_str, NULL, 10);
            }
        }

        symbol_define(as->symbols, def, value,
                      SYM_DEFINED | SYM_CONSTANT,
                      "<command-line>", 0);
        free(def);
    }
}

/* ========== Error Handling ========== */

void assembler_error(Assembler *as, const char *fmt, ...) {
    if (as->errors >= ASM_MAX_ERRORS) return;

    fprintf(stderr, "%s:%d: error: ",
            as->current_file ? as->current_file : "<input>",
            as->current_line);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    as->errors++;
}

void assembler_warning(Assembler *as, const char *fmt, ...) {
    if (as->warnings >= ASM_MAX_WARNINGS) return;

    fprintf(stderr, "%s:%d: warning: ",
            as->current_file ? as->current_file : "<input>",
            as->current_line);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    as->warnings++;
}

int assembler_has_errors(Assembler *as) {
    return as->errors > 0;
}

int assembler_error_count(Assembler *as) {
    return as->errors;
}

int assembler_warning_count(Assembler *as) {
    return as->warnings;
}

/* ========== Code Emission ========== */

void assembler_emit_byte(Assembler *as, uint8_t byte) {
    /* Use real_pc for actual output position when in pseudopc mode */
    uint16_t output_addr = as->in_pseudopc ? as->real_pc : as->pc;

    if (output_addr < as->lowest_addr) as->lowest_addr = output_addr;
    if (output_addr > as->highest_addr) as->highest_addr = output_addr;

    as->memory[output_addr] = byte;
    as->written[output_addr] = 1;

    /* Advance pc (virtual address for labels) */
    as->pc++;

    /* Always advance real_pc to track actual output position.
     * When not in pseudopc mode, real_pc follows pc.
     * When in pseudopc mode, real_pc tracks the actual memory location. */
    as->real_pc++;
}

void assembler_emit_word(Assembler *as, uint16_t word) {
    assembler_emit_byte(as, word & 0xFF);        /* Low byte */
    assembler_emit_byte(as, (word >> 8) & 0xFF); /* High byte */
}

void assembler_emit_bytes(Assembler *as, const uint8_t *bytes, int count) {
    for (int i = 0; i < count; i++) {
        assembler_emit_byte(as, bytes[i]);
    }
}

void assembler_set_pc(Assembler *as, uint16_t pc) {
    as->pc = pc;
    /* Also set real_pc if not in pseudopc mode */
    if (!as->in_pseudopc) {
        as->real_pc = pc;
    }
    if (as->pass == 1 && as->line_count == 0) {
        as->org = pc;
    }
}

uint16_t assembler_get_pc(Assembler *as) {
    return as->pc;
}

/* Advance PC by count bytes, also tracking real_pc for pseudopc support.
 * Use this in pass 1 where we don't actually emit bytes but need to track sizes.
 * In pass 2, use assembler_emit_byte which handles both. */
static void assembler_advance_pc(Assembler *as, int count) {
    as->pc += count;
    /* Also advance real_pc to track actual output position */
    as->real_pc += count;
}

/* ========== Utility Functions ========== */

int assembler_calc_branch_offset(uint16_t target, uint16_t pc) {
    /* Branch is relative to the address AFTER the branch instruction (pc + 2) */
    int32_t offset = (int32_t)target - (int32_t)(pc + 2);

    if (offset < -128 || offset > 127) {
        return INT_MIN; /* Out of range */
    }

    return (int)offset;
}

int assembler_is_zeropage(int32_t addr) {
    return addr >= 0 && addr <= 0xFF;
}

/* ========== Line Storage ========== */

static int add_assembled_line(Assembler *as, Statement *stmt, uint16_t address,
                              const char *source_text) {
    if (as->line_count >= as->line_capacity) {
        int new_capacity = as->line_capacity ? as->line_capacity * 2 : 256;
        AssembledLine *new_lines = realloc(as->lines, new_capacity * sizeof(AssembledLine));
        if (!new_lines) {
            assembler_error(as, "out of memory");
            return -1;
        }
        as->lines = new_lines;
        as->line_capacity = new_capacity;
    }

    AssembledLine *line = &as->lines[as->line_count++];
    memset(line, 0, sizeof(AssembledLine));
    line->stmt = stmt;
    line->address = address;
    line->source_text = source_text ? str_dup(source_text) : NULL;
    line->zone = as->current_zone ? str_dup(as->current_zone) : NULL;

    /* Extract cycle info from instruction if available */
    if (stmt && stmt->type == STMT_INSTRUCTION) {
        line->cycles = stmt->data.instruction.cycles;
        line->page_penalty = stmt->data.instruction.page_penalty;
    }

    return as->line_count - 1;
}

/* ========== Label Handling ========== */

static void define_label(Assembler *as, LabelInfo *label) {
    if (!label) return;

    uint8_t flags = SYM_DEFINED;
    if (assembler_is_zeropage(as->pc)) {
        flags |= SYM_ZEROPAGE;
    }

    if (label->is_anon_fwd) {
        anon_define_forward(as->anon_labels, as->pc, as->current_file, as->current_line);
    } else if (label->is_anon_back) {
        anon_define_backward(as->anon_labels, as->pc, as->current_file, as->current_line);
    } else if (label->is_local) {
        /* Mangle local label name with current zone */
        const char *local_part = label->name;
        if (local_part[0] == '.') local_part++;

        char *mangled;
        if (as->current_zone) {
            size_t len = strlen(as->current_zone) + 1 + strlen(local_part) + 1;
            mangled = malloc(len);
            if (mangled) {
                snprintf(mangled, len, "%s.%s", as->current_zone, local_part);
            }
        } else {
            size_t len = strlen(local_part) + 10;
            mangled = malloc(len);
            if (mangled) {
                snprintf(mangled, len, "_global.%s", local_part);
            }
        }

        if (mangled) {
            symbol_define(as->symbols, mangled, as->pc, flags,
                         as->current_file, as->current_line);
            free(mangled);
        }
    } else {
        /* Global label - also starts a new zone for local labels */
        symbol_define(as->symbols, label->name, as->pc, flags,
                     as->current_file, as->current_line);

        /* Update current zone to this label's name */
        free(as->current_zone);
        as->current_zone = strdup(label->name);
    }
}

/* ========== Instruction Assembly ========== */

int assembler_assemble_instruction(Assembler *as, Statement *stmt) {
    InstructionInfo *info = &stmt->data.instruction;

    /* Accumulator and implied modes don't need operand evaluation */
    if (info->mode == ADDR_ACCUMULATOR || info->mode == ADDR_IMPLIED) {
        if (as->pass == 2) {
            assembler_emit_byte(as, info->opcode);
        } else {
            assembler_advance_pc(as, 1);
        }
        return 0;
    }

    /* Get operand value if there is an operand */
    int32_t operand_value = 0;
    int value_defined = 1;

    if (info->operand) {
        ExprResult result = expr_eval(info->operand, as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
        operand_value = result.value;
        value_defined = result.defined;

        if (as->pass == 2 && !value_defined) {
            assembler_error(as, "undefined symbol in operand");
            return -1;
        }
    }

    /* Handle relative branches */
    if (info->mode == ADDR_RELATIVE) {
        if (as->pass == 2) {
            int offset = assembler_calc_branch_offset(operand_value, as->pc);
            if (offset == INT_MIN) {
                assembler_error(as, "branch target out of range");
                return -1;
            }
            assembler_emit_byte(as, info->opcode);
            assembler_emit_byte(as, (uint8_t)(offset & 0xFF));
        } else {
            assembler_advance_pc(as, 2);
        }
        return 0;
    }

    /* Re-evaluate addressing mode in pass 2 if operand is now known */
    if (as->pass == 2 && value_defined) {
        AddressingMode new_mode = info->mode;

        /* Check if we can use zero-page mode now that value is known */
        if (assembler_is_zeropage(operand_value)) {
            switch (info->mode) {
                case ADDR_ABSOLUTE:
                    if (opcode_find(info->mnemonic, ADDR_ZEROPAGE)) {
                        new_mode = ADDR_ZEROPAGE;
                    }
                    break;
                case ADDR_ABSOLUTE_X:
                    if (opcode_find(info->mnemonic, ADDR_ZEROPAGE_X)) {
                        new_mode = ADDR_ZEROPAGE_X;
                    }
                    break;
                case ADDR_ABSOLUTE_Y:
                    if (opcode_find(info->mnemonic, ADDR_ZEROPAGE_Y)) {
                        new_mode = ADDR_ZEROPAGE_Y;
                    }
                    break;
                default:
                    break;
            }
        }

        /* If mode changed, verify the new opcode */
        if (new_mode != info->mode) {
            const OpcodeEntry *op = opcode_find(info->mnemonic, new_mode);
            if (op && op->size == info->size) {
                /* Mode changed but size stayed same - update info */
                info->mode = new_mode;
                info->opcode = op->opcode;
            }
            /* If sizes differ, we've already committed to the larger size in pass 1 */
        }
    }

    /* Emit the instruction */
    if (as->pass == 2) {
        assembler_emit_byte(as, info->opcode);

        switch (info->size) {
            case 1:
                /* No operand */
                break;
            case 2:
                /* Single byte operand */
                if (info->mode == ADDR_IMMEDIATE) {
                    /* Immediate always low byte */
                    assembler_emit_byte(as, operand_value & 0xFF);
                } else {
                    /* Zero-page or other single-byte mode */
                    assembler_emit_byte(as, operand_value & 0xFF);
                }
                break;
            case 3:
                /* Word operand (little-endian) */
                assembler_emit_word(as, operand_value & 0xFFFF);
                break;
        }
    } else {
        assembler_advance_pc(as, info->size);
    }

    return 0;
}

/* ========== Directive Assembly ========== */

static int assemble_byte_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;

    for (int i = 0; i < dir->arg_count; i++) {
        ExprResult result = expr_eval(dir->args[i], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
        if (as->pass == 2 && !result.defined) {
            assembler_error(as, "undefined symbol in !byte directive");
            return -1;
        }

        if (as->pass == 2) {
            if (result.value < -128 || result.value > 255) {
                assembler_warning(as, "byte value $%X truncated", result.value);
            }
            assembler_emit_byte(as, result.value & 0xFF);
        } else {
            assembler_advance_pc(as, 1);
        }
    }

    return 0;
}

static int assemble_word_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;

    for (int i = 0; i < dir->arg_count; i++) {
        ExprResult result = expr_eval(dir->args[i], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
        if (as->pass == 2 && !result.defined) {
            assembler_error(as, "undefined symbol in !word directive");
            return -1;
        }

        if (as->pass == 2) {
            assembler_emit_word(as, result.value & 0xFFFF);
        } else {
            assembler_advance_pc(as, 2);
        }
    }

    return 0;
}

static int assemble_text_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;

    if (!dir->string_arg) {
        assembler_error(as, "!text requires a string argument");
        return -1;
    }

    int len = strlen(dir->string_arg);
    if (as->pass == 2) {
        assembler_emit_bytes(as, (const uint8_t *)dir->string_arg, len);
    } else {
        assembler_advance_pc(as, len);
    }

    return 0;
}

static int assemble_fill_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;

    if (dir->arg_count < 1) {
        assembler_error(as, "!fill requires count argument");
        return -1;
    }

    ExprResult count_result = expr_eval(dir->args[0], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
    if (!count_result.defined) {
        assembler_error(as, "!fill count must be constant");
        return -1;
    }

    int count = count_result.value;
    if (count < 0 || count > 65536) {
        assembler_error(as, "!fill count out of range");
        return -1;
    }

    uint8_t fill_value = 0;
    if (dir->arg_count >= 2) {
        ExprResult value_result = expr_eval(dir->args[1], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
        if (as->pass == 2 && !value_result.defined) {
            assembler_error(as, "!fill value must be defined");
            return -1;
        }
        fill_value = value_result.value & 0xFF;
    }

    if (as->pass == 2) {
        for (int i = 0; i < count; i++) {
            assembler_emit_byte(as, fill_value);
        }
    } else {
        assembler_advance_pc(as, count);
    }

    return 0;
}

static int assemble_org_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;

    if (dir->arg_count < 1) {
        assembler_error(as, "org directive requires address");
        return -1;
    }

    ExprResult result = expr_eval(dir->args[0], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
    if (!result.defined) {
        assembler_error(as, "org address must be constant");
        return -1;
    }

    uint16_t new_pc = result.value & 0xFFFF;
    assembler_set_pc(as, new_pc);

    return 0;
}

/* PETSCII conversion: lowercase ASCII to uppercase PETSCII */
/*
 * ASCII to PETSCII conversion table
 *
 * PETSCII on the C64 has the following character arrangement:
 * - $00-$1F: Control characters
 * - $20-$3F: Same as ASCII (space, punctuation, digits)
 * - $40-$5F: Uppercase letters (A-Z at $41-$5A)
 * - $60-$7F: Graphics characters
 * - $80-$9F: Control characters (shifted)
 * - $A0-$BF: Graphics characters
 * - $C0-$DF: Shifted letters (graphics in default charset)
 * - $E0-$FF: More graphics
 *
 * In the default uppercase/graphics mode (most common):
 * - PETSCII $41-$5A displays as uppercase A-Z
 *
 * The !pet directive converts ASCII to PETSCII uppercase:
 * - ASCII 'A'-'Z' ($41-$5A) -> PETSCII $41-$5A (uppercase)
 * - ASCII 'a'-'z' ($61-$7A) -> PETSCII $41-$5A (converted to uppercase)
 *
 * This matches ACME's behavior for compatibility.
 */
static uint8_t ascii_to_petscii(uint8_t c) {
    /* Uppercase A-Z: ASCII $41-$5A -> PETSCII $41-$5A */
    if (c >= 'A' && c <= 'Z') return c;

    /* Lowercase a-z: ASCII $61-$7A -> PETSCII uppercase $41-$5A */
    if (c >= 'a' && c <= 'z') return c - 0x20;  /* Convert to uppercase */

    /* Special character mappings */
    switch (c) {
        case '@':  return 0x40;  /* @ same position */
        case '[':  return 0x5B;  /* [ */
        case '\\': return 0x5C;  /* British pound £ in PETSCII */
        case ']':  return 0x5D;  /* ] */
        case '^':  return 0x5E;  /* Up arrow */
        case '_':  return 0xA4;  /* Underscore -> graphics underscore */
        case '`':  return 0x27;  /* Backtick -> apostrophe */
        case '{':  return 0x5B;  /* { -> [ */
        case '|':  return 0x7C;  /* | stays */
        case '}':  return 0x5D;  /* } -> ] */
        case '~':  return 0x7E;  /* ~ stays */
        default:
            /* Most ASCII chars $20-$3F (space, punctuation, digits) map directly */
            if (c >= 0x20 && c <= 0x3F) return c;
            /* Control codes and extended chars: pass through */
            return c;
    }
}

/*
 * ASCII to C64 Screen Code conversion table
 *
 * Screen codes are what the VIC-II chip uses for character display:
 * - $00-$1F: @, A-Z, [, \, ], ^, _  (letters start at $01)
 * - $20-$3F: Space, !, ", #, $, %, &, ', (, ), *, +, ,, -, ., /, 0-9, :, ;, <, =, >, ?
 * - $40-$5F: Graphics characters
 * - $60-$7F: Graphics characters
 * - $80-$FF: Reverse video versions of $00-$7F
 *
 * !scr converts ASCII to screen codes for direct screen RAM writing
 */
static const uint8_t ascii_to_screencode_table[128] = {
    /* $00-$0F: Control chars -> undefined, use $20 (space) or $3F (?) */
    0x20, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,

    /* $10-$1F: Control chars -> undefined */
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,

    /* $20-$2F: Space and punctuation */
    0x20,  /* Space */
    0x21,  /* ! */
    0x22,  /* " */
    0x23,  /* # */
    0x24,  /* $ */
    0x25,  /* % */
    0x26,  /* & */
    0x27,  /* ' */
    0x28,  /* ( */
    0x29,  /* ) */
    0x2A,  /* * */
    0x2B,  /* + */
    0x2C,  /* , */
    0x2D,  /* - */
    0x2E,  /* . */
    0x2F,  /* / */

    /* $30-$3F: Digits and more punctuation */
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,  /* 0-7 */
    0x38, 0x39,  /* 8-9 */
    0x3A,  /* : */
    0x3B,  /* ; */
    0x3C,  /* < */
    0x3D,  /* = */
    0x3E,  /* > */
    0x3F,  /* ? */

    /* $40-$5F: @, A-Z, and special chars */
    0x00,  /* @ -> screen code $00 */
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  /* A-G */
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,  /* H-O */
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,  /* P-W */
    0x18, 0x19, 0x1A,  /* X-Z */
    0x1B,  /* [ */
    0x1C,  /* \ (British pound) */
    0x1D,  /* ] */
    0x1E,  /* ^ (up arrow) */
    0x1F,  /* _ (left arrow in C64) */

    /* $60-$7F: lowercase a-z and special chars -> same as uppercase */
    0x00,  /* ` -> @ (screen code $00) */
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  /* a-g -> A-G */
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,  /* h-o -> H-O */
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,  /* p-w -> P-W */
    0x18, 0x19, 0x1A,  /* x-z -> X-Z */
    0x1B,  /* { -> [ */
    0x1C,  /* | -> \ */
    0x1D,  /* } -> ] */
    0x1E,  /* ~ -> ^ */
    0x3F   /* DEL -> ? */
};

static uint8_t ascii_to_screencode(uint8_t c) {
    if (c < 128) {
        return ascii_to_screencode_table[c];
    }
    /* Extended ASCII: return as-is or map to graphics chars */
    return c & 0x7F;
}

static int assemble_pet_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;

    if (!dir->string_arg) {
        assembler_error(as, "!pet requires a string argument");
        return -1;
    }

    int len = strlen(dir->string_arg);
    if (as->pass == 2) {
        for (int i = 0; i < len; i++) {
            assembler_emit_byte(as, ascii_to_petscii((uint8_t)dir->string_arg[i]));
        }
    } else {
        assembler_advance_pc(as, len);
    }

    return 0;
}

static int assemble_scr_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;

    if (!dir->string_arg) {
        assembler_error(as, "!scr requires a string argument");
        return -1;
    }

    int len = strlen(dir->string_arg);
    if (as->pass == 2) {
        for (int i = 0; i < len; i++) {
            assembler_emit_byte(as, ascii_to_screencode((uint8_t)dir->string_arg[i]));
        }
    } else {
        assembler_advance_pc(as, len);
    }

    return 0;
}

static int assemble_null_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;

    if (!dir->string_arg) {
        assembler_error(as, "!null requires a string argument");
        return -1;
    }

    int len = strlen(dir->string_arg);
    if (as->pass == 2) {
        assembler_emit_bytes(as, (const uint8_t *)dir->string_arg, len);
        assembler_emit_byte(as, 0x00); /* Null terminator */
    } else {
        assembler_advance_pc(as, len + 1);
    }

    return 0;
}

static int assemble_skip_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;

    if (dir->arg_count < 1) {
        assembler_error(as, "!skip requires count argument");
        return -1;
    }

    ExprResult count_result = expr_eval(dir->args[0], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
    if (!count_result.defined) {
        assembler_error(as, "!skip count must be constant");
        return -1;
    }

    int count = count_result.value;
    if (count < 0 || count > 65536) {
        assembler_error(as, "!skip count out of range");
        return -1;
    }

    /* Skip advances PC but doesn't emit anything (both pc and real_pc) */
    assembler_advance_pc(as, count);

    return 0;
}

static int assemble_align_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;

    if (dir->arg_count < 1) {
        assembler_error(as, "!align requires alignment argument");
        return -1;
    }

    ExprResult align_result = expr_eval(dir->args[0], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
    if (!align_result.defined) {
        assembler_error(as, "!align value must be constant");
        return -1;
    }

    int alignment = align_result.value;
    if (alignment <= 0 || alignment > 65536) {
        assembler_error(as, "!align value out of range");
        return -1;
    }

    /* Check if alignment is power of 2 */
    if ((alignment & (alignment - 1)) != 0) {
        assembler_warning(as, "!align value %d is not a power of 2", alignment);
    }

    /* Calculate padding needed */
    int remainder = as->pc % alignment;
    int padding = (remainder == 0) ? 0 : alignment - remainder;

    /* Get fill value */
    uint8_t fill_value = 0;
    if (dir->arg_count >= 2) {
        ExprResult value_result = expr_eval(dir->args[1], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
        if (as->pass == 2 && !value_result.defined) {
            assembler_error(as, "!align fill value must be defined");
            return -1;
        }
        fill_value = value_result.value & 0xFF;
    }

    if (as->pass == 2) {
        for (int i = 0; i < padding; i++) {
            assembler_emit_byte(as, fill_value);
        }
    } else {
        assembler_advance_pc(as, padding);
    }

    return 0;
}

static int assemble_binary_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;

    if (!dir->string_arg) {
        assembler_error(as, "!binary requires a filename argument");
        return -1;
    }

    int offset = 0;
    int length = 0;

    if (dir->arg_count >= 1) {
        ExprResult r = expr_eval(dir->args[0], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
        if (!r.defined) {
            assembler_error(as, "!binary size must be constant");
            return -1;
        }
        length = r.value;
    }

    if (dir->arg_count >= 2) {
        ExprResult r = expr_eval(dir->args[1], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
        if (!r.defined) {
            assembler_error(as, "!binary offset must be constant");
            return -1;
        }
        offset = r.value;
    }

    return assembler_include_binary(as, dir->string_arg, offset, length);
}

/* !basic directive - generate BASIC stub that SYS's to machine code
 *
 * Memory layout (for origin $0801):
 * $0801-$0802: Link to next line (points past end markers)
 * $0803-$0804: Line number (default 10)
 * $0805:       SYS token ($9E)
 * $0806-...:   Address as ASCII digits
 * ...:         End of line ($00)
 * ...:         End of program ($00 $00)
 * ...:         Machine code starts here
 *
 * Syntax:
 *   !basic           - line 10, SYS to next instruction
 *   !basic 2025      - line 2025, SYS to next instruction
 *   !basic 10, $C000 - line 10, SYS $C000
 */
static int assemble_basic_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;

    int line_number = 10;  /* Default BASIC line number */
    int sys_addr = 0;      /* 0 means "next instruction" */
    int explicit_addr = 0;

    /* Parse optional arguments */
    if (dir->arg_count >= 1) {
        ExprResult r = expr_eval(dir->args[0], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
        if (!r.defined && as->pass == 2) {
            assembler_error(as, "!basic line number must be constant");
            return -1;
        }
        line_number = r.value;
    }

    if (dir->arg_count >= 2) {
        ExprResult r = expr_eval(dir->args[1], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
        if (!r.defined && as->pass == 2) {
            assembler_error(as, "!basic SYS address must be constant");
            return -1;
        }
        sys_addr = r.value;
        explicit_addr = 1;
    }

    /* Calculate the SYS address if not explicitly provided */
    /* We need to know how many bytes we'll emit to know where code starts */
    /* Format: link(2) + linenum(2) + SYS(1) + digits(4-5) + null(1) + endmarker(2) */
    /* For addresses < 10000: 4 digits, >= 10000: 5 digits */

    int start_pc = as->pc;

    if (!explicit_addr) {
        /* Estimate: assume 5-digit address (worst case) for pass 1 */
        /* link(2) + linenum(2) + SYS(1) + digits(5) + null(1) + end(2) = 13 bytes */
        /* But we calculate precisely based on resulting address */
        int base_size = 2 + 2 + 1 + 1 + 2;  /* link + linenum + SYS + null + end */

        /* The SYS address is start_pc + total_size */
        /* total_size = base_size + digit_count */
        /* digit_count depends on (start_pc + total_size) */
        /* So we iterate to find the correct size */

        int digit_count;
        int addr;
        for (digit_count = 4; digit_count <= 5; digit_count++) {
            addr = start_pc + base_size + digit_count;
            int needed = (addr >= 10000) ? 5 : 4;
            if (needed == digit_count) break;
        }
        sys_addr = addr;
    }

    /* Calculate digit count and link address */
    int digit_count = (sys_addr >= 10000) ? 5 : 4;
    int total_size = 2 + 2 + 1 + digit_count + 1 + 2;  /* 12 or 13 bytes */
    int link_addr = start_pc + total_size - 2;  /* Points to the $00 $00 end marker */

    if (as->pass == 2) {
        /* Emit link pointer (little-endian) */
        assembler_emit_byte(as, link_addr & 0xFF);
        assembler_emit_byte(as, (link_addr >> 8) & 0xFF);

        /* Emit line number (little-endian) */
        assembler_emit_byte(as, line_number & 0xFF);
        assembler_emit_byte(as, (line_number >> 8) & 0xFF);

        /* Emit SYS token */
        assembler_emit_byte(as, 0x9E);

        /* Emit address as ASCII digits */
        char digits[6];
        snprintf(digits, sizeof(digits), "%d", sys_addr);
        for (int i = 0; digits[i]; i++) {
            assembler_emit_byte(as, digits[i]);
        }

        /* Emit end of line */
        assembler_emit_byte(as, 0x00);

        /* Emit end of program marker */
        assembler_emit_byte(as, 0x00);
        assembler_emit_byte(as, 0x00);
    } else {
        /* Pass 1: just advance PC */
        assembler_advance_pc(as, total_size);
    }

    return 0;
}

int assembler_assemble_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;
    const char *name = dir->name;

    /* Byte data */
    if (strcmp(name, "byte") == 0 || strcmp(name, "by") == 0 ||
        strcmp(name, "db") == 0 || strcmp(name, "08") == 0) {
        return assemble_byte_directive(as, stmt);
    }
    /* Word data */
    if (strcmp(name, "word") == 0 || strcmp(name, "wo") == 0 ||
        strcmp(name, "dw") == 0 || strcmp(name, "16") == 0) {
        return assemble_word_directive(as, stmt);
    }
    /* Text/string data */
    if (strcmp(name, "text") == 0 || strcmp(name, "tx") == 0) {
        return assemble_text_directive(as, stmt);
    }
    /* PETSCII string */
    if (strcmp(name, "pet") == 0) {
        return assemble_pet_directive(as, stmt);
    }
    /* Screen codes */
    if (strcmp(name, "scr") == 0) {
        return assemble_scr_directive(as, stmt);
    }
    /* Null-terminated string */
    if (strcmp(name, "null") == 0) {
        return assemble_null_directive(as, stmt);
    }
    /* Fill bytes */
    if (strcmp(name, "fill") == 0 || strcmp(name, "fi") == 0) {
        return assemble_fill_directive(as, stmt);
    }
    /* Skip/reserve bytes */
    if (strcmp(name, "skip") == 0 || strcmp(name, "res") == 0) {
        return assemble_skip_directive(as, stmt);
    }
    /* Alignment */
    if (strcmp(name, "align") == 0) {
        return assemble_align_directive(as, stmt);
    }
    /* Origin */
    if (strcmp(name, "org") == 0) {
        return assemble_org_directive(as, stmt);
    }
    /* Binary file include */
    if (strcmp(name, "binary") == 0 || strcmp(name, "bin") == 0) {
        return assemble_binary_directive(as, stmt);
    }
    /* BASIC stub generator */
    if (strcmp(name, "basic") == 0) {
        return assemble_basic_directive(as, stmt);
    }
    /* Source include - placeholder, actual processing in pass1 */
    if (strcmp(name, "source") == 0 || strcmp(name, "src") == 0 ||
        strcmp(name, "include") == 0) {
        /* Source inclusion happens during parsing/pass1 */
        return 0;
    }
    /* Macro-related directives handled in pass1 */
    if (strcmp(name, "macro") == 0 || strcmp(name, "endmacro") == 0 ||
        strcmp(name, "endm") == 0) {
        return 0;
    }
    /* Loop-related directives handled in pass1 */
    if (strcmp(name, "for") == 0 || strcmp(name, "while") == 0 ||
        strcmp(name, "end") == 0) {
        return 0;
    }

    /* Pseudo-PC: assemble as if at different address */
    if (strcmp(name, "pseudopc") == 0) {
        if (dir->arg_count < 1 || !dir->args[0]) {
            assembler_error(as, "!pseudopc requires an address");
            return -1;
        }
        ExprResult result = expr_eval(dir->args[0], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
        if (!result.defined) {
            assembler_error(as, "!pseudopc address must be a defined value");
            return -1;
        }
        return assembler_pseudopc_start(as, (uint16_t)result.value);
    }

    /* End pseudo-PC mode */
    if (strcmp(name, "realpc") == 0) {
        return assembler_pseudopc_end(as);
    }

    /* CPU selection - accepts string "6502", "6510", "65c02" or number */
    if (strcmp(name, "cpu") == 0) {
        const char *cpu_name = NULL;

        /* Check for string argument first */
        if (dir->string_arg) {
            cpu_name = dir->string_arg;
        } else if (dir->arg_count >= 1 && dir->args[0]) {
            /* Try to get from expression */
            Expr *arg = dir->args[0];
            if (arg->type == EXPR_SYMBOL) {
                cpu_name = arg->data.symbol;
            } else if (arg->type == EXPR_NUMBER) {
                /* Handle numeric cpu types like 6502, 6510 */
                static char num_buf[16];
                snprintf(num_buf, sizeof(num_buf), "%d", (int)arg->data.number);
                cpu_name = num_buf;
            }
        }

        if (!cpu_name) {
            assembler_error(as, "!cpu requires a CPU type (6502, 6510, or 65c02)");
            return -1;
        }
        return assembler_set_cpu(as, cpu_name);
    }

    /* Zone directive - sets current zone for local labels */
    if (strcmp(name, "zone") == 0 || strcmp(name, "zn") == 0) {
        const char *zone_name = NULL;

        /* Check for string argument first */
        if (dir->string_arg) {
            zone_name = dir->string_arg;
        } else if (dir->arg_count >= 1 && dir->args[0]) {
            /* Try to get from symbol expression */
            Expr *arg = dir->args[0];
            if (arg->type == EXPR_SYMBOL) {
                zone_name = arg->data.symbol;
            }
        }

        /* Update current zone */
        free(as->current_zone);
        if (zone_name && zone_name[0] != '\0') {
            as->current_zone = strdup(zone_name);
        } else {
            /* Anonymous zone - use unique name */
            static int anon_zone_counter = 0;
            char buf[32];
            snprintf(buf, sizeof(buf), "_zone_%d", ++anon_zone_counter);
            as->current_zone = strdup(buf);
        }
        return 0;
    }

    /* Error/warning messages */
    if (strcmp(name, "error") == 0) {
        /* Get message from string argument */
        if (dir->string_arg) {
            assembler_error(as, "%s", dir->string_arg);
        } else {
            assembler_error(as, "user error");
        }
        return -1;
    }

    if (strcmp(name, "warn") == 0 || strcmp(name, "warning") == 0) {
        if (dir->string_arg) {
            assembler_warning(as, "%s", dir->string_arg);
        } else {
            assembler_warning(as, "user warning");
        }
        return 0;
    }

    /* Unknown directive */
    assembler_warning(as, "unknown directive !%s ignored", name);
    return 0;
}

/* ========== Assignment Assembly ========== */

static int assemble_assignment(Assembler *as, Statement *stmt) {
    AssignmentInfo *assign = &stmt->data.assignment;

    ExprResult result = expr_eval(assign->value, as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);

    /* Determine flags for the symbol definition:
     * - In pass 1 outside loops: SYM_CONSTANT (traditional behavior)
     * - In pass 1 inside loops: SYM_DEFINED | SYM_FORCE_UPDATE (allows reassignment)
     * - In pass 2: SYM_DEFINED | SYM_FORCE_UPDATE (replaying, may need to update) */
    uint8_t flags;
    if (as->pass == 2 || assembler_in_loop(as)) {
        flags = SYM_DEFINED | SYM_FORCE_UPDATE;
    } else {
        flags = SYM_CONSTANT;
    }
    if (result.defined && assembler_is_zeropage(result.value)) {
        flags |= SYM_ZEROPAGE;
    }

    symbol_define(as->symbols, assign->name, result.value, flags,
                 as->current_file, as->current_line);

    return 0;
}

/* ========== Statement Assembly ========== */

int assembler_assemble_statement(Assembler *as, Statement *stmt) {
    as->current_line = stmt->line;

    /* Handle label first */
    if (stmt->label) {
        if (as->pass == 1) {
            /* Define symbols in pass 1 */
            define_label(as, stmt->label);
        } else {
            /* In pass 2, still need to track zone for local label resolution */
            if (!stmt->label->is_local && !stmt->label->is_anon_fwd && !stmt->label->is_anon_back) {
                /* Global label - update current zone */
                free(as->current_zone);
                as->current_zone = strdup(stmt->label->name);
            }
        }
    }

    /* Process statement based on type */
    switch (stmt->type) {
        case STMT_EMPTY:
        case STMT_LABEL:
            /* Nothing to assemble */
            return 0;

        case STMT_INSTRUCTION:
            return assembler_assemble_instruction(as, stmt);

        case STMT_DIRECTIVE:
            return assembler_assemble_directive(as, stmt);

        case STMT_ASSIGNMENT:
            return assemble_assignment(as, stmt);

        case STMT_MACRO_CALL:
            /* Macros not yet implemented */
            assembler_error(as, "macros not yet implemented");
            return -1;

        case STMT_ERROR:
            assembler_error(as, "%s", stmt->error_msg ? stmt->error_msg : "parse error");
            return -1;
    }

    return 0;
}

/* ========== Pass 1 Implementation ========== */

/* Check if statement is a source include directive */
static int is_source_directive(Statement *stmt) {
    if (stmt->type != STMT_DIRECTIVE) return 0;
    const char *name = stmt->data.directive.name;
    return strcmp(name, "source") == 0 ||
           strcmp(name, "src") == 0 ||
           strcmp(name, "include") == 0;
}

/* Check if statement is a conditional directive */
static int is_conditional_directive(Statement *stmt) {
    if (stmt->type != STMT_DIRECTIVE) return 0;
    const char *name = stmt->data.directive.name;
    return strcmp(name, "if") == 0 ||
           strcmp(name, "else") == 0 ||
           strcmp(name, "endif") == 0 ||
           strcmp(name, "ifdef") == 0 ||
           strcmp(name, "ifndef") == 0;
}

/* Check if statement is a macro definition directive */
static int is_macro_directive(Statement *stmt) {
    if (stmt->type != STMT_DIRECTIVE) return 0;
    const char *name = stmt->data.directive.name;
    return strcmp(name, "macro") == 0;
}

/* Check if statement is a loop directive */
static int is_loop_directive(Statement *stmt) {
    if (stmt->type != STMT_DIRECTIVE) return 0;
    const char *name = stmt->data.directive.name;
    return strcmp(name, "for") == 0 || strcmp(name, "while") == 0;
}

/* Find the start of the current line in source */
static const char *find_line_start(const char *source, const char *pos) {
    while (pos > source && pos[-1] != '\n') {
        pos--;
    }
    return pos;
}

/* Find the end of the line (including the newline) */
static const char *find_line_end(const char *pos) {
    while (*pos && *pos != '\n') {
        pos++;
    }
    if (*pos == '\n') pos++;  /* Include the newline */
    return pos;
}

/* Collect macro body until matching } or !endmacro */
static char *collect_macro_body(Lexer *lexer, Parser *parser) {
    size_t capacity = 1024;
    size_t length = 0;
    char *body = malloc(capacity);
    if (!body) return NULL;
    body[0] = '\0';

    int depth = 1; /* We're inside the macro */

    while (*lexer->current || parser->current.type != TOK_EOF) {
        /* Find line start from current parser token position */
        const char *line_start = find_line_start(lexer->source, parser->current.start);
        /* Find line end from line start */
        const char *line_end = find_line_end(line_start);

        /* Parse next statement */
        Statement *stmt = parser_parse_line(parser);
        if (!stmt) break;

        /* Check for nested macro start or end */
        if (stmt->type == STMT_DIRECTIVE) {
            const char *dir_name = stmt->data.directive.name;
            if (strcmp(dir_name, "macro") == 0) {
                depth++;
            } else if (strcmp(dir_name, "endmacro") == 0 || strcmp(dir_name, "endm") == 0) {
                depth--;
                if (depth == 0) {
                    statement_free(stmt);
                    break;
                }
            }
        }

        /* Append this line to body */
        size_t line_len = line_end - line_start;
        while (length + line_len + 2 >= capacity) {
            capacity *= 2;
            char *new_body = realloc(body, capacity);
            if (!new_body) {
                free(body);
                statement_free(stmt);
                return NULL;
            }
            body = new_body;
        }

        /* Copy the line */
        memcpy(body + length, line_start, line_len);
        length += line_len;
        body[length] = '\0';

        statement_free(stmt);

        /* Check for EOF */
        if (parser->current.type == TOK_EOF) break;
    }

    if (depth != 0) {
        /* Unterminated macro */
        free(body);
        return NULL;
    }

    return body;
}

/* Process !macro directive */
static int process_macro_directive(Assembler *as, Statement *stmt, Lexer *lexer, Parser *parser) {
    DirectiveInfo *dir = &stmt->data.directive;

    /* Get macro name - it should be in string_arg or first expression */
    const char *macro_name = NULL;
    if (dir->string_arg) {
        macro_name = dir->string_arg;
    } else if (dir->arg_count > 0 && dir->args[0] && dir->args[0]->type == EXPR_SYMBOL) {
        macro_name = dir->args[0]->data.symbol;
    }

    if (!macro_name) {
        assembler_error(as, "!macro requires a name");
        return -1;
    }

    /* Get parameters (remaining arguments if they are symbol expressions) */
    char *params[ASM_MAX_MACRO_ARGS];
    int param_count = 0;

    /* Parameters start from arg index 1 if name was in args[0], otherwise from 0 if name was string_arg */
    int start_idx = dir->string_arg ? 0 : 1;
    for (int i = start_idx; i < dir->arg_count && param_count < ASM_MAX_MACRO_ARGS; i++) {
        if (dir->args[i] && dir->args[i]->type == EXPR_SYMBOL) {
            params[param_count++] = (char *)dir->args[i]->data.symbol;
        }
    }

    /* Collect the macro body */
    char *body = collect_macro_body(lexer, parser);
    if (!body) {
        assembler_error(as, "unterminated macro '%s'", macro_name);
        return -1;
    }

    /* Define the macro */
    int result = macro_define(as, macro_name, params, param_count, body,
                              as->current_file, as->current_line);
    free(body);

    return result;
}

/* Collect loop body until matching } or !end */
static char *collect_loop_body(Lexer *lexer, Parser *parser) {
    size_t capacity = 1024;
    size_t length = 0;
    char *body = malloc(capacity);
    if (!body) return NULL;
    body[0] = '\0';

    int depth = 1; /* We're inside the loop */

    while (*lexer->current || parser->current.type != TOK_EOF) {
        /* Find line start from current parser token position */
        const char *line_start = find_line_start(lexer->source, parser->current.start);
        /* Find line end from line start */
        const char *line_end = find_line_end(line_start);

        /* Parse next statement */
        Statement *stmt = parser_parse_line(parser);
        if (!stmt) break;

        /* Check for nested loop start or end */
        if (stmt->type == STMT_DIRECTIVE) {
            const char *dir_name = stmt->data.directive.name;
            if (strcmp(dir_name, "for") == 0 || strcmp(dir_name, "while") == 0) {
                depth++;
            } else if (strcmp(dir_name, "end") == 0) {
                depth--;
                if (depth == 0) {
                    statement_free(stmt);
                    break;
                }
            }
        }

        /* Append this line to body */
        size_t line_len = line_end - line_start;
        while (length + line_len + 2 >= capacity) {
            capacity *= 2;
            char *new_body = realloc(body, capacity);
            if (!new_body) {
                free(body);
                statement_free(stmt);
                return NULL;
            }
            body = new_body;
        }

        /* Copy the line */
        memcpy(body + length, line_start, line_len);
        length += line_len;
        body[length] = '\0';

        statement_free(stmt);

        /* Check for EOF */
        if (parser->current.type == TOK_EOF) break;
    }

    if (depth != 0) {
        /* Unterminated loop */
        free(body);
        return NULL;
    }

    return body;
}

/* Process !for or !while directive */
static int process_loop_directive(Assembler *as, Statement *stmt, Lexer *lexer, Parser *parser) {
    DirectiveInfo *dir = &stmt->data.directive;
    const char *name = dir->name;

    if (strcmp(name, "for") == 0) {
        /* !for var, start, end - need at least 3 arguments */
        /* Format: !for i, 0, 9 { } or !for i, 0, 9 ... !end */
        if (dir->arg_count < 3) {
            assembler_error(as, "!for requires variable, start, and end");
            return -1;
        }

        /* Get variable name */
        const char *var_name = NULL;
        if (dir->args[0] && dir->args[0]->type == EXPR_SYMBOL) {
            var_name = dir->args[0]->data.symbol;
        }
        if (!var_name) {
            assembler_error(as, "!for requires a variable name");
            return -1;
        }

        /* Evaluate start and end */
        ExprResult start_result = expr_eval(dir->args[1], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
        ExprResult end_result = expr_eval(dir->args[2], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);

        if (!start_result.defined || !end_result.defined) {
            assembler_error(as, "!for start and end must be defined values");
            return -1;
        }

        int32_t start = start_result.value;
        int32_t end = end_result.value;

        /* Collect loop body */
        char *body = collect_loop_body(lexer, parser);
        if (!body) {
            assembler_error(as, "unterminated !for loop");
            return -1;
        }

        /* Execute the loop */
        int result = assembler_loop_for(as, var_name, start, end, body);
        free(body);
        return result;
    }
    else if (strcmp(name, "while") == 0) {
        /* !while expr { } or !while expr ... !end */
        if (dir->arg_count < 1 || !dir->args[0]) {
            assembler_error(as, "!while requires a condition expression");
            return -1;
        }

        /* Collect loop body */
        char *body = collect_loop_body(lexer, parser);
        if (!body) {
            assembler_error(as, "unterminated !while loop");
            return -1;
        }

        /* Execute the loop */
        int result = assembler_loop_while(as, dir->args[0], body);
        free(body);
        return result;
    }

    return 0;
}

/* Process a conditional directive - returns 0 on success, -1 on error */
static int process_conditional_directive(Assembler *as, Statement *stmt) {
    DirectiveInfo *dir = &stmt->data.directive;
    const char *name = dir->name;

    if (strcmp(name, "if") == 0) {
        /* !if expr - evaluate expression */
        if (dir->arg_count < 1 || !dir->args[0]) {
            assembler_error(as, "!if requires a condition expression");
            return -1;
        }
        ExprResult result = expr_eval(dir->args[0], as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
        /* In pass 1, treat undefined as false for forward refs */
        int condition = result.defined ? result.value : 0;
        return assembler_cond_if(as, condition);
    }
    else if (strcmp(name, "ifdef") == 0) {
        /* !ifdef symbol - check if symbol is defined */
        if (!dir->string_arg && (dir->arg_count < 1 || !dir->args[0])) {
            assembler_error(as, "!ifdef requires a symbol name");
            return -1;
        }
        /* Get symbol name from string arg or first expression (symbol ref) */
        const char *sym_name = NULL;
        if (dir->string_arg) {
            sym_name = dir->string_arg;
        } else if (dir->args[0] && dir->args[0]->type == EXPR_SYMBOL) {
            sym_name = dir->args[0]->data.symbol;
        }
        if (!sym_name) {
            assembler_error(as, "!ifdef requires a symbol name");
            return -1;
        }
        return assembler_cond_ifdef(as, sym_name);
    }
    else if (strcmp(name, "ifndef") == 0) {
        /* !ifndef symbol - check if symbol is NOT defined */
        if (!dir->string_arg && (dir->arg_count < 1 || !dir->args[0])) {
            assembler_error(as, "!ifndef requires a symbol name");
            return -1;
        }
        const char *sym_name = NULL;
        if (dir->string_arg) {
            sym_name = dir->string_arg;
        } else if (dir->args[0] && dir->args[0]->type == EXPR_SYMBOL) {
            sym_name = dir->args[0]->data.symbol;
        }
        if (!sym_name) {
            assembler_error(as, "!ifndef requires a symbol name");
            return -1;
        }
        return assembler_cond_ifndef(as, sym_name);
    }
    else if (strcmp(name, "else") == 0) {
        return assembler_cond_else(as);
    }
    else if (strcmp(name, "endif") == 0) {
        return assembler_cond_endif(as);
    }

    return 0;
}

/* Internal recursive pass1 implementation */
static int assembler_pass1_internal(Assembler *as, const char *source, const char *filename);

int assembler_include_file(Assembler *as, const char *filename) {
    /* Check include depth */
    if (as->include_depth >= ASM_MAX_INCLUDE_DEPTH) {
        assembler_error(as, "include nesting too deep (max %d)", ASM_MAX_INCLUDE_DEPTH);
        return -1;
    }

    /* Find the file */
    char *path = assembler_find_include(as, filename);
    if (!path) {
        assembler_error(as, "cannot find include file: %s", filename);
        return -1;
    }

    /* Read file content */
    long size;
    char *content = read_file_content(path, &size);
    if (!content) {
        assembler_error(as, "cannot read include file: %s", path);
        free(path);
        return -1;
    }

    /* Push onto include stack */
    IncludeEntry *entry = &as->include_stack[as->include_depth];
    entry->filename = strdup(as->current_file ? as->current_file : "<input>");
    entry->line_number = as->current_line;
    entry->source = NULL; /* Will be freed by caller */
    as->include_depth++;

    /* Save current state */
    const char *saved_file = as->current_file;

    /* Process included file */
    int result = assembler_pass1_internal(as, content, path);

    /* Restore state */
    as->current_file = saved_file;

    /* Pop include stack */
    as->include_depth--;
    free(as->include_stack[as->include_depth].filename);
    as->include_stack[as->include_depth].filename = NULL;

    free(content);
    free(path);

    return result;
}

static int assembler_pass1_internal(Assembler *as, const char *source, const char *filename) {
    as->pass = 1;
    as->current_file = filename;

    Lexer lexer;
    Parser parser;

    lexer_init(&lexer, source, filename);
    parser_init(&parser, &lexer, as->symbols);
    parser_set_pc(&parser, as->pc);
    parser_set_pass(&parser, 1);

    /* Parse and process all lines */
    while (1) {
        uint16_t line_pc = as->pc;

        Statement *stmt = parser_parse_line(&parser);
        if (!stmt) {
            break;
        }

        /* Capture source line for listings by finding it in the source buffer */
        char *source_line = NULL;
        if (stmt->line > 0) {
            /* Find the line in source */
            const char *p = source;
            int current_line = 1;
            while (*p && current_line < stmt->line) {
                if (*p == '\n') current_line++;
                p++;
            }
            /* p now points to start of the line */
            const char *line_start = p;
            while (*line_start == ' ' || *line_start == '\t') line_start++;
            const char *line_end = line_start;
            while (*line_end && *line_end != '\n') line_end++;
            int line_len = (int)(line_end - line_start);
            /* Strip trailing whitespace */
            while (line_len > 0 && (line_start[line_len-1] == ' ' ||
                   line_start[line_len-1] == '\t' || line_start[line_len-1] == '\r')) {
                line_len--;
            }
            if (line_len > 0) {
                source_line = malloc(line_len + 1);
                if (source_line) {
                    memcpy(source_line, line_start, line_len);
                    source_line[line_len] = '\0';
                }
            }
        }

        /* Update parser PC for expression evaluation */
        parser_set_pc(&parser, as->pc);
        as->current_line = stmt->line;

        /* Conditional directives are ALWAYS processed, even in inactive blocks */
        if (is_conditional_directive(stmt)) {
            process_conditional_directive(as, stmt);
            statement_free(stmt);
            free(source_line);
            continue;
        }

        /* Skip all other statements if in an inactive conditional block */
        if (!assembler_is_active(as)) {
            int is_eof = (stmt->type == STMT_EMPTY && *lexer.current == '\0');
            statement_free(stmt);
            free(source_line);
            if (is_eof) break;
            continue;
        }

        /* Check for source include directive - process immediately */
        if (is_source_directive(stmt)) {
            DirectiveInfo *dir = &stmt->data.directive;
            if (!dir->string_arg) {
                assembler_error(as, "!source requires a filename argument");
            } else {
                assembler_include_file(as, dir->string_arg);
            }
            statement_free(stmt);
            free(source_line);
            continue;
        }

        /* Check for macro definition - process immediately */
        if (is_macro_directive(stmt)) {
            process_macro_directive(as, stmt, &lexer, &parser);
            statement_free(stmt);
            free(source_line);
            continue;
        }

        /* Check for loop directive - process immediately */
        if (is_loop_directive(stmt)) {
            process_loop_directive(as, stmt, &lexer, &parser);
            statement_free(stmt);
            free(source_line);
            continue;
        }

        /* Check for macro invocation */
        if (stmt->type == STMT_MACRO_CALL) {
            MacroCallInfo *call = &stmt->data.macro_call;
            macro_expand(as, call->name, call->args, call->arg_count);
            statement_free(stmt);
            free(source_line);
            continue;
        }

        /* Store for pass 2 - transfers ownership of source_line */
        int line_idx = add_assembled_line(as, stmt, line_pc, source_line);
        free(source_line);  /* add_assembled_line makes a copy */
        if (line_idx < 0) {
            return -1;
        }

        /* Assemble statement (for size determination) */
        if (assembler_assemble_statement(as, stmt) < 0) {
            /* Error already reported */
        }

        /* Check for EOF */
        if (stmt->type == STMT_EMPTY && *lexer.current == '\0') {
            break;
        }

        /* Stop on too many errors */
        if (as->errors >= ASM_MAX_ERRORS) {
            assembler_error(as, "too many errors, stopping");
            break;
        }
    }

    /* Check for unclosed conditionals */
    if (as->cond_depth > 0) {
        CondEntry *entry = &as->cond_stack[as->cond_depth - 1];
        assembler_error(as, "unterminated !if (started at %s:%d)",
                       entry->filename ? entry->filename : "<input>",
                       entry->line_number);
    }

    return as->errors > 0 ? -1 : 0;
}

int assembler_pass1(Assembler *as, const char *source, const char *filename) {
    as->pc = as->org;
    return assembler_pass1_internal(as, source, filename);
}

/* ========== Pass 2 Implementation ========== */

int assembler_pass2(Assembler *as) {
    as->pass = 2;
    as->pc = as->org;
    as->real_pc = as->org;
    as->in_pseudopc = 0;

    /* Reset zone tracking for pass 2 */
    free(as->current_zone);
    as->current_zone = NULL;

    /* Reset macro unique counter so IDs match between passes */
    as->macro_unique_counter = 0;

    /* Reset anonymous label tracking for pass 2 */
    anon_reset_pass(as->anon_labels);

    /* Re-process all stored statements */
    for (int i = 0; i < as->line_count; i++) {
        AssembledLine *line = &as->lines[i];
        Statement *stmt = line->stmt;

        as->current_line = stmt->line;
        /* Restore PC to stored address for symbol resolution
         * The real_pc tracks actual output position separately when in pseudopc */
        as->pc = line->address;
        uint16_t start_pc = as->in_pseudopc ? as->real_pc : as->pc;

        /* Restore zone for local label resolution */
        free(as->current_zone);
        as->current_zone = line->zone ? str_dup(line->zone) : NULL;

        /* Re-define labels for anonymous label tracking */
        if (stmt->label) {
            if (stmt->label->is_anon_fwd) {
                anon_define_forward(as->anon_labels, as->pc, as->current_file, stmt->line);
            } else if (stmt->label->is_anon_back) {
                anon_define_backward(as->anon_labels, as->pc, as->current_file, stmt->line);
            }
        }

        if (assembler_assemble_statement(as, stmt) < 0) {
            /* Error already reported */
        }

        /* Capture generated bytes for listing */
        uint16_t end_pc = as->in_pseudopc ? as->real_pc : as->pc;
        int byte_count = end_pc - start_pc;
        if (byte_count > 0 && byte_count <= 8) {
            line->byte_count = byte_count;
            for (int j = 0; j < byte_count; j++) {
                line->bytes[j] = as->memory[start_pc + j];
            }
        } else if (byte_count > 8) {
            /* For long data, just show first 8 bytes */
            line->byte_count = 8;
            for (int j = 0; j < 8; j++) {
                line->bytes[j] = as->memory[start_pc + j];
            }
        }

        /* Update cycle info from instruction */
        if (stmt->type == STMT_INSTRUCTION) {
            line->cycles = stmt->data.instruction.cycles;
            line->page_penalty = stmt->data.instruction.page_penalty;
        }

        if (as->errors >= ASM_MAX_ERRORS) {
            break;
        }
    }

    return as->errors > 0 ? -1 : 0;
}

/* ========== Main Assembly Function ========== */

int assembler_assemble_string(Assembler *as, const char *source, const char *filename) {
    assembler_reset(as);

    /* Pass 1: Symbol collection and size determination */
    if (as->verbose) {
        fprintf(stderr, "Pass 1: Parsing and symbol collection...\n");
    }
    if (assembler_pass1(as, source, filename) < 0) {
        if (as->verbose) {
            fprintf(stderr, "Pass 1 completed with %d error(s)\n", as->errors);
        }
    } else if (as->verbose) {
        fprintf(stderr, "Pass 1: %d lines, %d symbols defined\n",
                as->line_count, symbol_count(as->symbols));
    }

    if (as->errors > 0) {
        return as->errors;
    }

    /* Pass 2: Code generation */
    if (as->verbose) {
        fprintf(stderr, "Pass 2: Code generation...\n");
    }
    if (assembler_pass2(as) < 0) {
        if (as->verbose) {
            fprintf(stderr, "Pass 2 completed with %d error(s)\n", as->errors);
        }
    } else if (as->verbose) {
        int code_size = (as->highest_addr >= as->lowest_addr) ?
                        (as->highest_addr - as->lowest_addr + 1) : 0;
        fprintf(stderr, "Pass 2: Generated %d bytes ($%04X-$%04X)\n",
                code_size, as->lowest_addr, as->highest_addr);
    }

    return as->errors;
}

int assembler_assemble_file(Assembler *as, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        assembler_error(as, "cannot open file: %s", filename);
        return -1;
    }

    /* Get file size */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* Read entire file */
    char *source = malloc(size + 1);
    if (!source) {
        fclose(f);
        assembler_error(as, "out of memory reading file");
        return -1;
    }

    size_t read = fread(source, 1, size, f);
    fclose(f);
    source[read] = '\0';

    int result = assembler_assemble_string(as, source, filename);

    free(source);
    return result;
}

/* ========== Output Functions ========== */

const uint8_t *assembler_get_output(Assembler *as, uint16_t *start_addr, int *size) {
    if (as->lowest_addr > as->highest_addr) {
        /* No output */
        *start_addr = as->org;
        *size = 0;
        return as->memory;
    }

    *start_addr = as->lowest_addr;
    *size = as->highest_addr - as->lowest_addr + 1;
    return &as->memory[as->lowest_addr];
}

int assembler_write_output(Assembler *as, const char *filename) {
    if (as->lowest_addr > as->highest_addr) {
        assembler_warning(as, "no output generated");
        return 0;
    }

    FILE *f = fopen(filename, "wb");
    if (!f) {
        assembler_error(as, "cannot create output file: %s", filename);
        return -1;
    }

    /* Write load address header for PRG format */
    if (as->format == OUTPUT_PRG) {
        uint8_t header[2];
        header[0] = as->lowest_addr & 0xFF;
        header[1] = (as->lowest_addr >> 8) & 0xFF;
        fwrite(header, 1, 2, f);
    }

    /* Write code */
    int size = as->highest_addr - as->lowest_addr + 1;
    fwrite(&as->memory[as->lowest_addr], 1, size, f);

    fclose(f);
    return 0;
}

int assembler_write_symbols(Assembler *as, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        assembler_error(as, "cannot create symbol file: %s", filename);
        return -1;
    }
    int result = symbol_write_vice(as->symbols, fp);
    fclose(fp);
    return result;
}

int assembler_write_listing(Assembler *as, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        assembler_error(as, "cannot create listing file: %s", filename);
        return -1;
    }

    /* Write header */
    fprintf(fp, "; ASM64 Listing File\n");
    fprintf(fp, "; Generated from assembled source\n");
    fprintf(fp, ";\n");
    if (as->show_cycles) {
        fprintf(fp, "; Address  Bytes         Cycles  Source\n");
        fprintf(fp, "; -------  ----------    ------  ------\n");
    } else {
        fprintf(fp, "; Address  Bytes         Source\n");
        fprintf(fp, "; -------  ----------    ------\n");
    }
    fprintf(fp, "\n");

    /* Write each assembled line */
    for (int i = 0; i < as->line_count; i++) {
        AssembledLine *line = &as->lines[i];
        Statement *stmt = line->stmt;

        /* Skip empty statements without source */
        if (stmt && stmt->type == STMT_EMPTY && !line->source_text) {
            continue;
        }

        /* Check if this is an ORG directive (doesn't generate bytes) */
        int is_org = 0;
        if (line->source_text && line->source_text[0] == '*' &&
            strchr(line->source_text, '=')) {
            is_org = 1;
        }

        /* Format address - show new address for ORG, otherwise current address */
        if (line->byte_count > 0 && !is_org) {
            fprintf(fp, "%04X  ", line->address);
        } else if (stmt && stmt->type == STMT_LABEL) {
            fprintf(fp, "%04X  ", line->address);
        } else if (stmt && stmt->type == STMT_DIRECTIVE) {
            /* Don't show address for assignments and some directives */
            const char *name = stmt->data.directive.name;
            if (strcmp(name, "=") != 0) {
                fprintf(fp, "      ");
            } else {
                fprintf(fp, "      ");
            }
        } else {
            fprintf(fp, "      ");
        }

        /* Format hex bytes (up to 4 bytes on the line, more if needed) */
        /* Skip bytes display for ORG directives */
        char hex_buf[32] = "";
        int hex_len = 0;
        if (!is_org) {
            int bytes_shown = line->byte_count > 4 ? 4 : line->byte_count;
            for (int j = 0; j < bytes_shown; j++) {
                hex_len += snprintf(hex_buf + hex_len, sizeof(hex_buf) - hex_len,
                                   "%02X ", line->bytes[j]);
            }
        }
        /* Pad to fixed width */
        fprintf(fp, "%-12s", hex_buf);

        /* Format cycle count if enabled */
        if (as->show_cycles) {
            if (line->cycles > 0) {
                if (line->page_penalty) {
                    fprintf(fp, "  %2d+   ", line->cycles);
                } else {
                    fprintf(fp, "  %2d    ", line->cycles);
                }
            } else {
                fprintf(fp, "        ");
            }
        }

        /* Source text */
        if (line->source_text) {
            fprintf(fp, "  %s", line->source_text);
        } else if (stmt && stmt->type == STMT_INSTRUCTION) {
            /* Generate instruction text if source not available */
            fprintf(fp, "  %s", stmt->data.instruction.mnemonic);
        }

        fprintf(fp, "\n");

        /* If more than 4 bytes, continue on next lines (skip for ORG) */
        if (line->byte_count > 4 && !is_org) {
            int pos = 4;
            while (pos < line->byte_count) {
                fprintf(fp, "%04X  ", line->address + pos);
                hex_buf[0] = '\0';
                hex_len = 0;
                int count = (line->byte_count - pos) > 4 ? 4 : (line->byte_count - pos);
                for (int j = 0; j < count; j++) {
                    hex_len += snprintf(hex_buf + hex_len, sizeof(hex_buf) - hex_len,
                                       "%02X ", line->bytes[pos + j]);
                }
                fprintf(fp, "%-12s", hex_buf);
                if (as->show_cycles) {
                    fprintf(fp, "        ");
                }
                fprintf(fp, "\n");
                pos += count;
            }
        }
    }

    /* Write symbol table summary */
    fprintf(fp, "\n; Symbol Table\n");
    fprintf(fp, "; ------------\n");
    symbol_write_vice(as->symbols, fp);

    fclose(fp);
    return 0;
}

/* ========== Include Path Functions ========== */

int assembler_add_include_path(Assembler *as, const char *path) {
    if (as->include_path_count >= ASM_MAX_INCLUDE_PATHS) {
        return -1;
    }
    as->include_paths[as->include_path_count] = strdup(path);
    if (!as->include_paths[as->include_path_count]) {
        return -1;
    }
    as->include_path_count++;
    return 0;
}

void assembler_add_include_paths_from_env(Assembler *as, const char *env_var) {
    const char *env_value = getenv(env_var);
    if (!env_value || !*env_value) return;

    /* Make a copy since we'll modify it */
    char *paths = strdup(env_value);
    if (!paths) return;

    /* Split by colon (Unix) or semicolon (Windows) */
    char *path = paths;
    char *sep;

    /* Try colon first (Unix), then semicolon (Windows) */
    char delimiter = ':';
#ifdef _WIN32
    delimiter = ';';
#endif

    while (path && *path) {
        sep = strchr(path, delimiter);
        if (sep) *sep = '\0';

        if (*path) {
            assembler_add_include_path(as, path);
        }

        if (sep) {
            path = sep + 1;
        } else {
            break;
        }
    }

    free(paths);
}

int assembler_define_symbol(Assembler *as, const char *definition) {
    if (!definition || !*definition) return -1;

    /* Make a copy to parse */
    char *def = strdup(definition);
    if (!def) return -1;

    /* Find '=' if present */
    char *eq = strchr(def, '=');
    int32_t value = 1;  /* Default value if no '=' */

    if (eq) {
        *eq = '\0';
        const char *val_str = eq + 1;

        /* Parse the value - support hex ($XX), binary (%XXXX), decimal */
        if (val_str[0] == '$') {
            value = (int32_t)strtol(val_str + 1, NULL, 16);
        } else if (val_str[0] == '%') {
            value = (int32_t)strtol(val_str + 1, NULL, 2);
        } else if (val_str[0] == '0' && (val_str[1] == 'x' || val_str[1] == 'X')) {
            value = (int32_t)strtol(val_str + 2, NULL, 16);
        } else {
            value = (int32_t)strtol(val_str, NULL, 10);
        }
    }

    /* Validate symbol name */
    const char *name = def;
    if (!*name) {
        free(def);
        return -1;
    }

    /* Store for re-application after reset */
    if (as->cmdline_define_count < ASM_MAX_CMDLINE_DEFINES) {
        as->cmdline_defines[as->cmdline_define_count++] = strdup(definition);
    }

    /* Define as a constant (command-line symbols have no file/line) */
    Symbol *sym = symbol_define(as->symbols, name, value,
                                SYM_DEFINED | SYM_CONSTANT,
                                "<command-line>", 0);
    free(def);

    return sym ? 0 : -1;
}

static char *path_join(const char *dir, const char *file) {
    if (!dir || !*dir) return strdup(file);
    size_t dir_len = strlen(dir);
    size_t file_len = strlen(file);
    int need_slash = (dir[dir_len - 1] != '/');
    char *result = malloc(dir_len + file_len + need_slash + 1);
    if (!result) return NULL;
    strcpy(result, dir);
    if (need_slash) strcat(result, "/");
    strcat(result, file);
    return result;
}

static char *get_directory(const char *filepath) {
    char *copy = strdup(filepath);
    if (!copy) return NULL;
    char *dir = dirname(copy);
    char *result = strdup(dir);
    free(copy);
    return result;
}

char *assembler_find_include(Assembler *as, const char *filename) {
    /* First, try relative to current file */
    if (as->current_file) {
        char *dir = get_directory(as->current_file);
        if (dir) {
            char *path = path_join(dir, filename);
            free(dir);
            if (path && file_exists(path)) {
                return path;
            }
            free(path);
        }
    }

    /* Try include paths */
    for (int i = 0; i < as->include_path_count; i++) {
        char *path = path_join(as->include_paths[i], filename);
        if (path && file_exists(path)) {
            return path;
        }
        free(path);
    }

    /* Try current directory */
    if (file_exists(filename)) {
        return strdup(filename);
    }

    return NULL;
}

static char *read_file_content(const char *filename, long *size_out) {
    FILE *f = fopen(filename, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(content, 1, size, f);
    fclose(f);
    content[read] = '\0';

    if (size_out) *size_out = read;
    return content;
}

char *assembler_get_include_trace(Assembler *as) {
    if (as->include_depth == 0) return NULL;

    size_t total = 256;
    char *trace = malloc(total);
    if (!trace) return NULL;
    trace[0] = '\0';

    for (int i = as->include_depth - 1; i >= 0; i--) {
        char line[256];
        snprintf(line, sizeof(line), "  included from %s:%d\n",
                 as->include_stack[i].filename,
                 as->include_stack[i].line_number);
        size_t len = strlen(trace) + strlen(line) + 1;
        if (len > total) {
            total = len + 256;
            char *new_trace = realloc(trace, total);
            if (!new_trace) {
                free(trace);
                return NULL;
            }
            trace = new_trace;
        }
        strcat(trace, line);
    }

    return trace;
}

int assembler_include_binary(Assembler *as, const char *filename,
                             int offset, int length) {
    char *path = assembler_find_include(as, filename);
    if (!path) {
        assembler_error(as, "cannot find binary file: %s", filename);
        return -1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        assembler_error(as, "cannot open binary file: %s", path);
        free(path);
        return -1;
    }

    /* Get file size */
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);

    if (offset < 0 || offset > file_size) {
        assembler_error(as, "binary offset %d out of range (file size %ld)", offset, file_size);
        fclose(f);
        free(path);
        return -1;
    }

    /* Calculate read length */
    int read_len = length;
    if (read_len <= 0) {
        read_len = file_size - offset;
    }
    if (offset + read_len > file_size) {
        read_len = file_size - offset;
    }

    if (read_len <= 0) {
        fclose(f);
        free(path);
        return 0; /* Nothing to read */
    }

    /* Seek to offset and read */
    fseek(f, offset, SEEK_SET);
    uint8_t *buffer = malloc(read_len);
    if (!buffer) {
        assembler_error(as, "out of memory reading binary file");
        fclose(f);
        free(path);
        return -1;
    }

    size_t actual = fread(buffer, 1, read_len, f);
    fclose(f);
    free(path);

    /* Emit bytes */
    if (as->pass == 2) {
        assembler_emit_bytes(as, buffer, actual);
    } else {
        assembler_advance_pc(as, actual);
    }

    free(buffer);
    return 0;
}

/* ========== Conditional Assembly Functions ========== */

int assembler_is_active(Assembler *as) {
    if (as->cond_depth == 0) return 1; /* No conditionals - always active */
    return as->cond_stack[as->cond_depth - 1].active;
}

int assembler_cond_if(Assembler *as, int condition) {
    if (as->cond_depth >= ASM_MAX_COND_DEPTH) {
        assembler_error(as, "!if nesting too deep (max %d)", ASM_MAX_COND_DEPTH);
        return -1;
    }

    CondEntry *entry = &as->cond_stack[as->cond_depth];
    entry->parent_active = assembler_is_active(as);
    entry->active = entry->parent_active && (condition != 0);
    entry->else_seen = 0;
    entry->filename = as->current_file;
    entry->line_number = as->current_line;

    as->cond_depth++;
    return 0;
}

int assembler_cond_ifdef(Assembler *as, const char *symbol_name) {
    Symbol *sym = symbol_lookup(as->symbols, symbol_name);
    int defined = (sym != NULL && (sym->flags & SYM_DEFINED));
    return assembler_cond_if(as, defined);
}

int assembler_cond_ifndef(Assembler *as, const char *symbol_name) {
    Symbol *sym = symbol_lookup(as->symbols, symbol_name);
    int defined = (sym != NULL && (sym->flags & SYM_DEFINED));
    return assembler_cond_if(as, !defined);
}

int assembler_cond_else(Assembler *as) {
    if (as->cond_depth == 0) {
        assembler_error(as, "!else without matching !if");
        return -1;
    }

    CondEntry *entry = &as->cond_stack[as->cond_depth - 1];

    if (entry->else_seen) {
        assembler_error(as, "duplicate !else for !if at %s:%d",
                       entry->filename ? entry->filename : "<input>",
                       entry->line_number);
        return -1;
    }

    entry->else_seen = 1;
    /* Flip active state only if parent is active */
    if (entry->parent_active) {
        entry->active = !entry->active;
    }

    return 0;
}

int assembler_cond_endif(Assembler *as) {
    if (as->cond_depth == 0) {
        assembler_error(as, "!endif without matching !if");
        return -1;
    }

    as->cond_depth--;
    return 0;
}

/* ========== Macro Functions ========== */

static unsigned int macro_hash(const char *name) {
    unsigned int hash = 5381;
    while (*name) {
        char c = *name++;
        if (c >= 'A' && c <= 'Z') c += 32; /* Case insensitive */
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

MacroTable *macro_table_create(int bucket_count) {
    MacroTable *table = calloc(1, sizeof(MacroTable));
    if (!table) return NULL;

    table->buckets = calloc(bucket_count, sizeof(Macro *));
    if (!table->buckets) {
        free(table);
        return NULL;
    }

    table->bucket_count = bucket_count;
    table->macro_count = 0;
    return table;
}

static void macro_free(Macro *macro) {
    if (!macro) return;
    free(macro->name);
    for (int i = 0; i < macro->param_count; i++) {
        free(macro->params[i]);
    }
    free(macro->params);
    free(macro->body);
    free(macro);
}

void macro_table_free(MacroTable *table) {
    if (!table) return;

    for (int i = 0; i < table->bucket_count; i++) {
        Macro *m = table->buckets[i];
        while (m) {
            Macro *next = m->next;
            macro_free(m);
            m = next;
        }
    }
    free(table->buckets);
    free(table);
}

Macro *macro_lookup(MacroTable *table, const char *name) {
    if (!table || !name) return NULL;

    unsigned int idx = macro_hash(name) % table->bucket_count;
    Macro *m = table->buckets[idx];

    while (m) {
        if (strcasecmp(m->name, name) == 0) {
            return m;
        }
        m = m->next;
    }
    return NULL;
}

int macro_define(Assembler *as, const char *name, char **params, int param_count,
                 const char *body, const char *filename, int line_number) {
    MacroTable *table = as->macros;

    /* Check for duplicate */
    if (macro_lookup(table, name)) {
        assembler_error(as, "macro '%s' already defined", name);
        return -1;
    }

    Macro *macro = calloc(1, sizeof(Macro));
    if (!macro) return -1;

    macro->name = strdup(name);
    macro->param_count = param_count;
    macro->filename = filename;
    macro->line_number = line_number;

    if (param_count > 0) {
        macro->params = calloc(param_count, sizeof(char *));
        if (!macro->params) {
            macro_free(macro);
            return -1;
        }
        for (int i = 0; i < param_count; i++) {
            macro->params[i] = strdup(params[i]);
        }
    }

    macro->body = strdup(body ? body : "");

    /* Insert into table */
    unsigned int idx = macro_hash(name) % table->bucket_count;
    macro->next = table->buckets[idx];
    table->buckets[idx] = macro;
    table->macro_count++;

    return 0;
}

/* Substitute macro parameters in body text */
static char *macro_substitute(const char *body, Macro *macro,
                              char **args, int arg_count, int unique_id) {
    (void)unique_id; /* Reserved for local label mangling */

    /* Estimate output size */
    size_t body_len = strlen(body);
    size_t out_capacity = body_len * 2 + 256;
    char *output = malloc(out_capacity);
    if (!output) return NULL;

    size_t out_len = 0;
    const char *p = body;

    while (*p) {
        /* Check for parameter reference (simple word match) */
        int found_param = 0;

        if ((p == body || !isalnum((unsigned char)p[-1])) && isalpha((unsigned char)*p)) {
            /* Start of identifier - check if it's a parameter */
            const char *word_start = p;
            while (isalnum((unsigned char)*p) || *p == '_') p++;
            size_t word_len = p - word_start;

            for (int i = 0; i < macro->param_count && i < arg_count; i++) {
                if (strlen(macro->params[i]) == word_len &&
                    strncasecmp(macro->params[i], word_start, word_len) == 0) {
                    /* Replace with argument value */
                    size_t arg_len = strlen(args[i]);
                    while (out_len + arg_len + 1 >= out_capacity) {
                        out_capacity *= 2;
                        char *new_out = realloc(output, out_capacity);
                        if (!new_out) { free(output); return NULL; }
                        output = new_out;
                    }
                    memcpy(output + out_len, args[i], arg_len);
                    out_len += arg_len;
                    found_param = 1;
                    break;
                }
            }

            if (!found_param) {
                /* Not a parameter - copy the word as-is */
                while (out_len + word_len + 1 >= out_capacity) {
                    out_capacity *= 2;
                    char *new_out = realloc(output, out_capacity);
                    if (!new_out) { free(output); return NULL; }
                    output = new_out;
                }
                memcpy(output + out_len, word_start, word_len);
                out_len += word_len;
            }
        } else {
            /* Copy character */
            if (out_len + 2 >= out_capacity) {
                out_capacity *= 2;
                char *new_out = realloc(output, out_capacity);
                if (!new_out) { free(output); return NULL; }
                output = new_out;
            }
            output[out_len++] = *p++;
        }
    }

    output[out_len] = '\0';
    return output;
}

int macro_expand(Assembler *as, const char *name, char **args, int arg_count) {
    Macro *macro = macro_lookup(as->macros, name);
    if (!macro) {
        assembler_error(as, "undefined macro '%s'", name);
        return -1;
    }

    /* Check argument count */
    if (arg_count != macro->param_count) {
        assembler_error(as, "macro '%s' expects %d arguments, got %d",
                       name, macro->param_count, arg_count);
        return -1;
    }

    /* Check recursion depth */
    if (as->macro_depth >= ASM_MAX_MACRO_DEPTH) {
        assembler_error(as, "macro expansion too deep (max %d)", ASM_MAX_MACRO_DEPTH);
        return -1;
    }

    /* Create expansion context */
    MacroExpansion *exp = calloc(1, sizeof(MacroExpansion));
    if (!exp) return -1;

    exp->name = name;
    exp->arg_count = arg_count;
    exp->unique_id = ++as->macro_unique_counter;

    if (arg_count > 0) {
        exp->arg_values = calloc(arg_count, sizeof(char *));
        for (int i = 0; i < arg_count; i++) {
            exp->arg_values[i] = strdup(args[i]);
        }
    }

    /* Push onto stack */
    as->macro_stack[as->macro_depth++] = exp;

    /* Substitute parameters in body */
    char *expanded = macro_substitute(macro->body, macro, args, arg_count, exp->unique_id);
    if (!expanded) {
        as->macro_depth--;
        for (int i = 0; i < arg_count; i++) free(exp->arg_values[i]);
        free(exp->arg_values);
        free(exp);
        return -1;
    }

    /* Assemble the expanded body */
    /* Save current file/line and zone */
    const char *saved_file = as->current_file;
    int saved_line = as->current_line;
    char *saved_zone = as->current_zone;

    /* Create unique zone for this macro expansion */
    char macro_zone[64];
    snprintf(macro_zone, sizeof(macro_zone), "_macro_%d", exp->unique_id);
    as->current_zone = strdup(macro_zone);

    /* Create a pseudo-filename for error messages */
    char macro_file[256];
    snprintf(macro_file, sizeof(macro_file), "<%s>", name);

    Lexer lexer;
    Parser parser;
    lexer_init(&lexer, expanded, macro_file);
    parser_init(&parser, &lexer, as->symbols);
    parser_set_pc(&parser, as->pc);
    parser_set_pass(&parser, as->pass);

    /* Parse and assemble each line */
    while (*lexer.current) {
        Statement *stmt = parser_parse_line(&parser);
        if (!stmt) break;

        as->current_file = macro_file;
        as->current_line = stmt->line;

        if (stmt->type == STMT_EMPTY && *lexer.current == '\0') {
            statement_free(stmt);
            break;
        }

        /* Handle conditional directives */
        if (is_conditional_directive(stmt)) {
            process_conditional_directive(as, stmt);
            statement_free(stmt);
            continue;
        }

        /* Skip if conditional assembly is inactive */
        if (!assembler_is_active(as)) {
            statement_free(stmt);
            continue;
        }

        /* Handle nested macro calls */
        if (stmt->type == STMT_MACRO_CALL) {
            MacroCallInfo *call = &stmt->data.macro_call;
            macro_expand(as, call->name, call->args, call->arg_count);
            statement_free(stmt);
            continue;
        }

        /* Handle statement (label definitions, instructions, etc.) */
        if (as->pass == 1) {
            /* Store for pass 2 - macro source not captured for listings */
            add_assembled_line(as, stmt, as->pc, NULL);
        }

        assembler_assemble_statement(as, stmt);

        if (as->pass == 2) {
            statement_free(stmt);
        }

        if (as->errors >= ASM_MAX_ERRORS) break;
    }

    free(expanded);

    /* Restore file/line and zone */
    as->current_file = saved_file;
    as->current_line = saved_line;
    free(as->current_zone);
    as->current_zone = saved_zone;

    /* Pop expansion context */
    as->macro_depth--;
    for (int i = 0; i < exp->arg_count; i++) {
        free(exp->arg_values[i]);
    }
    free(exp->arg_values);
    free(exp);

    return 0;
}

int assembler_in_macro(Assembler *as) {
    return as->macro_depth > 0;
}

int assembler_macro_unique_id(Assembler *as) {
    if (as->macro_depth == 0) return 0;
    return as->macro_stack[as->macro_depth - 1]->unique_id;
}

/* ========== Loop Functions ========== */

/* Free a loop entry */
static void loop_entry_free(LoopEntry *entry) {
    if (!entry) return;
    free(entry->body);
    free(entry->var_name);
    if (entry->condition) expr_free(entry->condition);
    free(entry);
}

/* Execute loop body once */
static int execute_loop_body(Assembler *as, LoopEntry *loop) {
    /* Save current file/line */
    const char *saved_file = as->current_file;
    int saved_line = as->current_line;

    /* Create a pseudo-filename for error messages */
    char loop_file[256];
    if (loop->type == LOOP_FOR) {
        snprintf(loop_file, sizeof(loop_file), "<for %s>", loop->var_name);
    } else {
        snprintf(loop_file, sizeof(loop_file), "<while>");
    }

    Lexer lexer;
    Parser parser;
    lexer_init(&lexer, loop->body, loop_file);
    parser_init(&parser, &lexer, as->symbols);
    parser_set_pc(&parser, as->pc);
    parser_set_pass(&parser, as->pass);

    /* Parse and assemble each line */
    while (*lexer.current || parser.current.type != TOK_EOF) {
        Statement *stmt = parser_parse_line(&parser);
        if (!stmt) break;

        as->current_file = loop_file;
        as->current_line = stmt->line;

        if (stmt->type == STMT_EMPTY && parser.current.type == TOK_EOF) {
            statement_free(stmt);
            break;
        }

        /* Handle conditional directives */
        if (is_conditional_directive(stmt)) {
            process_conditional_directive(as, stmt);
            statement_free(stmt);
            continue;
        }

        /* Skip if conditional assembly is inactive */
        if (!assembler_is_active(as)) {
            statement_free(stmt);
            continue;
        }

        /* Handle macro calls */
        if (stmt->type == STMT_MACRO_CALL) {
            MacroCallInfo *call = &stmt->data.macro_call;
            macro_expand(as, call->name, call->args, call->arg_count);
            statement_free(stmt);
            continue;
        }

        /* Handle nested loop directives */
        if (is_loop_directive(stmt)) {
            process_loop_directive(as, stmt, &lexer, &parser);
            statement_free(stmt);
            continue;
        }

        /* Handle statement (label definitions, instructions, etc.) */
        if (as->pass == 1) {
            /* Loop body source not captured for listings */
            add_assembled_line(as, stmt, as->pc, NULL);
        }

        assembler_assemble_statement(as, stmt);

        if (as->pass == 2) {
            statement_free(stmt);
        }

        if (as->errors >= ASM_MAX_ERRORS) break;
    }

    /* Restore file/line */
    as->current_file = saved_file;
    as->current_line = saved_line;

    return 0;
}

/* Substitute loop variable in body text */
static char *substitute_loop_var(const char *body, const char *var_name, int32_t value) {
    if (!body || !var_name) return strdup(body ? body : "");

    size_t var_len = strlen(var_name);
    char value_str[32];
    snprintf(value_str, sizeof(value_str), "%d", value);
    size_t value_len = strlen(value_str);

    /* First pass: count replacements */
    int count = 0;
    const char *p = body;
    while (*p) {
        if (strncasecmp(p, var_name, var_len) == 0) {
            /* Check word boundaries */
            int at_start = (p == body || (!isalnum((unsigned char)p[-1]) && p[-1] != '_'));
            int at_end = (!isalnum((unsigned char)p[var_len]) && p[var_len] != '_');
            if (at_start && at_end) {
                count++;
                p += var_len;
                continue;
            }
        }
        p++;
    }

    /* Allocate result */
    size_t new_len = strlen(body) + count * (value_len > var_len ? value_len - var_len : 0) + 1;
    char *result = malloc(new_len + count * value_len);  /* Extra safety */
    if (!result) return NULL;

    /* Second pass: do substitution */
    char *out = result;
    p = body;
    while (*p) {
        if (strncasecmp(p, var_name, var_len) == 0) {
            int at_start = (p == body || (!isalnum((unsigned char)p[-1]) && p[-1] != '_'));
            int at_end = !isalnum((unsigned char)p[var_len]) && p[var_len] != '_';
            if (at_start && at_end) {
                memcpy(out, value_str, value_len);
                out += value_len;
                p += var_len;
                continue;
            }
        }
        *out++ = *p++;
    }
    *out = '\0';

    return result;
}

int assembler_loop_for(Assembler *as, const char *var_name,
                       int32_t start, int32_t end, const char *body) {
    /* Check nesting depth */
    if (as->loop_depth >= ASM_MAX_LOOP_DEPTH) {
        assembler_error(as, "loop nesting too deep (max %d)", ASM_MAX_LOOP_DEPTH);
        return -1;
    }

    /* Determine direction */
    int32_t step = (start <= end) ? 1 : -1;

    /* Execute loop */
    for (int32_t i = start; step > 0 ? (i <= end) : (i >= end); i += step) {
        /* Substitute loop variable in body */
        char *expanded = substitute_loop_var(body, var_name, i);
        if (!expanded) {
            assembler_error(as, "out of memory in loop expansion");
            return -1;
        }

        /* Also define the variable as a symbol for expressions */
        symbol_define(as->symbols, var_name, i, SYM_DEFINED,
                      as->current_file, as->current_line);

        /* Create a temporary loop entry for tracking */
        LoopEntry *entry = calloc(1, sizeof(LoopEntry));
        if (!entry) {
            free(expanded);
            return -1;
        }
        entry->type = LOOP_FOR;
        entry->var_name = strdup(var_name);
        entry->current = i;
        entry->end = end;
        entry->step = step;
        entry->body = expanded;
        entry->filename = as->current_file;
        entry->line_number = as->current_line;

        as->loop_stack[as->loop_depth++] = entry;

        /* Execute the body */
        execute_loop_body(as, entry);

        /* Pop loop */
        as->loop_depth--;
        loop_entry_free(entry);

        if (as->errors >= ASM_MAX_ERRORS) break;
    }

    return 0;
}

int assembler_loop_while(Assembler *as, Expr *condition, const char *body) {
    /* Check nesting depth */
    if (as->loop_depth >= ASM_MAX_LOOP_DEPTH) {
        assembler_error(as, "loop nesting too deep (max %d)", ASM_MAX_LOOP_DEPTH);
        return -1;
    }

    /* Create loop entry */
    LoopEntry *entry = calloc(1, sizeof(LoopEntry));
    if (!entry) return -1;

    entry->type = LOOP_WHILE;
    entry->condition = expr_clone(condition);
    entry->body = strdup(body);
    entry->filename = as->current_file;
    entry->line_number = as->current_line;

    as->loop_stack[as->loop_depth++] = entry;

    /* Execute while condition is true */
    int iterations = 0;
    const int max_iterations = 100000;  /* Safety limit */

    while (iterations < max_iterations) {
        /* Evaluate condition */
        ExprResult result = expr_eval(entry->condition, as->symbols, as->anon_labels, as->pc, as->pass, as->current_zone);
        if (!result.defined) {
            assembler_error(as, "undefined symbol in !while condition");
            break;
        }
        if (result.value == 0) {
            break;  /* Condition is false, exit loop */
        }

        /* Execute body */
        execute_loop_body(as, entry);

        iterations++;
        if (as->errors >= ASM_MAX_ERRORS) break;
    }

    if (iterations >= max_iterations) {
        assembler_error(as, "!while loop exceeded maximum iterations (%d)", max_iterations);
    }

    /* Pop loop */
    as->loop_depth--;
    loop_entry_free(entry);

    return 0;
}

int assembler_in_loop(Assembler *as) {
    return as->loop_depth > 0;
}

int32_t assembler_loop_var_value(Assembler *as, const char *var_name) {
    /* Search loop stack for a matching variable */
    for (int i = as->loop_depth - 1; i >= 0; i--) {
        LoopEntry *entry = as->loop_stack[i];
        if (entry->type == LOOP_FOR && entry->var_name &&
            strcasecmp(entry->var_name, var_name) == 0) {
            return entry->current;
        }
    }
    return 0;
}

/* ========== Pseudo-PC Functions ========== */

int assembler_pseudopc_start(Assembler *as, uint16_t pseudo_addr) {
    if (as->in_pseudopc) {
        assembler_error(as, "nested !pseudopc not allowed");
        return -1;
    }

    /* Save the real PC before switching */
    as->real_pc = as->pc;
    as->in_pseudopc = 1;

    /* Set the pseudo PC */
    as->pc = pseudo_addr;

    return 0;
}

int assembler_pseudopc_end(Assembler *as) {
    if (!as->in_pseudopc) {
        assembler_error(as, "!realpc without matching !pseudopc");
        return -1;
    }

    /* The real_pc has been tracking the actual output position.
     * Now restore pc to continue from real_pc. */
    as->pc = as->real_pc;
    as->in_pseudopc = 0;

    return 0;
}

int assembler_in_pseudopc(Assembler *as) {
    return as->in_pseudopc;
}

uint16_t assembler_get_real_pc(Assembler *as) {
    return as->in_pseudopc ? as->real_pc : as->pc;
}

/* ========== CPU Selection Functions ========== */

int assembler_set_cpu(Assembler *as, const char *cpu_name) {
    if (strcasecmp(cpu_name, "6502") == 0) {
        as->cpu_type = CPU_6502;
        return 0;
    } else if (strcasecmp(cpu_name, "6510") == 0) {
        as->cpu_type = CPU_6510;
        return 0;
    } else if (strcasecmp(cpu_name, "65c02") == 0) {
        as->cpu_type = CPU_65C02;
        return 0;
    }

    assembler_error(as, "unknown CPU type: %s", cpu_name);
    return -1;
}

CpuType assembler_get_cpu(Assembler *as) {
    return as->cpu_type;
}

int assembler_opcode_valid_for_cpu(Assembler *as, uint8_t opcode) {
    /* Get opcode info */
    const OpcodeEntry *entry = opcode_find_by_opcode(opcode);
    if (!entry) return 0;

    /* Check if the mnemonic is an illegal opcode */
    int is_illegal = opcode_is_illegal(entry->mnemonic);

    switch (as->cpu_type) {
        case CPU_6502:
            /* Standard 6502: no illegal opcodes */
            return !is_illegal;

        case CPU_6510:
            /* 6510: all opcodes including illegal */
            return 1;

        case CPU_65C02:
            /* 65C02: standard + extended, no illegal */
            /* For now, just disallow illegal opcodes */
            return !is_illegal;
    }

    return 0;
}
