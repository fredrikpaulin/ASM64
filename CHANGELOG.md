# ASM64 Changelog

All notable changes to ASM64 are documented in this file.

## [1.0.2] 2026-06-05

### Behavior Changes
- Expression evaluation now fails assembly for division or modulo by zero, invalid shift counts, signed 32-bit overflow, and `INT32_MIN / -1` style arithmetic traps.
- Program counter tracking no longer wraps silently past `$FFFF`; assemblies that cross the 64K address space now report an error.
- Directive names are case-insensitive. Unknown directives are now source errors instead of warnings.
- `!if` conditions must be defined when evaluated. Forward-referenced conditions no longer silently assemble as false.
- `!cpu 6502` and `!cpu 65c02` reject illegal/undocumented opcodes. The default `6510` mode still accepts them.

### Bug Fixes
- Fixed anonymous forward labels so multiple references to the same `+` label resolve correctly, including mixed `+` and `++` references.
- Fixed pass-2 diagnostics from includes, macros, and loops so they report the stored source filename and line.
- Fixed parser token ownership for directive string arguments and lexer/parser expression errors.
- Reset macros, macro expansion state, loop state, and CPU mode when reusing an `Assembler`.
- Hardened source, binary include, listing, symbol, and output file I/O against seek, tell, read, write, close, and partial-transfer failures.
- Added allocation checks around macro definition and expansion paths before partially built structures can be used.

### Performance
- Added opcode lookup tables for mnemonic/addressing-mode lookup and opcode-byte lookup.
- Changed `!binary` pass 1 to validate the file span and advance the PC without reading and emitting the file contents.
- Removed listing source capture's repeated source-buffer rescan in favor of direct current-line slicing.

### Tests
- Added regression coverage for CPU opcode restrictions, anonymous forward labels, expression errors, include diagnostics, directive casing, unknown directives, forward-referenced `!if`, PC overflow, and assembler reuse.
- Verified with the normal test suite and an AddressSanitizer/UndefinedBehaviorSanitizer build.

---

## [1.0.1] - 2026-04-14

### Bug Fixes
- Fixed crash in `str_rtrim()` when called with an empty string (unsigned underflow)
- Fixed undefined behavior in expression evaluator when shift amount >= 32
- Fixed unchecked `ftell()` return values in `assembler_assemble_file()` and `assembler_include_binary()` that could cause invalid allocations on I/O errors
- Fixed partial allocation failure in `symbol_define()` and `symbol_reference()` that could leak memory or dereference NULL
- Fixed memory leaks on `realloc` failure in directive and macro call argument parsing
- Fixed `lexer_peek()` leaking string token allocations when peeking past string literals
- Fixed `hash_set()` not checking `str_dup()` return, preventing NULL key dereference on OOM
- Fixed `scope_push()` not checking `str_dup()` return for scope name

### Performance
- Fixed over-allocation in loop variable substitution that allocated ~2x needed memory per iteration

### Cleanup
- Removed unreachable dead code for default output filename generation in `main.c`

---

## [1.0.0] - 2026-02-02

### Initial Release

ASM64 is a portable 6502/6510 cross-assembler designed for Commodore 64 development, with ACME-compatible syntax.

### Features

#### Core Assembler
- Two-pass assembly for forward reference support
- Support for 6502, 6510, and 65C02 instruction sets
- Automatic zero-page optimization for known addresses
- All standard addressing modes supported
- Anonymous labels with `+` and `-` references

#### Data Directives
- `!byte` / `!by` / `!b` - Define bytes
- `!word` / `!wo` / `!w` - Define 16-bit words (little-endian)
- `!text` / `!tx` - ASCII text strings
- `!scr` - C64 screen code text
- `!pet` - PETSCII text
- `!null` - Null-terminated strings
- `!fill` - Fill memory with repeated values

#### Program Counter
- `*=` - Set program counter (origin)
- `!org` - Alternative origin syntax
- `!skip` - Reserve uninitialized bytes
- `!align` - Align to memory boundary
- `!pseudopc` / `!realpc` - Virtual address assembly

#### Macros
- `!macro` / `!endmacro` - Define macros with parameters
- `+name` - Macro invocation syntax
- Parameter substitution in macro bodies

#### Conditional Assembly
- `!if` / `!else` / `!endif` - Expression-based conditionals
- `!ifdef` / `!ifndef` - Symbol definition checks

#### Loops
- `!for` / `!end` - Repeat blocks with loop variable

#### File Inclusion
- `!source` / `!src` - Include source files
- `!binary` / `!bin` - Include binary data

#### C64 Support
- `!basic` - Generate BASIC SYS stub
- Illegal opcode support for 6510

#### Diagnostics
- `!error` - Generate assembly error
- `!warn` - Generate warning message
- `!zone` - Label scoping zones

#### Expression Support
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Bitwise: `&`, `|`, `^`, `~`, `<<`, `>>`
- Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical: `!`
- Address operators: `<` (low byte), `>` (high byte)

#### Output Options
- Raw PRG output (default, with load address)
- Assembly listing generation
- Symbol table output
- Verbose mode for debugging

### Testing
- 458 unit tests across 13 test modules
- 13 integration test programs
- Comprehensive directive coverage

### Documentation
- README with quick start guide
- Complete directive reference
- Example programs demonstrating key features

---

## Version History

| Version | Date | Description |
|---------|------|-------------|
| Unreleased | TBD | Audit remediation, stricter diagnostics, robustness, performance |
| 1.0.1 | 2026-04-14 | Bug fixes, memory safety, performance |
| 1.0.0 | 2026-02-02 | Initial release |
