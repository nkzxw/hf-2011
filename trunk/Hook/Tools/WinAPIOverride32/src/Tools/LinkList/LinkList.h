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
// Object: provides a Link list
//-----------------------------------------------------------------------------

#pragma once
#include "linklistbase.h"


class CLinkList:public virtual CLinkListBase
{
protected:
    size_t ItemSize;
public:
    CLinkList(size_t ItemSize);
    virtual ~CLinkList(void);

    virtual CLinkListItem* AddItem();
    virtual CLinkListItem* AddItem(BOOL bUserManagesLock);
    virtual CLinkListItem* AddItem(PVOID PointerToItemData);
    virtual CLinkListItem* AddItem(PVOID PointerToItemData,BOOL bUserManagesLock);
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem);
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem,BOOL bUserManagesLock);
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem,PVOID PointerToItemData);
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem,PVOID PointerToItemData,BOOL bUserManagesLock);
    virtual BOOL RemoveItem(CLinkListItem* Item);
    virtual BOOL RemoveItem(CLinkListItem* Item,BOOL bUserManagesLock);
    virtual BOOL RemoveItemFromItemData(PVOID PointerToItemData);
    virtual BOOL RemoveItemFromItemData(PVOID PointerToItemData,BOOL bUserManagesLock);
    virtual BOOL RemoveAllItems();
    virtual BOOL RemoveAllItems(BOOL bUserManagesLock);

    PVOID ToSecureArray(DWORD* pdwArraySize);

};
