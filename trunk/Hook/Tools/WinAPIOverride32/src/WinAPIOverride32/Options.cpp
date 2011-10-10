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

//-----------------------------------------------------------------------------
// Object: manages winapioverride options
//-----------------------------------------------------------------------------

#include "options.h"

COptions::COptions(TCHAR* FileName)
{
    this->FileName=_tcsdup(FileName);
    this->CommonConstructor();
}
COptions::COptions()
{
    this->FileName=NULL;
    this->CommonConstructor();
}
void COptions::CommonConstructor()
{
    this->MonitoringFilesList=new CLinkList(MAX_PATH*sizeof(TCHAR));
    this->OverridingDllList=new CLinkList(MAX_PATH*sizeof(TCHAR));
    *this->ExclusionFiltersModulesFileList=0;
    *this->InclusionFiltersModulesFileList=0;
    *this->StartupApplicationPath=0;
    *this->StartupApplicationCmd=0;
    this->ExportOnlySelected=FALSE;
    this->ExportOnlyVisible=FALSE;
    this->ExportFullParametersContent=FALSE;
    this->AutoAnalysis=FIRST_BYTES_AUTO_ANALYSIS_NONE;
    this->bLogCallStack=FALSE;
    this->bMonitoringFileDebugMode=FALSE;
    this->CallStackEbpRetrievalSize=16;
    this->bOnlyBaseModule=FALSE;
    this->StartupApplicationbOnlyAfter=FALSE;
    this->StartupApplicationOnlyAfterMs=100;
    this->StartAllProcessesbOnlyAfter=FALSE;
    this->StartAllProcessesOnlyAfterMs=100;
    this->bFiltersApplyToMonitoring=TRUE;
    this->bFiltersApplyToOverriding=TRUE;
    this->StartWay=START_WAY_PID;
    this->bFilterModulesFileListIsExclusionList=FALSE;
    *this->AllProcessesInclusion=0;
    *this->AllProcessesExclusion=0;
    *this->AllProcessesParentPID=0;
    this->StartupApplicationPID=0;
    this->BreakDialogDontBreakAPIOverrideThreads=FALSE;
    this->bUseFilterModulesFileList=TRUE;
    this->bAllowTlsCallbackHooks=TRUE;
    this->bExeIATHooks=FALSE;

    // com options
    memset(&this->ComOptions,0,sizeof(HOOK_COM_OPTIONS));
    this->bComAutoHookingEnabled=FALSE;
    this->ComOptions.ReportHookedCOMObject=TRUE;
    this->ComOptions.IDispatchAutoMonitoring=TRUE;
    this->ComOptions.QueryMethodToHookForInterfaceParsedByIDispatch=FALSE;
    this->ComOptions.ReportCLSIDNotSupportingIDispatch=TRUE;
    this->ComOptions.ReportIIDHavingNoMonitoringFileAssociated=TRUE;
    this->ComOptions.ReportUseNameInsteadOfIDIfPossible=TRUE;
    this->ComOptions.bUseClsidFilter=FALSE;
    _tcscpy(this->ComOptions.pszConfigFileComObjectCreationHookedFunctions,COM_OBJECT_CREATION_HOOKED_FUNCTIONS_DEFAULT_CONFIG_FILENAME);
    this->ComOptions.CLSIDFilterType=COM_CLSID_FilterDontHookSpecified;
    _tcscpy(this->ComOptions.pszOnlyHookedFileName,COM_OBJECT_HOOKED_CLSID_FILENAME);
    _tcscpy(this->ComOptions.pszNotHookedFileName,COM_OBJECT_NOT_HOOKED_CLSID_FILENAME);

    // net options
    this->bNetProfilingEnabled=FALSE;
    this->bNetAutoHookingEnabled=FALSE;
    memset(&this->NetOptions,0,sizeof(HOOK_NET_OPTIONS));
    this->NetOptions.DisableOptimization=FALSE;
    this->NetOptions.EnableFrameworkMonitoring=FALSE;
    this->NetOptions.MonitorException=TRUE;
    this->StopAndKillAfter=FALSE;
    this->StopAndKillAfterMs=5000;

    int cnt;
	COLUMN_INFOS* pColumnsInfos;
    for (cnt=0;cnt<CApiOverride::NbColumns;cnt++)
    {
        pColumnsInfos = &this->ListviewLogsColumnsInfos[cnt];
        pColumnsInfos->bVisible =TRUE;
        pColumnsInfos->iOrder = cnt;// default index = id
        pColumnsInfos->Size = 0;
    }
}

