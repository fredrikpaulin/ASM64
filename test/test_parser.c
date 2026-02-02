/*
 * test_parser.c - Unit tests for statement parser
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "lexer.h"
#include "symbols.h"
#include "opcodes.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) static int test_##name(void)
#define RUN_TEST(name) do { \
    tests_run++; \
    printf("  %-40s ", #name); \
    if (test_##name()) { \
        tests_passed++; \
        printf("\033[0;32mPASS\033[0m\n"); \
    } else { \
        printf("\033[0;31mFAIL\033[0m\n"); \
    } \
} while(0)

/* Helper to parse a single line */
static Statement *parse_line(const char *text) {
    Lexer lexer;
    Parser parser;
    lexer_init(&lexer, text, "test");
    parser_init(&parser, &lexer, NULL);
    return parser_parse_line(&parser);
}

/* Helper with symbol table */
static Statement *parse_line_sym(const char *text, SymbolTable *symbols) {
    Lexer lexer;
    Parser parser;
    lexer_init(&lexer, text, "test");
    parser_init(&parser, &lexer, symbols);
    return parser_parse_line(&parser);
}

/* ========== Empty Line Tests ========== */

TEST(empty_line) {
    Statement *stmt = parse_line("");
    int ok = stmt && stmt->type == STMT_EMPTY;
    statement_free(stmt);
    return ok;
}

TEST(comment_only) {
    Statement *stmt = parse_line("; this is a comment");
    int ok = stmt && stmt->type == STMT_EMPTY;
    statement_free(stmt);
    return ok;
}

/* ========== Label Tests ========== */

TEST(label_only) {
    Statement *stmt = parse_line("MyLabel:");
    int ok = stmt && stmt->type == STMT_LABEL &&
             stmt->label && strcmp(stmt->label->name, "MyLabel") == 0 &&
             !stmt->label->is_local;
    statement_free(stmt);
    return ok;
}

TEST(label_no_colon) {
    Statement *stmt = parse_line("MyLabel");
    int ok = stmt && stmt->label && strcmp(stmt->label->name, "MyLabel") == 0;
    statement_free(stmt);
    return ok;
}

TEST(local_label) {
    Statement *stmt = parse_line(".loop:");
    int ok = stmt && stmt->label &&
             strcmp(stmt->label->name, ".loop") == 0 &&
             stmt->label->is_local;
    statement_free(stmt);
    return ok;
}

TEST(label_with_instruction) {
    Statement *stmt = parse_line("Start: LDA #$00");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->label && strcmp(stmt->label->name, "Start") == 0 &&
             strcmp(stmt->data.instruction.mnemonic, "LDA") == 0;
    statement_free(stmt);
    return ok;
}

/* ========== Instruction Tests - Implied ========== */

TEST(instr_implied_inx) {
    Statement *stmt = parse_line("INX");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             strcmp(stmt->data.instruction.mnemonic, "INX") == 0 &&
             stmt->data.instruction.mode == ADDR_IMPLIED &&
             stmt->data.instruction.size == 1;
    statement_free(stmt);
    return ok;
}

TEST(instr_implied_rts) {
    Statement *stmt = parse_line("RTS");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             strcmp(stmt->data.instruction.mnemonic, "RTS") == 0 &&
             stmt->data.instruction.mode == ADDR_IMPLIED;
    statement_free(stmt);
    return ok;
}

TEST(instr_implied_nop) {
    Statement *stmt = parse_line("NOP");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.opcode == 0xEA;
    statement_free(stmt);
    return ok;
}

/* ========== Instruction Tests - Immediate ========== */

TEST(instr_immediate_lda) {
    Statement *stmt = parse_line("LDA #$10");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_IMMEDIATE &&
             stmt->data.instruction.opcode == 0xA9 &&
             stmt->data.instruction.size == 2;
    statement_free(stmt);
    return ok;
}

TEST(instr_immediate_ldx) {
    Statement *stmt = parse_line("LDX #$FF");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_IMMEDIATE &&
             stmt->data.instruction.opcode == 0xA2;
    statement_free(stmt);
    return ok;
}

TEST(instr_immediate_cmp) {
    Statement *stmt = parse_line("CMP #0");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_IMMEDIATE;
    statement_free(stmt);
    return ok;
}

/* ========== Instruction Tests - Zero Page ========== */

