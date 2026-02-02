/*
 * expr.h - Expression Parser and Evaluator
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#ifndef EXPR_H
#define EXPR_H

#include <stdint.h>
#include "lexer.h"
#include "symbols.h"

/* Expression node types */
typedef enum {
    EXPR_NUMBER,    /* Literal number */
    EXPR_SYMBOL,    /* Symbol reference */
    EXPR_UNARY,     /* Unary operator: -, ~, !, <, > */
    EXPR_BINARY,    /* Binary operator: +, -, *, /, etc. */
    EXPR_CURRENT    /* Current program counter (*) */
} ExprType;

/* Unary operators */
typedef enum {
    UNARY_NEG,      /* - (negate) */
    UNARY_NOT,      /* ! (logical not) */
    UNARY_COMP,     /* ~ (bitwise complement) */
    UNARY_LOW,      /* < (low byte) */
    UNARY_HIGH      /* > (high byte) */
} UnaryOp;

/* Binary operators */
typedef enum {
    BINARY_ADD,     /* + */
    BINARY_SUB,     /* - */
    BINARY_MUL,     /* * */
    BINARY_DIV,     /* / */
    BINARY_MOD,     /* % (modulo) */
    BINARY_AND,     /* & (bitwise and) */
    BINARY_OR,      /* | (bitwise or) */
    BINARY_XOR,     /* ^ (bitwise xor) */
    BINARY_SHL,     /* << (shift left) */
    BINARY_SHR,     /* >> (shift right) */
    BINARY_EQ,      /* = (equal) */
    BINARY_NE,      /* <> (not equal) */
    BINARY_LT,      /* < (less than) */
    BINARY_GT,      /* > (greater than) */
    BINARY_LE,      /* <= (less or equal) */
    BINARY_GE       /* >= (greater or equal) */
} BinaryOp;

/* Expression AST node */
typedef struct Expr {
    ExprType type;
    union {
        int32_t number;         /* For EXPR_NUMBER */
        char *symbol;           /* For EXPR_SYMBOL (allocated) */
        struct {                /* For EXPR_UNARY */
            UnaryOp op;
            struct Expr *operand;
        } unary;
        struct {                /* For EXPR_BINARY */
            BinaryOp op;
            struct Expr *left;
            struct Expr *right;
        } binary;
    } data;
} Expr;

/* Expression evaluation result */
typedef struct {
    int32_t value;          /* Evaluated value */
    int defined;            /* 1 if all symbols were defined */
    int is_zeropage;        /* 1 if value is known to fit in zero page */
} ExprResult;

/* Expression parser context */
typedef struct {
    Lexer *lexer;           /* Lexer to read tokens from */
    Token current;          /* Current token */
    Token peek;             /* Lookahead token */
    int has_peek;           /* 1 if peek is valid */
    const char *error;      /* Error message (if any) */
} ExprParser;

/* ========== Expression Creation ========== */

/*
 * Create a number expression.
 */
Expr *expr_number(int32_t value);

/*
 * Create a symbol reference expression.
 * Makes a copy of the symbol name.
 */
Expr *expr_symbol(const char *name);

/*
 * Create a unary expression.
 * Takes ownership of operand.
 */
Expr *expr_unary(UnaryOp op, Expr *operand);

/*
 * Create a binary expression.
 * Takes ownership of left and right.
 */
Expr *expr_binary(BinaryOp op, Expr *left, Expr *right);

/*
 * Create a current PC (*) expression.
 */
Expr *expr_current(void);

/*
 * Free an expression tree.
 */
void expr_free(Expr *expr);

/*
 * Clone (deep copy) an expression tree.
 */
Expr *expr_clone(Expr *expr);

/* ========== Expression Parsing ========== */

/*
 * Initialize expression parser with a lexer.
 * Advances to get the first token.
 */
void expr_parser_init(ExprParser *parser, Lexer *lexer);

/*
 * Initialize expression parser with a pre-loaded token.
 * Use this when integrating with another parser that already
 * has a current token loaded.
 */
void expr_parser_init_with_token(ExprParser *parser, Lexer *lexer, Token current);

/*
 * Parse an expression from the token stream.
 * Returns NULL on error (check parser->error).
 * Caller must free the returned expression with expr_free().
 */
Expr *expr_parse(ExprParser *parser);

/*
 * Parse a primary expression (number, symbol, parenthesized expr).
 */
Expr *expr_parse_primary(ExprParser *parser);

/*
 * Get last parser error message.
 */
const char *expr_parser_error(ExprParser *parser);

/* ========== Expression Evaluation ========== */

/*
 * Evaluate an expression.
 * symbols: symbol table for lookups (can be NULL for constant expressions)
 * anon: anonymous label tracker
 * pc: current program counter value (for * references)
 * pass: assembly pass (1 or 2) - affects undefined symbol handling
 * current_zone: name of current zone for local label mangling (can be NULL)
 *
 * Returns result with value and defined flag.
 * On pass 1, undefined symbols result in defined=0 but no error.
 * On pass 2, undefined symbols are an error.
 */
ExprResult expr_eval(Expr *expr, SymbolTable *symbols, AnonLabels *anon, uint16_t pc, int pass, const char *current_zone);

/*
 * Evaluate an expression and return just the value.
 * Returns 0 if any symbols are undefined.
 * Use expr_eval() if you need to know about undefined symbols.
 */
int32_t expr_eval_value(Expr *expr, SymbolTable *symbols, uint16_t pc);

/*
 * Check if an expression contains any symbol references.
 * Returns 1 if it does, 0 if it's a pure constant expression.
 */
int expr_has_symbols(Expr *expr);

/*
 * Check if an expression is a simple number (no operators or symbols).
 */
int expr_is_simple_number(Expr *expr);

/* ========== Debugging ========== */

/*
 * Print expression tree to stdout (for debugging).
 */
void expr_print(Expr *expr);

#endif /* EXPR_H */
