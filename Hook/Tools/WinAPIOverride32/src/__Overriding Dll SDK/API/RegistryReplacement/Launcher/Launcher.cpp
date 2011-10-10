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

// include winsock2.h or #define _WINSOCKAPI_ before including windows.h
#include <winsock2.h>
#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

// This is a modified version of the example provided for using winapioveride libraries 
// notice : WinApiOverride.lib is included only with winapioveride source code and is located in folder WinAPIOverride32\_lib
//          source code is available at http://jacquelin.potier.free.fr/winapioverride32/

// ! REQUIRE FULL WINAPIOVERRIDE SOURCES TO COMPILE !
// sources are available at http://jacquelin.potier.free.fr/winapioverride32/
#include "../../../../../Tools/Process/APIOverride/ApiOverride.h"
#include "../../../../../Tools/File/StdFileOperations.h"
#include "../../../../../Tools/Ini/IniFile.h"


#include "LauncherOptions.h"
#include "LauncherConfigGUI.h"
#include "CommandLineParsing.h"
#include "../RegistryReplacementInjectedDll/RegReplaceOptions.h"

void STDMETHODCALLTYPE CallBackBeforeAppResume(DWORD dwProcessID,PVOID pUserParam);
void STDMETHODCALLTYPE CallbackMonitoringLog(LOG_ENTRY* pLog,PVOID pUserParam);
void STDMETHODCALLTYPE CallBackUnexpectedUnload(DWORD dwProcessID,PVOID pUserParam);

#define REGISTRY_REPLACEMENT_DLL_NAME _T("RegistryReplacement.dll")
#define LAUNCHER_EVENT_INJECTION_FINISHED _T("Global\\InjectFinished")

#define REGISTRY_REPLACEMENT_SET_CONFIGURATIONA _T("_SetConfigurationA@4")
#define REGISTRY_REPLACEMENT_SET_CONFIGURATIONW _T("_SetConfigurationW@4")
#if (defined(UNICODE)||defined(_UNICODE))
    #define REGISTRY_REPLACEMENT_SET_CONFIGURATION REGISTRY_REPLACEMENT_SET_CONFIGURATIONW
#else
    #define REGISTRY_REPLACEMENT_SET_CONFIGURATION REGISTRY_REPLACEMENT_SET_CONFIGURATIONA
#endif

HWND hWndListView=NULL;
HANDLE hevtUnexpectedUnload=NULL;

