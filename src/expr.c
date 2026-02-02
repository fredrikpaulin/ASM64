/*
 * expr.c - Expression Parser and Evaluator Implementation
 * ASM64 - 6502/6510 Assembler for Commodore 64
 *
 * Recursive descent parser with operator precedence:
 *   Lowest:  | (OR)
 *            ^ (XOR)
 *            & (AND)
 *            =, <>, <, >, <=, >= (comparison)
 *            <<, >> (shift)
 *            +, - (additive)
 *            *, /, % (multiplicative)
 *   Highest: unary -, ~, !, <, >
 */

#include "expr.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ========== Expression Creation ========== */

Expr *expr_number(int32_t value) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) return NULL;
    e->type = EXPR_NUMBER;
    e->data.number = value;
    return e;
}

Expr *expr_symbol(const char *name) {
    if (!name) return NULL;
    Expr *e = malloc(sizeof(Expr));
    if (!e) return NULL;
    e->type = EXPR_SYMBOL;
    e->data.symbol = malloc(strlen(name) + 1);
    if (!e->data.symbol) {
        free(e);
        return NULL;
    }
    strcpy(e->data.symbol, name);
    return e;
}

Expr *expr_unary(UnaryOp op, Expr *operand) {
    if (!operand) return NULL;
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        expr_free(operand);
        return NULL;
    }
    e->type = EXPR_UNARY;
    e->data.unary.op = op;
    e->data.unary.operand = operand;
    return e;
}

Expr *expr_binary(BinaryOp op, Expr *left, Expr *right) {
    if (!left || !right) {
        expr_free(left);
        expr_free(right);
        return NULL;
    }
    Expr *e = malloc(sizeof(Expr));
    if (!e) {
        expr_free(left);
        expr_free(right);
        return NULL;
    }
    e->type = EXPR_BINARY;
    e->data.binary.op = op;
    e->data.binary.left = left;
    e->data.binary.right = right;
    return e;
}

Expr *expr_current(void) {
    Expr *e = malloc(sizeof(Expr));
    if (!e) return NULL;
    e->type = EXPR_CURRENT;
    return e;
}

void expr_free(Expr *expr) {
    if (!expr) return;
    switch (expr->type) {
        case EXPR_SYMBOL:
            free(expr->data.symbol);
            break;
        case EXPR_UNARY:
            expr_free(expr->data.unary.operand);
            break;
        case EXPR_BINARY:
            expr_free(expr->data.binary.left);
            expr_free(expr->data.binary.right);
            break;
        default:
            break;
    }
    free(expr);
}

Expr *expr_clone(Expr *expr) {
    if (!expr) return NULL;

    switch (expr->type) {
        case EXPR_NUMBER:
            return expr_number(expr->data.number);

        case EXPR_SYMBOL:
            return expr_symbol(expr->data.symbol);

        case EXPR_CURRENT:
            return expr_current();

        case EXPR_UNARY:
            return expr_unary(expr->data.unary.op,
                              expr_clone(expr->data.unary.operand));

        case EXPR_BINARY:
            return expr_binary(expr->data.binary.op,
                               expr_clone(expr->data.binary.left),
                               expr_clone(expr->data.binary.right));
    }
    return NULL;
}

/* ========== Parser Helper Functions ========== */

static void parser_advance(ExprParser *parser) {
    if (parser->has_peek) {
        parser->current = parser->peek;
        parser->has_peek = 0;
    } else {
        parser->current = lexer_next(parser->lexer);
    }
}

static int parser_check(ExprParser *parser, TokenType type) {
    return parser->current.type == type;
}

/* Peek at the next token without consuming current */
static TokenType parser_peek_type(ExprParser *parser) {
    if (!parser->has_peek) {
        parser->peek = lexer_next(parser->lexer);
        parser->has_peek = 1;
    }
    return parser->peek.type;
}

/* Check if a token type could be the start of an expression primary */
static int is_primary_start(TokenType type) {
    return type == TOK_NUMBER ||
           type == TOK_IDENTIFIER ||
           type == TOK_LOCAL_LABEL ||
           type == TOK_CHAR ||
           type == TOK_STAR ||
           type == TOK_LPAREN;
}

