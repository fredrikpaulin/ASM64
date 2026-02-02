#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

/* Error context - tracks current file and line for error messages */
typedef struct {
    const char *filename;
    int line;
    int column;
} ErrorContext;

/* Initialize error system */
void error_init(void);

/* Set current error context (file/line being processed) */
void error_set_context(const char *filename, int line, int column);

/* Get current context */
ErrorContext *error_get_context(void);

/* Report an error - assembly will fail */
void error(const char *fmt, ...);

/* Report an error with specific location */
void error_at(const char *filename, int line, const char *fmt, ...);

/* Report a warning - assembly continues */
void warning(const char *fmt, ...);

/* Report a warning with specific location */
void warning_at(const char *filename, int line, const char *fmt, ...);

/* Report a fatal error and exit immediately */
void fatal(const char *fmt, ...);

/* Get error count */
int error_count(void);

/* Get warning count */
int warning_count(void);

/* Reset error/warning counts */
void error_reset(void);

/* Check if errors occurred */
int has_errors(void);

#endif /* ERROR_H */
