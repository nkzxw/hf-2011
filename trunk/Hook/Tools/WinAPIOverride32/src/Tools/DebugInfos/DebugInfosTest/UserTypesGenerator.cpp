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
// Object: generates user types (struct, union, enum) files from pdb
//-----------------------------------------------------------------------------

#include "UserTypesGenerator.h"

#define CUserTypesGenerator__unnamed_Prefix L"__unnamed::"
#define CUserTypesGenerator__unnamed_Type L"::__unnamed"

CUserTypesGenerator::CUserTypesGenerator(IDiaSymbol* pSymbol,TCHAR* UserTypePath,CTypesGeneratedManager* pTypesGenerated)
{
    this->pRootSymbol = pSymbol;
    this->UserTypePath = UserTypePath;
    this->pTypesGenerated = pTypesGenerated;
    this->RootSymbolSymTag = SymTagNull;
    this->PreviousSymbol = 0;
    this->CurrentTypeArrayDimensions[0]=0;
}

CUserTypesGenerator::~CUserTypesGenerator(void)
{
}
////////////////////////////////////////////////////////////
// Print bound information
//
void CUserTypesGenerator::PrintBound(FILE* hFile,IDiaSymbol* pSymbol)
{
    DWORD dwTag = 0;
    DWORD dwKind;

    if(pSymbol->get_symTag(&dwTag) != S_OK){
        fwprintf( hFile, L"ERROR - PrintBound() get_symTag");
        return;
    }
    if(pSymbol->get_locationType(&dwKind) != S_OK){
        fwprintf( hFile, L"ERROR - PrintBound() get_locationType");
        return;
    }
    this->PrintName(hFile,pSymbol);

}

////////////////////////////////////////////////////////////
// Print the name of the symbol
//
void CUserTypesGenerator::PrintName(FILE* hFile , IDiaSymbol* pSymbol)
{
    BSTR wszName;
    BSTR wszUndName;
    BSTR wszShortName;

    if(pSymbol->get_name(&wszName) != S_OK)
    {
        fwprintf( hFile, L"UNNAMED");
        if (this->CurrentTypeArrayDimensions[0]!=0)
        {
            fwprintf( hFile,this->CurrentTypeArrayDimensions);
            this->CurrentTypeArrayDimensions[0] = 0;
        }
        return;
    }
    if(pSymbol->get_undecoratedName(&wszUndName) == S_OK)
    {
        ::SysFreeString(wszName);
        wszName = wszUndName;
    }

    if (wcsncmp(wszName,CUserTypesGenerator__unnamed_Prefix,wcslen(CUserTypesGenerator__unnamed_Prefix))==0)
    {
        wszShortName = &wszName[wcslen(CUserTypesGenerator__unnamed_Prefix)];
    }
    else
    {
        wszShortName = wszName;
    }

    // look for L"::__unnamed" -->  to replace with L"__unnamed_ID" 
    WCHAR* wp;
    WCHAR UnamedName[MAX_PATH];
    wp = wcsstr(wszShortName,CUserTypesGenerator__unnamed_Type);
    if (wp)
    {
        DWORD Id;
        if (pSymbol->get_symIndexId(&Id)==S_OK)
        {
            _stprintf(UnamedName,L"__unnamed_0x%X",Id);
            wszShortName = UnamedName;
        }
    }

    fwprintf( hFile, L"%s", wszShortName);
    if (this->CurrentTypeArrayDimensions[0]!=0)
    {
        fwprintf( hFile,this->CurrentTypeArrayDimensions);
        this->CurrentTypeArrayDimensions[0] = 0;
    }

    ::SysFreeString(wszName);
}

void CUserTypesGenerator::PrintUDT(FILE* hFile ,IDiaSymbol* pSymbol)
{
    this->PrintSymbolType(hFile,pSymbol);
    this->PrintName(hFile,pSymbol);
}

