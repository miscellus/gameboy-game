DEF ASSERTIONS = 0

INCLUDE "macros.s"
INCLUDE "hardware.s"
INCLUDE "header.s"
INCLUDE "tiles.s"
INCLUDE "map.s"
INCLUDE "high_ram.s"
INCLUDE "memory.s"
INCLUDE "variables.s"

SECTION "Program Start",ROM0[$150]
ProgramStart:
	ei				 ;enable interrupts
	ld  sp,$FFFE  ; Init stack pointer
	ld  a,IEF_VBLANK ;enable vblank interrupt
	ld  [rInterruptEnable], a

	xor a
	ldh [rLcdControl],a 	 ;LCD off
	ldh [rLcdControlStatus],a

	ld  a,%11100100  ;shade palette (11 10 01 00)
	ldh [rBackgroundPalette],a 	 ;setup palettes
	ldh [rObjectPaletteData],a
	ldh [rObjectPalette0],a
	ld a, %11100001
	ldh [rObjectPalette1],a

	call LoadTiles
	call LoadMap
	call ClearSprites
	; call ClearScreen

	ld  a,(LCDCF_ON|LCDCF_WIN9C00|LCDCF_BG8000|LCDCF_OBJON|LCDCF_BGON|LCDCF_OBJ16) ;turn on LCD, BG0, OBJ0, etc
	ldh [rLcdControl],a    ;load LCD flags

	rst CopyDmaResetVector

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; InitRam:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	xor a
	ld [DeltaX], a
	ld [DeltaY], a
	ld [PlayerJumpInputBuffering], a

	ld a, 160/2
	ld [SprPlayer.X], a
	add a, 8
	ld [SprPlayer_2.X], a
	ld a, 144/2
	ld [SprPlayer.Y], a
	ld [SprPlayer_2.Y], a
	ld a, Tile_Player_bl
	ld [SprPlayer.Tile], a
	ld a, Tile_Player_br
	ld [SprPlayer_2.Tile], a
	ld a, (0*OAMF_PRI|OAMF_PAL1)
	ld [SprPlayer.Flags], a
	;or a, OAMF_XFLIP
	ld [SprPlayer_2.Flags], a

	ld a, 80
	ld [PlayerX], a
	ld a, 64
	ld [PlayerY], a

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GameLoop:
; The outline of the main game loop is as follows:
;     1) Wait for a vblank to have occured
;     2) Read and store the state of the buttons on the gameboy
;     3) Update the player's position (handling collisions)
;     4) Doing the DMA transfer to the pixel processing unit (PPU)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    call WaitForVBlank
    call UpdateInput
    call UpdatePlayer
    call UpdateViewPort
	jr GameLoop

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ClearSprites:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ld  hl, Sprites
	xor a
	ld  c, 4*40
	call MemFill255
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ClearScreen:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ld  hl, _SCRN0    ;load map0 ram
	xor a
	ld  bc, 1024
	call MemFill
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
LoadTiles:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ld  hl, TileData
	ld  de, _VRAM
	ld  bc, (TileDataEnd-TileData)
	call MemCopy
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
LoadMap:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ld  hl, MapData  ;same as LoadTiles
	ld  de, _SCRN0
	ld  bc, (1024)
	call MemCopy
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
IsOccupiedBySolid:
; Takes: A as the Y coordinate
; Takes: L as the X corrdinate
; Returns: If occupied in the zero flag (ZF=1 means occupied)
; Clobbers: A, D, H, L, F
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	rlca
	rlca
	ld h, a
	and a, %00011100; These are the offset bits for Y
	ld a, %1111;0000
	;ld a, %11111111
	jr nz, .HasYOffset
	and a, %0011;0000
.HasYOffset:
	ld d, a
	ld a, l
	and a, %00000111; These are the offset bits for X
	ld a, d
	jr nz, .HasXOffset
	and a, %0101;0000
