/*
 * parser.h - Statement Parser for ASM64
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include "lexer.h"
#include "opcodes.h"
#include "expr.h"
#include "symbols.h"

/* Statement types */
typedef enum {
    STMT_EMPTY,         /* Empty line or comment only */
    STMT_LABEL,         /* Label definition only */
    STMT_INSTRUCTION,   /* CPU instruction */
    STMT_DIRECTIVE,     /* Assembler directive (!byte, etc.) */
    STMT_ASSIGNMENT,    /* Symbol assignment (NAME = value) */
    STMT_MACRO_CALL,    /* Macro invocation (+macroname) */
    STMT_ERROR          /* Parse error */
} StatementType;

/* Parsed label info */
typedef struct {
    char *name;         /* Label name (allocated) */
    int is_local;       /* 1 if local label (.name) */
    int is_anon_fwd;    /* 1 if anonymous forward (+) */
    int is_anon_back;   /* 1 if anonymous backward (-) */
} LabelInfo;

/* Parsed instruction */
typedef struct {
    char *mnemonic;         /* Instruction mnemonic (allocated) */
    AddressingMode mode;    /* Determined addressing mode */
    Expr *operand;          /* Operand expression (owned) */
    int forced_zp;          /* 1 if forced zero-page with <operand */
    int forced_abs;         /* 1 if forced absolute with !operand or >operand */
    uint8_t opcode;         /* Resolved opcode byte */
    int size;               /* Instruction size in bytes */
    int cycles;             /* Base cycle count */
    int page_penalty;       /* 1 if +1 cycle on page cross */
} InstructionInfo;

/* Parsed directive */
typedef struct {
    char *name;             /* Directive name without ! (allocated) */
    Expr **args;            /* Array of argument expressions (owned) */
    int arg_count;          /* Number of arguments */
    char *string_arg;       /* String argument if any (allocated) */
} DirectiveInfo;

/* Parsed assignment */
typedef struct {
    char *name;             /* Symbol name (allocated) */
    Expr *value;            /* Value expression (owned) */
} AssignmentInfo;

/* Parsed macro call */
typedef struct {
    char *name;             /* Macro name (allocated) */
    char **args;            /* Array of argument strings (allocated) */
    int arg_count;          /* Number of arguments */
} MacroCallInfo;

/* Complete parsed statement */
typedef struct {
    StatementType type;
    int line;               /* Source line number */
    int column;             /* Source column */
    const char *file;       /* Source filename */

    LabelInfo *label;       /* Label on this line (or NULL) */

    union {
        InstructionInfo instruction;
        DirectiveInfo directive;
        AssignmentInfo assignment;
        MacroCallInfo macro_call;
    } data;

    char *error_msg;        /* Error message if type == STMT_ERROR */
} Statement;

/* Parser context */
typedef struct {
    Lexer *lexer;           /* Lexer for tokenization */
    Token current;          /* Current token */
    Token previous;         /* Previous token */
    SymbolTable *symbols;   /* Symbol table for lookups */
    uint16_t pc;            /* Current program counter */
    int pass;               /* Assembly pass (1 or 2) */
    const char *error;      /* Last error message */
} Parser;

/* ========== Parser Functions ========== */

/*
 * Initialize parser with lexer and symbol table.
 */
void parser_init(Parser *parser, Lexer *lexer, SymbolTable *symbols);

/*
 * Set current program counter (for * references).
 */
void parser_set_pc(Parser *parser, uint16_t pc);

/*
 * Set assembly pass (1 or 2).
 */
void parser_set_pass(Parser *parser, int pass);

/*
 * Parse a single line into a statement.
 * Returns statement that caller must free with statement_free().
 */
Statement *parser_parse_line(Parser *parser);

/*
 * Free a statement and all its resources.
 */
void statement_free(Statement *stmt);

/*
 * Get last parser error.
 */
const char *parser_error(Parser *parser);

/* ========== Addressing Mode Detection ========== */

/*
 * Determine addressing mode from parsed operand.
 * Uses expression result to choose zero-page vs absolute.
 *
 * mnemonic: instruction mnemonic (for mode validation)
 * expr: operand expression (can be NULL for implied)
 * has_hash: 1 if # prefix was present (immediate)
 * has_x_index: 1 if ,X suffix was present
 * has_y_index: 1 if ,Y suffix was present
 * is_indirect: 1 if parentheses were present
 * value: evaluated expression value (if known)
 * value_known: 1 if value is defined
 *
 * Returns addressing mode or ADDR_INVALID on error.
 */
AddressingMode detect_addressing_mode(
    const char *mnemonic,
    Expr *expr,
    int has_hash,
    int has_x_index,
    int has_y_index,
    int is_indirect,
    int32_t value,
    int value_known
);

/*
 * Validate that an addressing mode is valid for a mnemonic.
 * Returns 1 if valid, 0 if not.
 */
int validate_addressing_mode(const char *mnemonic, AddressingMode mode);

/*
 * Get instruction size for a statement.
 * Returns size in bytes (1, 2, or 3).
 */
int get_instruction_size(Statement *stmt);

/* ========== Helper Functions ========== */

/*
 * Check if a mnemonic is a branch instruction.
 */
int is_branch_instruction(const char *mnemonic);

/*
 * Check if a mnemonic requires accumulator mode explicitly.
 * (ASL, LSR, ROL, ROR can take A or memory operand)
 */
int is_accumulator_optional(const char *mnemonic);

/*
 * Print statement for debugging.
 */
void statement_print(Statement *stmt);

#endif /* PARSER_H */
