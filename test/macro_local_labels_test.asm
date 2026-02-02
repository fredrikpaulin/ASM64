; Test macro local labels are scoped per expansion
*= $1000

!macro inc_if_less value, limit
    lda value
    cmp #limit
    bcs .done
    inc value
.done:
!endmacro

var1: !byte 0
var2: !byte 0

start:
    +inc_if_less var1, 5    ; First expansion - .done should be unique
    +inc_if_less var2, 10   ; Second expansion - .done should be unique
    rts
