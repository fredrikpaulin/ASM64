/*
 * parser.c - Statement Parser Implementation
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* For strcasecmp */
#include <ctype.h>
#include <stdio.h>

/* ========== Helper Functions ========== */

static char *str_dup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *dup = malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

static char *token_to_string(Token *tok) {
    char *str = malloc(tok->length + 1);
    if (!str) return NULL;
    memcpy(str, tok->start, tok->length);
    str[tok->length] = '\0';
    return str;
}

static char *token_to_upper(Token *tok) {
    char *str = token_to_string(tok);
    if (str) {
        for (char *p = str; *p; p++) {
            *p = toupper((unsigned char)*p);
        }
    }
    return str;
}

/* ========== Parser Core ========== */

static void advance(Parser *parser) {
    parser->previous = parser->current;
    parser->current = lexer_next(parser->lexer);
}

static int check(Parser *parser, TokenType type) {
    return parser->current.type == type;
}

static int match(Parser *parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return 1;
    }
    return 0;
}

static int at_line_end(Parser *parser) {
    return check(parser, TOK_EOL) || check(parser, TOK_EOF);
}

void parser_init(Parser *parser, Lexer *lexer, SymbolTable *symbols) {
    parser->lexer = lexer;
    parser->symbols = symbols;
    parser->pc = 0x0801;  /* Default C64 BASIC start */
    parser->pass = 1;
    parser->error = NULL;
    advance(parser);  /* Load first token */
}

void parser_set_pc(Parser *parser, uint16_t pc) {
    parser->pc = pc;
}

void parser_set_pass(Parser *parser, int pass) {
    parser->pass = pass;
}

const char *parser_error(Parser *parser) {
    return parser->error;
}

/* ========== Statement Allocation ========== */

static Statement *statement_new(StatementType type, int line, const char *file) {
    Statement *stmt = calloc(1, sizeof(Statement));
    if (!stmt) return NULL;
    stmt->type = type;
    stmt->line = line;
    stmt->file = file;
    return stmt;
}

static LabelInfo *label_new(void) {
    return calloc(1, sizeof(LabelInfo));
}

static void label_free(LabelInfo *label) {
    if (!label) return;
    free(label->name);
    free(label);
}

void statement_free(Statement *stmt) {
    if (!stmt) return;

    label_free(stmt->label);

    switch (stmt->type) {
        case STMT_INSTRUCTION:
            free(stmt->data.instruction.mnemonic);
            expr_free(stmt->data.instruction.operand);
            break;

        case STMT_DIRECTIVE:
            free(stmt->data.directive.name);
            for (int i = 0; i < stmt->data.directive.arg_count; i++) {
                expr_free(stmt->data.directive.args[i]);
            }
            free(stmt->data.directive.args);
            free(stmt->data.directive.string_arg);
            break;

        case STMT_ASSIGNMENT:
            free(stmt->data.assignment.name);
            expr_free(stmt->data.assignment.value);
            break;

        case STMT_MACRO_CALL:
            free(stmt->data.macro_call.name);
            for (int i = 0; i < stmt->data.macro_call.arg_count; i++) {
                free(stmt->data.macro_call.args[i]);
            }
            free(stmt->data.macro_call.args);
            break;

        default:
            break;
    }

    free(stmt->error_msg);
    free(stmt);
}

/* ========== Operand Parsing ========== */

typedef struct {
    Expr *expr;
    int has_hash;       /* # prefix */
    int has_x_index;    /* ,X suffix */
    int has_y_index;    /* ,Y suffix */
    int is_indirect;    /* ( ) wrapper */
    int forced_zp;      /* < prefix for forced zero-page */
    int forced_abs;     /* ! prefix for forced absolute */
} OperandInfo;

