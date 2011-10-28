@echo off

set drv=example
set C_INCLUDE_PATH=C:\MinGW\include\ddk\;
set LIB=C:\PellesC\Lib\;C:\PellesC\Lib\Win\;C:\WINDDK\3790.1830\lib\wxp\i386;..\..\LIBRARY\;

\masm32\bin\ml /nologo /c /coff %drv%.asm
\PellesC\bin\PoLink /SUBSYSTEM:NATIVE /DRIVER  %drv%.obj  BeaEngine.lib ntoskrnl.lib /OUT:%drv%.sys

rem \masm32\bin\link /nologo /driver /base:0x10000 /align:32 /out:%drv%.sys /subsystem:native %drv%.obj

del %drv%.obj

if "%1"=="NOPAUSE" Goto Sortie 
if "%2"=="NOPAUSE" Goto Sortie
pause
:Sortie

