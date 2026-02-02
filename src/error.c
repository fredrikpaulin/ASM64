#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ErrorContext g_context = { NULL, 0, 0 };
static int g_error_count = 0;
static int g_warning_count = 0;

void error_init(void) {
    g_context.filename = NULL;
    g_context.line = 0;
    g_context.column = 0;
    g_error_count = 0;
    g_warning_count = 0;
}

void error_set_context(const char *filename, int line, int column) {
    g_context.filename = filename;
    g_context.line = line;
    g_context.column = column;
}

ErrorContext *error_get_context(void) {
    return &g_context;
}

static void print_location(const char *filename, int line) {
    if (filename) {
        fprintf(stderr, "%s:", filename);
        if (line > 0) {
            fprintf(stderr, "%d:", line);
        }
        fprintf(stderr, " ");
    }
}

void error(const char *fmt, ...) {
    va_list args;

    print_location(g_context.filename, g_context.line);
    fprintf(stderr, "error: ");

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    g_error_count++;
}

void error_at(const char *filename, int line, const char *fmt, ...) {
    va_list args;

    print_location(filename, line);
    fprintf(stderr, "error: ");

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    g_error_count++;
}

void warning(const char *fmt, ...) {
    va_list args;

    print_location(g_context.filename, g_context.line);
    fprintf(stderr, "warning: ");

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    g_warning_count++;
}

void warning_at(const char *filename, int line, const char *fmt, ...) {
    va_list args;

    print_location(filename, line);
    fprintf(stderr, "warning: ");

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    g_warning_count++;
}

void fatal(const char *fmt, ...) {
    va_list args;

    fprintf(stderr, "asm64: fatal: ");

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    exit(1);
}

int error_count(void) {
    return g_error_count;
}

int warning_count(void) {
    return g_warning_count;
}

void error_reset(void) {
    g_error_count = 0;
    g_warning_count = 0;
}

int has_errors(void) {
    return g_error_count > 0;
}
