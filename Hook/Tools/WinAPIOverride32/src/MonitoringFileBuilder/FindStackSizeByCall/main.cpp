/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <windows.h>
#include <stdio.h>

#include "FindStackSizeByCall.h"
#include "..\..\Tools\String\AnsiUnicodeConvert.h"

#define APPLICATION_NAME _T("FindStackSizeByCall")
#define USAGE_ERROR (-1)

void ShowUsage()
{
    ::MessageBox(NULL,
                 _T("Usage :\r\n")
                 APPLICATION_NAME
                 _T(" DllPath FunctionName\r\n")
                 _T("or \r\n")
                 APPLICATION_NAME
                 _T(" DllPath 0xFunctionOrdinal\r\n")
                 ,
                 _T("Bad Options"),
                 MB_OK | MB_ICONERROR
                 );
}


// returns :
// 1) any possible value on crash (or if function internally calls an ExitProcess like API)
// 2) a (CFindStackSizeByCall::FAILURE | CFindStackSizeByCall::ERROR_CODES) on error
// 3) a (CFindStackSizeByCall::SUCCESS | Stack size) on success
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow
                   )
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    int RetValue=USAGE_ERROR;
    TCHAR pszLibName[MAX_PATH];
    TCHAR pszFuncName[MAX_PATH];
    DWORD Ordinal=0;
    BOOL bOrdinal=FALSE;
    DWORD StackSize=0;


    ////////////////////////
    // parameters parsing
    ////////////////////////

    // get command line
    int NbArgs=0;
    LPWSTR* ArgsW=::CommandLineToArgvW(::GetCommandLineW(),&NbArgs);
    if (!ArgsW)
        NbArgs=0;

    // ArgsW[0] is app name
    // ArgsW[1] // dll path
    // ArgsW[2] // 0xOrdinalValue || Function name

    // check parameters number
    if (NbArgs!=3)
    {
        ShowUsage();
        ::GlobalFree(ArgsW);
        return USAGE_ERROR;
    }

    // get dll name
#if (defined(UNICODE)||defined(_UNICODE))
    wcscpy(pszLibName,ArgsW[1]);
#else
    CAnsiUnicodeConvert::UnicodeToAnsi(ArgsW[1],pszLibName,MAX_PATH);
#endif

    // try to get ordinal value
    if (swscanf(ArgsW[2],L"0x%x",&Ordinal)==1)
    {
        bOrdinal=TRUE;
    }
    // if not ordinal
    else
    {
        // get function name
#if (defined(UNICODE)||defined(_UNICODE))
        wcscpy(pszFuncName,ArgsW[2]);
#else
        CAnsiUnicodeConvert::UnicodeToAnsi(ArgsW[2],pszFuncName,MAX_PATH);
#endif
    }
 
    DWORD Ret=CFindStackSizeByCall::FindStackSizeByCall(pszLibName,pszFuncName,Ordinal,bOrdinal,&StackSize);
    if (Ret==CFindStackSizeByCall::ERROR_CODE_NO_ERROR)
    {
        // on success, add stack size to return
        RetValue=(CFindStackSizeByCall::SUCCESS) | (StackSize & CFindStackSizeByCall::MAX_NUMBER_OF_PARAM);
    }
    else
    {
        RetValue=(CFindStackSizeByCall::FAILURE) | (Ret & CFindStackSizeByCall::RESULT_MASK);
    }

    ::GlobalFree(ArgsW);
    return RetValue;
}