void CUserTypesGenerator::EndPreviousBitFieldIfRequired(FILE* hFile ,IDiaSymbol* pSymbol)
{
    if (!this->PreviousSymbol)
        return;

    DWORD dwLocType;
    DWORD PreviousLocType;
    ULONGLONG PreviousTypeSize;

    // get previous location type
    if(this->PreviousSymbol->get_locationType(&PreviousLocType) != S_OK)
        return;

    // if previous type wasn't a bit field, nothing to complete
    if (PreviousLocType != LocIsBitField)
        return;

    // get previous type base type (type without bit field set)
    IDiaSymbol* pPreviousSymbolType = NULL;
    if(this->PreviousSymbol->get_type(&pPreviousSymbolType) != S_OK)
        return;
    // get previous type base type len
    pPreviousSymbolType->get_length(&PreviousTypeSize);
    // translate from byte count to bit count
    PreviousTypeSize*=8;

    LONG PreviousByteOffset;
    DWORD PreviousBitPosition;
    ULONGLONG PreviousBitsLen;
    SSIZE_T UnusedBitsLen;

    if (   (this->PreviousSymbol->get_offset(&PreviousByteOffset) != S_OK) 
        || (this->PreviousSymbol->get_bitPosition(&PreviousBitPosition) != S_OK)
        || (this->PreviousSymbol->get_length(&PreviousBitsLen) != S_OK )
        )
        goto CleanUp;

    // get current location type
    if(pSymbol->get_locationType(&dwLocType) != S_OK)
        goto CleanUp;

    // if current type is not a bit field
    if (dwLocType != LocIsBitField)
    {
        // complete previous type
        UnusedBitsLen = PreviousTypeSize - (PreviousBitPosition + PreviousBitsLen);
        // if type has been fully completed
        if (UnusedBitsLen<=0)
            goto CleanUp;
        // else

        // print previous symbol
        this->PrintDepth(hFile,1);
        this->PrintType(hFile,pPreviousSymbolType);
        fwprintf( hFile, L" unused:%u;\r\n",UnusedBitsLen);  
    }
    else // dwLocType == LocIsBitField
    {      
        DWORD dwBitPos;
        LONG lOffset;
        ULONGLONG ulLen;
        if(
            (pSymbol->get_offset(&lOffset) == S_OK) &&
            (pSymbol->get_bitPosition(&dwBitPos) == S_OK) &&
            (pSymbol->get_length(&ulLen) == S_OK )
            )
        {
            // check if previous type should be completed

            // if offset has changed
            if (lOffset>PreviousByteOffset)
            {
                UnusedBitsLen = PreviousTypeSize - (PreviousBitPosition + PreviousBitsLen);

                // if type has been fully completed
                if (UnusedBitsLen<=0)
                    goto CleanUp;
                // else

                // print previous symbol
                this->PrintDepth(hFile,1);
                this->PrintType(hFile,pPreviousSymbolType);
                fwprintf( hFile, L" unused:%u;\r\n",UnusedBitsLen);  
            }
        }
    }
CleanUp:
    if(pPreviousSymbolType)
        pPreviousSymbolType->Release();
}

////////////////////////////////////////////////////////////
// Print a string corresponding to a location type
//
void CUserTypesGenerator::PrintBitField(FILE* hFile ,IDiaSymbol* pSymbol)
{
    DWORD dwLocType;
    ULONGLONG ulLen;

    if(pSymbol->get_locationType(&dwLocType) != S_OK)
        return;

    switch(dwLocType)
    {
    case LocIsBitField:
        if(pSymbol->get_length(&ulLen) == S_OK )
        {
            fwprintf( hFile, L":%u",ulLen);
#ifdef _DEBUG
            LONG lOffset;
            DWORD dwBitPos;
            pSymbol->get_offset(&lOffset);
            pSymbol->get_bitPosition(&dwBitPos);
            fwprintf( hFile, L"/*Offset : %u,BitPos :%u*/",lOffset,dwBitPos);
#endif
        }
        break;
    }
}

