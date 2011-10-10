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
// Object: manage informations on a single COM object
//-----------------------------------------------------------------------------

#include "hookedobject.h"

//-----------------------------------------------------------------------------
// Name: CHookedObject
// Object: constructor
// Parameters :
//     in  : IUnknown* pObject : pointer to object to hook
//           IID* pIID : associated pObject Interface ID
//     out : 
//     return : TRUE if constructor was successfully completed, FALSE else
//-----------------------------------------------------------------------------
CHookedObject::CHookedObject(IUnknown* pObject,IID* pIid)
{
    this->OwningThread=GetCurrentThreadId();
    this->bCreatedByStaticFileLoading=FALSE;
    this->IsGoingToBeDestroyed=FALSE;
    this->pIUnknown=NULL;
    this->NbLocks=0;
    this->hevtUnlocked=CreateEvent(NULL,FALSE,TRUE,NULL);

    this->pObject=pObject;
    if (this->pObject==NULL)
        return;

    // create a link list for storing all object representations (interfaces pointers)
    this->pLinkListInterfacePointers=new CLinkList(sizeof(OBJECT_INFO_INTERFACE_POINTER));
    if (this->pLinkListInterfacePointers==NULL)
        return;
    
    // query IUnknown interface (pIUnknown is used for identifying release object)
    if (IsEqualIID(*pIid,IID_IUnknown))
    {
        this->pIUnknown=pObject;
    }
    else
    {
        this->pIUnknown=NULL;
        BOOL bSuccess=SUCCEEDED(CSecureIUnknown::QueryInterface(pObject,IID_IUnknown,(void**)&this->pIUnknown));
        if (bSuccess)
        {
            if (IsBadReadPtr(this->pIUnknown,sizeof(IUnknown)))
                bSuccess=FALSE;
        }
        if (bSuccess)
        {
            // release IUnknown interface
            CSecureIUnknown::Release(this->pIUnknown);
        }
        else
        {
            this->pIUnknown=NULL;
#ifdef _DEBUG
            if (IsDebuggerPresent())// avoid to crash application if no debugger
                DebugBreak();
#endif
        }
    }
    IID Iid=IID_IUnknown;
    this->AddInterfacePointer(this->pIUnknown,&Iid);
    this->AddInterfacePointer(pObject,pIid);
}
//-----------------------------------------------------------------------------
// Name: SuccessfullyCreated
// Object: allow to know if constructor was successfully completed
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if constructor was successfully completed, FALSE else
//-----------------------------------------------------------------------------
BOOL CHookedObject::SuccessfullyCreated()
{
    return (this->pIUnknown!=NULL);
}

CHookedObject::~CHookedObject(void)
{
    // set IsGoingToBeDestroyed flag
    this->IsGoingToBeDestroyed=TRUE;
    
    DWORD NbWaitingItems=this->NbLocks;

    // if lock was in use
    if (NbWaitingItems>0)
    {
        NbWaitingItems--;// remove 1 for the lock in use

        // release all waiting locks
        for (DWORD cnt=0;cnt<NbWaitingItems;cnt++)
        {
            this->Unlock();
        }

        // wait until used lock is released
        WaitForSingleObject(this->hevtUnlocked,INFINITE);
    }

    CloseHandle(this->hevtUnlocked);
    if (this->pLinkListInterfacePointers)
        delete this->pLinkListInterfacePointers;
}

//-----------------------------------------------------------------------------
// Name: Lock
// Object: add Lock to object
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if object is locked, FALSE else
//-----------------------------------------------------------------------------
BOOL CHookedObject::Lock(BOOL* pbIsGoingToBeDestroyed)
{
    DWORD dwRet;
    if (this->IsGoingToBeDestroyed)
    {
        *pbIsGoingToBeDestroyed=TRUE;
        return FALSE;
    }

    this->NbLocks++;
    dwRet=WaitForSingleObject(this->hevtUnlocked,INFINITE);
    *pbIsGoingToBeDestroyed=this->IsGoingToBeDestroyed;
    if (dwRet!=WAIT_OBJECT_0)
    {
        this->NbLocks--;
        return FALSE;
    }

    return (!this->IsGoingToBeDestroyed);
}

