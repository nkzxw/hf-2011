#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CKernelProcessesInfo
{
public:
    #define MAXIMUM_FILENAME_LENGTH 256
    typedef struct tagMODULE_INFO
    {
        DWORD   dwReservedl;
        DWORD   dwReserved2;
        PVOID   pBase;
        DWORD   dwSize;
        DWORD   dwFlags;
        WORD    wIndex;
        WORD    wRank;
        WORD    wLoadCount;
        WORD    wNameOffset;
        BYTE    pbPath[MAXIMUM_FILENAME_LENGTH];
    }MODULE_INFO,*PMODULE_INFO;

#pragma warning (push)
#pragma warning(disable : 4200)
    typedef struct tagMODULE_LIST
    {
        DWORD dwModules;
        MODULE_INFO pModulesInfo[];
    }MODULE_LIST,*PMODULE_LIST;
#pragma warning (pop)

    PMODULE_LIST pSystemModuleInformation;
    CKernelProcessesInfo(void);
    ~CKernelProcessesInfo(void);
    BOOL Update();
};