////////////////////////////////////////////////////////////
// Print the information details for a type symbol
//
// Basic types
const wchar_t * const rgBaseType[] = 
{
    L"NoType",             // btNoType = 0,
        L"void",                 // btVoid = 1,
        L"char",                 // btChar = 2,
        L"wchar_t",              // btWChar = 3,
        L"signed char",
        L"unsigned char",
        L"int",                  // btInt = 6,
        L"unsigned int",         // btUInt = 7,
        L"float",                // btFloat = 8,
        L"BCD",                // btBCD = 9,
        L"bool",                 // btBool = 10,
        L"short",
        L"unsigned short",
        L"long",                 // btLong = 13,
        L"unsigned long",        // btULong = 14,
        L"__int8",
        L"__int16",
        L"__int32",
        L"__int64",
        L"__int128",
        L"unsigned __int8",
        L"unsigned __int16",
        L"unsigned __int32",
        L"unsigned __int64",
        L"unsigned __int128",
        L"currency",           // btCurrency = 25,
        L"date",               // btDate = 26,
        L"VARIANT",              // btVariant = 27,
        L"complex",            // btComplex = 28,
        L"bit",                // btBit = 29,
        L"BSTR",                 // btBSTR = 30,
        L"HRESULT"              // btHresult = 31
};

void CUserTypesGenerator::PrintType(FILE* hFile ,IDiaSymbol* pSymbol)
{
    IDiaSymbol* pBaseType;
    DWORD dwTag;
    DWORD dwInfo;
    ULONGLONG ulLen;
    HRESULT hResult;

    if(pSymbol->get_symTag(&dwTag) != S_OK)
        return;

    pSymbol->get_length(&ulLen);
    switch(dwTag)
    {
    case SymTagUDT:
    case SymTagEnum:
        this->PrintName(hFile,pSymbol);
        {
            CUserTypesGenerator UserTypesGenerator(pSymbol,this->UserTypePath,this->pTypesGenerated);
            UserTypesGenerator.Generate();
        }
        break;
    case SymTagPointerType:
        hResult = pSymbol->get_type(&pBaseType);
        if ( FAILED(hResult) || (pBaseType == NULL) )
            break;

        this->PrintType(hFile, pBaseType);
        pBaseType->Release();
        fwprintf( hFile, L"*");
        break;
    case SymTagArrayType:
        hResult = pSymbol->get_type(&pBaseType);
        if ( FAILED(hResult) || (pBaseType == NULL) )
            break;
        {
            this->PrintType(hFile, pBaseType);

            DWORD dwCountElems;
            ULONGLONG ulLenArray;
            ULONGLONG ulLenElem;
            WCHAR ArrayCurrentDimensionSize[64];
            *ArrayCurrentDimensionSize=0;
            // use swprintf instead of fwprintf to put dimensions after name
            if(pSymbol->get_count(&dwCountElems) == S_OK)
            {
                swprintf (ArrayCurrentDimensionSize, L"[%u]", dwCountElems);
            }
            else if (
                (pSymbol->get_length(&ulLenArray) == S_OK)
                && (pBaseType->get_length(&ulLenElem) == S_OK)
                )
            {
                if(ulLenElem == 0)
                {
                    swprintf (ArrayCurrentDimensionSize,  L"[%u]", ulLenArray );
                }else
                {
                    swprintf (ArrayCurrentDimensionSize,  L"[%u]", ulLenArray/ulLenElem);
                }
            }
            _tcscat(this->CurrentTypeArrayDimensions,ArrayCurrentDimensionSize);
            pBaseType->Release();
        }
        break;
    case SymTagBaseType:
        if(pSymbol->get_baseType(&dwInfo) != S_OK)
            return;

        switch(dwInfo)
        {
        case btUInt :
        case btInt :
            switch(ulLen)
            {
            case 1:
                fwprintf( hFile, L"char");
                break;
            case 2:
                fwprintf( hFile, L"short");
                break;
            case 4:
                fwprintf( hFile, L"int");
                break;
            case 8:
                fwprintf( hFile, L"__int64");
                break;
            }
            dwInfo = 0xFFFFFFFF;
            break;
        case btFloat :
            switch(ulLen)
            {
            case 4:
                fwprintf( hFile, L"float");
                break;
            case 8:
                fwprintf( hFile, L"double");
                break;
            }
            dwInfo = 0xFFFFFFFF;
            break;
        }

        if(dwInfo == 0xFFFFFFFF)
        {
            break;
        }
        fwprintf( hFile, L"%s", rgBaseType[dwInfo]);
        break;
    case SymTagTypedef:
        this->PrintName(hFile,pSymbol);
        {
            CUserTypesGenerator UserTypesGenerator(pSymbol,this->UserTypePath,this->pTypesGenerated);
            UserTypesGenerator.Generate();
        }
        break;

#ifdef _DEBUG
    default:
        ::DebugBreak();
        break;
#endif

    }
}