BOOL ApplyOptionsAndLoadOverridingDll(CApiOverride* pApiOverride,CLauncherOptions* pLauncherOptions,BOOL bSpyModeOnly)
{
    // change some options
    // set the hooking way 
    // for multi threading security (avoid to miss a single call)
    // must be FIRST_BYTES_AUTO_ANALYSIS_SECURE or FIRST_BYTES_AUTO_ANALYSIS_INSECURE
    pApiOverride->SetAutoAnalysis(FIRST_BYTES_AUTO_ANALYSIS_INSECURE); 

    // set filters
    
    // enable filters for monitoring
    // not needed in this example
    pApiOverride->SetMonitoringModuleFiltersState(FALSE); 

    // enable filters for overriding
    pApiOverride->SetFakingModuleFiltersState(TRUE); 
    TCHAR FilteringNameAbsolutePath[MAX_PATH];
    CStdFileOperations::GetAbsolutePath(pLauncherOptions->FilteringTypeFileName,FilteringNameAbsolutePath);
    
    switch(pLauncherOptions->FilteringType)
    {
        case FilteringType_ONLY_BASE_MODULE:
            pApiOverride->LogOnlyBaseModule(TRUE); // log only the .exe not other modules
            break;
        case FilteringType_INCLUDE_ONLY_SPECIFIED:
            pApiOverride->LogOnlyBaseModule(FALSE); // log all module not only the .exe
            pApiOverride->SetModuleFilteringWay(FILTERING_WAY_ONLY_SPECIFIED_MODULES); // log only specified modules
            pApiOverride->AddToFiltersModuleList(FilteringNameAbsolutePath); // set filters by file containing multiple definitions
            // not needed in this example
            // pApiOverride->SetModuleLogState(_T("<TargetDir>\\*"),TRUE); // set module logging state directly
            // pApiOverride->SetModuleLogState(_T("c:\\SomePath\\Somedll.dll"),TRUE); // set module logging state directly            
            break;
        case FilteringType_INCLUDE_ALL_BUT_SPECIFIED:
            pApiOverride->LogOnlyBaseModule(FALSE); // log all module not only the .exe
            pApiOverride->SetModuleFilteringWay(FILTERING_WAY_NOT_SPECIFIED_MODULES); // log only specified modules
            pApiOverride->AddToFiltersModuleList(FilteringNameAbsolutePath); // set filters by file containing multiple definitions
            break;            
    }    
    
// assume base dll won't be redirected (they are loaded before apioverride.dll)
pApiOverride->SetModuleLogState(_T("<system>\\kernel32.dll"),FALSE);
pApiOverride->SetModuleLogState(_T("<system>\\kernelbase.dll"),FALSE);
pApiOverride->SetModuleLogState(_T("<system>\\ntdll.dll"),FALSE);


    // load an overriding dll (overriding or pre/post hook or COM object creation monitoring)
    TCHAR LauncherPath[MAX_PATH];
    TCHAR InjectedDllName[MAX_PATH];
    
    // get current path
    CStdFileOperations::GetAppPath(LauncherPath,MAX_PATH);
    
    // forge dll name
    _tcscpy(InjectedDllName,LauncherPath);
    _tcscat(InjectedDllName,REGISTRY_REPLACEMENT_DLL_NAME);

    // load registry replacement
    pApiOverride->LoadFakeAPI(InjectedDllName);
    
    // in the target process, the overriding dll must load registry configuration file
    // --> do registry loading by using pApiOverride remote call capabilities
    // provide informations required to translate current options to created sub processes
   
    REGISTRY_REPLACEMENT_OPTIONS Options;
    Options.Version = REGISTRY_REPLACEMENT_OPTIONS_VERSION;
    Options.Flags =pLauncherOptions->Flags;
    Options.FilteringType = pLauncherOptions->FilteringType;

    
    if (bSpyModeOnly)
    {
        CStdFileOperations::GetAbsolutePath(_T("GenericRegistryForSpying.xml"),Options.EmulatedRegistryConfigFileAbsolutePath);
        CStdFileOperations::GetAbsolutePath(pLauncherOptions->EmulatedRegistryConfigFile,Options.OutputFileWhenSpyModeEnabledAbsolutePath);
    }
    else
    {
        CStdFileOperations::GetAbsolutePath(pLauncherOptions->EmulatedRegistryConfigFile,Options.EmulatedRegistryConfigFileAbsolutePath);
        CStdFileOperations::GetAbsolutePath(pLauncherOptions->EmulatedRegistryConfigFile,Options.OutputFileWhenSpyModeEnabledAbsolutePath);
    }

    CStdFileOperations::GetAbsolutePath(pLauncherOptions->FilteringTypeFileName,Options.FilteringTypeFileAbsolutePath);

    // call SetConfiguration function of registry replacement dll
    PBYTE RetValue;
    STRUCT_FUNC_PARAM Param;
    Param.bPassAsRef = TRUE;    
    Param.dwDataSize = sizeof (REGISTRY_REPLACEMENT_OPTIONS);
    Param.pData = (PBYTE)&Options;

    // do the call in the remote process
    if (!pApiOverride->ProcessInternalCall(REGISTRY_REPLACEMENT_DLL_NAME,REGISTRY_REPLACEMENT_SET_CONFIGURATION,1,&Param,&RetValue,20000))
        return FALSE;

    // return REGISTRY_REPLACEMENT_SET_CONFIGURATION function returned value
    return (BOOL)RetValue;
}

