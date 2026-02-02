; Pseudopc test  
; Tests: !pseudopc, !realpc for relocatable code

*=$1000

; Main code
main:
    jsr relocated_code_addr
    rts

; This will be copied to $C000 at runtime
; Labels inside get $C000-based addresses
!pseudopc $C000
relocated:
    lda #$01
    sta $d020
    jmp relocated   ; jumps to $C000
relocated_end:
!realpc

; Back to normal assembly
relocated_code_addr = $C000
end:
    nop
