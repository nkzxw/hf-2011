// ExtArray.cpp
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong0115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "ExtArray.h"


CExtArray g_ExtMgr;//扩展名管理

//初始化扩展名列表
//该列表中的扩展名所对应的图标不唯一
//必须存储或者实时调用

PWCHAR g_pOmitExt[]=
{
    L"cur"
    ,L"exe"
    ,L"ico"
    ,L"lnk"
};

// PWCHAR g_pInitExt[]=
// {
//     L"avi"
//     ,L"bin"
//     ,L"bmp"
//     ,L"chm"
//     ,L"cpp"
//     ,L"cur"
//     ,L"dat"
//     ,L"dll"
//     ,L"exe"
//     ,L"gif"
//     ,L"htm"
//     ,L"html"
//     ,L"ico"
//     ,L"ime"
//     ,L"ini"
//     ,L"jar"
//     ,L"java"
//     ,L"jpeg"
//     ,L"jpg"
//     ,L"js"
//     ,L"jsp"
//     ,L"kdh"     //CAJViewr 同方知网
//     ,L"key"     //注册表
//     ,L"krc"     //歌词文件
//     ,L"lib"
//     ,L"lnk"     //快捷方式
//     ,L"log"
//     ,L"m"
//     ,L"mdb"
//     ,L"mp3"
//     ,L"ncb"
//     ,L"nh"
//     ,L"o"
//     ,L"obj"
//     ,L"ocx"
//     ,L"opt"
//     ,L"p"       //matlab
//     ,L"pdf"
//     ,L"pdg"
//     ,L"png"
//     ,L"ppt"
//     ,L"qpyd"
//     ,L"rar"     //压缩文件
//     ,L"rc"
//     ,L"rc2"
//     ,L"reg"
//     ,L"resx"
//     ,L"skn"
//     ,L"sln"
//     ,L"suo"
//     ,L"swf"
//     ,L"sys"
//     ,L"tmp"
//     ,L"txt"
//     ,L"url"
//     ,L"vb"
//     ,L"vbs"
//     ,L"vcproj"
//     ,L"wav"
//     ,L"wmf"
//     ,L"xls"
//     ,L"xml"
//     ,L"xsl"
//     ,L"zip"
// };

//实时调用图标个数
DWORD CExtArray::s_dwOmitExt=sizeof(g_pOmitExt)/sizeof(*g_pOmitExt);

CExtArray::CExtArray()
{
    m_size=0;
    m_dwMax=512;
    m_piIcon=(int*)g_MemoryMgr.malloc(sizeof(int)*m_dwMax);

    m_vpExtName=(PWCHAR*)g_MemoryMgr.malloc(sizeof(PWCHAR)*m_dwMax);
    
    m_vpIndexExtName=(PINDEXNODE)g_MemoryMgr.malloc(sizeof(INDEXNODE)*m_dwMax);

    m_bInitReal=FALSE;
}

CExtArray::~CExtArray()
{
    if(!m_bInitReal)
    {//动态分配的
        for(int i=0;i<s_dwOmitExt;++i){
            g_MemoryMgr.free(m_vpExtName[i]);
        }
    }
    for(int i=s_dwOmitExt;i<m_size;++i){
        g_MemoryMgr.free(m_vpExtName[i]);
    }
    g_MemoryMgr.free(m_vpExtName);
    g_MemoryMgr.free(m_piIcon);
    g_MemoryMgr.free(m_vpIndexExtName);
}

//初始化实时ICON扩展名
void CExtArray::InitRealTimeCallExt()
{   
    m_bInitReal=TRUE;
    for(int id=0;id<s_dwOmitExt;++id)
    {
        m_vpExtName[id]=g_pOmitExt[id];
        m_piIcon[id]=ICON_INDEX_REALTIMECALL;
        m_vpIndexExtName[id].pExtName=g_pOmitExt[id];//
        m_vpIndexExtName[id].idExtName=id;
    }
    m_size+=s_dwOmitExt;
}

