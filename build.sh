#!/bin/bash
python genmap.py > map.s
#python gentiles.py > tiles.s
rgbasm -o main.obj main.s && \
rgblink -m main.map -n main.sym -o main.gb main.obj && \
rgbfix -p 0 -v main.gb && \
#mednafen main.gb
STEAM_COMPAT_DATA_PATH=~/.proton/ ~/.steam/steam/steamapps/common/Proton*/proton run ../BGB/bgb.exe main.gb
