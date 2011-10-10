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
// Object: class helper for doing api override or api monitoring
//         it manages the apioverride.dll
//         (It's the hart of project winapioverride)
//-----------------------------------------------------------------------------


#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <windows.h>
#include <stdio.h>

#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "IApiOverride.h"
#include "SupportedParameters.h"
#include "../InjLib/injlib.h"
#include "ApiOverrideFuncAndParams.h"
#include "../MailSlot/MailSlotClient.h"
#include "../MailSlot/MailSlotServer.h"
#include "../../GUI/ListView/ListView.h"
#include "../../FIFO/FIFO.h"
#include "../../APIError/ApiError.h"
#include "../../CleanCloseHandle/CleanCloseHandle.h"
#include "../ProcessHelper/ProcessHelper.h"
#include "../memory/processmemory.h"
#include "../../PE/PE.h"
#include "../../String/trimstring.h"
#include "../../File/TextFile.h"
#include "../../File/StdFileOperations.h"
#include "../SetEnvVarProc/SetEnvironmentVariableToProcess.h"

#include <shlobj.h>

// IAT import const
#define CApiOverride_ADDIMPORTS_SECTION_NAME _T(".winapi") // any non standard name can be used
#define CApiOverride_IAT_LOADER_DLL_NAME     _T("IATLoader.dll")// name of dll statically linked with target
#define CApiOverride_IAT_LOADER_TMP_FILE_EXTENSION _T(".wao")// use to create tmp file name. Any name can be used
#ifndef _countof
    #define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

// timeouts in ms
#define TIME_REQUIERED_TO_LOAD 15000 // max time in sec required for dll/monitoring file to be loaded
#define TIME_REQUIERED_TO_UNLOAD 10000 // max time in sec required for dll/monitoring file to be unloaded
#define HOOK_END_POOLING_IN_MS 10
#define MAX_POOLING_TIME_IN_MS 20000

#define APIOVERRIDE_NO_MORE_MESSAGE_IF_NO_MESSAGE_DURING_TIME_IN_MS 500 // time with no message before closing mailslot server in case of unexpected unload
#define APIOVERRIDE_CMD_REPLY_MAX_TIME_IN_MS 4000 // max time in ms for a command to get is associated reply
#define APIOVERRIDE_MAX_ONE_PARAM_STRING_SIZE_FOR_CALL_COLUMN 64

#define CApiOverride_SUSPENDED_HOOK_END_FLAG 0xBADCAFE

// for FILETIME conversion
#define _SECOND ((ULONGLONG) 10000000)
#define _MINUTE (60 * _SECOND)
#define _HOUR   (60 * _MINUTE)
#define _DAY    (24 * _HOUR) 

// First listview column text for report messages
#define LISTVIEW_ITEM_TEXT_INFORMATION  _T("I")
#define LISTVIEW_ITEM_TEXT_ERROR        _T("E")
#define LISTVIEW_ITEM_TEXT_WARNING      _T("W")

#define INCREMENT_STRING _T("-")


class CApiOverride:public IApiOverride
{
protected:
    BOOL bDestructorCalled;
    CRITICAL_SECTION CriticalSection;// critical section is required to avoid object to be deleted while displaying a messagebox in another thread
    BOOL bAPIOverrideDllLoaded;
    tagCallBackLogFunc pCallBackLogFunc;
    BOOL bManualFreeLogEntry;
    LPVOID pCallBackLogFuncUserParam;
    tagCallBackUnexpectedUnload pCallBackUnexpectedUnloadFunc;
    LPVOID pCallBackUnexpectedUnloadFuncUserParam;
    tagCallBackReportMessages pCallBackReportMessage;
    LPVOID pCallBackReportMessagesUserParam;
    tagpCallBackOverridingDllQuery pCallBackOverridingDllQuery;
    LPVOID pCallBackOverridingDllQueryUserParam;
    DWORD dwCurrentProcessId;// hooked process id
    TCHAR ProcessFullPathName[MAX_PATH];// hooked process full path name
    TCHAR ProcessName[MAX_PATH];// hooked process name
    TCHAR ProcessPath[MAX_PATH];// hooked process Path
    CMailSlotServer* pMailSlotServer;
    CMailSlotClient* pMailSlotClient;
    CListview* pListview;
    CListview* pInternalListview;
    InjectLib pInjectLib;// InjectLib function pointer
    EjectLib pEjectLib; // EjectLib function pointer 
    HMODULE hmodInjlib; // handle of injlib dll
    HANDLE hThreadWatchingEvents;// handle of watching event thread
    HANDLE hThreadLogging;// handle of logging thread
    TCHAR pszAppPath[MAX_PATH];// winapioverride application path
    PBYTE NotLoggedModulesArray;
    HANDLE hevtGetNotLoggedModulesReply;
    CLinkListSimple* pCurrentRemoteCalls;
    HOOK_COM_OPTIONS ComHookingOptions;
    BOOL bComAutoHookingEnabled;
    HOOK_NET_OPTIONS NetHookingOptions;
    BOOL bNetAutoHookingEnabled;
    HANDLE hStopUnlocked; // lock for Stop function
    HANDLE hDestructorUnlocked; // lock for destructor function

