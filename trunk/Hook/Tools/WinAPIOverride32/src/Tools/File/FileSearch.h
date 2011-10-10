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
// Object: File Searching helper
//-----------------------------------------------------------------------------

#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <Windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

// return TRUE to continue parsing, FALSE to stop it
typedef BOOL (pfFileFoundCallBack)(TCHAR* Directory,WIN32_FIND_DATA* pWin32FindData,PVOID UserParam);

class CFileSearch
{
public:
    static BOOL IsDirectory(WIN32_FIND_DATA* pWin32FindData);
    static BOOL Search(TCHAR* PathWithWildChar,BOOL SearchInSubDirectories,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam);
    static BOOL Search(TCHAR* PathWithWildChar,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam);
    static BOOL Search(TCHAR* PathWithWildChar,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL* pSearchCanceled);
    static BOOL SearchMultipleNames(TCHAR* Directory, TCHAR** FileNameWithWildCharArray,SIZE_T FileNameWithWildCharArraySize,BOOL SearchInSubDirectories,HANDLE hCancelEvent,pfFileFoundCallBack FileFoundCallBack,PVOID UserParam,BOOL* pSearchCanceled);
};