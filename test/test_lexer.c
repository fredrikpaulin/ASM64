#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "util.h"

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

#define ASSERT(cond) do { if (!(cond)) return 0; } while(0)
#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)

/* Helper to lex a single token */
static Token lex_one(const char *src) {
    Lexer lex;
    lexer_init(&lex, src, "test");
    return lexer_next(&lex);
}

/* Test hex numbers */
TEST(hex_number) {
    Token tok = lex_one("$FF");
    ASSERT_EQ(tok.type, TOK_NUMBER);
    ASSERT_EQ(tok.value.number, 0xFF);
    return 1;
}

TEST(hex_number_lower) {
    Token tok = lex_one("$d020");
    ASSERT_EQ(tok.type, TOK_NUMBER);
    ASSERT_EQ(tok.value.number, 0xD020);
    return 1;
}

TEST(hex_number_zero) {
    Token tok = lex_one("$0000");
    ASSERT_EQ(tok.type, TOK_NUMBER);
    ASSERT_EQ(tok.value.number, 0);
    return 1;
}

TEST(hex_number_max) {
    Token tok = lex_one("$FFFF");
    ASSERT_EQ(tok.type, TOK_NUMBER);
    ASSERT_EQ(tok.value.number, 0xFFFF);
    return 1;
}

/* Test binary numbers */
TEST(binary_number) {
    Token tok = lex_one("%10101010");
    ASSERT_EQ(tok.type, TOK_NUMBER);
    ASSERT_EQ(tok.value.number, 0xAA);
    return 1;
}

TEST(binary_number_small) {
    Token tok = lex_one("%1");
    ASSERT_EQ(tok.type, TOK_NUMBER);
    ASSERT_EQ(tok.value.number, 1);
    return 1;
}

TEST(binary_number_byte) {
    Token tok = lex_one("%11111111");
    ASSERT_EQ(tok.type, TOK_NUMBER);
    ASSERT_EQ(tok.value.number, 0xFF);
    return 1;
}

/* Test decimal numbers */
TEST(decimal_number) {
    Token tok = lex_one("123");
    ASSERT_EQ(tok.type, TOK_NUMBER);
    ASSERT_EQ(tok.value.number, 123);
    return 1;
}

TEST(decimal_zero) {
    Token tok = lex_one("0");
    ASSERT_EQ(tok.type, TOK_NUMBER);
    ASSERT_EQ(tok.value.number, 0);
    return 1;
}

TEST(decimal_large) {
    Token tok = lex_one("65535");
    ASSERT_EQ(tok.type, TOK_NUMBER);
    ASSERT_EQ(tok.value.number, 65535);
    return 1;
}

/* Test character literals */
TEST(char_literal) {
    Token tok = lex_one("'A'");
    ASSERT_EQ(tok.type, TOK_CHAR);
    ASSERT_EQ(tok.value.number, 'A');
    return 1;
}

TEST(char_escape_newline) {
    Token tok = lex_one("'\\n'");
    ASSERT_EQ(tok.type, TOK_CHAR);
    ASSERT_EQ(tok.value.number, 0x0d);  /* PETSCII newline */
    return 1;
}

TEST(char_escape_quote) {
    Token tok = lex_one("'\\''");
    ASSERT_EQ(tok.type, TOK_CHAR);
    ASSERT_EQ(tok.value.number, '\'');
    return 1;
}

/* Test string literals */
TEST(string_simple) {
    Token tok = lex_one("\"hello\"");
    ASSERT_EQ(tok.type, TOK_STRING);
    ASSERT_STR_EQ(tok.value.string.data, "hello");
    ASSERT_EQ(tok.value.string.len, 5);
    token_free(&tok);
    return 1;
}

TEST(string_empty) {
    Token tok = lex_one("\"\"");
    ASSERT_EQ(tok.type, TOK_STRING);
    ASSERT_STR_EQ(tok.value.string.data, "");
    ASSERT_EQ(tok.value.string.len, 0);
    token_free(&tok);
    return 1;
}

TEST(string_escape) {
    Token tok = lex_one("\"a\\\"b\"");
    ASSERT_EQ(tok.type, TOK_STRING);
    ASSERT_STR_EQ(tok.value.string.data, "a\"b");
    token_free(&tok);
    return 1;
}

/* Test identifiers */
TEST(identifier_simple) {
    Token tok = lex_one("label");
    ASSERT_EQ(tok.type, TOK_IDENTIFIER);
    ASSERT(token_equals(&tok, "label"));
    return 1;
}

