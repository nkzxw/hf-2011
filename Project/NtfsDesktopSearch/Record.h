// Record.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "global.h"

#define GetBasicInfo(dwDri,frn) (((dwDri)<<27)|(DWORD)(frn))
#define GetDirectoryRecordLength(dwCodeNameLen) ((dwCodeNameLen)+10)
#define GetExtIdLength(dwExtId) ((dwExtId)<0x100 ? 1:((dwExtId)<0x10000 ? 2:4))
#define GetNameLenLength(dwCodeNameLen) ((dwCodeNameLen)<0x100?1:2)

//dwCodeNameLen为编码文件名长度
#define GetNormalFileRecordLength(dwCodeNameLen,dwNameLenLength,dwExtIdLength,dwIconLen) (8+(dwCodeNameLen)+(dwNameLenLength)+(dwExtIdLength)+(dwIconLen))
    

struct DirectoryRecord
{
    static const  WORD BITMASK_NONASCII=0x8000;
    static const  WORD BITMASK_NAMElEN=0x7FFF;

    DWORD           BasicInfo;          //高5位为卷号 ''-'A' ~ ''-'Z'

    union
    {
        DirectoryRecord*        ParentPtr;          //父目录的指针
        DWORD                   ParentOffset;       //父目录的偏移(用于数据库导出导入)
        DWORD                   ParentBasicInfo;    //用于初始时建立目录树
    }; 
    WORD            NameLength;         //最高位表示目录文件中是否含有>0x7F字符
                                        //其后表示文件名长
    BYTE            Name[1];            //编码后的 文件名

    //在pAddr处写入压缩数据
    //文件名公共部分长度为dwCommLen
    //返回写入字节数
    DWORD WriteCompressData(DWORD dwCommLen,OUT PBYTE pAddr)
    {
        PBYTE pBase=pAddr;
        *(DWORDLONG*)pAddr=*(DWORDLONG*)this;
        pAddr+=8;
        //使用三字节存储文件名长度以及0x7F信息
        if(NameLength&BITMASK_NONASCII)
        {
            NameLength&=BITMASK_NAMElEN;
            *(DWORD*)pAddr=(dwCommLen|((NameLength-dwCommLen)<<10)|(1<<20));
        }
        else
        {
            *(DWORD*)pAddr=(dwCommLen|((NameLength-dwCommLen)<<10));
        }
        pAddr+=3;
        for(PBYTE p=Name+dwCommLen,pEnd=Name+NameLength;p<pEnd;++p)
        {
            *pAddr++=*p;
        }
        return pAddr-pBase;
    }

    void InitializeData(DWORD dwDri,DWORDLONG frn,DWORD parent_frn,PBYTE pCodeName,DWORD dwCodeNameLen,BOOL bNonAscii)
    {
        if((frn&0xffffffff)>0x7ffffff)
        {
            MessageBox(0,0,0,0);
        }
        BasicInfo=GetBasicInfo(dwDri,frn);
        ParentBasicInfo=GetBasicInfo(dwDri,parent_frn);
        NameLength=(WORD)dwCodeNameLen;
        for(PBYTE pName=Name,pNameEnd=Name+dwCodeNameLen;pName<pNameEnd;++pName)
        {
            *pName=*pCodeName++;
        }       
        if(bNonAscii) NameLength|=BITMASK_NONASCII;
    }

    void SetData(DWORD dwDri,DWORDLONG frn,DirectoryRecord* pParent,PBYTE pCodeName,DWORD dwCodeNameLen,BOOL bNonAscii)
    {
        BasicInfo=GetBasicInfo(dwDri,frn);
        ParentPtr=pParent;
        NameLength=(WORD)dwCodeNameLen;
        for(PBYTE pName=Name,pNameEnd=Name+dwCodeNameLen;pName<pNameEnd;++pName)
        {
            *pName=*pCodeName++;
        }       
        if(bNonAscii) NameLength|=BITMASK_NONASCII;
    }


