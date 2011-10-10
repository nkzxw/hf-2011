/*
Copyright (C) 2010 Jacquelin POTIER <jacquelin.potier@free.fr>
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

#include "CommandLineParsing.h"

#include <stdio.h>
#include "../../../../../Tools/File/StdFileOperations.h"
#include "../../../../../Tools/String/AnsiUnicodeConvert.h"

// return : TRUE on SUCCESS, FALSE on error
BOOL CEmulatedRegistryEditorCommandLine::ParseCommandLine()
{
    BOOL bRes=TRUE;
    WCHAR pszError[MAX_PATH];
    LPWSTR* lppwargv;
    int argc=0;
    int cnt;

    // use CommandLineToArgvW
    // Notice : CommandLineToArgvW translate 'option="a";"b"' into 'option=a;b'
    lppwargv=CommandLineToArgvW(
                                GetCommandLineW(),
                                &argc
                                );
    // if no params
    if (argc<=1)
    {
        LocalFree(lppwargv);
        return FALSE;
    }
#ifdef _DEBUG
    // debug : must be present to attach a debugger at startup when command line
    ::MessageBox(0,_T("Starting --> Ready to be attached for debug"),_T("Emulated Registry Editor"),MB_ICONINFORMATION);
#endif
    // for each param
    for(cnt=1;cnt<argc;cnt++)// cnt[0] is app name
    {
        // if configuration file name is specified
        if (wcsnicmp(lppwargv[cnt],L"File",wcslen(L"File"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
                
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(this->FileName,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,this->FileName,MAX_PATH);
#endif
                this->FileName[MAX_PATH-1]=0;
                
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else
        {
            _snwprintf(pszError,MAX_PATH,L"Unknown command line option %s",lppwargv[cnt]);
            MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
            bRes=FALSE;
            break;
        }
    }
    LocalFree(lppwargv);        

    return this->CheckConsistency();
}