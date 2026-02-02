; Expression test
; Tests: arithmetic, bitwise, high/low byte

*=$5000

BASE = $1000
OFFSET = $20

value1:
    !byte <BASE         ; low byte = $00
    !byte >BASE         ; high byte = $10
    !byte BASE + OFFSET ; $20
    !byte BASE - $100   ; $00
    !word BASE | $000F  ; $100F
    !word BASE & $FF00  ; $1000
    !byte BASE >> 8     ; $10
    !byte (BASE << 4) & $FF ; $00
end:
    rts
