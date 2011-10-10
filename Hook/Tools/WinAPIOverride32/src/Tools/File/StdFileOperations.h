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

#pragma once

#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501
#endif

#include <Windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CStdFileOperations
{
public:
    static BOOL DoesFileExists(TCHAR* FullPath);
    static BOOL GetFileSize(TCHAR* FullPath,OUT DWORD* pFileSize);
    static BOOL DoesExtensionMatch(TCHAR* FileName,TCHAR* Extension);
    static TCHAR* GetFileExt(TCHAR* FileName);
    static BOOL ChangeFileExt(TCHAR* FileName,TCHAR* NewExtension);
    static BOOL RemoveFileExt(TCHAR* FileName);
    static TCHAR* GetFileName(TCHAR* FullPath);
    static BOOL GetFilePath(TCHAR* FullPath,OUT TCHAR* Path,DWORD PathMaxSize);
    static BOOL GetAppName(OUT TCHAR* ApplicationName,DWORD ApplicationNameMaxSize);
    static BOOL GetAppPath(OUT TCHAR* ApplicationPath,DWORD ApplicationPathMaxSize);
    static BOOL GetModulePath(IN HMODULE hModule,OUT TCHAR* ModulePath,DWORD ModulePathMaxSize);
    static BOOL GetTempFileName(TCHAR* BaseFileName,OUT TCHAR* TempFileName);
    static BOOL IsFullPath(TCHAR* FileName);
    static BOOL IsDirectory(TCHAR* FullPath);
    static BOOL CreateDirectoryForFile(TCHAR* FileNameToCreate);
    static BOOL CreateDirectory(TCHAR* Directory);
    static BOOL DoesDirectoryExists(TCHAR* Directory);
    static BOOL GetAbsolutePath(TCHAR* RelativePath,OUT TCHAR* AbsolutePath);
};