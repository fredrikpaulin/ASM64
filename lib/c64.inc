;-----------------------------------------------------------------------------
; c64.inc - Commodore 64 Hardware Definitions
; ASM64 - 6502/6510 Assembler for Commodore 64
;
; This file provides symbolic names for all C64 hardware registers
; and common memory locations. Include it with:
;
;   !source "c64.inc"
;
;-----------------------------------------------------------------------------

;-----------------------------------------------------------------------------
; PROCESSOR PORT ($0000-$0001)
;-----------------------------------------------------------------------------

CPU_PORT_DDR    = $0000     ; Data Direction Register (read/write)
CPU_PORT        = $0001     ; Processor Port (read/write)

; CPU Port bits
LORAM           = %00000001 ; BASIC ROM at $A000-$BFFF (1=ROM, 0=RAM)
HIRAM           = %00000010 ; Kernal ROM at $E000-$FFFF (1=ROM, 0=RAM)
CHAREN          = %00000100 ; Character ROM at $D000-$DFFF (1=I/O, 0=Char ROM)
CASSETTE_DATA   = %00001000 ; Cassette data output
CASSETTE_SENSE  = %00010000 ; Cassette switch sense (1=no button pressed)
CASSETTE_MOTOR  = %00100000 ; Cassette motor control (0=motor on)

; Common memory configurations
MEM_ALL_RAM     = %00110101 ; All RAM visible ($35)
MEM_IO_ONLY     = %00110110 ; RAM + I/O visible ($36)
MEM_BASIC       = %00110111 ; BASIC + Kernal + I/O ($37) - default

;-----------------------------------------------------------------------------
; ZERO PAGE - Important Locations ($0000-$00FF)
;-----------------------------------------------------------------------------

ZP_START        = $0002     ; First usable ZP location
ZP_END          = $00FF     ; Last ZP location (stack pointer area)

; BASIC/Kernal working storage
CURLIN          = $39       ; Current BASIC line number (2 bytes)
TXTPTR          = $7A       ; BASIC text pointer (2 bytes)

;-----------------------------------------------------------------------------
; STACK ($0100-$01FF)
;-----------------------------------------------------------------------------

STACK_START     = $0100
STACK_END       = $01FF

;-----------------------------------------------------------------------------
; DEFAULT SCREEN MEMORY ($0400-$07FF)
;-----------------------------------------------------------------------------

SCREEN          = $0400     ; Default screen memory (1000 bytes)
SCREEN_SIZE     = 1000      ; 40x25 characters

;-----------------------------------------------------------------------------
; BASIC PROGRAM AREA ($0801-$9FFF)
;-----------------------------------------------------------------------------

BASIC_START     = $0801     ; Default BASIC program start
BASIC_END       = $9FFF     ; End of BASIC RAM

;-----------------------------------------------------------------------------
; VIC-II REGISTERS ($D000-$D3FF, mirrors every 64 bytes)
;-----------------------------------------------------------------------------

VIC             = $D000     ; VIC-II base address

; Sprite positions (X coordinate low byte, Y coordinate)
VIC_SP0X        = $D000     ; Sprite 0 X position
VIC_SP0Y        = $D001     ; Sprite 0 Y position
VIC_SP1X        = $D002     ; Sprite 1 X position
VIC_SP1Y        = $D003     ; Sprite 1 Y position
VIC_SP2X        = $D004     ; Sprite 2 X position
VIC_SP2Y        = $D005     ; Sprite 2 Y position
VIC_SP3X        = $D006     ; Sprite 3 X position
VIC_SP3Y        = $D007     ; Sprite 3 Y position
VIC_SP4X        = $D008     ; Sprite 4 X position
VIC_SP4Y        = $D009     ; Sprite 4 Y position
VIC_SP5X        = $D00A     ; Sprite 5 X position
VIC_SP5Y        = $D00B     ; Sprite 5 Y position
VIC_SP6X        = $D00C     ; Sprite 6 X position
VIC_SP6Y        = $D00D     ; Sprite 6 Y position
VIC_SP7X        = $D00E     ; Sprite 7 X position
VIC_SP7Y        = $D00F     ; Sprite 7 Y position

; Sprite X MSB (bit 8 of X coordinate for all sprites)
VIC_SPXMSB      = $D010     ; Sprite X MSB (bit 0=sprite 0, bit 7=sprite 7)

