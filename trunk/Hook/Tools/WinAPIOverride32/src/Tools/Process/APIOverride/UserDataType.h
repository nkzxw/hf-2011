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
// Object: manage user data type (an user union/struct enum, type renaming, ... ) 
//-----------------------------------------------------------------------------
#pragma once

#include "SupportedParameters.h"

#include "../../LinkList/LinkList.h"
#include "../../LinkList/LinkListSimple.h"

#define CUserDataType_ENUM_NAME_MAX_SIZE 256
#define CUserDataType_NumberOfAlternativePointerNames 4
#define CUserDataType_AlternativePointerNamesMaxSize 12

class CUserDataTypeManager;

class CUserDataType
{
    friend class CUserDataTypeManager;
    friend class CUserDataTypeVar;
public:
    typedef enum tagExpressionsType
    {
        TYPE_BASE,
        TYPE_RENAMING,
        TYPE_ENUM,
        TYPE_STRUCT,
        TYPE_UNION
    }EXPRESSIONS_TYPE;
    typedef void (*pfReportError)(IN TCHAR* const ErrorMessage,IN PBYTE const UserParam);

    typedef enum DEFINITION_FOUND
    {
        DEFINITION_FOUND_NOT_FOUND,
        DEFINITION_FOUND_IN_SPECIALIZED_CACHE,
        DEFINITION_FOUND_IN_DEFAULT_CACHE,
        DEFINITION_FOUND_BY_FILE_PARSING_IN_SPECIALIZED_DIRECTORY,
        DEFINITION_FOUND_BY_FILE_PARSING_IN_DEFAULT_DIRECTORY,
        DEFINITION_FOUND_IN_FULLY_SUPPORTED_LIST
    };

protected:
    SIZE_T Size;

    typedef struct tagEnumMapContent 
    {
        PBYTE Value;
        TCHAR Name[CUserDataType_ENUM_NAME_MAX_SIZE];
    }ENUM_MAP_CONTENT;
    CLinkList* pEnumMap;

    static TCHAR AlternativePointerPrefix[CUserDataType_NumberOfAlternativePointerNames][CUserDataType_AlternativePointerNamesMaxSize];

    typedef struct tagTypeNameInfos 
    {
        TCHAR Name[MAX_PATH];
        SIZE_T NbPointedTimes;
    }TYPE_NAME_INFOS;

    typedef struct tagExpression 
    {
        TCHAR* pszBegin;
    }EXPRESSION;

    static BOOL IsKeyWord(IN TCHAR* const Name);
    static BOOL StaticEnumGetValue(IN TCHAR* const Expression,OUT ULONG_PTR* pValue,PBYTE StringToValueFunctionUserParam);
    static CUserDataType* ParseEnumUnionStructContent(TCHAR* CurrentParsedFileName,TCHAR* UserDataTypePath, TCHAR* ModuleName,TCHAR* const Content,EXPRESSIONS_TYPE Type,CUserDataType* pCurrentUserType,CLinkListSimple* pParentUserDataTypeList, pfReportError const ReportError,PBYTE const ReportErrorUserParam);
    static CUserDataType* ParseContent(TCHAR* CurrentParsedFileName,TCHAR* UserDataTypePath, TCHAR* ModuleName, TCHAR* Content, TCHAR* SearchedType,CLinkListSimple* pParentUserDataTypeList,pfReportError const ReportError,PBYTE const ReportErrorUserParam);
    static CUserDataType* ParseFile(TCHAR* const FileName,TCHAR* const UserDataTypePath,TCHAR* const ModuleName,TCHAR* const SearchBaseType,CLinkListSimple* pParentUserDataTypeList,pfReportError const ReportError,PBYTE const ReportErrorUserParam);
    static void ReportDescriptionNotFoundError(TCHAR* const TypeName, TCHAR* const ModuleName,pfReportError const ReportError,PBYTE const ReportErrorUserParam);
    static CUserDataType* CUserDataType::FindOrAddUnknownTypeInDefaultList();
    static void ReplaceForbiddenFileChar(TCHAR* Str);
    static CUserDataType* Parse(TCHAR* const UserDataTypePath, TCHAR* const ModuleName, TCHAR* const TypeName,IN OUT SIZE_T* pNbPointedTimes,CLinkListSimple* pParentUserDataTypeList,pfReportError const ReportError,PBYTE const ReportErrorUserParam,DEFINITION_FOUND* pDefinitionFound);
    
public:

    EXPRESSIONS_TYPE Type;

    // for all types
    CLinkList* pNames;// list of TYPE_NAME_INFOS struct

    // for base type only
    DWORD BaseType;

    // for ! base type only    
    CLinkListSimple* pSubType;// list of CUserDataTypeVar objects. NULL for base types

    CUserDataType();
    ~CUserDataType();
    BOOL IsEnum();
    BOOL AddEnumValue(TCHAR* Name, PBYTE Value);
    BOOL GetEnumValue(TCHAR* Name, OUT PBYTE* pValue);

    // String must be at least CUserDataType_ENUM_NAME_MAX_SIZE
    BOOL EnumToString(PBYTE Value, OUT TCHAR* String);
    BOOL IsBaseType();
    SIZE_T GetSize();
    static CUserDataType* Parse(TCHAR* const UserDataTypePath, TCHAR* const ModuleName, TCHAR* const TypeName,IN OUT SIZE_T* pNbPointedTimes,pfReportError const ReportError,PBYTE const ReportErrorUserParam,DEFINITION_FOUND* pDefinitionFound);
    static void ClearCache();
};