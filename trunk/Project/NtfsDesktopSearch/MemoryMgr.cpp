// MemoryMgr.cpp
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "MemoryMgr.h"


CMemoryMgr::CMemoryMgr(){
    if(NULL==(hHeap=::HeapCreate(0,0,0))){
        DebugStringA("�����Ѿ��ʧ��:%d",GetLastError());
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
        DebugStringA("�����ڴ�ʧ��:%d",GetLastError());
    }
    return (PBYTE)pRet;
}
VOID CMemoryMgr::FreeMemory(PBYTE ptr)
{
    HeapFree(hHeap, 0, ptr);
}

//����realloc
PVOID CMemoryMgr::malloc(DWORD dwSize)
{
    return HeapAlloc(hHeap,0,dwSize);
}

//���dwSize��ԭ���ĸ�С����ֻ����ԭ��ǰ����ǲ���
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

