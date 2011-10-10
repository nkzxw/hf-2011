/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software=0; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation=0; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY=0; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program=0; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: Apioverride object interface
//-----------------------------------------------------------------------------

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <windows.h>

#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "interprocesscommunication.h"
#include "HookCom/HookComOptions.h"
#include "HookNet/HookNetOptions.h"

typedef struct tagCallStackItemInfo
{
    PBYTE  Address;
    PBYTE  RelativeAddress;
    PBYTE  Parameters;
    TCHAR* pszModuleName;
}CALLSTACK_ITEM_INFO,*PCALLSTACK_ITEM_INFO;

typedef struct tagLogEntry
{
    PLOG_ENTRY_FIXED_SIZE pHookInfos;
    TCHAR* pszModuleName;
    TCHAR* pszApiName;
    TCHAR* pszCallingModuleName;
    PPARAMETER_LOG_INFOS ParametersInfoArray;// number of items is defined by pHookInfos->bNumberOfParameters
    PCALLSTACK_ITEM_INFO CallSackInfoArray;// number of items is defined by pHookInfos->CallStackSize
    tagExtendedFunctionInfosForHookType HookTypeExtendedFunctionInfos; // contains extended informations to uniquely identify function
}LOG_ENTRY,*PLOG_ENTRY;

// types of report messages
// DON'T CHANGE EXISTING VALUES TO AVOID TROUBLES RELOADING OLD MONITORING FILES
enum tagMsgTypes
{
    MSG_INFORMATION=REPORT_MESSAGE_INFORMATION,
    MSG_WARNING=REPORT_MESSAGE_WARNING,
    MSG_ERROR=REPORT_MESSAGE_ERROR,
    MSG_EXCEPTION=REPORT_MESSAGE_EXCEPTION
};

// types of logs
// DON'T CHANGE EXISTING VALUES TO AVOID TROUBLES RELOADING OLD MONITORING FILES
enum tagLogListEntryTypes
{
    ENTRY_LOG,
    ENTRY_MSG_INFORMATION=MSG_INFORMATION,
    ENTRY_MSG_WARNING=MSG_WARNING,
    ENTRY_MSG_ERROR=MSG_ERROR,
    ENTRY_MSG_EXCEPTION=MSG_EXCEPTION
};

typedef struct tagReportEntry
{
    TCHAR* pUserMsg;
    FILETIME ReportTime;
}REPORT_ENTRY,*PREPORT_ENTRY;

typedef struct tagLogListEntry
{
    DWORD dwId;
    tagLogListEntryTypes Type;// log or information
    union
    {
        LOG_ENTRY* pLog;
        REPORT_ENTRY ReportEntry;
    };
}LOG_LIST_ENTRY,*PLOG_LIST_ENTRY;

typedef void (STDMETHODCALLTYPE *tagCallBackLogFunc)(LOG_ENTRY* pLog,PVOID pUserParam);
typedef void (STDMETHODCALLTYPE *tagCallBackUnexpectedUnload)(DWORD dwProcessID,PVOID pUserParam);
typedef void (STDMETHODCALLTYPE *tagpCallBackBeforeAppResume)(DWORD dwProcessID,PVOID pUserParam);
typedef void (STDMETHODCALLTYPE *tagCallBackReportMessages)(tagReportMessageType ReportMessageType,TCHAR* ReportMessage,FILETIME FileTime,LPVOID UserParam);
typedef void (STDMETHODCALLTYPE *tagpCallBackBeforeAppResume)(DWORD dwProcessID,PVOID pUserParam);
typedef void (STDMETHODCALLTYPE *tagpCallBackOverridingDllQuery)(TCHAR* PluginName,PVOID MessageId,PBYTE pMsg,SIZE_T MsgSize,PVOID UserParam);

typedef struct tagRemoteCallInfos
{
    PBYTE ProcessInternalCallReply;
    HANDLE hevtProcessInternalCallReply;
}REMOTE_CALL_INFOS,*PREMOTE_CALL_INFOS;

class IApiOverride
{
public:
    enum StartWays
    {
        StartWaySleep,
        StartWaySuspended,
        StartWayIAT
    };

    enum tagColumnsIndex
    {
        ColumnsIndexId=0,
        ColumnsIndexDirection,
        ColumnsIndexCall,
        ColumnsIndexReturnValue,
        ColumnsIndexCallerAddress,
        ColumnsIndexCallerRelativeIndex,
        ColumnsIndexProcessID,
        ColumnsIndexThreadID,
        ColumnsIndexLastError,
        ColumnsIndexRegistersBeforeCall,
        ColumnsIndexRegistersAfterCall,
        ColumnsIndexFloatingReturnValue,
        ColumnsIndexCallTime,
        ColumnsIndexCallDuration,
        ColumnsIndexModuleName,
        ColumnsIndexAPIName,
        ColumnsIndexCallerFullPath,

        NbColumns,// delimiter, must be at the end of the enum
        FirstColumn = 0,// delimiter
        LastColumn = (NbColumns-1)// delimiter
    }ColumnsIndex;