    //仅设置BasicInfo ，以及文件名信息，不设置父
    void SimpleSetData(DWORD dwDri,DWORDLONG frn,PBYTE pCodeName,DWORD dwCodeNameLen,BOOL bNonAscii)
    {
        BasicInfo=GetBasicInfo(dwDri,frn);
        NameLength=(WORD)dwCodeNameLen;
        for(PBYTE pName=Name,pNameEnd=Name+dwCodeNameLen;pName<pNameEnd;++pName)
        {
            *pName=*pCodeName++;
        }       
        if(bNonAscii) NameLength|=BITMASK_NONASCII;
    }

    DirectoryRecord& operator=(const DirectoryRecord& obj)
    {
        if(this!=&obj)
        {
            CopyMemory(this,&obj,obj.GetLength());
        }
        return *this;
    }
    BOOL HasNonAscii()const//含有汉字或>0x7F吗?
    {
        return NameLength&BITMASK_NONASCII;
    }

    int GetLength()const//获取本记录长度
    {
        return GetDirectoryRecordLength(NameLength&BITMASK_NAMElEN);
    }

    int GetCodeName(OUT PBYTE &pName)//获取编码文件名长度，及其首地址
    {
        pName=Name;
        return NameLength&BITMASK_NAMElEN;
    }
     
    int GetUnicodeName(OUT PWCHAR pUcs2)const//返回UNICODE字符个数
    {
        return Helper_CodeToUcs2Case(pUcs2,(PBYTE)Name,NameLength&BITMASK_NAMElEN);
    }

};

typedef DirectoryRecord DIRECTORY_RECORD,*PDIRECTORY_RECORD;


//普通文件记录
struct NormalFileRecord
{
    static const  DWORD BITMASK_NONASCII=0x80000000;
    static const  DWORD BITMASK_TWOBYTE_NAMELEN=0x40000000;
    static const  DWORD BITMASK_EXTTENDNAME=0x30000000;
    static const  DWORD BITMASK_ONEBYTE_EXTENDNAME=0x10000000;
    static const  DWORD BITMASK_TWOBYTE_EXTENDNAME=0x20000000;
    static const  DWORD BITMASK_ICON=0x8000000;//存在ICON号
    static const  DWORD BITMASK_FOURBYTE_EXTENDNAME=BITMASK_EXTTENDNAME;

    DWORD           BasicInfo;          //最高位表示文件中是否含有>0x7F字符 1表示含有
                                        //次高位为0表示文件名长占1字节 1表是文件名长占2字节 
                                        //次低2位，表示扩展名字节数0 1 2 4
                                        //低位表示FRN_ID
    union
    {
        PDIRECTORY_RECORD   ParentPtr;          //父目录的指针
        DWORD               ParentOffset;       //父目录的偏移(用于数据库导出导入)
        DWORD               ParentBasicInfo;    //用于初始时建立目录树
    }; 
    BYTE            NameInformation[1]; //存储方式:文件名编码长度+文件名编码+[扩展名ID 若ext存在]

    //在pAddr处写入压缩数据
    //文件名公共部分长度为dwCommLen
    //返回写入字节数
    DWORD WriteCompressData(DWORD dwCommLen,PBYTE pAddr)
    {
        PBYTE pBase=pAddr;
        *(DWORDLONG*)pAddr=*(DWORDLONG*)this; //写入头8字节
        pAddr+=8;
        PBYTE Name;
        DWORD NameLength;
        if(BasicInfo&BITMASK_TWOBYTE_NAMELEN){
            Name=NameInformation+2;
            NameLength=*(WORD*)NameInformation;
        }else{
            Name=NameInformation+1;
            NameLength=*NameInformation;
        }
        DWORD *pTag=(DWORD*)pAddr;
        pAddr+=3;
        *pTag=(dwCommLen|((NameLength-dwCommLen)<<10));
        //使用三字节存储文件名长度 10共长 10剩下长 4扩展名长       
        PBYTE p=Name+dwCommLen,pEnd=Name+NameLength;
        for(;p<pEnd;++p)
        {
            *pAddr++=*p;
        }
        DWORD dwExtIdLength=(BasicInfo&BITMASK_EXTTENDNAME)>>28;
        if(dwExtIdLength>0)
        {
            if(1==dwExtIdLength){//1字节
                *pAddr++=*pEnd;
                *pTag|=(1<<20);
            }else{
                if(2==dwExtIdLength){//2字节
                    *(WORD*)pAddr=*(WORD*)pEnd;
                    pAddr+=2;
                    *pTag|=(2<<20);
                }else{//4字节
                    *(DWORD*)pAddr=*(DWORD*)pEnd;
                    pAddr+=4;
                    *pTag|=(4<<20);
                }
            }
        }
        //最后写入扩展名
        return pAddr-pBase;
    }


