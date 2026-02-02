; Conditional assembly test
; Tests: !if, !ifdef, !ifndef, !else, !endif

*=$4000

DEBUG = 1
RELEASE = 0

!ifdef DEBUG
    lda #$01    ; debug mode
!else
    lda #$00    ; release mode
!endif

!if 1
    sta $d020   ; always include
!endif

!ifndef UNDEFINED
    nop         ; this should be included
!endif

    rts
