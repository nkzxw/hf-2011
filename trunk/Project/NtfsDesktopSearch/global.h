// global.h
// ��Ȩ����(C) ����
// Homepage:
// Email:chenxiong115@163.com chenxiong115@qq.com
// purpose:
// ���������κη�ʽʹ�ñ����룬������Ա����벻����
// �����Խ�����顣��Ҳ����ɾ����Ȩ��Ϣ��������ϵ��ʽ��
// ���������һ�������Ļ��ᣬ�ҽ���ָ�л��
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#pragma warning(disable: 4244 4996 4800 4018 4311 4312)

#if !defined(_WIN32_WINNT) || _WIN32_WINNT!=0x5000
#define _WIN32_WINNT 0x5000
#endif

#define _CRT_SECURE_NO_DEPRECATE

#define CURDIR L"c:\\" //���ݿ��ļ����ڴ�Ŀ¼��

#include "resource.h"
#include <shlobj.h>
#include <assert.h>
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <winioctl.h>

#include "Lock.h"
#include "Helper.h"
#include "shell.h"
#include "ntfs.h"
#include "NameSort.h"
#include "MemoryMgr.h"
#include "MemoryPool.h"
#include "ExtArray.h"
#include "queue.h"
#include "StrMatch.h"
#include "Record.h"
#include "IndexNodeBlock.h"
#include "Index.h"
#include "DirBasicInfoMap.h"
#include "OutVector.h"
#include "WriteMgr.h"
#include "DirFilterList.h"
#include "FilterCtrl.h"

#pragma comment(lib,"comctl32.lib")


#ifndef _T
#ifdef UNICODE
#define _T(x) L##x
#else
#define _T(x) x
#endif
#endif


#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")


typedef PVOID   *PPVOID;
typedef PDWORD  *PPDWORD;
typedef PBYTE   *PPBYTE;
typedef PPBYTE  *PPPBYTE;




extern HANDLE g_hEvent;
extern HWND g_hStateWnd;
extern HWND g_hListCtrl;
extern HWND g_hEdit;
extern BOOL g_bCanSearch;
extern BYTE g_NoCaseTable[]; //����ʱ�Ѿ���ʼ��

extern CMemoryMgr g_MemoryMgr;
extern CMemoryPool g_MemFile,g_MemDir;//�ļ���Ŀ¼�����ݶ������ڴ˴�
extern CIndex g_vDirIndex,g_vFileIndex;
extern CDirBasicInfoMap g_DirMap;
extern CExtArray g_ExtMgr;
extern COutVector g_vDirOutPtr,g_vFileOutPtr;


inline void DebugStringA(char *pszFormat,...)
{
//#ifdef _DEBUG
    va_list argList;
    va_start(argList,pszFormat);
    char strInfo[4096];
    vsprintf(strInfo,pszFormat,argList);
    va_end(argList);
    MessageBoxA(NULL,strInfo,"����",MB_ICONHAND);
//#endif
}

inline void DebugStringW(WCHAR *pszFormat,...)
{
//#ifdef _DEBUG
    va_list argList;
    va_start(argList,pszFormat);
    WCHAR strInfo[4096];
    vswprintf(strInfo,pszFormat,argList);
    va_end(argList);
    MessageBoxW(NULL,strInfo,L"����",MB_ICONHAND);
//#endif
}

#ifdef UNICODE
#define DebugString  DebugStringW
#else
#define DebugString  DebugStringA
#endif

class CDebugTrace
{
public:
    CDebugTrace()
    {
        m_pFile=NULL;
    }
    virtual ~CDebugTrace()
    {
        if(m_pFile) {
            fclose(m_pFile);
            m_pFile=NULL;
        }
    }
    void operator=(FILE *pFile)
    {
        m_pFile=pFile;
    }
    operator FILE*(){return m_pFile;}
private:
    FILE *m_pFile;
};

extern CDebugTrace g_dbgTrace;

inline void DebugTrace0(char *DebugInfo,...)//inline�޷���������ֹ�ض���
{
    static char szInfoBuf[1024];
    if(NULL==g_dbgTrace)
    {
        g_dbgTrace=fopen("C:\\NtfsTitleFinder.log","w");
        if(NULL==g_dbgTrace) return;
        g_dbgTrace=g_dbgTrace;
    }
    va_list argList;
    va_start(argList,DebugInfo);
    vsprintf(szInfoBuf,DebugInfo,argList);
    va_end(argList);
    fprintf(g_dbgTrace,"%s",szInfoBuf);
    fflush(g_dbgTrace);
}

#ifdef _DEBUG
#define DebugTrace(...)	DebugTrace0(__VA_ARGS__)
#else
#define DebugTrace(...)
#endif


inline void ScreenToClient(__in HWND hWnd, __inout RECT* lpRect)
{
    POINT *pt1=(POINT *)lpRect;
    POINT *pt2=pt1+1;
    ScreenToClient(hWnd,pt1);
    ScreenToClient(hWnd,pt2);
}
