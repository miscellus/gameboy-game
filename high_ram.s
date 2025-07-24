DEF HRAM_BASE = 0x80

SECTION "High RAM - DmaRoutine", HRAM[0xff00 + HRAM_BASE]
HRAM_DmaRoutine:

SECTION "Stored High RAM To be copied", ROM0
StoredDma:
    push af
    ld a, 0xC1 ; Upper byte of target address for OAM: $C100
    ld [rDMA], a
    ld a, 40

    ; DMA transfer begins, we need to wait 160 microseconds while it transfers
    ; the following loop takes exactly that long
:   dec a
    jr nz, :-

    pop af
    reti
.End:
