// MemoryPool.cpp
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "MemoryPool.h"

CMemoryPool g_MemFile,g_MemDir;//�ļ���Ŀ¼�����ݶ������ڴ˴�

void CMemoryPool::AddLastBlock()
{
    m_pLastBlock=m_ppBlock[m_dwBlockCount]=(PBLOCK)g_MemoryMgr.GetMemory(sizeof(BLOCK),TRUE);
    m_iLastPos=0;
    ++m_dwBlockCount;
    if(m_dwBlockCount==m_dwMaxCount){//����BUF����������֮
        m_dwMaxCount+=INDEX_COUNT_DELT;
        m_ppBlock=(PBLOCK*)g_MemoryMgr.realloc(m_ppBlock,m_dwMaxCount*sizeof(PBLOCK));                
    }
}

CMemoryPool::CMemoryPool()
{
    m_dwMaxCount=128;
    m_ppBlock=(PBLOCK*)g_MemoryMgr.malloc(m_dwMaxCount*sizeof(PBLOCK));
    *m_ppBlock=m_pLastBlock=(PBLOCK)g_MemoryMgr.GetMemory(sizeof(BLOCK),TRUE);
    m_dwBlockCount=1;
    m_iLastPos=0;
}
/*virtual */CMemoryPool::~CMemoryPool()
{
    for(int i=0;i<m_dwBlockCount;++i){
        g_MemoryMgr.FreeMemory((PBYTE)m_ppBlock[i]);
    }
    g_MemoryMgr.free(m_ppBlock);  
}

/**
*	Function:
*      ��ʼ��ʱʹ��
*	Parameter(s):
*
*	Return:	
*
*	Commons:
**/
PVOID CMemoryPool::PushBack(DWORD dwRecordLen)
{
    if(m_iLastPos+dwRecordLen>BLOCK_SIZE)
    {
        AddLastBlock();
    }
    PBYTE pAlloc=(PBYTE)m_pLastBlock+m_iLastPos;
    m_iLastPos+=dwRecordLen;
    return pAlloc;
}

/**
*	Function:
*      ����ά��ʱ�����ڴ�
*	Parameter(s):
*
*	Return:	
*
*	Commons:
*
**/
PVOID CMemoryPool::Alloc(DWORD dwRecordLen)
{
    return PushBack(dwRecordLen);
}

/**
*	Function:
*       ����ά��ʱ�ͷ��ڴ�
*	Parameter(s):
*
*	Return:	
*
*	Commons:
*
**/
void  CMemoryPool::Free(PVOID pRecord,DWORD dwRecordLen)
{

}





