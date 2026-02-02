#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "error.h"
#include "assembler.h"

#define VERSION "1.0.0"
#define MAX_DEFINES 64
#define MAX_INCLUDE_PATHS 16

/* Command line options */
typedef struct {
    char *input_file;
    char *output_file;
    char *listing_file;
    char *symbol_file;
    char *defines[MAX_DEFINES];
    int define_count;
    char *include_paths[MAX_INCLUDE_PATHS];
    int include_count;
    OutputFormat format;
    int verbose;
    int show_cycles;
} Options;

static Options g_options;

static void print_usage(const char *prog) {
    printf("Usage: %s [options] <source.asm>\n", prog);
    printf("\n");
    printf("Options:\n");
    printf("  -o <file>       Output filename (default: source.prg)\n");
    printf("  -f <format>     Output format: prg (default), raw\n");
    printf("  -l <file>       Generate listing file\n");
    printf("  -s <file>       Generate symbol file (VICE format)\n");
    printf("  -D NAME=value   Define symbol from command line\n");
    printf("  -I <path>       Add include search path\n");
    printf("  -v              Verbose output\n");
    printf("  --cycles        Include cycle counts in listing\n");
    printf("  --help          Show this help\n");
    printf("  --version       Show version\n");
}

static void print_version(void) {
    printf("asm64 version %s\n", VERSION);
    printf("6502/6510 Cross-Assembler for Commodore 64\n");
}

static char *make_output_filename(const char *input) {
    /* Replace extension with .prg or add it */
    size_t len = strlen(input);
    const char *dot = strrchr(input, '.');
    const char *slash = strrchr(input, '/');

    /* Make sure dot is after last slash (part of filename, not path) */
    if (dot && slash && dot < slash) {
        dot = NULL;
    }

    size_t base_len = dot ? (size_t)(dot - input) : len;
    char *output = mem_alloc(base_len + 5); /* .prg + null */
    memcpy(output, input, base_len);
    strcpy(output + base_len, ".prg");
    return output;
}

static int parse_args(int argc, char **argv) {
    /* Initialize defaults */
    memset(&g_options, 0, sizeof(g_options));
    g_options.format = OUTPUT_PRG;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        }
        if (strcmp(argv[i], "--version") == 0) {
            print_version();
            exit(0);
        }
        if (strcmp(argv[i], "--cycles") == 0) {
            g_options.show_cycles = 1;
            continue;
        }
        if (strcmp(argv[i], "-v") == 0) {
            g_options.verbose = 1;
            continue;
        }
        if (strcmp(argv[i], "-o") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "error: -o requires an argument\n");
                return 0;
            }
            g_options.output_file = argv[i];
            continue;
        }
        if (strcmp(argv[i], "-f") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "error: -f requires an argument\n");
                return 0;
            }
            if (strcmp(argv[i], "prg") == 0) {
                g_options.format = OUTPUT_PRG;
            } else if (strcmp(argv[i], "raw") == 0) {
                g_options.format = OUTPUT_RAW;
            } else {
                fprintf(stderr, "error: unknown format '%s'\n", argv[i]);
                return 0;
            }
            continue;
        }
        if (strcmp(argv[i], "-l") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "error: -l requires an argument\n");
                return 0;
            }
            g_options.listing_file = argv[i];
            continue;
        }
        if (strcmp(argv[i], "-s") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "error: -s requires an argument\n");
                return 0;
            }
            g_options.symbol_file = argv[i];
            continue;
        }
        /* Handle -D with or without space: -D NAME=value or -DNAME=value */
        if (strcmp(argv[i], "-D") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "error: -D requires an argument\n");
                return 0;
            }
            if (g_options.define_count >= MAX_DEFINES) {
                fprintf(stderr, "error: too many -D definitions\n");
                return 0;
            }
            g_options.defines[g_options.define_count++] = argv[i];
            continue;
        }
        if (strncmp(argv[i], "-D", 2) == 0 && argv[i][2] != '\0') {
            if (g_options.define_count >= MAX_DEFINES) {
                fprintf(stderr, "error: too many -D definitions\n");
                return 0;
            }
            g_options.defines[g_options.define_count++] = argv[i] + 2;
            continue;
        }
        /* Handle -I with or without space: -I path or -Ipath */
        if (strcmp(argv[i], "-I") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "error: -I requires an argument\n");
                return 0;
            }
            if (g_options.include_count >= MAX_INCLUDE_PATHS) {
                fprintf(stderr, "error: too many -I paths\n");
                return 0;
            }
            g_options.include_paths[g_options.include_count++] = argv[i];
            continue;
        }
        if (strncmp(argv[i], "-I", 2) == 0 && argv[i][2] != '\0') {
            if (g_options.include_count >= MAX_INCLUDE_PATHS) {
                fprintf(stderr, "error: too many -I paths\n");
                return 0;
            }
            g_options.include_paths[g_options.include_count++] = argv[i] + 2;
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
            return 0;
        }

        /* Must be input file */
        if (g_options.input_file) {
            fprintf(stderr, "error: multiple input files specified\n");
            return 0;
        }
        g_options.input_file = argv[i];
    }

    if (!g_options.input_file) {
        fprintf(stderr, "error: no input file specified\n");
        return 0;
    }

    /* Generate default output filename if not specified */
    if (!g_options.output_file) {
        g_options.output_file = make_output_filename(g_options.input_file);
    }

    return 1;
}

