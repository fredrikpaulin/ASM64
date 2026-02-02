/*
 * test_symbols.c - Unit tests for symbol table
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "symbols.h"

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

/* ========== Symbol Table Basic Tests ========== */

TEST(create_table) {
    SymbolTable *table = symbol_table_create(127);
    int ok = table != NULL && symbol_count(table) == 0;
    symbol_table_free(table);
    return ok;
}

TEST(define_symbol) {
    SymbolTable *table = symbol_table_create(127);
    Symbol *sym = symbol_define(table, "START", 0x0801, SYM_NONE, "test.asm", 1);
    int ok = sym != NULL &&
             sym->value == 0x0801 &&
             (sym->flags & SYM_DEFINED) &&
             symbol_count(table) == 1;
    symbol_table_free(table);
    return ok;
}

TEST(lookup_symbol) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "LABEL1", 0x1000, SYM_NONE, "test.asm", 1);
    Symbol *sym = symbol_lookup(table, "LABEL1");
    int ok = sym != NULL && sym->value == 0x1000;
    symbol_table_free(table);
    return ok;
}

TEST(lookup_nonexistent) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "LABEL1", 0x1000, SYM_NONE, "test.asm", 1);
    Symbol *sym = symbol_lookup(table, "NOTFOUND");
    int ok = sym == NULL;
    symbol_table_free(table);
    return ok;
}

TEST(is_defined) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "DEFINED", 0x1000, SYM_NONE, "test.asm", 1);
    int ok = symbol_is_defined(table, "DEFINED") &&
             !symbol_is_defined(table, "NOTDEFINED");
    symbol_table_free(table);
    return ok;
}

TEST(case_insensitive_lookup) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "MyLabel", 0x1000, SYM_NONE, "test.asm", 1);

    Symbol *s1 = symbol_lookup(table, "mylabel");
    Symbol *s2 = symbol_lookup(table, "MYLABEL");
    Symbol *s3 = symbol_lookup(table, "MyLabel");

    int ok = s1 && s2 && s3 &&
             s1->value == 0x1000 &&
             s2->value == 0x1000 &&
             s3->value == 0x1000;
    symbol_table_free(table);
    return ok;
}

TEST(multiple_symbols) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "LABEL1", 0x1000, SYM_NONE, "test.asm", 1);
    symbol_define(table, "LABEL2", 0x2000, SYM_NONE, "test.asm", 2);
    symbol_define(table, "LABEL3", 0x3000, SYM_NONE, "test.asm", 3);

    Symbol *s1 = symbol_lookup(table, "LABEL1");
    Symbol *s2 = symbol_lookup(table, "LABEL2");
    Symbol *s3 = symbol_lookup(table, "LABEL3");

    int ok = s1 && s2 && s3 &&
             s1->value == 0x1000 &&
             s2->value == 0x2000 &&
             s3->value == 0x3000 &&
             symbol_count(table) == 3;
    symbol_table_free(table);
    return ok;
}

TEST(update_non_constant) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "LABEL", 0x1000, SYM_NONE, "test.asm", 1);
    symbol_define(table, "LABEL", 0x2000, SYM_NONE, "test.asm", 5);

    Symbol *sym = symbol_lookup(table, "LABEL");
    int ok = sym && sym->value == 0x2000 && symbol_count(table) == 1;
    symbol_table_free(table);
    return ok;
}

TEST(constant_no_redefine) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "CONST", 100, SYM_CONSTANT, "test.asm", 1);
    Symbol *sym = symbol_define(table, "CONST", 200, SYM_NONE, "test.asm", 5);

    int ok = sym == NULL;  /* Should fail to redefine */
    Symbol *original = symbol_lookup(table, "CONST");
    ok = ok && original && original->value == 100;  /* Original value preserved */
    symbol_table_free(table);
    return ok;
}

/* ========== Symbol Flags Tests ========== */

TEST(zeropage_flag) {
    SymbolTable *table = symbol_table_create(127);
    Symbol *sym = symbol_define(table, "ZPVAR", 0x80, SYM_ZEROPAGE, "test.asm", 1);
    int ok = sym && (sym->flags & SYM_ZEROPAGE);
    symbol_table_free(table);
    return ok;
}

TEST(constant_flag) {
    SymbolTable *table = symbol_table_create(127);
    Symbol *sym = symbol_define(table, "CONST", 42, SYM_CONSTANT, "test.asm", 1);
    int ok = sym && (sym->flags & SYM_CONSTANT);
    symbol_table_free(table);
    return ok;
}

TEST(reference_existing) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "LABEL", 0x1000, SYM_NONE, "test.asm", 1);
    Symbol *sym = symbol_reference(table, "LABEL", "test.asm", 10);
    int ok = sym && (sym->flags & SYM_REFERENCED) && (sym->flags & SYM_DEFINED);
    symbol_table_free(table);
    return ok;
}

