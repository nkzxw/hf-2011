// OutVector.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "IndexNodeBlock.h" //使用了类型IndexElemType

//文件存储区指针，构造较简单，用单链表连接
const DWORD SIZE_INDEX_BLOCK=(1<<16);
const DWORD COUNT_PER_INDEX_BLOCK=(SIZE_INDEX_BLOCK>>2);


extern CMemoryMgr g_MemoryMgr;

//本类存储结果偏移值
//m_ppOutPtrBlock ->  [PDWORD1 PDWORD2 ... PDWORDn]
//求结果无须互斥
class COutVector
{
    static const DWORD INDEX_COUNT_DELT=32; //用于扩展索引数组空间，当结点较小时，它应较大
public:
    static const DWORD MaxNumOfElemPerBlock=COUNT_PER_INDEX_BLOCK;//每个节点的最大元素个数
public:
    COutVector(int _N=16)
    {
        m_dwMaxCount=_N;
        m_ppOutPtrBlock=(PIndexElemType*)g_MemoryMgr.malloc(m_dwMaxCount*sizeof(PIndexElemType));//简化realloc

        m_size=0;
        m_ppNextCurPos=m_ppOutPtrBlock[0]=(PIndexElemType)g_MemoryMgr.GetMemory(SIZE_INDEX_BLOCK);
        m_ppCurEnd=m_ppNextCurPos+COUNT_PER_INDEX_BLOCK;
        m_iCurBlock=0;//当前第0块
        m_dwBlockCount=1;//已分配1块空间
    }
    virtual ~COutVector()
    {
        for(int i=0;i<m_dwBlockCount;++i){
            g_MemoryMgr.FreeMemory((PBYTE)m_ppOutPtrBlock[i]);
        }
        g_MemoryMgr.free(m_ppOutPtrBlock);
    }
    void COutVector::push_back(IndexElemType RecordPtr){
        ++m_size;
        *m_ppNextCurPos++=RecordPtr;

        if(m_ppNextCurPos==m_ppCurEnd){
            ++m_iCurBlock;
            if(m_iCurBlock<m_dwBlockCount){

            }else{
                m_ppOutPtrBlock[m_dwBlockCount++]=(PIndexElemType)g_MemoryMgr.GetMemory(SIZE_INDEX_BLOCK);
                if(m_dwBlockCount==m_dwMaxCount){
                    m_dwMaxCount+=4;
                    m_ppOutPtrBlock=(PIndexElemType*)g_MemoryMgr.realloc(m_ppOutPtrBlock,m_dwMaxCount*sizeof(PIndexElemType));                    
                }
            }
            m_ppNextCurPos=m_ppOutPtrBlock[m_iCurBlock];
            m_ppCurEnd=m_ppNextCurPos+COUNT_PER_INDEX_BLOCK;
        }
    }
    DWORD size()const
    {
        return m_size;
    }
    void clear(){
        m_size=0;
        m_iCurBlock=0;
        m_ppNextCurPos=m_ppOutPtrBlock[m_iCurBlock];//当前要插入的块的第一个元素
        m_ppCurEnd=m_ppNextCurPos+COUNT_PER_INDEX_BLOCK;//当前要插入的块的最后一个元素   
    }
    void clear_all(){//完全清除内存
        clear();
        for(int i=1;i<m_dwBlockCount;++i)
        {
            g_MemoryMgr.FreeMemory((PBYTE)m_ppOutPtrBlock[i]);
        }
        m_dwBlockCount=1;     
    }
    IndexElemType GetAt(int i)const{
        int k=i/COUNT_PER_INDEX_BLOCK;
        return m_ppOutPtrBlock[k][i-k*COUNT_PER_INDEX_BLOCK];
    }
    IndexElemType operator[](int i)const{
        int k=i/COUNT_PER_INDEX_BLOCK;
        return m_ppOutPtrBlock[k][i-k*COUNT_PER_INDEX_BLOCK];
    }

    PIndexElemType  *GetHeadPtr()
    {
        return m_ppOutPtrBlock;
    }
private:
    PIndexElemType  *m_ppOutPtrBlock;
    DWORD   m_dwMaxCount;//索引指针数组分配的空间

    DWORD   m_iCurBlock;//当前考查的块号
    DWORD   m_dwBlockCount;//已分配了多少个块

    PIndexElemType  m_ppNextCurPos;
    PIndexElemType  m_ppCurEnd;
    DWORD   m_size;
};
