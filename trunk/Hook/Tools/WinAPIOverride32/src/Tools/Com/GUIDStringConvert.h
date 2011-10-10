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
// Object: IID <--> Interface name and CLSID <--> class name convert routines
//-----------------------------------------------------------------------------

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include <Windows.h>
#include <objbase.h>
#pragma comment(lib,"ole32")

#include "../String/AnsiUnicodeConvert.h"
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#define CGUIDSTRINGCONVERT_STRING_GUID_SIZE 50 // string guid size in TCHAR

class CGUIDStringConvert
{
public:
    static BOOL TcharFromCLSID(IN CLSID* pClsid,OUT TCHAR* pszCLSID);
    static BOOL CLSIDFromTchar(IN TCHAR* pszClsid,OUT CLSID* pClsid);
    static BOOL GetClassProgId(IN CLSID* pClsid,OUT TCHAR* pszProgId,IN LONG pszProgIdNameMaxSize);
    static BOOL CLSIDFromProgId(IN TCHAR* pszProgId,OUT CLSID* pClsid);
    static BOOL GetClassName(IN CLSID* pClsid,OUT TCHAR* pszCLSIDName,IN LONG pszCLSIDNameMaxSize);

    static BOOL TcharFromIID(IN IID* pIid,OUT TCHAR* pszIID);
    static BOOL IIDFromTchar(IN TCHAR* pszIID,OUT IID* pIid);
    static BOOL GetInterfaceName(IN TCHAR* pszIID,OUT TCHAR* pszIIDName,IN LONG pszIIDNameMaxSize);
    static BOOL IIDFromInterfaceName(IN TCHAR* pszIIDName,OUT TCHAR* pszIID);

    static BOOL IsKnownIID(IN TCHAR* pszIID);
    static BOOL IsKnownCLSID(IN TCHAR* pszClsid);
};
