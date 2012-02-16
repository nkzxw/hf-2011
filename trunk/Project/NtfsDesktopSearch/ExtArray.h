// ExtArray.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong0115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once


//扩展名数据库，由于扩展名数量较少故该库仅增不删
//保存扩展名按字典序建立索引

#define ICON_INDEX_UNINITIALIZED    -3
#define ICON_INDEX_REALTIMECALL     -2
#define ICON_INDEX_NOICON           -1

class CExtArray
{

    static const DWORD DELT=64;
public:
    static DWORD s_dwOmitExt;  //必须实时调用的图标数量 
    CExtArray();
    ~CExtArray();
    DWORD size(){return m_size;}

    void InitRealTimeCallExt();
    
    //返回索引中的顺序
    //返回-1表示没有找到
    int  find(PWCHAR pszExtName)const;

    //返回索引中的顺序
    //返回-1表示没有找到
    int find(PWCHAR pszExtName,int ExtNameLen)const; 

    //遍历文件库时需要插入扩展名
    //并返回该扩展名所对应的ID
    int insert(PWCHAR pszExtName,int ExtNameLen); 

    int GetExtIndexSequence(int idExtName)const;

    int Compare(int idExt1,int idExt2)const;//比较两个扩展名大小

    PWCHAR GetExtName(int idExtName)const;
    //获得几号扩展名的ICON索引
    //若返回>-2 可直接使用
    //==-2 需要用户调用SHGetFileInfo
    //==-3 表明还没有初始化该id
    //用户需要调用SetIconIndex来初始化扩展名索引
    int GetIconIndex(int idExtName)const;

    //设置扩展名索引
    int SetIconIndex(int idExtName,PWCHAR pFilePath,int filePathLen,PWCHAR pFileName,int fileLen);

    //扩展名数据库操作，优先加载
    //若返回FALSE 则文件名数据库中的扩展名部分无效
    //这时需要重新生成文件名数据库
    BOOL LoadFromFile(PWCHAR pExtFileName);

    //在写文件名数据库之后写到扩展名数据库
    BOOL WriteToFile(PWCHAR pExtFileName);
    
    
private:
    BOOL  m_bInitReal;
    DWORD m_dwMax;
    DWORD m_size;

    int *m_piIcon;  //扩展名对应的系统图标号 如果扩展名
                    //-3表示未初始化 -2表示需要实时调用 -1表示无图标 >=0可直接返回
                    //一定为-2的 exe ico cur 

    PWCHAR *m_vpExtName;//扩展名指针数组

    typedef struct IndexNode
    {
        PWCHAR  pExtName;       //扩展名存储的数据指针,浪费了空间，以便搜索加速
        int     idExtName;      //扩展名的id
        void exchange(IndexNode& node)//交换两个结点
        {
            DWORDLONG *p=(DWORDLONG *)&node,
                *pThis=(DWORDLONG *)this;
            DWORDLONG temp=*p; 
            *p=*pThis; 
            *pThis=temp;
        }
    }INDEXNODE,*PINDEXNODE;
    INDEXNODE *m_vpIndexExtName;//扩展名索引指针数组
};

