#pragma once

#include <Windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <Tlhelp32.h>
#include "HeapWalk.h"

#define HEAP_WALKER_MEMORY_FLAG_ALL         _T("All")
#define HEAP_WALKER_MEMORY_FLAG_FIXED       _T("Fixed")
#define HEAP_WALKER_MEMORY_FLAG_MOVABLE     _T("Movable")
#define HEAP_WALKER_MEMORY_FLAG_FREE        _T("Free")

enum ColumnsIndex
{
    ColumnIndexAddress,
    ColumnIndexBlockSize,
    ColumnIndexFlags,
    ColumnIndexLockCount,
    ColumnIndexData,
    ColumnIndexAsciiData
};


typedef struct tagHeapContent 
{
    HEAPENTRY HeapEntry;
    PBYTE     pData;
}HEAP_CONTENT,*PHEAP_CONTENT;