SECTION "RAM Variables",WRAM0[$C000]
IsWaitingForVBlank: db
VBlankCount: db

; bits:
;down/up/left/rigt/start/select/a/b
ButtonsDown: db
ButtonsPressed: db
ButtonsReleased: db

PlayerX: db
PlayerY: db

DeltaX: db
DeltaY: db

PlayerJumpInputBuffering: db

DebugStack:
REPT 64
    db
ENDR

SECTION "OAM Work Memory",WRAM0,ALIGN[8]
Sprites:
SprPlayer:
.Y: db
.X: db
.Tile: db
.Flags: db
SprPlayer_2:
.Y: db
.X: db
.Tile: db
.Flags: db
