// MemoryPool.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#pragma once

/***************************************
���⣺һ���ض������±䳤�ڴ�صĹ���

���ã��洢Ŀ¼��¼���ļ���¼

��Ҫ�ԣ�ϵͳ��ռ�ڴ�����ɴ�����

�ص㣺
1�� ͬʱ���������£���¼����ɾ��Ҫ������ʣ���
    FileIndex��DirIndex�Ѿ����û������
    ���ڴ�����뻥�⣬�Ա�����Ч��
2�� ��Ŀ¼��¼���ļ���¼��ʼ���ǣ��ڴ�ռ���������޿�϶��
    �ڴ��ά�������ھ����ʱ������ڴ�ı䣬
    Ϊ�˽�ʡ�ڴ棬�ļ����ӵ�ʵʱ��ʾ�ı��Կɽ���
3�� Free��Ҫ�û����������С�������ļ���¼��Ŀ¼��¼�����С�Ѿ��洢
***************************************/


class CMemoryPool
{
    static const DWORD BLOCK_SIZE=4096;
    struct Block
    {
        BYTE Data[BLOCK_SIZE];
    };
    typedef Block BLOCK,*PBLOCK;
public:
    static int GetBlockSize(){return BLOCK_SIZE;}
private:
    PBLOCK* m_ppBlock;
    DWORD   m_dwMaxCount;
    DWORD   m_dwBlockCount;
    PBLOCK  m_pLastBlock;
    DWORD   m_iLastPos;
protected:
    static const int INDEX_COUNT_DELT=32;
    void AddLastBlock();
public:
    CMemoryPool();
    virtual ~CMemoryPool();

    /**
     *	Function:
     *      ��ʼ��ʱʹ��
     *	Parameter(s):
     *
     *	Return:	
     *
     *	Commons:
     **/
    PVOID PushBack(DWORD dwRecordLen);

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
    PVOID Alloc(DWORD dwRecordLen);

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
    void  Free(PVOID pRecord,DWORD dwRecordLen);
public://DB���뵼��ʱ����

    DWORD DB_GetTotalSize()//��ȡ���ڴ�ר����
    {
        return BLOCK_SIZE*m_dwBlockCount;
    }
    //16λ��ʾ��ţ���16λ��ʾ����ƫ��
    DWORD DB_FromPtrToOffset(PVOID pRecord)
    {
        assert(pRecord && "��ַ����Ϊ0");
        DWORD i;
        for(i=0;i<m_dwBlockCount;++i)
        {
            if(pRecord>=m_ppBlock[i] && pRecord<PBYTE(m_ppBlock[i]+1)) break;
        }
        assert(i<m_dwBlockCount && "һ��Ӧ���ҵ����ָ��?");
        return (i<<16)|(PBYTE(pRecord)-PBYTE(m_ppBlock[i]));
    }

    PVOID DB_FromOffsetToPtr(DWORD dwOffset)
    {
        if(0xffffffff==dwOffset) return NULL;
        return PBYTE(m_ppBlock[dwOffset>>16])+(dwOffset&0xFFFF);
    }

    //���Ӽ�¼��ͬʱ��ü�¼��ƫ�ƺ͵�ַ
    void DB_PushBack(IN DWORD dwRecordLen,PVOID &pRecord,DWORD &dwOffset)
    {
        if(m_iLastPos+dwRecordLen>BLOCK_SIZE)
        {
            AddLastBlock();
        }
        pRecord=(PBYTE)m_pLastBlock+m_iLastPos;
        dwOffset=((m_dwBlockCount-1)<<16)|(m_iLastPos&0xFFFF);
        m_iLastPos+=dwRecordLen;
    }

};