TEST(identifier_underscore) {
    Token tok = lex_one("_private");
    ASSERT_EQ(tok.type, TOK_IDENTIFIER);
    ASSERT(token_equals(&tok, "_private"));
    return 1;
}

TEST(identifier_mixed) {
    Token tok = lex_one("Label_123");
    ASSERT_EQ(tok.type, TOK_IDENTIFIER);
    ASSERT(token_equals(&tok, "Label_123"));
    return 1;
}

/* Test local labels */
TEST(local_label) {
    Token tok = lex_one(".loop");
    ASSERT_EQ(tok.type, TOK_LOCAL_LABEL);
    ASSERT(token_equals(&tok, ".loop"));
    return 1;
}

/* Test directives */
TEST(directive_byte) {
    Token tok = lex_one("!byte");
    ASSERT_EQ(tok.type, TOK_DIRECTIVE);
    ASSERT(token_equals(&tok, "!byte"));
    return 1;
}

TEST(directive_if) {
    Token tok = lex_one("!if");
    ASSERT_EQ(tok.type, TOK_DIRECTIVE);
    ASSERT(token_equals(&tok, "!if"));
    return 1;
}

/* Test macro calls */
TEST(macro_call) {
    Token tok = lex_one("+mymacro");
    ASSERT_EQ(tok.type, TOK_MACRO_CALL);
    ASSERT(token_equals(&tok, "+mymacro"));
    return 1;
}

/* Test anonymous labels */
TEST(anon_forward) {
    Lexer lex;
    lexer_init(&lex, "++", "test");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_ANON_FWD);
    ASSERT_EQ(tok.value.number, 2);
    return 1;
}

TEST(anon_backward) {
    Lexer lex;
    lexer_init(&lex, "---\n", "test");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_ANON_BACK);
    ASSERT_EQ(tok.value.number, 3);
    return 1;
}

/* Test operators */
TEST(operators) {
    /* Note: standalone + becomes ANON_FWD, - behavior depends on context */
    const char *src = "* / = < > & | ^";
    TokenType expected[] = {
        TOK_STAR, TOK_SLASH, TOK_EQ,
        TOK_LT, TOK_GT, TOK_AMP, TOK_PIPE, TOK_CARET, TOK_EOF
    };

    Lexer lex;
    lexer_init(&lex, src, "test");

    for (int i = 0; expected[i] != TOK_EOF; i++) {
        Token tok = lexer_next(&lex);
        ASSERT_EQ(tok.type, expected[i]);
    }
    return 1;
}

TEST(two_char_operators) {
    const char *src = "<< >> <= >= <>";
    TokenType expected[] = {
        TOK_LSHIFT, TOK_RSHIFT, TOK_LE, TOK_GE, TOK_NE, TOK_EOF
    };

    Lexer lex;
    lexer_init(&lex, src, "test");

    for (int i = 0; expected[i] != TOK_EOF; i++) {
        Token tok = lexer_next(&lex);
        ASSERT_EQ(tok.type, expected[i]);
    }
    return 1;
}

/* Test delimiters */
TEST(delimiters) {
    const char *src = "(){}[],:#";
    TokenType expected[] = {
        TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
        TOK_LBRACKET, TOK_RBRACKET, TOK_COMMA, TOK_COLON, TOK_HASH, TOK_EOF
    };

    Lexer lex;
    lexer_init(&lex, src, "test");

    for (int i = 0; expected[i] != TOK_EOF; i++) {
        Token tok = lexer_next(&lex);
        ASSERT_EQ(tok.type, expected[i]);
    }
    return 1;
}

/* Test comments */
TEST(comment_skip) {
    const char *src = "label ; this is a comment\n";
    Lexer lex;
    lexer_init(&lex, src, "test");

    Token tok1 = lexer_next(&lex);
    ASSERT_EQ(tok1.type, TOK_IDENTIFIER);

    Token tok2 = lexer_next(&lex);
    ASSERT_EQ(tok2.type, TOK_EOL);

    return 1;
}

/* Test line tracking */
TEST(line_tracking) {
    const char *src = "line1\nline2\nline3";
    Lexer lex;
    lexer_init(&lex, src, "test");

    Token tok1 = lexer_next(&lex);
    ASSERT_EQ(tok1.line, 1);

    lexer_next(&lex);  /* EOL */

    Token tok2 = lexer_next(&lex);
    ASSERT_EQ(tok2.line, 2);

    return 1;
}