//遍历文件库时需要插入扩展名
//并返回该扩展名所对应的ID
int CExtArray::insert(PWCHAR pszExtName,int ExtNameLen)
{
    int low,high,mid;//逐字二分搜索标记
    int iInsert;//要插入的位置
    int i,j;//临时表量
    for(i=0;i<ExtNameLen;++i)//转换文件名为小写
    {
        if(pszExtName[i]>=L'A' && pszExtName[i]<=L'Z') pszExtName[i]+=32;
    }

    //首先搜索,看其是否存在
    WCHAR wchDest,wch,*pName;
    low=0;high=m_size-1;
    for(i=0;i<ExtNameLen;++i)
    {
        wchDest=pszExtName[i];        
        while(low<=high)
        {
            mid=low+((high-low)>>1);
            wch=m_vpIndexExtName[mid].pExtName[i];
            if(wch>wchDest) high=mid-1;
            else if(wch<wchDest) low=mid+1;
            else{
                for(j=mid-1;j>=low && m_vpIndexExtName[j].pExtName[i]==wchDest;--j);
                if(j>=low) low=j+1;
                for(j=mid+1;j<=high && m_vpIndexExtName[j].pExtName[i]==wchDest;++j);
                if(j<=high) high=j-1;
                break;
            }
        }
        if(low>high)
        {//第i处没有找到匹配，将词后缀插入库中
            iInsert=low;//待验证
            break;
        }
    }
 
    //有可能此处low<high 即存在多个前缀匹配 low是最小
    //匹配成功
    if(i==ExtNameLen){
        if(L'\0'==m_vpIndexExtName[low].pExtName[ExtNameLen])
            return m_vpIndexExtName[low].idExtName;
        iInsert=low;
    }
        
    if(m_size==m_dwMax) //当前空间已满
    {
        m_dwMax+=DELT;
        m_vpExtName=(PWCHAR*)g_MemoryMgr.realloc(m_vpExtName,sizeof(PWCHAR)*m_dwMax);
        m_piIcon=(int*)g_MemoryMgr.realloc(m_piIcon,sizeof(int)*m_dwMax);

        PINDEXNODE pTemp=(PINDEXNODE)g_MemoryMgr.malloc(sizeof(INDEXNODE)*m_dwMax);
        CopyMemory(pTemp,m_vpIndexExtName,iInsert*sizeof(INDEXNODE));
        CopyMemory(pTemp+iInsert+1,m_vpIndexExtName+iInsert,(m_size-iInsert)*sizeof(INDEXNODE));
        g_MemoryMgr.free(m_vpIndexExtName);
        m_vpIndexExtName=pTemp;
    }
    else
    {
        for(j=m_size;j>iInsert;--j)//索引后移
        {
            m_vpIndexExtName[j]=m_vpIndexExtName[j-1];
        }
    }
    pName=(PWCHAR)g_MemoryMgr.malloc((sizeof(WCHAR)*(ExtNameLen+1)));
    pName[ExtNameLen]=L'\0';
    wcsncpy(pName,pszExtName,ExtNameLen);
    m_vpExtName[m_size]=pName;
    m_vpIndexExtName[iInsert].pExtName=pName;
    m_vpIndexExtName[iInsert].idExtName=m_size;             
    m_piIcon[m_size]=ICON_INDEX_UNINITIALIZED;
    ++m_size;  
    return m_size-1;   
}



//遍历文件库时需要插入扩展名
//并返回该扩展名所对应的ID
int CExtArray::find(PWCHAR pszExtName,int ExtNameLen)const
{
    int low,high,mid;//逐字二分搜索标记
    int i,j;//临时表量
    for(i=0;i<ExtNameLen;++i)//转换文件名为小写
    {
        if(pszExtName[i]>=L'A' && pszExtName[i]<=L'Z') pszExtName[i]+=32;
    }

    //首先搜索,看其是否存在
    WCHAR wchDest,wch;
    low=0;high=m_size-1;
    for(i=0;i<ExtNameLen;++i)
    {
        wchDest=pszExtName[i];        
        while(low<=high)
        {
            mid=low+((high-low)>>1);
            wch=m_vpIndexExtName[mid].pExtName[i];
            if(wch>wchDest) high=mid-1;
            else if(wch<wchDest) low=mid+1;
            else{
                for(j=mid-1;j>=low && m_vpIndexExtName[j].pExtName[i]==wchDest;--j);
                if(j>=low) low=j+1;
                for(j=mid+1;j<=high && m_vpIndexExtName[j].pExtName[i]==wchDest;++j);
                if(j<=high) high=j-1;
                break;
            }
        }
        if(low>high)
        {//第i处没有找到匹配，将词后缀插入库中
            break;
        }
    }

    //有可能此处low<high 即存在多个前缀匹配 low是最小
    //匹配成功
    if(i==ExtNameLen){
        if(L'\0'==m_vpIndexExtName[low].pExtName[ExtNameLen]){
            return m_vpIndexExtName[low].idExtName;
        }
    }
    return -1;
}


