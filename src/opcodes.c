/*
 * opcodes.c - 6502/6510 Opcode Table Implementation
 * ASM64 - 6502/6510 Assembler for Commodore 64
 *
 * Complete instruction table for official 6502 opcodes plus
 * common illegal/undocumented 6510 opcodes.
 */

#include "opcodes.h"
#include <string.h>
#include <ctype.h>

/* Mode bit masks for valid_modes field */
#define M_IMP  (1 << ADDR_IMPLIED)
#define M_ACC  (1 << ADDR_ACCUMULATOR)
#define M_IMM  (1 << ADDR_IMMEDIATE)
#define M_ZP   (1 << ADDR_ZEROPAGE)
#define M_ZPX  (1 << ADDR_ZEROPAGE_X)
#define M_ZPY  (1 << ADDR_ZEROPAGE_Y)
#define M_ABS  (1 << ADDR_ABSOLUTE)
#define M_ABX  (1 << ADDR_ABSOLUTE_X)
#define M_ABY  (1 << ADDR_ABSOLUTE_Y)
#define M_IND  (1 << ADDR_INDIRECT)
#define M_INX  (1 << ADDR_INDIRECT_X)
#define M_INY  (1 << ADDR_INDIRECT_Y)
#define M_REL  (1 << ADDR_RELATIVE)

/*
 * Complete 6502 opcode table.
 * Each entry: { mnemonic, mode, opcode, size, cycles, page_penalty }
 *
 * page_penalty: 1 if instruction takes +1 cycle when crossing page boundary
 */