; Control registers
VIC_CTRL1       = $D011     ; Control register 1
VIC_RASTER      = $D012     ; Raster line (read) / IRQ trigger line (write)
VIC_LIGHTX      = $D013     ; Light pen X position
VIC_LIGHTY      = $D014     ; Light pen Y position
VIC_SPEN        = $D015     ; Sprite enable (bit per sprite)
VIC_CTRL2       = $D016     ; Control register 2
VIC_SPYEXP      = $D017     ; Sprite Y expansion
VIC_MEMPTR      = $D018     ; Memory pointers (screen/char memory)
VIC_IRQ         = $D019     ; Interrupt register (read: status, write: ack)
VIC_IRQEN       = $D01A     ; Interrupt enable mask
VIC_SPPRI       = $D01B     ; Sprite-data priority
VIC_SPMC        = $D01C     ; Sprite multicolor enable
VIC_SPXEXP      = $D01D     ; Sprite X expansion
VIC_SPSPCOL     = $D01E     ; Sprite-sprite collision (read)
VIC_SPDATCOL    = $D01F     ; Sprite-data collision (read)

; Color registers
VIC_BORDER      = $D020     ; Border color
VIC_BGCOLOR0    = $D021     ; Background color 0
VIC_BGCOLOR1    = $D022     ; Background color 1 (multicolor/extended)
VIC_BGCOLOR2    = $D023     ; Background color 2 (multicolor/extended)
VIC_BGCOLOR3    = $D024     ; Background color 3 (extended)
VIC_SPMC0       = $D025     ; Sprite multicolor 0
VIC_SPMC1       = $D026     ; Sprite multicolor 1
VIC_SP0COL      = $D027     ; Sprite 0 color
VIC_SP1COL      = $D028     ; Sprite 1 color
VIC_SP2COL      = $D029     ; Sprite 2 color
VIC_SP3COL      = $D02A     ; Sprite 3 color
VIC_SP4COL      = $D02B     ; Sprite 4 color
VIC_SP5COL      = $D02C     ; Sprite 5 color
VIC_SP6COL      = $D02D     ; Sprite 6 color
VIC_SP7COL      = $D02E     ; Sprite 7 color

; VIC_CTRL1 bits
VIC_CTRL1_YSCROLL = %00000111  ; Y scroll (bits 0-2)
VIC_CTRL1_RSEL    = %00001000  ; Row select (0=24 rows, 1=25 rows)
VIC_CTRL1_DEN     = %00010000  ; Display enable
VIC_CTRL1_BMM     = %00100000  ; Bitmap mode
VIC_CTRL1_ECM     = %01000000  ; Extended color mode
VIC_CTRL1_RST8    = %10000000  ; Raster bit 8

; VIC_CTRL2 bits
VIC_CTRL2_XSCROLL = %00000111  ; X scroll (bits 0-2)
VIC_CTRL2_CSEL    = %00001000  ; Column select (0=38 cols, 1=40 cols)
VIC_CTRL2_MCM     = %00010000  ; Multicolor mode
VIC_CTRL2_RES     = %00100000  ; Reset bit (not used)

; VIC_IRQ bits
VIC_IRQ_RASTER    = %00000001  ; Raster compare interrupt
VIC_IRQ_SPBC      = %00000010  ; Sprite-background collision
VIC_IRQ_SPSP      = %00000100  ; Sprite-sprite collision
VIC_IRQ_LP        = %00001000  ; Light pen interrupt
VIC_IRQ_ANY       = %10000000  ; Set if any interrupt occurred

;-----------------------------------------------------------------------------
; SID REGISTERS ($D400-$D7FF, mirrors every 32 bytes)
;-----------------------------------------------------------------------------

SID             = $D400     ; SID base address

; Voice 1
SID_V1FREQLO    = $D400     ; Voice 1 frequency low byte
SID_V1FREQHI    = $D401     ; Voice 1 frequency high byte
SID_V1PWLO      = $D402     ; Voice 1 pulse width low byte
SID_V1PWHI      = $D403     ; Voice 1 pulse width high nybble
SID_V1CTRL      = $D404     ; Voice 1 control register
SID_V1AD        = $D405     ; Voice 1 attack/decay
SID_V1SR        = $D406     ; Voice 1 sustain/release

; Voice 2
SID_V2FREQLO    = $D407     ; Voice 2 frequency low byte
SID_V2FREQHI    = $D408     ; Voice 2 frequency high byte
SID_V2PWLO      = $D409     ; Voice 2 pulse width low byte
SID_V2PWHI      = $D40A     ; Voice 2 pulse width high nybble
SID_V2CTRL      = $D40B     ; Voice 2 control register
SID_V2AD        = $D40C     ; Voice 2 attack/decay
SID_V2SR        = $D40D     ; Voice 2 sustain/release

; Voice 3
SID_V3FREQLO    = $D40E     ; Voice 3 frequency low byte
SID_V3FREQHI    = $D40F     ; Voice 3 frequency high byte
SID_V3PWLO      = $D410     ; Voice 3 pulse width low byte
SID_V3PWHI      = $D411     ; Voice 3 pulse width high nybble
SID_V3CTRL      = $D412     ; Voice 3 control register
SID_V3AD        = $D413     ; Voice 3 attack/decay
SID_V3SR        = $D414     ; Voice 3 sustain/release