    // events from api override class to injected dll
    HANDLE hevtStartMonitoring;// query to start monitoring
    HANDLE hevtStopMonitoring;// query to stop monitoring
    HANDLE hevtStartFaking;// query to start overriding
    HANDLE hevtStopFaking;// query to stop overriding
    HANDLE hevtFreeProcess;// query to free process
    HANDLE hevtTlsHookEndLoop;// query tls hook to stop watching for monitoring file/overriding dll loading
    
    // events from injected dll to api override class
    HANDLE hevtAPIOverrideDllProcessAttachCompleted; // api override dll is successfully loaded in targeted process
    HANDLE hevtAPIOverrideDllProcessDetachCompleted; // api override dll has been unloaded from targeted process
    HANDLE hevtProcessFree;// api override dll associated memory and process has been free. 
                           // hevtAPIOverrideDllProcessDetachCompleted can follow, but hevtProcessFree
                           // can be raised at dll loading too in case of error
    HANDLE hevtMonitoringFileLoaded; // monitoring file successfully loaded
    HANDLE hevtMonitoringFileUnloaded; // monitoring file successfully unloaded
    HANDLE hevtFakeAPIDLLLoaded;// fake api dll successfully loaded
    HANDLE hevtFakeAPIDLLUnloaded;// fake api dll successfully unloaded
    HANDLE hevtError;// an error has occurred
    HANDLE hevtClientMailslotOpen; // client is ready to receive commands from mailslot

    PBYTE HookEntryPointRemoteHook;
    PBYTE HookEntryPointRemoteLibName;
    CProcessMemory* HookEntryPointpProcessMemory;
    
    DWORD NotLoggedModulesArraySize;

    tagFirstBytesAutoAnalysis AutoAnalysis;
    BOOL bLogCallStack;
    BOOL bMonitoringFileDebugMode;
    DWORD CallStackEbpRetrievalSize;
    BOOL bOnlyBaseModule;
    BOOL bBreakDialogDontBreakApioverrideThreads;
    BOOL bAllowTlsCallBackHook;
    HANDLE MonitoringHeap;
    HANDLE hEvtMonitoringLogHeapUnlocked;
    HWND hParentWindow;

    void Initialize();
    void MailSlotServerCallback(PVOID pData,DWORD dwDataSize);
    void __fastcall MonitoringCallback(PBYTE LogBuffer);
    void __fastcall MonitoringCallback(LOG_ENTRY* pLog);
    void ShowApiOverrideNotStartedMsg();
    void DllUnloadedCallBack();
    void SetInitialOptions();
    int  UserMessage(TCHAR* pszMessage,TCHAR* pszTitle,UINT uType);
    int  UserMessage(HWND hWnd,TCHAR* pszMessage,TCHAR* pszTitle,UINT uType);
    void ReportError(TCHAR* pszErrorMessage);
    void ReportError(HWND hWnd, TCHAR* pszErrorMessage);

    BOOL InitializeStart(DWORD dwPID);
    BOOL FillPorcessNameFromPID(DWORD dwPID);
    BOOL InjectDllByCreateRemoteThread(DWORD dwPID);
    BOOL WaitForInjectedDllToBeLoaded();
    void ResetInjectedDllLoadEvents();
    BOOL HookEntryPoint(TCHAR* pszFileName, DWORD dwProcessId,HANDLE hProcessHandle,HANDLE hThreadHandle,BOOL* pTlsHook);
    BOOL HookEntryPointFree();
    BOOL ResumeThreadForStartSuspended(HANDLE hThread,HANDLE hProcess);
    BOOL CallHookedProcessCallBackForStartSuspended(HANDLE hThread,tagpCallBackBeforeAppResume pCallBackFunc,PVOID pUserParam);
    BOOL CallHookedProcessCallBack(tagpCallBackBeforeAppResume pCallBackFunc,PVOID pUserParam);

