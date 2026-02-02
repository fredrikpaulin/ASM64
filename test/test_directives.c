/*
 * test_directives.c - Tests for Extended Directives and File Inclusion
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

/* ========== PETSCII Directive Tests ========== */

TEST(pet_basic) {
    /* !pet "HELLO" should convert to uppercase PETSCII */
    const char *source = "* = $1000\n!pet \"HELLO\"";
    uint8_t expected[] = { 'H', 'E', 'L', 'L', 'O' };
    ASSERT(assemble_and_check(source, expected, 5));
}

TEST(pet_lowercase) {
    /* !pet "hello" should convert lowercase to uppercase */
    const char *source = "* = $1000\n!pet \"hello\"";
    uint8_t expected[] = { 'H', 'E', 'L', 'L', 'O' };
    ASSERT(assemble_and_check(source, expected, 5));
}

TEST(pet_mixed_case) {
    /* Mixed case should all become uppercase */
    const char *source = "* = $1000\n!pet \"HeLLo\"";
    uint8_t expected[] = { 'H', 'E', 'L', 'L', 'O' };
    ASSERT(assemble_and_check(source, expected, 5));
}

TEST(pet_numbers) {
    /* Numbers should pass through unchanged */
    const char *source = "* = $1000\n!pet \"123\"";
    uint8_t expected[] = { '1', '2', '3' };
    ASSERT(assemble_and_check(source, expected, 3));
}

/* ========== Screen Code Directive Tests ========== */

TEST(scr_basic) {
    /* !scr "@ABC" should convert to screen codes: @=0, A=1, B=2, C=3 */
    const char *source = "* = $1000\n!scr \"@ABC\"";
    uint8_t expected[] = { 0, 1, 2, 3 };
    ASSERT(assemble_and_check(source, expected, 4));
}

TEST(scr_space) {
    /* Space should be 0x20 in screen codes */
    const char *source = "* = $1000\n!scr \" \"";
    uint8_t expected[] = { 0x20 };
    ASSERT(assemble_and_check(source, expected, 1));
}

TEST(scr_lowercase) {
    /* Lowercase should convert same as uppercase */
    const char *source = "* = $1000\n!scr \"abc\"";
    uint8_t expected[] = { 1, 2, 3 }; /* Same as ABC */
    ASSERT(assemble_and_check(source, expected, 3));
}

TEST(scr_digits) {
    /* Digits should be $30-$39 in screen codes */
    const char *source = "* = $1000\n!scr \"0123456789\"";
    uint8_t expected[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 };
    ASSERT(assemble_and_check(source, expected, 10));
}

TEST(scr_punctuation) {
    /* Test various punctuation marks */
    const char *source = "* = $1000\n!scr \"!?:;\"";
    uint8_t expected[] = { 0x21, 0x3F, 0x3A, 0x3B };
    ASSERT(assemble_and_check(source, expected, 4));
}

TEST(scr_full_alphabet) {
    /* Test full uppercase alphabet */
    const char *source = "* = $1000\n!scr \"ABCDEFGHIJKLMNOPQRSTUVWXYZ\"";
    uint8_t expected[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 };
    ASSERT(assemble_and_check(source, expected, 26));
}

TEST(pet_special_chars) {
    /* Test special character conversions in PETSCII */
    Assembler *as = assembler_create();
    /* Note: @[] are at positions 0x40, 0x5B, 0x5D in both ASCII and PETSCII */
    const char *source = "* = $1000\n!pet \"@[]^\"";
    assembler_assemble_string(as, source, "test.asm");

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    ASSERT_EQ(size, 4);
    ASSERT_EQ(output[0], 0x40);  /* @ */
    ASSERT_EQ(output[1], 0x5B);  /* [ */
    ASSERT_EQ(output[2], 0x5D);  /* ] */
    ASSERT_EQ(output[3], 0x5E);  /* ^ (up arrow in PETSCII) */

    assembler_free(as);
}

/* ========== Null-Terminated String Tests ========== */