static OperandInfo parse_operand(Parser *parser) {
    OperandInfo info = {0};
    ExprParser expr_parser;

    /* Check for immediate mode prefix */
    if (match(parser, TOK_HASH)) {
        info.has_hash = 1;
    }

    /* Check for forced addressing mode prefixes */
    /* Note: < and > are handled as unary operators in expressions */
    /* For forced ZP/ABS, some assemblers use special syntax */

    /* Check for indirect mode opening paren */
    if (match(parser, TOK_LPAREN)) {
        info.is_indirect = 1;

        /* Parse expression inside parens */
        expr_parser_init_with_token(&expr_parser, parser->lexer, parser->current);
        info.expr = expr_parse(&expr_parser);
        parser->current = expr_parser.current;

        /* Check for ,X before closing paren: (expr,X) */
        if (match(parser, TOK_COMMA)) {
            if (check(parser, TOK_IDENTIFIER)) {
                char *reg = token_to_upper(&parser->current);
                if (reg && strcmp(reg, "X") == 0) {
                    info.has_x_index = 1;
                    advance(parser);
                }
                free(reg);
            }
        }

        /* Expect closing paren */
        if (!match(parser, TOK_RPAREN)) {
            parser->error = "expected ')'";
        }

        /* Check for ,Y after closing paren: (expr),Y */
        if (match(parser, TOK_COMMA)) {
            if (check(parser, TOK_IDENTIFIER)) {
                char *reg = token_to_upper(&parser->current);
                if (reg && strcmp(reg, "Y") == 0) {
                    info.has_y_index = 1;
                    advance(parser);
                }
                free(reg);
            }
        }
    } else if (!at_line_end(parser)) {
        /* Parse non-indirect operand expression */
        expr_parser_init_with_token(&expr_parser, parser->lexer, parser->current);
        info.expr = expr_parse(&expr_parser);
        parser->current = expr_parser.current;

        /* Check for index register suffix */
        if (match(parser, TOK_COMMA)) {
            if (check(parser, TOK_IDENTIFIER)) {
                char *reg = token_to_upper(&parser->current);
                if (reg) {
                    if (strcmp(reg, "X") == 0) {
                        info.has_x_index = 1;
                        advance(parser);
                    } else if (strcmp(reg, "Y") == 0) {
                        info.has_y_index = 1;
                        advance(parser);
                    }
                    free(reg);
                }
            }
        }
    }

    return info;
}

/* ========== Addressing Mode Detection ========== */

int is_branch_instruction(const char *mnemonic) {
    static const char *branches[] = {
        "BCC", "BCS", "BEQ", "BMI", "BNE", "BPL", "BVC", "BVS", NULL
    };
    for (int i = 0; branches[i]; i++) {
        if (strcasecmp(mnemonic, branches[i]) == 0) return 1;
    }
    return 0;
}

int is_accumulator_optional(const char *mnemonic) {
    static const char *accum[] = {
        "ASL", "LSR", "ROL", "ROR", NULL
    };
    for (int i = 0; accum[i]; i++) {
        if (strcasecmp(mnemonic, accum[i]) == 0) return 1;
    }
    return 0;
}

