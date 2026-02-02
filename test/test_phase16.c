/* Test suite for Phase 16: Advanced Features */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/assembler.h"
#include "../include/opcodes.h"
#include "../include/symbols.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) static int test_##name(void)
#define RUN_TEST(name) do { \
    tests_run++; \
    printf("  %-40s ", #name); \
    if (test_##name()) { \
        printf("\033[0;32mPASS\033[0m\n"); \
        tests_passed++; \
    } else { \
        printf("\033[0;31mFAIL\033[0m\n"); \
    } \
} while(0)

/* ========== Pseudopc Tests ========== */

TEST(pseudopc_basic) {
    Assembler *as = assembler_create();

    const char *src =
        "*=$1000\n"
        "    lda #$01\n"
        "!pseudopc $C000\n"
        "    lda #$02\n"
        "    nop\n"
        "!realpc\n"
        "    lda #$03\n"
        "    rts\n";

    int result = assembler_assemble_string(as, src, "test.asm");
    if (result != 0 || as->errors > 0) {
        assembler_free(as);
        return 0;
    }

    /* Check that 8 bytes were generated */
    int size = as->highest_addr - as->lowest_addr + 1;
    if (size != 8) {
        printf("(expected 8 bytes, got %d) ", size);
        assembler_free(as);
        return 0;
    }

    /* Check bytes are in correct positions */
    /* $1000: A9 01 (lda #$01) */
    if (as->memory[0x1000] != 0xA9 || as->memory[0x1001] != 0x01) {
        printf("(wrong bytes at $1000) ");
        assembler_free(as);
        return 0;
    }

    /* $1002: A9 02 EA (pseudopc block - lda #$02, nop) */
    if (as->memory[0x1002] != 0xA9 || as->memory[0x1003] != 0x02 || as->memory[0x1004] != 0xEA) {
        printf("(wrong bytes at $1002) ");
        assembler_free(as);
        return 0;
    }

    /* $1005: A9 03 60 (lda #$03, rts) */
    if (as->memory[0x1005] != 0xA9 || as->memory[0x1006] != 0x03 || as->memory[0x1007] != 0x60) {
        printf("(wrong bytes at $1005) ");
        assembler_free(as);
        return 0;
    }

    assembler_free(as);
    return 1;
}

TEST(pseudopc_labels) {
    Assembler *as = assembler_create();

    const char *src =
        "*=$1000\n"
        "start:\n"
        "!pseudopc $C000\n"
        "pseudo_start:\n"
        "    nop\n"
        "pseudo_end:\n"
        "!realpc\n"
        "real_end:\n"
        "    rts\n";

    int result = assembler_assemble_string(as, src, "test.asm");
    if (result != 0 || as->errors > 0) {
        assembler_free(as);
        return 0;
    }

    /* Check labels have correct values */
    Symbol *sym;

    /* start should be $1000 */
    sym = symbol_lookup(as->symbols, "start");
    if (!sym || sym->value != 0x1000) {
        printf("(start != $1000) ");
        assembler_free(as);
        return 0;
    }

    /* pseudo_start should be $C000 (virtual address) */
    sym = symbol_lookup(as->symbols, "pseudo_start");
    if (!sym || sym->value != 0xC000) {
        printf("(pseudo_start != $C000) ");
        assembler_free(as);
        return 0;
    }

    /* pseudo_end should be $C001 (after nop) */
    sym = symbol_lookup(as->symbols, "pseudo_end");
    if (!sym || sym->value != 0xC001) {
        printf("(pseudo_end != $C001) ");
        assembler_free(as);
        return 0;
    }

    /* real_end should be $1001 (real address after pseudopc block) */
    sym = symbol_lookup(as->symbols, "real_end");
    if (!sym || sym->value != 0x1001) {
        printf("(real_end != $1001, got $%04X) ", sym ? sym->value : 0);
        assembler_free(as);
        return 0;
    }

    assembler_free(as);
    return 1;
}

TEST(pseudopc_branch) {
    Assembler *as = assembler_create();

    const char *src =
        "*=$1000\n"
        "!pseudopc $C000\n"
        "loop:\n"
        "    nop\n"
        "    bne loop\n"   /* Should branch to $C000 using relative offset */
        "!realpc\n"
        "    rts\n";

    int result = assembler_assemble_string(as, src, "test.asm");
    if (result != 0 || as->errors > 0) {
        assembler_free(as);
        return 0;
    }

    /* Check branch offset is correct */
    /* nop at $C000 = EA */
    /* bne loop at $C001 = D0 FD (-3 to loop) */
    if (as->memory[0x1000] != 0xEA) {
        printf("(expected EA at $1000) ");
        assembler_free(as);
        return 0;
    }

    if (as->memory[0x1001] != 0xD0 || as->memory[0x1002] != 0xFD) {
        printf("(wrong branch at $1001: %02X %02X) ", as->memory[0x1001], as->memory[0x1002]);
        assembler_free(as);
        return 0;
    }

    assembler_free(as);
    return 1;
}

