@echo off

set PATH=\PellesC\bin;
set LIB=C:\PellesC\Lib\;C:\PellesC\Lib\Win\;
set name=example

if exist %name%.exe del %name%.exe
if exist %name%.obj del %name%.obj

echo ____________________________________
echo *
echo *  COMPILATION WITH FASM.EXE (FASM 1.67)
echo *
echo ____________________________________
\fasm\fasm %name%.asm
echo ____________________________________
echo *
echo *   LINK WITH POLINK (PellesC)
echo *
echo ____________________________________
PoLink /ENTRY:start /SUBSYSTEM:console %name%.obj ..\..\LIBRARY\BeaEngine.lib msvcrt.lib kernel32.lib

if exist %name%.obj del %name%.obj
pause