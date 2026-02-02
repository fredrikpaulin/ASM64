/*
 * test_expr.c - Unit tests for expression parser and evaluator
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "expr.h"
#include "lexer.h"
#include "symbols.h"

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

/* Helper to parse and evaluate an expression string */
static ExprResult parse_eval(const char *text, SymbolTable *symbols, uint16_t pc) {
    Lexer lexer;
    ExprParser parser;
    lexer_init(&lexer, text, "test");
    expr_parser_init(&parser, &lexer);
    Expr *expr = expr_parse(&parser);
    ExprResult result = { 0, 0, 0 };
    if (expr) {
        result = expr_eval(expr, symbols, NULL, pc, 2, NULL);
        expr_free(expr);
    }
    return result;
}

/* Helper to just get the value */
static int32_t eval(const char *text) {
    ExprResult r = parse_eval(text, NULL, 0);
    return r.value;
}

/* Helper with symbols */
static int32_t eval_sym(const char *text, SymbolTable *symbols) {
    ExprResult r = parse_eval(text, symbols, 0x1000);
    return r.value;
}

/* ========== Basic Number Tests ========== */

TEST(number_decimal) {
    return eval("123") == 123;
}

TEST(number_zero) {
    return eval("0") == 0;
}

TEST(number_hex) {
    return eval("$FF") == 255;
}

TEST(number_hex_large) {
    return eval("$FFFF") == 65535;
}

TEST(number_binary) {
    return eval("%10101010") == 170;
}

TEST(number_char) {
    return eval("'A'") == 65;
}

/* ========== Arithmetic Tests ========== */

TEST(add_simple) {
    return eval("10 + 20") == 30;
}

TEST(sub_simple) {
    return eval("100 - 30") == 70;
}

TEST(mul_simple) {
    return eval("6 * 7") == 42;
}

TEST(div_simple) {
    return eval("100 / 4") == 25;
}

TEST(mod_simple) {
    return eval("17 % 5") == 2;
}

TEST(add_multiple) {
    return eval("1 + 2 + 3 + 4") == 10;
}

TEST(mixed_add_sub) {
    return eval("10 + 5 - 3") == 12;
}

TEST(mixed_mul_div) {
    return eval("24 / 4 * 2") == 12;
}

/* ========== Operator Precedence Tests ========== */

TEST(precedence_add_mul) {
    return eval("2 + 3 * 4") == 14;  /* 2 + (3*4) = 14 */
}

TEST(precedence_mul_add) {
    return eval("3 * 4 + 2") == 14;  /* (3*4) + 2 = 14 */
}

TEST(precedence_sub_mul) {
    return eval("10 - 2 * 3") == 4;  /* 10 - (2*3) = 4 */
}

TEST(precedence_parens) {
    return eval("(2 + 3) * 4") == 20;
}

TEST(precedence_nested_parens) {
    return eval("((1 + 2) * (3 + 4))") == 21;
}

TEST(precedence_shift_add) {
    return eval("1 + 2 << 3") == 24;  /* (1+2) << 3 = 24 */
}

TEST(precedence_and_or) {
    return eval("$F0 | $0F & $FF") == 0xFF;  /* $F0 | ($0F & $FF) */
}

/* ========== Bitwise Operator Tests ========== */

TEST(bitwise_and) {
    return eval("$FF & $0F") == 0x0F;
}

TEST(bitwise_or) {
    return eval("$F0 | $0F") == 0xFF;
}

TEST(bitwise_xor) {
    return eval("$FF ^ $0F") == 0xF0;
}

TEST(shift_left) {
    return eval("1 << 4") == 16;
}

TEST(shift_right) {
    return eval("$80 >> 4") == 8;
}

TEST(shift_left_multiple) {
    return eval("$01 << 8") == 256;
}

/* ========== Unary Operator Tests ========== */

TEST(unary_negate) {
    return eval("-10") == -10;
}

TEST(unary_negate_expr) {
    return eval("-(5 + 3)") == -8;
}

TEST(unary_complement) {
    return eval("~$00") == -1;  /* All bits set (signed) */
}

TEST(unary_not_zero) {
    return eval("!0") == 1;
}

TEST(unary_not_nonzero) {
    return eval("!42") == 0;
}

TEST(unary_low_byte) {
    return eval("<$1234") == 0x34;
}

TEST(unary_high_byte) {
    return eval(">$1234") == 0x12;
}

TEST(unary_low_byte_small) {
    return eval("<$FF") == 0xFF;
}

TEST(unary_high_byte_small) {
    return eval(">$FF") == 0x00;
}

TEST(unary_combined) {
    return eval("<($1000 + $234)") == 0x34;  /* <$1234 */
}

TEST(unary_high_combined) {
    return eval(">($1000 + $234)") == 0x12;  /* >$1234 */
}

/* ========== Comparison Operator Tests ========== */

TEST(cmp_equal_true) {
    return eval("5 = 5") == 1;
}

TEST(cmp_equal_false) {
    return eval("5 = 6") == 0;
}

