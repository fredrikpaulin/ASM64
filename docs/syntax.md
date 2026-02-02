# ASM64 Syntax Reference

## Overview

ASM64 uses ACME-compatible syntax with `!` prefix for directives. This document provides a complete reference for the assembly language syntax.

## Basic Structure

A typical assembly line has the format:

```
[label:]  [instruction/directive]  [operand]  [; comment]
```

All parts are optional. Empty lines and comment-only lines are allowed.

### Examples

```asm
; This is a comment
label:                      ; Label only
        lda #$10            ; Instruction with operand
counter: !byte 0            ; Label and directive
        sta $d020           ; Instruction only
```

## Labels

### Global Labels

Global labels are visible throughout the entire source file.

```asm
main_loop:
        jmp main_loop

subroutine:
        rts
```

Labels can end with a colon (optional but recommended for clarity):
```asm
start:  sei         ; With colon
start   sei         ; Without colon (also valid)
```

### Local Labels

Local labels begin with a dot (`.`) and are scoped to the current zone or the most recent global label.

```asm
print_string:
        ldx #0
.loop:  lda message,x
        beq .done
        jsr $ffd2
        inx
        bne .loop
.done:  rts

clear_screen:
.loop:                  ; Different .loop, no conflict
        sta $0400,x
        inx
        bne .loop
        rts
```

### Anonymous Labels

Anonymous labels use `+` (forward) and `-` (backward) characters:

```asm
-       lda $d012       ; Anonymous label
        cmp #$80
        bne -           ; Branch to previous -

        ldx #0
-       lda data,x      ; Another anonymous label
        beq +           ; Branch to next +
        jsr process
        inx
        bne -           ; Branch to previous -
+       rts             ; Target of beq +
```

Multiple `+` or `-` skip that many anonymous labels:
```asm
--      ...             ; Target of bne --
-       ...             ; Target of bne -
        bne --          ; Skip one, go to first
```

### Equates (Constants)

Define symbolic constants:

```asm
SCREEN      = $0400
BORDER      = $d020
BLACK       = 0
WHITE       = 1

        lda #BLACK
        sta BORDER
```

## Numbers

### Formats

| Format | Prefix | Example |
|--------|--------|---------|
| Hexadecimal | `$` | `$FF`, `$D020` |
| Binary | `%` | `%10101010` |
| Decimal | (none) | `123`, `255` |
| Character | `'` | `'A'`, `'*'` |

### Examples

```asm
        lda #$10        ; Hex: 16
        lda #%00001111  ; Binary: 15
        lda #100        ; Decimal: 100
        lda #'A'        ; Character: 65
```

## Expressions

Expressions can be used anywhere a numeric value is expected.

### Operators

| Operator | Description | Precedence |
|----------|-------------|------------|
| `(` `)` | Grouping | Highest |
| `<` | Low byte | Unary |
| `>` | High byte | Unary |
| `-` | Negate | Unary |
| `~` | Bitwise NOT | Unary |
| `!` | Logical NOT | Unary |
| `*` `/` `%` | Multiply, divide, modulo | 7 |
| `+` `-` | Add, subtract | 6 |
| `<<` `>>` | Shift left, right | 5 |
| `<` `>` `<=` `>=` | Comparison | 4 |
| `=` `<>` | Equal, not equal | 4 |
| `&` | Bitwise AND | 3 |
| `^` | Bitwise XOR | 2 |
| `\|` | Bitwise OR | Lowest |

### Special Values

- `*` - Current program counter

### Examples

```asm
        lda #<address       ; Low byte of address
        ldy #>address       ; High byte of address

        !fill end - start, 0    ; Calculate size

        lda #(BASE + OFFSET) & $ff

        !if * > $c000 {
            !error "Code too large!"
        }
```

## Addressing Modes

### Summary Table

| Mode | Syntax | Size | Example |
|------|--------|------|---------|
| Implied | (none) | 1 | `inx` |
| Accumulator | `A` | 1 | `asl a` |
| Immediate | `#value` | 2 | `lda #$10` |
| Zero Page | `$zz` | 2 | `lda $80` |
| Zero Page,X | `$zz,x` | 2 | `lda $80,x` |
| Zero Page,Y | `$zz,y` | 2 | `ldx $80,y` |
| Absolute | `$aaaa` | 3 | `lda $1000` |
| Absolute,X | `$aaaa,x` | 3 | `lda $1000,x` |
| Absolute,Y | `$aaaa,y` | 3 | `lda $1000,y` |
| Indirect | `($aaaa)` | 3 | `jmp ($fffc)` |
| Indexed Indirect | `($zz,x)` | 2 | `lda ($80,x)` |
| Indirect Indexed | `($zz),y` | 2 | `lda ($80),y` |
| Relative | `label` | 2 | `bne loop` |

### Zero Page Optimization

The assembler automatically uses zero-page addressing when the operand is in the range $00-$FF:

```asm
zp_var = $80

        lda zp_var      ; Assembles as LDA $80 (zero page)
        lda $1000       ; Assembles as LDA $1000 (absolute)
```

**Important:** Define zero-page labels before use to ensure optimization.

## Strings

### Encoding Types

| Directive | Encoding | Use Case |
|-----------|----------|----------|
| `!text` | ASCII | Raw ASCII bytes |
| `!pet` | PETSCII | PRINT output |
| `!scr` | Screen codes | Direct screen memory |

### Examples

```asm
message:
        !text "Hello"           ; ASCII: $48 $65 $6c $6c $6f
        !pet "Hello"            ; PETSCII (case swapped)
        !scr "Hello"            ; Screen codes
        !byte 0                 ; Null terminator
```

### Escape Sequences

| Sequence | Meaning |
|----------|---------|
| `\\` | Backslash |
| `\"` | Double quote |
| `\n` | Newline ($0d in PETSCII) |
| `\r` | Return |
| `\t` | Tab |

## Comments

```asm
; Full line comment

label:  lda #0      ; End of line comment

        ; Indented comment
        sta $d020
```

## Case Sensitivity

- **Instructions:** Case insensitive (`LDA`, `lda`, `Lda` are equivalent)
- **Directives:** Case insensitive (`!BYTE`, `!byte`, `!Byte`)
- **Labels:** Case sensitive (`Label` and `label` are different)
- **Registers:** Case insensitive (`A`, `a`, `X`, `x`, `Y`, `y`)
