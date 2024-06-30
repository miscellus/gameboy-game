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
OldPlayerX: db
OldPlayerY: db

DeltaX: db
DeltaY: db

PlayerJumpInputBuffering: db
PlayerBuildingUpJumpForce: db

PlayerTileX: db
PlayerTileY: db

PlayerNumFramesInAir: db

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