//-----------------------------------------------------------------------------
// Name: Unlock
// Object: remove Lock to object
// Parameters :
//     in  : 
//     out : 
//     return : TRUE
//-----------------------------------------------------------------------------
BOOL CHookedObject::Unlock()
{
    if (this->NbLocks==0)
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())
        {
            DebugBreak();
        }
#endif
        return FALSE;
    }

    SetEvent(this->hevtUnlocked);
    this->NbLocks--;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetpIDispatch
// Object: get pointer to IDispatch for the hooked object
// Parameters :
//     in  : IUnknown* pObject : current object instance
//     out : 
//     return : pointer to IDispatch, NULL on error or if object dosen't support IDispatch
//-----------------------------------------------------------------------------
IDispatch* CHookedObject::GetpIDispatch(IUnknown* pObject)
{
    IDispatch* pIDispatch;
    if (FAILED(CSecureIUnknown::QueryInterface(pObject,IID_IDispatch,(void**)&pIDispatch)))
        return NULL;

    if (IsBadReadPtr(pIDispatch,sizeof(IDispatch)))
        return NULL;

    IID Iid=IID_IDispatch;
    this->AddInterfacePointer(pIDispatch,&Iid);

    // release Interface
    CSecureIUnknown::Release(pIDispatch);

    return pIDispatch;
}

//-----------------------------------------------------------------------------
// Name: GetpIUnknown
// Object: get pointer to IUnknown for the hooked object
// Parameters :
//     in  : IUnknown* pObject : current object instance
//     out : 
//     return : pointer to IDispatch, NULL on error or if object dosen't support IDispatch
//-----------------------------------------------------------------------------
IUnknown* CHookedObject::GetpIUnknown(IUnknown* pObject)
{
    IUnknown* pIUnknown;
    if (FAILED(CSecureIUnknown::QueryInterface(pObject,IID_IUnknown,(void**)&pIUnknown)))
        return NULL;

    if (IsBadReadPtr(pIUnknown,sizeof(IUnknown)))
        return NULL;

    IID Iid=IID_IUnknown;
    this->AddInterfacePointer(pIUnknown,&Iid);

    // release Interface
    CSecureIUnknown::Release(pIUnknown);

    return pIUnknown;
}

//-----------------------------------------------------------------------------
// Name: GetInterfacePointer
// Object: get pointer associated to interface
// Parameters :
//     in  : IID* pIID :  Interface ID
//     out : 
//     return : pointer to object or NULL if not found
//-----------------------------------------------------------------------------
IUnknown* CHookedObject::GetInterfacePointer(IID* pIid)
{
    return this->GetInterfacePointer(pIid,FALSE);
}

//-----------------------------------------------------------------------------
// Name: GetInterfacePointer
// Object: get pointer associated to interface
// Parameters :
//     in  : IID* pIID :  Interface ID
//           BOOL UserManagesListLock : TRUE is user manages pLinkListInterfacePointers lock
//     out : 
//     return : pointer to object or NULL if not found
//-----------------------------------------------------------------------------
IUnknown* CHookedObject::GetInterfacePointer(IID* pIid,BOOL UserManagesListLock)
{
    CLinkListItem* pItem;
    pItem=this->GetLinkListInterfacePointersItem(pIid,UserManagesListLock);
    if (!pItem)
        return NULL;

    return ((OBJECT_INFO_INTERFACE_POINTER*)pItem->ItemData)->pInterface;
}