    virtual BOOL STDMETHODCALLTYPE SetParentWindow(HWND hParentWindow)=0;

    virtual BOOL STDMETHODCALLTYPE LoadMonitoringFile(TCHAR* pszFileName)=0;
    virtual BOOL STDMETHODCALLTYPE UnloadMonitoringFile(TCHAR* pszFileName)=0;
    virtual BOOL STDMETHODCALLTYPE LoadFakeAPI(TCHAR* pszFileName)=0;
    virtual BOOL STDMETHODCALLTYPE UnloadFakeAPI(TCHAR* pszFileName)=0;

    virtual BOOL STDMETHODCALLTYPE StartMonitoring()=0;
    virtual BOOL STDMETHODCALLTYPE StopMonitoring()=0;
    virtual BOOL STDMETHODCALLTYPE StartFaking()=0;
    virtual BOOL STDMETHODCALLTYPE StopFaking()=0;

    virtual BOOL STDMETHODCALLTYPE LogOnlyBaseModule(BOOL bOnlyBaseModule)=0;
    virtual BOOL STDMETHODCALLTYPE SetModuleFilteringWay(tagFilteringWay FilteringWay)=0;
    virtual BOOL STDMETHODCALLTYPE SetModuleLogState(TCHAR* pszModuleFullPath,BOOL bLog)=0;
    virtual BOOL STDMETHODCALLTYPE AddToFiltersModuleList(TCHAR* pszFileName)=0;
    virtual BOOL STDMETHODCALLTYPE RemoveFromFiltersModuleList(TCHAR* pszFileName)=0;
    virtual BOOL STDMETHODCALLTYPE ClearFiltersModuleList()=0;
    virtual BOOL STDMETHODCALLTYPE GetNotLoggedModuleList(TCHAR*** pArrayNotLoggedModulesNames,DWORD* pdwArrayNotLoggedModulesNamesSize)=0;
    virtual BOOL STDMETHODCALLTYPE SetMonitoringModuleFiltersState(BOOL bEnable)=0;
    virtual BOOL STDMETHODCALLTYPE SetFakingModuleFiltersState(BOOL bEnable)=0;
    virtual BOOL STDMETHODCALLTYPE SetAutoAnalysis(tagFirstBytesAutoAnalysis AutoAnalysis)=0;
    virtual BOOL STDMETHODCALLTYPE AllowTlsCallbackHooking(BOOL bAllow)=0;
    virtual BOOL STDMETHODCALLTYPE EnableCOMAutoHooking(BOOL bEnable,BOOL ShowTryToUnhookIfNeeded)=0;
    virtual BOOL STDMETHODCALLTYPE EnableCOMAutoHooking(BOOL bEnable)=0;
    virtual BOOL STDMETHODCALLTYPE SetCOMOptions(HOOK_COM_OPTIONS* pComOptions)=0;
    virtual BOOL STDMETHODCALLTYPE ShowCOMInteractionDialog()=0;
    virtual BOOL STDMETHODCALLTYPE EnableNETProfiling(BOOL bEnable)=0;// must be called to really enable NetHooking
    virtual BOOL STDMETHODCALLTYPE EnableNetAutoHooking(BOOL bEnable)=0;// used only to signal user events (in case an application was ".Net profiled")
    // as cor profiling state can't be changed after application startup
    virtual BOOL STDMETHODCALLTYPE EnableNetAutoHooking(BOOL bEnable,BOOL ShowTryToUnhookIfNeeded)=0;// used only to signal user events (in case an application was ".Net profiled")
    // as cor profiling state can't be changed after application startup
    virtual BOOL STDMETHODCALLTYPE SetNetOptions(HOOK_NET_OPTIONS* pNetOptions)=0;
	virtual BOOL STDMETHODCALLTYPE AddHookNetFromTokenForCompiledFuntions(TCHAR* AssemblyName,SIZE_T NbToken,ULONG32* TokenArray)=0;
	virtual BOOL STDMETHODCALLTYPE RemoveHookNetFromTokenForCompiledFuntion(TCHAR* AssemblyName,SIZE_T NbToken,ULONG32* TokenArray)=0;
    virtual BOOL STDMETHODCALLTYPE ShowNetInteractionDialog()=0;
    virtual BOOL STDMETHODCALLTYPE SetCallStackRetrieval(BOOL bLogCallStack,DWORD CallStackParametersRetrievalSize)=0;
    virtual BOOL STDMETHODCALLTYPE BreakDialogDontBreakApioverrideThreads(BOOL bDontBreak)=0;
    virtual BOOL STDMETHODCALLTYPE SetMonitoringFileDebugMode(BOOL bActiveMode)=0;

    virtual BOOL STDMETHODCALLTYPE Dump()=0;

