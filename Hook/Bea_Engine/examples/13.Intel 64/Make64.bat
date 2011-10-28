@echo off

set INCLUDE=C:\Program Files\PellesC\Include\;C:\Program Files\PellesC\Include\Win\;
set LIB=C:\Program Files\PellesC\Lib\;C:\Program Files\PellesC\Lib\Win64\;

set name=example
if exist %name%.exe del %name%.exe
if exist %name%.obj del %name%.obj

:PellesC

echo ____________________________________
echo *
echo *  COMPILATION WITH POCC.EXE (Pelles C)
echo *
echo ____________________________________
"\Program Files\PellesC\bin\pocc" /Tamd64-coff /Ze /W2 %name%.c

echo ____________________________________
echo *
echo *   LINK WITH POLINK.EXE (Pelles C)
echo *
echo ____________________________________
"\Program Files\PellesC\bin\PoLink" /SUBSYSTEM:console /MACHINE:X64 %name%.obj BeaEngine64.lib kernel32.lib

if exist %name%.obj del %name%.obj
pause