TEST(instr_zeropage_lda) {
    Statement *stmt = parse_line("LDA $80");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ZEROPAGE &&
             stmt->data.instruction.opcode == 0xA5 &&
             stmt->data.instruction.size == 2;
    statement_free(stmt);
    return ok;
}

TEST(instr_zeropage_sta) {
    Statement *stmt = parse_line("STA $02");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ZEROPAGE &&
             stmt->data.instruction.opcode == 0x85;
    statement_free(stmt);
    return ok;
}

TEST(instr_zeropage_x) {
    Statement *stmt = parse_line("LDA $80,X");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ZEROPAGE_X &&
             stmt->data.instruction.opcode == 0xB5;
    statement_free(stmt);
    return ok;
}

TEST(instr_zeropage_y) {
    Statement *stmt = parse_line("LDX $80,Y");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ZEROPAGE_Y &&
             stmt->data.instruction.opcode == 0xB6;
    statement_free(stmt);
    return ok;
}

/* ========== Instruction Tests - Absolute ========== */

TEST(instr_absolute_lda) {
    Statement *stmt = parse_line("LDA $1000");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ABSOLUTE &&
             stmt->data.instruction.opcode == 0xAD &&
             stmt->data.instruction.size == 3;
    statement_free(stmt);
    return ok;
}

TEST(instr_absolute_sta) {
    Statement *stmt = parse_line("STA $D020");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ABSOLUTE;
    statement_free(stmt);
    return ok;
}

TEST(instr_absolute_x) {
    Statement *stmt = parse_line("LDA $1000,X");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ABSOLUTE_X &&
             stmt->data.instruction.opcode == 0xBD;
    statement_free(stmt);
    return ok;
}

TEST(instr_absolute_y) {
    Statement *stmt = parse_line("LDA $1000,Y");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ABSOLUTE_Y &&
             stmt->data.instruction.opcode == 0xB9;
    statement_free(stmt);
    return ok;
}

/* ========== Instruction Tests - Indirect ========== */

TEST(instr_indirect_jmp) {
    Statement *stmt = parse_line("JMP ($FFFC)");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_INDIRECT &&
             stmt->data.instruction.opcode == 0x6C;
    statement_free(stmt);
    return ok;
}

TEST(instr_indirect_x) {
    Statement *stmt = parse_line("LDA ($80,X)");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_INDIRECT_X &&
             stmt->data.instruction.opcode == 0xA1;
    statement_free(stmt);
    return ok;
}

TEST(instr_indirect_y) {
    Statement *stmt = parse_line("LDA ($80),Y");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_INDIRECT_Y &&
             stmt->data.instruction.opcode == 0xB1;
    statement_free(stmt);
    return ok;
}

/* ========== Instruction Tests - Accumulator ========== */

TEST(instr_accumulator_asl) {
    Statement *stmt = parse_line("ASL A");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ACCUMULATOR &&
             stmt->data.instruction.opcode == 0x0A;
    statement_free(stmt);
    return ok;
}

TEST(instr_accumulator_implied) {
    Statement *stmt = parse_line("ASL");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ACCUMULATOR;
    statement_free(stmt);
    return ok;
}

TEST(instr_accumulator_ror) {
    Statement *stmt = parse_line("ROR A");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ACCUMULATOR;
    statement_free(stmt);
    return ok;
}

/* ========== Instruction Tests - Branches ========== */

TEST(instr_branch_bne) {
    Statement *stmt = parse_line("BNE $1000");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_RELATIVE &&
             stmt->data.instruction.opcode == 0xD0 &&
             stmt->data.instruction.size == 2;
    statement_free(stmt);
    return ok;
}

TEST(instr_branch_beq) {
    Statement *stmt = parse_line("BEQ label");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_RELATIVE;
    statement_free(stmt);
    return ok;
}

TEST(instr_branch_bcc) {
    Statement *stmt = parse_line("BCC $08");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_RELATIVE &&
             stmt->data.instruction.opcode == 0x90;
    statement_free(stmt);
    return ok;
}

/* ========== Instruction Tests - Jump ========== */

TEST(instr_jmp_absolute) {
    Statement *stmt = parse_line("JMP $1000");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ABSOLUTE &&
             stmt->data.instruction.opcode == 0x4C;
    statement_free(stmt);
    return ok;
}

TEST(instr_jsr_absolute) {
    Statement *stmt = parse_line("JSR $2000");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ABSOLUTE &&
             stmt->data.instruction.opcode == 0x20;
    statement_free(stmt);
    return ok;
}

