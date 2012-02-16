// Helper.h
// 版权所有(C) 陈雄
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// 您可以以任何方式使用本代码，如果您对本代码不满，
// 您可以将其粉碎。您也可以删除版权信息和作者联系方式。
// 如果您给我一个进步的机会，我将万分感谢。
/////////////////////////////////////////////////////////////////////////////////
#pragma once


extern BYTE g_NoCaseTable[]; //搜索时已经初始化

//返回ucs2Len
//ucs2Name 以L'\0'结尾
__forceinline int Helper_CodeToUcs2Case(OUT PWCHAR ucs2Name,IN PBYTE pCode,IN int codeLen)
{
    PBYTE pCodeEnd=pCode+codeLen;
    PWCHAR pUcs2=ucs2Name;
    for(;pCode<pCodeEnd;++pUcs2)
    {
        if(*pCode<0x80){
            *pUcs2=*pCode++;
        }else{
            ++pCode;
            *pUcs2=*(PWCHAR)pCode;
            pCode+=2;
        }
    }
    *pUcs2=L'\0';
    return pUcs2-ucs2Name;
}





//返回ucs2Len
//ucs2Name 以L'\0'结尾
//ucs2Name中的字母为小写
__forceinline int Helper_CodeToUcs2NoCase(OUT PWCHAR ucs2Name,IN PBYTE pCode,IN int codeLen)
{
    PBYTE pCodeEnd=pCode+codeLen;
    PWCHAR pUcs2=ucs2Name;
    BYTE code;
    for(;pCode<pCodeEnd;++pUcs2)
    {
        code=*pCode;
        if(code<0x80){
            *pUcs2=g_NoCaseTable[code];
            ++pCode;
        }else{
            ++pCode;//跳过标识字符
            *pUcs2=*(PWCHAR)pCode;
            pCode+=2;
        }
    }
    *pUcs2=L'\0';
    return pUcs2-ucs2Name;
}

//将UCS2编码 编码成1字节或者3字节
__forceinline int Helper_Ucs2ToCodeCase(OUT PBYTE pCode,IN PWCHAR pUcs2,IN int ucs2Len)
{
    const PWCHAR pUcs2End=pUcs2+ucs2Len;
    const PBYTE pCodeBeg=pCode;
    WCHAR wch;
    for(;pUcs2<pUcs2End;++pUcs2)
    {
        wch=*pUcs2;
        if(wch<0x80){//常用字母数字
            *pCode++=wch;
        }else{
            *pCode++=0x81;
            *(PWCHAR)pCode=wch;
            pCode+=2;
        }
    }
    return pCode-pCodeBeg;
}
__forceinline int Helper_Ucs2ToCode(OUT PBYTE pCode,IN PWCHAR pUcs2,IN int ucs2Len)
{
    const PWCHAR pUcs2End=pUcs2+ucs2Len;
    const PBYTE pCodeBeg=pCode;
    WCHAR wch;
    for(;pUcs2<pUcs2End;++pUcs2)
    {
        wch=*pUcs2;
        if(wch<0x80){//常用字母数字
            *pCode++=wch;
        }else{
            *pCode++=0x81;
            *(PWCHAR)pCode=wch;
            pCode+=2;
        }
    }
    return pCode-pCodeBeg;
}



__forceinline bool CreateUsnJournal(HANDLE hVolume,DWORDLONG MaximumSize, DWORDLONG AllocationDelta)
{
    DWORD cb;
    CREATE_USN_JOURNAL_DATA cujd;
    cujd.MaximumSize = MaximumSize;
    cujd.AllocationDelta = AllocationDelta;
    return DeviceIoControl(hVolume, FSCTL_CREATE_USN_JOURNAL, 
        &cujd, sizeof(cujd), NULL, 0, &cb, NULL);
}

__forceinline bool QueryUsnJournal(HANDLE hVolume,PUSN_JOURNAL_DATA pUsnJournalData)
{
    DWORD cb;
    return DeviceIoControl(hVolume
        ,FSCTL_QUERY_USN_JOURNAL
        ,NULL,0, 
        pUsnJournalData,sizeof(USN_JOURNAL_DATA), &cb
        ,NULL);
}


extern HWND g_hStateWnd;

//d为-1输出字符串信息
//d非负表示结果数
inline void Helper_SetCurrentState(int d,char* pszState,...)
{
    char strInfo[1024];
    if(-1==d)
    {
        va_list argList;
        va_start(argList,pszState);       
        vsprintf(strInfo,pszState,argList);
        va_end(argList);
        SetWindowTextA(g_hStateWnd,strInfo);
    }
    else
    {
        sprintf(strInfo,"%d 个结果!",d);
        SetWindowTextA(g_hStateWnd,strInfo);
    }
}



//计算可变字节长数据长度
inline int Help_VariantByteSpace(int data)
{
    if(data<128) return 1;              //2^7个
    else if(data<16384)  return 2;      //2^14个
    else if(data<2097152) return 3;     //2^21
    else if(data<268435456) return 4;   //2^28
    else return 5;
}
inline int Help_VariantByteSpaceEx(LONGLONG data)
{
    if(data<128) return 1;  //2^7个
    else if(data<16384)  return 2; //2^14个
    else if(data<2097152) return 3; //2^21
    else if(data<268435456) return 4; //2^28
    else if(data<34359738368) return 5; //2^35
    else if(data<4398046511104) return 6; //2^42
    else return 10;//ex...
}







//年7b 月4b    日5b     时5b   分6b
//     0000   00000  00000    000000
//年从1980起
#ifndef YEAR_START
#define YEAR_START 1980
#endif
//存储时间时使用
__forceinline DWORD Helper_ConvertSystemTimeToTime32(SYSTEMTIME &sysTime)
{
    DWORD time32;
    time32=sysTime.wMinute;
    time32|=(sysTime.wHour<<6);
    time32|=(sysTime.wDay<<11);
    time32|=(DWORD(sysTime.wMonth)<<16);
    time32|=(DWORD(sysTime.wYear-YEAR_START)<<20);
    return time32;
}

//结果列表中显示时间时使用
__forceinline void Helper_ConvertTime32ToSystemTime(OUT SYSTEMTIME &sysTime,IN DWORD time32)
{
    sysTime.wMinute=time32&0x3f;time32>>=6;
    sysTime.wHour=time32&0x1f;time32>>=5;
    sysTime.wDay=time32&0x1f;time32>>=5;
    sysTime.wMonth=time32&0xf;time32>>=4;
    sysTime.wYear=time32+YEAR_START;
}



__forceinline DWORDLONG Helper_GetFileSize(PWCHAR pszPath,int pathLen,PWCHAR pFileName,int fileLen)
{
    int j=pathLen;
    for(int i=0;i<fileLen;++i)
    {
        pszPath[j++]=pFileName[i];
    }
    pszPath[j]=L'\0';

    HANDLE hFile=CreateFileW(pszPath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,OPEN_EXISTING,NULL,NULL);
    pszPath[pathLen]=L'\0';
    if(INVALID_HANDLE_VALUE==hFile) return -1;

    LARGE_INTEGER lSize;
    GetFileSizeEx(hFile,&lSize);
    CloseHandle(hFile);
    return lSize.QuadPart;  
}

