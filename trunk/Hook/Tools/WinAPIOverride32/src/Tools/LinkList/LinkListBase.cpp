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
// Object: Link list base interface
//-----------------------------------------------------------------------------

#include "linklistbase.h"

CLinkListBase::CLinkListBase()
{
    this->Head=NULL;
    this->Tail=NULL;
    // use process heap by default
    this->LinkListHeap=GetProcessHeap();
    this->hevtListUnlocked=CreateEvent(NULL,FALSE,TRUE,NULL);
    this->hevtInternalLockUnlocked=CreateEvent(NULL,FALSE,TRUE,NULL);
    
    this->ItemsNumber=0;
    this->LockWaitTime=INFINITE;
    this->bAllowToAddItemDuringLock=FALSE;
}

CLinkListBase::~CLinkListBase(void)
{
    // close unlocked event
    if (this->hevtListUnlocked)
    {
        CloseHandle(this->hevtListUnlocked);
        this->hevtListUnlocked=NULL;
    }
    if (this->hevtInternalLockUnlocked)
    {
        CloseHandle(this->hevtInternalLockUnlocked);
        this->hevtInternalLockUnlocked=NULL;
    }
}


//-----------------------------------------------------------------------------
// Name: SetHeap
// Object: allow to specify a heap for items allocation
//              Allocating all elements in a specified heap allow a quick deletion
//              by calling CLinkListBase::Lock; // assume list is locked before destroying memory
//                         ::HeapDestroy();
//                         CLinkListBase::ReportHeapDestruction();
//                              NewHeap=::HeapCreate(0,0,0);[optional,only if you want to use list again]
//                              CLinkListBase::SetHeap(NewHeap);[optional,only if you want to use list again]
//                         CLinkListBase::Unlock();
//
// Parameters :
//     in  : HANDLE HeapHandle : specified heap handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CLinkListBase::SetHeap(HANDLE HeapHandle)
{
    this->LinkListHeap=HeapHandle;
}
//-----------------------------------------------------------------------------
// Name: GetHeap
// Object: return the specified heap for items allocation
//
// Parameters :
//     in  : 
//     out :
//     return : HANDLE HeapHandle : specified heap handle
//-----------------------------------------------------------------------------
HANDLE CLinkListBase::GetHeap()
{
    return this->LinkListHeap;
}

//-----------------------------------------------------------------------------
// Name: ReportHeapDestruction
// Object: SEE CLinkListBase::SetHeap DOCUMENTATION FOR USING SEQUENCE
//          if all elements have been created and allocated in specified heap
//         a quick memory freeing can be done by HeapDestroy.
//         If HeapDestroy has been called, all items must be removed from the list
//         without calling HeapFree
//         So this function remove all items from list without trying to free memory
//         USE IT ONLY IF MEMORY HAS BEEN REALLY DESTROYED, ELSE ALLOCATED MEMORY WON'T BE FREED
//
// Parameters :
//     in  : HANDLE HeapHandle : specified heap handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CLinkListBase::ReportHeapDestruction()
{
    // all memory is destroyed, just reset some fields
    this->Head=NULL;
    this->Tail=NULL;
    this->ItemsNumber=0;
}

//-----------------------------------------------------------------------------
// Name: IsEmpty
// Object: check if list is empty
//
// Parameters :
//     in  : 
//     out :
//     return : TRUE if list is empty
//-----------------------------------------------------------------------------
BOOL CLinkListBase::IsEmpty()
{
    return (this->Head == NULL);
}

//-----------------------------------------------------------------------------
// Name: IsItemStillInList
// Object: check if an item is still in the list
//          Notice : it's quite note a secure way as original item can have 
//                   been replaced by another one (if HeapAlloc returns the same address 
//                   for a new item)
//                   but it's assume at least memory is still available with the same type of object
//          in conclusion : it gives a safer way than IsBadReadPtr :)
// Parameters :
//     in  : CLinkListItem* pItem
//     out :
//     return : TRUE if item is still in list
//-----------------------------------------------------------------------------
BOOL CLinkListBase::IsItemStillInList(CLinkListItem* pItem)
{
    return this->IsItemStillInList(pItem,FALSE);
}

//-----------------------------------------------------------------------------
// Name: IsItemStillInList
// Object: check if an item is still in the list
//          Notice : it's quite note a secure way as original item can have 
//                   been replaced by another one (if HeapAlloc returns the same address 
//                   for a new item)
//                   but it's assume at least memory is still available with the same type of object
//          in conclusion : it gives a safer way than IsBadReadPtr :)
// Parameters :
//     in  : CLinkListItem* pItem
//           BOOL bUserManagesLock : TRUE is user manages Lock
//     out :
//     return : TRUE if item is still in list
//-----------------------------------------------------------------------------
BOOL CLinkListBase::IsItemStillInList(CLinkListItem* pItem,BOOL bUserManagesLock)
{
    BOOL bRet=FALSE;

    // lock list if needed
    if (!bUserManagesLock)
        this->LockForExternalCalls();

    WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);// must be after this->LockForExternalCalls call

    CLinkListItem* pListItem;
    // parse list
    for (pListItem=this->Head;pListItem;pListItem=pListItem->NextItem)
    {
        // if Cnt match Index
        if (pListItem==pItem)
        {
            bRet=TRUE;
            break;
        }
    }

    SetEvent(this->hevtInternalLockUnlocked);

    // unlock list if needed
    if (!bUserManagesLock)
        this->UnlockForExternalCalls();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: GetItemsCount
