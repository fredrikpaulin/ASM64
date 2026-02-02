/*
 * symbols.h - Symbol Table for ASM64
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <stdint.h>
#include <stdio.h>

/* Symbol flags */
typedef enum {
    SYM_NONE        = 0x00,
    SYM_DEFINED     = 0x01,  /* Value has been assigned */
    SYM_ZEROPAGE    = 0x02,  /* Known to be in zero page ($00-$FF) */
    SYM_CONSTANT    = 0x04,  /* Defined via = (cannot be redefined) */
    SYM_REFERENCED  = 0x08,  /* Has been referenced in code */
    SYM_LOCAL       = 0x10,  /* Local label (starts with .) */
    SYM_EXPORTED    = 0x20,  /* Should appear in symbol file */
    SYM_FORCE_UPDATE = 0x40  /* Force update even if constant (for loops/pass2) */
} SymbolFlags;

/* Symbol entry */
typedef struct Symbol {
    char *name;              /* Symbol name (mangled for locals) */
    char *display_name;      /* Original name for display/output */
    int32_t value;           /* Symbol value (address or constant) */
    uint8_t flags;           /* SymbolFlags bitmask */
    const char *file;        /* File where defined */
    int line;                /* Line where defined */
    struct Symbol *next;     /* Hash chain pointer */
} Symbol;

/* Symbol table */
typedef struct {
    Symbol **buckets;        /* Hash buckets */
    int size;                /* Number of buckets */
    int count;               /* Number of symbols */
} SymbolTable;

/* Scope entry for zone/macro scoping */
typedef struct Scope {
    char *name;              /* Scope name (zone name or NULL) */
    struct Scope *parent;    /* Parent scope */
} Scope;

/* Anonymous label entry */
typedef struct {
    uint16_t address;        /* Label address */
    const char *file;        /* File where defined */
    int line;                /* Line where defined */
} AnonLabel;

/* Anonymous label stacks */
typedef struct {
    AnonLabel *forward;      /* Forward (+) labels */
    int forward_count;
    int forward_capacity;
    int forward_index;       /* Current position for resolution */

    AnonLabel *backward;     /* Backward (-) labels */
    int backward_count;
    int backward_capacity;
} AnonLabels;

/* ========== Symbol Table Functions ========== */

/*
 * Create a new symbol table.
 * size: number of hash buckets (use prime number for better distribution)
 * Returns NULL on allocation failure.
 */
SymbolTable *symbol_table_create(int size);

/*
 * Free a symbol table and all its symbols.
 */
void symbol_table_free(SymbolTable *table);

/*
 * Define a symbol with a value.
 * Returns the symbol on success, NULL on error (duplicate constant, etc.)
 * If symbol already exists and is not a constant, updates the value.
 */
Symbol *symbol_define(SymbolTable *table, const char *name, int32_t value,
                      uint8_t flags, const char *file, int line);

/*
 * Look up a symbol by name.
 * Returns NULL if not found.
 */
Symbol *symbol_lookup(SymbolTable *table, const char *name);

/*
 * Check if a symbol is defined.
 * Returns 1 if defined, 0 if not found or not yet defined.
 */
int symbol_is_defined(SymbolTable *table, const char *name);

/*
 * Mark a symbol as referenced.
 * Creates an undefined symbol entry if it doesn't exist.
 * Returns the symbol.
 */
Symbol *symbol_reference(SymbolTable *table, const char *name,
                         const char *file, int line);

/*
 * Check for undefined symbols that were referenced.
 * Calls error_fn for each undefined symbol.
 * Returns count of undefined symbols.
 */
int symbol_check_undefined(SymbolTable *table,
                           void (*error_fn)(const char *file, int line,
                                           const char *name));

/*
 * Iterate over all symbols in the table.
 * Calls callback for each symbol. If callback returns non-zero, stops iteration.
 */
void symbol_iterate(SymbolTable *table,
                    int (*callback)(Symbol *sym, void *userdata),
                    void *userdata);

/*
 * Write symbols to VICE-format label file.
 * Format: "al C:XXXX .symbolname"
 * Returns 0 on success, -1 on error.
 */
int symbol_write_vice(SymbolTable *table, FILE *fp);

/*
 * Get symbol count.
 */
int symbol_count(SymbolTable *table);

/* ========== Scope Functions ========== */

/*
 * Create initial (global) scope.
 */
Scope *scope_create(void);

/*
 * Push a new scope (for zone or macro).
 * name can be NULL for anonymous scopes.
 */
Scope *scope_push(Scope *current, const char *name);

/*
 * Pop current scope, returning parent.
 * Frees the popped scope.
 */
Scope *scope_pop(Scope *current);

/*
 * Free entire scope chain.
 */
void scope_free(Scope *scope);

/*
 * Mangle a local label name with current scope.
 * Returns newly allocated string: "scope_name.local_name"
 * Caller must free the result.
 */
char *scope_mangle_local(Scope *scope, const char *local_name);

/*
 * Get current scope name (or empty string for global).
 */
const char *scope_get_name(Scope *scope);

/* ========== Anonymous Label Functions ========== */

/*
 * Create anonymous label tracking structure.
 */
AnonLabels *anon_create(void);

/*
 * Free anonymous label structure.
 */
void anon_free(AnonLabels *anon);

/*
 * Reset anonymous labels for a new pass.
 * Keeps the labels but resets forward resolution index.
 */
void anon_reset_pass(AnonLabels *anon);

/*
 * Clear all anonymous labels (for new assembly).
 */
void anon_clear(AnonLabels *anon);

/*
 * Define a forward (+) anonymous label at current address.
 */
void anon_define_forward(AnonLabels *anon, uint16_t address,
                         const char *file, int line);

/*
 * Define a backward (-) anonymous label at current address.
 */
void anon_define_backward(AnonLabels *anon, uint16_t address,
                          const char *file, int line);

/*
 * Resolve a forward reference (+ or ++ etc).
 * count: number of + characters (1 = next, 2 = second next, etc.)
 * Returns address or -1 if not enough forward labels.
 */
int32_t anon_resolve_forward(AnonLabels *anon, int count);

/*
 * Resolve a backward reference (- or -- etc).
 * count: number of - characters (1 = previous, 2 = second previous, etc.)
 * Returns address or -1 if not enough backward labels.
 */
int32_t anon_resolve_backward(AnonLabels *anon, int count);

/*
 * Advance forward resolution index (called after processing each + label def).
 */
void anon_advance_forward(AnonLabels *anon);

#endif /* SYMBOLS_H */
