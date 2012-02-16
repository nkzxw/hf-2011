// MemoryMgr.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "global.h"

class CMemoryMgr
{
private://static
    HANDLE hHeap;
public:
    CMemoryMgr();
    ~CMemoryMgr();
    PBYTE GetMemory(DWORD _byte,BOOL bIniZero=FALSE);
    VOID FreeMemory(PBYTE ptr);

    PVOID malloc(DWORD dwSize);
    PVOID realloc(PVOID pSrc,DWORD dwSize);
    void free(PVOID pSrc);
};

