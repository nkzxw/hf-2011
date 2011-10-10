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
// Object: standard File operations
//-----------------------------------------------------------------------------

#include "StdFileOperations.h"
#include <malloc.h>

//-----------------------------------------------------------------------------
// Name: DoesFileExists
// Object: check if file exists
// Parameters :
//     in : TCHAR* FullPath : full path
// Return : TRUE if file exists
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::DoesFileExists(TCHAR* FullPath)
{
    BOOL bFileExists=FALSE;

    HANDLE hFile=CreateFile(FullPath,
                            FILE_READ_ATTRIBUTES, 
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING, 
                            0, 
                            NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        bFileExists=TRUE;
        CloseHandle(hFile);
    }
    return bFileExists;
}

//-----------------------------------------------------------------------------
// Name: GetFileSize
// Object: Get file size 
// Parameters :
//     in : TCHAR* FullPath : full path
//     our : DWORD* pFileSize : file size
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetFileSize(TCHAR* FullPath,OUT DWORD* pFileSize)
{
    BOOL bRet=TRUE;
    // Open the file 
    HANDLE hFile=CreateFile(FullPath,GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, 0, NULL);

    if( hFile == INVALID_HANDLE_VALUE ) 
        return FALSE; 
    // Obtain the size of the file 
    *pFileSize =::GetFileSize( hFile, NULL ); 
    if(*pFileSize==INVALID_FILE_SIZE) 
    {
        bRet=FALSE;
    }

    CloseHandle(hFile);
    return bRet; 
}

//-----------------------------------------------------------------------------
// Name: GetFileName
// Object: Get file name from FullPath
// Parameters :
//     in : TCHAR* FullPath : full path
// Return : pointer on FileName inside FullPath buffer, NULL on error
//-----------------------------------------------------------------------------
TCHAR* CStdFileOperations::GetFileName(TCHAR* FullPath)
{
    if (IsBadReadPtr(FullPath,sizeof(TCHAR)))
        return NULL;

    TCHAR* psz;

    // find last '\'
    psz=_tcsrchr(FullPath,'\\');

    // if not found
    if (!psz)
        return FullPath;

    // go after '\'
    psz++;

    return psz;
}

