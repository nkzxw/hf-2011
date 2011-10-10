#include "x86CrossCompatibility.h"

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

namespace x86CrossCompatibility
{

#define Cx86CrossCompatibility_WAIT_TIMEOUT 30000

Cx86CrossCompatibility::Cx86CrossCompatibility(TCHAR* x86CrossCompatibilityBinPath)
{
    ///////////////////////////////////////////////
    // create client and server mailslots
    ///////////////////////////////////////////////
    
    // make slots base names
	TCHAR MailSlotsBaseName[MAX_PATH];
    TCHAR MailSlotName[MAX_PATH];
    _sntprintf(MailSlotsBaseName,_countof(MailSlotsBaseName),_T("x86CC%x%x%x"),::GetCurrentProcessId(),::GetCurrentThreadId(),::GetTickCount());

    _sntprintf(MailSlotName,_countof(MailSlotName),_T("\\\\.\\mailslot\\%sQuery"),MailSlotsBaseName);
    pMailSlotQuery = new CMailSlotClient( MailSlotName );

    _sntprintf(MailSlotName,_countof(MailSlotName),_T("\\\\.\\mailslot\\%sReply"),MailSlotsBaseName);
    pMailSlotReply = new CMailSlotServer( MailSlotName,MailSlotCallback,this) ;
    this->MailSlotServerDataArrival=0;
    this->MailSlotServerReleaseData=0;
    if (pMailSlotReply->Start(FALSE))
    {
        // create events only in case of server creation success to force waiting functions to fail
        this->MailSlotServerDataArrival=::CreateEvent(0,0,0,0);
        this->MailSlotServerReleaseData=::CreateEvent(0,0,0,0);
    }
    this->MailSlotServerData=0;
    this->MailSlotServerDataSize=0;

    ///////////////////////////////////////////////
    // create process doing compatibility
    ///////////////////////////////////////////////
	TCHAR CmdLine[2*MAX_PATH];
	PROCESS_INFORMATION pi={0};
	STARTUPINFO si={0};
	si.cb=sizeof(si);
	_sntprintf(CmdLine,_countof(CmdLine),_T("\"%s\" MailSlotsBaseName=%s"),x86CrossCompatibilityBinPath,MailSlotsBaseName);
	this->hProcess=NULL;

    HANDLE hSubProcessReady = ::CreateEvent(0,0,0,MailSlotsBaseName);
	if(::CreateProcess(0,CmdLine,0,0,0,0,0,0,&si,&pi))
	{
		this->hProcess = pi.hProcess;
		::CloseHandle(pi.hThread);
        :: WaitForSingleObject(hSubProcessReady,Cx86CrossCompatibility_WAIT_TIMEOUT);
        pMailSlotQuery->Open();
	}
    ::CloseHandle(hSubProcessReady);

    // create critical section
	::InitializeCriticalSection(&this->CriticalSection);
}

Cx86CrossCompatibility::~Cx86CrossCompatibility(void)
{
	::EnterCriticalSection(&this->CriticalSection);

    BASE_HEADER StopCommand;
    StopCommand.CommandId = COMMAND_ID_Stopx86CrossCompatibilityProcess;
    pMailSlotQuery->Write(&StopCommand,sizeof(StopCommand));
    pMailSlotReply->Stop();
    delete pMailSlotQuery;
    delete pMailSlotReply;

	if (this->hProcess)
	{
		// send end process query

		// wait end of process
		::WaitForSingleObject(this->hProcess,INFINITE);
		::CloseHandle(this->hProcess);
	}
	this->hProcess = NULL;

	::DeleteCriticalSection(&this->CriticalSection);
}

HANDLE64 WINAPI Cx86CrossCompatibility::CreateToolhelp32Snapshot(
	__in  DWORD dwFlags,
	__in  DWORD th32ProcessID
	)
{
    HANDLE64 RetValue = 0;

	::EnterCriticalSection(&this->CriticalSection);
	if (this->hProcess==NULL)// error on process creation or destructor has been called
		goto CleanUp;

    CreateToolhelp32SnapshotQuery Query;
    Query.Header.CommandId = COMMAND_ID_CreateToolhelp32Snapshot;
    Query.dwFlags = dwFlags;
    Query.th32ProcessID = th32ProcessID;
    if (!this->pMailSlotQuery->Write(&Query,sizeof(Query)))
        goto CleanUp;

    CreateToolhelp32SnapshotReply* pReply;
    DWORD Ret = ::WaitForSingleObject(this->MailSlotServerDataArrival,Cx86CrossCompatibility_WAIT_TIMEOUT);
    if (Ret!=WAIT_OBJECT_0)
        goto CleanUp;

    if (this->MailSlotServerDataSize<sizeof(*pReply))
        goto CleanUp;

    pReply = (CreateToolhelp32SnapshotReply*)this->MailSlotServerData;
    if (pReply->Header.CommandId!=COMMAND_ID_CreateToolhelp32Snapshot)
        goto CleanUp;
    
    RetValue = pReply->Handle;

CleanUp:
    ::SetEvent(this->MailSlotServerReleaseData);
	::LeaveCriticalSection(&this->CriticalSection);
    return RetValue;
}

BOOL WINAPI Cx86CrossCompatibility::Module32First(
	__in     HANDLE64 hSnapshot,
	__inout  LPMODULEENTRY3264 lpme
	)
{
    BOOL RetValue = 0;

    if (::IsBadWritePtr(lpme,sizeof(MODULEENTRY3264)))
        return FALSE;

    ::EnterCriticalSection(&this->CriticalSection);
    if (this->hProcess==NULL)// error on process creation or destructor has been called
        goto CleanUp;

    Module32FirstQuery Query;
#if (defined(UNICODE)||defined(_UNICODE))
    Query.Header.CommandId = COMMAND_ID_Module32FirstW;
#else
    Query.Header.CommandId = COMMAND_ID_Module32FirstA;
#endif

    Query.hSnapshot = hSnapshot;
    if (!this->pMailSlotQuery->Write(&Query,sizeof(Query)))
        goto CleanUp;

#if (defined(UNICODE)||defined(_UNICODE))
    Module32FirstWReply* pReply;
#else
    Module32FirstAReply* pReply;
#endif
    DWORD Ret = ::WaitForSingleObject(this->MailSlotServerDataArrival,Cx86CrossCompatibility_WAIT_TIMEOUT);
    if (Ret!=WAIT_OBJECT_0)
        goto CleanUp;

    if (this->MailSlotServerDataSize<sizeof(*pReply))
        goto CleanUp;

    pReply = 
#if (defined(UNICODE)||defined(_UNICODE))
            (Module32FirstWReply*)
#else
            (Module32FirstAReply*)
#endif
                                    this->MailSlotServerData;
#if (defined(UNICODE)||defined(_UNICODE))
    if (pReply->Header.CommandId!=COMMAND_ID_Module32FirstW)
#else
    if (pReply->Header.CommandId!=COMMAND_ID_Module32FirstA)
#endif
        goto CleanUp;

    memcpy(lpme,&pReply->me,sizeof(MODULEENTRY3264));
    RetValue = pReply->RetValue;

CleanUp:
    ::SetEvent(this->MailSlotServerReleaseData);
    ::LeaveCriticalSection(&this->CriticalSection);
    return RetValue;
}

BOOL WINAPI Cx86CrossCompatibility::Module32Next(
	__in   HANDLE64 hSnapshot,
	__out  LPMODULEENTRY3264 lpme
	)
{
    BOOL RetValue = 0;

    if (::IsBadWritePtr(lpme,sizeof(MODULEENTRY3264)))
        return FALSE;

    ::EnterCriticalSection(&this->CriticalSection);
    if (this->hProcess==NULL)// error on process creation or destructor has been called
        goto CleanUp;

    Module32NextQuery Query;
#if (defined(UNICODE)||defined(_UNICODE))
    Query.Header.CommandId = COMMAND_ID_Module32NextW;
#else
    Query.Header.CommandId = COMMAND_ID_Module32NextA;
#endif
    Query.hSnapshot = hSnapshot;
    if (!this->pMailSlotQuery->Write(&Query,sizeof(Query)))
        goto CleanUp;

#if (defined(UNICODE)||defined(_UNICODE))
    Module32NextWReply* pReply;
#else
    Module32NextAReply* pReply;
#endif
    DWORD Ret = ::WaitForSingleObject(this->MailSlotServerDataArrival,Cx86CrossCompatibility_WAIT_TIMEOUT);
    if (Ret!=WAIT_OBJECT_0)
        goto CleanUp;

    if (this->MailSlotServerDataSize<sizeof(*pReply))
        goto CleanUp;

    pReply = 
#if (defined(UNICODE)||defined(_UNICODE))
            (Module32NextWReply*)
#else
            (Module32NextAReply*)
#endif
                                    this->MailSlotServerData;

#if (defined(UNICODE)||defined(_UNICODE))
    if (pReply->Header.CommandId!=COMMAND_ID_Module32NextW)
#else
    if (pReply->Header.CommandId!=COMMAND_ID_Module32NextA)
#endif
        goto CleanUp;

    memcpy(lpme,&pReply->me,sizeof(MODULEENTRY3264));
    RetValue = pReply->RetValue;

CleanUp:
    ::SetEvent(this->MailSlotServerReleaseData);
    ::LeaveCriticalSection(&this->CriticalSection);
    return RetValue;
}

BOOL WINAPI Cx86CrossCompatibility::CloseHandle(
	__in  HANDLE64 hObject
	)
{
    BOOL RetValue = 0;

    ::EnterCriticalSection(&this->CriticalSection);
    if (this->hProcess==NULL)// error on process creation or destructor has been called
        goto CleanUp;

    CloseHandleQuery Query;
    Query.Header.CommandId = COMMAND_ID_CloseHandle;
    Query.Handle = hObject;
    if (!this->pMailSlotQuery->Write(&Query,sizeof(Query)))
        goto CleanUp;

    CloseHandleReply* pReply;
    DWORD Ret = ::WaitForSingleObject(this->MailSlotServerDataArrival,Cx86CrossCompatibility_WAIT_TIMEOUT);
    if (Ret!=WAIT_OBJECT_0)
        goto CleanUp;

    if (this->MailSlotServerDataSize<sizeof(*pReply))
        goto CleanUp;

    pReply = (CloseHandleReply*)this->MailSlotServerData;
    if (pReply->Header.CommandId!=COMMAND_ID_CloseHandle)
        goto CleanUp;

    RetValue = pReply->RetValue;

CleanUp:
    ::SetEvent(this->MailSlotServerReleaseData);
    ::LeaveCriticalSection(&this->CriticalSection);
    return RetValue;
}

BOOL WINAPI Cx86CrossCompatibility::VirtualProtectEx(
	__in   HANDLE64 hProcess,
	__in   __int64 lpAddress,
	__in   __int64 dwSize,
	__in   DWORD flNewProtect,
	__out  PDWORD lpflOldProtect
	)
{
    BOOL RetValue = 0;

    if (::IsBadWritePtr(lpflOldProtect,sizeof(DWORD)))
        return FALSE;

    ::EnterCriticalSection(&this->CriticalSection);
    if (this->hProcess==NULL)// error on process creation or destructor has been called
        goto CleanUp;

    VirtualProtectExQuery Query;
    Query.Header.CommandId = COMMAND_ID_VirtualProtectEx;
    Query.hProcess = hProcess;
    Query.lpAddress = lpAddress;
    Query.dwSize = dwSize;
    Query.flNewProtect = flNewProtect;
    if (!this->pMailSlotQuery->Write(&Query,sizeof(Query)))
        goto CleanUp;

    VirtualProtectExReply* pReply;
    DWORD Ret = ::WaitForSingleObject(this->MailSlotServerDataArrival,Cx86CrossCompatibility_WAIT_TIMEOUT);
    if (Ret!=WAIT_OBJECT_0)
        goto CleanUp;

    if (this->MailSlotServerDataSize<sizeof(*pReply))
        goto CleanUp;

    pReply = (VirtualProtectExReply*)this->MailSlotServerData;
    if (pReply->Header.CommandId!=COMMAND_ID_VirtualProtectEx)
        goto CleanUp;

    *lpflOldProtect = pReply->OldProtect;
    RetValue = pReply->RetValue;

CleanUp:
    ::SetEvent(this->MailSlotServerReleaseData);
    ::LeaveCriticalSection(&this->CriticalSection);
    return RetValue;
}

__int64 WINAPI Cx86CrossCompatibility::VirtualQueryEx(
	__in      HANDLE64 hProcess,
	__in_opt  __int64 lpAddress,
	__out     PMEMORY_BASIC_INFORMATION64 lpBuffer,
	__in      __int64 dwLength
	)
{
    HANDLE64 RetValue = 0;

    if (dwLength!=sizeof(MEMORY_BASIC_INFORMATION64))
        return FALSE;

    if (::IsBadWritePtr(lpBuffer,sizeof(MEMORY_BASIC_INFORMATION64)))
        return FALSE;

    ::EnterCriticalSection(&this->CriticalSection);
    if (this->hProcess==NULL)// error on process creation or destructor has been called
        goto CleanUp;

    VirtualQueryExQuery Query;
    Query.Header.CommandId = COMMAND_ID_VirtualQueryEx;
    Query.hProcess = hProcess;
    Query.lpAddress = lpAddress;
    if (!this->pMailSlotQuery->Write(&Query,sizeof(Query)))
        goto CleanUp;

    VirtualQueryExReply* pReply;
    DWORD Ret = ::WaitForSingleObject(this->MailSlotServerDataArrival,Cx86CrossCompatibility_WAIT_TIMEOUT);
    if (Ret!=WAIT_OBJECT_0)
        goto CleanUp;

    if (this->MailSlotServerDataSize<sizeof(*pReply))
        goto CleanUp;

    pReply = (VirtualQueryExReply*)this->MailSlotServerData;
    if (pReply->Header.CommandId!=COMMAND_ID_VirtualQueryEx)
        goto CleanUp;

    memcpy(lpBuffer,&pReply->lpBuffer,sizeof(MEMORY_BASIC_INFORMATION64));
    RetValue = pReply->RetValue;

CleanUp:
    ::SetEvent(this->MailSlotServerReleaseData);
    ::LeaveCriticalSection(&this->CriticalSection);
    return RetValue;
}

BOOL WINAPI Cx86CrossCompatibility::ReadProcessMemory(
	__in   HANDLE64 hProcess,
	__in   __int64 lpBaseAddress,
	__out  LPVOID lpBuffer,
	__in   __int64 nSize,
	__out  __int64 *lpNumberOfBytesRead
	)
{
    BOOL RetValue = 0;

    if (::IsBadWritePtr(lpBuffer,nSize))
        return FALSE;

    if (::IsBadWritePtr(lpNumberOfBytesRead,sizeof(__int64)))
        return FALSE;

    *lpNumberOfBytesRead = 0;

    ::EnterCriticalSection(&this->CriticalSection);
    if (this->hProcess==NULL)// error on process creation or destructor has been called
        goto CleanUp;

    ReadProcessMemoryQuery Query;
    Query.Header.CommandId = COMMAND_ID_ReadProcessMemory;
    Query.hProcess = hProcess;
    Query.lpBaseAddress = lpBaseAddress;
    Query.nSize = nSize;
    if (!this->pMailSlotQuery->Write(&Query,sizeof(Query)))
        goto CleanUp;

    ReadProcessMemoryReply* pReply;
    DWORD Ret = ::WaitForSingleObject(this->MailSlotServerDataArrival,Cx86CrossCompatibility_WAIT_TIMEOUT);
    if (Ret!=WAIT_OBJECT_0)
        goto CleanUp;

    if (this->MailSlotServerDataSize<sizeof(*pReply))
        goto CleanUp;

    pReply = (ReadProcessMemoryReply*)this->MailSlotServerData;
    if (pReply->Header.CommandId!=COMMAND_ID_ReadProcessMemory)
        goto CleanUp;

    if (this->MailSlotServerDataSize<sizeof(*pReply)+pReply->NumberOfBytesRead)
        goto CleanUp;

    *lpNumberOfBytesRead = pReply->NumberOfBytesRead;
    memcpy(lpBuffer,(PBYTE)this->MailSlotServerData+sizeof(*pReply),pReply->NumberOfBytesRead);
    RetValue = pReply->RetValue;

CleanUp:
    ::SetEvent(this->MailSlotServerReleaseData);
    ::LeaveCriticalSection(&this->CriticalSection);
    return RetValue;
}

BOOL WINAPI Cx86CrossCompatibility::WriteProcessMemory(
	__in   HANDLE64 hProcess,
	__in   __int64 lpBaseAddress,
	__in   LPCVOID lpBuffer,
	__in   __int64 nSize,
	__out  __int64* lpNumberOfBytesWritten
	)
{
    BOOL RetValue = 0;
    PBYTE Buffer = NULL;

    if (::IsBadWritePtr(lpNumberOfBytesWritten,sizeof(__int64)))
        return FALSE;

    if (::IsBadReadPtr(lpBuffer,nSize))
        return FALSE;

    ::EnterCriticalSection(&this->CriticalSection);
    if (this->hProcess==NULL)// error on process creation or destructor has been called
        goto CleanUp;

    WriteProcessMemoryQuery Query;
    Query.Header.CommandId = COMMAND_ID_WriteProcessMemory;
    Query.hProcess = hProcess;
    Query.lpBaseAddress = lpBaseAddress;
    Query.nSize = nSize;

    Buffer = new BYTE[sizeof(Query)+nSize];
    if (!Buffer)
        goto CleanUp;

    memcpy(Buffer,&Query,sizeof(Query));

    if (!this->pMailSlotQuery->Write(Buffer,(DWORD)(sizeof(Query)+nSize)))
        goto CleanUp;

    WriteProcessMemoryReply* pReply;
    DWORD Ret = ::WaitForSingleObject(this->MailSlotServerDataArrival,Cx86CrossCompatibility_WAIT_TIMEOUT);
    if (Ret!=WAIT_OBJECT_0)
        goto CleanUp;

    if (this->MailSlotServerDataSize<sizeof(*pReply))
        goto CleanUp;

    pReply = (WriteProcessMemoryReply*)this->MailSlotServerData;
    if (pReply->Header.CommandId!=COMMAND_ID_WriteProcessMemory)
        goto CleanUp;

    *lpNumberOfBytesWritten = pReply->NumberOfBytesWritten;
    RetValue = pReply->RetValue;

CleanUp:
    ::SetEvent(this->MailSlotServerReleaseData);
    if (Buffer)
        delete [] Buffer;
    ::LeaveCriticalSection(&this->CriticalSection);
    return RetValue;
}
HANDLE64 WINAPI Cx86CrossCompatibility::OpenProcess(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  DWORD dwProcessId
	)
{
    HANDLE64 RetValue = 0;

    ::EnterCriticalSection(&this->CriticalSection);
    if (this->hProcess==NULL)// error on process creation or destructor has been called
        goto CleanUp;

    OpenProcessQuery Query;
    Query.Header.CommandId = COMMAND_ID_OpenProcess;
    Query.dwDesiredAccess = dwDesiredAccess;
    Query.bInheritHandle = bInheritHandle;
    Query.dwProcessId = dwProcessId;
    if (!this->pMailSlotQuery->Write(&Query,sizeof(Query)))
        goto CleanUp;

    OpenProcessReply* pReply;
    DWORD Ret = ::WaitForSingleObject(this->MailSlotServerDataArrival,Cx86CrossCompatibility_WAIT_TIMEOUT);
    if (Ret!=WAIT_OBJECT_0)
        goto CleanUp;

    if (this->MailSlotServerDataSize<sizeof(*pReply))
        goto CleanUp;

    pReply = (OpenProcessReply*)this->MailSlotServerData;
    if (pReply->Header.CommandId!=COMMAND_ID_OpenProcess)
        goto CleanUp;

    RetValue = pReply->RetValue;

CleanUp:
    ::SetEvent(this->MailSlotServerReleaseData);
    ::LeaveCriticalSection(&this->CriticalSection);
    return RetValue;
}

LONG WINAPI Cx86CrossCompatibility::NtQueryInformationProcess(
    IN HANDLE64 ProcessHandle,
    IN __int64 ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    LONG RetValue = 0;
    
    if (::IsBadWritePtr(ProcessInformation,ProcessInformationLength))
        return FALSE;
    if (ReturnLength)
    {
        if (::IsBadWritePtr(ReturnLength,sizeof(ULONG)))
            return FALSE;

        *ReturnLength=0;
    }

    ::EnterCriticalSection(&this->CriticalSection);
    if (this->hProcess==NULL)// error on process creation or destructor has been called
        goto CleanUp;

    NtQueryInformationProcessQuery Query;
    Query.Header.CommandId = COMMAND_ID_NtQueryInformationProcess;
    Query.ProcessHandle = ProcessHandle;
    Query.ProcessInformationClass = ProcessInformationClass;
    Query.ProcessInformationLength = ProcessInformationLength;
    if (!this->pMailSlotQuery->Write(&Query,sizeof(Query)))
        goto CleanUp;

    NtQueryInformationProcessReply* pReply;
    DWORD Ret = ::WaitForSingleObject(this->MailSlotServerDataArrival,Cx86CrossCompatibility_WAIT_TIMEOUT);
    if (Ret!=WAIT_OBJECT_0)
        goto CleanUp;

    if (this->MailSlotServerDataSize<sizeof(NtQueryInformationProcessReply))
        goto CleanUp;

    pReply = (NtQueryInformationProcessReply*)this->MailSlotServerData;
    if (pReply->Header.CommandId!=COMMAND_ID_NtQueryInformationProcess)
        goto CleanUp;

    if (this->MailSlotServerDataSize<sizeof(*pReply)+pReply->ReturnLength)
        goto CleanUp;

    if (ReturnLength)
        *ReturnLength = pReply->ReturnLength;
    memcpy(ProcessInformation,(PBYTE)this->MailSlotServerData+sizeof(*pReply),pReply->ReturnLength);
    RetValue = pReply->RetValue;

CleanUp:
    ::SetEvent(this->MailSlotServerReleaseData);
    ::LeaveCriticalSection(&this->CriticalSection);
    return RetValue;
}

void Cx86CrossCompatibility::MailSlotCallback(PVOID pData,DWORD dwDataSize,PVOID pUserData)
{
    Cx86CrossCompatibility* px86CrossCompatibility = (Cx86CrossCompatibility*)pUserData;
    ::ResetEvent(px86CrossCompatibility->MailSlotServerReleaseData);
    px86CrossCompatibility->MailSlotServerData = pData;
    px86CrossCompatibility->MailSlotServerDataSize = dwDataSize;
    ::SetEvent(px86CrossCompatibility->MailSlotServerDataArrival);
    ::WaitForSingleObject(px86CrossCompatibility->MailSlotServerReleaseData,INFINITE);
}

}