    static BOOL AddModuleListParseLineStatic(TCHAR* FileName,TCHAR* pszLine,DWORD dwLineNumber,LPVOID UserParam);
    static BOOL RemoveModuleListParseLineStatic(TCHAR* FileName,TCHAR* pszLine,DWORD dwLineNumber,LPVOID UserParam);
    void FilterModuleListParseLine(TCHAR* FileName,TCHAR* pszLine,DWORD dwLineNumber,BOOL ShouldBeLogged);

    static void StaticMailSlotServerCallback(PVOID pData,DWORD dwDataSize,PVOID pUserData);
    static DWORD WINAPI LoggingThreadListener(LPVOID lpParam);
    static DWORD WINAPI DllUnloadedThreadListener(LPVOID lpParam);

    typedef struct tagStartWayIATMailSlotCallbackParam
    {
        CApiOverride* pApiOverride;
        tagpCallBackBeforeAppResume pCallBackFunc;
        PVOID pCallBackFuncUserParam;
        HANDLE hEventReady;
        HANDLE hEventEndOfInitialization;
        HANDLE hEventRemoteMailslotActive;
        BOOL bResult;
    }StartWayIATMailSlotCallback_PARAM;
    static void StartWayIATMailSlotCallback(PVOID pData,DWORD dwDataSize,PVOID pUserData);

    typedef struct tagDeleteIATInjectedFileThreadParam
    {
        TCHAR* FileName;
        HANDLE hProcess;
        HANDLE hEventEndOfInitialization;
    }DeleteIATInjectedFileThread_PARAM;
    static DWORD WINAPI DeleteIATInjectedFileThread(LPVOID lpParameter);

    BOOL StartAtProcessCreation(TCHAR* pszFileName,DWORD ProcessId,HANDLE hProcess,DWORD ThreadId,HANDLE hThread,BOOL bLetThreadSuspended,tagpCallBackBeforeAppResume pCallBackFunc,PVOID pUserParam);
    BOOL Start(DWORD dwPID,HANDLE hSuspendedThread);
    BOOL StartWithoutEnteringCriticalSection(DWORD dwPID,HANDLE hSuspendedThread);
    BOOL StopWithoutEnteringCriticalSection();
    BOOL StopWithoutEnteringCriticalSection(BOOL bCalledByhThreadWatchingEvents);
    BOOL OpenClientMailSlot();

    BOOL LoadInjectLibrary();
    BOOL FreeInjectLibrary();
public:
    CApiOverride();
    CApiOverride(tagCallBackLogFunc pCallBackLogFunc);
    CApiOverride(HWND hParentWindow,HWND hListView);
    CApiOverride(HWND hParentWindow);
    virtual ~CApiOverride(void);

    virtual BOOL STDMETHODCALLTYPE SetParentWindow(HWND hParentWindow);

    virtual BOOL STDMETHODCALLTYPE LoadMonitoringFile(TCHAR* pszFileName);
    virtual BOOL STDMETHODCALLTYPE UnloadMonitoringFile(TCHAR* pszFileName);
    virtual BOOL STDMETHODCALLTYPE LoadFakeAPI(TCHAR* pszFileName);
    virtual BOOL STDMETHODCALLTYPE UnloadFakeAPI(TCHAR* pszFileName);

    virtual BOOL STDMETHODCALLTYPE StartMonitoring();
    virtual BOOL STDMETHODCALLTYPE StopMonitoring();
    virtual BOOL STDMETHODCALLTYPE StartFaking();
    virtual BOOL STDMETHODCALLTYPE StopFaking();