.HasXOffset:
	ld d, a
	ld a, l
	rrca
	rrca
	rrca
	and a, %00011111
	ld l, a

	ld a, h
	and a, %11100000
	add a, l
	;add a, MapData & 0xff
	ld l, a

	ld a, h
	and a, %00000011
	add a, CollisionMap>>8
	ld h, a

	; Always look at (x+0,y+0)
	ld a, [hl]
	and a, d
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
CoordinateToMapOffset:
	; Takes: A as the Y coordinate
	; Takes: L as the X corrdinate
	; Returns address of underlying tile in AL
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	rlca
	rlca
	ld h, a

	ld a, l
	rrca
	rrca
	rrca
	and a, %00011111
	ld l, a

	ld a, h
	and a, %11100000
	add a, l
	;add a, (MapData&$ff)
	ld l, a

	ld a, h
	;and a, %00011100 These are the offset bits
	and a, %00000011
	;add a, (MapData>>8)
	;ld h, a

	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
WaitForVBlank:
; Waits for VBlank, and increments VBlankCount
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	halt ; @HardwareBug:
	nop ; If interrupts are disabled, HALT jumps one instruction!

	ld a, [IsWaitingForVBlank]
	or a
	jr nz, WaitForVBlank ; zero means vblank has happened

	; Here a should be zero, so increment to 1 to signify wait for vblank
	inc a
	ld [IsWaitingForVBlank], a

	ld a, [VBlankCount]
	inc a
	ld [VBlankCount], a

    ret

;;;;;;;;;;;;;;;;;;;
UpdateInput:
;
; NOTE(jakob):
; a bit value of 0 means button is down
;
;;;;;;;;;;;;;;;;;;;
	ld  a,JoyPad_Select_DPad
	ldh  [JoyPad],a
	ldh  a,[JoyPad]    ;takes a few cycles to get accurate reading
	ldh  a,[JoyPad]
	ldh  a,[JoyPad]
	ldh  a,[JoyPad]
;	cpl ;complement a
	or %11110000	;mask dpad buttons
	swap a
	ld  b,a ; Save D-Pad states in register b

	ld  a,JoyPad_Select_Other
	ldh  [JoyPad],a
	ldh  a,[JoyPad]
	ldh  a,[JoyPad]
	ldh  a,[JoyPad]
	ldh  a,[JoyPad]
;	cpl
	or %11110000 ; mask other buttons
	and  b


	ld  c,a ; New Down state

	ld  a,[ButtonsDown]
	ld l, a ; Save old button state
	cpl
	or c
	ld  [ButtonsPressed],a
	ld a, c
	cpl
	or l
	ld [ButtonsReleased], a
	ld  a, c
	ld  [ButtonsDown],a

	ld  a, JoyPad_Select_Other|JoyPad_Select_DPad
	ld  [JoyPad], a
    
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
UpdatePlayer:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    DEF JumpForce equ 28
    DEF XSpeed equ 3

    ld a, [ButtonsDown]
    ld c, a

    ; Compute X input
    ;
	xor a

	bit ButtonRight, c
	jr nz, :+
	add a, XSpeed
:

	bit ButtonLeft, c
	jr nz, :+
	add a, -XSpeed
:
	
    ld e, a ; Save xSpeed in e

IF 0
    pusha
    ld hl, DebugStack
    ld [hl], a
    ld d, h
    ld e, l
    printf "Desired X speed: %hd"
    popa
ENDC


	ld a, [DeltaX]
	ld b, a
	sra a
	sra a

    ; Add 1 to deltaX if going left
	bit 7, a
	jr z, .deltaXIsNotNegative
	inc a
.deltaXIsNotNegative:

	sra a

    ; DeltaX += Xspeed - DeltaX/8
	ld d, a
	ld a, b 
	sub a, d
	add a, e

	ld [DeltaX], a

	sra a
	sra a
	bit 7, a
	jr z, .dsgjkljdsakgldsag
	inc a
.dsgjkljdsakgldsag:
	sra a
	ld e, a

	or a
	jr z, .AddDeltaXToPosition
	bit 7, a
	jr z, .PointSpriteRightwards

	ld a, Tile_Player_bl
	ld [SprPlayer.Tile], a
	ld a, Tile_Player_br
	ld [SprPlayer_2.Tile], a
	ld a, OAMF_PAL1
	ld [SprPlayer.Flags], a
	ld [SprPlayer_2.Flags], a

	jr .AddDeltaXToPosition
