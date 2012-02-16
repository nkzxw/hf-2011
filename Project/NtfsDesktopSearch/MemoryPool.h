// MemoryPool.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once

/***************************************
主题：一种特定条件下变长内存池的构建

作用：存储目录记录和文件记录

重要性：系统所占内存基本由此消耗

特点：
1， 同时多个卷监视下，记录的增删需要互斥访问，但
    FileIndex和DirIndex已经设置互斥操作
    本内存池无须互斥，以便提升效率
2， 在目录记录和文件记录初始化是，内存占用连续，无空隙，
    内存池维护发生在卷监视时引起的内存改变，
    为了节省内存，文件监视的实时显示改变性可降低
3， Free需要用户传入自身大小，由于文件记录和目录记录自身大小已经存储
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
     *      初始化时使用
     *	Parameter(s):
     *
     *	Return:	
     *
     *	Commons:
     **/
    PVOID PushBack(DWORD dwRecordLen);

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
    PVOID Alloc(DWORD dwRecordLen);

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
    void  Free(PVOID pRecord,DWORD dwRecordLen);
public://DB导入导出时调用

    DWORD DB_GetTotalSize()//获取总内存专用量
    {
        return BLOCK_SIZE*m_dwBlockCount;
    }
    //16位表示块号，低16位表示块内偏移
    DWORD DB_FromPtrToOffset(PVOID pRecord)
    {
        assert(pRecord && "地址不能为0");
        DWORD i;
        for(i=0;i<m_dwBlockCount;++i)
        {
            if(pRecord>=m_ppBlock[i] && pRecord<PBYTE(m_ppBlock[i]+1)) break;
        }
        assert(i<m_dwBlockCount && "一定应该找到这个指针?");
        return (i<<16)|(PBYTE(pRecord)-PBYTE(m_ppBlock[i]));
    }

    PVOID DB_FromOffsetToPtr(DWORD dwOffset)
    {
        if(0xffffffff==dwOffset) return NULL;
        return PBYTE(m_ppBlock[dwOffset>>16])+(dwOffset&0xFFFF);
    }

    //增加记录，同时获得记录的偏移和地址
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