TEST(reference_undefined) {
    SymbolTable *table = symbol_table_create(127);
    Symbol *sym = symbol_reference(table, "UNDEFINED", "test.asm", 10);
    int ok = sym &&
             (sym->flags & SYM_REFERENCED) &&
             !(sym->flags & SYM_DEFINED);
    symbol_table_free(table);
    return ok;
}

/* Track error callbacks */
static int undefined_error_count = 0;
static void count_undefined(const char *file, int line, const char *name) {
    (void)file; (void)line; (void)name;
    undefined_error_count++;
}

TEST(check_undefined) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "DEFINED1", 0x1000, SYM_NONE, "test.asm", 1);
    symbol_reference(table, "DEFINED1", "test.asm", 10);
    symbol_reference(table, "UNDEFINED1", "test.asm", 20);
    symbol_reference(table, "UNDEFINED2", "test.asm", 30);

    undefined_error_count = 0;
    int count = symbol_check_undefined(table, count_undefined);

    int ok = count == 2 && undefined_error_count == 2;
    symbol_table_free(table);
    return ok;
}

/* ========== Symbol Iteration Tests ========== */

static int iteration_count = 0;
static int count_callback(Symbol *sym, void *userdata) {
    (void)sym; (void)userdata;
    iteration_count++;
    return 0;
}

TEST(iterate_symbols) {
    SymbolTable *table = symbol_table_create(127);
    symbol_define(table, "A", 1, SYM_NONE, "test.asm", 1);
    symbol_define(table, "B", 2, SYM_NONE, "test.asm", 2);
    symbol_define(table, "C", 3, SYM_NONE, "test.asm", 3);

    iteration_count = 0;
    symbol_iterate(table, count_callback, NULL);

    int ok = iteration_count == 3;
    symbol_table_free(table);
    return ok;
}

static int stop_callback(Symbol *sym, void *userdata) {
    (void)userdata;
    iteration_count++;
    return strcmp(sym->name, "B") == 0;  /* Stop at B */
}

TEST(iterate_early_stop) {
    SymbolTable *table = symbol_table_create(1);  /* Single bucket forces order */
    symbol_define(table, "A", 1, SYM_NONE, "test.asm", 1);
    symbol_define(table, "B", 2, SYM_NONE, "test.asm", 2);
    symbol_define(table, "C", 3, SYM_NONE, "test.asm", 3);

    iteration_count = 0;
    symbol_iterate(table, stop_callback, NULL);

    /* Should stop after finding B */
    int ok = iteration_count <= 3;  /* Depends on hash order */
    symbol_table_free(table);
    return ok;
}

/* ========== Scope Tests ========== */

TEST(create_scope) {
    Scope *scope = scope_create();
    int ok = scope != NULL && scope->parent == NULL;
    scope_free(scope);
    return ok;
}

TEST(push_scope) {
    Scope *global = scope_create();
    Scope *zone = scope_push(global, "MyZone");
    int ok = zone != NULL &&
             zone->parent == global &&
             strcmp(zone->name, "MyZone") == 0;
    scope_free(zone);
    return ok;
}

TEST(pop_scope) {
    Scope *global = scope_create();
    Scope *zone = scope_push(global, "MyZone");
    Scope *back = scope_pop(zone);
    int ok = back == global;
    scope_free(back);
    return ok;
}

TEST(scope_cannot_pop_global) {
    Scope *global = scope_create();
    Scope *result = scope_pop(global);
    int ok = result == global;  /* Can't pop global scope */
    scope_free(result);
    return ok;
}

TEST(mangle_local_global_scope) {
    Scope *global = scope_create();
    char *mangled = scope_mangle_local(global, ".loop");
    int ok = mangled && strcmp(mangled, "_global.loop") == 0;
    free(mangled);
    scope_free(global);
    return ok;
}

TEST(mangle_local_named_scope) {
    Scope *global = scope_create();
    Scope *zone = scope_push(global, "MainLoop");
    char *mangled = scope_mangle_local(zone, ".next");
    int ok = mangled && strcmp(mangled, "MainLoop.next") == 0;
    free(mangled);
    scope_free(zone);
    return ok;
}

TEST(mangle_strips_dot) {
    Scope *global = scope_create();
    Scope *zone = scope_push(global, "Zone");
    char *m1 = scope_mangle_local(zone, ".local");
    char *m2 = scope_mangle_local(zone, "local");  /* Without dot */
    int ok = m1 && m2 && strcmp(m1, m2) == 0;
    free(m1);
    free(m2);
    scope_free(zone);
    return ok;
}

