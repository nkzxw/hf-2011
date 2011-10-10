#pragma once

#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#pragma comment (lib,"Version")

class CVersion
{
protected:
    typedef struct tagLANGANDCODEPAGE 
    {
        WORD wLanguage;
        WORD wCodePage;
    }LANGANDCODEPAGE,*PLANGANDCODEPAGE;
    
    LANGANDCODEPAGE LangAndCodePage;
    BYTE* pVersionInfo;

public:
    VS_FIXEDFILEINFO FixedFileInfo;

    TCHAR CompanyName[MAX_PATH];
    TCHAR FileDescription[MAX_PATH];
    TCHAR FileVersion[MAX_PATH];
    TCHAR InternalName[MAX_PATH];
    TCHAR LegalCopyright[MAX_PATH];
    TCHAR OriginalFilename[MAX_PATH];
    TCHAR ProductName[MAX_PATH];
    TCHAR ProductVersion[MAX_PATH];

    CVersion(void);
    ~CVersion(void);
    BOOL Read(LPCTSTR FileName);
    BOOL GetValue(LPCTSTR lpKeyName,LPTSTR szValue);
    static BOOL GetPrettyVersion(LPCTSTR lpVersion,IN const DWORD NbDigits,OUT LPTSTR PrettyVersion,IN const SIZE_T PrettyVersionMaxSize);
};
