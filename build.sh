#!/bin/bash
rgbasm -o main.obj main.z80 && \
rgblink -m main.map -n main.sym -o main.gb main.obj && \
rgbfix -p 0 -v main.gb && \
STEAM_COMPAT_DATA_PATH=~/.proton/ ~/.steam/steam/steamapps/common/Proton*/proton run ../BGB/bgb.exe main.gb