COptions::~COptions(void)
{
    if (this->FileName)
        free(this->FileName);
    if (this->MonitoringFilesList)
        delete this->MonitoringFilesList;
    if (this->OverridingDllList)
        delete this->OverridingDllList;
}

// FALSE if option file don't exists
BOOL COptions::Load()
{
    if (!this->FileName)
        return FALSE;
    return this->Load(this->FileName);
}

BOOL COptions::Load(TCHAR* OptionsFileName)
{
    BOOL Ret=TRUE;

    // check if file exists
    Ret=CStdFileOperations::DoesFileExists(OptionsFileName);
    // don't return yet in case of constructor / GetPrivateProfileXX default value difference
    // if (!Ret) return FALSE; // constructor set values to 0 so just return a new object
    
// options.cpp is shared between main project and HookNet.dll
// hooknet dll is injected before hooking process start, so hooknet.dll must parse config file for .NET options only
#ifndef HOOKNET_EXPORTS
    TCHAR KeyName[MAX_PATH];
    TCHAR pszValue[MAX_PATH];
    int cnt;
    int NbItems;

    // read columns information
    COLUMN_INFOS* pColumnsInfos;
    for (cnt=0;cnt<CApiOverride::NbColumns;cnt++)
    {
        _itot(cnt,KeyName,10);
        pColumnsInfos = &this->ListviewLogsColumnsInfos[cnt];
        pColumnsInfos->bVisible =(BOOL)::GetPrivateProfileInt(
                                    COPTIONS_SECTION_NAME_VISIBLE_COLUMNS,
                                    KeyName,
                                    TRUE,// default Visible
                                    OptionsFileName
                                    );
        pColumnsInfos->iOrder =::GetPrivateProfileInt(
                                    COPTIONS_SECTION_NAME_COLUMNS_INDEX,
                                    KeyName,
                                    cnt,// default index = id
                                    OptionsFileName
                                    );
        pColumnsInfos->Size =::GetPrivateProfileInt(
                                    COPTIONS_SECTION_NAME_COLUMNS_SIZE,
                                    KeyName,
                                    0,// default
                                    OptionsFileName
                                    );
    }


    // start way
    this->StartWay=(tagStartWay)GetPrivateProfileInt( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                                    COPTIONS_KEY_NAME_START_WAY,
                                                    0,
                                                    OptionsFileName
                                                    );

    // startup pid
    this->StartupApplicationPID=(DWORD)GetPrivateProfileInt(
                                COPTIONS_SECTION_NAME_USER_INTERFACE,
                                COPTIONS_KEY_NAME_STARTUP_APP_PID,
                                0,
                                OptionsFileName
                                );

    // startup app path
    GetPrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_STARTUP_APP_PATH,
                               _T(""),
                               this->StartupApplicationPath,
                               MAX_PATH,
                               OptionsFileName
                               );

    // startup app cmd
    GetPrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_STARTUP_APP_CMD,
                               _T(""),
                               this->StartupApplicationCmd,
                               MAX_PATH,
                               OptionsFileName
                               );

    // startup app only after 
    this->StartupApplicationbOnlyAfter=GetPrivateProfileInt( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                                COPTIONS_KEY_NAME_STARTUP_APP_ONLY_AFTER,
                                                0,
                                                OptionsFileName
                                                );

    // startup app only after ms
    this->StartupApplicationOnlyAfterMs=GetPrivateProfileInt( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                                         COPTIONS_KEY_NAME_STARTUP_APP_ONLY_AFTER_MS,
                                                         100,
                                                         OptionsFileName
                                                         );

    // stop and kill after
    this->StopAndKillAfter=GetPrivateProfileInt( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                                            COPTIONS_KEY_NAME_STOP_AND_KILL,
                                                            0,
                                                            OptionsFileName
                                                            );
    // stop and kill after ms
    this->StopAndKillAfterMs=GetPrivateProfileInt( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                                            COPTIONS_KEY_NAME_STOP_AND_KILL_MS,
                                                            5000,
                                                            OptionsFileName
                                                            );

    // start all only after 
    this->StartAllProcessesbOnlyAfter=GetPrivateProfileInt( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                                            COPTIONS_KEY_NAME_ALL_PROCESSES_ONLY_AFTER,
                                                            0,
                                                            OptionsFileName
                                                            );

    // start all only after ms
    this->StartAllProcessesOnlyAfterMs=GetPrivateProfileInt( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                                            COPTIONS_KEY_NAME_ALL_PROCESSES_ONLY_AFTER_MS,
                                                            100,
                                                            OptionsFileName
                                                            );

    // not logged modules list file
    this->bFilterModulesFileListIsExclusionList=GetPrivateProfileInt( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                                        COPTIONS_KEY_NAME_USE_NOT_LOGGED_MODULES_FILE,
                                                        1,
                                                        OptionsFileName
                                                        );

    this->bUseFilterModulesFileList=GetPrivateProfileInt( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                                        COPTIONS_KEY_NAME_USE_FILTER_MODULES_FILE_LIST,
                                                        1,
                                                        OptionsFileName
                                                        );
    


    GetPrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_NOT_LOGGED_MODULES_FILE,
                               COPTION_OPTION_MODULE_EXCLUSION_FILENAME,
                               this->ExclusionFiltersModulesFileList,
                               MAX_PATH,
                               OptionsFileName
                               );

    GetPrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_LOGGED_ONLY_MODULES_FILE,
                               COPTION_OPTION_MODULE_INCLUSION_FILENAME,
                               this->InclusionFiltersModulesFileList,
                               MAX_PATH,
                               OptionsFileName
                               );

    // log only base module
    this->bOnlyBaseModule=GetPrivateProfileInt( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                                COPTIONS_KEY_NAME_ONLY_BASE_MODULE,
                                                0,
                                                OptionsFileName
                                                );

    // filters apply to monitoring
    this->bFiltersApplyToMonitoring=GetPrivateProfileInt( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                                COPTIONS_KEY_NAME_FILTERS_APPLY_TO_MONITORING_FILES,
                                                1,
                                                OptionsFileName
                                                );
    // filters apply to overriding
    this->bFiltersApplyToOverriding=GetPrivateProfileInt( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                                COPTIONS_KEY_NAME_FILTERS_APPLY_TO_OVERRIDING_DLL,
                                                1,
                                                OptionsFileName
                                                );



    // process filters options
    GetPrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_FILTERS_ALL_PROCESSES_INCLUSION,
                               _T(""),
                               this->AllProcessesInclusion,
                               MAX_PATH,
                               OptionsFileName
                               );
    GetPrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_FILTERS_ALL_PROCESSES_EXCLUSION,
                               _T(""),
                               this->AllProcessesExclusion,
                               MAX_PATH,
                               OptionsFileName
                               );
    GetPrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_FILTERS_ALL_PROCESSES_PARENT_PID,
                               _T(""),
                               this->AllProcessesParentPID,
                               MAX_PATH,
                               OptionsFileName
                               );

    /////////////////////
    // MonitoringFilesList
    /////////////////////

    // get number of items
    NbItems=GetPrivateProfileInt( COPTIONS_SECTION_NAME_MONITORING_FILES,
                                COPTIONS_KEY_NAME_COUNT,
                                0,
                                OptionsFileName
                                );
    // get items
    for (cnt=0;cnt<NbItems;cnt++)
    {
        _itot(cnt,KeyName,10);
        GetPrivateProfileString( COPTIONS_SECTION_NAME_MONITORING_FILES,
                                    KeyName,
                                    _T(""),
                                    pszValue,
                                    MAX_PATH,
                                    OptionsFileName
                                    );

        this->MonitoringFilesList->AddItem(pszValue);
    }

    /////////////////////
    // OverridingDllList
    /////////////////////

    // get number of items
    NbItems=GetPrivateProfileInt( COPTIONS_SECTION_NAME_OVERRIDING_DLL,
                                COPTIONS_KEY_NAME_COUNT,
                                0,
                                OptionsFileName
                                );
    // get items
    for (cnt=0;cnt<NbItems;cnt++)
    {
        _itot(cnt,KeyName,10);
        GetPrivateProfileString( COPTIONS_SECTION_NAME_OVERRIDING_DLL,
                                    KeyName,
                                    _T(""),
                                    pszValue,
                                    MAX_PATH,
                                    OptionsFileName
                                    );

        this->OverridingDllList->AddItem(pszValue);
    }

 

    // export only selected
    this->ExportOnlySelected=(BOOL)GetPrivateProfileInt(
                                    COPTIONS_SECTION_NAME_USER_INTERFACE,
                                    COPTIONS_KEY_NAME_EXPORT_ONLY_SELECTED,
                                    FALSE,
                                    OptionsFileName
                                    );

    // export only visible
    this->ExportOnlyVisible=(BOOL)GetPrivateProfileInt(
                                    COPTIONS_SECTION_NAME_USER_INTERFACE,
                                    COPTIONS_KEY_NAME_EXPORT_ONLY_VISIBLE,
                                    FALSE,
                                    OptionsFileName
                                    );

    // export full parameter content
    this->ExportFullParametersContent=(BOOL)GetPrivateProfileInt(
                                        COPTIONS_SECTION_NAME_USER_INTERFACE,
                                        COPTIONS_KEY_NAME_EXPORT_FULL_PARAMETERS_CONTENT,
                                        FALSE,
                                        OptionsFileName
                                        );

    /////////////////////
    // options
    /////////////////////
    this->AutoAnalysis=(tagFirstBytesAutoAnalysis)GetPrivateProfileInt(
                            COPTIONS_SECTION_NAME_OPTIONS,
                            COPTIONS_KEY_NAME_AUTOANALYSIS,
                            FIRST_BYTES_AUTO_ANALYSIS_NONE,
                            OptionsFileName
                            );
    this->bLogCallStack=(BOOL)GetPrivateProfileInt(
                                COPTIONS_SECTION_NAME_OPTIONS,
                                COPTIONS_KEY_NAME_LOG_CALL_STACK,
                                FALSE,
                                OptionsFileName
                                );
    this->CallStackEbpRetrievalSize=(DWORD)GetPrivateProfileInt(
                                            COPTIONS_SECTION_NAME_OPTIONS,
                                            COPTIONS_KEY_NAME_CALL_STACK_PARAMETERS_SIZE,
                                            16,
                                            OptionsFileName
                                            );
    this->bMonitoringFileDebugMode=(BOOL)GetPrivateProfileInt(
                                            COPTIONS_SECTION_NAME_OPTIONS,
                                            COPTIONS_KEY_NAME_MONITORING_FILE_DEBUG_MODE,
                                            FALSE,
                                            OptionsFileName
                                            );

    this->BreakDialogDontBreakAPIOverrideThreads=(BOOL)GetPrivateProfileInt( 
                                                    COPTIONS_SECTION_NAME_OPTIONS,
                                                    COPTIONS_KEY_NAME_BREAK_DONT_BREAK_APIOVERRIDE_THREADS,
                                                    TRUE,
                                                    OptionsFileName
                                                    );

    this->bAllowTlsCallbackHooks=(BOOL)GetPrivateProfileInt( 
                                                    COPTIONS_SECTION_NAME_OPTIONS,
                                                    COPTIONS_KEY_NAME_ALLOW_TLS_CALLBACK_HOOKS,
                                                    TRUE,
                                                    OptionsFileName
                                                    );
    this->bExeIATHooks=(BOOL)GetPrivateProfileInt( 
                                                    COPTIONS_SECTION_NAME_OPTIONS,
                                                    COPTIONS_KEY_NAME_EXE_IAT_HOOKING,
                                                    FALSE,
                                                    OptionsFileName
                                                    );

    ///////////////
    // com options
    ///////////////
    this->bComAutoHookingEnabled=(BOOL)GetPrivateProfileInt( 
                                                    COPTIONS_SECTION_NAME_COM_OPTIONS,
                                                    COPTIONS_KEY_NAME_COM_AUTO_HOOKING_ENABLED,
                                                    FALSE,
                                                    OptionsFileName
                                                    );

    this->ComOptions.ReportHookedCOMObject=(BOOL)GetPrivateProfileInt( 
                                                    COPTIONS_SECTION_NAME_COM_OPTIONS,
                                                    COPTIONS_KEY_NAME_REPORT_HOOKED_COM_OBJECT,
                                                    TRUE,
                                                    OptionsFileName
                                                    );
    this->ComOptions.IDispatchAutoMonitoring=(BOOL)GetPrivateProfileInt( 
                                                    COPTIONS_SECTION_NAME_COM_OPTIONS,
                                                    COPTIONS_KEY_NAME_IDISPATCH_AUTO_MONITORING,
                                                    TRUE,
                                                    OptionsFileName
                                                    );
    this->ComOptions.QueryMethodToHookForInterfaceParsedByIDispatch=(BOOL)GetPrivateProfileInt( 
                                                    COPTIONS_SECTION_NAME_COM_OPTIONS,
                                                    COPTIONS_KEY_NAME_QUERY_METHOD_TO_HOOK,
                                                    FALSE,
                                                    OptionsFileName
                                                    );
    this->ComOptions.ReportCLSIDNotSupportingIDispatch=(BOOL)GetPrivateProfileInt( 
                                                    COPTIONS_SECTION_NAME_COM_OPTIONS,
                                                    COPTIONS_KEY_NAME_REPORT_CLSID_NOT_SUPPORTING_IDISPATCH,
                                                    TRUE,
                                                    OptionsFileName
                                                    );
    this->ComOptions.ReportIIDHavingNoMonitoringFileAssociated=(BOOL)GetPrivateProfileInt( 
                                                    COPTIONS_SECTION_NAME_COM_OPTIONS,
                                                    COPTIONS_KEY_NAME_REPORT_IID_HAVING_NO_MONITORING_FILE_ASSOCIATED,
                                                    TRUE,
                                                    OptionsFileName
                                                    );
    this->ComOptions.ReportUseNameInsteadOfIDIfPossible=(BOOL)GetPrivateProfileInt( 
                                                    COPTIONS_SECTION_NAME_COM_OPTIONS,
                                                    COPTIONS_KEY_NAME_REPORT_USE_NAME_INSTEAD_ID,
                                                    TRUE,
                                                    OptionsFileName
                                                    );

    this->ComOptions.CLSIDFilterType=(tagCOMCLSIDFilterTypes)GetPrivateProfileInt( 
                                                    COPTIONS_SECTION_NAME_COM_OPTIONS,
                                                    COPTIONS_KEY_NAME_CLSID_FILTER_TYPE,
                                                    COM_CLSID_FilterDontHookSpecified,
                                                    OptionsFileName
                                                    );

    this->ComOptions.bUseClsidFilter=(BOOL)GetPrivateProfileInt( 
                                            COPTIONS_SECTION_NAME_COM_OPTIONS,
                                            COPTIONS_KEY_NAME_USE_CLSID_FILTER,
                                            FALSE,
                                            OptionsFileName
                                            );

    GetPrivateProfileString( COPTIONS_SECTION_NAME_COM_OPTIONS,
                            COPTIONS_KEY_NAME_CONFIG_FILE_COM_OBJECT_CREATION,
                            COM_OBJECT_CREATION_HOOKED_FUNCTIONS_DEFAULT_CONFIG_FILENAME,
                            this->ComOptions.pszConfigFileComObjectCreationHookedFunctions,
                            MAX_PATH,
                            OptionsFileName
                            );

   
    GetPrivateProfileString( COPTIONS_SECTION_NAME_COM_OPTIONS,
                            COPTIONS_KEY_NAME_HOOKED_CLSID_FILENAME,
                            COM_OBJECT_HOOKED_CLSID_FILENAME,
                            this->ComOptions.pszOnlyHookedFileName,
                            MAX_PATH,
                            OptionsFileName
                            );

    GetPrivateProfileString( COPTIONS_SECTION_NAME_COM_OPTIONS,
                            COPTIONS_KEY_NAME_NOT_HOOKED_CLSID_FILENAME,
                            COM_OBJECT_NOT_HOOKED_CLSID_FILENAME,
                            this->ComOptions.pszNotHookedFileName,
                            MAX_PATH,
                            OptionsFileName
                            );
