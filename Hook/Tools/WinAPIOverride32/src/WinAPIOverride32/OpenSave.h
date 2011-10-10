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
// Object: manages log saving and reloading
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "../tools/String/StrToHex.h"
#include "../tools/Process/APIOverride/ApiOverride.h"
#include "../Tools/LinkList/LinkList.h"
#include "StructsAndDefines.h"
#include "../Tools/File/textfile.h"
#include "../tools/Process/APIOverride/SupportedParameters.h"
#include "../Tools/Version/Version.h"

class COpenSave
{
private:
    typedef union tagBeforeV3_1CallTime
    {
        struct tagStrBeforeV3_1CallTime
        {
            DWORD   MilliSeconds:10, 
                    Second:6,
                    Minute:6,
                    Hour:5,
                    unused:5;
        }strBeforeV3_1CallTime;
        DWORD dw;
    }BEFORE_V3_1CALLTIME,*PBEFORE_V3_1CALLTIME;

    static BOOL ReadXMLValue(TCHAR* FullString,TCHAR* Markup,DWORD* pValue,TCHAR** pPointerAfterEndingMarkup);
    static BOOL ReadXMLValue(TCHAR* FullString,TCHAR* Markup,PBYTE* pValue,TCHAR** pPointerAfterEndingMarkup);
    static BOOL ReadXMLValue(TCHAR* FullString,TCHAR* Markup,BYTE** pBuffer,DWORD* pBufferLengthInByte,TCHAR** pPointerAfterEndingMarkup);
    static BOOL ReadXMLValue(TCHAR* FullString,TCHAR* Markup,TCHAR** pszValue,BOOL bUnicodeFile,TCHAR** pPointerAfterEndingMarkup);
    static BOOL ReadXMLMarkupContent(TCHAR* FullString,TCHAR* Markup,TCHAR** ppszContent,DWORD* pContentLength,TCHAR** pPointerAfterEndingMarkup);
    static void WriteXMLValue(HANDLE hFile,TCHAR* Markup,DWORD Value);
    static void WriteXMLValue(HANDLE hFile,TCHAR* Markup,PBYTE Value);
    static void WriteXMLValue(HANDLE hFile,TCHAR* Markup,PBYTE Buffer,DWORD BufferLengthInByte);
    static void WriteXMLValue(HANDLE hFile,TCHAR* Markup,TCHAR* Value);
public:
    static BOOL Load(TCHAR* pszFile);
    static BOOL Save(TCHAR* pszFile);
};
