@echo off

set INCLUDE=C:\PellesC\Include\;C:\PellesC\Include\Win\;
set LIB=C:\PellesC\Lib\;C:\PellesC\Lib\Win\;

set name=example
if exist %name%.exe del %name%.exe
if exist %name%.obj del %name%.obj

if "%1"=="PellesC" Goto PellesC
if "%1"=="LCC" Goto LCC
echo ************************************
echo *
echo *  CHOIX DU COMPILATEUR/LINKER
echo *
echo *  1. PellesC
echo *  2. LCC
echo *
echo ************************************
set /p test= Quel compilateur/linker ?
if %test%==1 goto PellesC
if %test%==2 goto LCC

:PellesC

echo ____________________________________
echo *
echo *  COMPILATION WITH POCC.EXE (Pelles C)
echo *
echo ____________________________________
\PellesC\bin\Pocc /Ze /W2 %name%.c
echo ____________________________________
echo *
echo *   LINK WITH POLINK.EXE (Pelles C)
echo *
echo ____________________________________
\PellesC\bin\PoLink /SUBSYSTEM:console %name%.obj ..\LIBRARY\BeaEngine.lib  kernel32.lib msvcrt.lib

goto fin

:LCC

echo ____________________________________
echo *
echo *  COMPILATION WITH LCC.EXE (LCC)
echo *
echo ____________________________________
\lcc\bin\lcc -O -I\lcc\include %name%.c
echo ____________________________________
echo *
echo *   LINK WITH LCCLNK.EXE (LCC)
echo *
echo ____________________________________
\lcc\bin\lcclnk -s -subsystem console %name%.obj ..\LIBRARY\BeaEngine.lib

:fin

if exist %name%.obj del %name%.obj

if "%1"=="NOPAUSE" Goto Sortie 
if "%2"=="NOPAUSE" Goto Sortie
pause
:Sortie





