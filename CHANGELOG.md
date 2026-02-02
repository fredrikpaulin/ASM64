# ASM64 Changelog

All notable changes to ASM64 are documented in this file.

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
- Logical: `&&`, `||`, `!`
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
| 1.0.0 | 2026-02-02 | Initial release |
