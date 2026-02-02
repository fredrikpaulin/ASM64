/*
 * test_assembler.c - Unit tests for two-pass assembler core
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "assembler.h"

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

/* Helper to assemble and check output bytes */
static int check_output(const char *source, const uint8_t *expected, int expected_size, uint16_t expected_addr) {
    Assembler *as = assembler_create();
    if (!as) return 0;

    int errors = assembler_assemble_string(as, source, "test");
    if (errors > 0) {
        assembler_free(as);
        return 0;
    }

    uint16_t start_addr;
    int size;
    const uint8_t *output = assembler_get_output(as, &start_addr, &size);

    int ok = (start_addr == expected_addr) &&
             (size == expected_size) &&
             (memcmp(output, expected, expected_size) == 0);

    if (!ok) {
        printf("\n    Expected addr: $%04X, got: $%04X\n", expected_addr, start_addr);
        printf("    Expected size: %d, got: %d\n", expected_size, size);
        if (size > 0 && size == expected_size) {
            printf("    Expected: ");
            for (int i = 0; i < expected_size; i++) printf("%02X ", expected[i]);
            printf("\n    Got:      ");
            for (int i = 0; i < size; i++) printf("%02X ", output[i]);
            printf("\n");
        }
    }

    assembler_free(as);
    return ok;
}

/* Helper to assemble and check for errors */
static int check_error(const char *source) {
    Assembler *as = assembler_create();
    if (!as) return 0;

    int errors = assembler_assemble_string(as, source, "test");
    assembler_free(as);

    return errors > 0;
}

/* ========== Basic Assembly Tests ========== */

TEST(create_assembler) {
    Assembler *as = assembler_create();
    int ok = (as != NULL);
    assembler_free(as);
    return ok;
}

TEST(empty_source) {
    Assembler *as = assembler_create();
    int errors = assembler_assemble_string(as, "", "test");
    int ok = (errors == 0);
    assembler_free(as);
    return ok;
}

TEST(comment_only) {
    Assembler *as = assembler_create();
    int errors = assembler_assemble_string(as, "; just a comment\n", "test");
    int ok = (errors == 0);
    assembler_free(as);
    return ok;
}

/* ========== Implied Mode Tests ========== */

TEST(implied_nop) {
    uint8_t expected[] = { 0xEA };
    return check_output("*=$1000\nNOP", expected, 1, 0x1000);
}

TEST(implied_sequence) {
    uint8_t expected[] = { 0xEA, 0xE8, 0xC8, 0xCA, 0x88 };
    return check_output("*=$1000\nNOP\nINX\nINY\nDEX\nDEY", expected, 5, 0x1000);
}

TEST(implied_rts) {
    uint8_t expected[] = { 0x60 };
    return check_output("*=$1000\nRTS", expected, 1, 0x1000);
}

/* ========== Immediate Mode Tests ========== */

TEST(immediate_lda) {
    uint8_t expected[] = { 0xA9, 0x42 };
    return check_output("*=$1000\nLDA #$42", expected, 2, 0x1000);
}

TEST(immediate_ldx) {
    uint8_t expected[] = { 0xA2, 0xFF };
    return check_output("*=$1000\nLDX #$FF", expected, 2, 0x1000);
}

TEST(immediate_ldy) {
    uint8_t expected[] = { 0xA0, 0x00 };
    return check_output("*=$1000\nLDY #0", expected, 2, 0x1000);
}

TEST(immediate_expression) {
    uint8_t expected[] = { 0xA9, 0x30 };
    return check_output("*=$1000\nLDA #$10+$20", expected, 2, 0x1000);
}

/* ========== Zero Page Mode Tests ========== */

TEST(zeropage_lda) {
    uint8_t expected[] = { 0xA5, 0x80 };
    return check_output("*=$1000\nLDA $80", expected, 2, 0x1000);
}

TEST(zeropage_sta) {
    uint8_t expected[] = { 0x85, 0x02 };
    return check_output("*=$1000\nSTA $02", expected, 2, 0x1000);
}

TEST(zeropage_x) {
    uint8_t expected[] = { 0xB5, 0x80 };
    return check_output("*=$1000\nLDA $80,X", expected, 2, 0x1000);
}

TEST(zeropage_y) {
    uint8_t expected[] = { 0xB6, 0x80 };
    return check_output("*=$1000\nLDX $80,Y", expected, 2, 0x1000);
}

/* ========== Absolute Mode Tests ========== */

