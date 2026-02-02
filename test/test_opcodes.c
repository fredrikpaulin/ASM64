/*
 * test_opcodes.c - Unit tests for opcode table
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#include <stdio.h>
#include <string.h>
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

/* ========== Official Opcode Tests ========== */

TEST(lda_immediate) {
    const OpcodeEntry *op = opcode_find("LDA", ADDR_IMMEDIATE);
    return op && op->opcode == 0xA9 && op->size == 2 && op->cycles == 2;
}

TEST(lda_zeropage) {
    const OpcodeEntry *op = opcode_find("LDA", ADDR_ZEROPAGE);
    return op && op->opcode == 0xA5 && op->size == 2 && op->cycles == 3;
}

TEST(lda_zeropage_x) {
    const OpcodeEntry *op = opcode_find("LDA", ADDR_ZEROPAGE_X);
    return op && op->opcode == 0xB5 && op->size == 2 && op->cycles == 4;
}

TEST(lda_absolute) {
    const OpcodeEntry *op = opcode_find("LDA", ADDR_ABSOLUTE);
    return op && op->opcode == 0xAD && op->size == 3 && op->cycles == 4;
}

TEST(lda_absolute_x) {
    const OpcodeEntry *op = opcode_find("LDA", ADDR_ABSOLUTE_X);
    return op && op->opcode == 0xBD && op->size == 3 && op->cycles == 4 && op->page_penalty == 1;
}

TEST(lda_absolute_y) {
    const OpcodeEntry *op = opcode_find("LDA", ADDR_ABSOLUTE_Y);
    return op && op->opcode == 0xB9 && op->size == 3 && op->cycles == 4 && op->page_penalty == 1;
}

TEST(lda_indirect_x) {
    const OpcodeEntry *op = opcode_find("LDA", ADDR_INDIRECT_X);
    return op && op->opcode == 0xA1 && op->size == 2 && op->cycles == 6;
}

TEST(lda_indirect_y) {
    const OpcodeEntry *op = opcode_find("LDA", ADDR_INDIRECT_Y);
    return op && op->opcode == 0xB1 && op->size == 2 && op->cycles == 5 && op->page_penalty == 1;
}

TEST(sta_zeropage) {
    const OpcodeEntry *op = opcode_find("STA", ADDR_ZEROPAGE);
    return op && op->opcode == 0x85 && op->size == 2 && op->cycles == 3;
}

TEST(sta_absolute) {
    const OpcodeEntry *op = opcode_find("STA", ADDR_ABSOLUTE);
    return op && op->opcode == 0x8D && op->size == 3 && op->cycles == 4;
}

TEST(ldx_immediate) {
    const OpcodeEntry *op = opcode_find("LDX", ADDR_IMMEDIATE);
    return op && op->opcode == 0xA2 && op->size == 2 && op->cycles == 2;
}

TEST(ldx_zeropage_y) {
    const OpcodeEntry *op = opcode_find("LDX", ADDR_ZEROPAGE_Y);
    return op && op->opcode == 0xB6 && op->size == 2 && op->cycles == 4;
}

TEST(ldy_immediate) {
    const OpcodeEntry *op = opcode_find("LDY", ADDR_IMMEDIATE);
    return op && op->opcode == 0xA0 && op->size == 2 && op->cycles == 2;
}

TEST(adc_immediate) {
    const OpcodeEntry *op = opcode_find("ADC", ADDR_IMMEDIATE);
    return op && op->opcode == 0x69 && op->size == 2 && op->cycles == 2;
}

TEST(sbc_immediate) {
    const OpcodeEntry *op = opcode_find("SBC", ADDR_IMMEDIATE);
    return op && op->opcode == 0xE9 && op->size == 2 && op->cycles == 2;
}

TEST(and_immediate) {
    const OpcodeEntry *op = opcode_find("AND", ADDR_IMMEDIATE);
    return op && op->opcode == 0x29 && op->size == 2 && op->cycles == 2;
}

TEST(ora_immediate) {
    const OpcodeEntry *op = opcode_find("ORA", ADDR_IMMEDIATE);
    return op && op->opcode == 0x09 && op->size == 2 && op->cycles == 2;
}

TEST(eor_immediate) {
    const OpcodeEntry *op = opcode_find("EOR", ADDR_IMMEDIATE);
    return op && op->opcode == 0x49 && op->size == 2 && op->cycles == 2;
}

/* ===== Shift/Rotate Tests ===== */

