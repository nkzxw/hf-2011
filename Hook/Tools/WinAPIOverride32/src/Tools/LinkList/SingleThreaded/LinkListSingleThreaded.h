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
#include "LinkListBaseSingleThreaded.h"


class CLinkListSingleThreaded:public virtual CLinkListBaseSingleThreaded
{
protected:
    size_t ItemSize;
public:
    CLinkListSingleThreaded(size_t ItemSize);
    virtual ~CLinkListSingleThreaded(void);

    virtual CLinkListItem* AddItem();
    virtual CLinkListItem* AddItem(PVOID PointerToItemData);
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem);
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem,PVOID PointerToItemData);
    virtual BOOL RemoveItem(CLinkListItem* Item);
    virtual BOOL RemoveItemFromItemData(PVOID PointerToItemData);
    virtual BOOL RemoveAllItems();

    PVOID ToSecureArray(DWORD* pdwArraySize);

};
