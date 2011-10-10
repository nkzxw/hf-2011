#include "kernelprocessesinfo.h"

typedef DWORD (__stdcall *pZwQuerySystemInformation)(DWORD,LPVOID,DWORD,DWORD*);
#define SystemModuleInformation 11 // SYSTEMINFOCLASS

CKernelProcessesInfo::CKernelProcessesInfo(void)
{
    this->pSystemModuleInformation=NULL;

    this->Update();
}

BOOL CKernelProcessesInfo::Update()
{
    ULONG ReturnLength;
    ULONG SystemInformationLength;
    DWORD Status;

    if (this->pSystemModuleInformation)
        delete[] pSystemModuleInformation;

    this->pSystemModuleInformation=NULL;

    SystemInformationLength=0;
    pZwQuerySystemInformation ZwQuerySystemInformation=(pZwQuerySystemInformation)GetProcAddress(GetModuleHandle(_T("ntdll.dll")),"ZwQuerySystemInformation");
    ZwQuerySystemInformation(SystemModuleInformation,pSystemModuleInformation,SystemInformationLength,&ReturnLength);
    SystemInformationLength=ReturnLength;
    pSystemModuleInformation=(PMODULE_LIST)new BYTE[SystemInformationLength];
    Status=ZwQuerySystemInformation(SystemModuleInformation,pSystemModuleInformation,SystemInformationLength,&ReturnLength);
    if (Status!=ERROR_SUCCESS)
    {
        if (this->pSystemModuleInformation)
        {
            delete[] this->pSystemModuleInformation;
            this->pSystemModuleInformation=NULL;
            return FALSE;
        }
    }
    return TRUE;
}

CKernelProcessesInfo::~CKernelProcessesInfo(void)
{
    if (this->pSystemModuleInformation)
        delete[] pSystemModuleInformation;
}
