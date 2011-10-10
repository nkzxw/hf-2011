#include "CommandLineParsing.h"
extern BOOL bCommandLine;
extern BOOL bNoGUI;
extern COptions* pOptions;
extern CFilters* pFilters;
extern DWORD CommandLineMonitoringFilesArraySize;
extern TCHAR** CommandLineMonitoringFilesArray;
extern DWORD CommandLineOverridingDllArraySize;
extern TCHAR** CommandLineOverridingDllArray;
extern TCHAR* CommandLineNoGuiSavingFileName;

//-----------------------------------------------------------------------------
// Name: ParseCommandLine
// Object: parse cmd line
// Parameters :
//     in  : 
//     out :
//     return : TRUE on SUCCESS, FALSE on error
//-----------------------------------------------------------------------------
BOOL ParseCommandLine()
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
    if (argc==0)
    {
        LocalFree(lppwargv);
        return TRUE;
    }
    // See Options.h for default options
    pOptions->bUseFilterModulesFileList=FALSE;

    // we are in command line mode
    bCommandLine=TRUE;

    // for each param
    for(cnt=1;cnt<argc;cnt++)// cnt[0] is app name
    {
        if (wcsicmp(lppwargv[cnt],L"NoGUI")==0)
        {
            bNoGUI=TRUE;
        }
        else if (wcsicmp(lppwargv[cnt],L"All")==0)
        {
            pOptions->StartWay=COptions::START_WAY_ALL_PROCESSES;
        }
        else if (wcsicmp(lppwargv[cnt],L"SecureLenDAsm")==0)
        {
            pOptions->AutoAnalysis=FIRST_BYTES_AUTO_ANALYSIS_SECURE;
        }
        else if (wcsicmp(lppwargv[cnt],L"InsecureLenDAsm")==0)
        {
            pOptions->AutoAnalysis=FIRST_BYTES_AUTO_ANALYSIS_INSECURE;
        }
        else if (wcsicmp(lppwargv[cnt],L"OnlyBaseModule")==0)
        {
            pOptions->bOnlyBaseModule=TRUE;
        }
        else if (wcsicmp(lppwargv[cnt],L"GetCallStack")==0)
        {
            pOptions->bLogCallStack=TRUE;
        }
        else if (wcsicmp(lppwargv[cnt],L"DontBreakAPIOverrideThreads")==0)
        {
            pOptions->bMonitoringFileDebugMode=TRUE;
        }
        else if (wcsicmp(lppwargv[cnt],L"MonitoringFileDebug")==0)
        {
            pOptions->bMonitoringFileDebugMode=TRUE;
        }
        else if (wcsnicmp(lppwargv[cnt],L"CallStackNbParams",wcslen(L"CallStackNbParams"))==0)
        {
            WCHAR* pwc;
            int iScanfRes=0;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
                if(wcsnicmp(pwc,L"0x",2)==0)
                    iScanfRes=swscanf(pwc+2,L"%x",&pOptions->CallStackEbpRetrievalSize);
                else
                    iScanfRes=swscanf(pwc,L"%u",&pOptions->CallStackEbpRetrievalSize);
            }
            if ((pwc==NULL)||(iScanfRes==0))
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
            // translate from nb param to nb bytes size
            pOptions->CallStackEbpRetrievalSize*=sizeof(PBYTE);
        }
        else if (wcsnicmp(lppwargv[cnt],L"InjectOnlyAfter",wcslen(L"InjectOnlyAfter"))==0)
        {
            WCHAR* pwc;
            int iScanfRes=0;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
                if(wcsnicmp(pwc,L"0x",2)==0)
                    iScanfRes=swscanf(pwc+2,L"%x",&pOptions->StartupApplicationOnlyAfterMs);
                else
                    iScanfRes=swscanf(pwc,L"%u",&pOptions->StartupApplicationOnlyAfterMs);
            }
            if ((pwc==NULL)||(iScanfRes==0))
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
            // fill info for both startup application and start all processes has we may don't know the starting way (according to arg order)
            pOptions->StartupApplicationbOnlyAfter=TRUE;
            pOptions->StartAllProcessesbOnlyAfter=TRUE;
            pOptions->StartAllProcessesOnlyAfterMs=pOptions->StartupApplicationOnlyAfterMs;
        }
        else if (wcsnicmp(lppwargv[cnt],L"StopAndKillAfter",wcslen(L"StopAndKillAfter"))==0)
        {
            WCHAR* pwc;
            int iScanfRes=0;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
                if(wcsnicmp(pwc,L"0x",2)==0)
                    iScanfRes=swscanf(pwc+2,L"%x",&pOptions->StopAndKillAfterMs);
                else
                    iScanfRes=swscanf(pwc,L"%u",&pOptions->StopAndKillAfterMs);
            }
            if ((pwc==NULL)||(iScanfRes==0))
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
            // fill flag info
            pOptions->StopAndKillAfter=TRUE;
        }
        else if (wcsnicmp(lppwargv[cnt],L"AppPid",wcslen(L"AppPid"))==0)
        {
            WCHAR* pwc;
            int iScanfRes=0;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
                if(wcsnicmp(pwc,L"0x",2)==0)
                    iScanfRes=swscanf(pwc+2,L"%x",&pOptions->StartupApplicationPID);
                else
                    iScanfRes=swscanf(pwc,L"%u",&pOptions->StartupApplicationPID);
                pOptions->StartWay=COptions::START_WAY_PID;
            }
            if ((pwc==NULL)||(iScanfRes==0))
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
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
                wcsncpy(pOptions->ExclusionFiltersModulesFileList,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pOptions->ExclusionFiltersModulesFileList,MAX_PATH);
