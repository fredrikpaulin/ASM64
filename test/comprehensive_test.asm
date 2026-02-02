; Comprehensive assembler test
; Tests multiple features together

*=$0801

; BASIC stub: 10 SYS 2064
!byte $0c, $08, $0a, $00, $9e, $32, $30, $36, $34, $00, $00, $00

*=$0810

; Constants
SCREEN = $0400
COLOR_RAM = $D800
BORDER = $D020
BACKGROUND = $D021

; Macros
!macro set_color color
    lda #color
    sta BORDER
    sta BACKGROUND
!endmacro

!macro clear_screen char
    ldx #$00
-   lda #char
    sta SCREEN,x
    sta SCREEN+$100,x
    sta SCREEN+$200,x
    sta SCREEN+$300,x
    dex
    bne -
!endmacro

; Main program
main:
    +set_color $00      ; black
    +clear_screen $20   ; space character

    ; Print message
    ldx #$00
print_loop:
    lda message,x
    beq done
    sta SCREEN,x
    inx
    bne print_loop

done:
    rts

message:
    !scr "HELLO C64!"
    !byte $00

; Data section at fixed address
*=$2000
data_table:
!for i, 0, 15
    !byte i * 2
!end

sprite_data:
    !fill 63, $ff
    !byte $00   ; sprite enable byte