static const OpcodeEntry opcode_table[] = {
    /* ===== ADC - Add with Carry ===== */
    { "ADC", ADDR_IMMEDIATE,   0x69, 2, 2, 0 },
    { "ADC", ADDR_ZEROPAGE,    0x65, 2, 3, 0 },
    { "ADC", ADDR_ZEROPAGE_X,  0x75, 2, 4, 0 },
    { "ADC", ADDR_ABSOLUTE,    0x6D, 3, 4, 0 },
    { "ADC", ADDR_ABSOLUTE_X,  0x7D, 3, 4, 1 },
    { "ADC", ADDR_ABSOLUTE_Y,  0x79, 3, 4, 1 },
    { "ADC", ADDR_INDIRECT_X,  0x61, 2, 6, 0 },
    { "ADC", ADDR_INDIRECT_Y,  0x71, 2, 5, 1 },

    /* ===== AND - Logical AND ===== */
    { "AND", ADDR_IMMEDIATE,   0x29, 2, 2, 0 },
    { "AND", ADDR_ZEROPAGE,    0x25, 2, 3, 0 },
    { "AND", ADDR_ZEROPAGE_X,  0x35, 2, 4, 0 },
    { "AND", ADDR_ABSOLUTE,    0x2D, 3, 4, 0 },
    { "AND", ADDR_ABSOLUTE_X,  0x3D, 3, 4, 1 },
    { "AND", ADDR_ABSOLUTE_Y,  0x39, 3, 4, 1 },
    { "AND", ADDR_INDIRECT_X,  0x21, 2, 6, 0 },
    { "AND", ADDR_INDIRECT_Y,  0x31, 2, 5, 1 },

    /* ===== ASL - Arithmetic Shift Left ===== */
    { "ASL", ADDR_ACCUMULATOR, 0x0A, 1, 2, 0 },
    { "ASL", ADDR_ZEROPAGE,    0x06, 2, 5, 0 },
    { "ASL", ADDR_ZEROPAGE_X,  0x16, 2, 6, 0 },
    { "ASL", ADDR_ABSOLUTE,    0x0E, 3, 6, 0 },
    { "ASL", ADDR_ABSOLUTE_X,  0x1E, 3, 7, 0 },

    /* ===== Branch Instructions ===== */
    { "BCC", ADDR_RELATIVE,    0x90, 2, 2, 1 },  /* Branch if Carry Clear */
    { "BCS", ADDR_RELATIVE,    0xB0, 2, 2, 1 },  /* Branch if Carry Set */
    { "BEQ", ADDR_RELATIVE,    0xF0, 2, 2, 1 },  /* Branch if Equal (Z=1) */
    { "BMI", ADDR_RELATIVE,    0x30, 2, 2, 1 },  /* Branch if Minus (N=1) */
    { "BNE", ADDR_RELATIVE,    0xD0, 2, 2, 1 },  /* Branch if Not Equal (Z=0) */
    { "BPL", ADDR_RELATIVE,    0x10, 2, 2, 1 },  /* Branch if Plus (N=0) */
    { "BVC", ADDR_RELATIVE,    0x50, 2, 2, 1 },  /* Branch if Overflow Clear */
    { "BVS", ADDR_RELATIVE,    0x70, 2, 2, 1 },  /* Branch if Overflow Set */

    /* ===== BIT - Bit Test ===== */
    { "BIT", ADDR_ZEROPAGE,    0x24, 2, 3, 0 },
    { "BIT", ADDR_ABSOLUTE,    0x2C, 3, 4, 0 },

    /* ===== BRK - Break ===== */
    { "BRK", ADDR_IMPLIED,     0x00, 1, 7, 0 },

    /* ===== Clear Flag Instructions ===== */
    { "CLC", ADDR_IMPLIED,     0x18, 1, 2, 0 },  /* Clear Carry */
    { "CLD", ADDR_IMPLIED,     0xD8, 1, 2, 0 },  /* Clear Decimal */
    { "CLI", ADDR_IMPLIED,     0x58, 1, 2, 0 },  /* Clear Interrupt Disable */
    { "CLV", ADDR_IMPLIED,     0xB8, 1, 2, 0 },  /* Clear Overflow */

    /* ===== CMP - Compare Accumulator ===== */
    { "CMP", ADDR_IMMEDIATE,   0xC9, 2, 2, 0 },
    { "CMP", ADDR_ZEROPAGE,    0xC5, 2, 3, 0 },
    { "CMP", ADDR_ZEROPAGE_X,  0xD5, 2, 4, 0 },
    { "CMP", ADDR_ABSOLUTE,    0xCD, 3, 4, 0 },
    { "CMP", ADDR_ABSOLUTE_X,  0xDD, 3, 4, 1 },
    { "CMP", ADDR_ABSOLUTE_Y,  0xD9, 3, 4, 1 },
    { "CMP", ADDR_INDIRECT_X,  0xC1, 2, 6, 0 },
    { "CMP", ADDR_INDIRECT_Y,  0xD1, 2, 5, 1 },

    /* ===== CPX - Compare X Register ===== */
    { "CPX", ADDR_IMMEDIATE,   0xE0, 2, 2, 0 },
    { "CPX", ADDR_ZEROPAGE,    0xE4, 2, 3, 0 },
    { "CPX", ADDR_ABSOLUTE,    0xEC, 3, 4, 0 },

    /* ===== CPY - Compare Y Register ===== */
    { "CPY", ADDR_IMMEDIATE,   0xC0, 2, 2, 0 },
    { "CPY", ADDR_ZEROPAGE,    0xC4, 2, 3, 0 },
    { "CPY", ADDR_ABSOLUTE,    0xCC, 3, 4, 0 },

    /* ===== DEC - Decrement Memory ===== */
    { "DEC", ADDR_ZEROPAGE,    0xC6, 2, 5, 0 },
    { "DEC", ADDR_ZEROPAGE_X,  0xD6, 2, 6, 0 },
    { "DEC", ADDR_ABSOLUTE,    0xCE, 3, 6, 0 },
    { "DEC", ADDR_ABSOLUTE_X,  0xDE, 3, 7, 0 },

    /* ===== DEX, DEY - Decrement X, Y ===== */
    { "DEX", ADDR_IMPLIED,     0xCA, 1, 2, 0 },
    { "DEY", ADDR_IMPLIED,     0x88, 1, 2, 0 },

    /* ===== EOR - Exclusive OR ===== */
    { "EOR", ADDR_IMMEDIATE,   0x49, 2, 2, 0 },
    { "EOR", ADDR_ZEROPAGE,    0x45, 2, 3, 0 },
    { "EOR", ADDR_ZEROPAGE_X,  0x55, 2, 4, 0 },
    { "EOR", ADDR_ABSOLUTE,    0x4D, 3, 4, 0 },
    { "EOR", ADDR_ABSOLUTE_X,  0x5D, 3, 4, 1 },
    { "EOR", ADDR_ABSOLUTE_Y,  0x59, 3, 4, 1 },
    { "EOR", ADDR_INDIRECT_X,  0x41, 2, 6, 0 },
    { "EOR", ADDR_INDIRECT_Y,  0x51, 2, 5, 1 },

    /* ===== INC - Increment Memory ===== */
    { "INC", ADDR_ZEROPAGE,    0xE6, 2, 5, 0 },
    { "INC", ADDR_ZEROPAGE_X,  0xF6, 2, 6, 0 },
    { "INC", ADDR_ABSOLUTE,    0xEE, 3, 6, 0 },
    { "INC", ADDR_ABSOLUTE_X,  0xFE, 3, 7, 0 },

    /* ===== INX, INY - Increment X, Y ===== */
    { "INX", ADDR_IMPLIED,     0xE8, 1, 2, 0 },
    { "INY", ADDR_IMPLIED,     0xC8, 1, 2, 0 },

    /* ===== JMP - Jump ===== */
    { "JMP", ADDR_ABSOLUTE,    0x4C, 3, 3, 0 },
    { "JMP", ADDR_INDIRECT,    0x6C, 3, 5, 0 },

    /* ===== JSR - Jump to Subroutine ===== */
    { "JSR", ADDR_ABSOLUTE,    0x20, 3, 6, 0 },

    /* ===== LDA - Load Accumulator ===== */
    { "LDA", ADDR_IMMEDIATE,   0xA9, 2, 2, 0 },
    { "LDA", ADDR_ZEROPAGE,    0xA5, 2, 3, 0 },
    { "LDA", ADDR_ZEROPAGE_X,  0xB5, 2, 4, 0 },
    { "LDA", ADDR_ABSOLUTE,    0xAD, 3, 4, 0 },
    { "LDA", ADDR_ABSOLUTE_X,  0xBD, 3, 4, 1 },
    { "LDA", ADDR_ABSOLUTE_Y,  0xB9, 3, 4, 1 },
    { "LDA", ADDR_INDIRECT_X,  0xA1, 2, 6, 0 },
    { "LDA", ADDR_INDIRECT_Y,  0xB1, 2, 5, 1 },

    /* ===== LDX - Load X Register ===== */
    { "LDX", ADDR_IMMEDIATE,   0xA2, 2, 2, 0 },
    { "LDX", ADDR_ZEROPAGE,    0xA6, 2, 3, 0 },
    { "LDX", ADDR_ZEROPAGE_Y,  0xB6, 2, 4, 0 },
    { "LDX", ADDR_ABSOLUTE,    0xAE, 3, 4, 0 },
    { "LDX", ADDR_ABSOLUTE_Y,  0xBE, 3, 4, 1 },

    /* ===== LDY - Load Y Register ===== */
    { "LDY", ADDR_IMMEDIATE,   0xA0, 2, 2, 0 },
    { "LDY", ADDR_ZEROPAGE,    0xA4, 2, 3, 0 },
    { "LDY", ADDR_ZEROPAGE_X,  0xB4, 2, 4, 0 },
    { "LDY", ADDR_ABSOLUTE,    0xAC, 3, 4, 0 },
    { "LDY", ADDR_ABSOLUTE_X,  0xBC, 3, 4, 1 },

    /* ===== LSR - Logical Shift Right ===== */
    { "LSR", ADDR_ACCUMULATOR, 0x4A, 1, 2, 0 },
    { "LSR", ADDR_ZEROPAGE,    0x46, 2, 5, 0 },
    { "LSR", ADDR_ZEROPAGE_X,  0x56, 2, 6, 0 },
    { "LSR", ADDR_ABSOLUTE,    0x4E, 3, 6, 0 },
    { "LSR", ADDR_ABSOLUTE_X,  0x5E, 3, 7, 0 },

    /* ===== NOP - No Operation ===== */
    { "NOP", ADDR_IMPLIED,     0xEA, 1, 2, 0 },

    /* ===== ORA - Logical OR ===== */
    { "ORA", ADDR_IMMEDIATE,   0x09, 2, 2, 0 },
    { "ORA", ADDR_ZEROPAGE,    0x05, 2, 3, 0 },
    { "ORA", ADDR_ZEROPAGE_X,  0x15, 2, 4, 0 },
    { "ORA", ADDR_ABSOLUTE,    0x0D, 3, 4, 0 },
    { "ORA", ADDR_ABSOLUTE_X,  0x1D, 3, 4, 1 },
    { "ORA", ADDR_ABSOLUTE_Y,  0x19, 3, 4, 1 },
    { "ORA", ADDR_INDIRECT_X,  0x01, 2, 6, 0 },
    { "ORA", ADDR_INDIRECT_Y,  0x11, 2, 5, 1 },

    /* ===== Stack Instructions ===== */
    { "PHA", ADDR_IMPLIED,     0x48, 1, 3, 0 },  /* Push Accumulator */
    { "PHP", ADDR_IMPLIED,     0x08, 1, 3, 0 },  /* Push Processor Status */
    { "PLA", ADDR_IMPLIED,     0x68, 1, 4, 0 },  /* Pull Accumulator */
    { "PLP", ADDR_IMPLIED,     0x28, 1, 4, 0 },  /* Pull Processor Status */

    /* ===== ROL - Rotate Left ===== */
    { "ROL", ADDR_ACCUMULATOR, 0x2A, 1, 2, 0 },
    { "ROL", ADDR_ZEROPAGE,    0x26, 2, 5, 0 },
    { "ROL", ADDR_ZEROPAGE_X,  0x36, 2, 6, 0 },
    { "ROL", ADDR_ABSOLUTE,    0x2E, 3, 6, 0 },
    { "ROL", ADDR_ABSOLUTE_X,  0x3E, 3, 7, 0 },

    /* ===== ROR - Rotate Right ===== */
    { "ROR", ADDR_ACCUMULATOR, 0x6A, 1, 2, 0 },
    { "ROR", ADDR_ZEROPAGE,    0x66, 2, 5, 0 },
    { "ROR", ADDR_ZEROPAGE_X,  0x76, 2, 6, 0 },
    { "ROR", ADDR_ABSOLUTE,    0x6E, 3, 6, 0 },
    { "ROR", ADDR_ABSOLUTE_X,  0x7E, 3, 7, 0 },

    /* ===== RTI - Return from Interrupt ===== */
    { "RTI", ADDR_IMPLIED,     0x40, 1, 6, 0 },

    /* ===== RTS - Return from Subroutine ===== */
    { "RTS", ADDR_IMPLIED,     0x60, 1, 6, 0 },

    /* ===== SBC - Subtract with Carry ===== */
    { "SBC", ADDR_IMMEDIATE,   0xE9, 2, 2, 0 },
    { "SBC", ADDR_ZEROPAGE,    0xE5, 2, 3, 0 },
    { "SBC", ADDR_ZEROPAGE_X,  0xF5, 2, 4, 0 },
    { "SBC", ADDR_ABSOLUTE,    0xED, 3, 4, 0 },
    { "SBC", ADDR_ABSOLUTE_X,  0xFD, 3, 4, 1 },
    { "SBC", ADDR_ABSOLUTE_Y,  0xF9, 3, 4, 1 },
    { "SBC", ADDR_INDIRECT_X,  0xE1, 2, 6, 0 },
    { "SBC", ADDR_INDIRECT_Y,  0xF1, 2, 5, 1 },

    /* ===== Set Flag Instructions ===== */
    { "SEC", ADDR_IMPLIED,     0x38, 1, 2, 0 },  /* Set Carry */
    { "SED", ADDR_IMPLIED,     0xF8, 1, 2, 0 },  /* Set Decimal */
    { "SEI", ADDR_IMPLIED,     0x78, 1, 2, 0 },  /* Set Interrupt Disable */

    /* ===== STA - Store Accumulator ===== */
    { "STA", ADDR_ZEROPAGE,    0x85, 2, 3, 0 },
    { "STA", ADDR_ZEROPAGE_X,  0x95, 2, 4, 0 },
    { "STA", ADDR_ABSOLUTE,    0x8D, 3, 4, 0 },
    { "STA", ADDR_ABSOLUTE_X,  0x9D, 3, 5, 0 },
    { "STA", ADDR_ABSOLUTE_Y,  0x99, 3, 5, 0 },
    { "STA", ADDR_INDIRECT_X,  0x81, 2, 6, 0 },
    { "STA", ADDR_INDIRECT_Y,  0x91, 2, 6, 0 },

    /* ===== STX - Store X Register ===== */
    { "STX", ADDR_ZEROPAGE,    0x86, 2, 3, 0 },
    { "STX", ADDR_ZEROPAGE_Y,  0x96, 2, 4, 0 },
    { "STX", ADDR_ABSOLUTE,    0x8E, 3, 4, 0 },

    /* ===== STY - Store Y Register ===== */
    { "STY", ADDR_ZEROPAGE,    0x84, 2, 3, 0 },
    { "STY", ADDR_ZEROPAGE_X,  0x94, 2, 4, 0 },
    { "STY", ADDR_ABSOLUTE,    0x8C, 3, 4, 0 },

    /* ===== Transfer Instructions ===== */
    { "TAX", ADDR_IMPLIED,     0xAA, 1, 2, 0 },  /* Transfer A to X */
    { "TAY", ADDR_IMPLIED,     0xA8, 1, 2, 0 },  /* Transfer A to Y */
    { "TSX", ADDR_IMPLIED,     0xBA, 1, 2, 0 },  /* Transfer S to X */
    { "TXA", ADDR_IMPLIED,     0x8A, 1, 2, 0 },  /* Transfer X to A */
    { "TXS", ADDR_IMPLIED,     0x9A, 1, 2, 0 },  /* Transfer X to S */
    { "TYA", ADDR_IMPLIED,     0x98, 1, 2, 0 },  /* Transfer Y to A */

    /* ========================================= */
    /* ===== ILLEGAL/UNDOCUMENTED OPCODES ===== */
    /* ========================================= */

    /* ===== LAX - Load A and X ===== */
    { "LAX", ADDR_ZEROPAGE,    0xA7, 2, 3, 0 },
    { "LAX", ADDR_ZEROPAGE_Y,  0xB7, 2, 4, 0 },
    { "LAX", ADDR_ABSOLUTE,    0xAF, 3, 4, 0 },
    { "LAX", ADDR_ABSOLUTE_Y,  0xBF, 3, 4, 1 },
    { "LAX", ADDR_INDIRECT_X,  0xA3, 2, 6, 0 },
    { "LAX", ADDR_INDIRECT_Y,  0xB3, 2, 5, 1 },

    /* ===== SAX - Store A AND X ===== */
    { "SAX", ADDR_ZEROPAGE,    0x87, 2, 3, 0 },
    { "SAX", ADDR_ZEROPAGE_Y,  0x97, 2, 4, 0 },
    { "SAX", ADDR_ABSOLUTE,    0x8F, 3, 4, 0 },
    { "SAX", ADDR_INDIRECT_X,  0x83, 2, 6, 0 },

    /* ===== DCP - Decrement then Compare ===== */
    { "DCP", ADDR_ZEROPAGE,    0xC7, 2, 5, 0 },
    { "DCP", ADDR_ZEROPAGE_X,  0xD7, 2, 6, 0 },
    { "DCP", ADDR_ABSOLUTE,    0xCF, 3, 6, 0 },
    { "DCP", ADDR_ABSOLUTE_X,  0xDF, 3, 7, 0 },
    { "DCP", ADDR_ABSOLUTE_Y,  0xDB, 3, 7, 0 },
    { "DCP", ADDR_INDIRECT_X,  0xC3, 2, 8, 0 },
    { "DCP", ADDR_INDIRECT_Y,  0xD3, 2, 8, 0 },
    /* DCM is alias for DCP */
    { "DCM", ADDR_ZEROPAGE,    0xC7, 2, 5, 0 },
    { "DCM", ADDR_ZEROPAGE_X,  0xD7, 2, 6, 0 },
    { "DCM", ADDR_ABSOLUTE,    0xCF, 3, 6, 0 },
    { "DCM", ADDR_ABSOLUTE_X,  0xDF, 3, 7, 0 },
    { "DCM", ADDR_ABSOLUTE_Y,  0xDB, 3, 7, 0 },
    { "DCM", ADDR_INDIRECT_X,  0xC3, 2, 8, 0 },
    { "DCM", ADDR_INDIRECT_Y,  0xD3, 2, 8, 0 },

    /* ===== ISC/ISB - Increment then Subtract ===== */
    { "ISC", ADDR_ZEROPAGE,    0xE7, 2, 5, 0 },
    { "ISC", ADDR_ZEROPAGE_X,  0xF7, 2, 6, 0 },
    { "ISC", ADDR_ABSOLUTE,    0xEF, 3, 6, 0 },
    { "ISC", ADDR_ABSOLUTE_X,  0xFF, 3, 7, 0 },
    { "ISC", ADDR_ABSOLUTE_Y,  0xFB, 3, 7, 0 },
    { "ISC", ADDR_INDIRECT_X,  0xE3, 2, 8, 0 },
    { "ISC", ADDR_INDIRECT_Y,  0xF3, 2, 8, 0 },
    /* ISB/INS are aliases for ISC */
    { "ISB", ADDR_ZEROPAGE,    0xE7, 2, 5, 0 },
    { "ISB", ADDR_ZEROPAGE_X,  0xF7, 2, 6, 0 },
    { "ISB", ADDR_ABSOLUTE,    0xEF, 3, 6, 0 },
    { "ISB", ADDR_ABSOLUTE_X,  0xFF, 3, 7, 0 },
    { "ISB", ADDR_ABSOLUTE_Y,  0xFB, 3, 7, 0 },
    { "ISB", ADDR_INDIRECT_X,  0xE3, 2, 8, 0 },
    { "ISB", ADDR_INDIRECT_Y,  0xF3, 2, 8, 0 },
    { "INS", ADDR_ZEROPAGE,    0xE7, 2, 5, 0 },
    { "INS", ADDR_ZEROPAGE_X,  0xF7, 2, 6, 0 },
    { "INS", ADDR_ABSOLUTE,    0xEF, 3, 6, 0 },
    { "INS", ADDR_ABSOLUTE_X,  0xFF, 3, 7, 0 },
    { "INS", ADDR_ABSOLUTE_Y,  0xFB, 3, 7, 0 },
    { "INS", ADDR_INDIRECT_X,  0xE3, 2, 8, 0 },
    { "INS", ADDR_INDIRECT_Y,  0xF3, 2, 8, 0 },

    /* ===== SLO - Shift Left then OR ===== */
    { "SLO", ADDR_ZEROPAGE,    0x07, 2, 5, 0 },
    { "SLO", ADDR_ZEROPAGE_X,  0x17, 2, 6, 0 },
    { "SLO", ADDR_ABSOLUTE,    0x0F, 3, 6, 0 },
    { "SLO", ADDR_ABSOLUTE_X,  0x1F, 3, 7, 0 },
    { "SLO", ADDR_ABSOLUTE_Y,  0x1B, 3, 7, 0 },
    { "SLO", ADDR_INDIRECT_X,  0x03, 2, 8, 0 },
    { "SLO", ADDR_INDIRECT_Y,  0x13, 2, 8, 0 },
    /* ASO is alias for SLO */
    { "ASO", ADDR_ZEROPAGE,    0x07, 2, 5, 0 },
    { "ASO", ADDR_ZEROPAGE_X,  0x17, 2, 6, 0 },
    { "ASO", ADDR_ABSOLUTE,    0x0F, 3, 6, 0 },
    { "ASO", ADDR_ABSOLUTE_X,  0x1F, 3, 7, 0 },
    { "ASO", ADDR_ABSOLUTE_Y,  0x1B, 3, 7, 0 },
    { "ASO", ADDR_INDIRECT_X,  0x03, 2, 8, 0 },
    { "ASO", ADDR_INDIRECT_Y,  0x13, 2, 8, 0 },

    /* ===== RLA - Rotate Left then AND ===== */
    { "RLA", ADDR_ZEROPAGE,    0x27, 2, 5, 0 },
    { "RLA", ADDR_ZEROPAGE_X,  0x37, 2, 6, 0 },
    { "RLA", ADDR_ABSOLUTE,    0x2F, 3, 6, 0 },
    { "RLA", ADDR_ABSOLUTE_X,  0x3F, 3, 7, 0 },
    { "RLA", ADDR_ABSOLUTE_Y,  0x3B, 3, 7, 0 },
    { "RLA", ADDR_INDIRECT_X,  0x23, 2, 8, 0 },
    { "RLA", ADDR_INDIRECT_Y,  0x33, 2, 8, 0 },

    /* ===== SRE - Shift Right then EOR ===== */
    { "SRE", ADDR_ZEROPAGE,    0x47, 2, 5, 0 },
    { "SRE", ADDR_ZEROPAGE_X,  0x57, 2, 6, 0 },
    { "SRE", ADDR_ABSOLUTE,    0x4F, 3, 6, 0 },
    { "SRE", ADDR_ABSOLUTE_X,  0x5F, 3, 7, 0 },
    { "SRE", ADDR_ABSOLUTE_Y,  0x5B, 3, 7, 0 },
    { "SRE", ADDR_INDIRECT_X,  0x43, 2, 8, 0 },
    { "SRE", ADDR_INDIRECT_Y,  0x53, 2, 8, 0 },
    /* LSE is alias for SRE */
    { "LSE", ADDR_ZEROPAGE,    0x47, 2, 5, 0 },
    { "LSE", ADDR_ZEROPAGE_X,  0x57, 2, 6, 0 },
    { "LSE", ADDR_ABSOLUTE,    0x4F, 3, 6, 0 },
    { "LSE", ADDR_ABSOLUTE_X,  0x5F, 3, 7, 0 },
    { "LSE", ADDR_ABSOLUTE_Y,  0x5B, 3, 7, 0 },
    { "LSE", ADDR_INDIRECT_X,  0x43, 2, 8, 0 },
    { "LSE", ADDR_INDIRECT_Y,  0x53, 2, 8, 0 },

    /* ===== RRA - Rotate Right then ADC ===== */
    { "RRA", ADDR_ZEROPAGE,    0x67, 2, 5, 0 },
    { "RRA", ADDR_ZEROPAGE_X,  0x77, 2, 6, 0 },
    { "RRA", ADDR_ABSOLUTE,    0x6F, 3, 6, 0 },
    { "RRA", ADDR_ABSOLUTE_X,  0x7F, 3, 7, 0 },
    { "RRA", ADDR_ABSOLUTE_Y,  0x7B, 3, 7, 0 },
    { "RRA", ADDR_INDIRECT_X,  0x63, 2, 8, 0 },
    { "RRA", ADDR_INDIRECT_Y,  0x73, 2, 8, 0 },

    /* ===== ANC - AND with Carry ===== */
    { "ANC", ADDR_IMMEDIATE,   0x0B, 2, 2, 0 },
    /* ANC2 is alternate opcode */
    { "ANC2", ADDR_IMMEDIATE,  0x2B, 2, 2, 0 },

    /* ===== ALR/ASR - AND then LSR ===== */
    { "ALR", ADDR_IMMEDIATE,   0x4B, 2, 2, 0 },
    { "ASR", ADDR_IMMEDIATE,   0x4B, 2, 2, 0 },

    /* ===== ARR - AND then ROR ===== */
    { "ARR", ADDR_IMMEDIATE,   0x6B, 2, 2, 0 },

    /* ===== XAA/ANE - A AND X AND imm ===== */
    { "XAA", ADDR_IMMEDIATE,   0x8B, 2, 2, 0 },
    { "ANE", ADDR_IMMEDIATE,   0x8B, 2, 2, 0 },

    /* ===== AHX/SHA - Store A AND X AND high ===== */
    { "AHX", ADDR_ABSOLUTE_Y,  0x9F, 3, 5, 0 },
    { "AHX", ADDR_INDIRECT_Y,  0x93, 2, 6, 0 },
    { "SHA", ADDR_ABSOLUTE_Y,  0x9F, 3, 5, 0 },
    { "SHA", ADDR_INDIRECT_Y,  0x93, 2, 6, 0 },

    /* ===== TAS/SHS - Transfer A AND X to S ===== */
    { "TAS", ADDR_ABSOLUTE_Y,  0x9B, 3, 5, 0 },
    { "SHS", ADDR_ABSOLUTE_Y,  0x9B, 3, 5, 0 },

    /* ===== SHX/SXA - Store X AND high ===== */
    { "SHX", ADDR_ABSOLUTE_Y,  0x9E, 3, 5, 0 },
    { "SXA", ADDR_ABSOLUTE_Y,  0x9E, 3, 5, 0 },

    /* ===== SHY/SYA - Store Y AND high ===== */
    { "SHY", ADDR_ABSOLUTE_X,  0x9C, 3, 5, 0 },
    { "SYA", ADDR_ABSOLUTE_X,  0x9C, 3, 5, 0 },

    /* ===== LAS - LDA AND TSX ===== */
    { "LAS", ADDR_ABSOLUTE_Y,  0xBB, 3, 4, 1 },
    { "LAR", ADDR_ABSOLUTE_Y,  0xBB, 3, 4, 1 },

    /* ===== SBC alternate (undocumented EB) ===== */
    { "USB", ADDR_IMMEDIATE,   0xEB, 2, 2, 0 },

    /* ===== NOP variants ===== */
    { "DOP", ADDR_IMMEDIATE,   0x80, 2, 2, 0 },  /* Double NOP */
    { "DOP", ADDR_ZEROPAGE,    0x04, 2, 3, 0 },
    { "DOP", ADDR_ZEROPAGE_X,  0x14, 2, 4, 0 },
    { "TOP", ADDR_ABSOLUTE,    0x0C, 3, 4, 0 },  /* Triple NOP */
    { "TOP", ADDR_ABSOLUTE_X,  0x1C, 3, 4, 1 },

    /* ===== JAM/KIL - Halt processor ===== */
    { "JAM", ADDR_IMPLIED,     0x02, 1, 0, 0 },
    { "KIL", ADDR_IMPLIED,     0x02, 1, 0, 0 },
    { "HLT", ADDR_IMPLIED,     0x02, 1, 0, 0 },

    /* Sentinel - end of table */
    { NULL, ADDR_INVALID, 0, 0, 0, 0 }
};

