# ASM64 Directive Reference

This document provides a complete reference for all directives supported by ASM64.

## Data Definition Directives

### !byte / !by / !b
Define one or more byte values.

```asm
!byte $01, $02, $03
!byte <label         ; Low byte of address
!byte >label         ; High byte of address
!byte "A"            ; Character value
```

### !word / !wo / !w
Define one or more 16-bit words (little-endian).

```asm
!word $1234          ; Stored as $34, $12
!word label          ; Address of label
!word $1234, $5678   ; Multiple words
```

### !text / !tx
Define ASCII text string.

```asm
!text "Hello World"
!text "Line 1", $0d, "Line 2"  ; With embedded bytes
```

### !scr
Define text in C64 screen code format.

```asm
!scr "HELLO"         ; Converted to screen codes
```

### !pet
Define text in PETSCII format.

```asm
!pet "hello"         ; Converted to PETSCII
```

### !null
Define null-terminated string.

```asm
!null "String"       ; Adds $00 terminator
```

### !fill
Fill memory with a repeated byte value.

```asm
!fill 100            ; 100 zero bytes
!fill 64, $EA        ; 64 NOP instructions
!fill size, value    ; Expression support
```

## Program Counter Directives

### * (asterisk)
Set the program counter (origin).

```asm
*=$0801              ; Set PC to $0801
* = $1000            ; Spaces allowed
```

### !org
Alternative syntax for setting program counter.

```asm
!org $1000
```

### !skip
Reserve bytes without initializing them.

```asm
!skip 256            ; Skip 256 bytes
```

### !align
Align program counter to a boundary.

```asm
!align 256           ; Align to page boundary
!align 16, $EA       ; Align with NOP fill
```

## Macro Directives

### !macro / !endmacro
Define a macro.

```asm
!macro name
    ; macro body
!endmacro

!macro name arg1, arg2
    lda #arg1
    sta arg2
!endmacro
```

### + (plus prefix)
Invoke a macro.

```asm
+name                ; No arguments
+name $42            ; One argument
+name $42, $d020     ; Multiple arguments
```

## Conditional Assembly Directives

### !if / !else / !endif
Conditional assembly based on expression.

```asm
!if expression
    ; assembled if expression is non-zero
!else
    ; assembled if expression is zero
!endif
```

### !ifdef / !ifndef
Conditional assembly based on symbol definition.

```asm
!ifdef SYMBOL
    ; assembled if SYMBOL is defined
!endif

!ifndef SYMBOL
    ; assembled if SYMBOL is NOT defined
!endif
```

## Loop Directives

### !for / !end
Repeat a block with a loop variable.

```asm
!for i, start, end
    !byte i
!end
```

The loop variable is available in expressions:

```asm
!for i, 0, 7
    !byte i * 2      ; 0, 2, 4, 6, 8, 10, 12, 14
!end
```

## File Inclusion Directives

### !source / !src
Include another source file.

```asm
!source "library.asm"
!src "macros.asm"
```

### !binary / !bin
Include a binary file.

```asm
!binary "data.bin"              ; Include entire file
!binary "data.bin", length      ; Include first N bytes
!binary "data.bin", length, offset  ; Include with offset
```

## Pseudo-PC Directives

### !pseudopc
Set a virtual program counter for label calculation.

```asm
!pseudopc $C000      ; Labels get $C000-based addresses
    label:           ; label = $C000
    nop
!realpc
```

### !realpc
End pseudo-PC mode and return to real addresses.

```asm
!realpc
```

## CPU Selection Directives

### !cpu
Select the target CPU type.

```asm
!cpu 6502            ; Standard 6502
!cpu 6510            ; 6510 with illegal opcodes (default)
!cpu "65c02"         ; 65C02 extended instructions
```

## Diagnostic Directives

### !error
Generate an error and stop assembly.

```asm
!error "Configuration error"
```

### !warn
Generate a warning but continue assembly.

```asm
!warn "Deprecated feature used"
```

## Zone/Scope Directives

### !zone / !zn
Start a new zone for local label scoping.

```asm
!zone routine1
.loop:               ; Local to this zone
    nop
    bne .loop

!zone routine2
.loop:               ; Different label, same name
    nop
    bne .loop
```

## C64-Specific Directives

### !basic
Generate a BASIC stub for SYS command.

```asm
*=$0801
!basic $0810         ; 10 SYS 2064
```

Note: This generates the standard BASIC stub:
```
!byte $0c, $08, $0a, $00, $9e, $32, $30, $36, $34, $00, $00, $00
```

## Deprecated/Unsupported

The following ACME directives are not currently supported:

- `!bank` - Bank switching (planned for future)
- `!segment` - Named segments (planned for future)
- `!set` - Use `=` assignment instead
- `!convtab` - Character conversion tables

## Expression Operators

Expressions can be used in most directive arguments:

| Operator | Description |
|----------|-------------|
| `+` | Addition |
| `-` | Subtraction / Negation |
| `*` | Multiplication |
| `/` | Division |
| `%` | Modulo |
| `&` | Bitwise AND |
| `\|` | Bitwise OR |
| `^` | Bitwise XOR |
| `~` | Bitwise NOT |
| `<<` | Left shift |
| `>>` | Right shift |
| `<` | Low byte |
| `>` | High byte |
| `==` | Equality |
| `!=` / `<>` | Inequality |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less or equal |
| `>=` | Greater or equal |
| `&&` | Logical AND |
| `\|\|` | Logical OR |
| `!` | Logical NOT |
