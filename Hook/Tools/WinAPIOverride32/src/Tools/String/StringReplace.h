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
// Object: replace a string by another one
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CStringReplace
{
public:
    static void Replace(TCHAR* pszInputString,TCHAR* pszOutputString,TCHAR* pszOldStr,TCHAR* pszNewStr);
    static void ReplaceA(CHAR* pszInputString,CHAR* pszOutputString,CHAR* pszOldStr,CHAR* pszNewStr);
    static void ReplaceW(WCHAR* pszInputString,WCHAR* pszOutputString,WCHAR* pszOldStr,WCHAR* pszNewStr);

    static void Replace(TCHAR* pszInputString,TCHAR* pszOutputString,TCHAR* pszOldStr,TCHAR* pszNewStr,BOOL CaseSensitive);
    static void ReplaceA(CHAR* pszInputString,CHAR* pszOutputString,CHAR* pszOldStr,CHAR* pszNewStr,BOOL CaseSensitive);
    static void ReplaceW(WCHAR* pszInputString,WCHAR* pszOutputString,WCHAR* pszOldStr,WCHAR* pszNewStr,BOOL CaseSensitive);

    static SIZE_T ComputeMaxRequieredSizeForReplace(TCHAR* pszInputString,TCHAR* pszOldStr,TCHAR* pszNewStr);
};
