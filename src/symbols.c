/*
 * symbols.c - Symbol Table Implementation
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#include "symbols.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Default initial capacity for anonymous label arrays */
#define ANON_INITIAL_CAPACITY 64

/* ========== Hash Function ========== */

/* Case-insensitive hash for symbol names */
static unsigned int hash_symbol(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + toupper(c);
    }
    return hash;
}

/* Case-insensitive string comparison */
static int str_equal_nocase(const char *a, const char *b) {
    while (*a && *b) {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

/* Duplicate a string */
static char *str_dup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *dup = malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

/* ========== Symbol Table Implementation ========== */

SymbolTable *symbol_table_create(int size) {
    SymbolTable *table = malloc(sizeof(SymbolTable));
    if (!table) return NULL;

    table->buckets = calloc(size, sizeof(Symbol *));
    if (!table->buckets) {
        free(table);
        return NULL;
    }

    table->size = size;
    table->count = 0;
    return table;
}

void symbol_table_free(SymbolTable *table) {
    if (!table) return;

    for (int i = 0; i < table->size; i++) {
        Symbol *sym = table->buckets[i];
        while (sym) {
            Symbol *next = sym->next;
            free(sym->name);
            free(sym->display_name);
            free(sym);
            sym = next;
        }
    }

    free(table->buckets);
    free(table);
}

Symbol *symbol_define(SymbolTable *table, const char *name, int32_t value,
                      uint8_t flags, const char *file, int line) {
    if (!table || !name) return NULL;

    unsigned int bucket = hash_symbol(name) % table->size;

    /* Check if symbol already exists */
    Symbol *sym = table->buckets[bucket];
    while (sym) {
        if (str_equal_nocase(sym->name, name)) {
            /* Found existing symbol */
            if (sym->flags & SYM_CONSTANT) {
                if (flags & SYM_FORCE_UPDATE) {
                    /* Force update is allowed (loop context or pass 2).
                     * Clear the constant flag to allow future reassignments. */
                    sym->flags &= ~SYM_CONSTANT;
                } else {
                    /* Cannot redefine a constant */
                    return NULL;
                }
            }
            /* Update value and flags (don't persist the FORCE_UPDATE flag) */
            sym->value = value;
            sym->flags |= ((flags & ~SYM_FORCE_UPDATE) | SYM_DEFINED);
            return sym;
        }
        sym = sym->next;
    }

    /* Create new symbol */
    sym = malloc(sizeof(Symbol));
    if (!sym) return NULL;

    sym->name = str_dup(name);
    sym->display_name = str_dup(name);
    if (!sym->name) {
        free(sym);
        return NULL;
    }

    sym->value = value;
    sym->flags = flags | SYM_DEFINED;
    sym->file = file;
    sym->line = line;

    /* Insert at head of bucket chain */
    sym->next = table->buckets[bucket];
    table->buckets[bucket] = sym;
    table->count++;

    return sym;
}

Symbol *symbol_lookup(SymbolTable *table, const char *name) {
    if (!table || !name) return NULL;

    unsigned int bucket = hash_symbol(name) % table->size;
    Symbol *sym = table->buckets[bucket];

    while (sym) {
        if (str_equal_nocase(sym->name, name)) {
            return sym;
        }
        sym = sym->next;
    }

    return NULL;
}

int symbol_is_defined(SymbolTable *table, const char *name) {
    Symbol *sym = symbol_lookup(table, name);
    return sym && (sym->flags & SYM_DEFINED);
}

Symbol *symbol_reference(SymbolTable *table, const char *name,
                         const char *file, int line) {
    if (!table || !name) return NULL;

    Symbol *sym = symbol_lookup(table, name);
    if (sym) {
        sym->flags |= SYM_REFERENCED;
        return sym;
    }

    /* Create undefined symbol entry */
    unsigned int bucket = hash_symbol(name) % table->size;

    sym = malloc(sizeof(Symbol));
    if (!sym) return NULL;

    sym->name = str_dup(name);
    sym->display_name = str_dup(name);
    if (!sym->name) {
        free(sym);
        return NULL;
    }

    sym->value = 0;
    sym->flags = SYM_REFERENCED;  /* Not SYM_DEFINED */
    sym->file = file;
    sym->line = line;

    sym->next = table->buckets[bucket];
    table->buckets[bucket] = sym;
    table->count++;

    return sym;
}

int symbol_check_undefined(SymbolTable *table,
                           void (*error_fn)(const char *file, int line,
                                           const char *name)) {
    int undefined_count = 0;

    for (int i = 0; i < table->size; i++) {
        Symbol *sym = table->buckets[i];
        while (sym) {
            if ((sym->flags & SYM_REFERENCED) && !(sym->flags & SYM_DEFINED)) {
                undefined_count++;
                if (error_fn) {
                    error_fn(sym->file, sym->line, sym->name);
                }
            }
            sym = sym->next;
        }
    }

    return undefined_count;
}

void symbol_iterate(SymbolTable *table,
                    int (*callback)(Symbol *sym, void *userdata),
                    void *userdata) {
    if (!table || !callback) return;

    for (int i = 0; i < table->size; i++) {
        Symbol *sym = table->buckets[i];
        while (sym) {
            if (callback(sym, userdata)) {
                return;  /* Callback requested stop */
            }
            sym = sym->next;
        }
    }
}

/* Comparison function for qsort - sort by address then name */
static int symbol_compare(const void *a, const void *b) {
    const Symbol *sa = *(const Symbol **)a;
    const Symbol *sb = *(const Symbol **)b;

    if (sa->value != sb->value) {
        return (sa->value > sb->value) ? 1 : -1;
    }
    return strcmp(sa->display_name, sb->display_name);
}

int symbol_write_vice(SymbolTable *table, FILE *fp) {
    if (!table || !fp) return -1;

    /* Collect all defined symbols into array for sorting */
    Symbol **symbols = malloc(table->count * sizeof(Symbol *));
    if (!symbols) return -1;

    int count = 0;
    for (int i = 0; i < table->size; i++) {
        Symbol *sym = table->buckets[i];
        while (sym) {
            if (sym->flags & SYM_DEFINED) {
                symbols[count++] = sym;
            }
            sym = sym->next;
        }
    }

    /* Sort by address */
    qsort(symbols, count, sizeof(Symbol *), symbol_compare);

    /* Write in VICE format */
    for (int i = 0; i < count; i++) {
        Symbol *sym = symbols[i];
        /* VICE format: "al C:XXXX .symbolname" */
        fprintf(fp, "al C:%04X .%s\n",
                (uint16_t)sym->value,
                sym->display_name);
    }

    free(symbols);
    return 0;
}

int symbol_count(SymbolTable *table) {
    return table ? table->count : 0;
}

/* ========== Scope Implementation ========== */

Scope *scope_create(void) {
    Scope *scope = malloc(sizeof(Scope));
    if (!scope) return NULL;

    scope->name = NULL;  /* Global scope has no name */
    scope->parent = NULL;
    return scope;
}

Scope *scope_push(Scope *current, const char *name) {
    Scope *scope = malloc(sizeof(Scope));
    if (!scope) return current;

    scope->name = name ? str_dup(name) : NULL;
    scope->parent = current;
    return scope;
}

Scope *scope_pop(Scope *current) {
    if (!current || !current->parent) {
        return current;  /* Can't pop global scope */
    }

    Scope *parent = current->parent;
    free(current->name);
    free(current);
    return parent;
}

void scope_free(Scope *scope) {
    while (scope) {
        Scope *parent = scope->parent;
        free(scope->name);
        free(scope);
        scope = parent;
    }
}

char *scope_mangle_local(Scope *scope, const char *local_name) {
    if (!local_name) return NULL;

    /* Skip leading . if present */
    if (local_name[0] == '.') {
        local_name++;
    }

    if (!scope || !scope->name) {
        /* Global scope - use _global prefix */
        size_t len = strlen(local_name) + 10;
        char *mangled = malloc(len);
        if (mangled) {
            snprintf(mangled, len, "_global.%s", local_name);
        }
        return mangled;
    }

    /* Build mangled name: scope_name.local_name */
    size_t scope_len = strlen(scope->name);
    size_t local_len = strlen(local_name);
    size_t total_len = scope_len + 1 + local_len + 1;

    char *mangled = malloc(total_len);
    if (mangled) {
        snprintf(mangled, total_len, "%s.%s", scope->name, local_name);
    }
    return mangled;
}

const char *scope_get_name(Scope *scope) {
    if (!scope || !scope->name) {
        return "";
    }
    return scope->name;
}

/* ========== Anonymous Labels Implementation ========== */

AnonLabels *anon_create(void) {
    AnonLabels *anon = malloc(sizeof(AnonLabels));
    if (!anon) return NULL;

    anon->forward = malloc(ANON_INITIAL_CAPACITY * sizeof(AnonLabel));
    anon->backward = malloc(ANON_INITIAL_CAPACITY * sizeof(AnonLabel));

    if (!anon->forward || !anon->backward) {
        free(anon->forward);
        free(anon->backward);
        free(anon);
        return NULL;
    }

    anon->forward_count = 0;
    anon->forward_capacity = ANON_INITIAL_CAPACITY;
    anon->forward_index = 0;

    anon->backward_count = 0;
    anon->backward_capacity = ANON_INITIAL_CAPACITY;

    return anon;
}

void anon_free(AnonLabels *anon) {
    if (!anon) return;
    free(anon->forward);
    free(anon->backward);
    free(anon);
}

void anon_reset_pass(AnonLabels *anon) {
    if (!anon) return;
    anon->forward_index = 0;
    anon->backward_count = 0;  /* Reset backward labels for pass 2 re-definition */
}

void anon_clear(AnonLabels *anon) {
    if (!anon) return;
    anon->forward_count = 0;
    anon->forward_index = 0;
    anon->backward_count = 0;
}

static int anon_grow_forward(AnonLabels *anon) {
    int new_cap = anon->forward_capacity * 2;
    AnonLabel *new_arr = realloc(anon->forward, new_cap * sizeof(AnonLabel));
    if (!new_arr) return 0;
    anon->forward = new_arr;
    anon->forward_capacity = new_cap;
    return 1;
}

static int anon_grow_backward(AnonLabels *anon) {
    int new_cap = anon->backward_capacity * 2;
    AnonLabel *new_arr = realloc(anon->backward, new_cap * sizeof(AnonLabel));
    if (!new_arr) return 0;
    anon->backward = new_arr;
    anon->backward_capacity = new_cap;
    return 1;
}

void anon_define_forward(AnonLabels *anon, uint16_t address,
                         const char *file, int line) {
    if (!anon) return;

    if (anon->forward_count >= anon->forward_capacity) {
        if (!anon_grow_forward(anon)) return;
    }

    AnonLabel *label = &anon->forward[anon->forward_count++];
    label->address = address;
    label->file = file;
    label->line = line;
}

void anon_define_backward(AnonLabels *anon, uint16_t address,
                          const char *file, int line) {
    if (!anon) return;

    if (anon->backward_count >= anon->backward_capacity) {
        if (!anon_grow_backward(anon)) return;
    }

    AnonLabel *label = &anon->backward[anon->backward_count++];
    label->address = address;
    label->file = file;
    label->line = line;
}

int32_t anon_resolve_forward(AnonLabels *anon, int count) {
    if (!anon || count < 1) return -1;

    /* Forward references look ahead from current position */
    int target_index = anon->forward_index + count - 1;
    if (target_index >= anon->forward_count) {
        return -1;  /* Not enough forward labels */
    }

    return anon->forward[target_index].address;
}

int32_t anon_resolve_backward(AnonLabels *anon, int count) {
    if (!anon || count < 1) return -1;

    /* Backward references look back from current position */
    int target_index = anon->backward_count - count;
    if (target_index < 0) {
        return -1;  /* Not enough backward labels */
    }

    return anon->backward[target_index].address;
}

void anon_advance_forward(AnonLabels *anon) {
    if (!anon) return;
    if (anon->forward_index < anon->forward_count) {
        anon->forward_index++;
    }
}
