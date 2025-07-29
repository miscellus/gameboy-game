IF DEF(DEBUG)
    MACRO TODO
        ld  d, d
        jr .End\@
        DW $6464
        DW $0000
        DB \1
    .End\@:
        DebugBreak
    ENDM

    ; Prints a message to the no$gmb / bgb debugger
    ; Accepts a string as input, see emulator doc for support
    MACRO DebugPrint
        ld  d, d
        jr .end\@
        DW $6464
        DW $0000
        DB \1
    .end\@:
    ENDM

    MACRO DebugPrintF
        push hl
        push de
        ld de, DebugRam
        ld hl, .Fmt\@
        ld d, d
        jr .End\@
        dw $6464
        dw $0200
    .End\@:
        pop de
        pop hl
        jr .AfterFmt\@
    .Fmt\@:
        db \1, 0
    .AfterFmt\@:
    ENDM

    MACRO DebugPrintRegA
        push hl
        push de
        ld [DebugRam], a
        ld de, DebugRam
        ld hl, .Fmt\@
        ld d, d
        jr .End\@
        dw 0x6464
        dw 0x0200
    .End\@:
        pop de
        pop hl
        jr .AfterFmt\@
    .Fmt\@:
        db \1, 0
    .AfterFmt\@:
    ENDM

    MACRO DebugBreak
        ld b, b
    ENDM

    MACRO DebugExpect
        jr \1, .End\@
        dprint \2
        DebugBreak
    .End\@
    ENDM

SECTION "Debug Ram", WRAM0
DebugRam::
DS 128

ELSE

    MACRO TODO
    ENDM

    MACRO DebugPrint
    ENDM

    MACRO DebugPrintF
    ENDM

    MACRO DebugBreak
    ENDM

    MACRO DebugExpect
    ENDM

ENDC

