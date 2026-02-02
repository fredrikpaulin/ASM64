#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *str_dup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *copy = mem_alloc(len + 1);
    memcpy(copy, s, len + 1);
    return copy;
}

char *str_ndup(const char *s, size_t n) {
    if (!s) return NULL;
    size_t len = strlen(s);
    if (n < len) len = n;
    char *copy = mem_alloc(len + 1);
    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}

char *str_tolower(char *s) {
    if (!s) return NULL;
    for (char *p = s; *p; p++) {
        *p = (char)tolower((unsigned char)*p);
    }
    return s;
}

char *str_toupper(char *s) {
    if (!s) return NULL;
    for (char *p = s; *p; p++) {
        *p = (char)toupper((unsigned char)*p);
    }
    return s;
}

char *str_ltrim(char *s) {
    if (!s) return NULL;
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

char *str_rtrim(char *s) {
    if (!s) return NULL;
    char *end = s + strlen(s) - 1;
    while (end >= s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return s;
}

char *str_trim(char *s) {
    return str_rtrim(str_ltrim(s));
}

int str_starts_with(const char *s, const char *prefix) {
    if (!s || !prefix) return 0;
    size_t prefix_len = strlen(prefix);
    return strncmp(s, prefix, prefix_len) == 0;
}

int str_ends_with(const char *s, const char *suffix) {
    if (!s || !suffix) return 0;
    size_t s_len = strlen(s);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > s_len) return 0;
    return strcmp(s + s_len - suffix_len, suffix) == 0;
}

void *mem_alloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr && size > 0) {
        fprintf(stderr, "asm64: fatal: out of memory\n");
        exit(1);
    }
    return ptr;
}

void *mem_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        fprintf(stderr, "asm64: fatal: out of memory\n");
        exit(1);
    }
    return new_ptr;
}

void mem_free(void *ptr) {
    free(ptr);
}

char *file_read(const char *filename, size_t *out_size) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) {
        fclose(f);
        return NULL;
    }

    char *buf = mem_alloc((size_t)size + 1);
    size_t read = fread(buf, 1, (size_t)size, f);
    fclose(f);

    buf[read] = '\0';
    if (out_size) *out_size = read;
    return buf;
}

int file_exists(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

/* Dynamic array implementation */

void dynarray_init(DynArray *arr) {
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

void dynarray_push(DynArray *arr, void *item) {
    if (arr->count >= arr->capacity) {
        size_t new_cap = arr->capacity == 0 ? 16 : arr->capacity * 2;
        arr->items = mem_realloc(arr->items, new_cap * sizeof(void *));
        arr->capacity = new_cap;
    }
    arr->items[arr->count++] = item;
}

void *dynarray_pop(DynArray *arr) {
    if (arr->count == 0) return NULL;
    return arr->items[--arr->count];
}

void *dynarray_get(DynArray *arr, size_t index) {
    if (index >= arr->count) return NULL;
    return arr->items[index];
}

void dynarray_free(DynArray *arr) {
    free(arr->items);
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

/* Hash table implementation using djb2 hash */

uint32_t hash_string(const char *s) {
    uint32_t hash = 5381;
    int c;
    while ((c = (unsigned char)*s++)) {
        hash = ((hash << 5) + hash) + (uint32_t)c;
    }
    return hash;
}

void hash_init(HashTable *table) {
    memset(table->buckets, 0, sizeof(table->buckets));
}

void hash_set(HashTable *table, const char *key, void *value) {
    uint32_t idx = hash_string(key) % HASH_SIZE;
    HashEntry *entry = table->buckets[idx];

    /* Check if key exists */
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            return;
        }
        entry = entry->next;
    }

    /* Add new entry */
    entry = mem_alloc(sizeof(HashEntry));
    entry->key = str_dup(key);
    entry->value = value;
    entry->next = table->buckets[idx];
    table->buckets[idx] = entry;
}

void *hash_get(HashTable *table, const char *key) {
    uint32_t idx = hash_string(key) % HASH_SIZE;
    HashEntry *entry = table->buckets[idx];

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

int hash_has(HashTable *table, const char *key) {
    uint32_t idx = hash_string(key) % HASH_SIZE;
    HashEntry *entry = table->buckets[idx];

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return 1;
        }
        entry = entry->next;
    }
    return 0;
}

void hash_remove(HashTable *table, const char *key) {
    uint32_t idx = hash_string(key) % HASH_SIZE;
    HashEntry *entry = table->buckets[idx];
    HashEntry *prev = NULL;

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                table->buckets[idx] = entry->next;
            }
            free(entry->key);
            free(entry);
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

void hash_free(HashTable *table, void (*free_value)(void *)) {
    for (int i = 0; i < HASH_SIZE; i++) {
        HashEntry *entry = table->buckets[i];
        while (entry) {
            HashEntry *next = entry->next;
            free(entry->key);
            if (free_value && entry->value) {
                free_value(entry->value);
            }
            free(entry);
            entry = next;
        }
        table->buckets[i] = NULL;
    }
}