typedef struct tagApplicationResumeInfos
{ 
    CLauncherOptions* pLauncherOptions;
    CApiOverride* pApiOverride;
    BOOL bSpyModeOnly;
}APPLICATION_RESUME_INFOS;

int LaunchApplicationWithEmulation( CLauncherOptions* pLauncherOptions,BOOL bSpyModeOnly);

int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

#ifdef _DEBUG
    // debug : must be present to attach a debugger at startup when command line
    ::MessageBox(0,_T("Starting --> Ready to be attached for debug"),_T("Launcher"),MB_ICONINFORMATION);
#endif

    CLauncherOptions LauncherOptions;
    if (!ParseCommandLine(&LauncherOptions))
    {
        // show launcher config dialog
        if (CLauncherConfigGUI::Show(hInstance,&LauncherOptions))
        {
            TCHAR DefaultConfigFile[MAX_PATH];
            CStdFileOperations::GetAppName(DefaultConfigFile,MAX_PATH);
            CStdFileOperations::ChangeFileExt(DefaultConfigFile,_T("ini"));
            LauncherOptions.Save(DefaultConfigFile);
        }
        else
        {
            return -1;
        }
    }
    // at this point pLauncherOptions MUST have been configured correctly

    // launch process with registry emulation
    int RetValue = LaunchApplicationWithEmulation(&LauncherOptions,FALSE);

    return RetValue;
}