/* Number of entries (calculated at init) */
static int opcode_count = 0;

/* Mnemonic information table for quick lookup */
static const MnemonicInfo mnemonic_info[] = {
    /* Official instructions */
    { "ADC", M_IMM|M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_NONE },
    { "AND", M_IMM|M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_NONE },
    { "ASL", M_ACC|M_ZP|M_ZPX|M_ABS|M_ABX, INST_NONE },
    { "BCC", M_REL, INST_BRANCH },
    { "BCS", M_REL, INST_BRANCH },
    { "BEQ", M_REL, INST_BRANCH },
    { "BIT", M_ZP|M_ABS, INST_NONE },
    { "BMI", M_REL, INST_BRANCH },
    { "BNE", M_REL, INST_BRANCH },
    { "BPL", M_REL, INST_BRANCH },
    { "BRK", M_IMP, INST_BREAK },
    { "BVC", M_REL, INST_BRANCH },
    { "BVS", M_REL, INST_BRANCH },
    { "CLC", M_IMP, INST_NONE },
    { "CLD", M_IMP, INST_NONE },
    { "CLI", M_IMP, INST_NONE },
    { "CLV", M_IMP, INST_NONE },
    { "CMP", M_IMM|M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_NONE },
    { "CPX", M_IMM|M_ZP|M_ABS, INST_NONE },
    { "CPY", M_IMM|M_ZP|M_ABS, INST_NONE },
    { "DEC", M_ZP|M_ZPX|M_ABS|M_ABX, INST_NONE },
    { "DEX", M_IMP, INST_NONE },
    { "DEY", M_IMP, INST_NONE },
    { "EOR", M_IMM|M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_NONE },
    { "INC", M_ZP|M_ZPX|M_ABS|M_ABX, INST_NONE },
    { "INX", M_IMP, INST_NONE },
    { "INY", M_IMP, INST_NONE },
    { "JMP", M_ABS|M_IND, INST_JUMP },
    { "JSR", M_ABS, INST_JUMP },
    { "LDA", M_IMM|M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_NONE },
    { "LDX", M_IMM|M_ZP|M_ZPY|M_ABS|M_ABY, INST_NONE },
    { "LDY", M_IMM|M_ZP|M_ZPX|M_ABS|M_ABX, INST_NONE },
    { "LSR", M_ACC|M_ZP|M_ZPX|M_ABS|M_ABX, INST_NONE },
    { "NOP", M_IMP, INST_NONE },
    { "ORA", M_IMM|M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_NONE },
    { "PHA", M_IMP, INST_STACK },
    { "PHP", M_IMP, INST_STACK },
    { "PLA", M_IMP, INST_STACK },
    { "PLP", M_IMP, INST_STACK },
    { "ROL", M_ACC|M_ZP|M_ZPX|M_ABS|M_ABX, INST_NONE },
    { "ROR", M_ACC|M_ZP|M_ZPX|M_ABS|M_ABX, INST_NONE },
    { "RTI", M_IMP, INST_RETURN },
    { "RTS", M_IMP, INST_RETURN },
    { "SBC", M_IMM|M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_NONE },
    { "SEC", M_IMP, INST_NONE },
    { "SED", M_IMP, INST_NONE },
    { "SEI", M_IMP, INST_NONE },
    { "STA", M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_NONE },
    { "STX", M_ZP|M_ZPY|M_ABS, INST_NONE },
    { "STY", M_ZP|M_ZPX|M_ABS, INST_NONE },
    { "TAX", M_IMP, INST_NONE },
    { "TAY", M_IMP, INST_NONE },
    { "TSX", M_IMP, INST_NONE },
    { "TXA", M_IMP, INST_NONE },
    { "TXS", M_IMP, INST_NONE },
    { "TYA", M_IMP, INST_NONE },

    /* Illegal instructions */
    { "LAX", M_ZP|M_ZPY|M_ABS|M_ABY|M_INX|M_INY, INST_ILLEGAL },
    { "SAX", M_ZP|M_ZPY|M_ABS|M_INX, INST_ILLEGAL },
    { "DCP", M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_ILLEGAL },
    { "DCM", M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_ILLEGAL },
    { "ISC", M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_ILLEGAL },
    { "ISB", M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_ILLEGAL },
    { "INS", M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_ILLEGAL },
    { "SLO", M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_ILLEGAL },
    { "ASO", M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_ILLEGAL },
    { "RLA", M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_ILLEGAL },
    { "SRE", M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_ILLEGAL },
    { "LSE", M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_ILLEGAL },
    { "RRA", M_ZP|M_ZPX|M_ABS|M_ABX|M_ABY|M_INX|M_INY, INST_ILLEGAL },
    { "ANC", M_IMM, INST_ILLEGAL },
    { "ANC2", M_IMM, INST_ILLEGAL },
    { "ALR", M_IMM, INST_ILLEGAL },
    { "ASR", M_IMM, INST_ILLEGAL },
    { "ARR", M_IMM, INST_ILLEGAL },
    { "XAA", M_IMM, INST_ILLEGAL },
    { "ANE", M_IMM, INST_ILLEGAL },
    { "AHX", M_ABY|M_INY, INST_ILLEGAL },
    { "SHA", M_ABY|M_INY, INST_ILLEGAL },
    { "TAS", M_ABY, INST_ILLEGAL },
    { "SHS", M_ABY, INST_ILLEGAL },
    { "SHX", M_ABY, INST_ILLEGAL },
    { "SXA", M_ABY, INST_ILLEGAL },
    { "SHY", M_ABX, INST_ILLEGAL },
    { "SYA", M_ABX, INST_ILLEGAL },
    { "LAS", M_ABY, INST_ILLEGAL },
    { "LAR", M_ABY, INST_ILLEGAL },
    { "USB", M_IMM, INST_ILLEGAL },
    { "DOP", M_IMM|M_ZP|M_ZPX, INST_ILLEGAL },
    { "TOP", M_ABS|M_ABX, INST_ILLEGAL },
    { "JAM", M_IMP, INST_ILLEGAL },
    { "KIL", M_IMP, INST_ILLEGAL },
    { "HLT", M_IMP, INST_ILLEGAL },

    /* Sentinel */
    { NULL, 0, 0 }
};