TEST(asl_accumulator) {
    const OpcodeEntry *op = opcode_find("ASL", ADDR_ACCUMULATOR);
    return op && op->opcode == 0x0A && op->size == 1 && op->cycles == 2;
}

TEST(asl_zeropage) {
    const OpcodeEntry *op = opcode_find("ASL", ADDR_ZEROPAGE);
    return op && op->opcode == 0x06 && op->size == 2 && op->cycles == 5;
}

TEST(lsr_accumulator) {
    const OpcodeEntry *op = opcode_find("LSR", ADDR_ACCUMULATOR);
    return op && op->opcode == 0x4A && op->size == 1 && op->cycles == 2;
}

TEST(rol_accumulator) {
    const OpcodeEntry *op = opcode_find("ROL", ADDR_ACCUMULATOR);
    return op && op->opcode == 0x2A && op->size == 1 && op->cycles == 2;
}

TEST(ror_accumulator) {
    const OpcodeEntry *op = opcode_find("ROR", ADDR_ACCUMULATOR);
    return op && op->opcode == 0x6A && op->size == 1 && op->cycles == 2;
}

/* ===== Branch Tests ===== */

TEST(bne_relative) {
    const OpcodeEntry *op = opcode_find("BNE", ADDR_RELATIVE);
    return op && op->opcode == 0xD0 && op->size == 2 && op->cycles == 2 && op->page_penalty == 1;
}

TEST(beq_relative) {
    const OpcodeEntry *op = opcode_find("BEQ", ADDR_RELATIVE);
    return op && op->opcode == 0xF0 && op->size == 2 && op->cycles == 2;
}

TEST(bcc_relative) {
    const OpcodeEntry *op = opcode_find("BCC", ADDR_RELATIVE);
    return op && op->opcode == 0x90 && op->size == 2 && op->cycles == 2;
}

TEST(bcs_relative) {
    const OpcodeEntry *op = opcode_find("BCS", ADDR_RELATIVE);
    return op && op->opcode == 0xB0 && op->size == 2 && op->cycles == 2;
}

TEST(bmi_relative) {
    const OpcodeEntry *op = opcode_find("BMI", ADDR_RELATIVE);
    return op && op->opcode == 0x30 && op->size == 2 && op->cycles == 2;
}

TEST(bpl_relative) {
    const OpcodeEntry *op = opcode_find("BPL", ADDR_RELATIVE);
    return op && op->opcode == 0x10 && op->size == 2 && op->cycles == 2;
}

/* ===== Jump/Call Tests ===== */

TEST(jmp_absolute) {
    const OpcodeEntry *op = opcode_find("JMP", ADDR_ABSOLUTE);
    return op && op->opcode == 0x4C && op->size == 3 && op->cycles == 3;
}

TEST(jmp_indirect) {
    const OpcodeEntry *op = opcode_find("JMP", ADDR_INDIRECT);
    return op && op->opcode == 0x6C && op->size == 3 && op->cycles == 5;
}

TEST(jsr_absolute) {
    const OpcodeEntry *op = opcode_find("JSR", ADDR_ABSOLUTE);
    return op && op->opcode == 0x20 && op->size == 3 && op->cycles == 6;
}

TEST(rts_implied) {
    const OpcodeEntry *op = opcode_find("RTS", ADDR_IMPLIED);
    return op && op->opcode == 0x60 && op->size == 1 && op->cycles == 6;
}

TEST(rti_implied) {
    const OpcodeEntry *op = opcode_find("RTI", ADDR_IMPLIED);
    return op && op->opcode == 0x40 && op->size == 1 && op->cycles == 6;
}

/* ===== Implied Mode Tests ===== */

TEST(inx_implied) {
    const OpcodeEntry *op = opcode_find("INX", ADDR_IMPLIED);
    return op && op->opcode == 0xE8 && op->size == 1 && op->cycles == 2;
}

TEST(iny_implied) {
    const OpcodeEntry *op = opcode_find("INY", ADDR_IMPLIED);
    return op && op->opcode == 0xC8 && op->size == 1 && op->cycles == 2;
}

TEST(dex_implied) {
    const OpcodeEntry *op = opcode_find("DEX", ADDR_IMPLIED);
    return op && op->opcode == 0xCA && op->size == 1 && op->cycles == 2;
}

