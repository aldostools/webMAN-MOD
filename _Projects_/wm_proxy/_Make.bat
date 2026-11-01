@echo off
cls
set CELL_SDK=/c/msys/1.0/cell_4.50
set PS3SDK=/c/PSDK3v2
set PS3DEV=/c/PSDK3v2/ps3dev2
set WIN_PS3SDK=C:/PSDK3v2
SET PATH=%WIN_PS3SDK%/mingw/msys/1.0/bin;%WIN_PS3SDK%/mingw/bin;%PS3DEV%/ppu/bin;%PATH%;

//make clean

make all

pause