#endif
    // NET options
    this->bNetProfilingEnabled=(BOOL)GetPrivateProfileInt( 
                                COPTIONS_SECTION_NAME_NET_OPTIONS,
                                COPTIONS_KEY_NAME_NET_PROFILING_ENABLED,
                                FALSE,
                                OptionsFileName
                                );

    this->bNetAutoHookingEnabled=(BOOL)GetPrivateProfileInt( 
                                COPTIONS_SECTION_NAME_NET_OPTIONS,
                                COPTIONS_KEY_NAME_NET_AUTO_HOOKING_ENABLED,
                                FALSE,
                                OptionsFileName
                                );
    this->NetOptions.DisableOptimization=(BOOL)GetPrivateProfileInt( 
                                        COPTIONS_SECTION_NAME_NET_OPTIONS,
                                        COPTIONS_KEY_NAME_NET_DISABLE_OPTIMIZATION,
                                        FALSE,
                                        OptionsFileName
                                        );
    this->NetOptions.MonitorException=(BOOL)GetPrivateProfileInt( 
                                        COPTIONS_SECTION_NAME_NET_OPTIONS,
                                        COPTIONS_KEY_NAME_NET_MONITOR_EXCEPTIONS,
                                        TRUE,
                                        OptionsFileName
                                        );


    this->NetOptions.EnableFrameworkMonitoring=(BOOL)GetPrivateProfileInt( 
                                        COPTIONS_SECTION_NAME_NET_OPTIONS,
                                        COPTIONS_KEY_NAME_NET_ENABLE_FRAMEWORK_MONITORING,
                                        FALSE,
                                        OptionsFileName
                                        );
    return Ret;

}
BOOL COptions::Save()
{
    if (!this->FileName)
        return FALSE;
    return this->Save(this->FileName);
}
BOOL COptions::Save(TCHAR* OptionsFileName)
{
    TCHAR pszValue[MAX_PATH];

// options.cpp is shared between main project and HookNet.dll
// hooknet dll is injected before hooking process start, so hooknet.dll must parse config file for .NET options only
#ifndef HOOKNET_EXPORTS
    TCHAR KeyName[MAX_PATH];
    int cnt;
    CLinkListItem* pItem;

    // write columns informations
    COLUMN_INFOS* pColumnsInfos;
    for (cnt=0;cnt<CApiOverride::NbColumns;cnt++)
    {
        _itot(cnt,KeyName,10);
        pColumnsInfos = &this->ListviewLogsColumnsInfos[cnt];

        _itot(pColumnsInfos->bVisible,pszValue,10);
        WritePrivateProfileString(
                                    COPTIONS_SECTION_NAME_VISIBLE_COLUMNS,
                                    KeyName,
                                    pszValue,
                                    OptionsFileName
                                    );

        _itot(pColumnsInfos->iOrder,pszValue,10);
        WritePrivateProfileString(
                                    COPTIONS_SECTION_NAME_COLUMNS_INDEX,
                                    KeyName,
                                    pszValue,
                                    OptionsFileName
                                    );
        _itot(pColumnsInfos->Size,pszValue,10);
        WritePrivateProfileString(
                                    COPTIONS_SECTION_NAME_COLUMNS_SIZE,
                                    KeyName,
                                    pszValue,
                                    OptionsFileName
                                    );
    }

    // start way
    _itot(this->StartWay,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_START_WAY,
                               pszValue,
                               OptionsFileName
                               );

    // startup pid
    _itot(this->StartupApplicationPID,pszValue,10);
    WritePrivateProfileString(
                            COPTIONS_SECTION_NAME_USER_INTERFACE,
                            COPTIONS_KEY_NAME_STARTUP_APP_PID,
                            pszValue,
                            OptionsFileName
                            );

    // startup app path
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_STARTUP_APP_PATH,
                               this->StartupApplicationPath,
                               OptionsFileName
                               );

    // startup app cmd
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_STARTUP_APP_CMD,
                               this->StartupApplicationCmd,
                               OptionsFileName
                               );


    // startup app only after
    _itot(this->StartupApplicationbOnlyAfter,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_STARTUP_APP_ONLY_AFTER,
                               pszValue,
                               OptionsFileName
                               );

    // startup app only after ms
    _itot(this->StartupApplicationOnlyAfterMs,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_STARTUP_APP_ONLY_AFTER_MS,
                               pszValue,
                               OptionsFileName
                               );

    // stop and kill after
    _itot(this->StopAndKillAfter,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_STOP_AND_KILL,
                               pszValue,
                               OptionsFileName
                               );

    // stop and kill after ms
    _itot(this->StopAndKillAfterMs,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_STOP_AND_KILL_MS,
                               pszValue,
                               OptionsFileName
                               );

    // start all only after 
    _itot(this->StartAllProcessesbOnlyAfter,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_ALL_PROCESSES_ONLY_AFTER,
                               pszValue,
                               OptionsFileName
                               );
    // start all only after ms
    _itot(this->StartAllProcessesOnlyAfterMs,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_ALL_PROCESSES_ONLY_AFTER_MS,
                               pszValue,
                               OptionsFileName
                               );


    // use not logged modules list
    _itot(this->bFilterModulesFileListIsExclusionList,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                            COPTIONS_KEY_NAME_USE_NOT_LOGGED_MODULES_FILE,
                            pszValue,
                            OptionsFileName
                            );

    _itot(this->bUseFilterModulesFileList,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                            COPTIONS_KEY_NAME_USE_FILTER_MODULES_FILE_LIST,
                            pszValue,
                            OptionsFileName
                            );

    // not logged modules list file
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_NOT_LOGGED_MODULES_FILE,
                               this->ExclusionFiltersModulesFileList,
                               OptionsFileName
                               );

    // logged only modules list file
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                               COPTIONS_KEY_NAME_LOGGED_ONLY_MODULES_FILE,
                               this->InclusionFiltersModulesFileList,
                               OptionsFileName
                               );

    // log only base module
    _itot(this->bOnlyBaseModule,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                COPTIONS_KEY_NAME_ONLY_BASE_MODULE,
                                pszValue,
                                OptionsFileName
                                );

    // filters apply to monitoring
    _itot(this->bFiltersApplyToMonitoring,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                COPTIONS_KEY_NAME_FILTERS_APPLY_TO_MONITORING_FILES,
                                pszValue,
                                OptionsFileName
                                );

    // filters apply to overriding
    _itot(this->bFiltersApplyToOverriding,pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                COPTIONS_KEY_NAME_FILTERS_APPLY_TO_OVERRIDING_DLL,
                                pszValue,
                                OptionsFileName
                                );


    // processes filters options
    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                COPTIONS_KEY_NAME_FILTERS_ALL_PROCESSES_INCLUSION,
                                this->AllProcessesInclusion,
                                OptionsFileName
                                );

    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                COPTIONS_KEY_NAME_FILTERS_ALL_PROCESSES_EXCLUSION,
                                this->AllProcessesExclusion,
                                OptionsFileName
                                );

    WritePrivateProfileString( COPTIONS_SECTION_NAME_USER_INTERFACE,
                                COPTIONS_KEY_NAME_FILTERS_ALL_PROCESSES_PARENT_PID,
                                this->AllProcessesParentPID,
                                OptionsFileName
                                );

 
    /////////////////////
    // MonitoringFilesList
    /////////////////////

    // write number of items
    _itot(this->MonitoringFilesList->GetItemsCount(),pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_MONITORING_FILES,
                                COPTIONS_KEY_NAME_COUNT,
                                pszValue,
                                OptionsFileName
                                );
    // write items
    
    cnt=0;
    this->MonitoringFilesList->Lock();
    for(pItem=this->MonitoringFilesList->Head;pItem;pItem=pItem->NextItem)
    {
        _itot(cnt,KeyName,10);
        WritePrivateProfileString( COPTIONS_SECTION_NAME_MONITORING_FILES,
                                    KeyName,
                                    (TCHAR*)pItem->ItemData,
                                    OptionsFileName
                                    );
        cnt++;
    }
    this->MonitoringFilesList->Unlock();

    /////////////////////
    // OverridingDllList
    /////////////////////

    // write number of items
    _itot(this->OverridingDllList->GetItemsCount(),pszValue,10);
    WritePrivateProfileString( COPTIONS_SECTION_NAME_OVERRIDING_DLL,
                                COPTIONS_KEY_NAME_COUNT,
                                pszValue,
                                OptionsFileName
                                );
    // write items
    
    cnt=0;
    this->OverridingDllList->Lock();
    for(pItem=this->OverridingDllList->Head;pItem;pItem=pItem->NextItem)
    {
        _itot(cnt,KeyName,10);
        WritePrivateProfileString( COPTIONS_SECTION_NAME_OVERRIDING_DLL,
                                    KeyName,
                                    (TCHAR*)pItem->ItemData,
                                    OptionsFileName
                                    );
        
        cnt++;
    }
    this->OverridingDllList->Unlock();


    // export only selected
    _itot(this->ExportOnlySelected,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_USER_INTERFACE,
                                COPTIONS_KEY_NAME_EXPORT_ONLY_SELECTED,
                                pszValue,
                                OptionsFileName
                                );

    // export only visible
    _itot(this->ExportOnlyVisible,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_USER_INTERFACE,
                                COPTIONS_KEY_NAME_EXPORT_ONLY_VISIBLE,
                                pszValue,
                                OptionsFileName
                                );

    // export full parameter content
    _itot(this->ExportFullParametersContent,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_USER_INTERFACE,
                                COPTIONS_KEY_NAME_EXPORT_FULL_PARAMETERS_CONTENT,
                                pszValue,
                                OptionsFileName
                                );

    /////////////////////
    // options
    /////////////////////
    _itot(this->AutoAnalysis,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_OPTIONS,
                                COPTIONS_KEY_NAME_AUTOANALYSIS,
                                pszValue,
                                OptionsFileName
                                );

    _itot(this->bLogCallStack,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_OPTIONS,
                                COPTIONS_KEY_NAME_LOG_CALL_STACK,
                                pszValue,
                                OptionsFileName
                                );

    _itot(this->CallStackEbpRetrievalSize,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_OPTIONS,
                                COPTIONS_KEY_NAME_CALL_STACK_PARAMETERS_SIZE,
                                pszValue,
                                OptionsFileName
                                );

    _itot(this->bMonitoringFileDebugMode,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_OPTIONS,
                                COPTIONS_KEY_NAME_MONITORING_FILE_DEBUG_MODE,
                                pszValue,
                                OptionsFileName
                                );

    _itot(this->BreakDialogDontBreakAPIOverrideThreads,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_OPTIONS,
                                COPTIONS_KEY_NAME_BREAK_DONT_BREAK_APIOVERRIDE_THREADS,
                                pszValue,
                                OptionsFileName
                                );

    _itot(this->bAllowTlsCallbackHooks,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_OPTIONS,
                                COPTIONS_KEY_NAME_ALLOW_TLS_CALLBACK_HOOKS,
                                pszValue,
                                OptionsFileName
                                );

    _itot(this->bExeIATHooks,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_OPTIONS,
                                COPTIONS_KEY_NAME_EXE_IAT_HOOKING,
                                pszValue,
                                OptionsFileName
                                );
 
    /////////////////////
    // com
    /////////////////////
    _itot(this->bComAutoHookingEnabled,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_COM_OPTIONS,
                                COPTIONS_KEY_NAME_COM_AUTO_HOOKING_ENABLED,
                                pszValue,
                                OptionsFileName
                                );


    _itot(this->ComOptions.ReportHookedCOMObject,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_COM_OPTIONS,
                                COPTIONS_KEY_NAME_REPORT_HOOKED_COM_OBJECT,
                                pszValue,
                                OptionsFileName
                                );
    _itot(this->ComOptions.IDispatchAutoMonitoring,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_COM_OPTIONS,
                                COPTIONS_KEY_NAME_IDISPATCH_AUTO_MONITORING,
                                pszValue,
                                OptionsFileName
                                );

    _itot(this->ComOptions.QueryMethodToHookForInterfaceParsedByIDispatch,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_COM_OPTIONS,
                                COPTIONS_KEY_NAME_QUERY_METHOD_TO_HOOK,
                                pszValue,
                                OptionsFileName
                                );
    _itot(this->ComOptions.ReportCLSIDNotSupportingIDispatch,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_COM_OPTIONS,
                                COPTIONS_KEY_NAME_REPORT_CLSID_NOT_SUPPORTING_IDISPATCH,
                                pszValue,
                                OptionsFileName
                                );

    _itot(this->ComOptions.ReportIIDHavingNoMonitoringFileAssociated,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_COM_OPTIONS,
                                COPTIONS_KEY_NAME_REPORT_IID_HAVING_NO_MONITORING_FILE_ASSOCIATED,
                                pszValue,
                                OptionsFileName
                                );

    _itot(this->ComOptions.ReportUseNameInsteadOfIDIfPossible,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_COM_OPTIONS,
                                COPTIONS_KEY_NAME_REPORT_USE_NAME_INSTEAD_ID,
                                pszValue,
                                OptionsFileName
                                );

    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_COM_OPTIONS,
                                COPTIONS_KEY_NAME_CONFIG_FILE_COM_OBJECT_CREATION,
                                this->ComOptions.pszConfigFileComObjectCreationHookedFunctions,
                                OptionsFileName
                                );

    _itot(this->ComOptions.CLSIDFilterType,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_COM_OPTIONS,
                                COPTIONS_KEY_NAME_CLSID_FILTER_TYPE,
                                pszValue,
                                OptionsFileName
                                );

    _itot(this->ComOptions.bUseClsidFilter,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_COM_OPTIONS,
                                COPTIONS_KEY_NAME_USE_CLSID_FILTER,
                                pszValue,
                                OptionsFileName
                                );

    WritePrivateProfileString( COPTIONS_SECTION_NAME_COM_OPTIONS,
                            COPTIONS_KEY_NAME_HOOKED_CLSID_FILENAME,
                            this->ComOptions.pszOnlyHookedFileName,
                            OptionsFileName
                            );

    WritePrivateProfileString( COPTIONS_SECTION_NAME_COM_OPTIONS,
                            COPTIONS_KEY_NAME_NOT_HOOKED_CLSID_FILENAME,
                            this->ComOptions.pszNotHookedFileName,
                            OptionsFileName
                            );
