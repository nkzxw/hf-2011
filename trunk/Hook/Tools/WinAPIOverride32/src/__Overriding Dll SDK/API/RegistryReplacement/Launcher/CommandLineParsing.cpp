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
BOOL ParseCommandLine(IN OUT CLauncherOptions* pLauncherOptions)
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
        
        // load default config file
        TCHAR DefaultConfigFile[MAX_PATH];
        CStdFileOperations::GetAppName(DefaultConfigFile,MAX_PATH);
        CStdFileOperations::ChangeFileExt(DefaultConfigFile,_T("ini"));
        if(!pLauncherOptions->Load(DefaultConfigFile,FALSE))
            return FALSE;
        return pLauncherOptions->CheckConsistency(FALSE);
    }

    // for each param
    for(cnt=1;cnt<argc;cnt++)// cnt[0] is app name
    {
        // if configuration file name is specified
        if (wcsnicmp(lppwargv[cnt],L"CfgFile",wcslen(L"CfgFile"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
                
                TCHAR LauncherConfigFile[MAX_PATH];
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(LauncherConfigFile,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,LauncherConfigFile,MAX_PATH);
#endif
                LauncherConfigFile[MAX_PATH-1]=0;
                
               
                TCHAR ConfigFileNameAbsolutePath[MAX_PATH];
                CStdFileOperations::GetAbsolutePath(LauncherConfigFile,ConfigFileNameAbsolutePath);  
                pLauncherOptions->Load(ConfigFileNameAbsolutePath,TRUE);
                // continue parsing in case ProcessID and ThreadId are specified
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Launcher Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        if (wcsnicmp(lppwargv[cnt],L"TargetName",wcslen(L"TargetName"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(pLauncherOptions->TargetName,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pLauncherOptions->TargetName,MAX_PATH);
#endif
                pLauncherOptions->TargetName[MAX_PATH-1]=0;
               
                pLauncherOptions->StartingWay = CLauncherOptions::StartingWay_FROM_NAME;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Launcher Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        if (wcsnicmp(lppwargv[cnt],L"TargetCmdLine",wcslen(L"TargetCmdLine"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
                
                SIZE_T CmdLineSize = wcslen(pwc)+1;
                pLauncherOptions->TargetCommandLine = new TCHAR[CmdLineSize];
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(pLauncherOptions->TargetCommandLine,pwc,CmdLineSize-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pLauncherOptions->TargetCommandLine,CmdLineSize);
#endif
                pLauncherOptions->TargetCommandLine[CmdLineSize-1]=0;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Launcher Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }        
        else if (wcsnicmp(lppwargv[cnt],L"ProcessID",wcslen(L"ProcessID"))==0)
        {
            WCHAR* pwc;
            int iScanfRes=0;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
                if(wcsnicmp(pwc,L"0x",2)==0)
                    iScanfRes=swscanf(pwc+2,L"%x",&pLauncherOptions->ProcessId);
                else
                    iScanfRes=swscanf(pwc,L"%u",&pLauncherOptions->ProcessId);
            }
            if ((pwc==NULL)||(iScanfRes==0))
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Launcher Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
            pLauncherOptions->StartingWay=CLauncherOptions::StartingWay_FROM_PROCESS_ID_AND_THREAD_ID_OF_SUSPENDED_PROCESS;
        } 
        else if (wcsnicmp(lppwargv[cnt],L"ThreadId",wcslen(L"ThreadId"))==0)
        {
            WCHAR* pwc;
            int iScanfRes=0;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
                if(wcsnicmp(pwc,L"0x",2)==0)
                    iScanfRes=swscanf(pwc+2,L"%x",&pLauncherOptions->ThreadId);
                else
                    iScanfRes=swscanf(pwc,L"%u",&pLauncherOptions->ThreadId);
            }
            if ((pwc==NULL)||(iScanfRes==0))
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Launcher Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
            pLauncherOptions->StartingWay=CLauncherOptions::StartingWay_FROM_PROCESS_ID_AND_THREAD_ID_OF_SUSPENDED_PROCESS;
        } 
        
        else if (wcsnicmp(lppwargv[cnt],L"Flags",wcslen(L"Flags"))==0)
        {
            WCHAR* pwc;
            int iScanfRes=0;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
                if(wcsnicmp(pwc,L"0x",2)==0)
                    iScanfRes=swscanf(pwc+2,L"%x",&pLauncherOptions->Flags);
                else
                    iScanfRes=swscanf(pwc,L"%u",&pLauncherOptions->Flags);
            }
            if ((pwc==NULL)||(iScanfRes==0))
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Launcher Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }            

        else if (wcsnicmp(lppwargv[cnt],L"ExclusionList",wcslen(L"ExclusionList"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(pLauncherOptions->FilteringTypeFileName,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pLauncherOptions->FilteringTypeFileName,MAX_PATH);
#endif
                pLauncherOptions->FilteringTypeFileName[MAX_PATH-1]=0;
                pLauncherOptions->FilteringType=FilteringType_INCLUDE_ALL_BUT_SPECIFIED;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Launcher Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else if (wcsnicmp(lppwargv[cnt],L"InclusionList",wcslen(L"InclusionList"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(pLauncherOptions->FilteringTypeFileName,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pLauncherOptions->FilteringTypeFileName,MAX_PATH);
#endif
                pLauncherOptions->FilteringTypeFileName[MAX_PATH-1]=0;
                pLauncherOptions->FilteringType=FilteringType_INCLUDE_ONLY_SPECIFIED;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Launcher Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else if (wcsnicmp(lppwargv[cnt],L"OnlyBaseModule",wcslen(L"OnlyBaseModule"))==0)
        {
            pLauncherOptions->FilteringType=FilteringType_ONLY_BASE_MODULE;
        }
        else if (wcsnicmp(lppwargv[cnt],L"EmulatedRegistryFile",wcslen(L"EmulatedRegistryFile"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(pLauncherOptions->EmulatedRegistryConfigFile,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pLauncherOptions->EmulatedRegistryConfigFile,MAX_PATH);
#endif
                pLauncherOptions->EmulatedRegistryConfigFile[MAX_PATH-1]=0;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Launcher Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }

        else
        {
            _snwprintf(pszError,MAX_PATH,L"Unknown command line option %s",lppwargv[cnt]);
            MessageBoxW(NULL,pszError,L"Launcher Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
            bRes=FALSE;
            break;
        }
    }
    LocalFree(lppwargv);        

    return pLauncherOptions->CheckConsistency(FALSE);
}