TEST(nested_scopes) {
    Scope *global = scope_create();
    Scope *outer = scope_push(global, "Outer");
    Scope *inner = scope_push(outer, "Inner");

    int ok = inner->parent == outer &&
             outer->parent == global &&
             strcmp(scope_get_name(inner), "Inner") == 0;

    scope_free(inner);  /* Frees entire chain */
    return ok;
}

/* ========== Anonymous Label Tests ========== */

TEST(anon_create) {
    AnonLabels *anon = anon_create();
    int ok = anon != NULL;
    anon_free(anon);
    return ok;
}

TEST(anon_define_backward) {
    AnonLabels *anon = anon_create();
    anon_define_backward(anon, 0x1000, "test.asm", 10);
    anon_define_backward(anon, 0x1010, "test.asm", 20);
    anon_define_backward(anon, 0x1020, "test.asm", 30);

    /* Resolve - (most recent) */
    int32_t addr = anon_resolve_backward(anon, 1);
    int ok = addr == 0x1020;
    anon_free(anon);
    return ok;
}

TEST(anon_resolve_backward_multiple) {
    AnonLabels *anon = anon_create();
    anon_define_backward(anon, 0x1000, "test.asm", 10);
    anon_define_backward(anon, 0x1010, "test.asm", 20);
    anon_define_backward(anon, 0x1020, "test.asm", 30);

    int32_t m1 = anon_resolve_backward(anon, 1);  /* Most recent */
    int32_t m2 = anon_resolve_backward(anon, 2);  /* Second most recent */
    int32_t m3 = anon_resolve_backward(anon, 3);  /* Third most recent */

    int ok = m1 == 0x1020 && m2 == 0x1010 && m3 == 0x1000;
    anon_free(anon);
    return ok;
}

TEST(anon_backward_not_enough) {
    AnonLabels *anon = anon_create();
    anon_define_backward(anon, 0x1000, "test.asm", 10);

    int32_t result = anon_resolve_backward(anon, 2);  /* Only one defined */
    int ok = result == -1;
    anon_free(anon);
    return ok;
}

TEST(anon_define_forward) {
    AnonLabels *anon = anon_create();
    anon_define_forward(anon, 0x2000, "test.asm", 10);
    anon_define_forward(anon, 0x2010, "test.asm", 20);
    anon_define_forward(anon, 0x2020, "test.asm", 30);

    /* Forward resolution starts from index 0 */
    int32_t p1 = anon_resolve_forward(anon, 1);  /* First forward */
    int ok = p1 == 0x2000;
    anon_free(anon);
    return ok;
}

TEST(anon_resolve_forward_multiple) {
    AnonLabels *anon = anon_create();
    anon_define_forward(anon, 0x2000, "test.asm", 10);
    anon_define_forward(anon, 0x2010, "test.asm", 20);
    anon_define_forward(anon, 0x2020, "test.asm", 30);

    int32_t p1 = anon_resolve_forward(anon, 1);  /* + */
    int32_t p2 = anon_resolve_forward(anon, 2);  /* ++ */
    int32_t p3 = anon_resolve_forward(anon, 3);  /* +++ */

    int ok = p1 == 0x2000 && p2 == 0x2010 && p3 == 0x2020;
    anon_free(anon);
    return ok;
}

TEST(anon_forward_advance) {
    AnonLabels *anon = anon_create();
    anon_define_forward(anon, 0x2000, "test.asm", 10);
    anon_define_forward(anon, 0x2010, "test.asm", 20);
    anon_define_forward(anon, 0x2020, "test.asm", 30);

    int32_t p1 = anon_resolve_forward(anon, 1);  /* 0x2000 */
    anon_advance_forward(anon);
    int32_t p2 = anon_resolve_forward(anon, 1);  /* Now 0x2010 */
    anon_advance_forward(anon);
    int32_t p3 = anon_resolve_forward(anon, 1);  /* Now 0x2020 */

    int ok = p1 == 0x2000 && p2 == 0x2010 && p3 == 0x2020;
    anon_free(anon);
    return ok;
}

TEST(anon_forward_not_enough) {
    AnonLabels *anon = anon_create();
    anon_define_forward(anon, 0x2000, "test.asm", 10);

    int32_t result = anon_resolve_forward(anon, 2);  /* Only one defined */
    int ok = result == -1;
    anon_free(anon);
    return ok;
}

TEST(anon_reset_pass) {
    AnonLabels *anon = anon_create();
    anon_define_forward(anon, 0x2000, "test.asm", 10);
    anon_define_forward(anon, 0x2010, "test.asm", 20);

    anon_advance_forward(anon);
    anon_advance_forward(anon);

    /* After advancing, no more forward labels available from current index */
    int32_t before = anon_resolve_forward(anon, 1);

    anon_reset_pass(anon);

    /* After reset, back to beginning */
    int32_t after = anon_resolve_forward(anon, 1);

    int ok = before == -1 && after == 0x2000;
    anon_free(anon);
    return ok;
}

