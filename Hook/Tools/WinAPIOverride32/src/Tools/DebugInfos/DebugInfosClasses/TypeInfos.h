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
// Object: manages debug informations for types
//-----------------------------------------------------------------------------
#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "../DiaSDK/include/dia2.h"
#include "../DiaSDK/include/cvconst.h"
#include "../../String/AnsiUnicodeConvert.h"
#include "SymbolLocation.h"

#pragma once

class CTypeInfos
{
private:
    BOOL GetType(IDiaSymbol* pSymbol, BOOL bBaseType );
public:
    DWORD DataKind;
    TCHAR* Name;     // name of parameter or local var
    TCHAR* TypeName; // name of type
    BOOL bConst;     // TRUE if const
    BOOL bVolatile;  // TRUE if volatile
    BOOL bUnaligned; // TRUE if unaligned
    BOOL bEnum;      // TRUE if enum
    BOOL bFunction;  // TRUE if function
    BOOL bRefPointer;// TRUE if pointer pass as ref (& instead of *)
    BOOL bUserDefineType;// TRUE if user define type
    DWORD UserDefineTypeKind;
    DWORD PointerLevel;// pointer level : 0 if passed by value, 1 for simple pointer *, 2 for pointer of pointer **, ...
    ULONGLONG Size;  // size of type in byte
    DWORD BaseType;  // base type value, see enum BasicType in cvconst.h
    CSymbolLocation* pSymbolLocation;// location of type
    static BOOL IsParam(DWORD DataKind);// TRUE if it is a parameter
    BOOL GetPrettyName(TCHAR* pszPrettyName,SIZE_T pszPrettyNameMaxSize);// forge name for current type

    CTypeInfos(void);
    ~CTypeInfos(void);
    BOOL Parse(IDiaSymbol *pSymbol);
};
