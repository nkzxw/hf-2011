// DirBasicInfoMap.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong0115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "global.h"
//用于存储目录Basic信息
//map<BasicInfo,DirPtr> 不需要获得第i个元素 无须累积计数
//当不存在BasicInfo对应的目录时 DirPtr=NULL
//DWORDLONG:  BasicInfo,DirPtr 低DWORD存BasicInfo
//存储结构按低4字节有序
//增加目录时需要增加
//设计此类用意在于降低std::map的内存开销

typedef IndexBlockNode<DWORDLONG,1024> DirFrnIndexBlockNode,*PDirFrnIndexBlockNode;

class CDirBasicInfoMap
{   
public:
    CDirBasicInfoMap(int _N=32);
    ~CDirBasicInfoMap();

    //仅在初始化时使用，以遍提高效率
    void push_back(DWORD BasicInfo,IndexElemType DirPtr);
    //可以不提供删除节点功能
    //没有删除的信息也不会需要被查找
    //若新增的利用的先前的FRN，则
    //因为插入是如果遇到重复值遍对DirOffset进行了修改   
    IndexElemType erase(DWORD BasicInfo);//删除节点
    
    void insert(DWORD BasicInfo,IndexElemType DirPtr);

    DWORDLONG* find(DWORD BasicInfo);

    //用于顺序遍历，让用户管理提高速度
    PDirFrnIndexBlockNode *GetBlockIndex()const
    {
        return m_pIndex;
    }
    int GetBlockCount()const
    {
        return m_dwBlockCount;
    }

protected:
    void AddLastBlock();
    void InsertBlock(int i);//在第i处插入新块

private:
    PDirFrnIndexBlockNode *m_pIndex;
    DWORD   m_dwMaxCount;//索引指针数组分配的空间

    PDirFrnIndexBlockNode m_pLastNode;//DWORD   m_iCurBlock;//最后一个节点块
    DWORD   m_dwBlockCount;//已分配了多少个块（已分配的块总是位于m_pIndex的前m_dwBlockCount项）

    DWORD m_size;

};