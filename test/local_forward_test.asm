; Local label forward reference test
*= $1000

myfunc:
    lda #$00
    beq .done       ; forward reference to .done
    nop
.done:
    rts
