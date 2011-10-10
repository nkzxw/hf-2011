#pragma once
#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "../callback.h"

#include "../../../COM/registercomcomponent.h"
#include "../../../File/StdFileOperations.h"
#include "../../../String/ansiunicodeconvert.h"

#define DIA_DLL_NAME _T("msdia80.dll")
class CHasDebugInfos
{
public:
    static BOOL HasDebugInfos(TCHAR* FileName);
};
