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

#include "heapwalk.h"

CHeapWalk::CHeapWalk(void)
{

}

CHeapWalk::~CHeapWalk(void)
{

}


BOOL CHeapWalk::WalkHeapList(DWORD ProcessId,tagCallBackHeapList CallBackHeapList,PVOID pUserParam)
{
    HEAPLIST HeapList = {0};
    // Fill the size of the structure before using it. 
    HeapList.dwSize = sizeof(HEAPLIST); 

    if (IsBadCodePtr((FARPROC)CallBackHeapList))
        return FALSE;


    HANDLE hSnap =CreateToolhelpSnapshot(THCS_SNAPHEAPLIST,ProcessId);
    if (hSnap == INVALID_HANDLE_VALUE) 
        return FALSE; 

    // Walk the heap list of the process
    if (!HeapListFirst(hSnap, &HeapList))
    {
        CloseHandle(hSnap);
        return FALSE;
    }
    do 
    {
        // only call callback
        if (!CallBackHeapList(&HeapList,pUserParam))
            break;
    } 
    while (HeapListNext(hSnap, &HeapList)); 

    // clean up the snapshot object. 
    CloseHandle (hSnap); 

    return TRUE;
}

BOOL CHeapWalk::WalkHeapEntry(HEAPLIST* pHeapList,HANDLE hEvtCancel,tagCallBackHeapEntry CallBackHeapEntry,PVOID pUserParam)
{
    HEAPENTRY HeapEntry={0};
    HeapEntry.dwSize= sizeof(HEAPENTRY);

    if (IsBadCodePtr((FARPROC)CallBackHeapEntry))
        return FALSE;

    // walk heap entry of the current HeapList
    if (!HeapFirst(&HeapEntry,pHeapList->th32ProcessID,pHeapList->th32HeapID))
        return FALSE;
    do 
    {
        if (hEvtCancel)
        {
            // if hEvtCancel is signaled
            if (WaitForSingleObject(hEvtCancel,0)==WAIT_OBJECT_0)
                return FALSE;
        }

        if (!CallBackHeapEntry(&HeapEntry,pUserParam))
            return FALSE;
    }
    while(HeapNext(&HeapEntry));

    return TRUE;
}