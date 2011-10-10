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
// Object: manages COM method info from IDispatch parsing
//-----------------------------------------------------------------------------

#include "methodinfo.h"


CMethodInfo::CMethodInfo()
{
    this->pItemAPIInfo=NULL;
    this->pParameterInfoList=new CLinkListSimple();
    this->pLinkListOfBaseInterfacesID=new CLinkList(sizeof(IID));

    this->Name=NULL;
    this->SetName(_T(""));

    this->VTBLIndex=0;
    this->VTBLAddress=(PBYTE)(-1);
    this->Address=0;
    this->AskedToBeNotLogged=FALSE;
    this->ResultOfIDispatchParsing=FALSE;
    this->memid=0;
    this->funckind=FUNC_VIRTUAL;
    this->invkind=INVOKE_FUNC;
    this->callconv=CC_STDCALL;
}

CMethodInfo::~CMethodInfo()
{
    this->FreeMemory();
    if (this->pLinkListOfBaseInterfacesID)
        delete this->pLinkListOfBaseInterfacesID;
}

//-----------------------------------------------------------------------------
// Name: SetListOfBaseInterfaces
// Object: set list of based interfaces
// Parameters :
//     in  : CLinkList* pLinkListOfBaseInterfacesID : list of based interfaces
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CMethodInfo::SetListOfBaseInterfaces(CLinkList* pLinkListOfBaseInterfacesID)
{
    if(IsBadReadPtr(pLinkListOfBaseInterfacesID,sizeof(CLinkList)))
        return FALSE;
    // keep only most derived informations
    if (pLinkListOfBaseInterfacesID->GetItemsCount()<this->pLinkListOfBaseInterfacesID->GetItemsCount())
        return TRUE;
    return CLinkList::Copy(this->pLinkListOfBaseInterfacesID,pLinkListOfBaseInterfacesID);
}

//-----------------------------------------------------------------------------
// Name: FreeMemory
// Object: free all memory allocated
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CMethodInfo::FreeMemory()
{
    this->FreeParametersMemory();

    if (this->Name)
    {
        SysFreeString(this->Name);
        this->Name=NULL;
    }
}

//-----------------------------------------------------------------------------
// Name: FreeParametersMemory
// Object: free memory allocated by parsing
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CMethodInfo::FreeParametersMemory()
{
    if(!this->pParameterInfoList)
        return;

    CParameterInfo* pParameterInfo;
    CLinkListItem* pItem;
    // for each parameter
    for (pItem=this->pParameterInfoList->Head;pItem;pItem=pItem->NextItem)
    {
        pParameterInfo=(CParameterInfo*)pItem->ItemData;
        if (pParameterInfo)
        {
            delete pParameterInfo;
            pItem->ItemData=NULL;
        }
    }
    delete this->pParameterInfoList;
    this->pParameterInfoList=NULL;
        
}

//-----------------------------------------------------------------------------
// Name: HasAnOutParameter
// Object: check if method has an out parameter
// Parameters :
//     in  : 
//     out : 
//     return : return TRUE if method has an out parameter
//-----------------------------------------------------------------------------
BOOL CMethodInfo::HasAnOutParameter()
{
    CParameterInfo* pParameterInfo;
    CLinkListItem* pItem;
    for (pItem=this->pParameterInfoList->Head;pItem;pItem=pItem->NextItem)
    {
        pParameterInfo=(CParameterInfo*)pItem->ItemData;
        if (pParameterInfo->IsOutParameter())
            return TRUE;
    }
    return FALSE;
}
//-----------------------------------------------------------------------------
// Name: GetStackSize
// Object: get method required stack size
// Parameters :
//     in  : 
//     out : 
//     return : required stack size
//-----------------------------------------------------------------------------
DWORD CMethodInfo::GetStackSize()
{
    DWORD StackSize=0;
    CParameterInfo* pParameterInfo;
    CLinkListItem* pItem;
    for (pItem=this->pParameterInfoList->Head;pItem;pItem=pItem->NextItem)
    {
        pParameterInfo=(CParameterInfo*)pItem->ItemData;
        StackSize+=pParameterInfo->GetStackSize();
    }
    return StackSize;
}