/* Test mnemonic detection */
TEST(mnemonic_lda) {
    Token tok = lex_one("LDA");
    ASSERT_EQ(tok.type, TOK_IDENTIFIER);
    ASSERT(token_is_mnemonic(&tok));
    return 1;
}

TEST(mnemonic_case_insensitive) {
    Token tok = lex_one("jmp");
    ASSERT(token_is_mnemonic(&tok));

    tok = lex_one("JMP");
    ASSERT(token_is_mnemonic(&tok));

    tok = lex_one("Jmp");
    ASSERT(token_is_mnemonic(&tok));
    return 1;
}

TEST(not_mnemonic) {
    Token tok = lex_one("mylabel");
    ASSERT(!token_is_mnemonic(&tok));
    return 1;
}

/* Test full assembly line */
TEST(assembly_line) {
    const char *src = "loop: LDA #$10 ; load value\n";
    TokenType expected[] = {
        TOK_IDENTIFIER, TOK_COLON, TOK_IDENTIFIER,
        TOK_HASH, TOK_NUMBER, TOK_EOL, TOK_EOF
    };

    Lexer lex;
    lexer_init(&lex, src, "test");

    for (int i = 0; expected[i] != TOK_EOF; i++) {
        Token tok = lexer_next(&lex);
        ASSERT_EQ(tok.type, expected[i]);
    }
    return 1;
}

TEST(indexed_addressing) {
    const char *src = "LDA $1000,X";
    Lexer lex;
    lexer_init(&lex, src, "test");

    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_IDENTIFIER);  /* LDA */

    tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_NUMBER);  /* $1000 */
    ASSERT_EQ(tok.value.number, 0x1000);

    tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_COMMA);

    tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_IDENTIFIER);  /* X */
    ASSERT(token_equals(&tok, "X"));

    return 1;
}

TEST(indirect_indexed) {
    const char *src = "LDA ($FB),Y";
    Lexer lex;
    lexer_init(&lex, src, "test");

    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_IDENTIFIER);  /* LDA */

    tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_LPAREN);

    tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_NUMBER);  /* $FB */
    ASSERT_EQ(tok.value.number, 0xFB);

    tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_RPAREN);

    tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_COMMA);

    tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_IDENTIFIER);  /* Y */
    ASSERT(token_equals(&tok, "Y"));

    return 1;
}

int main(void) {
    printf("Lexer Tests\n");
    printf("===========\n\n");

    printf("Number parsing:\n");
    RUN_TEST(hex_number);
    RUN_TEST(hex_number_lower);
    RUN_TEST(hex_number_zero);
    RUN_TEST(hex_number_max);
    RUN_TEST(binary_number);
    RUN_TEST(binary_number_small);
    RUN_TEST(binary_number_byte);
    RUN_TEST(decimal_number);
    RUN_TEST(decimal_zero);
    RUN_TEST(decimal_large);

    printf("\nCharacter/String literals:\n");
    RUN_TEST(char_literal);
    RUN_TEST(char_escape_newline);
    RUN_TEST(char_escape_quote);
    RUN_TEST(string_simple);
    RUN_TEST(string_empty);
    RUN_TEST(string_escape);

    printf("\nIdentifiers and labels:\n");
    RUN_TEST(identifier_simple);
    RUN_TEST(identifier_underscore);
    RUN_TEST(identifier_mixed);
    RUN_TEST(local_label);

    printf("\nDirectives and macros:\n");
    RUN_TEST(directive_byte);
    RUN_TEST(directive_if);
    RUN_TEST(macro_call);

    printf("\nAnonymous labels:\n");
    RUN_TEST(anon_forward);
    RUN_TEST(anon_backward);

    printf("\nOperators:\n");
    RUN_TEST(operators);
    RUN_TEST(two_char_operators);
    RUN_TEST(delimiters);

    printf("\nComments and lines:\n");
    RUN_TEST(comment_skip);
    RUN_TEST(line_tracking);

    printf("\nMnemonics:\n");
    RUN_TEST(mnemonic_lda);
    RUN_TEST(mnemonic_case_insensitive);
    RUN_TEST(not_mnemonic);

    printf("\nFull lines:\n");
    RUN_TEST(assembly_line);
    RUN_TEST(indexed_addressing);
    RUN_TEST(indirect_indexed);

    printf("\n===========\n");
    printf("Results: %d/%d passed\n", tests_passed, tests_run);

    return tests_passed == tests_run ? 0 : 1;
}