TEST(dey_implied) {
    const OpcodeEntry *op = opcode_find("DEY", ADDR_IMPLIED);
    return op && op->opcode == 0x88 && op->size == 1 && op->cycles == 2;
}

TEST(tax_implied) {
    const OpcodeEntry *op = opcode_find("TAX", ADDR_IMPLIED);
    return op && op->opcode == 0xAA && op->size == 1 && op->cycles == 2;
}

TEST(tay_implied) {
    const OpcodeEntry *op = opcode_find("TAY", ADDR_IMPLIED);
    return op && op->opcode == 0xA8 && op->size == 1 && op->cycles == 2;
}

TEST(txa_implied) {
    const OpcodeEntry *op = opcode_find("TXA", ADDR_IMPLIED);
    return op && op->opcode == 0x8A && op->size == 1 && op->cycles == 2;
}

TEST(tya_implied) {
    const OpcodeEntry *op = opcode_find("TYA", ADDR_IMPLIED);
    return op && op->opcode == 0x98 && op->size == 1 && op->cycles == 2;
}

TEST(nop_implied) {
    const OpcodeEntry *op = opcode_find("NOP", ADDR_IMPLIED);
    return op && op->opcode == 0xEA && op->size == 1 && op->cycles == 2;
}

TEST(brk_implied) {
    const OpcodeEntry *op = opcode_find("BRK", ADDR_IMPLIED);
    return op && op->opcode == 0x00 && op->size == 1 && op->cycles == 7;
}

/* ===== Stack Tests ===== */

TEST(pha_implied) {
    const OpcodeEntry *op = opcode_find("PHA", ADDR_IMPLIED);
    return op && op->opcode == 0x48 && op->size == 1 && op->cycles == 3;
}

TEST(pla_implied) {
    const OpcodeEntry *op = opcode_find("PLA", ADDR_IMPLIED);
    return op && op->opcode == 0x68 && op->size == 1 && op->cycles == 4;
}

TEST(php_implied) {
    const OpcodeEntry *op = opcode_find("PHP", ADDR_IMPLIED);
    return op && op->opcode == 0x08 && op->size == 1 && op->cycles == 3;
}

TEST(plp_implied) {
    const OpcodeEntry *op = opcode_find("PLP", ADDR_IMPLIED);
    return op && op->opcode == 0x28 && op->size == 1 && op->cycles == 4;
}

/* ===== Flag Tests ===== */

TEST(clc_implied) {
    const OpcodeEntry *op = opcode_find("CLC", ADDR_IMPLIED);
    return op && op->opcode == 0x18 && op->size == 1 && op->cycles == 2;
}

TEST(sec_implied) {
    const OpcodeEntry *op = opcode_find("SEC", ADDR_IMPLIED);
    return op && op->opcode == 0x38 && op->size == 1 && op->cycles == 2;
}

TEST(cli_implied) {
    const OpcodeEntry *op = opcode_find("CLI", ADDR_IMPLIED);
    return op && op->opcode == 0x58 && op->size == 1 && op->cycles == 2;
}

TEST(sei_implied) {
    const OpcodeEntry *op = opcode_find("SEI", ADDR_IMPLIED);
    return op && op->opcode == 0x78 && op->size == 1 && op->cycles == 2;
}

/* ===== Compare Tests ===== */

TEST(cmp_immediate) {
    const OpcodeEntry *op = opcode_find("CMP", ADDR_IMMEDIATE);
    return op && op->opcode == 0xC9 && op->size == 2 && op->cycles == 2;
}

TEST(cpx_immediate) {
    const OpcodeEntry *op = opcode_find("CPX", ADDR_IMMEDIATE);
    return op && op->opcode == 0xE0 && op->size == 2 && op->cycles == 2;
}

TEST(cpy_immediate) {
    const OpcodeEntry *op = opcode_find("CPY", ADDR_IMMEDIATE);
    return op && op->opcode == 0xC0 && op->size == 2 && op->cycles == 2;
}

TEST(bit_zeropage) {
    const OpcodeEntry *op = opcode_find("BIT", ADDR_ZEROPAGE);
    return op && op->opcode == 0x24 && op->size == 2 && op->cycles == 3;
}

/* ===== Inc/Dec Memory Tests ===== */

TEST(inc_zeropage) {
    const OpcodeEntry *op = opcode_find("INC", ADDR_ZEROPAGE);
    return op && op->opcode == 0xE6 && op->size == 2 && op->cycles == 5;
}

