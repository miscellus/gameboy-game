SECTION "Memory Subroutines",ROM0

; hl = Source address
; de = Destination address
; bc = Byte count
MemCopy:
	inc c
	inc b
	jr .begin
.loop:
	ld a, [hl+]
	ld [de], a
	inc de
.begin:
	dec c
	jr nz, .loop
	dec b
	jr nz, .loop
	ret

; hl = Source address
; de = Destination address
; c = Byte count
MemCopy255:
	or c
	ret z
.loop:
	ld a, [hl+]
	ld [de], a
	inc de
	dec c
	jr nz, .loop
	ret

; hl = Destination address
; a = value
; bc = Byte count
MemFill:
	inc c
	inc b
	jr .begin
.loop:
	ld [hl+], a
.begin:
	dec c
	jp nz, .loop
	dec b
	jp nz, .loop
	ret

; hl = Destination address
; a = value
; c = Byte count
MemFill255:
	or c
	ret z
.loop
	ld  [hl+],a
	dec c
	jr  nz,.loop
	ret