#endif
    /////////////
    // NET
    /////////////
    _itot(this->bNetProfilingEnabled,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_NET_OPTIONS,
                                COPTIONS_KEY_NAME_NET_PROFILING_ENABLED,
                                pszValue,
                                OptionsFileName
                                );

    _itot(this->bNetAutoHookingEnabled,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_NET_OPTIONS,
                                COPTIONS_KEY_NAME_NET_AUTO_HOOKING_ENABLED,
                                pszValue,
                                OptionsFileName
                                );

    _itot(this->NetOptions.DisableOptimization,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_NET_OPTIONS,
                                COPTIONS_KEY_NAME_NET_DISABLE_OPTIMIZATION,
                                pszValue,
                                OptionsFileName
                                );

    _itot(this->NetOptions.MonitorException,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_NET_OPTIONS,
                                COPTIONS_KEY_NAME_NET_MONITOR_EXCEPTIONS,
                                pszValue,
                                OptionsFileName
                                );


    _itot(this->NetOptions.EnableFrameworkMonitoring,pszValue,10);
    WritePrivateProfileString(
                                COPTIONS_SECTION_NAME_NET_OPTIONS,
                                COPTIONS_KEY_NAME_NET_ENABLE_FRAMEWORK_MONITORING,
                                pszValue,
                                OptionsFileName
                                );
    return TRUE;
}