//获得几号扩展名的ICON索引
//若返回>-2 可直接使用
//==-2 需要用户实时获取
//==-3 表明还没有初始化该id ,用户需要调用SetIconIndex来初始化扩展名索引
int CExtArray::GetIconIndex(int idExtName)const
{
    assert(idExtName>=0 && idExtName<m_size &&"GetIconIndex");
    return m_piIcon[idExtName];
}

int CExtArray::Compare(int idExt1,int idExt2)const
{
    assert(idExt1>=0 && idExt1<m_size && idExt2>=0 && idExt2<m_size);
    PWCHAR str1=m_vpExtName[idExt1],str2=m_vpExtName[idExt2];
    int ret=0 ;
    while(!(ret=(int)(*str1-*str2))&&*str2)
        ++str1,++str2;
    if(ret<0)
        ret=-1 ;
    else if(ret>0)
        ret=1 ;
    return(ret);
}

PWCHAR CExtArray::GetExtName(int idExtName)const
{
    assert(idExtName>=0 && idExtName<m_size &&"GetExtName");
    return m_vpExtName[idExtName];
}

//
int CExtArray::GetExtIndexSequence(int idExtName)const//获取扩展名存储ID位于索引ID的第几号
{
    if(idExtName<0 || idExtName>=m_size) return -1;
    return this->find(m_vpExtName[idExtName]);
}


//idExtName为扩展名ID
//pFilePath为带斜线目录名
//filePathLen为带斜线目录名长度
//pFileName文件名
//fileLen文件名长度
int CExtArray::SetIconIndex(int idExtName,PWCHAR pFilePath,int filePathLen,PWCHAR pFileName,int fileLen)
{
    assert(idExtName>=s_dwOmitExt && "已经初始化实时调用的不能再传递进来");

    SHFILEINFOW sfi;

    //现将filePath扩展为文件名
    int j=filePathLen;
    for(int i=0;i<fileLen;++i)
    {
        pFilePath[j++]=pFileName[i];
    }
    pFilePath[j]=L'\0';

    SHGetFileInfoW(pFilePath,  
        FILE_ATTRIBUTE_NORMAL,  
        &sfi,  
        sizeof(sfi),  
        SHGFI_SMALLICON
        | SHGFI_SYSICONINDEX 
        | SHGFI_USEFILEATTRIBUTES
        ) ;
    pFilePath[filePathLen]=L'\0';//还原filePath
    m_piIcon[idExtName]=sfi.iIcon;
    return sfi.iIcon;
}


//扩展名数据库操作，优先加载
//若返回FALSE 则文件名数据库中的扩展名部分无效
//这时需要重新生成文件名数据库
BOOL CExtArray::LoadFromFile(PWCHAR pwszExtDatabase)
{
    HANDLE hFile=CreateFileW(pwszExtDatabase
        ,GENERIC_READ
        ,FILE_SHARE_READ
        ,NULL
        ,OPEN_EXISTING
        ,FILE_ATTRIBUTE_NORMAL
        ,NULL);
    if(INVALID_HANDLE_VALUE==hFile){
        DebugStringA("读扩展名文件失败:%d",GetLastError());
        return FALSE;
    }
    DWORD dwFileSize=GetFileSize(hFile,NULL);
    PBYTE pByte=g_MemoryMgr.GetMemory(dwFileSize),pEnd=pByte+dwFileSize;
    DWORD dwRead;
    ReadFile(hFile,pByte,dwFileSize,&dwRead,NULL);
    assert(dwFileSize==dwRead);
    m_size=*(DWORD*)(pByte);
    if(m_size>=m_dwMax)
    {
        m_dwMax=m_size+DELT;
        m_vpExtName=(PWCHAR*)g_MemoryMgr.realloc(m_vpExtName,sizeof(PWCHAR)*m_dwMax);
        m_piIcon=(int*)g_MemoryMgr.realloc(m_piIcon,sizeof(int)*m_dwMax);
        m_vpIndexExtName=(PINDEXNODE)g_MemoryMgr.realloc(m_vpIndexExtName,sizeof(INDEXNODE)*m_dwMax);
    }
    size_t extLen;
    int i=0,j;
    int idExt;
    PWCHAR pName;
    for(pByte+=4;pByte<pEnd;)
    {
         idExt=*(int*)pByte;
         m_vpIndexExtName[i].idExtName=idExt;
         pByte+=4;
         m_piIcon[idExt]=*(int*)pByte;
         pName=PWCHAR(pByte+4);
         extLen=wcslen(pName);
         m_vpExtName[idExt]=(PWCHAR)g_MemoryMgr.malloc((sizeof(WCHAR)*(extLen+1)));
         m_vpIndexExtName[i].pExtName=m_vpExtName[idExt];
         for(j=0;j<extLen;++j){
            m_vpExtName[idExt][j]=*pName++;
         }
         m_vpExtName[idExt][j]=L'\0';
         pByte=PBYTE(pName+1);   
         i++;
    }
    assert(i==m_size);
    CloseHandle(hFile);
    return TRUE;
}

