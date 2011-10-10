/*

Thanks to A. Miguel Feijao

Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: retrieve process or thread Id from process or thread handle
//-----------------------------------------------------------------------------

#include "ProcessAndThreadID.h"
#include "../PEB_TEB/PEB_TEB.h"

CProcessAndThreadID::CProcessAndThreadID()
{
    this->pNtQueryInformationThread=NULL;
    this->pNtQueryInformationProcess=NULL;
    this->pRtlNtStatusToDosError=NULL;
    this->pGetProcessIdOfThread =NULL;
    this->pGetThreadId=NULL;
    this->pGetProcessId=NULL;

    // load libraries and get functions address
    // for future calls
    // Requiered funcs:
    //      NtQueryInformationThread
    //      NtQueryInformationProcess
    //      RtlNtStatusToDosError
    //      GetProcessIdOfThread
    //      GetThreadId
    //      GetProcessId

    // kernel32 and ntdll are always loaded so only GetModuleHandle
    this->hModuleNtDll=GetModuleHandle(_T("Ntdll.dll"));
    this->hModuleKernel32=GetModuleHandle(_T("Kernel32.dll"));
    if (this->hModuleNtDll)
    {
        this->pNtQueryInformationThread=(ptrNtQueryInformationThread)GetProcAddress(this->hModuleNtDll,"NtQueryInformationThread");
        this->pNtQueryInformationProcess=(ptrNtQueryInformationProcess)GetProcAddress(this->hModuleNtDll,"NtQueryInformationProcess");
        this->pRtlNtStatusToDosError=(ptrRtlNtStatusToDosError)GetProcAddress(this->hModuleNtDll,"RtlNtStatusToDosError");
    }
    if (this->hModuleKernel32)
    {
        this->pGetProcessIdOfThread =(ptrGetProcessIdOfThread)GetProcAddress(this->hModuleKernel32,"GetProcessIdOfThread");
        this->pGetThreadId=(ptrGetThreadId)GetProcAddress(this->hModuleKernel32,"GetThreadId");
        this->pGetProcessId=(ptrGetProcessId)GetProcAddress(this->hModuleKernel32,"GetProcessId");
    }
}
CProcessAndThreadID::~CProcessAndThreadID()
{

}

//-----------------------------------------------------------------------------
// Name: GetProcessIdOfThreadOldOS
// Object: GetProcessIdOfThread for xp or older OS for new os just use GetProcessIdOfThread
// Parameters :
//     in  : HANDLE hThread : thread handle
//     out :
//     return : ProcessId or 0 if Failed
//-----------------------------------------------------------------------------
DWORD CProcessAndThreadID::GetProcessIdOfThreadOldOS(HANDLE hThread)
{
    DWORD dwProcessId=0;
    HANDLE        hThreadSnap = NULL;
    THREADENTRY32 te32        = {0}; 
    DWORD dwThreadId;

    dwThreadId=this->GetThreadIdOldOS(hThread);
    if (dwThreadId==0)
        return 0;

    // Take a snapshot of all threads currently in the system. 
    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); 
    if (hThreadSnap == INVALID_HANDLE_VALUE) 
        return dwProcessId; 
 
    // Fill in the size of the structure before using it. 
    te32.dwSize = sizeof(THREADENTRY32); 
 
    // Walk the thread snapshot to find all threads of the process. 
    // If the thread belongs to the process, add its information 
    // to the display list.
    if (Thread32First(hThreadSnap, &te32)) 
    { 
        do 
        { 
            if (dwThreadId == te32.th32ThreadID) 
            { 
                dwProcessId=te32.th32OwnerProcessID;
                break;
            } 
        } 
        while (Thread32Next(hThreadSnap, &te32)); 
    } 
    else 
    {
        CAPIError::ShowLastError();
    }

    // Do not forget to clean up the snapshot object. 
    CloseHandle (hThreadSnap); 

    return dwProcessId;
}

//-----------------------------------------------------------------------------
// Name: GetThreadIdOldOS
// Object: get thread id for given process handle for old os
// Parameters :
//     in  : HANDLE hThread : thread handle
//     out :
//     return : ThreadId or 0 if Failed
//-----------------------------------------------------------------------------
DWORD CProcessAndThreadID::GetThreadIdOldOS(HANDLE hThread)
{
	NTSTATUS Status;
    CTemplatePebTeb<UINT32>::THREAD_BASIC_INFORMATIONT tbi;// old os supposed to be 32bit
    HANDLE					hDupHandle;
    HANDLE					hCurrentProcess;

    if ((this->pNtQueryInformationThread==NULL)||(this->pRtlNtStatusToDosError==NULL))
    {
#ifndef TOOLS_NO_MESSAGEBOX
        MessageBox(NULL,_T("Error loading NtQueryInformationThread or RtlNtStatusToDosError in Ntdll.dll"),
            _T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
#endif
        return 0;
    }

    hCurrentProcess = GetCurrentProcess();

    // Use DuplicateHandle() to get THREAD_QUERY_INFORMATION access right
    if (!DuplicateHandle(hCurrentProcess, 
                         hThread, 
                         hCurrentProcess, 
                         &hDupHandle, 
                         THREAD_QUERY_INFORMATION, 
                         FALSE, 
                         0))
    {
        SetLastError(ERROR_ACCESS_DENIED);
        CAPIError::ShowLastError();
        return 0;
    }

    /*
    NTSTATUS
    NtQueryInformationThread (
        IN HANDLE ThreadHandle,
        IN THREADINFOCLASS ThreadInformationClass,
        OUT PVOID ThreadInformation,
        IN ULONG ThreadInformationLength,
        OUT PULONG ReturnLength OPTIONAL
        );
    */

    // get thread id
    Status = this->pNtQueryInformationThread(hDupHandle,
                                      ThreadBasicInformation,
                                      &tbi,
                                      sizeof(tbi),
                                      NULL
                                      );
    CloseHandle(hDupHandle);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(this->pRtlNtStatusToDosError(Status));
        CAPIError::ShowLastError();
        return 0;
    }
    return (DWORD)tbi.ClientId.UniqueThread;
}

