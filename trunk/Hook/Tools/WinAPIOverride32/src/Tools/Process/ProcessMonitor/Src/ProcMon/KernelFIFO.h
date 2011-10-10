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

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "DriversCommon.h" // place it before <ntddk.h> to avoid warnings
#include <ntddk.h>

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define KERNEL_FIFO_MEMORY_TAG 'ofiF'

typedef struct tagKERNEL_FIFO_ITEM
{
    PVOID ItemData;
    struct tagKERNEL_FIFO_ITEM* NextItem;
}KERNEL_FIFO_ITEM,*PKERNEL_FIFO_ITEM;

typedef struct tagKERNEL_FIFO
{
    KEVENT hevtListUnlocked;
    unsigned long ItemSize;
    unsigned long  ItemsNumber;
    PKERNEL_FIFO_ITEM Head;
    PKERNEL_FIFO_ITEM Tail;
}KERNEL_FIFO,*PKERNEL_FIFO;

//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------

PKERNEL_FIFO KernelFIFONew(unsigned long ItemSize);
void KernelFIFODelete(PKERNEL_FIFO pKernelFIFO);

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add Item to the fifo allocating necessary memory for ItemData
// Parameters :
//     in  : 
//     out :
//     return : PKERNEL_FIFO_ITEM Item : pointer to the added item
//-----------------------------------------------------------------------------
PKERNEL_FIFO_ITEM KernelFIFOAddItem(PKERNEL_FIFO pKernelFIFO);

//-----------------------------------------------------------------------------
// Name: AddItemEx
// Object: Add Item to the fifo allocating necessary memory for ItemData
// Parameters :
//     in  : 
//     out :
//     return : PKERNEL_FIFO_ITEM Item : pointer to the added item
//-----------------------------------------------------------------------------
PKERNEL_FIFO_ITEM KernelFIFOAddItemEx(PKERNEL_FIFO pKernelFIFO,PVOID ItemData);

//-----------------------------------------------------------------------------
// Name: Flush
// Object: Remove all items from the fifo
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void KernelFIFOFlush(PKERNEL_FIFO pKernelFIFO);

//-----------------------------------------------------------------------------
// Name: Push
// Object: push data to FIFO. As a local copy of data is done for fifo, pData can
//         be delete after push call
// Parameters :
//     in  : pData
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOLEAN KernelFIFOPush(PKERNEL_FIFO pKernelFIFO,PVOID pData);

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
BOOLEAN KernelFIFOPop(PKERNEL_FIFO pKernelFIFO,PVOID pItemData);

//-----------------------------------------------------------------------------
// Name: PopEx
// Object: pop data from FIFO
// Parameters :
//     in  : BOOLEAN bRetreiveData : TRUE if data have to be retreive
//                                   FALSE if we don't want to retreive data (for fifo flushing)
//     out : PVOID pItemData : pointer to data. Pointed buffer must be 
//                             large enought to contain data 
//                             (size must be ItemSize see constructor)
//     return : TRUE on success, FALSE if pointed buffer space is not sufficent 
//               or if FIFO is Empty
//-----------------------------------------------------------------------------
BOOLEAN KernelFIFOPopEx(PKERNEL_FIFO pKernelFIFO,PVOID pItemData,BOOLEAN bRetreiveData);