    //注意，当0==dwExtIdLength时，传入的dwExtId无效
    void InitializeData(BOOL bNonAscii,DWORD dwNameLenLength,DWORD dwExtIdLength,DWORDLONG frn
        ,DWORD dwDri,DWORDLONG parent_frn,PBYTE pCodeName,DWORD dwCodeNameLen,DWORD dwExtId)
    {
        if((frn&0xffffffff)>0x7ffffff)
        {
            MessageBox(0,0,0,0);
        }
        PBYTE pName=NameInformation,pNameEnd;
        BasicInfo=(DWORD)frn;
        if(bNonAscii) BasicInfo|=BITMASK_NONASCII;
        if(2==dwNameLenLength) {
            BasicInfo|=BITMASK_TWOBYTE_NAMELEN;
            *(WORD*)pName=(WORD)dwCodeNameLen;
            pName+=2;           
        }else{
            *pName=(BYTE)dwCodeNameLen;
            ++pName;
        }
        pNameEnd=pName+dwCodeNameLen;

        if(dwExtIdLength>0)
        {
            if(1==dwExtIdLength){//1字节
                BasicInfo|=BITMASK_ONEBYTE_EXTENDNAME; 
                *pNameEnd=(BYTE)dwExtId;
            }else{
                if(2==dwExtIdLength){//2字节
                    BasicInfo|=BITMASK_TWOBYTE_EXTENDNAME; 
                    *(WORD*)pNameEnd=(WORD)dwExtId;
                }else{//4字节
                    BasicInfo|=BITMASK_FOURBYTE_EXTENDNAME;  
                    *(DWORD*)pNameEnd=dwExtId;
                }
            }
            if(dwExtId<CExtArray::s_dwOmitExt) {
                BasicInfo|=BITMASK_ICON;//初始化ICON BIT
                *(DWORD*)(pNameEnd+dwExtIdLength)=0xFFFFFFFF;//初始化ICON
            }
        }

        ParentBasicInfo=GetBasicInfo(dwDri,parent_frn);
        for(;pName<pNameEnd;++pName)
        {
            *pName=*pCodeName++;
        }   
    }

    //增加文件时调用
    void SetData(BOOL bNonAscii,DWORD dwNameLenLength,DWORD dwExtIdLength,DWORDLONG frn
        ,DWORD dwDri,PDIRECTORY_RECORD pParent,PBYTE pCodeName,DWORD dwCodeNameLen,DWORD dwExtId)
    {
        PBYTE pName=NameInformation,pNameEnd;
        BasicInfo=(DWORD)frn;
        if(bNonAscii) BasicInfo|=BITMASK_NONASCII;
        if(2==dwNameLenLength) {
            BasicInfo|=BITMASK_TWOBYTE_NAMELEN;
            *(WORD*)pName=(WORD)dwCodeNameLen;
            pName+=2;           
        }else{
            *pName=(BYTE)dwCodeNameLen;
            ++pName;
        }
        pNameEnd=pName+dwCodeNameLen;

        if(dwExtIdLength>0)
        {
            if(1==dwExtIdLength){//1字节
                BasicInfo|=BITMASK_ONEBYTE_EXTENDNAME; 
                *pNameEnd=(BYTE)dwExtId;
            }else{
                if(2==dwExtIdLength){//2字节
                    BasicInfo|=BITMASK_TWOBYTE_EXTENDNAME; 
                    *(WORD*)pNameEnd=(WORD)dwExtId;
                }else{//4字节
                    BasicInfo|=BITMASK_FOURBYTE_EXTENDNAME;  
                    *(DWORD*)pNameEnd=dwExtId;
                }
            }
            if(dwExtId<CExtArray::s_dwOmitExt) {
                BasicInfo|=BITMASK_ICON;//初始化ICON BIT
                *(DWORD*)(pNameEnd+dwExtIdLength)=0xFFFFFFFF;//初始化ICON
            }
        }
        ParentPtr=pParent;
        for(;pName<pNameEnd;++pName)
        {
            *pName=*pCodeName++;
        }          
    }