AddressingMode detect_addressing_mode(
    const char *mnemonic,
    Expr *expr,
    int has_hash,
    int has_x_index,
    int has_y_index,
    int is_indirect,
    int32_t value,
    int value_known
) {
    /* Branch instructions always use relative addressing */
    if (is_branch_instruction(mnemonic)) {
        return ADDR_RELATIVE;
    }

    /* Immediate mode: #value */
    if (has_hash) {
        return ADDR_IMMEDIATE;
    }

    /* No operand: implied or accumulator */
    if (!expr) {
        if (is_accumulator_optional(mnemonic)) {
            /* Could be either - check if accumulator mode exists */
            if (opcode_find(mnemonic, ADDR_ACCUMULATOR)) {
                return ADDR_ACCUMULATOR;
            }
        }
        return ADDR_IMPLIED;
    }

    /* Check for explicit accumulator operand (A) */
    if (expr->type == EXPR_SYMBOL && expr->data.symbol) {
        if (strcasecmp(expr->data.symbol, "A") == 0 &&
            is_accumulator_optional(mnemonic)) {
            return ADDR_ACCUMULATOR;
        }
    }

    /* Indirect modes */
    if (is_indirect) {
        if (has_x_index) {
            /* (expr,X) - indexed indirect */
            return ADDR_INDIRECT_X;
        } else if (has_y_index) {
            /* (expr),Y - indirect indexed */
            return ADDR_INDIRECT_Y;
        } else {
            /* (expr) - indirect (only for JMP) */
            return ADDR_INDIRECT;
        }
    }

    /* Indexed modes */
    if (has_x_index) {
        /* Decide between zero-page,X and absolute,X */
        if (value_known && value >= 0 && value <= 0xFF) {
            /* Check if ZP,X mode exists for this instruction */
            if (opcode_find(mnemonic, ADDR_ZEROPAGE_X)) {
                return ADDR_ZEROPAGE_X;
            }
        }
        return ADDR_ABSOLUTE_X;
    }

    if (has_y_index) {
        /* Decide between zero-page,Y and absolute,Y */
        if (value_known && value >= 0 && value <= 0xFF) {
            if (opcode_find(mnemonic, ADDR_ZEROPAGE_Y)) {
                return ADDR_ZEROPAGE_Y;
            }
        }
        return ADDR_ABSOLUTE_Y;
    }

    /* Plain address - decide between zero-page and absolute */
    if (value_known && value >= 0 && value <= 0xFF) {
        if (opcode_find(mnemonic, ADDR_ZEROPAGE)) {
            return ADDR_ZEROPAGE;
        }
    }

    return ADDR_ABSOLUTE;
}

int validate_addressing_mode(const char *mnemonic, AddressingMode mode) {
    return opcode_find(mnemonic, mode) != NULL;
}

int get_instruction_size(Statement *stmt) {
    if (stmt->type != STMT_INSTRUCTION) return 0;
    return stmt->data.instruction.size;
}

/* ========== Instruction Parsing ========== */

static Statement *parse_instruction(Parser *parser, const char *mnemonic, int line) {
    Statement *stmt = statement_new(STMT_INSTRUCTION, line, parser->lexer->filename);
    if (!stmt) return NULL;

    stmt->data.instruction.mnemonic = str_dup(mnemonic);

    /* Parse operand */
    OperandInfo operand = parse_operand(parser);
    stmt->data.instruction.operand = operand.expr;
    stmt->data.instruction.forced_zp = operand.forced_zp;
    stmt->data.instruction.forced_abs = operand.forced_abs;

    /* Evaluate operand if possible */
    int32_t value = 0;
    int value_known = 0;
    if (operand.expr) {
        ExprResult result = expr_eval(operand.expr, parser->symbols, NULL, parser->pc, parser->pass);
        value = result.value;
        value_known = result.defined;
    }

    /* Determine addressing mode */
    AddressingMode mode = detect_addressing_mode(
        mnemonic,
        operand.expr,
        operand.has_hash,
        operand.has_x_index,
        operand.has_y_index,
        operand.is_indirect,
        value,
        value_known
    );

    stmt->data.instruction.mode = mode;

    /* Look up opcode */
    const OpcodeEntry *op = opcode_find(mnemonic, mode);
    if (op) {
        stmt->data.instruction.opcode = op->opcode;
        stmt->data.instruction.size = op->size;
        stmt->data.instruction.cycles = op->cycles;
        stmt->data.instruction.page_penalty = op->page_penalty;
    } else {
        /* Invalid addressing mode for this instruction */
        /* In pass 1, might be due to unknown forward reference - assume absolute */
        if (!value_known && parser->pass == 1) {
            /* Try absolute mode as fallback */
            op = opcode_find(mnemonic, ADDR_ABSOLUTE);
            if (op) {
                stmt->data.instruction.mode = ADDR_ABSOLUTE;
                stmt->data.instruction.opcode = op->opcode;
                stmt->data.instruction.size = op->size;
                stmt->data.instruction.cycles = op->cycles;
                stmt->data.instruction.page_penalty = op->page_penalty;
            } else {
                stmt->type = STMT_ERROR;
                stmt->error_msg = str_dup("invalid addressing mode for instruction");
            }
        } else {
            stmt->type = STMT_ERROR;
            stmt->error_msg = str_dup("invalid addressing mode for instruction");
        }
    }

    return stmt;
}