/* ========== Directive Tests ========== */

TEST(directive_byte) {
    Statement *stmt = parse_line("!byte $01, $02, $03");
    int ok = stmt && stmt->type == STMT_DIRECTIVE &&
             strcmp(stmt->data.directive.name, "byte") == 0 &&
             stmt->data.directive.arg_count == 3;
    statement_free(stmt);
    return ok;
}

TEST(directive_word) {
    Statement *stmt = parse_line("!word $1234");
    int ok = stmt && stmt->type == STMT_DIRECTIVE &&
             strcmp(stmt->data.directive.name, "word") == 0 &&
             stmt->data.directive.arg_count == 1;
    statement_free(stmt);
    return ok;
}

TEST(directive_text) {
    Statement *stmt = parse_line("!text \"Hello\"");
    int ok = stmt && stmt->type == STMT_DIRECTIVE &&
             strcmp(stmt->data.directive.name, "text") == 0 &&
             stmt->data.directive.string_arg &&
             strcmp(stmt->data.directive.string_arg, "Hello") == 0;
    statement_free(stmt);
    return ok;
}

TEST(directive_fill) {
    Statement *stmt = parse_line("!fill 10, $EA");
    int ok = stmt && stmt->type == STMT_DIRECTIVE &&
             strcmp(stmt->data.directive.name, "fill") == 0 &&
             stmt->data.directive.arg_count == 2;
    statement_free(stmt);
    return ok;
}

/* ========== Origin Directive ========== */

TEST(origin_directive) {
    Statement *stmt = parse_line("*=$0801");
    int ok = stmt && stmt->type == STMT_DIRECTIVE &&
             strcmp(stmt->data.directive.name, "org") == 0 &&
             stmt->data.directive.arg_count == 1;
    statement_free(stmt);
    return ok;
}

TEST(origin_with_expression) {
    Statement *stmt = parse_line("*= $0800 + $100");
    int ok = stmt && stmt->type == STMT_DIRECTIVE &&
             strcmp(stmt->data.directive.name, "org") == 0;
    statement_free(stmt);
    return ok;
}

/* ========== Assignment Tests ========== */

TEST(assignment_simple) {
    Statement *stmt = parse_line("VALUE = $10");
    int ok = stmt && stmt->type == STMT_ASSIGNMENT &&
             strcmp(stmt->data.assignment.name, "VALUE") == 0 &&
             stmt->data.assignment.value != NULL;
    statement_free(stmt);
    return ok;
}

TEST(assignment_expression) {
    Statement *stmt = parse_line("OFFSET = BASE + $100");
    int ok = stmt && stmt->type == STMT_ASSIGNMENT &&
             strcmp(stmt->data.assignment.name, "OFFSET") == 0;
    statement_free(stmt);
    return ok;
}

/* ========== Symbol Resolution Tests ========== */

TEST(symbol_zeropage) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "ZPVAR", 0x80, SYM_ZEROPAGE, "test", 1);

    Statement *stmt = parse_line_sym("LDA ZPVAR", table);
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ZEROPAGE;

    statement_free(stmt);
    symbol_table_free(table);
    return ok;
}

TEST(symbol_absolute) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "ADDR", 0x1000, SYM_NONE, "test", 1);

    Statement *stmt = parse_line_sym("LDA ADDR", table);
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ABSOLUTE;

    statement_free(stmt);
    symbol_table_free(table);
    return ok;
}

/* ========== Case Insensitivity Tests ========== */

TEST(mnemonic_lowercase) {
    Statement *stmt = parse_line("lda #$00");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_IMMEDIATE;
    statement_free(stmt);
    return ok;
}

TEST(mnemonic_mixedcase) {
    Statement *stmt = parse_line("Lda #$00");
    int ok = stmt && stmt->type == STMT_INSTRUCTION;
    statement_free(stmt);
    return ok;
}

TEST(register_lowercase) {
    Statement *stmt = parse_line("LDA $80,x");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.mode == ADDR_ZEROPAGE_X;
    statement_free(stmt);
    return ok;
}

/* ========== Illegal Opcode Tests ========== */

TEST(illegal_lax) {
    Statement *stmt = parse_line("LAX $80");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.opcode == 0xA7;
    statement_free(stmt);
    return ok;
}

