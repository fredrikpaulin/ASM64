; Test local labels with zone scoping
; Each global label starts a new zone

*= $1000

; First function - insert_char zone
insert_char:
    lda #$01
    beq .done
    bcc .error
.done:
    rts
.error:
    sec
    rts

; Second function - delete_char zone
; Should have its own .done and .error labels
delete_char:
    lda #$02
    beq .done
.done:
    rts

; Test explicit !zone directive
!zone myzone
.local1:
    nop
    jmp .local1

; Another explicit zone
!zone otherzone
.local1:            ; Different from myzone's .local1
    nop
    jmp .local1

; Back to implicit zone from global label
third_func:
.loop:
    dex
    bne .loop
    rts
