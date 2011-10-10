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
// Object: class helper for dumping process
//-----------------------------------------------------------------------------

#include "Dump.h"

// process must have PROCESS_QUERY_INFORMATION|PROCESS_VM_READ rights
BOOL CDump::Dump(HANDLE hProcess,HMODULE hModule)
{
    PBYTE pbuffer;
    SIZE_T ReallyRead=0;
    TCHAR pszFileName[MAX_PATH]; 
    TCHAR psz[MAX_PATH]; 
    OPENFILENAME ofn;
    TCHAR* pcExt;
    MODULEENTRY32 me32 = {0}; 
    HANDLE hModuleSnap;
    BOOL bFound=FALSE;
    DWORD dwProcessId;
    CProcessAndThreadID ProcesAndThreadID;
    dwProcessId=ProcesAndThreadID.GetProcessId(hProcess);

    if (dwProcessId==0)
    {
#if (!defined(TOOLS_NO_MESSAGEBOX))
        _stprintf(psz,_T("Error getting process ID for process handle 0x%p"),hProcess);
        MessageBox(NULL,psz,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
#endif
        return FALSE;
    }

    // search module information
    hModuleSnap =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,dwProcessId);
    if (hModuleSnap == INVALID_HANDLE_VALUE) 
    {
        CAPIError::ShowLastError();
        return FALSE; 
    }
    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32); 
 
    // Walk the module list of the process
    if (!Module32First(hModuleSnap, &me32))
    {
        CloseHandle(hModuleSnap);
        CAPIError::ShowLastError();
        return FALSE;
    }
    do 
    { 
        if (me32.hModule==hModule)
        {
            bFound=TRUE;
            break;
        }
    } 
    while (Module32Next(hModuleSnap, &me32)); 
 
    // clean up the snapshot object. 
    CloseHandle (hModuleSnap); 

    // if hModule not found in process
    if (!bFound)
    {
#if (!defined(TOOLS_NO_MESSAGEBOX))
        _stprintf(psz,_T("Module with handle 0x%p not found in process 0x%x"),hModule,dwProcessId);
        MessageBox(NULL,psz,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
#endif
        return FALSE;
    }

    _tcscpy(pszFileName,me32.szExePath);
    // change ProcessName extension to .dmp
    if (*pszFileName)
    {
        // change its extension to .dmp
        pcExt=_tcsrchr(pszFileName,'.');
        if (pcExt)
        {
            *pcExt=0;
            _tcscat(pszFileName,_T(".dmp"));
        }
    }

    // allocate enough memory
    pbuffer=new BYTE[me32.modBaseSize];
    ReallyRead=0;
    // read remote process memory
    if (!ReadProcessMemory(hProcess,
            me32.modBaseAddr,
            (LPVOID)pbuffer,
            me32.modBaseSize,//SIZE_T nSize,
            &ReallyRead
        ))
    {
        if (ReallyRead==0)
        {
            CAPIError::ShowLastError();
            delete[] pbuffer;
            return FALSE;
        }
#if (!defined(TOOLS_NO_MESSAGEBOX))
        // else
        _stprintf(psz,_T("Error reading memory.\r\nOnly first 0x%X bytes are readable"),ReallyRead);
        MessageBox(NULL,psz,_T("Warning"),MB_OK|MB_ICONWARNING|MB_TOPMOST);
#endif
    }

    // save file dialog
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=NULL;
    ofn.hInstance=NULL;
    ofn.lpstrFilter=_T("dmp\0*.dmp\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt=_T("dmp");
    ofn.lpstrFile=pszFileName;
    ofn.nMaxFile=MAX_PATH;
    
    if (!GetSaveFileName(&ofn))
    {
        delete[] pbuffer;
        return TRUE;
    }

    // write dump
    HANDLE hFile = CreateFile(
        ofn.lpstrFile,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, 
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        CAPIError::ShowLastError();
        delete[] pbuffer;
        return FALSE;
    }

    if (!WriteFile(hFile,
                    pbuffer,
                    ReallyRead,
                    &ReallyRead,
                    NULL
                    ))
    {
        CAPIError::ShowLastError();
        CloseHandle(hFile);
        delete[] pbuffer;
        return FALSE;
    }

    CloseHandle(hFile);
    delete[] pbuffer;
#if (!defined(TOOLS_NO_MESSAGEBOX))
    MessageBox(NULL,_T("Dump successfully completed"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
#endif
    return TRUE;
}

BOOL CDump::Dump(DWORD dwProcessID)
{
    BOOL bRet;
    // get process handle

    MODULEENTRY32 me32 = {0}; 
    HANDLE hModuleSnap =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,dwProcessID);
    if (hModuleSnap == INVALID_HANDLE_VALUE) 
    {
        CAPIError::ShowLastError();
        return FALSE; 
    }

    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32); 
 
    // Walk the module list of the process
    if (!Module32First(hModuleSnap, &me32))
    {
        CloseHandle(hModuleSnap);
        CAPIError::ShowLastError();
        return FALSE;
    }
    CloseHandle(hModuleSnap);

    bRet=CDump::Dump(dwProcessID,me32.hModule);

    return bRet;
}

BOOL CDump::Dump(DWORD dwProcessID,HMODULE hModule)
{
    HANDLE hProcess;

    BOOL bRet;
    // get process handle
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |PROCESS_VM_READ,
                                        FALSE,dwProcessID);

    bRet=CDump::Dump(hProcess,hModule);

    // close hande to process
    CloseHandle(hProcess);

    return bRet;
}