/* Extract identifier string from token */
static char *token_to_string(Token *tok) {
    char *str = malloc(tok->length + 1);
    if (!str) return NULL;
    memcpy(str, tok->start, tok->length);
    str[tok->length] = '\0';
    return str;
}

/* ========== Expression Parser ========== */

void expr_parser_init(ExprParser *parser, Lexer *lexer) {
    parser->lexer = lexer;
    parser->has_peek = 0;
    parser->error = NULL;
    parser_advance(parser);  /* Load first token */
}

void expr_parser_init_with_token(ExprParser *parser, Lexer *lexer, Token current) {
    parser->lexer = lexer;
    parser->current = current;
    parser->has_peek = 0;
    parser->error = NULL;
}

const char *expr_parser_error(ExprParser *parser) {
    return parser->error;
}

/* Forward declarations for recursive descent */
static Expr *parse_or(ExprParser *parser);
static Expr *parse_xor(ExprParser *parser);
static Expr *parse_and(ExprParser *parser);
static Expr *parse_comparison(ExprParser *parser);
static Expr *parse_shift(ExprParser *parser);
static Expr *parse_additive(ExprParser *parser);
static Expr *parse_multiplicative(ExprParser *parser);
static Expr *parse_unary(ExprParser *parser);
static Expr *parse_primary(ExprParser *parser);

/* Main entry point */
Expr *expr_parse(ExprParser *parser) {
    parser->error = NULL;
    return parse_or(parser);
}

/* OR: expr | expr */
static Expr *parse_or(ExprParser *parser) {
    Expr *left = parse_xor(parser);
    if (!left) return NULL;

    while (parser_check(parser, TOK_PIPE)) {
        parser_advance(parser);
        Expr *right = parse_xor(parser);
        if (!right) {
            expr_free(left);
            return NULL;
        }
        left = expr_binary(BINARY_OR, left, right);
        if (!left) return NULL;
    }
    return left;
}

/* XOR: expr ^ expr */
static Expr *parse_xor(ExprParser *parser) {
    Expr *left = parse_and(parser);
    if (!left) return NULL;

    while (parser_check(parser, TOK_CARET)) {
        parser_advance(parser);
        Expr *right = parse_and(parser);
        if (!right) {
            expr_free(left);
            return NULL;
        }
        left = expr_binary(BINARY_XOR, left, right);
        if (!left) return NULL;
    }
    return left;
}

/* AND: expr & expr */
static Expr *parse_and(ExprParser *parser) {
    Expr *left = parse_comparison(parser);
    if (!left) return NULL;

    while (parser_check(parser, TOK_AMP)) {
        parser_advance(parser);
        Expr *right = parse_comparison(parser);
        if (!right) {
            expr_free(left);
            return NULL;
        }
        left = expr_binary(BINARY_AND, left, right);
        if (!left) return NULL;
    }
    return left;
}

/* Comparison: =, <>, <, >, <=, >= */
static Expr *parse_comparison(ExprParser *parser) {
    Expr *left = parse_shift(parser);
    if (!left) return NULL;

    for (;;) {
        BinaryOp op;
        if (parser_check(parser, TOK_EQ)) {
            op = BINARY_EQ;
        } else if (parser_check(parser, TOK_NE)) {
            op = BINARY_NE;
        } else if (parser_check(parser, TOK_LE)) {
            op = BINARY_LE;
        } else if (parser_check(parser, TOK_GE)) {
            op = BINARY_GE;
        } else if (parser_check(parser, TOK_LT)) {
            /* Check if this is < as comparison or < as low-byte unary */
            /* If followed by something that looks like an operand, it's comparison */
            /* For now, treat as comparison at this level */
            op = BINARY_LT;
        } else if (parser_check(parser, TOK_GT)) {
            op = BINARY_GT;
        } else {
            break;
        }
        parser_advance(parser);
        Expr *right = parse_shift(parser);
        if (!right) {
            expr_free(left);
            return NULL;
        }
        left = expr_binary(op, left, right);
        if (!left) return NULL;
    }
    return left;
}

