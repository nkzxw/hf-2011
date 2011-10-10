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
#include "UserDataType.h"
#include "UserDataTypeVar.h"
#include "ExpressionParser.h"

#include "../../File/TextFile.h"
#include "../../File/StdFileOperations.h"
#include "../../String/StringConverter.h"
#include "../../String/CodeParserHelper.h"

#pragma intrinsic (memcpy,memset,memcmp)

#ifdef _DEBUG
    #define BreakInDebugMode if ( ::IsDebuggerPresent() ) { ::DebugBreak();}
#else 
    #define BreakInDebugMode 
#endif


// CUserDataTypeManager : manages memory of the CUserDataType* linked list
//                        the list containing the content of already parsed user data type files
class CUserDataTypeManager
{
public:
    typedef struct tagUserDataTypeListInfos
    {
        CLinkListSimple* pUserDataTypeList;
        TCHAR ModuleName[MAX_PATH]; // associated module name
    }USER_DATA_TYPE_LIST_INFOS;

    CLinkList* pUserDataTypeLists; // list of USER_DATA_TYPE_LIST_INFOS
                                 // each item contains informations on all items specific to the ModuleName (structs stored in ModuleName subdirectory of UserTypes)
                                 // head is pUserDataTypeDefaultList
    USER_DATA_TYPE_LIST_INFOS* pUserDataTypeDefaultListInfos; // default list : contains struct stored in UserTypes directory
    CUserDataTypeManager()
    {
        CLinkListItem* pItem;
        // create list of lists infos (USER_DATA_TYPE_LIST_INFOS items)
        this->pUserDataTypeLists = new CLinkList(sizeof(USER_DATA_TYPE_LIST_INFOS));

        // create a list default list containing structs located in the root folder
        USER_DATA_TYPE_LIST_INFOS UserDataTypeListInfos;
        UserDataTypeListInfos.ModuleName[0]=0; // no name for the default list
        UserDataTypeListInfos.pUserDataTypeList = new CLinkListSimple();
        
        // add list infos to pUserDataTypeLists
        pItem = this->pUserDataTypeLists->AddItem(&UserDataTypeListInfos);

        // store pointer of memory allocated inside list heap instead of memory allocated in stack
        this->pUserDataTypeDefaultListInfos = (USER_DATA_TYPE_LIST_INFOS*)pItem->ItemData;
    }
    ~CUserDataTypeManager()
    {
        this->FreeMemory();
        delete this->pUserDataTypeLists;
    }
    // free memory associated to items of linked list, and remove items of the linked list
    void CUserDataTypeManager::FreeMemory()
    {
        CLinkListItem* pItem;
        CLinkListItem* pItemType;
        CUserDataType* pUserDataType;
        USER_DATA_TYPE_LIST_INFOS* pListInfos;
        this->pUserDataTypeLists->Lock(FALSE);
        // for each list
        for (pItem = this->pUserDataTypeLists->Head;pItem;pItem = pItem->NextItem)
        {
            pListInfos = (USER_DATA_TYPE_LIST_INFOS*)pItem->ItemData;

            // for each type
            pListInfos->pUserDataTypeList->Lock();
            for (pItemType = pListInfos->pUserDataTypeList->Head;pItemType;pItemType = pItemType->NextItem)
            {
                pUserDataType = (CUserDataType*)pItemType->ItemData;
                delete pUserDataType;
            }
            pListInfos->pUserDataTypeList->RemoveAllItems(TRUE);
            pListInfos->pUserDataTypeList->Unlock();
            delete pListInfos->pUserDataTypeList;
        }
        this->pUserDataTypeDefaultListInfos = NULL;
        this->pUserDataTypeLists->RemoveAllItems(TRUE);
        this->pUserDataTypeLists->Unlock();
    }

    USER_DATA_TYPE_LIST_INFOS* CUserDataTypeManager::FindSpecifiedList(TCHAR* ModuleName)
    {
        CLinkListItem* pItem;
        USER_DATA_TYPE_LIST_INFOS* pListInfos;

        this->pUserDataTypeLists->Lock(FALSE);
        // for each list
        for (pItem = this->pUserDataTypeLists->Head;pItem;pItem = pItem->NextItem)
        {
            pListInfos = (USER_DATA_TYPE_LIST_INFOS*)pItem->ItemData;
            // if module name match module name associated to list
            if ( _tcsicmp(pListInfos->ModuleName,ModuleName)==0)
            {
                this->pUserDataTypeLists->Unlock();
                return pListInfos;
            }
        }
        // not found
        this->pUserDataTypeLists->Unlock();
        return NULL;
    }

    CUserDataType* CUserDataTypeManager::FindType(CLinkListSimple* pUserDataTypeList,TCHAR* TypeName)
    {
        CUserDataType* pUserDataType;
        CLinkListItem* pItem;
        CLinkListItem* pItemName;

        pUserDataTypeList->Lock();
        // check if type belongs to the list
        for (pItem = pUserDataTypeList->Head;pItem;pItem = pItem->NextItem)
        {
            pUserDataType = (CUserDataType*)pItem->ItemData;
            for ( pItemName = pUserDataType->pNames->Head; pItemName ; pItemName=pItemName->NextItem )
            {
                if (_tcscmp( ((CUserDataType::TYPE_NAME_INFOS*)pItemName->ItemData)->Name,TypeName )==0)
                {
                    pUserDataTypeList->Unlock();
                    return pUserDataType;
                }
            }
        }
        // not found
        pUserDataTypeList->Unlock();
        return NULL;
    }

    CUserDataType* CUserDataTypeManager::FindTypeInSpecializedList(TCHAR* ModuleName,TCHAR* TypeName)
    {
        CUserDataType* pUserDataType;
        USER_DATA_TYPE_LIST_INFOS* pUserDataTypeListInfos;

        // if a list with module name exists
        pUserDataTypeListInfos = this->FindSpecifiedList(ModuleName);
        if (pUserDataTypeListInfos)
        {
            // if type is found in this specified module list
            pUserDataType = this->FindType(pUserDataTypeListInfos->pUserDataTypeList,TypeName);
            if (pUserDataType)
                return pUserDataType;
        }

        // not found
        return NULL;
    }

    typedef enum TagFindTypeWay
    {
        FindTypeWay_IN_CACHE_ONLY,
        FindTypeWay_BY_PARSING_ONLY,
        FindTypeWay_IN_CACHE_OR_BY_PARSING
    }FIND_TYPE_WAY;

    // look for type in specialized or default list
    CUserDataType* CUserDataTypeManager::FindType(
                                                TCHAR* const UserDataTypePath,
                                                TCHAR* const ModuleName,
                                                TCHAR* const TypeName,
                                                FIND_TYPE_WAY FindTypeWay,
                                                CLinkListSimple* pParentUserDataTypeList,
                                                CUserDataType::pfReportError const ReportError,
                                                PBYTE const ReportErrorUserParam,
                                                CUserDataType::DEFINITION_FOUND* pDefinitionFound)
    {
        CUserDataType* pUserDataType;
        TCHAR FileName[MAX_PATH];

        *pDefinitionFound = CUserDataType::DEFINITION_FOUND_NOT_FOUND;

        if (*ModuleName)
        {
            ////////////////////////////////
            // 1) look in specialized list
            ////////////////////////////////
            if ( (FindTypeWay == FindTypeWay_IN_CACHE_ONLY) || (FindTypeWay == FindTypeWay_IN_CACHE_OR_BY_PARSING) )
            {
                // type has not been found in specified module list --> use the default list
                pUserDataType = this->FindTypeInSpecializedList(ModuleName,TypeName);
                if (pUserDataType)
                {
                    *pDefinitionFound = CUserDataType::DEFINITION_FOUND_IN_SPECIALIZED_CACHE;
                    return pUserDataType;
                }
            }

            ////////////////////////////////
            // 2) look in specialized directory to parse file if found
            ////////////////////////////////

            if ( (FindTypeWay == FindTypeWay_BY_PARSING_ONLY) || (FindTypeWay == FindTypeWay_IN_CACHE_OR_BY_PARSING) )
            {
                TCHAR* FileTypeName = _tcsdup(TypeName);
                // as file can't exist with ':' in name (that should be the case for type defined in class like Class::Type)
                // with have to replace ':' with '_'
                CUserDataType::ReplaceForbiddenFileChar(FileTypeName);

                // forge UserDataTypePath/ModuleName/TypeName
                _sntprintf(FileName,MAX_PATH,_T("%s%s\\%s.txt"),UserDataTypePath,ModuleName,FileTypeName);
                free(FileTypeName);

                if (CStdFileOperations::DoesFileExists(FileName))
                {
                    pUserDataType = CUserDataType::ParseFile(FileName,
                                                            UserDataTypePath,
                                                            ModuleName,
                                                            TypeName,
                                                            pParentUserDataTypeList,
                                                            ReportError,
                                                            ReportErrorUserParam);
                    if (pUserDataType)
                    {
                        *pDefinitionFound = CUserDataType::DEFINITION_FOUND_BY_FILE_PARSING_IN_SPECIALIZED_DIRECTORY;
                        return pUserDataType;
                    }

                    // in case of parsing error continue to try to find a valid definition in default list or default directory
                }
            }
        }

        ////////////////////////////////
        // 3) look in default list
        ////////////////////////////////
        if ( (FindTypeWay == FindTypeWay_IN_CACHE_ONLY) || (FindTypeWay == FindTypeWay_IN_CACHE_OR_BY_PARSING) )
        {
            if (this->pUserDataTypeDefaultListInfos)
            {
                // type has not been found in specified module list --> use the default list
                pUserDataType = this->FindType(this->pUserDataTypeDefaultListInfos->pUserDataTypeList,TypeName);
                if (pUserDataType)
                {
                    *pDefinitionFound = CUserDataType::DEFINITION_FOUND_IN_DEFAULT_CACHE;
                    return pUserDataType;
                }
            }
#ifdef _DEBUG
            else
            {
                if (::IsDebuggerPresent())
                    ::DebugBreak();
            }
#endif
        }

        ////////////////////////////////
        // 4) look in default directory to parse file if found
        ////////////////////////////////

        if ( (FindTypeWay == FindTypeWay_BY_PARSING_ONLY) || (FindTypeWay == FindTypeWay_IN_CACHE_OR_BY_PARSING) )
        {

            TCHAR* FileTypeName = _tcsdup(TypeName);
            // as file can't exist with ':' in name (that should be the case for type defined in class like Class::Type)
            // with have to replace ':' with '_'
            CUserDataType::ReplaceForbiddenFileChar(FileTypeName);

            // forge UserDataTypePath/TypeName
            _sntprintf(FileName,MAX_PATH,_T("%s%s.txt"),UserDataTypePath,FileTypeName);
            free(FileTypeName);



            if (CStdFileOperations::DoesFileExists(FileName))
            {
                pUserDataType = CUserDataType::ParseFile(FileName,
                                                        UserDataTypePath,
                                                        // use "" insead of ModuleName, as if type is in default dir,
                                                        // these subtypes must be searched in default directory (in case a specialized subtype get the same name with different content)
                                                        _T(""),//ModuleName,
                                                        TypeName,
                                                        pParentUserDataTypeList,
                                                        ReportError,
                                                        ReportErrorUserParam);
                if (pUserDataType)
                {
                    *pDefinitionFound = CUserDataType::DEFINITION_FOUND_BY_FILE_PARSING_IN_DEFAULT_DIRECTORY;
                    return pUserDataType;
                }
            }
        }

        // not found
        return NULL;
    }