TEST(null_basic) {
    /* !null "HI" should output HI followed by 0x00 */
    const char *source = "* = $1000\n!null \"HI\"";
    uint8_t expected[] = { 'H', 'I', 0x00 };
    ASSERT(assemble_and_check(source, expected, 3));
}

TEST(null_empty) {
    /* Empty string should just output null terminator */
    const char *source = "* = $1000\n!null \"\"";
    uint8_t expected[] = { 0x00 };
    ASSERT(assemble_and_check(source, expected, 1));
}

/* ========== Skip/Reserve Directive Tests ========== */

TEST(skip_basic) {
    /* !skip 5 should advance PC by 5 without emitting */
    Assembler *as = assembler_create();
    const char *source = "* = $1000\n!byte $AA\n!skip 5\n!byte $BB";
    assembler_assemble_string(as, source, "test.asm");

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    /* Should have byte at 1000 and byte at 1006, size = 7 */
    ASSERT_EQ(size, 7);
    ASSERT_EQ(output[0], 0xAA);
    ASSERT_EQ(output[6], 0xBB);

    assembler_free(as);
}

TEST(res_alias) {
    /* !res is alias for !skip */
    Assembler *as = assembler_create();
    const char *source = "* = $1000\n!byte $AA\n!res 3\n!byte $BB";
    assembler_assemble_string(as, source, "test.asm");

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    ASSERT_EQ(size, 5);
    ASSERT_EQ(output[0], 0xAA);
    ASSERT_EQ(output[4], 0xBB);

    assembler_free(as);
}

/* ========== Alignment Directive Tests ========== */

TEST(align_basic) {
    /* !align 256 should pad to next 256-byte boundary */
    Assembler *as = assembler_create();
    const char *source = "* = $1001\n!byte $AA\n!align 256\n!byte $BB";
    assembler_assemble_string(as, source, "test.asm");

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    /* Start at $1001, after byte at $1002, align to $1100, put BB there */
    /* Size = $1100 - $1001 + 1 = 256 */
    ASSERT_EQ(start, 0x1001);
    ASSERT_EQ(output[0], 0xAA);
    /* The BB should be at offset $1100 - $1001 = $FF = 255 */
    ASSERT_EQ(output[255], 0xBB);

    assembler_free(as);
}

TEST(align_already_aligned) {
    /* If already aligned, no padding needed */
    Assembler *as = assembler_create();
    const char *source = "* = $1000\n!align 256\n!byte $BB";
    assembler_assemble_string(as, source, "test.asm");

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    ASSERT_EQ(size, 1);
    ASSERT_EQ(output[0], 0xBB);

    assembler_free(as);
}

TEST(align_with_fill) {
    /* !align 4, $EA should fill with $EA (NOP) */
    Assembler *as = assembler_create();
    const char *source = "* = $1001\n!align 4, $EA";
    assembler_assemble_string(as, source, "test.asm");

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    /* From $1001, align to $1004, need 3 bytes of padding */
    ASSERT_EQ(size, 3);
    ASSERT_EQ(output[0], 0xEA);
    ASSERT_EQ(output[1], 0xEA);
    ASSERT_EQ(output[2], 0xEA);

    assembler_free(as);
}

/* ========== !08 and !16 Alias Tests ========== */

TEST(byte_08_alias) {
    const char *source = "* = $1000\n!08 $12, $34";
    uint8_t expected[] = { 0x12, 0x34 };
    ASSERT(assemble_and_check(source, expected, 2));
}

TEST(word_16_alias) {
    const char *source = "* = $1000\n!16 $1234";
    uint8_t expected[] = { 0x34, 0x12 }; /* Little-endian */
    ASSERT(assemble_and_check(source, expected, 2));
}

/* ========== Include Path Tests ========== */

TEST(include_path_add) {
    Assembler *as = assembler_create();

    ASSERT_EQ(assembler_add_include_path(as, "/usr/local/include"), 0);
    ASSERT_EQ(assembler_add_include_path(as, "/home/user/asm"), 0);
    ASSERT_EQ(as->include_path_count, 2);

    assembler_free(as);
}

