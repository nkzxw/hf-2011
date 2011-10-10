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
// Object: provides template linked list
//-----------------------------------------------------------------------------
#pragma once
#include "LinkListTemplate.h"

#pragma message (__FILE__ " Information : don't forget #include \"LinkListTemplate.cpp\"\r\n")

template <class T> 
CLinkListTemplate<T>::CLinkListTemplate()
{
    this->bDeleteObjectMemoryWhenRemovingObjectFromList=TRUE;
    this->pLinkListSimple=new CLinkListSimple();
}
template <class T> 
CLinkListTemplate<T>::~CLinkListTemplate(void)
{
    this->Lock();
    // remove all items
    this->RemoveAllItems(TRUE);
    this->Unlock();
    delete this->pLinkListSimple;
}
template <class T>
CLinkListItemTemplate<T>* CLinkListTemplate<T>::GetHead()
{
    return (CLinkListItemTemplate<T>*)this->pLinkListSimple->Head;
}
template <class T>
CLinkListItemTemplate<T>* CLinkListTemplate<T>::GetTail()
{
    return (CLinkListItemTemplate<T>*)this->pLinkListSimple->Tail;
}
template <class T>
void CLinkListTemplate<T>::SetLockWaitTime(DWORD LockWaitTime)
{
    this->pLinkListSimple->LockWaitTime=LockWaitTime;
}

//-----------------------------------------------------------------------------
// Name: RemoveAllItems
// Object: Remove all items from the list
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplate<T>::RemoveAllItems()
{
    return this->RemoveAllItems(FALSE);
}