//-----------------------------------------------------------------------------
// Name: MustThisPointerBeAddedAsFirstParameter
// Object: check if Interface pointer must be added has first parameter
// Parameters :
//     in  : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CMethodInfo::MustThisPointerBeAddedAsFirstParameter()
{
    // FUNC_STATIC don't take this pointer as first parameter
    if (this->funckind==FUNC_STATIC)
        return FALSE;

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: CanBeHookedByVTBL
// Object: allow to know if function can be hooked using VTBL, or if we have to use the 
//          classical API way (modify function first bytes)
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if hook can be done by VTBL, FALSE if hook must be done using function first bytes
//-----------------------------------------------------------------------------
BOOL CMethodInfo::CanBeHookedByVTBL()
{
    switch(this->funckind)
    {
    case FUNC_NONVIRTUAL:
    case FUNC_STATIC:
        return FALSE;
    default:
        return TRUE;
    }
}

//-----------------------------------------------------------------------------
// Name: SetName
// Object: set method Name
// Parameters :
//     in  : TCHAR* pszName : new name
//     return : 
//-----------------------------------------------------------------------------
void CMethodInfo::SetName(TCHAR* pszName)
{
    // free already allocated name
    if (this->Name)
    {
        SysFreeString(this->Name);
        this->Name=NULL;
    }

#if (defined(UNICODE)||defined(_UNICODE))
    this->Name=SysAllocString(pszName);
#else
    WCHAR* pw;
    CAnsiUnicodeConvert::AnsiToUnicode(pszName,&pw);
    this->Name=SysAllocString(pw);
    free(pw);
#endif
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: parse method from IDispatch
// Parameters :
//     in  : IUnknown* pObject : object pointer can be NULL for type library (.tlb) parsing
//           IDispatch* pDispatch : object IDispatch interface pointer can be NULL for type library (.tlb) parsing
//           ITypeInfo* pTypeInfo : type info
//           const FUNCDESC* pFuncDesc : func descr
//     return : parsing success state
//-----------------------------------------------------------------------------
HRESULT CMethodInfo::Parse(IUnknown* pObject,IDispatch* pDispatch,ITypeInfo* pTypeInfo,const FUNCDESC* pFuncDesc)
{
    UNREFERENCED_PARAMETER(pDispatch);
    CParameterInfo* pParameterInfo;
    DWORD cntString;
    HRESULT hResult=S_OK;

    this->ResultOfIDispatchParsing=TRUE;
    this->FreeMemory();
    this->pParameterInfoList=new CLinkListSimple();
    if (!this->pParameterInfoList)
        return E_OUTOFMEMORY;

    BSTR* pbstrNames = (BSTR*)HeapAlloc(GetProcessHeap(),0,(1+pFuncDesc->cParams)*sizeof(BSTR));
    if (!pbstrNames)
        return E_OUTOFMEMORY;

    memset(pbstrNames,0,(1+pFuncDesc->cParams)*sizeof(BSTR));

    this->memid=pFuncDesc->memid;
    this->callconv=pFuncDesc->callconv;
    this->funckind=pFuncDesc->funckind;
    this->invkind=pFuncDesc->invkind;

    // find function address, and VTBL address if any
    switch(this->funckind)
    {
    // for static functions, we have to get address with pTypeInfo->AddressOfMember
    case FUNC_NONVIRTUAL:
    case FUNC_STATIC:
#ifdef _DEBUG
        // To debug only. I currently never fall in this case, so just to see what's happen :)
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        this->VTBLAddress=(PBYTE)(-1);
        pTypeInfo->AddressOfMember(this->memid,this->invkind,(PVOID*)&this->Address);
        break;
    // case FUNC_DISPATCH:
    // case FUNC_VIRTUAL:
    // case FUNC_PUREVIRTUAL:
    default:
        if ((pFuncDesc->oVft==0)&&(pFuncDesc->invkind!=INVOKE_FUNC))
        {
#ifdef _DEBUG
            // To debug only. I fall in this case when I was hooking IDispatchEx and crashing stack :)
            if (IsDebuggerPresent())// avoid to crash application if no debugger
                DebugBreak();
            OutputDebugString(_T("Not Hookable virtual property (Bad pFuncDesc->oVft)\r\n"));
#endif
            hResult=E_FAIL;
        }
        else
        {
            // WE MUST PATCH INTERFACE EXPOSED THROUGH IDISPATCH, NOT IDSIPATCH ITSELF
            // pObject and pDispatch can be equal, but it's not always the case, 
            // and if pObject!=pDispatch, the vtbl that needs to be patched is the pObject vtbl not the pDispatch one

            this->VTBLIndex=pFuncDesc->oVft/sizeof(PBYTE);

            if (pObject)
            {
                // we get address using the pObject Virtual Table informations
                // this->VTBLAddress=pObject->lpVtbl+pFuncDesc->oVft;
                this->VTBLAddress=(*(PBYTE*)pObject)+pFuncDesc->oVft;

                // in the case of pDispatch==pObject we can do
                //  // this->VTBLAddress=pDispatch->lpVtbl+pFuncDesc->oVft;
                //  // this->VTBLAddress=(*(PBYTE*)pDispatch)+pFuncDesc->oVft;
                
                if (IsBadReadPtr(this->VTBLAddress,sizeof(PBYTE)))
                {
#ifdef _DEBUG
                    if (IsDebuggerPresent())// avoid to crash application if no debugger
                        DebugBreak();
                    OutputDebugString(_T("Not Hookable virtual function / property (Bad VTBL Address)\r\n"));
#endif
                    this->VTBLAddress=(PBYTE)(-1);
                    this->Address=0;
                    hResult=E_FAIL;
                }
                else
                {
                    this->Address=*((PBYTE*)this->VTBLAddress);
                    if (IsBadCodePtr((FARPROC)this->Address))
                    {
#ifdef _DEBUG
                        if (IsDebuggerPresent())// avoid to crash application if no debugger
                            DebugBreak();
                        OutputDebugString(_T("Not Hookable virtual function / property (Bad Code Pointer)\r\n"));
#endif
                        hResult=E_FAIL;
                    }
                }
            }
        }
        break;
    }
    if( FAILED(hResult))
        return hResult;

    //Notice: pFuncDesc->cParams specifies the total number of required and optional parameters

    // retrieve method and parameters name
    UINT NbNames;
    
    hResult=pTypeInfo->GetNames(pFuncDesc->memid,pbstrNames,1+pFuncDesc->cParams,&NbNames);
    if(FAILED(hResult))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
        OutputDebugString(_T("Error retrieving method name\r\n"));
#endif
        this->SetName(_T(""));
        return hResult;
    }
    this->Name=pbstrNames[0];

#ifdef _DEBUG
    // display debug infos
    OutputDebugString(_T("\r\nMethod:"));
    if (this->Name)
        OutputDebugStringW(this->Name);

    if (!this->CanBeHookedByVTBL())
        OutputDebugString(_T(" Static"));


    TCHAR TmpPszAddress[100];
    _stprintf(TmpPszAddress,_T(" VTBL_Address: 0x%p") ,this->VTBLAddress);
    OutputDebugString(TmpPszAddress);
    _stprintf(TmpPszAddress,_T(" Function_Address: 0x%p") ,this->Address);
    OutputDebugString(TmpPszAddress);
#endif

    // return
    {
        // parse parameter object
        hResult=this->Return.Parse(pTypeInfo,L"",(ELEMDESC*)&pFuncDesc->elemdescFunc);

#ifdef _DEBUG
        OutputDebugString(_T("\r\nReturn:"));
        OutputDebugString(CSupportedParameters::GetParamName(this->Return.GetWinAPIOverrideType()));
        OutputDebugString(_T("\r\n"));
#endif
    }

    // for each parameter
    for (SHORT cnt=0;cnt<pFuncDesc->cParams;cnt++)
    {
        pParameterInfo=new CParameterInfo();

        this->pParameterInfoList->AddItem(pParameterInfo);

        // parse parameter object
        hResult=pParameterInfo->Parse(pTypeInfo,pbstrNames[cnt+1],&pFuncDesc->lprgelemdescParam[cnt]);
        if( FAILED(hResult))
            continue;

#ifdef _DEBUG
        OutputDebugString(_T("\r\nParameter:"));
        OutputDebugString(CSupportedParameters::GetParamName(pParameterInfo->GetWinAPIOverrideType()));
        OutputDebugString(_T(" "));
        OutputDebugStringW(pbstrNames[cnt+1]);
#endif
    }

#ifdef _DEBUG
    OutputDebugString(_T("\r\n"));
#endif

    // free strings but method name (at pos 0), 
    // as pParameterInfo->Parse allocate memory for internal string storing 
    for(cntString=1;cntString<(UINT)(1+pFuncDesc->cParams);cntString++)
    {
        if(pbstrNames[cntString])
            SysFreeString(pbstrNames[cntString]);
    }
    HeapFree(GetProcessHeap(),0,pbstrNames);

    // for INVOKE_PROPERTYGET and INVOKE_FUNC, return parameter can be marked as the function return
    // although they still return an HRESULT...
    // so take the return value type to ensure this is not the last parameter
    BOOL InsertReturnAsLastParameter=FALSE;
    switch (this->invkind)
    {
    case INVOKE_FUNC:
        // if return type is not void
        if ( (pFuncDesc->elemdescFunc.tdesc.vt!=VT_VOID)
            && (pFuncDesc->elemdescFunc.tdesc.vt!=VT_HRESULT) // needed ??? where is the m$ documentation ? :)
            && (pFuncDesc->wFuncFlags!=FUNCFLAG_FRESTRICTED)  // needed ??? where is the m$ documentation ? :)
            && ( pObject && (pFuncDesc->oVft/sizeof(PBYTE)>(7-1)) )// if IDispatch parsing and not for IDispatch functions (function 1 has a 0 offset and IDispatch has 7 methods)
            )
        {
            InsertReturnAsLastParameter=TRUE;
        }
        break;
    case INVOKE_PROPERTYGET:
        // if property has no parameters
        if (pFuncDesc->cParams==0)
        {
            InsertReturnAsLastParameter=TRUE;
        }
        else
        {
            // if last parameter of property has not the PARAMFLAG_FRETVAL set
            InsertReturnAsLastParameter=!((CParameterInfo*)this->pParameterInfoList->Tail->ItemData)->IsReturnedValue();
        }
        break;
    }

    if (InsertReturnAsLastParameter)
    {
        pParameterInfo=new CParameterInfo();

        // add return as parameter list
        this->pParameterInfoList->AddItem(pParameterInfo);

        hResult=pParameterInfo->MakeParameterFromReturn(pTypeInfo,(ELEMDESC*)&pFuncDesc->elemdescFunc);
#ifdef _DEBUG
        if( SUCCEEDED(hResult))
        {
            OutputDebugString(_T("\r\nReturned Parameter:"));
            OutputDebugString(CSupportedParameters::GetParamName(pParameterInfo->GetWinAPIOverrideType()));
        }
#endif
    }

    CLinkList LinkListOfBaseInterfaces(sizeof(IID));
    IID Iid=IID_IDispatch;
    LinkListOfBaseInterfaces.AddItem(&Iid);
    this->SetListOfBaseInterfaces(&LinkListOfBaseInterfaces);

    return hResult;
}

//-----------------------------------------------------------------------------
// Name: ToString
// Object: get a string representation of method and parameters
// Parameters :
//     in  : BOOL AddAdvancedInformation : TRUE to get advanced informations
//     out : TCHAR* pszFuncDesc : string representation of method and parameters
//                                must be large enough no checking is done (TCHAR pszFuncDesc[2048]; should be enough)
//     return : 
//-----------------------------------------------------------------------------
void CMethodInfo::ToString(IN BOOL AddAdvancedInformation,OUT TCHAR* pszFuncDesc)
{
    BOOL FirstParameter=TRUE;
    TCHAR pszParameter[MAX_PATH];

    CLinkListItem* pItemParameter;
    CParameterInfo* pParameterInfo;
    USHORT ParamFlag;
#if ((!defined(UNICODE))&&(!defined(_UNICODE)))
    size_t Size;
#endif

    *pszFuncDesc=0;

    /////////////////////////////////
    // forge method name and parameters
    /////////////////////////////////
    if (this->Name)
    {
#if (defined(UNICODE)||defined(_UNICODE))
        _tcscat(pszFuncDesc,this->Name);
#else
        CAnsiUnicodeConvert::UnicodeToAnsi(this->Name,&pszFuncDesc[_tcslen(pszFuncDesc)],MAX_PATH);
#endif
    }

    _tcscat(pszFuncDesc,_T("("));

    // if !static
    if(this->MustThisPointerBeAddedAsFirstParameter())
    {
        // add IUnknown* pObject pointer as first parameter
        _tcscat(pszFuncDesc,_T("IUnknown* pObject"));

        // force splitter to be shown for next param
        FirstParameter=FALSE;
    }

    // for each parameter of method
    this->pParameterInfoList->Lock();
    for(pItemParameter=this->pParameterInfoList->Head;pItemParameter;pItemParameter=pItemParameter->NextItem)
    {
        pParameterInfo=(CParameterInfo*)pItemParameter->ItemData;
        *pszParameter=0;
        if(FirstParameter)
        {
            FirstParameter=FALSE;
        }
        else
        {
            // add splitter
            _tcscat(pszFuncDesc,_T(", "));
        }
        if (AddAdvancedInformation && this->ResultOfIDispatchParsing)
        {
            ParamFlag=pParameterInfo->GetParamFlag();
            if (ParamFlag&PARAMFLAG_FIN)
                _tcscat(pszParameter,_T("[in] "));
            if (ParamFlag&PARAMFLAG_FOUT)
                _tcscat(pszParameter,_T("[out] "));
            if (ParamFlag&PARAMFLAG_FRETVAL)
                _tcscat(pszParameter,_T("[retval] "));
            if (ParamFlag&PARAMFLAG_FOPT)
                _tcscat(pszParameter,_T("[optional] "));
        }

        // get parameter type
        _tcscat(pszParameter,CSupportedParameters::GetParamName(pParameterInfo->GetWinAPIOverrideType()));

        if (pParameterInfo->Name)
        {
            _tcscat(pszParameter,_T(" "));
            // get property name
#if (defined(UNICODE)||defined(_UNICODE))
            _tcscat(pszParameter,pParameterInfo->Name);
#else
            Size=_tcslen(pszParameter);
            CAnsiUnicodeConvert::UnicodeToAnsi(pParameterInfo->Name,&pszParameter[Size],(DWORD)(MAX_PATH-Size));
#endif
        }
        _tcscat(pszFuncDesc,pszParameter);
    }
    this->pParameterInfoList->Unlock();

    _tcscat(pszFuncDesc,_T(")"));

    if (AddAdvancedInformation && this->ResultOfIDispatchParsing)
    {
        if (!this->MustThisPointerBeAddedAsFirstParameter())
            _tcscat(pszFuncDesc,_T(" [static]"));

        switch (this->invkind)
        {
        case INVOKE_PROPERTYGET:
            _tcscat(pszFuncDesc,_T(" [get]"));
            break;
        case INVOKE_PROPERTYPUT:
            _tcscat(pszFuncDesc,_T(" [put]"));
            break;
        case INVOKE_PROPERTYPUTREF:
            _tcscat(pszFuncDesc,_T(" [put ref]"));
            break;
        }

        switch(this->callconv)
        {
        case CC_FASTCALL:
            _tcscat(pszFuncDesc,_T(" FASTCALL"));
            break;
        case CC_CDECL:
            _tcscat(pszFuncDesc,_T(" CDECL"));
            break;
        // // CC_MSCPASCAL==CC_PASCAL
        //case CC_MSCPASCAL:
        //    _tcscat(pszFuncDesc,_T(" MSCPASCAL"));
        //    break;
        case CC_PASCAL:
            _tcscat(pszFuncDesc,_T(" PASCAL"));
            break;
        case CC_MACPASCAL:
            _tcscat(pszFuncDesc,_T(" MACPASCAL"));
            break;
        case CC_STDCALL:
            _tcscat(pszFuncDesc,_T(" STDCALL"));
            break;
        case CC_FPFASTCALL:
            _tcscat(pszFuncDesc,_T(" FPFASTCALL"));
            break;
        case CC_SYSCALL:
            _tcscat(pszFuncDesc,_T(" SYSCALL"));
            break;
        case CC_MPWCDECL:
            _tcscat(pszFuncDesc,_T(" MPWCDECL"));
            break;
        case CC_MPWPASCAL:
            _tcscat(pszFuncDesc,_T(" MPWPASCAL"));
            break;
        case CC_MAX:
            _tcscat(pszFuncDesc,_T(" MAX"));
            break;
        }
    }
}