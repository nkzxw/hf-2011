// Helper.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#pragma once


extern BYTE g_NoCaseTable[]; //����ʱ�Ѿ���ʼ��

//����ucs2Len
//ucs2Name ��L'\0'��β
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





//����ucs2Len
//ucs2Name ��L'\0'��β
//ucs2Name�е���ĸΪСд
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
            ++pCode;//������ʶ�ַ�
            *pUcs2=*(PWCHAR)pCode;
            pCode+=2;
        }
    }
    *pUcs2=L'\0';
    return pUcs2-ucs2Name;
}

//��UCS2���� �����1�ֽڻ���3�ֽ�
__forceinline int Helper_Ucs2ToCodeCase(OUT PBYTE pCode,IN PWCHAR pUcs2,IN int ucs2Len)
{
    const PWCHAR pUcs2End=pUcs2+ucs2Len;
    const PBYTE pCodeBeg=pCode;
    WCHAR wch;
    for(;pUcs2<pUcs2End;++pUcs2)
    {
        wch=*pUcs2;
        if(wch<0x80){//������ĸ����
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
        if(wch<0x80){//������ĸ����
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

//dΪ-1����ַ�����Ϣ
//d�Ǹ���ʾ�����
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
        sprintf(strInfo,"%d �����!",d);
        SetWindowTextA(g_hStateWnd,strInfo);
    }
}



//����ɱ��ֽڳ����ݳ���
inline int Help_VariantByteSpace(int data)
{
    if(data<128) return 1;              //2^7��
    else if(data<16384)  return 2;      //2^14��
    else if(data<2097152) return 3;     //2^21
    else if(data<268435456) return 4;   //2^28
    else return 5;
}
inline int Help_VariantByteSpaceEx(LONGLONG data)
{
    if(data<128) return 1;  //2^7��
    else if(data<16384)  return 2; //2^14��
    else if(data<2097152) return 3; //2^21
    else if(data<268435456) return 4; //2^28
    else if(data<34359738368) return 5; //2^35
    else if(data<4398046511104) return 6; //2^42
    else return 10;//ex...
}







//��7b ��4b    ��5b     ʱ5b   ��6b
//     0000   00000  00000    000000
//���1980��
#ifndef YEAR_START
#define YEAR_START 1980
#endif
//�洢ʱ��ʱʹ��
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

//����б�����ʾʱ��ʱʹ��
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