//-----------------------------------------------------------------------------
// Name: RemoveAllItems
// Object: delete all items data and remove them from the list
// Parameters :
//     in  : BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplate<T>::RemoveAllItems(BOOL bUserManagesLock)
{
    while (this->pLinkListSimple->Head)
    {
        this->RemoveItem((CLinkListItemTemplate<T>*)(this->pLinkListSimple->Head),bUserManagesLock);
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add Item to the link list allocating necessary memory for pObject
// Parameters :
//     in  : 
//     out :
//     return : CLinkListItemTemplate<T>* Item : pointer to the added item
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplate<T>::AddItem()
{
    return this->AddItem(FALSE);
}
//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add Item to the link list allocating necessary memory for pObject
// Parameters :
//     in  : BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : CLinkListItemTemplate<T>* Item : pointer to the added item
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplate<T>::AddItem(BOOL bUserManagesLock)
{
    return this->AddItem(NULL,bUserManagesLock);
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add Item to the link list allocating necessary memory for pObject
// Parameters :
//     in  : T* pObject : Pointer to object
//     out :
//     return : CLinkListItemTemplate<T>* Item : pointer to the added item
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplate<T>::AddItem(T* pObject)
{
    return this->AddItem(pObject,FALSE);
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add Item to the link list allocating necessary memory for pObject
// Parameters :
//     in  : T* pObject : Pointer to object
//           BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : CLinkListItemTemplate<T>* Item : pointer to the added item
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplate<T>::AddItem(T* pObject,BOOL bUserManagesLock)
{
    return this->InsertItem((CLinkListItemTemplate<T>*)this->pLinkListSimple->Tail,pObject,bUserManagesLock);
}

//-----------------------------------------------------------------------------
// Name: InsertItem
// Object: insert Item to the link list after PreviousItem,
//              allocating necessary memory for pObject
// Parameters :
//     in  : CLinkListItemTemplate<T>* PreviousItem : item after which new item will be added
//                                          Null if added item must be the first one
//     out :
//     return : CLinkListItemTemplate<T>* Item : pointer to the added item
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplate<T>::InsertItem(CLinkListItemTemplate<T>* PreviousItem)
{
    return this->InsertItem(PreviousItem,FALSE);
}

//-----------------------------------------------------------------------------
// Name: InsertItem
// Object: insert Item to the link list after PreviousItem,
//              allocating necessary memory for pObject
// Parameters :
//     in  : CLinkListItemTemplate<T>* PreviousItem : item after which new item will be added
//                                          Null if added item must be the first one
//           BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : CLinkListItemTemplate<T>* Item : pointer to the added item
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplate<T>::InsertItem(CLinkListItemTemplate<T>* PreviousItem,BOOL bUserManagesLock)
{
    return this->InsertItem(PreviousItem,NULL,bUserManagesLock);
}

//-----------------------------------------------------------------------------
// Name: InsertItem
// Object: insert Item to the link list after PreviousItem,
//              allocating necessary memory for pObject and copying pObject
// Parameters :
//     in  : CLinkListItemTemplate<T>* PreviousItem : item after which new item will be added
//                                          Null if added item must be the first one
//           T* pObject : Pointer to object
//     out :
//     return : CLinkListItemTemplate<T>* Item : pointer to the added item
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplate<T>::InsertItem(CLinkListItemTemplate<T>* PreviousItem,T* pObject)
{
    return this->InsertItem(PreviousItem,pObject,FALSE);
}

//-----------------------------------------------------------------------------
// Name: InsertItem
// Object: insert Item to the link list after PreviousItem,
//              allocating necessary memory for pObject and copying pObject
// Parameters :
//     in  : CLinkListItemTemplate<T>* PreviousItem : item after which new item will be added
//                                          Null if added item must be the first one
//           T* pObject : Pointer to object
//           BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : CLinkListItemTemplate<T>* Item : pointer to the added item
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplate<T>::InsertItem(CLinkListItemTemplate<T>* PreviousItem,T* pObject,BOOL bUserManagesLock)
{
    return (CLinkListItemTemplate<T>*)this->pLinkListSimple->InsertItem((CLinkListItem*)PreviousItem,(PVOID)pObject,bUserManagesLock);
}

//-----------------------------------------------------------------------------
// Name: RemoveItem
// Object: Remove item from list and free memory pointed by pObject
// Parameters :
//     in  : CLinkListItemTemplate<T>* Item : pointer to the item to remove
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplate<T>::RemoveItem(CLinkListItemTemplate<T>* Item)
{
    return this->RemoveItem(Item,FALSE);
}

//-----------------------------------------------------------------------------
// Name: RemoveItem
// Object: Remove item from list and free memory pointed by pObject
// Parameters :
//     in  : CLinkListItemTemplate<T>* Item : pointer to the item to remove
//           BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplate<T>::RemoveItem(CLinkListItemTemplate<T>* Item,BOOL bUserManagesLock)
{
    T* pObject=Item->ItemData;
    BOOL bRet = this->pLinkListSimple->RemoveItem((CLinkListItem*)Item,bUserManagesLock);
    if (bRet && this->bDeleteObjectMemoryWhenRemovingObjectFromList)
        delete pObject;
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: RemoveItemFromItemData
// Object: Remove an item identify by its data
//          Data should be unique in list, else first matching item will be removed
// Parameters :
//     in  : T* pObject : pointer to object
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplate<T>::RemoveItemFromItemData(T* pObject)
{
    return this->RemoveItemFromItemData(pObject,FALSE);
}

//-----------------------------------------------------------------------------
// Name: RemoveItemFromItemData
// Object: Remove an object identify by its pointer, and delete this object
//          Data should be unique in list, else first matching item will be removed
// Parameters :
//     in  : T* pObject : pointer to object
//           BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplate<T>::RemoveItemFromItemData(T* pObject,BOOL bUserManagesLock)
{
    BOOL bRet = this->pLinkListSimple->RemoveItemFromItemData((PVOID)pObject,bUserManagesLock);
    if (bRet && this->bDeleteObjectMemoryWhenRemovingObjectFromList)
        delete pObject;
    return bRet;
}


//-----------------------------------------------------------------------------
// Name: SetHeap
// Object: allow to specify a heap for items allocation
//              Allocating all elements in a specified heap allow a quick deletion
//              by calling CLinkListTemplate<T>::Lock; // assume list is locked before destroying memory
//                         ::HeapDestroy();
//                         CLinkListTemplate<T>::ReportHeapDestruction();
//                              NewHeap=::HeapCreate(0,0,0);[optionnal,only if you want to use list again]
//                              CLinkListTemplate<T>::SetHeap(NewHeap);[optionnal,only if you want to use list again]
//                         CLinkListTemplate<T>::Unlock();
//
// Parameters :
//     in  : HANDLE HeapHandle : specified heap handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
void CLinkListTemplate<T>::SetHeap(HANDLE HeapHandle)
{
    this->pLinkListSimple->SetHeap(HeapHandle);
}

//-----------------------------------------------------------------------------
// Name: ReportHeapDestruction
// Object: SEE CLinkListTemplate<T>::SetHeap DOCUMENTATION FOR USING SEQUENCE
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
template <class T> 
void CLinkListTemplate<T>::ReportHeapDestruction()
{
    this->pLinkListSimple->ReportHeapDestruction();
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
//     in  : CLinkListItemTemplate<T>* pItem
//     out :
//     return : TRUE if item is still in list
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplate<T>::IsItemStillInList(CLinkListItemTemplate<T>* pItem)
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
//     in  : CLinkListItemTemplate<T>* pItem
//           BOOL bUserManagesLock : TRUE is user manages Lock
//     out :
//     return : TRUE if item is still in list
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplate<T>::IsItemStillInList(CLinkListItemTemplate<T>* pItem,BOOL bUserManagesLock)
{
    return this->pLinkListSimple->IsItemStillInList((CLinkListItem*)pItem,bUserManagesLock);
}

//-----------------------------------------------------------------------------
// Name: GetItemsCount
// Object: get number of items in the list
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
DWORD CLinkListTemplate<T>::GetItemsCount()
{
    return this->pLinkListSimple->GetItemsCount();
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
template <class T> 
DWORD CLinkListTemplate<T>::Lock()
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
template <class T> 
DWORD CLinkListTemplate<T>::Lock(BOOL bAllowToAddItemDuringLock)
{
    return this->pLinkListSimple->Lock(bAllowToAddItemDuringLock);
}
//-----------------------------------------------------------------------------
// Name: Unlock
// Object: unlock list
// Parameters :
//     in  : 
//     out :
//     return : SetEvent result
//-----------------------------------------------------------------------------
template <class T> 
DWORD CLinkListTemplate<T>::Unlock()
{
    return this->pLinkListSimple->Unlock();
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
template <class T> 
T* CLinkListTemplate<T>::ToArray(DWORD* pdwArraySize)
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
template <class T> 
T* CLinkListTemplate<T>::ToArray(DWORD* pdwArraySize,BOOL bUserManagesLock)
{
    return (T*)this->pLinkListSimple->ToArray(pdwArraySize,bUserManagesLock);
}

//-----------------------------------------------------------------------------
// Name: IsLocked
// Object: allow to know if list is currently locked
// Parameters :
//     return : TRUE if locked, FALSE else
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplate<T>::IsLocked()
{
    return this->pLinkListSimple->IsLocked();
}

//-----------------------------------------------------------------------------
// Name: GetItem
// Object: Get item at specified address
// Parameters :
//         in : DWORD ItemIndex : 0 based item index
//     return : CLinkListItemTemplate<T> if found, NULL else
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplate<T>::GetItem(DWORD ItemIndex)
{
    return this->GetItem(ItemIndex,FALSE);
}
//-----------------------------------------------------------------------------
// Name: GetItem
// Object: Get item at specified address
// Parameters :
//         in : DWORD ItemIndex : 0 based item index
//              BOOL bUserManagesLock : TRUE if user manages lock
//     return : CLinkListItemTemplate<T> if found, NULL else
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplate<T>::GetItem(DWORD ItemIndex,BOOL bUserManagesLock)
{
    return (CLinkListItemTemplate<T>*)this->pLinkListSimple->GetItem(ItemIndex,bUserManagesLock);
}

//-----------------------------------------------------------------------------
// Name: Copy
// Object: copy all elements from pSrc to pDst
// Parameters :
//     in     : CLinkListTemplate<T>* pSrc : source list
//     in out : CLinkListTemplate<T>* pDst destination list 
//     out : DWORD* pdwArraySize : returned array size
//     return : array of pointer to elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplate<T>::Copy(CLinkListTemplate<T>* pDst,CLinkListTemplate<T>* pSrc)
{
    return CLinkListTemplate<T>::Copy(pDst,pSrc,FALSE,FALSE);
}
//-----------------------------------------------------------------------------
// Name: Copy
// Object: copy all elements from pSrc to pDst
// Parameters :
//     in     : CLinkListTemplate<T>* pSrc : source list
//              BOOL DstLocked : TRUE if user manages pDst Lock
//              BOOL SrcLocked : TRUE if user manages pSrc Lock
//     in out : CLinkListTemplate<T>* pDst destination list 
//     out : DWORD* pdwArraySize : returned array size
//     return : array of pointer to elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplate<T>::Copy(CLinkListTemplate<T>* pDst,CLinkListTemplate<T>* pSrc,BOOL DstLocked,BOOL SrcLocked)
{
    if (IsBadReadPtr(pSrc,sizeof(CLinkListTemplate<T>))||IsBadReadPtr(pDst,sizeof(CLinkListTemplate<T>)))
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