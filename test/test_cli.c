/*
 * test_cli.c - CLI Integration Tests
 * Tests command-line options and features
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../include/assembler.h"
#include "../include/opcodes.h"

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Colors for output */
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

/* Test result macros */
#define PASS() do { \
    printf(GREEN "PASS" RESET "\n"); \
    tests_passed++; \
} while(0)

#define FAIL(msg) do { \
    printf(RED "FAIL" RESET " - %s\n", msg); \
    tests_failed++; \
} while(0)

#define TEST(name) printf("  %-40s ", name)

/* Helper: create temp file with content */
static char *create_temp_file(const char *content, const char *suffix) {
    static char path[256];
    snprintf(path, sizeof(path), "/tmp/test_cli_%d%s", getpid(), suffix);
    FILE *f = fopen(path, "w");
    if (f) {
        fputs(content, f);
        fclose(f);
    }
    return path;
}

/* Helper: read file contents */
static uint8_t *read_file_bytes(const char *path, int *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    *size = (int)ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = malloc(*size);
    if (data) {
        *size = (int)fread(data, 1, *size, f);
    }
    fclose(f);
    return data;
}

/* Helper: check if file exists */
static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

/* ========== Define Tests ========== */

void test_define_simple(void) {
    TEST("define_simple");

    const char *source =
        "*=$C000\n"
        "!ifdef DEBUG\n"
        "    lda #$01\n"
        "!else\n"
        "    lda #$00\n"
        "!endif\n"
        "    rts\n";

    char *srcfile = create_temp_file(source, ".asm");

    Assembler *as = assembler_create();
    assembler_define_symbol(as, "DEBUG");

    int result = assembler_assemble_file(as, srcfile);

    if (result != 0) {
        FAIL("assembly failed");
    } else {
        uint16_t start;
        int size;
        const uint8_t *output = assembler_get_output(as, &start, &size);

        /* Should have: A9 01 60 (LDA #$01, RTS) */
        if (size >= 3 && output[0] == 0xA9 && output[1] == 0x01 && output[2] == 0x60) {
            PASS();
        } else {
            FAIL("wrong output bytes");
        }
    }

    assembler_free(as);
    unlink(srcfile);
}

void test_define_value(void) {
    TEST("define_value");

    const char *source =
        "*=$C000\n"
        "    lda #VALUE\n"
        "    rts\n";

    char *srcfile = create_temp_file(source, ".asm");

    Assembler *as = assembler_create();
    assembler_define_symbol(as, "VALUE=42");

    int result = assembler_assemble_file(as, srcfile);

    if (result != 0) {
        FAIL("assembly failed");
    } else {
        uint16_t start;
        int size;
        const uint8_t *output = assembler_get_output(as, &start, &size);

        /* Should have: A9 2A 60 (LDA #42, RTS) */
        if (size >= 3 && output[0] == 0xA9 && output[1] == 42 && output[2] == 0x60) {
            PASS();
        } else {
            FAIL("wrong output bytes");
        }
    }

    assembler_free(as);
    unlink(srcfile);
}

void test_define_hex_value(void) {
    TEST("define_hex_value");

    const char *source =
        "*=$C000\n"
        "    lda #VALUE\n"
        "    rts\n";

    char *srcfile = create_temp_file(source, ".asm");

    Assembler *as = assembler_create();
    assembler_define_symbol(as, "VALUE=$FF");

    int result = assembler_assemble_file(as, srcfile);

    if (result != 0) {
        FAIL("assembly failed");
    } else {
        uint16_t start;
        int size;
        const uint8_t *output = assembler_get_output(as, &start, &size);

        /* Should have: A9 FF 60 (LDA #$FF, RTS) */
        if (size >= 3 && output[0] == 0xA9 && output[1] == 0xFF && output[2] == 0x60) {
            PASS();
        } else {
            FAIL("wrong output bytes");
        }
    }

    assembler_free(as);
    unlink(srcfile);
}

