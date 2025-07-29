DEF DEBUG EQU 1

INCLUDE "macros.s"
INCLUDE "debug_macros.s"
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

    ; Enable sound
    ld a, %10001111 ; Enable all 4 channels
    ld [rNR52], a

    ld a, %10000000
    ld [rNR14], a ; Enable channel 1

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
;     |        |        |                                                       
;    -+--------+--------+-                                                      
;     | (0, 0) | (1, 0) |   What each bit in a byte of the collision map means: 
;     |   A    |   B    |   The coordinates are offsets from the parameters X,Y 
;     |  Tile  |  Tile  |                                                       
;     |        |        |                   76543210                            
;    -+--------+--------+-           (2,2) -'||||||`- A = (0,0)                 
;     | (0, 1) | (1, 1) |      (0,2)|(1,2) --'||||`-- B = (1,0)                 
;     |   C    |   D    |      (2,0)|(2,1) ---'||`--- C = (0,1)                 
;     |  Tile  |  Tile  |          A|B|C|D ----'`---- D = (1,1)                 
;     |        |        |                                                       
;    -+--------+--------+-  A 1 bit means Solid, 0 means Air                    
;     |        |        |                                                       
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   This implementation assumes a map width of 32
;
    DEF SOLID_00 EQU (1<<0)
    DEF SOLID_10 EQU (1<<1)
    DEF SOLID_01 EQU (1<<2)
    DEF SOLID_11 EQU (1<<3)
    DEF SOLID_00_10_01_11 EQU (1<<4)
    DEF SOLID_20_21 EQU (1<<5)
    DEF SOLID_02_12 EQU (1<<6)
    DEF SOLID_22 EQU (1<<7)

    ;;;;;;;;;;
    ; Y AXIS ;
    ;;;;;;;;;;

	rlca
	rlca
	ld h, a
	and a, %00011100; These are the offset bits for Y

    ; Figure out what solid flags to include based on the coordinate offset
    ; 
    ; By default, include solid flags for (0, 0), (1, 0), (0, 1), and (1, 1)
    ;
    ; Howver, if aligned on Y, don't include (0,1) and (1, 1)
    ; If aligned on X, don't include (1, 0) and (1, 1)
    ; So, if aligned on both X and Y, only include solid flag (0, 0)
	ld a, %1111
	jr nz, .HasYOffset
	and a, %0011
.HasYOffset:
	ld d, a ; Save solid flags for Y in d

	ld a, l
	and a, %00000111; These are the offset bits for X
	ld a, d
	jr nz, .HasXOffset
	and a, %0101
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
    
    call GetPlayerXInput
    ld e, a ; Save xSpeed in e

    ; DeltaX += XInput - DeltaX/8
    ld hl, DeltaX
	ld a, [hl]
	ld d, a
	sra d
	sra d
	sra d
	sub a, d
	add a, e
	ld [hl], a

    ;ld [DebugRam], a
    ;DebugPrintF "Diminished DeltaX: %hd"

    ; Get integer part of DeltaX (5.3 fixed point)
	sra a
	sra a
	bit 7, a
	jr z, :+
	dec a
:
	sra a
	ld e, a ; E <- DeltaX integer part

    call FlipPlayerSprite
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
	jp nc, .NoJump
	;dprint "Slack %A%"

	; Check if on ground
	ld a, [PlayerY] ; restore y
	ld c, a ; save y
	and a, %111
	ld a, 0
	jp nz, .NoJump ; Must be aligned with grid on Y to jump
	ld a, b ; restore X position
	ld l, a
	ld a, c ; restore y
	call CoordinateToMapOffset
	add a, CollisionMap >> 8
	ld h, a
	ld a, [hl]
	and a, %1100;0000
	ld c, a
	jp z, .NoJump
	ld a, b
	and a, %111
	ld a, c
	jr nz, .NoXoffset
	and %0100;0000
	jp z, .NoJump
.NoXoffset:
	xor a
	ld [PlayerJumpInputBuffering], a
	ld a, -JumpForce
	ld [DeltaY], a

    ;
    ; Play jump sound
    ;
    
    ; [rNR10]: 80
    ; [rNR11]: bf
    ; [rNR12]: f3
    ; [rNR13]: ff
    ; [rNR14]: bf

    push af
    ld a, %00110001
    ld [rNR10], a

    ld a, %10000110
    ld [rNR11], a ; 

    ld a, %11110101
    ld [rNR12], a ; 

    ld a, %11111111
    ld [rNR13], a ; 

    ld a, %11000001
    ld [rNR14], a ; Enable channel 1
    
    ld a, [rNR10]
    DebugPrintRegA "[rNR10]: %hx"

    ld a, [rNR11]
    DebugPrintRegA "[rNR11]: %hx"

    ld a, [rNR12]
    DebugPrintRegA "[rNR12]: %hx"

    ld a, [rNR13]
    DebugPrintRegA "[rNR13]: %hx"

    ld a, [rNR14]
    DebugPrintRegA "[rNR14]: %hx"

    pop af

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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
GetPlayerXInput:
;   Results: A <- Player X input
;   Clobbers: C
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
DEF JumpForce equ 28
DEF XSpeed equ 3

    ld a, [ButtonsDown]
    ld c, a

	xor a

	bit ButtonRight, c
	jr nz, :+
	add a, XSpeed
:

	bit ButtonLeft, c
	jr nz, :+
	add a, -XSpeed
:
    ret
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
FlipPlayerSprite:
; Takes: A as DeltaX
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	or a
	ret z ; Don't flip sprite if DeltaX == 0

    push af
	bit 7, a
	jr z, .FlipRight

.FlipLeft:
	ld a, Tile_Player_bl
	ld [SprPlayer.Tile], a
	ld a, Tile_Player_br
	ld [SprPlayer_2.Tile], a
	ld a, OAMF_PAL1
	ld [SprPlayer.Flags], a
	ld [SprPlayer_2.Flags], a
    pop af
	ret

    ; Flip Right
.FlipRight:
	ld a, Tile_Player_br
	ld [SprPlayer.Tile], a
	ld a, Tile_Player_bl
	ld [SprPlayer_2.Tile], a
	ld a, (OAMF_PAL1|OAMF_XFLIP)
	ld [SprPlayer.Flags], a
	ld [SprPlayer_2.Flags], a
    pop af
    ret