    CUserDataType* CUserDataTypeManager::FindTypeInDefaultList(TCHAR* TypeName)
    {
        if (this->pUserDataTypeDefaultListInfos)
            return this->FindType(this->pUserDataTypeDefaultListInfos->pUserDataTypeList,TypeName);

        // else
#ifdef _DEBUG
        if (::IsDebuggerPresent())
            ::DebugBreak();
#endif
        return NULL;
    }

    BOOL CUserDataTypeManager::AddToList(USER_DATA_TYPE_LIST_INFOS* pUserDataTypeListInfos,CUserDataType* pUserDataType)
    {
        if (pUserDataTypeListInfos == 0)
        {
#ifdef _DEBUG
            if (::IsDebuggerPresent())
                ::DebugBreak();
#endif
            return FALSE;
        }

#ifdef _DEBUG
        // display some pUserDataType infos
        CLinkListItem* pItemName;
        CUserDataType::TYPE_NAME_INFOS* pNameInfos;
        TCHAR Msg[3*MAX_PATH];
        TCHAR ListDescription[2*MAX_PATH];
        if (pUserDataTypeListInfos->ModuleName[0]!=0)
        {
            _tcscpy(ListDescription,_T(" added to list : "));
            _tcscat(ListDescription,pUserDataTypeListInfos->ModuleName);
        }
        else
        {
            _tcscpy(ListDescription,_T(" added to default list"));
        }

        for ( pItemName = pUserDataType->pNames->Head; pItemName ; pItemName=pItemName->NextItem )
        {
            pNameInfos = ((CUserDataType::TYPE_NAME_INFOS*)pItemName->ItemData);
            _sntprintf(Msg,
                3*MAX_PATH,
                _T("Added Type : %s nb pointed times : %u, Size : %u %s\r\n"),
                pNameInfos->Name,
                pNameInfos->NbPointedTimes,
                pUserDataType->GetSize(),
                ListDescription
                );
            OutputDebugString(Msg);
        }
#endif

#ifdef _DEBUG
        CLinkListItem* pItemList;
        CLinkListItem* pItemType;
        USER_DATA_TYPE_LIST_INFOS* pUserDataTypeListInfosDebug;

        // search through all lists for the same pointer to avoid memory freeing error
        this->pUserDataTypeLists->Lock();

        // for each list
        for (pItemList = this->pUserDataTypeLists->Head;pItemList;pItemList = pItemList->NextItem)
        {
            pUserDataTypeListInfosDebug = (USER_DATA_TYPE_LIST_INFOS*)pItemList->ItemData;
        
            pUserDataTypeListInfosDebug->pUserDataTypeList->Lock();

            // for each item of the list
            for (pItemType = pUserDataTypeListInfosDebug->pUserDataTypeList->Head;pItemType;pItemType = pItemType->NextItem)
            {
                // if add pointer belongs to list
                if (pUserDataType == (CUserDataType*)pItemType->ItemData)
                {
                    BreakInDebugMode
                }
            }
            pUserDataTypeListInfosDebug->pUserDataTypeList->Unlock();

        }
        this->pUserDataTypeLists->Unlock();
#endif

        return (pUserDataTypeListInfos->pUserDataTypeList->AddItem(pUserDataType)!=0);
    }
    BOOL CUserDataTypeManager::AddToList(TCHAR* ModuleName,CUserDataType* pUserDataType)
    {
        USER_DATA_TYPE_LIST_INFOS* pUserDataTypeListInfos = this->FindSpecifiedList(ModuleName);
        // if list doesn't exist, create it
        if (!pUserDataTypeListInfos)
        {
            USER_DATA_TYPE_LIST_INFOS Infos;
            CLinkListItem* pItem;

            _tcsncpy(Infos.ModuleName,ModuleName,MAX_PATH);
            Infos.ModuleName[MAX_PATH-1]=0;
            Infos.pUserDataTypeList = new CLinkListSimple();

            pItem = this->pUserDataTypeLists->AddItem(&Infos);

            // make pUserDataTypeListInfos point to memory allocated inside heap,
            // instead of memory allocated inside stack
            pUserDataTypeListInfos = (USER_DATA_TYPE_LIST_INFOS*)pItem->ItemData;
        }

        return this->AddToList(pUserDataTypeListInfos,pUserDataType);
    }
    BOOL CUserDataTypeManager::AddToDefaultList(CUserDataType* pUserDataType)
    {
        return this->AddToList(this->pUserDataTypeDefaultListInfos,pUserDataType);
    }
};
CUserDataTypeManager UserDataTypeManager;


void CUserDataType::ReplaceForbiddenFileChar(TCHAR* Str)
{
    if (!Str)
        return;
    TCHAR* pc;
    for ( pc = Str; *pc ; pc++)
    {
        switch (*pc)
        {
        case ':':
            *pc = '.';
            break;
        case '<':
            *pc = '(';
            break;
        case '>':
            *pc = ')';
            break;
        }
    }
}

//-----------------------------------------------------------------------------
// Name: ReportDescriptionNotFoundError
// Object: report the "Description not found" error message
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
void CUserDataType::ReportDescriptionNotFoundError(TCHAR* const TypeName, TCHAR* const ModuleName,pfReportError const ReportError,PBYTE const ReportErrorUserParam)
{
    if (ReportError)
    {
        TCHAR Msg[3*MAX_PATH];
        TCHAR DisplayModuleName[2*MAX_PATH];
        // if search is done in the default directory
        if (*ModuleName==0)
        {
            _sntprintf(DisplayModuleName,2*MAX_PATH,_T("in %s"),APIOVERRIDE_USER_TYPES_PATH);
        }
        else // search is done for a specialized module
        {
            _sntprintf(DisplayModuleName,2*MAX_PATH,_T("in %s nor in %s subdirectory"),APIOVERRIDE_USER_TYPES_PATH,ModuleName);            
        }
        DisplayModuleName[2*MAX_PATH-1]=0; // assume string is ended

        _sntprintf(Msg,3*MAX_PATH,_T("User type description file not found for type %s %s"),TypeName,DisplayModuleName);
        Msg[3*MAX_PATH-1]=0; // assume string is ended
        ReportError(Msg,ReportErrorUserParam);// report error
    }
#ifdef _DEBUG
    if (::IsDebuggerPresent())
        ::DebugBreak();
#endif
}