/* Shift: <<, >> */
static Expr *parse_shift(ExprParser *parser) {
    Expr *left = parse_additive(parser);
    if (!left) return NULL;

    for (;;) {
        BinaryOp op;
        if (parser_check(parser, TOK_LSHIFT)) {
            op = BINARY_SHL;
        } else if (parser_check(parser, TOK_RSHIFT)) {
            op = BINARY_SHR;
        } else {
            break;
        }
        parser_advance(parser);
        Expr *right = parse_additive(parser);
        if (!right) {
            expr_free(left);
            return NULL;
        }
        left = expr_binary(op, left, right);
        if (!left) return NULL;
    }
    return left;
}

/* Additive: +, - */
static Expr *parse_additive(ExprParser *parser) {
    Expr *left = parse_multiplicative(parser);
    if (!left) return NULL;

    for (;;) {
        BinaryOp op;
        /* Handle both TOK_PLUS and TOK_ANON_FWD as addition in expression context */
        if (parser_check(parser, TOK_PLUS) || parser_check(parser, TOK_ANON_FWD)) {
            op = BINARY_ADD;
        } else if (parser_check(parser, TOK_MINUS) || parser_check(parser, TOK_ANON_BACK)) {
            op = BINARY_SUB;
        } else {
            break;
        }
        parser_advance(parser);
        Expr *right = parse_multiplicative(parser);
        if (!right) {
            expr_free(left);
            return NULL;
        }
        left = expr_binary(op, left, right);
        if (!left) return NULL;
    }
    return left;
}

/* Multiplicative: *, /, % */
static Expr *parse_multiplicative(ExprParser *parser) {
    Expr *left = parse_unary(parser);
    if (!left) return NULL;

    for (;;) {
        BinaryOp op;
        if (parser_check(parser, TOK_STAR)) {
            op = BINARY_MUL;
        } else if (parser_check(parser, TOK_SLASH)) {
            op = BINARY_DIV;
        } else if (parser_check(parser, TOK_PERCENT)) {
            op = BINARY_MOD;
        } else {
            break;
        }
        parser_advance(parser);
        Expr *right = parse_unary(parser);
        if (!right) {
            expr_free(left);
            return NULL;
        }
        left = expr_binary(op, left, right);
        if (!left) return NULL;
    }
    return left;
}

/* Unary: -, ~, !, <, > */
static Expr *parse_unary(ExprParser *parser) {
    UnaryOp op;
    int is_unary = 0;

    /* TOK_MINUS is always unary negation here */
    if (parser_check(parser, TOK_MINUS)) {
        op = UNARY_NEG;
        is_unary = 1;
    } else if (parser_check(parser, TOK_ANON_BACK)) {
        /* TOK_ANON_BACK is unary minus ONLY if followed by expression start.
         * Otherwise it's an anonymous label reference handled in parse_primary.
         * This distinguishes "bne -" (anon label) from "-5" (negative number). */
        TokenType next = parser_peek_type(parser);
        if (is_primary_start(next)) {
            op = UNARY_NEG;
            is_unary = 1;
        }
        /* If not followed by expression start, fall through to parse_primary */
    } else if (parser_check(parser, TOK_TILDE)) {
        op = UNARY_COMP;
        is_unary = 1;
    } else if (parser_check(parser, TOK_BANG)) {
        op = UNARY_NOT;
        is_unary = 1;
    } else if (parser_check(parser, TOK_LT)) {
        op = UNARY_LOW;
        is_unary = 1;
    } else if (parser_check(parser, TOK_GT)) {
        op = UNARY_HIGH;
        is_unary = 1;
    }

    if (is_unary) {
        parser_advance(parser);
        Expr *operand = parse_unary(parser);  /* Right-associative */
        if (!operand) return NULL;
        return expr_unary(op, operand);
    }

    return parse_primary(parser);
}

