// MemoryPool.cpp
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "MemoryPool.h"

CMemoryPool g_MemFile,g_MemDir;//文件和目录的数据都保存在此处

void CMemoryPool::AddLastBlock()
{
    m_pLastBlock=m_ppBlock[m_dwBlockCount]=(PBLOCK)g_MemoryMgr.GetMemory(sizeof(BLOCK),TRUE);
    m_iLastPos=0;
    ++m_dwBlockCount;
    if(m_dwBlockCount==m_dwMaxCount){//索引BUF已满，增加之
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
*      初始化时使用
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
*      监视维护时分配内存
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
*       监视维护时释放内存
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