    //仅设置BasicInfo ，以及文件名信息，不设置父
    //用于删除文件时 查找 其中BasicInfo信息以及文件名 扩展名信息 必须全
    //可无须父指针 以及ICON
    void SimpleSetData(BOOL bNonAscii,DWORD dwNameLenLength,DWORD dwExtIdLength,DWORDLONG frn
        ,DWORD dwDri,PBYTE pCodeName,DWORD dwCodeNameLen,DWORD dwExtId)
    {
        PBYTE pName=NameInformation,pNameEnd;
        BasicInfo=(DWORD)frn;
        if(bNonAscii) BasicInfo|=BITMASK_NONASCII;
        if(2==dwNameLenLength) {
            BasicInfo|=BITMASK_TWOBYTE_NAMELEN;
            *(WORD*)pName=(WORD)dwCodeNameLen;
            pName+=2;           
        }else{
            *pName=(BYTE)dwCodeNameLen;
            ++pName;
        }
        pNameEnd=pName+dwCodeNameLen;

        if(dwExtIdLength>0)
        {
            if(1==dwExtIdLength){//1字节
                BasicInfo|=BITMASK_ONEBYTE_EXTENDNAME; 
                *pNameEnd=(BYTE)dwExtId;
            }else{
                if(2==dwExtIdLength){//2字节
                    BasicInfo|=BITMASK_TWOBYTE_EXTENDNAME; 
                    *(WORD*)pNameEnd=(WORD)dwExtId;
                }else{//4字节
                    BasicInfo|=BITMASK_FOURBYTE_EXTENDNAME;  
                    *(DWORD*)pNameEnd=dwExtId;
                }
            }
            if(dwExtId<CExtArray::s_dwOmitExt) {
                BasicInfo|=BITMASK_ICON;
                //具体ICON值可不必设置
            }
        }
        for(;pName<pNameEnd;++pName)
        {
            *pName=*pCodeName++;
        }          
    }
    
    
    int GetLength()const//获取记录长度
    {
        DWORD dwExtIdLength=(BasicInfo&BITMASK_EXTTENDNAME)>>28;
        DWORD dwIconLen=0;
        if(BasicInfo&BITMASK_ICON) dwIconLen=4;
        if(3==dwExtIdLength) dwExtIdLength=4;
        if(BasicInfo&BITMASK_TWOBYTE_NAMELEN){
            return GetNormalFileRecordLength(*(WORD*)NameInformation,2,dwExtIdLength,dwIconLen); 
        }else{
            return GetNormalFileRecordLength(*NameInformation,1,dwExtIdLength,dwIconLen); 
        }
    }

    NormalFileRecord& operator=(const NormalFileRecord& obj)
    {
        if(this!=&obj)
        {
            CopyMemory(this,&obj,obj.GetLength());
        }
        return *this;
    }

    BOOL HasNonAscii()const//含有汉字或>0x7F吗?
    {
        return BasicInfo&BITMASK_NONASCII;
    }
    
    BOOL HasExtendName()const
    {
        return BasicInfo&BITMASK_EXTTENDNAME;
    }

    //返回扩展名ID
    //如果返回值为-1 表示无扩展名
    //返回扩展名所占用内存字节数pExtLen
    DWORD GetExtendID(PBYTE pNameEnd,DWORD *pExtLen=NULL)
    {
        DWORD dwExtIdLength=(BasicInfo&BITMASK_EXTTENDNAME)>>28;
        if(pExtLen) *pExtLen=dwExtIdLength;
        if(dwExtIdLength>0)
        {
            if(1==dwExtIdLength){//1字节
                return *pNameEnd;
            }else{
                if(2==dwExtIdLength){//2字节
                    return *(WORD*)pNameEnd;
                }else{//4字节
                    return  *(DWORD*)pNameEnd;
                }
            }
        }
        return -1;
    }