TEST(absolute_lda) {
    uint8_t expected[] = { 0xAD, 0x00, 0x10 };
    return check_output("*=$1000\nLDA $1000", expected, 3, 0x1000);
}

TEST(absolute_sta) {
    uint8_t expected[] = { 0x8D, 0x20, 0xD0 };
    return check_output("*=$1000\nSTA $D020", expected, 3, 0x1000);
}

TEST(absolute_x) {
    uint8_t expected[] = { 0xBD, 0x00, 0x10 };
    return check_output("*=$1000\nLDA $1000,X", expected, 3, 0x1000);
}

TEST(absolute_y) {
    uint8_t expected[] = { 0xB9, 0x00, 0x10 };
    return check_output("*=$1000\nLDA $1000,Y", expected, 3, 0x1000);
}

/* ========== Indirect Mode Tests ========== */

TEST(indirect_jmp) {
    uint8_t expected[] = { 0x6C, 0xFC, 0xFF };
    return check_output("*=$1000\nJMP ($FFFC)", expected, 3, 0x1000);
}

TEST(indirect_x) {
    uint8_t expected[] = { 0xA1, 0x80 };
    return check_output("*=$1000\nLDA ($80,X)", expected, 2, 0x1000);
}

TEST(indirect_y) {
    uint8_t expected[] = { 0xB1, 0x80 };
    return check_output("*=$1000\nLDA ($80),Y", expected, 2, 0x1000);
}

/* ========== Accumulator Mode Tests ========== */

TEST(accumulator_asl) {
    uint8_t expected[] = { 0x0A };
    return check_output("*=$1000\nASL A", expected, 1, 0x1000);
}

TEST(accumulator_implied) {
    uint8_t expected[] = { 0x0A };
    return check_output("*=$1000\nASL", expected, 1, 0x1000);
}

TEST(accumulator_ror) {
    uint8_t expected[] = { 0x6A };
    return check_output("*=$1000\nROR A", expected, 1, 0x1000);
}

/* ========== Branch Tests ========== */

TEST(branch_forward) {
    /* BNE skips over 2 bytes, offset = 2 */
    uint8_t expected[] = { 0xD0, 0x02, 0xEA, 0xEA, 0x60 };
    return check_output("*=$1000\nBNE skip\nNOP\nNOP\nskip: RTS", expected, 5, 0x1000);
}

TEST(branch_backward) {
    /* BNE jumps back 4 bytes (to loop), offset = -4 = 0xFC */
    uint8_t expected[] = { 0xEA, 0xEA, 0xD0, 0xFC };
    return check_output("*=$1000\nloop: NOP\nNOP\nBNE loop", expected, 4, 0x1000);
}

TEST(branch_beq) {
    uint8_t expected[] = { 0xF0, 0x00 };
    return check_output("*=$1000\nBEQ here\nhere:", expected, 2, 0x1000);
}

TEST(branch_bcc) {
    uint8_t expected[] = { 0x90, 0x01, 0xEA, 0x60 };
    return check_output("*=$1000\nBCC skip\nNOP\nskip: RTS", expected, 4, 0x1000);
}

/* ========== Label Tests ========== */

TEST(forward_reference) {
    uint8_t expected[] = { 0x4C, 0x03, 0x10, 0x60 };
    return check_output("*=$1000\nJMP target\ntarget: RTS", expected, 4, 0x1000);
}

TEST(backward_reference) {
    uint8_t expected[] = { 0x60, 0x4C, 0x00, 0x10 };
    return check_output("*=$1000\nstart: RTS\nJMP start", expected, 4, 0x1000);
}

TEST(multiple_labels) {
    uint8_t expected[] = { 0xEA, 0xEA, 0x4C, 0x00, 0x10 };
    return check_output("*=$1000\none: NOP\ntwo: NOP\nJMP one", expected, 5, 0x1000);
}

TEST(label_expression) {
    uint8_t expected[] = { 0xEA, 0xAD, 0x01, 0x10 };
    return check_output("*=$1000\ndata: NOP\nLDA data+1", expected, 4, 0x1000);
}

/* ========== Directive Tests ========== */

TEST(byte_single) {
    uint8_t expected[] = { 0x42 };
    return check_output("*=$1000\n!byte $42", expected, 1, 0x1000);
}

TEST(byte_multiple) {
    uint8_t expected[] = { 0x01, 0x02, 0x03 };
    return check_output("*=$1000\n!byte $01, $02, $03", expected, 3, 0x1000);
}