TEST(cmp_not_equal_true) {
    return eval("5 <> 6") == 1;
}

TEST(cmp_not_equal_false) {
    return eval("5 <> 5") == 0;
}

TEST(cmp_less_than) {
    return eval("3 < 5") == 1 && eval("5 < 3") == 0;
}

TEST(cmp_greater_than) {
    return eval("5 > 3") == 1 && eval("3 > 5") == 0;
}

TEST(cmp_less_equal) {
    return eval("3 <= 5") == 1 && eval("5 <= 5") == 1 && eval("6 <= 5") == 0;
}

TEST(cmp_greater_equal) {
    return eval("5 >= 3") == 1 && eval("5 >= 5") == 1 && eval("4 >= 5") == 0;
}

/* ========== Symbol Tests ========== */

TEST(symbol_lookup) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "VALUE", 42, SYM_NONE, "test", 1);
    int32_t result = eval_sym("VALUE", table);
    symbol_table_free(table);
    return result == 42;
}

TEST(symbol_in_expr) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "BASE", 0x1000, SYM_NONE, "test", 1);
    symbol_define(table, "OFFSET", 0x100, SYM_NONE, "test", 2);
    int32_t result = eval_sym("BASE + OFFSET", table);
    symbol_table_free(table);
    return result == 0x1100;
}

TEST(symbol_low_byte) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "ADDR", 0x1234, SYM_NONE, "test", 1);
    int32_t result = eval_sym("<ADDR", table);
    symbol_table_free(table);
    return result == 0x34;
}

TEST(symbol_high_byte) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "ADDR", 0x1234, SYM_NONE, "test", 1);
    int32_t result = eval_sym(">ADDR", table);
    symbol_table_free(table);
    return result == 0x12;
}

TEST(symbol_case_insensitive) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "MyLabel", 100, SYM_NONE, "test", 1);
    int32_t r1 = eval_sym("mylabel", table);
    int32_t r2 = eval_sym("MYLABEL", table);
    int32_t r3 = eval_sym("MyLabel", table);
    symbol_table_free(table);
    return r1 == 100 && r2 == 100 && r3 == 100;
}

/* ========== Program Counter Tests ========== */

TEST(pc_reference) {
    ExprResult r = parse_eval("*", NULL, 0x0810);
    return r.value == 0x0810;
}

TEST(pc_in_expr) {
    ExprResult r = parse_eval("* + 2", NULL, 0x0810);
    return r.value == 0x0812;
}

TEST(pc_relative) {
    ExprResult r = parse_eval("* - $0800", NULL, 0x0810);
    return r.value == 0x10;
}

/* ========== Undefined Symbol Tests ========== */

TEST(undefined_symbol_pass1) {
    Lexer lexer;
    ExprParser parser;
    lexer_init(&lexer, "UNDEFINED", "test");
    expr_parser_init(&parser, &lexer);
    Expr *expr = expr_parse(&parser);
    ExprResult result = expr_eval(expr, NULL, NULL, 0, 1, NULL);  /* Pass 1 */
    expr_free(expr);
    return result.defined == 0;  /* Should be marked undefined */
}

TEST(undefined_in_expr_pass1) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "KNOWN", 100, SYM_NONE, "test", 1);

    Lexer lexer;
    ExprParser parser;
    lexer_init(&lexer, "KNOWN + UNKNOWN", "test");
    expr_parser_init(&parser, &lexer);
    Expr *expr = expr_parse(&parser);
    ExprResult result = expr_eval(expr, table, NULL, 0, 1, NULL);
    expr_free(expr);
    symbol_table_free(table);

    return result.defined == 0;  /* One undefined makes whole expr undefined */
}

/* ========== Zero Page Detection Tests ========== */

TEST(zeropage_number) {
    ExprResult r = parse_eval("$80", NULL, 0);
    return r.is_zeropage == 1;
}

TEST(zeropage_overflow) {
    ExprResult r = parse_eval("$100", NULL, 0);
    return r.is_zeropage == 0;
}

TEST(zeropage_low_byte) {
    ExprResult r = parse_eval("<$1234", NULL, 0);
    return r.is_zeropage == 1;  /* Low byte always fits */
}

TEST(zeropage_high_byte) {
    ExprResult r = parse_eval(">$1234", NULL, 0);
    return r.is_zeropage == 1;  /* High byte always fits */
}

/* ========== Expression Structure Tests ========== */

TEST(has_symbols_number) {
    Expr *e = expr_number(42);
    int result = expr_has_symbols(e);
    expr_free(e);
    return result == 0;
}

TEST(has_symbols_symbol) {
    Expr *e = expr_symbol("TEST");
    int result = expr_has_symbols(e);
    expr_free(e);
    return result == 1;
}

TEST(has_symbols_binary) {
    Expr *e = expr_binary(BINARY_ADD, expr_number(1), expr_symbol("X"));
    int result = expr_has_symbols(e);
    expr_free(e);
    return result == 1;
}

