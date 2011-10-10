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

#include "LinkListSingleThreaded.h"
#pragma intrinsic (memcpy,memset,memcmp)

CLinkListSingleThreaded::CLinkListSingleThreaded(size_t ItemSize)
{
    this->ItemSize=ItemSize;
}

// user must have lock when calling this function (necessary to avoid deadlocks)
CLinkListSingleThreaded::~CLinkListSingleThreaded(void)
{
    // remove all items
    this->RemoveAllItems();
}

//-----------------------------------------------------------------------------
// Name: RemoveAllItems
// Object: Remove all items from the list
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CLinkListSingleThreaded::RemoveAllItems()
{
    while (this->Head)
        this->RemoveItem(this->Head);
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
CLinkListItem* CLinkListSingleThreaded::AddItem()
{
    return this->AddItem(NULL);
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add Item to the link list allocating necessary memory for ItemData
// Parameters :
//     in  : PVOID PointerToItemData : pointer to data to add
//     out :
//     return : CLinkListItem* Item : pointer to the added item
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListSingleThreaded::AddItem(PVOID PointerToItemData)
{
    return this->InsertItem(this->Tail,PointerToItemData);
}

//-----------------------------------------------------------------------------
// Name: InsertItem
// Object: insert Item to the link list after PreviousItem, allocating 
//         necessary memory for ItemData and copying data pointed by PointerToItemData
// Parameters :
//     in  : CLinkListItem* PreviousItem : item after which new item will be added
//                                          Null if added item must be the first one
//     out :
//     return : CLinkListItem* Item : pointer to the added item
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListSingleThreaded::InsertItem(CLinkListItem* PreviousItem)
{
    return this->InsertItem(PreviousItem,NULL);
}

//-----------------------------------------------------------------------------
// Name: InsertItem
// Object: insert Item to the link list after PreviousItem, allocating 
//         necessary memory for ItemData and copying data pointed by PointerToItemData
// Parameters :
//     in  : CLinkListItem* PreviousItem : item after which new item will be added
//                                          Null if added item must be the first one
//           PVOID PointerToItemData : pointer to data to add
//     out :
//     return : CLinkListItem* Item : pointer to the added item
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListSingleThreaded::InsertItem(CLinkListItem* PreviousItem,PVOID PointerToItemData)
{
    CLinkListItem* Item;
  

    // Item = new CLinkListItem();
    Item = (CLinkListItem*)HeapAlloc(this->LinkListHeap,HEAP_ZERO_MEMORY,sizeof(CLinkListItem));
    if (Item==NULL)
        return NULL;

    // allocate memory for item data 
    Item->ItemData = HeapAlloc(this->LinkListHeap, HEAP_ZERO_MEMORY, this->ItemSize);

    // check allocation
    if (Item->ItemData==NULL)
    {
        // delete Item;
        HeapFree(this->LinkListHeap,0,Item);
        return NULL;
    }

    // copy data before adding item
    if (PointerToItemData)
        // copy data information
        memcpy(Item->ItemData,PointerToItemData,this->ItemSize);

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

            // store PreviousItem
            Item->PreviousItem=PreviousItem;

            // update NextItem->PreviousItem=Item
            if (Item->NextItem)
                Item->NextItem->PreviousItem=Item;
            // update PreviousItem->NextItem
            PreviousItem->NextItem=Item;
        }
    } 

    this->ItemsNumber++;

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
BOOL CLinkListSingleThreaded::RemoveItem(CLinkListItem* Item)
{
#if _DEBUG
    // assume item is still in list
    if (!this->IsItemStillInList(Item))
    {
        if (IsDebuggerPresent())
            DebugBreak();
        return FALSE;
    }
#endif

    if (this->Head == NULL)
        return FALSE;

    if (Item == this->Head)
    {
        if (this->Head == this->Tail) 
        {
            this->Head = NULL;
            this->Tail = NULL;

            // free item data memory
            HeapFree(this->LinkListHeap, 0, Item->ItemData);

            // put memory to null to allow quick memory checking
            Item->ItemData=NULL;
            Item->NextItem=NULL;

            // free item itself
            // delete Item;
            HeapFree(this->LinkListHeap,0,Item);
            
        }
        else
        {
            this->Head = this->Head->NextItem;// must be done before freeing item
            // clear new head previous item
            this->Head->PreviousItem=NULL;

            // free item data memory
            HeapFree(this->LinkListHeap, 0, Item->ItemData);

            // put memory to null to allow quick memory checking
            Item->ItemData=NULL;
            Item->NextItem=NULL;

            // free item itself
            // delete Item;
            HeapFree(this->LinkListHeap,0,Item);
        }

        this->ItemsNumber--;
        return TRUE;
    }

    // item is not the head.
    // check previous item validity
    if (IsBadReadPtr(Item->PreviousItem,sizeof(CLinkListItem*)))
        return FALSE;

    // update PreviousItem->NextItem
    Item->PreviousItem->NextItem=Item->NextItem;

    if (Item == this->Tail)
        // update Tail
        this->Tail = Item->PreviousItem;
    else
        // update NextItem->PreviousItem
        Item->NextItem->PreviousItem=Item->PreviousItem;

    // free item data memory
    HeapFree(this->LinkListHeap, 0, Item->ItemData);

    // put memory to null to allow quick memory checking
    Item->ItemData=NULL;
    Item->NextItem=NULL;

    // free item itself
    // delete Item;
    HeapFree(this->LinkListHeap,0,Item);

    this->ItemsNumber--;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RemoveItemFromItemData
// Object: Remove an item identify by its data
//          Data should be unique in list, else first matching item will be removed
// Parameters :
//     in  : PointerToItemData : a pointer to Item data
//                               check is done for content of data
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CLinkListSingleThreaded::RemoveItemFromItemData(PVOID PointerToItemData)
{
    if (IsBadReadPtr(PointerToItemData,this->ItemSize))
        return FALSE;

    CLinkListItem* CurrentItem = this->Head;
    CLinkListItem* Item;
    if (this->Head == NULL)
        return FALSE;

    if (memcmp(PointerToItemData,this->Head->ItemData,this->ItemSize)==0)
    {
        Item=this->Head;
        if (this->Head == this->Tail) 
        {
            this->Head = NULL;
            this->Tail = NULL;

            // free item data memory
            HeapFree(this->LinkListHeap, 0, Item->ItemData);

            // put memory to null to allow quick memory checking
            Item->ItemData=NULL;
            Item->NextItem=NULL;

            // free item itself
            // delete Item;
            HeapFree(this->LinkListHeap,0,Item);
        }
        else
        {
            this->Head = this->Head->NextItem;// must be done before freeing item

            // free item data memory
            HeapFree(this->LinkListHeap, 0, Item->ItemData);

            // put memory to null to allow quick memory checking
            Item->ItemData=NULL;
            Item->NextItem=NULL;

            // free item itself
            // delete Item;
            HeapFree(this->LinkListHeap,0,Item);
        }

        this->ItemsNumber--;
        return TRUE;
    }

    while (CurrentItem->NextItem != NULL)
    {
        if (memcmp(CurrentItem->NextItem->ItemData,PointerToItemData,this->ItemSize)==0)
            break;
        CurrentItem = CurrentItem->NextItem;
    }

    // if item not found
    if (CurrentItem->NextItem==NULL)
        return FALSE;

    Item=CurrentItem->NextItem;
    CurrentItem->NextItem = Item->NextItem;

    if (Item == this->Tail)
        this->Tail = CurrentItem;

    // free item data memory
    HeapFree(this->LinkListHeap, 0, Item->ItemData);

    // put memory to null to allow quick memory checking
    Item->ItemData=NULL;
    Item->NextItem=NULL;

    // free item itself
    // delete Item;
    HeapFree(this->LinkListHeap,0,Item);

    this->ItemsNumber--;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ToSecureArray
// Object: return an array of pointed value of ItemData (that means if you 
//           store an object 'obj' you'll get an obj* as result)
//         Items are accessible even if they are removed from list and list object is destroyed
//         CALLER HAVE TO FREE ARRAY calling "delete[] Array;"
// Parameters :
//     in  : 
//     out : DWORD* pdwArraySize : returned array size
//     return : array of elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
PVOID CLinkListSingleThreaded::ToSecureArray(DWORD* pdwArraySize)
{
    BYTE* Array;
    DWORD dwCounter;

    // check param
    if (IsBadWritePtr(pdwArraySize,sizeof(DWORD)))
        return NULL;

    // get number of Items
    *pdwArraySize=this->GetItemsCount();
    
    if (*pdwArraySize == 0)
        return NULL;

    // we just allocate an array to pointer list
    // this is the speediest way as we don't need 
    // to do memory allocation for each Item
    Array=new BYTE[(*pdwArraySize)*this->ItemSize];
    CLinkListItem* CurrentItem = this->Head;
    for (dwCounter=0;dwCounter<*pdwArraySize;dwCounter++)// as while (CurrentItem->NextItem != NULL)) but more secure
    {
        memcpy(&Array[dwCounter*this->ItemSize],CurrentItem->ItemData,this->ItemSize);
        CurrentItem = CurrentItem->NextItem;
    }
    return (PVOID)Array;
}