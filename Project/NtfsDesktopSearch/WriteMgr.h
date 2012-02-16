// WriteMgr.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once

//管理写入数据库
#define WIRTE_SIZE 4096
typedef struct WriteNode
{
    static const DWORD _NodeMaxSize=WIRTE_SIZE;
    DWORD _dwBytesLeft;//当前节点写入的字节数 保存变量快速写入
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


//不使用BZIP压缩
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

    //不会出现dwSize写多块>1情形
    VOID Write(PVOID pAddr,DWORD dwSize)
    {
        m_dwTotalWrite+=dwSize;

        int dwSecondCount=dwSize-m_ppHead[m_iCurNode]->_dwBytesLeft;
        PBYTE pSrc=(PBYTE)pAddr,pSrcEnd=pSrc+dwSize;
        PBYTE pDest;
        if(dwSecondCount>0)
        {//写入跨块
            //先写入m_ppHead[m_iCurNode]->_dwBytesLeft长到第一块
            pDest=m_ppHead[m_iCurNode]->_buffer+(WRITE_NODE::_NodeMaxSize-m_ppHead[m_iCurNode]->_dwBytesLeft);
            PBYTE pSrcTempEnd=pSrc+m_ppHead[m_iCurNode]->_dwBytesLeft;
            for(;pSrc<pSrcTempEnd;) *pDest++=*pSrc++;
            m_ppHead[m_iCurNode]->_dwBytesLeft=0;
            dwSize=dwSecondCount;

            //增加新节点
            ++m_iCurNode;
            if(m_iCurNode==m_dwMaxCount)//当前块已用完
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

    //重写第一块的头几个DWORD值
    //注意，本处没有对节点的m_ppHead[m_iCurNode]->_dwBytesLeft进行操作
    //本函数必须在写入头部之后再调用
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

    DWORD m_iCurNode;//最后一个有效节点
    DWORD m_dwTotalWrite;
};