/* ========== Directive Parsing ========== */

static Statement *parse_directive(Parser *parser, int line) {
    Statement *stmt = statement_new(STMT_DIRECTIVE, line, parser->lexer->filename);
    if (!stmt) return NULL;

    /* Get directive name (skip the !) */
    char *full_name = token_to_string(&parser->current);
    if (full_name && full_name[0] == '!') {
        stmt->data.directive.name = str_dup(full_name + 1);
    } else {
        stmt->data.directive.name = full_name;
        full_name = NULL;
    }
    free(full_name);
    advance(parser);

    /* Parse arguments - for now, collect expressions until EOL */
    Expr **args = NULL;
    int arg_count = 0;
    int arg_capacity = 0;

    /* Special handling for !macro directive: parse space-separated identifiers */
    int is_macro_directive = (strcasecmp(stmt->data.directive.name, "macro") == 0);

    while (!at_line_end(parser)) {
        /* Check for string argument */
        if (check(parser, TOK_STRING)) {
            stmt->data.directive.string_arg = malloc(parser->current.value.string.len + 1);
            if (stmt->data.directive.string_arg) {
                memcpy(stmt->data.directive.string_arg,
                       parser->current.value.string.data,
                       parser->current.value.string.len);
                stmt->data.directive.string_arg[parser->current.value.string.len] = '\0';
            }
            advance(parser);
        } else if (is_macro_directive && check(parser, TOK_IDENTIFIER)) {
            /* For !macro, collect identifiers as symbol expressions (name and params) */
            char *ident = token_to_string(&parser->current);
            Expr *arg = expr_symbol(ident);
            free(ident);
            if (arg) {
                if (arg_count >= arg_capacity) {
                    arg_capacity = arg_capacity ? arg_capacity * 2 : 4;
                    Expr **new_args = realloc(args, arg_capacity * sizeof(Expr *));
                    if (!new_args) {
                        expr_free(arg);
                        break;
                    }
                    args = new_args;
                }
                args[arg_count++] = arg;
            }
            advance(parser);
            /* For !macro, continue parsing even without comma (space-separated) */
            match(parser, TOK_COMMA);  /* Optional comma */
            continue;
        } else {
            /* Parse expression argument */
            ExprParser expr_parser;
            expr_parser_init_with_token(&expr_parser, parser->lexer, parser->current);
            Expr *arg = expr_parse(&expr_parser);
            parser->current = expr_parser.current;

            if (arg) {
                if (arg_count >= arg_capacity) {
                    arg_capacity = arg_capacity ? arg_capacity * 2 : 4;
                    Expr **new_args = realloc(args, arg_capacity * sizeof(Expr *));
                    if (!new_args) {
                        expr_free(arg);
                        break;
                    }
                    args = new_args;
                }
                args[arg_count++] = arg;
            }
        }

        /* Skip comma between arguments */
        if (!match(parser, TOK_COMMA)) {
            break;
        }
    }

    stmt->data.directive.args = args;
    stmt->data.directive.arg_count = arg_count;

    return stmt;
}

/* ========== Assignment Parsing ========== */

static Statement *parse_assignment(Parser *parser, const char *name, int line) {
    Statement *stmt = statement_new(STMT_ASSIGNMENT, line, parser->lexer->filename);
    if (!stmt) return NULL;

    stmt->data.assignment.name = str_dup(name);

    /* Skip the = token */
    advance(parser);

    /* Parse value expression */
    ExprParser expr_parser;
    expr_parser_init_with_token(&expr_parser, parser->lexer, parser->current);
    stmt->data.assignment.value = expr_parse(&expr_parser);
    parser->current = expr_parser.current;

    return stmt;
}

/* ========== Macro Call Parsing ========== */