.PointSpriteRightwards:
	ld a, Tile_Player_br
	ld [SprPlayer.Tile], a
	ld a, Tile_Player_bl
	ld [SprPlayer_2.Tile], a
	ld a, (OAMF_PAL1|OAMF_XFLIP)
	ld [SprPlayer.Flags], a
	ld [SprPlayer_2.Flags], a

.AddDeltaXToPosition:
	; Add delta X
	ld a, [PlayerX]
	add a, e
	ld [PlayerX], a

	ld b, a

	ld l, a
	ld a, [PlayerY]
	call IsOccupiedBySolid;(l,a)

	jr z, .NoCollisionX
	; A collision happend on the x axis
	ld a, b            ; Restore new x coordinate
	bit 7, e
	jr z, .GoingRight
	sub a, e           ; If deltaX was negative, subtract it to snap to the left
.GoingRight:
	and a, %11111000   ; mask off the offset to snap into grid-alignment
	ld [PlayerX], a
	ld b, a

	xor a
	ld [DeltaX], a
.NoCollisionX:


	ld a, [ButtonsPressed]
	bit ButtonA, a
	ld a, [PlayerJumpInputBuffering]
	jr nz, .SkipJumpInputBufferRenew
	ld a, %00001111
.SkipJumpInputBufferRenew:
	srl a
	ld [PlayerJumpInputBuffering], a
	jr nc, .NoJump
	;dprint "Slack %A%"

	; Check if on ground
	ld a, [PlayerY] ; restore y
	ld c, a ; save y
	and a, %111
	ld a, 0
	jr nz, .NoJump ; Must be aligned with grid on Y to jump
	ld a, b ; restore X position
	ld l, a
	ld a, c ; restore y
	call CoordinateToMapOffset
	add a, CollisionMap >> 8
	ld h, a
	ld a, [hl]
	and a, %1100;0000
	ld c, a
	jr z, .NoJump
	ld a, b
	and a, %111
	ld a, c
	jr nz, .NoXoffset
	and %0100;0000
	jr z, .NoJump
.NoXoffset:
	xor a
	ld [PlayerJumpInputBuffering], a
	ld a, -JumpForce
	ld [DeltaY], a
.NoJump:

	;ButtonHandle ButtonDown, add a\, Speed


	ld e, a
	inc e
	inc e
	inc e

	ld a, [DeltaY]
	add a, e
	; n = n - ((n + (n>0 ? 7 : 0))>>3)
	ld l, a
	bit 7, a
	jr nz, .NonPositiveDeltaY
	or a
	jr z, .NonPositiveDeltaY
	; a > 0
	add a, 15
.NonPositiveDeltaY:
	sra a
	sra a
	sra a
	sra a
	sra a
	ld d, a
	ld a, l
	sub a, d

	dec a ; Limit DeltaY


	ld [DeltaY], a
	sra a
	sra a
	sra a
	ld e, a

	ld a, [ButtonsReleased]
	bit ButtonA, a
	jr nz, .SkipStopJump
	bit 7, e
	jr z, .SkipStopJump
	ld a, e
	sra a
	ld e, a
	ld [DeltaY], a
.SkipStopJump:

	; Add delta Y
	ld a, [PlayerY]
	add a, e
	ld [PlayerY], a

	ld l, b ; Current X coordinate
	ld b, a ; Candidate Y corrdinate
	call IsOccupiedBySolid;(l,a)

	jr z, .NoCollisionY
	; A collision happend on the y axis
	ld a, b            ; Restore new y coordinate
	bit 7, e
	jr z, .GoingUp
	sub a, e           ; If deltaY was negative, subtract it to snap upwards
.GoingUp:
	and a, %11111000   ; mask off the offset to snap into grid-alignment
	ld [PlayerY], a
	ld b, a
	xor a
	ld [DeltaY], a
.NoCollisionY:

	ld a, [ButtonsPressed]
	bit ButtonB, a
	jr nz, .SkipBankSwitchTest
	ld b, b
	ld a, 2
	ld [BankRegister], a
.SkipBankSwitchTest:

    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
UpdateViewPort:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ld a, [PlayerX]
	sub a, 160/2 - 4
	ld [rScreenX], a

	ld a, [PlayerY]
	sub a, 144/2 - 8
	ld [rScreenY], a

	call HRAM_DmaRoutine

    ret

