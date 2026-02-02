; ============================================
; Sprite Demo - ASM64 Example
; ============================================
; Displays a simple sprite that bounces
; around the screen. Demonstrates sprite
; hardware and movement.
; ============================================

*=$0801

; BASIC stub
!byte $0c, $08, $0a, $00, $9e, $32, $30, $36, $34, $00, $00, $00

*=$0810

; ============================================
; Hardware Registers
; ============================================
VIC_BASE    = $d000
SPRITE0_X   = VIC_BASE + $00    ; Sprite 0 X position
SPRITE0_Y   = VIC_BASE + $01    ; Sprite 0 Y position
SPRITE_EN   = VIC_BASE + $15    ; Sprite enable
SPRITE_XHI  = VIC_BASE + $10    ; Sprite X high bits
SPRITE0_COL = VIC_BASE + $27    ; Sprite 0 color
SPRITE_PTR  = $07f8             ; Sprite pointer (screen + $3f8)

BORDER      = $d020
BACKGROUND  = $d021

; ============================================
; Zero Page Variables
; ============================================
sprite_x    = $02       ; 16-bit X position
sprite_x_hi = $03
sprite_y    = $04       ; Y position
x_dir       = $05       ; X direction (0=right, 1=left)
y_dir       = $06       ; Y direction (0=down, 1=up)

; ============================================
; Main Program
; ============================================
start:
    ; Set screen colors
    lda #$00
    sta BORDER
    sta BACKGROUND

    ; Set up sprite pointer (sprite data at $2000 = block 128)
    lda #128
    sta SPRITE_PTR

    ; Initialize sprite position
    lda #$a0
    sta sprite_x
    lda #$00
    sta sprite_x_hi
    lda #$64
    sta sprite_y

    ; Initialize directions
    lda #$00
    sta x_dir
    sta y_dir

    ; Set sprite color
    lda #$01            ; White
    sta SPRITE0_COL

    ; Enable sprite 0
    lda #$01
    sta SPRITE_EN

main_loop:
    ; Wait for raster
    lda #$ff
-   cmp $d012
    bne -

    ; Update sprite position
    jsr move_sprite

    ; Set sprite hardware position
    lda sprite_x
    sta SPRITE0_X
    lda sprite_y
    sta SPRITE0_Y

    ; Handle X high bit
    lda sprite_x_hi
    beq +
    lda SPRITE_XHI
    ora #$01
    sta SPRITE_XHI
    jmp main_loop
+   lda SPRITE_XHI
    and #$fe
    sta SPRITE_XHI

    jmp main_loop

; ============================================
; Move Sprite
; ============================================
move_sprite:
    ; Move X
    lda x_dir
    bne move_left

move_right:
    inc sprite_x
    bne +
    inc sprite_x_hi
+   ; Check right boundary (344 = $158)
    lda sprite_x_hi
    cmp #$01
    bcc check_y
    lda sprite_x
    cmp #$58
    bcc check_y
    lda #$01
    sta x_dir
    jmp check_y

move_left:
    lda sprite_x
    bne +
    dec sprite_x_hi
+   dec sprite_x
    ; Check left boundary (24)
    lda sprite_x_hi
    bne check_y
    lda sprite_x
    cmp #$18
    bcs check_y
    lda #$00
    sta x_dir

check_y:
    ; Move Y
    lda y_dir
    bne move_up

move_down:
    inc sprite_y
    lda sprite_y
    cmp #$e6            ; Bottom boundary
    bcc done
    lda #$01
    sta y_dir
    jmp done

move_up:
    dec sprite_y
    lda sprite_y
    cmp #$32            ; Top boundary
    bcs done
    lda #$00
    sta y_dir

done:
    rts

; ============================================
; Sprite Data - A simple ball
; ============================================
*=$2000
sprite_data:
    !byte %00000011, %11000000, %00000000
    !byte %00001111, %11110000, %00000000
    !byte %00011111, %11111000, %00000000
    !byte %00111111, %11111100, %00000000
    !byte %01111111, %11111110, %00000000
    !byte %01111111, %11111110, %00000000
    !byte %11111111, %11111111, %00000000
    !byte %11111111, %11111111, %00000000
    !byte %11111111, %11111111, %00000000
    !byte %11111111, %11111111, %00000000
    !byte %11111111, %11111111, %00000000
    !byte %01111111, %11111110, %00000000
    !byte %01111111, %11111110, %00000000
    !byte %00111111, %11111100, %00000000
    !byte %00011111, %11111000, %00000000
    !byte %00001111, %11110000, %00000000
    !byte %00000011, %11000000, %00000000
    !byte %00000000, %00000000, %00000000
    !byte %00000000, %00000000, %00000000
    !byte %00000000, %00000000, %00000000
    !byte %00000000, %00000000, %00000000
    !byte $00       ; Sprite enable byte
