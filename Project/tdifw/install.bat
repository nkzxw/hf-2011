@echo off

md %SystemRoot%\system32\LogFiles\tdifw > nul

echo default config
copy /y tdifw.conf %SystemRoot%\system32\drivers\etc > nul

echo driver
copy /y bin\tdifw_drv.sys %SystemRoot%\system32\drivers > nul
bin\install install

echo service
copy bin\tdifw.exe %SystemRoot%\system32 > nul
%SystemRoot%\system32\tdifw.exe install %SystemRoot%\system32\drivers\etc\tdifw.conf

echo.
echo Change %SystemRoot%\system32\drivers\etc\tdifw.conf file for your taste and restart Windows.
pause
