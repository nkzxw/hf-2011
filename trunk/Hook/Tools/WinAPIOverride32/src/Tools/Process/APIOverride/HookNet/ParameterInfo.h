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
#pragma once

#include "../SupportedParameters.h"
#include "../injected_dll/Struct.h"
#include <windows.h>
#include <cor.h>
#include <malloc.h>
#include <stdio.h>
#include <tchar.h>
#define MAX_LENGTH MAX_PATH

namespace NET
{

class CParameterInfo
{
private:
    BOOL  bPointedParameter;
    BOOL  bIsOutParameter;

public:
    DWORD WinAPIOverrideType;
    DWORD StackSize;
    DWORD PointedSize;
    TCHAR szName[MAX_LENGTH];

    CParameterInfo(void);
    ~CParameterInfo(void);

    BOOL IsOutParameter();

    BOOL  GetName(OUT TCHAR* pszName,DWORD NameMaxLen);
    DWORD GetWinAPIOverrideType();
    DWORD GetStackSize();
    DWORD GetPointedSize();

    static CParameterInfo* CParameterInfo::Parse(IN IMetaDataImport *pMetaDataImport,IN PCOR_SIGNATURE pSigParam,OUT PCOR_SIGNATURE* pNextSig);
};

}