/* ========== Include Stack Trace Tests ========== */

TEST(include_trace_empty) {
    Assembler *as = assembler_create();

    char *trace = assembler_get_include_trace(as);
    ASSERT(trace == NULL);

    assembler_free(as);
}

/* ========== Label with Directives ========== */

TEST(label_before_pet) {
    Assembler *as = assembler_create();
    const char *source = "* = $1000\nmsg: !pet \"HI\"";
    assembler_assemble_string(as, source, "test.asm");

    Symbol *sym = symbol_lookup(as->symbols, "msg");
    ASSERT(sym != NULL);
    ASSERT_EQ(sym->value, 0x1000);

    assembler_free(as);
}

TEST(label_before_align) {
    Assembler *as = assembler_create();
    const char *source = "* = $1001\nstart: !align 256\ncode: nop";
    assembler_assemble_string(as, source, "test.asm");

    Symbol *start = symbol_lookup(as->symbols, "start");
    Symbol *code = symbol_lookup(as->symbols, "code");
    ASSERT(start != NULL);
    ASSERT(code != NULL);
    ASSERT_EQ(start->value, 0x1001);
    ASSERT_EQ(code->value, 0x1100);

    assembler_free(as);
}

/* ========== Combined Directives ========== */

TEST(multiple_string_types) {
    /* Test all string types together */
    Assembler *as = assembler_create();
    const char *source =
        "* = $1000\n"
        "!text \"AB\"\n"
        "!pet \"cd\"\n"
        "!scr \"EF\"\n"
        "!null \"G\"";

    assembler_assemble_string(as, source, "test.asm");

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    /* !text "AB" = 2 + !pet "cd" = 2 + !scr "EF" = 2 + !null "G" = 2 (G + null) = 8 */
    ASSERT_EQ(size, 8);
    /* !text "AB" */
    ASSERT_EQ(output[0], 'A');
    ASSERT_EQ(output[1], 'B');
    /* !pet "cd" -> "CD" uppercase */
    ASSERT_EQ(output[2], 'C');
    ASSERT_EQ(output[3], 'D');
    /* !scr "EF" -> 5, 6 in screen codes */
    ASSERT_EQ(output[4], 5);
    ASSERT_EQ(output[5], 6);
    /* !null "G" -> 'G', 0 */
    ASSERT_EQ(output[6], 'G');
    ASSERT_EQ(output[7], 0x00);

    assembler_free(as);
}

/* ========== Source Include Tests ========== */

TEST(source_include_basic) {
    /* Write test include file first */
    FILE *f = fopen("/tmp/test_inc.asm", "w");
    if (!f) {
        printf("[SKIP - cannot create test file]\n");
        tests_passed++;
        return;
    }
    fprintf(f, "inc_label:\n    lda #$42\n    rts\n");
    fclose(f);

    Assembler *as = assembler_create();
    /* Add /tmp to include paths */
    assembler_add_include_path(as, "/tmp");

    const char *source =
        "* = $1000\n"
        "    jsr inc_label\n"
        "!source \"test_inc.asm\"\n";

    int errors = assembler_assemble_string(as, source, "test.asm");

    /* Should succeed - the !source brings in inc_label */
    ASSERT_EQ(errors, 0);

    /* Check inc_label was defined */
    Symbol *sym = symbol_lookup(as->symbols, "inc_label");
    ASSERT(sym != NULL);

    assembler_free(as);
}

TEST(include_nested_depth) {
    /* Test that include depth is limited */
    Assembler *as = assembler_create();

    /* Try to nest includes too deeply - would require test files */
    /* For now just check the limit is set */
    ASSERT(ASM_MAX_INCLUDE_DEPTH > 0);

    assembler_free(as);
}

/* ========== Binary Include Tests ========== */