TEST(word_single) {
    uint8_t expected[] = { 0x34, 0x12 };
    return check_output("*=$1000\n!word $1234", expected, 2, 0x1000);
}

TEST(word_multiple) {
    uint8_t expected[] = { 0x01, 0x08, 0x00, 0xD0 };
    return check_output("*=$1000\n!word $0801, $D000", expected, 4, 0x1000);
}

TEST(text_string) {
    uint8_t expected[] = { 'H', 'E', 'L', 'L', 'O' };
    return check_output("*=$1000\n!text \"HELLO\"", expected, 5, 0x1000);
}

TEST(fill_directive) {
    uint8_t expected[] = { 0xEA, 0xEA, 0xEA, 0xEA, 0xEA };
    return check_output("*=$1000\n!fill 5, $EA", expected, 5, 0x1000);
}

TEST(fill_zeros) {
    uint8_t expected[] = { 0x00, 0x00, 0x00 };
    return check_output("*=$1000\n!fill 3", expected, 3, 0x1000);
}

/* ========== Origin Tests ========== */

TEST(origin_change) {
    uint8_t expected[] = { 0xEA };
    return check_output("*=$2000\nNOP", expected, 1, 0x2000);
}

TEST(origin_expression) {
    uint8_t expected[] = { 0xEA };
    return check_output("*=$1000+$1000\nNOP", expected, 1, 0x2000);
}

/* ========== Assignment Tests ========== */

TEST(assignment_simple) {
    uint8_t expected[] = { 0xA9, 0x42 };
    return check_output("*=$1000\nVALUE = $42\nLDA #VALUE", expected, 2, 0x1000);
}

TEST(assignment_expression) {
    uint8_t expected[] = { 0xA9, 0x10 };
    return check_output("*=$1000\nBASE = $08\nOFFSET = BASE * 2\nLDA #OFFSET", expected, 2, 0x1000);
}

TEST(assignment_zeropage) {
    uint8_t expected[] = { 0xA5, 0x80 };
    return check_output("*=$1000\nZP = $80\nLDA ZP", expected, 2, 0x1000);
}

/* ========== Case Sensitivity Tests ========== */

TEST(lowercase_mnemonic) {
    uint8_t expected[] = { 0xA9, 0x00 };
    return check_output("*=$1000\nlda #$00", expected, 2, 0x1000);
}

TEST(mixedcase_mnemonic) {
    uint8_t expected[] = { 0xA9, 0x00 };
    return check_output("*=$1000\nLdA #$00", expected, 2, 0x1000);
}

TEST(lowercase_register) {
    uint8_t expected[] = { 0xB5, 0x80 };
    return check_output("*=$1000\nLDA $80,x", expected, 2, 0x1000);
}

/* ========== Illegal Opcode Tests ========== */

TEST(illegal_lax) {
    uint8_t expected[] = { 0xA7, 0x80 };
    return check_output("*=$1000\nLAX $80", expected, 2, 0x1000);
}

TEST(illegal_sax) {
    uint8_t expected[] = { 0x87, 0x80 };
    return check_output("*=$1000\nSAX $80", expected, 2, 0x1000);
}

/* ========== Complex Program Tests ========== */

TEST(simple_loop) {
    /* LDX #$00 / loop: INX / CPX #$10 / BNE loop / RTS */
    uint8_t expected[] = {
        0xA2, 0x00,      /* LDX #$00 */
        0xE8,            /* INX */
        0xE0, 0x10,      /* CPX #$10 */
        0xD0, 0xFB,      /* BNE loop (-5) */
        0x60             /* RTS */
    };
    return check_output(
        "*=$1000\n"
        "LDX #$00\n"
        "loop: INX\n"
        "CPX #$10\n"
        "BNE loop\n"
        "RTS",
        expected, 8, 0x1000);
}

TEST(data_table_access) {
    uint8_t expected[] = {
        0xA2, 0x02,      /* LDX #$02 */
        0xBD, 0x06, 0x10,/* LDA table,X */
        0x60,            /* RTS */
        0x10, 0x20, 0x30 /* table: !byte $10, $20, $30 */
    };
    return check_output(
        "*=$1000\n"
        "LDX #$02\n"
        "LDA table,X\n"
        "RTS\n"
        "table: !byte $10, $20, $30",
        expected, 9, 0x1000);
}

