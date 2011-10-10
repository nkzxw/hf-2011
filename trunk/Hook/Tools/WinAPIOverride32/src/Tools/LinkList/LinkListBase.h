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

#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#include "LinkListItem.h"


class CLinkListBase
{
protected:
    HANDLE LinkListHeap;
    HANDLE hevtListUnlocked;
    HANDLE hevtInternalLockUnlocked;
    DWORD  ItemsNumber;
    BOOL bAllowToAddItemDuringLock;

    CLinkListBase();
    virtual ~CLinkListBase();
    DWORD LockForExternalCalls();
    DWORD UnlockForExternalCalls();

public:
    CLinkListItem* Head;
    CLinkListItem* Tail;
    DWORD  LockWaitTime;
    void SetHeap(HANDLE HeapHandle);
    HANDLE GetHeap();
    void ReportHeapDestruction();

    virtual CLinkListItem* AddItem()=0;
    virtual CLinkListItem* AddItem(BOOL bUserManagesLock)=0;
    virtual CLinkListItem* AddItem(PVOID ItemData)=0;
    virtual CLinkListItem* AddItem(PVOID ItemData,BOOL bUserManagesLock)=0;
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem)=0;
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem,BOOL bUserManagesLock)=0;
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem,PVOID ItemData)=0;
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem,PVOID ItemData,BOOL bUserManagesLock)=0;
    virtual BOOL RemoveItem(CLinkListItem* Item)=0;
    virtual BOOL RemoveItem(CLinkListItem* Item,BOOL bUserManagesLock)=0;
    virtual BOOL RemoveItemFromItemData(PVOID ItemData)=0;
    virtual BOOL RemoveItemFromItemData(PVOID ItemData,BOOL bUserManagesLock)=0;
    virtual BOOL RemoveAllItems()=0;// must be pure virtual has it call the pure virtual RemoveItem method
    virtual BOOL RemoveAllItems(BOOL bUserManagesLock)=0;// must be pure virtual has it call the pure virtual RemoveItem method
    DWORD GetItemsCount();
    PVOID* ToArray(DWORD* pdwArraySize);
    PVOID* ToArray(DWORD* pdwArraySize,BOOL bUserManagesLock);
    DWORD Lock(BOOL bAllowToAddItemDuringLock);
    DWORD Lock();
    DWORD Unlock();
    BOOL IsLocked();
    BOOL IsEmpty();
    BOOL IsItemStillInList(CLinkListItem* pItem);
    BOOL IsItemStillInList(CLinkListItem* pItem,BOOL bUserManagesLock);
    CLinkListItem* GetItem(DWORD ItemIndex);
    CLinkListItem* GetItem(DWORD ItemIndex,BOOL bUserManagesLock);
    static BOOL Copy(CLinkListBase* pDst,CLinkListBase*pSrc);
    static BOOL Copy(CLinkListBase* pDst,CLinkListBase*pSrc,BOOL DstLocked,BOOL SrcLocked);
};