void test_define_binary_value(void) {
    TEST("define_binary_value");

    const char *source =
        "*=$C000\n"
        "    lda #VALUE\n"
        "    rts\n";

    char *srcfile = create_temp_file(source, ".asm");

    Assembler *as = assembler_create();
    assembler_define_symbol(as, "VALUE=%10101010");

    int result = assembler_assemble_file(as, srcfile);

    if (result != 0) {
        FAIL("assembly failed");
    } else {
        uint16_t start;
        int size;
        const uint8_t *output = assembler_get_output(as, &start, &size);

        /* Should have: A9 AA 60 (LDA #$AA, RTS) - binary 10101010 = $AA */
        if (size >= 3 && output[0] == 0xA9 && output[1] == 0xAA && output[2] == 0x60) {
            PASS();
        } else {
            FAIL("wrong output bytes");
        }
    }

    assembler_free(as);
    unlink(srcfile);
}

void test_define_multiple(void) {
    TEST("define_multiple");

    const char *source =
        "*=$C000\n"
        "    lda #VAL1\n"
        "    ldx #VAL2\n"
        "    ldy #VAL3\n"
        "    rts\n";

    char *srcfile = create_temp_file(source, ".asm");

    Assembler *as = assembler_create();
    assembler_define_symbol(as, "VAL1=1");
    assembler_define_symbol(as, "VAL2=2");
    assembler_define_symbol(as, "VAL3=3");

    int result = assembler_assemble_file(as, srcfile);

    if (result != 0) {
        FAIL("assembly failed");
    } else {
        uint16_t start;
        int size;
        const uint8_t *output = assembler_get_output(as, &start, &size);

        /* Should have: A9 01 A2 02 A0 03 60 */
        if (size >= 7 &&
            output[0] == 0xA9 && output[1] == 0x01 &&
            output[2] == 0xA2 && output[3] == 0x02 &&
            output[4] == 0xA0 && output[5] == 0x03 &&
            output[6] == 0x60) {
            PASS();
        } else {
            FAIL("wrong output bytes");
        }
    }

    assembler_free(as);
    unlink(srcfile);
}

void test_define_ifndef(void) {
    TEST("define_ifndef");

    const char *source =
        "*=$C000\n"
        "!ifndef DEBUG\n"
        "    lda #$00\n"
        "!else\n"
        "    lda #$01\n"
        "!endif\n"
        "    rts\n";

    char *srcfile = create_temp_file(source, ".asm");

    /* Test 1: DEBUG not defined - should use #$00 */
    Assembler *as1 = assembler_create();
    int result1 = assembler_assemble_file(as1, srcfile);

    uint16_t start1;
    int size1;
    const uint8_t *output1 = assembler_get_output(as1, &start1, &size1);
    int test1_ok = (result1 == 0 && size1 >= 3 && output1[1] == 0x00);

    assembler_free(as1);

    /* Test 2: DEBUG defined - should use #$01 */
    Assembler *as2 = assembler_create();
    assembler_define_symbol(as2, "DEBUG");
    int result2 = assembler_assemble_file(as2, srcfile);

    uint16_t start2;
    int size2;
    const uint8_t *output2 = assembler_get_output(as2, &start2, &size2);
    int test2_ok = (result2 == 0 && size2 >= 3 && output2[1] == 0x01);

    assembler_free(as2);

    if (test1_ok && test2_ok) {
        PASS();
    } else {
        FAIL("conditional failed");
    }

    unlink(srcfile);
}

/* ========== Include Path Tests ========== */

void test_include_path(void) {
    TEST("include_path");

    /* Create include directory and file */
    char incdir[256];
    snprintf(incdir, sizeof(incdir), "/tmp/test_inc_%d", getpid());
    mkdir(incdir, 0755);

    char incfile[256];
    snprintf(incfile, sizeof(incfile), "%s/values.inc", incdir);
    FILE *f = fopen(incfile, "w");
    if (f) {
        fprintf(f, "VALUE = $42\n");
        fclose(f);
    }

    const char *source =
        "*=$C000\n"
        "!source \"values.inc\"\n"
        "    lda #VALUE\n"
        "    rts\n";

    char *srcfile = create_temp_file(source, ".asm");

    Assembler *as = assembler_create();
    assembler_add_include_path(as, incdir);

    int result = assembler_assemble_file(as, srcfile);

    if (result != 0) {
        FAIL("assembly failed");
    } else {
        uint16_t start;
        int size;
        const uint8_t *output = assembler_get_output(as, &start, &size);

        /* Should have: A9 42 60 (LDA #$42, RTS) */
        if (size >= 3 && output[0] == 0xA9 && output[1] == 0x42 && output[2] == 0x60) {
            PASS();
        } else {
            FAIL("wrong output bytes");
        }
    }

    assembler_free(as);
    unlink(srcfile);
    unlink(incfile);
    rmdir(incdir);
}

