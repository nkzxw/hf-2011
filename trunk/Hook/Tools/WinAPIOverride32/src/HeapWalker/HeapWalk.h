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

#pragma once

#include <Windows.h>
#include <Tlhelp32.h>

typedef HEAPLIST32 HEAPLIST;
typedef HEAPENTRY32 HEAPENTRY;
#define CreateToolhelpSnapshot CreateToolhelp32Snapshot
#define HeapListFirst Heap32ListFirst
#define HeapListNext Heap32ListNext
#define HeapNext Heap32Next
#define HeapFirst Heap32First
#define THCS_SNAPHEAPLIST TH32CS_SNAPHEAPLIST

typedef BOOL (*tagCallBackHeapList)(HEAPLIST* pHeapList,PVOID pUserParam);// return FALSE to stop parsing
typedef BOOL (*tagCallBackHeapEntry)(HEAPENTRY* pHeapEntry,PVOID pUserParam);// return FALSE to stop parsing

class CHeapWalk
{
private:

public:
    CHeapWalk(void);
    ~CHeapWalk(void);
    BOOL static WalkHeapList(DWORD ProcessId,tagCallBackHeapList CallBackHeapList,PVOID pUserParam);
    BOOL static WalkHeapEntry(HEAPLIST* pHeapList,HANDLE hEvtCancel,tagCallBackHeapEntry CallBackHeapEntry,PVOID pUserParam);
};
