/*
 * test_loop.c - Unit tests for assembly-time loops
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Test macros */
#define PASS() do { printf("[PASS]\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("[FAIL]\n    %s\n    At %s:%d\n", msg, __FILE__, __LINE__); tests_failed++; } while(0)
#define ASSERT(cond) do { if (!(cond)) { FAIL("Assertion failed: " #cond); return; } } while(0)

/* Helper to assemble and check output bytes */
static int assemble_and_check(const char *source, const uint8_t *expected, int expected_size) {
    Assembler *as = assembler_create();
    if (!as) return 0;

    int errors = assembler_assemble_string(as, source, "test.asm");
    if (errors != 0) {
        assembler_free(as);
        return 0;
    }

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    int match = (size == expected_size);
    if (match) {
        for (int i = 0; i < size; i++) {
            if (output[i] != expected[i]) {
                match = 0;
                break;
            }
        }
    }

    assembler_free(as);
    return match;
}

/* Helper to check if assembly produces errors */
static int assemble_has_error(const char *source) {
    Assembler *as = assembler_create();
    if (!as) return 1;

    int errors = assembler_assemble_string(as, source, "test.asm");
    assembler_free(as);
    return errors != 0;
}

/* ========== Basic For Loop Tests ========== */

static void test_for_simple(void) {
    printf("  for_simple                                     ");
    /* !for i, 0, 2 should iterate 3 times (0, 1, 2) */
    const char *source =
        "* = $1000\n"
        "!for i, 0, 2\n"
        "    nop\n"
        "!end\n";
    uint8_t expected[] = { 0xEA, 0xEA, 0xEA };  /* 3 NOPs */
    ASSERT(assemble_and_check(source, expected, 3));
    PASS();
}

static void test_for_with_variable(void) {
    printf("  for_with_variable                              ");
    /* Loop variable should be substituted in body */
    const char *source =
        "* = $1000\n"
        "!for i, 1, 3\n"
        "    lda #i\n"
        "!end\n";
    uint8_t expected[] = {
        0xA9, 0x01,  /* LDA #1 */
        0xA9, 0x02,  /* LDA #2 */
        0xA9, 0x03   /* LDA #3 */
    };
    ASSERT(assemble_and_check(source, expected, 6));
    PASS();
}

static void test_for_descending(void) {
    printf("  for_descending                                 ");
    /* Descending loop: 3 down to 1 */
    const char *source =
        "* = $1000\n"
        "!for i, 3, 1\n"
        "    lda #i\n"
        "!end\n";
    uint8_t expected[] = {
        0xA9, 0x03,  /* LDA #3 */
        0xA9, 0x02,  /* LDA #2 */
        0xA9, 0x01   /* LDA #1 */
    };
    ASSERT(assemble_and_check(source, expected, 6));
    PASS();
}

static void test_for_single_iteration(void) {
    printf("  for_single_iteration                           ");
    /* Loop from 5 to 5 should iterate once */
    const char *source =
        "* = $1000\n"
        "!for i, 5, 5\n"
        "    lda #i\n"
        "!end\n";
    uint8_t expected[] = { 0xA9, 0x05 };  /* LDA #5 */
    ASSERT(assemble_and_check(source, expected, 2));
    PASS();
}

static void test_for_expression_bounds(void) {
    printf("  for_expression_bounds                          ");
    /* Start and end can be expressions */
    const char *source =
        "* = $1000\n"
        "START = 2\n"
        "COUNT = 3\n"
        "!for i, START, START+COUNT-1\n"
        "    nop\n"
        "!end\n";
    uint8_t expected[] = { 0xEA, 0xEA, 0xEA };  /* 3 NOPs (i=2,3,4) */
    ASSERT(assemble_and_check(source, expected, 3));
    PASS();
}

static void test_for_var_in_expression(void) {
    printf("  for_var_in_expression                          ");
    /* Loop variable used in expression */
    const char *source =
        "* = $1000\n"
        "BASE = $10\n"
        "!for i, 0, 2\n"
        "    lda BASE+i\n"
        "!end\n";
    uint8_t expected[] = {
        0xA5, 0x10,  /* LDA $10 */
        0xA5, 0x11,  /* LDA $11 */
        0xA5, 0x12   /* LDA $12 */
    };
    ASSERT(assemble_and_check(source, expected, 6));
    PASS();
}

static void test_for_generates_table(void) {
    printf("  for_generates_table                            ");
    /* Generate a byte table */
    const char *source =
        "* = $1000\n"
        "!for i, 0, 3\n"
        "    !byte i*2\n"
        "!end\n";
    uint8_t expected[] = { 0x00, 0x02, 0x04, 0x06 };
    ASSERT(assemble_and_check(source, expected, 4));
    PASS();
}

/* ========== Nested Loop Tests ========== */

