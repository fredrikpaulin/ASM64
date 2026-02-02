#!/bin/bash
# ASM64 Test Runner
# Run from the test/ directory or project root

# Don't exit on error - we handle errors ourselves

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Find project root and assembler
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Allow passing assembler path as argument
if [ -n "$1" ]; then
    ASM64="$1"
else
    ASM64="$PROJECT_ROOT/build/asm64"
fi

# Counters
PASSED=0
FAILED=0
SKIPPED=0

# Check if assembler exists
if [ ! -x "$ASM64" ]; then
    echo -e "${RED}Error: asm64 not found at $ASM64${NC}"
    echo "Run 'make' first to build the assembler."
    exit 1
fi

echo "ASM64 Test Suite"
echo "================"
echo ""

# Function to run a single test
run_test() {
    local test_file="$1"
    local test_name="$(basename "$test_file" .asm)"
    local expected_file="$SCRIPT_DIR/expected/${test_name}.prg"
    local output_file="/tmp/asm64_test_${test_name}.prg"

    printf "  %-30s " "$test_name"

    # Run assembler
    local asm_output
    asm_output=$("$ASM64" -o "$output_file" "$test_file" 2>&1)
    local asm_status=$?

    # Check if assembler is not yet implemented
    if echo "$asm_output" | grep -q "not yet implemented"; then
        echo -e "${YELLOW}SKIP${NC} (assembler not implemented)"
        ((SKIPPED++))
        return
    fi

    if [ $asm_status -ne 0 ]; then
        # Check if this is an expected failure test
        if [ -f "$SCRIPT_DIR/expected/${test_name}.error" ]; then
            echo -e "${GREEN}PASS${NC} (expected error)"
            ((PASSED++))
        else
            echo -e "${RED}FAIL${NC} (assembly error)"
            ((FAILED++))
        fi
        return
    fi

    # Compare output if expected file exists
    if [ -f "$expected_file" ]; then
        if cmp -s "$output_file" "$expected_file"; then
            echo -e "${GREEN}PASS${NC}"
            ((PASSED++))
        else
            echo -e "${RED}FAIL${NC} (output mismatch)"
            ((FAILED++))
            # Show diff in hex
            if command -v xxd &> /dev/null; then
                echo "    Expected:"
                xxd "$expected_file" | head -5 | sed 's/^/      /'
                echo "    Got:"
                xxd "$output_file" | head -5 | sed 's/^/      /'
            fi
        fi
    else
        echo -e "${YELLOW}SKIP${NC} (no expected output)"
        ((SKIPPED++))
    fi

    # Cleanup
    rm -f "$output_file"
}

# Run CLI tests
echo "CLI Tests:"
echo "----------"

# Test --help
printf "  %-30s " "--help"
if "$ASM64" --help | grep -q "Usage:"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Test --version
printf "  %-30s " "--version"
if "$ASM64" --version | grep -q "asm64 version"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Test missing file error
printf "  %-30s " "missing file error"
if ! "$ASM64" nonexistent_file_12345.asm 2>&1 | grep -q "cannot open"; then
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
else
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
fi

echo ""

# Run assembly tests
echo "Assembly Tests:"
echo "---------------"

# Find all .asm test files
if ls "$SCRIPT_DIR"/*.asm 1> /dev/null 2>&1; then
    for test_file in "$SCRIPT_DIR"/*.asm; do
        run_test "$test_file"
    done
else
    echo "  (no test files found)"
fi

echo ""

# Summary
echo "================"
echo -e "Results: ${GREEN}$PASSED passed${NC}, ${RED}$FAILED failed${NC}, ${YELLOW}$SKIPPED skipped${NC}"

if [ $FAILED -gt 0 ]; then
    exit 1
fi

exit 0
