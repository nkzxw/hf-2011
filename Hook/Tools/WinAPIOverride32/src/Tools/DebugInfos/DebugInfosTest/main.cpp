#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "debuginfosui.h"

// required lib : comctl32.lib
#pragma comment (lib,"comctl32")

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow
                   )
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    TCHAR* psz;
    InitCommonControls();

    if (*lpCmdLine==0)
        psz=NULL;
    else
    {
#if (defined(UNICODE)||defined(_UNICODE))
        SIZE_T Size=strlen(lpCmdLine)+1;
        psz=(TCHAR*)_alloca(Size*sizeof(TCHAR));
        CAnsiUnicodeConvert::AnsiToUnicode(lpCmdLine,psz,Size);
#else
        psz=lpCmdLine;
#endif
    }
    CDebugInfosUI::Show(hInstance,NULL,psz);
    return 0;
}