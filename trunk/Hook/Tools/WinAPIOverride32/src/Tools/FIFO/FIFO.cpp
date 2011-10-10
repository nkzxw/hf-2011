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
// Object: provides a FIFO
//-----------------------------------------------------------------------------


#include "fifo.h"

CFIFO::CFIFO(size_t ItemSize):CLinkList(ItemSize)
{

}
CFIFO::~CFIFO(void)
{

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
BOOL CFIFO::Push(PVOID pData)
{
    CLinkListItem* pLinkListItem=this->AddItem(pData);
    if (pLinkListItem==NULL)
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
BOOL CFIFO::Pop(PVOID pItemData)
{
    CLinkListItem* OldHead;

    // check dest pointer
    if (IsBadWritePtr(pItemData,this->ItemSize))
        return FALSE;

    WaitForSingleObject(this->hevtListUnlocked,CLINKLIST_MAX_WAIT_IN_MS);

    if (this->Head==NULL)
    {
        SetEvent(this->hevtListUnlocked);
        return FALSE;
    }
    // copy item data to pointer
    memcpy(pItemData,this->Head->ItemData,this->ItemSize);

    // remove head
    if (this->Head == this->Tail) 
    {
        // free item data memory
        HeapFree(GetProcessHeap(), 0, this->Head->ItemData);
        this->Head->ItemData=NULL;
        // free item itself
        delete this->Head;

        this->Head = NULL;// must be done after freeing item
    }
    else
    {
        OldHead=this->Head;
        this->Head = OldHead->NextItem;// must be done before freeing item
        
        // free item data memory
        HeapFree(GetProcessHeap(), 0, OldHead->ItemData);
        OldHead->ItemData=NULL;
        OldHead->NextItem=NULL;
        // free item itself
        delete OldHead;
        OldHead=NULL;
    }

    this->ItemsNumber--;
    SetEvent(this->hevtListUnlocked);
    return TRUE;
}