void test_multiple_include_paths(void) {
    TEST("multiple_include_paths");

    /* Create two include directories */
    char incdir1[256], incdir2[256];
    snprintf(incdir1, sizeof(incdir1), "/tmp/test_inc1_%d", getpid());
    snprintf(incdir2, sizeof(incdir2), "/tmp/test_inc2_%d", getpid());
    mkdir(incdir1, 0755);
    mkdir(incdir2, 0755);

    char incfile1[256], incfile2[256];
    snprintf(incfile1, sizeof(incfile1), "%s/val1.inc", incdir1);
    snprintf(incfile2, sizeof(incfile2), "%s/val2.inc", incdir2);

    FILE *f1 = fopen(incfile1, "w");
    if (f1) { fprintf(f1, "VAL1 = $11\n"); fclose(f1); }

    FILE *f2 = fopen(incfile2, "w");
    if (f2) { fprintf(f2, "VAL2 = $22\n"); fclose(f2); }

    const char *source =
        "*=$C000\n"
        "!source \"val1.inc\"\n"
        "!source \"val2.inc\"\n"
        "    lda #VAL1\n"
        "    ldx #VAL2\n"
        "    rts\n";

    char *srcfile = create_temp_file(source, ".asm");

    Assembler *as = assembler_create();
    assembler_add_include_path(as, incdir1);
    assembler_add_include_path(as, incdir2);

    int result = assembler_assemble_file(as, srcfile);

    if (result != 0) {
        FAIL("assembly failed");
    } else {
        uint16_t start;
        int size;
        const uint8_t *output = assembler_get_output(as, &start, &size);

        /* Should have: A9 11 A2 22 60 */
        if (size >= 5 &&
            output[0] == 0xA9 && output[1] == 0x11 &&
            output[2] == 0xA2 && output[3] == 0x22 &&
            output[4] == 0x60) {
            PASS();
        } else {
            FAIL("wrong output bytes");
        }
    }

    assembler_free(as);
    unlink(srcfile);
    unlink(incfile1);
    unlink(incfile2);
    rmdir(incdir1);
    rmdir(incdir2);
}

/* ========== Output Tests ========== */

void test_prg_format(void) {
    TEST("prg_format");

    const char *source =
        "*=$C000\n"
        "    lda #$00\n"
        "    rts\n";

    char *srcfile = create_temp_file(source, ".asm");
    char outfile[256];
    snprintf(outfile, sizeof(outfile), "/tmp/test_prg_%d.prg", getpid());

    Assembler *as = assembler_create();
    as->format = OUTPUT_PRG;

    int result = assembler_assemble_file(as, srcfile);
    if (result == 0) {
        result = assembler_write_output(as, outfile);
    }

    if (result != 0) {
        FAIL("assembly/output failed");
    } else {
        int size;
        uint8_t *data = read_file_bytes(outfile, &size);

        /* PRG format: 2-byte header ($00 $C0) + code (A9 00 60) */
        if (data && size == 5 &&
            data[0] == 0x00 && data[1] == 0xC0 &&  /* Load address */
            data[2] == 0xA9 && data[3] == 0x00 && data[4] == 0x60) {
            PASS();
        } else {
            FAIL("wrong PRG format");
        }
        free(data);
    }

    assembler_free(as);
    unlink(srcfile);
    unlink(outfile);
}