    virtual BOOL STDMETHODCALLTYPE SetReportMessagesCallBack(tagCallBackReportMessages pCallBackFunc,LPVOID pUserParam)=0;
    virtual BOOL STDMETHODCALLTYPE SetUnexpectedUnloadCallBack(tagCallBackUnexpectedUnload pCallBackFunc,LPVOID pUserParam)=0;
    virtual BOOL STDMETHODCALLTYPE SetMonitoringCallback(tagCallBackLogFunc pCallBackLogFunc,LPVOID pUserParam,BOOL bManualFreeLogEntry)=0;
    virtual void STDMETHODCALLTYPE FreeLogEntry(LOG_ENTRY* pLog)=0;
    virtual void STDMETHODCALLTYPE FreeLogEntry(LOG_ENTRY* pLog,HANDLE Heap)=0;
    virtual BOOL STDMETHODCALLTYPE SetMonitoringListview(HWND hListView)=0;
    virtual BOOL STDMETHODCALLTYPE InitializeMonitoringListview()=0;

    virtual BOOL STDMETHODCALLTYPE AddLogEntry(LOG_LIST_ENTRY* pLogEntry,BOOL bStorePointerInListViewItemUserData)=0;
    virtual BOOL STDMETHODCALLTYPE AddLogEntry(LOG_LIST_ENTRY* pLogEntry,BOOL bStorePointerInListViewItemUserData,int Increment)=0;

    virtual BOOL STDMETHODCALLTYPE Stop()=0;
    virtual BOOL STDMETHODCALLTYPE Start(DWORD dwPID)=0;
    virtual BOOL STDMETHODCALLTYPE Start(TCHAR* pszFileName)=0;
    virtual BOOL STDMETHODCALLTYPE Start(TCHAR* pszFileName,tagpCallBackBeforeAppResume pCallBackFunc,LPVOID pUserParam)=0;
    virtual BOOL STDMETHODCALLTYPE Start(TCHAR* pszFileName,TCHAR* pszCmdLine,tagpCallBackBeforeAppResume pCallBackFunc,LPVOID pUserParam)=0;
    virtual BOOL STDMETHODCALLTYPE Start(TCHAR* pszFileName,TCHAR* pszCmdLine,tagpCallBackBeforeAppResume pCallBackFunc,LPVOID pUserParam,StartWays StartMethod,DWORD dwResumeTimeAtStartup)=0;
    virtual BOOL STDMETHODCALLTYPE Start(TCHAR* pszFileName,TCHAR* pszCmdLine,tagpCallBackBeforeAppResume pCallBackFunc,LPVOID pUserParam,StartWays StartMethod,DWORD dwResumeTimeAtStartup,OUT STARTUPINFO* pStartupInfo,OUT PROCESS_INFORMATION* pProcessInformation)=0;
    virtual BOOL STDMETHODCALLTYPE StartAtProcessCreation(DWORD ProcessId,DWORD ThreadId,BOOL bLetThreadSuspended,tagpCallBackBeforeAppResume pCallBackFunc,PVOID pUserParam)=0;

    virtual BOOL STDMETHODCALLTYPE ProcessInternalCall(TCHAR* LibName,TCHAR* FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,PBYTE* pReturnValue)=0;
    virtual BOOL STDMETHODCALLTYPE ProcessInternalCall(TCHAR* LibName,TCHAR* FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,PBYTE* pReturnValue,DWORD dwTimeOutMs)=0;
    virtual BOOL STDMETHODCALLTYPE ProcessInternalCall(TCHAR* LibName,TCHAR* FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pReturnValue,DWORD dwTimeOutMs)=0;
    virtual BOOL STDMETHODCALLTYPE ProcessInternalCall(TCHAR* LibName,TCHAR* FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pReturnValue,double* FloatingReturn,DWORD dwTimeOutMs,DWORD ThreadId)=0;
    virtual BOOL STDMETHODCALLTYPE ProcessInternalCall(TCHAR* LibName,TCHAR* FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pReturnValue,double* FloatingReturn,DWORD dwTimeOutMs,DWORD ThreadId,tagCALLING_CONVENTION CallingConvention)=0;

    virtual DWORD STDMETHODCALLTYPE GetProcessID()=0;
    virtual BOOL  STDMETHODCALLTYPE GetProcessName(TCHAR* ProcessName,int ProcessNameMaxSize)=0;
    virtual BOOL  STDMETHODCALLTYPE GetProcessFullPathName(TCHAR* ProcessFullPathName,int ProcessFullPathNameMaxSize)=0;

    virtual BOOL STDMETHODCALLTYPE SetMonitoringLogHeap(HANDLE Heap)=0; // must be protected by WaitAndLockMonitoringLogHeap and UnlockMonitoringLogHeap
    virtual BOOL STDMETHODCALLTYPE WaitAndLockMonitoringLogHeap()=0;
    virtual BOOL STDMETHODCALLTYPE UnlockMonitoringLogHeap()=0;

    virtual BOOL STDMETHODCALLTYPE ClearUserDataTypeCache()=0;
    
    virtual BOOL STDMETHODCALLTYPE SetOverridingDllQueryCallBack(tagpCallBackOverridingDllQuery pCallBackFunc,LPVOID pUserParam)=0;
    virtual BOOL STDMETHODCALLTYPE SendReplyToOverridingDllQuery(HANDLE MessageId,PBYTE pMsg,SIZE_T MsgSize)=0;
};