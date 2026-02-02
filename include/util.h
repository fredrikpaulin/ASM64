#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>

/* Safe string duplication - caller must free */
char *str_dup(const char *s);

/* Duplicate n characters - caller must free */
char *str_ndup(const char *s, size_t n);

/* Convert string to lowercase in place */
char *str_tolower(char *s);

/* Convert string to uppercase in place */
char *str_toupper(char *s);

/* Trim leading whitespace, returns pointer into same string */
char *str_ltrim(char *s);

/* Trim trailing whitespace in place */
char *str_rtrim(char *s);

/* Trim both ends */
char *str_trim(char *s);

/* Check if string starts with prefix */
int str_starts_with(const char *s, const char *prefix);

/* Check if string ends with suffix */
int str_ends_with(const char *s, const char *suffix);

/* Safe memory allocation - exits on failure */
void *mem_alloc(size_t size);

/* Safe reallocation - exits on failure */
void *mem_realloc(void *ptr, size_t size);

/* Free and NULL the pointer */
void mem_free(void *ptr);

/* Read entire file into buffer - caller must free */
char *file_read(const char *filename, size_t *out_size);

/* Check if file exists */
int file_exists(const char *filename);

/* Dynamic array (simple vector) */
typedef struct {
    void **items;
    size_t count;
    size_t capacity;
} DynArray;

void dynarray_init(DynArray *arr);
void dynarray_push(DynArray *arr, void *item);
void *dynarray_pop(DynArray *arr);
void *dynarray_get(DynArray *arr, size_t index);
void dynarray_free(DynArray *arr);

/* Simple hash table */
#define HASH_SIZE 1024

typedef struct HashEntry {
    char *key;
    void *value;
    struct HashEntry *next;
} HashEntry;

typedef struct {
    HashEntry *buckets[HASH_SIZE];
} HashTable;

void hash_init(HashTable *table);
void hash_set(HashTable *table, const char *key, void *value);
void *hash_get(HashTable *table, const char *key);
int hash_has(HashTable *table, const char *key);
void hash_remove(HashTable *table, const char *key);
void hash_free(HashTable *table, void (*free_value)(void *));

/* Hash function for strings */
uint32_t hash_string(const char *s);

#endif /* UTIL_H */