// Object: get number of items in the list
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD CLinkListBase::GetItemsCount()
{
    return this->ItemsNumber;
}

//-----------------------------------------------------------------------------
// Name: Lock
// Object: wait for list to be unlocked, and lock it
//          useful to avoid item removal when parsing list 
// Parameters :
//     in  : 
//     out :
//     return : WaitForSingleObject result
//-----------------------------------------------------------------------------
DWORD CLinkListBase::Lock()
{
    return this->Lock(FALSE);
}

//-----------------------------------------------------------------------------
// Name: Lock
// Object: wait for list to be unlocked, and lock it
//          useful to avoid item removal when parsing list 
// Parameters :
//     in  : BOOL bAllowToAddItemDuringLock : TRUE if items can be added into list
//                                             despite lock (useful for list parsing only)
//     out :
//     return : WaitForSingleObject result
//-----------------------------------------------------------------------------
DWORD CLinkListBase::Lock(BOOL bAllowToAddItemDuringLock)
{
    DWORD dwRet;
    dwRet=WaitForSingleObject(this->hevtListUnlocked,this->LockWaitTime);
    WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);
    this->bAllowToAddItemDuringLock=bAllowToAddItemDuringLock;
    SetEvent(this->hevtInternalLockUnlocked);
    return dwRet;
}

//-----------------------------------------------------------------------------
// Name: Unlock
// Object: unlock list
// Parameters :
//     in  : 
//     out :
//     return : SetEvent result
//-----------------------------------------------------------------------------
DWORD CLinkListBase::Unlock()
{
    // assume we are not adding an item before releasing lock
    if (this->bAllowToAddItemDuringLock)
    {
        WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);
        this->bAllowToAddItemDuringLock=FALSE;
        SetEvent(this->hevtInternalLockUnlocked);
    }
    return SetEvent(this->hevtListUnlocked);
}

//-----------------------------------------------------------------------------
// Name: LockForExternalCalls used internally only when this->hevtInternalLockUnlocked is already set
// Object: wait for list to be unlocked, and lock it
//          useful to avoid item removal when parsing list 
// Parameters :
//     in  : BOOL bAllowToAddItemDuringLock : TRUE if items can be added into list
//                                             despite lock (useful for list parsing only)
//     out :
//     return : WaitForSingleObject result
//-----------------------------------------------------------------------------
DWORD CLinkListBase::LockForExternalCalls()
{
    return WaitForSingleObject(this->hevtListUnlocked,this->LockWaitTime);
}
//-----------------------------------------------------------------------------
// Name: UnlockForExternalCalls used internally only when this->hevtInternalLockUnlocked is already set
// Object: unlock list
// Parameters :
//     in  : 
//     out :
//     return : SetEvent result
//-----------------------------------------------------------------------------
DWORD CLinkListBase::UnlockForExternalCalls()
{
    return SetEvent(this->hevtListUnlocked);
}

//-----------------------------------------------------------------------------
// Name: ToArray
// Object: return an array of pointer to ItemData
//         CALLER HAVE TO FREE ARRAY calling "delete[] Array;"
// Parameters :
//     in  : 
//     out : DWORD* pdwArraySize : returned array size
//     return : array of pointer to elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
PVOID* CLinkListBase::ToArray(DWORD* pdwArraySize)
{
    return this->ToArray(pdwArraySize,FALSE);
}

//-----------------------------------------------------------------------------
// Name: ToArray
// Object: return an array of pointer to ItemData
//         CALLER HAVE TO FREE ARRAY calling "delete[] Array;"
// Parameters :
//     in  : BOOL bUserManagesLock : TRUE is user manages lock
//     out : DWORD* pdwArraySize : returned array size
//     return : array of pointer to elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
PVOID* CLinkListBase::ToArray(DWORD* pdwArraySize,BOOL bUserManagesLock)
{
    PVOID* Array;
    DWORD dwCounter;

    // check param
    if (IsBadWritePtr(pdwArraySize,sizeof(DWORD)))
        return NULL;

    if (!bUserManagesLock)
        // wait for synchro
        this->LockForExternalCalls();

    WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);// must be after this->LockForExternalCalls call

    // get number of Items
    *pdwArraySize=this->GetItemsCount();
    
    if (*pdwArraySize == 0)
    {
        SetEvent(this->hevtInternalLockUnlocked);

        if (!bUserManagesLock)
            this->UnlockForExternalCalls();
        
        return NULL;
    }

    // we just allocate an array to pointer list
    // this is the speediest way as we don't need 
    // to do memory allocation for each Item
    Array=new PVOID[*pdwArraySize];
    CLinkListItem* CurrentItem = this->Head;
    for (dwCounter=0;dwCounter<*pdwArraySize;dwCounter++)// as while (CurrentItem->NextItem != NULL)) but more secure
    {
        Array[dwCounter]=CurrentItem->ItemData;
        CurrentItem = CurrentItem->NextItem;
    }

    SetEvent(this->hevtInternalLockUnlocked);

    if (!bUserManagesLock)
        this->UnlockForExternalCalls();
    
    return Array;
}

