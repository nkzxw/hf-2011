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
// Object: manage informations on a hooked class
//-----------------------------------------------------------------------------
#include "hookedclass.h"

extern DWORD dwSystemPageSize;
extern HOOK_COM_OPTIONS HookComOptions;
extern HOOK_COM_INIT HookComInfos;
extern HINSTANCE DllhInstance;
extern TCHAR pszMonitoringFilesPath[];
extern CLinkList* pLinkListMonitoringFileInfo;
extern CCOMCreationPostApiCallHooks* pCOMCreationPostApiCallHooks;
extern BOOL bCOMAutoHookingEnabled;
// "342D1EA0-AE25-11D1-89C5-006008C3FBFC"
const IID IID_IClassFactoryEx = {0x342D1EA0,0xAE25,0x11D1,{0x89,0xC5,0x00,0x60,0x08,0xC3,0xFB,0xFC}};
// "A6EF9860-C720-11d0-9337-00A0C90DCAA9"
const IID IID_IDispatchEx = {0xA6EF9860,0xC720,0x11D0,{0x93,0x37,0x00,0xA0,0xC9,0x0D,0xCA,0xA9}};

MONITORING_FILE_INFOS HookComDll_IDispatchMonitoringFileInfo={_T("HookCom.dll IDispatch parsing")};

CHookedClass::CHookedClass(CLSID* pClsid)
{
    this->Clsid=*pClsid;
    this->pLinkListHookedInterfaces=new CLinkListSimple();
    this->pLinkListHookedObjects=new CLinkListSimple();

    this->HookedInterfacesHeap=HeapCreate(0,dwSystemPageSize,0);
    this->HookedObjectsHeap=HeapCreate(0,dwSystemPageSize,0);

    if (this->HookedInterfacesHeap)
        this->pLinkListHookedInterfaces->SetHeap(this->HookedInterfacesHeap);
    if (this->HookedObjectsHeap)
        this->pLinkListHookedObjects->SetHeap(this->HookedObjectsHeap);
    
    this->NbReleaseProcessing=0;
    this->bSupportIDispatchEx=FALSE;
    this->DllType=DLL_TYPE_UNKNOWN;
    
    this->AssociatedModuleName[0]=0;
    this->Initialize();
}         

//-----------------------------------------------------------------------------
// Name: ReInitialize
// Object: Unhook, and set values that need it to default values
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CHookedClass::ReInitialize()
{
    // unhook
    this->Unhook();

    // reset values
    this->Initialize();
}

//-----------------------------------------------------------------------------
// Name: Initialize
// Object: set values that need it to default values
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CHookedClass::Initialize()
{
    this->pInterfaceExposedByIDispatch=NULL;
    this->bHookedByAutoMonitoring=FALSE;
    this->bIDispatchParsingSuccessFull=FALSE;
    this->bIDispatchParsingHasBeenTried=FALSE;
    this->IUnknownVTBLAddress=0;
    this->AssociatedModuleBaseAddress=0;
}

CHookedClass::~CHookedClass(void)
{
    // unhook
    this->Unhook();

    delete this->pLinkListHookedInterfaces;
    delete this->pLinkListHookedObjects;

    HeapDestroy(this->HookedInterfacesHeap);
    HeapDestroy(this->HookedObjectsHeap);
}

//-----------------------------------------------------------------------------
// Name: GetHookedObject
// Object: find CHookedObject associated to object pointer
// Parameters :
//     in  : IUnknown* pIUnknown : COM object pointer
//     out : BOOL* pIsGoingToBeDestroyed : destroy state
//     return : a locked CHookedObject object pointer if found, NULL if not found
//-----------------------------------------------------------------------------
CHookedObject* CHookedClass::GetHookedObject(IUnknown* pIUnknown,BOOL* pIsGoingToBeDestroyed,BOOL* pbLocked)
{
    CHookedObject* pMatchingHookedObject=NULL;
    CHookedObject* pHookedObject;
    CLinkListItem* pItem;
    this->pLinkListHookedObjects->Lock();
    // for each hooked object
    for(pItem=this->pLinkListHookedObjects->Head;pItem;pItem=pItem->NextItem)
    {
        pHookedObject=(CHookedObject*)pItem->ItemData;
        // check if IUnknown pointer is already known
        if (pHookedObject->DoesPointerBelongToObject(pIUnknown))
        {
            // assume we wont return an object being destroyed
            if (!pHookedObject->IsGoingToBeDestroyed)
            {
                *pbLocked=pHookedObject->Lock(pIsGoingToBeDestroyed);
                pMatchingHookedObject=pHookedObject;
            }
            break;
        }
    }
    this->pLinkListHookedObjects->Unlock();
    return pMatchingHookedObject;
}

