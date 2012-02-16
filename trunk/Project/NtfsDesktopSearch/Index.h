// Index.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "IndexNodeBlock.h"

typedef IndexBlockNode<> INDEX_BLOCK_NODE,*PINDEX_BLOCK_NODE;

class CIndex
{  
public:
    CIndex(int _N=16);
    ~CIndex();

    void Lock();
    void UnLock();

    void  DecreaseSize();
    DWORD size()const;
    void push_back(IndexElemType Record);

    //���ļ���˳�����
    void insert(IndexElemType Record,BOOL bDir)
    ;

    //ɾ����ʱ�����ڻ�����������󣬱����ṩ�ļ�BasicInfo
    //���ڵ�Ϊ��ʱ���ͷſսڵ�飬��������û�пսڵ��
    void erase(IndexElemType Record,BOOL bDir)
    ;

    //����˳����������û���������ٶ�
    PINDEX_BLOCK_NODE *GetBlockIndex()const;
    int GetBlockCount()const;

    //�����ڳ�ʼ��
    //���ڹ���Ŀ¼���ݿ⣬����ʼ��ʱ�������������
    //ע�⣬��ʱ�������ظ���BasicInfo�����ַ�����������ʽ��ͬ
    //IndexElemType FindFileIdentify(DWORD BasicInfo);
    
protected:
    void AddLastBlock();
    
    void InsertBlock(int i);//�ڵ�i�������¿�
    
    void EraseBlock(int i);//���ǵ��սڵ�Ӱ�����Ч�� ɾ����i��սڵ�

private:
    PINDEX_BLOCK_NODE *m_pIndex;
    DWORD   m_dwMaxCount;//����ָ���������Ŀռ�

    PINDEX_BLOCK_NODE m_pLastNode;//DWORD   m_iCurBlock;//��ǰ����Ŀ�� ���ڳ�ʼ����������
    DWORD   m_dwBlockCount;//�ѷ����˶��ٸ��飨�ѷ���Ŀ�����λ��m_pIndex��ǰm_dwBlockCount�

    DWORD   m_size;//��Ԫ�ظ���

    CRITICAL_SECTION m_cs;
};

