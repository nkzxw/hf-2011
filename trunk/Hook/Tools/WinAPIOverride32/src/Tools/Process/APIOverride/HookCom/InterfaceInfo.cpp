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
// Object: manages COM interface info from IDispatch parsing
//-----------------------------------------------------------------------------
#include "interfaceinfo.h"

CInterfaceInfo::CInterfaceInfo()
{
    this->CommonConstructor();
}
CInterfaceInfo::CInterfaceInfo(IID* pIid,PBYTE* pVTBLAddress)
{
    this->CommonConstructor();

    if (IsBadReadPtr(pIid,sizeof(IID)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
    }
    else
        this->Iid=*pIid;

    if (IsBadReadPtr(pVTBLAddress,sizeof(PBYTE)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
    }
    else
        this->VTBLAddress=*pVTBLAddress;

    if (IsBadCodePtr((FARPROC)this->VTBLAddress))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        this->VTBLAddress=NULL;
    }
}
void CInterfaceInfo::CommonConstructor()
{
    this->Iid=IID_IUnknown;
    this->VTBLAddress=NULL;
    this->Name = NULL;

    this->pMethodInfoList=new CLinkListSimple();
    this->pPropertyInfoList=new CLinkListSimple();
}
CInterfaceInfo::~CInterfaceInfo()
{
    this->FreeMemory();
    delete this->pMethodInfoList;
    delete this->pPropertyInfoList;
}

//-----------------------------------------------------------------------------
// Name: FreeMemory
// Object: free all memory allocated
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CInterfaceInfo::FreeMemory()
{
    this->FreePropertiesMemory();
    this->FreeMethodsMemory();
    if (this->Name)
    {
        SysFreeString(this->Name);
        this->Name = NULL;
    }
}

//-----------------------------------------------------------------------------
// Name: FreeMethodsMemory
// Object: free memory allocated method parsing
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CInterfaceInfo::FreeMethodsMemory()
{
    if (!this->pMethodInfoList)
        return;

    CLinkListItem* pItem;
    CMethodInfo* pMethodInfo;

    // free each allocated CMethodInfo object
    this->pMethodInfoList->Lock();
    for (pItem=this->pMethodInfoList->Head;pItem;pItem=pItem->NextItem)
    {
        pMethodInfo=(CMethodInfo*) pItem->ItemData;
        if (pMethodInfo)
        {
            delete pMethodInfo;
            pItem->ItemData=NULL;
        }
    }
    this->pMethodInfoList->Unlock();

}

//-----------------------------------------------------------------------------
// Name: FreePropertiesMemory
// Object: free memory allocated properties parsing
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CInterfaceInfo::FreePropertiesMemory()
{
    if (!this->pPropertyInfoList)
        return;

    CLinkListItem* pItem;
    CPropertyInfo* pPropertyInfo;

    // free each allocated CMethodInfo object
    this->pPropertyInfoList->Lock();
    for (pItem=this->pPropertyInfoList->Head;pItem;pItem=pItem->NextItem)
    {
        pPropertyInfo=(CPropertyInfo*) pItem->ItemData;
        if (pPropertyInfo)
        {
            delete pPropertyInfo;
            pItem->ItemData=NULL;
        }
    }
    this->pPropertyInfoList->Unlock();
   
}

//-----------------------------------------------------------------------------
// Name: GetMethodInfoFromVTBLAddress
// Object: find CMethodInfo object associated to the VTBL address
// Parameters :
//     in  : PBYTE VTBLAddress
//     out : 
//     return : pointer to CMethodInfo object associated to the VTBL address on success
//              NULL on failure
//-----------------------------------------------------------------------------
CMethodInfo* CInterfaceInfo::GetMethodInfoFromVTBLAddress(PBYTE VTBLAddress)
{
    CLinkListItem* pItemMethod;
    CMethodInfo* pMethodInfo;

    // for each method
    this->pMethodInfoList->Lock();
    for (pItemMethod=this->pMethodInfoList->Head;pItemMethod!=NULL;pItemMethod=pItemMethod->NextItem)
    {
        pMethodInfo=(CMethodInfo*)pItemMethod->ItemData;
        if (IsBadReadPtr(pMethodInfo,sizeof(CMethodInfo)))
        {
#ifdef _DEBUG
            if (IsDebuggerPresent())// avoid to crash application if no debugger
                DebugBreak();
#endif
            continue;
        }
        // if vtbl address is the same
        if (VTBLAddress==pMethodInfo->VTBLAddress)
        {
            // unlock list
            this->pMethodInfoList->Unlock();
            // return address
            return pMethodInfo;
        }
    }
    this->pMethodInfoList->Unlock();

    return NULL;
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: parse interface using IDispatch
// Parameters :
//     in  : IUnknown* pObject : pointer to object interface
//           IDispatch* pDispatch : pointer to IDispatch interface
//           ITypeInfo* pTypeInfo
//           BOOL bParseMethods : TRUE to parse methods
//           BOOL bParseProperties : TRUE to parse properties
//     out : 
//     return : parsing result
//-----------------------------------------------------------------------------
HRESULT CInterfaceInfo::Parse(IUnknown* pObject,IDispatch* pDispatch,ITypeInfo* pTypeInfo,BOOL bParseMethods,BOOL bParseProperties)
{
    HRESULT hResult;
    TYPEATTR* pTypeAttr;
    FUNCDESC* pFuncDesc;
    VARDESC* pVarDesc;
    CMethodInfo* pMethodInfo;
    CPropertyInfo* pPropertyInfo;

    int cnt;

    // if nothing to do
    if ((bParseMethods && bParseProperties)==FALSE)
        return S_OK;

    // free previously allocated memory
    if (bParseProperties)
    {
        this->FreePropertiesMemory();
    }
    if (bParseMethods)
    {
        this->FreeMethodsMemory();
    }

    // check parameters
    if (IsBadReadPtr(pTypeInfo, sizeof(ITypeInfo)))
        return E_INVALIDARG;

    // get interface name
    hResult=pTypeInfo->GetDocumentation(-1,&this->Name,0,0,0);

    // get type Attr
    hResult = pTypeInfo->GetTypeAttr(&pTypeAttr);
    if( FAILED(hResult))
        return hResult;

    // fill iid field
    this->Iid = pTypeAttr->guid;

    // get pointer to interface of iid
    IUnknown* pObjectExposdByIDispatch=NULL;

    if (pObject)
    {
        hResult = CSecureIUnknown::QueryInterface(pObject,this->Iid,(void**)&pObjectExposdByIDispatch);
        if( FAILED(hResult))
            return hResult;
        if (!pObjectExposdByIDispatch)
            return E_FAIL;
    }


    /////////////////////////////////////////
    // Parse Methods
    /////////////////////////////////////////
    if (bParseMethods&&pTypeAttr->cFuncs)
    {
        for(cnt=0;cnt<pTypeAttr->cFuncs;cnt++)
        {
            // get it's description
            // Warning pTypeInfo->GetFuncDesc reported to crash on some dll (bad install or dll COM bad implementation)
            // no try catch is done here for better performance
            hResult = pTypeInfo->GetFuncDesc(cnt,&pFuncDesc);
            if( FAILED(hResult))
            {
                pTypeInfo->ReleaseTypeAttr(pTypeAttr);
                if (pObjectExposdByIDispatch)
                    CSecureIUnknown::Release(pObjectExposdByIDispatch);
                return hResult;
            }

            // create new CMethodInfo object
            pMethodInfo=new CMethodInfo();
            if(!pMethodInfo)
            {
                pTypeInfo->ReleaseFuncDesc(pFuncDesc);
                pTypeInfo->ReleaseTypeAttr(pTypeAttr);
                if (pObjectExposdByIDispatch)
                    CSecureIUnknown::Release(pObjectExposdByIDispatch);
                return E_OUTOFMEMORY;
            }

            hResult = pMethodInfo->Parse(pObjectExposdByIDispatch,pDispatch,pTypeInfo,pFuncDesc);
            if( SUCCEEDED(hResult))
            {
                // in case of parsing success store created object into list
                this->pMethodInfoList->AddItem(pMethodInfo);
            }
            else
                delete pMethodInfo;



            // release function description
            pTypeInfo->ReleaseFuncDesc(pFuncDesc);
        }
    }

    if (pObjectExposdByIDispatch)
        CSecureIUnknown::Release(pObjectExposdByIDispatch);

#ifdef _DEBUG
    OutputDebugString(_T("\r\n"));
#endif

    /////////////////////////////////////////
    // Parse Properties
    /////////////////////////////////////////

    if (bParseProperties&&pTypeAttr->cVars)
    {
        // for each var
        for(cnt=0;cnt<pTypeAttr->cVars;cnt++)
        {
            // get it's description
            hResult = pTypeInfo->GetVarDesc(cnt,&pVarDesc);
            if( FAILED(hResult))
            {
                pTypeInfo->ReleaseTypeAttr(pTypeAttr);
                return hResult;
            }

            // create new CPropertyInfo object
            pPropertyInfo=new CPropertyInfo();
            if(!pPropertyInfo)
            {
                pTypeInfo->ReleaseVarDesc(pVarDesc);
                pTypeInfo->ReleaseTypeAttr(pTypeAttr);
                return E_OUTOFMEMORY;
            }

            // store created object into list
            this->pPropertyInfoList->AddItem(pPropertyInfo);

            hResult = pPropertyInfo->Parse(pTypeInfo,pVarDesc);
            if( FAILED(hResult))
            {
                pTypeInfo->ReleaseVarDesc(pVarDesc);
                pTypeInfo->ReleaseTypeAttr(pTypeAttr);
                return hResult;
            }

            // release var description
            pTypeInfo->ReleaseVarDesc(pVarDesc);
        }

#ifdef _DEBUG
        OutputDebugString(_T("\r\n"));
#endif
/*
if( (pVarDesc->varkind == VAR_DISPATCH) && !(pVarDesc->wVarFlags&
    VARFLAG_FRESTRICTED) )
    INVOKE_PROPERTYGET
if (!pVarDesc->wVarFlags&VARFLAG_FREADONLY)
    INVOKE_PROPERTYPUT
*/
    }
    
    // release type attr 
    pTypeInfo->ReleaseTypeAttr(pTypeAttr);

    return( S_OK );
}