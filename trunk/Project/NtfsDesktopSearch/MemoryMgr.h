// MemoryMgr.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
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

