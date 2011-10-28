@echo off

set LIB=../../LIBRARY
set INCLUDE=../../HEADERS
set name=example_x64

if exist %name%.exe del %name%.exe
if exist %name%.obj del %name%.obj
echo ____________________________________
echo *
echo *  COMPILATION WITH GOASM.EXE (GoAsm)
echo *
echo ____________________________________
\GoAsm\Goasm /x64 %name%.asm


echo ____________________________________
echo *
echo *   LINK WITH GOLINK 
echo *
echo ____________________________________

\GoAsm\GoLink /console %name%.obj kernel32.dll msvcrt.dll

if exist %name%.obj del %name%.obj

pause


