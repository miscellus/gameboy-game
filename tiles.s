SECTION "Tiles",ROM0

TileData:
DEF Tile_Background0 EQU 0
dw $0000,$0000,$0000,$0000,$0000,$0000,$0000,$0000
DEF Tile_Background1 EQU 1
dw $0000,$0000,$0000,$0010,$0000,$0000,$0000,$0000
DEF Tile_Background2 EQU 2
dw $0040,$0000,$0000,$0000,$0001,$0000,$0000,$0000
DEF Tile_Wall_01 EQU 3
dw $fffe,$bfc2,$ffbe,$ffbe,$ffbe,$ffbe,$fffe,$ff01
DEF Tile_Wall_02 EQU 4
dw $ffff,$bfc3,$ffbf,$ffbf,$ffbf,$ffbf,$ffff,$ffff
DEF Tile_Wall_03 EQU 5
dw $ffff,$ffff,$ffff,$ffff,$ffff,$ffff,$ffff,$ffff
DEF Tile_CircleCorner_0 EQU 6
dw $0303,$0f0c,$1f10,$3f20,$3f20,$3f20,$1f10,$1f10
DEF Tile_CircleCorner_1 EQU 7
dw $1f10,$1f10,$3f20,$3f20,$1f10,$1f10,$0f0c,$0303

incbin "Images/player.2bpp"

DEF Tile_Player_tl EQU 8
;dw $0f0f,$3f00,$1926,$243f,$243f,$7f00,$7f40,$3f3f
DEF Tile_Player_bl EQU 9
;dw $0303,$0101,$0300,$0701,$0701,$0f01,$000e,$0e0e
DEF Tile_Player_tr EQU 10
;dw $8080,$c040,$e020,$f030,$f030,$f030,$f070,$e0e0
DEF Tile_Player_br EQU 11
;dw $c0c0,$e060,$e020,$f010,$7010,$7010,$0070,$7070
DEF Tile_Pattern1 EQU 12
dw $0002,$0040,$0008,$0001,$0020,$0004,$0080,$0010
DEF Tile_unused815 EQU 13
dw $51bd,$5560,$3750,$7876,$dd2b,$0c43,$1e6a,$9941
DEF Tile_unused113 EQU 14
dw $ec3c,$5253,$c1a8,$8e98,$8164,$7376,$b793,$1496
DEF Tile_unused805 EQU 15
dw $7b86,$69c9,$6621,$23b0,$351a,$9fb0,$0b81,$5ddd
DEF Tile_unused463 EQU 16
dw $435d,$2d4a,$9ab8,$8fe0,$4106,$660b,$1776,$54f4
DEF Tile_unused928 EQU 17
dw $e157,$4b7f,$3dd2,$43ed,$af0c,$a40a,$a3d0,$4ad9
DEF Tile_unused611 EQU 18
dw $5cb6,$e2fa,$26bb,$14dd,$84c1,$4186,$e5bc,$7847
DEF Tile_unused276 EQU 19
dw $6b1f,$986c,$24ea,$2d64,$d4ae,$aac5,$7d6d,$7a9d
DEF Tile_unused242 EQU 20
dw $8903,$b46e,$5d5d,$e3db,$cac5,$73b4,$6db3,$3552
DEF Tile_unused043 EQU 21
dw $6eec,$5dcd,$7c1f,$1d1d,$974d,$6a6a,$6f13,$3309
DEF Tile_unused039 EQU 22
dw $7945,$5fa1,$018b,$8820,$e1c6,$881a,$b25a,$1233
DEF Tile_unused686 EQU 23
dw $ff6f,$e288,$2d96,$7a64,$d134,$d953,$0be6,$d0bc
DEF Tile_unused196 EQU 24
dw $47b2,$d52e,$9378,$8607,$2617,$3452,$21f9,$b951
DEF Tile_unused893 EQU 25
dw $3ea1,$99ca,$fd63,$b931,$34f2,$f4f3,$3cab,$5898
DEF Tile_unused619 EQU 26
dw $111f,$b6b6,$1a75,$f17d,$8b1b,$1b5d,$37e4,$ab63
DEF Tile_unused812 EQU 27
dw $84f3,$a53f,$7724,$6e94,$f8bf,$ffa3,$54e0,$72c4
DEF Tile_unused471 EQU 28
dw $3426,$5f39,$df78,$59f0,$20d7,$b46b,$5b09,$8a11
DEF Tile_unused044 EQU 29
dw $4ae1,$2fcc,$e2ce,$ab7f,$8e07,$6b1d,$5de1,$12c9
DEF Tile_unused007 EQU 30
dw $c443,$c271,$9fbe,$f82d,$5d06,$3ba6,$e2d6,$922b
DEF Tile_Been EQU 31
dw $7e7e,$ff81,$ff81,$7e42,$7e42,$ff81,$ff81,$7e7e
TileDataEnd:
