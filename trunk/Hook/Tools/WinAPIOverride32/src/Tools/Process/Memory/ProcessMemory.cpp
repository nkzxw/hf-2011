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
// Object: class helper for reading or writing process memory
//-----------------------------------------------------------------------------

#include "processmemory.h"

//-----------------------------------------------------------------------------
// Name: CProcessMemory
// Object: 
// Parameters :
//     in  : DWORD dwProcessID : process id to work with
//           BOOL bReadOnly    : TRUE if memory will only be accessed for reading
//           BOOL bShowErrorMessage : TRUE to display error messages
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CProcessMemory::CommonConstructor(DWORD dwProcessID,BOOL bReadOnly,BOOL bShowErrorMessage)
{
    this->bShowErrorMessage=bShowErrorMessage;

    // store process ID
    this->dwProcessID=dwProcessID;

    // set access rights
    if (bReadOnly)
        this->dwDesiredAccess=PROCESS_VM_READ;//|PROCESS_QUERY_INFORMATION;
    else
        this->dwDesiredAccess=PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION;//PROCESS_ALL_ACCESS;

    // open process
    this->hProcess = OpenProcess(this->dwDesiredAccess,FALSE,this->dwProcessID);
    if ((!this->hProcess) && this->bShowErrorMessage)
    {
        TCHAR szMessage[2*MAX_PATH];
        _stprintf(szMessage,
            _T("The following error as occured when trying to opening process with pid %u\r\n"),
            this->dwProcessID);

        CAPIError::ShowLastError(szMessage);
    }
}


CProcessMemory::CProcessMemory(DWORD dwProcessID,BOOL bReadOnly,BOOL bShowErrorMessage)
{
    this->CommonConstructor(dwProcessID,bReadOnly,bShowErrorMessage);
}

CProcessMemory::CProcessMemory(DWORD dwProcessID,BOOL bReadOnly)
{
    this->CommonConstructor(dwProcessID,bReadOnly,TRUE);
}

CProcessMemory::~CProcessMemory(void)
{
    if (!this->hProcess)
        return;
    // close process handle
    CloseHandle(this->hProcess);
}