; Filter and volume
SID_FLTLO       = $D415     ; Filter cutoff low 3 bits
SID_FLTHI       = $D416     ; Filter cutoff high 8 bits
SID_FLTCTRL     = $D417     ; Filter control (resonance + routing)
SID_VOLUME      = $D418     ; Master volume + filter mode

; Misc (read-only)
SID_POTX        = $D419     ; Potentiometer X (read)
SID_POTY        = $D41A     ; Potentiometer Y (read)
SID_OSC3        = $D41B     ; Oscillator 3 output (read)
SID_ENV3        = $D41C     ; Envelope 3 output (read)

; Voice control bits
SID_CTRL_GATE   = %00000001 ; Gate (start/stop ADSR)
SID_CTRL_SYNC   = %00000010 ; Sync oscillator to voice 3/1/2
SID_CTRL_RING   = %00000100 ; Ring modulation
SID_CTRL_TEST   = %00001000 ; Test bit (reset oscillator)
SID_CTRL_TRI    = %00010000 ; Triangle waveform
SID_CTRL_SAW    = %00100000 ; Sawtooth waveform
SID_CTRL_PULSE  = %01000000 ; Pulse waveform
SID_CTRL_NOISE  = %10000000 ; Noise waveform

; Filter control bits
SID_FILT_V1     = %00000001 ; Route voice 1 through filter
SID_FILT_V2     = %00000010 ; Route voice 2 through filter
SID_FILT_V3     = %00000100 ; Route voice 3 through filter
SID_FILT_EXT    = %00001000 ; Route external audio through filter

; Volume/filter mode bits
SID_MODE_LP     = %00010000 ; Low-pass filter
SID_MODE_BP     = %00100000 ; Band-pass filter
SID_MODE_HP     = %01000000 ; High-pass filter
SID_MODE_V3OFF  = %10000000 ; Disconnect voice 3 output

;-----------------------------------------------------------------------------
; COLOR RAM ($D800-$DBFF)
;-----------------------------------------------------------------------------

COLOR           = $D800     ; Color RAM (1000 bytes, same layout as screen)
COLOR_SIZE      = 1000

;-----------------------------------------------------------------------------
; CIA 1 REGISTERS ($DC00-$DCFF)
;-----------------------------------------------------------------------------

CIA1            = $DC00     ; CIA 1 base address

CIA1_PRA        = $DC00     ; Port A (keyboard column select)
CIA1_PRB        = $DC01     ; Port B (keyboard row read)
CIA1_DDRA       = $DC02     ; Data direction A
CIA1_DDRB       = $DC03     ; Data direction B
CIA1_TALO       = $DC04     ; Timer A low byte
CIA1_TAHI       = $DC05     ; Timer A high byte
CIA1_TBLO       = $DC06     ; Timer B low byte
CIA1_TBHI       = $DC07     ; Timer B high byte
CIA1_TOD10      = $DC08     ; Time of Day 1/10 seconds
CIA1_TODSEC     = $DC09     ; Time of Day seconds
CIA1_TODMIN     = $DC0A     ; Time of Day minutes
CIA1_TODHR      = $DC0B     ; Time of Day hours
CIA1_SDR        = $DC0C     ; Serial data register
CIA1_ICR        = $DC0D     ; Interrupt control register
CIA1_CRA        = $DC0E     ; Control register A
CIA1_CRB        = $DC0F     ; Control register B

;-----------------------------------------------------------------------------
; CIA 2 REGISTERS ($DD00-$DDFF)
;-----------------------------------------------------------------------------

CIA2            = $DD00     ; CIA 2 base address

CIA2_PRA        = $DD00     ; Port A (VIC bank, serial bus)
CIA2_PRB        = $DD01     ; Port B (user port)
CIA2_DDRA       = $DD02     ; Data direction A
CIA2_DDRB       = $DD03     ; Data direction B
CIA2_TALO       = $DD04     ; Timer A low byte
CIA2_TAHI       = $DD05     ; Timer A high byte
CIA2_TBLO       = $DD06     ; Timer B low byte
CIA2_TBHI       = $DD07     ; Timer B high byte
CIA2_TOD10      = $DD08     ; Time of Day 1/10 seconds
CIA2_TODSEC     = $DD09     ; Time of Day seconds
CIA2_TODMIN     = $DD0A     ; Time of Day minutes
CIA2_TODHR      = $DD0B     ; Time of Day hours
CIA2_SDR        = $DD0C     ; Serial data register
CIA2_ICR        = $DD0D     ; Interrupt control register
CIA2_CRA        = $DD0E     ; Control register A
CIA2_CRB        = $DD0F     ; Control register B

