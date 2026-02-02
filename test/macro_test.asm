; Macro test
; Tests: macro definition and expansion

*=$3000

!macro inc16 addr
    inc addr
    bne +
    inc addr+1
+
!endmacro

counter: !word $0000

start:
    +inc16 counter
    +inc16 counter
    rts
