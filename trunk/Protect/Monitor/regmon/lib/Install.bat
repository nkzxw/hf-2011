@REM This line is the copy that works when Filemon
@REM is built with the NT 4 DDK
copy %BASEDIR%\lib\%CPU%\%DDKBUILDENV%\regmlib.lib ..\sys\.
@REM This line is the copy that works when Regmon
@REM is built with the Win2K DDK
copy obj%BUILD_ALT_DIR%\%CPU%\regmlib.lib ..\sys\.
@REM This line works when Filemon is built for 64-bit
copy obj%BUILD_ALT_DIR%\%_BUILDARCH%\regmlib.lib ..\sys\.

rmdir /q /s obj objfre objchk
del *.log


