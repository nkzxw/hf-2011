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
// Object: manage user data type var (an union/struct var name) : an user data type this an associated var name, 
//          number of items (in case of array), bit fields infos, the number of referenced time
//-----------------------------------------------------------------------------
#pragma once

#include "UserDataType.h"

class CUserDataTypeVar
{
    friend class CUserDataType;
public:
    typedef struct tagBitsFieldInfos
    {
        SIZE_T MaskAfterShifting;
        SIZE_T Shift;
        TCHAR VarName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE];
        SIZE_T Value;// used only for ToString methods
    }BITS_FIELD_INFOS;

    typedef struct tagBitsFieldInfosLite
    {
        SIZE_T Size;
        TCHAR VarName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE];
    }BITS_FIELD_INFOS_LITE;

    SIZE_T NbItems;
    SIZE_T NbPointedTimes;
    BOOL bParentIsUnion;

    CLinkList* pBitsFieldInfosList;// link list of BITS_FIELD_INFOS struct
    SIZE_T BitsFieldsUsedSize;
    SIZE_T GetBitsFieldRemainingSize();
    BOOL IsBitsFields();
    BOOL AddBitsField(IN BITS_FIELD_INFOS_LITE* const pBitsFieldInfos);

    CUserDataType* pUserDataType;
    void SetName(const TCHAR* VarName);

    CUserDataTypeVar();
    ~CUserDataTypeVar();
    SIZE_T GetSize();
    static CUserDataTypeVar* Parse(TCHAR* UserDataTypePath, TCHAR* ModuleName, TCHAR* TypeName, TCHAR* VarName, SIZE_T NbItems, SIZE_T NbPointedTimes,CUserDataType::pfReportError ReportError,PBYTE ReportErrorUserParam,CUserDataType::DEFINITION_FOUND* pDefinitionFound);
    BOOL ToString(PARAMETER_LOG_INFOS* const pLogInfos,TCHAR** pString,SIZE_T NbRequieredChars);

protected:
    TCHAR VarName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE];
    BOOL ToString(PARAMETER_LOG_INFOS* const pLogInfos,TCHAR** pString,const SIZE_T NbRequieredChars,const SIZE_T NbPointedTimesFromRoot);
    BOOL CheckStringEnd(TCHAR* String, SIZE_T NbRequieredChars);
    // do not call for top level (top level : NbPointedTimesFromRoot ==0 )
    BOOL MakeLogParamFromBuffer(CUserDataTypeVar* pUserDataTypeVar , PBYTE Buffer, SIZE_T BufferSize,OUT PARAMETER_LOG_INFOS* pLogInfos);
    BOOL MakeBufferFromLogParam(const IN PARAMETER_LOG_INFOS* pLogInfos, OUT PBYTE* pBuffer,OUT SIZE_T* pBufferSize);
    static CUserDataTypeVar* Parse(TCHAR* UserDataTypePath, TCHAR* ModuleName, TCHAR* TypeName, TCHAR* VarName, SIZE_T NbItems, SIZE_T NbPointedTimes,CLinkListSimple* pParentUserDataTypeList,CUserDataType::pfReportError ReportError,PBYTE ReportErrorUserParam,CUserDataType::DEFINITION_FOUND* pDefinitionFound);
};