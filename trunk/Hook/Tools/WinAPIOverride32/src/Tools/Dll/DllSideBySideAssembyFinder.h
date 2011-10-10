#pragma once

#include <Windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CDllSideBySideAssembyFinder
{
public:
    static BOOL FindSideBySideAssemby(TCHAR* ImportingModulePath,TCHAR* tWorkingDirectory,TCHAR* tDllName,OUT TCHAR* tFullPath);
};