/* Addressing mode names for error messages */
static const char *mode_names[] = {
    "implied",
    "accumulator",
    "immediate",
    "zero page",
    "zero page,X",
    "zero page,Y",
    "absolute",
    "absolute,X",
    "absolute,Y",
    "indirect",
    "(indirect,X)",
    "(indirect),Y",
    "relative",
    "invalid"
};

/* Addressing mode sizes */
static const int mode_sizes[] = {
    1,  /* IMPLIED */
    1,  /* ACCUMULATOR */
    2,  /* IMMEDIATE */
    2,  /* ZEROPAGE */
    2,  /* ZEROPAGE_X */
    2,  /* ZEROPAGE_Y */
    3,  /* ABSOLUTE */
    3,  /* ABSOLUTE_X */
    3,  /* ABSOLUTE_Y */
    3,  /* INDIRECT */
    2,  /* INDIRECT_X */
    2,  /* INDIRECT_Y */
    2,  /* RELATIVE */
    0   /* INVALID */
};

/* Case-insensitive string comparison */
static int strcasecmp_local(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int c1 = toupper((unsigned char)*s1);
        int c2 = toupper((unsigned char)*s2);
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return toupper((unsigned char)*s1) - toupper((unsigned char)*s2);
}

/* Initialize opcode tables */
void opcodes_init(void) {
    /* Count entries in opcode table */
    opcode_count = 0;
    while (opcode_table[opcode_count].mnemonic != NULL) {
        opcode_count++;
    }
}

