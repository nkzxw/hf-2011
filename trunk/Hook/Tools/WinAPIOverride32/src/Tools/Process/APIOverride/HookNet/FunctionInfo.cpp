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
#include "FunctionInfo.h"

namespace NET
{

CFunctionInfo::CFunctionInfo(void)
{
    *this->szName=0;
    this->pReturnInfo=NULL;
    this->bIsStatic=FALSE;
    this->pParameterInfoList=new CLinkListSimple();
    this->CallingConvention=0;
    this->ParamCount=0;
    this->FunctionToken=0;

}

CFunctionInfo::~CFunctionInfo(void)
{
    CLinkListItem* pItem;
    CParameterInfo* pParamInfo;

    if (this->pReturnInfo)
    {
        delete this->pReturnInfo;
        this->pReturnInfo=NULL;
    }
    if (this->pParameterInfoList)
    {
        // for each parameter
        this->pParameterInfoList->Lock();
        for (pItem=this->pParameterInfoList->Head;pItem;pItem=pItem->NextItem)
        {
            pParamInfo=(CParameterInfo*)pItem->ItemData;
            delete pParamInfo;
        }
        this->pParameterInfoList->RemoveAllItems(TRUE);
        this->pParameterInfoList->Unlock();
        delete this->pParameterInfoList;
    }
}

// WARNING param count can be != param parsed successfully
// for param successfully parsed, use pListParametersInfos->GetItemsCount()
ULONG CFunctionInfo::GetParamCount()
{
    return this->ParamCount;
}

BOOL CFunctionInfo::IsStatic()
{
    return this->bIsStatic;
}

//-----------------------------------------------------------------------------
// Name: GetWinApiOverrideCallingConvention
// Object: translate Net calling convention enum to Winapioverride calling convention enum
// Parameters :
//     in : 
// Return : Winapioverride calling convention enum value
//-----------------------------------------------------------------------------
tagCALLING_CONVENTION CFunctionInfo::GetWinApiOverrideCallingConvention()
{
    switch (this->CallingConvention & IMAGE_CEE_CS_CALLCONV_MASK)
    {
    case IMAGE_CEE_UNMANAGED_CALLCONV_C:
        return CALLING_CONVENTION_CDECL;

    case IMAGE_CEE_UNMANAGED_CALLCONV_STDCALL:
        return CALLING_CONVENTION_STDCALL;

    case IMAGE_CEE_UNMANAGED_CALLCONV_THISCALL:
        return CALLING_CONVENTION_THISCALL;

    case IMAGE_CEE_UNMANAGED_CALLCONV_FASTCALL:
        return CALLING_CONVENTION_FASTCALL;

    case IMAGE_CEE_CS_CALLCONV_DEFAULT: // default is fastcall in .NET
    default:
        return CALLING_CONVENTION_FASTCALL;
    }
    //    IMAGE_CEE_CS_CALLCONV_DEFAULT   = 0x0,  
    //
    //    IMAGE_CEE_CS_CALLCONV_VARARG    = 0x5,  
    //    IMAGE_CEE_CS_CALLCONV_FIELD     = 0x6,  
    //    IMAGE_CEE_CS_CALLCONV_LOCAL_SIG = 0x7,
    //    IMAGE_CEE_CS_CALLCONV_PROPERTY  = 0x8,
    //    IMAGE_CEE_CS_CALLCONV_UNMGD     = 0x9,
    //    IMAGE_CEE_CS_CALLCONV_MAX       = 0x10,  // first invalid calling convention    

}

BOOL CFunctionInfo::GetName(OUT TCHAR* pszName,DWORD NameMaxLen)
{
    _tcsncpy(pszName,this->szName,NameMaxLen-1);
    pszName[NameMaxLen-1]=0;

    return TRUE;
}

ULONG CFunctionInfo::GetCallingConvention()
{
    return this->CallingConvention;
}

mdToken CFunctionInfo::GetToken()
{
    return this->FunctionToken;
}

BOOL CFunctionInfo::GetStringCallingConvention(OUT TCHAR* pszCallingConvention,DWORD MaxLen)
{
    *pszCallingConvention=0;
    static TCHAR* callConvNames[IMAGE_CEE_CS_CALLCONV_MAX] = 
    {	
        // use calling convention for winapioverride monitoring files
        _T(""), // _T("default "), 
        _T("__cdecl "), 
        _T("__stdcall "),	
        _T("__thiscall "),	
        _T("__fastcall "),	
        _T(""), //_T("vararg "),	 
        _T(""), //_T("field "),
        _T(""), //_T("local sig "),
        _T(""), //_T("property "),
        _T(""), //_T("unmanaged ")
    };	
    if (
        ((this->CallingConvention & IMAGE_CEE_CS_CALLCONV_MASK)!=0)// avoid to show default calling convention
        || ((this->CallingConvention & IMAGE_CEE_CS_CALLCONV_MASK)>IMAGE_CEE_CS_CALLCONV_MAX)
        )
    {
        
        _tcsncpy(pszCallingConvention,callConvNames[this->CallingConvention & IMAGE_CEE_CS_CALLCONV_MASK],MaxLen);
        pszCallingConvention[MaxLen-1]=0;
    }
    return TRUE;
}

// see "MetaData Unmanaged API.doc" file available inside 
// Program Files\Microsoft Visual Studio .NET 2003\SDK\v1.1\Tool Developers Guide\docs\ 
CFunctionInfo* CFunctionInfo::Parse(IMetaDataImport* pMetaDataImport,mdTypeDef ClassToken,mdToken FunctionToken)
{
    CFunctionInfo* pFunctionInfo;

    pFunctionInfo=new CFunctionInfo();
    if (!pFunctionInfo)
        return NULL;

    if (!pFunctionInfo->mParse(pMetaDataImport,ClassToken,FunctionToken))
    {
        delete pFunctionInfo;
        return NULL;
    }

    return pFunctionInfo;
}
BOOL CFunctionInfo::mParse(IMetaDataImport* pMetaDataImport,mdTypeDef ClassToken,mdToken FunctionToken)
{
    HRESULT hResult = S_OK;
    WCHAR ClassName[MAX_LENGTH]=L"";
    WCHAR FuncName[MAX_LENGTH]=L"";
    ULONG FuncNameSize=0;
    ULONG ClassNameSize=0;
    PCOR_SIGNATURE pSigFunc=NULL;
    DWORD MethodAttr = 0;
    ULONG cnt;
    CParameterInfo* pParameterInfo;

    this->FunctionToken=FunctionToken;

    // get method properties
    hResult = pMetaDataImport->GetMethodProps( FunctionToken,
                                                NULL,
                                                FuncName,
                                                MAX_LENGTH,
                                                &FuncNameSize,
                                                &MethodAttr,
                                                (PCCOR_SIGNATURE*)&pSigFunc,
                                                NULL,
                                                NULL, 
                                                NULL );
    if (FAILED(hResult))
        return FALSE;

    if (   (FuncNameSize>=(MAX_LENGTH-1))
        || (FuncNameSize==0)
        || (*FuncName<32) || (*FuncName>126) // assume readable name
        )
    {
        wsprintfW(FuncName,L"FuncToken%u",FunctionToken);
    }

    /////////////////////////////////////////////////////
    // forge function name with ClassName::FunctionName
    /////////////////////////////////////////////////////

    // if a class is specified
    if (( ClassToken != mdTypeDefNil )
        && ClassToken)
    {
        // get class name
        hResult = pMetaDataImport->GetTypeDefProps(ClassToken,
                                                    ClassName,
                                                    MAX_LENGTH,
                                                    &ClassNameSize,
                                                    NULL,
                                                    NULL); 
        ClassName[MAX_LENGTH-1]=0;
        if (FAILED(hResult)
            || (ClassNameSize>=(MAX_LENGTH/2))
            || (ClassNameSize==0)
            || (*ClassName<32) || (*ClassName>126) // assume readable name
            )
        {
            wsprintfW(ClassName,L"ClassToken%u",ClassToken);
        }
    }

    // add ::
    wcscat(ClassName,L"::");
    // add FunctionName
    wcsncat(ClassName,FuncName,MAX_LENGTH-wcslen(ClassName)-1);
    ClassName[MAX_LENGTH-1]=0;

#if (defined(UNICODE)||defined(_UNICODE))
    _tcsncpy(this->szName,ClassName,MAX_LENGTH);
#else
    wcstombs(this->szName,ClassName,MAX_LENGTH);
#endif
    this->szName[MAX_LENGTH-1]=0;

    /////////////////////////////////////////////////////
    // get method informations and parameters
    /////////////////////////////////////////////////////

    // Is the method static
    this->bIsStatic=IsMdStatic(MethodAttr);

    //func signature : 
    //1) calling convention
    //2) parameter count
    //3) returned type
    //4) parameter 1 type
    //   parameter 2 type
    //   ...
    //   parameter N type
    //
    //
    //typedef enum CorCallingConvention
    //{
    //    IMAGE_CEE_CS_CALLCONV_DEFAULT   = 0x0,  
    //
    //    IMAGE_CEE_CS_CALLCONV_VARARG    = 0x5,  
    //    IMAGE_CEE_CS_CALLCONV_FIELD     = 0x6,  
    //    IMAGE_CEE_CS_CALLCONV_LOCAL_SIG = 0x7,
    //    IMAGE_CEE_CS_CALLCONV_PROPERTY  = 0x8,
    //    IMAGE_CEE_CS_CALLCONV_UNMGD     = 0x9,
    //    IMAGE_CEE_CS_CALLCONV_MAX       = 0x10,  // first invalid calling convention    
    //
    //
    //        // The high bits of the calling convention convey additional info   
    //    IMAGE_CEE_CS_CALLCONV_MASK      = 0x0f,  // Calling convention is bottom 4 bits 
    //    IMAGE_CEE_CS_CALLCONV_HASTHIS   = 0x20,  // Top bit indicates a 'this' parameter    
    //    IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS = 0x40,  // This parameter is explicitly in the signature
    //} CorCallingConvention;
    //
    //
    //typedef enum CorUnmanagedCallingConvention
    //{
    //    IMAGE_CEE_UNMANAGED_CALLCONV_C         = 0x1,  
    //    IMAGE_CEE_UNMANAGED_CALLCONV_STDCALL   = 0x2,  
    //    IMAGE_CEE_UNMANAGED_CALLCONV_THISCALL  = 0x3,  
    //    IMAGE_CEE_UNMANAGED_CALLCONV_FASTCALL  = 0x4,  
    //
    //    IMAGE_CEE_CS_CALLCONV_C         = IMAGE_CEE_UNMANAGED_CALLCONV_C,  
    //    IMAGE_CEE_CS_CALLCONV_STDCALL   = IMAGE_CEE_UNMANAGED_CALLCONV_STDCALL,  
    //    IMAGE_CEE_CS_CALLCONV_THISCALL  = IMAGE_CEE_UNMANAGED_CALLCONV_THISCALL,  
    //    IMAGE_CEE_CS_CALLCONV_FASTCALL  = IMAGE_CEE_UNMANAGED_CALLCONV_FASTCALL,  
    //
    //} CorUnmanagedCallingConvention;


    // 1) calling convention
    pSigFunc += CorSigUncompressData(pSigFunc,&this->CallingConvention);
    if (this->CallingConvention!=IMAGE_CEE_CS_CALLCONV_FIELD)
    {
        // 2) get param count
        pSigFunc+=CorSigUncompressData(pSigFunc,&this->ParamCount);

        // 3) Get return type
        this->pReturnInfo=CParameterInfo::Parse(pMetaDataImport,pSigFunc,&pSigFunc);
        // check function return
        if (!this->pReturnInfo)
            return FALSE;

        // 4) get parameters

        for (cnt=0;cnt<this->ParamCount;cnt++)
        {
            pParameterInfo=CParameterInfo::Parse(pMetaDataImport,pSigFunc,&pSigFunc);
            // check function return
            if (!pParameterInfo)
                return FALSE;

            this->pParameterInfoList->AddItem(pParameterInfo);
        }	
    }
    else // IMAGE_CEE_CS_CALLCONV_FIELD
    {
        this->ParamCount=0;
        // Get the return type
        this->pReturnInfo=CParameterInfo::Parse(pMetaDataImport,pSigFunc,&pSigFunc);
        // check function return
        if (!this->pReturnInfo)
            return FALSE;
    }

    // if function has this and a not explicit this, add "this" parameter
    if (// (!pFunctionInfo->bIsStatic) &&
        ((this->CallingConvention & IMAGE_CEE_CS_CALLCONV_HASTHIS) != 0)
        && ((this->CallingConvention & IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS) == 0)
        )
    {
        this->ParamCount++;

        pParameterInfo=new CParameterInfo();
        if (!pParameterInfo)
            return FALSE;
        _tcscpy(pParameterInfo->szName,_T("pObject"));
        pParameterInfo->PointedSize=0;
        pParameterInfo->StackSize=sizeof(PBYTE);
        pParameterInfo->WinAPIOverrideType=PARAM_POINTER;
        // insert as first param
        this->pParameterInfoList->InsertItem(NULL,pParameterInfo);
    }

    return TRUE;
}

}