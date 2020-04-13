IF !DEF(JKK_MACROS)
JKK_MACROS SET 1

Increment: MACRO
	ld  a, [\1]
	inc a
	ld  [\1], a
ENDM

AddTo: MACRO
	ld  a, [\1]
	add a, \2
	ld [\1], a
ENDM

Decrement: MACRO
	ld  a, [\1]
	dec a
	ld  [\1], a
ENDM

LeafCall: MACRO
	ld hl, .return\@
	jp \1
.return\@
ENDM

LeafCallRelative: MACRO
	ld hl, .return\@
	jr \1
.return\@
ENDM

; Prints a message to the no$gmb / bgb debugger
; Accepts a string as input, see emulator doc for support
dprint: MACRO
	ld  d, d
	jr .end\@
	DW $6464
	DW $0000
	DB \1
.end\@:
ENDM

IF ASSERTIONS

assertz: MACRO
	jr z, .End\@
	dprint \1
	die
.End\@
ENDM

assertnz: MACRO
	jr nz, .End\@
	dprint \1
	die
.End\@
ENDM

assertc: MACRO
	jr c, .End\@
	dprint \1
	die
.End\@
ENDM

assertnc: MACRO
	jr nc, .End\@
	dprint \1
	die
.End\@
ENDM

ELSE
assertz: MACRO ENDM
assertnz: MACRO ENDM
assertc: MACRO ENDM
assertnc: MACRO ENDM

ENDC; DEF(ASSERTIONS) 

die: MACRO
.DieLoop\@
	halt
	nop
	jr .DieLoop\@
ENDM

skipz: MACRO
	jr z, .end\@
	\1,\2,\3,\4,\5,\6,\7,\8,\9
.end\@
ENDM

ButtonA equ 0
ButtonB equ 1
ButtonSelect equ 2
ButtonStart equ 3
ButtonRight equ 4
ButtonLeft equ 5
ButtonUp equ 6
ButtonDown equ 7

BankRegister equ $2000

ENDC ; JKK_MACROS