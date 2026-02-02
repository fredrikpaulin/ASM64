/*
 * test_output.c - Output Generation Tests
 * ASM64 - 6502/6510 Assembler for Commodore 64
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "assembler.h"

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* ANSI color codes */
#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define RESET "\033[0m"

#define PASS() do { tests_passed++; printf(GREEN "PASS" RESET "\n"); } while(0)
#define FAIL(msg) do { tests_failed++; printf(RED "FAIL" RESET ": %s\n", msg); } while(0)

/* Helper to check file exists */
static int file_exists_test(const char *path) {
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

/* Helper to read file content */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (content) {
        fread(content, 1, size, f);
        content[size] = '\0';
    }
    fclose(f);
    return content;
}

/* Test PRG output generation */
static void test_prg_output(void) {
    printf("  %-40s ", "prg_output");

    Assembler *as = assembler_create();
    const char *source =
        "*=$C000\n"
        "lda #$01\n"
        "rts\n";

    assembler_assemble_string(as, source, "test.asm");

    /* Write output */
    assembler_write_output(as, "/tmp/test_prg_output.prg");

    /* Check file exists */
    if (!file_exists_test("/tmp/test_prg_output.prg")) {
        FAIL("output file not created");
        assembler_free(as);
        return;
    }

    /* Check file content */
    FILE *f = fopen("/tmp/test_prg_output.prg", "rb");
    if (!f) {
        FAIL("cannot open output file");
        assembler_free(as);
        return;
    }

    uint8_t header[2];
    fread(header, 1, 2, f);
    fclose(f);

    /* Check load address header */
    uint16_t load_addr = header[0] | (header[1] << 8);
    if (load_addr != 0xC000) {
        FAIL("wrong load address in header");
        assembler_free(as);
        return;
    }

    assembler_free(as);
    unlink("/tmp/test_prg_output.prg");
    PASS();
}

/* Test RAW output generation */
static void test_raw_output(void) {
    printf("  %-40s ", "raw_output");

    Assembler *as = assembler_create();
    as->format = OUTPUT_RAW;

    const char *source =
        "*=$C000\n"
        "lda #$01\n"
        "rts\n";

    assembler_assemble_string(as, source, "test.asm");
    assembler_write_output(as, "/tmp/test_raw_output.bin");

    /* Get file size */
    FILE *f = fopen("/tmp/test_raw_output.bin", "rb");
    if (!f) {
        FAIL("cannot open output file");
        assembler_free(as);
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);

    /* RAW format should not have header - just the code bytes */
    /* lda #$01 = A9 01 (2 bytes), rts = 60 (1 byte) = 3 bytes total */
    if (size != 3) {
        char msg[64];
        snprintf(msg, sizeof(msg), "wrong file size: expected 3, got %ld", size);
        FAIL(msg);
        assembler_free(as);
        return;
    }

    assembler_free(as);
    unlink("/tmp/test_raw_output.bin");
    PASS();
}

/* Test symbol file output */
static void test_symbol_output(void) {
    printf("  %-40s ", "symbol_output");

    Assembler *as = assembler_create();
    const char *source =
        "*=$C000\n"
        "start:\n"
        "loop:\n"
        "    jmp loop\n";

    assembler_assemble_string(as, source, "test.asm");
    assembler_write_symbols(as, "/tmp/test_symbols.sym");

    char *content = read_file("/tmp/test_symbols.sym");
    if (!content) {
        FAIL("cannot read symbol file");
        assembler_free(as);
        return;
    }

    /* Check that symbols are present */
    if (!strstr(content, ".start") || !strstr(content, ".loop")) {
        FAIL("missing symbols in output");
        free(content);
        assembler_free(as);
        return;
    }

    /* Check VICE format */
    if (!strstr(content, "al C:")) {
        FAIL("not in VICE format");
        free(content);
        assembler_free(as);
        return;
    }

    free(content);
    assembler_free(as);
    unlink("/tmp/test_symbols.sym");
    PASS();
}