TEST(dec_zeropage) {
    const OpcodeEntry *op = opcode_find("DEC", ADDR_ZEROPAGE);
    return op && op->opcode == 0xC6 && op->size == 2 && op->cycles == 5;
}

/* ========== Illegal Opcode Tests ========== */

TEST(lax_zeropage) {
    const OpcodeEntry *op = opcode_find("LAX", ADDR_ZEROPAGE);
    return op && op->opcode == 0xA7 && op->size == 2 && op->cycles == 3;
}

TEST(lax_absolute) {
    const OpcodeEntry *op = opcode_find("LAX", ADDR_ABSOLUTE);
    return op && op->opcode == 0xAF && op->size == 3 && op->cycles == 4;
}

TEST(sax_zeropage) {
    const OpcodeEntry *op = opcode_find("SAX", ADDR_ZEROPAGE);
    return op && op->opcode == 0x87 && op->size == 2 && op->cycles == 3;
}

TEST(dcp_zeropage) {
    const OpcodeEntry *op = opcode_find("DCP", ADDR_ZEROPAGE);
    return op && op->opcode == 0xC7 && op->size == 2 && op->cycles == 5;
}

TEST(isc_zeropage) {
    const OpcodeEntry *op = opcode_find("ISC", ADDR_ZEROPAGE);
    return op && op->opcode == 0xE7 && op->size == 2 && op->cycles == 5;
}

TEST(slo_zeropage) {
    const OpcodeEntry *op = opcode_find("SLO", ADDR_ZEROPAGE);
    return op && op->opcode == 0x07 && op->size == 2 && op->cycles == 5;
}

TEST(rla_zeropage) {
    const OpcodeEntry *op = opcode_find("RLA", ADDR_ZEROPAGE);
    return op && op->opcode == 0x27 && op->size == 2 && op->cycles == 5;
}

TEST(sre_zeropage) {
    const OpcodeEntry *op = opcode_find("SRE", ADDR_ZEROPAGE);
    return op && op->opcode == 0x47 && op->size == 2 && op->cycles == 5;
}

TEST(rra_zeropage) {
    const OpcodeEntry *op = opcode_find("RRA", ADDR_ZEROPAGE);
    return op && op->opcode == 0x67 && op->size == 2 && op->cycles == 5;
}

TEST(anc_immediate) {
    const OpcodeEntry *op = opcode_find("ANC", ADDR_IMMEDIATE);
    return op && op->opcode == 0x0B && op->size == 2 && op->cycles == 2;
}

TEST(alr_immediate) {
    const OpcodeEntry *op = opcode_find("ALR", ADDR_IMMEDIATE);
    return op && op->opcode == 0x4B && op->size == 2 && op->cycles == 2;
}

TEST(arr_immediate) {
    const OpcodeEntry *op = opcode_find("ARR", ADDR_IMMEDIATE);
    return op && op->opcode == 0x6B && op->size == 2 && op->cycles == 2;
}

/* ========== Lookup Function Tests ========== */

TEST(case_insensitive_lookup) {
    const OpcodeEntry *op1 = opcode_find("lda", ADDR_IMMEDIATE);
    const OpcodeEntry *op2 = opcode_find("LDA", ADDR_IMMEDIATE);
    const OpcodeEntry *op3 = opcode_find("Lda", ADDR_IMMEDIATE);
    return op1 && op2 && op3 &&
           op1->opcode == op2->opcode &&
           op2->opcode == op3->opcode;
}

TEST(invalid_mode_returns_null) {
    const OpcodeEntry *op = opcode_find("LDA", ADDR_INDIRECT);  /* LDA doesn't have indirect */
    return op == NULL;
}

TEST(invalid_mnemonic_returns_null) {
    const OpcodeEntry *op = opcode_find("XXX", ADDR_IMMEDIATE);
    return op == NULL;
}

TEST(valid_mnemonic_check) {
    return opcode_is_valid_mnemonic("LDA") &&
           opcode_is_valid_mnemonic("lda") &&
           opcode_is_valid_mnemonic("LAX") &&
           !opcode_is_valid_mnemonic("XXX");
}

TEST(illegal_opcode_check) {
    return opcode_is_illegal("LAX") &&
           opcode_is_illegal("SAX") &&
           opcode_is_illegal("DCP") &&
           !opcode_is_illegal("LDA") &&
           !opcode_is_illegal("STA");
}

