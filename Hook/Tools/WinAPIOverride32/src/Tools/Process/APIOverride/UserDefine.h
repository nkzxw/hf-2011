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
// Object: parse c/c++ defines integer definitions (don't support float/double or string)
//-----------------------------------------------------------------------------

#pragma once

#include "SupportedParameters.h"
#include "../../LinkList/LinkListItem.h"
#include "../../LinkList/LinkList.h"

#define CUserDefine_DEFINE_NAME_MAX_SIZE 256

class CUserDefine
{
public:
    typedef void (*pfReportError)(IN TCHAR* const ErrorMessage,IN PBYTE const UserParam);

    CUserDefine();
    ~CUserDefine();
    
    BOOL DefineToString(TCHAR* const VarName,PBYTE const Value,IN OUT TCHAR** pString);
    BOOL Parse(TCHAR* const DefinePath,TCHAR* const UserDefinesRelativePath,pfReportError const ReportError,PBYTE const ReportErrorUserParam);

protected:
    typedef struct tagDefineMapContent 
    {
        PBYTE Value;
        TCHAR Name[CUserDefine_DEFINE_NAME_MAX_SIZE];
    }DEFINE_MAP_CONTENT;
    CLinkList* pDefineMap;

    pfReportError ReportError;
    PBYTE ReportErrorUserParam;

    BOOL AddDefineValue(TCHAR* Name, PBYTE Value);
    BOOL GetDefineValue(TCHAR* Name, OUT PBYTE* pValue);
    BOOL Parse(TCHAR* const AbsoluteFilePath);
    BOOL ParseLine(TCHAR* Line);

    static BOOL StaticDefineGetValue(IN TCHAR* const Expression,OUT ULONG_PTR* pValue,PBYTE StringToValueFunctionUserParam);
};
