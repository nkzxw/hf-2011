@echo off

if exist %SystemRoot%\system32\psapi.dll goto m1
copy bin\psapi.dll %SystemRoot%\system32\psapi.dll
:m1

md %SystemRoot%\system32\LogFiles\tdifw > nul

echo default config
copy tdifw.conf %SystemRoot%\system32\drivers\etc > nul

echo driver
copy bin\tdifw_drv.sys %SystemRoot%\system32\drivers > nul
regedit /s install_nt4.reg

echo service
copy bin\tdifw.exe %SystemRoot%\system32 > nul
%SystemRoot%\system32\tdifw.exe install %SystemRoot%\system32\drivers\etc\tdifw.conf

echo.
echo Change %SystemRoot%\system32\drivers\etc\tdifw.conf file for your taste and restart Windows.
pause