    virtual BOOL STDMETHODCALLTYPE LogOnlyBaseModule(BOOL bOnlyBaseModule);
    virtual BOOL STDMETHODCALLTYPE SetModuleFilteringWay(tagFilteringWay FilteringWay);
    virtual BOOL STDMETHODCALLTYPE SetModuleLogState(TCHAR* pszModuleFullPath,BOOL bLog);
    virtual BOOL STDMETHODCALLTYPE AddToFiltersModuleList(TCHAR* pszFileName);
    virtual BOOL STDMETHODCALLTYPE RemoveFromFiltersModuleList(TCHAR* pszFileName);
    virtual BOOL STDMETHODCALLTYPE ClearFiltersModuleList();
    virtual BOOL STDMETHODCALLTYPE GetNotLoggedModuleList(TCHAR*** pArrayNotLoggedModulesNames,DWORD* pdwArrayNotLoggedModulesNamesSize);
    virtual BOOL STDMETHODCALLTYPE SetMonitoringModuleFiltersState(BOOL bEnable);
    virtual BOOL STDMETHODCALLTYPE SetFakingModuleFiltersState(BOOL bEnable);
    virtual BOOL STDMETHODCALLTYPE SetAutoAnalysis(tagFirstBytesAutoAnalysis AutoAnalysis);
    virtual BOOL STDMETHODCALLTYPE AllowTlsCallbackHooking(BOOL bAllow);
    virtual BOOL STDMETHODCALLTYPE EnableCOMAutoHooking(BOOL bEnable,BOOL ShowTryToUnhookIfNeeded);
    virtual BOOL STDMETHODCALLTYPE EnableCOMAutoHooking(BOOL bEnable);
    virtual BOOL STDMETHODCALLTYPE SetCOMOptions(HOOK_COM_OPTIONS* pComOptions);
    virtual BOOL STDMETHODCALLTYPE ShowCOMInteractionDialog();
    virtual BOOL STDMETHODCALLTYPE EnableNETProfiling(BOOL bEnable);// must be called to really enable NetHooking
    static BOOL EnableNETProfilingStatic(BOOL bEnable);// must be called to really enable NetHooking
    virtual BOOL STDMETHODCALLTYPE EnableNetAutoHooking(BOOL bEnable);// used only to signal user events (in case an application was ".Net profiled")
                                            // as cor profiling state can't be changed after application startup
    virtual BOOL STDMETHODCALLTYPE EnableNetAutoHooking(BOOL bEnable,BOOL ShowTryToUnhookIfNeeded);// used only to signal user events (in case an application was ".Net profiled")
                                                                         // as cor profiling state can't be changed after application startup
    virtual BOOL STDMETHODCALLTYPE SetNetOptions(HOOK_NET_OPTIONS* pNetOptions);
	virtual BOOL STDMETHODCALLTYPE AddHookNetFromTokenForCompiledFuntions(TCHAR* AssemblyName,SIZE_T NbToken,ULONG32* TokenArray);
	virtual BOOL STDMETHODCALLTYPE RemoveHookNetFromTokenForCompiledFuntion(TCHAR* AssemblyName,SIZE_T NbToken,ULONG32* TokenArray);
    virtual BOOL STDMETHODCALLTYPE ShowNetInteractionDialog();
    virtual BOOL STDMETHODCALLTYPE SetCallStackRetrieval(BOOL bLogCallStack,DWORD CallStackParametersRetrievalSize);
    virtual BOOL STDMETHODCALLTYPE BreakDialogDontBreakApioverrideThreads(BOOL bDontBreak);
    virtual BOOL STDMETHODCALLTYPE SetMonitoringFileDebugMode(BOOL bActiveMode);

    virtual BOOL STDMETHODCALLTYPE Dump();

    virtual BOOL STDMETHODCALLTYPE SetReportMessagesCallBack(tagCallBackReportMessages pCallBackFunc,LPVOID pUserParam);
    virtual BOOL STDMETHODCALLTYPE SetUnexpectedUnloadCallBack(tagCallBackUnexpectedUnload pCallBackFunc,LPVOID pUserParam);
    virtual BOOL STDMETHODCALLTYPE SetMonitoringCallback(tagCallBackLogFunc pCallBackLogFunc,LPVOID pUserParam,BOOL bManualFreeLogEntry);
    static void FreeLogEntryStatic(LOG_ENTRY* pLog);
    static void FreeLogEntryStatic(LOG_ENTRY* pLog,HANDLE Heap);
    virtual void STDMETHODCALLTYPE FreeLogEntry(LOG_ENTRY* pLog);
    virtual void STDMETHODCALLTYPE FreeLogEntry(LOG_ENTRY* pLog,HANDLE Heap);
    virtual BOOL STDMETHODCALLTYPE SetMonitoringListview(HWND hListView);
    virtual BOOL STDMETHODCALLTYPE SetMonitoringListview(CListview* pListView);
    virtual BOOL STDMETHODCALLTYPE InitializeMonitoringListview();

