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

#include "linklistsimple.h"
CLinkListSimple::CLinkListSimple()
{
}

CLinkListSimple::~CLinkListSimple(void)
{
    this->Lock();
    // remove all items
    this->RemoveAllItems(TRUE);
}

//-----------------------------------------------------------------------------
// Name: RemoveAllItems
// Object: Remove all items from the list
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CLinkListSimple::RemoveAllItems()
{
    return this->RemoveAllItems(FALSE);
}

//-----------------------------------------------------------------------------
// Name: RemoveAllItems
// Object: Remove all items from the list
// Parameters :
//     in  : BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CLinkListSimple::RemoveAllItems(BOOL bUserManagesLock)
{
    while (this->Head)
        this->RemoveItem(this->Head,bUserManagesLock);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add Item to the link list allocating necessary memory for ItemData
// Parameters :
//     in  : 
//     out :
//     return : CLinkListItem* Item : pointer to the added item
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListSimple::AddItem()
{
    return this->AddItem(FALSE);
}
//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add Item to the link list allocating necessary memory for ItemData
// Parameters :
//     in  : BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : CLinkListItem* Item : pointer to the added item
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListSimple::AddItem(BOOL bUserManagesLock)
{
    return this->AddItem(NULL,bUserManagesLock);
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add Item to the link list allocating necessary memory for ItemData
// Parameters :
//     in  : PVOID ItemData : data value (DWORD, BYTE, ...) casted to PVOID
//     out :
//     return : CLinkListItem* Item : pointer to the added item
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListSimple::AddItem(PVOID ItemData)
{
    return this->AddItem(ItemData,FALSE);
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add Item to the link list allocating necessary memory for ItemData
// Parameters :
//     in  : PVOID ItemData : data value (DWORD, BYTE, ...) casted to PVOID
//           BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : CLinkListItem* Item : pointer to the added item
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListSimple::AddItem(PVOID ItemData,BOOL bUserManagesLock)
{
    return this->InsertItem(this->Tail,ItemData,bUserManagesLock);
}

//-----------------------------------------------------------------------------
// Name: InsertItem
// Object: insert Item to the link list after PreviousItem,
//              allocating necessary memory for ItemData
// Parameters :
//     in  : CLinkListItem* PreviousItem : item after which new item will be added
//                                          Null if added item must be the first one
//     out :
//     return : CLinkListItem* Item : pointer to the added item
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListSimple::InsertItem(CLinkListItem* PreviousItem)
{
    return this->InsertItem(PreviousItem,FALSE);
}

//-----------------------------------------------------------------------------
// Name: InsertItem
// Object: insert Item to the link list after PreviousItem,
//              allocating necessary memory for ItemData
// Parameters :
//     in  : CLinkListItem* PreviousItem : item after which new item will be added
//                                          Null if added item must be the first one
//           BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : CLinkListItem* Item : pointer to the added item
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListSimple::InsertItem(CLinkListItem* PreviousItem,BOOL bUserManagesLock)
{
    return this->InsertItem(PreviousItem,NULL,bUserManagesLock);
}

//-----------------------------------------------------------------------------
// Name: InsertItem
// Object: insert Item to the link list after PreviousItem,
//              allocating necessary memory for ItemData and copying ItemData
// Parameters :
//     in  : CLinkListItem* PreviousItem : item after which new item will be added
//                                          Null if added item must be the first one
//           PVOID ItemData : data value (DWORD, BYTE, ...) casted to PVOID
//     out :
//     return : CLinkListItem* Item : pointer to the added item
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListSimple::InsertItem(CLinkListItem* PreviousItem,PVOID ItemData)
{
    return this->InsertItem(PreviousItem,ItemData,FALSE);
}

//-----------------------------------------------------------------------------
// Name: InsertItem
// Object: insert Item to the link list after PreviousItem,
//              allocating necessary memory for ItemData and copying ItemData
// Parameters :
//     in  : CLinkListItem* PreviousItem : item after which new item will be added
//                                          Null if added item must be the first one
//           PVOID ItemData : data value (DWORD, BYTE, ...) casted to PVOID
//           BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : CLinkListItem* Item : pointer to the added item
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListSimple::InsertItem(CLinkListItem* PreviousItem,PVOID ItemData,BOOL bUserManagesLock)
{
    CLinkListItem* Item;
    BOOL bLocked=FALSE;

    if ((!this->bAllowToAddItemDuringLock)&&(!bUserManagesLock))
    {
        this->LockForExternalCalls();
        bLocked=TRUE;
    }

    WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);// must be after this->LockForExternalCalls call

    // Item = new CLinkListItem();
    Item = (CLinkListItem*)HeapAlloc(this->LinkListHeap,HEAP_ZERO_MEMORY,sizeof(CLinkListItem));
    if (Item==NULL)
    {
        SetEvent(this->hevtInternalLockUnlocked);
        if (bLocked)
            this->UnlockForExternalCalls();
        
        return NULL;
    }

    // add data to item before adding it to list
    Item->ItemData=ItemData;

    if (this->ItemsNumber==0)
    {
        this->Head = Item;
        this->Tail = Item;
        Item->NextItem=NULL;
        Item->PreviousItem=NULL;
    }
    else
    {
        // if previous item was tail, update new tail
        if (PreviousItem==this->Tail)
            this->Tail=Item;

        // if Item should be the first one
        if (PreviousItem==NULL)
        {
            // item replace head
            Item->NextItem=this->Head;
            Item->PreviousItem=NULL;

            // update head previous item
            Item->NextItem->PreviousItem=Item;
            this->Head=Item;
        }
        else
        {
            // insert item into list

            // store PreviousItem->NextItem into Item->NextItem
            Item->NextItem=PreviousItem->NextItem;
            // store previous item
            Item->PreviousItem=PreviousItem;

            // update NextItem->PreviousItem=Item
            if (Item->NextItem)
                Item->NextItem->PreviousItem=Item;
            // update PreviousItem->NextItem
            PreviousItem->NextItem=Item;
        }
    }  

    this->ItemsNumber++;

    SetEvent(this->hevtInternalLockUnlocked);
    if (bLocked)
        this->UnlockForExternalCalls();

    return Item;
}

//-----------------------------------------------------------------------------
// Name: RemoveItem
// Object: Remove item from list and free memory pointed by ItemData
// Parameters :
//     in  : CLinkListItem* Item : pointer to the item to remove
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CLinkListSimple::RemoveItem(CLinkListItem* Item)
{
    return this->RemoveItem(Item,FALSE);
}

//-----------------------------------------------------------------------------
// Name: RemoveItem
// Object: Remove item from list and free memory pointed by ItemData
// Parameters :
//     in  : CLinkListItem* Item : pointer to the item to remove
//           BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CLinkListSimple::RemoveItem(CLinkListItem* Item,BOOL bUserManagesLock)
{

#if _DEBUG
    // assume item is still in list
    if (!this->IsItemStillInList(Item,bUserManagesLock))
    {
        if (IsDebuggerPresent())
            DebugBreak();
        return FALSE;
    }
#endif

    if(!bUserManagesLock)
        this->LockForExternalCalls();

    WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);// must be after this->LockForExternalCalls call

    if (this->Head == NULL)
    {
        SetEvent(this->hevtInternalLockUnlocked);
        if(!bUserManagesLock)
            this->UnlockForExternalCalls();
        return FALSE;
    }
    // if first item
    if (Item == this->Head)
    {
        // if only one element in list
        if (this->Head == this->Tail) 
        {
            // clear Head and Tail
            this->Head = NULL;
            this->Tail = NULL;
            // free item itself
            // delete Item;
            HeapFree(this->LinkListHeap,0,Item);
        }
        else
        {
            // update Head with the next item
            this->Head = this->Head->NextItem;// must be done before freeing item
            // clear new head previous item
            this->Head->PreviousItem=NULL;
            // free item itself
            // delete Item;
            HeapFree(this->LinkListHeap,0,Item);
        }

        this->ItemsNumber--;
        SetEvent(this->hevtInternalLockUnlocked);
        if(!bUserManagesLock)
            this->UnlockForExternalCalls();
        return TRUE;
    }

    // item is not the head.
    // check previous item validity
    if (IsBadReadPtr(Item->PreviousItem,sizeof(CLinkListItem*)))
    {
        SetEvent(this->hevtInternalLockUnlocked);
        if(!bUserManagesLock)
            this->UnlockForExternalCalls();
        return FALSE;
    }

    // update PreviousItem->NextItem
    Item->PreviousItem->NextItem=Item->NextItem;

    // if item is tail
    if (Item == this->Tail)
        // update Tail
        this->Tail = Item->PreviousItem;
    else
        // update NextItem->PreviousItem
        Item->NextItem->PreviousItem=Item->PreviousItem;

    // free item itself
    // delete Item;
    HeapFree(this->LinkListHeap,0,Item);

    this->ItemsNumber--;
    SetEvent(this->hevtInternalLockUnlocked);
    if(!bUserManagesLock)
        this->UnlockForExternalCalls();

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RemoveItemFromItemData
// Object: Remove an item identify by its data
//          Data should be unique in list, else first matching item will be removed
// Parameters :
//     in  : ItemData : Item data
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CLinkListSimple::RemoveItemFromItemData(PVOID ItemData)
{
    return this->RemoveItemFromItemData(ItemData,FALSE);
}

//-----------------------------------------------------------------------------
// Name: RemoveItemFromItemData
// Object: Remove an item identify by its data
//          Data should be unique in list, else first matching item will be removed
// Parameters :
//     in  : ItemData : Item data
//           BOOL bUserManagesLock : if user has already take the lock manually, 
//                                 and knows current operation can be execute with security in 
//                                 multi thread application
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CLinkListSimple::RemoveItemFromItemData(PVOID ItemData,BOOL bUserManagesLock)
{
    if(!bUserManagesLock)
        this->LockForExternalCalls();

    WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);// must be after this->LockForExternalCalls call

    CLinkListItem* CurrentItem = this->Head;
    CLinkListItem* Item;
    if (this->Head == NULL)
    {
        SetEvent(this->hevtInternalLockUnlocked);
        if(!bUserManagesLock)
            this->UnlockForExternalCalls();
        
        return FALSE;
    }

    if (this->Head->ItemData==ItemData)
    {
        Item=this->Head;
        if (this->Head == this->Tail) 
        {
            this->Head = NULL;
            this->Tail = NULL;
            // free item itself
            // delete Item;
            HeapFree(this->LinkListHeap,0,Item);
        }
        else
        {
            this->Head = this->Head->NextItem;// must be done before freeing item

            // free item itself
            // delete Item;
            HeapFree(this->LinkListHeap,0,Item);
        }

        this->ItemsNumber--;
        SetEvent(this->hevtInternalLockUnlocked);
        if(!bUserManagesLock)
            this->UnlockForExternalCalls();
        
        return TRUE;
    }

    while (CurrentItem->NextItem != NULL)
    {
        if (CurrentItem->NextItem->ItemData==ItemData)
            break;
        CurrentItem = CurrentItem->NextItem;
    }

    // if item not found
    if (CurrentItem->NextItem==NULL)
    {
        SetEvent(this->hevtInternalLockUnlocked);
        if(!bUserManagesLock)
            this->UnlockForExternalCalls();
        return FALSE;
    }
    Item=CurrentItem->NextItem;
    CurrentItem->NextItem = Item->NextItem;

    if (Item == this->Tail)
        this->Tail = CurrentItem;

    // free item itself
    // delete Item;
    HeapFree(this->LinkListHeap,0,Item);

    this->ItemsNumber--;
    SetEvent(this->hevtInternalLockUnlocked);
    if(!bUserManagesLock)
        this->UnlockForExternalCalls();
    return TRUE;
}