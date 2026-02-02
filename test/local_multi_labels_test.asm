; Test multiple local labels in same zone with forward refs
*= $1000

insert_char:
    lda #$01
    beq .done
    bcc .error
.done:
    rts
.error:
    sec
    rts
