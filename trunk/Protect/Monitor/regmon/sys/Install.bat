@REM This line is the copy that works when Filemon
@REM is built with the NT 4 DDK
copy %BASEDIR%\lib\%CPU%\%DDKBUILDENV%\regsys.sys .
@REM This line is the copy that works when Filemon
@REM is built with the Win2K DDK
copy obj%BUILD_ALT_DIR%\%CPU%\regsys.sys .
copy obj%BUILD_ALT_DIR%\I386\regswnet.sys .
@REM This line works when Filemon is built for 64-bit
copy obj%BUILD_ALT_DIR%\%_BUILDARCH%\regsys.sys .
copy obj%BUILD_ALT_DIR%\%_BUILDARCH%\regsys.pdb .

copy regsys.sys ..\exe\release\.
copy regswnet.sys ..\exe\release\.
copy regsys.sys ..\exe\debug\.
copy regswnet.sys ..\exe\release\.
rmdir /q /s obj objfre objchk
del *.log