TEST(get_valid_modes_lda) {
    uint16_t modes = opcode_get_valid_modes("LDA");
    return (modes & (1 << ADDR_IMMEDIATE)) &&
           (modes & (1 << ADDR_ZEROPAGE)) &&
           (modes & (1 << ADDR_ABSOLUTE)) &&
           !(modes & (1 << ADDR_IMPLIED)) &&
           !(modes & (1 << ADDR_INDIRECT));
}

TEST(get_valid_modes_jmp) {
    uint16_t modes = opcode_get_valid_modes("JMP");
    return (modes & (1 << ADDR_ABSOLUTE)) &&
           (modes & (1 << ADDR_INDIRECT)) &&
           !(modes & (1 << ADDR_IMMEDIATE));
}

TEST(get_flags_branch) {
    uint8_t flags = opcode_get_flags("BNE");
    return (flags & INST_BRANCH) != 0;
}

TEST(get_flags_jump) {
    uint8_t flags = opcode_get_flags("JMP");
    return (flags & INST_JUMP) != 0;
}

TEST(get_flags_return) {
    uint8_t flags = opcode_get_flags("RTS");
    return (flags & INST_RETURN) != 0;
}

TEST(get_flags_illegal) {
    uint8_t flags = opcode_get_flags("LAX");
    return (flags & INST_ILLEGAL) != 0;
}

/* ===== Mode Size Tests ===== */

TEST(mode_size_implied) {
    return opcode_mode_size(ADDR_IMPLIED) == 1;
}

TEST(mode_size_immediate) {
    return opcode_mode_size(ADDR_IMMEDIATE) == 2;
}

TEST(mode_size_zeropage) {
    return opcode_mode_size(ADDR_ZEROPAGE) == 2;
}

TEST(mode_size_absolute) {
    return opcode_mode_size(ADDR_ABSOLUTE) == 3;
}

TEST(mode_size_indirect) {
    return opcode_mode_size(ADDR_INDIRECT) == 3;
}

TEST(mode_size_relative) {
    return opcode_mode_size(ADDR_RELATIVE) == 2;
}

/* ===== Mode Name Tests ===== */

TEST(mode_name_immediate) {
    return strcmp(opcode_mode_name(ADDR_IMMEDIATE), "immediate") == 0;
}

TEST(mode_name_zeropage) {
    return strcmp(opcode_mode_name(ADDR_ZEROPAGE), "zero page") == 0;
}

TEST(mode_name_indirect_y) {
    return strcmp(opcode_mode_name(ADDR_INDIRECT_Y), "(indirect),Y") == 0;
}

/* ===== Alias Tests ===== */

TEST(dcm_alias_for_dcp) {
    const OpcodeEntry *dcp = opcode_find("DCP", ADDR_ZEROPAGE);
    const OpcodeEntry *dcm = opcode_find("DCM", ADDR_ZEROPAGE);
    return dcp && dcm && dcp->opcode == dcm->opcode;
}

TEST(isb_alias_for_isc) {
    const OpcodeEntry *isc = opcode_find("ISC", ADDR_ZEROPAGE);
    const OpcodeEntry *isb = opcode_find("ISB", ADDR_ZEROPAGE);
    return isc && isb && isc->opcode == isb->opcode;
}

TEST(aso_alias_for_slo) {
    const OpcodeEntry *slo = opcode_find("SLO", ADDR_ZEROPAGE);
    const OpcodeEntry *aso = opcode_find("ASO", ADDR_ZEROPAGE);
    return slo && aso && slo->opcode == aso->opcode;
}

/* ========== Main ========== */

