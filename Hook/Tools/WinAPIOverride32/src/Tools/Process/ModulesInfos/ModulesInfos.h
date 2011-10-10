#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <vector>
#include "../Memory/ProcessMemory.h"

class CModulesInfos
{
public:
    CModulesInfos(void);
#ifndef _WIN64
    void SetCrossCompatibilityBinaryPath(TCHAR* CrossCompatibilityBinaryPath);
#endif

    virtual ~CModulesInfos(void);
    BOOL ParseProcess(DWORD ProcessId);
    BOOL CModulesInfos::GetModuleInfos(HMODULE hModule,SIZE_T* pEntryPoint,SIZE_T* pLoadCount,SIZE_T* pFlags,SIZE_T* pTlsIndex,SIZE_T* pTimeDateStamp);
    BOOL ReadProcessMemory(LPCVOID lpBaseAddress,LPVOID lpBuffer,SIZE_T nSize,SIZE_T* lpNumberOfBytesRead);

private:
    std::vector<PVOID> ModulesInfos;
    BOOL b32BitTargetProcess;
    typedef LONG (WINAPI *ptrNtQueryInformationProcess) (
        IN HANDLE ProcessHandle,
        IN SIZE_T ProcessInformationClass,
        OUT PVOID ProcessInformation,
        IN ULONG ProcessInformationLength,
        OUT PULONG ReturnLength OPTIONAL
        );

    template <class T> BOOL ParseProcessT(HANDLE hProcess);
    void FreeMemory();
    CProcessMemory* pProcessMemory;
    ptrNtQueryInformationProcess pNtQueryInformationProcess;

#ifndef _WIN64
    TCHAR CrossCompatibilityBinaryPath[MAX_PATH];
#endif

#ifndef _WIN64
public:
#else
private:
#endif
    template <class T> BOOL GetModuleInfosT(T hModule,T* pEntryPoint,SIZE_T* pLoadCount,SIZE_T* pFlags,SIZE_T* pTlsIndex,SIZE_T* pTimeDateStamp);
};