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
#include "typeinfos.h"

CTypeInfos::CTypeInfos(void)
{
    this->Name=NULL;
    this->bConst=FALSE;
    this->bVolatile=FALSE;
    this->bUnaligned=FALSE;
    this->bEnum=FALSE;
    this->bFunction=FALSE;
    this->bRefPointer=FALSE;
    this->bUserDefineType=FALSE;
    this->PointerLevel=0;
    this->Size=0;
    this->TypeName=NULL;
    this->BaseType=btNoType;
    this->UserDefineTypeKind=UdtStruct;
    this->pSymbolLocation=new CSymbolLocation();
    this->DataKind=SymTagNull;
}

CTypeInfos::~CTypeInfos(void)
{
    if (this->Name)
        free(this->Name);
    if (this->TypeName)
        free(this->TypeName);
    delete this->pSymbolLocation;
}

//-----------------------------------------------------------------------------
// Name: GetPrettyName
// Object: get type name with all it's attributes
// Parameters :
//     inout : TCHAR* pszPrettyName : buffer containing informations
//     in  : SIZE_T pszPrettyNameMaxSize : pszPrettyName max size in TCHAR
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CTypeInfos::GetPrettyName(TCHAR* pszPrettyName,SIZE_T pszPrettyNameMaxSize)
{
    pszPrettyNameMaxSize--;
    *pszPrettyName=0;
    pszPrettyName[pszPrettyNameMaxSize-1]=0;
    SIZE_T RemainingSize=pszPrettyNameMaxSize;

    if (this->bConst)
    {
        _tcsncat(pszPrettyName,_T("const "),RemainingSize);
        RemainingSize=pszPrettyNameMaxSize-_tcslen(pszPrettyName);
        if (RemainingSize==0)
            return FALSE;
    }
    if (this->bVolatile)
    {
        _tcsncat(pszPrettyName,_T("volatile "),RemainingSize);
        RemainingSize=pszPrettyNameMaxSize-_tcslen(pszPrettyName);
        if (RemainingSize==0)
            return FALSE;
    }
    if (this->bEnum)
    {
        _tcsncat(pszPrettyName,_T("enum "),RemainingSize);
        RemainingSize=pszPrettyNameMaxSize-_tcslen(pszPrettyName);
        if (RemainingSize==0)
            return FALSE;
    }
    // add param type
    if (this->TypeName)
    {
        _tcsncat(pszPrettyName,this->TypeName,RemainingSize);
        RemainingSize=pszPrettyNameMaxSize-_tcslen(pszPrettyName);
        if (RemainingSize==0)
            return FALSE;
    }

    // add pointer/ref
    for (DWORD Cnt=0;Cnt<this->PointerLevel;Cnt++)
    {
        if (this->bRefPointer)
        {
            _tcsncat(pszPrettyName,_T("&"),RemainingSize);
            RemainingSize--;
            if (RemainingSize==0)
                return FALSE;
        }
        else
        {
            _tcscat(pszPrettyName,_T("*"));
            RemainingSize--;
            if (RemainingSize==0)
                return FALSE;
        }
    }
    // add space
    _tcscat(pszPrettyName,_T(" "));
    RemainingSize--;
    if (RemainingSize==0)
        return FALSE;

    // add param name
    if (this->Name)
    {
        _tcsncat(pszPrettyName,this->Name,RemainingSize);
        RemainingSize=pszPrettyNameMaxSize-_tcslen(pszPrettyName);
        if (RemainingSize==0)
            return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsParam
// Object: check if type is a parameter
// Parameters :
//     in  : DWORD DataKind
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CTypeInfos::IsParam(DWORD DataKind)
{
    /*
    DataIsUnknown 
    Data symbol cannot be determined. 
    DataIsLocal 
    Data item is a local variable. 
    DataIsStaticLocal 
    Data item is a static local variable. 
    DataIsParam 
    Data item is a formal parameter. 
    DataIsObjectPtr 
    Data item is an object pointer (this). 
    DataIsFileStatic 
    Data item is a file-scoped variable. 
    DataIsGlobal 
    Data item is a global variable. 
    DataIsMember 
    Data item is an object member variable. 
    DataIsStaticMember 
    Data item is a class static variable. 
    DataIsConstant 
    Data item is a constant value. 
    */
    return (DataKind==DataIsParam);
}

BOOL CTypeInfos::Parse(IDiaSymbol *pSymbol)
{
    if (!this->pSymbolLocation->Parse(pSymbol))
        return FALSE;
    BOOL bRet=this->GetType(pSymbol,FALSE);
    if (!this->Name)
        this->Name=_tcsdup(_T(""));
    return bRet;
}

const wchar_t * const rgUdtKind[] =
{
    L"struct",
    L"class",
    L"union",
    L"enum"
};

// Basic types
const wchar_t * const rgBaseType[] = {
    L"void",                 // btNoType = 0,
    L"void",                 // btVoid = 1,
    L"char",                 // btChar = 2,
    L"wchar_t",              // btWChar = 3,
    L"signed char",
    L"unsigned char",
    L"int",                  // btInt = 6,
    L"unsigned int",         // btUInt = 7,
    L"float",                // btFloat = 8,
    L"BCD",                  // btBCD = 9,
    L"bool",                 // btBool = 10,
    L"short",
    L"unsigned short",
    L"long",                 // btLong = 13,
    L"unsigned long",        // btULong = 14,
    L"byte",
    L"short",
    L"int",
    L"int64",
    L"int128",
    L"byte",
    L"ushort",
    L"DWORD",
    L"uint64",
    L"uint128",
    L"currency",           // btCurrency = 25,
    L"date",               // btDate = 26,
    L"VARIANT",              // btVariant = 27,
    L"complex",            // btComplex = 28,
    L"bit",                // btBit = 29,
    L"BSTR",                 // btBSTR = 30,
    L"HRESULT"              // btHresult = 31
};

BOOL CTypeInfos::GetType(IDiaSymbol* pSymbol, BOOL bBaseType )
{
    BOOL bRet;
    HRESULT hResult;
    IDiaSymbol* pBaseType;
    DWORD dwTag=SymTagNull;
    
    // until a name is found try to get name
    if (!this->Name)
    {
        BSTR Name=NULL;
        pSymbol->get_name(&Name);

        if (Name)
        {
#if (defined(UNICODE)||defined(_UNICODE))
            this->Name=_tcsdup(Name);
#else
            CAnsiUnicodeConvert::UnicodeToAnsi(Name,&this->Name);
#endif
            SysFreeString(Name);
        }
    }
    else
    {
        if ( bBaseType && (!this->TypeName) )
        {
            BSTR TypeName=NULL;
            pSymbol->get_name(&TypeName);

            if (TypeName)
            {
#if (defined(UNICODE)||defined(_UNICODE))
                this->TypeName=_tcsdup(TypeName);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(TypeName,&this->TypeName);
#endif
                SysFreeString(TypeName);
            }
        }
    }

    pSymbol->get_length(&this->Size);
    pSymbol->get_constType(&this->bConst);
    pSymbol->get_volatileType(&this->bVolatile);
    pSymbol->get_unalignedType(&this->bUnaligned);

    hResult=pSymbol->get_symTag(&dwTag);
    if(FAILED(hResult) || (dwTag==SymTagNull))
        return FALSE;

    if (this->DataKind==SymTagNull)
        this->DataKind=dwTag;

    switch(dwTag)
    {
    case SymTagUDT:
        {
            BSTR UDTName;
            if (FAILED(pSymbol->get_udtKind(&this->UserDefineTypeKind)))
                return FALSE;

            pSymbol->get_name(&UDTName);
            WCHAR* wName;
            // don't put struct / enum / class keyword info to this->TypeName
            // keep only type name
            // wName =(WCHAR*) _alloca((wcslen(rgUdtKind[this->UserDefineTypeKind])+wcslen(UDTName)+2)*sizeof(WCHAR));
            //swprintf(wName,L"%s %s",rgUdtKind[this->UserDefineTypeKind],UDTName);
            wName =(WCHAR*) _alloca((wcslen(UDTName)+1)*sizeof(WCHAR));
            wcscpy(wName,UDTName);
            this->bUserDefineType=TRUE;

#if (defined(UNICODE)||defined(_UNICODE))
            this->TypeName=_tcsdup(wName);
#else
            CAnsiUnicodeConvert::UnicodeToAnsi(wName,&this->TypeName);
#endif
            SysFreeString(UDTName);
        }
        break;

    case SymTagEnum:
        this->bEnum=TRUE;
        break;

    case SymTagFunctionType:
        this->bFunction=TRUE;
        break;

    case SymTagPointerType:
        pBaseType=0;
        pSymbol->get_reference(&this->bRefPointer);
        hResult=pSymbol->get_type(&pBaseType);
        if(FAILED(hResult) || (pBaseType==0))
            return FALSE;

        bRet=this->GetType(pBaseType,TRUE);
        this->PointerLevel++;
        pBaseType->Release();
        if (!bRet)
            return FALSE;
        break;

    case SymTagArrayType:
        pBaseType=0;
        hResult=pSymbol->get_type(&pBaseType);
        if(FAILED(hResult) || (pBaseType==0))
            return FALSE;

        // simplify , don't get array length (sized provided for local vars)
        this->PointerLevel++;
        bRet=this->GetType(pBaseType,TRUE);
        pBaseType->Release();
        if (!bRet)
            return FALSE;
        break;
    case SymTagBaseType:
        {
            WCHAR wsType[MAX_PATH];
            ULONGLONG ulLen;
            pSymbol->get_length(&ulLen); // ukLen may differ from this->Size (if pointer or other type)
            *wsType=0;

            if(FAILED(pSymbol->get_baseType(&this->BaseType)))
                return FALSE;

            switch(this->BaseType)
            {
                case btUInt :
                    wcscpy(wsType,L"unsigned ");
                // don't break btUInt to Fall through btInt
                case btInt :
                    switch(ulLen)
                    {
                        case 1:
                            wcscat(wsType,L"char");
                            break;
                        case 2:
                            wcscat(wsType,L"short");
                            break;
                        case 4:
                            wcscat(wsType,L"int");
                            break;
                        case 8:
                            wcscat(wsType,L"int64");
                            break;
                    }
                    break;
                case btFloat :
                    switch(ulLen)
                    {
                        case 4:
                            wcscpy(wsType,L"float");
                            break;
                        case 8:
                            wcscpy(wsType,L"double");
                            break;
                    }
                    break;
                }

                // if type not filled
                if(*wsType==0)
                    // get the one defined by array
                    wcscpy(wsType,rgBaseType[this->BaseType]);
            
#if (defined(UNICODE)||defined(_UNICODE))
                this->TypeName=_tcsdup(wsType);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(wsType,&this->TypeName);
#endif
        }
        break;
    case SymTagCustomType: 
        break;
    case SymTagData:
        pBaseType=0;
        hResult=pSymbol->get_type(&pBaseType);
        if(FAILED(hResult) || (pBaseType==0))
            return FALSE; // no information : can appear for local type (static can be name of local vars. Debug infos ???)

        bRet=this->GetType(pBaseType,TRUE);
        pBaseType->Release();
        if (!bRet)
            return FALSE;
        break;
    case SymTagUsingNamespace:
        break;
    case SymTagTypedef: // should not appear
        break;
    default:
#ifdef _DEBUG
        if (IsDebuggerPresent())
            DebugBreak();
#endif
        break;
    }

    return TRUE;
}