TEST(subroutine_call) {
    /* JSR at $1000 (3 bytes) + JMP at $1003 (3 bytes) = sub at $1006 */
    uint8_t expected[] = {
        0x20, 0x06, 0x10,/* JSR sub ($1006) */
        0x4C, 0x00, 0x10,/* JMP $1000 */
        0xE8,            /* sub: INX */
        0x60             /* RTS */
    };
    return check_output(
        "*=$1000\n"
        "JSR sub\n"
        "JMP $1000\n"
        "sub: INX\n"
        "RTS",
        expected, 8, 0x1000);
}

/* ========== Error Tests ========== */

TEST(undefined_symbol_error) {
    return check_error("*=$1000\nLDA undefined_label");
}

TEST(invalid_mnemonic_error) {
    /* XXX followed by an operand would be an unknown instruction */
    return check_error("*=$1000\nlabel: XXX #$00");
}

TEST(branch_out_of_range_forward) {
    /* Create a branch that's too far forward */
    char source[8192];
    strcpy(source, "*=$1000\nBNE target\n");
    /* Add lots of NOPs to push target out of range */
    for (int i = 0; i < 140; i++) {
        strcat(source, "NOP\n");
    }
    strcat(source, "target: RTS");
    return check_error(source);
}

/* ========== Main ========== */

int main(void) {
    printf("\nAssembler Tests\n");
    printf("===============\n\n");

    printf("Basic:\n");
    RUN_TEST(create_assembler);
    RUN_TEST(empty_source);
    RUN_TEST(comment_only);

    printf("\nImplied Mode:\n");
    RUN_TEST(implied_nop);
    RUN_TEST(implied_sequence);
    RUN_TEST(implied_rts);

    printf("\nImmediate Mode:\n");
    RUN_TEST(immediate_lda);
    RUN_TEST(immediate_ldx);
    RUN_TEST(immediate_ldy);
    RUN_TEST(immediate_expression);

    printf("\nZero Page Mode:\n");
    RUN_TEST(zeropage_lda);
    RUN_TEST(zeropage_sta);
    RUN_TEST(zeropage_x);
    RUN_TEST(zeropage_y);

    printf("\nAbsolute Mode:\n");
    RUN_TEST(absolute_lda);
    RUN_TEST(absolute_sta);
    RUN_TEST(absolute_x);
    RUN_TEST(absolute_y);

    printf("\nIndirect Mode:\n");
    RUN_TEST(indirect_jmp);
    RUN_TEST(indirect_x);
    RUN_TEST(indirect_y);

    printf("\nAccumulator Mode:\n");
    RUN_TEST(accumulator_asl);
    RUN_TEST(accumulator_implied);
    RUN_TEST(accumulator_ror);

    printf("\nBranch Instructions:\n");
    RUN_TEST(branch_forward);
    RUN_TEST(branch_backward);
    RUN_TEST(branch_beq);
    RUN_TEST(branch_bcc);

    printf("\nLabels:\n");
    RUN_TEST(forward_reference);
    RUN_TEST(backward_reference);
    RUN_TEST(multiple_labels);
    RUN_TEST(label_expression);

    printf("\nDirectives:\n");
    RUN_TEST(byte_single);
    RUN_TEST(byte_multiple);
    RUN_TEST(word_single);
    RUN_TEST(word_multiple);
    RUN_TEST(text_string);
    RUN_TEST(fill_directive);
    RUN_TEST(fill_zeros);

    printf("\nOrigin:\n");
    RUN_TEST(origin_change);
    RUN_TEST(origin_expression);

    printf("\nAssignments:\n");
    RUN_TEST(assignment_simple);
    RUN_TEST(assignment_expression);
    RUN_TEST(assignment_zeropage);

    printf("\nCase Sensitivity:\n");
    RUN_TEST(lowercase_mnemonic);
    RUN_TEST(mixedcase_mnemonic);
    RUN_TEST(lowercase_register);

    printf("\nIllegal Opcodes:\n");
    RUN_TEST(illegal_lax);
    RUN_TEST(illegal_sax);

    printf("\nComplex Programs:\n");
    RUN_TEST(simple_loop);
    RUN_TEST(data_table_access);
    RUN_TEST(subroutine_call);

    printf("\nError Detection:\n");
    RUN_TEST(undefined_symbol_error);
    RUN_TEST(invalid_mnemonic_error);
    RUN_TEST(branch_out_of_range_forward);

    printf("\n===============\n");
    printf("Results: %d/%d passed\n\n", tests_passed, tests_run);

    return tests_passed == tests_run ? 0 : 1;
}