//-----------------------------------------------------------------------------
// Name: GetLinkListInterfacePointersItem
// Object: get pointer associated to interface
// Parameters :
//     in  : IID* pIID :  Interface ID
//           BOOL UserManagesListLock : TRUE is user manages pLinkListInterfacePointers lock
//     out : 
//     return : pointer to object or NULL if not found
//-----------------------------------------------------------------------------
CLinkListItem* CHookedObject::GetLinkListInterfacePointersItem(IID* pIid,BOOL UserManagesListLock)
{
    CLinkListItem* pItem;
    OBJECT_INFO_INTERFACE_POINTER* pHookedInterfacePointer;
    // check that object doesn't already belong to list
    //for each item
    if (!UserManagesListLock)
        this->pLinkListInterfacePointers->Lock();
    for(pItem=this->pLinkListInterfacePointers->Head;pItem;pItem=pItem->NextItem)
    {
        pHookedInterfacePointer=(OBJECT_INFO_INTERFACE_POINTER*)pItem->ItemData;
        // if interface found, do nothing
        if (IsEqualIID(pHookedInterfacePointer->Iid,*pIid))
        {
            if (!UserManagesListLock)
                this->pLinkListInterfacePointers->Unlock();
            return pItem;
        }
    }
    if (!UserManagesListLock)
        this->pLinkListInterfacePointers->Unlock();

    return NULL;
}

//-----------------------------------------------------------------------------
// Name: AddInterfacePointer
// Object: add pointer to interface to object references list
// Parameters :
//     in  : IUnknown* pObject : pointer to object to hook
//           IID* pIID : associated pObject Interface ID
//     out : 
//     return : TRUE if added, FALSE error or already existing
//-----------------------------------------------------------------------------
BOOL CHookedObject::AddInterfacePointer(IUnknown* pObject,IID* pIid)
{
    BOOL bRet;

    CLinkListItem* pItem;
    OBJECT_INFO_INTERFACE_POINTER* pHookedInterfacePointer;
    this->pLinkListInterfacePointers->Lock();
    for(pItem=this->pLinkListInterfacePointers->Head;pItem;pItem=pItem->NextItem)
    {
        pHookedInterfacePointer=(OBJECT_INFO_INTERFACE_POINTER*)pItem->ItemData;
        // if interface found, do nothing
        if ((pObject==pHookedInterfacePointer->pInterface)
             && (IsEqualIID(pHookedInterfacePointer->Iid,*pIid)))
        {
            this->pLinkListInterfacePointers->Unlock();
            return TRUE;
        }
    }

    // if interface not found, add Interface and its pointer to list
    OBJECT_INFO_INTERFACE_POINTER HookedInterfacePointer;
    HookedInterfacePointer.Iid=*pIid;
    HookedInterfacePointer.pInterface=pObject;
    bRet=(this->pLinkListInterfacePointers->AddItem(&HookedInterfacePointer,TRUE)!=NULL);
    this->pLinkListInterfacePointers->Unlock();
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: DoesPointerBelongToObject
// Object: search throw all object interfaces if an interface match the provided pointer
//         if pointer match an interface, that means pointer is associated to this object
// Parameters :
//     in  : IUnknown* pObject : pointer to object to hook
//     out : 
//     return : TRUE if pointer is associated to this hooked object
//-----------------------------------------------------------------------------
BOOL CHookedObject::DoesPointerBelongToObject(IUnknown* pObject)
{
    CLinkListItem* pItem;
    OBJECT_INFO_INTERFACE_POINTER* pHookedInterfacePointer;
    this->pLinkListInterfacePointers->Lock();
    for(pItem=this->pLinkListInterfacePointers->Head;pItem;pItem=pItem->NextItem)
    {
        pHookedInterfacePointer=(OBJECT_INFO_INTERFACE_POINTER*)pItem->ItemData;
        // if interface found, do nothing
        if (pObject==pHookedInterfacePointer->pInterface)
        {
            this->pLinkListInterfacePointers->Unlock();
            return TRUE;
        }
    }
    // if interface not found
    this->pLinkListInterfacePointers->Unlock();
    return FALSE;
}