int main(void) {
    printf("\nOpcode Table Tests\n");
    printf("==================\n\n");

    opcodes_init();

    printf("Official Opcodes - Load/Store:\n");
    RUN_TEST(lda_immediate);
    RUN_TEST(lda_zeropage);
    RUN_TEST(lda_zeropage_x);
    RUN_TEST(lda_absolute);
    RUN_TEST(lda_absolute_x);
    RUN_TEST(lda_absolute_y);
    RUN_TEST(lda_indirect_x);
    RUN_TEST(lda_indirect_y);
    RUN_TEST(sta_zeropage);
    RUN_TEST(sta_absolute);
    RUN_TEST(ldx_immediate);
    RUN_TEST(ldx_zeropage_y);
    RUN_TEST(ldy_immediate);

    printf("\nOfficial Opcodes - Arithmetic:\n");
    RUN_TEST(adc_immediate);
    RUN_TEST(sbc_immediate);
    RUN_TEST(and_immediate);
    RUN_TEST(ora_immediate);
    RUN_TEST(eor_immediate);

    printf("\nOfficial Opcodes - Shift/Rotate:\n");
    RUN_TEST(asl_accumulator);
    RUN_TEST(asl_zeropage);
    RUN_TEST(lsr_accumulator);
    RUN_TEST(rol_accumulator);
    RUN_TEST(ror_accumulator);

    printf("\nOfficial Opcodes - Branch:\n");
    RUN_TEST(bne_relative);
    RUN_TEST(beq_relative);
    RUN_TEST(bcc_relative);
    RUN_TEST(bcs_relative);
    RUN_TEST(bmi_relative);
    RUN_TEST(bpl_relative);

    printf("\nOfficial Opcodes - Jump/Call:\n");
    RUN_TEST(jmp_absolute);
    RUN_TEST(jmp_indirect);
    RUN_TEST(jsr_absolute);
    RUN_TEST(rts_implied);
    RUN_TEST(rti_implied);

    printf("\nOfficial Opcodes - Implied:\n");
    RUN_TEST(inx_implied);
    RUN_TEST(iny_implied);
    RUN_TEST(dex_implied);
    RUN_TEST(dey_implied);
    RUN_TEST(tax_implied);
    RUN_TEST(tay_implied);
    RUN_TEST(txa_implied);
    RUN_TEST(tya_implied);
    RUN_TEST(nop_implied);
    RUN_TEST(brk_implied);

    printf("\nOfficial Opcodes - Stack:\n");
    RUN_TEST(pha_implied);
    RUN_TEST(pla_implied);
    RUN_TEST(php_implied);
    RUN_TEST(plp_implied);

    printf("\nOfficial Opcodes - Flags:\n");
    RUN_TEST(clc_implied);
    RUN_TEST(sec_implied);
    RUN_TEST(cli_implied);
    RUN_TEST(sei_implied);

    printf("\nOfficial Opcodes - Compare:\n");
    RUN_TEST(cmp_immediate);
    RUN_TEST(cpx_immediate);
    RUN_TEST(cpy_immediate);
    RUN_TEST(bit_zeropage);

    printf("\nOfficial Opcodes - Inc/Dec Memory:\n");
    RUN_TEST(inc_zeropage);
    RUN_TEST(dec_zeropage);

    printf("\nIllegal Opcodes:\n");
    RUN_TEST(lax_zeropage);
    RUN_TEST(lax_absolute);
    RUN_TEST(sax_zeropage);
    RUN_TEST(dcp_zeropage);
    RUN_TEST(isc_zeropage);
    RUN_TEST(slo_zeropage);
    RUN_TEST(rla_zeropage);
    RUN_TEST(sre_zeropage);
    RUN_TEST(rra_zeropage);
    RUN_TEST(anc_immediate);
    RUN_TEST(alr_immediate);
    RUN_TEST(arr_immediate);

    printf("\nLookup Functions:\n");
    RUN_TEST(case_insensitive_lookup);
    RUN_TEST(invalid_mode_returns_null);
    RUN_TEST(invalid_mnemonic_returns_null);
    RUN_TEST(valid_mnemonic_check);
    RUN_TEST(illegal_opcode_check);
    RUN_TEST(get_valid_modes_lda);
    RUN_TEST(get_valid_modes_jmp);
    RUN_TEST(get_flags_branch);
    RUN_TEST(get_flags_jump);
    RUN_TEST(get_flags_return);
    RUN_TEST(get_flags_illegal);

    printf("\nMode Size:\n");
    RUN_TEST(mode_size_implied);
    RUN_TEST(mode_size_immediate);
    RUN_TEST(mode_size_zeropage);
    RUN_TEST(mode_size_absolute);
    RUN_TEST(mode_size_indirect);
    RUN_TEST(mode_size_relative);

    printf("\nMode Names:\n");
    RUN_TEST(mode_name_immediate);
    RUN_TEST(mode_name_zeropage);
    RUN_TEST(mode_name_indirect_y);

    printf("\nAliases:\n");
    RUN_TEST(dcm_alias_for_dcp);
    RUN_TEST(isb_alias_for_isc);
    RUN_TEST(aso_alias_for_slo);

    printf("\n==================\n");
    printf("Results: %d/%d passed\n\n", tests_passed, tests_run);

    return tests_passed == tests_run ? 0 : 1;
}
