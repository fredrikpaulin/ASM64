; ============================================
; Raster Bars - ASM64 Example
; ============================================
; Creates colorful raster bars by changing
; the border/background color on each raster line.
; Demonstrates interrupts and timing.
;
; Press RUN/STOP + RESTORE to exit.
; ============================================

*=$0801

; BASIC stub
!byte $0c, $08, $0a, $00, $9e, $32, $30, $36, $34, $00, $00, $00

*=$0810

; ============================================
; Hardware Registers
; ============================================
VIC_CTRL    = $d011     ; VIC control register
VIC_RASTER  = $d012     ; Current raster line
VIC_IRQ     = $d019     ; VIC interrupt register
VIC_IRQEN   = $d01a     ; VIC interrupt enable
BORDER      = $d020     ; Border color
BACKGROUND  = $d021     ; Background color
CIA1_ICR    = $dc0d     ; CIA 1 interrupt control
IRQ_VECTOR  = $0314     ; IRQ vector (low byte)
IRQ_VECTORH = $0315     ; IRQ vector (high byte)

; ============================================
; Main Program
; ============================================
start:
    sei                 ; Disable interrupts

    ; Disable CIA interrupts
    lda #$7f
    sta CIA1_ICR
    lda CIA1_ICR        ; Acknowledge pending

    ; Enable raster interrupts
    lda #$01
    sta VIC_IRQEN

    ; Set raster line for first interrupt
    lda #$00
    sta VIC_RASTER

    ; Clear high bit of raster compare
    lda VIC_CTRL
    and #$7f
    sta VIC_CTRL

    ; Set up IRQ vector
    lda #<irq_handler
    sta IRQ_VECTOR
    lda #>irq_handler
    sta IRQ_VECTORH

    cli                 ; Enable interrupts

    ; Main loop - just wait
main_loop:
    jmp main_loop

; ============================================
; IRQ Handler
; ============================================
irq_handler:
    ; Acknowledge VIC interrupt
    lda #$01
    sta VIC_IRQ

    ; Get current raster line as color
    lda VIC_RASTER
    lsr                 ; Divide by 16 for color
    lsr
    lsr
    lsr
    and #$0f            ; Mask to valid color
    sta BORDER
    sta BACKGROUND

    ; Return from interrupt
    jmp $ea31           ; Jump to standard IRQ end
