#include "ModulesInfos.h"
#include "../ProcessHelper/ProcessHelper.h"
#include "../PEB_TEB/PEB_TEB.h"
// #include <winternl.h>

#ifndef _WIN64
#include "../x86CrossCompatibility/x86CrossCompatibility.h"
#endif

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

CModulesInfos::CModulesInfos(void)
{
    this->b32BitTargetProcess = FALSE;
    this->pProcessMemory = NULL;
    this->pNtQueryInformationProcess=(ptrNtQueryInformationProcess)GetProcAddress(GetModuleHandle(_T("Ntdll.dll")),"NtQueryInformationProcess");
#ifndef _WIN64
    *this->CrossCompatibilityBinaryPath=0;
#endif
}
CModulesInfos::~CModulesInfos(void)
{
    this->FreeMemory();
}
void CModulesInfos::FreeMemory()
{
    if (this->pProcessMemory)
    {
        delete this->pProcessMemory;
        this->pProcessMemory = NULL;
    }

    std::vector<PVOID>::const_iterator it;
    PVOID Pointer;
    for( it = this->ModulesInfos.begin(); it != this->ModulesInfos.end(); ++it)
    {
        Pointer = (PVOID)*it;
        delete[] Pointer;
    }
    this->ModulesInfos.clear();
    this->b32BitTargetProcess = FALSE;
}

#ifndef _WIN64
void CModulesInfos::SetCrossCompatibilityBinaryPath(TCHAR* CrossCompatibilityBinaryPath)
{
    _tcsncpy(this->CrossCompatibilityBinaryPath,CrossCompatibilityBinaryPath,_countof(this->CrossCompatibilityBinaryPath));
}
#endif

BOOL CModulesInfos::ParseProcess(DWORD ProcessId)
{
    if (!this->pNtQueryInformationProcess)
        return FALSE;
    this->FreeMemory();

    this->pProcessMemory = new CProcessMemory(ProcessId,TRUE,TRUE);
    HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,ProcessId);
    CProcessHelper::Is32bitsProcessHandle(hProcess,&this->b32BitTargetProcess);
    if (this->b32BitTargetProcess)
    {
        return this->ParseProcessT<UINT32>(hProcess);
    }
    else
    {
#ifdef _WIN64
		return this->ParseProcessT<UINT64>(hProcess);
#else
        ::CloseHandle(hProcess);

		// it would be nice but NtQueryInformationProcess call from 32 bit process querying 64 bit process infos fails :
		// in fact NtQueryInformationProcess return success, but pbi.PebBaseAddress equal 0. So we can do nothing with such bad value
		// --> create a 64 bit process to get informations
        if (*this->CrossCompatibilityBinaryPath==0)
        {
#ifdef _DEBUG         
            // a file name must be specified for cross compatibility with SetCrossCompatibilityBinaryPath
            ::DebugBreak();
#endif
            return FALSE;
        }
        
        BOOL bRetValue = FALSE;
        x86CrossCompatibility::Cx86CrossCompatibility CrossCompatibility(this->CrossCompatibilityBinaryPath);
        HANDLE64 hProcess64 = CrossCompatibility.OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION,FALSE,ProcessId);
        CTemplatePebTeb<UINT64>::PROCESS_BASIC_INFORMATIONT pbi;

        if ( CrossCompatibility.NtQueryInformationProcess(hProcess64,ProcessBasicInformation,&pbi,sizeof(pbi),NULL)
             <0
             )
            goto CleanUp;

        CTemplatePebTeb<UINT64>::PEBT Peb;
        __int64 ReadSize64;

        if ( !CrossCompatibility.ReadProcessMemory(hProcess64, pbi.PebBaseAddress, &Peb, sizeof(Peb), &ReadSize64) )
            goto CleanUp;

        CTemplatePebTeb<UINT64>::PEB_LDR_DATAT PebLdrData;
        if ( !CrossCompatibility.ReadProcessMemory(hProcess64, Peb.LoaderData, &PebLdrData, sizeof(PebLdrData), &ReadSize64) )
            goto CleanUp;

        CTemplatePebTeb<UINT64>::LDR_MODULET* pPebLdrModule;
        void* ReadAddr = (void*) PebLdrData.InLoadOrderModuleList.Flink;

        // Go through each modules one by one in their load order.
        while( ReadAddr )
        {
            pPebLdrModule = new CTemplatePebTeb<UINT64>::LDR_MODULET();
            if (!CrossCompatibility.ReadProcessMemory(hProcess64, (UINT64)ReadAddr, pPebLdrModule, sizeof(*pPebLdrModule), &ReadSize64))
                 break;

            // list is looping (last element (which is invalid) points to the first one
            // --> detect invalid element to avoid infinite loop
            if (pPebLdrModule->BaseAddress == 0) // && (PebLdrModule.Flags == 0) && (PebLdrModule.TimeDateStamp == 0xF000)
                 break;

            this->ModulesInfos.push_back(pPebLdrModule);

            ReadAddr = (void *) pPebLdrModule->InLoadOrderModuleList.Flink;
        }
        bRetValue = TRUE;
