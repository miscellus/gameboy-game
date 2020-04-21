;-------------
;	Header
;-------------

DMA equ $FF80

SECTION "Copy Data",ROM0[$28] ;copy DMA routine to HRAM
CopyDmaResetVector:
; 	ld de, $FF80 ;Hi mem address of DMACode
	ld hl, DMACode
	ld c, $80
	ld b, DMACodeEnd-DMACode
.loop
	ld a,[hl+]
	ld [$ff00+c], a
	inc c
	dec b
	jr nz,.loop
	reti
; 
SECTION "VBlank IRQ",ROM0[$40]
VBlankResetVector:
	xor  a
	ld  [IsWaitingForVBlank], a ; make IsWaitingForVBlank = 0
	reti

SECTION	"LCD IRQ Vector",ROM0[$48]
LcdResetVector:
	reti

SECTION	"Timer IRQ Vector",ROM0[$50]
TimerResetVector:
	reti

SECTION	"Serial IRQ Vector",ROM0[$58]
SerialResetVector:
	reti

SECTION	"Joypad IRQ Vector",ROM0[$60]
JoyPadResetVector:
	reti

SECTION "Stored DMA Routine", ROM0
DMACode:
	DB  $F5, $3E, $C1, $EA, $46, $FF, $3E, $28, $3D, $20, $FD, $F1, $D9
DMACodeEnd:


SECTION "Header",ROM0[$100]
	nop
	jp ProgramStart
CartridgeHeader:
.NintendoLogo:
	; $0104-$0133 Nintendo logo !DONT MODIFY!
	DB  $CE,$ED,$66,$66,$CC,$0D,$00,$0B,$03,$73,$00,$83,$00,$0C,$00,$0D
	DB  $00,$08,$11,$1F,$88,$89,$00,$0E,$DC,$CC,$6E,$E6,$DD,$DD,$D9,$99
	DB  $BB,$BB,$67,$63,$6E,$0E,$EC,$CC,$DD,$DC,$99,$9F,$BB,$B9,$33,$3E

.GameTitle:
	; $0134-$013E Game title
	DB	"Muto Locus",0
		  ;0123456789A

.ProductCode:
	; $013F-$0142 Product code, leave blank
	DB	0,0,0,0
		;0123

.CompatibilityMode:
	; $0143 compatibility mode
	DB	$00	; $00 - DMG 
			; $80 - DMG/GBC
			; $C0 - GBC Only cartridge

.LicenceCodeHighNibble:
	; $0144 (High-nibble of license code - normally $00 if $014B != $33)
	DB	$00

.LicenceCodeLowNibble:
	; $0145 (Low-nibble of license code - normally $00 if $014B != $33)
	DB	$00


.GameBoyOrSuperGameBoyIndicator:

	; $0146 (GameBoy/Super GameBoy indicator)
	DB	$00	; $00 - GameBoy

.CartridgeType:
	; $0147 (Cartridge type - all Color GameBoy cartridges are at least $19)
	DB	$01 ; $00 - ROM Only
			; $01 - MBC1
			; $19 - MBC5

.RomSizeCode:
	; $0148 (ROM size)
	DB	$00	; $00 - 256Kbit = 32Kbyte = 2 banks

.RamSizeCode:
	; $0149 (RAM size)
	DB	$00	; $00 - None

.RegionCode:
	; $014A (Destination code)
	DB	$01	; $01 - All others
			; $00 - Japan

.LicenceeCode:
	; $014B (Licensee code - this _must_ be $33)
	DB	$33	; $33 - Check $0144/$0145 for Licensee code.

.MaskRomVersion:
	; $014C (Mask ROM version - handled by RGBFIX)
	DB	$00

.ComplementCheck:
	; $014D (Complement check - handled by RGBFIX)
	DB	$00

.CartridgeChecksum:
	; $014E-$014F (Cartridge checksum - handled by RGBFIX)
	DW	$00