//-----------------------------------------------------------------------------
// Name: IsLocked
// Object: allow to know if list is currently locked
// Parameters :
//     return : TRUE if locked, FALSE else
//-----------------------------------------------------------------------------
BOOL CLinkListBase::IsLocked()
{
    switch(WaitForSingleObject(this->hevtListUnlocked,0))
    {
    case WAIT_TIMEOUT:
        return TRUE;
    case WAIT_OBJECT_0:
        // restore unlocked state reseted by WaitForSingleObject
        SetEvent(this->hevtListUnlocked);
        return FALSE;
    default:
        return FALSE;
    }
}

//-----------------------------------------------------------------------------
// Name: GetItem
// Object: Get item at specified address
// Parameters :
//         in : DWORD ItemIndex : 0 based item index
//     return : CLinkListItem if found, NULL else
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListBase::GetItem(DWORD ItemIndex)
{
    return this->GetItem(ItemIndex,FALSE);
}
//-----------------------------------------------------------------------------
// Name: GetItem
// Object: Get item at specified address
// Parameters :
//         in : DWORD ItemIndex : 0 based item index
//              BOOL bUserManagesLock : TRUE if user manages lock
//     return : CLinkListItem if found, NULL else
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListBase::GetItem(DWORD ItemIndex,BOOL bUserManagesLock)
{
    // do a quick index checking
    if (ItemIndex+1>this->GetItemsCount())
        return NULL;

    // lock list if needed
    if (!bUserManagesLock)
        this->LockForExternalCalls();

    WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);// must be after this->LockForExternalCalls call

    CLinkListItem* pReturnedItem=NULL;
    CLinkListItem* pItem;
    DWORD Cnt=0;
    // parse list
    for (pItem=this->Head;pItem;pItem=pItem->NextItem,Cnt++)
    {
        // if Cnt match Index
        if (Cnt==ItemIndex)
        {
            pReturnedItem=pItem;
            break;
        }
    }

    SetEvent(this->hevtInternalLockUnlocked);

    // unlock list if needed
    if (!bUserManagesLock)
        this->UnlockForExternalCalls();

    return pReturnedItem;
}

//-----------------------------------------------------------------------------
// Name: Copy
// Object: copy all elements from pSrc to pDst
// Parameters :
//     in     : CLinkListBase* pSrc : source list
//     in out : CLinkListBase* pDst destination list 
//     out : DWORD* pdwArraySize : returned array size
//     return : array of pointer to elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
BOOL CLinkListBase::Copy(CLinkListBase* pDst,CLinkListBase* pSrc)
{
    return CLinkListBase::Copy(pDst,pSrc,FALSE,FALSE);
}
//-----------------------------------------------------------------------------
// Name: Copy
// Object: copy all elements from pSrc to pDst
// Parameters :
//     in     : CLinkListBase* pSrc : source list
//              BOOL DstLocked : TRUE if user manages pDst Lock
//              BOOL SrcLocked : TRUE if user manages pSrc Lock
//     in out : CLinkListBase* pDst destination list 
//     out : DWORD* pdwArraySize : returned array size
//     return : array of pointer to elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
BOOL CLinkListBase::Copy(CLinkListBase* pDst,CLinkListBase* pSrc,BOOL DstLocked,BOOL SrcLocked)
{
    if (IsBadReadPtr(pSrc,sizeof(CLinkListBase))||IsBadReadPtr(pDst,sizeof(CLinkListBase)))
        return FALSE;

    if(!DstLocked)
        pDst->Lock();
    if(!SrcLocked)
        pSrc->Lock();

    // clear pDst content
    pDst->RemoveAllItems(TRUE);

    // for each item of pSrc
    CLinkListItem* pItem;
    for (pItem=pSrc->Head;pItem;pItem=pItem->NextItem)
    {
        pDst->AddItem(pItem->ItemData,TRUE);
    }

    if(!DstLocked)
        pDst->Unlock();
    if(!SrcLocked)
        pSrc->Unlock();

    return TRUE;
}