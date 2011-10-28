@echo off

set C_INCLUDE_PATH=C:\MinGW\include\ddk\;
set LIB=C:\PellesC\Lib\;C:\PellesC\Lib\Win\;C:\WINDDK\3790.1830\lib\wxp\i386;..\..\LIBRARY\;

set name=example
if exist %name%.exe del %name%.exe
if exist %name%.obj del %name%.obj

echo ____________________________________
echo *
echo *  COMPILATION WITH GCC.EXE (MinGW)
echo *
echo ____________________________________
\MinGW\bin\gcc -c %name%.c -o %name%.obj 

echo ____________________________________
echo *
echo *   LINK WITH POLINK.EXE (Pelles C)
echo *
echo ____________________________________
\PellesC\bin\PoLink /SUBSYSTEM:NATIVE /DRIVER  %name%.obj  BeaEngine.lib ntoskrnl.lib /OUT:%name%.sys 



if exist %name%.obj del %name%.obj

if "%1"=="NOPAUSE" Goto Sortie 
if "%2"=="NOPAUSE" Goto Sortie
pause
:Sortie






