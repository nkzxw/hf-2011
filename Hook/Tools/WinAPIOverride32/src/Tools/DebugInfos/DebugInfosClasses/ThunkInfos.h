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
// Object: manages debug informations for thunks
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "../DiaSDK/include/dia2.h"
#include "../../String/AnsiUnicodeConvert.h"

#pragma once

class CThunkInfos
{
public:
    TCHAR* Name;
    DWORD Offset;
    DWORD SectionIndex;
    ULONGLONG Length;
    DWORD ThunkOrdinal;

    DWORD TargetOffset;
    DWORD TargetSectionIndex;
    ULONGLONG TargetRelativeVirtualAddress;

    CThunkInfos(void);
    ~CThunkInfos(void);
    BOOL Parse(IDiaSymbol* pSymbol);
    TCHAR* GetOrdinalString();
};
