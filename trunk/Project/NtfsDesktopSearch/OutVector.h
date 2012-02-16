// OutVector.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "IndexNodeBlock.h" //ʹ��������IndexElemType

//�ļ��洢��ָ�룬����ϼ򵥣��õ���������
const DWORD SIZE_INDEX_BLOCK=(1<<16);
const DWORD COUNT_PER_INDEX_BLOCK=(SIZE_INDEX_BLOCK>>2);


extern CMemoryMgr g_MemoryMgr;

//����洢���ƫ��ֵ
//m_ppOutPtrBlock ->  [PDWORD1 PDWORD2 ... PDWORDn]
//�������뻥��
class COutVector
{
    static const DWORD INDEX_COUNT_DELT=32; //������չ��������ռ䣬������Сʱ����Ӧ�ϴ�
public:
    static const DWORD MaxNumOfElemPerBlock=COUNT_PER_INDEX_BLOCK;//ÿ���ڵ�����Ԫ�ظ���
public:
    COutVector(int _N=16)
    {
        m_dwMaxCount=_N;
        m_ppOutPtrBlock=(PIndexElemType*)g_MemoryMgr.malloc(m_dwMaxCount*sizeof(PIndexElemType));//��realloc

        m_size=0;
        m_ppNextCurPos=m_ppOutPtrBlock[0]=(PIndexElemType)g_MemoryMgr.GetMemory(SIZE_INDEX_BLOCK);
        m_ppCurEnd=m_ppNextCurPos+COUNT_PER_INDEX_BLOCK;
        m_iCurBlock=0;//��ǰ��0��
        m_dwBlockCount=1;//�ѷ���1��ռ�
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
        m_ppNextCurPos=m_ppOutPtrBlock[m_iCurBlock];//��ǰҪ����Ŀ�ĵ�һ��Ԫ��
        m_ppCurEnd=m_ppNextCurPos+COUNT_PER_INDEX_BLOCK;//��ǰҪ����Ŀ�����һ��Ԫ��   
    }
    void clear_all(){//��ȫ����ڴ�
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
    DWORD   m_dwMaxCount;//����ָ���������Ŀռ�

    DWORD   m_iCurBlock;//��ǰ����Ŀ��
    DWORD   m_dwBlockCount;//�ѷ����˶��ٸ���

    PIndexElemType  m_ppNextCurPos;
    PIndexElemType  m_ppCurEnd;
    DWORD   m_size;
};