////////////////////////////////////////////////////////////
// Print a string representing the type of a symbol
//
void CUserTypesGenerator::PrintSymbolType(FILE* hFile, IDiaSymbol* pSymbol)
{
    IDiaSymbol* pType;
    if(pSymbol->get_type(&pType) == S_OK)
    {
        this->PrintType(hFile,pType);
        pType->Release();
    }
    else
    {
        fwprintf( hFile, L"UNKNOWN ");
#ifdef _DEBUG
        if (::IsDebuggerPresent())
            ::DebugBreak();
#endif
    }
}

void CUserTypesGenerator::PrintTypeInDetail(FILE* hFile,IDiaSymbol* pSymbol)
{
    DWORD dwSymTag;

    if (pSymbol->get_symTag(&dwSymTag) != S_OK)
        return;

    switch(dwSymTag)
    {
    case SymTagData:
        // enum
        if (this->RootSymbolSymTag == SymTagEnum)
        {
            this->PrintDepth(hFile,1);
            // don't print symbol type "int" for enum
            this->PrintName(hFile,pSymbol);

            VARIANT v;
            // MSDN : get_value param MUST be initialized to VT_EMPTY
            v.vt=VT_EMPTY;
            if (pSymbol->get_value(&v)==S_OK)
            {
                switch (v.vt)
                {
                //    VT_UI1 An unsigned 1-byte character is stored in bVal. 
                //    VT_UI2 An unsigned 2-byte integer value is stored in uiVal. 
                //    VT_UI4 An unsigned 4-byte integer value is stored in ulVal. 
                //    VT_UI8 An unsigned 8-byte integer value is stored in ullVal. 
                //    VT_UINT An unsigned integer value is stored in uintVal. 
                //    VT_INT An integer value is stored in intVal. 
                //    VT_I1 A 1-byte character value is stored in cVal. 
                //    VT_I2 A 2-byte integer value is stored in iVal. 
                //    VT_I4 A 4-byte integer value is stored in lVal. 
                //    VT_I8 A 8-byte integer value is stored in llVal. 
                case VT_UI1:
                    fwprintf( hFile, L" = 0x%x",v.bVal);
                    break;
                case VT_UI2:
                    fwprintf( hFile, L" = 0x%x",v.uiVal);
                    break;
                case VT_UI4:
                    fwprintf( hFile, L" = 0x%x",v.ulVal);
                    break;
                case VT_UI8:
                    fwprintf( hFile, L" = 0x%I64x",v.ullVal);
                    break;
                case VT_INT:
                    fwprintf( hFile, L" = 0x%x",v.intVal);
                    break;
                case VT_UINT:
                    fwprintf( hFile, L" = 0x%x",v.uintVal);
                    break;
                case VT_I1:
                    fwprintf( hFile, L" = 0x%x",v.cVal);
                    break;
                case VT_I2:
                    fwprintf( hFile, L" = 0x%x",v.iVal);
                    break;
                case VT_I4:
                    fwprintf( hFile, L" = 0x%x",v.lVal);
                    break;
                case VT_I8:
                    fwprintf( hFile, L" = 0x%I64x",v.llVal);
                    break;

                //VT_NULL
                //VT_R4	
                //VT_R8	
                //VT_CY	
                //VT_DATE	
                //VT_BSTR	
                //VT_DISPATCH
                //VT_ERROR	
                //VT_BOOL	   
                //VT_VARIANT	
                //VT_UNKNOWN	
                //VT_DECIMAL	
                //VT_VOID	
                //VT_HRESULT	
                //VT_EMPTY
                //VT_PTR	    
                //VT_SAFEARRAY
                //VT_CARRAY	
                //VT_USERDEFINED
                //VT_LPSTR	
                //VT_LPWSTR	
                //VT_RECORD	
                //VT_INT_PTR	
                //VT_UINT_PTR	
                //VT_FILETIME	
                //VT_BLOB	
                //VT_STREAM	
                //VT_STORAGE	
                //VT_STREAMED_OBJECT
                //VT_STORED_OBJECT	
                //VT_BLOB_OBJECT
                //VT_CF	
                //VT_CLSID
                //VT_VERSIONED_STREAM
                //VT_BSTR_BLOB
                //VT_VECTOR	
                //VT_ARRAY	
                //VT_BYREF	
                //VT_RESERVED	
                //VT_ILLEGAL	
                //VT_ILLEGALMASKED
                //VT_TYPEMASK	

                }
            }
            fwprintf( hFile, L",\r\n"); // Notice even last enum value will have an ending ","
        }
        else if (this->RootSymbolSymTag == SymTagUDT)// struct or union
        {
            // for struct check if previous bit field is full
            // for union no check is required next type will be in the same place of the current type (ByteOffset and BitPosition are always == 0)
            if (this->RootSymbolSymUdtKind == UdtStruct) 
                this->EndPreviousBitFieldIfRequired(hFile,pSymbol);

            this->PrintDepth(hFile,1);
            this->PrintSymbolType(hFile,pSymbol);
            fwprintf( hFile, L" ");
            this->PrintName(hFile,pSymbol);

            // check struct bit field
            this->PrintBitField(hFile,pSymbol);

            // add end of type
            fwprintf( hFile, L";\r\n");
        }
        break;
    case SymTagTypedef:
    case SymTagEnum:
    case SymTagUDT:
        {
            CUserTypesGenerator UserTypesGenerator(pSymbol,this->UserTypePath,this->pTypesGenerated);
            UserTypesGenerator.Generate();
        }
        return;
        break;
    case SymTagPointerType:
        this->PrintDepth(hFile,1);
        this->PrintType(hFile,pSymbol);
        fwprintf( hFile, L"* ");
        this->PrintName(hFile,pSymbol);
        fwprintf( hFile, L";\r\n");
        break;
    case SymTagArrayType:
        this->PrintDepth(hFile,1);
        this->PrintSymbolType(hFile,pSymbol);
        this->PrintName(hFile,pSymbol);
        fwprintf( hFile, L";\r\n");
        break;
    }

}

