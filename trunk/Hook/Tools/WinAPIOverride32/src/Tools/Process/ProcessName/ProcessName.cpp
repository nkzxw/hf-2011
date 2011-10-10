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
// Object: Process name retriever, even if process was started with a CREATE_SUSPENDED flag
//         and if first module is not loaded
//-----------------------------------------------------------------------------

#include "ProcessName.h"
#include <stdio.h>
#include <Psapi.h> // for ::GetProcessImageFileName
#pragma comment (lib,"Psapi.lib")

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif
#define CProcessName_FS_NAME_MAX_SIZE 0x1000
#define CProcessName_NETWORK_DRIVE _T("\\Device\\LanmanRedirector\\")

BOOL CProcessName::GetProcessName(DWORD ProcessID,TCHAR* ProcessName)
{
    BOOL bSuccess;
    HANDLE hProcess;
    // open process
    hProcess = ::OpenProcess(PROCESS_ALL_ACCESS,FALSE,ProcessID);
    if (!hProcess)
        return FALSE;

    bSuccess = CProcessName::GetProcessName(hProcess,ProcessName);

    ::CloseHandle(hProcess);

    return bSuccess;
}
BOOL CProcessName::GetProcessName(HANDLE hProcess,TCHAR* ProcessName)
{
    *ProcessName = 0;

    // Get process name
    TCHAR FsProcessPath[CProcessName_FS_NAME_MAX_SIZE];
    if (::GetProcessImageFileName(hProcess,FsProcessPath,CProcessName_FS_NAME_MAX_SIZE)==0)
        return FALSE;
    
    if (!CProcessName::GetFsFileName(FsProcessPath,ProcessName))
        return FALSE;

    return TRUE;
}

// From device file name to DOS filename

BOOL CProcessName::GetFsFileName( TCHAR* DeviceFileName, TCHAR* fsFileName )
{
    BOOL bSuccess = FALSE;
    TCHAR DeviceName[CProcessName_FS_NAME_MAX_SIZE];
    TCHAR Drive[8] = _T("A:");

    // Iterating through the drive letters
    for (TCHAR actDrive = _T('A'); actDrive <= _T('Z'); actDrive++ )
    {
        Drive[0] = actDrive;

        // Query the device for the drive letter
        if ( ::QueryDosDevice( Drive, DeviceName, CProcessName_FS_NAME_MAX_SIZE ) != 0 )
        {
            // Network drive?
            if ( _tcsnicmp( CProcessName_NETWORK_DRIVE, DeviceName, _countof(CProcessName_NETWORK_DRIVE)) == 0 )
            {
                //Mapped network drive 
                char cDriveLetter;
                DWORD dwParam;
                TCHAR lpSharedName[CProcessName_FS_NAME_MAX_SIZE];
                if ( _stscanf(  DeviceName, 
                                CProcessName_NETWORK_DRIVE _T(";%c:%d\\%s"),  
                                &cDriveLetter, 
                                &dwParam,  
                                lpSharedName  
                              )
                              != 3 )
                    continue;
                _tcscpy( DeviceName, CProcessName_NETWORK_DRIVE );
                _tcscat( DeviceName, lpSharedName );
            }

            // Is this the drive letter we are looking for?
            if ( _tcsnicmp( DeviceName, DeviceFileName, _tcslen( DeviceName ) ) == 0 )
            {
                TCHAR* p = (TCHAR*)( DeviceFileName + _tcslen(DeviceName));
                _tcscpy(fsFileName,Drive);
                _tcscat(fsFileName, p);
                bSuccess = TRUE;
                break;
            }
        }
    }
    return bSuccess;
}