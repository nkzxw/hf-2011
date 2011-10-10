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
// Object: text file parser
//         support ascii or little endian unicode files
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)


class CTextFile
{
private:
    static void ReportError(TCHAR* pszMsg);
public:
    // return TRUE to continue parsing, FALSE to stop parsing
    typedef BOOL (*tagLineCallBack)(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam);

    static BOOL CreateTextFile(TCHAR* FullPath,OUT HANDLE* phFile);
    static BOOL CreateOrOpenForAppending(TCHAR* FullPath,OUT HANDLE* phFile);
    static BOOL WriteText(HANDLE hFile,TCHAR* Text);
    static BOOL WriteText(HANDLE hFile,TCHAR* Text,SIZE_T LenInTCHAR);
    
    static BOOL Read(TCHAR* FileName,TCHAR** ppszContent);
    static BOOL Read(TCHAR* FileName,TCHAR** ppszContent,BOOL* pbUnicodeFile);
    static BOOL ParseLines(TCHAR* pszFileName,tagLineCallBack LineCallBack,LPVOID CallBackUserParam);
    static BOOL ParseLines(TCHAR* pszFileName,HANDLE hCancelEvent,tagLineCallBack LineCallBack,LPVOID CallBackUserParam);
};
