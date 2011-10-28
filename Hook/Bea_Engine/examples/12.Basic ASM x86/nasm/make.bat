@echo off
set name=example

set INCLUDE=C:\PellesC\Include\;C:\PellesC\Include\Win\;
set LIB=C:\PellesC\Lib\;C:\PellesC\Lib\Win\;

if exist %name%.exe del %name%.exe
if exist %name%.obj del %name%.obj
echo ____________________________________
echo *
echo *  COMPILATION WITH NASM.EXE (NASM)
echo *
echo ____________________________________
\nasm\bin\nasmw -fwin32 %name%.asm
echo ____________________________________
echo *
echo *   LINK WITH POLINK (PellesC)
echo *
echo ____________________________________
\PellesC\bin\PoLink /ENTRY:start /SUBSYSTEM:console %name%.obj ..\..\LIBRARY\BeaEngine.lib msvcrt.lib kernel32.lib



if exist %name%.obj del %name%.obj
pause