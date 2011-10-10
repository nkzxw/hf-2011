/*
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
// Object: Process helper class. It allows easy operation on processes
//         like retrieving name or checking if a process is still running
//-----------------------------------------------------------------------------

#include "ProcessHelper.h"


BOOL  CProcessHelper::Is32bitsProcess(DWORD ProcessID,OUT BOOL* pbIs32bits)
{
    // PROCESS_QUERY_LIMITED_INFORMATION (0x1000) // not supported on XP
    HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION/*PROCESS_QUERY_LIMITED_INFORMATION*/,FALSE,ProcessID);
    if (!hProcess)
        return FALSE;

    BOOL bRet = CProcessHelper::Is32bitsProcessHandle(hProcess,pbIs32bits);
    ::CloseHandle(hProcess);

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: Is32bitsProcessHandle
// Object: check if a process is 32 bits process
// Parameters :
//     in :HANDLE hProcess : process handle. The handle must have the PROCESS_QUERY_INFORMATION or PROCESS_QUERY_LIMITED_INFORMATION access right
//     out : BOOL* pbIs32bits : TRUE if process is 32 bits, else FALSE 
// Return : TRUE on Success
//-----------------------------------------------------------------------------
BOOL CProcessHelper::Is32bitsProcessHandle(HANDLE hProcess,OUT BOOL* pbIs32bits)
{
    *pbIs32bits = FALSE;
    SYSTEM_INFO SysInfos={0};
    ::GetNativeSystemInfo(&SysInfos);
    // if platform is 32 bits
    if (SysInfos.wProcessorArchitecture == 0) //PROCESSOR_ARCHITECTURE_INTEL    0    x86
    {
        *pbIs32bits = TRUE; // process is 32 bits
        return TRUE;
    }

    // else --> platform should be 64
    // check if WOW64 (32bits)
    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
    LPFN_ISWOW64PROCESS fnIsWow64Process;
    BOOL bIsWow64 = FALSE;

    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(
        GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if(fnIsWow64Process)
    {
        if (!fnIsWow64Process(hProcess,&bIsWow64))
        {
            //handle error
            return FALSE;
        }
    }
    *pbIs32bits = bIsWow64;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsAlive
// Object: check if a process is still running 
// Parameters :
//     in : DWORD ProcessID : ID of process we want to check
// Return : TRUE if process is running, else FALSE 
//-----------------------------------------------------------------------------
BOOL CProcessHelper::IsAlive(DWORD ProcessID)
{
    PROCESSENTRY32 pe32 = {0};
    HANDLE hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (hSnap == INVALID_HANDLE_VALUE) 
        return FALSE; 

    // Fill the size of the structure before using it. 
    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
    // Walk the process list of the system
    if (!Process32First(hSnap, &pe32))
    {
        CloseHandle(hSnap);
        return FALSE;
    }
    do 
    {
        if (pe32.th32ProcessID!=ProcessID)
            continue;
        CloseHandle (hSnap);
        return TRUE;
    } 
    while (Process32Next(hSnap, &pe32)); 
 
    // clean up the snapshot object. 
    CloseHandle (hSnap); 

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: GetProcessName
// Object: get the name of the given process ID
// Parameters :
//     in : DWORD ProcessID : ID of process we require name
//     out : TCHAR* ProcessName : name of the process size must be at least
//          MAX_PATH  in TCHAR count
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CProcessHelper::GetProcessName(DWORD ProcessID,TCHAR* ProcessName)
{
    if (IsBadWritePtr(ProcessName,sizeof(TCHAR)))
        return FALSE;
    *ProcessName=0;

    PROCESSENTRY32 pe32 = {0};
    HANDLE hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (hSnap == INVALID_HANDLE_VALUE) 
        return FALSE; 

    // Fill the size of the structure before using it. 
    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
    // Walk the process list of the system
    if (!Process32First(hSnap, &pe32))
    {
        CloseHandle(hSnap);
        return FALSE;
    }
    do 
    {
        if (pe32.th32ProcessID!=ProcessID)
            continue;
        
        _tcscpy(ProcessName,pe32.szExeFile);
        CloseHandle (hSnap);
        return TRUE;
    } 
    while (Process32Next(hSnap, &pe32)); 
 
    // clean up the snapshot object. 
    CloseHandle (hSnap); 

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: GetProcessFullPath
// Object: get the name of the given process ID
// Parameters :
//     in : DWORD ProcessID : ID of process we require name
//     out : TCHAR* ProcessName : name of the process size must be at least
//          MAX_PATH  in TCHAR count
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CProcessHelper::GetProcessFullPath(DWORD ProcessID,TCHAR* ProcessName)
{
    if (IsBadWritePtr(ProcessName,sizeof(TCHAR)))
        return FALSE;
    *ProcessName=0;

    MODULEENTRY32 me32 = {0}; 
    HANDLE hSnap;

    hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,ProcessID);
    if (hSnap == INVALID_HANDLE_VALUE) 
    {
        return FALSE; 
    }
    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32); 
 
    // get first module entry
    if (!Module32First(hSnap, &me32))
    {
        CloseHandle(hSnap);
        return FALSE;
    }

    _tcscpy(ProcessName,me32.szExePath);
    // clean up the snapshot object. 
    CloseHandle (hSnap); 
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: IsFirstModuleLoaded
// Object: Allow to know if first module of a process is loaded
//          (it's allow to know if the Pe loader has finished is job)
// Parameters :
//     in : DWORD ProcessID : ID of process
// Return : TRUE if first module is loaded, FALSE on error
//-----------------------------------------------------------------------------
BOOL CProcessHelper::IsFirstModuleLoaded(DWORD ProcessID)
{
    // tricks : if first module is not loaded, Module32First failed
    
    MODULEENTRY32 me32 = {0}; 
    HANDLE hSnap;

    hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,ProcessID);
    if (hSnap == INVALID_HANDLE_VALUE) 
    {
        return FALSE; 
    }
    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32); 
 
    // Walk the module list of the process
    if (!Module32First(hSnap, &me32))
    {
        CloseHandle(hSnap);
        return FALSE;
    }
    // clean up the snapshot object. 
    CloseHandle (hSnap); 
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SuspendProcess
// Object: suspend all thread of a process
// Parameters :
//     in : DWORD ProcessID : ID of process
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CProcessHelper::SuspendProcess(DWORD ProcessID)
{
    BOOL bSuccess;
    HANDLE hSnapshot;

    hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    bSuccess=CProcessHelper::SuspendProcess(hSnapshot,ProcessID);
    CloseHandle(hSnapshot);

    return bSuccess;
}

//-----------------------------------------------------------------------------
// Name: ResumeProcess
// Object: resume all thread of a process
// Parameters :
//     in : DWORD ProcessID : ID of process
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CProcessHelper::ResumeProcess(DWORD ProcessID)
{
    BOOL bSuccess;
    HANDLE hSnapshot;

    hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    bSuccess=CProcessHelper::ResumeProcess(hSnapshot,ProcessID);
    CloseHandle(hSnapshot);

    return bSuccess;
}

//-----------------------------------------------------------------------------
// Name: SuspendProcess
// Object: suspend all thread of a process 
// Parameters :
//     in : HANDLE hSnapshot : a specified snapshot 
//          DWORD ProcessID : ID of process
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CProcessHelper::SuspendProcess(HANDLE hSnapshot,DWORD ProcessID)
{
    BOOL bSuccess=TRUE;
    HANDLE hThread;
    THREADENTRY32 Threadentry32={0};
    Threadentry32.dwSize=sizeof(THREADENTRY32);

    // avoid deadlock
    if (ProcessID==GetCurrentProcessId())
        return FALSE;

    //////////////////////////////////////////
    // suspend all threads of the process
    //////////////////////////////////////////
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        // do a loop throw all treads of the system
        if (Thread32First(hSnapshot,&Threadentry32))
        {
            do
            {
                // check thread Process Id owner
                if (Threadentry32.th32OwnerProcessID!=ProcessID)
                    continue;
                // open thread
                hThread=OpenThread( THREAD_ALL_ACCESS,// THREAD_SUSPEND_RESUME 
                                    FALSE,
                                    Threadentry32.th32ThreadID
                                    );
                // suspend it
                if (SuspendThread(hThread)==(DWORD)-1)
                    bSuccess=FALSE;
                // close handle of opened thread
                CloseHandle(hThread);

            }while(Thread32Next(hSnapshot,&Threadentry32));
        }
    }
    return bSuccess;
}

//-----------------------------------------------------------------------------
// Name: ResumeProcess
// Object: resume all thread of a process
// Parameters :
//     in : HANDLE hSnapshot : a specified snapshot 
//          DWORD ProcessID : ID of process
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CProcessHelper::ResumeProcess(HANDLE hSnapshot,DWORD ProcessID)
{
    BOOL bSuccess=TRUE;
    HANDLE hThread;
    THREADENTRY32 Threadentry32={0};
    Threadentry32.dwSize=sizeof(THREADENTRY32);

    //////////////////////////////////////////
    // resume all threads of the process
    //////////////////////////////////////////
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        // do a loop throw all treads of the system
        if (Thread32First(hSnapshot,&Threadentry32))
        {
            do
            {
                // check thread Process Id owner
                if (Threadentry32.th32OwnerProcessID!=ProcessID)
                    continue;
                // open thread
                hThread=OpenThread( THREAD_ALL_ACCESS,// THREAD_SUSPEND_RESUME 
                                    FALSE,
                                    Threadentry32.th32ThreadID
                                    );
                // resume it
                if (ResumeThread(hThread)==(DWORD)-1)
                    bSuccess=FALSE;
                // close handle of opened thread
                CloseHandle(hThread);
            }while(Thread32Next(hSnapshot,&Threadentry32));
        }
    }
    return bSuccess;
}