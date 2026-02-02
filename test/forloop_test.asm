; For loop test
; Tests: !for directive

*=$6000

; Generate table of squares (0-4)
squares:
!for i, 0, 4
    !byte i * i
!end

; Repeat bytes
pattern:
!for n, 0, 2
    !byte $AA, $55
!end

end:
    rts
