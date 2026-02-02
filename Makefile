# ASM64 - 6502/6510 Assembler for Commodore 64
# Portable Makefile for macOS and Linux

CC ?= cc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2
CFLAGS_DEBUG = -std=c99 -Wall -Wextra -pedantic -g -DDEBUG

SRCDIR = src
INCDIR = include
TESTDIR = test
BUILDDIR = build

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

TARGET = $(BUILDDIR)/asm64
TEST_LEXER = $(BUILDDIR)/test_lexer
TEST_OPCODES = $(BUILDDIR)/test_opcodes
TEST_SYMBOLS = $(BUILDDIR)/test_symbols
TEST_EXPR = $(BUILDDIR)/test_expr
TEST_PARSER = $(BUILDDIR)/test_parser
TEST_ASSEMBLER = $(BUILDDIR)/test_assembler
TEST_DIRECTIVES = $(BUILDDIR)/test_directives
TEST_CONDITIONAL = $(BUILDDIR)/test_conditional
TEST_MACRO = $(BUILDDIR)/test_macro
TEST_LOOP = $(BUILDDIR)/test_loop
TEST_OUTPUT = $(BUILDDIR)/test_output
TEST_CLI = $(BUILDDIR)/test_cli
TEST_PHASE16 = $(BUILDDIR)/test_phase16

.PHONY: all clean debug test test-lexer test-opcodes test-symbols test-expr test-parser test-assembler test-directives test-conditional test-macro test-loop test-output test-cli test-phase16

all: $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(BUILDDIR) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c -o $@ $<

debug: CFLAGS = $(CFLAGS_DEBUG)
debug: clean $(TARGET)

clean:
	rm -rf $(BUILDDIR)

# Run all tests
test: $(TARGET) test-lexer test-opcodes test-symbols test-expr test-parser test-assembler test-directives test-conditional test-macro test-loop test-output test-cli test-phase16
	@echo "Running integration tests..."
	@cd $(TESTDIR) && ./run_tests.sh ../$(TARGET)

# Build and run lexer unit tests
test-lexer: $(BUILDDIR)/lexer.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o $(TESTDIR)/test_lexer.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $(TEST_LEXER) $(TESTDIR)/test_lexer.c \
		$(BUILDDIR)/lexer.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o
	@echo ""
	@./$(TEST_LEXER)

# Build and run opcode table unit tests
test-opcodes: $(BUILDDIR)/opcodes.o $(TESTDIR)/test_opcodes.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $(TEST_OPCODES) $(TESTDIR)/test_opcodes.c \
		$(BUILDDIR)/opcodes.o
	@echo ""
	@./$(TEST_OPCODES)

# Build and run symbol table unit tests
test-symbols: $(BUILDDIR)/symbols.o $(TESTDIR)/test_symbols.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $(TEST_SYMBOLS) $(TESTDIR)/test_symbols.c \
		$(BUILDDIR)/symbols.o
	@echo ""
	@./$(TEST_SYMBOLS)

# Build and run expression evaluator unit tests
test-expr: $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o $(TESTDIR)/test_expr.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $(TEST_EXPR) $(TESTDIR)/test_expr.c \
		$(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o
	@echo ""
	@./$(TEST_EXPR)

# Build and run parser unit tests
test-parser: $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o $(TESTDIR)/test_parser.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $(TEST_PARSER) $(TESTDIR)/test_parser.c \
		$(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o
	@echo ""
	@./$(TEST_PARSER)

# Build and run assembler unit tests
test-assembler: $(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o $(TESTDIR)/test_assembler.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $(TEST_ASSEMBLER) $(TESTDIR)/test_assembler.c \
		$(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o
	@echo ""
	@./$(TEST_ASSEMBLER)

# Build and run directive unit tests
test-directives: $(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o $(TESTDIR)/test_directives.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $(TEST_DIRECTIVES) $(TESTDIR)/test_directives.c \
		$(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o
	@echo ""
	@./$(TEST_DIRECTIVES)

# Build and run conditional assembly unit tests
test-conditional: $(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o $(TESTDIR)/test_conditional.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $(TEST_CONDITIONAL) $(TESTDIR)/test_conditional.c \
		$(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o
	@echo ""
	@./$(TEST_CONDITIONAL)

# Build and run macro unit tests
test-macro: $(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o $(TESTDIR)/test_macro.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $(TEST_MACRO) $(TESTDIR)/test_macro.c \
		$(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o
	@echo ""
	@./$(TEST_MACRO)

# Build and run loop unit tests
test-loop: $(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o $(TESTDIR)/test_loop.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $(TEST_LOOP) $(TESTDIR)/test_loop.c \
		$(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o
	@echo ""
	@./$(TEST_LOOP)

# Build and run output generation unit tests
test-output: $(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o $(TESTDIR)/test_output.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $(TEST_OUTPUT) $(TESTDIR)/test_output.c \
		$(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o
	@echo ""
	@./$(TEST_OUTPUT)

# Build and run CLI unit tests
test-cli: $(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o $(TESTDIR)/test_cli.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $(TEST_CLI) $(TESTDIR)/test_cli.c \
		$(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o
	@echo ""
	@./$(TEST_CLI)

# Build and run Phase 16 (Advanced Features) unit tests
test-phase16: $(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o $(TESTDIR)/test_phase16.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $(TEST_PHASE16) $(TESTDIR)/test_phase16.c \
		$(BUILDDIR)/assembler.o $(BUILDDIR)/parser.o $(BUILDDIR)/expr.o $(BUILDDIR)/lexer.o $(BUILDDIR)/symbols.o $(BUILDDIR)/opcodes.o $(BUILDDIR)/util.o $(BUILDDIR)/error.o
	@echo ""
	@./$(TEST_PHASE16)

# Dependencies
$(BUILDDIR)/main.o: $(SRCDIR)/main.c $(INCDIR)/error.h $(INCDIR)/util.h
$(BUILDDIR)/error.o: $(SRCDIR)/error.c $(INCDIR)/error.h
$(BUILDDIR)/util.o: $(SRCDIR)/util.c $(INCDIR)/util.h
$(BUILDDIR)/lexer.o: $(SRCDIR)/lexer.c $(INCDIR)/lexer.h $(INCDIR)/error.h $(INCDIR)/util.h
$(BUILDDIR)/opcodes.o: $(SRCDIR)/opcodes.c $(INCDIR)/opcodes.h
$(BUILDDIR)/symbols.o: $(SRCDIR)/symbols.c $(INCDIR)/symbols.h
$(BUILDDIR)/expr.o: $(SRCDIR)/expr.c $(INCDIR)/expr.h $(INCDIR)/lexer.h $(INCDIR)/symbols.h
$(BUILDDIR)/parser.o: $(SRCDIR)/parser.c $(INCDIR)/parser.h $(INCDIR)/lexer.h $(INCDIR)/expr.h $(INCDIR)/opcodes.h $(INCDIR)/symbols.h
$(BUILDDIR)/assembler.o: $(SRCDIR)/assembler.c $(INCDIR)/assembler.h $(INCDIR)/parser.h $(INCDIR)/lexer.h $(INCDIR)/expr.h $(INCDIR)/opcodes.h $(INCDIR)/symbols.h