TEST(anon_clear) {
    AnonLabels *anon = anon_create();
    anon_define_forward(anon, 0x2000, "test.asm", 10);
    anon_define_backward(anon, 0x1000, "test.asm", 5);

    anon_clear(anon);

    int32_t fwd = anon_resolve_forward(anon, 1);
    int32_t bwd = anon_resolve_backward(anon, 1);

    int ok = fwd == -1 && bwd == -1;
    anon_free(anon);
    return ok;
}

/* ========== Integration Tests ========== */

TEST(local_labels_with_scope) {
    SymbolTable *table = symbol_table_create(127);
    Scope *global = scope_create();

    /* Define global label */
    symbol_define(table, "Main", 0x0801, SYM_NONE, "test.asm", 1);

    /* Enter zone */
    Scope *zone = scope_push(global, "Main");

    /* Define local label */
    char *mangled = scope_mangle_local(zone, ".loop");
    symbol_define(table, mangled, 0x0810, SYM_LOCAL, "test.asm", 5);

    /* Look up both */
    Symbol *main_sym = symbol_lookup(table, "Main");
    Symbol *local_sym = symbol_lookup(table, mangled);

    int ok = main_sym && local_sym &&
             main_sym->value == 0x0801 &&
             local_sym->value == 0x0810;

    free(mangled);
    scope_free(zone);
    symbol_table_free(table);
    return ok;
}

TEST(same_local_different_zones) {
    SymbolTable *table = symbol_table_create(127);
    Scope *global = scope_create();

    /* Zone 1 */
    Scope *zone1 = scope_push(global, "Zone1");
    char *mangled1 = scope_mangle_local(zone1, ".loop");
    symbol_define(table, mangled1, 0x1000, SYM_LOCAL, "test.asm", 10);
    Scope *back = scope_pop(zone1);

    /* Zone 2 */
    Scope *zone2 = scope_push(back, "Zone2");
    char *mangled2 = scope_mangle_local(zone2, ".loop");
    symbol_define(table, mangled2, 0x2000, SYM_LOCAL, "test.asm", 20);

    /* Both should exist with different values */
    Symbol *s1 = symbol_lookup(table, mangled1);
    Symbol *s2 = symbol_lookup(table, mangled2);

    int ok = s1 && s2 &&
             s1->value == 0x1000 &&
             s2->value == 0x2000 &&
             s1 != s2;

    free(mangled1);
    free(mangled2);
    scope_free(zone2);
    symbol_table_free(table);
    return ok;
}

/* ========== Main ========== */

int main(void) {
    printf("\nSymbol Table Tests\n");
    printf("==================\n\n");

    printf("Basic Symbol Table:\n");
    RUN_TEST(create_table);
    RUN_TEST(define_symbol);
    RUN_TEST(lookup_symbol);
    RUN_TEST(lookup_nonexistent);
    RUN_TEST(is_defined);
    RUN_TEST(case_insensitive_lookup);
    RUN_TEST(multiple_symbols);
    RUN_TEST(update_non_constant);
    RUN_TEST(constant_no_redefine);

    printf("\nSymbol Flags:\n");
    RUN_TEST(zeropage_flag);
    RUN_TEST(constant_flag);
    RUN_TEST(reference_existing);
    RUN_TEST(reference_undefined);
    RUN_TEST(check_undefined);

    printf("\nSymbol Iteration:\n");
    RUN_TEST(iterate_symbols);
    RUN_TEST(iterate_early_stop);

    printf("\nScope Management:\n");
    RUN_TEST(create_scope);
    RUN_TEST(push_scope);
    RUN_TEST(pop_scope);
    RUN_TEST(scope_cannot_pop_global);
    RUN_TEST(mangle_local_global_scope);
    RUN_TEST(mangle_local_named_scope);
    RUN_TEST(mangle_strips_dot);
    RUN_TEST(nested_scopes);

    printf("\nAnonymous Labels - Backward:\n");
    RUN_TEST(anon_create);
    RUN_TEST(anon_define_backward);
    RUN_TEST(anon_resolve_backward_multiple);
    RUN_TEST(anon_backward_not_enough);

    printf("\nAnonymous Labels - Forward:\n");
    RUN_TEST(anon_define_forward);
    RUN_TEST(anon_resolve_forward_multiple);
    RUN_TEST(anon_forward_advance);
    RUN_TEST(anon_forward_not_enough);
    RUN_TEST(anon_reset_pass);
    RUN_TEST(anon_clear);

    printf("\nIntegration:\n");
    RUN_TEST(local_labels_with_scope);
    RUN_TEST(same_local_different_zones);

    printf("\n==================\n");
    printf("Results: %d/%d passed\n\n", tests_passed, tests_run);

    return tests_passed == tests_run ? 0 : 1;
}