    virtual BOOL STDMETHODCALLTYPE AddLogEntry(LOG_LIST_ENTRY* pLogEntry,BOOL bStorePointerInListViewItemUserData);
    virtual BOOL STDMETHODCALLTYPE AddLogEntry(LOG_LIST_ENTRY* pLogEntry,BOOL bStorePointerInListViewItemUserData,int Increment);

    virtual BOOL STDMETHODCALLTYPE Stop();
    virtual BOOL STDMETHODCALLTYPE Start(DWORD dwPID);
    virtual BOOL STDMETHODCALLTYPE Start(TCHAR* pszFileName);
    virtual BOOL STDMETHODCALLTYPE Start(TCHAR* pszFileName,tagpCallBackBeforeAppResume pCallBackFunc,LPVOID pUserParam);
    virtual BOOL STDMETHODCALLTYPE Start(TCHAR* pszFileName,TCHAR* pszCmdLine,tagpCallBackBeforeAppResume pCallBackFunc,LPVOID pUserParam);
    virtual BOOL STDMETHODCALLTYPE Start(TCHAR* pszFileName,TCHAR* pszCmdLine,tagpCallBackBeforeAppResume pCallBackFunc,LPVOID pUserParam,StartWays StartMethod,DWORD dwResumeTimeAtStartup);
    virtual BOOL STDMETHODCALLTYPE Start(TCHAR* pszFileName,TCHAR* pszCmdLine,tagpCallBackBeforeAppResume pCallBackFunc,LPVOID pUserParam,StartWays StartMethod,DWORD dwResumeTimeAtStartup,OUT STARTUPINFO* pStartupInfo,OUT PROCESS_INFORMATION* pProcessInformation);
    virtual BOOL STDMETHODCALLTYPE StartAtProcessCreation(DWORD ProcessId,DWORD ThreadId,BOOL bLetThreadSuspended,tagpCallBackBeforeAppResume pCallBackFunc,PVOID pUserParam);

    virtual BOOL STDMETHODCALLTYPE ProcessInternalCall(LPTSTR LibName,LPTSTR FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,PBYTE* pReturnValue);
    virtual BOOL STDMETHODCALLTYPE ProcessInternalCall(LPTSTR LibName,LPTSTR FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,PBYTE* pReturnValue,DWORD dwTimeOutMs);
    virtual BOOL STDMETHODCALLTYPE ProcessInternalCall(LPTSTR LibName,LPTSTR FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pReturnValue,DWORD dwTimeOutMs);
    virtual BOOL STDMETHODCALLTYPE ProcessInternalCall(LPTSTR LibName,LPTSTR FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pReturnValue,double* FloatingReturn,DWORD dwTimeOutMs,DWORD ThreadId);
    virtual BOOL STDMETHODCALLTYPE ProcessInternalCall(LPTSTR LibName,LPTSTR FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pReturnValue,double* FloatingReturn,DWORD dwTimeOutMs,DWORD ThreadId,tagCALLING_CONVENTION CallingConvention);

    virtual DWORD STDMETHODCALLTYPE GetProcessID();
    virtual BOOL  STDMETHODCALLTYPE GetProcessName(TCHAR* ProcessName,int ProcessNameMaxSize);
    virtual BOOL  STDMETHODCALLTYPE GetProcessFullPathName(TCHAR* ProcessFullPathName,int ProcessFullPathNameMaxSize);

    virtual BOOL STDMETHODCALLTYPE SetMonitoringLogHeap(HANDLE Heap);

    virtual BOOL STDMETHODCALLTYPE ClearUserDataTypeCache();

    virtual BOOL STDMETHODCALLTYPE SetOverridingDllQueryCallBack(tagpCallBackOverridingDllQuery pCallBackFunc,LPVOID pUserParam);
    virtual BOOL STDMETHODCALLTYPE SendReplyToOverridingDllQuery(HANDLE MessageId,PBYTE pMsg,SIZE_T MsgSize);

    virtual BOOL STDMETHODCALLTYPE WaitAndLockMonitoringLogHeap();
    virtual BOOL STDMETHODCALLTYPE UnlockMonitoringLogHeap();
};