/*
 * test_conditional.c - Tests for Conditional Assembly
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  %-50s ", #name); \
    fflush(stdout); \
    test_##name(); \
    tests_passed++; \
    printf("[PASS]\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("[FAIL]\n"); \
        printf("    Assertion failed: %s\n", #cond); \
        printf("    At %s:%d\n", __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("[FAIL]\n"); \
        printf("    Expected: %d, Got: %d\n", (int)(b), (int)(a)); \
        printf("    At %s:%d\n", __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

/* Helper to assemble and get output */
static int assemble_and_check(const char *source, uint8_t *expected, int expected_size) {
    Assembler *as = assembler_create();
    if (!as) return 0;

    int errors = assembler_assemble_string(as, source, "test.asm");
    if (errors > 0) {
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

/* ========== Basic !if Tests ========== */

TEST(if_true_includes_code) {
    /* !if 1 should include the code */
    const char *source =
        "* = $1000\n"
        "!if 1\n"
        "    lda #$42\n"
        "!endif\n";
    uint8_t expected[] = { 0xA9, 0x42 };  /* LDA #$42 */
    ASSERT(assemble_and_check(source, expected, 2));
}

TEST(if_false_excludes_code) {
    /* !if 0 should exclude the code */
    const char *source =
        "* = $1000\n"
        "!if 0\n"
        "    lda #$42\n"
        "!endif\n"
        "nop\n";
    uint8_t expected[] = { 0xEA };  /* Just NOP */
    ASSERT(assemble_and_check(source, expected, 1));
}

TEST(if_expression) {
    /* !if with an expression */
    const char *source =
        "* = $1000\n"
        "DEBUG = 1\n"
        "!if DEBUG\n"
        "    lda #$FF\n"
        "!endif\n";
    uint8_t expected[] = { 0xA9, 0xFF };
    ASSERT(assemble_and_check(source, expected, 2));
}

TEST(if_expression_false) {
    /* !if with expression evaluating to 0 */
    const char *source =
        "* = $1000\n"
        "DEBUG = 0\n"
        "!if DEBUG\n"
        "    lda #$FF\n"
        "!endif\n"
        "nop\n";
    uint8_t expected[] = { 0xEA };
    ASSERT(assemble_and_check(source, expected, 1));
}

TEST(if_comparison) {
    /* !if with comparison expression */
    const char *source =
        "* = $1000\n"
        "VALUE = 5\n"
        "!if VALUE > 3\n"
        "    lda #$01\n"
        "!endif\n";
    uint8_t expected[] = { 0xA9, 0x01 };
    ASSERT(assemble_and_check(source, expected, 2));
}

/* ========== !if / !else Tests ========== */

TEST(if_else_true_branch) {
    /* When condition is true, include first block */
    const char *source =
        "* = $1000\n"
        "!if 1\n"
        "    lda #$11\n"
        "!else\n"
        "    lda #$22\n"
        "!endif\n";
    uint8_t expected[] = { 0xA9, 0x11 };
    ASSERT(assemble_and_check(source, expected, 2));
}

TEST(if_else_false_branch) {
    /* When condition is false, include else block */
    const char *source =
        "* = $1000\n"
        "!if 0\n"
        "    lda #$11\n"
        "!else\n"
        "    lda #$22\n"
        "!endif\n";
    uint8_t expected[] = { 0xA9, 0x22 };
    ASSERT(assemble_and_check(source, expected, 2));
}

/* ========== !ifdef / !ifndef Tests ========== */

TEST(ifdef_defined) {
    /* !ifdef when symbol IS defined */
    const char *source =
        "* = $1000\n"
        "MYSYM = 1\n"
        "!ifdef MYSYM\n"
        "    lda #$AA\n"
        "!endif\n";
    uint8_t expected[] = { 0xA9, 0xAA };
    ASSERT(assemble_and_check(source, expected, 2));
}

TEST(ifdef_undefined) {
    /* !ifdef when symbol is NOT defined */
    const char *source =
        "* = $1000\n"
        "!ifdef UNDEFINED_SYM\n"
        "    lda #$AA\n"
        "!endif\n"
        "nop\n";
    uint8_t expected[] = { 0xEA };
    ASSERT(assemble_and_check(source, expected, 1));
}

TEST(ifndef_defined) {
    /* !ifndef when symbol IS defined (should skip) */
    const char *source =
        "* = $1000\n"
        "MYSYM = 1\n"
        "!ifndef MYSYM\n"
        "    lda #$AA\n"
        "!endif\n"
        "nop\n";
    uint8_t expected[] = { 0xEA };
    ASSERT(assemble_and_check(source, expected, 1));
}

TEST(ifndef_undefined) {
    /* !ifndef when symbol is NOT defined (should include) */
    const char *source =
        "* = $1000\n"
        "!ifndef UNDEFINED_SYM\n"
        "    lda #$BB\n"
        "!endif\n";
    uint8_t expected[] = { 0xA9, 0xBB };
    ASSERT(assemble_and_check(source, expected, 2));
}

/* ========== Nested Conditionals ========== */

TEST(nested_if_both_true) {
    const char *source =
        "* = $1000\n"
        "!if 1\n"
        "    lda #$01\n"
        "    !if 1\n"
        "        lda #$02\n"
        "    !endif\n"
        "    lda #$03\n"
        "!endif\n";
    uint8_t expected[] = { 0xA9, 0x01, 0xA9, 0x02, 0xA9, 0x03 };
    ASSERT(assemble_and_check(source, expected, 6));
}

TEST(nested_if_outer_false) {
    /* When outer is false, inner should also be skipped */
    const char *source =
        "* = $1000\n"
        "!if 0\n"
        "    lda #$01\n"
        "    !if 1\n"
        "        lda #$02\n"
        "    !endif\n"
        "    lda #$03\n"
        "!endif\n"
        "nop\n";
    uint8_t expected[] = { 0xEA };
    ASSERT(assemble_and_check(source, expected, 1));
}

TEST(nested_if_inner_false) {
    const char *source =
        "* = $1000\n"
        "!if 1\n"
        "    lda #$01\n"
        "    !if 0\n"
        "        lda #$02\n"
        "    !endif\n"
        "    lda #$03\n"
        "!endif\n";
    uint8_t expected[] = { 0xA9, 0x01, 0xA9, 0x03 };
    ASSERT(assemble_and_check(source, expected, 4));
}

TEST(nested_if_with_else) {
    const char *source =
        "* = $1000\n"
        "!if 1\n"
        "    !if 0\n"
        "        lda #$11\n"
        "    !else\n"
        "        lda #$22\n"
        "    !endif\n"
        "!else\n"
        "    lda #$33\n"
        "!endif\n";
    uint8_t expected[] = { 0xA9, 0x22 };
    ASSERT(assemble_and_check(source, expected, 2));
}

/* ========== ACME-style syntax ========== */

TEST(if_line_based) {
    /* Line-based !if / !endif syntax */
    const char *source =
        "* = $1000\n"
        "!if 1\n"
        "    lda #$42\n"
        "!endif\n";
    uint8_t expected[] = { 0xA9, 0x42 };
    ASSERT(assemble_and_check(source, expected, 2));
}

/* ========== Labels in Conditionals ========== */

TEST(label_in_true_block) {
    Assembler *as = assembler_create();
    const char *source =
        "* = $1000\n"
        "!if 1\n"
        "label: nop\n"
        "!endif\n";
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT_EQ(errors, 0);

    Symbol *sym = symbol_lookup(as->symbols, "label");
    ASSERT(sym != NULL);
    ASSERT_EQ(sym->value, 0x1000);

    assembler_free(as);
}

TEST(label_in_false_block_not_defined) {
    Assembler *as = assembler_create();
    const char *source =
        "* = $1000\n"
        "!if 0\n"
        "hidden_label: nop\n"
        "!endif\n"
        "visible: nop\n";
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT_EQ(errors, 0);

    /* hidden_label should NOT be defined */
    Symbol *hidden = symbol_lookup(as->symbols, "hidden_label");
    ASSERT(hidden == NULL);

    /* visible should be at $1000 */
    Symbol *visible = symbol_lookup(as->symbols, "visible");
    ASSERT(visible != NULL);
    ASSERT_EQ(visible->value, 0x1000);

    assembler_free(as);
}

/* ========== Error Cases ========== */

TEST(else_without_if) {
    Assembler *as = assembler_create();
    const char *source =
        "* = $1000\n"
        "!else\n"
        "    nop\n"
        "!endif\n";
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT(errors > 0);
    assembler_free(as);
}

TEST(endif_without_if) {
    Assembler *as = assembler_create();
    const char *source =
        "* = $1000\n"
        "!endif\n";
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT(errors > 0);
    assembler_free(as);
}

TEST(unclosed_if) {
    Assembler *as = assembler_create();
    const char *source =
        "* = $1000\n"
        "!if 1\n"
        "    nop\n";
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT(errors > 0);
    assembler_free(as);
}

TEST(double_else) {
    Assembler *as = assembler_create();
    const char *source =
        "* = $1000\n"
        "!if 1\n"
        "    nop\n"
        "!else\n"
        "    nop\n"
        "!else\n"
        "    nop\n"
        "!endif\n";
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT(errors > 0);
    assembler_free(as);
}

/* ========== Main ========== */

int main(void) {
    printf("Conditional Assembly Tests\n");
    printf("==========================\n\n");

    printf("Basic !if Tests:\n");
    RUN_TEST(if_true_includes_code);
    RUN_TEST(if_false_excludes_code);
    RUN_TEST(if_expression);
    RUN_TEST(if_expression_false);
    RUN_TEST(if_comparison);

    printf("\n!if / !else Tests:\n");
    RUN_TEST(if_else_true_branch);
    RUN_TEST(if_else_false_branch);

    printf("\n!ifdef / !ifndef Tests:\n");
    RUN_TEST(ifdef_defined);
    RUN_TEST(ifdef_undefined);
    RUN_TEST(ifndef_defined);
    RUN_TEST(ifndef_undefined);

    printf("\nNested Conditional Tests:\n");
    RUN_TEST(nested_if_both_true);
    RUN_TEST(nested_if_outer_false);
    RUN_TEST(nested_if_inner_false);
    RUN_TEST(nested_if_with_else);

    printf("\nSyntax Tests:\n");
    RUN_TEST(if_line_based);

    printf("\nLabel in Conditional Tests:\n");
    RUN_TEST(label_in_true_block);
    RUN_TEST(label_in_false_block_not_defined);

    printf("\nError Case Tests:\n");
    RUN_TEST(else_without_if);
    RUN_TEST(endif_without_if);
    RUN_TEST(unclosed_if);
    RUN_TEST(double_else);

    printf("\n==========================\n");
    printf("Total: %d passed, %d failed\n", tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
