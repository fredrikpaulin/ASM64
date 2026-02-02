; Simple local label test
*= $1000

myfunc:
.loop:
    dex
    bne .loop
    rts