TEST(binary_basic) {
    /* Write test binary file */
    FILE *f = fopen("/tmp/test.bin", "wb");
    if (!f) {
        printf("[SKIP - cannot create test file]\n");
        tests_passed++;
        return;
    }
    fputc(0x11, f);
    fputc(0x22, f);
    fputc(0x33, f);
    fclose(f);

    Assembler *as = assembler_create();
    assembler_add_include_path(as, "/tmp");

    const char *source = "* = $1000\n!binary \"test.bin\"";
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT_EQ(errors, 0);

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    ASSERT_EQ(size, 3);
    ASSERT_EQ(output[0], 0x11);
    ASSERT_EQ(output[1], 0x22);
    ASSERT_EQ(output[2], 0x33);

    assembler_free(as);
}

TEST(binary_with_offset) {
    /* Write test binary file */
    FILE *f = fopen("/tmp/test_off.bin", "wb");
    if (!f) {
        printf("[SKIP - cannot create test file]\n");
        tests_passed++;
        return;
    }
    fputc(0xAA, f);
    fputc(0xBB, f);
    fputc(0xCC, f);
    fputc(0xDD, f);
    fclose(f);

    Assembler *as = assembler_create();
    assembler_add_include_path(as, "/tmp");

    /* Skip first byte, read 2 bytes */
    const char *source = "* = $1000\n!binary \"test_off.bin\", 2, 1";
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT_EQ(errors, 0);

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    ASSERT_EQ(size, 2);
    ASSERT_EQ(output[0], 0xBB);
    ASSERT_EQ(output[1], 0xCC);

    assembler_free(as);
}

/* ========== BASIC Stub Directive Tests ========== */

TEST(basic_default) {
    /* !basic with no args: line 10, SYS to next instruction */
    Assembler *as = assembler_create();
    const char *source =
        "* = $0801\n"
        "!basic\n"
        "    nop\n";

    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT_EQ(errors, 0);

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    /* BASIC stub + NOP = 12 or 13 bytes (depending on digit count) + 1 */
    ASSERT(size >= 13);

    /* Check start address */
    ASSERT_EQ(start, 0x0801);

    /* Check line number (bytes 2-3, little-endian) */
    ASSERT_EQ(output[2], 0x0A);  /* 10 = $000A */
    ASSERT_EQ(output[3], 0x00);

    /* Check SYS token (byte 4) */
    ASSERT_EQ(output[4], 0x9E);

    /* Check that the last byte is NOP ($EA) */
    ASSERT_EQ(output[size - 1], 0xEA);

    assembler_free(as);
}

TEST(basic_custom_line) {
    /* !basic 2025 - custom line number */
    Assembler *as = assembler_create();
    const char *source =
        "* = $0801\n"
        "!basic 2025\n"
        "    rts\n";

    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT_EQ(errors, 0);

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    /* Check line number 2025 = $07E9 (little-endian) */
    ASSERT_EQ(output[2], 0xE9);
    ASSERT_EQ(output[3], 0x07);

    /* Check SYS token */
    ASSERT_EQ(output[4], 0x9E);

    /* Check that the last byte is RTS ($60) */
    ASSERT_EQ(output[size - 1], 0x60);

    assembler_free(as);
}

TEST(basic_custom_address) {
    /* !basic 10, $C000 - custom SYS address */
    Assembler *as = assembler_create();
    const char *source =
        "* = $0801\n"
        "!basic 10, $C000\n"
        "    nop\n";

    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT_EQ(errors, 0);

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    /* Check SYS token */
    ASSERT_EQ(output[4], 0x9E);

    /* Check that address string is "49152" (ASCII for $C000 = 49152) */
    /* SYS address is at bytes 5-9 for 5-digit number */
    ASSERT_EQ(output[5], '4');
    ASSERT_EQ(output[6], '9');
    ASSERT_EQ(output[7], '1');
    ASSERT_EQ(output[8], '5');
    ASSERT_EQ(output[9], '2');

    assembler_free(as);
}