TEST(is_simple_number) {
    Expr *e1 = expr_number(42);
    Expr *e2 = expr_binary(BINARY_ADD, expr_number(1), expr_number(2));
    int r1 = expr_is_simple_number(e1);
    int r2 = expr_is_simple_number(e2);
    expr_free(e1);
    expr_free(e2);
    return r1 == 1 && r2 == 0;
}

/* ========== Edge Cases ========== */

TEST(division_by_zero) {
    /* Should not crash, returns 0 */
    return eval("10 / 0") == 0;
}

TEST(modulo_by_zero) {
    return eval("10 % 0") == 0;
}

TEST(complex_expression) {
    /* (($10 + $20) << 2) & $FF */
    return eval("(($10 + $20) << 2) & $FF") == 0xC0;
}

TEST(deeply_nested) {
    return eval("((((1 + 2) + 3) + 4) + 5)") == 15;
}

TEST(negative_result) {
    return eval("10 - 20") == -10;
}

TEST(hex_arithmetic) {
    return eval("$D020 + 1") == 0xD021;
}

/* ========== Main ========== */

int main(void) {
    printf("\nExpression Evaluator Tests\n");
    printf("==========================\n\n");

    printf("Basic Numbers:\n");
    RUN_TEST(number_decimal);
    RUN_TEST(number_zero);
    RUN_TEST(number_hex);
    RUN_TEST(number_hex_large);
    RUN_TEST(number_binary);
    RUN_TEST(number_char);

    printf("\nArithmetic:\n");
    RUN_TEST(add_simple);
    RUN_TEST(sub_simple);
    RUN_TEST(mul_simple);
    RUN_TEST(div_simple);
    RUN_TEST(mod_simple);
    RUN_TEST(add_multiple);
    RUN_TEST(mixed_add_sub);
    RUN_TEST(mixed_mul_div);

    printf("\nOperator Precedence:\n");
    RUN_TEST(precedence_add_mul);
    RUN_TEST(precedence_mul_add);
    RUN_TEST(precedence_sub_mul);
    RUN_TEST(precedence_parens);
    RUN_TEST(precedence_nested_parens);
    RUN_TEST(precedence_shift_add);
    RUN_TEST(precedence_and_or);

    printf("\nBitwise Operators:\n");
    RUN_TEST(bitwise_and);
    RUN_TEST(bitwise_or);
    RUN_TEST(bitwise_xor);
    RUN_TEST(shift_left);
    RUN_TEST(shift_right);
    RUN_TEST(shift_left_multiple);

    printf("\nUnary Operators:\n");
    RUN_TEST(unary_negate);
    RUN_TEST(unary_negate_expr);
    RUN_TEST(unary_complement);
    RUN_TEST(unary_not_zero);
    RUN_TEST(unary_not_nonzero);
    RUN_TEST(unary_low_byte);
    RUN_TEST(unary_high_byte);
    RUN_TEST(unary_low_byte_small);
    RUN_TEST(unary_high_byte_small);
    RUN_TEST(unary_combined);
    RUN_TEST(unary_high_combined);

    printf("\nComparison Operators:\n");
    RUN_TEST(cmp_equal_true);
    RUN_TEST(cmp_equal_false);
    RUN_TEST(cmp_not_equal_true);
    RUN_TEST(cmp_not_equal_false);
    RUN_TEST(cmp_less_than);
    RUN_TEST(cmp_greater_than);
    RUN_TEST(cmp_less_equal);
    RUN_TEST(cmp_greater_equal);

    printf("\nSymbol Handling:\n");
    RUN_TEST(symbol_lookup);
    RUN_TEST(symbol_in_expr);
    RUN_TEST(symbol_low_byte);
    RUN_TEST(symbol_high_byte);
    RUN_TEST(symbol_case_insensitive);

    printf("\nProgram Counter:\n");
    RUN_TEST(pc_reference);
    RUN_TEST(pc_in_expr);
    RUN_TEST(pc_relative);

    printf("\nUndefined Symbols:\n");
    RUN_TEST(undefined_symbol_pass1);
    RUN_TEST(undefined_in_expr_pass1);

    printf("\nZero Page Detection:\n");
    RUN_TEST(zeropage_number);
    RUN_TEST(zeropage_overflow);
    RUN_TEST(zeropage_low_byte);
    RUN_TEST(zeropage_high_byte);

    printf("\nExpression Structure:\n");
    RUN_TEST(has_symbols_number);
    RUN_TEST(has_symbols_symbol);
    RUN_TEST(has_symbols_binary);
    RUN_TEST(is_simple_number);

    printf("\nEdge Cases:\n");
    RUN_TEST(division_by_zero);
    RUN_TEST(modulo_by_zero);
    RUN_TEST(complex_expression);
    RUN_TEST(deeply_nested);
    RUN_TEST(negative_result);
    RUN_TEST(hex_arithmetic);

    printf("\n==========================\n");
    printf("Results: %d/%d passed\n\n", tests_passed, tests_run);

    return tests_passed == tests_run ? 0 : 1;
}