//-----------------------------------------------------------------------------
// Name: GetFilePath
// Object: Get Path from FullPath keeps last '\'
// Parameters :
//     in : TCHAR* FullPath : full path
//          DWORD PathMaxSize : path max size in TCHAR
//          TCHAR* Path : app path with ending '\'
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetFilePath(IN TCHAR* FullPath,OUT TCHAR* Path,IN DWORD PathMaxSize)
{
    if (IsBadReadPtr(FullPath,sizeof(TCHAR)))
        return FALSE;

    if (IsBadWritePtr(Path,sizeof(TCHAR)))
        return FALSE;

    TCHAR* psz;
    // don't put *Path=0; in case FullPath and Path point to the same buffer
    // *Path=0;

    // find last '\'
    psz=_tcsrchr(FullPath,'\\');

    // if not found
    if (!psz)
    {
        *Path=0;
        return FALSE;
    }

    // we want to keep '\'
    psz++;
    if (PathMaxSize<(DWORD)(psz-FullPath))
    {
        *Path=0;
        return FALSE;
    }

    _tcsncpy(Path,FullPath,psz-FullPath);
    Path[psz-FullPath]=0;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ChangeFileExt
// Object: replace old extension of FileName by the new provided one
// Parameters :
//     in : TCHAR* FileName : filename, buffer should long enough to support new extension
//          TCHAR* NewExtension : new extension without '.'
// Return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::ChangeFileExt(IN OUT TCHAR* FileName,IN TCHAR* NewExtension)
{
    if (IsBadReadPtr(NewExtension,sizeof(TCHAR)))
        return FALSE;

    TCHAR* psz=CStdFileOperations::GetFileExt(FileName);
    if (!psz)
        return FALSE;

    if (IsBadWritePtr(psz,(_tcslen(NewExtension)+1)*sizeof(TCHAR)))
        return FALSE;

    _tcscpy(psz,NewExtension);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RemoveFileExt
// Object: remove extension of FileName
// Parameters :
//     in : TCHAR* FileName : filename
// Return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::RemoveFileExt(IN OUT TCHAR* FileName)
{
    TCHAR* psz=_tcsrchr(FileName,'.');
    if (!psz)
        return FALSE;
    
    *psz=0;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetFileExt
// Object: get pointer on file extension
// Parameters :
//     in : TCHAR* FileName : filename
// Return : pointer on file ext in FileName buffer, NULL on error
//-----------------------------------------------------------------------------
TCHAR* CStdFileOperations::GetFileExt(IN TCHAR* FileName)
{
    if (IsBadReadPtr(FileName,sizeof(TCHAR)))
        return NULL;

    TCHAR* psz;

    // find last point
    psz=_tcsrchr(FileName,'.');

    // if not found
    if (!psz)
        return NULL;

    // go after point
    psz++;
    
    return psz;
}

//-----------------------------------------------------------------------------
// Name: DoesExtensionMatch
// Object: check if filename has the same extension as the provided one
// Parameters :
//     in : TCHAR* FileName : filename
//          TCHAR* Extension : extension to check (without "." like "exe", "zip", ...)
// Return : TRUE if extension match
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::DoesExtensionMatch(IN TCHAR* FileName,IN TCHAR* Extension)
{
    if (IsBadReadPtr(Extension,sizeof(TCHAR)))
        return NULL;

    TCHAR* psz;
    
    // get extension
    psz=CStdFileOperations::GetFileExt(FileName);
    if(!psz)
        return FALSE;

    // check extension
    return (_tcsicmp(psz,Extension)==0);
}

//-----------------------------------------------------------------------------
// Name: GetAppName
// Object: get current application name full path
// Parameters :
//     in : DWORD ApplicationNameMaxSize : user allocated buffer max size
//     in out : TCHAR* ApplicationName : fill full path
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetAppName(OUT TCHAR* ApplicationName,IN DWORD ApplicationNameMaxSize)
{
    *ApplicationName=0;
    return ::GetModuleFileName(::GetModuleHandle(NULL),ApplicationName,ApplicationNameMaxSize);
}

//-----------------------------------------------------------------------------
// Name: GetAppPath
// Object: get current application path (keep last '\')
// Parameters :
//     in : DWORD ApplicationPathMaxSize : user allocated buffer max size
//     in out : TCHAR* ApplicationPath : fill a user allocated buffer
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetAppPath(OUT TCHAR* ApplicationPath,IN DWORD ApplicationPathMaxSize)
{
    if (!CStdFileOperations::GetAppName(ApplicationPath,ApplicationPathMaxSize))
        return FALSE;
    return CStdFileOperations::GetFilePath(ApplicationPath,ApplicationPath,ApplicationPathMaxSize);
}

//-----------------------------------------------------------------------------
// Name: GetModulePath
// Object: get specified module path (keep last '\')
// Parameters :
//     in : HMODULE hModule : Module handle
//          DWORD ModulePathMaxSize : user allocated buffer max size
//     in out : TCHAR* ModulePath : fill a user allocated buffer
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetModulePath(IN HMODULE hModule,OUT TCHAR* ModulePath,DWORD ModulePathMaxSize)
{
    // get module file name
    DWORD dwRes=::GetModuleFileName(hModule,ModulePath,ModulePathMaxSize);
    // in case of failure of if buffer is too small
    if ((dwRes==0)||(dwRes==ModulePathMaxSize))
        return FALSE;

    return CStdFileOperations::GetFilePath(ModulePath,ModulePath,ModulePathMaxSize);
}

//-----------------------------------------------------------------------------
// Name: IsFullPath
// Object: Check if filename contains full path
// Parameters :
//     in : TCHAR* FileName : file name to check 
// Return : TRUE if contains full path, FALSE if FileName contains no path info (filename only)
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::IsFullPath(TCHAR* FileName)
{
    // look for a backslash
    return (_tcschr(FileName,'\\')!=0);
}

//-----------------------------------------------------------------------------
// Name: GetTempFileName
// Object: get current application path (keep last '\')
// Parameters :
//     in : TCHAR* BaseFileName : name used to create temp file
//     in out : TCHAR* TempFileName: This buffer should be MAX_PATH characters
//                  to accommodate the path plus the terminating null character
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetTempFileName(IN TCHAR* BaseFileName,IN OUT TCHAR* TempFileName)
{
    TCHAR Path[MAX_PATH];
    if (!CStdFileOperations::GetFilePath(BaseFileName,Path,MAX_PATH))
        return FALSE;
    return (::GetTempFileName(Path,_T("~"),0,TempFileName)!=0);
}

//-----------------------------------------------------------------------------
// Name: CreateDirectory
// Object: create directory with all intermediate directories 
// Parameters :
//     in : TCHAR* Directory : Directory to create
// Return : TRUE on success or if directory was already existing
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::CreateDirectory(TCHAR* Directory)
{
    DWORD dwLastError;
    size_t Len=_tcslen(Directory);
    // remove ending back slash if any
    if (Directory[Len-1]=='\\')
        Directory[Len-1]=0;

    // create directory
    if (::CreateDirectory(Directory,NULL)!=0)
        return TRUE;

    // on failure
    // get last error
    dwLastError=GetLastError();
    switch(dwLastError)
    {
    case ERROR_ALREADY_EXISTS:
        // directory already exists --> nothing to do
        return TRUE;
    case ERROR_PATH_NOT_FOUND:
        {
            // upper dir doesn't exists --> create it

            // get upper dir
            TCHAR* psz;
            TCHAR* UpperDir=(TCHAR*)_alloca((_tcslen(Directory)+1)*sizeof(TCHAR));
            _tcscpy(UpperDir,Directory);
            // find last '\\'
            psz=_tcsrchr(UpperDir,'\\');
            if (!psz)
                return FALSE;
            // ends upper dir
            *psz=0;

             if (!CStdFileOperations::CreateDirectory(UpperDir))
                 return FALSE;

             return (::CreateDirectory(Directory,NULL)!=0);
        }
    default:
        return FALSE;
    }
}

//-----------------------------------------------------------------------------
// Name: CreateDirectoryForFile
// Object: create directory if directory does not exists for specified FileName
//         can be used before calling CreateFile, to assume directory exists and avoid CreateFile failure
// Parameters :
//     in : TCHAR* FileNameToCreate : name of file that is going to be created
// Return : TRUE on success or if directory was already existing
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::CreateDirectoryForFile(TCHAR* FileNameToCreate)
{
    TCHAR szPath[MAX_PATH];
    // get path
    CStdFileOperations::GetFilePath(FileNameToCreate,szPath,MAX_PATH);
    // check if directory already exists
    if (CStdFileOperations::DoesDirectoryExists(szPath))
        return TRUE;
    // create directory
    return CStdFileOperations::CreateDirectory(szPath);
}

//-----------------------------------------------------------------------------
// Name: DoesDirectoryExists
// Object: check if directory exists
// Parameters :
//     in : TCHAR* Directory : directory name
// Return : TRUE if directory exists
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::DoesDirectoryExists(TCHAR* Directory)
{
    BOOL bFileExists=FALSE;
    HANDLE hFile=CreateFile (
                            Directory,
                            GENERIC_READ,
                            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS,
                            NULL
                            );
    if (hFile != INVALID_HANDLE_VALUE)
    {
        bFileExists=TRUE;
        CloseHandle(hFile);
    }
    return bFileExists;
}

//-----------------------------------------------------------------------------
// Name: IsDirectory
// Object: assume FullPath exists and is a directory
// Parameters :
//     in : TCHAR* Directory : directory name
// Return : TRUE if directory exists
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::IsDirectory(TCHAR* FullPath)
{
    DWORD Ret;
    Ret=::GetFileAttributes(FullPath);
    if (Ret==INVALID_FILE_ATTRIBUTES)
        return FALSE;

    return (Ret & FILE_ATTRIBUTE_DIRECTORY);
}

//-----------------------------------------------------------------------------
// Name: GetAbsolutePath
// Object: get absolute path from relative path
// Parameters :
//     in : TCHAR* RelativePath : Relative path
//     out : TCHAR* AbsolutePath : absolute path must be at least MAX_PATH len in TCHAR
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStdFileOperations::GetAbsolutePath(TCHAR* RelativePath,OUT TCHAR* AbsolutePath)
{
    LPTSTR pszFileName = NULL;
    if (AbsolutePath)
        *AbsolutePath=0;
#ifdef _DEBUG
    if (RelativePath == AbsolutePath)
    {
        // Relative path must differ from absolute path
        DebugBreak();
    }
#endif 
    // _tfullpath can be used too for more portable way (in this file we call as much API as we can to be linked only to kernel32)
    return (::GetFullPathName(RelativePath,MAX_PATH,AbsolutePath,&pszFileName)!=0);
}