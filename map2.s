; MAP2.Z80
;
; Map Source File.
;
; Info:
;   Section       : ROM0
;   Bank          : 0
;   Map size      : 30 x 30
;   Tile set      : C:\Documents and Settings\me\Desktop\Games\GB\TileDesigner\test.gbr
;   Plane count   : 2 planes (16 bits)
;   Plane order   : Tiles are continues
;   Tile offset   : 0
;   Split data    : Yes
;   Block size    : 1024
;   Inc bank      : No
;
; This file was generated by GBMB v1.8

MapWidth  EQU 30
MapHeight EQU 30
MapBank   EQU 0

 SECTION "Map", ROM0[$600]

; IMPORTANT(jakob): Maps must be 256-byte aligned

MapData:
DB $02,$02,$02,$02,$02,$02,$02,$02,$02,$02
DB $02,$02,$02,$02,$02,$02,$02,$02,$02,$02
DB $02,$02,$02,$02,$02,$02,$02,$02,$02,$02
DB $02,$02,$02,$02,$02,$02,$02,$02,$03,$03
DB $03,$03,$03,$03,$03,$03,$03,$03,$03,$02
DB $02,$02,$02,$03,$03,$03,$02,$02,$02,$02
DB $02,$02,$02,$02,$03,$02,$00,$00,$00,$00
DB $00,$00,$00,$00,$00,$00,$00,$00,$00,$00
DB $02,$02,$03,$03,$03,$03,$03,$02,$02,$02
DB $02,$02,$01,$01,$03,$03,$02,$00,$00,$00
DB $00,$00,$00,$00,$00,$00,$00,$00,$00,$00
DB $00,$02,$00,$00,$00,$00,$00,$02,$02,$02
DB $02,$02,$01,$01,$03,$03,$02,$00,$00,$00
DB $00,$00,$02,$00,$00,$00,$00,$00,$00,$00
DB $00,$00,$00,$00,$00,$00,$00,$00,$02,$02
DB $02,$02,$01,$01,$02,$02,$00,$00,$00,$00
DB $00,$00,$03,$00,$00,$00,$00,$02,$02,$02
DB $02,$02,$02,$02,$00,$00,$00,$00,$00,$02
DB $02,$02,$01,$01,$02,$00,$00,$00,$00,$00
DB $00,$00,$00,$00,$00,$00,$02,$02,$02,$02
DB $02,$02,$02,$02,$00,$00,$00,$00,$00,$02
DB $02,$00,$00,$00,$02,$00,$00,$00,$00,$00
DB $00,$00,$00,$00,$00,$00,$01,$01,$01,$01
DB $01,$01,$02,$02,$00,$00,$00,$00,$00,$02
DB $02,$00,$00,$00,$00,$00,$00,$02,$02,$00
DB $00,$00,$00,$00,$00,$01,$01,$01,$01,$01
DB $01,$01,$02,$00,$00,$00,$00,$00,$00,$02
DB $02,$00,$00,$00,$00,$00,$02,$01,$02,$02
DB $00,$00,$00,$00,$02,$02,$02,$02,$02,$02
DB $02,$02,$02,$00,$00,$00,$00,$00,$02,$02
DB $02,$00,$00,$00,$00,$02,$00,$00,$02,$02
DB $02,$00,$00,$00,$02,$02,$02,$02,$02,$02
DB $02,$02,$00,$00,$00,$00,$00,$00,$02,$02
DB $02,$00,$02,$02,$02,$00,$00,$00,$00,$00
DB $00,$00,$00,$00,$00,$02,$02,$02,$02,$02
DB $00,$00,$00,$00,$00,$00,$00,$02,$02,$02
DB $02,$00,$02,$02,$00,$00,$00,$00,$00,$00
DB $00,$00,$00,$00,$02,$02,$02,$02,$02,$02
DB $02,$00,$00,$00,$00,$00,$00,$02,$02,$02
DB $02,$00,$00,$00,$00,$00,$00,$00,$00,$00
DB $00,$00,$00,$02,$02,$02,$02,$03,$03,$02
DB $02,$02,$02,$00,$00,$00,$02,$02,$02,$02
DB $02,$00,$00,$00,$00,$00,$00,$00,$00,$00
DB $00,$00,$00,$02,$02,$02,$03,$03,$03,$03
DB $02,$02,$02,$02,$00,$00,$02,$02,$02,$02
DB $02,$00,$00,$00,$00,$00,$00,$00,$00,$00
DB $00,$00,$00,$01,$01,$00,$00,$00,$00,$00
DB $00,$02,$02,$02,$00,$00,$00,$00,$02,$02
DB $02,$00,$00,$00,$00,$00,$00,$00,$00,$00
DB $00,$00,$00,$01,$01,$02,$00,$00,$00,$00
DB $00,$02,$02,$02,$00,$00,$00,$00,$02,$02
DB $02,$00,$00,$00,$02,$02,$02,$02,$00,$00
DB $00,$00,$02,$02,$02,$02,$02,$00,$00,$00
DB $00,$00,$00,$02,$02,$00,$00,$00,$02,$02
DB $02,$00,$00,$02,$02,$02,$02,$02,$02,$02
DB $00,$00,$00,$00,$02,$02,$02,$02,$00,$00
DB $00,$00,$00,$00,$00,$00,$00,$02,$02,$02
DB $02,$00,$02,$03,$03,$03,$03,$03,$02,$02
DB $02,$00,$00,$00,$03,$03,$02,$02,$00,$00
DB $00,$00,$00,$00,$00,$00,$00,$02,$02,$02
DB $02,$00,$02,$03,$03,$03,$03,$03,$03,$02
DB $02,$02,$00,$00,$00,$00,$02,$02,$02,$00
DB $00,$00,$00,$00,$00,$02,$02,$02,$02,$02
DB $02,$02,$03,$03,$03,$03,$03,$03,$03,$02
DB $02,$02,$02,$00,$00,$00,$03,$03,$02,$00
DB $00,$00,$00,$00,$00,$03,$03,$03,$02,$02
DB $02,$02,$03,$03,$03,$03,$03,$03,$03,$02
DB $02,$02,$02,$00,$00,$00,$00,$00,$02,$02
DB $02,$02,$02,$00,$00,$00,$00,$00,$02,$02
DB $02,$00,$00,$00,$00,$00,$00,$00,$00,$01
DB $02,$02,$01,$00,$00,$00,$00,$00,$00,$02
DB $02,$02,$02,$00,$00,$00,$00,$00,$02,$02
DB $02,$00,$00,$00,$00,$00,$00,$00,$00,$01
DB $01,$01,$01,$00,$00,$00,$00,$00,$02,$02
DB $02,$02,$00,$00,$00,$00,$00,$00,$02,$02
DB $02,$02,$00,$00,$00,$00,$00,$00,$00,$01
DB $01,$01,$01,$00,$00,$00,$00,$00,$02,$02
DB $02,$00,$00,$00,$00,$00,$02,$02,$02,$02
DB $02,$02,$02,$02,$02,$00,$00,$00,$00,$01
DB $01,$01,$01,$00,$00,$00,$00,$00,$02,$02
DB $00,$00,$00,$00,$00,$00,$02,$02,$02,$02
DB $02,$02,$02,$02,$00,$00,$00,$02,$02,$02
DB $01,$01,$01,$00,$00,$00,$00,$00,$02,$00
DB $00,$00,$00,$00,$00,$02,$02,$02,$02,$02
DB $02,$02,$02,$02,$00,$00,$00,$03,$03,$03
DB $01,$01,$01,$00,$00,$00,$00,$00,$00,$00
DB $00,$00,$02,$02,$02,$02,$02,$02,$02,$02
DB $02,$02,$02,$02,$02,$02,$02,$02,$02,$02
DB $02,$02,$02,$02,$02,$02,$02,$02,$02,$02
DB $02,$02,$02,$02,$02,$02,$02,$02,$02,$02
MapDataEnd:
; End of MAP2.Z80
