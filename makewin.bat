@echo off
SET PATH=C:\MinGW\bin;%PATH%
cd C:\fceuxd\
make -s -f Makefile.win
copy fceuxd.exe C:\nes\fceu\fceuxd.exe
pause
cls