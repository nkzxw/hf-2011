// WriteMgr.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#pragma once

//����д�����ݿ�
#define WIRTE_SIZE 4096
typedef struct WriteNode
{
    static const DWORD _NodeMaxSize=WIRTE_SIZE;
    DWORD _dwBytesLeft;//��ǰ�ڵ�д����ֽ��� �����������д��
    BYTE  _buffer[_NodeMaxSize];
    VOID Init(){_dwBytesLeft=_NodeMaxSize;}
}WRITE_NODE,*PWRITE_NODE;

__forceinline void MyCopyMemory(PVOID pVoidDest,PVOID pVoidSrc,DWORD dwSize)
{
    DWORD count=(dwSize>>3);
    DWORD sizeLeft=dwSize-(count<<3);

    DWORDLONG *pDest=(DWORDLONG*)pVoidDest;
    DWORDLONG *pSrc=(DWORDLONG*)pVoidSrc,
        *pSrcEnd=pSrc+count;

    for(;pSrc<pSrcEnd;) *pDest++=*pSrc++;

    PBYTE pByteDest=(PBYTE)pDest;
    PBYTE pByteSrc=(PBYTE)pSrc,
        pByteSrcEnd=pByteSrc+sizeLeft;

    for(;pByteSrc<pByteSrcEnd;) *pByteDest++=*pByteSrc++;
}


// #define MyCopyMemory(pVoidDest,pVoidSrc,dwSize)\
// {\
//     DWORD count=((dwSize)>>3);\
//     DWORD sizeLeft=(dwSize)-(count<<3);\
// 
//     DWORDLONG *pDest=(DWORDLONG*)(pVoidDest);\
//     DWORDLONG *pSrc=(DWORDLONG*)(pVoidSrc),\
//     *pSrcEnd=pSrc+count;\
// 
//     for(;pSrc<pSrcEnd;) {*pDest++=*pSrc++;}\
// 
//     PBYTE pByteDest=(PBYTE)pDest;\
//     PBYTE pByteSrc=(PBYTE)pSrc,\
//     pByteSrcEnd=pByteSrc+sizeLeft;\
// 
//     for(;pByteSrc<pByteSrcEnd;) {*pByteDest++=*pByteSrc++;}\
// }


//��ʹ��BZIPѹ��
class CWriteMgr
{
public:
    CWriteMgr()
    {
        m_dwMaxCount=512;
        m_ppHead=(PWRITE_NODE*)g_MemoryMgr.malloc(m_dwMaxCount*sizeof(PWRITE_NODE));
        m_iCurNode=0;
        m_ppHead[m_iCurNode]=(PWRITE_NODE)g_MemoryMgr.GetMemory(sizeof(WRITE_NODE));
        m_ppHead[m_iCurNode]->Init(); 

        m_dwTotalWrite=0;
    }
    virtual ~CWriteMgr()
    {
        for(int i=0;i<=m_iCurNode;++i)
        {
            g_MemoryMgr.FreeMemory(PBYTE(m_ppHead[i]));
        }
        g_MemoryMgr.free(m_ppHead);
    }
public:  
    DWORD GetTotalWirte()const{
        return m_dwTotalWrite;
    }

    //�������dwSizeд���>1����
    VOID Write(PVOID pAddr,DWORD dwSize)
    {
        m_dwTotalWrite+=dwSize;

        int dwSecondCount=dwSize-m_ppHead[m_iCurNode]->_dwBytesLeft;
        PBYTE pSrc=(PBYTE)pAddr,pSrcEnd=pSrc+dwSize;
        PBYTE pDest;
        if(dwSecondCount>0)
        {//д����
            //��д��m_ppHead[m_iCurNode]->_dwBytesLeft������һ��
            pDest=m_ppHead[m_iCurNode]->_buffer+(WRITE_NODE::_NodeMaxSize-m_ppHead[m_iCurNode]->_dwBytesLeft);
            PBYTE pSrcTempEnd=pSrc+m_ppHead[m_iCurNode]->_dwBytesLeft;
            for(;pSrc<pSrcTempEnd;) *pDest++=*pSrc++;
            m_ppHead[m_iCurNode]->_dwBytesLeft=0;
            dwSize=dwSecondCount;

            //�����½ڵ�
            ++m_iCurNode;
            if(m_iCurNode==m_dwMaxCount)//��ǰ��������
            {
                m_ppHead=(PWRITE_NODE*)g_MemoryMgr.realloc(m_ppHead,(m_dwMaxCount+=32)*sizeof(PWRITE_NODE));
            }
            m_ppHead[m_iCurNode]=(PWRITE_NODE)g_MemoryMgr.GetMemory(sizeof(WRITE_NODE));
            m_ppHead[m_iCurNode]->Init();
        }
        pDest=m_ppHead[m_iCurNode]->_buffer+(WRITE_NODE::_NodeMaxSize-m_ppHead[m_iCurNode]->_dwBytesLeft);
        for(;pSrc<pSrcEnd;) *pDest++=*pSrc++;
        m_ppHead[m_iCurNode]->_dwBytesLeft-=dwSize;
    }

    //��д��һ���ͷ����DWORDֵ
    //ע�⣬����û�жԽڵ��m_ppHead[m_iCurNode]->_dwBytesLeft���в���
    //������������д��ͷ��֮���ٵ���
    void ReWriteFirstBlock(DWORD iDword,DWORD dwVal)
    {
        DWORD *pDest=(DWORD*)(m_ppHead[0]->_buffer+(iDword<<2));
        *pDest=dwVal;
    }

    BOOL WriteOver(PWCHAR pFileName)
    {
        HANDLE hFile=CreateFileW(pFileName
            ,GENERIC_READ|GENERIC_WRITE
            ,FILE_SHARE_READ|FILE_SHARE_WRITE
            ,NULL
            ,CREATE_ALWAYS
            ,FILE_ATTRIBUTE_NORMAL
            ,NULL);
        if(INVALID_HANDLE_VALUE==hFile){
            return FALSE;
        }
        
        DWORD dwWrite;
        for(int i=0;i<=m_iCurNode;++i)
        {
            if(!WriteFile(hFile,m_ppHead[i]->_buffer,WRITE_NODE::_NodeMaxSize-m_ppHead[i]->_dwBytesLeft,&dwWrite,NULL))
            {
                CloseHandle(hFile);
                return FALSE;
            }
        }

        CloseHandle(hFile);   
        return TRUE;
    }
private:
    PWRITE_NODE* m_ppHead;
    DWORD m_dwMaxCount;

    DWORD m_iCurNode;//���һ����Ч�ڵ�
    DWORD m_dwTotalWrite;
};