#endif
                pOptions->ExclusionFiltersModulesFileList[MAX_PATH-1]=0;
                pOptions->bUseFilterModulesFileList=TRUE;
                pOptions->bFilterModulesFileListIsExclusionList=TRUE;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
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
                wcsncpy(pOptions->InclusionFiltersModulesFileList,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pOptions->InclusionFiltersModulesFileList,MAX_PATH);
#endif
                pOptions->InclusionFiltersModulesFileList[MAX_PATH-1]=0;
                pOptions->bUseFilterModulesFileList=TRUE;
                pOptions->bFilterModulesFileListIsExclusionList=FALSE;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else if (wcsnicmp(lppwargv[cnt],L"AppPath",wcslen(L"AppPath"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(pOptions->StartupApplicationPath,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pOptions->StartupApplicationPath,MAX_PATH);
#endif
                pOptions->StartupApplicationPath[MAX_PATH-1]=0;
                pOptions->StartWay=COptions::START_WAY_LAUNCH_APPLICATION;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else if (wcsnicmp(lppwargv[cnt],L"AppCmdLine",wcslen(L"AppCmdLine"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(pOptions->StartupApplicationCmd,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pOptions->StartupApplicationCmd,MAX_PATH);
#endif
                pOptions->StartupApplicationCmd[MAX_PATH-1]=0;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else if (wcsnicmp(lppwargv[cnt],L"MonitoringFiles",wcslen(L"MonitoringFiles"))==0)
        {
            DWORD Size=(DWORD)wcslen(lppwargv[cnt])+1;
            TCHAR* psz=new TCHAR[Size];
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(psz,pwc,Size-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,psz,Size);
#endif
                psz[Size-1]=0;

                // split arg at ";" into array
                CommandLineMonitoringFilesArray=CMultipleElementsParsing::ParseString(psz,&CommandLineMonitoringFilesArraySize);
            }
            delete[] psz;
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else if (wcsnicmp(lppwargv[cnt],L"OverridingDlls",wcslen(L"OverridingDlls"))==0)
        {
            DWORD Size=(DWORD)wcslen(lppwargv[cnt])+1;
            TCHAR* psz=new TCHAR[Size];
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(psz,pwc,Size-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,psz,Size);
#endif
                psz[Size-1]=0;

                // split arg at ";" into array
                CommandLineOverridingDllArray=CMultipleElementsParsing::ParseString(psz,&CommandLineOverridingDllArraySize);
            }
            delete[] psz;
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else if (wcsnicmp(lppwargv[cnt],L"AllProcessesInclusion",wcslen(L"AllProcessesInclusion"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(pOptions->AllProcessesInclusion,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pOptions->AllProcessesInclusion,MAX_PATH);
#endif
                pOptions->AllProcessesInclusion[MAX_PATH-1]=0;

            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else if (wcsnicmp(lppwargv[cnt],L"AllProcessesExclusion",wcslen(L"AllProcessesExclusion"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(pOptions->AllProcessesExclusion,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pOptions->AllProcessesExclusion,MAX_PATH);
#endif
                pOptions->AllProcessesExclusion[MAX_PATH-1]=0;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else if (wcsnicmp(lppwargv[cnt],L"AllProcessesParentPID",wcslen(L"AllProcessesParentPID"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(pOptions->AllProcessesParentPID,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pOptions->AllProcessesParentPID,MAX_PATH);
#endif
                pOptions->AllProcessesParentPID[MAX_PATH-1]=0;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else if (wcsnicmp(lppwargv[cnt],L"AllProcessesPID",wcslen(L"AllProcessesPID"))==0)
        {
            DWORD Size=(DWORD)wcslen(lppwargv[cnt])+1;
            TCHAR* psz=new TCHAR[Size];
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(psz,pwc,Size-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,psz,Size);
#endif
                psz[Size-1]=0;

                pFilters->pdwFiltersCurrentPorcessID=CMultipleElementsParsing::ParseDWORD(psz,&pFilters->pdwFiltersCurrentPorcessIDSize);
            }
            delete[] psz;
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else if (wcsicmp(lppwargv[cnt],L"HookBeforeDllExecution")==0)
        {
            pOptions->bExeIATHooks=TRUE;
        }
        else if (wcsicmp(lppwargv[cnt],L"COMAutoHook")==0)
        {
            pOptions->bComAutoHookingEnabled=TRUE;
        }
        else if (wcsicmp(lppwargv[cnt],L"COMIDispatchAutoMonitoring")==0)
        {
            pOptions->ComOptions.IDispatchAutoMonitoring=TRUE;
        }
        else if (wcsicmp(lppwargv[cnt],L"COMIDispatchQueryMethodToHook")==0)
        {
            pOptions->ComOptions.QueryMethodToHookForInterfaceParsedByIDispatch=TRUE;
        }
        else if (wcsicmp(lppwargv[cnt],L"COMReportObjectsLife")==0)
        {
            pOptions->ComOptions.ReportHookedCOMObject=TRUE;
        }
        else if (wcsicmp(lppwargv[cnt],L"COMUseName")==0)
        {
            pOptions->ComOptions.ReportUseNameInsteadOfIDIfPossible=TRUE;
        }
        else if (wcsicmp(lppwargv[cnt],L"COMReportMonitoringFileLack")==0)
        {
            pOptions->ComOptions.ReportIIDHavingNoMonitoringFileAssociated=TRUE;
        }
        else if (wcsnicmp(lppwargv[cnt],L"COMObjectCreationFile",wcslen(L"COMObjectCreationFile"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(pOptions->ComOptions.pszConfigFileComObjectCreationHookedFunctions,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pOptions->ComOptions.pszConfigFileComObjectCreationHookedFunctions,MAX_PATH);
#endif
                pOptions->ComOptions.pszConfigFileComObjectCreationHookedFunctions[MAX_PATH-1]=0;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else if (wcsnicmp(lppwargv[cnt],L"COMNotHookedClsid",wcslen(L"COMNotHookedClsid"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(pOptions->ComOptions.pszNotHookedFileName,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pOptions->ComOptions.pszNotHookedFileName,MAX_PATH);
#endif
                pOptions->ComOptions.pszNotHookedFileName[MAX_PATH-1]=0;
                pOptions->ComOptions.bUseClsidFilter=TRUE;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }
        else if (wcsnicmp(lppwargv[cnt],L"COMOnlyHookedClsid",wcslen(L"COMOnlyHookedClsid"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(pOptions->ComOptions.pszOnlyHookedFileName,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,pOptions->ComOptions.pszOnlyHookedFileName,MAX_PATH);
#endif
                pOptions->ComOptions.pszOnlyHookedFileName[MAX_PATH-1]=0;
                pOptions->ComOptions.bUseClsidFilter=TRUE;
            }
            if (pwc==NULL)
            {
                _snwprintf(pszError,MAX_PATH,L"Bad command line option %s",lppwargv[cnt]);
                MessageBoxW(NULL,pszError,L"Error",MB_OK|MB_ICONERROR|MB_TOPMOST);
                bRes=FALSE;
                break;
            }
        }

        else if (wcsnicmp(lppwargv[cnt],L"SavingFileName",wcslen(L"SavingFileName"))==0)
        {
            WCHAR* pwc;
            pwc=wcschr(lppwargv[cnt],'=');
            if (pwc)
            {
                if (CommandLineNoGuiSavingFileName)
                    delete[] CommandLineNoGuiSavingFileName;

                CommandLineNoGuiSavingFileName = new TCHAR[MAX_PATH];

                // point after "="
                pwc++;
#if (defined(UNICODE)||defined(_UNICODE))
                wcsncpy(CommandLineNoGuiSavingFileName,pwc,MAX_PATH-1);
#else
                CAnsiUnicodeConvert::UnicodeToAnsi(pwc,CommandLineNoGuiSavingFileName,MAX_PATH);
#endif
                pOptions->ComOptions.pszConfigFileComObjectCreationHookedFunctions[MAX_PATH-1]=0;
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

    if ((pOptions->StartWay==COptions::START_WAY_PID)&&(pOptions->StartupApplicationPID==0))
    {
        MessageBox(NULL,_T("No application specified in command line"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        bRes=FALSE;
    }

    // assume bNoGUI is set
    if (CommandLineNoGuiSavingFileName && (!bNoGUI))
    {
        delete[] CommandLineNoGuiSavingFileName;
        CommandLineNoGuiSavingFileName = NULL;
    }

    return bRes;
}