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
// Object: try to locate dll (use kernel32.dll only if TOOLS_NO_MESSAGEBOX is defined)
//-----------------------------------------------------------------------------

#include "DllFinder.h"
#include "DllSideBySideAssembyFinder.h"


BOOL CDllFinder::DoesFileExists(TCHAR* FullPath)
{
    BOOL bFileExists=FALSE;

    HANDLE hFile=CreateFile(FullPath,FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        bFileExists=TRUE;
        CloseHandle(hFile);
    }
    return bFileExists;
}

BOOL CDllFinder::FindDll(TCHAR* tWorkingDirectory,TCHAR* tDllName,OUT TCHAR* tFullPath)
{
    return CDllFinder::FindDll(NULL,tWorkingDirectory,tDllName,tFullPath,TRUE);
}
BOOL CDllFinder::FindDll(TCHAR* tWorkingDirectory,TCHAR* tDllName,OUT TCHAR* tFullPath,BOOL bShowManualSearchDialogIfNotFound)
{
    return CDllFinder::FindDll(NULL,tWorkingDirectory,tDllName,tFullPath,bShowManualSearchDialogIfNotFound);
}

// OUT TCHAR* tFullPath : provided pointer have be at least MAX_PATH TCHAR
BOOL CDllFinder::FindDll(TCHAR* ImportingModulePath,TCHAR* tWorkingDirectory,TCHAR* tDllName,OUT TCHAR* tFullPath)
{
    return CDllFinder::FindDll(ImportingModulePath,tWorkingDirectory,tDllName,tFullPath,TRUE);
}

// OUT TCHAR* tFullPath : provided pointer have be at least MAX_PATH TCHAR
// bShowManualSearchDialogIfNotFound : TRUE to show Dialog Box to select dll when dll not found
BOOL CDllFinder::FindDll(TCHAR* ImportingModulePath,TCHAR* tWorkingDirectory,TCHAR* tDllName,OUT TCHAR* tFullPath,BOOL bShowManualSearchDialogIfNotFound)
{
    //The \WINNT\SYSTEM32 directory. 
    //The directory of the executable for the process that is loading the DLL. 
    //The current directory of the process that is loading the DLL. 
    //The \WINNT directory. 
    //A directory listed in the PATH environment variable. 
    
    TCHAR psz[MAX_PATH];
    TCHAR pszDirectory[MAX_PATH];
    TCHAR DllName[MAX_PATH];
    TCHAR* pstr;

    // in case full path is already specified
    if (CDllFinder::DoesFileExists(tDllName))
    {
        _tcscpy(tFullPath,tDllName);
        return TRUE;
    }

    // remove directory if any
    _tcsncpy(DllName,tDllName,MAX_PATH);
    DllName[MAX_PATH-1]=0;
    pstr=_tcsrchr(tDllName,'\\');
    if (pstr)
        _tcscpy(DllName,pstr);

    // check if file exists in SYSTEM32 path
    *pszDirectory=0;
    ::GetSystemDirectory(pszDirectory,MAX_PATH);
    _tcscpy(psz,pszDirectory);
    _tcscat(psz,DllName);
    if (CDllFinder::DoesFileExists(psz))
    {
        _tcscpy(tFullPath,psz);
        return TRUE;
    }

    // check if file exists in WorkingDirectory
    if (tWorkingDirectory)
    {
        // forge full name

        _tcscpy(psz,tWorkingDirectory);
        // if directory doesn't ends with '\' add it
        if (tWorkingDirectory[_tcslen(tWorkingDirectory)-1]!='\\')
            _tcscat(psz,_T("\\"));
        _tcscat(psz,DllName);

        if (CDllFinder::DoesFileExists(psz))
        {
            _tcscpy(tFullPath,psz);
            return TRUE;
        }
    }

    // check if file exists in Windows directory
    *pszDirectory=0;
    GetWindowsDirectory(pszDirectory,MAX_PATH);
    _tcscpy(psz,pszDirectory);
    _tcscat(psz,DllName);
    if (CDllFinder::DoesFileExists(psz))
    {
        _tcscpy(tFullPath,psz);
        return TRUE;
    }

    // check environment path var
    TCHAR* pszEnvPath;
    DWORD size=1024;
    DWORD dwRet;
    pszEnvPath=new TCHAR[size];
    dwRet=GetEnvironmentVariable(_T("Path"),pszEnvPath,size);
    if (dwRet!=0)
    {
        // if buffer is not large enough
        if (dwRet>size)
        {
            // increase buffer size
            size=dwRet;
            delete[] pszEnvPath;
            pszEnvPath=new TCHAR[size];
            // query env path again
            GetEnvironmentVariable(_T("Path"),pszEnvPath,size);
        }

        // for each path in pszEnvPath
        TCHAR* pszPointer;
        TCHAR* pszPointerDir;
        pszPointerDir=pszEnvPath;

        while(pszPointerDir)
        {
            pszPointer=_tcschr(pszPointerDir,';');
            if (pszPointer)
            {
                *pszPointer=0;
                pszPointer++;
            }
            if (*pszPointerDir==0)
                break;

            _tcscpy(psz,pszPointerDir);
            if (pszPointerDir[_tcslen(pszPointerDir)-1]!='\\')
                _tcscat(psz,_T("\\"));
            _tcscat(psz,DllName);
            // check if file exists
            if (CDllFinder::DoesFileExists(psz))
            {
                _tcscpy(tFullPath,psz);
                delete[] pszEnvPath;
                return TRUE;
            }
            // point to the next dir
            pszPointerDir=pszPointer;
        }
    }
    delete[] pszEnvPath;

    // check for Assembly Identities
    // looks for "Retrieve the Assembly Identities from a Manifest using C++" article on CodeProject By marc ochsenmeier (www.winitor.net) 20 Jun 2010
    if (CDllSideBySideAssembyFinder::FindSideBySideAssemby(ImportingModulePath,tWorkingDirectory,tDllName,tFullPath))
        return TRUE;

    // not found
#if (defined(TOOLS_NO_MESSAGEBOX))
    UNREFERENCED_PARAMETER(bShowManualSearchDialogIfNotFound);
#else
    if (bShowManualSearchDialogIfNotFound)
    {
        TCHAR pszTmp[2*MAX_PATH];
        _stprintf(pszTmp,_T("File %s can't be found.\r\nDo you want to search it manually ?"),DllName);
        if (MessageBox(NULL,pszTmp,_T("Warning"),MB_YESNO|MB_ICONWARNING|MB_TOPMOST)==IDYES)
        {
            *psz=0;

            // open dialog
            OPENFILENAME ofn;
            memset(&ofn,0,sizeof (OPENFILENAME));
            ofn.lStructSize=sizeof (OPENFILENAME);
            ofn.hwndOwner=NULL;
            ofn.hInstance=NULL;
            ofn.lpstrFilter=_T("exe, ocx, dll, tlb\0*.exe;*.ocx;*.dll;*.tlb\0All\0*.*\0");
            ofn.nFilterIndex = 1;
            ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
            ofn.lpstrFile=DllName;
            ofn.nMaxFile=MAX_PATH;
            ofn.lpstrTitle=_T("Select dll file");

            // get file name
            if (GetOpenFileName(&ofn))
            {
                _tcscpy(tFullPath,DllName);
                return TRUE;
            }
        }
    }
#endif
    return FALSE;
}
