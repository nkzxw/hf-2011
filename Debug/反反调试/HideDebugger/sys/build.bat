call d:\winddk\bin\setenv.bat d:\winddk fre WXP
d:
cd D:\exer\HideDebugger\sys
rd /S /Q objfre_wxp_x86
build
dir objfre_wxp_x86\i386\*.sys
copy objfre_wxp_x86\i386\drv.sys ..\tx
cd ..\tx
call make_sys_bin.exe
pause