TEST(illegal_sax) {
    Statement *stmt = parse_line("SAX $80");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.opcode == 0x87;
    statement_free(stmt);
    return ok;
}

/* ========== Cycle Count Tests ========== */

TEST(cycles_immediate) {
    Statement *stmt = parse_line("LDA #$00");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.cycles == 2;
    statement_free(stmt);
    return ok;
}

TEST(cycles_zeropage) {
    Statement *stmt = parse_line("LDA $80");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.cycles == 3;
    statement_free(stmt);
    return ok;
}

TEST(cycles_absolute) {
    Statement *stmt = parse_line("LDA $1000");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.cycles == 4;
    statement_free(stmt);
    return ok;
}

TEST(cycles_page_penalty) {
    Statement *stmt = parse_line("LDA $1000,X");
    int ok = stmt && stmt->type == STMT_INSTRUCTION &&
             stmt->data.instruction.page_penalty == 1;
    statement_free(stmt);
    return ok;
}

/* ========== Main ========== */

int main(void) {
    printf("\nParser Tests\n");
    printf("============\n\n");

    /* Initialize opcode table */
    opcodes_init();

    printf("Empty Lines:\n");
    RUN_TEST(empty_line);
    RUN_TEST(comment_only);

    printf("\nLabels:\n");
    RUN_TEST(label_only);
    RUN_TEST(label_no_colon);
    RUN_TEST(local_label);
    RUN_TEST(label_with_instruction);

    printf("\nImplied Mode:\n");
    RUN_TEST(instr_implied_inx);
    RUN_TEST(instr_implied_rts);
    RUN_TEST(instr_implied_nop);

    printf("\nImmediate Mode:\n");
    RUN_TEST(instr_immediate_lda);
    RUN_TEST(instr_immediate_ldx);
    RUN_TEST(instr_immediate_cmp);

    printf("\nZero Page Mode:\n");
    RUN_TEST(instr_zeropage_lda);
    RUN_TEST(instr_zeropage_sta);
    RUN_TEST(instr_zeropage_x);
    RUN_TEST(instr_zeropage_y);

    printf("\nAbsolute Mode:\n");
    RUN_TEST(instr_absolute_lda);
    RUN_TEST(instr_absolute_sta);
    RUN_TEST(instr_absolute_x);
    RUN_TEST(instr_absolute_y);

    printf("\nIndirect Mode:\n");
    RUN_TEST(instr_indirect_jmp);
    RUN_TEST(instr_indirect_x);
    RUN_TEST(instr_indirect_y);

    printf("\nAccumulator Mode:\n");
    RUN_TEST(instr_accumulator_asl);
    RUN_TEST(instr_accumulator_implied);
    RUN_TEST(instr_accumulator_ror);

    printf("\nBranch Instructions:\n");
    RUN_TEST(instr_branch_bne);
    RUN_TEST(instr_branch_beq);
    RUN_TEST(instr_branch_bcc);

    printf("\nJump Instructions:\n");
    RUN_TEST(instr_jmp_absolute);
    RUN_TEST(instr_jsr_absolute);

    printf("\nDirectives:\n");
    RUN_TEST(directive_byte);
    RUN_TEST(directive_word);
    RUN_TEST(directive_text);
    RUN_TEST(directive_fill);

    printf("\nOrigin Directive:\n");
    RUN_TEST(origin_directive);
    RUN_TEST(origin_with_expression);

    printf("\nAssignments:\n");
    RUN_TEST(assignment_simple);
    RUN_TEST(assignment_expression);

    printf("\nSymbol Resolution:\n");
    RUN_TEST(symbol_zeropage);
    RUN_TEST(symbol_absolute);

    printf("\nCase Insensitivity:\n");
    RUN_TEST(mnemonic_lowercase);
    RUN_TEST(mnemonic_mixedcase);
    RUN_TEST(register_lowercase);

    printf("\nIllegal Opcodes:\n");
    RUN_TEST(illegal_lax);
    RUN_TEST(illegal_sax);

    printf("\nCycle Counts:\n");
    RUN_TEST(cycles_immediate);
    RUN_TEST(cycles_zeropage);
    RUN_TEST(cycles_absolute);
    RUN_TEST(cycles_page_penalty);

    printf("\n============\n");
    printf("Results: %d/%d passed\n\n", tests_passed, tests_run);

    return tests_passed == tests_run ? 0 : 1;
}