void CUserTypesGenerator::PrintDepth(FILE* hFile,SIZE_T Depth)
{
    SIZE_T Cnt;
    for (Cnt = 0; Cnt<Depth ; Cnt++)
    {
        fwprintf( hFile, L"    ");
    }
}

typedef enum UdtKindIndex
{
    UdtKindIndex_none,
    UdtKindIndex_struct,
    UdtKindIndex_class,
    UdtKindIndex_union,
    UdtKindIndex_enum
};
const wchar_t * const rgUdtKind[] =
{
    L"",
    L"struct",
    L"class",
    L"union",
    L"enum"
};

BOOL CUserTypesGenerator::Generate()
{
    DWORD dwSymTag;
    DWORD dwSymUdtKind;
    HRESULT hResult;
    WCHAR* wszShortName;
    WCHAR* wszName = NULL;
    FILE* hFile = NULL;
    BOOL bSuccess = FALSE;
    IDiaSymbol* pSymbol = this->pRootSymbol;
    UdtKindIndex UdtIndex = UdtKindIndex_none;
    

    TCHAR UserTypeFile[MAX_PATH];
    TCHAR* pStr;

    // get type name
    if(pSymbol->get_name(&wszName) != S_OK)
        goto CleanUp;

    // get type type
    if(pSymbol->get_symTag(&dwSymTag) != S_OK)
        goto CleanUp;

    this->RootSymbolSymTag = dwSymTag;

    // take into account only enum union and struct
    switch(dwSymTag)
    {
    case SymTagTypedef:
        UdtIndex = UdtKindIndex_none;
        break;
    case SymTagEnum:
        UdtIndex = UdtKindIndex_enum;
        break;
    case SymTagUDT:
        // assume type is not a class
        hResult = pSymbol->get_udtKind(&dwSymUdtKind);
        if( hResult != S_OK)
            goto CleanUp;
        switch (dwSymUdtKind)
        {
        case UdtStruct:
            this->RootSymbolSymUdtKind = UdtStruct;
            UdtIndex = UdtKindIndex_struct;
            break;
        case UdtUnion:
            this->RootSymbolSymUdtKind = UdtUnion;
            UdtIndex = UdtKindIndex_union;
            break;
        default:
            goto CleanUp;
        }
        break;
    default:
        goto CleanUp;
    }

    if (this->pTypesGenerated->IsGenerated(wszName))
    {
        bSuccess = TRUE;
        goto CleanUp;
    }

    // trash __unnamed:: prefix
    if (wcsncmp(wszName,CUserTypesGenerator__unnamed_Prefix,wcslen(CUserTypesGenerator__unnamed_Prefix))==0)
    {
        wszShortName = &wszName[wcslen(CUserTypesGenerator__unnamed_Prefix)];
    }
    else
    {
        wszShortName = wszName;
    }
    // look for L"::__unnamed" -->  to replace with L"__unnamed_ID" 
    WCHAR* wp;
    WCHAR UnamedName[MAX_PATH];
    wp = wcsstr(wszShortName,CUserTypesGenerator__unnamed_Type);
    if (wp)
    {
        DWORD Id;
        if (pSymbol->get_symIndexId(&Id)==S_OK)
        {
            _stprintf(UnamedName,L"__unnamed_0x%X",Id);
            wszShortName = UnamedName;
        }
    }

    // forge type file name
    _tcscpy(UserTypeFile,this->UserTypePath);
#if (defined(UNICODE)||defined(_UNICODE))
    pStr = wszShortName;
#else
    CAnsiUnicodeConvert::UnicodeToAnsi(wszShortName,&pStr);
#endif
    TCHAR* ShortName = _tcsdup(pStr);
    // as file can't exist with ':' in name (that should be the case for type defined in class like Class::Type)
    // with have to replace ':' with '_'
    CUserTypesGenerator::ReplaceForbiddenFileChar(ShortName);

    _tcscat(UserTypeFile,ShortName);
    free(ShortName);
#if ((!defined(UNICODE)) && (!defined(_UNICODE)))
    free(pStr);
#endif    
    _tcscat(UserTypeFile,_T(".txt"));

    // assume directory exists (avoid _tfopen failure)
    CStdFileOperations::CreateDirectoryForFile(UserTypeFile);

    // create type file
    hFile = _tfopen(UserTypeFile,_T("wb"));

    if (hFile == NULL)
    {
        TCHAR Msg[2*MAX_PATH];
        _stprintf(Msg,_T("Error creating file %s"),UserTypeFile);
        ::MessageBox(NULL,Msg,_T("Error"),MB_OK|MB_ICONERROR);
        goto CleanUp;
    }

    // write the unicode little endian header (FFFE)
    BYTE pbUnicodeHeader[2]={0xFF,0xFE};
    fwrite(pbUnicodeHeader,1,2,hFile);

    this->pTypesGenerated->Add(wszShortName);

    fwprintf( hFile, L"typedef ");
    if (dwSymTag == SymTagTypedef)
    {
        this->PrintSymbolType(hFile,pSymbol);

        // print name
        fwprintf( hFile, L" %s;",wszShortName);
    }
    else
    {
        // print enum / union / struct
        fwprintf( hFile,rgUdtKind[UdtIndex]);

        // print name
        fwprintf( hFile, L" %s",wszShortName);

        fwprintf( hFile, L"\r\n{\r\n");

        // for each child
        IDiaEnumSymbols* pEnumChildren;
        IDiaSymbol* pChild;
        ULONG celt = 0;

        hResult = pSymbol->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren);
        if(hResult == S_OK)
        {
            for (;;)
            {
                hResult = pEnumChildren->Next(1, &pChild, &celt);
                if ((hResult != S_OK) || (celt != 1))
                    break;

                this->PrintTypeInDetail(hFile,pChild);

                // change PreviousSymbol 
                if (this->PreviousSymbol)
                    this->PreviousSymbol->Release();
                this->PreviousSymbol = pChild;
                // the following is useless : pChild will be released at next iteration
                // this->PreviousSymbol->AddRef();
                // pChild->Release();
            }
            if (this->PreviousSymbol)
                this->PreviousSymbol->Release();

            pEnumChildren->Release();
        }
        fwprintf( hFile, L"};");
    }

    bSuccess = TRUE;
CleanUp:
    if (wszName)
        ::SysFreeString(wszName);

    if (hFile)
        fclose(hFile);

    return bSuccess;
}
void CUserTypesGenerator::ReplaceForbiddenFileChar(TCHAR* Str)
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