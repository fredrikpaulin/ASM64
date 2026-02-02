; Zero page addressing test
; Tests: automatic zeropage optimization

*=$1000

zp_var = $10
abs_var = $0200

start:
    lda zp_var      ; should use zero page
    sta zp_var+1    ; should use zero page
    lda abs_var     ; should use absolute
    sta abs_var+1   ; should use absolute
    rts
