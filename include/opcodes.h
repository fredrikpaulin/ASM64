/*
 * opcodes.h - 6502/6510 Opcode Definitions
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>

/* Addressing modes for 6502/6510 */
typedef enum {
    ADDR_IMPLIED,       /* INX, DEX, etc. - no operand */
    ADDR_ACCUMULATOR,   /* ASL A, ROL A, etc. */
    ADDR_IMMEDIATE,     /* LDA #$10 */
    ADDR_ZEROPAGE,      /* LDA $80 */
    ADDR_ZEROPAGE_X,    /* LDA $80,X */
    ADDR_ZEROPAGE_Y,    /* LDX $80,Y */
    ADDR_ABSOLUTE,      /* LDA $1000 */
    ADDR_ABSOLUTE_X,    /* LDA $1000,X */
    ADDR_ABSOLUTE_Y,    /* LDA $1000,Y */
    ADDR_INDIRECT,      /* JMP ($FFFC) */
    ADDR_INDIRECT_X,    /* LDA ($80,X) - indexed indirect */
    ADDR_INDIRECT_Y,    /* LDA ($80),Y - indirect indexed */
    ADDR_RELATIVE,      /* BNE label - branches */
    ADDR_INVALID        /* Invalid/unsupported mode */
} AddressingMode;

/* Opcode entry in the instruction table */
typedef struct {
    const char *mnemonic;   /* Instruction name (uppercase) */
    AddressingMode mode;    /* Addressing mode */
    uint8_t opcode;         /* Machine code byte */
    uint8_t size;           /* Instruction size: 1, 2, or 3 bytes */
    uint8_t cycles;         /* Base cycle count */
    uint8_t page_penalty;   /* 1 if +1 cycle on page crossing */
} OpcodeEntry;

/* Instruction flags for categorization */
typedef enum {
    INST_NONE       = 0x00,
    INST_BRANCH     = 0x01,  /* Branch instruction (BNE, BEQ, etc.) */
    INST_JUMP       = 0x02,  /* Jump instruction (JMP, JSR) */
    INST_RETURN     = 0x04,  /* Return instruction (RTS, RTI) */
    INST_ILLEGAL    = 0x08,  /* Illegal/undocumented opcode */
    INST_STACK      = 0x10,  /* Stack operation (PHA, PLA, etc.) */
    INST_BREAK      = 0x20   /* BRK instruction */
} InstructionFlags;

/* Mnemonic info - describes what modes an instruction supports */
typedef struct {
    const char *mnemonic;
    uint16_t valid_modes;    /* Bitmask of valid AddressingMode values */
    uint8_t flags;           /* InstructionFlags */
} MnemonicInfo;

/* ========== Opcode Lookup Functions ========== */

/*
 * Find opcode entry by mnemonic and addressing mode.
 * Returns NULL if the combination is invalid.
 */
const OpcodeEntry *opcode_find(const char *mnemonic, AddressingMode mode);

/*
 * Get all valid addressing modes for a mnemonic.
 * Returns bitmask of valid modes, or 0 if mnemonic is unknown.
 */
uint16_t opcode_get_valid_modes(const char *mnemonic);

/*
 * Get instruction flags for a mnemonic.
 * Returns INST_NONE if mnemonic is unknown.
 */
uint8_t opcode_get_flags(const char *mnemonic);

/*
 * Check if a mnemonic is a valid 6502 instruction.
 * Returns 1 if valid, 0 if not.
 */
int opcode_is_valid_mnemonic(const char *mnemonic);

/*
 * Check if a mnemonic is an illegal/undocumented opcode.
 * Returns 1 if illegal, 0 if official.
 */
int opcode_is_illegal(const char *mnemonic);

/*
 * Get instruction size for an addressing mode.
 * Returns 1, 2, or 3.
 */
int opcode_mode_size(AddressingMode mode);

/*
 * Get addressing mode name as string (for error messages).
 */
const char *opcode_mode_name(AddressingMode mode);

/*
 * Initialize opcode tables. Call once at startup.
 */
void opcodes_init(void);

/*
 * Find an opcode entry by its opcode byte.
 * Returns pointer to entry or NULL if not found/invalid.
 */
const OpcodeEntry *opcode_find_by_opcode(uint8_t opcode);

#endif /* OPCODES_H */
