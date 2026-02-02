# ASM64 - 6502/6510 Cross-Assembler for Commodore 64

ASM64 is a modern, portable cross-assembler for the MOS 6502/6510 processor, designed specifically for Commodore 64 development. It features ACME-compatible syntax, powerful macro support, and generates ready-to-run .PRG files.

## Features

- **Full 6502/6510 instruction set** including illegal/undocumented opcodes
- **ACME-compatible syntax** for easy migration of existing projects
- **Powerful macro system** with parameters and local labels
- **Conditional assembly** with `!if`, `!ifdef`, `!ifndef`
- **Loop constructs** with `!for` directive
- **Pseudo-PC** for relocatable code (`!pseudopc`, `!realpc`)
- **Automatic zero-page optimization**
- **Expression evaluator** with full arithmetic and bitwise operators
- **Multiple output formats** (.PRG with load address, raw binary)
- **Symbol and listing file generation**
- **Include file support** with configurable search paths

## Quick Start

### Building

```bash
# Clone and build
git clone https://github.com/fredrikpaulin/asm64.git
cd asm64
make

# Run tests (optional)
make test
```

### Hello World

Create `hello.asm`:

```asm
*=$0801

; BASIC stub: 10 SYS 2064
!byte $0c, $08, $0a, $00, $9e, $32, $30, $36, $34, $00, $00, $00

*=$0810
    ldx #$00
loop:
    lda message,x
    beq done
    jsr $ffd2       ; KERNAL CHROUT
    inx
    bne loop
done:
    rts

message:
    !text "HELLO, WORLD!"
    !byte $0d, $00  ; newline + null
```

Assemble and run:

```bash
./asm64 hello.asm -o hello.prg
# Load hello.prg in your favorite C64 emulator
```

## Command Line Usage

```
asm64 [options] <source.asm>

Options:
  -o <file>       Output filename (default: a.prg)
  -f <format>     Output format: prg (default), raw
  -l <file>       Generate listing file
  -s <file>       Generate symbol file
  -I <path>       Add include search path
  -D <name=val>   Define symbol (e.g., -DDEBUG=1)
  -v              Verbose output
  --help          Show help
  --version       Show version
```

## Syntax Reference

### Numbers

```asm
lda #$FF        ; Hexadecimal
lda #%10101010  ; Binary
lda #255        ; Decimal
lda #'A'        ; Character
```

### Labels

```asm
global_label:       ; Global label
.local_label:       ; Local label (scoped to zone)
+                   ; Anonymous forward reference
-                   ; Anonymous backward reference
```

### Addressing Modes

```asm
lda #$42        ; Immediate
lda $42         ; Zero Page
lda $42,x       ; Zero Page,X
lda $42,y       ; Zero Page,Y (LDX/STX only)
lda $1234       ; Absolute
lda $1234,x     ; Absolute,X
lda $1234,y     ; Absolute,Y
lda ($42,x)     ; Indexed Indirect
lda ($42),y     ; Indirect Indexed
jmp ($1234)     ; Indirect (JMP only)
bne label       ; Relative (branches)
```

### Directives

#### Data Definition

```asm
!byte $01, $02, $03     ; Define bytes
!word $1234, $5678      ; Define 16-bit words (little-endian)
!text "Hello"           ; ASCII text
!scr "Hello"            ; C64 screen codes
!pet "Hello"            ; PETSCII
!null "String"          ; Null-terminated string
!fill 100, $00          ; Fill 100 bytes with $00
```

#### Program Counter

```asm
*=$0801                 ; Set program counter
!org $1000              ; Alternative syntax
```

#### Macros

```asm
!macro name arg1, arg2
    lda #arg1
    sta arg2
!endmacro

+name $42, $d020        ; Invoke macro
```

#### Conditional Assembly

```asm
DEBUG = 1

!ifdef DEBUG
    ; Debug code
!else
    ; Release code
!endif

!if DEBUG == 1
    ; Conditional on expression
!endif

!ifndef SYMBOL
    ; If symbol not defined
!endif
```

#### Loops

```asm
!for i, 0, 7
    !byte i * 2
!end
; Generates: $00, $02, $04, $06, $08, $0A, $0C, $0E
```

#### Pseudo-PC (Relocatable Code)

```asm
*=$0900
; Copy routine here...

!pseudopc $00       ; Assemble as if at $00
zp_routine:
    lda #$05
    sta $d020
    rts
!realpc             ; Back to real address
```

#### CPU Selection

```asm
!cpu 6502           ; Standard 6502 (no illegal opcodes)
!cpu 6510           ; 6510 with illegal opcodes (default)
!cpu "65c02"        ; 65C02 extended instructions
```

#### File Inclusion

```asm
!source "library.asm"       ; Include source file
!binary "data.bin"          ; Include binary file
!binary "sprite.bin", 63, 1 ; Include with offset and length
```

#### Alignment and Skip

```asm
!align 256          ; Align to 256-byte boundary
!align 16, $EA      ; Align with NOP fill
!skip 100           ; Reserve 100 bytes
```

### Expressions

```asm
value = $1000 + $20     ; Addition
value = $1000 - $100    ; Subtraction
value = 10 * 20         ; Multiplication
value = 100 / 4         ; Division
value = 17 % 5          ; Modulo

value = $FF & $0F       ; Bitwise AND
value = $F0 | $0F       ; Bitwise OR
value = $FF ^ $AA       ; Bitwise XOR
value = ~$FF            ; Bitwise NOT
value = $01 << 4        ; Left shift
value = $80 >> 4        ; Right shift

low = <$1234            ; Low byte ($34)
high = >$1234           ; High byte ($12)
```

## Output Formats

### PRG (Default)

Standard C64 program format with 2-byte load address header:

```bash
./asm64 source.asm -o program.prg
```

### Raw Binary

No header, just the assembled bytes:

```bash
./asm64 source.asm -o program.bin -f raw
```

### Symbol File

Vice-compatible symbol file for debugging:

```bash
./asm64 source.asm -o program.prg -s program.sym
```

### Listing File

Assembly listing with addresses and bytes:

```bash
./asm64 source.asm -o program.prg -l program.lst
```

## Environment Variables

- `ASM64_INCLUDE` - Colon-separated list of include search paths

```bash
export ASM64_INCLUDE="/path/to/includes:/another/path"
```

## Building from Source

### Requirements

- C99 compatible compiler (GCC, Clang)
- Make
- POSIX-compatible system (Linux, macOS, WSL)

### Build Commands

```bash
make            # Build optimized binary
make debug      # Build with debug symbols
make test       # Run test suite
make clean      # Remove build artifacts
```

### Installation

To install `asm64` to `/usr/local/bin`:

```bash
./install.sh
```

To install to a custom location:

```bash
INSTALL_DIR=~/bin ./install.sh
```

To uninstall:

```bash
sudo rm /usr/local/bin/asm64
```

## License

MIT License - see LICENSE file for details.

## Version History

See [CHANGELOG.md](CHANGELOG.md) for release notes.