CleanUp:
        if (hProcess64)
            CrossCompatibility.CloseHandle(hProcess64);
        return bRetValue;

#endif       
    }
}

template <class T> BOOL CModulesInfos::ParseProcessT(HANDLE hProcess)
{
    CTemplatePebTeb<T>::PROCESS_BASIC_INFORMATIONT pbi;
    if ( this->pNtQueryInformationProcess(hProcess,ProcessBasicInformation,&pbi,sizeof(pbi),NULL)
         <0
        )
    {
        ::CloseHandle(hProcess);
        return FALSE;
    }
    ::CloseHandle(hProcess);

    CTemplatePebTeb<T>::PEBT Peb;
    SIZE_T ReadSize;
    if ( !this->pProcessMemory->Read((LPCVOID)pbi.PebBaseAddress, &Peb, sizeof(Peb), &ReadSize) )
        return FALSE;

    CTemplatePebTeb<T>::PEB_LDR_DATAT PebLdrData;
    if ( !this->pProcessMemory->Read((LPCVOID)Peb.LoaderData, &PebLdrData, sizeof(PebLdrData), &ReadSize) )
        return FALSE;

    CTemplatePebTeb<T>::LDR_MODULET* pPebLdrModule;
    void* ReadAddr = (void*) PebLdrData.InLoadOrderModuleList.Flink;

    // Go through each modules one by one in their load order.
    while( ReadAddr )
    {
        pPebLdrModule = new CTemplatePebTeb<T>::LDR_MODULET();
        if (!this->pProcessMemory->Read((LPCVOID)ReadAddr, pPebLdrModule, sizeof(*pPebLdrModule), &ReadSize))
            break;

        // list is looping (last element (which is invalid) points to the first one
        // --> detect invalid element to avoid infinite loop
        if (pPebLdrModule->BaseAddress == 0) // && (PebLdrModule.Flags == 0) && (PebLdrModule.TimeDateStamp == 0xF000)
            break;

        this->ModulesInfos.push_back(pPebLdrModule);

        ReadAddr = (void *) pPebLdrModule->InLoadOrderModuleList.Flink;
    }

    return TRUE;
}

BOOL CModulesInfos::ReadProcessMemory(LPCVOID lpBaseAddress,LPVOID lpBuffer,SIZE_T nSize,SIZE_T* lpNumberOfBytesRead)
{
    if (!this->pProcessMemory)
        return FALSE;
    return this->pProcessMemory->Read(lpBaseAddress,lpBuffer,nSize,lpNumberOfBytesRead);
}

BOOL CModulesInfos::GetModuleInfos(HMODULE hModule,SIZE_T* pEntryPoint,SIZE_T* pLoadCount,SIZE_T* pFlags,SIZE_T* pTlsIndex,SIZE_T* pTimeDateStamp)
{
    if (this->b32BitTargetProcess)
    {
        return this->GetModuleInfosT<UINT32>( (UINT32)hModule, (UINT32*)pEntryPoint, pLoadCount, pFlags, pTlsIndex, pTimeDateStamp);
    }
    else
    {
#ifndef _WIN64
#ifdef _DEBUG
        ::DebugBreak(); // GetModuleInfosT should be called instead of this function
#endif
        return FALSE;
        // stupid stuf only to force GetModuleInfosT<UINT64> to be generated
        return this->GetModuleInfosT<UINT64>( (UINT64)hModule, (UINT64*)pEntryPoint, pLoadCount, pFlags, pTlsIndex, pTimeDateStamp);
#else
		return this->GetModuleInfosT<UINT64>( (UINT64)hModule, (UINT64*)pEntryPoint, pLoadCount, pFlags, pTlsIndex, pTimeDateStamp);
#endif
    }
}
template <class T> BOOL CModulesInfos::GetModuleInfosT(T hModule,T* pEntryPoint,SIZE_T* pLoadCount,SIZE_T* pFlags,SIZE_T* pTlsIndex,SIZE_T* pTimeDateStamp)
{
    std::vector<PVOID>::iterator Iterator;
    CTemplatePebTeb<T>::LDR_MODULET* pPebLdrModule;
    *pEntryPoint= 0;
    *pLoadCount = 0;
    *pFlags = 0;
    *pTlsIndex = 0;
    *pTimeDateStamp = 0;
    for (Iterator = this->ModulesInfos.begin(); Iterator!=this->ModulesInfos.end(); Iterator++)
    {
        pPebLdrModule= (CTemplatePebTeb<T>::LDR_MODULET*)(*Iterator);
        if (pPebLdrModule->BaseAddress==hModule)
        {
            *pEntryPoint= pPebLdrModule->EntryPoint;
            *pLoadCount = pPebLdrModule->LoadCount;
            *pFlags = pPebLdrModule->Flags;
            *pTlsIndex = pPebLdrModule->TlsIndex;
            *pTimeDateStamp = pPebLdrModule->TimeDateStamp;
            return TRUE;
        }
    }
    return FALSE;
}