//-----------------------------------------------------------------------------
// Name: GetProcessIdOldOS
// Object: get process id for given process handle for old os
// Parameters :
//     in  : HANDLE hProcess : process handle
//     out :
//     return : ProcessId or 0 if Failed
//-----------------------------------------------------------------------------
DWORD CProcessAndThreadID::GetProcessIdOldOS(HANDLE hProcess)
{
    NTSTATUS				Status;
    CTemplatePebTeb<UINT32>::PROCESS_BASIC_INFORMATIONT pbi;// old os supposed to be 32bit
    HANDLE                    hDupHandle;
    HANDLE                    hCurrentProcess;

    if ((this->pNtQueryInformationThread==NULL)||(this->pRtlNtStatusToDosError==NULL))
    {
#ifndef TOOLS_NO_MESSAGEBOX
        MessageBox(NULL,_T("Error loading NtQueryInformationThread or RtlNtStatusToDosError in Ntdll.dll"),
            _T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
#endif
        return 0;
    }

    hCurrentProcess = GetCurrentProcess();

    // Use DuplicateHandle() to get PROCESS_QUERY_INFORMATION access right
    if (!DuplicateHandle(hCurrentProcess, 
                         hProcess, 
                         hCurrentProcess, 
                         &hDupHandle, 
                         PROCESS_QUERY_INFORMATION, 
                         FALSE, 
                         0))
    {
        SetLastError(ERROR_ACCESS_DENIED);
        CAPIError::ShowLastError();
        return 0;
    }

    /*
    NTSTATUS
    NtQueryInformationProcess (
        IN HANDLE ProcessHandle,
        IN PROCESSINFOCLASS ProcessInformationClass,
        OUT PVOID ProcessInformation,
        IN ULONG ProcessInformationLength,
        OUT PULONG ReturnLength OPTIONAL
        );
    */
    Status = this->pNtQueryInformationProcess(hDupHandle, 
                                       ProcessBasicInformation, 
                                       &pbi,
                                       sizeof(pbi), 
                                       NULL);

    CloseHandle(hDupHandle);

    if (!NT_SUCCESS(Status))
    {        
        SetLastError(this->pRtlNtStatusToDosError(Status));
        return 0;
    }
    // Return PID
    return (DWORD)pbi.UniqueProcessId;
}

//-----------------------------------------------------------------------------
// Name: GetProcessIdOfThread
// Object: get process id for given thread handle
// Parameters :
//     in  : HANDLE hThread : thread handle
//     out :
//     return : ProcessId or 0 if Failed
//-----------------------------------------------------------------------------
DWORD CProcessAndThreadID::GetProcessIdOfThread(HANDLE hThread)
{
    if(this->pGetProcessIdOfThread)
        return this->pGetProcessIdOfThread(hThread);
    else
        return this->GetProcessIdOfThreadOldOS(hThread);
}

//-----------------------------------------------------------------------------
// Name: GetThreadId
// Object: get thread id for given thread handle
// Parameters :
//     in  : HANDLE hThread : thread handle
//     out :
//     return : ThreadId or 0 if Failed
//-----------------------------------------------------------------------------
DWORD CProcessAndThreadID::GetThreadId(HANDLE hThread)
{
    if(this->pGetThreadId)
        return this->pGetThreadId(hThread);
    return this->GetThreadIdOldOS(hThread);
}

//-----------------------------------------------------------------------------
// Name: GetProcessId
// Object: get process id for given process handle
// Parameters :
//     in  : HANDLE hProcess : process handle
//     out :
//     return : ProcessId or 0 if Failed
//-----------------------------------------------------------------------------
DWORD CProcessAndThreadID::GetProcessId(HANDLE hProcess)
{
    if(this->pGetProcessId)
        return this->pGetProcessId(hProcess);
    return this->GetProcessIdOldOS(hProcess);
}