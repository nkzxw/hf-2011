// MemoryMgr.cpp
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "MemoryMgr.h"


CMemoryMgr::CMemoryMgr(){
    if(NULL==(hHeap=::HeapCreate(0,0,0))){
        DebugStringA("创建堆句柄失败:%d",GetLastError());
    }
}
CMemoryMgr::~CMemoryMgr(){
    HeapDestroy(hHeap);
}
PBYTE CMemoryMgr::GetMemory(DWORD _byte,BOOL bIniZero)
{
    PVOID pRet;
    if(bIniZero)
        pRet=HeapAlloc(hHeap, HEAP_ZERO_MEMORY, _byte);
    else
        pRet=HeapAlloc(hHeap, 0, _byte);
    if(NULL==pRet){
        DebugStringA("申请内存失败:%d",GetLastError());
    }
    return (PBYTE)pRet;
}
VOID CMemoryMgr::FreeMemory(PBYTE ptr)
{
    HeapFree(hHeap, 0, ptr);
}

//用于realloc
PVOID CMemoryMgr::malloc(DWORD dwSize)
{
    return HeapAlloc(hHeap,0,dwSize);
}

//如果dwSize比原来的更小，则只复制原来前面的那部分
PVOID CMemoryMgr::realloc(PVOID pSrc,DWORD dwSize)
{
    DWORD dwSrcSize=HeapSize(hHeap,0,pSrc);
    PVOID pDest=HeapAlloc(hHeap,0,dwSize);
    if(dwSrcSize>dwSize) dwSrcSize=dwSize;
    CopyMemory(pDest,pSrc,dwSrcSize);
    HeapFree(hHeap,0,pSrc);
    return pDest;
}
void CMemoryMgr::free(PVOID pSrc)
{
    HeapFree(hHeap,0,pSrc);
}