//-----------------------------------------------------------------------------
// Name: ClearCache
// Object: clear the already parsed user data type cache
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
void CUserDataType::ClearCache()
{
    UserDataTypeManager.FreeMemory();
}
#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif
TCHAR TypeKeyWord[][64] = 
{
    _T("unsigned"),
    _T("FAR"),
    _T("const")
};
BOOL CUserDataType::IsKeyWord(IN TCHAR* const KetWord)
{
    SIZE_T Cnt;
    for (Cnt = 0; Cnt< _countof(TypeKeyWord) ;Cnt++)
    {
        if (_tcsicmp(KetWord,TypeKeyWord[Cnt])==0)
            return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: CUserDataType
// Object: constructor
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
CUserDataType::CUserDataType()
{
    this->BaseType = PARAM_UNKNOWN;
    this->pEnumMap = 0;

    this->pNames = new CLinkList ( sizeof(TYPE_NAME_INFOS) );
    this->pSubType = NULL;
    this->Type = CUserDataType::TYPE_BASE;

    this->Size=0;
}

//-----------------------------------------------------------------------------
// Name: CUserDataType
// Object: destructor
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
CUserDataType::~CUserDataType()
{
    if (this->pSubType)
    {
        // for each subtype var,
        CLinkListItem* pItem;
        CUserDataTypeVar* pUserDataTypeVar;
        this->pSubType->Lock(FALSE);
        for (pItem=this->pSubType->Head;pItem;pItem=pItem->NextItem)
        {
            // destroy the associated CUserDataTypeVar object 
            // (Notice: this doesn't destroy the CUserDataType subtype)
            pUserDataTypeVar = (CUserDataTypeVar*)pItem->ItemData;
            delete pUserDataTypeVar;
        }
        this->pSubType->RemoveAllItems(TRUE);
        this->pSubType->Unlock();
        // delete subtype list
        delete this->pSubType;
        this->pSubType = NULL;
    }

    // delete enum map
    if (this->pEnumMap)
        delete this->pEnumMap;

    // delete names list associated to current type
    if (this->pNames)
        delete this->pNames;
}

//-----------------------------------------------------------------------------
// Name: IsEnum
// Object: check if current type is an enum
// Parameters :
// Return : TRUE if current type is an enum
//-----------------------------------------------------------------------------
BOOL CUserDataType::IsEnum()
{
    return ( this->pEnumMap != 0 );
}

//-----------------------------------------------------------------------------
// Name: AddEnumValue
// Object: check if current type is an enum
// Parameters :
//      TCHAR* Name : enum name
//      PBYTE Value : value associated to enum name
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CUserDataType::AddEnumValue(TCHAR* Name, PBYTE Value)
{
    // create enum map if not already done
    if (!this->pEnumMap)
        this->pEnumMap = new CLinkList(sizeof(ENUM_MAP_CONTENT));

    // copy name and value into ENUM_MAP_CONTENT struct
    ENUM_MAP_CONTENT Content={0};
    Content.Value = Value;
    _tcsncpy(Content.Name,Name,CUserDataType_ENUM_NAME_MAX_SIZE);
    Content.Name[CUserDataType_ENUM_NAME_MAX_SIZE-1]=0;

    // add struct to list
    return (this->pEnumMap->AddItem(&Content) != NULL );
}

//-----------------------------------------------------------------------------
// Name: GetEnumValue
// Object: get value associated to specified name
// Parameters :
//      IN TCHAR* Name : enum name
//      OUT PBYTE Value : value associated to enum name
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CUserDataType::GetEnumValue(TCHAR* Name, OUT PBYTE* pValue)
{
    // default output value
    *pValue = 0;

    if (!this->pEnumMap)
        return FALSE;

    // search value into map
    CLinkListItem* pItem;
    ENUM_MAP_CONTENT* pContent;

    this->pEnumMap->Lock();
    // for each item of *ppLinkListSimple
    for (pItem=this->pEnumMap->Head;pItem;pItem=pItem->NextItem)
    {
        pContent = (ENUM_MAP_CONTENT*)pItem->ItemData;

        // if found : copy string from map
        if (_tcscmp(pContent->Name ,Name)==0)
        {
            *pValue = pContent->Value;
            this->pEnumMap->Unlock();
            return TRUE;
        }
    }
    this->pEnumMap->Unlock();

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: GetEnumValue
// Object: get enum name associated to specified value (if any match)
//         WARNING : String must be at least CUserDataType_ENUM_NAME_MAX_SIZE
// Parameters :
//      IN PBYTE Value : value associated to enum name
//      OUT TCHAR* String : enum name
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CUserDataType::EnumToString(PBYTE Value, OUT TCHAR* String)
{
    // default output value
    *String = 0;

    if (!this->pEnumMap)
        return FALSE;

    // search value into map
    CLinkListItem* pItem;
    ENUM_MAP_CONTENT* pContent;

    this->pEnumMap->Lock();
    // for each item of *ppLinkListSimple
    for (pItem=this->pEnumMap->Head;pItem;pItem=pItem->NextItem)
    {
        pContent = (ENUM_MAP_CONTENT*)pItem->ItemData;
        
        // if found : copy string from map
        if (pContent->Value == Value)
        {
            _tcscpy(String,pContent->Name);
            break;
        }
    }
    this->pEnumMap->Unlock();

    return (*String != 0);
}

//-----------------------------------------------------------------------------
// Name: IsBaseType
// Object: check if type is fully supported by CSupportedParameters 
//         WARNING : an enum is a SIZE_T so it's considered as a basic type
// Parameters :
// Return : TRUE if type is basic (fully supported by CSupportedParameters)
//-----------------------------------------------------------------------------
BOOL CUserDataType::IsBaseType()
{
    return (this->pSubType == NULL);
}

//-----------------------------------------------------------------------------
// Name: GetSize
// Object: get real size not the stack used size
// Parameters :
// Return : real size of the user type
//-----------------------------------------------------------------------------
SIZE_T CUserDataType::GetSize()
{
    // earn time if size has already been computed
    if (this->Size)
        return this->Size;

    // if type is a base one, we directly have it's size
    if (this->IsBaseType())
    {
        // call CSupportedParameters function
        this->Size = CSupportedParameters::GetParamRealSize(this->BaseType);

        // if data size is null
        if (this->Size == 0)
        {
            // this is a pointer
            this->Size = GetRegisterSize();
        }
        return this->Size;
    }

    
    CLinkListItem* pItem;
    CUserDataTypeVar* pUserDataTypeVar;

    // if type is an union
    if (this->Type == CUserDataType::TYPE_UNION)
    {
        // get the maximum of size of subitems
        SIZE_T SubItemSize;
        for (pItem=this->pSubType->Head,this->Size = 0;pItem;pItem=pItem->NextItem)
        {
            pUserDataTypeVar = (CUserDataTypeVar*)(pItem->ItemData);

            // get sub type size
            SubItemSize = pUserDataTypeVar->GetSize();
            // if size is bigger than previous ones
            if (SubItemSize > this->Size)
                // store current size as the union size
                this->Size = SubItemSize;
        }
    }
    else // type is a struct
    {
        // get the sum of size of subitems
        for (pItem=this->pSubType->Head,this->Size = 0;pItem;pItem=pItem->NextItem)
        {
            pUserDataTypeVar = (CUserDataTypeVar*)(pItem->ItemData);
            // increase size by subtype size
            this->Size+=pUserDataTypeVar->GetSize();
        }
    }

    // return type size
    return this->Size;
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: check if type is fully supported by CSupportedParameters 
//         WARNING : an enum is a SIZE_T so it's considered as a basic type
// Parameters :
//          TCHAR* UserDataTypePath : directory inside which we should look
//          TCHAR* ModuleName : the module name (= potential subdirectory name inside which we have to look)
//          TCHAR* TypeName : the name of Type we are looking for
//          IN OUT SIZE_T* pNbPointedTimes : 
//          pfReportError const ReportError : user report error function
//          PBYTE const ReportErrorUserParam : user report error function user param
//          OUT DEFINITION_FOUND* pDefinitionFound : found/not found informations
// Return : the found CUserDataType object associated to type name if pDefinitionFound != DEFINITION_FOUND_NOT_FOUND,
//          a default CUserDataType object of type PARAM_UNKNOWN if pDefinitionFound == DEFINITION_FOUND_NOT_FOUND,
//-----------------------------------------------------------------------------
#define CUserDataType_NumberOfAlternativePointerNames 4
#define CUserDataType_AlternativePointerNamesMaxSize 12
TCHAR CUserDataType::AlternativePointerPrefix[CUserDataType_NumberOfAlternativePointerNames][CUserDataType_AlternativePointerNamesMaxSize] =
{
    _T("PC"),
    _T("P"),
    _T("LPC"),
    _T("LP")
};
CUserDataType* CUserDataType::Parse(TCHAR* const UserDataTypePath, TCHAR* const ModuleName, TCHAR* const TypeName,IN OUT SIZE_T* pNbPointedTimes,pfReportError const ReportError,PBYTE const ReportErrorUserParam,DEFINITION_FOUND* pDefinitionFound)
{
    CUserDataType* pUserDataType;
    CLinkListSimple* pParentUserDataTypeList;
    pParentUserDataTypeList = new CLinkListSimple();
    pUserDataType = CUserDataType::Parse(UserDataTypePath, ModuleName, TypeName, pNbPointedTimes, pParentUserDataTypeList, ReportError, ReportErrorUserParam, pDefinitionFound);
    delete pParentUserDataTypeList;
    return pUserDataType;
}

CUserDataType* CUserDataType::Parse(TCHAR* const UserDataTypePath, TCHAR* const ModuleName, TCHAR* const TypeName,IN OUT SIZE_T* pNbPointedTimes,CLinkListSimple* pParentUserDataTypeList,pfReportError const ReportError,PBYTE const ReportErrorUserParam,DEFINITION_FOUND* pDefinitionFound)
{
    CUserDataType* pUserDataType;
    TYPE_NAME_INFOS TypeNameInfos;
    SUPPORTED_PARAMETERS_STRUCT* pInfos;
    SIZE_T Cnt;
    SIZE_T AlternativePointerNamePrefixSize;
    SIZE_T NbPointedTimesFromBasicType = 0;

    /////////////////////////////////////////////
    // a) search in cache list
    /////////////////////////////////////////////
    // search in cache for TYPE
    pUserDataType = UserDataTypeManager.FindType(UserDataTypePath,ModuleName,TypeName,CUserDataTypeManager::FindTypeWay_IN_CACHE_ONLY,pParentUserDataTypeList,ReportError,ReportErrorUserParam,pDefinitionFound);
    if (pUserDataType)
        return pUserDataType;

    // if not found, look for TYPE in case of LTYPE, PCTYPE, LPTYPE, LPCTYPE (avoid to define all P, PC, LP, LPC for basic types)
    for (Cnt = 0; Cnt< CUserDataType_NumberOfAlternativePointerNames; Cnt++)
    {
        AlternativePointerNamePrefixSize = _tcslen(CUserDataType::AlternativePointerPrefix[Cnt]);
        if (_tcsncmp(TypeName,CUserDataType::AlternativePointerPrefix[Cnt],AlternativePointerNamePrefixSize)==0)
        {
            pUserDataType = UserDataTypeManager.FindType(UserDataTypePath,ModuleName,&TypeName[AlternativePointerNamePrefixSize],CUserDataTypeManager::FindTypeWay_IN_CACHE_ONLY, pParentUserDataTypeList,ReportError,ReportErrorUserParam,pDefinitionFound);

            // if type has been found
            if (pUserDataType)
            {
                // increment NbPointedTimes by one due to P,PC,LP or LPC prefix
                (*pNbPointedTimes)+= 1;
                // return found type
                return pUserDataType;
            }
        }
    }

    /////////////////////////////////////////////
    // b) check if type belongs to fully supported types 
    // Notice : doing this before file existence checking avoid disk access for basic type like DWORD
    /////////////////////////////////////////////
    pInfos = CSupportedParameters::GetParamInfos(TypeName);

    // if not found, look for TYPE in case of PTYPE, PCTYPE, LPTYPE, LPCTYPE (avoid to define all P, LP, LPC for basic types)
    if (pInfos->ParamType == PARAM_UNKNOWN)
    {
        // if P, PC, LP, LPC, 
        //      check if type belongs to the list
        for (Cnt = 0; Cnt< CUserDataType_NumberOfAlternativePointerNames; Cnt++)
        {
            AlternativePointerNamePrefixSize = _tcslen(CUserDataType::AlternativePointerPrefix[Cnt]);
            if (_tcsncmp(TypeName,CUserDataType::AlternativePointerPrefix[Cnt],AlternativePointerNamePrefixSize)==0)
            {
                pInfos = CSupportedParameters::GetParamInfos(&TypeName[AlternativePointerNamePrefixSize]);
                // if type has been found
                if (pInfos->ParamType != PARAM_UNKNOWN)
                {
                    NbPointedTimesFromBasicType++;
                    break;
                }
            }
        }
    }

    // if type is fully supported
    if (pInfos->ParamType != PARAM_UNKNOWN)
    {
        // increase pointed time is necessary
        (*pNbPointedTimes)+= NbPointedTimesFromBasicType;

        // if data size is null
        if (pInfos->DataSize == 0)
            // this is a pointed type --> increase NbPointedTimes
            (*pNbPointedTimes)++;

        // create a type matching fully supported one
        pUserDataType = new CUserDataType();

        // copy fully supported type infos
        pUserDataType->BaseType = pInfos->ParamType;
        memset(&TypeNameInfos,0,sizeof(TYPE_NAME_INFOS));
        _tcsncpy(TypeNameInfos.Name, pInfos->ParamName,MAX_PATH-1);
        TypeNameInfos.NbPointedTimes = NbPointedTimesFromBasicType;
        pUserDataType->pNames->AddItem(&TypeNameInfos);

        // add to list
        UserDataTypeManager.AddToDefaultList(pUserDataType);

        *pDefinitionFound = DEFINITION_FOUND_IN_FULLY_SUPPORTED_LIST;
        return pUserDataType;
    }


    /////////////////////////////////////////////
    // c) search in struct definitions directories
    /////////////////////////////////////////////

    // search in UserTypes directory for TYPE
    pUserDataType = UserDataTypeManager.FindType(UserDataTypePath,ModuleName,TypeName,CUserDataTypeManager::FindTypeWay_BY_PARSING_ONLY,pParentUserDataTypeList,ReportError,ReportErrorUserParam,pDefinitionFound);
    if (pUserDataType)
        return pUserDataType;

    // if not found, look for TYPE in case of LTYPE, PCTYPE, LPTYPE, LPCTYPE (avoid to define all P, PC, LP, LPC for basic types)
    for (Cnt = 0; Cnt< CUserDataType_NumberOfAlternativePointerNames; Cnt++)
    {
        AlternativePointerNamePrefixSize = _tcslen(CUserDataType::AlternativePointerPrefix[Cnt]);
        if (_tcsncmp(TypeName,CUserDataType::AlternativePointerPrefix[Cnt],AlternativePointerNamePrefixSize)==0)
        {
            pUserDataType = UserDataTypeManager.FindType(UserDataTypePath,ModuleName,&TypeName[AlternativePointerNamePrefixSize],CUserDataTypeManager::FindTypeWay_BY_PARSING_ONLY,pParentUserDataTypeList,ReportError,ReportErrorUserParam,pDefinitionFound);

            // if type has been found
            if (pUserDataType)
            {
                // increment NbPointedTimes by one due to P,PC,LP or LPC prefix
                (*pNbPointedTimes)+= 1;
                // return found type
                return pUserDataType;
            }
        }
    }


    /////////////////////////////////////////////
    // d) If no definition has been found
    /////////////////////////////////////////////
#ifdef _DEBUG
    {
        TCHAR Msg[3*MAX_PATH];
        _sntprintf(Msg,
            3*MAX_PATH,
            _T("Unknown type with no definition file %s \r\n"),
            TypeName
            );
        OutputDebugString(Msg);
    }
#endif

    // nothing found : return PARAM_UNKNOWN
    *pDefinitionFound = DEFINITION_FOUND_NOT_FOUND;
    pUserDataType = CUserDataType::FindOrAddUnknownTypeInDefaultList();

    return pUserDataType;
}

CUserDataType* CUserDataType::FindOrAddUnknownTypeInDefaultList()
{
    CUserDataType* pUserDataType;
    TYPE_NAME_INFOS TypeNameInfos;
    SUPPORTED_PARAMETERS_STRUCT* pInfos;

    // find PARAM_UNKNOWN in list
    pUserDataType = UserDataTypeManager.FindTypeInDefaultList(_T("UNKNOWN"));
    // if param unknown is in list
    if (pUserDataType)
        return pUserDataType;

    // PARAM_UNKNOWN not in list, add it to list with _T("UNKNOWN") name
    pInfos = CSupportedParameters::GetParamInfos(PARAM_UNKNOWN);
    pUserDataType = new CUserDataType();
    pUserDataType->BaseType = pInfos->ParamType;
    memset(&TypeNameInfos,0,sizeof(TYPE_NAME_INFOS));
    _tcsncpy(TypeNameInfos.Name, pInfos->ParamName,MAX_PATH-1);
    TypeNameInfos.NbPointedTimes = 0;
    pUserDataType->pNames->AddItem(&TypeNameInfos);

    // add to list
    UserDataTypeManager.AddToDefaultList(pUserDataType);

    return pUserDataType;
}

//-----------------------------------------------------------------------------
// Name: ParseFile
// Object: parse a file to extract type informations
// Parameters :

// Return : matching CUserDataType* object or NULL on error or not found
//-----------------------------------------------------------------------------
CUserDataType* CUserDataType::ParseFile(TCHAR* const FileName,
                                        TCHAR* const UserDataTypePath,
                                        TCHAR* const ModuleName,
                                        TCHAR* const SearchBaseType,
                                        CLinkListSimple* pParentUserDataTypeList,
                                        pfReportError const ReportError,
                                        PBYTE const ReportErrorUserParam)
{
    // a file supposed to contain definition has been found
    TCHAR* FileContent;
    CUserDataType* pUserDataType;

    // put content of file in memory
    CTextFile::Read(FileName,&FileContent);

    // parse file content
    pUserDataType = CUserDataType::ParseContent(FileName,UserDataTypePath, ModuleName, FileContent,SearchBaseType,pParentUserDataTypeList,ReportError,ReportErrorUserParam);

    // free content of file from memory
    delete FileContent;

    // return found type
    return pUserDataType;
}

//-----------------------------------------------------------------------------
// Name: StaticEnumGetValue
// Object: get value associated to enum string
// Parameters :
//          IN TCHAR* const Expression : enum string for which we query value
//          OUT ULONG_PTR* pValue : value associated if string has been found
//          PBYTE StringToValueFunctionUserParam : CUserDataType* object
// Return : TRUE on SUCCESS
//-----------------------------------------------------------------------------
BOOL CUserDataType::StaticEnumGetValue(IN TCHAR* const Expression,OUT ULONG_PTR* pValue,PBYTE StringToValueFunctionUserParam)
{
    CUserDataType* pUserDataType = (CUserDataType*)StringToValueFunctionUserParam;
    return pUserDataType->GetEnumValue(Expression,(PBYTE*)pValue);
}

//-----------------------------------------------------------------------------
// Name: ParseEnumUnionStructContent
// Object: parse content of an enum, struct or union
// Parameters :
//          CLinkListSimple* pParentUserDataTypeList : list of parent type if any (can be an empty list)
//          TCHAR* UserDataTypePath : directory inside which we should look
//          TCHAR* ModuleName : the module name (= potential subdirectory name inside which we have to look)
//          TCHAR* const Content : content of enum / struct / union
//          EXPRESSIONS_TYPE Type : Type (enum / struct / union)
//          pfReportError const ReportError : user report error function
//          PBYTE const ReportErrorUserParam : user report error function user param
// Return : parent user data type (CUserDataType*) (pParentUserDataTypeList->Tail->ItemData)
//-----------------------------------------------------------------------------
CUserDataType* CUserDataType::ParseEnumUnionStructContent(TCHAR* CurrentParsedFileName,TCHAR* UserDataTypePath, TCHAR* ModuleName, TCHAR* const Content,EXPRESSIONS_TYPE Type,CUserDataType* pCurrentUserType,CLinkListSimple* pParentUserDataTypeList, pfReportError const ReportError,PBYTE const ReportErrorUserParam)
{
    if (Type == TYPE_ENUM)
    {
        if (!pCurrentUserType)
            return NULL;

        /////////////////////////
        // parse enum
        /////////////////////////

        TCHAR Name[CUserDataType_ENUM_NAME_MAX_SIZE];
        TCHAR* NameBegin ;
        TCHAR* NameEnd;
        TCHAR* ValueBegin;
        TCHAR* SingleEnumEnd;
        PBYTE Value;
        SIZE_T Size;

        PBYTE LastEnumValue=0;
        BOOL LastEnumDefinition;

        // for each enum field
        for (LastEnumValue=0, LastEnumDefinition = FALSE ,NameBegin = Content;
            !LastEnumDefinition ; 
            NameBegin = SingleEnumEnd +1)
        {
            // search for enum value splitter ','
            SingleEnumEnd = _tcschr(NameBegin,',');
            if (SingleEnumEnd)
            {
                // point before ','
                SingleEnumEnd--;
                // find previous alpha or num char
                if (!CCodeParserHelper::IsAlphaNumOrUnderscore(*SingleEnumEnd))
                    SingleEnumEnd = CCodeParserHelper::FindPreviousNotSplitterChar(NameBegin,SingleEnumEnd);
            }
            else // for last enum value
            {
                SingleEnumEnd = &NameBegin[_tcslen(NameBegin)-1];
                LastEnumDefinition = TRUE;
            }
            // end value
            *(SingleEnumEnd+1) = 0;

            // make SingleEnumEnd point on ',' (just replaced by \0)
            // so NameBegin = SingleEnumEnd +1 will work for the next loop
            SingleEnumEnd++;

            // look for a specified value
            NameEnd = _tcschr(NameBegin,'=');

            // if not '='
            if (NameEnd==0)
            {
                // check if there is data (a single enum name)
                NameBegin = CCodeParserHelper::FindNextNotSplitterChar(NameBegin);
                if (*NameBegin == 0)
                {
                    if ( LastEnumDefinition )
                        break;
                    else
                        continue;
                }

                // find last not null splitter
                NameEnd = &NameBegin[_tcslen(NameBegin)-1];
                if ( ! CCodeParserHelper::IsAlphaNumOrUnderscore(*NameEnd) )
                    NameEnd = CCodeParserHelper::FindPreviousNotSplitterChar(NameBegin,NameEnd);

                // copy name
                Size = __min(NameEnd-NameBegin+1,CUserDataType_ENUM_NAME_MAX_SIZE-1);
                _tcsncpy(Name,NameBegin,Size);
                Name[Size]=0;
                
                // use last enum value incremented by one
                LastEnumValue++;
                Value = LastEnumValue;

                // add enum {name,value} to pParentUserDataType
                // pParentUserDataType->AddEnumValue(Name,Value);
                pCurrentUserType->AddEnumValue(Name,Value);
                
                continue;
            }

            // point after '='
            ValueBegin = NameEnd+1;
            NameEnd--;// point before '='
            // remove spacers after name
            if ( ! CCodeParserHelper::IsAlphaNumOrUnderscore(*NameEnd) )
                NameEnd = CCodeParserHelper::FindPreviousNotSplitterChar(NameBegin,NameEnd);

            // remove spacers before name
            NameBegin = CCodeParserHelper::FindNextNotSplitterChar(NameBegin);

            // copy enum name into Name
            Size = __min(NameEnd-NameBegin+1,CUserDataType_ENUM_NAME_MAX_SIZE-1);
            _tcsncpy(Name,NameBegin,Size);
            Name[Size]=0;

            // remove spacers before enum value
            ValueBegin = CCodeParserHelper::FindNextNotSplitterChar(ValueBegin);

            // if subexpression 
            if (_tcspbrk(ValueBegin,_T("()+-*/!~^&|")))
            {
                // parse expression
                CExpressionParser::ParseExpression(ValueBegin,(ULONG_PTR*)&Value,StaticEnumGetValue,(PBYTE)pCurrentUserType,ReportError,0);
            }
            else
            {
                // try to convert string value to value
                if ( !CStringConverter::StringToPBYTE(ValueBegin,&Value) )
                {
                    // check EnumName = PreviousEnumName
                    if ( !(pCurrentUserType->GetEnumValue(ValueBegin,&Value)) )
                    {
                        if (ReportError)
                        {
                            TCHAR Msg[MAX_PATH];
                            _sntprintf(Msg,MAX_PATH,_T("Can't find definition for %s"),ValueBegin);
                            Msg[MAX_PATH-1]=0;
                            ReportError(Msg,ReportErrorUserParam);
                        }
                        continue;
                    }
                }
            }
            // add enum {name,value} to pParentUserDataType
            pCurrentUserType->AddEnumValue(Name,Value);

            LastEnumValue = Value;
        }
        return pCurrentUserType;
    }
    else // if type is a struct or an union
    {
        pParentUserDataTypeList->AddItem(pCurrentUserType);
        CUserDataType::ParseContent(CurrentParsedFileName,UserDataTypePath, ModuleName, Content, NULL,pParentUserDataTypeList,ReportError,ReportErrorUserParam);
        pParentUserDataTypeList->RemoveItem(pParentUserDataTypeList->Tail);
        return pCurrentUserType;
    }
}

//-----------------------------------------------------------------------------
// Name: ParseContent
// Object: parse content of file, struct or union
// Parameters :
//          CUserDataType* pParentUserDataType : parent type if any (NULL else)
//          TCHAR* UserDataTypePath : directory inside which we should look
//          TCHAR* ModuleName : the module name (= potential subdirectory name inside which we have to look)
//          TCHAR* const Content : content of file, struct or union
//          TCHAR* SearchedType : searched type name WARNING : case sensitive search is done
//          pfReportError const ReportError : user report error function
//          PBYTE const ReportErrorUserParam : user report error function user param
// Return : pParentUserDataType (function fills pParentUserDataType content)
//-----------------------------------------------------------------------------
CUserDataType* CUserDataType::ParseContent(TCHAR* CurrentParsedFileName,TCHAR* UserDataTypePath, TCHAR* ModuleName, TCHAR* const Content, TCHAR* SearchedType,CLinkListSimple* pParentUserDataTypeList,pfReportError const ReportError,PBYTE const ReportErrorUserParam)
{
    SIZE_T Depth;
    
    TYPE_NAME_INFOS TypeNameInfos;
    TCHAR* pExpressionBegin;
    TCHAR* pCurrentPosition;
    TCHAR* pSubExpressionEnd;
    TCHAR* pCommentBegin;
    TCHAR* pCommentEnd;
    CUserDataType* ReturnedpUserDataType = NULL;
    EXPRESSION ExpressionItem;
    CUserDataType* pParentUserDataType = NULL;

    if (pParentUserDataTypeList->Tail)
    {
        pParentUserDataType = (CUserDataType*)pParentUserDataTypeList->Tail->ItemData;
    }

    pExpressionBegin = Content;

    CLinkList Expressions(sizeof(EXPRESSION));
    Depth = 0;

    // in a first step we are going to remove comments, breaking line, 
    // and split expressions into a list
    pCurrentPosition = pExpressionBegin;
    for (;;)
    {
        // look for slitters : subexpression, comments, and ';' for end of instruction
        pCurrentPosition = _tcspbrk(pCurrentPosition,_T(";{}/\"\\"));
        if (pCurrentPosition == NULL)
        {
            break;
        }
        // look for line break
        if (*pCurrentPosition == '\\')
        {
            TCHAR* pTmp = pCurrentPosition;
            pTmp++;
            if ( *(pTmp) == '\r' )
                pTmp++;
            if ( *(pTmp) == '\n' )
            {
                pTmp++;
                // remove line break from content
                memmove(pCurrentPosition,pTmp, (_tcslen(pTmp)+1) * sizeof(TCHAR) );
            }
        }
        // look for end of instruction
        else if (*pCurrentPosition == ';')
        {
            // if instruction belongs to a subexpression
            if (Depth>0)
            {
                // continue : sub expression will be parsed in a second pass (recursive call)
                pCurrentPosition++;
                continue;
            }

            *pCurrentPosition = 0;
            pCurrentPosition++;
            // remove spacers
            pCurrentPosition=CCodeParserHelper::FindNextNotSplitterChar(pCurrentPosition);

            // add expression to list
            memset(&ExpressionItem, 0, sizeof (sizeof(CUserDataType::EXPRESSION)));
            ExpressionItem.pszBegin = pExpressionBegin;
            Expressions.AddItem(&ExpressionItem);

            // update next sub expression begin
            pExpressionBegin = pCurrentPosition;

            continue;
        }
        // check for comments sing or multiple lines
        else if (*pCurrentPosition == '/')
        {
            pCommentBegin = pCurrentPosition;
            if ( *(pCurrentPosition+1) == '/' ) // single line comment
            {
                pSubExpressionEnd = pCurrentPosition + 2;
SingleLineComment:
                // find end of line or breaking line char
                pSubExpressionEnd = _tcspbrk(pSubExpressionEnd,_T("\n\\"));
                if (pSubExpressionEnd == NULL)
                {
                    // unterminated comment
                    // remove comment from string
                    *pCommentBegin = 0;
                    break;
                }
                // check for breaking line
                else if (*pSubExpressionEnd == '\\')
                {
                    // next char(s) must be '\n' or "\r\n"
                    if ( *(pSubExpressionEnd+1) == '\n' )
                    {
                        pSubExpressionEnd = pSubExpressionEnd +2 ;
                        goto SingleLineComment;
                    }
                    else if ( *(pSubExpressionEnd+1) == '\r' )
                    {
                        if ( *(pSubExpressionEnd+2) == '\n' )
                        {
                            pSubExpressionEnd = pSubExpressionEnd +3 ;
                            goto SingleLineComment;
                        }
                    }
                }
                // end of single line comment
                else if (*pSubExpressionEnd == '\n')
                {
                    // update pCurrentPosition to find end of current type
                    pCurrentPosition = pSubExpressionEnd + 1;

                    // remove comment from string
                    pCommentEnd = pCurrentPosition;
                    memmove(pCommentBegin,pCommentEnd, (_tcslen(pCommentEnd)+1) * sizeof(TCHAR) );

                    // as comment as been remove, next item to be parsed is at pCommentBegin
                    pCurrentPosition = pCommentBegin;
                    continue;
                }
            }
            else if ( *(pCurrentPosition+1) == '*' ) // multiple lines comment
            {
                // look for "*/"
                pSubExpressionEnd = _tcsstr (pCurrentPosition+2 , _T("*/") );
                if (pSubExpressionEnd ==NULL)
                {
                    // unterminated comment
                    // remove comment from string
                    *pCommentBegin = 0;
                    break;
                }

                // update pCurrentPosition to find end of current type
                pCurrentPosition = pSubExpressionEnd + 2;

                // remove comment from string
                pCommentEnd = pCurrentPosition;
                memmove(pCommentBegin,pCommentEnd, (_tcslen(pCommentEnd)+1) * sizeof(TCHAR) );

                // as comment as been remove, next item to be parsed is at pCommentBegin
                pCurrentPosition = pCommentBegin;
                continue;
            }
            else
            {
                // '/' is not used for comment (division)
                pCurrentPosition++;
            }
        }
        // check for string
        else if (*pCurrentPosition == '"')
        {
            // look for "
            pSubExpressionEnd = pCurrentPosition;
FindEndOfString:
            // find end of string
            pSubExpressionEnd = _tcschr (pSubExpressionEnd+1 , '"' );
            if (pSubExpressionEnd ==NULL)
            {
                // unterminated string
                break;
            }
            // if escape code is before "
            else if ( *(pSubExpressionEnd-1) == '\\' )
            {
                // find next "
                goto FindEndOfString;
            }

            // update pCurrentPosition to find end of current type
            pCurrentPosition = pSubExpressionEnd + 2;
            continue;
        }
        // if subexpression begin
        else if (*pCurrentPosition == '{')
        {
            pCurrentPosition++;
            Depth ++;
        }
        // if subexpression end
        else if (*pCurrentPosition == '}')
        {
            pCurrentPosition++;
            Depth --;
        }
    }

    // if not finished subexpression
    if (Depth !=0)
    {
        // Parsing Error / malformed expression
        if (ReportError)
        {
            TCHAR Msg[MAX_PATH];
            _sntprintf(Msg,MAX_PATH,_T("Parsing error inside %s definition : end of file found before a left brace '{' was matched"),SearchedType);
            Msg[MAX_PATH-1]=0;
            ReportError(Msg,ReportErrorUserParam);
        }

        return NULL;
    }

    //////////////////////////////
    // parse type and sub types
    //////////////////////////////


    // typedef union _my_union{ TYPE_1 a; TYPE_2 b; }my_union;
    // typedef ULONG DWORD;
    // typedef struct _my_struct { TYPE_1 a; TYPE_2 b; } my_struct,*pmy_struct;
    // DWORD Bit1:1 , Bit2:1, Bit3_4:2, unused:28;
    // typedef void (*pfn_callback_IT) (unsigned int unHandle,...);
    // typedef enum _my_enum { VALUE1,VALUE2=8,VALUE3} my_enum;
    // typedef enum _my_enum { VALUE1,VALUE2=8,VALUE3,VALUE4 = VALUE1} my_enum;
    // typedef enum _my_enum { VALUE1,VALUE2=3,VALUE3,VALUE4 = VALUE1,VALUE5,VALUE6,VALUE7} my_enum; // here VALUE7 == VALUE2 (at least on VS2003) --> previous value ++
    // typedef union _my_uninon { struct { unsigned int    Bit1:1, Unused:31; } UnamedStruct; unsigned int Value; }my_uninon,*pmy_uninon;
    // struct [_my_struct] { TYPE_1 a; TYPE_2 b; } VarName;
    // re entering like
    //          typedef struct _SINGLE_LIST_ENTRY {
    //                      struct _SINGLE_LIST_ENTRY *Next;
    //          } SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;


    EXPRESSION* pExpressionItem;
    BOOL bTypeDef = FALSE;
    TCHAR* pTypeBegin;
    TCHAR* pTypeName;
    TCHAR* pTypeNameEnd;
    SIZE_T NbPointedTimes;
    SIZE_T Size;
    CLinkListItem* pItem;
    DEFINITION_FOUND DefinitionFound;

    // default type
    CUserDataType::EXPRESSIONS_TYPE Type = CUserDataType::TYPE_RENAMING;

    // for each found expression
    for (pItem = Expressions.Head;pItem;pItem = pItem->NextItem)
    {
        DefinitionFound = DEFINITION_FOUND_NOT_FOUND;

        pExpressionItem = (EXPRESSION*)pItem->ItemData;

        pTypeBegin = pExpressionItem->pszBegin;

        // point on first not spacing character of expression
        pTypeBegin = CCodeParserHelper::FindNextNotSplitterChar(pTypeBegin);

        // default type
        Type = CUserDataType::TYPE_RENAMING;
        bTypeDef = FALSE;

        // if "typedef" 
        if (_tcsncmp(_T("typedef"),pTypeBegin,_tcslen(_T("typedef"))) == 0 )
        {
            if (!CCodeParserHelper::IsAlphaNumOrUnderscore(*(pTypeBegin+ _tcslen(_T("typedef"))) ))
            {
                bTypeDef = TRUE;
                pTypeBegin+= _tcslen(_T("typedef"));
            }
        }

        // point on first not space character
        pTypeBegin = CCodeParserHelper::FindNextNotSplitterChar(pTypeBegin);

        if (_tcsncmp(_T("enum"),pTypeBegin,_tcslen(_T("enum"))) == 0 )
        {
            if (!CCodeParserHelper::IsAlphaNumOrUnderscore(*(pTypeBegin+ _tcslen(_T("enum"))) ))
            {
                Type = CUserDataType::TYPE_ENUM;
                pTypeBegin+= _tcslen(_T("enum"));
            }
        }
        else if (_tcsncmp(_T("struct"),pTypeBegin,_tcslen(_T("struct"))) == 0 )
        {
            if (!CCodeParserHelper::IsAlphaNumOrUnderscore(*(pTypeBegin+ _tcslen(_T("struct"))) ))
            {
                Type = CUserDataType::TYPE_STRUCT;
                pTypeBegin+= _tcslen(_T("struct"));
            }
        }
        else if (_tcsncmp(_T("union"),pTypeBegin,_tcslen(_T("union"))) == 0 )
        {
            if (!CCodeParserHelper::IsAlphaNumOrUnderscore(*(pTypeBegin+ _tcslen(_T("union"))) ))
            {
                Type = CUserDataType::TYPE_UNION;
                pTypeBegin+= _tcslen(_T("union"));
            }
        }

        // point on first not space character
        pTypeBegin = CCodeParserHelper::FindNextNotSplitterChar(pTypeBegin);

        // if type is a complex one (union, struct or enum)
        if (    (Type == CUserDataType::TYPE_UNION)
             || (Type == CUserDataType::TYPE_STRUCT)
             || (Type == CUserDataType::TYPE_ENUM)
            )
        {
            TCHAR* pSubBlockBegin;
            TCHAR* pSubBlockEnd;
            TCHAR* pSubBlockContent;
            SIZE_T SubBlockSize;
            BOOL bHasContent = TRUE;
            BOOL bCurrentStructUnionPointer = FALSE;
            BOOL bNeedToBeAddToList;

            // look for content
            pSubBlockBegin = _tcschr(pTypeBegin,'{');
            pSubBlockEnd = _tcsrchr(pTypeBegin,'}');
            if (    (pSubBlockBegin == NULL)
                 && (pSubBlockEnd == NULL)
                 && (!bTypeDef)
                )
            {
                bHasContent = FALSE;
                // re entering like
                //          typedef struct _SINGLE_LIST_ENTRY {
                //                      struct _SINGLE_LIST_ENTRY *Next;
                //          } SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;
                // or other struct
                pSubBlockBegin = CCodeParserHelper::FindNextNotAlphaNumOrUnderscore(pTypeBegin);
                pSubBlockEnd = pSubBlockBegin;
            }
            else if ( (pSubBlockBegin == NULL)
                    || (pSubBlockEnd == NULL)
                    )
            {
                // Parsing Error / malformed expression
                if (ReportError)
                {
                    TCHAR Msg[MAX_PATH];
                    _sntprintf(Msg,MAX_PATH,_T("Parsing error inside %s definition :  no '{' or '}' found for enum/struct/enum definition"),SearchedType);
                    Msg[MAX_PATH-1]=0;
                    ReportError(Msg,ReportErrorUserParam);
                }
                continue;
            }

            // create an object associated to 
            CUserDataType* pUserDataType;

            // create a new type object
            pUserDataType = new CUserDataType();
            pUserDataType->Type = Type;

            bNeedToBeAddToList = TRUE; // pUserDataType need to be added to cache lists

            if (bHasContent)
            {
                ///////////////////////////////////////
                // find name before struct / union definition
                // Notice : must be done before ParseEnumUnionStructContent in case of re entering struct / union
                ///////////////////////////////////////
                pTypeNameEnd = pSubBlockBegin;
                if (pTypeNameEnd 
                    && ( pTypeNameEnd != pTypeBegin) // unnamed struct
                    )
                {
                    // find first char before '{'
                    pTypeNameEnd = CCodeParserHelper::FindPreviousNotSplitterChar(pTypeBegin,pTypeNameEnd);

                    // add name to pUserDataType
                    memset(&TypeNameInfos,0,sizeof(TYPE_NAME_INFOS));
                    Size = __min( pTypeNameEnd-pTypeBegin+1, MAX_PATH-1);
                    _tcsncpy(TypeNameInfos.Name, pTypeBegin, Size );
                    TypeNameInfos.Name[Size]=0;

                    TypeNameInfos.NbPointedTimes = 0;
                    pUserDataType->pNames->AddItem(&TypeNameInfos);

                    // if we are looking for a type
                    if (SearchedType)
                    {
                        // if name match the searched one
                        if (_tcscmp(SearchedType, TypeNameInfos.Name)== 0)
                        {
                            // set the returned value
                            ReturnedpUserDataType = pUserDataType;
                        }
                    }
                }

                // create a copy of sub block for parsing
                SubBlockSize = pSubBlockEnd-pSubBlockBegin;
                pSubBlockContent = (TCHAR*)malloc(SubBlockSize*sizeof(TCHAR)); 
                _tcsncpy(pSubBlockContent ,pSubBlockBegin+1,SubBlockSize);
                pSubBlockContent[SubBlockSize-1]=0;

                // parse content
                CUserDataType::ParseEnumUnionStructContent(CurrentParsedFileName,UserDataTypePath, ModuleName, pSubBlockContent , Type, pUserDataType, pParentUserDataTypeList, ReportError,ReportErrorUserParam);
                free(pSubBlockContent);
            }
            else // no content --> type must be known (struct A a;)
            {
                TCHAR FieldTypeName[MAX_PATH];
                Size = __min( pSubBlockBegin-1-pTypeBegin+1, MAX_PATH-1);//in this case pSubBlockBegin points to char after type so last type letter is at pos pSubBlockBegin-1
                _tcsncpy(FieldTypeName, pTypeBegin, Size );
                FieldTypeName[Size] = 0;

                CLinkListItem* pItemParentUserDataType;
                CUserDataType* pParentUserDataType2=NULL;
                for (pItemParentUserDataType = pParentUserDataTypeList->Tail;pItemParentUserDataType;pItemParentUserDataType = pItemParentUserDataType->PreviousItem)
                {
                    pParentUserDataType2 = (CUserDataType*)pItemParentUserDataType->ItemData;
                    // if struct is the same that is being parsed
                    if (pParentUserDataType2) // if we have a parent (we are inside a struct or union definition)
                    {
                        if (pParentUserDataType2->pNames) // if parent name list exist
                        {
                            // search through parents struct/union names
                            // to check if it is a pointer on the struct/union being defined
                            CLinkListItem* pItemName;
                            TYPE_NAME_INFOS* pNameInfos;
                            BOOL bCurrentStruct = FALSE;
                            BOOL bPointerToUsedTypeName = FALSE;
                            for (pItemName = pParentUserDataType2->pNames->Head;pItemName;pItemName = pItemName->NextItem)
                            {
                                pNameInfos = (TYPE_NAME_INFOS*)pItemName->ItemData;
                                if (_tcscmp(FieldTypeName,pNameInfos->Name)==0)
                                {
                                    // the type is the struct / union being defined
                                    bCurrentStruct = TRUE;
                                    bPointerToUsedTypeName = (pNameInfos->NbPointedTimes !=0);
                                    break;
                                }
                            }
                            // if we are using struct/union being defined as type
                            if (bCurrentStruct)
                            {
                                // assume it is a pointer on this type
                                if (_tcschr(pSubBlockEnd,'*') || bPointerToUsedTypeName)
                                {
                                    bCurrentStructUnionPointer = TRUE;
                                }
                                else
                                {
                                    if (ReportError)
                                    {
                                        TCHAR Msg[MAX_PATH];
                                        _sntprintf(Msg,MAX_PATH,_T("Struct/Union using itself for it's definition %s"),FieldTypeName);
                                        Msg[MAX_PATH-1]=0;
                                        ReportError(Msg,ReportErrorUserParam);
                                        return NULL;
                                    }
                                }

                                break;
                            }
                        }
                    }
                }

                if (bCurrentStructUnionPointer)
                {
                    bNeedToBeAddToList = FALSE;// the caller will add pParentUserDataType to cache lists

                    // delete previously created type
                    delete pUserDataType;

                    // make our CuserDataTypeObject point to the parent one, even it is not currently filled
                    pUserDataType = pParentUserDataType2;
                }
                else
                {
                    bNeedToBeAddToList = FALSE; // CUserDataType::Parse will add type to cache lists

                    // delete previously created type
                    delete pUserDataType;

                    SIZE_T BaseTypeNbPointedTimes = 0;
                    pUserDataType = CUserDataType::Parse(UserDataTypePath,ModuleName,FieldTypeName,&BaseTypeNbPointedTimes,pParentUserDataTypeList,ReportError,ReportErrorUserParam,&DefinitionFound);
                    if (DefinitionFound == DEFINITION_FOUND_NOT_FOUND)
                        CUserDataType::ReportDescriptionNotFoundError(FieldTypeName, ModuleName, ReportError, ReportErrorUserParam);
                }
            }
            
            // we got something like _my_struct { TYPE_1 a; TYPE_2 b; } my_structNameAlias1,my_structNameAlias2,*pmy_struct;
            //                    or _my_struct { TYPE_1 a; TYPE_2 b; } VarName;


            // if expression is a typedef
            if (bTypeDef)
            {
                TCHAR* pNextTypeName;

                ///////////////////////////////
                // Find alternative type names
                ///////////////////////////////
                pTypeName = pSubBlockEnd;
                while (*pTypeName)
                {
                    pTypeName++;

                    // point on first not space character
                    pTypeName = CCodeParserHelper::FindNextNotSplitterChar(pTypeName);

                    // find name type end
                    pTypeNameEnd = _tcspbrk(pTypeName,_T(" ,;"));
                    if (!pTypeNameEnd)
                        pTypeNameEnd = pTypeName+_tcslen(pTypeName);

                    // store next type name position
                    pNextTypeName = pTypeNameEnd;

                    // remove spaces
                    if (!CCodeParserHelper::IsAlphaNumOrUnderscore(*pTypeNameEnd))
                        pTypeNameEnd = CCodeParserHelper::FindPreviousNotSplitterChar(pTypeName,pTypeNameEnd);


                    memset(&TypeNameInfos,0,sizeof(TYPE_NAME_INFOS));

                    // TypeNameInfos.NbPointedTimes = 0; // done by memset
                    while(*pTypeName=='*')
                    {
                        TypeNameInfos.NbPointedTimes++;
                        pTypeName++;
                        pTypeName = CCodeParserHelper::FindNextNotSplitterChar(pTypeName);
                    }

                    // associate name to the type by including it to name list
                    Size = __min( pTypeNameEnd-pTypeName+1, MAX_PATH-1);
                    _tcsncpy(TypeNameInfos.Name, pTypeName, Size );
                    TypeNameInfos.Name[Size]=0;
                    if (*(TypeNameInfos.Name)!=0)
                    {
                        pUserDataType->pNames->AddItem(&TypeNameInfos);

                        // if we are looking for a type
                        if (SearchedType)
                        {
                            // if type is the searched one
                            if (_tcscmp(SearchedType, TypeNameInfos.Name)== 0)
                            {
                                // set the returned value
                                ReturnedpUserDataType = pUserDataType;
                            }
                        }
                    }
                    pTypeName = pNextTypeName;
                }
            }
            else // bTypeDef == FALSE
            {
                // add var to current type
                if (pParentUserDataType)
                {
                    TCHAR* pNbItems;
                    CUserDataTypeVar* pUserDataTypeVar = new CUserDataTypeVar();
                    pUserDataTypeVar->pUserDataType = pUserDataType;
                    if (pParentUserDataType->Type == CUserDataType::TYPE_UNION)
                    {
                        pUserDataTypeVar->bParentIsUnion = TRUE;
                    }

                    // look for var name
                    pTypeName = pSubBlockEnd;

                    // get the number of pointed time
                    pTypeName++;
                    for(;;)
                    {
                        pTypeName = CCodeParserHelper::FindNextNotSplitterChar(pTypeName);
                        if (*pTypeName != '*')
                            break;

                        pUserDataTypeVar->NbPointedTimes++;
                        pTypeName++;
                    }

                    // point on first not space character
                    pTypeName = CCodeParserHelper::FindNextNotSplitterChar(pTypeName);

                    // find name type end
                    pTypeNameEnd = _tcspbrk(pTypeName,_T(" ,;["));
                    if (!pTypeNameEnd)
                        pTypeNameEnd = pTypeName+_tcslen(pTypeName);

                    // remove spaces
                    if (!CCodeParserHelper::IsAlphaNumOrUnderscore(*pTypeNameEnd))
                        pTypeNameEnd = CCodeParserHelper::FindPreviousNotSplitterChar(pTypeName,pTypeNameEnd);

                    Size = __min(PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1,pTypeNameEnd-pTypeName+1);
                    _tcsncpy(pUserDataTypeVar->VarName,pTypeName,Size);
                    pUserDataTypeVar->VarName[Size]=0;

                    // Notice : var name can be empty (unnamed union in struct)
                    //if (*(pUserDataTypeVar->VarName)==0)
                    //{
                    //    delete pUserDataTypeVar;
                    //}
                    //else
                    {
                        // look for array
                        pNbItems = _tcschr(pTypeNameEnd, '[');
                        if (pNbItems)
                        {
                            TCHAR* pArrayDefBegin;
                            TCHAR* pArrayDefEnd;
                            SIZE_T NbArrayItems = 1;
                            SIZE_T NbArrayItemsForCurrentDimension;
                            BOOL bParsingError = FALSE;
                            BOOL bFirstDimension = TRUE;

                            pArrayDefBegin = pNbItems-1; 
                            pArrayDefEnd = pArrayDefBegin;
                            for(;pArrayDefBegin;)
                            {
// may todo : improvement check for [] bad syntax like [[ ] or []]
                                pArrayDefBegin = _tcschr(pArrayDefEnd+1,'[');
                                pArrayDefEnd = _tcschr(pArrayDefEnd+1,']');// use pArrayDefEnd in case pArrayDefBegin is null
                                // if [ not found
                                if (!pArrayDefBegin)
                                {
                                    if (!pArrayDefEnd)// no more dimension
                                        break;
                                }
                                else
                                {
                                    // we get [ without ]
                                    if (!pArrayDefEnd)
                                        bParsingError = TRUE;
                                }

                                if (bParsingError)
                                {
                                    NbArrayItems = 1;
                                    if (ReportError)
                                    {
                                        TCHAR Msg[MAX_PATH];
                                        _sntprintf(Msg,MAX_PATH,_T("Parsing error inside %s definition :   '[' and ']' don't match"),pTypeName);
                                        Msg[MAX_PATH-1]=0;
                                        ReportError(Msg,ReportErrorUserParam);
                                    }

                                    break;
                                }
                                else
                                {
                                    // get size of array dimension
                                    CStringConverter::StringToSIZE_T(pArrayDefBegin+1,&NbArrayItemsForCurrentDimension);
                                    // in case of no number like [] or [0]
                                    if (NbArrayItemsForCurrentDimension == 0)
                                    {
                                        NbArrayItemsForCurrentDimension = 1;
                                    }
                                }

                                // in memory char[x][y][z] is like char[x*y*z] 
                                NbArrayItems *= NbArrayItemsForCurrentDimension;

                                if (bFirstDimension)
                                {
                                    // Warning Do not increase NbPointedTimes in case of struct or union !!!
                                    // // increase pointed time only once for all the array dimensions : char[x][y][z] is like char[x*y*z] in memory 
                                    // NbPointedTimes++;

                                    // get end of name
                                    pTypeNameEnd = pArrayDefBegin;

                                    bFirstDimension = FALSE;
                                }
                            }
                            pUserDataTypeVar->NbItems = NbArrayItems;
                        }

                        // add var to parent type
                        if (!pParentUserDataType->pSubType)
                            pParentUserDataType->pSubType = new CLinkListSimple();
                        pParentUserDataType->pSubType->AddItem(pUserDataTypeVar);

#ifdef _DEBUG
                        {
                            // display some pUserDataTypeVar infos
                            TCHAR Msg[3*MAX_PATH];
                            _sntprintf(Msg,
                                3*MAX_PATH,
                                _T("Added Var : %s nb pointed times : %u, Nb Items : %u, Size : %u \r\n"),
                                pUserDataTypeVar->VarName,
                                pUserDataTypeVar->NbPointedTimes,
                                pUserDataTypeVar->NbItems,
                                pUserDataTypeVar->GetSize()
                                );
                            OutputDebugString(Msg);
                        }
#endif
                    }

                }
            }

            /////////////////////////////
            // add type to cache list
            /////////////////////////////

            // check if current parsed file is in the default directory or in the specified ModuleName subdirectory
            // to add it to the specified list
            // in case of multiple same type declaration in different paths, this avoid confusion

            // assume definition is not already in cache
            // else the same object will be present twice in cache lists that will produce crash at memory releasing
            if (bNeedToBeAddToList)
            {
                // if ModuleName is found in CurrentParsedFileName
                if (_tcsstr(CurrentParsedFileName,ModuleName))
                    UserDataTypeManager.AddToList(ModuleName,pUserDataType);
                else
                    UserDataTypeManager.AddToDefaultList(pUserDataType);
            }
        }
        else // TYPE_RENAMING
        {
            if (bTypeDef)
            {
                TCHAR OldTypeName[MAX_PATH];
                TCHAR NewTypeName[MAX_PATH];
                NbPointedTimes=0;

                //////////////////
                // find new type name
                //////////////////

                // in case of function callback type name is before first ')'
                pTypeNameEnd = _tcschr(pTypeBegin,')');
                if (!pTypeNameEnd)
                // else type name is last string before ';'
                {
                    pTypeNameEnd = _tcsrchr(pTypeBegin,';');
                }
                if (!pTypeNameEnd)
                {
                    pTypeNameEnd = pTypeBegin + _tcslen(pTypeBegin);
                }
                if (!CCodeParserHelper::IsAlphaNumOrUnderscore(*pTypeNameEnd))
                    pTypeNameEnd = CCodeParserHelper::FindPreviousNotSplitterChar(pTypeBegin,pTypeNameEnd);
                pTypeName = CCodeParserHelper::FindPreviousNotAlphaNumOrUnderscore(pTypeBegin,pTypeNameEnd);
                if (pTypeName!=pTypeBegin)
                    pTypeName++;

                Size = __min(pTypeNameEnd-pTypeName+1,MAX_PATH-1);
                _tcsncpy(NewTypeName,pTypeName,Size);
                NewTypeName[Size]=0;

                // look for * before pTypeName to find number of pointed times
                for(;pTypeName>pTypeBegin;)
                {
                    pTypeName = CCodeParserHelper::FindPreviousNotSplitterChar(pTypeBegin,pTypeName);
                    if (*pTypeName != '*')
                        break;

                    NbPointedTimes++;
                    pTypeName--;
                }

                //////////////////
                // find old type name
                //////////////////
                pTypeName = pTypeBegin;
                pTypeNameEnd = CCodeParserHelper::FindNextNotTypeDescriptorChar(pTypeBegin);
                pTypeNameEnd--;

                Size = __min(pTypeNameEnd-pTypeName+1,MAX_PATH-1);
                _tcsncpy(OldTypeName,pTypeName,Size);
                OldTypeName[Size]=0;

                // assume the old base type being rename is parsed
                SIZE_T BaseTypeNbPointedTimes=0;
                CUserDataType* pUserDataType = CUserDataType::Parse(UserDataTypePath,ModuleName,OldTypeName,&BaseTypeNbPointedTimes,pParentUserDataTypeList,ReportError,ReportErrorUserParam,&DefinitionFound);
                if (DefinitionFound == DEFINITION_FOUND_NOT_FOUND)
                    CUserDataType::ReportDescriptionNotFoundError(OldTypeName, ModuleName, ReportError, ReportErrorUserParam);

                // add new name to existing base type
                memset(&TypeNameInfos,0,sizeof(TYPE_NAME_INFOS));
                _tcsncpy(TypeNameInfos.Name, NewTypeName,MAX_PATH-1);
                TypeNameInfos.NbPointedTimes = NbPointedTimes;
                pUserDataType->pNames->AddItem(&TypeNameInfos);

                // if we are looking for a type
                if (SearchedType)
                {
                    // if name match the searched one
                    if (_tcscmp(SearchedType, TypeNameInfos.Name)== 0)
                    {
                        // set the returned value
                        ReturnedpUserDataType = pUserDataType;
                    }
                }
            }
            else // bTypeDef == FALSE
            // expression // TYPE[*] [keywords] NAME[[NbItems]];
            {
                if (pParentUserDataType)
                {
                    TCHAR TypeName[MAX_PATH];
                    TCHAR* pLastVarSeparator;
                    TCHAR* pNextBitsFieldSeparator;
                    CUserDataType* pUserDataType;
                    CUserDataTypeVar* pUserDataTypeVar; 
                    BOOL bAddCurrentVarToParentType;

                    NbPointedTimes=0;
                    pUserDataType= NULL;

                    //////////////////
                    // find type name
                    //////////////////

                    // get name
                    pTypeName = pTypeBegin;
FindTypeName:
                    pTypeNameEnd = CCodeParserHelper::FindNextNotTypeDescriptorChar(pTypeName);
                    pTypeNameEnd--;

                    Size = __min(pTypeNameEnd-pTypeName+1,MAX_PATH-1);
                    _tcsncpy(TypeName,pTypeName,Size);
                    TypeName[Size]=0;
                    // trash unsigned, const, ... keywords
                    if (IsKeyWord(TypeName))
                    {
                        pTypeName = CCodeParserHelper::FindNextNotSplitterChar(pTypeNameEnd+1);
                        goto FindTypeName;
                    }

                    // look for * after type
                    pTypeNameEnd++;
                    for(;;)
                    {
                        pTypeNameEnd = CCodeParserHelper::FindNextNotSplitterChar(pTypeNameEnd);
                        if (*pTypeNameEnd != '*')
                            break;

                        NbPointedTimes++;
                        pTypeNameEnd++;
                    }
                    CLinkListItem* pItemParentUserDataType;
                    CUserDataType* pParentUserDataType2;
                    for (pItemParentUserDataType = pParentUserDataTypeList->Tail;pItemParentUserDataType;pItemParentUserDataType = pItemParentUserDataType->PreviousItem)
                    {
                        pParentUserDataType2 = (CUserDataType*)pItemParentUserDataType->ItemData;
                        if (pParentUserDataType2->pNames) // if parent name list exist
                        {
                            // search through parents struct/union names
                            // to check if it is a pointer on the struct/union being defined
                            CLinkListItem* pItemName;
                            TYPE_NAME_INFOS* pNameInfos;
                            BOOL bCurrentStruct = FALSE;
                            BOOL bPointerToUsedTypeName = FALSE;
                            for (pItemName = pParentUserDataType2->pNames->Head;pItemName;pItemName = pItemName->NextItem)
                            {
                                pNameInfos = (TYPE_NAME_INFOS*)pItemName->ItemData;
                                if (_tcscmp(TypeName,pNameInfos->Name)==0)
                                {
                                    // the type is the struct / union being defined
                                    bCurrentStruct = TRUE;
                                    bPointerToUsedTypeName = (pNameInfos->NbPointedTimes !=0);
                                    break;
                                }
                            }
                            // if we are using struct/union being defined as type
                            if (bCurrentStruct)
                            {
                                // assume it is a pointer on this type
                                if (NbPointedTimes || bPointerToUsedTypeName)
                                {
                                    // use unknown type as unknown is treated like pointer
                                    pUserDataType = CUserDataType::FindOrAddUnknownTypeInDefaultList();
                                }
                                else
                                {
                                    if (ReportError)
                                    {
                                        TCHAR Msg[MAX_PATH];
                                        _sntprintf(Msg,MAX_PATH,_T("Struct/Union using itself for it's definition %s"),TypeName);
                                        Msg[MAX_PATH-1]=0;
                                        ReportError(Msg,ReportErrorUserParam);
                                        return NULL;
                                    }
                                }

                                break;
                            }
                        }
                    }

                    if (!pUserDataType)
                    {
                        if (NbPointedTimes!=0)  // required has NbPointedTimes is not passed into pParentUserDataTypeList
                                                // to avoid "Struct/Union using itself for it's definition" error, we should pass 
                                                // NbPointedTimes with UserDataType in pParentUserDataTypeList (to check for pointer next)
                                                // so we use the lazy way : undefined pointer 
                        {
                            // use unknown type as unknown is treated like pointer
                            pUserDataType = CUserDataType::FindOrAddUnknownTypeInDefaultList();
                        }
                        else
                        {
                            // find type informations
                            SIZE_T BaseTypeNbPointedTimes=NbPointedTimes;
                            pUserDataType = CUserDataType::Parse(UserDataTypePath,ModuleName,TypeName,&BaseTypeNbPointedTimes,pParentUserDataTypeList,ReportError,ReportErrorUserParam,&DefinitionFound);
                            if (DefinitionFound == DEFINITION_FOUND_NOT_FOUND)
                                CUserDataType::ReportDescriptionNotFoundError(TypeName, ModuleName, ReportError, ReportErrorUserParam);
                        }
                    }

                    //////////////////
                    // find var name(s)
                    //////////////////
                    pLastVarSeparator = pTypeNameEnd;
                    pTypeBegin = pTypeNameEnd;
                    for(;(pLastVarSeparator!=0);pTypeBegin = pLastVarSeparator +1)
                    {
                        // reset flag to add var
                        bAddCurrentVarToParentType = TRUE;

                        // get end of expression ';'
                        // and look for potentials multiple definitions ','
                        pLastVarSeparator = _tcspbrk(pTypeBegin,_T(";,"));
                        pTypeNameEnd = pLastVarSeparator;
                        if (!pTypeNameEnd) // in case ';' was not kept
                        {
                            // get remaining chars
                            pTypeNameEnd = pTypeBegin + _tcslen(pTypeBegin);
                        }
                        // remove ending spacers
                        if (!CCodeParserHelper::IsAlphaNumOrUnderscore(*pTypeNameEnd))
                            pTypeNameEnd = CCodeParserHelper::FindPreviousNotSplitterChar(pTypeBegin,pTypeNameEnd);

                        // look for bit fields
                        pNextBitsFieldSeparator = _tcschr(pTypeBegin,':');
                        if (    (pNextBitsFieldSeparator!=0) // if a bit field as been found
                             && (pNextBitsFieldSeparator<pTypeNameEnd) // and bit fields apply to current var (currently pTypeNameEnd points to first not splitter char before pLastVarSeparator)
                            )
                        {
                            // DWORD Bit1:1 , Bit2:1, Bit3_4:2, unused:28;

                            // or

                            //struct Date {
                            //    unsigned nWeekDay  : 3;    // 0..7   (3 bits)
                            //    unsigned nMonthDay : 6;    // 0..31  (6 bits)
                            //    unsigned           : 0;    // Force alignment to next boundary.
                            //    unsigned nMonth    : 5;    // 0..12  (5 bits)
                            //    unsigned nYear     : 8;    // 0..100 (8 bits)
                            //};

                            // Microsoft Specific The ordering of data declared as bit fields is from low to high bit

                            CUserDataTypeVar::BITS_FIELD_INFOS_LITE BitsFieldInfosLite;

                            // find field name
                            if (CCodeParserHelper::IsSpaceOrSplitter(*(pNextBitsFieldSeparator-1)))
                                pTypeNameEnd = CCodeParserHelper::FindPreviousNotSplitterChar(pTypeBegin,pNextBitsFieldSeparator-1);
                            else
                                pTypeNameEnd = pNextBitsFieldSeparator-1;
                            pTypeName = CCodeParserHelper::FindPreviousNotAlphaNumOrUnderscore(pTypeBegin, pTypeNameEnd);
                            if (pTypeName != pTypeBegin)
                                pTypeName++;

                            // save var name
                            memset(&BitsFieldInfosLite,0,sizeof(BitsFieldInfosLite));
                            Size= __min(pTypeNameEnd-pTypeName+1,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1);
                            _tcsncpy(BitsFieldInfosLite.VarName,pTypeName, Size);
                            BitsFieldInfosLite.VarName[Size]=0;

                            // check the case of unnamed field (generally the case for TYPE :0; for boundary alignment
                            if (pTypeName == pTypeNameEnd)
                            {
                                // if unnamed field
                                if (!CCodeParserHelper::IsAlphaNumOrUnderscore(*pTypeName))
                                    // assume to remove potential not alpha num char
                                    BitsFieldInfosLite.VarName[0]=0;
                            }

                            // find bit number
                            pNextBitsFieldSeparator++;

                            // find bits field size
                            CStringConverter::StringToSIZE_T(pNextBitsFieldSeparator,&BitsFieldInfosLite.Size);

                            // if previous type 
                            //  - is the same
                            //  - is bitfield
                            //  - has enough place to store data
                            pUserDataTypeVar = NULL;
                            if (pParentUserDataType->pSubType)
                            {
                                if (pParentUserDataType->pSubType->Tail)
                                {
                                    if (pParentUserDataType->pSubType->Tail->ItemData)
                                    {
                                        CUserDataTypeVar* PreviousVar = (CUserDataTypeVar*) pParentUserDataType->pSubType->Tail->ItemData;

                                        // if previous type is the same
                                        if (PreviousVar->pUserDataType == pUserDataType)
                                        {
                                            // if previous type is a bits fields
                                            if (PreviousVar->IsBitsFields())
                                            {
                                                // if previous type can store required bits number
                                                if (PreviousVar->GetBitsFieldRemainingSize()>=BitsFieldInfosLite.Size)
                                                {
                                                    // use the previous type instead
                                                    pUserDataTypeVar = PreviousVar;

                                                    // current var musn't be added one more time
                                                    bAddCurrentVarToParentType = FALSE;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            // if we can't use previous var for bit storing
                            if (!pUserDataTypeVar) // bAddCurrentVarToParentType == TRUE
                            {
                                // create CUserDataTypeVar and initialize it with known values
                                pUserDataTypeVar=new CUserDataTypeVar();
                                pUserDataTypeVar->bParentIsUnion = (pParentUserDataType->Type == CUserDataType::TYPE_UNION);
                                pUserDataTypeVar->pUserDataType = pUserDataType;
                            }

                            // add current bits field to current var
                            if (!pUserDataTypeVar->AddBitsField(&BitsFieldInfosLite))
                            {
#ifdef _DEBUG
                                if (::IsDebuggerPresent())
                                    ::DebugBreak();
#endif
                            }
#ifdef _DEBUG
                            {
                                // display some BitsFieldInfos infos
                                TCHAR Msg[3*MAX_PATH];
                                _sntprintf(Msg,
                                    3*MAX_PATH,
                                    _T("BitsField Name : %s Size : 0x%X\r\n"),
                                    BitsFieldInfosLite.VarName,
                                    BitsFieldInfosLite.Size
                                    );
                                OutputDebugString(Msg);
                            }
#endif

                        } // end look for bit fields
                        else // no bits fields
                        {
                            // create CUserDataTypeVar and initialize it with known values
                            pUserDataTypeVar=new CUserDataTypeVar();
                            pUserDataTypeVar->bParentIsUnion = (pParentUserDataType->Type == CUserDataType::TYPE_UNION);
                            pUserDataTypeVar->pUserDataType = pUserDataType;

                            // look for array
                            if (*pTypeNameEnd == ']')
                            {
                                TCHAR* pArrayDefBegin;
                                TCHAR* pArrayDefEnd;
                                SIZE_T NbArrayItems = 1;
                                SIZE_T NbArrayItemsForCurrentDimension;
                                BOOL bParsingError = FALSE;
                                BOOL bFirstDimension = TRUE;

                                pArrayDefBegin = pTypeBegin; 
                                pArrayDefEnd = pArrayDefBegin;
                                for(;pArrayDefBegin;)
                                {
// may todo : improvement check for [] bad syntax like [[ ] or []]
                                    pArrayDefBegin = _tcschr(pArrayDefEnd+1,'[');
                                    pArrayDefEnd = _tcschr(pArrayDefEnd+1,']');// use pArrayDefEnd in case pArrayDefBegin is null
                                    // if [ not found
                                    if (!pArrayDefBegin)
                                    {
                                        // if first loop
                                        if (pArrayDefEnd)
                                        {
                                            // we get ] without [
                                            bParsingError = TRUE;
                                        }
                                        else // no more dimension
                                            break;
                                    }
                                    else
                                    {
                                        // we get [ without ]
                                        if (!pArrayDefEnd)
                                            bParsingError = TRUE;
                                    }

                                    if (bParsingError)
                                    {
                                        NbArrayItems = 1;
                                        if (ReportError)
                                        {
                                            TCHAR Msg[MAX_PATH];
                                            _sntprintf(Msg,MAX_PATH,_T("Parsing error inside %s definition :   '[' and ']' don't match"),pTypeBegin);
                                            Msg[MAX_PATH-1]=0;
                                            ReportError(Msg,ReportErrorUserParam);
                                        }

                                        break;
                                    }
                                    else
                                    {
                                        // get number of items in array
                                        CStringConverter::StringToSIZE_T(pArrayDefBegin+1,&NbArrayItemsForCurrentDimension);
                                        // in case of no number like [] or [0]
                                        if (NbArrayItemsForCurrentDimension == 0)
                                        {
                                            NbArrayItemsForCurrentDimension = 1;
                                        }
                                    }

                                    // in memory char[x][y][z] is like char[x*y*z] 
                                    NbArrayItems *= NbArrayItemsForCurrentDimension;

                                    if (bFirstDimension)
                                    {
                                        // Warning Do not increase NbPointedTimes in case of struct or union !!!
                                        // // increase pointed time only once for all the array dimensions : char[x][y][z] is like char[x*y*z] in memory 
                                        // NbPointedTimes++;

                                        // end var name (point before '[')
                                        pTypeNameEnd = pArrayDefBegin-1;

                                        bFirstDimension = FALSE;
                                    }
                                }
                                // fill number of item struct field
                                pUserDataTypeVar->NbItems = NbArrayItems;
                            }

                            // find type name begin (remove spacers)
                            pTypeName = CCodeParserHelper::FindPreviousNotAlphaNumOrUnderscore(pTypeBegin,pTypeNameEnd);
                            if (pTypeName!=pTypeBegin)
                                pTypeName++;

                            // copy name to pUserDataTypeVar object
                            Size = __min(pTypeNameEnd-pTypeName+1,MAX_PATH-1);
                            _tcsncpy(pUserDataTypeVar->VarName,pTypeName,Size);
                            pUserDataTypeVar->VarName[Size]=0;

                            //// look for * before var name already done see "look for * after type"
                            //for(;pTypeName>pTypeBegin;)
                            //{
                            //    pTypeName = CCodeParserHelper::FindPreviousNotSplitterChar(pTypeBegin,pTypeName);
                            //    if (*pTypeName != '*')
                            //        break;

                            //    NbPointedTimes++;
                            //    pTypeName--;
                            //}

                            // fill the number of pointed time
                            pUserDataTypeVar->NbPointedTimes = NbPointedTimes;
                        }


                        if (bAddCurrentVarToParentType)
                        {
                            // create sub type list if not already done
                            if (!pParentUserDataType->pSubType)
                                pParentUserDataType->pSubType = new CLinkListSimple();

                            // add new var to list 
                            pParentUserDataType->pSubType->AddItem(pUserDataTypeVar);

#ifdef _DEBUG
                            {
                                // display some pUserDataTypeVar infos
                                TCHAR Msg[3*MAX_PATH];
                                _sntprintf(Msg,
                                    3*MAX_PATH,
                                    _T("Added Var : %s nb pointed times : %u, Nb Items : %u, Size : %u \r\n"),
                                    pUserDataTypeVar->VarName,
                                    pUserDataTypeVar->NbPointedTimes,
                                    pUserDataTypeVar->NbItems,
                                    pUserDataTypeVar->GetSize()
                                    );
                                OutputDebugString(Msg);
                            }
#endif
                        }// enf if (bAddCurrentVarToParentType)
                    } // end for each var names associated to type
                } // end if (pParentUserDataType)
            }// end // bTypeDef == FALSE
        }// end else // TYPE_RENAMING

    }// for each found expression

    // return ReturnedpUserDataType containing user type info if found, NULL else
    return ReturnedpUserDataType;
}