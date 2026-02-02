; Basic loader - standard C64 program header
; Tests: org, basic header generation, jsr, rts

*=$0801

; BASIC stub: 10 SYS 2064
!byte $0c, $08, $0a, $00, $9e, $32, $30, $36, $34, $00, $00, $00

*=$0810
start:
    lda #$00        ; clear border
    sta $d020
    lda #$00        ; clear background
    sta $d021
    rts