int main(int argc, char **argv) {
    error_init();

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (!parse_args(argc, argv)) {
        return 1;
    }

    if (g_options.verbose) {
        printf("asm64 %s\n", VERSION);
        printf("Input:  %s\n", g_options.input_file);
        printf("Output: %s\n", g_options.output_file);
        if (g_options.listing_file) {
            printf("Listing: %s\n", g_options.listing_file);
        }
        if (g_options.symbol_file) {
            printf("Symbols: %s\n", g_options.symbol_file);
        }
    }

    /* Check input file exists */
    if (!file_exists(g_options.input_file)) {
        fprintf(stderr, "error: cannot open '%s'\n", g_options.input_file);
        return 1;
    }

    /* Create assembler context */
    Assembler *as = assembler_create();
    if (!as) {
        fprintf(stderr, "error: failed to create assembler context\n");
        return 1;
    }

    /* Configure assembler options */
    as->format = (g_options.format == OUTPUT_PRG) ? OUTPUT_PRG : OUTPUT_RAW;
    as->verbose = g_options.verbose;
    as->show_cycles = g_options.show_cycles;

    /* Add include paths from environment variable first (lower priority) */
    assembler_add_include_paths_from_env(as, "ASM64_INCLUDE");

    /* Add include paths from command line (higher priority) */
    for (int i = 0; i < g_options.include_count; i++) {
        assembler_add_include_path(as, g_options.include_paths[i]);
    }

    /* Define command-line symbols */
    for (int i = 0; i < g_options.define_count; i++) {
        if (assembler_define_symbol(as, g_options.defines[i]) != 0) {
            fprintf(stderr, "error: invalid symbol definition '%s'\n", g_options.defines[i]);
            assembler_free(as);
            return 1;
        }
    }

    /* Run assembly */
    if (g_options.verbose) {
        printf("Assembling %s...\n", g_options.input_file);
    }

    int result = assembler_assemble_file(as, g_options.input_file);

    if (result == 0) {
        /* Write output file */
        const char *output = g_options.output_file;
        if (!output) {
            /* Generate default output filename */
            static char default_output[256];
            const char *ext = (g_options.format == OUTPUT_PRG) ? ".prg" : ".bin";
            const char *dot = strrchr(g_options.input_file, '.');
            if (dot) {
                int len = (int)(dot - g_options.input_file);
                snprintf(default_output, sizeof(default_output), "%.*s%s", len, g_options.input_file, ext);
            } else {
                snprintf(default_output, sizeof(default_output), "%s%s", g_options.input_file, ext);
            }
            output = default_output;
        }

        result = assembler_write_output(as, output);
        if (result == 0 && g_options.verbose) {
            uint16_t start_addr;
            int size;
            assembler_get_output(as, &start_addr, &size);
            printf("Output: %s (%d bytes, $%04X-$%04X)\n",
                   output, size + 2, start_addr, start_addr + size - 1);
        }

        /* Write symbol file if requested */
        if (g_options.symbol_file && result == 0) {
            result = assembler_write_symbols(as, g_options.symbol_file);
            if (result == 0 && g_options.verbose) {
                printf("Symbols: %s\n", g_options.symbol_file);
            }
        }

        /* Write listing file if requested */
        if (g_options.listing_file && result == 0) {
            result = assembler_write_listing(as, g_options.listing_file);
            if (result == 0 && g_options.verbose) {
                printf("Listing: %s\n", g_options.listing_file);
            }
        }
    }

    /* Report errors/warnings */
    if (as->errors > 0) {
        fprintf(stderr, "%d error%s\n", as->errors, as->errors == 1 ? "" : "s");
    }
    if (as->warnings > 0 && g_options.verbose) {
        fprintf(stderr, "%d warning%s\n", as->warnings, as->warnings == 1 ? "" : "s");
    }

    int exit_code = (as->errors > 0) ? 1 : 0;
    assembler_free(as);

    return exit_code;
}