/* Find opcode entry by mnemonic and addressing mode */
const OpcodeEntry *opcode_find(const char *mnemonic, AddressingMode mode) {
    for (int i = 0; i < opcode_count; i++) {
        if (strcasecmp_local(opcode_table[i].mnemonic, mnemonic) == 0 &&
            opcode_table[i].mode == mode) {
            return &opcode_table[i];
        }
    }
    return NULL;
}

/* Get valid addressing modes for a mnemonic */
uint16_t opcode_get_valid_modes(const char *mnemonic) {
    for (int i = 0; mnemonic_info[i].mnemonic != NULL; i++) {
        if (strcasecmp_local(mnemonic_info[i].mnemonic, mnemonic) == 0) {
            return mnemonic_info[i].valid_modes;
        }
    }
    return 0;
}

/* Get instruction flags for a mnemonic */
uint8_t opcode_get_flags(const char *mnemonic) {
    for (int i = 0; mnemonic_info[i].mnemonic != NULL; i++) {
        if (strcasecmp_local(mnemonic_info[i].mnemonic, mnemonic) == 0) {
            return mnemonic_info[i].flags;
        }
    }
    return INST_NONE;
}

/* Check if mnemonic is valid */
int opcode_is_valid_mnemonic(const char *mnemonic) {
    for (int i = 0; mnemonic_info[i].mnemonic != NULL; i++) {
        if (strcasecmp_local(mnemonic_info[i].mnemonic, mnemonic) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Check if mnemonic is an illegal opcode */
int opcode_is_illegal(const char *mnemonic) {
    for (int i = 0; mnemonic_info[i].mnemonic != NULL; i++) {
        if (strcasecmp_local(mnemonic_info[i].mnemonic, mnemonic) == 0) {
            return (mnemonic_info[i].flags & INST_ILLEGAL) != 0;
        }
    }
    return 0;
}

/* Get instruction size for addressing mode */
int opcode_mode_size(AddressingMode mode) {
    if (mode >= 0 && mode <= ADDR_INVALID) {
        return mode_sizes[mode];
    }
    return 0;
}

/* Get addressing mode name */
const char *opcode_mode_name(AddressingMode mode) {
    if (mode >= 0 && mode <= ADDR_INVALID) {
        return mode_names[mode];
    }
    return "unknown";
}

/* Find opcode entry by opcode byte */
const OpcodeEntry *opcode_find_by_opcode(uint8_t opcode) {
    for (int i = 0; i < opcode_count; i++) {
        if (opcode_table[i].opcode == opcode) {
            return &opcode_table[i];
        }
    }
    return NULL;
}