/* Primary: number, symbol, *, (expr) */
static Expr *parse_primary(ExprParser *parser) {
    /* Number literal */
    if (parser_check(parser, TOK_NUMBER)) {
        int32_t value = parser->current.value.number;
        parser_advance(parser);
        return expr_number(value);
    }

    /* Character literal (treated as number) */
    if (parser_check(parser, TOK_CHAR)) {
        int32_t value = parser->current.value.number;
        parser_advance(parser);
        return expr_number(value);
    }

    /* Symbol/identifier */
    if (parser_check(parser, TOK_IDENTIFIER)) {
        char *name = token_to_string(&parser->current);
        if (!name) {
            parser->error = "out of memory";
            return NULL;
        }
        parser_advance(parser);
        Expr *e = expr_symbol(name);
        free(name);
        return e;
    }

    /* Local label */
    if (parser_check(parser, TOK_LOCAL_LABEL)) {
        char *name = token_to_string(&parser->current);
        if (!name) {
            parser->error = "out of memory";
            return NULL;
        }
        parser_advance(parser);
        Expr *e = expr_symbol(name);
        free(name);
        return e;
    }

    /* Current PC (*) - but only if not followed by something that makes it multiply */
    if (parser_check(parser, TOK_STAR)) {
        parser_advance(parser);
        return expr_current();
    }

    /* Parenthesized expression */
    if (parser_check(parser, TOK_LPAREN)) {
        parser_advance(parser);
        Expr *inner = parse_or(parser);
        if (!inner) return NULL;
        if (!parser_check(parser, TOK_RPAREN)) {
            parser->error = "expected ')'";
            expr_free(inner);
            return NULL;
        }
        parser_advance(parser);
        return inner;
    }

    /* Anonymous forward reference (+, ++, etc.) */
    if (parser_check(parser, TOK_ANON_FWD)) {
        /* Create a symbol with special name: __anon_fwd_N where N is count */
        int count = parser->current.value.number;
        char name[32];
        snprintf(name, sizeof(name), "__anon_fwd_%d", count);
        parser_advance(parser);
        return expr_symbol(name);
    }

    /* Anonymous backward reference (-, --, etc.) */
    if (parser_check(parser, TOK_ANON_BACK)) {
        int count = parser->current.value.number;
        char name[32];
        snprintf(name, sizeof(name), "__anon_back_%d", count);
        parser_advance(parser);
        return expr_symbol(name);
    }

    parser->error = "expected expression";
    return NULL;
}

Expr *expr_parse_primary(ExprParser *parser) {
    return parse_primary(parser);
}

/* ========== Expression Evaluation ========== */

/* Helper to mangle local label name with zone */
static char *mangle_local_name(const char *name, const char *zone) {
    if (!name || name[0] != '.') return NULL;

    const char *local_part = name + 1;  /* Skip the leading . */

    if (!zone || zone[0] == '\0') {
        /* No zone - use _global prefix */
        size_t len = strlen(local_part) + 10;
        char *mangled = malloc(len);
        if (mangled) {
            snprintf(mangled, len, "_global.%s", local_part);
        }
        return mangled;
    }

    /* Build mangled name: zone.local_part */
    size_t zone_len = strlen(zone);
    size_t local_len = strlen(local_part);
    size_t total_len = zone_len + 1 + local_len + 1;

    char *mangled = malloc(total_len);
    if (mangled) {
        snprintf(mangled, total_len, "%s.%s", zone, local_part);
    }
    return mangled;
}