TEST(basic_sys_address_digits) {
    /* Test that 4-digit vs 5-digit addresses work correctly */
    Assembler *as = assembler_create();

    /* Address < 10000 should use 4 digits */
    const char *source1 =
        "* = $0801\n"
        "!basic 10, 2061\n";  /* 4-digit address */

    assembler_reset(as);
    int errors = assembler_assemble_string(as, source1, "test.asm");
    ASSERT_EQ(errors, 0);

    uint16_t start;
    int size;
    const uint8_t *output = assembler_get_output(as, &start, &size);

    /* Check address string "2061" */
    ASSERT_EQ(output[5], '2');
    ASSERT_EQ(output[6], '0');
    ASSERT_EQ(output[7], '6');
    ASSERT_EQ(output[8], '1');
    ASSERT_EQ(output[9], 0x00);  /* End of line marker */

    assembler_free(as);
}

/* ========== Error Cases ========== */

TEST(pet_missing_string) {
    Assembler *as = assembler_create();
    const char *source = "* = $1000\n!pet";
    int errors = assembler_assemble_string(as, source, "test.asm");
    /* Should be an error or just skip */
    assembler_free(as);
}

TEST(skip_negative) {
    Assembler *as = assembler_create();
    const char *source = "* = $1000\n!skip -1";
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT(errors > 0);
    assembler_free(as);
}

TEST(align_zero) {
    Assembler *as = assembler_create();
    const char *source = "* = $1000\n!align 0";
    int errors = assembler_assemble_string(as, source, "test.asm");
    ASSERT(errors > 0);
    assembler_free(as);
}

/* ========== Main ========== */

int main(void) {
    printf("Extended Directives Tests\n");
    printf("=========================\n\n");

    printf("PETSCII Directive Tests:\n");
    RUN_TEST(pet_basic);
    RUN_TEST(pet_lowercase);
    RUN_TEST(pet_mixed_case);
    RUN_TEST(pet_numbers);

    printf("\nScreen Code Directive Tests:\n");
    RUN_TEST(scr_basic);
    RUN_TEST(scr_space);
    RUN_TEST(scr_lowercase);
    RUN_TEST(scr_digits);
    RUN_TEST(scr_punctuation);
    RUN_TEST(scr_full_alphabet);

    printf("\nPETSCII Special Character Tests:\n");
    RUN_TEST(pet_special_chars);

    printf("\nNull-Terminated String Tests:\n");
    RUN_TEST(null_basic);
    RUN_TEST(null_empty);

    printf("\nSkip/Reserve Directive Tests:\n");
    RUN_TEST(skip_basic);
    RUN_TEST(res_alias);

    printf("\nAlignment Directive Tests:\n");
    RUN_TEST(align_basic);
    RUN_TEST(align_already_aligned);
    RUN_TEST(align_with_fill);

    printf("\nAlias Tests:\n");
    RUN_TEST(byte_08_alias);
    RUN_TEST(word_16_alias);

    printf("\nInclude Path Tests:\n");
    RUN_TEST(include_path_add);
    RUN_TEST(include_trace_empty);

    printf("\nLabel with Directive Tests:\n");
    RUN_TEST(label_before_pet);
    RUN_TEST(label_before_align);

    printf("\nCombined Directive Tests:\n");
    RUN_TEST(multiple_string_types);

    printf("\nSource Include Tests:\n");
    RUN_TEST(source_include_basic);
    RUN_TEST(include_nested_depth);

    printf("\nBinary Include Tests:\n");
    RUN_TEST(binary_basic);
    RUN_TEST(binary_with_offset);

    printf("\nBASIC Stub Directive Tests:\n");
    RUN_TEST(basic_default);
    RUN_TEST(basic_custom_line);
    RUN_TEST(basic_custom_address);
    RUN_TEST(basic_sys_address_digits);

    printf("\nError Case Tests:\n");
    RUN_TEST(pet_missing_string);
    RUN_TEST(skip_negative);
    RUN_TEST(align_zero);

    printf("\n=========================\n");
    printf("Total: %d passed, %d failed\n", tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