int LaunchApplicationWithEmulation( CLauncherOptions* pLauncherOptions,BOOL bSpyModeOnly)
{
    STARTUPINFO StartupInfo={0};
    PROCESS_INFORMATION ProcessInfo={0};
    
    CApiOverride* pApiOverride;
    pApiOverride=new CApiOverride(hWndListView); // hWndListView : handle to the listview that will receive logs,
                                                 // put to NULL if you don't use a listview

    // you can optionally set a monitoring log call back --> no use for this example
    // pApiOverride->SetMonitoringCallback(CallbackMonitoringLog, // call back
    //                                     pApiOverride,          // call back parameter
    //                                     FALSE);                //we don't want to keep log in memory 
    //                                                            // after callback has been called

    // set an unload callback to now when the hooked process ends
    pApiOverride->SetUnexpectedUnloadCallBack(CallBackUnexpectedUnload, // call back
                                              pApiOverride              // call back parameter
                                             );

    // create event for unexpected unload
    hevtUnexpectedUnload=::CreateEvent(NULL,FALSE,FALSE,NULL);

    TCHAR TargetNameAbsolutePath[MAX_PATH];
    CStdFileOperations::GetAbsolutePath(pLauncherOptions->TargetName,TargetNameAbsolutePath);
    HANDLE hProcess = NULL;
    switch (pLauncherOptions->StartingWay)
    {
        case CLauncherOptions::StartingWay_FROM_NAME:
            {
                APPLICATION_RESUME_INFOS ApplicationResumeInfos;
                ApplicationResumeInfos.pLauncherOptions = pLauncherOptions;
                ApplicationResumeInfos.pApiOverride = pApiOverride;
                ApplicationResumeInfos.bSpyModeOnly = bSpyModeOnly;

                // start using name
                if (!pApiOverride->Start(TargetNameAbsolutePath,        // app path
                                        pLauncherOptions->TargetCommandLine,// app command line
                                        CallBackBeforeAppResume,        // call back before resuming application : trap to inject 
                                                                        // monitoring files and faking dll before the application start.
                                                                        // application will be resumed at the end of the callback
                                        &ApplicationResumeInfos,        // callback parameter
                                        CApiOverride::StartWaySuspended,// start with suspended app
                                        0,                              // useless parameter for StartWaySuspended start
                                        &StartupInfo,                   // optional allow to get process infos
                                        &ProcessInfo                    // optional allow to get process infos
                                        ))
                {
                    delete pApiOverride;
                    ::CloseHandle(hevtUnexpectedUnload);
                    if (ProcessInfo.hProcess)
                        ::CloseHandle(ProcessInfo.hProcess);
                    if (ProcessInfo.hThread)
                        ::CloseHandle(ProcessInfo.hThread);
                    return -1;
                }

                if (ProcessInfo.hThread)
                    ::CloseHandle(ProcessInfo.hThread);
                
                hProcess = ProcessInfo.hProcess;
            }
            break;
    
    case CLauncherOptions::StartingWay_FROM_PROCESS_ID_AND_THREAD_ID_OF_SUSPENDED_PROCESS:
        {
            BOOL bSuccess;
            hProcess = ::OpenProcess(PROCESS_ALL_ACCESS,FALSE,pLauncherOptions->ProcessId);
            bSuccess = pApiOverride->StartAtProcessCreation(pLauncherOptions->ProcessId,pLauncherOptions->ThreadId,TRUE,NULL,NULL);
            if (bSuccess)
                ApplyOptionsAndLoadOverridingDll(pApiOverride,pLauncherOptions,bSpyModeOnly);
                
            // set event even in case of error
            TCHAR EventName[MAX_PATH];
            _sntprintf(EventName,MAX_PATH,_T("%s%u"),LAUNCHER_EVENT_INJECTION_FINISHED, pLauncherOptions->ProcessId);
            HANDLE hEvent = ::OpenEvent(EVENT_MODIFY_STATE,FALSE,EventName);
            if (hEvent)
            {
                ::SetEvent(hEvent); // to go out of hooked CreateProcess function + resume created process (if necessary) inside caller injected dll
                ::CloseHandle(hEvent);
            }
        
            if (!bSuccess)
            {
                delete pApiOverride;
                ::CloseHandle(hevtUnexpectedUnload);
                if (hProcess)
                    ::CloseHandle(hProcess);
                return -1;
            }
        }
        break;
    }

    SIZE_T NbObjectToWait = 1;
    HANDLE WaitingObjectHandles[2];
    WaitingObjectHandles[0] = hevtUnexpectedUnload;
    if (hProcess)
    {
        NbObjectToWait++;
        WaitingObjectHandles[1] = hProcess; // better if process crash without reporting hevtUnexpectedUnload event
    }

    // wait for the end of launched application
    ::WaitForMultipleObjects(NbObjectToWait,WaitingObjectHandles,FALSE,INFINITE);

    ::CloseHandle(hevtUnexpectedUnload);
    if (hProcess)
        ::CloseHandle(hProcess);

    // free memory
    delete pApiOverride;
    
    return 0;
}

void STDMETHODCALLTYPE CallBackBeforeAppResume(DWORD dwProcessID,PVOID pUserParam)
{

    APPLICATION_RESUME_INFOS* pApplicationResumeInfos=(APPLICATION_RESUME_INFOS*)pUserParam;

    ApplyOptionsAndLoadOverridingDll(pApplicationResumeInfos->pApiOverride,pApplicationResumeInfos->pLauncherOptions,pApplicationResumeInfos->bSpyModeOnly);
    // load a monitoring file
    // pApiOverride->LoadMonitoringFile(_T("c:\\winapioverride\\monitoring files\\InputTextDataRetrival.txt"));
}

void STDMETHODCALLTYPE CallbackMonitoringLog(LOG_ENTRY* pLog,PVOID pUserParam)
{
    // get object on which event applies
    CApiOverride* pApiOverride=(CApiOverride*)pUserParam;

    // do whatever you want with the log (write some fields to file,....)
}

void STDMETHODCALLTYPE CallBackUnexpectedUnload(DWORD dwProcessID,PVOID pUserParam)
{
    // get object on which event applies
    CApiOverride* pApiOverride=(CApiOverride*)pUserParam;

    // do some of your freeing memory actions

    // signal end of process hooking
    if (hevtUnexpectedUnload)
        ::SetEvent(hevtUnexpectedUnload);
}