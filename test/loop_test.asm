; Loop test - tests branching and loops
; Tests: beq, bne, dec, loop constructs

*=$1000

start:
    ldx #$05        ; loop counter
loop:
    dex             ; decrement
    bne loop        ; loop until zero
    rts