    DWORD GetExtendID()
    {
        PBYTE pName;
        int len=GetCodeName(pName);//获取编码文件名长度，及其首地址
        return GetExtendID(pName+len);
    } 
    
    int GetCodeName(OUT PBYTE &pName)//获取编码文件名长度，及其首地址
    {
        if(BasicInfo&BITMASK_TWOBYTE_NAMELEN){
            pName=NameInformation+2;
            return *(WORD*)NameInformation;
        }else{
            pName=NameInformation+1;
            return *NameInformation;
        }
    }

    int GetUnicodeName(OUT PWCHAR pUcs2)const
    {
        if(BasicInfo&BITMASK_TWOBYTE_NAMELEN){
            return Helper_CodeToUcs2Case(pUcs2,PBYTE(NameInformation+2),*(WORD*)NameInformation);
        }else{
            return Helper_CodeToUcs2Case(pUcs2,PBYTE(NameInformation+1),*NameInformation);
        }
    }

    DWORD GetIcon()
    {

    }

    void SetIcon(DWORD iIcon)
    {

    }
    
};

typedef NormalFileRecord NORMALFILE_RECORD,*PNORMALFILE_RECORD;

//获取文件所在目录,以L'\\'结尾
//如果文件是c:等，其父目录为空
__forceinline int Helper_GetPath(
            OUT WCHAR path[]                    //获取的路径
            ,IN PDIRECTORY_RECORD ParentPtr     //父目录指针
)
{
    static WCHAR dirFileName[MAX_PATH];//仅在显示结果线程中使用本函数，不会产生多线程访问错误
    int nameLen;
    int i,j,iPath=0;
    PDIRECTORY_RECORD pRecord=NULL;

    for(;ParentPtr!=0;ParentPtr=pRecord->ParentPtr)
    {
        pRecord=PDIRECTORY_RECORD(ParentPtr);
        path[iPath++]=L'\\';
        nameLen=pRecord->GetUnicodeName(dirFileName);
        for(i=nameLen-1;i>=0;--i)
        {
            path[iPath++]=dirFileName[i];
        }
    }   
    WCHAR wch;
    for(i=0,j=iPath-1;i<j;++i,--j)
    {
        wch=path[i];path[i]=path[j];path[j]=wch;
    }
    path[iPath]=L'\0';
    return iPath;
}

//无未调用
__forceinline int Helper_GetFullName(
            OUT WCHAR wszFullName[]             //获取的绝对文件名
            ,IN PDIRECTORY_RECORD   ParentPtr   //父目录指针
            ,IN PWCHAR fileName                 //当前UNICODE文件名
            ,IN int nameLen                     //文件名文件名长度UNICODE字符
)
{    
    if(fileName[1]==L':'){
        wszFullName[0]=fileName[0];wszFullName[1]=L':';wszFullName[2]=L'\\';wszFullName[3]=L'\0';
        return 3;
    }
    int i,j,iPath=0;
    for(i=nameLen-1;i>=0;--i)
    {
        wszFullName[iPath++]=fileName[i];
    }
    static WCHAR dirFileName[MAX_PATH];
    PDIRECTORY_RECORD pRecord=NULL;
    for(;ParentPtr!=0;ParentPtr=pRecord->ParentPtr)
    {
        pRecord=PDIRECTORY_RECORD(ParentPtr);
        wszFullName[iPath++]=L'\\';
        nameLen=pRecord->GetUnicodeName(dirFileName);
        for(i=nameLen-1;i>=0;--i)
        {
            wszFullName[iPath++]=dirFileName[i];
        }
    }   
    WCHAR wch;
    for(i=0,j=iPath-1;i<j;++i,--j)
    {
        wch=wszFullName[i];wszFullName[i]=wszFullName[j];wszFullName[j]=wch;
    }
    wszFullName[iPath]=L'\0';
    return iPath;
}
