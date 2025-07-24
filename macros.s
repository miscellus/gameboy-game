IF !DEF(JKK_MACROS)
DEF JKK_MACROS = 1

MACRO pusha
    push af
    push de
    push hl
    push bc
ENDM

MACRO popa
    pop bc
    pop hl
    pop de
    pop af
ENDM

; Prints a message to the no$gmb / bgb debugger
; Accepts a string as input, see emulator doc for support
MACRO dprint
	ld  d, d
	jr .end\@
	DW $6464
	DW $0000
	DB \1
.end\@:
ENDM

; printf expects the data to be pointed to by DE
MACRO printf
    push hl
    ld hl, .Fmt\@
    ld d, d
    jr .End\@
    dw $6464
    dw $0200
.End\@:
    pop hl
    jr :+
.Fmt\@: db \1, 0
:
ENDM

IF ASSERTIONS

MACRO assertz
	jr z, .End\@
	dprint \1
	die
.End\@
ENDM

MACRO assertnz
	jr nz, .End\@
	dprint \1
	die
.End\@
ENDM

MACRO assertc
	jr c, .End\@
	dprint \1
	die
.End\@
ENDM

MACRO assertnc
	jr nc, .End\@
	dprint \1
	die
.End\@
ENDM

ELSE
MACRO assertz
ENDM
MACRO assertnz
ENDM
MACRO assertc
ENDM
MACRO assertnc
ENDM

ENDC; DEF(ASSERTIONS) 

MACRO die
.DieLoop\@
	halt
	nop
	jr .DieLoop\@
ENDM

MACRO skipz
	jr z, .end\@
	\1,\2,\3,\4,\5,\6,\7,\8,\9
.end\@
ENDM

DEF ButtonA EQU 0
DEF ButtonB EQU 1
DEF ButtonSelect EQU 2
DEF ButtonStart EQU 3
DEF ButtonRight EQU 4
DEF ButtonLeft EQU 5
DEF ButtonUp EQU 6
DEF ButtonDown EQU 7

DEF BankRegister EQU $2000

ENDC ; JKK_MACROS
