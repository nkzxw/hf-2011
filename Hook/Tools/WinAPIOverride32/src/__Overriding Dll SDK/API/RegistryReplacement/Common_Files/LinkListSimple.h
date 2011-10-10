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
// Object: provides a speed Link list for item with size less than sizeof(PVOID)
//-----------------------------------------------------------------------------

#pragma once
#include "linklistbase.h"

namespace EmulatedRegistry
{
class CLinkListSimple:public virtual CLinkListBase
{
protected:
public:
    CLinkListSimple();
    virtual ~CLinkListSimple();

    virtual CLinkListItem* AddItem();
    virtual CLinkListItem* AddItem(BOOL bUserManagesLock);
    virtual CLinkListItem* AddItem(PVOID ItemData);
    virtual CLinkListItem* AddItem(PVOID ItemData,BOOL bUserManagesLock);
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem);
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem,BOOL bUserManagesLock);
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem,PVOID ItemData);
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem,PVOID ItemData,BOOL bUserManagesLock);
    virtual BOOL RemoveItem(CLinkListItem* Item);
    virtual BOOL RemoveItem(CLinkListItem* Item,BOOL bUserManagesLock);
    virtual BOOL RemoveItemFromItemData(PVOID ItemData);
    virtual BOOL RemoveItemFromItemData(PVOID ItemData,BOOL bUserManagesLock);
    virtual BOOL RemoveAllItems();
    virtual BOOL RemoveAllItems(BOOL bUserManagesLock);
};
}