static void test_for_nested(void) {
    printf("  for_nested                                     ");
    /* Nested loops */
    const char *source =
        "* = $1000\n"
        "!for i, 0, 1\n"
        "    !for j, 0, 1\n"
        "        nop\n"
        "    !end\n"
        "!end\n";
    uint8_t expected[] = { 0xEA, 0xEA, 0xEA, 0xEA };  /* 2*2 = 4 NOPs */
    ASSERT(assemble_and_check(source, expected, 4));
    PASS();
}

static void test_for_nested_variables(void) {
    printf("  for_nested_variables                           ");
    /* Nested loops with variables */
    const char *source =
        "* = $1000\n"
        "!for i, 0, 1\n"
        "    !for j, 0, 1\n"
        "        !byte i*2+j\n"
        "    !end\n"
        "!end\n";
    uint8_t expected[] = { 0x00, 0x01, 0x02, 0x03 };  /* 0*2+0, 0*2+1, 1*2+0, 1*2+1 */
    ASSERT(assemble_and_check(source, expected, 4));
    PASS();
}

/* ========== While Loop Tests ========== */

static void test_while_simple(void) {
    printf("  while_simple                                   ");
    /* Simple while loop */
    const char *source =
        "* = $1000\n"
        "count = 3\n"
        "!while count > 0\n"
        "    nop\n"
        "    count = count - 1\n"
        "!end\n";
    uint8_t expected[] = { 0xEA, 0xEA, 0xEA };  /* 3 NOPs */
    ASSERT(assemble_and_check(source, expected, 3));
    PASS();
}

static void test_while_with_variable(void) {
    printf("  while_with_variable                            ");
    /* While loop generating values */
    const char *source =
        "* = $1000\n"
        "i = 1\n"
        "!while i <= 3\n"
        "    !byte i\n"
        "    i = i + 1\n"
        "!end\n";
    uint8_t expected[] = { 0x01, 0x02, 0x03 };
    ASSERT(assemble_and_check(source, expected, 3));
    PASS();
}

static void test_while_false_condition(void) {
    printf("  while_false_condition                          ");
    /* While with initially false condition should not execute */
    const char *source =
        "* = $1000\n"
        "!while 0\n"
        "    nop\n"
        "!end\n"
        "inx\n";
    uint8_t expected[] = { 0xE8 };  /* Just INX, no NOP */
    ASSERT(assemble_and_check(source, expected, 1));
    PASS();
}

/* ========== Loop with Conditionals ========== */

static void test_for_with_conditional(void) {
    printf("  for_with_conditional                           ");
    /* Conditional inside loop - use | for OR (bitwise works for 0/1 values) */
    const char *source =
        "* = $1000\n"
        "!for i, 0, 3\n"
        "    !if i = 1 | i = 2\n"
        "        nop\n"
        "    !endif\n"
        "!end\n";
    uint8_t expected[] = { 0xEA, 0xEA };  /* NOPs for i=1 and i=2 only */
    ASSERT(assemble_and_check(source, expected, 2));
    PASS();
}

/* ========== Error Cases ========== */

static void test_for_missing_end(void) {
    printf("  for_missing_end                                ");
    const char *source =
        "* = $1000\n"
        "!for i, 0, 2\n"
        "    nop\n";  /* Missing !end */
    ASSERT(assemble_has_error(source));
    PASS();
}

static void test_for_missing_args(void) {
    printf("  for_missing_args                               ");
    const char *source =
        "* = $1000\n"
        "!for i, 0\n"  /* Missing end value */
        "    nop\n"
        "!end\n";
    ASSERT(assemble_has_error(source));
    PASS();
}

static void test_while_missing_end(void) {
    printf("  while_missing_end                              ");
    const char *source =
        "* = $1000\n"
        "i = 1\n"
        "!while i\n"
        "    nop\n";  /* Missing !end */
    ASSERT(assemble_has_error(source));
    PASS();
}

/* ========== Test Runner ========== */

int main(void) {
    printf("\nLoop Tests\n");
    printf("==========\n\n");

    printf("Basic For Loops:\n");
    test_for_simple();
    test_for_with_variable();
    test_for_descending();
    test_for_single_iteration();
    test_for_expression_bounds();
    test_for_var_in_expression();
    test_for_generates_table();

    printf("\nNested Loops:\n");
    test_for_nested();
    test_for_nested_variables();

    printf("\nWhile Loops:\n");
    test_while_simple();
    test_while_with_variable();
    test_while_false_condition();

    printf("\nLoops with Conditionals:\n");
    test_for_with_conditional();

    printf("\nError Cases:\n");
    test_for_missing_end();
    test_for_missing_args();
    test_while_missing_end();

    printf("\n==========\n");
    printf("Total: %d passed, %d failed\n", tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