ExprResult expr_eval(Expr *expr, SymbolTable *symbols, AnonLabels *anon, uint16_t pc, int pass, const char *current_zone) {
    ExprResult result = { 0, 1, 0 };  /* Default: value=0, defined=true */

    if (!expr) {
        result.defined = 0;
        return result;
    }

    switch (expr->type) {
        case EXPR_NUMBER:
            result.value = expr->data.number;
            result.is_zeropage = (result.value >= 0 && result.value <= 0xFF);
            break;

        case EXPR_CURRENT:
            result.value = pc;
            result.is_zeropage = (pc <= 0xFF);
            break;

        case EXPR_SYMBOL: {
            const char *name = expr->data.symbol;
            char *mangled_name = NULL;

            /* Check for anonymous forward label special names */
            if (strncmp(name, "__anon_fwd_", 11) == 0) {
                int count = atoi(name + 11);
                if (pass == 1) {
                    /* In pass 1, forward references are undefined */
                    result.defined = 0;
                    result.value = 0;
                } else {
                    /* In pass 2, resolve from anonymous label tracker */
                    int32_t addr = anon_resolve_forward(anon, count);
                    if (addr < 0) {
                        result.defined = 0;
                        result.value = 0;
                    } else {
                        result.value = addr;
                        result.defined = 1;
                        result.is_zeropage = (addr <= 0xFF);
                    }
                    /* Advance the forward index after resolution */
                    anon_advance_forward(anon);
                }
                break;
            }

            /* Check for anonymous backward label special names */
            if (strncmp(name, "__anon_back_", 12) == 0) {
                int count = atoi(name + 12);
                /* Backward references can be resolved in both passes */
                int32_t addr = anon_resolve_backward(anon, count);
                if (addr < 0) {
                    result.defined = 0;
                    result.value = 0;
                } else {
                    result.value = addr;
                    result.defined = 1;
                    result.is_zeropage = (addr <= 0xFF);
                }
                break;
            }

            if (!symbols) {
                result.defined = 0;
                result.value = 0;
                break;
            }

            /* Handle local labels - mangle with current zone */
            if (name[0] == '.') {
                mangled_name = mangle_local_name(name, current_zone);
                if (mangled_name) {
                    name = mangled_name;
                }
            }

            Symbol *sym = symbol_lookup(symbols, name);
            if (!sym || !(sym->flags & SYM_DEFINED)) {
                result.defined = 0;
                result.value = 0;  /* Use 0 as placeholder */
            } else {
                result.value = sym->value;
                result.defined = 1;
                result.is_zeropage = (sym->flags & SYM_ZEROPAGE) ||
                                     (result.value >= 0 && result.value <= 0xFF);
            }

            free(mangled_name);
            break;
        }

        case EXPR_UNARY: {
            ExprResult operand = expr_eval(expr->data.unary.operand, symbols, anon, pc, pass, current_zone);
            result.defined = operand.defined;

            switch (expr->data.unary.op) {
                case UNARY_NEG:
                    result.value = -operand.value;
                    break;
                case UNARY_NOT:
                    result.value = !operand.value;
                    break;
                case UNARY_COMP:
                    result.value = ~operand.value;
                    break;
                case UNARY_LOW:
                    result.value = operand.value & 0xFF;
                    result.is_zeropage = 1;  /* Low byte always fits in ZP */
                    break;
                case UNARY_HIGH:
                    result.value = (operand.value >> 8) & 0xFF;
                    result.is_zeropage = 1;  /* High byte always fits in ZP */
                    break;
            }
            break;
        }

        case EXPR_BINARY: {
            ExprResult left = expr_eval(expr->data.binary.left, symbols, anon, pc, pass, current_zone);
            ExprResult right = expr_eval(expr->data.binary.right, symbols, anon, pc, pass, current_zone);
            result.defined = left.defined && right.defined;

            switch (expr->data.binary.op) {
                case BINARY_ADD:
                    result.value = left.value + right.value;
                    break;
                case BINARY_SUB:
                    result.value = left.value - right.value;
                    break;
                case BINARY_MUL:
                    result.value = left.value * right.value;
                    break;
                case BINARY_DIV:
                    if (right.value == 0) {
                        result.value = 0;
                        /* Could set an error flag here */
                    } else {
                        result.value = left.value / right.value;
                    }
                    break;
                case BINARY_MOD:
                    if (right.value == 0) {
                        result.value = 0;
                    } else {
                        result.value = left.value % right.value;
                    }
                    break;
                case BINARY_AND:
                    result.value = left.value & right.value;
                    break;
                case BINARY_OR:
                    result.value = left.value | right.value;
                    break;
                case BINARY_XOR:
                    result.value = left.value ^ right.value;
                    break;
                case BINARY_SHL:
                    result.value = left.value << right.value;
                    break;
                case BINARY_SHR:
                    result.value = (uint32_t)left.value >> right.value;
                    break;
                case BINARY_EQ:
                    result.value = (left.value == right.value) ? 1 : 0;
                    break;
                case BINARY_NE:
                    result.value = (left.value != right.value) ? 1 : 0;
                    break;
                case BINARY_LT:
                    result.value = (left.value < right.value) ? 1 : 0;
                    break;
                case BINARY_GT:
                    result.value = (left.value > right.value) ? 1 : 0;
                    break;
                case BINARY_LE:
                    result.value = (left.value <= right.value) ? 1 : 0;
                    break;
                case BINARY_GE:
                    result.value = (left.value >= right.value) ? 1 : 0;
                    break;
            }

            /* Check if result fits in zero page */
            if (result.defined) {
                result.is_zeropage = (result.value >= 0 && result.value <= 0xFF);
            }
            break;
        }
    }

    return result;
}

