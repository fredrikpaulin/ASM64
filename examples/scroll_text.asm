; ============================================
; Scroll Text - ASM64 Example
; ============================================
; Smooth horizontal scrolling text demo.
; Demonstrates fine scrolling and text handling.
; ============================================

*=$0801

; BASIC stub
!byte $0c, $08, $0a, $00, $9e, $32, $30, $36, $34, $00, $00, $00

*=$0810

; ============================================
; Hardware Registers
; ============================================
VIC_CTRL2   = $d016     ; VIC control register 2 (scroll)
BORDER      = $d020
BACKGROUND  = $d021

SCREEN      = $0400     ; Default screen memory
SCROLL_ROW  = SCREEN + 12 * 40  ; Middle row

; ============================================
; Zero Page Variables
; ============================================
scroll_pos  = $02       ; Fine scroll position (0-7)
text_ptr    = $03       ; Pointer into message (16-bit)
text_ptr_hi = $04

; ============================================
; Main Program
; ============================================
start:
    ; Set colors
    lda #$00
    sta BORDER
    lda #$06            ; Blue background
    sta BACKGROUND

    ; Clear screen
    ldx #$00
    lda #$20            ; Space character
-   sta SCREEN,x
    sta SCREEN+$100,x
    sta SCREEN+$200,x
    sta SCREEN+$300,x
    dex
    bne -

    ; Initialize scroll
    lda #$07
    sta scroll_pos

    ; Initialize text pointer
    lda #<message
    sta text_ptr
    lda #>message
    sta text_ptr_hi

main_loop:
    ; Wait for raster line 250
-   lda $d012
    cmp #$fa
    bne -

    ; Update fine scroll
    dec scroll_pos
    bpl set_scroll

    ; Reset fine scroll and shift characters
    lda #$07
    sta scroll_pos
    jsr scroll_chars

set_scroll:
    ; Set hardware scroll register
    lda VIC_CTRL2
    and #$f8            ; Clear scroll bits
    ora scroll_pos      ; Set new scroll
    sta VIC_CTRL2

    jmp main_loop

; ============================================
; Scroll Characters Left
; ============================================
scroll_chars:
    ; Shift all characters left
    ldx #$00
-   lda SCROLL_ROW+1,x
    sta SCROLL_ROW,x
    inx
    cpx #39
    bne -

    ; Get next character from message
    ldy #$00
    lda (text_ptr),y
    bne +

    ; End of message - wrap around
    lda #<message
    sta text_ptr
    lda #>message
    sta text_ptr_hi
    lda (text_ptr),y

+   ; Convert to screen code (simple ASCII)
    cmp #$40
    bcc ++
    cmp #$60
    bcs +
    sec
    sbc #$40
    jmp store_char
+   sec
    sbc #$20
    jmp store_char
++  ; Already screen code

store_char:
    sta SCROLL_ROW+39

    ; Advance text pointer
    inc text_ptr
    bne +
    inc text_ptr_hi
+
    rts

; ============================================
; Scroll Message
; ============================================
message:
    !scr "    WELCOME TO ASM64 - THE PORTABLE 6502 CROSS-ASSEMBLER FOR C64!     "
    !scr "FEATURING MACROS, LOOPS, CONDITIONALS, AND MORE...     "
    !scr "PRESS RUN/STOP + RESTORE TO EXIT.          "
    !byte $00
