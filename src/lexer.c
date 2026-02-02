#include "lexer.h"
#include "error.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Character classification helpers */
static int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int is_hex_digit(char c) {
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int is_binary_digit(char c) {
    return c == '0' || c == '1';
}

static int is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

static int is_at_end(Lexer *lex) {
    return *lex->current == '\0';
}

static char peek(Lexer *lex) {
    return *lex->current;
}

static char advance(Lexer *lex) {
    return *lex->current++;
}

static int match(Lexer *lex, char expected) {
    if (is_at_end(lex)) return 0;
    if (*lex->current != expected) return 0;
    lex->current++;
    return 1;
}

static Token make_token(Lexer *lex, TokenType type, const char *start) {
    Token tok;
    tok.type = type;
    tok.start = start;
    tok.length = (int)(lex->current - start);
    tok.line = lex->line;
    tok.column = (int)(start - lex->line_start) + 1;
    tok.value.number = 0;
    return tok;
}

static Token error_token(Lexer *lex, const char *message) {
    Token tok;
    tok.type = TOK_ERROR;
    tok.start = message;
    tok.length = (int)strlen(message);
    tok.line = lex->line;
    tok.column = (int)(lex->current - lex->line_start) + 1;
    tok.value.number = 0;
    return tok;
}

static void skip_whitespace(Lexer *lex) {
    for (;;) {
        char c = peek(lex);
        switch (c) {
            case ' ':
            case '\t':
            case '\r':
                advance(lex);
                break;
            case ';':
                /* Comment - skip to end of line */
                while (!is_at_end(lex) && peek(lex) != '\n') {
                    advance(lex);
                }
                break;
            default:
                return;
        }
    }
}

/* Parse hexadecimal number after $ prefix */
static Token parse_hex(Lexer *lex, const char *start) {
    int32_t value = 0;
    int digits = 0;

    while (is_hex_digit(peek(lex))) {
        char c = advance(lex);
        int digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else digit = c - 'A' + 10;

        value = (value << 4) | digit;
        digits++;

        if (digits > 8) {
            return error_token(lex, "hex number too large");
        }
    }

    if (digits == 0) {
        return error_token(lex, "expected hex digits after $");
    }

    Token tok = make_token(lex, TOK_NUMBER, start);
    tok.value.number = value;
    return tok;
}

/* Parse binary number after % prefix */
static Token parse_binary(Lexer *lex, const char *start) {
    int32_t value = 0;
    int digits = 0;

    while (is_binary_digit(peek(lex))) {
        char c = advance(lex);
        value = (value << 1) | (c - '0');
        digits++;

        if (digits > 32) {
            return error_token(lex, "binary number too large");
        }
    }

    if (digits == 0) {
        return error_token(lex, "expected binary digits after %");
    }

    Token tok = make_token(lex, TOK_NUMBER, start);
    tok.value.number = value;
    return tok;
}

/* Parse decimal number */
static Token parse_decimal(Lexer *lex, const char *start) {
    int32_t value = 0;

    /* We already consumed the first digit, back up */
    lex->current = start;

    while (is_digit(peek(lex))) {
        char c = advance(lex);
        int32_t new_value = value * 10 + (c - '0');

        /* Check for overflow */
        if (new_value < value) {
            return error_token(lex, "decimal number too large");
        }
        value = new_value;
    }

    Token tok = make_token(lex, TOK_NUMBER, start);
    tok.value.number = value;
    return tok;
}

/* Parse character literal 'x' */
static Token parse_char(Lexer *lex, const char *start) {
    if (is_at_end(lex)) {
        return error_token(lex, "unterminated character literal");
    }

    char c = advance(lex);
    int32_t value;

    if (c == '\\') {
        /* Escape sequence */
        if (is_at_end(lex)) {
            return error_token(lex, "unterminated escape sequence");
        }
        char esc = advance(lex);
        switch (esc) {
            case 'n': value = 0x0d; break;  /* PETSCII newline */
            case 'r': value = 0x0d; break;
            case 't': value = 0x09; break;
            case '\\': value = '\\'; break;
            case '\'': value = '\''; break;
            case '"': value = '"'; break;
            case '0': value = 0; break;
            default:
                return error_token(lex, "unknown escape sequence");
        }
    } else {
        value = (unsigned char)c;
    }

    if (!match(lex, '\'')) {
        return error_token(lex, "unterminated character literal");
    }

    Token tok = make_token(lex, TOK_CHAR, start);
    tok.value.number = value;
    return tok;
}

/* Parse string literal "..." */
static Token parse_string(Lexer *lex, const char *start) {
    /* Estimate size and allocate buffer */
    size_t capacity = 64;
    char *buf = mem_alloc(capacity);
    size_t len = 0;

    while (!is_at_end(lex) && peek(lex) != '"' && peek(lex) != '\n') {
        char c = advance(lex);

        if (c == '\\' && !is_at_end(lex)) {
            /* Escape sequence */
            char esc = advance(lex);
            switch (esc) {
                case 'n': c = 0x0d; break;  /* PETSCII newline */
                case 'r': c = 0x0d; break;
                case 't': c = 0x09; break;
                case '\\': c = '\\'; break;
                case '"': c = '"'; break;
                case '\'': c = '\''; break;
                case '0': c = 0; break;
                default:
                    free(buf);
                    return error_token(lex, "unknown escape sequence");
            }
        }

        /* Grow buffer if needed */
        if (len + 1 >= capacity) {
            capacity *= 2;
            buf = mem_realloc(buf, capacity);
        }
        buf[len++] = c;
    }

    if (is_at_end(lex) || peek(lex) == '\n') {
        free(buf);
        return error_token(lex, "unterminated string");
    }

    advance(lex);  /* Consume closing " */
    buf[len] = '\0';

    Token tok = make_token(lex, TOK_STRING, start);
    tok.value.string.data = buf;
    tok.value.string.len = (int)len;
    return tok;
}

/* Parse identifier or local label */
static Token parse_identifier(Lexer *lex, const char *start) {
    while (is_alnum(peek(lex))) {
        advance(lex);
    }

    return make_token(lex, TOK_IDENTIFIER, start);
}

/* Parse local label starting with . */
static Token parse_local_label(Lexer *lex, const char *start) {
    while (is_alnum(peek(lex))) {
        advance(lex);
    }

    return make_token(lex, TOK_LOCAL_LABEL, start);
}

/* Parse directive starting with ! */
static Token parse_directive(Lexer *lex, const char *start) {
    /* First char after ! determines what we have */
    char first = peek(lex);

    /* If first char is a letter, definitely a directive */
    if (is_alpha(first)) {
        advance(lex);
        while (is_alnum(peek(lex))) {
            advance(lex);
        }
        return make_token(lex, TOK_DIRECTIVE, start);
    }

    /* If first char is a digit, need to check for known directives like !08, !16 */
    if (is_digit(first)) {
        /* Peek ahead to see the full number */
        const char *peek_pos = lex->current;
        advance(lex);
        while (is_digit(peek(lex))) {
            advance(lex);
        }

        /* Check if it's a known numeric directive (!08, !16, !24, !32) */
        int len = (int)(lex->current - peek_pos);
        if (len == 2 &&
            ((peek_pos[0] == '0' && peek_pos[1] == '8') ||
             (peek_pos[0] == '1' && peek_pos[1] == '6') ||
             (peek_pos[0] == '2' && peek_pos[1] == '4') ||
             (peek_pos[0] == '3' && peek_pos[1] == '2'))) {
            return make_token(lex, TOK_DIRECTIVE, start);
        }

        /* Not a known directive - backtrack and return just the ! */
        lex->current = peek_pos;
        return make_token(lex, TOK_BANG, start);
    }

    /* Just a lone ! - treat as operator */
    return make_token(lex, TOK_BANG, start);
}

/* Parse macro call starting with + */
static Token parse_macro_or_anon(Lexer *lex, const char *start) {
    /* Count leading + characters */
    int plus_count = 1;  /* Already consumed one */
    while (peek(lex) == '+') {
        advance(lex);
        plus_count++;
    }

    /* If followed by identifier, check context to determine if macro call or operator.
     * Macro calls (+macroname) appear at the start of a line (possibly after whitespace/label).
     * In expressions like A+B, the +B should be TOK_PLUS + identifier. */
    if (plus_count == 1 && is_alpha(peek(lex))) {
        /* Check if this + appears at the beginning of a meaningful token sequence
         * by looking at what comes before it on the current line. */
        const char *p = start;
        int only_whitespace_before = 1;
        while (p > lex->line_start) {
            p--;
            if (*p == ':') {
                /* After a label colon - could be macro call */
                break;
            }
            if (!(*p == ' ' || *p == '\t')) {
                /* Non-whitespace before + means we're in an expression */
                only_whitespace_before = 0;
                break;
            }
        }

        if (only_whitespace_before) {
            /* At line start (after optional whitespace/label) - this is a macro call */
            while (is_alnum(peek(lex))) {
                advance(lex);
            }
            return make_token(lex, TOK_MACRO_CALL, start);
        } else {
            /* In an expression context - this is the + operator */
            return make_token(lex, TOK_PLUS, start);
        }
    }

    /* If single + followed by digit/expression start, treat as operator */
    if (plus_count == 1 && (is_digit(peek(lex)) || peek(lex) == '$' ||
                            peek(lex) == '%' || peek(lex) == '(' ||
                            peek(lex) == '\'' || peek(lex) == '*' ||
                            peek(lex) == '<' || peek(lex) == '>' ||
                            peek(lex) == '-' || peek(lex) == '~' ||
                            peek(lex) == '!')) {
        return make_token(lex, TOK_PLUS, start);
    }

    /* Otherwise it's anonymous forward label(s) */
    Token tok = make_token(lex, TOK_ANON_FWD, start);
    tok.value.number = plus_count;
    return tok;
}

/* Parse anonymous backward label(s) */
static Token parse_anon_back(Lexer *lex, const char *start) {
    int minus_count = 1;  /* Already consumed one */
    while (peek(lex) == '-') {
        advance(lex);
        minus_count++;
    }

    /* If this could be a negative number or expression, treat single - as operator */
    if (minus_count == 1 && (is_digit(peek(lex)) || peek(lex) == '$' ||
                             peek(lex) == '%' || peek(lex) == '(' ||
                             is_alpha(peek(lex)))) {
        return make_token(lex, TOK_MINUS, start);
    }

    Token tok = make_token(lex, TOK_ANON_BACK, start);
    tok.value.number = minus_count;
    return tok;
}

void lexer_init(Lexer *lex, const char *source, const char *filename) {
    lex->source = source;
    lex->current = source;
    lex->line_start = source;
    lex->line = 1;
    lex->filename = filename;
}

Token lexer_next(Lexer *lex) {
    skip_whitespace(lex);

    if (is_at_end(lex)) {
        return make_token(lex, TOK_EOF, lex->current);
    }

    const char *start = lex->current;
    char c = advance(lex);

    /* Newline */
    if (c == '\n') {
        Token tok = make_token(lex, TOK_EOL, start);
        lex->line++;
        lex->line_start = lex->current;
        return tok;
    }

    /* Numbers */
    if (c == '$') {
        return parse_hex(lex, start);
    }
    if (c == '%' && is_binary_digit(peek(lex))) {
        return parse_binary(lex, start);
    }
    if (is_digit(c)) {
        return parse_decimal(lex, start);
    }

    /* Character literal */
    if (c == '\'') {
        return parse_char(lex, start);
    }

    /* String literal */
    if (c == '"') {
        return parse_string(lex, start);
    }

    /* Identifiers and labels */
    if (is_alpha(c)) {
        return parse_identifier(lex, start);
    }

    /* Local label */
    if (c == '.' && is_alpha(peek(lex))) {
        return parse_local_label(lex, start);
    }

    /* Directive (starts with ! followed by alphanum for !08, !16, etc.) */
    if (c == '!' && is_alnum(peek(lex))) {
        return parse_directive(lex, start);
    }

    /* Macro call or anonymous forward label */
    if (c == '+') {
        return parse_macro_or_anon(lex, start);
    }

    /* Anonymous backward label or minus */
    if (c == '-') {
        return parse_anon_back(lex, start);
    }

    /* Two-character operators */
    if (c == '<') {
        if (match(lex, '<')) return make_token(lex, TOK_LSHIFT, start);
        if (match(lex, '=')) return make_token(lex, TOK_LE, start);
        if (match(lex, '>')) return make_token(lex, TOK_NE, start);
        return make_token(lex, TOK_LT, start);
    }
    if (c == '>') {
        if (match(lex, '>')) return make_token(lex, TOK_RSHIFT, start);
        if (match(lex, '=')) return make_token(lex, TOK_GE, start);
        return make_token(lex, TOK_GT, start);
    }

    /* Single-character tokens */
    switch (c) {
        case '*': return make_token(lex, TOK_STAR, start);
        case '/': return make_token(lex, TOK_SLASH, start);
        case '%': return make_token(lex, TOK_PERCENT, start);
        case '&': return make_token(lex, TOK_AMP, start);
        case '|': return make_token(lex, TOK_PIPE, start);
        case '^': return make_token(lex, TOK_CARET, start);
        case '~': return make_token(lex, TOK_TILDE, start);
        case '!': return make_token(lex, TOK_BANG, start);
        case '=': return make_token(lex, TOK_EQ, start);
        case '(': return make_token(lex, TOK_LPAREN, start);
        case ')': return make_token(lex, TOK_RPAREN, start);
        case '{': return make_token(lex, TOK_LBRACE, start);
        case '}': return make_token(lex, TOK_RBRACE, start);
        case '[': return make_token(lex, TOK_LBRACKET, start);
        case ']': return make_token(lex, TOK_RBRACKET, start);
        case ',': return make_token(lex, TOK_COMMA, start);
        case ':': return make_token(lex, TOK_COLON, start);
        case '#': return make_token(lex, TOK_HASH, start);
        case '.': return make_token(lex, TOK_ERROR, start);  /* Lone dot */
    }

    return error_token(lex, "unexpected character");
}

Token lexer_peek(Lexer *lex) {
    /* Save state */
    const char *saved_current = lex->current;
    const char *saved_line_start = lex->line_start;
    int saved_line = lex->line;

    Token tok = lexer_next(lex);

    /* Restore state */
    lex->current = saved_current;
    lex->line_start = saved_line_start;
    lex->line = saved_line;

    return tok;
}

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOK_EOF: return "EOF";
        case TOK_EOL: return "EOL";
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING: return "STRING";
        case TOK_CHAR: return "CHAR";
        case TOK_IDENTIFIER: return "IDENTIFIER";
        case TOK_LOCAL_LABEL: return "LOCAL_LABEL";
        case TOK_ANON_BACK: return "ANON_BACK";
        case TOK_ANON_FWD: return "ANON_FWD";
        case TOK_DIRECTIVE: return "DIRECTIVE";
        case TOK_MACRO_CALL: return "MACRO_CALL";
        case TOK_PLUS: return "PLUS";
        case TOK_MINUS: return "MINUS";
        case TOK_STAR: return "STAR";
        case TOK_SLASH: return "SLASH";
        case TOK_PERCENT: return "PERCENT";
        case TOK_AMP: return "AMP";
        case TOK_PIPE: return "PIPE";
        case TOK_CARET: return "CARET";
        case TOK_TILDE: return "TILDE";
        case TOK_BANG: return "BANG";
        case TOK_LT: return "LT";
        case TOK_GT: return "GT";
        case TOK_EQ: return "EQ";
        case TOK_NE: return "NE";
        case TOK_LE: return "LE";
        case TOK_GE: return "GE";
        case TOK_LSHIFT: return "LSHIFT";
        case TOK_RSHIFT: return "RSHIFT";
        case TOK_LPAREN: return "LPAREN";
        case TOK_RPAREN: return "RPAREN";
        case TOK_LBRACE: return "LBRACE";
        case TOK_RBRACE: return "RBRACE";
        case TOK_LBRACKET: return "LBRACKET";
        case TOK_RBRACKET: return "RBRACKET";
        case TOK_COMMA: return "COMMA";
        case TOK_COLON: return "COLON";
        case TOK_HASH: return "HASH";
        case TOK_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

char *token_text(const Token *tok) {
    return str_ndup(tok->start, (size_t)tok->length);
}

int token_equals(const Token *tok, const char *text) {
    size_t len = strlen(text);
    if ((size_t)tok->length != len) return 0;

    for (size_t i = 0; i < len; i++) {
        char a = tok->start[i];
        char b = text[i];
        /* Case-insensitive comparison */
        if (tolower((unsigned char)a) != tolower((unsigned char)b)) {
            return 0;
        }
    }
    return 1;
}

/* 6502 instruction mnemonics */
static const char *mnemonics[] = {
    "adc", "and", "asl", "bcc", "bcs", "beq", "bit", "bmi",
    "bne", "bpl", "brk", "bvc", "bvs", "clc", "cld", "cli",
    "clv", "cmp", "cpx", "cpy", "dec", "dex", "dey", "eor",
    "inc", "inx", "iny", "jmp", "jsr", "lda", "ldx", "ldy",
    "lsr", "nop", "ora", "pha", "php", "pla", "plp", "rol",
    "ror", "rti", "rts", "sbc", "sec", "sed", "sei", "sta",
    "stx", "sty", "tax", "tay", "tsx", "txa", "txs", "tya",
    /* Illegal opcodes */
    "lax", "sax", "dcp", "isc", "slo", "rla", "sre", "rra",
    "anc", "alr", "arr", "xaa", "ahx", "tas", "shx", "shy",
    "las", "jam", "kil",
    NULL
};

int token_is_mnemonic(const Token *tok) {
    if (tok->type != TOK_IDENTIFIER) return 0;

    for (const char **m = mnemonics; *m; m++) {
        if (token_equals(tok, *m)) return 1;
    }
    return 0;
}

void token_free(Token *tok) {
    if (tok->type == TOK_STRING && tok->value.string.data) {
        free(tok->value.string.data);
        tok->value.string.data = NULL;
    }
}
