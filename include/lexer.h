#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stddef.h>

/* Token types */
typedef enum {
    TOK_EOF = 0,        /* End of file */
    TOK_EOL,            /* End of line */

    /* Literals */
    TOK_NUMBER,         /* Numeric literal (value in token) */
    TOK_STRING,         /* String literal "..." */
    TOK_CHAR,           /* Character literal 'x' */

    /* Identifiers */
    TOK_IDENTIFIER,     /* Label or symbol name */
    TOK_LOCAL_LABEL,    /* .local label */
    TOK_ANON_BACK,      /* - anonymous label (backward ref) */
    TOK_ANON_FWD,       /* + used as anonymous label (forward ref) */

    /* Directives and instructions */
    TOK_DIRECTIVE,      /* !directive */
    TOK_MACRO_CALL,     /* +macroname */

    /* Operators */
    TOK_PLUS,           /* + */
    TOK_MINUS,          /* - */
    TOK_STAR,           /* * */
    TOK_SLASH,          /* / */
    TOK_PERCENT,        /* % (modulo, not binary prefix) */
    TOK_AMP,            /* & */
    TOK_PIPE,           /* | */
    TOK_CARET,          /* ^ */
    TOK_TILDE,          /* ~ */
    TOK_BANG,           /* ! (when not directive) */
    TOK_LT,             /* < */
    TOK_GT,             /* > */
    TOK_EQ,             /* = */
    TOK_NE,             /* <> */
    TOK_LE,             /* <= */
    TOK_GE,             /* >= */
    TOK_LSHIFT,         /* << */
    TOK_RSHIFT,         /* >> */

    /* Delimiters */
    TOK_LPAREN,         /* ( */
    TOK_RPAREN,         /* ) */
    TOK_LBRACE,         /* { */
    TOK_RBRACE,         /* } */
    TOK_LBRACKET,       /* [ */
    TOK_RBRACKET,       /* ] */
    TOK_COMMA,          /* , */
    TOK_COLON,          /* : */
    TOK_HASH,           /* # */

    /* Special */
    TOK_ERROR           /* Lexer error */
} TokenType;

/* Token structure */
typedef struct {
    TokenType type;
    const char *start;      /* Pointer to start of token in source */
    int length;             /* Length of token text */
    int line;               /* Line number (1-based) */
    int column;             /* Column number (1-based) */

    /* Value for literals */
    union {
        int32_t number;     /* For TOK_NUMBER */
        struct {
            char *data;     /* For TOK_STRING (allocated, caller frees) */
            int len;
        } string;
    } value;
} Token;

/* Lexer state */
typedef struct {
    const char *source;     /* Source code */
    const char *current;    /* Current position */
    const char *line_start; /* Start of current line */
    int line;               /* Current line number */
    const char *filename;   /* Source filename for errors */
} Lexer;

/* Initialize lexer with source code */
void lexer_init(Lexer *lex, const char *source, const char *filename);

/* Get next token */
Token lexer_next(Lexer *lex);

/* Peek at next token without consuming */
Token lexer_peek(Lexer *lex);

/* Get token type name for debugging */
const char *token_type_name(TokenType type);

/* Extract token text as new string (caller must free) */
char *token_text(const Token *tok);

/* Check if token matches specific text (case-insensitive for mnemonics) */
int token_equals(const Token *tok, const char *text);

/* Check if token is an instruction mnemonic */
int token_is_mnemonic(const Token *tok);

/* Free any allocated data in token (for TOK_STRING) */
void token_free(Token *tok);

#endif /* LEXER_H */