void test_raw_format(void) {
    TEST("raw_format");

    const char *source =
        "*=$C000\n"
        "    lda #$00\n"
        "    rts\n";

    char *srcfile = create_temp_file(source, ".asm");
    char outfile[256];
    snprintf(outfile, sizeof(outfile), "/tmp/test_raw_%d.bin", getpid());

    Assembler *as = assembler_create();
    as->format = OUTPUT_RAW;

    int result = assembler_assemble_file(as, srcfile);
    if (result == 0) {
        result = assembler_write_output(as, outfile);
    }

    if (result != 0) {
        FAIL("assembly/output failed");
    } else {
        int size;
        uint8_t *data = read_file_bytes(outfile, &size);

        /* RAW format: no header, just code (A9 00 60) */
        if (data && size == 3 &&
            data[0] == 0xA9 && data[1] == 0x00 && data[2] == 0x60) {
            PASS();
        } else {
            FAIL("wrong RAW format");
        }
        free(data);
    }

    assembler_free(as);
    unlink(srcfile);
    unlink(outfile);
}

void test_symbol_file_output(void) {
    TEST("symbol_file_output");

    const char *source =
        "*=$C000\n"
        "START:\n"
        "    lda #$00\n"
        "LOOP:\n"
        "    jmp LOOP\n";

    char *srcfile = create_temp_file(source, ".asm");
    char symfile[256];
    snprintf(symfile, sizeof(symfile), "/tmp/test_sym_%d.sym", getpid());

    Assembler *as = assembler_create();

    int result = assembler_assemble_file(as, srcfile);
    if (result == 0) {
        result = assembler_write_symbols(as, symfile);
    }

    if (result != 0) {
        FAIL("assembly/symbol output failed");
    } else if (!file_exists(symfile)) {
        FAIL("symbol file not created");
    } else {
        /* Read and check symbol file content */
        FILE *f = fopen(symfile, "r");
        char line[256];
        int found_start = 0, found_loop = 0;

        while (f && fgets(line, sizeof(line), f)) {
            if (strstr(line, "C000") && strstr(line, "START")) found_start = 1;
            if (strstr(line, "C002") && strstr(line, "LOOP")) found_loop = 1;
        }
        if (f) fclose(f);

        if (found_start && found_loop) {
            PASS();
        } else {
            FAIL("missing symbols in file");
        }
    }

    assembler_free(as);
    unlink(srcfile);
    unlink(symfile);
}

void test_listing_file_output(void) {
    TEST("listing_file_output");

    const char *source =
        "*=$C000\n"
        "    lda #$00\n"
        "    rts\n";

    char *srcfile = create_temp_file(source, ".asm");
    char lstfile[256];
    snprintf(lstfile, sizeof(lstfile), "/tmp/test_lst_%d.lst", getpid());

    Assembler *as = assembler_create();

    int result = assembler_assemble_file(as, srcfile);
    if (result == 0) {
        result = assembler_write_listing(as, lstfile);
    }

    if (result != 0) {
        FAIL("assembly/listing output failed");
    } else if (!file_exists(lstfile)) {
        FAIL("listing file not created");
    } else {
        /* Read and check listing file has addresses and hex bytes */
        FILE *f = fopen(lstfile, "r");
        char line[512];
        int found_addr = 0, found_hex = 0;

        while (f && fgets(line, sizeof(line), f)) {
            if (strstr(line, "C000")) found_addr = 1;
            if (strstr(line, "A9") || strstr(line, "a9")) found_hex = 1;
        }
        if (f) fclose(f);

        if (found_addr && found_hex) {
            PASS();
        } else {
            FAIL("listing missing address/hex");
        }
    }

    assembler_free(as);
    unlink(srcfile);
    unlink(lstfile);
}

/* ========== Main ========== */

int main(void) {
    printf("\nCLI Integration Tests\n");
    printf("=====================\n\n");

    opcodes_init();

    printf("Define Options:\n");
    test_define_simple();
    test_define_value();
    test_define_hex_value();
    test_define_binary_value();
    test_define_multiple();
    test_define_ifndef();

    printf("\nInclude Paths:\n");
    test_include_path();
    test_multiple_include_paths();

    printf("\nOutput Formats:\n");
    test_prg_format();
    test_raw_format();
    test_symbol_file_output();
    test_listing_file_output();

    printf("\n=====================\n");
    printf("Total: %d passed, %d failed\n", tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
