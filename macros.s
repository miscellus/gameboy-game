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
