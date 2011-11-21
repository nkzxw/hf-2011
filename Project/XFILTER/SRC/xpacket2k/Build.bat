if "%1" == "" goto InvalidParameter

if not exist %1\bin\setenv.bat goto SetenvNotFound

call %1\bin\setenv.bat %1 %2
%3
cd %4
build.exe -cZ
if "%2" == "checked" goto CopyChecked
if "%2" == "free" goto CopyFree
goto exit

:CopyChecked
copy .\objchk\i386\xpacket.sys ..\release\xpacket.sys
goto exit

:CopyFree
copy .\objfre\i386\xpacket.sys ..\release\xpacket.sys
goto exit

:InvalidParameter
echo Invalid Parameter.
goto exit

:SetenvNotFound
echo Can't found Setenv.bat.
goto exit

:exit