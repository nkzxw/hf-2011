@echo on
@echo %1 is chk or fre %2 is WXP or W2K or WNET.

if "%3"=="/a" call my_clean %1 %2

pushd.
call %BASEDIR%\bin\setenv.bat %BASEDIR% %1 %2
popd

set INCLUDE=%INCLUDE%;%BASEDIR%\inc\ddk\;%BASEDIR%\inc\%2\

@echo on
build