; CIA control bits
CIA_CR_START    = %00000001 ; Start timer
CIA_CR_PBON     = %00000010 ; Timer output on port B
CIA_CR_TOGGLE   = %00000100 ; Toggle PB (else pulse)
CIA_CR_ONESHOT  = %00001000 ; One-shot mode
CIA_CR_LOAD     = %00010000 ; Force load timer

; CIA2 Port A bits (VIC bank selection)
CIA2_VIC_BANK_MASK = %00000011  ; Bits 0-1 select VIC bank
CIA2_VIC_BANK_3    = %00000000  ; VIC Bank 3: $C000-$FFFF
CIA2_VIC_BANK_2    = %00000001  ; VIC Bank 2: $8000-$BFFF
CIA2_VIC_BANK_1    = %00000010  ; VIC Bank 1: $4000-$7FFF
CIA2_VIC_BANK_0    = %00000011  ; VIC Bank 0: $0000-$3FFF (default)

;-----------------------------------------------------------------------------
; KERNAL VECTORS ($0314-$0333)
;-----------------------------------------------------------------------------

IRQ_VECTOR      = $0314     ; Hardware IRQ vector (default: $EA31)
BRK_VECTOR      = $0316     ; BRK instruction vector (default: $FE66)
NMI_VECTOR      = $0318     ; NMI vector (default: $FE47)

;-----------------------------------------------------------------------------
; KERNAL JUMP TABLE ($FF81-$FFF5)
;-----------------------------------------------------------------------------

CINT            = $FF81     ; Initialize screen editor
IOINIT          = $FF84     ; Initialize I/O devices
RAMTAS          = $FF87     ; RAM test and initialize
RESTOR          = $FF8A     ; Restore default I/O vectors
VECTOR          = $FF8D     ; Set/read I/O vectors
SETMSG          = $FF90     ; Set Kernal message control
SECOND          = $FF93     ; Send secondary address after LISTEN
TKSA            = $FF96     ; Send secondary address after TALK
MEMTOP          = $FF99     ; Read/set top of memory
MEMBOT          = $FF9C     ; Read/set bottom of memory
SCNKEY          = $FF9F     ; Scan keyboard
SETTMO          = $FFA2     ; Set IEEE timeout
ACPTR           = $FFA5     ; Get byte from serial bus
CIOUT           = $FFA8     ; Send byte to serial bus
UNTALK          = $FFAB     ; Command serial to UNTALK
UNLSN           = $FFAE     ; Command serial to UNLISTEN
LISTEN          = $FFB1     ; Command serial to LISTEN
TALK            = $FFB4     ; Command serial to TALK
READST          = $FFB7     ; Read I/O status word
SETLFS          = $FFBA     ; Set logical file parameters
SETNAM          = $FFBD     ; Set filename
OPEN            = $FFC0     ; Open logical file
CLOSE           = $FFC3     ; Close logical file
CHKIN           = $FFC6     ; Set input channel
CHKOUT          = $FFC9     ; Set output channel
CLRCHN          = $FFCC     ; Clear I/O channels
CHRIN           = $FFCF     ; Get character from input
CHROUT          = $FFD2     ; Output character
LOAD            = $FFD5     ; Load from device
SAVE            = $FFD8     ; Save to device
SETTIM          = $FFDB     ; Set real-time clock
RDTIM           = $FFDE     ; Read real-time clock
STOP            = $FFE1     ; Check RUN/STOP key
GETIN           = $FFE4     ; Get character from keyboard
CLALL           = $FFE7     ; Close all files
UDTIM           = $FFEA     ; Update real-time clock
SCREEN          = $FFED     ; Return screen size
PLOT            = $FFF0     ; Read/set cursor position
IOBASE          = $FFF3     ; Return I/O base address

;-----------------------------------------------------------------------------
; COLORS
;-----------------------------------------------------------------------------

BLACK           = 0
WHITE           = 1
RED             = 2
CYAN            = 3
PURPLE          = 4
GREEN           = 5
BLUE            = 6
YELLOW          = 7
ORANGE          = 8
BROWN           = 9
LIGHT_RED       = 10
DARK_GRAY       = 11
GRAY            = 12
LIGHT_GREEN     = 13
LIGHT_BLUE      = 14
LIGHT_GRAY      = 15

;-----------------------------------------------------------------------------
; SPRITE POINTERS (relative to screen memory)
;-----------------------------------------------------------------------------

; Default sprite pointers at SCREEN + $03F8
SPRITE_PTRS     = SCREEN + $03F8

;-----------------------------------------------------------------------------
; End of c64.inc
;-----------------------------------------------------------------------------
