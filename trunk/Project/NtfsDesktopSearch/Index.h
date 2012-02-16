// Index.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
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

    //按文件名顺序插入
    void insert(IndexElemType Record,BOOL bDir)
    ;

    //删除的时候由于会出现重名现象，必须提供文件BasicInfo
    //当节点为空时，释放空节点块，这样表中没有空节点块
    void erase(IndexElemType Record,BOOL bDir)
    ;

    //用于顺序遍历，让用户管理提高速度
    PINDEX_BLOCK_NODE *GetBlockIndex()const;
    int GetBlockCount()const;

    //仅用于初始化
    //用于构建目录数据库，仅初始化时父索引对象调用
    //注意，此时库中无重复的BasicInfo，和字符串的搜索方式不同
    //IndexElemType FindFileIdentify(DWORD BasicInfo);
    
protected:
    void AddLastBlock();
    
    void InsertBlock(int i);//在第i处插入新块
    
    void EraseBlock(int i);//考虑到空节点影响访问效率 删除第i块空节点

private:
    PINDEX_BLOCK_NODE *m_pIndex;
    DWORD   m_dwMaxCount;//索引指针数组分配的空间

    PINDEX_BLOCK_NODE m_pLastNode;//DWORD   m_iCurBlock;//当前考查的块号 便于初始化接连插入
    DWORD   m_dwBlockCount;//已分配了多少个块（已分配的块总是位于m_pIndex的前m_dwBlockCount项）

    DWORD   m_size;//总元素个数

    CRITICAL_SECTION m_cs;
};

