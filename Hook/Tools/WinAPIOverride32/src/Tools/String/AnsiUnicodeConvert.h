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
// Object: class helper to convert ansi to unicode, and unicode to ansi
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CAnsiUnicodeConvert
{
public:
    SIZE_T static AnsiToUnicode(LPCSTR pszA,OUT LPWSTR* ppszW);
    SIZE_T static AnsiToUnicode(LPCSTR pszA,OUT LPWSTR pszW,SIZE_T pszWMaxSize);
    SIZE_T static UnicodeToAnsi(LPCWSTR pszW,OUT LPSTR* ppszA);
    SIZE_T static UnicodeToAnsi(LPCWSTR pszW,OUT LPSTR pszA,SIZE_T pszAMaxSize);
    SIZE_T static UnicodeToTchar(LPCWSTR pszW,OUT LPTSTR psz,SIZE_T pszMaxSize);
    SIZE_T static UnicodeToTchar(LPCWSTR pszW, OUT LPTSTR* ppsz);
    SIZE_T static AnsiToTchar(LPCSTR pszA,OUT LPTSTR psz,SIZE_T pszMaxSize);
    SIZE_T static AnsiToTchar(LPCSTR pszA, OUT LPTSTR* ppsz);
    SIZE_T static TcharToAnsi(LPCTSTR psz,OUT LPSTR pszA,SIZE_T pszAMaxSize);
    SIZE_T static TcharToUnicode(LPCTSTR psz,OUT LPWSTR pszW,SIZE_T pszWMaxSize);
    SIZE_T static TcharToAnsi(LPCTSTR psz,OUT LPSTR* ppszA);
    SIZE_T static TcharToUnicode(LPCTSTR psz,OUT LPWSTR* ppszW);
};