//-----------------------------------------------------------------------------
// Name: ReleaseCallBack
// Object: called when IUnknown::Release was called (object oriented) 
//          AND RESULT OF THIS FUNCTION IS 0 (--> object is destroyed)
// Parameters :
//     in  : 
//     out : 
//     return : TRUE
//-----------------------------------------------------------------------------
BOOL CHookedClass::ReleaseCallBack(IUnknown* pIUnknown)
{
    BOOL bFound;
    BOOL bLocked=FALSE;

    // get object pointer from IUnknown
    CHookedObject* pHookedObject=NULL;
    CLinkListItem* pItem;
    CLinkListItem* pNextItem;
    this->pLinkListHookedObjects->Lock(FALSE);
    bFound=FALSE;
    // for each hooked object
    for(pItem=this->pLinkListHookedObjects->Head;pItem;pItem=pNextItem)
    {
        pNextItem=pItem->NextItem;

        pHookedObject=(CHookedObject*)pItem->ItemData;
        // if pointer belongs to hooked object
        if (pHookedObject->DoesPointerBelongToObject(pIUnknown))
        {
            BOOL IsGoingToBeDestroyed;
            
            bLocked=pHookedObject->Lock(&IsGoingToBeDestroyed);
            pHookedObject->IsGoingToBeDestroyed=TRUE;

            bFound=TRUE;
            // remove object from list
            this->pLinkListHookedObjects->RemoveItem(pItem,TRUE);

            // pHookedObject contains informations on the destroyed hooked object,
            // so don't delete it now

            break;
        }
    }
    this->pLinkListHookedObjects->Unlock();

    if (!bFound)
    {
        // object may not belong to this class
        // It can appear that objects with different CLSID share part of same VTBL,
        // so multiple post hooks will be installed for the same function, 
        // that means each time function is called, it will be raised in all CHookedClass sharing same VTBL address,
        // and this call back can be called by object having a different CLSID
        return TRUE;
    }

    // else 
    // pHookedObject contains pointer to matching CHookedObject, and pHookedObject memory is not destroyed

    // if we were query to report COM object life
    if (HookComOptions.ReportHookedCOMObject)
    {
        pHookedObject->pLinkListInterfacePointers->Lock();

        // report message
        TCHAR* pszMsg=new TCHAR[STRING_PROGID_MAX_SIZE+100
                                //+ (nb items)*needed TCHAR
                                +(pHookedObject->pLinkListInterfacePointers->GetItemsCount())*20+10 
                                ];

        BOOL bReportMessage;
        bReportMessage=FALSE;

        if (HookComOptions.ReportUseNameInsteadOfIDIfPossible)
        {
            TCHAR _tProgId[STRING_PROGID_MAX_SIZE];
            bReportMessage=CGUIDStringConvert::GetClassName(&this->Clsid,_tProgId,STRING_PROGID_MAX_SIZE);
            if (bReportMessage)
                _stprintf(pszMsg,_T("COM: Object 0x%p of type %s has been destroyed"),pHookedObject->pObject,_tProgId);
        }
        // if progId retrieval failure or !ReportUseProgIdInsteadOfCLSIIfPossible
        if (!bReportMessage)
        {
            TCHAR _tClsid[STRING_GUID_SIZE];
            bReportMessage=CGUIDStringConvert::TcharFromCLSID(&this->Clsid,_tClsid);
            if (bReportMessage)
                _stprintf(pszMsg,_T("COM: Object 0x%p with CLSID %s has been destroyed"),pHookedObject->pObject,_tClsid);
        }
        if(bReportMessage)
        {
            BOOL bOtherObjectRefAdded=FALSE;
            BOOL bPointerAlreadyDisplayed;
            TCHAR psz[20];

            CLinkListItem* pItem;
            CLinkListItem* pPreviousItem;
            CHookedObject::OBJECT_INFO_INTERFACE_POINTER* pHookedInterfacePointer;
            CHookedObject::OBJECT_INFO_INTERFACE_POINTER* pPreviousHookedInterfacePointer;
            for(pItem=pHookedObject->pLinkListInterfacePointers->Head;pItem;pItem=pItem->NextItem)
            {
                // get current interface pointer
                pHookedInterfacePointer=(CHookedObject::OBJECT_INFO_INTERFACE_POINTER*)pItem->ItemData;

                // if you don't want object address to be displayed twice, uncomment the following lines
                // personally I prefer to have all pointer in a same list
                // // if pointer is already the displayed one
                // if (pHookedInterfacePointer->pInterface==pHookedObject->pObject)
                //     // go to next interface pointer
                //     continue;

                bPointerAlreadyDisplayed=FALSE;
                // a same pointer can belong to multiple interface, we have to check we add a pointer only once
                // by looping the list of pointer backward
                for(pPreviousItem=pItem->PreviousItem;pPreviousItem;pPreviousItem=pPreviousItem->PreviousItem)
                {
                    pPreviousHookedInterfacePointer=(CHookedObject::OBJECT_INFO_INTERFACE_POINTER*)pPreviousItem->ItemData;
                    if (pPreviousHookedInterfacePointer->pInterface==pHookedInterfacePointer->pInterface)
                    {
                        bPointerAlreadyDisplayed=TRUE;
                        break;
                    }
                }
                // if pointer has already been reported
                if (bPointerAlreadyDisplayed)
                    // go to next interface pointer
                    continue;

                // else we have to add it to list
                if (bOtherObjectRefAdded)
                    // continue string representation
                    _tcscat(pszMsg,_T(", "));
                else
                {
                    // open string representation
                    _tcscat(pszMsg,_T(" (Ref: "));

                    // set bOtherObjectRefAdded flag
                    bOtherObjectRefAdded=TRUE;
                }

                // add pointer to representation
                _stprintf(psz,_T("0x%p") ,pHookedInterfacePointer->pInterface);
                _tcscat(pszMsg,psz);
            }
            // if another object pointer has been reported
            if (bOtherObjectRefAdded)
                // close string representation
                _tcscat(pszMsg,_T(")"));

            // Notice: don't unlock pHookedObject->pLinkListInterfacePointers as
            // we are going to destroy pHookedObject and so this list

            // report message
            HookComInfos.ReportMessage(REPORT_MESSAGE_INFORMATION,pszMsg);
        }
        pHookedObject->pLinkListInterfacePointers->Unlock();
    }

    // delete pHookedObject only now
    if (bLocked)
        pHookedObject->Unlock();
    delete pHookedObject;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: QueryInterfaceCallBack
// Object: called when IUnknown::QueryInterface was called (object oriented)
//          install hooks for specified IID
// Parameters :
//     in  : 
//     out : 
//     return : results of Interface auto hooking
//-----------------------------------------------------------------------------
BOOL CHookedClass::QueryInterfaceCallBack(IUnknown* pIUnknown,IUnknown* pInterface,IID* pIid)
{
    // never hook interface other that IUnknown if class support IDispatchEx
    if (this->bSupportIDispatchEx)
        return FALSE;

    // find CHookedObject object associated with pIUnknown
    // Notice : object may not belong to this class
    // It can appear that objects with different CLSID share part of same VTBL,
    // so multiple post hooks will be installed for the same function, 
    // that means each time function is called, it will be raised in all CHookedClass sharing same VTBL address,
    // and this call back can be called by object having a different CLSID
    BOOL IsGoingToBeDestroyed=FALSE;
    BOOL Locked=FALSE;
    CHookedObject* pHookedObject=this->GetHookedObject(pIUnknown,&IsGoingToBeDestroyed,&Locked);
    if(!pHookedObject)
        return FALSE;
    if (IsGoingToBeDestroyed || !Locked)
        return FALSE;

    // WARNING to avoid deadlock make sure that all functions
    // called before pHookedObject->Unlock();
    // don't do a direct or indirect QueryInterface call on hooked interface of current object

    // assume object won't be released during current operations
    if (CSecureIUnknown::AddRef(pInterface)==0)
    {
        if (Locked)
            pHookedObject->Unlock();
        return FALSE;
    }

    // store pointer to interface to identify object
    pHookedObject->AddInterfacePointer(pInterface,pIid);

    // if interface is already hooked 
    if (this->IsInterfaceHookedByAutoMonitoring(pInterface,pIid))
    {
        if (Locked)
            pHookedObject->Unlock();
        CSecureIUnknown::Release(pInterface);
        // do nothing
        return TRUE;
    }

    // add post hooks on query interface and Release if they are not the same
    this->AddIUnknownPostHooks(pInterface,pIid);

    // add monitoring for specified interface
    this->SetMonitoringHookFromFile(pHookedObject,pInterface,pIid,pIid,NULL,TRUE,NULL,NULL);

    if (Locked)
        pHookedObject->Unlock();

    CSecureIUnknown::Release(pInterface);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetNumberOfHookedObjects
// Object: get number of hooked objects in this->pLinkListHookedObjects
// Parameters :
//     in  : 
//     out : 
//     return : number of hooked objects in this->pLinkListHookedObjects
//-----------------------------------------------------------------------------
DWORD CHookedClass::GetNumberOfHookedObjects()
{
    return this->pLinkListHookedObjects->GetItemsCount();
}
//-----------------------------------------------------------------------------
// Name: RemoveObject
// Object: remove an hooked object from this->pLinkListHookedObjects
//         warning : hooks won't be removed for this object
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if object has been removed, FALSE else
//-----------------------------------------------------------------------------
BOOL CHookedClass::RemoveObject(IUnknown* pObject)
{
    // get object pointer from IUnknown
    CHookedObject* pHookedObject;
    CLinkListItem* pItem;
    CLinkListItem* pNextItem;
    this->pLinkListHookedObjects->Lock();

    // for each hooked object
    for(pItem=this->pLinkListHookedObjects->Head;pItem;pItem=pNextItem)
    {
        pNextItem=pItem->NextItem;
        pHookedObject=(CHookedObject*)pItem->ItemData;
        // if IUnknown pointer is the same 
        // Notice : microsoft specifications say that QuerInterface IID_IUnknown should always return same pointer,
        // to quickly check object, but it's not the always the case (including m$ components... )
        pHookedObject->DoesPointerBelongToObject(pObject);
        {
            // remove object from list
            this->pLinkListHookedObjects->RemoveItem(pItem,TRUE);

            this->pLinkListHookedObjects->Unlock();
            return TRUE;
        }

    }
    this->pLinkListHookedObjects->Unlock();
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: GetAssociatedModuleBaseAddress
// Object: return dll HMODULE associated to the class
// Parameters :
//     in  : 
//     out : 
//     return : dll HMODULE associated to the class
//-----------------------------------------------------------------------------
HMODULE CHookedClass::GetAssociatedModuleBaseAddress()
{
    return this->AssociatedModuleBaseAddress;
}

//-----------------------------------------------------------------------------
// Name: FindAssociatedDllName
// Object: find dll name associated to class
// Parameters :
//     in  : 
//     out : 
//     return : TRUE dll name has been found, FALSE else
//-----------------------------------------------------------------------------
BOOL CHookedClass::FindAssociatedDllName(CHookedObject* pHookedObject)
{
    PBYTE VtblAddress;
    BOOL bRet=FALSE;

    // pHookedObject->pIUnknown or pHookedObject->pObject  vtbl address can give information on module owning object

    // try pHookedObject->pObject vtbl address
    if (!IsBadReadPtr(pHookedObject->pObject,sizeof(PBYTE)))
    {
        VtblAddress=*(PBYTE*)pHookedObject->pObject;
        bRet= GetModuleName(VtblAddress,this->AssociatedModuleName,MAX_PATH,&this->AssociatedModuleBaseAddress);
    }

    // if not found by pHookedObject->pObject vtbl address
    if (!bRet)
    {
        if (pHookedObject->pObject!=pHookedObject->pIUnknown)
        {
            // try pHookedObject->pIUnknown vtbl address
            if (!IsBadReadPtr(pHookedObject->pIUnknown,sizeof(PBYTE)))
            {
                VtblAddress=*(PBYTE*)pHookedObject->pIUnknown;
                bRet= GetModuleName(VtblAddress,this->AssociatedModuleName,MAX_PATH,&this->AssociatedModuleBaseAddress);
            }
        }
    }

    if (!bRet)
    {
        // try to dereference once again (can occurs in some cases)
        if (!IsBadReadPtr(pHookedObject->pObject,sizeof(PBYTE)))
        {
            if (!IsBadReadPtr(*(PBYTE*)pHookedObject->pObject,sizeof(PBYTE)))
            {
                VtblAddress=**(PBYTE**)pHookedObject->pObject;
                bRet= GetModuleName(VtblAddress,this->AssociatedModuleName,MAX_PATH,&this->AssociatedModuleBaseAddress);
            }
        }
    }

    if (!bRet)
    {
        // try to dereference once again (can occurs in some cases)
        if (pHookedObject->pObject!=pHookedObject->pIUnknown)
        {
            // try pHookedObject->pIUnknown vtbl address
            if (!IsBadReadPtr(pHookedObject->pIUnknown,sizeof(PBYTE)))
            {
                if (!IsBadReadPtr(*(PBYTE*)pHookedObject->pIUnknown,sizeof(PBYTE)))
                {
                    VtblAddress=**(PBYTE**)pHookedObject->pIUnknown;
                    bRet= GetModuleName(VtblAddress,this->AssociatedModuleName,MAX_PATH,&this->AssociatedModuleBaseAddress);
                }
            }
        }
    }

    return bRet;
}
//-----------------------------------------------------------------------------
// Name: IsAssociatedDllLoaded
// Object: check that dll associated to class is still loaded
// Parameters :
//     in  : 
//     out : 
//     return : TRUE dll is still in memory, FALSE else
//-----------------------------------------------------------------------------
BOOL CHookedClass::IsAssociatedDllLoaded()
{
    return (GetModuleHandle(this->AssociatedModuleName)!=NULL);
}

//-----------------------------------------------------------------------------
// Name: IsAssociatedDllInSameSpaceAddress
// Object: check that dll associated to class, has is still loaded at the same address
//          if dll has been unloaded and next reloaded, that could be at a different memory address
//          (slowest than IsAssociatedDllInSameSpaceAddress(CHookedObject* pHookedObject))
// Parameters :
//     in  : 
//     out : 
//     return : TRUE dll is still at the same address, FALSE else
//-----------------------------------------------------------------------------
BOOL CHookedClass::IsAssociatedDllInSameSpaceAddress()
{
    return (GetModuleHandle(this->AssociatedModuleName)==this->AssociatedModuleBaseAddress);
}

//-----------------------------------------------------------------------------
// Name: AreHooksStillInstalled
// Object: check that hooks are still installed for this class
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if hooks are still installed, FALSE else
//-----------------------------------------------------------------------------
BOOL CHookedClass::AreHooksStillInstalled()
{
    CLinkListItem* pItemInterface;
    CHookedInterface* pInterfaceInfo;
    CLinkListItem* pItemMethod;
    CMethodInfo* pMethodInfo;
    API_INFO* pApiInfo;
    BOOL bHooksStillInstalled=FALSE;
    // for each hooked interface
    this->pLinkListHookedInterfaces->Lock();
    for (pItemInterface=this->pLinkListHookedInterfaces->Head;pItemInterface;pItemInterface=pItemInterface->NextItem)
    {
        pInterfaceInfo=(CHookedInterface*)pItemInterface->ItemData;

        pInterfaceInfo->pMethodInfoList->Lock();
        for (pItemMethod=pInterfaceInfo->pMethodInfoList->Head;pItemMethod;pItemMethod=pItemMethod->NextItem)
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
            if (IsBadReadPtr(pMethodInfo->pItemAPIInfo,sizeof(CLinkListItem)))
                continue;

            pApiInfo=(API_INFO*)pMethodInfo->pItemAPIInfo->ItemData;
            if (IsBadReadPtr(pApiInfo,sizeof(API_INFO)))
                continue;

            // check hook opcode 

            // check if hook is in use
            if (pApiInfo->UseCount>0)
            {
                bHooksStillInstalled=TRUE;
            }
            else // check if hook is installed by comparing memory at pApiInfo->APIAddress with 
                    // our hook opcodes pApiInfo->pbHookCodes
            {
                // bOriginalOpcodes is set, that means hook is going to be restored --> don't require new installation
                if (pApiInfo->bOriginalOpcodes)
                    continue;
                if (IsBadReadPtr(pApiInfo->APIAddress,sizeof(PBYTE)))
                    bHooksStillInstalled=FALSE;
                else
                    bHooksStillInstalled=(memcmp(pApiInfo->APIAddress,pApiInfo->pbHookCodes,pApiInfo->OpcodeReplacementSize)==0);
            }
            pInterfaceInfo->pMethodInfoList->Unlock();
            this->pLinkListHookedInterfaces->Unlock();

            return bHooksStillInstalled;
        }
        pInterfaceInfo->pMethodInfoList->Unlock();
    }
    this->pLinkListHookedInterfaces->Unlock();
    // no hooks --> they are installed
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CheckAndRestoreClassHooks
// Object: check that class hooks are still installed, and if it's not the case,
//          re install them
//         MUST BE CALLED AT THE BEGINNING OF ADD OBJECT, BEFORE OBJECT IS INCLUDED TO LIST
// Parameters :
//     in  : 
//     out : 
//     return : TRUE
//-----------------------------------------------------------------------------
BOOL CHookedClass::CheckAndRestoreClassHooks(CHookedObject* pHookedObject)
{
    // if class has never been hooked
    if (this->IUnknownVTBLAddress==0)
    {
        this->IUnknownVTBLAddress=*(PBYTE*)pHookedObject->pIUnknown;
        this->AssociatedModuleBaseAddress=GetModuleHandle(this->AssociatedModuleName);
        return TRUE;
    }

    // if an hooked object has been created internally by our dll, we have assume it is not released,
    // so matching CLSID dll hasn't been unloaded 
    if (this->GetInternallyCreatedObject())
        return TRUE;
    
    if (!this->IsAssociatedDllInSameSpaceAddress())
    // module address memory space has changed
    // we have to Unhook all functions (vtbl addresses are at different memory place)
    {
        this->ReInitialize();

        // store new IUnknownVTBLAddress value and module handle (only after unhooking)
        this->IUnknownVTBLAddress=*(PBYTE*)pHookedObject->pIUnknown;
        this->AssociatedModuleBaseAddress=GetModuleHandle(this->AssociatedModuleName);
        return TRUE;
    }

    // else we have to check hooks to see if they are still installed
    // this is not the case if dll has been unloaded and next reloaded
    if (this->AreHooksStillInstalled())
        return TRUE;

    // hooks are no more installed : --> dll has been unloaded and reloaded
    // all previous hooked object were forced to be unloaded by func like CoUninitialize
    // --> free hooked objects and report their bad release
    this->FreeAllHookedObjectsAndReportNotReleased();

    CLinkListItem* pItemInterface;
    CHookedInterface* pInterfaceInfo;
    CLinkListItem* pItemMethod;
    CMethodInfo* pMethodInfo;
    API_INFO* pApiInfo;

    // hooks needs to be re installed
    this->pLinkListHookedInterfaces->Lock();
    for (pItemInterface=this->pLinkListHookedInterfaces->Head;pItemInterface;pItemInterface=pItemInterface->NextItem)
    {
        pInterfaceInfo=(CHookedInterface*)pItemInterface->ItemData;

        pInterfaceInfo->pMethodInfoList->Lock();
        for (pItemMethod=pInterfaceInfo->pMethodInfoList->Head;pItemMethod;pItemMethod=pItemMethod->NextItem)
        {
            pMethodInfo=(CMethodInfo*)pItemMethod->ItemData;

            if (IsBadReadPtr(pMethodInfo->pItemAPIInfo,sizeof(CLinkListItem)))
                continue;

            pApiInfo=(API_INFO*)pMethodInfo->pItemAPIInfo->ItemData;
            if (IsBadReadPtr(pApiInfo,sizeof(API_INFO)))
                continue;

            if (IsBadReadPtr(pApiInfo->APIAddress,sizeof(PBYTE)))
                continue;

            // restore hook
            // remove memory protection
            // as pApiInfo->APIAddress is not system page rounded, do not use "if (!VirtualProtect(pApiInfo->APIAddress, dwSystemPageSize,..."
            if(VirtualProtect(pApiInfo->APIAddress, pApiInfo->OpcodeReplacementSize, PAGE_EXECUTE_READWRITE, &pApiInfo->dwOldProtectionFlags))
            {
                // reinstall hook
                memcpy(pApiInfo->APIAddress,pApiInfo->pbHookCodes,pApiInfo->OpcodeReplacementSize);
            }

        }
        pInterfaceInfo->pMethodInfoList->Unlock();
    }
    this->pLinkListHookedInterfaces->Unlock();

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetInternallyCreatedObject
// Object: Get pointer to object created internally by this HookCom dll
//          for installing static hooks
// Parameters :
//     in  : 
//     out : 
//     return : object pointer on success, NULL if no object of this class has been created
//-----------------------------------------------------------------------------
CHookedObject* CHookedClass::GetInternallyCreatedObject()
{
    // look for an hooked object having bHookedByAutoMonitoring==FALSE
    CHookedObject* pHookedObject=NULL;
    CLinkListItem* pItem;
    this->pLinkListHookedObjects->Lock();
    // for each hooked object
    for(pItem=this->pLinkListHookedObjects->Head;pItem;pItem=pItem->NextItem)
    {
        pHookedObject=(CHookedObject*)pItem->ItemData;
        if (pHookedObject->bCreatedByStaticFileLoading)
        {
            this->pLinkListHookedObjects->Unlock();
            return pHookedObject;
        }

    }
    this->pLinkListHookedObjects->Unlock();
    return NULL;
}

//-----------------------------------------------------------------------------
// Name: GetObject
// Object: get hooked object associated to provided interface pointer
// Parameters :
//     in  : 
//     out : 
//     return : pointer to hooked object associated to provided interface pointer on success
//              NULL if no hooked object of has been found in current class object
//-----------------------------------------------------------------------------
CHookedObject* CHookedClass::GetObject(IUnknown* pObject,IID* pIid)
{
    if (IsBadReadPtr(pObject,sizeof(IUnknown)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        return NULL;
    }

    CHookedObject* pHookedObject=NULL;
    CHookedObject* pAlreadyExistingHookedObject;
    pHookedObject=new CHookedObject(pObject,pIid);
    if (!pHookedObject)
        return NULL;
    if(!pHookedObject->SuccessfullyCreated())
    {
        delete pHookedObject;
        return NULL;
    }

    // check if object belongs to pLinkListHookedObjects
    // for each object in pLinkListHookedObjects
    CLinkListItem* pItem;
    this->pLinkListHookedObjects->Lock(FALSE);
    // for each hooked object
    for(pItem=this->pLinkListHookedObjects->Head;pItem;pItem=pItem->NextItem)
    {
        pAlreadyExistingHookedObject=(CHookedObject*)pItem->ItemData;

        // if pointer, or IUnknown pointer are already known
        if (pAlreadyExistingHookedObject->DoesPointerBelongToObject(pHookedObject->pIUnknown)
            || pAlreadyExistingHookedObject->DoesPointerBelongToObject(pHookedObject->pObject))
        {
            this->pLinkListHookedObjects->Unlock();
            // delete new created hooked object
            delete pHookedObject;
            return (CHookedObject*)pItem->ItemData;
        }

    }
    this->pLinkListHookedObjects->Unlock();
    delete pHookedObject;
    return NULL;
}

//-----------------------------------------------------------------------------
// Name: AddObject
// Object: add an hooked object to current class
// Parameters :
//     in  : 
//     out : 
//     return : pointer to hooked object associated to provided interface pointer on success
//              NULL on error or if object has already been registered
//-----------------------------------------------------------------------------
CHookedObject* CHookedClass::AddObject(IUnknown* pObject,IID* pIid)
{
    if (IsBadReadPtr(pObject,sizeof(IUnknown)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        return NULL;
    }

    CHookedObject* pHookedObject=NULL;
    pHookedObject=new CHookedObject(pObject,pIid);
    if (!pHookedObject)
        return NULL;
    if(!pHookedObject->SuccessfullyCreated())
    {
        delete pHookedObject;
        return NULL;
    }

    // if no AssociatedModuleBaseAddress ( and so no AssociatedModuleName)
    if (this->AssociatedModuleBaseAddress==0)
    {
        // find AssociatedModuleBaseAddress and AssociatedModuleName
        if(!this->FindAssociatedDllName(pHookedObject))
        {
            // this occurs on just in time compiling like for .Net COM objects
            this->DllType=DLL_TYPE_JIT_COMPILED;
            TCHAR pszMsg[MAX_PATH];
            _stprintf(pszMsg,_T("COM: Object 0x%p can't be hooked because it belongs to a just in time compiling library (like .Net)."),pObject);
            HookComInfos.ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);
            delete pHookedObject;
            return NULL;
        }
    }

    this->DllType=DLL_TYPE_STANDARD;

    // in case dll was unloaded and we don't catch it (avoid to loose COM calls) 
    this->CheckAndRestoreClassHooks(pHookedObject);

    
    // check that object is not already added
    // for each object in pLinkListHookedObjects
    CLinkListItem* pItem;
    CHookedObject* pAlreadyExistingHookedObject;
    this->pLinkListHookedObjects->Lock(FALSE);
    // for each hooked object
    for(pItem=this->pLinkListHookedObjects->Head;pItem;pItem=pItem->NextItem)
    {
        pAlreadyExistingHookedObject=(CHookedObject*)pItem->ItemData;

        // if pointer or IUnknown pointer are already known
        if (pAlreadyExistingHookedObject->DoesPointerBelongToObject(pHookedObject->pIUnknown)
            || pAlreadyExistingHookedObject->DoesPointerBelongToObject(pHookedObject->pObject)
            )
        {
            this->pLinkListHookedObjects->Unlock();
            // delete new created hooked object
            delete pHookedObject;
            return NULL;
        }

    }
    this->pLinkListHookedObjects->Unlock();

    // add object to hooked object list
    if(!this->pLinkListHookedObjects->AddItem(pHookedObject))
    {
        delete pHookedObject;
        return NULL;
    }

    if (this->bIDispatchParsingSuccessFull)
        // force IDispatch retrieval for current object
        pHookedObject->GetpIDispatch(pObject);

    return pHookedObject;
}

//-----------------------------------------------------------------------------
// Name: AddAutoHookedObject
// Object: Call this function after an API creating COM object has been called successfully.
//         hooks will be installed automatically
// Parameters :
//     in  : 
//           IUnknown* pObject : pointer to object interface associated with pIid
//           IID* pIid : pointer to IID
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CHookedClass::AddAutoHookedObject(IUnknown* pObject,IID* pIid)
{
    if (IsBadReadPtr(pObject,sizeof(IUnknown)))
        return FALSE;

    // assume object won't be released
    CSecureIUnknown::AddRef(pObject);

    CHookedObject* pHookedObject;

    //////////////////////////////////////
    // add object to current object list (and register Interface associated to pIid for object)
    //////////////////////////////////////
    pHookedObject=this->AddObject(pObject,pIid);

    // on error, or if object has already been registered
    if (!pHookedObject)
    {
        CSecureIUnknown::Release(pObject);
        return FALSE;
    }

    //////////////////////////////////////
    // report object hooking if necessary
    //////////////////////////////////////
    if (HookComOptions.ReportHookedCOMObject)
    {
        TCHAR pszMsg[STRING_PROGID_MAX_SIZE+STRING_GUID_SIZE+100];
        BOOL bReportMessage=FALSE;
        if (HookComOptions.ReportUseNameInsteadOfIDIfPossible)
        {
            TCHAR _tProgId[STRING_PROGID_MAX_SIZE];
            bReportMessage=CGUIDStringConvert::GetClassName(&this->Clsid,_tProgId,STRING_PROGID_MAX_SIZE);
            if (bReportMessage)
                _stprintf(pszMsg,_T("COM: Object 0x%p of type %s"),pObject,_tProgId);
            else // IID can be used instead of CLSID when CLSID is unknown, to uniquely identify object class
            {
                if (CGUIDStringConvert::TcharFromIID(pIid,_tProgId))
                {
                    bReportMessage=CGUIDStringConvert::GetInterfaceName(_tProgId,_tProgId,STRING_IID_MAX_SIZE);
                    if (bReportMessage)
                        _stprintf(pszMsg,_T("COM: Object 0x%p implementing %s"),pObject,_tProgId);
                }
            }
        }
        // if progId retrieval failure or !ReportUseProgIdInsteadOfCLSIIfPossible
        if (!bReportMessage)
        {
            TCHAR _tClsid[STRING_GUID_SIZE];
            bReportMessage=CGUIDStringConvert::TcharFromCLSID(&this->Clsid,_tClsid);
            if (bReportMessage)
                _stprintf(pszMsg,_T("COM: Object 0x%p with CLSID %s"),pObject,_tClsid);
        }
        if(bReportMessage)
        {
            TCHAR _tIID[STRING_IID_MAX_SIZE];
            if (CGUIDStringConvert::TcharFromIID(pIid,_tIID))
            {
                if (HookComOptions.ReportUseNameInsteadOfIDIfPossible)
                    CGUIDStringConvert::GetInterfaceName(_tIID,_tIID,STRING_IID_MAX_SIZE);
                _tcscat(pszMsg,_T(" created with IID "));
                _tcscat(pszMsg,_tIID);
            }
            _tcscat(pszMsg,_T(" successfully hooked."));
            HookComInfos.ReportMessage(REPORT_MESSAGE_INFORMATION,pszMsg);
        }
    }

    // this occurs on just in time compiling like for .Net COM objects
    if (this->DllType==DLL_TYPE_JIT_COMPILED)
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("COM: Object 0x%p can't be hooked because it belongs to a just in time compiling library (like .Net)."),pObject);
        HookComInfos.ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);
        CSecureIUnknown::Release(pObject);
        return FALSE;
    }

    BOOL bIDispatchAssociatedIidMonitoringFileSet=FALSE;
    IID InterfaceExposedByIDispatchIid;


    // if interface is already hooked 
    if (this->IsInterfaceHookedByAutoMonitoring(pObject,pIid))
    {
        CSecureIUnknown::Release(pObject);
        // do nothing
        return TRUE;
    }

    IID Iid;
    // try to hook IDispatch if queried
    if (HookComOptions.IDispatchAutoMonitoring)
    {
        // IDispatch checking
        if(!this->bIDispatchParsingHasBeenTried)
        {
            // don't try IDispatch parsing if IClassFactory has been queried
            if (   (!IsEqualIID(*pIid,IID_IClassFactory))
                && (!IsEqualIID(*pIid,IID_IClassFactory2))
                && (!IsEqualIID(*pIid,IID_IClassFactoryEx))
                )
            {
                IUnknown* pIDispatchEx;
                // check IDispatchEx support
                if (SUCCEEDED(CSecureIUnknown::QueryInterface(pObject,IID_IDispatchEx,(void**)&pIDispatchEx)))
                {
                    this->bSupportIDispatchEx=TRUE;
                    CSecureIUnknown::Release(pIDispatchEx);
                    // WE CAN'T USE STATIC HOOKS AND IDISPATCH PARSING
                }

                // if IDispatchEx is not supported
                if (!this->bSupportIDispatchEx)
                {

                    IDispatch* pIDispatch=NULL;
                    Iid=IID_IDispatch;

                    // try to get IDispatch (and register IDispatch Interface for object)
                    pIDispatch=pHookedObject->GetpIDispatch(pObject);
                    if (pIDispatch)
                    {
                        // parse IDispatch
                        if (this->ParseIDispatch(pObject,pIid))
                        {
                            // load IDispatch monitoring file (load preferred definitions for func and args of IDispatch base interface)
                            this->SetMonitoringHookFromFile(pHookedObject,
                                                            pIDispatch,
                                                            &Iid,
                                                            &Iid,
                                                            NULL,
                                                            TRUE,
                                                            NULL,
                                                            NULL);

                            
                            // if there is no monitoring file associated to IID associated to IDispatch
                            CHookedInterface* pInterfaceInfo=this->pInterfaceExposedByIDispatch;
                            InterfaceExposedByIDispatchIid=pInterfaceInfo->Iid;
                            bIDispatchAssociatedIidMonitoringFileSet=TRUE;
                            if (!this->SetMonitoringHookFromFile(pHookedObject,pInterfaceInfo->pInterfaceAssociatedToIID,&pInterfaceInfo->Iid,&pInterfaceInfo->Iid,NULL,TRUE,NULL,NULL))
                            {
                                // add hook for functions exposed by IDispatch
                                this->SetMonitoringHookFromIDispatchParsing();
                            }

                            // add post hook for IDispatch interface
                            this->AddIUnknownPostHooks(pIDispatch,&Iid);

                            // register Interface exposed by IDispatch for object
                            pHookedObject->AddInterfacePointer(pInterfaceInfo->pInterfaceAssociatedToIID,&pInterfaceInfo->Iid);

                            // add post hook for Interface exposed by IDispatch
                            this->AddIUnknownPostHooks(pInterfaceInfo->pInterfaceAssociatedToIID,&pInterfaceInfo->Iid);
                        }
                        else
                        {
                            // load IDispatch Monitoring file even if IDispatch parsing fails
                            this->SetMonitoringHookFromFile(pHookedObject,
                                                            pIDispatch,
                                                            &Iid,
                                                            &Iid,
                                                            NULL,
                                                            TRUE,
                                                            NULL,
                                                            NULL);
                        }
                    }
                    else
                    {
                        this->bIDispatchParsingHasBeenTried=TRUE;
                        this->bIDispatchParsingSuccessFull=FALSE;

                        // if CLSID not supporting IDispatch must be reported
                        if (HookComOptions.ReportCLSIDNotSupportingIDispatch)
                        {
                            // report warning
                            TCHAR pszMsg[STRING_PROGID_MAX_SIZE+100];
                            BOOL bReportMessage=FALSE;

                            if (HookComOptions.ReportUseNameInsteadOfIDIfPossible)
                            {
                                TCHAR _tProgId[STRING_PROGID_MAX_SIZE];
                                bReportMessage=CGUIDStringConvert::GetClassName(&this->Clsid,_tProgId,STRING_PROGID_MAX_SIZE);
                                if (bReportMessage)
                                    _stprintf(pszMsg,_T("COM: type %s doesn't support IDispatch"),_tProgId);
                            }
                            // if progId retrieval failure or !ReportUseProgIdInsteadOfCLSIIfPossible
                            if (!bReportMessage)
                            {
                                TCHAR _tClsid[STRING_GUID_SIZE];
                                bReportMessage=CGUIDStringConvert::TcharFromCLSID(&this->Clsid,_tClsid);
                                if (bReportMessage)
                                    _stprintf(pszMsg,_T("COM: CLSID %s doesn't support IDispatch"),_tClsid);
                            }
                            if(bReportMessage)
                                HookComInfos.ReportMessage(REPORT_MESSAGE_WARNING,pszMsg);
                        }
                    }
                }
            }
        }
    }

    // if class has never been hooked by auto-monitoring
    if (!this->bHookedByAutoMonitoring)
    {
        // class auto-monitoring hooks are installed for this class
        // put flag to avoid to put them again
        this->bHookedByAutoMonitoring=TRUE;

        // add post hook for IUnknown interface of the object (add them only AFTER having done IDispatch parsing)
        Iid=IID_IUnknown;
        IUnknown* pIUnknown=pHookedObject->GetpIUnknown(pObject);
        if (pIUnknown)
        {
            this->AddIUnknownPostHooks(pIUnknown,&Iid);

            // add monitoring hook for IUnknown interface
            this->SetMonitoringHookFromFile(pHookedObject,pIUnknown,&Iid,&Iid,NULL,TRUE,NULL,NULL);
        }
    }

    // add post hook to the interfaces only if class doesn't support IDispatchEx
    if (!this->bSupportIDispatchEx)
    {
        // add post hook to Release and QueryInterface to the queried Interface
        this->AddIUnknownPostHooks(pObject,pIid);

        // parse wanted interface description file
        // only if not already loaded
        if (bIDispatchAssociatedIidMonitoringFileSet)
        {
            // check if file has not already been set
            if (!IsEqualIID(InterfaceExposedByIDispatchIid,*pIid))
            {
                // parse wanted interface description file
                this->SetMonitoringHookFromFile(pHookedObject,pObject,pIid,pIid,NULL,TRUE,NULL,NULL);
            }
        }
        else
        {
            // parse wanted interface description file
            this->SetMonitoringHookFromFile(pHookedObject,pObject,pIid,pIid,NULL,TRUE,NULL,NULL);
        }
    }

    CSecureIUnknown::Release(pObject);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: AddIUnknownPostHooks
// Object: Release and QueryInterface post hooks to interface
// Parameters :
//     in  : IUnknown* pInterface : interface pointer
//           IID* pIid : IID associated to interface pointer
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CHookedClass::AddIUnknownPostHooks(IUnknown* pInterface,IID* pIid)
{
    // if no com auto hooking, don't add post hooks
    if (!bCOMAutoHookingEnabled)
        return TRUE;

    if (IsBadReadPtr(pInterface,sizeof(IUnknown)))
        return FALSE;

    // get associated CHookedInterface objects
    CHookedInterface* pHookedInterface=this->GetOrCreateHookedInterface(pInterface,pIid);
    if(!pInterface)
        return FALSE;
    // if already hooked
    if (pHookedInterface->bIUnknownDerivedInterfaceHooked)
        return TRUE;

    CLinkListItem* pItem;
    CHookedInterface* pOtherHookedInterface;

    // for each hooked interfaces
    this->pLinkListHookedInterfaces->Lock();
    for (pItem=this->pLinkListHookedInterfaces->Head;pItem;pItem=pItem->NextItem)
    {
        pOtherHookedInterface=(CHookedInterface*)pItem->ItemData;
        // if current interface
        if(pOtherHookedInterface==pHookedInterface)
            continue;
        // if interface has not same VTBL address
        if (pOtherHookedInterface->VTBLAddress!=pHookedInterface->VTBLAddress)
            continue;

        if (pOtherHookedInterface->bIUnknownDerivedInterfaceHooked)
        {
            pHookedInterface->bIUnknownDerivedInterfaceHooked=TRUE;
            this->pLinkListHookedInterfaces->Unlock();
            return TRUE;
        }
    }
    this->pLinkListHookedInterfaces->Unlock();

    // VTBL is not hooked

    pHookedInterface->bIUnknownDerivedInterfaceHooked=TRUE;

    // install a post call hook on release method to spy object destruction
    this->AddPostReleaseCallHook(pInterface,pHookedInterface);

    // add query interface post hook only if interface doesn't support IDispatchEx
    if (!this->bSupportIDispatchEx)
    {
        // install a post call hook on QueryInterface so, each time an interface
        // is queried, we can add hook for it
        this->AddPostQueryInterfaceCallHook(pInterface,pHookedInterface);
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsInterfaceHookedByAutoMonitoring
// Object: allow to know if Interface has been hooked by auto monitoring, or
//         if interface has been hook by static monitoring file loading
// Parameters :
//     in  : 
//           IID* pIid : pointer to IID
//     out : 
//     return : TRUE if Interface has been hooked by auto monitoring, FALSE 
//              if interface has been hook by static monitoring file loading
//-----------------------------------------------------------------------------
BOOL CHookedClass::IsInterfaceHookedByAutoMonitoring(IUnknown* pInterfaceAssociatedToIID,IID* pIid)
{
    // get associated hooked interface
    CHookedInterface* pHookedInterface=this->GetHookedInterface(pInterfaceAssociatedToIID,pIid);

    // if associated interface doesn't exists
    if (!pHookedInterface)
        return FALSE;

    // return bHookedByAutoMonitoring information of matching pHookedInterface
    return pHookedInterface->bHookedByAutoMonitoring;

}
//-----------------------------------------------------------------------------
// Name: GetHookedInterface
// Object: get a CHookedInterface object associated to the specified IID
// Parameters :
//     in  : 
//           IID* pIid : pointer to IID
//     out : 
//     return : pointer on success, NULL on failure
//-----------------------------------------------------------------------------
CHookedInterface* CHookedClass::GetHookedInterface(IUnknown* pInterfaceAssociatedToIID,IID* pIid)
{
    CHookedInterface* pHookedInterface;
    CLinkListItem* pHookedInterfaceItem;

    // lock list
    this->pLinkListHookedInterfaces->Lock();

    // for each pLinkListHookedIntreface item
    for (pHookedInterfaceItem=this->pLinkListHookedInterfaces->Head;pHookedInterfaceItem;pHookedInterfaceItem=pHookedInterfaceItem->NextItem)
    {
        pHookedInterface=(CHookedInterface*)pHookedInterfaceItem->ItemData;
        // if pHookedClass defines the specified CLSID
        if (pInterfaceAssociatedToIID==pHookedInterface->pInterfaceAssociatedToIID)
        {
            if (IsEqualIID(pHookedInterface->Iid,*pIid))
            {

                // unlock list
                this->pLinkListHookedInterfaces->Unlock();

                // return HookedClass object corresponding to the CLSID
                return pHookedInterface;
            }
        }
    }
    // if not found
    // unlock list
    this->pLinkListHookedInterfaces->Unlock();
    return NULL;
}
//-----------------------------------------------------------------------------
// Name: GetOrCreateHookedInterface
// Object: get or create if not existing, a CHookedObject object associated to the
//          specified IID
// Parameters :
//     in  : 
//           IUnknown* pInterfaceAssociatedToIID : pointer to Interface specified by pIid
//           IID* pIid : pointer to IID
//     out : 
//     return : pointer on success, NULL on failure
//-----------------------------------------------------------------------------
CHookedInterface* CHookedClass::GetOrCreateHookedInterface(IUnknown* pInterfaceAssociatedToIID,IID* pIid)
{
    CHookedInterface* pHookedInterface=this->GetHookedInterface(pInterfaceAssociatedToIID,pIid);
    if(pHookedInterface)
        return pHookedInterface;

    if (IsBadReadPtr(pInterfaceAssociatedToIID,sizeof(IUnknown)))
        return NULL;

    // create an hooked interface
    pHookedInterface=new CHookedInterface(pInterfaceAssociatedToIID,pIid);
    if (!pHookedInterface)
        return NULL;


    // add created object to list
    if (!this->pLinkListHookedInterfaces->AddItem(pHookedInterface))
    {
        delete pHookedInterface;
        return NULL;
    }
    return pHookedInterface;
}
//-----------------------------------------------------------------------------
// Name: ParseIDispatch
// Object: parse IDispatch interface for auto defined class
// Parameters :
//     in  : 
//           IUnknown* pObject : pointer to Interface specified by pIid
//           IID* pIid : pointer to IID
//     out : 
//     return : TRUE on success, FALSE on failure
//-----------------------------------------------------------------------------
BOOL CHookedClass::ParseIDispatch(IUnknown* pObject,IID* pIid)
{
    if (this->bIDispatchParsingHasBeenTried)
        return this->bIDispatchParsingSuccessFull;

    if (IsBadReadPtr(pObject,sizeof(IUnknown)))
        return FALSE;

    this->bIDispatchParsingHasBeenTried=TRUE;
    ////////////////////////////////
    // get pointer on IDispatch
    ////////////////////////////////
    IDispatch* pIDispatch=NULL;

    // query the IDispatch pointer
    if (IsEqualIID(*pIid,IID_IDispatch))
    {
        pIDispatch=(IDispatch*)pObject;
        // AddRef during parsing
        CSecureIUnknown::AddRef(pIDispatch);
    }
    else
    {
        pIDispatch=NULL;
        BOOL bSuccess=SUCCEEDED(CSecureIUnknown::QueryInterface(pObject,IID_IDispatch,(void**)&pIDispatch));
        
        if (bSuccess)
        {
            if (IsBadReadPtr(pIDispatch,sizeof(IDispatch)))
                bSuccess=FALSE;
        }
        if (!bSuccess)
        {
            pIDispatch=NULL;
        }
    }

    // if not supporting IDispatch
    if (!pIDispatch)
    {
        this->bIDispatchParsingSuccessFull=FALSE;
        return FALSE;
    }


    ////////////////////////////////
    // parse IDispatch
    ////////////////////////////////
    ITypeInfo* pITypeInfo=NULL;
    UINT TypeInfoCount;
    HRESULT hResult;

    hResult=pIDispatch->GetTypeInfoCount(&TypeInfoCount);
    if (FAILED(hResult))
    {
        this->bIDispatchParsingSuccessFull=FALSE;
        // release dispatch interface
        CSecureIUnknown::Release(pIDispatch);
        return FALSE;
    }

    // If the object provides type information, this number is 1; otherwise the number is 0.
    if (TypeInfoCount==0)
    {
        this->bIDispatchParsingSuccessFull=FALSE;
        // release dispatch interface
        CSecureIUnknown::Release(pIDispatch);
        return FALSE;
    }

    // get Type Info for first item
    hResult=pIDispatch->GetTypeInfo(0,GetUserDefaultLCID(),&pITypeInfo);
    if (FAILED(hResult) || (!pITypeInfo))
    {
        this->bIDispatchParsingSuccessFull=FALSE;
        // release dispatch interface
        CSecureIUnknown::Release(pIDispatch);
        return FALSE;
    }

    // get pointer to interface of IDispatch associated iid
    TYPEATTR* pTypeAttr=NULL;
    IID IDispatchAssociatedIid;
    hResult = pITypeInfo->GetTypeAttr(&pTypeAttr);
    if( FAILED(hResult) || (!pTypeAttr))
        return FALSE;

    // fill iid field
    IDispatchAssociatedIid = pTypeAttr->guid;
    pITypeInfo->ReleaseTypeAttr(pTypeAttr);

    // get pointer to interface of iid
    IUnknown* pIDispatchAssociatedObject=NULL;
    hResult = CSecureIUnknown::QueryInterface(pObject,IDispatchAssociatedIid,(void**)&pIDispatchAssociatedObject);
    if( FAILED(hResult) || (!pIDispatchAssociatedObject))
        return FALSE;

    // get object methods and properties
    
    // the following works only if pIDispatch==pObject
    // // IID Iid=IID_IDispatch;
    // // CHookedInterface* pInterfaceInfo=this->GetOrCreateHookedInterface(pIDispatch,&Iid);
    // so we have to do
    CHookedInterface* pInterfaceInfo=this->GetOrCreateHookedInterface(pIDispatchAssociatedObject,&IDispatchAssociatedIid);

    if (!this->pInterfaceExposedByIDispatch)
        this->pInterfaceExposedByIDispatch=pInterfaceInfo;
    if (FAILED(pInterfaceInfo->Parse(pIDispatchAssociatedObject,pIDispatch,pITypeInfo,TRUE,TRUE)))
    {
        // release type info
        CSecureIUnknown::Release(pITypeInfo);
        // release dispatch interface
        CSecureIUnknown::Release(pIDispatch);
        this->bIDispatchParsingSuccessFull=FALSE;
        CSecureIUnknown::Release(pIDispatchAssociatedObject);
        return FALSE;
    }

    // release associated object
    CSecureIUnknown::Release(pIDispatchAssociatedObject);

    // release type info
    CSecureIUnknown::Release(pITypeInfo);

    // release dispatch interface
    CSecureIUnknown::Release(pIDispatch);

    this->bIDispatchParsingSuccessFull=TRUE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetMonitoringHookFromFile
// Object: install hook for specified interface by
//          1) look in COM monitoring file directory, if IID.txt file exists
//          2) if no monitoring IID.txt file found, try to install hooks from IDispatch Interface
// Parameters :
//     in  : CHookedObject* pHookedObject : pointer to a CHookedObject
//           IUnknown* pInterface : pointer to Interface specified by pIid
//           IID* pIid : pointer to IID
//           IID* pFileIid : COM monitoring file IID (current IID or a base IID) this is this iid that will be used for monitoring file
//           CLinkList* pLinkListOfBaseInterfacesID : ordered list of base interfaces (first element is the base interface)
//                                                  contains something like IID_IAgent,IID_IDispatch
//           BOOL HookedQueriedByAutoMonitoring : TRUE if SetMonitoringHookFromFile is called by automonitoring
//                                          FALSE if called by static monitoring
//           MONITORING_FILE_INFOS* SpecifiedpMonitoringFileInfo : forced monitoring file info
//                                                                 used by static COM hooks for unloading
//     out : 
//     return : TRUE on success, FALSE on failure
//-----------------------------------------------------------------------------
BOOL CHookedClass::SetMonitoringHookFromFile(CHookedObject* pHookedObject,IUnknown* pInterface,IID* pIid,IID* pFileIid,CLinkList* pLinkListOfBaseInterfacesID,BOOL HookQueriedByAutoMonitoring,MONITORING_FILE_INFOS* SpecifiedpMonitoringFileInfo,MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo)
{
    TCHAR pszMonitoringFileName[MAX_PATH];
    CLinkList* mpLinkListOfBaseInterfacesID;
    BOOL bFileError=FALSE;

    if (IsBadReadPtr(pInterface,sizeof(IUnknown)))
        return FALSE;


    // if a list of base interfaces ID is defined
    if (pLinkListOfBaseInterfacesID)
        mpLinkListOfBaseInterfacesID=pLinkListOfBaseInterfacesID;
    else
    {
        // find associated HookedInterface pointer
        CHookedInterface* pHookedInterface=this->GetOrCreateHookedInterface(pInterface,pIid);
        if(!pHookedInterface)
            return FALSE;    

        if (HookQueriedByAutoMonitoring)
        {
            // if interface is already hooked
            if (pHookedInterface->bHookedByAutoMonitoring)
                return TRUE;

            // set flag to not redo same job
            pHookedInterface->bHookedByAutoMonitoring=TRUE;
        }

        // create linked list of Base Interface ID
        mpLinkListOfBaseInterfacesID=new CLinkList(sizeof(IID));
    }
         
    // get Interface associated file name
    if (!GetMonitoringFileName(pFileIid,pszMonitoringFileName))
        bFileError=TRUE;
    else
    {
        // check if a monitoring file is defined for this interface
        if (!CStdFileOperations::DoesFileExists(pszMonitoringFileName))
        {
            if (HookComOptions.ReportIIDHavingNoMonitoringFileAssociated)
            {
                TCHAR _tIID[STRING_IID_MAX_SIZE];
                TCHAR pszMsg[STRING_IID_MAX_SIZE+100];

                // get string iid corresponding to interface 
                if (CGUIDStringConvert::TcharFromIID(pFileIid,_tIID))
                {
                    // send report to user
                    if (HookComOptions.ReportUseNameInsteadOfIDIfPossible)
                        CGUIDStringConvert::GetInterfaceName(_tIID,_tIID,STRING_IID_MAX_SIZE);
                    _tcscpy(pszMsg,_T("COM: No monitoring file found for interface "));
                    _tcscat(pszMsg,_tIID);
                    HookComInfos.ReportMessage(REPORT_MESSAGE_WARNING,pszMsg);
                }
            }
            bFileError=TRUE;
        }
    }
    if (bFileError)
    {
        if (HookQueriedByAutoMonitoring)
        {
            
            // force IUnknown monitoring
            IID Iid=IID_IUnknown;
            if (!IsEqualIID(*pFileIid,Iid))// avoid a nice infinite loop and stack overflow if IUnknown monitoring file doesn't exists
            {
                CLinkListItem* pItem=mpLinkListOfBaseInterfacesID->AddItem(&Iid);
                this->SetMonitoringHookFromFile(pHookedObject,pInterface,pIid,&Iid,mpLinkListOfBaseInterfacesID,HookQueriedByAutoMonitoring,SpecifiedpMonitoringFileInfo,ppAlreadyHookingMonitoringFileInfo);
                mpLinkListOfBaseInterfacesID->RemoveItem(pItem);
            }
        }

        if (!pLinkListOfBaseInterfacesID)
            delete mpLinkListOfBaseInterfacesID;
        return FALSE;
    }

    PARSE_COM_MONITORING_FILE_LINE_PARAM Param;
    Param.pHookedClass=this;
    Param.pHookedObject=pHookedObject;
    Param.pInterfaceAssociatedToIID=pInterface;
    Param.bAtLeastOneMethodHasBeenHooked=FALSE;
    Param.pIid=pIid;
    Param.pLinkListOfBaseInterfacesID=mpLinkListOfBaseInterfacesID;
    Param.HookQueriedByAutoMonitoring=HookQueriedByAutoMonitoring;
    Param.SpecifiedpMonitoringFileInfo=SpecifiedpMonitoringFileInfo;
    Param.pAlreadyHookingMonitoringFileInfo=NULL;

    // parse monitoring file
    CTextFile::ParseLines(pszMonitoringFileName,
                            HookComInfos.hevtFreeProcess,
                            CHookedClass::ParseCOMMonitoringFileLine,
                            &Param);

    if (!pLinkListOfBaseInterfacesID)
        delete mpLinkListOfBaseInterfacesID;

    if (ppAlreadyHookingMonitoringFileInfo)
        *ppAlreadyHookingMonitoringFileInfo=Param.pAlreadyHookingMonitoringFileInfo;

    return Param.bAtLeastOneMethodHasBeenHooked;
}

//-----------------------------------------------------------------------------
// Name: SetMonitoringHookFromIDispatchParsing
// Object: install hook from informations provided by IDispatch
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success, FALSE on failure
//-----------------------------------------------------------------------------
BOOL CHookedClass::SetMonitoringHookFromIDispatchParsing()
{
    BOOL bAlreadyHooked;
    BOOL bAtLeastOneHookIsInstalled=FALSE;

    if (HookComOptions.QueryMethodToHookForInterfaceParsedByIDispatch)
    {
        // list of functions name
        // option to generate monitoring file
        CIDispatchResultSelection::Show(this);
    }
    CMethodInfo*   pMethodInfo;
    CLinkListItem* pItemParameterInfo;
    CLinkListItem* pItemApiInfo;
    API_INFO*      pApiInfo;
    DWORD          ParameterIndex;
    CParameterInfo* pParameterInfo;
    CLinkListItem* pItem;
    CHookedInterface* pInterfaceInfo;
    BOOL           bUsingIDispatchMonitoringFileDefinition;
    TCHAR IIDName[STRING_IID_MAX_SIZE];
    TCHAR IIDAndFunctionName[MAX_PATH];
    TCHAR pszMonitoringFileName[MAX_PATH];
    IID IidIDispatch;

#if ((!defined(UNICODE)) && (!defined(_UNICODE)))
    CHAR* pc;
#endif

    // get the Interface associated to IDispatch parsing result
    pInterfaceInfo=this->pInterfaceExposedByIDispatch;

    // if IDispatch parsing has failed
    if (!pInterfaceInfo)
        return FALSE;

    // check if IDispatch monitoring file exists to avoid to overwrite it's options
    bUsingIDispatchMonitoringFileDefinition=FALSE;
    IidIDispatch=IID_IDispatch;
    if (GetMonitoringFileName(&IidIDispatch,pszMonitoringFileName))
    {
        // check if a monitoring file is defined for this interface
        if (CStdFileOperations::DoesFileExists(pszMonitoringFileName))
            bUsingIDispatchMonitoringFileDefinition=TRUE;
    }


    *IIDName=0;
    if (CGUIDStringConvert::TcharFromIID(&pInterfaceInfo->Iid,IIDName))
    {
        CGUIDStringConvert::GetInterfaceName(IIDName,IIDName,STRING_IID_MAX_SIZE);
        _tcsncat(IIDName,_T("::"),STRING_IID_MAX_SIZE-_tcslen(IIDName)-1);
    }

    // for each function to hook
    for (pItem=pInterfaceInfo->pMethodInfoList->Head;pItem;pItem=pItem->NextItem)
    {
        pMethodInfo=(CMethodInfo*)pItem->ItemData;
        if (IsBadReadPtr(pMethodInfo,sizeof(CMethodInfo)))
            continue;

        // if a monitoring file exists describing IDispatch interface
        if (bUsingIDispatchMonitoringFileDefinition)
        {
            // don't take into account IDispatch interface method
            if (pMethodInfo->VTBLIndex<=6)
                continue;
        }

        if (HookComOptions.QueryMethodToHookForInterfaceParsedByIDispatch)
        {
            // if dialog unactivated function (see upper)
            if (pMethodInfo->AskedToBeNotLogged)
                continue;
        }

        // if method has a not NULL pItemAPIInfo, that means method is already hooked
        if (pMethodInfo->pItemAPIInfo)
        {
            pItemApiInfo=pMethodInfo->pItemAPIInfo;
            bAlreadyHooked=TRUE;
        }
        else
        {
            // forge IID::Name
            _tcscpy(IIDAndFunctionName,IIDName);
#if (defined(UNICODE)||defined(_UNICODE))
            _tcsncat(IIDAndFunctionName,pMethodInfo->Name,MAX_PATH-_tcslen(IIDAndFunctionName)-1);
#else
            CAnsiUnicodeConvert::UnicodeToAnsi(pMethodInfo->Name,&pc);
            _tcsncat(IIDAndFunctionName,pc,MAX_PATH);
            free(pc);
#endif 
            
            pMethodInfo->SetName(IIDAndFunctionName);

            // initialize hook struct
            if (!InitializeHook(this->AssociatedModuleName,pMethodInfo,&pItemApiInfo,&bAlreadyHooked))
                continue;
        }

        pApiInfo=(API_INFO*)pItemApiInfo->ItemData;

        // if a hook was installed
        if (bAlreadyHooked)
        {
            // check if a monitoring hook was installed
            if (pApiInfo->pMonitoringFileInfos)
            {
                // don't associate pItemApiInfo to hooked method in case monitoring hook is owned by a static hook
                continue; // nothing to do
            }
        }

        // associate pItemApiInfo to hooked method
        pMethodInfo->pItemAPIInfo=pItemApiInfo;

        pApiInfo->pMonitoringFileInfos=&HookComDll_IDispatchMonitoringFileInfo;
        pApiInfo->ParamDirectionType=PARAM_DIR_IN;

        //////////////////////////
        // fill parameters infos
        //////////////////////////
        pApiInfo->MonitoringParamCount=0;
        if (!bAlreadyHooked)
            pApiInfo->StackSize=0;
        ParameterIndex=0;

        // according to function kind add a pvoid parameter corresponding to the object pointer
        if (pMethodInfo->MustThisPointerBeAddedAsFirstParameter())
        {
            _tcscpy(pApiInfo->ParamList[ParameterIndex].pszParameterName,_T("pObject"));
            pApiInfo->ParamList[ParameterIndex].dwSizeOfData=sizeof(PVOID);
            pApiInfo->ParamList[ParameterIndex].dwSizeOfPointedData=0;
            pApiInfo->ParamList[ParameterIndex].dwType=PARAM_POINTER;
            pApiInfo->ParamList[ParameterIndex].bSizeOfPointedDataDefinedByAnotherParameter=FALSE;
            pApiInfo->ParamList[ParameterIndex].pConditionalBreakContent=NULL;
            pApiInfo->ParamList[ParameterIndex].pConditionalLogContent=NULL;
            // if you want to add conditional break or log info use the following as
            //  HookCom.dll Heap can differ from ApiOverride.dll 
            //         --> standard memory allocation (malloc, new) done in HookCom.dll can't be deleted from ApiOverride.dll
            // HookComInfos.CreateParameterConditionalLogContentListIfDoesntExist(&pApiInfo->ParamList[ParameterIndex]);
            // HookComInfos.CreateParameterConditionalBreakContentListIfDoesntExist(&pApiInfo->ParamList[ParameterIndex]);

            // update pApiInfo
            if (!bAlreadyHooked)
                pApiInfo->StackSize+=pApiInfo->ParamList[ParameterIndex].dwSizeOfData;
            pApiInfo->MonitoringParamCount++;

            // increase ParameIndex value
            ParameterIndex++;
        }

        // for each parameter of method
        for (pItemParameterInfo=pMethodInfo->pParameterInfoList->Head;
            (pItemParameterInfo) && (ParameterIndex<MAX_PARAM);
            pItemParameterInfo=pItemParameterInfo->NextItem)
        {
            pParameterInfo=(CParameterInfo*)pItemParameterInfo->ItemData;

            // if parameter has an out direction spying type
            if (pParameterInfo->IsOutParameter())
                // set function spying direction to OUT
                pApiInfo->ParamDirectionType=PARAM_DIR_OUT;

            // get parameter info
            if (pParameterInfo->Name)
            {
#if (defined(UNICODE)||defined(_UNICODE))
                _tcsncpy(pApiInfo->ParamList[ParameterIndex].pszParameterName,pParameterInfo->Name,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE);
                pApiInfo->ParamList[ParameterIndex].pszParameterName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1]=0;
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pParameterInfo->Name,pApiInfo->ParamList[ParameterIndex].pszParameterName,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE);
#endif 
            }
            else
            {
                *pApiInfo->ParamList[ParameterIndex].pszParameterName=0;
            }
            pApiInfo->ParamList[ParameterIndex].dwSizeOfData=pParameterInfo->GetStackSize();
            pApiInfo->ParamList[ParameterIndex].dwSizeOfPointedData=pParameterInfo->GetPointedSize();;
            pApiInfo->ParamList[ParameterIndex].dwType=pParameterInfo->GetWinAPIOverrideType();
            pApiInfo->ParamList[ParameterIndex].bSizeOfPointedDataDefinedByAnotherParameter=FALSE;
            pApiInfo->ParamList[ParameterIndex].pConditionalBreakContent=NULL;
            pApiInfo->ParamList[ParameterIndex].pConditionalLogContent=NULL;
            // if you want to add conditional break or log info use the following as
            //  HookCom.dll Heap can differ from ApiOverride.dll 
            //         --> standard memory allocation (malloc, new) done in HookCom.dll can't be deleted from ApiOverride.dll
            // HookComInfos.CreateParameterConditionalLogContentListIfDoesntExist(&pApiInfo->ParamList[ParameterIndex]);
            // HookComInfos.CreateParameterConditionalBreakContentListIfDoesntExist(&pApiInfo->ParamList[ParameterIndex]);

            if (!bAlreadyHooked)
            {
                // update pApiInfo
                if (pApiInfo->ParamList[ParameterIndex].dwSizeOfData)
                    pApiInfo->StackSize+=pApiInfo->ParamList[ParameterIndex].dwSizeOfData;
                else // pointer or less than REGISTER_BYTE_SIZE
                    // add default register size in byte
                    pApiInfo->StackSize+=REGISTER_BYTE_SIZE;
                pApiInfo->MonitoringParamCount++;
            }

            // increase ParameIndex value
            ParameterIndex++;
        }

        if (!bAlreadyHooked)
        {
            pApiInfo->HookType=HOOK_TYPE_COM;
            pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.ClassID=this->Clsid;
            pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.InterfaceID=this->pInterfaceExposedByIDispatch->Iid;
            pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.VTBLIndex=pMethodInfo->VTBLIndex;

            // hook api function
            if (!HookComInfos.HookAPIFunction(pApiInfo))
            {
                // remove pItemApiInfo associated information
                pMethodInfo->pItemAPIInfo=NULL;

                // free memory associated to item
                HookComInfos.FreeApiInfoItem(pItemApiInfo);

                continue;
            }
            // add ItemApiInfo deletion callback
            if (!pApiInfo->DeletionCallback)
                pApiInfo->DeletionCallback=ApiInfoItemDeletionCallback;
        }
#ifdef _DEBUG
        TCHAR pszOutput[2*MAX_PATH];
        TCHAR tClsid[MAX_PATH];
        CGUIDStringConvert::TcharFromCLSID(&this->Clsid,tClsid);
        _sntprintf(pszOutput,2*MAX_PATH,_T("Monitoring hook installed at address 0x%p for method %s (CLSID:%s dll:%s)\r\n"),
            (PBYTE)pApiInfo->APIAddress,
            pApiInfo->szAPIName,
            tClsid,
            this->AssociatedModuleName);
        pszOutput[2*MAX_PATH-1]=0;
        OutputDebugString(pszOutput);
#endif

        // a method was successfully hooked
        bAtLeastOneHookIsInstalled=TRUE;
    }

    return bAtLeastOneHookIsInstalled;
}

//-----------------------------------------------------------------------------
// Name: AddPostReleaseCallHook
// Object: install callback for the IUnknown::Release method
// Parameters :
//     in  : IUnknown* pInterface : pointer on an interface
//           CHookedInterface* pHookedInterface : associated pHookedInterface
//     out : 
//     return : TRUE on success, FALSE on failure
//-----------------------------------------------------------------------------
BOOL CHookedClass::AddPostReleaseCallHook(IUnknown* pInterface,CHookedInterface* pHookedInterface)
{
    CMethodInfo*   pMethodInfo=NULL;
    CLinkListItem* pItemApiInfo;
    API_INFO*      pApiInfo=NULL;
    BOOL           bReleaseFound;
    BOOL           bAlreadyHooked;
    PBYTE          pReleaseVtblAddress;
    BOOL           pMethodInfoCreatedAndAdded;

    // if no com auto hooking, don't add post hooks
    if (!bCOMAutoHookingEnabled)
        return TRUE;

    if (*(PBYTE*)pInterface==NULL)
        return FALSE;

    bAlreadyHooked=FALSE;
    bReleaseFound=FALSE;
    pItemApiInfo=NULL;

    // use the pIUnknown Release method vtbl
    // Release method is the third in object vtbl
    pReleaseVtblAddress=(*(PBYTE*)pInterface)+HOOK_COM_RELEASE_VTBL_INDEX*sizeof(PBYTE);

    if (IsBadReadPtr(pReleaseVtblAddress,sizeof(PBYTE)))
        return FALSE;

    // get an ApiItem info
    pItemApiInfo=HookComInfos.GetAssociatedItemAPIInfo(pReleaseVtblAddress,&bAlreadyHooked);

    if (!pItemApiInfo)
        return FALSE;

    pApiInfo=(API_INFO*)pItemApiInfo->ItemData;
    if (!pApiInfo)
        return FALSE;

    // if vtbl was not hooked
    if (!bAlreadyHooked)
        HookComInfos.InitializeApiInfo(pApiInfo,this->AssociatedModuleName,_T("IUnknown::Release"));

    // get method info
    pMethodInfo=pHookedInterface->GetMethodInfoFromVTBLAddress(pReleaseVtblAddress);
    pMethodInfoCreatedAndAdded=FALSE;

    // if no method info was found
    // we have to create a CMethodInfo containing data for the Release func
    if (!pMethodInfo)
    {
        // create a fake new CMethodInfo
        pMethodInfo=new CMethodInfo();
        if (!pMethodInfo)
            return FALSE;
        // fill Method info content
        pMethodInfo->pItemAPIInfo=pItemApiInfo;

        pMethodInfo->funckind=FUNC_VIRTUAL;
        pMethodInfo->VTBLIndex=(SHORT)HOOK_COM_RELEASE_VTBL_INDEX;
        pMethodInfo->SetName(_T("IUnknown::Release"));
        pMethodInfo->Address=*((PBYTE*)pReleaseVtblAddress);
        pMethodInfo->VTBLAddress=pReleaseVtblAddress;

        pHookedInterface->pMethodInfoList->AddItem(pMethodInfo);
        pMethodInfoCreatedAndAdded=TRUE;
        // don't call SetListOfBaseInterfaces as pMethodInfoList->pLinkListOfBaseInterfacesID is already empty
    }

    // fill pApiInfo data
    pApiInfo->HookType=HOOK_TYPE_COM;
    pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.ClassID=this->Clsid;
    pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.InterfaceID=IID_IUnknown;
    pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.VTBLIndex=2;
    pApiInfo->StackSize=StackSizeOf(IUnknown*);
    pApiInfo->bFunctionPointer=TRUE;
    pApiInfo->APIAddress=(FARPROC)pReleaseVtblAddress;

    pHookedInterface->pReleaseHookInfo=pItemApiInfo;

    if (!HookComInfos.AddPreApiCallCallBack(pItemApiInfo,!bAlreadyHooked,(HMODULE)DllhInstance,(pfPreApiCallCallBack) CHookedClass::PreReleaseCallHook,this))
    {
        pHookedInterface->pReleaseHookInfo=NULL;
        if (pMethodInfoCreatedAndAdded)
        {
            pHookedInterface->pMethodInfoList->RemoveItemFromItemData(pMethodInfo);
            delete pMethodInfo;
        }
        return FALSE;
    }
    if (!HookComInfos.AddPostApiCallCallBack(pItemApiInfo,FALSE,(HMODULE)DllhInstance,(pfPostApiCallCallBack) CHookedClass::PostReleaseCallHook,this))
    {
        HookComInfos.RemovePreApiCallCallBack(pHookedInterface->pReleaseHookInfo,(pfPreApiCallCallBack)CHookedClass::PreReleaseCallHook,TRUE);
        pHookedInterface->pReleaseHookInfo=NULL;
        if (pMethodInfoCreatedAndAdded)
        {
            pHookedInterface->pMethodInfoList->RemoveItemFromItemData(pMethodInfo);
            delete pMethodInfo;
        }
        return FALSE;
    }

#ifdef _DEBUG
    TCHAR pszOutput[2*MAX_PATH];
    TCHAR tClsid[MAX_PATH];
    CGUIDStringConvert::TcharFromCLSID(&this->Clsid,tClsid);
    _sntprintf(pszOutput,2*MAX_PATH,_T("Release post hook installed at address 0x%p (CLSID:%s dll:%s)\r\n"),
        (PBYTE)pApiInfo->APIAddress,
        tClsid,
        this->AssociatedModuleName);
    pszOutput[2*MAX_PATH-1]=0;
    OutputDebugString(pszOutput);
#endif

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: AddPostQueryInterfaceCallHook
// Object: install callback for the IUnknown::QueryInterface method
// Parameters :
//     in  : IUnknown* pInterface : pointer on an interface
//           CHookedInterface* pHookedInterface : associated pHookedInterface
//     out : 
//     return : TRUE on success, FALSE on failure
//-----------------------------------------------------------------------------
BOOL CHookedClass::AddPostQueryInterfaceCallHook(IUnknown* pInterface,CHookedInterface* pHookedInterface)
{
    CMethodInfo*   pMethodInfo=NULL;
    CLinkListItem* pItemApiInfo;
    API_INFO*      pApiInfo=NULL;
    BOOL           bReleaseFound;
    BOOL           bAlreadyHooked;
    PBYTE          pQueryInterfaceVtblAddress;
    BOOL           pMethodInfoCreatedAndAdded;

    // if no com auto hooking, don't add post hooks
    if (!bCOMAutoHookingEnabled)
        return TRUE;

    if (*(PBYTE*)pInterface==NULL)
        return FALSE;

    bAlreadyHooked=FALSE;
    bReleaseFound=FALSE;
    pItemApiInfo=NULL;

    // use the pIUnknown QueryInterface method vtbl
    // Release method is the third in object vtbl
    pQueryInterfaceVtblAddress=(*(PBYTE*)pInterface)+HOOK_COM_QUERY_INTERFACE_VTBL_INDEX*sizeof(PBYTE);

    if (IsBadReadPtr(pQueryInterfaceVtblAddress,sizeof(PBYTE)))
        return FALSE;

    // get an ApiItem info
    pItemApiInfo=HookComInfos.GetAssociatedItemAPIInfo(pQueryInterfaceVtblAddress,&bAlreadyHooked);

    if (!pItemApiInfo)
        return FALSE;

    pApiInfo=(API_INFO*)pItemApiInfo->ItemData;
    if (!pApiInfo)
        return FALSE;

    // if vtbl was not hooked
    if (!bAlreadyHooked)
        HookComInfos.InitializeApiInfo(pApiInfo,this->AssociatedModuleName,_T("IUnknown::QueryInterface"));

    // get method info
    pMethodInfo=pHookedInterface->GetMethodInfoFromVTBLAddress(pQueryInterfaceVtblAddress);
    pMethodInfoCreatedAndAdded=FALSE;

    // if no method info was found
    // we have to create a CMethodInfo containing data for the QueryInterface func
    if (!pMethodInfo)
    {

        // create a fake new CMethodInfo
        pMethodInfo=new CMethodInfo();
        if (!pMethodInfo)
            return FALSE;

        // fill Method info content
        pMethodInfo->pItemAPIInfo=pItemApiInfo;

        pMethodInfo->funckind=FUNC_VIRTUAL;
        pMethodInfo->VTBLIndex=(SHORT)HOOK_COM_RELEASE_VTBL_INDEX;
        pMethodInfo->SetName(_T("IUnknown::QueryInterface"));
        pMethodInfo->Address=*((PBYTE*)pQueryInterfaceVtblAddress);
        pMethodInfo->VTBLAddress=pQueryInterfaceVtblAddress;

        pHookedInterface->pMethodInfoList->AddItem(pMethodInfo);
        pMethodInfoCreatedAndAdded=TRUE;

        // don't call SetListOfBaseInterfaces as pMethodInfoList->pLinkListOfBaseInterfacesID is already empty
    }

    // fill pApiInfo data
    pApiInfo->HookType=HOOK_TYPE_COM;
    pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.ClassID=this->Clsid;
    pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.InterfaceID=IID_IUnknown;
    pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.VTBLIndex=0;
    pApiInfo->StackSize=StackSizeOf(IUnknown*)+StackSizeOf(IID*)+StackSizeOf(void **);
    pApiInfo->bFunctionPointer=TRUE;
    pApiInfo->APIAddress=(FARPROC)pQueryInterfaceVtblAddress;
    pHookedInterface->pQueryInterfaceHookInfo=pItemApiInfo;
    if (!HookComInfos.AddPostApiCallCallBack(pItemApiInfo,!bAlreadyHooked,(HMODULE)DllhInstance,(pfPostApiCallCallBack) CHookedClass::PostQueryInterfaceCallHook,this))
    {
        pHookedInterface->pQueryInterfaceHookInfo=NULL;
        if (pMethodInfoCreatedAndAdded)
        {
            pHookedInterface->pMethodInfoList->RemoveItemFromItemData(pMethodInfo);
            delete pMethodInfo;
        }
        return FALSE;
    }

#ifdef _DEBUG
    TCHAR pszOutput[2*MAX_PATH];
    TCHAR tClsid[MAX_PATH];
    CGUIDStringConvert::TcharFromCLSID(&this->Clsid,tClsid);
    _sntprintf(pszOutput,2*MAX_PATH,_T("QueryInterface post hook installed at address 0x%p (CLSID:%s dll:%s)\r\n"),
        (PBYTE)pApiInfo->APIAddress,
        tClsid,
        this->AssociatedModuleName);
    pszOutput[2*MAX_PATH-1]=0;
    OutputDebugString(pszOutput);
#endif

    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: PostQueryInterfaceCallHook
// Object: callback called when the IUnknown::QueryInterface method was called
//          (used to auto hook new queried interfaces)
// Parameters :
//     in  : 
//     out : 
//     return : TRUE to continue post api call hook chain, FALSE to stop it
//-----------------------------------------------------------------------------
BOOL __stdcall CHookedClass::PostQueryInterfaceCallHook(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)
{
    UNREFERENCED_PARAMETER(pHookInfos);
    // Release method is like
    // ULONG STDMETHODCALLTYPE QueryInterface(IUnknown* pObject,IID* , void **ppvObj)

    IID* pIID;
    IUnknown* pIUnknown;
    IUnknown** ppInterface;
    CHookedClass* pHookedClass;


    // return is in eax
    // if QueryInterface failed, do nothing
    if (FAILED(pAfterCallRegisters->eax))
        // continue post call chain
        return TRUE;

    if (IsBadReadPtr(UserParam,sizeof(CHookedClass)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call chain
        return TRUE;
    }

    // get IUnknown from stack
    pIUnknown=*(IUnknown**)pEspArgs;

    // get IID from stack
    pIID=*(IID**)(pEspArgs+sizeof(PBYTE));
    if (IsBadReadPtr(pIID,sizeof(IID)))
    {
#ifdef _DEBUG
        if (pIID!=NULL)// seems to appear lot of time
        {
            if (IsDebuggerPresent())// avoid to crash application if no debugger
                DebugBreak();
        }
#endif
        // continue post call chain
        return TRUE;
    }

    // get interface from stack
    ppInterface=*(IUnknown***)(pEspArgs+2*sizeof(PBYTE));
    if (IsBadReadPtr(ppInterface,sizeof(IUnknown*)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call chain
        return TRUE;
    }

    pHookedClass=(CHookedClass*)UserParam;
    pHookedClass->QueryInterfaceCallBack(pIUnknown,*ppInterface,pIID);

    // continue post call chain
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: PostReleaseCallHook
// Object: callback called when the IUnkown::Release method of the class was called
// Parameters :
//     in  : 
//     out : 
//     return : TRUE to continue post api call hook chain, FALSE to stop it
//-----------------------------------------------------------------------------
BOOL __stdcall CHookedClass::PreReleaseCallHook(PBYTE pEspArgs,REGISTERS* pBeforeCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)
{
    UNREFERENCED_PARAMETER(pEspArgs);
    UNREFERENCED_PARAMETER(pBeforeCallRegisters);
    UNREFERENCED_PARAMETER(pHookInfos);

    CHookedClass* pHookedClass;
    if (IsBadReadPtr(UserParam,sizeof(CHookedClass)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue pre call chain
        return TRUE;
    }
    pHookedClass=(CHookedClass*)UserParam;

    // avoid the dll unload watching thread to report dll unload during the Release call
    // has if PreCallHook occurs, associated PostCallHook will occurs (except when Release crashs)
    pHookedClass->NbReleaseProcessing++;

    // continue pre call chain
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: PostReleaseCallHook
// Object: callback called when the IUnkown::Release method of the class was called
// Parameters :
//     in  : 
//     out : 
//     return : TRUE to continue post api call hook chain, FALSE to stop it
//-----------------------------------------------------------------------------
BOOL __stdcall CHookedClass::PostReleaseCallHook(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam)
{
    UNREFERENCED_PARAMETER(pHookInfos);
    // Release method is like
    // ULONG STDMETHODCALLTYPE Release(IUnknown* pObject)

    CHookedClass* pHookedClass;
    IUnknown* pIUnknown;

    if (IsBadReadPtr(UserParam,sizeof(CHookedClass)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())// avoid to crash application if no debugger
            DebugBreak();
#endif
        // continue post call chain
        return TRUE;
    }
    pHookedClass=(CHookedClass*)UserParam;

    // return is in eax
    // Release method return the number of reference counted
    // at soon as it's return is 0, object is destroyed

    // if there's still reference to object
    if (pAfterCallRegisters->eax>0)
    {
        // continue post call chain
        pHookedClass->NbReleaseProcessing--;
        return TRUE;
    }

    // get IUnknown from stack
    pIUnknown=*(IUnknown**)pEspArgs;

    pHookedClass->ReleaseCallBack(pIUnknown);

    // If no more object of this class type exist, 
    // the best way should to spy dll unloading of dll containing object code.
    // we could use IInitializeSpy class for each thread to spy CoUninitialize call,
    // add a hook on CoFreeUnusedLibraries, 
    // but will still can miss some dll unload if user make direct call on DllCanUnloadNow.
    // so a more secure way is to unhook a class as soon as it contains
    // no  more object and redo a parsing when a new object is created.
    // But unfortunately, as CoUninitialize unload dll without caring if all objects are released (DllCanUnloadNow return S_FALSE)
    // and IInitializeSpy is reserved for XP SP2 and Win server 2003 and later
    
    // --> So at each creation of object we have to check if hooks are still in memory (dll never unloaded)
    // or not (dll unloaded and reloaded)

    pHookedClass->NbReleaseProcessing--;
    // continue post call chain
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Unhook
// Object: remove all previously installed hooks
//          and remove all hooked objects from this->pLinkListHookedObjects
// Parameters :
//     in  : CHookedObject* pHookedObject : pointer to CHookedObject object
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHookedClass::Unhook()
{
    return this->Unhook(FALSE);
}

//-----------------------------------------------------------------------------
// Name: Unhook
// Object: remove all previously installed hooks
//          and remove all hooked objects from this->pLinkListHookedObjects
// Parameters :
//     in  : CHookedObject* pHookedObject : pointer to CHookedObject object
//           BOOL bLinkListHookedClassesLocked : TRUE if global pLinkListHookedClassesLocked
//                                              link list is currently locked
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHookedClass::Unhook(BOOL bLinkListHookedClassesLocked)
{
    API_INFO* pApiInfo;
    CHookedInterface* pInterface;
    CMethodInfo* pMethodInfo;
    CLinkListItem* pItemInterface;
    CLinkListItem* pNextItemInterface;
    CLinkListItem* pItemMethod;
    CLinkListItem* pNextItemMethod;
    BOOL bHooksStillInstalled=TRUE;

    // if associated dll is unloaded
    if (!this->IsAssociatedDllLoaded())
        bHooksStillInstalled=FALSE;
    else
    {
        // if associated dll is at another address space (dll has been unloaded and reloaded) 
        if(!this->IsAssociatedDllInSameSpaceAddress())
            bHooksStillInstalled=FALSE;
        else
        {
            if (!this->AreHooksStillInstalled())
                bHooksStillInstalled=FALSE;
        }
    }

    // free hooked objects
    CHookedObject* pHookedObject=NULL;
    CLinkListItem* pItem;

    this->pLinkListHookedObjects->Lock();
    // for each object 
    for(pItem=this->pLinkListHookedObjects->Head;pItem;pItem=pItem->NextItem)
    {
        pHookedObject=(CHookedObject*)pItem->ItemData;
        pHookedObject->IsGoingToBeDestroyed=TRUE;

        if(!bHooksStillInstalled)
        {
            // report bad release
            this->ReportNotReleasedObject(pHookedObject);
        }
        else
        {
            if (IsBadReadPtr(pHookedObject->pObject,sizeof(IUnknown)))
                // report bad release
                this->ReportNotReleasedObject(pHookedObject);
        }

        // free memory
        delete pHookedObject;
    }
    this->pLinkListHookedObjects->RemoveAllItems(TRUE);
    this->pLinkListHookedObjects->Unlock();

 
    // for each interface
    this->pLinkListHookedInterfaces->Lock();
    for (pItemInterface=this->pLinkListHookedInterfaces->Head;pItemInterface;pItemInterface=pNextItemInterface)
    {
        pNextItemInterface=pItemInterface->NextItem;

        pInterface=(CHookedInterface*)pItemInterface->ItemData;
        if (!pInterface)
            continue;

        pInterface->pMethodInfoList->Lock();
        // for each method
        for (pItemMethod=pInterface->pMethodInfoList->Head;pItemMethod;pItemMethod=pNextItemMethod)
        {
            pNextItemMethod=pItemMethod->NextItem;

            pMethodInfo=(CMethodInfo*)pItemMethod->ItemData;
            if (!pMethodInfo)
            {
                pInterface->pMethodInfoList->RemoveItem(pItemMethod,TRUE);
                continue;
            }
            
            // if method is not hooked
            if (IsBadReadPtr(pMethodInfo->pItemAPIInfo,sizeof(CLinkListItem)))
            {
                pInterface->pMethodInfoList->RemoveItem(pItemMethod,TRUE);
                continue;
            }

            // get hook info struct
            pApiInfo=(API_INFO*)pMethodInfo->pItemAPIInfo->ItemData;
            if (IsBadWritePtr(pApiInfo,sizeof(API_INFO)))
            {
                delete pMethodInfo;
                pInterface->pMethodInfoList->RemoveItem(pItemMethod,TRUE);
                continue;
            }
            if (IsBadReadPtr(pApiInfo->szAPIName,sizeof(TCHAR)))
            {
                delete pMethodInfo;
                pInterface->pMethodInfoList->RemoveItem(pItemMethod,TRUE);
                continue;
            }

            // if pApiInfo contains monitoring information (not only pre/post hook infos)
            if (pApiInfo->pMonitoringFileInfos)
            {
                // multiple class / interface can point to the same hooked object
                // using this, only last interface will free hooked item
                if (!IsMethodHookSharedWithAnotherInterface(pMethodInfo,pInterface,this,bLinkListHookedClassesLocked))
                {
                    // // debug purpose only to check that a pItemAPIInfo is not free twice
                    //TCHAR FreeMem[100];
                    //_stprintf(FreeMem,_T("api info free : %p\r\n"),pMethodInfo->pItemAPIInfo);
                    //OutputDebugString(FreeMem);

                    // dissociate pApiInfo from hook com dll by reseting it's pMonitoringFileInfos field
                    pApiInfo->pMonitoringFileInfos=NULL;

                    // Important : remove pApiInfo->DeletionCallback
                    //             it's avoid to call ApiInfoItemDeletionCallback which will cause a deadlock
                    pApiInfo->DeletionCallback=NULL;

                    // remove hook if possible
                    // always try to restore original opcodes hook even if !bHooksStillInstalled in case of bad detection (else program will surely crash)
                    HookComInfos.UnHookIfPossible(pMethodInfo->pItemAPIInfo,TRUE);
                }
            }

            // dissociate hook from method
            pMethodInfo->pItemAPIInfo=NULL;

            delete pMethodInfo;
            pInterface->pMethodInfoList->RemoveItem(pItemMethod,TRUE);
        }
        pInterface->pMethodInfoList->Unlock();


        // pre/post hooks must be removed ONLY AFTER ALL MONITORING HOOKS HAVE BEEN REMOVED,
        // because if a pre/post method is not monitored, pMethodInfo->pItemAPIInfo will be deleted but
        // pMethodInfo->pItemAPIInfo won't be put to NULL, and we will try to remove an already deleted item

        // release post Release and QueryInterface
        if (pInterface->pReleaseHookInfo)
        {
            // Notice : UnHookIfPossible is called internally by RemovePreApiCallCallBack / RemovePostApiCallCallBack

            // always try to restore original opcodes even if !bHooksStillInstalled in case of bad detection (else program will surely crash)
            // try to restore original opcode in case AddPostApiCallCallBack has failed for PostReleaseCallHook in this case hook can be hold only by PreReleaseCallHook
            HookComInfos.RemovePreApiCallCallBack(pInterface->pReleaseHookInfo,(pfPreApiCallCallBack)CHookedClass::PreReleaseCallHook,TRUE);
            // always try to restore original opcodes even if !bHooksStillInstalled in case of bad detection (else program will surely crash)
            HookComInfos.RemovePostApiCallCallBack(pInterface->pReleaseHookInfo,(pfPostApiCallCallBack)CHookedClass::PostReleaseCallHook,TRUE);
            pInterface->pReleaseHookInfo=NULL;
        }
        if (pInterface->pQueryInterfaceHookInfo)
        {
            // Notice : UnHookIfPossible is called internally by RemovePostApiCallCallBack

            // always try to restore original opcodes even if !bHooksStillInstalled in case of bad detection (else program will surely crash)
            HookComInfos.RemovePostApiCallCallBack(pInterface->pQueryInterfaceHookInfo,(pfPostApiCallCallBack)CHookedClass::PostQueryInterfaceCallHook,TRUE);
            pInterface->pQueryInterfaceHookInfo=NULL;
        }

        // delete pInterface
        delete pInterface;
        this->pLinkListHookedInterfaces->RemoveItem(pItemInterface,TRUE);
    }
    
    this->pLinkListHookedInterfaces->Unlock();
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ParseCOMMonitoringFileLine
// Object: parse a line of a Com monitoring file
// Parameters :
//     in  : TCHAR* pszFileName
//           DWORD dwLineNumber
//           TCHAR* Line : content of the line
//           LPVOID UserParam : pointer to a PARSE_COM_MONITORING_FILE_LINE_PARAM struct
//     out : 
//     return : TRUE to continue file parsing, FALSE to stop it
//-----------------------------------------------------------------------------
BOOL CHookedClass::ParseCOMMonitoringFileLine(TCHAR* pszFileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam)
{
    BOOL bParsingError;
    TCHAR* pszFunctionDefinition;
    HOOK_DEFINITION_INFOS HookDefinitionInfos;
    PARSE_COM_MONITORING_FILE_LINE_PARAM* pParam;

    pParam=(PARSE_COM_MONITORING_FILE_LINE_PARAM*)UserParam;

    Line=CTrimString::TrimString(Line);

    // if empty or commented line
    if ((*Line==0)||(*Line==';')||(*Line=='!'))
        // continue parsing
        return TRUE;

    // line is like 
    // vtbl hooking info|function definition|optional parameters

    // find first splitter (between vtbl hooking info definition and function definition)
    pszFunctionDefinition=_tcschr(Line,FIELDS_SEPARATOR);

    if (pszFunctionDefinition)
    {
        // ends line --> line contain only vtbl hooking info
        *pszFunctionDefinition=0;
        // point to function definition
        pszFunctionDefinition++;
    }


    // get vtbl hooking info definition
    if(!GetHookDefinitionInfo(Line,pParam->pIid,&HookDefinitionInfos))
    {
        return ReportParsingError(pszFileName,dwLineNumber);
    }

    // if we need a full iid spying, the matching IID file needs to be loaded
    if (HookDefinitionInfos.VTBLInfoType==VTBL_INFO_TYPE_OBJECT_FULL_IID)
    {
        ////////////////////////////////
        // stack overflow prevention :
        // check that iid is not the current one and don't belong to pParam->pLinkListOfBaseInterfacesID 
        ////////////////////////////////
        BOOL FileAlreadyLoaded=FALSE;

        if (pParam->pLinkListOfBaseInterfacesID)
        {
            CLinkListItem* pItem;
            for(pItem=pParam->pLinkListOfBaseInterfacesID->Head;pItem;pItem=pItem->NextItem)
            {
                if (IsEqualIID(HookDefinitionInfos.InterfaceID,*((IID*)pItem->ItemData)))
                {
                    FileAlreadyLoaded=TRUE;
                    break;
                }
            }
        }
        if (FileAlreadyLoaded)
        {
            TCHAR szMsg[2*MAX_PATH];
            _stprintf(szMsg,_T("Loop detected in COM Monitoring file %s at line %u"),pszFileName,dwLineNumber);
            HookComInfos.DynamicMessageBoxInDefaultStation(NULL,szMsg,_T("Error"),MB_OK|MB_ICONERROR);
            // continue parsing
            return TRUE;
        }

        ////////////////////////////////
        // iid is ok parse associated file
        ////////////////////////////////
        CLinkList* mpLinkListOfBaseInterfacesID;
        if (pParam->pLinkListOfBaseInterfacesID)
            mpLinkListOfBaseInterfacesID=pParam->pLinkListOfBaseInterfacesID;
        else
            mpLinkListOfBaseInterfacesID=new CLinkList(sizeof(IID));

        CLinkListItem* pItem=mpLinkListOfBaseInterfacesID->AddItem(&HookDefinitionInfos.InterfaceID);
        pParam->pHookedClass->SetMonitoringHookFromFile(
                                                        pParam->pHookedObject,
                                                        pParam->pInterfaceAssociatedToIID,
                                                        pParam->pIid,
                                                        &HookDefinitionInfos.InterfaceID,
                                                        mpLinkListOfBaseInterfacesID,
                                                        pParam->HookQueriedByAutoMonitoring,
                                                        pParam->SpecifiedpMonitoringFileInfo,
                                                        &pParam->pAlreadyHookingMonitoringFileInfo);
        mpLinkListOfBaseInterfacesID->RemoveItem(pItem);

        if (!pParam->pLinkListOfBaseInterfacesID)
            delete mpLinkListOfBaseInterfacesID;

        // continue parsing
        return TRUE;
    }

    if (!pszFunctionDefinition)
    {
        return ReportParsingError(pszFileName,dwLineNumber);
    }

    MONITORING_FILE_INFOS* pMonitoringFileInfo;
    // if monitoring file info has been forced
    if (pParam->SpecifiedpMonitoringFileInfo)
        pMonitoringFileInfo=pParam->SpecifiedpMonitoringFileInfo;
    else
        pMonitoringFileInfo=pParam->pHookedClass->GetOrCreateMonitoringFileInfo(pszFileName);

    // add hook from definition
    if (!pParam->pHookedClass->AddMonitoringHookForObjectMethod(pParam->pHookedObject,
                                                                pParam->pInterfaceAssociatedToIID,
                                                                pParam->pLinkListOfBaseInterfacesID,
                                                                pMonitoringFileInfo,
                                                                pParam->HookQueriedByAutoMonitoring,
                                                                &HookDefinitionInfos,
                                                                pszFunctionDefinition,
                                                                pszFileName,
                                                                dwLineNumber,
                                                                &pParam->pAlreadyHookingMonitoringFileInfo,
                                                                &bParsingError)
        )
    {
        if (bParsingError)
        {
            return ReportParsingError(pszFileName,dwLineNumber);
        }
    }
    else
    {
        pParam->bAtLeastOneMethodHasBeenHooked=TRUE;
    }
    // continue parsing
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: GetMethodInfoForHook
// Object: return method info associated to the COM function to hook according to pHookDefinitionInfos
// Parameters :
//     in  : CHookedObject* pHookedObject : an object associated with COM object
//           IUnknown* pInterfaceAssociatedToIID : pointer to interface associated to IID
//           HOOK_DEFINITION_INFOS* pHookDefinitionInfos : com hook informations
//           TCHAR* FunctionName : function name
//           TCHAR* pszFileName : for error report only
//           DWORD dwLineNumber : for error report only (line number or faking dll array index)
//           tagHookType HookType : for error report only
//           BOOL bAutoHook : TRUE if hook is beiing install by auto hook
//                            FALSE for static hooks (if MethodInfo is going to be destroyed after function return
//                            and must not be managed by pMethodInfoList)
//     out : CMethodInfo** ppMethodInfo : method associated to the pHookDefinitionInfos
//           BOOL* pbMethodInfoWasAlreadyExisting : TRUE if MethodInfo was already existing and muns't
//                                                  be destroyed even if TempMethodInfo
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CHookedClass::GetMethodInfoForHook(CHookedObject* pHookedObject,
                                        IUnknown* pInterfaceAssociatedToIID,
                                        CLinkList* pLinkListOfBaseInterfacesID,
                                          HOOK_DEFINITION_INFOS* pHookDefinitionInfos,
                                          TCHAR* FunctionName,
                                          TCHAR* pszFileName,
                                          DWORD dwLineNumber,
                                          tagHookType HookType,
                                          BOOL bAutoHook,
                                          OUT CMethodInfo** ppMethodInfo,
                                          OUT BOOL* pbMethodInfoWasAlreadyExisting)
{
    TCHAR pszMsg[3*MAX_PATH];
    CHookedInterface* pHookedInterface;
    // by default method info is not locally created
    *pbMethodInfoWasAlreadyExisting=TRUE;
    (*ppMethodInfo)=NULL;
    if (IsBadReadPtr(pInterfaceAssociatedToIID,sizeof(IUnknown)))
        return FALSE;

    if (pHookDefinitionInfos->VTBLInfoType==VTBL_INFO_TYPE_OBJECT_IID)
    {
        IUnknown* pInterfaceID;
 
        if (pInterfaceAssociatedToIID)
            pInterfaceID=pInterfaceAssociatedToIID;
        else
        {
            pInterfaceID=NULL;
            if (FAILED(CSecureIUnknown::QueryInterface(pInterfaceAssociatedToIID,pHookDefinitionInfos->InterfaceID,(void**)&pInterfaceID)))
                return FALSE;
            if (IsBadReadPtr(pInterfaceID,sizeof(IUnknown)))
                return FALSE;

            pHookedObject->AddInterfacePointer(pInterfaceID,&pHookDefinitionInfos->InterfaceID);

            CSecureIUnknown::Release(pInterfaceID);
        }

        pHookedInterface=this->GetOrCreateHookedInterface(pInterfaceID,&pHookDefinitionInfos->InterfaceID);
        if(!pHookedInterface)
            return FALSE;        

        // search if method is not already known
        // by checking existing methods vtbl addresses
        PBYTE VTBLAddress=(*(PBYTE*)pInterfaceID)+(pHookDefinitionInfos->VTBLIndex*sizeof(PBYTE));

        // find CMethodInfo* associated to function description
        *ppMethodInfo=pHookedInterface->GetMethodInfoFromVTBLAddress(VTBLAddress);

        if (*ppMethodInfo==NULL)
        {
            // create a new CMethodInfo and fill necessary informations
            *ppMethodInfo=new CMethodInfo();
            if(!*ppMethodInfo)
                return FALSE;
            // new has been called successfully
            *pbMethodInfoWasAlreadyExisting=FALSE;

            // fill ppMethodInfo necessary informations (not all)
            (*ppMethodInfo)->funckind=FUNC_VIRTUAL;
            (*ppMethodInfo)->VTBLIndex=(SHORT)pHookDefinitionInfos->VTBLIndex;
            (*ppMethodInfo)->SetName(FunctionName);
            //this->VTBLAddress=pInterfaceID+pFuncDesc->oVft;
            (*ppMethodInfo)->VTBLAddress=(*(PBYTE*)pInterfaceID)+(pHookDefinitionInfos->VTBLIndex*sizeof(PBYTE));
            if (IsBadReadPtr((*ppMethodInfo)->VTBLAddress,sizeof(PBYTE)))
            {
                // report error
                if (pszFileName && FunctionName)
                {
                    _stprintf(pszMsg,_T("Possible bad VTBL index (%u) for function %s in file %s at line %u."),pHookDefinitionInfos->VTBLIndex,FunctionName,pszFileName,dwLineNumber);
                    HookComInfos.ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);
                }
#ifdef _DEBUG
                if (IsDebuggerPresent())// avoid to crash application if no debugger
                    DebugBreak();
#endif
                (*ppMethodInfo)->Address=0;
            }
            else
            {
                (*ppMethodInfo)->Address=*((PBYTE*)(*ppMethodInfo)->VTBLAddress);
                if (IsBadCodePtr((FARPROC)(*ppMethodInfo)->Address))
                {
                    // report error
                    if (pszFileName && FunctionName)
                    {
                        _stprintf(pszMsg,_T("Possible bad VTBL index (%u) for function %s in file %s at line %u."),pHookDefinitionInfos->VTBLIndex,FunctionName,pszFileName,dwLineNumber);
                        HookComInfos.ReportMessage(REPORT_MESSAGE_ERROR,pszMsg);
                    }
#ifdef _DEBUG
                    if (IsDebuggerPresent())// avoid to crash application if no debugger
                        DebugBreak();
#endif
                    (*ppMethodInfo)->Address=0;
                }
            }
            // don't worry about pMethodInfo memory management, pHookedObject destructor will remove memory
            // even if not hooked (in this case, as pMethodInfo->pApiItemInfo is set to NULL, there won't be error)
            (*ppMethodInfo)->pItemAPIInfo=NULL;

            if (bAutoHook)
            {
                // add method to list
                pHookedInterface->pMethodInfoList->AddItem(*ppMethodInfo);
            }
        }

        // always call SetListOfBaseInterfaces, MethodInfo will check for the best LinkList to keep
        (*ppMethodInfo)->SetListOfBaseInterfaces(pLinkListOfBaseInterfacesID);
    }
    else // VTBL_INFO_TYPE_EXPOSED_THROW_IDISPATCH --> use of method name found thanks to the IDispatch method
    {
        CLinkListItem* pItemMethodInfo;
        CMethodInfo* pMethodInfo;

        if(!this->ParseIDispatch(pInterfaceAssociatedToIID,&pHookDefinitionInfos->InterfaceID))
            return FALSE;

        // get the Interface associated to IDispatch parsing result
        pHookedInterface=this->pInterfaceExposedByIDispatch;

        if(!pHookedInterface)
            return FALSE;

#if ((!defined(UNICODE))&& (!defined(_UNICODE)))
        WCHAR* pwFunctionName;
        CAnsiUnicodeConvert::AnsiToUnicode(FunctionName,&pwFunctionName);
#endif

        // find CMethodInfo* associated to function description
        for (pItemMethodInfo=pHookedInterface->pMethodInfoList->Head;pItemMethodInfo;pItemMethodInfo=pItemMethodInfo->NextItem)
        {
            pMethodInfo=(CMethodInfo*)pItemMethodInfo->ItemData;
            if (IsBadReadPtr(pMethodInfo,sizeof(CMethodInfo)))
                continue;
            if(!pMethodInfo->Name)
                continue;
#if (defined(UNICODE)||defined(_UNICODE))
            if (_tcsicmp(pMethodInfo->Name,FunctionName)==0)
#else
            if (wcsicmp(pMethodInfo->Name,pwFunctionName)==0)
#endif
            {
                // get pointer to method info object
                (*ppMethodInfo)=pMethodInfo;
                break;
            }
        }
#if ((!defined(UNICODE))&& (!defined(_UNICODE)))
        free(pwFunctionName);
#endif

        if ((*ppMethodInfo)==NULL)
        {
            if (pszFileName)
            {

                switch (HookType)
                {
                case HOOK_TYPE_MONITORING:
                    _sntprintf(pszMsg,3*MAX_PATH,_T("Method %s defined at line %u in %s not found for corresponding interface"),FunctionName,dwLineNumber,pszFileName);
                    break;
                case HOOK_TYPE_FAKING:
                    _sntprintf(pszMsg,3*MAX_PATH,_T("Method %s defined in faking array at index %u in %s not found for corresponding interface"),FunctionName,dwLineNumber,pszFileName);
                    break;
                case HOOK_TYPE_PRE_API_CALL:
                    _sntprintf(pszMsg,3*MAX_PATH,_T("Method %s defined in pre call array at index %u in %s not found for corresponding interface"),FunctionName,dwLineNumber,pszFileName);
                    break;
                case HOOK_TYPE_POST_API_CALL:
                    _sntprintf(pszMsg,3*MAX_PATH,_T("Method %s defined in post call array at index %u in %s not found for corresponding interface"),FunctionName,dwLineNumber,pszFileName);
                    break;
                }
            }
            else
                _sntprintf(pszMsg,3*MAX_PATH,_T("Method %s not found for corresponding interface"),FunctionName);

            HookComInfos.DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("Error"),MB_YESNO|MB_ICONERROR);

            return FALSE;
        }
    }

    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: GetOrCreateMonitoringFileInfo
// Object: retrieve or create a MONITORING_FILE_INFOS struct associated to pszFileName
// Parameters :
//     in  : TCHAR* pszMonitoringFileName : monitoring file name
//     return : MONITORING_FILE_INFOS* associated to pszFileName
//-----------------------------------------------------------------------------
MONITORING_FILE_INFOS* CHookedClass::GetOrCreateMonitoringFileInfo(TCHAR* pszMonitoringFileName)
{
    if (pLinkListMonitoringFileInfo==NULL)
        return NULL;

    MONITORING_FILE_INFOS* pMonitoringFileInfo=NULL;

    CLinkListItem* pItem;
    // search throw existing pMonitoringFileInfo in pLinkListMonitoringFileInfo
    for (pItem=pLinkListMonitoringFileInfo->Head;pItem;pItem=pItem->NextItem)
    {
        pMonitoringFileInfo=(MONITORING_FILE_INFOS*)pItem->ItemData;
        // if item has been found
        if (_tcsicmp(pMonitoringFileInfo->szFileName,pszMonitoringFileName)==0)
        {
            // return
            return pMonitoringFileInfo;
        }
    }
    // item has not been found : we have to create one associated struct
    MONITORING_FILE_INFOS MonitoringFileInfo;
    _tcsncpy(MonitoringFileInfo.szFileName,pszMonitoringFileName,MAX_PATH);
    MonitoringFileInfo.szFileName[MAX_PATH-1]=0;
    // add struct to list
    pItem=pLinkListMonitoringFileInfo->AddItem(&MonitoringFileInfo);
    if (!pItem)
        return NULL;
    // return permanent memory pointer on MONITORING_FILE_INFOS struct
    return (MONITORING_FILE_INFOS*)pItem->ItemData;
}

//-----------------------------------------------------------------------------
// Name: AddMonitoringHookForObjectMethod
// Object: parse a line of a Com monitoring file
// Parameters :
//     in  : CHookedObject* pHookedObject : object associated with COM object
//           IUnknown* pInterface : pointer to interface of specified IID
//           MONITORING_FILE_INFOS* pMonitoringFileInfo : pMonitoringFileInfo for API_INFO item 
//           BOOL bAutoHook : TRUE if hook is queried by auto hook
//                            static com loading MUST put this field to FALSE (else troubles will
//                            appear after static file has been unloaded because pMethodInfo->pItemAPIInfo will
//                            point to a destroyed item)
//           HOOK_DEFINITION_INFOS* pHookDefinitionInfos : com hook informations
//           TCHAR* pszFunctionDescription : full function description
//           TCHAR* pszFileName
//           DWORD dwLineNumber
//     out : MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo : contains monitoring file information owning the hook
//                                                                        if hook is already install
//                                                                        NULL if function wasn't already hooked
//           BOOL* pbParsingError : TRUE in case of parsing error
//     return : TRUE if method is hooked (already hooked or not)
//-----------------------------------------------------------------------------
BOOL CHookedClass::AddMonitoringHookForObjectMethod(CHookedObject* pHookedObject,
                                                    IUnknown* pInterfaceAssociatedToIID,
                                                    CLinkList* pLinkListOfBaseInterfacesID,
                                                    MONITORING_FILE_INFOS* pMonitoringFileInfo,
                                                    BOOL bAutoHook,
                                                    HOOK_DEFINITION_INFOS* pHookDefinitionInfos,
                                                    TCHAR* pszFunctionDescription,
                                                    TCHAR* pszFileName,
                                                    DWORD dwLineNumber,
                                                    OUT MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo,
                                                    OUT BOOL* pbParsingError)
{
    TCHAR* pszAPIName;
    TCHAR* pszParameterList;
    TCHAR* pszAPIOptionalParameters;
    TCHAR* pszComOptionalParameters=NULL;
    BOOL   bAlreadyHooked;
    CLinkListItem* pItemAPIInfo;
    API_INFO* pApiInfo;
    CMethodInfo* pMethodInfo;
    BOOL bMethodInfoWasAlreadyExisting;
    tagCALLING_CONVENTION CallingConvention;
    COM_OPTIONAL_PARAMETERS ComOptionalParameters;

    BOOL bRet;
    BOOL bHasComOptionnalParameters=FALSE;
    *pbParsingError=FALSE;
    *ppAlreadyHookingMonitoringFileInfo=NULL;

    // get function description
    if (!HookComInfos.ParseFunctionDescription(pszFunctionDescription,&pszAPIName,&pszParameterList,&pszAPIOptionalParameters,&CallingConvention))
    {
        *pbParsingError=TRUE;
        return FALSE;
    }

    // get method info
    if(!this->GetMethodInfoForHook(pHookedObject,
                                    pInterfaceAssociatedToIID,
                                    pLinkListOfBaseInterfacesID,
                                    pHookDefinitionInfos,
                                    pszAPIName,
                                    pszFileName,
                                    dwLineNumber,
                                    HOOK_TYPE_MONITORING,
                                    bAutoHook,
                                    &pMethodInfo,
                                    &bMethodInfoWasAlreadyExisting))
        return FALSE;

    // fill pMethodInfo Name
    pMethodInfo->SetName(pszAPIName);

    // if method has a not NULL pItemAPIInfo, that means method is already hooked
    if (pMethodInfo->pItemAPIInfo)
    {
        pItemAPIInfo=pMethodInfo->pItemAPIInfo;
        bAlreadyHooked=TRUE;
    }
    else
    {
        // initialize hook struct
        if (!InitializeHook(this->AssociatedModuleName,pMethodInfo,&pItemAPIInfo,&bAlreadyHooked))
        {
            // if static hook and method has been created by GetMethodInfoForHook
            if ((!bAutoHook)&&(!bMethodInfoWasAlreadyExisting))
                delete pMethodInfo;

            return FALSE;
        }
    }

    // if static hook and method has been created by GetMethodInfoForHook
    if ((!bAutoHook)&&(!bMethodInfoWasAlreadyExisting))
        delete pMethodInfo;

    pApiInfo=(API_INFO*)pItemAPIInfo->ItemData;

    // if a hook was installed
    if (bAlreadyHooked)
    {
        // check if a monitoring hook was installed
        if (pApiInfo->pMonitoringFileInfos)
        {
            *ppAlreadyHookingMonitoringFileInfo=pApiInfo->pMonitoringFileInfos;
            return TRUE;
        }
    }
    ////////////////////////////////////////////////////////
    // if not already hooked, parse parameters and options
    ////////////////////////////////////////////////////////

    // put default direction spying
    pApiInfo->ParamDirectionType=DEFAULT_PARAMETER_DIRECTION_SPYING;

    // parse parameters
    bRet=HookComInfos.ParseParameters(pApiInfo,pszParameterList,pszFileName,dwLineNumber);
    if (bRet)
    {
        // parse options
        if (pszAPIOptionalParameters)
        {
            // split specific com optional parameters and standard api optional parameters
            bRet=this->ComOptionalParametersSplit(pszAPIOptionalParameters,&pszComOptionalParameters);
            if (bRet)
            {
                // if there is Com Optional Parameters
                if (*pszComOptionalParameters)
                {
                    // parse specific com optional parameters
                    bRet=this->ComOptionalParametersParse(pszComOptionalParameters,&ComOptionalParameters);
                    if (bRet)
                        bHasComOptionnalParameters=TRUE;
                }

                if (bRet)
                    // parse standard api optional parameters
                    bRet=HookComInfos.ParseOptions(pApiInfo,pszAPIOptionalParameters,bAlreadyHooked,pszFileName,dwLineNumber);
            }
            delete pszComOptionalParameters;
            pszComOptionalParameters=NULL;
        }
    }

    // in case of parsing errors
    if (!bRet)
    {
        if (!bAlreadyHooked)
            // free memory associated to hook
            HookComInfos.FreeApiInfoItem(pItemAPIInfo);

        // signal parsing error
        *pbParsingError=TRUE;

        return FALSE;
    }

    if (bAutoHook)
    {
        // associate item to method
        pMethodInfo->pItemAPIInfo=pItemAPIInfo;
    }

    // associate monitoring file to monitoring hook
    pApiInfo->pMonitoringFileInfos=pMonitoringFileInfo;

    if (!bAlreadyHooked)
    {
        pApiInfo->HookType=HOOK_TYPE_COM;
        pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.ClassID=this->Clsid;
        pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.InterfaceID=pHookDefinitionInfos->InterfaceID;
        pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.VTBLIndex=pHookDefinitionInfos->VTBLIndex;
        pApiInfo->CallingConvention=CallingConvention;
        // the pApiInfo struct is fully filled, so hook vtbl or function
        if (!HookComInfos.HookAPIFunction(pApiInfo))
        {
            if (bAutoHook)
            {
                // remove item association with method
                pMethodInfo->pItemAPIInfo=NULL;
            }

            // free memory associated to hook
            HookComInfos.FreeApiInfoItem(pItemAPIInfo);

            return FALSE;
        }
        // add ItemApiInfo deletion callback
        if (!pApiInfo->DeletionCallback)
            pApiInfo->DeletionCallback=ApiInfoItemDeletionCallback;
    }

    if (bHasComOptionnalParameters)
        // Warning pApiInfo must be hooked before calling this method
        this->ComOptionalParametersApply(pApiInfo,&ComOptionalParameters);

#ifdef _DEBUG
    TCHAR pszOutput[2*MAX_PATH];
    TCHAR tClsid[MAX_PATH];
    CGUIDStringConvert::TcharFromCLSID(&this->Clsid,tClsid);
    _sntprintf(pszOutput,2*MAX_PATH,_T("Monitoring hook installed at address 0x%p for method %s (CLSID:%s dll:%s)\r\n"),
        (PBYTE)pApiInfo->APIAddress,
        pApiInfo->szAPIName,
        tClsid,
        this->AssociatedModuleName);
    pszOutput[2*MAX_PATH-1]=0;
    OutputDebugString(pszOutput);
#endif

    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: ComOptionalParametersSplit
// Object: split com optional option from optional api infos
//          ex: ApiOpt1|ComOpt1|ApiOpt2 --> ApiOpt1|ApiOpt2\0ComOpt1 and *pszComOptionalParameters point to ComOpt1
//         (currently only com object creation spying for an interface method is supported)
// Parameters :
//     inout : TCHAR* pszAPIOptionalParameters
//     out   : TCHAR** ppszComOptionalParameters : pointer to optional com informations,
//                                               or NULL if no COM optional informations
//                   WARNING *pszComOptionalParameters is allocated, must be deleted by "delete"
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CHookedClass::ComOptionalParametersSplit(TCHAR* pszAPIOptionalParameters,TCHAR** ppszComOptionalParameters)
{
    *ppszComOptionalParameters=new TCHAR[_tcslen(pszAPIOptionalParameters)+1];
    TCHAR* pszComOptionalParameters=*ppszComOptionalParameters;
    *pszComOptionalParameters=0;
    
    // search for com optional parameters
    
    TCHAR* pszOption=pszAPIOptionalParameters;
    TCHAR* pszNextOption=NULL;

    while (pszOption)
    {
        pszNextOption=_tcschr(pszOption, FIELDS_SEPARATOR);
        if (pszNextOption)
            pszNextOption++;

AfterNextOptionSearch:
        if (_tcsnicmp(pszOption,COM_DEFINITION_OBJECT_CREATION,_tcslen(COM_DEFINITION_OBJECT_CREATION))==0)
        {
            TCHAR* pszObjectCreationDefBegin;
            TCHAR* pszObjectCreationDefEnd;
            // option is like | ObjectCreation(data1|data2|data3) |

            pszObjectCreationDefBegin=pszOption;

            // find ')'
            pszObjectCreationDefEnd=_tcschr(pszObjectCreationDefBegin,')');
            if (!pszObjectCreationDefEnd)
                return FALSE;

            // adjust pszNextOption
            pszNextOption=_tcschr(pszObjectCreationDefEnd, FIELDS_SEPARATOR);
            if (!pszNextOption)
            {
                // if no options after COM_DEFINITION_OBJECT_CREATION
                _tcscat(pszComOptionalParameters,pszObjectCreationDefBegin);

                // ends previous parameter definition if any
                if (pszObjectCreationDefBegin!=pszAPIOptionalParameters)
                    *(pszObjectCreationDefBegin-1)=0;

                return TRUE;
            }
            else
            {
                // there's other options after COM_DEFINITION_OBJECT_CREATION

                // ends current definition
                *(pszNextOption)=0;
                pszNextOption++;

                // if not first com option
                if (*pszComOptionalParameters)
                    // add splitter
                    _tcscat(pszComOptionalParameters,FIELDS_SEPARATOR_STRING);

                // add com option to pszComOptionalParameters
                _tcscat(pszComOptionalParameters,pszObjectCreationDefBegin);

                // ends previous parameter definition if any
                if (pszObjectCreationDefBegin!=pszAPIOptionalParameters)
                    *(pszObjectCreationDefBegin-1)=0;

                // add remaining option to pszAPIOptionalParameters
                if (*pszAPIOptionalParameters) // if not first option
                    _tcscat(pszAPIOptionalParameters,FIELDS_SEPARATOR_STRING);

                _tcscat(pszAPIOptionalParameters,pszNextOption);

                // adjust pszNextOption : should point to pszObjectCreationDefBegin;
                pszNextOption=pszObjectCreationDefBegin;

                goto AfterNextOptionSearch;
            }
        }
        // else if()

        pszOption = pszNextOption;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ComOptionalParametersParse
// Object: parse com optional option of a com monitoring file 
//         (currently only com object creation spying for an interface method is supported)
// Parameters :
//     in  : 
//           TCHAR* pszComOptionalParameters : pointer to optional com informations
//     int out : COM_OPTIONAL_PARAMETERS* pComOptionalParameters : struct containing parsed optional informations
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CHookedClass::ComOptionalParametersParse(
                                              TCHAR* pszComOptionalParameters,
                                              COM_OPTIONAL_PARAMETERS* pComOptionalParameters)
{
    memset(pComOptionalParameters,0,sizeof(COM_OPTIONAL_PARAMETERS));
    DWORD StackSize=0;
    // currently pszComOptionalParameters is like "ObjectCreation(data1|data2|data3)"

    // find '('
    TCHAR* pszObjectCreationDefBegin=_tcschr(pszComOptionalParameters,'(');
    if (!pszObjectCreationDefBegin)
        return FALSE;
    // point after '('
    pszObjectCreationDefBegin++;

    // find ')'
    TCHAR* pszObjectCreationDefEnd=_tcschr(pszObjectCreationDefBegin,')');
    if (!pszObjectCreationDefEnd)
        return FALSE;

    // ends pszObjectCreationDefBegin
    *pszObjectCreationDefEnd=0;

    if (pCOMCreationPostApiCallHooks->ParseComCreationParameters(pszObjectCreationDefBegin,&this->Clsid,FALSE,&pComOptionalParameters->HookData,&StackSize))
    {
        pComOptionalParameters->HookDataSet=TRUE;
        return TRUE;
    }
    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: ComOptionalParametersApply
// Object: Apply com optional option of a com monitoring file 
//         (currently only com object creation spying for an interface method is supported)
//         WARNING PAPIINFO MUST BE HOOKED BEFORE CALLING THIS METHOD
// Parameters :
//     in  : API_INFO* pApiInfo : hooked api informations
//           COM_OPTIONAL_PARAMETERS* pComOptionalParameters : struct filled by ComOptionalParametersParse
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CHookedClass::ComOptionalParametersApply(API_INFO* pApiInfo,COM_OPTIONAL_PARAMETERS* pComOptionalParameters)
{
    BOOL bRet=TRUE;
    if (pComOptionalParameters->HookDataSet)
    {
        bRet=bRet && pCOMCreationPostApiCallHooks->InstallPostHook(pApiInfo->szModuleName,
                                                                    pApiInfo->szAPIName,
                                                                    (PBYTE)pApiInfo->APIAddress,
                                                                    pApiInfo->bFunctionPointer,
                                                                    pApiInfo->StackSize,
                                                                    &pComOptionalParameters->HookData
                                                                    );
    }
    return bRet;
}


//-----------------------------------------------------------------------------
// Name: AddFakingHookForObjectMethod
// Object: FAKING_HOOK_INFORMATIONS struct of a faking dll
// Parameters :
//     in  : CHookedObject* pHookedObject : object associated with COM object
//           IUnknown* pInterface : pointer to interface of specified IID
//           FAKING_HOOK_INFORMATIONS* pFakingHookInfos : all necessary 
//                 informations requiered to install fake, pre or post hook
//           FAKING_DLL_INFOS** ppAlreadyHookingFakingDllInfos : NULL if method wasn't already hooked
//                                                               faking dll information of dll owning the hook
//     return : TRUE if method is hooked (already hooked or not)
//-----------------------------------------------------------------------------
BOOL CHookedClass::AddFakingHookForObjectMethod(FAKING_DLL_INFOS* pFakingDllInfos,
                                                CHookedObject* pHookedObject,
                                                IUnknown* pInterfaceAssociatedToIID,
                                              FAKING_HOOK_INFORMATIONS* pFakingHookInfos,
                                              FAKING_DLL_INFOS** ppAlreadyHookingFakingDllInfos)
{
    BOOL   bAlreadyHooked;
    CLinkListItem* pItemAPIInfo;
    API_INFO* pApiInfo;
    CMethodInfo* pMethodInfo;
    TCHAR pszFakingModuleName[MAX_PATH];
    BOOL bAlreadyExisting;

    *ppAlreadyHookingFakingDllInfos=NULL;

    // get faking dll name
    GetModuleFileName(pFakingHookInfos->pFakingDllInfos->hModule,pszFakingModuleName,MAX_PATH);

    // get method info
    if(!this->GetMethodInfoForHook(pHookedObject,
                                    pInterfaceAssociatedToIID,
                                    NULL,
                                    &pFakingHookInfos->HookDefinitionInfos,
                                    pFakingHookInfos->FakeApiInfos.pszAPIName,
                                    pszFakingModuleName,
                                    pFakingHookInfos->FakingDllIndex,
                                    (tagHookType)pFakingHookInfos->FakingType,
                                    FALSE,// faking hooks are always static so don't add hook to auto monitored one's
                                    &pMethodInfo,
                                    &bAlreadyExisting))
        return FALSE;

    // if method has a not NULL pItemAPIInfo, that means method is already hooked
    if (pMethodInfo->pItemAPIInfo)
    {
        pItemAPIInfo=pMethodInfo->pItemAPIInfo;
        bAlreadyHooked=TRUE;
    }
    else
    {
        // initialize hook struct
        if (!InitializeHook(this->AssociatedModuleName,pMethodInfo,&pItemAPIInfo,&bAlreadyHooked))
        {
            if (!bAlreadyExisting)
                delete pMethodInfo;
            return FALSE;
        }
    }
    // DONT associate item to method
    // static com loading MUSN'T set this field (else troubles will appear after overriding dll has been unloaded 
    // because pMethodInfo->pItemAPIInfo will point to a destroyed item)
    // pMethodInfo->pItemAPIInfo=pItemAPIInfo;

    // from now pMethodInfo becomes useless
    if (!bAlreadyExisting)
        delete pMethodInfo;

    // get the pointer to the API Info struct
    pApiInfo=(API_INFO*)pItemAPIInfo->ItemData;

    // if a hook was installed
    if (bAlreadyHooked)
    {
        if (pFakingHookInfos->FakingType==HOOK_FAKING_TYPE_FAKE)
        {
            // check if a faking hook was installed
            if (pApiInfo->pFakeDllInfos)
            {
                *ppAlreadyHookingFakingDllInfos=pApiInfo->pFakeDllInfos;
                return TRUE;
            }
        }
    }
    else
    {
        // fill stack size
        pApiInfo->StackSize=pFakingHookInfos->FakeApiInfos.StackSize;

        if (pFakingDllInfos->ApiOverrideBuildVersionFramework>=6)
        {
            // for backward compatibility, 
            // pFakeApiInfos->FirstBytesCanExecuteAnywhereSize becomes an option flags and FirstBytesCanExecuteAnywhereSize is reduce to an unsigned 8 bits value (sufficient for our goal currently max size is 64 bits)

            // get FirstBytesCanExecuteAnywhereSize 
            pApiInfo->FirstBytesCanExecuteAnywhereSize = (pFakingHookInfos->FakeApiInfos.AdditionalOptions & OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_SIZE_MASK);
            if ((pApiInfo->FirstBytesCanExecuteAnywhereSize & OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE)== OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE)
            {
                pApiInfo->FirstBytesCanExecuteAnywhereSize = (DWORD)(-1);
            }

            pApiInfo->DontCheckModulesFiltersForFaking = ((pFakingHookInfos->FakeApiInfos.AdditionalOptions & OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS)== OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS);
        }
        else
        {
            // set the first bytes can execute anywhere option
            pApiInfo->FirstBytesCanExecuteAnywhereSize=pFakingHookInfos->FakeApiInfos.AdditionalOptions;
        }

    }

    pApiInfo->HookType=HOOK_TYPE_COM;
    pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.ClassID=pFakingHookInfos->Clsid;
    pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.InterfaceID=pFakingHookInfos->HookDefinitionInfos.InterfaceID;
    pApiInfo->HookTypeExtendedFunctionInfos.InfosForCOM.VTBLIndex=pFakingHookInfos->HookDefinitionInfos.VTBLIndex;

    // according to faking type
    switch(pFakingHookInfos->FakingType)
    {
    case HOOK_FAKING_TYPE_FAKE:
        // associate faking dll to faking hook
        pApiInfo->pFakeDllInfos=pFakingHookInfos->pFakingDllInfos;
        pApiInfo->FakeAPIAddress=pFakingHookInfos->FakeApiInfos.FakeAPI;
        if (!bAlreadyHooked)
        {
            // the pApiInfo struct is fully filled, so hook vtbl or function
            if (!HookComInfos.HookAPIFunction(pApiInfo))
            {
                // free memory associated to hook
                HookComInfos.FreeApiInfoItem(pItemAPIInfo);

                return FALSE;
            }
            // add ItemApiInfo deletion callback
            if (!pApiInfo->DeletionCallback)
                pApiInfo->DeletionCallback=ApiInfoItemDeletionCallback;
        }
        break;
    case HOOK_FAKING_TYPE_PRE_API_CALL:
        // add pre api call
        if (!HookComInfos.AddPreApiCallCallBack(pItemAPIInfo,
                                                !bAlreadyHooked,
                                                pFakingHookInfos->pFakingDllInfos->hModule,
                                                (pfPreApiCallCallBack)pFakingHookInfos->FakeApiInfos.FakeAPI,
                                                pFakingHookInfos->FakeApiInfos.UserParam)
                                                )
            return FALSE;
            // add ItemApiInfo deletion callback
            if (!pApiInfo->DeletionCallback)
                pApiInfo->DeletionCallback=ApiInfoItemDeletionCallback;
        break;
    case HOOK_FAKING_TYPE_POST_API_CALL:
        // add post api call
        if (!HookComInfos.AddPostApiCallCallBack(pItemAPIInfo,
                                                !bAlreadyHooked,
                                                pFakingHookInfos->pFakingDllInfos->hModule,
                                                (pfPostApiCallCallBack)pFakingHookInfos->FakeApiInfos.FakeAPI,
                                                pFakingHookInfos->FakeApiInfos.UserParam)
                                                )
            return FALSE;
            // add ItemApiInfo deletion callback
            if (!pApiInfo->DeletionCallback)
                pApiInfo->DeletionCallback=ApiInfoItemDeletionCallback;
        break;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ReportNotReleasedObject
// Object: report a message telling object was not fully released
// Parameters :
//     in  : CHookedObject* pHookedObject : pointer to CHookedObject object
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CHookedClass::ReportNotReleasedObject(CHookedObject* pHookedObject)
{
    // if we don't report hooked com objects
    if (!HookComOptions.ReportHookedCOMObject)
        return;

    // if object has been created for static monitoring/faking
    // it could have been done without COM auto monitoring
    // that means it's Release interface may not be hooked,
    // so we can't say that it is not released
    if (pHookedObject->bCreatedByStaticFileLoading)
        return;

    TCHAR pszMsg[MAX_PATH];
    BOOL bReportMessage=FALSE;

    if (HookComOptions.ReportUseNameInsteadOfIDIfPossible)
    {
        TCHAR _tProgId[STRING_PROGID_MAX_SIZE];
        bReportMessage=CGUIDStringConvert::GetClassName(&this->Clsid,_tProgId,STRING_PROGID_MAX_SIZE);
        if (bReportMessage)
            _stprintf(pszMsg,_T("COM: Object 0x%p of type %s seems to be not fully released"),pHookedObject->pObject,_tProgId);
    }
    // if progId retrieval failure or !ReportUseProgIdInsteadOfCLSIIfPossible
    if (!bReportMessage)
    {
        TCHAR _tClsid[STRING_GUID_SIZE];
        bReportMessage=CGUIDStringConvert::TcharFromCLSID(&this->Clsid,_tClsid);
        if (bReportMessage)
            _stprintf(pszMsg,_T("COM: Object 0x%p with CLSID %s seems to be not fully released"),pHookedObject->pObject,_tClsid);
    }
    if(bReportMessage)
        HookComInfos.ReportMessage(REPORT_MESSAGE_WARNING,pszMsg);
}

//-----------------------------------------------------------------------------
// Name: FreeAllHookedObjectsAndReportNotReleased
// Object: removed all pHookedObjects contained in this->pLinkListHookedObjects
//         and report a message telling they were not fully released
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CHookedClass::FreeAllHookedObjectsAndReportNotReleased()
{
    if (!this->pLinkListHookedObjects->Head)
        return;
    CHookedObject* pHookedObject=NULL;
    CLinkListItem* pItem;
    this->pLinkListHookedObjects->Lock();
    for(pItem=this->pLinkListHookedObjects->Head;pItem;pItem=pItem->NextItem)
    {
        if (IsBadReadPtr(pItem->ItemData,sizeof(CHookedObject)))
            continue;
        // get pointer on pHookedObject
        pHookedObject=(CHookedObject*)pItem->ItemData;

        // report not released
        this->ReportNotReleasedObject(pHookedObject);

        // free memory
        delete pHookedObject;
    }
    this->pLinkListHookedObjects->RemoveAllItems(TRUE);
    this->pLinkListHookedObjects->Unlock();
}