static Statement *parse_macro_call(Parser *parser, int line) {
    Statement *stmt = statement_new(STMT_MACRO_CALL, line, parser->lexer->filename);
    if (!stmt) return NULL;

    /* Get macro name (skip the +) */
    char *full_name = token_to_string(&parser->current);
    if (full_name && full_name[0] == '+') {
        stmt->data.macro_call.name = str_dup(full_name + 1);
    } else {
        stmt->data.macro_call.name = full_name;
        full_name = NULL;
    }
    free(full_name);
    advance(parser);

    /* Parse arguments as strings until EOL */
    /* For now, we'll collect raw text - proper macro expansion comes later */
    char **args = NULL;
    int arg_count = 0;
    int arg_capacity = 0;

    while (!at_line_end(parser)) {
        char *arg = token_to_string(&parser->current);
        if (arg) {
            if (arg_count >= arg_capacity) {
                arg_capacity = arg_capacity ? arg_capacity * 2 : 4;
                char **new_args = realloc(args, arg_capacity * sizeof(char *));
                if (!new_args) {
                    free(arg);
                    break;
                }
                args = new_args;
            }
            args[arg_count++] = arg;
        }
        advance(parser);

        if (!match(parser, TOK_COMMA)) {
            break;
        }
    }

    stmt->data.macro_call.args = args;
    stmt->data.macro_call.arg_count = arg_count;

    return stmt;
}

/* ========== Main Line Parser ========== */

Statement *parser_parse_line(Parser *parser) {
    int line = parser->current.line;
    parser->error = NULL;

    /* Skip empty lines */
    if (at_line_end(parser)) {
        Statement *stmt = statement_new(STMT_EMPTY, line, parser->lexer->filename);
        if (check(parser, TOK_EOL)) advance(parser);
        return stmt;
    }

    Statement *stmt = NULL;
    LabelInfo *label = NULL;

    /* Try to parse a label at the start */
    if (check(parser, TOK_IDENTIFIER) || check(parser, TOK_LOCAL_LABEL) ||
        check(parser, TOK_ANON_FWD) || check(parser, TOK_ANON_BACK)) {

        /* Peek ahead to see if this is a label or an instruction/assignment */
        Token saved = parser->current;
        char *name = token_to_string(&parser->current);

        advance(parser);

        if (check(parser, TOK_COLON)) {
            /* Definitely a label */
            advance(parser);  /* Skip colon */
            label = label_new();
            if (label) {
                label->name = name;
                label->is_local = (saved.type == TOK_LOCAL_LABEL);
            } else {
                free(name);
            }
        } else if (check(parser, TOK_EQ)) {
            /* Assignment: NAME = value */
            stmt = parse_assignment(parser, name, line);
            free(name);
            goto done;
        } else if (token_is_mnemonic(&saved)) {
            /* Instruction */
            stmt = parse_instruction(parser, name, line);
            free(name);
            goto done;
        } else if (at_line_end(parser)) {
            /* Just a label on its own */
            label = label_new();
            if (label) {
                label->name = name;
                label->is_local = (saved.type == TOK_LOCAL_LABEL);
                label->is_anon_fwd = (saved.type == TOK_ANON_FWD);
                label->is_anon_back = (saved.type == TOK_ANON_BACK);
            } else {
                free(name);
            }
        } else {
            /* Assume it's a label followed by something */
            label = label_new();
            if (label) {
                label->name = name;
                label->is_local = (saved.type == TOK_LOCAL_LABEL);
                label->is_anon_fwd = (saved.type == TOK_ANON_FWD);
                label->is_anon_back = (saved.type == TOK_ANON_BACK);
            } else {
                free(name);
            }
        }
    }

    /* Now parse what follows the label (if anything) */
    if (!at_line_end(parser)) {
        if (check(parser, TOK_DIRECTIVE)) {
            stmt = parse_directive(parser, line);
        } else if (check(parser, TOK_MACRO_CALL)) {
            stmt = parse_macro_call(parser, line);
        } else if (check(parser, TOK_IDENTIFIER)) {
            /* Should be an instruction */
            char *name = token_to_upper(&parser->current);
            if (token_is_mnemonic(&parser->current)) {
                advance(parser);
                stmt = parse_instruction(parser, name, line);
            } else {
                /* Unknown identifier - could be macro or error */
                stmt = statement_new(STMT_ERROR, line, parser->lexer->filename);
                stmt->error_msg = malloc(64 + strlen(name));
                if (stmt->error_msg) {
                    sprintf(stmt->error_msg, "unknown instruction or directive: %s", name);
                }
                advance(parser);
            }
            free(name);
        } else if (check(parser, TOK_STAR)) {
            /* Origin directive: *= expr */
            advance(parser);
            if (match(parser, TOK_EQ)) {
                stmt = statement_new(STMT_DIRECTIVE, line, parser->lexer->filename);
                stmt->data.directive.name = str_dup("org");

                ExprParser expr_parser;
                expr_parser_init_with_token(&expr_parser, parser->lexer, parser->current);
                Expr *arg = expr_parse(&expr_parser);
                parser->current = expr_parser.current;

                if (arg) {
                    stmt->data.directive.args = malloc(sizeof(Expr *));
                    stmt->data.directive.args[0] = arg;
                    stmt->data.directive.arg_count = 1;
                }
            } else {
                stmt = statement_new(STMT_ERROR, line, parser->lexer->filename);
                stmt->error_msg = str_dup("expected '=' after '*'");
            }
        }
    }

done:
    /* Create label-only statement if we have a label but no other statement */
    if (label && !stmt) {
        stmt = statement_new(STMT_LABEL, line, parser->lexer->filename);
    }

    /* Attach label to statement */
    if (stmt && label) {
        stmt->label = label;
    } else {
        label_free(label);
    }

    /* If still no statement, create empty */
    if (!stmt) {
        stmt = statement_new(STMT_EMPTY, line, parser->lexer->filename);
    }

    /* Consume rest of line */
    while (!at_line_end(parser)) {
        advance(parser);
    }
    if (check(parser, TOK_EOL)) advance(parser);

    return stmt;
}

