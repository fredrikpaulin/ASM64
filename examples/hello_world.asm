; ============================================
; Hello World - ASM64 Example
; ============================================
; A simple program that prints "HELLO, WORLD!"
; to the screen using the KERNAL CHROUT routine.
;
; Load and run with: LOAD"HELLO",8,1 then RUN
; ============================================

*=$0801

; ============================================
; BASIC Stub - Creates: 10 SYS 2064
; ============================================
!byte $0c, $08      ; Pointer to next BASIC line
!byte $0a, $00      ; Line number 10
!byte $9e           ; SYS token
!text "2064"        ; Address as text
!byte $00           ; End of line
!byte $00, $00      ; End of BASIC program

; ============================================
; Main Program - Starts at $0810 (2064)
; ============================================
*=$0810

CHROUT = $ffd2      ; KERNAL character output

start:
    ldx #$00        ; String index
print_loop:
    lda message,x   ; Get next character
    beq done        ; Zero = end of string
    jsr CHROUT      ; Print character
    inx             ; Next character
    bne print_loop  ; Continue (max 255 chars)
done:
    rts             ; Return to BASIC

; ============================================
; Data
; ============================================
message:
    !text "HELLO, WORLD!"
    !byte $0d       ; Carriage return
    !byte $00       ; String terminator
