SET RGBPATH=..\rgbds-0-3-7
SET BGBPATH=..\BGB
%RGBPATH%\rgbasm.exe -o main.obj main.gbz80 && ^
%RGBPATH%\rgblink.exe -m main.map -n main.sym -o main.gb main.obj && ^
%RGBPATH%\rgbfix.exe -p 0 -v main.gb && ^
%BGBPATH%\bgb.exe main.gb