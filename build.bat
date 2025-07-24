SET RGBPATH=tools\rgbds
SET BGBPATH=tools\bgb
SET OUTPATH=build_files
%RGBPATH%\rgbasm.exe -o %OUTPATH%\main.obj main.s && ^
%RGBPATH%\rgblink.exe -m %OUTPATH%\main.map -n %OUTPATH%\main.sym -o %OUTPATH%\main.gb %OUTPATH%\main.obj && ^
%RGBPATH%\rgbfix.exe -p 0 -v %OUTPATH%\main.gb && ^
%BGBPATH%\bgb.exe %OUTPATH%\main.gb
