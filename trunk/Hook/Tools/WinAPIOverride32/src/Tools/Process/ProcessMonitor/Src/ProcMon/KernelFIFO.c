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
// Object: Fifo in kernel mode
//-----------------------------------------------------------------------------


#include "kernelfifo.h"

//-----------------------------------------------------------------------------
// Name: KernelFIFONew
// Object: Fifo constructor
// Parameters :
//     in  : 
//     out :
//     return : PKERNEL_FIFO : Pointer to an allocated fifo, NULL on error
//-----------------------------------------------------------------------------
PKERNEL_FIFO KernelFIFONew(unsigned long ItemSize)
{
    PKERNEL_FIFO pKernelFIFO;
    pKernelFIFO=ExAllocatePoolWithTag(NonPagedPool,sizeof(KERNEL_FIFO),KERNEL_FIFO_MEMORY_TAG);
    if (!pKernelFIFO)
        return NULL;
    pKernelFIFO->ItemSize=ItemSize;
    pKernelFIFO->Head=NULL;
    pKernelFIFO->Tail=NULL;
    KeInitializeEvent(&pKernelFIFO->hevtListUnlocked,SynchronizationEvent,TRUE);
    pKernelFIFO->ItemsNumber=0;

    return pKernelFIFO;
}

//-----------------------------------------------------------------------------
// Name: KernelFIFODelete
// Object: Destructor: Flush the fifo and free memory associated with pKernelFIFO
// Parameters :
//     in  : PKERNEL_FIFO pKernelFIFO : fifo to free
//     out :
//     return : 
//-----------------------------------------------------------------------------
void KernelFIFODelete(PKERNEL_FIFO pKernelFIFO)
{
    if (!pKernelFIFO)
        return;

    // remove all items
    KernelFIFOFlush(pKernelFIFO);

    ExFreePoolWithTag(pKernelFIFO,KERNEL_FIFO_MEMORY_TAG);
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add Item to the fifo allocating necessary memory for ItemData
// Parameters :
//     in  : 
//     out :
//     return : PKERNEL_FIFO_ITEM Item : pointer to the added item
//-----------------------------------------------------------------------------
PKERNEL_FIFO_ITEM KernelFIFOAddItem(PKERNEL_FIFO pKernelFIFO)
{
    PKERNEL_FIFO_ITEM pItem;

    if (!pKernelFIFO)
        return NULL;

    KeWaitForSingleObject(&pKernelFIFO->hevtListUnlocked,Executive,KernelMode,FALSE,NULL);

    pItem=ExAllocatePoolWithTag(NonPagedPool,sizeof(KERNEL_FIFO_ITEM),KERNEL_FIFO_MEMORY_TAG);

    if (pItem==NULL)
    {
        KeSetEvent(&pKernelFIFO->hevtListUnlocked,IO_NO_INCREMENT,FALSE);
        return NULL;
    }

    // allocate memory for item data 
    pItem->ItemData=ExAllocatePoolWithTag(NonPagedPool,pKernelFIFO->ItemSize,KERNEL_FIFO_MEMORY_TAG);
    if (!pItem->ItemData)
    {
        // free previeus allocated memory
        ExFreePoolWithTag(pItem,KERNEL_FIFO_MEMORY_TAG);
        // set unlocked event
        KeSetEvent(&pKernelFIFO->hevtListUnlocked,IO_NO_INCREMENT,FALSE);
        return NULL;
    }
    // init struct to 0
    RtlZeroMemory(pItem->ItemData,pKernelFIFO->ItemSize);

    if (pKernelFIFO->Head == NULL)
    {
        pKernelFIFO->Head = pItem;
        pKernelFIFO->Tail = pItem;
    }
    else
    {
        pKernelFIFO->Tail->NextItem = pItem;
        pKernelFIFO->Tail = pItem;
    }  
    // update Tail
    pKernelFIFO->Tail->NextItem = NULL;

    // increase items number
    pKernelFIFO->ItemsNumber++;

    // set unlocked event
    KeSetEvent(&pKernelFIFO->hevtListUnlocked,IO_NO_INCREMENT,FALSE);

    return pItem;
}

//-----------------------------------------------------------------------------
// Name: AddItemEx
// Object: Add Item to the fifo allocating necessary memory for ItemData
// Parameters :
//     in  : 
//     out :
//     return : PKERNEL_FIFO_ITEM Item : pointer to the added item
//-----------------------------------------------------------------------------
PKERNEL_FIFO_ITEM KernelFIFOAddItemEx(PKERNEL_FIFO pKernelFIFO,PVOID ItemData)
{
    PKERNEL_FIFO_ITEM pKernelFIFOItem=KernelFIFOAddItem(pKernelFIFO);

    if (!pKernelFIFO)
        return NULL;

    if (pKernelFIFOItem==NULL)
        return NULL;

    // copy data information
    RtlCopyMemory(pKernelFIFOItem->ItemData,ItemData,pKernelFIFO->ItemSize);
    return pKernelFIFOItem;
}


//-----------------------------------------------------------------------------
// Name: Flush
// Object: Remove all items from the fifo
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void KernelFIFOFlush(PKERNEL_FIFO pKernelFIFO)
{
    if (!pKernelFIFO)
        return;

    while (KernelFIFOPopEx(pKernelFIFO,NULL,FALSE))
    {}
}

//-----------------------------------------------------------------------------
// Name: Push
// Object: push data to FIFO. As a local copy of data is done for fifo, pData can
//         be delete after push call
// Parameters :
//     in  : pData
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOLEAN KernelFIFOPush(PKERNEL_FIFO pKernelFIFO,PVOID pData)
{
    PKERNEL_FIFO_ITEM pKernelFIFOItem;
    if (!pKernelFIFO)
        return FALSE;

    pKernelFIFOItem=KernelFIFOAddItemEx(pKernelFIFO,pData);

    if (pKernelFIFOItem==NULL)
        return FALSE;
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: Pop
// Object: pop data from FIFO
// Parameters :
//     in  :
//     out : PVOID pItemData : pointer to data. Pointed buffer must be 
//                             large enought to contain data 
//                             (size must be ItemSize see constructor)
//     return : TRUE on success, FALSE if pointed buffer space is not sufficent 
//               or if FIFO is Empty
//-----------------------------------------------------------------------------
BOOLEAN KernelFIFOPop(PKERNEL_FIFO pKernelFIFO,PVOID pItemData)
{
    if (!pKernelFIFO)
        return FALSE;
    return KernelFIFOPopEx(pKernelFIFO,pItemData,TRUE);
}
//-----------------------------------------------------------------------------
// Name: PopEx
// Object: pop data from FIFO
// Parameters :
//     in  :
//           BOOLEAN bRetreiveData : TRUE if data have to be retreive
//                                   FALSE if we don't want to retreive data (for fifo flushing)
//     out : PVOID pItemData : pointer to data. Pointed buffer must be 
//                             large enought to contain data 
//                             (size must be ItemSize see constructor)
//     return : TRUE on success, FALSE if pointed buffer space is not sufficent 
//               or if FIFO is Empty
//-----------------------------------------------------------------------------
BOOLEAN KernelFIFOPopEx(PKERNEL_FIFO pKernelFIFO,PVOID pItemData,BOOLEAN bRetreiveData)
{
    PKERNEL_FIFO_ITEM OldHead;

    if (!pKernelFIFO)
        return FALSE;

    // check dest pointer
    if (bRetreiveData)
        if (!pItemData)
            return FALSE;

    KeWaitForSingleObject(&pKernelFIFO->hevtListUnlocked,Executive,KernelMode,FALSE,NULL);

    if (pKernelFIFO->Head==NULL)
    {
        KeSetEvent(&pKernelFIFO->hevtListUnlocked,IO_NO_INCREMENT,FALSE);
        return FALSE;
    }
    if (bRetreiveData)
        // copy item data to pointer
        RtlCopyMemory(pItemData,pKernelFIFO->Head->ItemData,pKernelFIFO->ItemSize);

    // remove head
    if (pKernelFIFO->Head == pKernelFIFO->Tail) 
    {
        // free item data memory
        ExFreePoolWithTag(pKernelFIFO->Head->ItemData,KERNEL_FIFO_MEMORY_TAG);
        pKernelFIFO->Head->ItemData=NULL;
        // free item itself
        ExFreePoolWithTag(pKernelFIFO->Head,KERNEL_FIFO_MEMORY_TAG);

        pKernelFIFO->Head = NULL;// must be done after freeing item
    }
    else
    {
        OldHead=pKernelFIFO->Head;
        pKernelFIFO->Head = OldHead->NextItem;// must be done before freeing item
        
        // free item data memory
        ExFreePoolWithTag(OldHead->ItemData,KERNEL_FIFO_MEMORY_TAG);
        // free item itself
        ExFreePoolWithTag(OldHead,KERNEL_FIFO_MEMORY_TAG);
        OldHead=NULL;
    }

    pKernelFIFO->ItemsNumber--;

    KeSetEvent(&pKernelFIFO->hevtListUnlocked,IO_NO_INCREMENT,FALSE);
    return TRUE;
}