//在写文件名数据库之后写到扩展名数据库
//格式：IDICON名L'\0'
BOOL CExtArray::WriteToFile(PWCHAR pwszExtDatabase)
{
    HANDLE hFile=CreateFileW(pwszExtDatabase
        ,GENERIC_READ|GENERIC_WRITE
        ,FILE_SHARE_READ|FILE_SHARE_WRITE
        ,NULL
        ,CREATE_ALWAYS
        ,FILE_ATTRIBUTE_NORMAL
        ,NULL);
    if(INVALID_HANDLE_VALUE==hFile){
        DebugStringA("写入扩展名文件失败:%d",GetLastError());
        return FALSE;
    }
    //按扩展名字母顺序写入
    DWORD dwWrited;
    char buf[1024];
    int len;
    int *pId=(int*)buf;
    int *pIcon=(int*)(buf+4);
    PWCHAR pExtName=(PWCHAR)(buf+8),pSrc;

    WriteFile(hFile,&m_size,sizeof(m_size),&dwWrited,NULL);//写入扩展名总数
    for(int i=0;i<m_size;++i)
    {      
        *pId=m_vpIndexExtName[i].idExtName;
        *pIcon=ICON_INDEX_UNINITIALIZED;
        pSrc=m_vpExtName[*pId];
        for(len=0;*pSrc;++pSrc)
        {
            pExtName[len++]=*pSrc;
        }
        pExtName[len++]=L'\0';
        len=(len<<1)+8;
        WriteFile(hFile,buf,len,&dwWrited,NULL);
        assert(len==dwWrited);
    }
    CloseHandle(hFile);
    return FALSE;
}


//返回-1表示没有找到
//扩展名以L'\0'结束
int  CExtArray::find(PWCHAR pszExtName)const
{
        int low,high,mid;//逐字二分搜索标记
        int i,j;//临时表量

        //首先搜索,看其是否存在
        WCHAR wchDest,wch;
        low=0;high=m_size-1;

        i=-1;
        for(;*pszExtName!=L'\0';++pszExtName)
        {
            ++i;
            wchDest=*pszExtName;        
            while(low<=high)
            {
                mid=low+((high-low)>>1);
                wch=m_vpIndexExtName[mid].pExtName[i];
                if(wch>wchDest) high=mid-1;
                else if(wch<wchDest) low=mid+1;
                else{
                    for(j=mid-1;j>=low && m_vpIndexExtName[j].pExtName[i]==wchDest;--j);
                    if(j>=low) low=j+1;
                    for(j=mid+1;j<=high && m_vpIndexExtName[j].pExtName[i]==wchDest;++j);
                    if(j<=high) high=j-1;
                    break;
                }
            }
            if(low>high)
            {//第i处没有找到匹配，将词后缀插入库中
                return -1;
            }
        }

        //有可能此处low<high 即存在多个前缀匹配 low是最小
        //匹配成功
        if(*pszExtName==L'\0'){
            if(L'\0'==m_vpIndexExtName[low].pExtName[i])
                return low;
        }
        return -1;
}
    