; Data directive tests
; Tests: !byte, !word, !text, !fill

*=$2000

bytes:
    !byte $01, $02, $03, $04
words:
    !word $1234, $5678
text:
    !text "HELLO"
fill:
    !fill 4, $ff
end:
    rts
