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
#include "linklistsimple.h"
#include "LinkListItemTemplate.h"

template <class T> 
class CLinkListTemplate
{
protected:
    CLinkListSimple* pLinkListSimple;
public:
    CLinkListTemplate();
    virtual ~CLinkListTemplate();
    CLinkListItemTemplate<T>* GetHead();
    CLinkListItemTemplate<T>* GetTail();
    void SetLockWaitTime(DWORD LockWaitTime);
    BOOL bDeleteObjectMemoryWhenRemovingObjectFromList;

    CLinkListItemTemplate<T>* AddItem();
    CLinkListItemTemplate<T>* AddItem(BOOL bUserManagesLock);
    CLinkListItemTemplate<T>* AddItem(T* pObject);
    CLinkListItemTemplate<T>* AddItem(T* pObject,BOOL bUserManagesLock);
    CLinkListItemTemplate<T>* InsertItem(CLinkListItemTemplate<T>* PreviousItem);
    CLinkListItemTemplate<T>* InsertItem(CLinkListItemTemplate<T>* PreviousItem,BOOL bUserManagesLock);
    CLinkListItemTemplate<T>* InsertItem(CLinkListItemTemplate<T>* PreviousItem,T* pObject);
    CLinkListItemTemplate<T>* InsertItem(CLinkListItemTemplate<T>* PreviousItem,T* pObject,BOOL bUserManagesLock);
    BOOL RemoveItem(CLinkListItemTemplate<T>* Item);
    BOOL RemoveItem(CLinkListItemTemplate<T>* Item,BOOL bUserManagesLock);
    BOOL RemoveItem(CLinkListItemTemplate<T>* Item,BOOL bUserManagesLock,BOOL DeleteObjectMemory);
    BOOL RemoveItemFromItemData(T* pObject);
    BOOL RemoveItemFromItemData(T* pObject,BOOL bUserManagesLock);
    BOOL RemoveItemFromItemData(T* pObject,BOOL bUserManagesLock,BOOL DeleteObjectMemory);
    BOOL RemoveAllItems();
    BOOL RemoveAllItems(BOOL bUserManagesLock);
    BOOL RemoveAllItems(BOOL bUserManagesLock,BOOL DeleteObjectsMemory);

    void SetHeap(HANDLE HeapHandle);
    void ReportHeapDestruction();
    DWORD GetItemsCount();
    T* ToArray(DWORD* pdwArraySize);
    T* ToArray(DWORD* pdwArraySize,BOOL bUserManagesLock);
    DWORD Lock(BOOL bAllowToAddItemDuringLock);
    DWORD Lock();
    DWORD Unlock();
    BOOL IsLocked();
    BOOL IsItemStillInList(CLinkListItemTemplate<T>* pItem);
    BOOL IsItemStillInList(CLinkListItemTemplate<T>* pItem,BOOL bUserManagesLock);
    CLinkListItemTemplate<T>* GetItem(DWORD ItemIndex);
    CLinkListItemTemplate<T>* GetItem(DWORD ItemIndex,BOOL bUserManagesLock);
    static BOOL Copy(CLinkListTemplate<T>* pDst,CLinkListTemplate<T>* pSrc);
    static BOOL Copy(CLinkListTemplate<T>* pDst,CLinkListTemplate<T>* pSrc,BOOL DstLocked,BOOL SrcLocked);
};