/* Test listing file output */
static void test_listing_output(void) {
    printf("  %-40s ", "listing_output");

    Assembler *as = assembler_create();
    const char *source =
        "*=$C000\n"
        "start:\n"
        "    lda #$01\n"
        "    rts\n";

    assembler_assemble_string(as, source, "test.asm");
    assembler_write_listing(as, "/tmp/test_listing.lst");

    char *content = read_file("/tmp/test_listing.lst");
    if (!content) {
        FAIL("cannot read listing file");
        assembler_free(as);
        return;
    }

    /* Check header is present */
    if (!strstr(content, "ASM64 Listing File")) {
        FAIL("missing header");
        free(content);
        assembler_free(as);
        return;
    }

    /* Check hex bytes are present */
    if (!strstr(content, "A9 01")) {
        FAIL("missing hex bytes");
        free(content);
        assembler_free(as);
        return;
    }

    /* Check symbol table is included */
    if (!strstr(content, "Symbol Table")) {
        FAIL("missing symbol table");
        free(content);
        assembler_free(as);
        return;
    }

    free(content);
    assembler_free(as);
    unlink("/tmp/test_listing.lst");
    PASS();
}

/* Test listing with cycles */
static void test_listing_cycles(void) {
    printf("  %-40s ", "listing_cycles");

    Assembler *as = assembler_create();
    as->show_cycles = 1;

    const char *source =
        "*=$C000\n"
        "    lda #$01\n"
        "    sta $D020\n"
        "    rts\n";

    assembler_assemble_string(as, source, "test.asm");
    assembler_write_listing(as, "/tmp/test_cycles.lst");

    char *content = read_file("/tmp/test_cycles.lst");
    if (!content) {
        FAIL("cannot read listing file");
        assembler_free(as);
        return;
    }

    /* Check cycles header */
    if (!strstr(content, "Cycles")) {
        FAIL("missing cycles column header");
        free(content);
        assembler_free(as);
        return;
    }

    /* Check cycle counts are present (lda #$01 = 2 cycles) */
    if (!strstr(content, "2 ")) {
        FAIL("missing cycle count");
        free(content);
        assembler_free(as);
        return;
    }

    free(content);
    assembler_free(as);
    unlink("/tmp/test_cycles.lst");
    PASS();
}

/* Test get output function */
static void test_get_output(void) {
    printf("  %-40s ", "get_output");

    Assembler *as = assembler_create();
    const char *source =
        "*=$C000\n"
        "    nop\n"
        "    nop\n"
        "    rts\n";

    assembler_assemble_string(as, source, "test.asm");

    uint16_t start_addr;
    int size;
    const uint8_t *data = assembler_get_output(as, &start_addr, &size);

    if (start_addr != 0xC000) {
        FAIL("wrong start address");
        assembler_free(as);
        return;
    }

    if (size != 3) {
        char msg[64];
        snprintf(msg, sizeof(msg), "wrong size: expected 3, got %d", size);
        FAIL(msg);
        assembler_free(as);
        return;
    }

    /* NOP = EA, RTS = 60 */
    if (data[0] != 0xEA || data[1] != 0xEA || data[2] != 0x60) {
        FAIL("wrong output bytes");
        assembler_free(as);
        return;
    }

    assembler_free(as);
    PASS();
}

/* Test empty output */
static void test_empty_output(void) {
    printf("  %-40s ", "empty_output");

    Assembler *as = assembler_create();
    const char *source =
        "; Just a comment\n"
        "label = $1234\n";

    assembler_assemble_string(as, source, "test.asm");

    uint16_t start_addr;
    int size;
    assembler_get_output(as, &start_addr, &size);

    if (size != 0) {
        FAIL("expected no output");
        assembler_free(as);
        return;
    }

    assembler_free(as);
    PASS();
}

int main(void) {
    printf("Output Generation Tests\n");
    printf("=======================\n\n");

    printf("PRG/RAW Output:\n");
    test_prg_output();
    test_raw_output();

    printf("\nSymbol File:\n");
    test_symbol_output();

    printf("\nListing File:\n");
    test_listing_output();
    test_listing_cycles();

    printf("\nOutput Data:\n");
    test_get_output();
    test_empty_output();

    printf("\n=======================\n");
    printf("Total: %d passed, %d failed\n", tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