/* ========== Debug Printing ========== */

static const char *stmt_type_name(StatementType type) {
    switch (type) {
        case STMT_EMPTY: return "EMPTY";
        case STMT_LABEL: return "LABEL";
        case STMT_INSTRUCTION: return "INSTRUCTION";
        case STMT_DIRECTIVE: return "DIRECTIVE";
        case STMT_ASSIGNMENT: return "ASSIGNMENT";
        case STMT_MACRO_CALL: return "MACRO_CALL";
        case STMT_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void statement_print(Statement *stmt) {
    if (!stmt) {
        printf("(null statement)\n");
        return;
    }

    printf("Line %d: %s", stmt->line, stmt_type_name(stmt->type));

    if (stmt->label) {
        printf(" [label: %s%s]",
               stmt->label->is_local ? "." : "",
               stmt->label->name);
    }

    switch (stmt->type) {
        case STMT_INSTRUCTION:
            printf(" %s mode=%d size=%d",
                   stmt->data.instruction.mnemonic,
                   stmt->data.instruction.mode,
                   stmt->data.instruction.size);
            break;

        case STMT_DIRECTIVE:
            printf(" !%s (%d args)",
                   stmt->data.directive.name,
                   stmt->data.directive.arg_count);
            if (stmt->data.directive.string_arg) {
                printf(" \"%s\"", stmt->data.directive.string_arg);
            }
            break;

        case STMT_ASSIGNMENT:
            printf(" %s = ...", stmt->data.assignment.name);
            break;

        case STMT_MACRO_CALL:
            printf(" +%s (%d args)",
                   stmt->data.macro_call.name,
                   stmt->data.macro_call.arg_count);
            break;

        case STMT_ERROR:
            printf(" %s", stmt->error_msg ? stmt->error_msg : "(unknown error)");
            break;

        default:
            break;
    }

    printf("\n");
}