int32_t expr_eval_value(Expr *expr, SymbolTable *symbols, uint16_t pc) {
    ExprResult result = expr_eval(expr, symbols, NULL, pc, 2, NULL);
    return result.value;
}

int expr_has_symbols(Expr *expr) {
    if (!expr) return 0;

    switch (expr->type) {
        case EXPR_NUMBER:
        case EXPR_CURRENT:
            return 0;
        case EXPR_SYMBOL:
            return 1;
        case EXPR_UNARY:
            return expr_has_symbols(expr->data.unary.operand);
        case EXPR_BINARY:
            return expr_has_symbols(expr->data.binary.left) ||
                   expr_has_symbols(expr->data.binary.right);
    }
    return 0;
}

int expr_is_simple_number(Expr *expr) {
    return expr && expr->type == EXPR_NUMBER;
}

/* ========== Debugging ========== */

static void print_indent(int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
}

static void expr_print_internal(Expr *expr, int depth) {
    if (!expr) {
        print_indent(depth);
        printf("(null)\n");
        return;
    }

    print_indent(depth);
    switch (expr->type) {
        case EXPR_NUMBER:
            printf("NUMBER: %d ($%04X)\n", expr->data.number, expr->data.number & 0xFFFF);
            break;
        case EXPR_SYMBOL:
            printf("SYMBOL: %s\n", expr->data.symbol);
            break;
        case EXPR_CURRENT:
            printf("PC (*)\n");
            break;
        case EXPR_UNARY:
            printf("UNARY: ");
            switch (expr->data.unary.op) {
                case UNARY_NEG: printf("-\n"); break;
                case UNARY_NOT: printf("!\n"); break;
                case UNARY_COMP: printf("~\n"); break;
                case UNARY_LOW: printf("<\n"); break;
                case UNARY_HIGH: printf(">\n"); break;
            }
            expr_print_internal(expr->data.unary.operand, depth + 1);
            break;
        case EXPR_BINARY:
            printf("BINARY: ");
            switch (expr->data.binary.op) {
                case BINARY_ADD: printf("+\n"); break;
                case BINARY_SUB: printf("-\n"); break;
                case BINARY_MUL: printf("*\n"); break;
                case BINARY_DIV: printf("/\n"); break;
                case BINARY_MOD: printf("%%\n"); break;
                case BINARY_AND: printf("&\n"); break;
                case BINARY_OR: printf("|\n"); break;
                case BINARY_XOR: printf("^\n"); break;
                case BINARY_SHL: printf("<<\n"); break;
                case BINARY_SHR: printf(">>\n"); break;
                case BINARY_EQ: printf("=\n"); break;
                case BINARY_NE: printf("<>\n"); break;
                case BINARY_LT: printf("<\n"); break;
                case BINARY_GT: printf(">\n"); break;
                case BINARY_LE: printf("<=\n"); break;
                case BINARY_GE: printf(">=\n"); break;
            }
            expr_print_internal(expr->data.binary.left, depth + 1);
            expr_print_internal(expr->data.binary.right, depth + 1);
            break;
    }
}

void expr_print(Expr *expr) {
    expr_print_internal(expr, 0);
}
