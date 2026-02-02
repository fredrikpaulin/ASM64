/*
 * test_macro.c - Tests for Macro Definition and Expansion
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

/* ========== Basic Macro Definition ========== */

TEST(macro_simple_no_args) {
    /* Define and invoke a simple macro with no arguments */
    const char *source =
        "* = $1000\n"
        "!macro nop3\n"
        "    nop\n"
        "    nop\n"
        "    nop\n"
        "!endmacro\n"
        "+nop3\n";
    uint8_t expected[] = { 0xEA, 0xEA, 0xEA };  /* Three NOPs */
    ASSERT(assemble_and_check(source, expected, 3));
}

TEST(macro_with_one_arg) {
    /* Macro with one argument */
    const char *source =
        "* = $1000\n"
        "!macro load_imm value\n"
        "    lda #value\n"
        "!endmacro\n"
        "+load_imm $42\n";
    uint8_t expected[] = { 0xA9, 0x42 };  /* LDA #$42 */
    ASSERT(assemble_and_check(source, expected, 2));
}

TEST(macro_with_two_args) {
    /* Macro with two arguments */
    const char *source =
        "* = $1000\n"
        "!macro store_val addr, val\n"
        "    lda #val\n"
        "    sta addr\n"
        "!endmacro\n"
        "+store_val $D020, $01\n";
    uint8_t expected[] = { 0xA9, 0x01, 0x8D, 0x20, 0xD0 };  /* LDA #$01; STA $D020 */
    ASSERT(assemble_and_check(source, expected, 5));
}

TEST(macro_multiple_invocations) {
    /* Invoke same macro multiple times */
    const char *source =
        "* = $1000\n"
        "!macro inc_x\n"
        "    inx\n"
        "!endmacro\n"
        "+inc_x\n"
        "+inc_x\n"
        "+inc_x\n";
    uint8_t expected[] = { 0xE8, 0xE8, 0xE8 };  /* Three INX */
    ASSERT(assemble_and_check(source, expected, 3));
}

TEST(macro_with_different_args) {
    /* Same macro with different argument values */
    const char *source =
        "* = $1000\n"
        "!macro load val\n"
        "    lda #val\n"
        "!endmacro\n"
        "+load $11\n"
        "+load $22\n"
        "+load $33\n";
    uint8_t expected[] = { 0xA9, 0x11, 0xA9, 0x22, 0xA9, 0x33 };
    ASSERT(assemble_and_check(source, expected, 6));
}

/* ========== Macro with Labels ========== */

TEST(macro_defines_label) {
    /* Label inside macro */
    Assembler *as = assembler_create();
    const char *source =
        "* = $1000\n"
        "!macro make_label\n"
        "mylabel: nop\n"
        "!endmacro\n"
        "+make_label\n";
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT_EQ(errors, 0);

    /* The label should be defined at $1000 */
    Symbol *sym = symbol_lookup(as->symbols, "mylabel");
    ASSERT(sym != NULL);
    ASSERT_EQ(sym->value, 0x1000);

    assembler_free(as);
}

/* ========== Macro with Expressions ========== */

TEST(macro_arg_in_expression) {
    /* Use macro argument in an expression */
    const char *source =
        "* = $1000\n"
        "!macro load_plus_one val\n"
        "    lda #val + 1\n"
        "!endmacro\n"
        "+load_plus_one $41\n";  /* Should produce LDA #$42 */
    uint8_t expected[] = { 0xA9, 0x42 };
    ASSERT(assemble_and_check(source, expected, 2));
}

/* ========== Nested Macros ========== */

TEST(macro_calls_macro) {
    /* One macro invokes another */
    const char *source =
        "* = $1000\n"
        "!macro inner\n"
        "    nop\n"
        "!endmacro\n"
        "!macro outer\n"
        "    +inner\n"
        "    +inner\n"
        "!endmacro\n"
        "+outer\n";
    uint8_t expected[] = { 0xEA, 0xEA };  /* Two NOPs */
    ASSERT(assemble_and_check(source, expected, 2));
}

/* ========== Endmacro Aliases ========== */

TEST(endm_alias) {
    /* Use !endm instead of !endmacro */
    const char *source =
        "* = $1000\n"
        "!macro test_endm\n"
        "    nop\n"
        "!endm\n"
        "+test_endm\n";
    uint8_t expected[] = { 0xEA };
    ASSERT(assemble_and_check(source, expected, 1));
}

/* ========== Macro with Conditionals ========== */

TEST(macro_with_conditional) {
    /* Conditional inside macro */
    const char *source =
        "* = $1000\n"
        "DEBUG = 1\n"
        "!macro maybe_nop\n"
        "    !if DEBUG\n"
        "        nop\n"
        "    !endif\n"
        "!endmacro\n"
        "+maybe_nop\n";
    uint8_t expected[] = { 0xEA };
    ASSERT(assemble_and_check(source, expected, 1));
}

TEST(macro_conditional_false) {
    /* Conditional in macro evaluates to false */
    const char *source =
        "* = $1000\n"
        "DEBUG = 0\n"
        "!macro maybe_nop\n"
        "    !if DEBUG\n"
        "        nop\n"
        "    !else\n"
        "        inx\n"
        "    !endif\n"
        "!endmacro\n"
        "+maybe_nop\n";
    uint8_t expected[] = { 0xE8 };  /* INX instead of NOP */
    ASSERT(assemble_and_check(source, expected, 1));
}

/* ========== Error Cases ========== */

TEST(undefined_macro) {
    Assembler *as = assembler_create();
    const char *source =
        "* = $1000\n"
        "+nonexistent\n";
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT(errors > 0);
    assembler_free(as);
}

TEST(wrong_arg_count) {
    Assembler *as = assembler_create();
    const char *source =
        "* = $1000\n"
        "!macro needs_two a, b\n"
        "    lda #a\n"
        "    ldx #b\n"
        "!endmacro\n"
        "+needs_two $42\n";  /* Only one arg, needs two */
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT(errors > 0);
    assembler_free(as);
}

TEST(unterminated_macro) {
    Assembler *as = assembler_create();
    const char *source =
        "* = $1000\n"
        "!macro incomplete\n"
        "    nop\n";  /* Missing !endmacro */
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT(errors > 0);
    assembler_free(as);
}

/* ========== Main ========== */

int main(void) {
    printf("Macro Tests\n");
    printf("===========\n\n");

    printf("Basic Macro Definition:\n");
    RUN_TEST(macro_simple_no_args);
    RUN_TEST(macro_with_one_arg);
    RUN_TEST(macro_with_two_args);
    RUN_TEST(macro_multiple_invocations);
    RUN_TEST(macro_with_different_args);

    printf("\nMacro with Labels:\n");
    RUN_TEST(macro_defines_label);

    printf("\nMacro with Expressions:\n");
    RUN_TEST(macro_arg_in_expression);

    printf("\nNested Macros:\n");
    RUN_TEST(macro_calls_macro);

    printf("\nEndmacro Aliases:\n");
    RUN_TEST(endm_alias);

    printf("\nMacro with Conditionals:\n");
    RUN_TEST(macro_with_conditional);
    RUN_TEST(macro_conditional_false);

    printf("\nError Cases:\n");
    RUN_TEST(undefined_macro);
    RUN_TEST(wrong_arg_count);
    RUN_TEST(unterminated_macro);

    printf("\n===========\n");
    printf("Total: %d passed, %d failed\n", tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
