; Multi-zone local label test
*= $1000

func1:
    lda #$01
    beq .done
.done:
    rts

func2:
    lda #$02
    beq .done       ; This .done is different from func1's .done
.done:
    rts