//-----------------------------------------------------------------------------
// Name: VirtualProtectEx
// Object: 
// Parameters :
//     inout  : same parameters as VirtualProtectEx without process handle
//     return : same return as VirtualProtectEx
//-----------------------------------------------------------------------------
BOOL CProcessMemory::Protect(LPVOID lpAddress,SIZE_T dwSize,DWORD flNewProtect,PDWORD lpflOldProtect)
{
    if (!this->hProcess)
        return FALSE;
    if (!VirtualProtectEx(this->hProcess,lpAddress,dwSize,flNewProtect,lpflOldProtect))
    {
        if ( this->bShowErrorMessage)
            CAPIError::ShowLastError();
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: VirtualQueryEx
// Object: 
// Parameters :
//     inout  : same parameters as VirtualQueryEx without process handle
//     return : same return as VirtualQueryEx
//-----------------------------------------------------------------------------
DWORD CProcessMemory::Query(LPCVOID lpAddress,PMEMORY_BASIC_INFORMATION lpBuffer,SIZE_T Length)
{
    if (!this->hProcess)
        return FALSE;
    if (!VirtualQueryEx(this->hProcess,lpAddress,lpBuffer,Length))
    {
        if ( this->bShowErrorMessage)
            CAPIError::ShowLastError();
        return FALSE;
    }
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: IsValidMemory
// Object: 
// Parameters :
//     in : LPCVOID lpAddress : virtual address
//          SIZE_T Length : length of memory to check
//     return : TRUE if memory is a valid memory address
//-----------------------------------------------------------------------------
BOOL CProcessMemory::IsValidMemory(LPCVOID lpAddress,SIZE_T Length)
{
    MEMORY_BASIC_INFORMATION Infos;

    if (!this->hProcess)
        return FALSE;
    if (!VirtualQueryEx(this->hProcess,lpAddress,&Infos,Length))
        return FALSE;

    return TRUE;
}



//-----------------------------------------------------------------------------
// Name: Read
// Object: 
// Parameters :
//     inout  : same parameters as ReadProcessMemory without process handle
//     return : same return as ReadProcessMemory
// Remark : on error just call the GetSystemInfo and use SYSTEM_INFO.dwPageSize
//          to make a loop with VirtualQueryEx
//-----------------------------------------------------------------------------
BOOL CProcessMemory::Read(LPCVOID lpBaseAddress,LPVOID lpBuffer,SIZE_T nSize,SIZE_T* lpNumberOfBytesRead)
{
    if (!this->hProcess)
        return FALSE;
    if (!ReadProcessMemory(this->hProcess,lpBaseAddress,lpBuffer,nSize,lpNumberOfBytesRead))
    {
        if ( this->bShowErrorMessage)
            CAPIError::ShowLastError();
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Write
// Object: 
// Parameters :
//     inout  : same parameters as WriteProcessMemory without process handle
//     return : same return as WriteProcessMemory
// Remark : on error just call the GetSystemInfo and use SYSTEM_INFO.dwPageSize
//          to make a loop with VirtualQueryEx
//-----------------------------------------------------------------------------
BOOL CProcessMemory::Write(LPVOID lpBaseAddress,LPCVOID lpBuffer,SIZE_T nSize,SIZE_T* lpNumberOfBytesWritten)
{
    if (!this->hProcess)
        return FALSE;

    BOOL bWriteSuccess;
    // try to write memory without calling virtual protect
    bWriteSuccess=WriteProcessMemory(this->hProcess,lpBaseAddress,lpBuffer,nSize,lpNumberOfBytesWritten);
    if (bWriteSuccess)
        return TRUE;

    // in case of failure try to remove memory protection
    DWORD dwProtectionFlags;
    DWORD dwScratch;
    
    // remove memory protection in case memory is locked (static string in .data for example) 
    if (!VirtualProtectEx(this->hProcess,lpBaseAddress, nSize, PAGE_EXECUTE_READWRITE, &dwProtectionFlags))
    {
        if (this->bShowErrorMessage)
            CAPIError::ShowLastError();
        return FALSE;
    }
#ifndef TOOLS_NO_MESSAGEBOX
    if(MessageBox(NULL,_T("Warning you're going to write data in a protected memory page.\r\nDo you want to continue ?"),_T("Warning"),MB_YESNO|MB_ICONWARNING|MB_TOPMOST)==IDNO)
    {
        // restore original memory protection
        VirtualProtectEx(this->hProcess,lpBaseAddress, nSize, dwProtectionFlags, &dwScratch);
        return FALSE;
    }
#endif

    bWriteSuccess=WriteProcessMemory(this->hProcess,lpBaseAddress,lpBuffer,nSize,lpNumberOfBytesWritten);
    if (!bWriteSuccess)
    {
        if ( this->bShowErrorMessage)
            CAPIError::ShowLastError();
    }
    // restore original memory protection
    VirtualProtectEx(this->hProcess,lpBaseAddress, nSize, dwProtectionFlags, &dwScratch);

    return bWriteSuccess;
}

//-----------------------------------------------------------------------------
// Name: Alloc
// Object: 
// Parameters :
//     in  : SIZE_T dwSize : size of memory to allocate
//     return : same return as VirtualAllocEx
//-----------------------------------------------------------------------------
LPVOID CProcessMemory::Alloc(SIZE_T dwSize)
{
    LPVOID RetValue;
    if (!this->hProcess)
        return NULL;
    RetValue=VirtualAllocEx(this->hProcess, NULL, dwSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if(!RetValue)
    {
        if (this->bShowErrorMessage)
            CAPIError::ShowLastError();
        return NULL;
    }
    return RetValue;
}
//-----------------------------------------------------------------------------
// Name: Free
// Object: 
// Parameters :
//     in  : address of previously allocated memory
//     return : same return as VirtualFreeEx
//-----------------------------------------------------------------------------
BOOL CProcessMemory::Free(LPVOID lpAddress)
{
    if (!this->hProcess)
        return FALSE;
    if(!VirtualFreeEx(this->hProcess, lpAddress, 0, MEM_RELEASE))
    {
        if (this->bShowErrorMessage)
            CAPIError::ShowLastError();
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetProcessHandle
// Object: return handle of current open process
// Parameters :
//     in  : address of previously allocated memory
//     return : same return as VirtualFreeEx
//-----------------------------------------------------------------------------
HANDLE CProcessMemory::GetProcessHandle()
{
    return this->hProcess;
}