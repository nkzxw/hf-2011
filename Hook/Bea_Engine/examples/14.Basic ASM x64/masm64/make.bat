@echo off
set INCLUDE=C:\Program Files\PellesC\Include\;C:\Program Files\PellesC\Include\Win\;
set LIB=C:\Program Files\PellesC\Lib\;C:\Program Files\PellesC\Lib\Win64\;
set name=example_x64

if exist %name%.exe del %name%.exe
if exist %name%.obj del %name%.obj

echo ____________________________________
echo *
echo *   ASSEMBLE WITH MASM64
echo *
echo ____________________________________

C:\AMD64\ml64 /c %name%.asm

echo ____________________________________
echo *
echo *   LINK WITH POLINK (Pelles C)
echo *
echo ____________________________________

"\Program Files\PellesC\bin\PoLink" /SUBSYSTEM:console /MACHINE:X64 %name%.obj ..\..\LIBRARY\BeaEngine64.lib kernel32.lib crt.lib

if exist %name%.obj del %name%.obj


echo.
echo.
echo.
echo Compilation terminee !!!

pause