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

// _WINSOCKAPI_ must be set in C/C++ preprocessor options

#include "ParameterInfo.h"
#include "../injected_dll/Struct.h" // for winapioverride calling conventions
#include "../../../LinkList/LinkListSimple.h"
#include <windows.h>
#include <cor.h>
#include <tchar.h>

namespace NET
{

class CFunctionInfo
{
private:
    BOOL bIsStatic;
    ULONG CallingConvention;
    ULONG ParamCount;
protected:
    BOOL CFunctionInfo::mParse(IMetaDataImport* pMetaDataImport,mdTypeDef ClassToken,mdToken FunctionToken);
public:
    CFunctionInfo(void);
    ~CFunctionInfo(void);

    TCHAR szName[MAX_LENGTH];// could be private, but putting it public allows fast access
    mdToken FunctionToken;

    CParameterInfo* pReturnInfo;
    CLinkListSimple* pParameterInfoList;// list of CParameterInfo*
    BOOL GetName(OUT TCHAR* pszName,DWORD NameMaxLen);
    BOOL GetStringCallingConvention(OUT TCHAR* pszCallingConvention,DWORD MaxLen);
    ULONG GetCallingConvention();
    tagCALLING_CONVENTION GetWinApiOverrideCallingConvention();
    ULONG GetParamCount();
    mdToken GetToken();
    BOOL IsManaged();
    BOOL IsStatic();

    static CFunctionInfo* CFunctionInfo::Parse(IMetaDataImport* pMetaDataImport,mdTypeDef ClassToken,mdToken FunctionToken);
    
};

}