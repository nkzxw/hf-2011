@echo off

echo service
net stop tdifw
%SystemRoot%\system32\tdifw.exe remove
del %SystemRoot%\system32\tdifw.exe

echo driver
bin\install remove
del %SystemRoot%\system32\drivers\tdifw_drv.sys

rem echo config and log files
rem del /Q %SystemRoot%\system32\LogFiles\tdifw > nul
rem rd %SystemRoot%\system32\LogFiles\tdifw > nul
rem del %SystemRoot%\system32\drivers\etc\tdifw.conf > nul

echo.
echo Restart Windows to unload driver from memory.
pause