TEST(pseudopc_nested_error) {
    Assembler *as = assembler_create();

    const char *src =
        "*=$1000\n"
        "!pseudopc $C000\n"
        "!pseudopc $D000\n"  /* Should error - nested pseudopc */
        "    nop\n"
        "!realpc\n"
        "!realpc\n";

    (void)assembler_assemble_string(as, src, "test.asm");

    /* Should have error */
    int passed = (as->errors > 0);

    assembler_free(as);
    return passed;
}

TEST(realpc_without_pseudopc_error) {
    Assembler *as = assembler_create();

    const char *src =
        "*=$1000\n"
        "!realpc\n"  /* Should error - no matching pseudopc */
        "    nop\n";

    (void)assembler_assemble_string(as, src, "test.asm");

    /* Should have error */
    int passed = (as->errors > 0);

    assembler_free(as);
    return passed;
}

/* ========== CPU Selection Tests ========== */

TEST(cpu_6502) {
    Assembler *as = assembler_create();

    const char *src =
        "*=$1000\n"
        "!cpu 6502\n"
        "    nop\n"
        "    lda #$00\n"
        "    rts\n";

    int result = assembler_assemble_string(as, src, "test.asm");

    int passed = (result == 0 && as->errors == 0 && as->cpu_type == CPU_6502);

    assembler_free(as);
    return passed;
}

TEST(cpu_6510) {
    Assembler *as = assembler_create();

    const char *src =
        "*=$1000\n"
        "!cpu 6510\n"
        "    nop\n"
        "    rts\n";

    int result = assembler_assemble_string(as, src, "test.asm");

    int passed = (result == 0 && as->errors == 0 && as->cpu_type == CPU_6510);

    assembler_free(as);
    return passed;
}

TEST(cpu_65c02) {
    Assembler *as = assembler_create();

    const char *src =
        "*=$1000\n"
        "!cpu \"65c02\"\n"  /* Test string form */
        "    nop\n"
        "    rts\n";

    int result = assembler_assemble_string(as, src, "test.asm");

    int passed = (result == 0 && as->errors == 0 && as->cpu_type == CPU_65C02);

    assembler_free(as);
    return passed;
}

TEST(cpu_invalid) {
    Assembler *as = assembler_create();

    const char *src =
        "*=$1000\n"
        "!cpu \"8086\"\n"  /* Invalid CPU type */
        "    nop\n";

    (void)assembler_assemble_string(as, src, "test.asm");

    /* Should have error for invalid CPU */
    int passed = (as->errors > 0);

    assembler_free(as);
    return passed;
}

/* ========== Error/Warn Directive Tests ========== */

TEST(error_directive) {
    Assembler *as = assembler_create();

    const char *src =
        "*=$1000\n"
        "!error \"Test error message\"\n"
        "    nop\n";

    (void)assembler_assemble_string(as, src, "test.asm");

    /* Should have error */
    int passed = (as->errors > 0);

    assembler_free(as);
    return passed;
}

TEST(warn_directive) {
    Assembler *as = assembler_create();

    const char *src =
        "*=$1000\n"
        "!warn \"Test warning message\"\n"
        "    nop\n"
        "    rts\n";

    int result = assembler_assemble_string(as, src, "test.asm");

    /* Should have warning but no error */
    int passed = (result == 0 && as->warnings > 0 && as->errors == 0);

    assembler_free(as);
    return passed;
}

/* ========== Main ========== */

int main(void) {
    opcodes_init();

    printf("\nPhase 16: Advanced Features Tests\n");
    printf("==================================\n\n");

    printf("Pseudopc Tests:\n");
    RUN_TEST(pseudopc_basic);
    RUN_TEST(pseudopc_labels);
    RUN_TEST(pseudopc_branch);
    RUN_TEST(pseudopc_nested_error);
    RUN_TEST(realpc_without_pseudopc_error);

    printf("\nCPU Selection Tests:\n");
    RUN_TEST(cpu_6502);
    RUN_TEST(cpu_6510);
    RUN_TEST(cpu_65c02);
    RUN_TEST(cpu_invalid);

    printf("\nError/Warn Tests:\n");
    RUN_TEST(error_directive);
    RUN_TEST(warn_directive);

    printf("\n==================================\n");
    printf("Total: %d passed, %d failed\n", tests_passed, tests_run - tests_passed);

    return tests_run == tests_passed ? 0 : 1;
}
