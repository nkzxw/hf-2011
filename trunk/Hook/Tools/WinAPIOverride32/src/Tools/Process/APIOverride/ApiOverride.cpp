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

#include "apioverride.h"
#include "../SuspendedProcess/SuspendedProcess.h"
#include "../ProcessName/ProcessName.h"
#include "HookNet/HookNetExport.h"

#pragma intrinsic (memcpy,memset,memcmp)

BOOL CApiOverridebNetProfilingEnabled=FALSE;
#define CApiOverride_SERVICES_PROCESS_NAME _T("services.exe")

//-----------------------------------------------------------------------------
// Name: Initialize
// Object: initializing part common to all constructors 
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
void CApiOverride::Initialize()
{
    this->bDestructorCalled=FALSE;
    InitializeCriticalSectionAndSpinCount(&this->CriticalSection,4000);
    
    this->pCallBackUnexpectedUnloadFunc=NULL;
    this->pCallBackUnexpectedUnloadFuncUserParam=NULL;
    this->pCallBackLogFunc=NULL;
    this->pCallBackLogFuncUserParam=NULL;
    this->pCallBackReportMessage=NULL;
    this->pCallBackReportMessagesUserParam=NULL;
    this->pCallBackOverridingDllQuery=NULL,
    this->pCallBackOverridingDllQueryUserParam=NULL;
    this->pListview=NULL;
    this->pInternalListview=NULL;
    this->dwCurrentProcessId=0;
    this->bAPIOverrideDllLoaded=FALSE;

    this->hThreadWatchingEvents=NULL;
    this->hThreadLogging=NULL;

    this->pMailSlotServer=NULL;
    this->pMailSlotClient=NULL;

    this->hevtStartMonitoring=NULL;
    this->hevtStopMonitoring=NULL;
    this->hevtStartFaking=NULL;
    this->hevtStopFaking=NULL;
    this->hevtFreeProcess=NULL;
    this->hevtTlsHookEndLoop=NULL;

    this->hevtAPIOverrideDllProcessAttachCompleted=NULL;
    this->hevtAPIOverrideDllProcessDetachCompleted=NULL;
    this->hevtProcessFree=NULL;
    this->hevtMonitoringFileLoaded=NULL;
    this->hevtMonitoringFileUnloaded=NULL;
    this->hevtFakeAPIDLLLoaded=NULL;
    this->hevtFakeAPIDLLUnloaded=NULL;
    this->hevtError=NULL;
    this->hevtClientMailslotOpen=NULL;

    this->pCurrentRemoteCalls=new CLinkListSimple();
    this->NotLoggedModulesArray=NULL;
    this->hevtGetNotLoggedModulesReply=CreateEvent(NULL,FALSE,FALSE,NULL);
    this->hDestructorUnlocked=CreateEvent(NULL,FALSE,TRUE,NULL);

    *this->ProcessName=0;
    *this->ProcessFullPathName=0;

    this->HookEntryPointRemoteHook=NULL;
    this->HookEntryPointRemoteLibName=NULL;
    this->HookEntryPointpProcessMemory=NULL;

    this->NotLoggedModulesArraySize=0;
    this->hParentWindow=NULL;

    this->AutoAnalysis=FIRST_BYTES_AUTO_ANALYSIS_NONE;
    this->bLogCallStack=FALSE;
    this->bMonitoringFileDebugMode=FALSE;
    this->CallStackEbpRetrievalSize=0;
    this->bOnlyBaseModule=FALSE;

    this->bAllowTlsCallBackHook=TRUE;

    this->bComAutoHookingEnabled=FALSE;
    memset(&this->ComHookingOptions,0,sizeof(HOOK_COM_OPTIONS));

    this->bNetAutoHookingEnabled=FALSE;
    memset(&this->NetHookingOptions,0,sizeof(HOOK_NET_OPTIONS));

    this->hStopUnlocked=CreateEvent(NULL,FALSE,TRUE,NULL);
    this->hEvtMonitoringLogHeapUnlocked=CreateEvent(NULL,FALSE,TRUE,NULL);

    this->MonitoringHeap=GetProcessHeap();

    // get current application directory
    CStdFileOperations::GetAppPath(this->pszAppPath,MAX_PATH);

    this->hmodInjlib=NULL;
    this->pInjectLib=NULL;
    this->pEjectLib=NULL;
}


//-----------------------------------------------------------------------------
// Name: FreeInjectLibrary
// Object: free InjLib dll
// Parameters :
//     in : 
// Return : TRUE
//-----------------------------------------------------------------------------
BOOL CApiOverride::FreeInjectLibrary()
{
    if (!this->hmodInjlib)
        return TRUE;

    this->pInjectLib=NULL;
    this->pEjectLib=NULL;
    FreeLibrary(this->hmodInjlib);
    this->hmodInjlib=NULL;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: LoadInjectLibrary
// Object: load InjLib dll if not already loaded
// Parameters :
//     in : 
// Return : TRUE if dll was already loaded or has been successufly loaded
//-----------------------------------------------------------------------------
BOOL CApiOverride::LoadInjectLibrary()
{
    TCHAR psz[MAX_PATH];

    // if library is already loaded
    if (this->hmodInjlib && this->pInjectLib && this->pEjectLib)
        return TRUE;

    // load injectlib dll
    _tcscpy(psz,this->pszAppPath);
    _tcscat(psz,INJECTLIB_DLL_NAME);
    this->hmodInjlib=LoadLibrary(psz);
    if (!this->hmodInjlib)
    {
        _stprintf(psz,_T("Error loading %s"),INJECTLIB_DLL_NAME);
        MessageBox(this->hParentWindow,psz,_T("Error"),MB_OK|MB_ICONERROR);
    }
    else
    {
        this->pInjectLib=(InjectLib)GetProcAddress(this->hmodInjlib,INJECTLIB_FUNC_NAME);
        this->pEjectLib=(EjectLib)GetProcAddress(this->hmodInjlib,EJECTLIB_FUNC_NAME);

        if (!this->pInjectLib)
        {
            _stprintf(psz,_T("%s not found in %s"),INJECTLIB_FUNC_NAME,INJECTLIB_DLL_NAME);
            MessageBox(this->hParentWindow,psz,_T("Error"),MB_OK|MB_ICONERROR);
        }
        if (!this->pEjectLib)
        {
            _stprintf(psz,_T("%s not found in %s"),EJECTLIB_FUNC_NAME,INJECTLIB_DLL_NAME);
            MessageBox(this->hParentWindow,psz,_T("Error"),MB_OK|MB_ICONERROR);
        }
    }
    return (this->hmodInjlib && this->pInjectLib && this->pEjectLib);
}

//-----------------------------------------------------------------------------
// Name: CApiOverride
// Object: Constructor 
//         use SetMonitoringCallback or SetMonitoringListview next to monitor hooks
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
CApiOverride::CApiOverride()
{
    this->Initialize();
}

//-----------------------------------------------------------------------------
// Name: CApiOverride
// Object: Constructor to manage yourself logging event
// Parameters :
//     in : tagCallBackLogFunc pCallBackLogFunc : monitoring callback 
//          warning we use mail slot so callback can be called few seconds after real function call
//          for real time function hooking just use a dll (see fake API dll sample)
// Return : 
//-----------------------------------------------------------------------------
CApiOverride::CApiOverride(tagCallBackLogFunc pCallBackLogFunc)
{
    this->Initialize();
    this->SetMonitoringCallback(pCallBackLogFunc,NULL,FALSE);
}

//-----------------------------------------------------------------------------
// Name: CApiOverride
// Object: Constructor. Listview will be configured automatically, and it will be filled by monitoring events
// Parameters :
//     in : HWND hParentWindow : handle of parent window. Allow to make modal messagebox
//          HWND hListView: Handle to a list view
// Return : 
//-----------------------------------------------------------------------------
CApiOverride::CApiOverride(HWND hParentWindow,HWND hListView)
{
    this->Initialize();
    this->hParentWindow=hParentWindow;
    this->SetMonitoringListview(hListView);
}

//-----------------------------------------------------------------------------
// Name: CApiOverride
// Object: Constructor. 
// Parameters :
//     in : HWND hParentWindow : handle of parent window. Allow to make modal messagebox
// Return : 
//-----------------------------------------------------------------------------
CApiOverride::CApiOverride(HWND hParentWindow)
{
    this->Initialize();
    this->hParentWindow=hParentWindow;
}

//-----------------------------------------------------------------------------
// Name: CApiOverride
// Object: destructor. 
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
CApiOverride::~CApiOverride(void)
{
    this->bDestructorCalled=TRUE;
    // assume callback won't be called after object destruction
    this->pCallBackUnexpectedUnloadFunc=NULL;
    this->pCallBackReportMessage=NULL;

    // assume no waiting function having release critical section are in use
    WaitForSingleObject(this->hDestructorUnlocked,INFINITE);
    CleanCloseHandle(&this->hDestructorUnlocked);

    EnterCriticalSection(&this->CriticalSection);

    this->StopWithoutEnteringCriticalSection();

    if (this->pInternalListview)
        delete this->pInternalListview;

    // assume injectlib dll is unloaded
    this->FreeInjectLibrary();

    CleanCloseHandle(&this->hevtGetNotLoggedModulesReply);
    CleanCloseHandle(&this->hStopUnlocked);

    if (this->HookEntryPointpProcessMemory)
        delete this->HookEntryPointpProcessMemory;

    if (this->pCurrentRemoteCalls)
        delete this->pCurrentRemoteCalls;

	// delete critical section only now when all other thread are finished
    DeleteCriticalSection(&this->CriticalSection);
}


BOOL STDMETHODCALLTYPE CApiOverride::SetParentWindow(HWND hParentWindow)
{
    this->hParentWindow = hParentWindow;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ReportError
// Object: Show an error message merging current process number
// Parameters :
//      in : - TCHAR* pszErrorMessage : error message without process number
// Return : 
//-----------------------------------------------------------------------------
void CApiOverride::ReportError(TCHAR* pszErrorMessage)
{
    this->ReportError(this->hParentWindow,pszErrorMessage);
}
//-----------------------------------------------------------------------------
// Name: ReportError
// Object: Show an error message merging current process number
// Parameters :
//      in : - TCHAR* pszErrorMessage : error message without process number
//           - HWND hWnd : allow to specify a window handle diferent than 
//                         this->hParentWindow like NULL
// Return : 
//-----------------------------------------------------------------------------
void CApiOverride::ReportError(HWND hWnd, TCHAR* pszErrorMessage)
{
    this->UserMessage(hWnd,pszErrorMessage,_T("Error"),MB_OK|MB_ICONERROR);
}

//-----------------------------------------------------------------------------
// Name: UserMessage
// Object: Show a messagebox merging current process number
// Parameters :
//      in : - TCHAR* pszErrorMessage : message without process number
//           - TCHAR* pszTitle : msgbox title
//           - UINT uType : msgbox title
// Return : msgbox result
//-----------------------------------------------------------------------------
int CApiOverride::UserMessage(TCHAR* pszMessage,TCHAR* pszTitle,UINT uType)
{
    return this->UserMessage(this->hParentWindow,pszMessage,pszTitle,uType);
}
//-----------------------------------------------------------------------------
// Name: UserMessage
// Object: Show a messagebox merging current process number
// Parameters :
//      in : - TCHAR* pszErrorMessage : message without process number
//           - TCHAR* pszTitle : msgbox title
//           - UINT uType : msgbox title
//           - HWND hWnd : allow to specify a window handle different than 
//                         this->hParentWindow like NULL
// Return : msgbox result
//-----------------------------------------------------------------------------
int CApiOverride::UserMessage(HWND hWnd,TCHAR* pszMessage,TCHAR* pszTitle,UINT uType)
{
    TCHAR pszMsg[2*MAX_PATH];
    if (this->dwCurrentProcessId!=0)
    {
        *pszMsg=0;
        _sntprintf(pszMsg,2*MAX_PATH,_T("%s for process ID 0x%.8X"),pszMessage,this->dwCurrentProcessId);
        if (*this->ProcessName)
        {
            _tcscat(pszMsg,_T(" ("));
            _tcscat(pszMsg,this->ProcessName);
            _tcscat(pszMsg,_T(")"));
        }
        return MessageBox(hWnd,pszMsg,pszTitle,uType);
    }
    else
    {
        return MessageBox(hWnd,pszMessage,pszTitle,uType);
    }
}

//-----------------------------------------------------------------------------
// Name: GetProcessID
// Object: return the process ID with which CApioverride is working or has worked at last
// Parameters :
// Return : PID if CAPIOverride
//-----------------------------------------------------------------------------
DWORD STDMETHODCALLTYPE CApiOverride::GetProcessID()
{
    return this->dwCurrentProcessId;
}

//-----------------------------------------------------------------------------
// Name: GetProcessName
// Object: return the process name with which CApioverride is working or has worked at last
// Parameters :
//      in : int ProcessNameMaxSize : max size of ProcessName in tchar
//      out: TCHAR* ProcessName : process name
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::GetProcessName(TCHAR* ProcessName,int ProcessNameMaxSize)
{
    if (IsBadWritePtr(ProcessName,ProcessNameMaxSize*sizeof(TCHAR)))
        return FALSE;

    _tcsncpy(ProcessName,this->ProcessName,ProcessNameMaxSize);
    ProcessName[ProcessNameMaxSize-1]=0;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetProcessFullPathName
// Object: return the process full path name with which CApioverride is working or has worked at last
// Parameters :
//      in : int ProcessNameMaxSize : max size of ProcessName in tchar
//      out: TCHAR* ProcessName : process name
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::GetProcessFullPathName(TCHAR* ProcessFullPathName,int ProcessFullPathNameMaxSize)
{
    if (IsBadWritePtr(ProcessFullPathName,ProcessFullPathNameMaxSize*sizeof(TCHAR)))
        return FALSE;

    _tcsncpy(ProcessFullPathName,this->ProcessFullPathName,ProcessFullPathNameMaxSize);
    ProcessFullPathName[ProcessFullPathNameMaxSize-1]=0;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: EnableNETProfiling
// Object: enable or disable .NET profiling
// Parameters :
//      in : BOOL bEnable : TRUE to start COM hooking, FALSE to stop it
//      out: 
// Return : FALSE on error
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::EnableNETProfiling(BOOL bEnable)
{
    return CApiOverride::EnableNETProfilingStatic(bEnable);
}

//-----------------------------------------------------------------------------
// Name: EnableNETProfilingStatic
// Object: enable or disable .NET profiling
// Parameters :
//      in : BOOL bEnable : TRUE to start COM hooking, FALSE to stop it
//      out: 
// Return : FALSE on error
//-----------------------------------------------------------------------------
BOOL CApiOverride::EnableNETProfilingStatic(BOOL bEnable)
{
    CApiOverridebNetProfilingEnabled=bEnable;
    BOOL bSuccess=TRUE;
    FARPROC pDllRegisterUnregisterServer;
    TCHAR szHookNetDllPath[MAX_PATH];
    HMODULE HookNetHModule;

    //////////////////////////////////////
    // assume HookNet.dll is registered
    //////////////////////////////////////

    // load hook net dll
    CStdFileOperations::GetAppPath(szHookNetDllPath,MAX_PATH);
    _tcscat(szHookNetDllPath,HOOKNET_DLL_NAME);
    HookNetHModule=LoadLibrary(szHookNetDllPath);
    if (!HookNetHModule)
    {
        TCHAR psz[2*MAX_PATH];
        _stprintf(psz,_T(".NET hooking dll %s not found"),szHookNetDllPath);
        MessageBox(NULL,psz,_T("Error"),MB_OK|MB_ICONERROR);
        return FALSE;
    }
    if (bEnable)
    {
        // get DllRegisterServer function pointer
        pDllRegisterUnregisterServer=GetProcAddress(HookNetHModule,"DllRegisterServer");
    }
    else
    {
        // get DllRegisterServer function pointer
        pDllRegisterUnregisterServer=GetProcAddress(HookNetHModule,"DllUnregisterServer");
    }

    if (!pDllRegisterUnregisterServer)
        return FALSE;

    // call DllRegisterServer
    pDllRegisterUnregisterServer();

    // free library
    FreeLibrary(HookNetHModule);

    //////////////////////////////////////
    // set environment variable
    //////////////////////////////////////
    // MSDN
    // To programmatically add or modify system environment variables, 
    // add them to the "HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\Environment" 
    // registry key, then broadcast a WM_SETTINGCHANGE message. 
    // This allows applications, such as the shell, to pick up your updates.
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("System\\CurrentControlSet\\Control\\Session Manager\\Environment"),0,KEY_ALL_ACCESS,&hKey)!=ERROR_SUCCESS)
    {
        if (bEnable)
        {
            MessageBox(NULL,_T("Error accessing registry, can't enable .Net hooking"),_T("Error"),MB_OK|MB_ICONERROR);

            // force dll to be unregistered
            CApiOverride::EnableNETProfilingStatic(FALSE);

            return FALSE;
        }
        else
        {
            return FALSE;
        }
    }


    if (bEnable)
    {
        // COR_ENABLE_PROFILING=0x1
        // COR_PROFILER={52AE91FC-569A-496f-A268-74D62B866D73}// CLSID_NET_PROFILER_GUID
        BOOL Failure=FALSE;
        if (RegSetValueEx(hKey,_T("COR_ENABLE_PROFILING"),0,REG_SZ,(PBYTE)_T("1"),2*sizeof(TCHAR))!=ERROR_SUCCESS)
            Failure=TRUE;
        if (RegSetValueEx(hKey,_T("COR_PROFILER"),0,REG_SZ,(PBYTE)CLSID_NET_PROFILER_GUID,(DWORD)(_tcslen(CLSID_NET_PROFILER_GUID)+1)*sizeof(TCHAR))!=ERROR_SUCCESS)
            Failure=TRUE;
        if (Failure)
        {
            MessageBox(NULL,_T("Error accessing registry, can't enable .Net hooking"),_T("Error"),MB_OK|MB_ICONERROR);

            // force dll to be unregistered
            CApiOverride::EnableNETProfilingStatic(FALSE);

            bSuccess=FALSE;
        }

        // change environment var for current process (needed for hook application at startup)
        SetEnvironmentVariable(_T("COR_ENABLE_PROFILING"),_T("1"));
        SetEnvironmentVariable(_T("COR_PROFILER"),CLSID_NET_PROFILER_GUID);

        // update services.exe process in case of .Net service restart
        CSetEnvironmentVariableToProcess SetEnv;
        CSetEnvironmentVariableToProcess::ENV_VAR_DEFINITION EnvVarDef[2] = { 
                                                                                {_T("COR_ENABLE_PROFILING"),_T("1")},
                                                                                {_T("COR_PROFILER"),CLSID_NET_PROFILER_GUID}
                                                                            };
        if (SetEnv.SetEnvironmentVariables(EnvVarDef,2))
            SetEnv.ApplyChanges(CApiOverride_SERVICES_PROCESS_NAME,FALSE);
    }
    else
    {
        // key to delete
        // COR_ENABLE_PROFILING
        // COR_PROFILER
        RegDeleteValue(hKey,_T("COR_ENABLE_PROFILING"));
        RegDeleteValue(hKey,_T("COR_PROFILER"));

        // change environment var for current process (needed for hook application at startup)
        SetEnvironmentVariable(_T("COR_ENABLE_PROFILING"),_T("0"));
        SetEnvironmentVariable(_T("COR_PROFILER"),_T(""));

        // update services.exe process in case of .Net service restart
        CSetEnvironmentVariableToProcess SetEnv;
        CSetEnvironmentVariableToProcess::ENV_VAR_DEFINITION EnvVarDef[2] = { 
                                                                                {_T("COR_ENABLE_PROFILING"),_T("0")},
                                                                                {_T("COR_PROFILER"),_T("")}
                                                                            };
        if (SetEnv.SetEnvironmentVariables(EnvVarDef,2))
            SetEnv.ApplyChanges(CApiOverride_SERVICES_PROCESS_NAME,FALSE);
    }
    // close key
    RegCloseKey(hKey);

    // force changes to be taken into account for already started application (like explorer)
    DWORD dwResult;
    SendMessageTimeout(HWND_BROADCAST,WM_SETTINGCHANGE,0,(LPARAM)_T("Environment"),SMTO_ABORTIFHUNG,50000,&dwResult);

    return bSuccess;
}

//-----------------------------------------------------------------------------
// Name: EnableNetAutoHooking
// Object: enable or disable Net auto hooking (in case an application was ".Net profiled")
//         Remarks : YOU SHOULD CALL SetNetOptions BEFORE CALLING THIS FUNCTION
// Parameters :
//      in : BOOL bEnable : TRUE to start COM hooking, FALSE to stop it
//      out: 
// Return : FALSE on error
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::EnableNetAutoHooking(BOOL bEnable)
{
    return this->EnableNetAutoHooking(bEnable,TRUE);
}
BOOL STDMETHODCALLTYPE CApiOverride::EnableNetAutoHooking(BOOL bEnable,BOOL ShowTryToUnhookIfNeeded)
{
    BOOL OldValue;
    EnterCriticalSection(&this->CriticalSection);

    // store old option
    OldValue=this->bNetAutoHookingEnabled;
    this->bNetAutoHookingEnabled=bEnable;

    // if injected dll is loaded
    if (this->bAPIOverrideDllLoaded)
    {
        // fill command
        STRUCT_COMMAND Cmd;
        Cmd.dwCommand_ID=CMD_NET_HOOKING_START_STOP;
        Cmd.Param[0]=this->bNetAutoHookingEnabled;
        // send command
        if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
        {
            ReportError(_T("Error modifying .NET option.\r\nMailSlot write error"));
            this->bComAutoHookingEnabled=OldValue;
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }

        if ((!this->bNetAutoHookingEnabled) // if we ask to disable Net auto hooking
            && (OldValue==TRUE)             // and net auto hooking was enabled
            && ShowTryToUnhookIfNeeded
            )
        {
            TCHAR pszMsg[MAX_PATH];
            if (*this->ProcessName)
                _stprintf(pszMsg,_T("Do you want to unhook already hooked .NET methods for application %s (0x%.8X)"),this->ProcessName,this->dwCurrentProcessId);
            else
                _stprintf(pszMsg,_T("Do you want to unhook already hooked .NET methods for process ID 0x%.8X"),this->dwCurrentProcessId);

            if (MessageBox(this->hParentWindow,
                pszMsg,
                _T("Question"),
                MB_TOPMOST|MB_ICONQUESTION|MB_YESNO
                )
                ==IDYES
                )
            {
                Cmd.dwCommand_ID=CMD_NET_RELEASE_HOOKED_METHODS;
                if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
                {
                    ReportError(_T("Error releasing hooked .NET methods.\r\nMailSlot write error"));
                    LeaveCriticalSection(&this->CriticalSection);
                    return FALSE;
                }
            }
        }
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetNetOptions
// Object: set .Net monitoring options
// Parameters :
//      in : HOOK_NET_OPTIONS* pNetOptions : struct containing new options to apply
//      out: 
// Return : FALSE on error
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetNetOptions(HOOK_NET_OPTIONS* pNetOptions)
{
    EnterCriticalSection(&this->CriticalSection);
    HOOK_NET_OPTIONS OldNetOptions={0};
    // store previous option
    if (&this->NetHookingOptions!=pNetOptions)
    {
        memcpy(&OldNetOptions,&this->NetHookingOptions,sizeof(HOOK_NET_OPTIONS));
        memcpy(&this->NetHookingOptions,pNetOptions,sizeof(HOOK_NET_OPTIONS));
    }

    // if injected dll is loaded
    if (this->bAPIOverrideDllLoaded)
    {
        BYTE pCmd[sizeof(DWORD)+sizeof(HOOK_NET_OPTIONS)];
        DWORD CmdId=CMD_NET_HOOKING_OPTIONS;

        // fill command Id
        memcpy(pCmd,&CmdId,sizeof(DWORD));
        // fill Com options
        memcpy(&pCmd[sizeof(DWORD)],pNetOptions,sizeof(HOOK_NET_OPTIONS));

        // send command
        if (!this->pMailSlotClient->Write(pCmd,sizeof(DWORD)+sizeof(HOOK_NET_OPTIONS)))
        {
            ReportError(_T("Error modifying NET options.\r\nMailSlot write error"));
            if (&this->NetHookingOptions!=pNetOptions)
            {
                // restore previous state 
                memcpy(&this->NetHookingOptions,&pNetOptions,sizeof(HOOK_NET_OPTIONS));
            }
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: HookFunctions
// Object: display .NET Interaction dialog
// Parameters :
//      out: 
// Return : FALSE on error
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::AddHookNetFromTokenForCompiledFuntions(TCHAR* AssemblyName,SIZE_T NbToken,/*mdToken*/ULONG32* TokenArray)
{
	if (IsBadReadPtr(AssemblyName,sizeof(TCHAR)))
		return FALSE;
	SIZE_T ArraySizeInByteCount=NbToken*sizeof(/*mdToken*/ULONG32);
	if (IsBadReadPtr(TokenArray,ArraySizeInByteCount))
		return FALSE;

	HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM Param;
	_tcsncpy(Param.AssemblyName,AssemblyName,MAX_PATH);
	Param.TokensNumber = (DWORD)NbToken;

	PBYTE Cmd = new BYTE[sizeof(DWORD)+sizeof(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM)+ArraySizeInByteCount];
	DWORD dwCommand_ID = CMD_NET_HOOK_FROM_TOKEN;
	PBYTE pBuffer= Cmd;

	memcpy(pBuffer,&dwCommand_ID,sizeof(DWORD));
	pBuffer+=sizeof(DWORD);

	memcpy(pBuffer,&Param,sizeof(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM));
	pBuffer+=sizeof(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM);

	memcpy(pBuffer,TokenArray,ArraySizeInByteCount);
	pBuffer+=ArraySizeInByteCount;

	SIZE_T CommandSize = pBuffer-Cmd;

	EnterCriticalSection(&this->CriticalSection);

    // if injected dll is not loaded
    if (!this->bAPIOverrideDllLoaded)
        ReportError(_T("Error Net hooking not started"));

    // else injected dll will do the job

    // send command
    if (!this->pMailSlotClient->Write(Cmd,CommandSize))
    {
        ReportError(_T("Error calling .NET AddHookNetFromTokenForCompiledFuntions Functions.\r\nMailSlot write error"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

BOOL STDMETHODCALLTYPE CApiOverride::RemoveHookNetFromTokenForCompiledFuntion(TCHAR* AssemblyName,SIZE_T NbToken,ULONG32* TokenArray)
{
	if (IsBadReadPtr(AssemblyName,sizeof(TCHAR)))
		return FALSE;
	SIZE_T ArraySizeInByteCount=NbToken*sizeof(/*mdToken*/ULONG32);
	if (IsBadReadPtr(TokenArray,ArraySizeInByteCount))
		return FALSE;

	HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM Param;
	_tcsncpy(Param.AssemblyName,AssemblyName,MAX_PATH);
	Param.TokensNumber = (DWORD)NbToken;

	PBYTE Cmd = new BYTE[sizeof(DWORD)+sizeof(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM)+ArraySizeInByteCount];
	DWORD dwCommand_ID = CMD_NET_UNHOOK_FROM_TOKEN;
	PBYTE pBuffer= Cmd;

	memcpy(pBuffer,&dwCommand_ID,sizeof(DWORD));
	pBuffer+=sizeof(DWORD);

	memcpy(pBuffer,&Param,sizeof(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM));
	pBuffer+=sizeof(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM);

	memcpy(pBuffer,TokenArray,ArraySizeInByteCount);
	pBuffer+=ArraySizeInByteCount;

	SIZE_T CommandSize = pBuffer-Cmd;

	EnterCriticalSection(&this->CriticalSection);

	// if injected dll is not loaded
	if (!this->bAPIOverrideDllLoaded)
		ReportError(_T("Error Net hooking not started"));

	// else injected dll will do the job

	// send command
	if (!this->pMailSlotClient->Write(Cmd,CommandSize))
    {
        ReportError(_T("Error calling .NET RemoveHookNetFromTokenForCompiledFuntion Functions.\r\nMailSlot write error"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ShowNetInteractionDialog
// Object: display .NET Interaction dialog
// Parameters :
//      out: 
// Return : FALSE on error
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::ShowNetInteractionDialog()
{
    EnterCriticalSection(&this->CriticalSection);

    // if injected dll is not loaded
    if (!this->bAPIOverrideDllLoaded)
        ReportError(_T("Error Net hooking not started"));

    // else injected dll will do the job

    // fill command
    STRUCT_COMMAND Cmd;
    Cmd.dwCommand_ID=CMD_NET_INTERACTION;
    // send command
    if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
    {
        ReportError(_T("Error calling .NET Interaction Dialog.\r\nMailSlot write error"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: EnableCOMAutoHooking
// Object: enable or disable com hooking
// Parameters :
//      in : BOOL bEnable : TRUE to start COM hooking, FALSE to stop it
//      out: 
// Return : FALSE on error
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::EnableCOMAutoHooking(BOOL bEnable)
{
    return this->EnableCOMAutoHooking(bEnable,TRUE);
}
BOOL STDMETHODCALLTYPE CApiOverride::EnableCOMAutoHooking(BOOL bEnable,BOOL ShowTryToUnhookIfNeeded)
{
    EnterCriticalSection(&this->CriticalSection);

    BOOL OldValue;
    // store old option
    OldValue=this->bComAutoHookingEnabled;
    this->bComAutoHookingEnabled=bEnable;

    // if injected dll is loaded
    if (this->bAPIOverrideDllLoaded)
    {
        // fill command
        STRUCT_COMMAND Cmd;
        Cmd.dwCommand_ID=CMD_COM_HOOKING_START_STOP;
        Cmd.Param[0]=this->bComAutoHookingEnabled;
        // send command
        if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
        {
            ReportError(_T("Error modifying COM option.\r\nMailSlot write error"));
            this->bComAutoHookingEnabled=OldValue;
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }

        if ((!this->bComAutoHookingEnabled)
            && (OldValue==TRUE)
            && ShowTryToUnhookIfNeeded
            )
        {
            TCHAR pszMsg[MAX_PATH];
            if (*this->ProcessName)
                _stprintf(pszMsg,_T("Do you want to unhook already auto hooked COM objects for application %s (0x%.8X)"),this->ProcessName,this->dwCurrentProcessId);
            else
                _stprintf(pszMsg,_T("Do you want to unhook already auto hooked COM objects for process ID 0x%.8X"),this->dwCurrentProcessId);

            if (MessageBox(this->hParentWindow,
                           pszMsg,
                           _T("Question"),
                           MB_TOPMOST|MB_ICONQUESTION|MB_YESNO
                           )
                           ==IDYES
                 )
            {
                Cmd.dwCommand_ID=CMD_COM_RELEASE_AUTO_HOOKED_OBJECTS;
                if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
                {
                    ReportError(_T("Error releasing auto hooked COM objects.\r\nMailSlot write error"));
                    LeaveCriticalSection(&this->CriticalSection);
                    return FALSE;
                }
            }
        }
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetCOMOptions
// Object: set COM options
// Parameters :
//      in : 
//      out: 
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetCOMOptions(HOOK_COM_OPTIONS* pComOptions)
{
    EnterCriticalSection(&this->CriticalSection);

    HOOK_COM_OPTIONS OldComOptions={0};
    // store previous option
    if (&this->ComHookingOptions!=pComOptions)
    {
        memcpy(&OldComOptions,&this->ComHookingOptions,sizeof(HOOK_COM_OPTIONS));
        memcpy(&this->ComHookingOptions,pComOptions,sizeof(HOOK_COM_OPTIONS));
    }

    // if injected dll is loaded
    if (this->bAPIOverrideDllLoaded)
    {
        BYTE pCmd[sizeof(DWORD)+sizeof(HOOK_COM_OPTIONS)];
        DWORD CmdId=CMD_COM_HOOKING_OPTIONS;

        // fill command Id
        memcpy(pCmd,&CmdId,sizeof(DWORD));
        // fill Com options
        memcpy(&pCmd[sizeof(DWORD)],pComOptions,sizeof(HOOK_COM_OPTIONS));

        // send command
        if (!this->pMailSlotClient->Write(pCmd,sizeof(DWORD)+sizeof(HOOK_COM_OPTIONS)))
        {
            ReportError(_T("Error modifying COM options.\r\nMailSlot write error"));
            if (&this->ComHookingOptions!=pComOptions)
            {
                // restore previous state 
                memcpy(&this->ComHookingOptions,&OldComOptions,sizeof(HOOK_COM_OPTIONS));
            }
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ShowCOMInteractionDialog
// Object: display COM Interaction dialog
// Parameters :
//      out: 
// Return : FALSE on error
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::ShowCOMInteractionDialog()
{
    EnterCriticalSection(&this->CriticalSection);
    // store old option
    if(!this->bComAutoHookingEnabled)
        ReportError(_T("Error COM hooking not started"));

    // if injected dll is loaded
    if (this->bAPIOverrideDllLoaded)
    {
        // fill command
        STRUCT_COMMAND Cmd;
        Cmd.dwCommand_ID=CMD_COM_INTERACTION;
        // send command
        if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
        {
            ReportError(_T("Error calling COM Interaction Dialog.\r\nMailSlot write error"));
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: SetAutoAnalysis
// Object: set auto analysis mode
// Parameters :
//      in : tagFirstBytesAutoAnalysis AutoAnalysis : new first bytes auto analysis
//      out: 
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetAutoAnalysis(tagFirstBytesAutoAnalysis AutoAnalysis)
{
    EnterCriticalSection(&this->CriticalSection);

    tagFirstBytesAutoAnalysis OldAutoAnalysis;
    OldAutoAnalysis=this->AutoAnalysis;

    this->AutoAnalysis=AutoAnalysis;
    if (this->bAPIOverrideDllLoaded)
    {
        STRUCT_COMMAND Cmd;
        Cmd.dwCommand_ID=CMD_AUTOANALYSIS;
        Cmd.Param[0]=this->AutoAnalysis;
        if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
        {
            ReportError(_T("Error modifying auto analysis mode.\r\nMailSlot write error"));
            // restore previous state 
            this->AutoAnalysis=OldAutoAnalysis;

            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: SetCallStackRetrieval
// Object: set if call stack must be log , and the size of stack (in bytes)  
//         that should be logged for each call 
// Parameters :
//      in : BOOL bLogCallStack : TRUE to log call stack
//           DWORD CallStackParametersRetrievalSize : size of stack (in bytes) logged for each call
//                                                    meaningful only if bLogCallStack is TRUE
//      out: 
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetCallStackRetrieval(BOOL bLogCallStack,DWORD CallStackParametersRetrievalSize)
{
    EnterCriticalSection(&this->CriticalSection);
    this->bLogCallStack=bLogCallStack;
    this->CallStackEbpRetrievalSize=CallStackParametersRetrievalSize;
    if (this->bAPIOverrideDllLoaded)
    {
        STRUCT_COMMAND Cmd;
        Cmd.dwCommand_ID=CMD_CALLSTACK_RETRIEVAL;
        Cmd.Param[0]=this->bLogCallStack;
        Cmd.Param[1]=this->CallStackEbpRetrievalSize;
        if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
        {
            ReportError(_T("Error modifying stack retrieval way.\r\nMailSlot write error"));
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetOverridingDllQueryCallBack
// Object: 
// Parameters :
//      in : HANDLE MessageId : Message Id matching the Query
//           PBYTE pMsg : reply content
//           SIZE_T MsgSize : reply content size
//      out: 
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetOverridingDllQueryCallBack(tagpCallBackOverridingDllQuery pCallBackFunc,LPVOID pUserParam)
{
    if (IsBadCodePtr((FARPROC)pCallBackFunc) && (pCallBackFunc!=0)) // pCallBackFunc==0 to remove callback
        return FALSE;
    this->pCallBackOverridingDllQuery=pCallBackFunc;
    this->pCallBackOverridingDllQueryUserParam=pUserParam;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SendReplyToOverridingDllQuery
// Object: 
// Parameters :
//      in : HANDLE MessageId : Message Id matching the Query
//           PBYTE pMsg : reply content
//           SIZE_T MsgSize : reply content size
//      out: 
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SendReplyToOverridingDllQuery(PVOID MessageId,PBYTE pMsg,SIZE_T MsgSize)
{
    if (!this->bAPIOverrideDllLoaded)
        return FALSE;

    EnterCriticalSection(&this->CriticalSection);
    
    DWORD CommandId;
    PBYTE MailSlotMsg;
    SIZE_T MailSlotMsgSize;
    SIZE_T Index;

    MailSlotMsgSize = sizeof(DWORD)+sizeof(PVOID)+sizeof(SIZE_T)+MsgSize;
    MailSlotMsg = new BYTE[MailSlotMsgSize];

    CommandId = CMD_PLUGIN_REPLY_TO_OVERRIDING_DLL_QUERY;

    // command id
    Index = 0;
    memcpy(&MailSlotMsg[Index],&CommandId,sizeof(DWORD));
    Index+= sizeof(DWORD);

    // message id
    memcpy(&MailSlotMsg[Index],&MessageId,sizeof(PVOID));
    Index+= sizeof(PVOID);

    // data size
    memcpy(&MailSlotMsg[Index],&MsgSize,sizeof(SIZE_T));
    Index+= sizeof(SIZE_T);

    // data
    memcpy(&MailSlotMsg[Index],pMsg,MsgSize);
    // Index+= MsgSize;

    if (!this->pMailSlotClient->Write(MailSlotMsg,MailSlotMsgSize))
    {
        ReportError(_T("SendReplyToOverridingDllQuery Error.\r\nMailSlot write error"));
        LeaveCriticalSection(&this->CriticalSection);
        delete[] MailSlotMsg;
        return FALSE;
    }

    delete[] MailSlotMsg;
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: AllowTlsCallbackHooking
// Object: allow to start hooking at tls callback [DllMain like for exe] (if any) instead of application entry point
//         Applies to start process in suspended way
// Parameters :
//      in : BOOL bActiveMode : TRUE to go in monitoring file debug mode
//                              FALSE to go out of monitoring file debug mode
//      out: 
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::AllowTlsCallbackHooking(BOOL bAllow)
{
    this->bAllowTlsCallBackHook=bAllow;
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: SetMonitoringFileDebugMode
// Object: put APIOverride in monitoring file debug mode or not
//         When put in monitoring file debug mode, all logged are configured in InOut direction
//         and sent regardless filters
// Parameters :
//      in : BOOL bActiveMode : TRUE to go in monitoring file debug mode
//                              FALSE to go out of monitoring file debug mode
//      out: 
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetMonitoringFileDebugMode(BOOL bActiveMode)
{
    EnterCriticalSection(&this->CriticalSection);
    this->bMonitoringFileDebugMode=bActiveMode;
    if (this->bAPIOverrideDllLoaded)
    {
        STRUCT_COMMAND Cmd;
        Cmd.dwCommand_ID=CMD_MONITORING_FILE_DEBUG_MODE;
        Cmd.Param[0]=this->bMonitoringFileDebugMode;
        if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
        {
            ReportError(_T("Error modifying monitoring file debug mode.\r\nMailSlot write error"));
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: BreakDialogDontBreakApioverrideThreads
// Object: Allow to specify if Break dialog will allow execution of ApiOverride dll threads
// Parameters :
//      in : BOOL bDontBreak : TRUE to avoid breaking ApiOverride threads
//                             FALSE break ApiOverride threads
//      out: 
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::BreakDialogDontBreakApioverrideThreads(BOOL bDontBreak)
{
    EnterCriticalSection(&this->CriticalSection);
    this->bBreakDialogDontBreakApioverrideThreads=bDontBreak;
    if (this->bAPIOverrideDllLoaded)
    {
        STRUCT_COMMAND Cmd;
        Cmd.dwCommand_ID=CMD_BREAK_DONT_BREAK_APIOVERRIDE_THREADS;
        Cmd.Param[0]=this->bBreakDialogDontBreakApioverrideThreads;
        if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
        {
            ReportError(_T("Error modifying ApiOverride breaking threads state on Break dialog.\r\nMailSlot write error"));
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetMonitoringCallback
// Object: Let you manage yourself logging event
// Parameters :
//    in : - tagCallBackLogFunc pCallBackLogFunc : monitoring callback 
//          warning we use mail slot so callback can be called few seconds after real function call
//          for real time function hooking just use a dll (see fake API dll sample)
//          if you want to stop callback call, just call SetMonitoringCallback with a NULL parameter
//         - LPVOID pUserParam : parameter for the callback
//         - BOOL bManualFreeLogEntry : TRUE if you want to keep log in memory after callback has been called
//                                      else data of log structure will be free as soon as callback returns
//                                      To manually free memory of a log entry, call FreeLogEntry with the specified log entry
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetMonitoringCallback(tagCallBackLogFunc pCallBackLogFunc,LPVOID pUserParam,BOOL bManualFreeLogEntry)
{
    if (IsBadCodePtr((FARPROC)pCallBackLogFunc) && (pCallBackLogFunc!=0)) // pCallBackLogFunc==0 to remove callback
        return FALSE;

    this->pCallBackLogFunc=pCallBackLogFunc;
    this->pCallBackLogFuncUserParam=pUserParam;
    this->bManualFreeLogEntry=bManualFreeLogEntry;
    if (pCallBackLogFunc==NULL)
        this->bManualFreeLogEntry=FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetMonitoringListview
// Object: Listview will be configured automatically, and it will be field by monitoring events
//         you don't need to manage yourself logging events 
// Parameters :
//     in : HWND hListView: Handle to a list view
// Return : 
//-----------------------------------------------------------------------------
BOOL CApiOverride::SetMonitoringListview(HWND hListView)
{
    if (this->pInternalListview)
    {
        delete this->pInternalListview;
        this->pInternalListview=NULL;
    }

    if (hListView==NULL)
    {
        this->pListview=NULL;
        return TRUE;
    }

    this->pInternalListview=new CListview(hListView);
    this->SetMonitoringListview(this->pInternalListview);
    this->InitializeMonitoringListview();
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: SetMonitoringListview
// Object: Listview will be field by monitoring events
//         you don't need to manage yourself logging events
// Parameters :
//     in : CListview pListView: CListview object (warning it's not the MFC one)
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetMonitoringListview(CListview* pListView)
{
    this->pListview=pListView;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: InitializeMonitoringListview
// Object: initialize monitoring listview if set
// Return : 
//-----------------------------------------------------------------------------
BOOL CApiOverride::InitializeMonitoringListview()
{
    if (!this->pListview)
        return FALSE;

    this->pListview->SetStyle(TRUE,FALSE,FALSE,FALSE);
    this->pListview->SetView(LV_VIEW_DETAILS);
    this->pListview->EnableColumnSorting(TRUE);

    this->pListview->Clear();
    this->pListview->RemoveAllColumns();

    for (int cpt=FirstColumn;cpt<=LastColumn;cpt++)
    {
        switch(cpt)
        {
            case ColumnsIndexId:
                this->pListview->SetColumn(cpt,_T("Id"),40,LVCFMT_RIGHT);
                break;
            case ColumnsIndexDirection:
                this->pListview->SetColumn(cpt,_T("Dir"),30,LVCFMT_CENTER);
                break;
            case ColumnsIndexCall:
                this->pListview->SetColumn(cpt,_T("Call"),360,LVCFMT_LEFT);
                break;
            case ColumnsIndexReturnValue:
                this->pListview->SetColumn(cpt,_T("Ret Value"),80,LVCFMT_CENTER);
                break;
            case ColumnsIndexCallerAddress:
                this->pListview->SetColumn(cpt,_T("Caller Addr"),80,LVCFMT_CENTER);
                break;
            case ColumnsIndexCallerRelativeIndex:
                this->pListview->SetColumn(cpt,_T("Caller Relative Addr"),140,LVCFMT_LEFT);
                break;
            case ColumnsIndexProcessID:
                this->pListview->SetColumn(cpt,_T("ProcessID"),80,LVCFMT_CENTER);
                break;
            case ColumnsIndexThreadID:
                this->pListview->SetColumn(cpt,_T("ThreadID"),80,LVCFMT_CENTER);
                break;
            case ColumnsIndexModuleName:
                this->pListview->SetColumn(cpt,_T("Module Name"),200,LVCFMT_LEFT);
                break;
            case ColumnsIndexCallerFullPath:
                this->pListview->SetColumn(cpt,_T("Caller Full Path"),200,LVCFMT_LEFT);
                break;
            case ColumnsIndexAPIName:
                this->pListview->SetColumn(cpt,_T("API Name"),80,LVCFMT_LEFT);
                break;
            case ColumnsIndexLastError:
                this->pListview->SetColumn(cpt,_T("Last Error"),100,LVCFMT_CENTER);
                break;
            case ColumnsIndexRegistersBeforeCall:
                this->pListview->SetColumn(cpt,_T("Registers Before Call"),200,LVCFMT_LEFT);
                break;
            case ColumnsIndexRegistersAfterCall:
                this->pListview->SetColumn(cpt,_T("Registers After Call"),200,LVCFMT_LEFT);
                break;
            case ColumnsIndexFloatingReturnValue:
                this->pListview->SetColumn(cpt,_T("Floating Ret"),80,LVCFMT_CENTER);
                break;
            case ColumnsIndexCallTime:
                this->pListview->SetColumn(cpt,_T("Start Time"),120,LVCFMT_CENTER);
                break;
            case ColumnsIndexCallDuration:
                this->pListview->SetColumn(cpt,_T("Duration (us)"),80,LVCFMT_RIGHT);
                break;
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: LoadMonitoringFile
// Object: start monitoring API hooked by the file (multiple files can be hooked at the same time)
// Parameters :
//     in : TCHAR* pszFileName: api monitoring file
// Return : TRUE if file is partially loaded (even if some non fatal errors occurs)
//          and so needs a call to UnloadMonitoringFile to restore all hooked func,
//          FALSE if file not loaded at all
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::LoadMonitoringFile(TCHAR* pszFileName)
{
    EnterCriticalSection(&this->CriticalSection);
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    HANDLE pH[4]={this->hevtMonitoringFileLoaded,this->hevtError,this->hevtFreeProcess,this->hevtProcessFree};
    STRUCT_COMMAND Cmd;
    Cmd.dwCommand_ID=CMD_LOAD_MONITORING_FILE;
    _tcsncpy(Cmd.pszStringParam,pszFileName,MAX_PATH);
    // reset event in case of previous timeout
    ResetEvent(pH[0]);
    ResetEvent(pH[1]);
    if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
    {
        ReportError(_T("Load Monitoring File error writing in MailSlot client"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    DWORD dwRet;

WaitEvent:
    dwRet=WaitForMultipleObjects(4,pH,FALSE,TIME_REQUIERED_TO_LOAD);
    if (dwRet==WAIT_TIMEOUT)
    {
        if(this->UserMessage(_T("Timeout for loading. Do you want to wait more ?"),_T("Question"),MB_YESNO|MB_ICONQUESTION)==IDYES)
            goto WaitEvent;
        else
        {
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }
    }

    if (dwRet!=WAIT_OBJECT_0)
    {
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: UnloadMonitoringFile
// Object: stop monitoring API hooked by the file
// Parameters :
//     in : TCHAR* pszFileName: api monitoring file
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::UnloadMonitoringFile(TCHAR* pszFileName)
{
    EnterCriticalSection(&this->CriticalSection);
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    HANDLE pH[4]={this->hevtMonitoringFileUnloaded,this->hevtError,this->hevtFreeProcess,this->hevtProcessFree};
    STRUCT_COMMAND Cmd;
    Cmd.dwCommand_ID=CMD_UNLOAD_MONITORING_FILE;
    _tcsncpy(Cmd.pszStringParam,pszFileName,MAX_PATH);
    // reset event in case of previous timeout
    ResetEvent(pH[0]);
    ResetEvent(pH[1]);
    if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
    {
        ReportError(_T("Unload Monitoring File Error writing in MailSlot client"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    DWORD dwRet;

WaitEvent:
    dwRet=WaitForMultipleObjects(4,pH,FALSE,TIME_REQUIERED_TO_UNLOAD);
    if (dwRet==WAIT_TIMEOUT)
    {
        if(this->UserMessage(_T("Timeout for unloading. Do you want to wait more ?"),_T("Question"),MB_YESNO|MB_ICONQUESTION)==IDYES)
            goto WaitEvent;
        else
        {
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }
    }

    if (dwRet!=WAIT_OBJECT_0)
    {
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: LoadFakeAPI
// Object: load dll and start faking api/func specified in the specified in the dll
//         (see the Fake API sample for more infos on specifying API in dll)
//         multiple fake library can be hooked at the same time
// Parameters :
//     in : TCHAR* pszFileName: fake api dll name
// Return : TRUE if library is loaded (even if some non fatal errors occurs)
//          and so needs a call to UnloadFakeAPI to restore all hooked func,
//          FALSE if library not loaded 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::LoadFakeAPI(TCHAR* pszFileName)
{
    EnterCriticalSection(&this->CriticalSection);
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    HANDLE pH[4]={this->hevtFakeAPIDLLLoaded,this->hevtError,this->hevtFreeProcess,this->hevtProcessFree};
    STRUCT_COMMAND Cmd;
    Cmd.dwCommand_ID=CMD_LOAD_FAKE_API_DLL;
    _tcsncpy(Cmd.pszStringParam,pszFileName,MAX_PATH);
    // reset event in case of previous timeout
    ResetEvent(pH[0]);
    ResetEvent(pH[1]);
    if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
    {
        ReportError(_T("Load Fake API error writing in MailSlot client"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }
    DWORD dwRet;

WaitEvent:
    dwRet=WaitForMultipleObjects(4,pH,FALSE,TIME_REQUIERED_TO_LOAD);
    if (dwRet==WAIT_TIMEOUT)
    {
        if(this->UserMessage(_T("Timeout for loading. Do you want to wait more ?"),_T("Question"),MB_YESNO|MB_ICONQUESTION)==IDYES)
            goto WaitEvent;
        else
        {
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }
    }

    if (dwRet!=WAIT_OBJECT_0)
    {
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: UnloadFakeAPI
// Object: stop faking api/func hooked by the dll before unloading this dll
// Parameters :
//     in : TCHAR* pszFileName: fake api dll name
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::UnloadFakeAPI(TCHAR* pszFileName)
{
    EnterCriticalSection(&this->CriticalSection);
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    HANDLE pH[4]={this->hevtFakeAPIDLLUnloaded,this->hevtError,this->hevtFreeProcess,this->hevtProcessFree};
    STRUCT_COMMAND Cmd;
    Cmd.dwCommand_ID=CMD_UNLOAD_FAKE_API_DLL;
    _tcsncpy(Cmd.pszStringParam,pszFileName,MAX_PATH);
    // reset event in case of previous timeout
    ResetEvent(pH[0]);
    ResetEvent(pH[1]);
    if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
    {
        ReportError(_T("Unload Fake API error writing in MailSlot client"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    DWORD dwRet;

WaitEvent:
    dwRet=WaitForMultipleObjects(4,pH,FALSE,TIME_REQUIERED_TO_UNLOAD);
    if (dwRet==WAIT_TIMEOUT)
    {
        if(this->UserMessage(_T("Timeout for unloading. Do you want to wait more ?"),_T("Question"),MB_YESNO|MB_ICONQUESTION)==IDYES)
            goto WaitEvent;
        else
        {
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }
    }

    if (dwRet!=WAIT_OBJECT_0)
    {
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: StartMonitoring
// Object: restore monitoring until the next StopMonitoring
//         API Override dll do monitoring by default (at start up)
//         So you only need to call this function after a StopMonitoring call
// Parameters :
//     in : 
// Return : TRUE on Success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::StartMonitoring()
{
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        return FALSE;
    }
    SetEvent(this->hevtStartMonitoring);
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: StopMonitoring
// Object: Temporary stop monitoring until the next StartMonitoring call
// Parameters :
//     in : 
// Return : TRUE on Success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::StopMonitoring()
{
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        return FALSE;
    }
    SetEvent(this->hevtStopMonitoring);
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: StartFaking
// Object: restore faking until the next StopFaking
//         API Override dll do faking by default (at start up)
//         So you only need to call this function after a StopFaking call
// Parameters :
//     in : 
// Return : TRUE on Success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::StartFaking()
{
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        return FALSE;
    }
    SetEvent(this->hevtStartFaking);
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: StopFaking
// Object: Temporary stop faking until the next StartFaking call
// Parameters :
//     in : 
// Return : TRUE on Success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::StopFaking()
{
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        return FALSE;
    }
    SetEvent(this->hevtStopFaking);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetMonitoringModuleFiltersState
// Object: enable or disable filters for monitoring
// Parameters :
//     in : BOOL bEnable : TRUE to enable filters, FALSE to disable them
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetMonitoringModuleFiltersState(BOOL bEnable)
{
    EnterCriticalSection(&this->CriticalSection);
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    STRUCT_COMMAND Cmd;
    if (bEnable)
        Cmd.dwCommand_ID=CMD_ENABLE_MODULE_FILTERS_FOR_MONITORING;
    else
        Cmd.dwCommand_ID=CMD_DISABLE_MODULE_FILTERS_FOR_MONITORING;
    if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
    {
        ReportError(_T("Error setting monitoring filters state.\r\nMailSlot write error"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetFakingModuleFiltersState
// Object: enable or disable filters for faking
// Parameters :
//     in : BOOL bEnable : TRUE to enable filters, FALSE to disable them
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetFakingModuleFiltersState(BOOL bEnable)
{
    EnterCriticalSection(&this->CriticalSection);
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    STRUCT_COMMAND Cmd;
    if (bEnable)
        Cmd.dwCommand_ID=CMD_ENABLE_MODULE_FILTERS_FOR_FAKING;
    else
        Cmd.dwCommand_ID=CMD_DISABLE_MODULE_FILTERS_FOR_FAKING;
    if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
    {
        ReportError(_T("Error setting Overriding filters state.\r\nMailSlot write error"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: LogOnlyBaseModule
// Object: Allow to log only base module or all modules
// Parameters :
//     in : BOOL bOnlyBaseModule : TRUE to log only base module
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::LogOnlyBaseModule(BOOL bOnlyBaseModule)
{
    EnterCriticalSection(&this->CriticalSection);
    this->bOnlyBaseModule=bOnlyBaseModule;

    if (this->bAPIOverrideDllLoaded)
    {
        STRUCT_COMMAND Cmd;
        if (bOnlyBaseModule)
            Cmd.dwCommand_ID=CMD_START_LOG_ONLY_BASE_MODULE;
        else
            Cmd.dwCommand_ID=CMD_STOP_LOG_ONLY_BASE_MODULE;
        if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
        {
            ReportError(_T("Error setting base module logging option.\r\nMailSlot write error"));
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
        }
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Dump
// Object: query the dump interface of the hooked process
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::Dump()
{
    EnterCriticalSection(&this->CriticalSection);
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }
    STRUCT_COMMAND Cmd;
    Cmd.dwCommand_ID=CMD_DUMP;
    if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
    {
        ReportError(_T("Error querying dump.\r\nMailSlot write error"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: AddNotLoggedModuleListParsLineStatic
// Object: static parse line callback of a Module List file
// Parameters :
//     in : TCHAR* FileName : name of file beeing parsed
//          TCHAR* pszLine : line content
//          DWORD dwLineNumber : line number
//          LPVOID UserParam : CApiOverride* object on which to apply changes
// return : TRUE to continue parsing, FALSE to stop it
//-----------------------------------------------------------------------------
BOOL CApiOverride::AddModuleListParseLineStatic(TCHAR* FileName,TCHAR* pszLine,DWORD dwLineNumber,LPVOID UserParam)
{
    if (IsBadReadPtr(UserParam,sizeof(CApiOverride)))
        return TRUE;
    // re enter object model
    ((CApiOverride*)UserParam)->FilterModuleListParseLine(FileName,pszLine,dwLineNumber,TRUE);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RemoveFromFiltersModuleListParseLineStatic
// Object: static parse line callback of a Module List file
// Parameters :
//     in : TCHAR* FileName : name of file beeing parsed
//          TCHAR* pszLine : line content
//          DWORD dwLineNumber : line number
//          LPVOID UserParam : CApiOverride* object on which to apply changes
// return : TRUE to continue parsing, FALSE to stop it
//-----------------------------------------------------------------------------
BOOL CApiOverride::RemoveModuleListParseLineStatic(TCHAR* FileName,TCHAR* pszLine,DWORD dwLineNumber,LPVOID UserParam)
{
    if (IsBadReadPtr(UserParam,sizeof(CApiOverride)))
        return TRUE;
    // re enter object model
    ((CApiOverride*)UserParam)->FilterModuleListParseLine(FileName,pszLine,dwLineNumber,FALSE);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: FilterModuleListParseLine
// Object: parse line of a Module List file
//          and set each module state depending of AddToNotLogged
// Parameters :
//     in : TCHAR* FileName : name of file beeing parsed
//          TCHAR* pszLine : line content
//          DWORD dwLineNumber : line number
//          BOOL AddToList : TRUE to add to filter list, FALSE to remove from filter list
//-----------------------------------------------------------------------------
void CApiOverride::FilterModuleListParseLine(TCHAR* FileName,TCHAR* pszLine,DWORD dwLineNumber,BOOL AddToList)
{
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(dwLineNumber);
    TCHAR pszFileName[MAX_PATH];
    TCHAR pszPath[MAX_PATH];
    *pszPath=0;

    CTrimString::TrimString(pszLine);
    // empty line
    if (*pszLine==0)
        return;
    // comment line
    if (*pszLine==';')
        return;

    // check application path directory
    if (_tcschr(pszLine,'\\')==0)
    {
        // we don't get full path, so add current exe path
        _tcscpy(pszFileName,this->pszAppPath);
        _tcscat(pszFileName,pszLine);
    }

    // check <windir> flag
    else if (_tcsnicmp(pszLine,_T("<windir>"),8)==0)
    {
        SHGetFolderPath(NULL,CSIDL_WINDOWS,NULL,SHGFP_TYPE_CURRENT,pszPath);

        _tcscpy(pszFileName,pszPath);
        _tcscat(pszFileName,&pszLine[8]);
    }

    // check <system> flag
    else if (_tcsnicmp(pszLine,_T("<system>"),8)==0)
    {
        SHGetFolderPath(NULL,CSIDL_SYSTEM,NULL,SHGFP_TYPE_CURRENT,pszPath);

        _tcscpy(pszFileName,pszPath);
        _tcscat(pszFileName,&pszLine[8]);
    }

    // check <ProgramFiles> flag
    else if (_tcsnicmp(pszLine,_T("<ProgramFiles>"),14)==0)
    {
        SHGetFolderPath(NULL,CSIDL_PROGRAM_FILES,NULL,SHGFP_TYPE_CURRENT,pszPath);

        _tcscpy(pszFileName,pszPath);
        _tcscat(pszFileName,&pszLine[14]);
    }

    // check <ProgramFilesCommon> flag
    else if (_tcsnicmp(pszLine,_T("<ProgramFilesCommon>"),20)==0)
    {
        SHGetFolderPath(NULL,CSIDL_PROGRAM_FILES_COMMON,NULL,SHGFP_TYPE_CURRENT,pszPath);

        _tcscpy(pszFileName,pszPath);
        _tcscat(pszFileName,&pszLine[20]);
    }

    // check <TargetDir> flag
    else if (_tcsnicmp(pszLine,_T("<TargetDir>"),11)==0)
    {
        _tcscpy(pszFileName,this->ProcessPath);
        _tcscat(pszFileName,&pszLine[11+1]);// +1 as this->ProcessPath contains '\'
    }

    else
        // we get full path only copy file name
        _tcscpy(pszFileName,pszLine);

    // don't check that module exists to allow the use of '*' and '?' jokers in name

    // set the log state of the module depending ShouldBeLogged
    this->SetModuleLogState(pszFileName,AddToList);
}

//-----------------------------------------------------------------------------
// Name: AddToFiltersModuleList
// Object: add all modules of a Module List file to filtering modules list
//         use it both for only hooked filters or not hooked
// Parameters :
//     in : TCHAR* FileName : Module List file 
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::AddToFiltersModuleList(TCHAR* FileName)
{
    return CTextFile::ParseLines(FileName,this->hevtFreeProcess,CApiOverride::AddModuleListParseLineStatic,this);
}

//-----------------------------------------------------------------------------
// Name: RemoveFromFiltersModuleList
// Object: remove all modules of a Module List file from filtering modules list
//         use it both for only hooked filters or not hooked
// Parameters :
//     in : TCHAR* FileName : Module List file 
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::RemoveFromFiltersModuleList(TCHAR* FileName)
{
    return CTextFile::ParseLines(FileName,this->hevtFreeProcess,CApiOverride::RemoveModuleListParseLineStatic,this);
}

//-----------------------------------------------------------------------------
// Name: ClearFiltersModuleList
// Object: clear the not logged modules list --> all modules will be logged
// Parameters :
//     in : 
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::ClearFiltersModuleList()
{
    EnterCriticalSection(&this->CriticalSection);
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    STRUCT_COMMAND Cmd;
    Cmd.dwCommand_ID=CMD_CLEAR_LOGGED_MODULE_LIST_FILTERS;
    if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
    {
        ReportError(_T("Error clearing filters module list.\r\nMailSlot write error"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ClearUserDataTypeCache
// Object: clear User data type cache. Must be called after a file change in UserTypes 
//         directory to be up to date
// Parameters :
//     in : 
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::ClearUserDataTypeCache()
{
    EnterCriticalSection(&this->CriticalSection);
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    STRUCT_COMMAND Cmd;
    Cmd.dwCommand_ID=CMD_CLEAR_USER_DATA_TYPE_CACHE;
    if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
    {
        ReportError(_T("Error clearing user data type cache.\r\nMailSlot write error"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetModuleLogState
// Object: Allow to log or stop logging calls done by modules
// Parameters :
//     in : TCHAR* pszModuleFullPath : full path of the module
//          BOOL bLog : TRUE to log the specified module
//                      FALSE to stop logging the specified module
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetModuleLogState(TCHAR* pszModuleFullPath,BOOL bLog)
{
    EnterCriticalSection(&this->CriticalSection);
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    STRUCT_COMMAND Cmd;
    if (bLog)
        Cmd.dwCommand_ID=CMD_START_MODULE_LOGGING;
    else
        Cmd.dwCommand_ID=CMD_STOP_MODULE_LOGGING;
    _tcsncpy(Cmd.pszStringParam,pszModuleFullPath,MAX_PATH);
    if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
    {
        ReportError(_T("Error setting module logging option.\r\nMailSlot write error"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetModuleFilteringWay
// Object: Set modules filtering way (inclusion or exclusion)
// Parameters :
//     in : 
//          tagFilteringWay FilteringWay : FILTERING_WAY_ONLY_SPECIFIED_MODULES
//                                         or FILTERING_WAY_NOT_SPECIFIED_MODULES
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetModuleFilteringWay(tagFilteringWay FilteringWay)
{
    EnterCriticalSection(&this->CriticalSection);
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    STRUCT_COMMAND Cmd;
    Cmd.dwCommand_ID=CMD_SET_LOGGED_MODULE_LIST_FILTERS_WAY;
    Cmd.Param[0]=(DWORD)FilteringWay;
    if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
    {
        ReportError(_T("Error setting filtering option.\r\nMailSlot write error"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetNotLoggedModuleList
// Object: Allow to retrieve a list of not loaded modules
// Parameters :
//     out : TCHAR*** pArrayNotLoggedModulesNames : pointer filled by a TCHAR[*pdwArrayNotLoggedModulesNamesSize][MAX_PATH]
//                                                 MUST BE FREE if *pdwArrayNotLoggedModulesNamesSize>0 by delete[] *pArrayNotLoggedModulesNames;
//                      sample of use
//                                TCHAR** pNotLoggedArray=NULL;
//                                DWORD dwNbNotLoggedModules=0;
//                                GetNotLoggedModuleList(&pNotLoggedArray,&dwNbNotLoggedModules);
//                                if (pNotLoggedArray) //  in case of dwNbNotLoggedModules==0
//                                        delete[] pNotLoggedArray;
//           DWORD* pdwArrayNotLoggedModulesNamesSize : number of module names
// Return : FALSE on error, TRUE on success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::GetNotLoggedModuleList(TCHAR*** pArrayNotLoggedModulesNames,DWORD* pdwArrayNotLoggedModulesNamesSize)
{
    // check parameters
    if (IsBadWritePtr(pArrayNotLoggedModulesNames,sizeof(TCHAR**))
        ||IsBadWritePtr(pdwArrayNotLoggedModulesNamesSize,sizeof(DWORD)))
        return FALSE;

    // init vars in case of failure
    *pdwArrayNotLoggedModulesNamesSize=0;
    *pArrayNotLoggedModulesNames=NULL;


    EnterCriticalSection(&this->CriticalSection);

    // assume dll is loaded
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    // reset events
    ResetEvent(this->hevtGetNotLoggedModulesReply);
    ResetEvent(this->hevtError);

    // send query command
    STRUCT_COMMAND Cmd;
    Cmd.dwCommand_ID=CMD_NOT_LOGGED_MODULE_LIST_QUERY;
    if (!this->pMailSlotClient->Write(&Cmd,sizeof(STRUCT_COMMAND)))
    {
        ReportError(_T("Error setting module logging option.\r\nMailSlot write error"));
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    // wait until list reply or error
    HANDLE ph[2]={this->hevtGetNotLoggedModulesReply,this->hevtError};
    if (WaitForMultipleObjects(2,ph,FALSE,APIOVERRIDE_CMD_REPLY_MAX_TIME_IN_MS)!=WAIT_OBJECT_0)
    {
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    // fill returned vars
    *pdwArrayNotLoggedModulesNamesSize=this->NotLoggedModulesArraySize;
    *pArrayNotLoggedModulesNames=(TCHAR**)this->NotLoggedModulesArray;

    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Stop
// Object: stop monitoring and faking and eject all dll of the current used process
// Parameters :
//     in : 
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::Stop()
{
    BOOL bRet;
    EnterCriticalSection(&this->CriticalSection);
    bRet=this->StopWithoutEnteringCriticalSection();
    LeaveCriticalSection(&this->CriticalSection);
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: StopWithoutEnteringCriticalSection
// Object: stop monitoring and faking and eject all dll of the current used process
//          WARNING function is not protected by critical section, the caller is responsible
//          of taking critical section
// Parameters :
//     in : 
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL CApiOverride::StopWithoutEnteringCriticalSection()
{
    return this->StopWithoutEnteringCriticalSection(FALSE);
}

//-----------------------------------------------------------------------------
// Name: StopWithoutEnteringCriticalSection
// Object: stop monitoring and faking and eject all dll of the current used process
//          WARNING function is not protected by critical section, the caller is responsible
//          of taking critical section
// Parameters :
//     in : BOOL bCalledByhThreadWatchingEvents : TRUE if called inside thread hThreadWatchingEvents
//                                                flag to avoid deadlocks
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL CApiOverride::StopWithoutEnteringCriticalSection(BOOL bCalledByhThreadWatchingEvents)
{
    TCHAR psz[2*MAX_PATH];
    int iMsgBoxRes=IDYES;
    DWORD dwRes;

    // assume Stop function is not called simultaneously by multiple threads
    WaitForSingleObject(this->hStopUnlocked,INFINITE);

    // reset event of injected dll 
    ResetEvent(this->hevtProcessFree);

    // ask the injected dll to stop its job / or 
    SetEvent(this->hevtFreeProcess);

    // avoid to wait if hooked process has crashed or has been killed
    if(CProcessHelper::IsAlive(this->dwCurrentProcessId))
    {
        // if api override is loaded wait for its hevtProcessFree event
        if (this->bAPIOverrideDllLoaded)
        {
            while(iMsgBoxRes==IDYES)
            {
                
                dwRes=WaitForSingleObject(this->hevtProcessFree,TIME_REQUIERED_TO_UNLOAD);
                
                switch(dwRes)
                {
                case WAIT_OBJECT_0:
                case WAIT_FAILED:
                    // end while loop
                    iMsgBoxRes=IDNO;
                    break;
                default:
                    // put the user aware and hope he will do actions that unlock fake api
                    // a good sample of blocking fake api are MessageBox
                    _stprintf(psz,
                            _T("Warning %s seems to be still not unloaded from host process 0x%.8X"),
                            API_OVERRIDE_DLL_NAME,
                            this->dwCurrentProcessId);
                    if (this->ProcessName)
                    {
                        _tcscat(psz,_T(" ("));
                        _tcscat(psz,this->ProcessName);
                        _tcscat(psz,_T(")"));
                    }
                    _tcscat(psz,_T("\r\nAssume that Overrided API are not in a blocking state.\r\nDo you want to wait more time ?"));
                    iMsgBoxRes=MessageBox(this->hParentWindow,psz,_T("Warning"),MB_YESNO|MB_ICONWARNING);
                    break;
                }
            }

            // eject apioverride DLL
            if (this->LoadInjectLibrary())
            {
                _tcscpy(psz,this->pszAppPath);
                _tcscat(psz,API_OVERRIDE_DLL_NAME);
                this->pEjectLib(this->dwCurrentProcessId,psz);
            }
        }
    }

    // if watching thread events has been launched
    if (this->hThreadWatchingEvents)
    {
        // if Stop not called internally by hThreadWatchingEvents
        if (!bCalledByhThreadWatchingEvents)
        {
            // wait the end of watching thread events
            // to assume that DllUnloadedCallBack has finished to be executed
            // try clean thread end
            if (::WaitForSingleObject(this->hThreadWatchingEvents,TIME_REQUIERED_TO_UNLOAD) != WAIT_OBJECT_0) // as hevtFreeProcess has been set it should terminate quickly
                // force thread to terminate
                ::TerminateThread(this->hThreadWatchingEvents,0xFFFFFFFC); // avoid ThreadWatchingEvents crash accessing invalid memory
        }
    }

    if (this->pMailSlotServer)
    {
        delete this->pMailSlotServer;
        this->pMailSlotServer=NULL;
    }
    if (this->pMailSlotClient)
    {
        delete this->pMailSlotClient;
        this->pMailSlotClient=NULL;
    }

    // close handle associated with thread
    CleanCloseHandle(&this->hThreadWatchingEvents);
    CleanCloseHandle(&this->hThreadLogging);

    // close events handle
    CleanCloseHandle(&this->hevtStartMonitoring);
    CleanCloseHandle(&this->hevtStopMonitoring);
    CleanCloseHandle(&this->hevtStartFaking);
    CleanCloseHandle(&this->hevtStopFaking);
    CleanCloseHandle(&this->hevtFreeProcess);
    CleanCloseHandle(&this->hevtTlsHookEndLoop);

    CleanCloseHandle(&this->hevtAPIOverrideDllProcessAttachCompleted);
    CleanCloseHandle(&this->hevtAPIOverrideDllProcessDetachCompleted);
    CleanCloseHandle(&this->hevtProcessFree);
    CleanCloseHandle(&this->hevtMonitoringFileLoaded);
    CleanCloseHandle(&this->hevtMonitoringFileUnloaded);
    CleanCloseHandle(&this->hevtFakeAPIDLLLoaded);
    CleanCloseHandle(&this->hevtFakeAPIDLLUnloaded);
    CleanCloseHandle(&this->hevtError);
    CleanCloseHandle(&this->hevtClientMailslotOpen);
    
    // set unloaded state
    this->bAPIOverrideDllLoaded=FALSE;

    // unlock calls to Stop
    SetEvent(this->hStopUnlocked);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Start
// Object: inject API Override dll in selected process ID to allow monitoring and faking
// Parameters :
//     in : DWORD dwPID : PID of process fully loaded. If Nt loader don't have finished to load process
//                        this func will probably failed
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::Start(DWORD dwPID)
{
    return this->Start(dwPID,NULL);
}


//-----------------------------------------------------------------------------
// Name: Start
// Object: inject API Override dll in selected process ID to allow monitoring and faking
// Parameters :
//     in : DWORD dwPID : PID of process fully loaded. If Nt loader don't have finished to load process
//                        this func will probably failed
//          HANDLE hSuspendedThread : suspended application thread. Can be NULL
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL CApiOverride::Start(DWORD dwPID,HANDLE hSuspendedThread)
{
    BOOL bRet;

    EnterCriticalSection(&this->CriticalSection);
    bRet=StartWithoutEnteringCriticalSection(dwPID,hSuspendedThread);
    LeaveCriticalSection(&this->CriticalSection);

    return bRet;
}
//-----------------------------------------------------------------------------
// Name: StartWithoutEnteringCriticalSection
// Object: inject API Override dll in selected process ID to allow monitoring and faking
//          WARNING function is not protected by critical section, the caller is responsible
//          of taking critical section
// Parameters :
//     in : DWORD dwPID : PID of process fully loaded. If Nt loader don't have finished to load process
//                        this func will probably failed
//          HANDLE hSuspendedThread : suspended application thread. Can be NULL
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL CApiOverride::StartWithoutEnteringCriticalSection(DWORD dwPID,HANDLE hSuspendedThread)
{
    if(!this->InitializeStart(dwPID))
        return FALSE;
   
    BOOL ApiOverrideDllLoadedByHookNet=FALSE;

    if (CApiOverridebNetProfilingEnabled)
    {
        CPE Pe(this->ProcessFullPathName);
        Pe.Parse();
        // check if .Net application
        if (Pe.IsNET())
        {
            BOOL bError=FALSE;
            int CurrentPriority;

            CurrentPriority=GetThreadPriority(GetCurrentThread());
            SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
            
            if (hSuspendedThread)
                // resume suspended thread
                ResumeThread(hSuspendedThread);

            /////////////////////////////////////////
            // wait CorInitialize event
            /////////////////////////////////////////
            TCHAR pszPID[50];
            TCHAR psz[MAX_PATH+50];
            // pid -> string
            _stprintf(pszPID,_T("0x%X"),this->dwCurrentProcessId);
            // create a unique event name
            _tcscpy(psz,APIOVERRIDE_EVENT_HOOKNET_STARTED);
            _tcscat(psz,pszPID);

            HANDLE hevtHookNetReady=OpenEvent(EVENT_ALL_ACCESS,FALSE,psz);

            // wait until hook net dll is loaded
            DWORD OriginalTickCount=GetTickCount();
            DWORD CurrentTickCount;
            while(hevtHookNetReady==NULL)
            {
                Sleep(HOOK_END_POOLING_IN_MS);
                CurrentTickCount=GetTickCount();
                if (OriginalTickCount+5000<CurrentTickCount)
                {
                    bError=TRUE;
                    MessageBox(NULL,_T(".Net hooking error. HookNet dll don't become ready"),_T("Error"),MB_OK|MB_ICONERROR);
                    goto NetEnd;
                }

                hevtHookNetReady=OpenEvent(EVENT_ALL_ACCESS,FALSE,psz);
            }

            // wait Hooknet dll init
            HANDLE pH[4];
            DWORD dwRet;

            // define events
            pH[0]=hevtHookNetReady;
            pH[1]=this->hevtError;
            pH[2]=this->hevtFreeProcess;
            pH[3]=this->hevtProcessFree;

            // library is supposed to be loaded now
            this->bAPIOverrideDllLoaded=TRUE;

            // wait for Ready to Work event
            dwRet=WaitForMultipleObjects(4,pH,FALSE,TIME_REQUIERED_TO_LOAD);
            switch(dwRet)
            {
            case WAIT_OBJECT_0:   // all is ok
                break;
            case WAIT_TIMEOUT:    // time out
            case WAIT_OBJECT_0+1: // error dll is no more loaded
            case WAIT_OBJECT_0+2: 
            case WAIT_OBJECT_0+3: 
            default:// wait failed
                bError=TRUE;
                MessageBox(NULL,_T(".Net hooking error. HookNet dll wasn't loaded"),_T("Error"),MB_OK|MB_ICONERROR);
                goto NetEnd;
            }
            CloseHandle(hevtHookNetReady);

            // if apioverride dll has been successfully loaded
            if (WaitForSingleObject(this->hevtAPIOverrideDllProcessAttachCompleted,0)==WAIT_OBJECT_0)
            {
                // hooknet dll has loded apioverride dll
                ApiOverrideDllLoadedByHookNet=TRUE;

                if (hSuspendedThread)
                    // suspend thread
                    SuspendThread(hSuspendedThread);
            }
NetEnd:
            // restore original thread priority
            SetThreadPriority(GetCurrentThread(),CurrentPriority);

            if (bError)
            {
                this->bAPIOverrideDllLoaded=FALSE;
                this->StopWithoutEnteringCriticalSection();
                return FALSE;
            }

            if (ApiOverrideDllLoadedByHookNet)
            {
                if(!this->OpenClientMailSlot())
                    return FALSE;
            }
        }
    }

    if (!ApiOverrideDllLoadedByHookNet)
    {
        if (hSuspendedThread)
            // resume suspended thread
            ResumeThread(hSuspendedThread);

        // do apioverride dll injection
        if(!this->InjectDllByCreateRemoteThread(dwPID))
            return FALSE;

        // wait for load events
        if (!this->WaitForInjectedDllToBeLoaded())
            return FALSE;

        if (hSuspendedThread)
            // suspend thread
            SuspendThread(hSuspendedThread);

        if(!this->OpenClientMailSlot())
            return FALSE;
    }

    // send settings
    this->SetInitialOptions();

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: InjectDllByCreateRemoteThread
// Object: inject API Override dll in selected process ID to allow monitoring and faking
// Parameters :
//     in : DWORD dwPID : PID of process fully loaded. If Nt loader don't have finished to load process
//                        this func will probably failed
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL CApiOverride::InjectDllByCreateRemoteThread(DWORD dwPID)
{
    if (!this->LoadInjectLibrary())
        return FALSE;

    TCHAR psz[MAX_PATH];
    TCHAR pszMsg[MAX_PATH];
    
    // make injected full path
    _tcscpy(psz,this->pszAppPath);
    _tcscat(psz,API_OVERRIDE_DLL_NAME);

    // reset load events
    this->ResetInjectedDllLoadEvents();

    // try to inject library
    if (!this->pInjectLib(dwPID,psz))
    {
        _sntprintf(pszMsg,MAX_PATH,_T("Error injecting library %s"),API_OVERRIDE_DLL_NAME);
        ReportError(pszMsg);
        this->StopWithoutEnteringCriticalSection();
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetInitialOptions
// Object: Set options after successful dll loading
//          WARNING function is not protected by critical section, the caller is responsible
//          of taking critical section
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
void CApiOverride::SetInitialOptions()
{
    this->SetAutoAnalysis(this->AutoAnalysis);
    this->LogOnlyBaseModule(this->bOnlyBaseModule);
    this->SetCallStackRetrieval(this->bLogCallStack,this->CallStackEbpRetrievalSize);
    this->BreakDialogDontBreakApioverrideThreads(this->bBreakDialogDontBreakApioverrideThreads);
    this->SetMonitoringFileDebugMode(this->bMonitoringFileDebugMode);

    // COM
    this->SetCOMOptions(&this->ComHookingOptions);
    if (this->bComAutoHookingEnabled)
    {
        // set com hooking options before starting com monitoring
        this->EnableCOMAutoHooking(this->bComAutoHookingEnabled,FALSE);
    }

    // .NET
    if (CApiOverridebNetProfilingEnabled)
    {
        this->SetNetOptions(&this->NetHookingOptions);
        // set net hooking options before starting net monitoring 
        this->EnableNetAutoHooking(this->bNetAutoHookingEnabled,FALSE);
    }
}

//-----------------------------------------------------------------------------
// Name: ResetInjectedDllLoadEvents
// Object: Reset event before a call to WaitForInjectedDllToBeLoaded
//          call order 1) ResetInjectedDllLoadEvents
//                     2) Do an action that loads the dll in remote process
//                     3) call WaitForInjectedDllToBeLoaded
// Parameters :
//     in : 
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
void CApiOverride::ResetInjectedDllLoadEvents()
{
    ResetEvent(this->hevtAPIOverrideDllProcessAttachCompleted);
    ResetEvent(this->hevtClientMailslotOpen);
    ResetEvent(this->hevtError);
    ResetEvent(this->hevtFreeProcess);
    ResetEvent(this->hevtProcessFree);
}
//-----------------------------------------------------------------------------
// Name: WaitForInjectedDllToBeLoaded
// Object: wait for the injected library to become ready
//          WARNING function is not protected by critical section, the caller is responsible
//          of taking critical section
// Parameters :
//     in : 
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL CApiOverride::WaitForInjectedDllToBeLoaded()
{
    TCHAR pszMsg[MAX_PATH];
    HANDLE pH[4];
    DWORD dwRet;

    // define events
    pH[0]=this->hevtAPIOverrideDllProcessAttachCompleted;
    pH[1]=this->hevtError;
    pH[2]=this->hevtFreeProcess;
    pH[3]=this->hevtProcessFree;

    // library is supposed to be loaded now
    this->bAPIOverrideDllLoaded=TRUE;

WaitEvent:
    // wait for Ready to Work event
    dwRet=WaitForMultipleObjects(4,pH,FALSE,TIME_REQUIERED_TO_LOAD);
    switch(dwRet)
    {
        case WAIT_TIMEOUT:    // time out
            _sntprintf(pszMsg,MAX_PATH,_T("Error %s don't become ready\r\nDo you want to wait more ?"),API_OVERRIDE_DLL_NAME);
            if (this->UserMessage(pszMsg,_T("Question"),MB_YESNO|MB_ICONQUESTION)==IDYES)
                goto WaitEvent;
            this->StopWithoutEnteringCriticalSection();
            return FALSE;
        case WAIT_OBJECT_0:   // all is ok
            break;
        case WAIT_OBJECT_0+1: // error dll is no more loaded
            this->bAPIOverrideDllLoaded=FALSE;
            this->StopWithoutEnteringCriticalSection();
            return FALSE;
        case WAIT_OBJECT_0+2: 
        case WAIT_OBJECT_0+3: 
            return FALSE;
        default:// wait failed
            this->StopWithoutEnteringCriticalSection();
            return FALSE;
    }

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: OpenClientMailSlot
// Object: open mailslot from current application to apioverride.dll
//          WARNING function is not protected by critical section, the caller is responsible
//          of taking critical section
// Parameters :
//     in : 
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL CApiOverride::OpenClientMailSlot()
{
    TCHAR psz[MAX_PATH];
    // start mailslot client for giving instructions (it can't be open before the Ready to Work event)
    TCHAR pszPID[32];
    // pid -> string
    _stprintf(pszPID,_T("0x%X"),this->dwCurrentProcessId);
    _tcscpy(psz,APIOVERRIDE_MAILSLOT_FROM_INJECTOR);
    _tcscat(psz,pszPID);
    if (this->pMailSlotClient)
        this->pMailSlotClient->Close();
    else
        this->pMailSlotClient=new CMailSlotClient(psz);

    if (::WaitForSingleObject(this->hevtClientMailslotOpen,10000) != WAIT_OBJECT_0)
    {
        MessageBox(this->hParentWindow,_T("Remote mailslot not ready"),_T("Error"),MB_OK|MB_ICONERROR);
        return FALSE;
    }

    if (!pMailSlotClient->Open())
    {
        ReportError(_T("Can't open MailSlot"));
        this->StopWithoutEnteringCriticalSection();
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: FillPorcessNameFromPID
// Object: get names from PID
// Parameters :
//     in : DWORD dwPID
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL CApiOverride::FillPorcessNameFromPID(DWORD dwPID)
{
    if (*this->ProcessFullPathName)
        return TRUE;
    *this->ProcessName=0;
    *this->ProcessFullPathName=0;
    *this->ProcessPath=0;

    if (!CProcessName::GetProcessName(dwPID,this->ProcessFullPathName))
        return FALSE;

    _tcscpy(this->ProcessName,CStdFileOperations::GetFileName(this->ProcessFullPathName));
    CStdFileOperations::GetFilePath(this->ProcessFullPathName,this->ProcessPath,MAX_PATH);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: InitializeStart
// Object: initialize all named event and mailslots
// Parameters :
//     in : DWORD dwPID
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL CApiOverride::InitializeStart(DWORD dwPID)
{
    TCHAR psz[MAX_PATH];
    TCHAR pszMsg[MAX_PATH];
    TCHAR pszPID[32];

    // store pid
    this->dwCurrentProcessId=dwPID;

    this->FillPorcessNameFromPID(dwPID);

    // check if we are started
    if (this->bAPIOverrideDllLoaded)
        this->StopWithoutEnteringCriticalSection();

    // check if injlib dll is loaded
    if (!this->LoadInjectLibrary())
    {
        _sntprintf(pszMsg,MAX_PATH,_T("Error loading %s. Make sure file exists and restart application"),INJECTLIB_DLL_NAME);
        ReportError(pszMsg);
        return FALSE;
    }

    // check if apioverride dll exists avoid lost of time
    _tcscpy(psz,this->pszAppPath);
    _tcscat(psz,API_OVERRIDE_DLL_NAME);

    if (!CStdFileOperations::DoesFileExists(psz))
    {
        _sntprintf(pszMsg,MAX_PATH,_T("Error %s not found. Make sure file exists and restart application"),API_OVERRIDE_DLL_NAME);
        ReportError(pszMsg);
        return FALSE;
    }
   
    // pid -> string
    _stprintf(pszPID,_T("0x%X"),dwPID);

    // create interprocess communication events

    //HANDLE CreateEvent(
    //    LPSECURITY_ATTRIBUTES lpEventAttributes,
    //    BOOL bManualReset,
    //    BOOL bInitialState,
    //    LPCTSTR lpName
    //    );

    // set event accessible for all account. Doing this allow to event to be opened by other user
    // --> we can inject dll into processes running under other users accounts
    SECURITY_DESCRIPTOR sd={0};
    sd.Revision=SECURITY_DESCRIPTOR_REVISION;
    sd.Control=SE_DACL_PRESENT;
    sd.Dacl=NULL; // assume everyone access
    SECURITY_ATTRIBUTES SecAttr={0};
    SecAttr.bInheritHandle=FALSE;
    SecAttr.nLength=sizeof(SECURITY_ATTRIBUTES);
    SecAttr.lpSecurityDescriptor=&sd;


    //(Injector -> APIOverride.dll)
    _tcscpy(psz,APIOVERRIDE_EVENT_START_MONITORING);
    _tcscat(psz,pszPID);
    
    this->hevtStartMonitoring=CreateEvent(&SecAttr,FALSE,TRUE,psz);
    if (GetLastError()==ERROR_ALREADY_EXISTS)
    {
        CleanCloseHandle(&this->hevtStartMonitoring);
        _sntprintf(pszMsg,MAX_PATH,_T("Error another application (Pid : %s) seems to have inject API override"),pszPID);
        ReportError(pszMsg);
        return FALSE;
    }
    _tcscpy(psz,APIOVERRIDE_EVENT_STOP_MONITORING);
    _tcscat(psz,pszPID);
    this->hevtStopMonitoring=CreateEvent(&SecAttr,FALSE,FALSE,psz);
    _tcscpy(psz,APIOVERRIDE_EVENT_START_FAKING);
    _tcscat(psz,pszPID);
    this->hevtStartFaking=CreateEvent(&SecAttr,FALSE,TRUE,psz);
    _tcscpy(psz,APIOVERRIDE_EVENT_STOP_FAKING);
    _tcscat(psz,pszPID);
    this->hevtStopFaking=CreateEvent(&SecAttr,FALSE,FALSE,psz);
    _tcscpy(psz,APIOVERRIDE_EVENT_FREE_PROCESS);
    _tcscat(psz,pszPID);
    this->hevtFreeProcess=CreateEvent(&SecAttr,TRUE,FALSE,psz);// must be manual reset event, even for injected dll

    this->hevtTlsHookEndLoop=NULL;
        
    // (APIOverride.dll -> Injector)
    _tcscpy(psz,APIOVERRIDE_EVENT_DLLPROCESS_ATTACH_COMPLETED);
    _tcscat(psz,pszPID);
    this->hevtAPIOverrideDllProcessAttachCompleted=CreateEvent(&SecAttr,FALSE,FALSE,psz);
    _tcscpy(psz,APIOVERRIDE_EVENT_DLL_DETACHED_COMPLETED);
    _tcscat(psz,pszPID);
    this->hevtAPIOverrideDllProcessDetachCompleted=CreateEvent(&SecAttr,FALSE,FALSE,psz);
    _tcscpy(psz,APIOVERRIDE_EVENT_PROCESS_FREE);
    _tcscat(psz,pszPID);
    this->hevtProcessFree=CreateEvent(&SecAttr,TRUE,FALSE,psz);
    _tcscpy(psz,APIOVERRIDE_EVENT_MONITORING_FILE_LOADED);
    _tcscat(psz,pszPID);
    this->hevtMonitoringFileLoaded=CreateEvent(&SecAttr,FALSE,FALSE,psz);
    _tcscpy(psz,APIOVERRIDE_EVENT_MONITORING_FILE_UNLOADED);
    _tcscat(psz,pszPID);
    this->hevtMonitoringFileUnloaded=CreateEvent(&SecAttr,FALSE,FALSE,psz);
    _tcscpy(psz,APIOVERRIDE_EVENT_FAKE_API_DLL_LOADED);
    _tcscat(psz,pszPID);
    this->hevtFakeAPIDLLLoaded=CreateEvent(&SecAttr,FALSE,FALSE,psz);
    _tcscpy(psz,APIOVERRIDE_EVENT_FAKE_API_DLL_UNLOADED);
    _tcscat(psz,pszPID);
    this->hevtFakeAPIDLLUnloaded=CreateEvent(&SecAttr,FALSE,FALSE,psz);
    _tcscpy(psz,APIOVERRIDE_EVENT_ERROR);
    _tcscat(psz,pszPID);
    this->hevtError=CreateEvent(&SecAttr,FALSE,FALSE,psz);
    _tcscpy(psz,APIOVERRIDE_EVENT_CLIENT_MAILSLOT_OPEN);
    _tcscat(psz,pszPID);
    this->hevtClientMailslotOpen=CreateEvent(&SecAttr,TRUE,FALSE,psz);

    if (!(this->hevtStartMonitoring&&this->hevtStopMonitoring&&this->hevtStartFaking&&this->hevtStopFaking&&this->hevtFreeProcess
            &&this->hevtAPIOverrideDllProcessAttachCompleted&&this->hevtAPIOverrideDllProcessDetachCompleted&&this->hevtProcessFree
            &&this->hevtMonitoringFileLoaded&&this->hevtMonitoringFileUnloaded
            &&this->hevtFakeAPIDLLLoaded&&this->hevtFakeAPIDLLUnloaded
            &&this->hevtClientMailslotOpen
            &&this->hevtError
            ))
    {
        ReportError(_T("Error creating named events"));
        return FALSE;
    }

    // start mailslot server for monitoring event logging (must be start before injecting API_OVERRIDE_DLL)
    _tcscpy(psz,APIOVERRIDE_MAILSLOT_TO_INJECTOR);
    _tcscat(psz,pszPID);
    pMailSlotServer=new CMailSlotServer(psz,StaticMailSlotServerCallback,this);
    if (!pMailSlotServer->Start(TRUE))
    {
        _sntprintf(pszMsg,MAX_PATH,_T("Error starting mailslot server %s"),psz);
        ReportError(pszMsg);
        this->StopWithoutEnteringCriticalSection();
        return FALSE;
    }
    
    // start thread to watch hevtAPIOverrideDllProcessDetachCompleted
    this->hThreadWatchingEvents=CreateThread(NULL,0,CApiOverride::DllUnloadedThreadListener,this,0,NULL);

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: Start
// Object: start the software specified by pszFileName, inject API Override dll at start up
// Parameters :
//     in : TCHAR* pszFileName : path of software to start
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::Start(TCHAR* pszFileName)
{
    return this->Start(pszFileName,NULL,NULL);
}

//-----------------------------------------------------------------------------
// Name: Start
// Object: start the software specified by pszFileName, inject API Override dll at start up,
//         call pCallBackFunc function  to allow to configure monitoring and faking
//         resume process when callback function returns
// Parameters :
//     in : TCHAR* pszFileName : path of software to start
//          FARPROC pCallBackFunc : instruction to do after pszFileName loading and before we resume the process
//                                          let us load monitoring file and fake api dll before software startup
//          LPVOID pUserParam : parameter for the callback
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::Start(TCHAR* pszFileName,tagpCallBackBeforeAppResume pCallBackFunc,LPVOID pUserParam)
{
    return this->Start(pszFileName,_T(""),pCallBackFunc,pUserParam);
}

//-----------------------------------------------------------------------------
// Name: Start
// Object: start the software specified by pszFileName, inject API Override dll at start up,
//         call pCallBackFunc function  to allow to configure monitoring and faking
//         resume process when callback function returns
// Parameters :
//     in : TCHAR* pszFileName : path of software to start
//          TCHAR* pszCmdLine  : command line
//          FARPROC pCallBackFunc : instruction to do after pszFileName loading and before we resume the process
//                                          let us load monitoring file and fake api dll before software startup
//          LPVOID pUserParam : parameter for the callback
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::Start(TCHAR* pszFileName,TCHAR* pszCmdLine,tagpCallBackBeforeAppResume pCallBackFunc,LPVOID pUserParam)
{
    return this->Start(pszFileName,pszCmdLine,pCallBackFunc,pUserParam,CApiOverride::StartWaySuspended,0);
}


//-----------------------------------------------------------------------------
// Name: Start
// Object: start the software specified by pszFileName, inject API Override dll at start up,
//         call pCallBackFunc function  to allow to configure monitoring and faking
//         Process is resume at Startup during dwResumeTimeAtStartup ms
//         resume process when callback function returns
// Parameters :
//     in : TCHAR* pszFileName : path of software to start
//          TCHAR* pszCmdLine  : command line
//          FARPROC pCallBackFunc : instruction to do after pszFileName loading and before we resume the process
//                                          let us load monitoring file and fake api dll before software startup
//          LPVOID pUserParam : parameter for the callback
//          StartWays StartMethod : Suspended, Sleep
//          DWORD dwResumeTimeAtStartup : Time in ms during which process will be resumed at startup
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::Start(TCHAR* pszFileName,TCHAR* pszCmdLine,tagpCallBackBeforeAppResume pCallBackFunc,LPVOID pUserParam,StartWays StartMethod,DWORD dwResumeTimeAtStartup)
{
    return this->Start(pszFileName,pszCmdLine,pCallBackFunc,pUserParam,StartMethod,dwResumeTimeAtStartup,NULL, NULL);
}
//-----------------------------------------------------------------------------
// Name: Start
// Object: start the software specified by pszFileName, inject API Override dll at start up,
//         call pCallBackFunc function  to allow to configure monitoring and faking
//         Process is resume at Startup during dwResumeTimeAtStartup ms
//         resume process when callback function returns
// Parameters :
//     in : TCHAR* pszFileName : path of software to start
//          TCHAR* pszCmdLine  : command line
//          FARPROC pCallBackFunc : instruction to do after pszFileName loading and before we resume the process
//                                          let us load monitoring file and fake api dll before software startup
//          LPVOID pUserParam : parameter for the callback
//          StartWays StartMethod : Suspended, Sleep
//          DWORD dwResumeTimeAtStartup : Time in ms during which process will be resumed at startup
//      out :
//          STARTUPINFO* pStartupInfo
//          PROCESS_INFORMATION* pProcessInformation
// Return : FALSE on error, TRUE if success
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::Start(TCHAR* pszFileName,TCHAR* pszCmdLine,tagpCallBackBeforeAppResume pCallBackFunc,LPVOID pUserParam,StartWays StartMethod,DWORD dwResumeTimeAtStartup,OUT STARTUPINFO* pStartupInfo,OUT PROCESS_INFORMATION* pProcessInformation)
{
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    TCHAR* pszFileNameDirectory;
    TCHAR* pszLastSep=NULL;
    TCHAR* pszDir;
    BOOL bRet;
    TCHAR pszMsg[2*MAX_PATH];
    TCHAR* pszCmdLineWithSpace;
    BOOL bSuccess = FALSE;
    BOOL bClosehProcess = TRUE;

    // check command line content
    if (IsBadReadPtr(pszCmdLine,sizeof(TCHAR)))
        pszCmdLine=_T("");

    memset(&ProcessInformation,0,sizeof(PROCESS_INFORMATION));
    memset(&StartupInfo,0,sizeof(STARTUPINFO));

    if (pProcessInformation)
        memset(pProcessInformation,0,sizeof(PROCESS_INFORMATION));
    if (pStartupInfo)
        memset(pStartupInfo,0,sizeof(STARTUPINFO));

    // check if file exists
    if (!CStdFileOperations::DoesFileExists(pszFileName))
    {
        _sntprintf(pszMsg,2*MAX_PATH,_T("File %s not found"),pszFileName);
        ReportError(pszMsg);
        return FALSE;
    }

    EnterCriticalSection(&this->CriticalSection);

    StartupInfo.cb=sizeof(STARTUPINFO);

    // get software directory 
    pszDir=_tcsdup(pszFileName);
    pszLastSep=_tcsrchr(pszDir,'\\');
    if (pszLastSep)
    {
        *pszLastSep=0;
        pszFileNameDirectory=pszDir;
    }
    else // work in current dir
        pszFileNameDirectory=NULL;

    // add a space before cmd line because CreateProcess sucks a little (first arg is lost by launched app)
    pszCmdLineWithSpace=new TCHAR[_tcslen(pszCmdLine)+2];
    if (*pszCmdLine)
    {
        _tcscpy(pszCmdLineWithSpace,_T(" "));
        _tcscat(pszCmdLineWithSpace,pszCmdLine);
    }
    else
    {
        *pszCmdLineWithSpace = 0;
    }

    {
        CPE Pe(pszFileName);
        if (Pe.IsNET())
        {
            // .NET doesn't support IAT patching
            StartMethod = CApiOverride::StartWaySuspended;
        }
    }


    switch (StartMethod)
    {
    case CApiOverride::StartWaySleep:

        // crate Process
        bRet=CreateProcess( pszFileName,                //LPCTSTR lpApplicationName,
                            pszCmdLineWithSpace,        //TCHAR* lpCommandLine,
                            NULL,                       //LPSECURITY_ATTRIBUTES lpProcessAttributes,
                            NULL,                       //LPSECURITY_ATTRIBUTES lpThreadAttributes,
                            FALSE,                      //BOOL bInheritHandles,
                            CREATE_DEFAULT_ERROR_MODE,  //DWORD dwCreationFlags,
                            NULL,                       //LPVOID lpEnvironment,
                            pszFileNameDirectory,       //LPCTSTR lpCurrentDirectory,
                            &StartupInfo,               //LPSTARTUPINFO lpStartupInfo,
                            &ProcessInformation         //LPPROCESS_INFORMATION lpProcessInformation
                            );
        if (!bRet)
        {
            // show last error
            CAPIError::ShowLastError();
            goto End;
        }
        // sleep if necessary
        if (dwResumeTimeAtStartup)
            Sleep(dwResumeTimeAtStartup);

        // suspend Process main thread
        if (SuspendThread(ProcessInformation.hThread)==0xFFFFFFFF)
        {
            // show last error
            CAPIError::ShowLastError();
            TerminateProcess(ProcessInformation.hProcess,0xFFFFFFFF);
            goto End;
        }

        // Do all work
        if (!this->StartWithoutEnteringCriticalSection(ProcessInformation.dwProcessId,NULL))
        {
            TerminateProcess(ProcessInformation.hProcess,0xFFFFFFFF);
            goto End;
        }

        // send settings
        this->SetInitialOptions();

        if (!CallHookedProcessCallBackForStartSuspended(ProcessInformation.hThread,pCallBackFunc,pUserParam))
            goto End;

        if (!ResumeThreadForStartSuspended(ProcessInformation.hThread,ProcessInformation.hProcess))
            goto End;

        break;
    case CApiOverride::StartWaySuspended:
        {

            // Load Process in a suspended mode
            bRet=CreateProcess( pszFileName,                //LPCTSTR lpApplicationName,
                                pszCmdLineWithSpace,        //TCHAR* lpCommandLine,
                                NULL,                       //LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                NULL,                       //LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                FALSE,                      //BOOL bInheritHandles,
                                CREATE_SUSPENDED | CREATE_DEFAULT_ERROR_MODE,           //DWORD dwCreationFlags,
                                NULL,                       //LPVOID lpEnvironment,
                                pszFileNameDirectory,       //LPCTSTR lpCurrentDirectory,
                                &StartupInfo,               //LPSTARTUPINFO lpStartupInfo,
                                &ProcessInformation         //LPPROCESS_INFORMATION lpProcessInformation
                                );
            
            if (!bRet)
            {
                // show last error
                CAPIError::ShowLastError();
                goto End;
            }
            // set process name
            _tcsncpy(this->ProcessFullPathName,pszFileName,MAX_PATH);
            this->ProcessFullPathName[MAX_PATH-1]=0;
            _tcscpy(this->ProcessName,CStdFileOperations::GetFileName(this->ProcessFullPathName));
            CStdFileOperations::GetFilePath(this->ProcessFullPathName,this->ProcessPath,MAX_PATH);

            if (!this->StartAtProcessCreation(pszFileName,
                                                ProcessInformation.dwProcessId,
                                                ProcessInformation.hProcess,
                                                ProcessInformation.dwThreadId,
                                                ProcessInformation.hThread,
                                                FALSE,
                                                pCallBackFunc,
                                                pUserParam
                                                )
                )
                goto End;
        }
        break;
    case CApiOverride::StartWayIAT:
        {
            TCHAR TmpFileName[MAX_PATH];
            TCHAR MailSlotName[MAX_PATH];
            TCHAR EventName[MAX_PATH];
            TCHAR* EnvVar=0;
            DWORD OriginalEnvVarSize=0;

            // set event accessible for all account. Doing this allow to event to be opened by other user
            // --> we can inject dll into processes running under other users accounts
            SECURITY_DESCRIPTOR sd={0};
            sd.Revision=SECURITY_DESCRIPTOR_REVISION;
            sd.Control=SE_DACL_PRESENT;
            sd.Dacl=NULL; // assume everyone access
            SECURITY_ATTRIBUTES SecAttr={0};
            SecAttr.bInheritHandle=FALSE;
            SecAttr.nLength=sizeof(SECURITY_ATTRIBUTES);
            SecAttr.lpSecurityDescriptor=&sd;
           
            _tcscpy(TmpFileName,pszFileName);
            _tcscat(TmpFileName,CApiOverride_IAT_LOADER_TMP_FILE_EXTENSION);

            if (!CStdFileOperations::DoesFileExists(TmpFileName))
            {
                if (!::CopyFile(pszFileName,TmpFileName,FALSE))
                {
                    _stprintf(pszMsg,_T("Error copying %s to change its IAT, make sure directory is writable"),pszFileName);
                    MessageBox(this->hParentWindow,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);
                    goto End;
                }

                CPE::ADD_IMPORT_FUNCTION_DESCRIPTION DllFunctions[] = { {_T("_Initialize@0"),0,0,FALSE} };
                CPE::ADD_IMPORT_LIBRARY_DESCRIPTION Imports [] =
                {
                    { CApiOverride_IAT_LOADER_DLL_NAME,  DllFunctions,  _countof(DllFunctions) },
                };

                // add import before existing ones to be the first loaded module (still after ntdll.dll and kernel32.dll, as these modules are used by apioverride.dll)
                if (!CPE::AddImports(TmpFileName,CApiOverride_ADDIMPORTS_SECTION_NAME,Imports,_countof(Imports),TRUE))
                {
                    _stprintf(pszMsg,_T("Error adding import to %s"),pszFileName);
                    MessageBox(this->hParentWindow,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);
                    goto End;
                }
            }

            StartWayIATMailSlotCallback_PARAM IATMailSlotCallbackParam;
            IATMailSlotCallbackParam.bResult = FALSE;
            IATMailSlotCallbackParam.pApiOverride = this;
            IATMailSlotCallbackParam.pCallBackFunc = pCallBackFunc;
            IATMailSlotCallbackParam.pCallBackFuncUserParam = pUserParam;
            IATMailSlotCallbackParam.hEventEndOfInitialization = :: CreateEvent(NULL,FALSE,FALSE,NULL);
            IATMailSlotCallbackParam.hEventRemoteMailslotActive = :: CreateEvent(NULL,FALSE,FALSE,NULL);

            _tcscpy(MailSlotName,_T("\\\\.\\mailslot\\WAO_"));
            _tcscat(MailSlotName,CStdFileOperations::GetFileName(TmpFileName));

            _tcscpy(EventName,_T("EVENT_"));
            _tcscat(EventName,CStdFileOperations::GetFileName(TmpFileName));
            IATMailSlotCallbackParam.hEventReady = ::CreateEvent(&SecAttr,FALSE,FALSE,EventName);
            
            // notice : all work will be done in StartWayIATMailSlotCallback
            CMailSlotServer Server( MailSlotName,StartWayIATMailSlotCallback,&IATMailSlotCallbackParam);
            if (!Server.Start(TRUE))
            {
                // show last error
                CAPIError::ShowLastError();
                goto StartWayIATEnd;
            }


            // change current process environment, 
            // and as child process inherit current environment, child process will be affected
            // notice : SetEnvironmentVariable has no effect on the system environment variables
            OriginalEnvVarSize=::GetEnvironmentVariable(_T("Path"),EnvVar,OriginalEnvVarSize);
            EnvVar = new TCHAR [OriginalEnvVarSize+_tcslen(this->pszAppPath)+3];// _alloca seems to cause some trouble here :( Too many blocks ?
            if (!::GetEnvironmentVariable(_T("Path"),EnvVar,OriginalEnvVarSize))
            {
                // show last error
                CAPIError::ShowLastError();
                goto StartWayIATEnd;
            }

            _tcscat(EnvVar,_T(";"));
            _tcscat(EnvVar,this->pszAppPath);
            _tcscat(EnvVar,_T(";"));
            if (!::SetEnvironmentVariable(_T("Path"),EnvVar))
            {
                // show last error
                CAPIError::ShowLastError();
                goto StartWayIATEnd;
            }

            // crate Process
            bRet=CreateProcess( TmpFileName,                //LPCTSTR lpApplicationName,
                pszCmdLineWithSpace,        //TCHAR* lpCommandLine,
                NULL,                       //LPSECURITY_ATTRIBUTES lpProcessAttributes,
                NULL,                       //LPSECURITY_ATTRIBUTES lpThreadAttributes,
                FALSE,                      //BOOL bInheritHandles,
                CREATE_DEFAULT_ERROR_MODE,  //DWORD dwCreationFlags,
                NULL,                       //LPVOID lpEnvironment,// inherit parent's environment 
                pszFileNameDirectory,       //LPCTSTR lpCurrentDirectory,
                &StartupInfo,               //LPSTARTUPINFO lpStartupInfo,
                &ProcessInformation         //LPPROCESS_INFORMATION lpProcessInformation
                );

            // restore environment
            EnvVar[OriginalEnvVarSize-1]=0;// OriginalEnvVarSize include \0
            ::SetEnvironmentVariable(_T("Path"),EnvVar);
            delete[] EnvVar;
            EnvVar = 0;

            if (!bRet)
            {
                // show last error
                CAPIError::ShowLastError();
                goto StartWayIATEnd;
            }
            HANDLE hThreadArray[]={IATMailSlotCallbackParam.hEventRemoteMailslotActive,ProcessInformation.hProcess};
            DWORD dwWaitForMultipleObjectsRet = ::WaitForMultipleObjects(_countof(hThreadArray),hThreadArray,FALSE,20000);
            switch (dwWaitForMultipleObjectsRet)
            {
            //case WAIT_OBJECT_0:
            //    // nothing to do
            //    break;
            case (WAIT_OBJECT_0+1):// process end
                ::DeleteFile(TmpFileName);
                goto StartWayIATEnd;
            case WAIT_TIMEOUT:
                ::TerminateProcess(ProcessInformation.hProcess,0);
                ::DeleteFile(TmpFileName);
                goto StartWayIATEnd;
            }

            DeleteIATInjectedFileThread_PARAM DeleteIATInjectedFileThreadParam;
            DeleteIATInjectedFileThreadParam.FileName=TmpFileName;
            DeleteIATInjectedFileThreadParam.hProcess = ProcessInformation.hProcess;
            DeleteIATInjectedFileThreadParam.hEventEndOfInitialization = CreateEvent(0,FALSE,FALSE,0);

            HANDLE hThreadDeleteIATInjectedFile;
            hThreadDeleteIATInjectedFile = ::CreateThread(NULL,0,DeleteIATInjectedFileThread,&DeleteIATInjectedFileThreadParam,0,0);
            if (hThreadDeleteIATInjectedFile)
            {
                ::WaitForSingleObject(DeleteIATInjectedFileThreadParam.hEventEndOfInitialization,INFINITE);
                bClosehProcess = FALSE;
                ::CloseHandle(hThreadDeleteIATInjectedFile);
            }
            ::CloseHandle(DeleteIATInjectedFileThreadParam.hEventEndOfInitialization);
            ::WaitForSingleObject(IATMailSlotCallbackParam.hEventEndOfInitialization,INFINITE);

StartWayIATEnd:
            ::CloseHandle(IATMailSlotCallbackParam.hEventEndOfInitialization);
            ::CloseHandle(IATMailSlotCallbackParam.hEventReady);
            ::CloseHandle(IATMailSlotCallbackParam.hEventRemoteMailslotActive);
            Server.Stop();
            if (EnvVar)
                delete[] EnvVar;

            // if an error has occurred
            if (!IATMailSlotCallbackParam.bResult)
                goto End;
        }
        break;
    default:
        goto End;
    }

    bSuccess = TRUE;
End: // Warning at this point object destruction can have been asked

    free(pszDir);

    LeaveCriticalSection(&this->CriticalSection);

    if (pProcessInformation)
    {
        // don't close hThread nor hProcess handles
        memcpy(pProcessInformation,&ProcessInformation,sizeof(PROCESS_INFORMATION));
    }
    else
    {
        if (ProcessInformation.hThread)
            CloseHandle(ProcessInformation.hThread);

        // if iat as been changed, let hprocess open to spy end of process
        if (bClosehProcess)
        {
            // close process handle
            if (ProcessInformation.hProcess)
                CloseHandle(ProcessInformation.hProcess);
        }
    }
    if (pStartupInfo)
        memcpy(pStartupInfo,&StartupInfo,sizeof(STARTUPINFO));

    if (pszCmdLineWithSpace)
        delete[] pszCmdLineWithSpace;
    return bSuccess;
}

DWORD WINAPI CApiOverride::DeleteIATInjectedFileThread(LPVOID lpParameter)
{
    DeleteIATInjectedFileThread_PARAM* pDeleteIATInjectedFileThread = (DeleteIATInjectedFileThread_PARAM*)lpParameter;
    // make local copy to be fully independent
    HANDLE hProcess = pDeleteIATInjectedFileThread->hProcess;
    TCHAR FileName[MAX_PATH];
    _tcscpy(FileName,pDeleteIATInjectedFileThread->FileName);

    //warn caller we have finish to use it's local var, so it can continue it's execution
    ::SetEvent(pDeleteIATInjectedFileThread->hEventEndOfInitialization);

    // wait end of process to delete file
    ::WaitForSingleObject(hProcess,INFINITE);

    // delete file
    ::DeleteFile(FileName);

    // close handle
    ::CloseHandle(hProcess);
    return 0;
}

void CApiOverride::StartWayIATMailSlotCallback(PVOID pData,DWORD dwDataSize,PVOID pUserData)
{
    StartWayIATMailSlotCallback_PARAM* Param = (StartWayIATMailSlotCallback_PARAM*)pUserData;

    DWORD ProcessId;
    SECURITY_DESCRIPTOR sd={0};
    sd.Revision=SECURITY_DESCRIPTOR_REVISION;
    sd.Control=SE_DACL_PRESENT;
    sd.Dacl=NULL; // assume everyone access
    SECURITY_ATTRIBUTES SecAttr={0};
    SecAttr.bInheritHandle=FALSE;
    SecAttr.nLength=sizeof(SECURITY_ATTRIBUTES);
    SecAttr.lpSecurityDescriptor=&sd;

    HANDLE hevtSingleThreadedMailSlotServerEndLoop = NULL;
    TCHAR pszPID[32];
    TCHAR psz[MAX_PATH];

    ::SetEvent(Param->hEventRemoteMailslotActive);

    if ( dwDataSize<sizeof(ProcessId) )
    {
        MessageBox(Param->pApiOverride->hParentWindow,_T("Error bad mailslot data content"),_T("Error"),MB_OK|MB_ICONERROR);
        goto CleanUp;
    }
    ProcessId = *((DWORD*)pData);

    if(!Param->pApiOverride->InitializeStart(ProcessId))
        goto CleanUp;

    // create event to stop mono threaded mailslot server (responsible of loading blocking)
    _stprintf(pszPID,_T("0x%X"),ProcessId);
    _tcscpy(psz,APIOVERRIDE_EVENT_SINGLETHREADEDMAILSLOTSERVER_END_WAITING_INSTRUCTIONS_LOOP);
    _tcscat(psz,pszPID);
    hevtSingleThreadedMailSlotServerEndLoop=::CreateEvent(&SecAttr,FALSE,FALSE,psz);

    // tell IAT Loader.dll we are ready
    ::SetEvent(Param->hEventReady);

    // wait for load events
    if (!Param->pApiOverride->WaitForInjectedDllToBeLoaded())
        goto CleanUp;

    // open mailslot client to connect to mono threaded mailslot server
    if(!Param->pApiOverride->OpenClientMailSlot())
        goto CleanUp;

    // send settings
    // critical section must be leaved to call SetInitialOptions
    LeaveCriticalSection(&Param->pApiOverride->CriticalSection);
    Param->pApiOverride->SetInitialOptions();
    EnterCriticalSection(&Param->pApiOverride->CriticalSection);
    
    // call user callback
    if (!Param->pApiOverride->CallHookedProcessCallBack(Param->pCallBackFunc,Param->pCallBackFuncUserParam))
        goto CleanUp;

    // hevtClientMailslotOpen is reset by remote process, but not speed enough for a call to OpenClientMailSlot
    // task switching may not be done --> force reset before OpenClientMailSlot call
    // do reset before setting hevtSingleThreadedMailSlotServerEndLoop in case task switch occurs and server is reopen
    // before reseting hevtClientMailslotOpen in current thread
    ::ResetEvent(Param->pApiOverride->hevtClientMailslotOpen);

    // stop single threaded loop to allow end of process loading
    ::SetEvent(hevtSingleThreadedMailSlotServerEndLoop);

    // open mailslot client for injected threaded mailslot server
    if(!Param->pApiOverride->OpenClientMailSlot())
        goto CleanUp;

    Param->bResult = TRUE;
CleanUp:
    if (hevtSingleThreadedMailSlotServerEndLoop)
        ::CloseHandle(hevtSingleThreadedMailSlotServerEndLoop);
    ::SetEvent(Param->hEventEndOfInitialization);
}

BOOL CApiOverride::ResumeThreadForStartSuspended(HANDLE hThread,HANDLE hProcess)
{
    // Resume Process main thread
    if (::ResumeThread(hThread)==((DWORD)-1))
    {
        // show last error
        CAPIError::ShowLastError();
        this->StopWithoutEnteringCriticalSection();
        TerminateProcess(hProcess,0xFFFFFFFF);
        return FALSE;
    }
    return TRUE;
}

// return FALSE if object has been destroyed during the callback call
BOOL CApiOverride::CallHookedProcessCallBackForStartSuspended(HANDLE hThread,tagpCallBackBeforeAppResume pCallBackFunc,PVOID pUserParam)
{
    // check if current object has been deleted during callback
    if (!CApiOverride::CallHookedProcessCallBack(pCallBackFunc,pUserParam))
    {
        // notice : assume that no object data is used here
        ResumeThread(hThread);
        return FALSE;
    }
    return TRUE;
}

// return FALSE if object has been destroyed during the callback call
BOOL CApiOverride::CallHookedProcessCallBack(tagpCallBackBeforeAppResume pCallBackFunc,PVOID pUserParam)
{
    // call the call back if any (let us load monitoring file and fake api dll before software startup)
    if (!IsBadCodePtr((FARPROC)pCallBackFunc))
    {
        BOOL bLocalDestructorCalled;

        // assume that destructor won't finish during user callback
        ResetEvent(this->hDestructorUnlocked);

        // leave critical section during callback (allow user to change some option, filters ...)
        LeaveCriticalSection(&this->CriticalSection);

        // call user callback
        pCallBackFunc(this->dwCurrentProcessId,pUserParam);

        // reenter critical section BEFORE setting hDestructorUnlocked to unlocked state
        EnterCriticalSection(&this->CriticalSection);

        // store object bDestructorCalled property in stack before allowing destructor to complete
        bLocalDestructorCalled=this->bDestructorCalled;

        // from now destructor can finish is job
        SetEvent(this->hDestructorUnlocked);

        if (bLocalDestructorCalled)
            return FALSE;
    }
    return TRUE;
}

BOOL STDMETHODCALLTYPE CApiOverride::StartAtProcessCreation(DWORD ProcessId,DWORD ThreadId,BOOL bLetThreadSuspended,tagpCallBackBeforeAppResume pCallBackFunc,PVOID pUserParam)
{
    BOOL bSuccess = FALSE;
    HANDLE hProcess = NULL;
    HANDLE hThread = NULL;
    TCHAR ProcessPath[MAX_PATH];
    
    // open process
    hProcess = ::OpenProcess(PROCESS_ALL_ACCESS,FALSE,ProcessId);
    if (!hProcess)
    {
        CAPIError::ShowLastError();
        goto End;
    }

    // get process name
    CProcessName::GetProcessName(hProcess,ProcessPath);

    // get thread handle
    hThread = ::OpenThread(THREAD_ALL_ACCESS,FALSE,ThreadId);
    if (!hThread)
    {
        CAPIError::ShowLastError();
        goto End;
    }

    ::EnterCriticalSection(&this->CriticalSection);

    bSuccess = this->StartAtProcessCreation(ProcessPath, ProcessId, hProcess, ThreadId, hThread, bLetThreadSuspended,pCallBackFunc,pUserParam);

    ::LeaveCriticalSection(&this->CriticalSection);

End:
    if (hProcess)
        ::CloseHandle(hProcess);
    if (hThread)
        ::CloseHandle(hThread);

    return bSuccess;
}

BOOL CApiOverride::StartAtProcessCreation(TCHAR* pszFileName,DWORD ProcessId,HANDLE hProcess,DWORD ThreadId,HANDLE hThread,BOOL bLetThreadSuspended,tagpCallBackBeforeAppResume pCallBackFunc,PVOID pUserParam)
{
    UNREFERENCED_PARAMETER(ThreadId);
    BOOL bSuccess = FALSE;
    BOOL TlsCallBackHook=FALSE;
    CPE Pe(pszFileName);
    if (!Pe.Parse())
        return FALSE;

    // check if .Net application
    if (Pe.IsNET())
    {
        // in case of .Net app, Entry point is a 6 bytes absolute jmp, and on some OS
        // these first bytes are not executed (nt loader doesn't use provided entry point)
        // --> resume process and let Start(DWORD PID) do the work
        if (CApiOverridebNetProfilingEnabled)
        {
            if (!this->StartWithoutEnteringCriticalSection(ProcessId,hThread))
            {
                goto End;
            }
        }
        else
        {
            // do pooling on first module loading
            // wait first module loading (= end of nt loader job) before call to InitializeStart and injection
            int CurrentPriority=SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
            DWORD OriginalTickCount=GetTickCount();
            DWORD CurrentTickCount;
            ::ResumeThread(hThread);

            while(!CProcessHelper::IsFirstModuleLoaded(ProcessId))
            {
                Sleep(1);
                CurrentTickCount=GetTickCount();
                if (OriginalTickCount+MAX_POOLING_TIME_IN_MS<CurrentTickCount)
                {
                    goto End;
                }
            }
            ::SuspendThread(hThread);
            this->StartWithoutEnteringCriticalSection(ProcessId,hThread);
            // restore thread priority
            ::SetThreadPriority(GetCurrentThread(),CurrentPriority);
        }
    }
    else
    {
        // initialize events
        if (!this->InitializeStart(ProcessId))
            goto End;

        // reset load events
        this->ResetInjectedDllLoadEvents();

        // load library at entry point
        if(!this->HookEntryPoint(pszFileName,ProcessId,hProcess,hThread,&TlsCallBackHook))
            goto End;

        // wait for the library to be loaded
        if (!this->WaitForInjectedDllToBeLoaded())
            goto End;

        // now GetPorcessNameFromPID should success
        if (!this->FillPorcessNameFromPID(ProcessId))
        {
            _tcscpy(this->ProcessName,CStdFileOperations::GetFileName(pszFileName));
            CStdFileOperations::GetFilePath(pszFileName,this->ProcessPath,MAX_PATH);
        }

        if(!this->OpenClientMailSlot())
            goto End;
    }

    this->SetInitialOptions();

    if (!this->CallHookedProcessCallBackForStartSuspended(hThread,pCallBackFunc,pUserParam))
        goto End;

    if (TlsCallBackHook)
    {
        // hevtClientMailslotOpen is reset by remote process, but not speed enough for a call to OpenClientMailSlot
        // task switching may not be done --> force reset before OpenClientMailSlot call
        // do reset before setting hevtTlsHookEndLoop in case task switch occurs and server is reopen
        // before reseting hevtClientMailslotOpen in current thread
        ::ResetEvent(this->hevtClientMailslotOpen);

        // send event to stop mailslot watching
        // doing this, the main thread is going to suspend itself
        // so we have to wait thread suspension

        // send event to stop mailslot watching
        SetEvent(this->hevtTlsHookEndLoop);

        // wait thread suspension
        DWORD OriginalTickCount=GetTickCount();
        DWORD CurrentTickCount;
        DWORD dw;
        DWORD dwTransferedSize;
        for(;;)
        {
            Sleep(HOOK_END_POOLING_IN_MS);
            CurrentTickCount=GetTickCount();
            if (OriginalTickCount+MAX_POOLING_TIME_IN_MS<CurrentTickCount)
            {
                TCHAR pszMsg[2*MAX_PATH];
                _stprintf(pszMsg,_T("Error hooking application %s in suspended way"),pszFileName);
                MessageBox(this->hParentWindow,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);
                goto End;
            }
            this->HookEntryPointpProcessMemory->Read( this->HookEntryPointRemoteHook ,&dw,sizeof(DWORD),&dwTransferedSize);

            if (dw==CApiOverride_SUSPENDED_HOOK_END_FLAG)
                break;

            // if process has crash don't wait infinite
            if (!CProcessHelper::IsAlive(ProcessId))
            {
                TCHAR pszMsg[2*MAX_PATH];
                _stprintf(pszMsg,_T("Error application %s seems to be closed"),pszFileName);
                MessageBox(this->hParentWindow,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);
                goto End;
            }
        }
        if(!this->OpenClientMailSlot())
            goto End;
    }

    bSuccess = TRUE;
End:

    // if thread should be resumed
    if (!bLetThreadSuspended)
    {
        // in case of success only
        if (bSuccess)
        {
            // resume thread
            this->ResumeThreadForStartSuspended(hThread,hProcess);
        }
    }

    // if not .Net, this->HookEntryPoint has been called, so we need to free it
    if (!Pe.IsNET())
    {
        // in case of failure, assume thread is resumed
        if (!bSuccess)
        {
            ResumeThread(hThread);
        }
        if ( (!bLetThreadSuspended)// if data is freed before thread is resume, we will crash target application; so we make a small memory lost
             || (!bSuccess)
           )
        {
            Sleep(500);//sleep a while to allow process to execute last hook operations and returns the original code flow

            // free data allocated by HookEntryPoint
            this->HookEntryPointFree();
        }
    }

    return bSuccess;
}

//-----------------------------------------------------------------------------
// Name: StaticMailSlotServerCallback
// Object: callback for log events
// Parameters :
//     in : PVOID pData : pointer to a tagLogEntry struct
//          PVOID pUserData : pointer to CApiOverride object belonging the MailSlotServer which reach the event
// Return : 
//-----------------------------------------------------------------------------
void CApiOverride::StaticMailSlotServerCallback(PVOID pData,DWORD dwDataSize,PVOID pUserData)
{
    if (dwDataSize==0)
        return;
    // re enter object oriented programming
    ((CApiOverride*)pUserData)->MailSlotServerCallback(pData,dwDataSize);
}

//-----------------------------------------------------------------------------
// Name: MailSlotServerCallback
// Object: internal callback for log events
// Parameters :
//     in : PVOID pData : pointer to a buffer send by injected dll
//          DWORD dwDataSize : pData size 
// Return : 
//-----------------------------------------------------------------------------
void CApiOverride::MailSlotServerCallback(PVOID pData,DWORD dwDataSize)
{
    DWORD msgCmd;
    DWORD ReplySize;
    PBYTE pbData=(PBYTE)pData;
    

    // check message size
    if (dwDataSize<sizeof(DWORD))
        return;

    memcpy(&msgCmd,pData,sizeof(DWORD));
    pbData+=sizeof(DWORD);
    switch (msgCmd)
    {
    case CMD_PROCESS_INTERNAL_CALL_REPLY:
        {
            CLinkListItem* pItem;
            REMOTE_CALL_INFOS* pRemoteCallItem;
            BOOL bFound=FALSE;

            // check message size
            if (dwDataSize<(2*sizeof(DWORD)))
                return;

            memcpy(&ReplySize,pbData,sizeof(DWORD));
            // point after reply size
            pbData+=sizeof(DWORD);

            // check message size
            if (dwDataSize<(sizeof(DWORD)+ReplySize))// reply size include the ReplySize Field
                return;

            // check if user still waits for function return
            // (= ID is still in pCurrentRemoteCalls list)
            pItem=this->pCurrentRemoteCalls->Head;
            while(pItem)
            {
                if (pItem->ItemData==*((PVOID*)pbData))
                {
                    bFound=TRUE;
                    break;
                }
                pItem=pItem->NextItem;
            }
            if (!bFound)
                return;

            // ID is still in list --> memory is still valid
            pRemoteCallItem=*((REMOTE_CALL_INFOS**)pbData);

            pRemoteCallItem->ProcessInternalCallReply=new BYTE[ReplySize];

            // restore pbData value (point before reply size as it's needed for decoding)
            pbData-=sizeof(DWORD);
            memcpy(pRemoteCallItem->ProcessInternalCallReply,pbData,ReplySize);
            SetEvent(pRemoteCallItem->hevtProcessInternalCallReply);
            return;
        }
    case CMD_NOT_LOGGED_MODULE_LIST_REPLY:
        // check message size
        if (dwDataSize<(2*sizeof(DWORD)))
                return;

        memcpy(&ReplySize,pbData,sizeof(DWORD));
        pbData+=sizeof(DWORD);

        // check message size
        if (dwDataSize<(sizeof(DWORD)+ReplySize))// reply size include the ReplySize Field
                return;

        this->NotLoggedModulesArraySize=ReplySize;
        if (ReplySize>0)
        {
            this->NotLoggedModulesArray=new BYTE[ReplySize*MAX_PATH*sizeof(TCHAR)];
            memcpy(this->NotLoggedModulesArray,pbData,ReplySize*MAX_PATH*sizeof(TCHAR));
        }
        else
            this->NotLoggedModulesArray=NULL;
        SetEvent(this->hevtGetNotLoggedModulesReply);
        return;

    case CMD_OVERRIDING_DLL_QUERY_TO_PLUGIN:
        // check message size
        if (dwDataSize<(sizeof(PVOID)+2*sizeof(SIZE_T)))
            return;
        {
            SIZE_T PluginNameSize;
            TCHAR* PluginName;
            PVOID MessageId;
            PBYTE pMsg;
            SIZE_T MsgSize;
            SIZE_T Index;

            Index = 0;

            // plugin name size
            memcpy(&PluginNameSize,&pbData[Index],sizeof(SIZE_T));
            Index+=sizeof(SIZE_T);
            
            // plugin name
            if ((dwDataSize-Index)<PluginNameSize)// dwDataSize-Index = remaining size
                return;
            PluginName = (TCHAR*)&pbData[Index];
            Index+=PluginNameSize;

            // message id
            if ((dwDataSize-Index)<sizeof(PVOID))// dwDataSize-Index = remaining size
                return;
            memcpy(&MessageId,&pbData[Index],sizeof(PVOID));
            Index+=sizeof(PVOID);

            // MsgSize
            if ((dwDataSize-Index)<sizeof(SIZE_T))// dwDataSize-Index = remaining size
                return;
            memcpy(&MsgSize,&pbData[Index],sizeof(SIZE_T));
            Index+=sizeof(SIZE_T);

            // Msg
            if ((dwDataSize-Index)<MsgSize)// dwDataSize-Index = remaining size
                return;
            pMsg = &pbData[Index];
            // Index+=MsgSize;

            if (this->pCallBackOverridingDllQuery)
                this->pCallBackOverridingDllQuery(PluginName,
                                                  MessageId,
                                                  pMsg,
                                                  MsgSize,
                                                  this->pCallBackOverridingDllQueryUserParam);
        }
        return;
    case CMD_MONITORING_LOG:
        {
            // check message size
            if (dwDataSize<(2*sizeof(DWORD)))
                return;

            memcpy(&ReplySize,pbData,sizeof(DWORD));
            pbData+=sizeof(DWORD);

            // check message size
            if (dwDataSize<(2*sizeof(DWORD)+ReplySize))// reply size don't include the ReplySize Field
                return;

            // call monitoring call back
            this->MonitoringCallback(pbData);

            return;
        }
    case CMD_REPORT_MESSAGE:
        {
            // check message size
            if (dwDataSize<(2*sizeof(DWORD)+sizeof(TCHAR)+sizeof(FILETIME)))
                return;

            DWORD ReportMessageType;
            DWORD StringLength; // string length in bytes
            FILETIME FileTime;

            memcpy(&ReportMessageType,pbData,sizeof(DWORD));
            pbData+=sizeof(DWORD);

            memcpy(&FileTime,pbData,sizeof(FILETIME));
            pbData+=sizeof(FILETIME);

            memcpy(&StringLength,pbData,sizeof(DWORD));
            pbData+=sizeof(DWORD);

            // check message size
            if (dwDataSize<(2*sizeof(DWORD)+sizeof(TCHAR)+sizeof(FILETIME)+StringLength))
                return;

            // call report callback
            if (this->pCallBackReportMessage)
                this->pCallBackReportMessage((tagReportMessageType)ReportMessageType,(TCHAR*)pbData,FileTime,this->pCallBackReportMessagesUserParam);

        }
        return;
    }
}

BOOL STDMETHODCALLTYPE CApiOverride::UnlockMonitoringLogHeap()
{
    ::SetEvent(this->hEvtMonitoringLogHeapUnlocked);
    return TRUE;
}
BOOL STDMETHODCALLTYPE CApiOverride::WaitAndLockMonitoringLogHeap()
{
    DWORD dwRet = ::WaitForSingleObject(this->hEvtMonitoringLogHeapUnlocked,INFINITE);
    return (dwRet==WAIT_OBJECT_0);
}

//-----------------------------------------------------------------------------
// Name: SetMonitoringLogHeap
// Object: allow to specify heap used for monitoring logs memory allocation
// Parameters :
//      in: HANDLE Heap : new heap
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetMonitoringLogHeap(HANDLE Heap)
{
    if (Heap==NULL)
        return FALSE;
    this->MonitoringHeap=Heap;
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: MonitoringCallback
// Object: call on each new monitoring event, parse the log buffer to put it in usable structure
// Parameters :
//      in: PBYTE LogBuffer : undecoded log buffer
// Return : 
//-----------------------------------------------------------------------------
void __fastcall CApiOverride::MonitoringCallback(PBYTE LogBuffer)
{
    //NOTICE : See encoding in LogAPI.cpp of InjectedDLL project to understand decoding

    ////////////////////////////////////////////////////////////////
    // convert the log buffer into a log entry structure
    ////////////////////////////////////////////////////////////////
    this->WaitAndLockMonitoringLogHeap();
    PLOG_ENTRY pLogEntry=(PLOG_ENTRY)HeapAlloc(this->MonitoringHeap,0,sizeof(LOG_ENTRY));

    SIZE_T SubStructSize;
    DWORD Lenght;
    DWORD ItemSize;
    WORD Cnt;

    // retrieve fixed size info
    pLogEntry->pHookInfos=(PLOG_ENTRY_FIXED_SIZE)HeapAlloc(this->MonitoringHeap,0,sizeof(LOG_ENTRY_FIXED_SIZE));
    memcpy(pLogEntry->pHookInfos,LogBuffer,sizeof(LOG_ENTRY_FIXED_SIZE));
    LogBuffer+=sizeof(LOG_ENTRY_FIXED_SIZE);

    // size of pszModuleName including \0;
    memcpy(&ItemSize,LogBuffer,sizeof(DWORD));
    LogBuffer+=sizeof(DWORD);

    // we could directly point to local buffer, but to make same memory 
    // allocation and deletion as done in load file, we allocate new buffer
    pLogEntry->pszModuleName=(TCHAR*)HeapAlloc(this->MonitoringHeap,0,ItemSize);
    memcpy(pLogEntry->pszModuleName,LogBuffer,ItemSize);
    LogBuffer+=ItemSize;

    // size of pszApiName including \0;
    memcpy(&ItemSize,LogBuffer,sizeof(DWORD));
    LogBuffer+=sizeof(DWORD);

    // we could directly point to local buffer, but to make same memory 
    // allocation and deletion as done in load file, we allocate new buffer
    pLogEntry->pszApiName=(TCHAR*)HeapAlloc(this->MonitoringHeap,0,ItemSize);
    memcpy(pLogEntry->pszApiName,LogBuffer,ItemSize);
    LogBuffer+=ItemSize;

    // size of pszCallingModuleName including \0
    memcpy(&ItemSize,LogBuffer,sizeof(DWORD));
    LogBuffer+=sizeof(DWORD);

    // we could directly point to local buffer, but to make same memory 
    // allocation and deletion as done in load file, we allocate new buffer
    pLogEntry->pszCallingModuleName=(TCHAR*)HeapAlloc(this->MonitoringHeap,0,ItemSize);
    memcpy(pLogEntry->pszCallingModuleName,LogBuffer,ItemSize);
    LogBuffer+=ItemSize;

    pLogEntry->ParametersInfoArray=NULL;
    // if func has params
    if (pLogEntry->pHookInfos->bNumberOfParameters>0)
    {
        // create array
        pLogEntry->ParametersInfoArray=(PARAMETER_LOG_INFOS*)HeapAlloc(this->MonitoringHeap,0,sizeof(PARAMETER_LOG_INFOS)*pLogEntry->pHookInfos->bNumberOfParameters);

        // fill array info for each parameter
        for (Cnt=0;Cnt<pLogEntry->pHookInfos->bNumberOfParameters;Cnt++)
        {
            // copy first parameters of PARAMETER_LOG_INFOS
            //DWORD dwType;
            //PBYTE Value;// if (dwSizeOfPointedData >0 )
            //DWORD dwSizeOfData;// size of Data. If <=REGISTER_BYTE_SIZE param value is stored in Value (no memory allocation) else in pbValue 
            //DWORD dwSizeOfPointedValue;// size of pbValue.
            SubStructSize = 3*sizeof(DWORD)+sizeof(PBYTE);

            memcpy(&pLogEntry->ParametersInfoArray[Cnt],LogBuffer,SubStructSize);
            LogBuffer+=SubStructSize;

            memcpy(&pLogEntry->ParametersInfoArray[Cnt].pszParameterName,LogBuffer,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE*sizeof(TCHAR));
            LogBuffer+=PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE*sizeof(TCHAR);

            // if pointed data
            if (pLogEntry->ParametersInfoArray[Cnt].dwSizeOfPointedValue)
            {
                pLogEntry->ParametersInfoArray[Cnt].pbValue=(BYTE*)HeapAlloc(this->MonitoringHeap,0,pLogEntry->ParametersInfoArray[Cnt].dwSizeOfPointedValue);
                memcpy(pLogEntry->ParametersInfoArray[Cnt].pbValue,LogBuffer,pLogEntry->ParametersInfoArray[Cnt].dwSizeOfPointedValue);
                LogBuffer+=pLogEntry->ParametersInfoArray[Cnt].dwSizeOfPointedValue;
            }
            // if more than sizeof(PBYTE) bytes param
            else if (pLogEntry->ParametersInfoArray[Cnt].dwSizeOfData>sizeof(PBYTE))
            {
                pLogEntry->ParametersInfoArray[Cnt].pbValue=(BYTE*)HeapAlloc(this->MonitoringHeap,0,pLogEntry->ParametersInfoArray[Cnt].dwSizeOfData);
                memcpy(pLogEntry->ParametersInfoArray[Cnt].pbValue,LogBuffer,pLogEntry->ParametersInfoArray[Cnt].dwSizeOfData);
                LogBuffer+=pLogEntry->ParametersInfoArray[Cnt].dwSizeOfData;
            }
            else
            {
                pLogEntry->ParametersInfoArray[Cnt].pbValue=0;
            }
            if (pLogEntry->ParametersInfoArray[Cnt].dwType & EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_DEFINE_VALUES_FILE)
            {
                memcpy(&Lenght,LogBuffer,sizeof(DWORD));
                LogBuffer+=sizeof(DWORD);

                pLogEntry->ParametersInfoArray[Cnt].pszDefineNamesFile = (TCHAR*)HeapAlloc(this->MonitoringHeap,0,Lenght);
                memcpy(pLogEntry->ParametersInfoArray[Cnt].pszDefineNamesFile , LogBuffer ,Lenght);
                LogBuffer+=Lenght;
            }
            else
                pLogEntry->ParametersInfoArray[Cnt].pszDefineNamesFile = NULL;

            if (pLogEntry->ParametersInfoArray[Cnt].dwType & EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_USER_DATA_TYPE_FILE)
            {
                memcpy(&Lenght,LogBuffer,sizeof(DWORD));
                LogBuffer+=sizeof(DWORD);

                pLogEntry->ParametersInfoArray[Cnt].pszUserDataTypeName = (TCHAR*)HeapAlloc(this->MonitoringHeap,0,Lenght);
                memcpy(pLogEntry->ParametersInfoArray[Cnt].pszUserDataTypeName , LogBuffer ,Lenght);
                LogBuffer+=Lenght;
            }
            else
                pLogEntry->ParametersInfoArray[Cnt].pszUserDataTypeName = NULL;
        }
    }

    pLogEntry->CallSackInfoArray=NULL;
    // if func has params
    if (pLogEntry->pHookInfos->CallStackSize>0)
    {
        pLogEntry->CallSackInfoArray=(CALLSTACK_ITEM_INFO*)HeapAlloc(this->MonitoringHeap,0,sizeof(CALLSTACK_ITEM_INFO)*pLogEntry->pHookInfos->CallStackSize);
        if (pLogEntry->CallSackInfoArray)
        {
            // fill array info for each call info
            for (Cnt=0;Cnt<pLogEntry->pHookInfos->CallStackSize;Cnt++)
            {
                // copy address
                memcpy(&pLogEntry->CallSackInfoArray[Cnt].Address,LogBuffer,sizeof(PBYTE));
                LogBuffer+=sizeof(PBYTE);
                // copy relative address
                memcpy(&pLogEntry->CallSackInfoArray[Cnt].RelativeAddress,LogBuffer,sizeof(PBYTE));
                LogBuffer+=sizeof(PBYTE);
                // copy length of module name
                memcpy(&ItemSize,LogBuffer,sizeof(DWORD));
                LogBuffer+=sizeof(DWORD);
                
                // copy module name
                if (ItemSize>sizeof(TCHAR))// if there's more than '\0'
                {
                    pLogEntry->CallSackInfoArray[Cnt].pszModuleName=(TCHAR*)HeapAlloc(this->MonitoringHeap,0,ItemSize);
                    if (pLogEntry->CallSackInfoArray[Cnt].pszModuleName)
                        memcpy(pLogEntry->CallSackInfoArray[Cnt].pszModuleName,LogBuffer,ItemSize);
                    else
                    {
#ifdef _DEBUG
                        if (IsDebuggerPresent())// avoid to crash application if no debugger
                            DebugBreak();
#endif
                        pLogEntry->CallSackInfoArray[Cnt].pszModuleName=NULL;
                    }
                }
                else
                    pLogEntry->CallSackInfoArray[Cnt].pszModuleName=NULL;
                LogBuffer+=ItemSize;

                // copy stack parameters
                if (pLogEntry->pHookInfos->CallStackEbpRetrievalSize==0)
                    pLogEntry->CallSackInfoArray[Cnt].Parameters=NULL;
                else
                {
                    pLogEntry->CallSackInfoArray[Cnt].Parameters=(BYTE*)HeapAlloc(this->MonitoringHeap,0,pLogEntry->pHookInfos->CallStackEbpRetrievalSize);
                    if (pLogEntry->CallSackInfoArray[Cnt].Parameters)
                    {
                        memcpy(pLogEntry->CallSackInfoArray[Cnt].Parameters,LogBuffer,pLogEntry->pHookInfos->CallStackEbpRetrievalSize);
                    }
                    else
                    {
#ifdef _DEBUG
                        if (IsDebuggerPresent())// avoid to crash application if no debugger
                            DebugBreak();
#endif
                    }
                    LogBuffer+=pLogEntry->pHookInfos->CallStackEbpRetrievalSize;
                }
            }
        }
        else
        {
            pLogEntry->CallSackInfoArray=0;
#ifdef _DEBUG
            if (IsDebuggerPresent())// avoid to crash application if no debugger
                DebugBreak();
#endif
        }
    }
    switch (pLogEntry->pHookInfos->HookType)
    {
    case HOOK_TYPE_COM:
        memcpy(&pLogEntry->HookTypeExtendedFunctionInfos.InfosForCOM,LogBuffer,sizeof(tagExtendedFunctionInfosForCOM));
        LogBuffer+=sizeof(tagExtendedFunctionInfosForCOM);
        break;
    case HOOK_TYPE_NET:
        memcpy(&pLogEntry->HookTypeExtendedFunctionInfos.InfosForNET,LogBuffer,sizeof(tagExtendedFunctionInfosForNET));
        LogBuffer+=sizeof(tagExtendedFunctionInfosForNET);
        break;
    //case HOOK_TYPE_API:
    //default:
    //    break;
    }

    // call the monitoring call back using decoded struct
    this->MonitoringCallback(pLogEntry);

    // free memory if log is no more required
    if (!this->bManualFreeLogEntry)
        CApiOverride::FreeLogEntry(pLogEntry,this->MonitoringHeap);

    this->UnlockMonitoringLogHeap();
}

//-----------------------------------------------------------------------------
// Name: FreeLogEntry
// Object: Free a log entry (use it only if you've specified 
//          a manual free in SetMonitoringCallback call
// Parameters :
//      in: LOG_ENTRY* pLog : Log entry to free
// Return : 
//-----------------------------------------------------------------------------
void CApiOverride::FreeLogEntry(LOG_ENTRY* pLog)
{
    return this->FreeLogEntry(pLog,GetProcessHeap());
}

//-----------------------------------------------------------------------------
// Name: FreeLogEntry
// Object: Free a log entry (use it only if you've specified 
//          a manual free in SetMonitoringCallback call
// Parameters :
//      in: LOG_ENTRY* pLog : Log entry to free
//          HANDLE Heap : heap specified by SetMonitoringLogHeap.
//                        if you don't call SetMonitoringLogHeap, use CApiOverride::FreeLogEntry(LOG_ENTRY* pLog)
// Return : 
//-----------------------------------------------------------------------------
void STDMETHODCALLTYPE CApiOverride::FreeLogEntry(LOG_ENTRY* pLog,HANDLE Heap)
{
    return CApiOverride::FreeLogEntryStatic(pLog,Heap);
}

//-----------------------------------------------------------------------------
// Name: FreeLogEntryStatic
// Object: Free a log entry (use it only if you've specified 
//          a manual free in SetMonitoringCallback call
// Parameters :
//      in: LOG_ENTRY* pLog : Log entry to free
// Return : 
//-----------------------------------------------------------------------------
void CApiOverride::FreeLogEntryStatic(LOG_ENTRY* pLog)
{
    return CApiOverride::FreeLogEntryStatic(pLog,GetProcessHeap());
}

//-----------------------------------------------------------------------------
// Name: FreeLogEntryStatic
// Object: Free a log entry (use it only if you've specified 
//          a manual free in SetMonitoringCallback call
// Parameters :
//      in: LOG_ENTRY* pLog : Log entry to free
//          HANDLE Heap : heap specified by SetMonitoringLogHeap.
//                        if you don't call SetMonitoringLogHeap, use CApiOverride::FreeLogEntry(LOG_ENTRY* pLog)
// Return : 
//-----------------------------------------------------------------------------
void CApiOverride::FreeLogEntryStatic(LOG_ENTRY* pLog,HANDLE Heap)
{
    WORD cnt;
    // free parameter array
    for (cnt=0;cnt<pLog->pHookInfos->bNumberOfParameters;cnt++)
    {
        if (pLog->ParametersInfoArray[cnt].pbValue)
        {
            HeapFree(Heap,0, pLog->ParametersInfoArray[cnt].pbValue);
            pLog->ParametersInfoArray[cnt].pbValue=NULL;
        }

        if (pLog->ParametersInfoArray[cnt].pszDefineNamesFile)
        {
            HeapFree(Heap,0, pLog->ParametersInfoArray[cnt].pszDefineNamesFile);
            pLog->ParametersInfoArray[cnt].pszDefineNamesFile=NULL;
        }

        if (pLog->ParametersInfoArray[cnt].pszUserDataTypeName)
        {
            HeapFree(Heap,0, pLog->ParametersInfoArray[cnt].pszUserDataTypeName);
            pLog->ParametersInfoArray[cnt].pszUserDataTypeName=NULL;
        }
    }
    if (pLog->ParametersInfoArray)
        HeapFree(Heap,0,pLog->ParametersInfoArray);

    // free call stack array
    for (cnt=0;cnt<pLog->pHookInfos->CallStackSize;cnt++)
    {
        if (pLog->CallSackInfoArray[cnt].pszModuleName)
        {
            HeapFree(Heap,0,pLog->CallSackInfoArray[cnt].pszModuleName);
            pLog->CallSackInfoArray[cnt].pszModuleName=NULL;
        }
        if (pLog->CallSackInfoArray[cnt].Parameters)
        {
            HeapFree(Heap,0,pLog->CallSackInfoArray[cnt].Parameters);
            pLog->CallSackInfoArray[cnt].Parameters=NULL;
        }
    }
    if (pLog->CallSackInfoArray)
        HeapFree(Heap,0,pLog->CallSackInfoArray);

    HeapFree(Heap,0,pLog->pszModuleName);
    HeapFree(Heap,0,pLog->pszApiName);
    HeapFree(Heap,0,pLog->pszCallingModuleName);

    // free allocated buffer
    HeapFree(Heap,0,pLog->pHookInfos);
    
    // delete log entry itself
    HeapFree(Heap,0,pLog);
}


//-----------------------------------------------------------------------------
// Name: MonitoringCallback
// Object: call on each new monitoring event
// Parameters :
//      in: LOG_ENTRY* pLog : new monitored Log entry
// Return : 
//-----------------------------------------------------------------------------
void __fastcall CApiOverride::MonitoringCallback(LOG_ENTRY* pLog)
{
    if (this->pCallBackLogFunc)
    {
		// if a callback has been specified
		this->pCallBackLogFunc(pLog,this->pCallBackLogFuncUserParam);
    }
    else
	{
		if (this->pListview)
		{
			LOG_LIST_ENTRY lle;

			// fill LOG_LIST_ENTRY struct
			lle.dwId=this->pListview->GetItemCount();
			lle.pLog=pLog;
			lle.Type=ENTRY_LOG;

			this->AddLogEntry(&lle,FALSE);
		}
	}
}

//-----------------------------------------------------------------------------
// Name: AddLogEntry
// Object: add a log entry to listview
// Parameters :
//      in: LOG_LIST_ENTRY* pLogEntry : new Log entry
//          BOOL bStorePointerInListViewItemUserData : TRUE to store pLogEntry
//                  in listview item user data (and allow a speed way to get log entry data from a listview item)
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::AddLogEntry(LOG_LIST_ENTRY* pLogEntry,BOOL bStorePointerInListViewItemUserData)
{
    return this->AddLogEntry(pLogEntry,bStorePointerInListViewItemUserData,0);
}

//-----------------------------------------------------------------------------
// Name: AddLogEntry
// Object: add a log entry to listview
// Parameters :
//      in: LOG_LIST_ENTRY* pLogEntry : new Log entry
//          BOOL bStorePointerInListViewItemUserData : TRUE to store pLogEntry
//                  in listview item user data (and allow a speed way to get log entry data from a listview item)
//          int Increment : number or INCREMENT_STRING put before api name and parameters
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::AddLogEntry(LOG_LIST_ENTRY* pLogEntry,BOOL bStorePointerInListViewItemUserData,int Increment)
{
    if (!this->pListview)
        return FALSE;

    TCHAR* pp[NbColumns];
    TCHAR ppc[NbColumns][MAX_PATH+1];

    TCHAR* pc;
    DWORD CallSize;
    TCHAR* ParamString;
    BYTE Cnt;
    LOG_ENTRY* pLog=pLogEntry->pLog;

    // if log
    if (pLogEntry->Type==ENTRY_LOG)
    {

        // Id
        _itot(pLogEntry->dwId,ppc[ColumnsIndexId],10);

        // param direction
        switch (pLog->pHookInfos->bParamDirectionType)
        {
        case PARAM_DIR_TYPE_IN:
            _tcscpy(ppc[ColumnsIndexDirection],_T("In"));
            break;
        case PARAM_DIR_TYPE_OUT:
            _tcscpy(ppc[ColumnsIndexDirection],_T("Out"));
            break;
        case PARAM_DIR_TYPE_IN_NO_RETURN:
            _tcscpy(ppc[ColumnsIndexDirection],_T("InNoRet"));
            break;
        }

        // api name
        _tcsncpy(ppc[ColumnsIndexAPIName],pLog->pszApiName,MAX_PATH);
        ppc[ColumnsIndexAPIName][MAX_PATH]=0;

        // ret value
        if (pLog->pHookInfos->bParamDirectionType==PARAM_DIR_TYPE_IN_NO_RETURN)
            *ppc[ColumnsIndexReturnValue]=0;
        else
            _sntprintf(ppc[ColumnsIndexReturnValue],MAX_PATH,_T("0x%p"),pLog->pHookInfos->ReturnValue);


        // origin address
        _sntprintf(ppc[ColumnsIndexCallerAddress],MAX_PATH,_T("0x%p"),pLog->pHookInfos->pOriginAddress);

        // caller relative address

        // get short name
        pc=_tcsrchr(pLog->pszCallingModuleName,'\\');
        if (pc)
            pc++;
        else
            pc=pLog->pszCallingModuleName;

        if (pLog->pHookInfos->RelativeAddressFromCallingModuleName!=0)
        {
            _sntprintf(ppc[ColumnsIndexCallerRelativeIndex],MAX_PATH,_T("%s + 0x%p"),
                        pc,
                        pLog->pHookInfos->RelativeAddressFromCallingModuleName
                        );
            _tcscpy(ppc[ColumnsIndexCallerFullPath],pLog->pszCallingModuleName);
        }
        else
        {
            _tcscpy(ppc[ColumnsIndexCallerRelativeIndex],_T("Not Found"));
            *ppc[ColumnsIndexCallerFullPath]=0;
        }


        // process ID
        _sntprintf(ppc[ColumnsIndexProcessID],MAX_PATH,_T("0x%.8X") ,pLog->pHookInfos->dwProcessId);

        // Thread ID
        _sntprintf(ppc[ColumnsIndexThreadID],MAX_PATH,_T("0x%.8X") ,pLog->pHookInfos->dwThreadId);

        // module name
        _tcsncpy(ppc[ColumnsIndexModuleName],pLog->pszModuleName,MAX_PATH);
        ppc[ColumnsIndexModuleName][MAX_PATH]=0;


        // last error
        _sntprintf(ppc[ColumnsIndexLastError],MAX_PATH,_T("0x%.8x"),pLog->pHookInfos->dwLastError);

        // registers before call
        _sntprintf(ppc[ColumnsIndexRegistersBeforeCall],
                    MAX_PATH,
                    _T("EAX=0x%.8x, EBX=0x%.8x, ECX=0x%.8x, EDX=0x%.8x, ESI=0x%.8x, EDI=0x%.8x, EFL=0x%.8x, ESP=0x%.8x, EBP=0x%.8x"),
                    pLog->pHookInfos->RegistersBeforeCall.eax,
                    pLog->pHookInfos->RegistersBeforeCall.ebx,
                    pLog->pHookInfos->RegistersBeforeCall.ecx,
                    pLog->pHookInfos->RegistersBeforeCall.edx,
                    pLog->pHookInfos->RegistersBeforeCall.esi,
                    pLog->pHookInfos->RegistersBeforeCall.edi,
                    pLog->pHookInfos->RegistersBeforeCall.efl,
                    pLog->pHookInfos->RegistersBeforeCall.esp,
                    pLog->pHookInfos->RegistersBeforeCall.ebp
                    );

        // registers after call
        if (pLog->pHookInfos->bParamDirectionType==PARAM_DIR_TYPE_IN_NO_RETURN)
            *ppc[ColumnsIndexRegistersAfterCall]=0;
        else
            _sntprintf(ppc[ColumnsIndexRegistersAfterCall],
                    MAX_PATH,
                    _T("EAX=0x%.8x, EBX=0x%.8x, ECX=0x%.8x, EDX=0x%.8x, ESI=0x%.8x, EDI=0x%.8x, EFL=0x%.8x, ESP=0x%.8x, EBP=0x%.8x"),
                    pLog->pHookInfos->RegistersAfterCall.eax,
                    pLog->pHookInfos->RegistersAfterCall.ebx,
                    pLog->pHookInfos->RegistersAfterCall.ecx,
                    pLog->pHookInfos->RegistersAfterCall.edx,
                    pLog->pHookInfos->RegistersAfterCall.esi,
                    pLog->pHookInfos->RegistersAfterCall.edi,
                    pLog->pHookInfos->RegistersAfterCall.efl,
                    pLog->pHookInfos->RegistersAfterCall.esp,
                    pLog->pHookInfos->RegistersAfterCall.ebp
                    );

        if (pLog->pHookInfos->bParamDirectionType==PARAM_DIR_TYPE_IN_NO_RETURN)
            *ppc[ColumnsIndexFloatingReturnValue]=0;
        else
            _stprintf(ppc[ColumnsIndexFloatingReturnValue],_T("%.19g"), pLog->pHookInfos->DoubleResult);

        // Call time
        // Copy the time into a quadword.
        ULONGLONG ul;
        ul = (((ULONGLONG) pLog->pHookInfos->CallTime.dwHighDateTime) << 32) + pLog->pHookInfos->CallTime.dwLowDateTime;
        int Nano100s=(int)(ul%10);
        int MicroSeconds=(int)((ul/10)%1000);
        int MilliSeconds=(int)((ul/10000)%1000);
        int Seconds=(int)((ul/_SECOND)%60);
        int Minutes=(int)((ul/_MINUTE)%60);
        int Hours=(int)((ul/_HOUR)%24);
        _sntprintf(ppc[ColumnsIndexCallTime],MAX_PATH,_T("%.2u:%.2u:%.2u:%.3u:%.3u,%.1u"),
                            Hours,
                            Minutes,
                            Seconds,
                            MilliSeconds,
                            MicroSeconds,
                            Nano100s
                            );

        // Call duration
        if (pLog->pHookInfos->bParamDirectionType==PARAM_DIR_TYPE_IN_NO_RETURN)
            *ppc[ColumnsIndexCallDuration]=0;
        else
            _sntprintf(ppc[ColumnsIndexCallDuration],MAX_PATH,_T("%u"),pLog->pHookInfos->dwCallDuration);

        //////////////////////////
        // api name and parameters
        //////////////////////////

        // add api name

        *ppc[ColumnsIndexCall]=0;

        while (Increment>0)
        {
            _tcsncat(ppc[ColumnsIndexCall],INCREMENT_STRING,MAX_PATH-1-_tcslen(ppc[ColumnsIndexCall]));
            Increment--;
        }
 
        _tcsncat(ppc[ColumnsIndexCall],pLog->pszApiName,MAX_PATH-1-_tcslen(ppc[ColumnsIndexCall]));
        ppc[ColumnsIndexCall][MAX_PATH-1]=0;
        // add (
        _tcscat(ppc[ColumnsIndexCall],_T("("));

        CallSize=(DWORD)_tcslen(ppc[ColumnsIndexCall]);
        for (Cnt=0;Cnt<pLog->pHookInfos->bNumberOfParameters;Cnt++)
        {
            if (Cnt!=0)
            {
                // add param splitter
                _tcscat(ppc[ColumnsIndexCall],_T(","));
                CallSize++;
            }

            // translate param to string
            CSupportedParameters::ParameterToString(pLog->pszModuleName,&pLog->ParametersInfoArray[Cnt],&ParamString,APIOVERRIDE_MAX_ONE_PARAM_STRING_SIZE_FOR_CALL_COLUMN,FALSE);

            // put a limit to parameter size to avoid a big param hide over members in preview
            if (_tcslen(ParamString)>APIOVERRIDE_MAX_ONE_PARAM_STRING_SIZE_FOR_CALL_COLUMN)
            {
                ParamString[APIOVERRIDE_MAX_ONE_PARAM_STRING_SIZE_FOR_CALL_COLUMN-4]='.';
                ParamString[APIOVERRIDE_MAX_ONE_PARAM_STRING_SIZE_FOR_CALL_COLUMN-3]='.';
                ParamString[APIOVERRIDE_MAX_ONE_PARAM_STRING_SIZE_FOR_CALL_COLUMN-2]='.';
                ParamString[APIOVERRIDE_MAX_ONE_PARAM_STRING_SIZE_FOR_CALL_COLUMN-1]=0;
            }

            // add it to ppc[ColumnsIndexCall]
            _tcsncat(ppc[ColumnsIndexCall],ParamString,MAX_PATH-4-CallSize);

            // free string allocated by ParameterToString
            delete ParamString;

            // compute call size
            CallSize=(DWORD)_tcslen(ppc[ColumnsIndexCall]);

            // check size
            if (CallSize>=MAX_PATH-4)
            {
                // add ... at the end of string
                ppc[ColumnsIndexCall][MAX_PATH-4]='.';
                ppc[ColumnsIndexCall][MAX_PATH-3]='.';
                ppc[ColumnsIndexCall][MAX_PATH-2]='.';
                ppc[ColumnsIndexCall][MAX_PATH-1]=0;

                //avoid to add ")"
                CallSize=MAX_PATH;

                // stop parsing parameters
                break;
            }
        }
        if (CallSize<MAX_PATH-1)
            _tcscat(ppc[ColumnsIndexCall],_T(")"));


        // logging to listview
        for (int cnt=0;cnt<NbColumns;cnt++)// conversion from TCHAR[][] to TCHAR**
            pp[cnt]=(TCHAR*)ppc+cnt*(MAX_PATH+1);

    }

    // if user message
    else
    {
        TCHAR sz[20];
        TCHAR* psz;

        for (Cnt=0;Cnt<NbColumns;Cnt++)
            pp[Cnt]=0;

        // Id to string
        _itot(pLogEntry->dwId,sz,10);
        pp[ColumnsIndexId]=sz;

        // log type
        switch(pLogEntry->Type)
        {
        case ENTRY_MSG_WARNING:
            pp[ColumnsIndexDirection]=LISTVIEW_ITEM_TEXT_WARNING;
            break;
        case ENTRY_MSG_ERROR:
        case ENTRY_MSG_EXCEPTION:
            pp[ColumnsIndexDirection]=LISTVIEW_ITEM_TEXT_ERROR;
            break;
        case ENTRY_MSG_INFORMATION:
        default:
            pp[ColumnsIndexDirection]=LISTVIEW_ITEM_TEXT_INFORMATION;
            break;
        }

        // log message
        if (pLogEntry->Type==ENTRY_MSG_EXCEPTION) // particular case for exception (contains exception register and thread Id)
        {
            // pLogEntry->ReportEntry.pUserMsg is like "exception msg | registers | ProcessId | ThreadId"
            // make a local copy of pLogEntry->ReportEntry.pUserMsg to avoid to modify original buffer
            psz=(TCHAR*)_alloca((_tcslen(pLogEntry->ReportEntry.pUserMsg)+1)*sizeof(TCHAR));
            _tcscpy(psz,pLogEntry->ReportEntry.pUserMsg);
            // get "exception msg"
            pp[ColumnsIndexCall]=psz;

            psz=_tcsstr(psz,_T("||"));
            if (psz)
            {
                *psz=0;
                psz+=2;// _tcslen(_T("||"))
                // get registers
                pp[ColumnsIndexRegistersBeforeCall]=psz;

                for (Cnt=0;Cnt<2;Cnt++)
                {
                    psz=_tcschr(psz,'|');
                    if (!psz)
                        break;
                    *psz=0;
                    psz++;
                    switch(Cnt)
                    {
                    case 0:
                        // get process id
                        pp[ColumnsIndexProcessID]=psz;
                        break;
                    case 1:
                        // get thread id
                        pp[ColumnsIndexThreadID]=psz;
                        break;
                    }
                }
            }
        }
        else
            pp[ColumnsIndexCall]=pLogEntry->ReportEntry.pUserMsg;

        // Call time
        // Copy the time into a quadword.
        ULONGLONG ul;
        ul = (((ULONGLONG) pLogEntry->ReportEntry.ReportTime.dwHighDateTime) << 32) + pLogEntry->ReportEntry.ReportTime.dwLowDateTime;
        if (ul!=0)// backward compatibility
        {
            int Nano100s=(int)(ul%10);
            int MicroSeconds=(int)((ul/10)%1000);
            int MilliSeconds=(int)((ul/10000)%1000);
            int Seconds=(int)((ul/_SECOND)%60);
            int Minutes=(int)((ul/_MINUTE)%60);
            int Hours=(int)((ul/_HOUR)%24);
            _sntprintf(ppc[ColumnsIndexCallTime],MAX_PATH,_T("%.2u:%.2u:%.2u:%.3u:%.3u,%.1u"),
                Hours,
                Minutes,
                Seconds,
                MilliSeconds,
                MicroSeconds,
                Nano100s
                );
            pp[ColumnsIndexCallTime]=ppc[ColumnsIndexCallTime];
        }

    }


    int ItemIndex=this->pListview->GetItemCount();

    // sort by date in listview, so only id will be inverted remove this part if you dislike it
    // as id are already attributed it's the only sort we can do
    if (pLogEntry->Type==ENTRY_LOG)
    {
        LONGLONG PreviousTimeQuadPart;
        LONGLONG TimeQuadPart;
        LOG_LIST_ENTRY* pPreviousLogEntry;

        // get current log time
        TimeQuadPart=(((LONGLONG)pLogEntry->pLog->pHookInfos->CallTime.dwHighDateTime)<<32)
                        +pLogEntry->pLog->pHookInfos->CallTime.dwLowDateTime;
        // check previous items in list for quad part
        for (;ItemIndex>0;ItemIndex--)
        {
            // on error getting value
            if (!pListview->GetItemUserData(ItemIndex-1,(LPVOID*)(&pPreviousLogEntry)))
                break;

            // if bad pointer
            if (pPreviousLogEntry==0)
                break;

            if (IsBadReadPtr(pPreviousLogEntry,sizeof(LOG_LIST_ENTRY)))
                break;

            // if a log entry
            if (pPreviousLogEntry->Type==ENTRY_LOG)
            {

                // if bas pointer
                if (IsBadReadPtr(pPreviousLogEntry->pLog,sizeof(LOG_ENTRY)))
                    break;

                // if previous date is lower than current one, order is ok --> break;
                PreviousTimeQuadPart=(((LONGLONG)pPreviousLogEntry->pLog->pHookInfos->CallTime.dwHighDateTime)<<32)
                    +pPreviousLogEntry->pLog->pHookInfos->CallTime.dwLowDateTime;
            }
            else
            {
                if ((pPreviousLogEntry->Type==ENTRY_MSG_INFORMATION)
                    || (pPreviousLogEntry->Type==ENTRY_MSG_WARNING)
                    || (pPreviousLogEntry->Type==ENTRY_MSG_ERROR)
                    || (pPreviousLogEntry->Type==ENTRY_MSG_EXCEPTION)
                    )
                   
                {
                    PreviousTimeQuadPart=(((LONGLONG)pPreviousLogEntry->ReportEntry.ReportTime.dwHighDateTime)<<32)
                                            +pPreviousLogEntry->ReportEntry.ReportTime.dwLowDateTime;

                    // if no time information on report (backward compatibility)
                    if (PreviousTimeQuadPart==0)
                        break;
                }
                else
                    // not supported log entry type
                    break;
            }

            if (PreviousTimeQuadPart<=TimeQuadPart)
                break;

            // else current item must be inserted before previous one
            // check the next previous func to do same checking
        }
    }
    // end of sort by date in listview, so only id will be inverted remove this part if you dislike it

    if (bStorePointerInListViewItemUserData)
        this->pListview->AddItemAndSubItems(NbColumns,pp,ItemIndex,TRUE,pLogEntry);
    else
        // don't add the pLog as userparam because it will be invalid memory
        this->pListview->AddItemAndSubItems(NbColumns,pp,ItemIndex,TRUE);

    return TRUE;
}




//-----------------------------------------------------------------------------
// Name: ProcessInternalCall
// Object: call specified function with parameters specified in pParams in the remote process
//          and store function return (eax) in pRet
// Parameters :
//      in: TCHAR* LibName : function address
//          TCHAR* FuncName
//          DWORD NbParams : nb params in pParams
//          PSTRUCT_FUNC_PARAM pParams : array of STRUCT_FUNC_PARAM. Can be null if no params
//      out : PBYTE* pReturnValue : returned value
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::ProcessInternalCall(TCHAR* LibName,TCHAR* FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,PBYTE* pReturnValue)
{
    return this->ProcessInternalCall(LibName,FuncName,NbParams,pParams,pReturnValue,INFINITE);
}
//-----------------------------------------------------------------------------
// Name: ProcessInternalCall
// Object: call specified function with parameters specified in pParams in the remote process
//          and store function return (eax) in pRet
// Parameters :
//      in: TCHAR* LibName : function address
//          TCHAR* FuncName
//          DWORD NbParams : nb params in pParams
//          PSTRUCT_FUNC_PARAM pParams : array of STRUCT_FUNC_PARAM. Can be null if no params
//          DWORD dwTimeOutMs : max time in ms to wait for function reply (0xFFFFFFFF for INFINITE)
//      out : PBYTE* pReturnValue : returned value
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::ProcessInternalCall(TCHAR* LibName,TCHAR* FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,PBYTE* pReturnValue,DWORD dwTimeOutMs)
{
    REGISTERS Registers;
    memset(&Registers,0,sizeof(REGISTERS));
    return this->ProcessInternalCall(LibName,FuncName,NbParams,pParams,&Registers,pReturnValue,dwTimeOutMs);
}

//-----------------------------------------------------------------------------
// Name: ProcessInternalCall
// Object: call specified function with parameters specified in pParams in the remote process
//          and store function return (eax) in pRet
// Parameters :
//      in: TCHAR* LibName : function address
//          TCHAR* FuncName
//          DWORD NbParams : nb params in pParams
//          PSTRUCT_FUNC_PARAM pParams : array of STRUCT_FUNC_PARAM. Can be null if no params
//          DWORD dwTimeOutMs : max time in ms to wait for function reply (0xFFFFFFFF for INFINITE)
//      in out : REGISTERS* pRegisters : in : register before call, out : registers after call
//      out : PBYTE* ReturnValue : returned value
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::ProcessInternalCall(TCHAR* LibName,TCHAR* FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pReturnValue,DWORD dwTimeOutMs)
{
    double d;
    return this->ProcessInternalCall(LibName,FuncName,NbParams,pParams,pRegisters,pReturnValue,&d,dwTimeOutMs,0);
}
//-----------------------------------------------------------------------------
// Name: ProcessInternalCall
// Object: call specified function with parameters specified in pParams in the remote process
//          and store function return (eax) in pRet
// Parameters :
//      in: TCHAR* LibName : function address
//          TCHAR* FuncName
//          DWORD NbParams : number of parameters in pParams
//          PSTRUCT_FUNC_PARAM pParams : array of STRUCT_FUNC_PARAM. Can be null if no params
//          DWORD dwTimeOutMs : max time in ms to wait for function reply (0xFFFFFFFF for INFINITE)
//          DWORD ThreadID : thread id into which call must be done, 0 if no thread preference
//      in out : REGISTERS* pRegisters : in : register before call, out : registers after call
//      out : PBYTE* ReturnValue : returned value
//            double* FloatingReturn : floating result
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::ProcessInternalCall(TCHAR* LibName,TCHAR* FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pReturnValue,double* pFloatingReturn,DWORD dwTimeOutMs,DWORD ThreadId)
{
    return this->ProcessInternalCall(LibName,FuncName,NbParams,pParams,pRegisters,pReturnValue,pFloatingReturn,dwTimeOutMs,ThreadId,CALLING_CONVENTION_STDCALL_OR_CDECL);
}

//-----------------------------------------------------------------------------
// Name: ProcessInternalCall
// Object: call specified function with parameters specified in pParams in the remote process
//          and store function return (eax) in pRet
// Parameters :
//      in: TCHAR* LibName : function address
//          TCHAR* FuncName
//          DWORD NbParams : number of parameters in pParams
//          PSTRUCT_FUNC_PARAM pParams : array of STRUCT_FUNC_PARAM. Can be null if no params
//          DWORD dwTimeOutMs : max time in ms to wait for function reply (0xFFFFFFFF for INFINITE)
//          DWORD ThreadID : thread id into which call must be done, 0 if no thread preference
//          CALLING_CONVENTION CallingConvention : calling convention
//      in out : REGISTERS* pRegisters : in : register before call, out : registers after call
//      out : PBYTE* ReturnValue : returned value
//            double* FloatingReturn : floating result
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::ProcessInternalCall(TCHAR* LibName,TCHAR* FuncName,DWORD NbParams,PSTRUCT_FUNC_PARAM pParams,REGISTERS* pRegisters,PBYTE* pReturnValue,double* FloatingReturn,DWORD dwTimeOutMs,DWORD ThreadId,tagCALLING_CONVENTION CallingConvention)
{
    TCHAR pszMsg[2*MAX_PATH];
    PBYTE pb;
    BOOL bRet;
    DWORD dwRet;
    DWORD dw;
    REMOTE_CALL_INFOS RemoteCallInfos;
    CApiOverrideFuncAndParams* pApiOverrideFuncAndParams;
    CLinkListItem* pRemoteCallItem;
    DWORD dwCnt;
    HANDLE pH[3];
    RemoteCallInfos.ProcessInternalCallReply=NULL;

    EnterCriticalSection(&this->CriticalSection);
    if (!this->bAPIOverrideDllLoaded)
    {
        this->ShowApiOverrideNotStartedMsg();
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    if (IsBadReadPtr(pReturnValue,sizeof(PBYTE)))// don't check pParams because it can be NULL if no params
    {
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    RemoteCallInfos.hevtProcessInternalCallReply=CreateEvent(NULL,FALSE,FALSE,NULL);
    if (!RemoteCallInfos.hevtProcessInternalCallReply)
    {
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    // encode params and func name
    pApiOverrideFuncAndParams=new CApiOverrideFuncAndParams();
    if (!pApiOverrideFuncAndParams->Encode(&RemoteCallInfos,LibName,FuncName,NbParams,pParams,pRegisters,ThreadId,dwTimeOutMs,CallingConvention))
    {
        delete pApiOverrideFuncAndParams;
        CloseHandle(RemoteCallInfos.hevtProcessInternalCallReply);
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    // send cmd buffer to injected lib
    pb=new BYTE[pApiOverrideFuncAndParams->EncodedBufferSize+2*sizeof(DWORD)];
    dw=CMD_PROCESS_INTERNAL_CALL_QUERY;
    memcpy(pb,&dw,sizeof(DWORD));
    memcpy(&pb[sizeof(DWORD)],pApiOverrideFuncAndParams->EncodedBuffer,pApiOverrideFuncAndParams->EncodedBufferSize);
    
    // define events
    pH[0]=RemoteCallInfos.hevtProcessInternalCallReply;
    pH[1]=this->hevtFreeProcess;
    pH[2]=this->hevtProcessFree;

    // add address of RemoteCallInfos to current pCurrentRemoteCalls list
    pRemoteCallItem=this->pCurrentRemoteCalls->AddItem(&RemoteCallInfos);

    bRet=this->pMailSlotClient->Write(pb,pApiOverrideFuncAndParams->EncodedBufferSize+sizeof(DWORD));
    if (!bRet)
    {
        delete[] pb;
        delete pApiOverrideFuncAndParams;
        CloseHandle(RemoteCallInfos.hevtProcessInternalCallReply);
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }
WaitEvent:
    // wait for injected lib reply
    dwRet=WaitForMultipleObjects(3,pH,FALSE,dwTimeOutMs);
    switch(dwRet)
    {
        case WAIT_TIMEOUT:
            _sntprintf(pszMsg,2*MAX_PATH,_T("Error no reply for %s:%s call in %d sec\r\nDo you want to wait more ?"),FuncName,LibName,dwTimeOutMs/1000);
            // report message in default window, as this function may not called
            // by parent window (in winapioverride it's avoid to brings main window upper than the call window 
            // which do the call)
            if (this->UserMessage(NULL,pszMsg,_T("Question"),MB_YESNO|MB_ICONQUESTION)==IDYES)
                goto WaitEvent;

            // remove address of RemoteCallInfos to current pCurrentRemoteCalls list
            this->pCurrentRemoteCalls->RemoveItem(pRemoteCallItem);
            delete[] pb;
            delete pApiOverrideFuncAndParams;
            CloseHandle(RemoteCallInfos.hevtProcessInternalCallReply);
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;

        case WAIT_OBJECT_0:
            break;
        case WAIT_OBJECT_0+1:
        case WAIT_OBJECT_0+2:
        default:
            // remove address of RemoteCallInfos to current pCurrentRemoteCalls list
            this->pCurrentRemoteCalls->RemoveItem(pRemoteCallItem);
            delete[] pb;
            delete pApiOverrideFuncAndParams;
            CloseHandle(RemoteCallInfos.hevtProcessInternalCallReply);
            LeaveCriticalSection(&this->CriticalSection);
            return FALSE;
    }

    // remove address of RemoteCallInfos to current pCurrentRemoteCalls list
    this->pCurrentRemoteCalls->RemoveItem(pRemoteCallItem);

    // decode reply
    bRet=pApiOverrideFuncAndParams->Decode(RemoteCallInfos.ProcessInternalCallReply);
    if ((!bRet)
        ||(pApiOverrideFuncAndParams->DecodedCallSuccess==FALSE)
        ||(NbParams!=pApiOverrideFuncAndParams->DecodedNbParams))
    {
        _sntprintf(pszMsg,2*MAX_PATH,_T("Error during the call of %s:%s"),FuncName,LibName);
        ReportError(NULL,pszMsg);// report error in default window, as this function may not called
        // by parent window (in winapioverride it's avoid to brings main window upper than the call window 
        // which do the call)
        delete[] pb;
        delete pApiOverrideFuncAndParams;
        if (RemoteCallInfos.ProcessInternalCallReply)
            delete RemoteCallInfos.ProcessInternalCallReply;
        CloseHandle(RemoteCallInfos.hevtProcessInternalCallReply);
        LeaveCriticalSection(&this->CriticalSection);
        return FALSE;
    }

    // copy data of received buffer
    *pReturnValue=pApiOverrideFuncAndParams->DecodedReturnedValue;

    // copy pParams
    for (dwCnt=0;dwCnt<pApiOverrideFuncAndParams->DecodedNbParams;dwCnt++)
    {
        // avoid buffer overflow here
        if (pApiOverrideFuncAndParams->DecodedParams[dwCnt].dwDataSize>pParams[dwCnt].dwDataSize)
            pApiOverrideFuncAndParams->DecodedParams[dwCnt].dwDataSize=pParams[dwCnt].dwDataSize;
        memcpy(pParams[dwCnt].pData,
            pApiOverrideFuncAndParams->DecodedParams[dwCnt].pData,
            pApiOverrideFuncAndParams->DecodedParams[dwCnt].dwDataSize=pParams[dwCnt].dwDataSize
            );
    }

    // copy Registers
    memcpy(pRegisters,&pApiOverrideFuncAndParams->DecodedRegisters,sizeof(REGISTERS));

    // copy floating return
    *FloatingReturn=pApiOverrideFuncAndParams->DecodedFloatingReturn;

    delete[] pb;
    delete pApiOverrideFuncAndParams;
    if (RemoteCallInfos.ProcessInternalCallReply)
        delete RemoteCallInfos.ProcessInternalCallReply;
    CloseHandle(RemoteCallInfos.hevtProcessInternalCallReply);
    LeaveCriticalSection(&this->CriticalSection);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ShowApiOverrideNotStartedMsg
// Object: show a standart error msg
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
void CApiOverride::ShowApiOverrideNotStartedMsg()
{
    ReportError(_T("Error ApiOverride not Started"));
}

//-----------------------------------------------------------------------------
// Name: SetUnexpectedUnload
// Object: Set call back for unexpected unload
//         This call back will be call if host process unload the dll without we ask it to do
//         It is call when host process close
// Parameters :
//     in : - FARPROC pCallBackFunc : callback function
//          - LPVOID pUserParam : parameter for the callback
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetUnexpectedUnloadCallBack(tagCallBackUnexpectedUnload pCallBackFunc,LPVOID pUserParam)
{
    if (IsBadCodePtr((FARPROC)pCallBackFunc) && (pCallBackFunc!=0)) // pCallBackFunc==0 to remove callback
        return FALSE;
    this->pCallBackUnexpectedUnloadFunc=pCallBackFunc;
    this->pCallBackUnexpectedUnloadFuncUserParam=pUserParam;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetReportMessagesCallBack
// Object: Set call back for report messages
// Parameters :
//     in : - FARPROC pCallBackFunc : callback function
//          - LPVOID pUserParam : parameter for the callback
// Return : 
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CApiOverride::SetReportMessagesCallBack(tagCallBackReportMessages pCallBackFunc,LPVOID pUserParam)
{
    if (IsBadCodePtr((FARPROC)pCallBackFunc) && (pCallBackFunc!=0)) // pCallBackFunc==0 to remove callback
        return FALSE;
    this->pCallBackReportMessage=pCallBackFunc;
    this->pCallBackReportMessagesUserParam=pUserParam;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: DllUnloadedCallBack
// Object: object callBack for dll unloading
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
void CApiOverride::DllUnloadedCallBack()
{
    if (this->bDestructorCalled) // return if destructor called, else EnterCriticalSection will cause a deadlock
        return;

    EnterCriticalSection(&this->CriticalSection);
    DWORD LocalCurrentProcessId = this->dwCurrentProcessId;
    PVOID LocalpCallBackUnexpectedUnloadFuncUserParam = this->pCallBackUnexpectedUnloadFuncUserParam;
    tagCallBackUnexpectedUnload LocalpCallBackUnexpectedUnloadFunc = this->pCallBackUnexpectedUnloadFunc;

    // signal that the dll is no more loaded (must be done before calling Stop)
    this->bAPIOverrideDllLoaded=FALSE;

    // wait the end of message retrieval
    this->pMailSlotServer->WaitUntilNoMessageDuringSpecifiedTime(
                                            APIOVERRIDE_NO_MORE_MESSAGE_IF_NO_MESSAGE_DURING_TIME_IN_MS,
                                            this->hevtFreeProcess);

    // Stop all threads and mailslots
    this->StopWithoutEnteringCriticalSection(TRUE);

    // leave critical section now as callback may want to delete current object
    // and in this case we go to a deadlock
    LeaveCriticalSection(&this->CriticalSection);

    // else call process exit associated callback
    // object may have been destroyed here --> use local var
    if (LocalpCallBackUnexpectedUnloadFunc)
        LocalpCallBackUnexpectedUnloadFunc(LocalCurrentProcessId,LocalpCallBackUnexpectedUnloadFuncUserParam);
  
}
//-----------------------------------------------------------------------------
// Name: ShowApiOverrideNotStartedMsg
// Object: watch for dll unloading and call associated object method
// Parameters :
//     in : LPVOID lpParam : CApiOverride*
// Return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CApiOverride::DllUnloadedThreadListener(LPVOID lpParam)
{
    CApiOverride* pCApiOverride=(CApiOverride*)lpParam;
    DWORD dwRes;
    DWORD RetValue = 0;
    HANDLE hProcess = ::OpenProcess(SYNCHRONIZE,FALSE,pCApiOverride->dwCurrentProcessId);
#define CApiOverride_DllUnloadedThreadListener_NB_EVENTS 3
    HANDLE ph[CApiOverride_DllUnloadedThreadListener_NB_EVENTS]={pCApiOverride->hevtAPIOverrideDllProcessDetachCompleted,hProcess,pCApiOverride->hevtFreeProcess};
    
    dwRes=WaitForMultipleObjects(CApiOverride_DllUnloadedThreadListener_NB_EVENTS,ph,FALSE,INFINITE);
    switch (dwRes)
    {
    case WAIT_OBJECT_0:// hevtAPIOverrideDllProcessDetachCompleted
        RetValue = 0xFFFFFFFD;
        // dll is unloading, but we don't have ask it
        pCApiOverride->DllUnloadedCallBack();
        break;
    case WAIT_OBJECT_0+1://hProcess process crash
        RetValue = 0xFFFFFFFE;
        // dll is unloading, but we don't have ask it
        pCApiOverride->DllUnloadedCallBack();
        break;
    case WAIT_OBJECT_0+2:// hevtFreeProcess
        // we have ask to free process
        break;
    default:// error occurs
        CAPIError::ShowLastError();
        RetValue = 0xFFFFFFFF;
        break;
    }

    ::CloseHandle(hProcess);
    return RetValue;
}


//-----------------------------------------------------------------------------
// Name: HookEntryPoint
// Object: hook entry point of a process created with CREATE_SUSPENDED arg
//          to free memory you have to call HookEntryPointFree after having resuming thread
//          call order  1)HookEntryPoint
//                      2)ResumeProcess
//                      3)HookEntryPointFree
// Parameters : 
//     in : TCHAR* pszFileName : name of exe launched in suspended mode
//          DWORD dwProcessId : process id of the exe launched (returned by CreateProcess)
//          HANDLE hThreadHandle : thread handle of the exe launched (returned by CreateProcess)
// Return : 
//-----------------------------------------------------------------------------
BOOL CApiOverride::HookEntryPoint(TCHAR* pszFileName, DWORD dwProcessId,HANDLE hProcessHandle,HANDLE hThreadHandle,BOOL* pTlsHook)
{
    // we can't directly change the eip as VirtualQueryEx returns PAGE_NO_ACCESS
    // we may could play with SetThreadContext for the main thread of the application, 
    // but if it works as the GetThreadContext I don't want to try it (quite work or not according to patchs)
    BOOL bSuccess=FALSE;

    *pTlsHook=FALSE;
    CPE Pe(pszFileName);
    if (!Pe.Parse(FALSE,FALSE,this->bAllowTlsCallBackHook))
        return FALSE;

    #define SIZEOF_HOOK_PROXY (1+sizeof(PBYTE)) // better to compute size for this 
    #define SIZEOF_HOOK 2000 // something enough (don't need to compute size)

    DWORD dwHookEndFlag=CApiOverride_SUSPENDED_HOOK_END_FLAG;
    DWORD dw=0;

#if (defined(UNICODE)||defined(_UNICODE))
    FARPROC pLoadLibrary=GetProcAddress(GetModuleHandle(_T("kernel32.dll")),"LoadLibraryW");
#else
    FARPROC pLoadLibrary=GetProcAddress(GetModuleHandle(_T("kernel32.dll")),"LoadLibraryA");
#endif

    FARPROC pGetCurrentThread=GetProcAddress(GetModuleHandle(_T("kernel32.dll")),"GetCurrentThread");
    FARPROC pSuspendThread=GetProcAddress(GetModuleHandle(_T("kernel32.dll")),"SuspendThread");

    TCHAR LocalLibName[MAX_PATH];
    SIZE_T dwTransferedSize=0;
    
    BYTE BufferIndex;

    BYTE LocalOriginalOpCode[SIZEOF_HOOK_PROXY];
    BYTE LocalProxy[SIZEOF_HOOK_PROXY];
    BYTE LocalHook[SIZEOF_HOOK];

    PBYTE RemoteHook;
    PBYTE RemoteLibName;

    _tcscpy(LocalLibName,this->pszAppPath);
    _tcscat(LocalLibName,API_OVERRIDE_DLL_NAME);

    PBYTE EntryPointAddress=0;
    PBYTE ExeBaseAddress=0;

    if (Pe.IsDynamicallyBased())
    {
        ExeBaseAddress = CSuspendedProcess::GetBaseAddress(hProcessHandle,hThreadHandle);
        if (ExeBaseAddress == 0)
            return FALSE;
    }
    else
    {
        ExeBaseAddress = (PBYTE)Pe.NTHeader.OptionalHeader.ImageBase;
    }

    this->HookEntryPointpProcessMemory=new CProcessMemory(dwProcessId,FALSE);

    // check for tls callbacks
    if (Pe.pTlsCallbacksTable->GetItemsCount()>0)
    {
        BOOL ValidItemFound=FALSE;
        CLinkListItem* pItem;
        ULONGLONG StartAddress;

        // loop through all tls callback to get a valid one 
        // Notice : some packers use tls callback array to store data; so tls callbacks are invalid memory pointers
        //          (Nt loader seems to be not disturb by invalid tls callbacks)
        for (pItem=Pe.pTlsCallbacksTable->Head;pItem;pItem=pItem->NextItem)
        {
            // get first valid callback start address
            StartAddress=*(ULONGLONG*)(pItem->ItemData);
            EntryPointAddress=(PBYTE)StartAddress;

            // 5.3.2 : memory is protected, we can't access it yet. So use Pe information instead
            // if (this->HookEntryPointpProcessMemory->IsValidMemory(EntryPointAddress,SIZEOF_HOOK_PROXY))
            if (Pe.IsExecutable((ULONGLONG)EntryPointAddress - Pe.NTHeader.OptionalHeader.ImageBase))
            {
                ValidItemFound=TRUE;
                break;
            }
        }

        // if bad tls callbacks have been found before the first valid one
        // ( message wont be display if first tls is valid and not the other(s) )
        if (pItem!=Pe.pTlsCallbacksTable->Head)
        {
            FILETIME ft;
            // get UTC time
            GetSystemTimeAsFileTime(&ft);
            // convert UTC time to local filetime
            FileTimeToLocalFileTime(&ft,&ft);
            // call report callback
            TCHAR szMsg[2*MAX_PATH];
            _sntprintf(szMsg,2*MAX_PATH,_T("Invalid TLS callbacks detected in %s"),pszFileName);
            if (this->pCallBackReportMessage)
                this->pCallBackReportMessage(REPORT_MESSAGE_ERROR,szMsg,ft,this->pCallBackReportMessagesUserParam);
        }


        // if valid tls has been found
        if (ValidItemFound)
        {
            // signal output parameter we use tls callback
            *pTlsHook=TRUE;

            // set event accessible for all account. Doing this allow to event to be opened by other user
            // --> we can inject dll into processes running under other users accounts
            SECURITY_DESCRIPTOR sd={0};
            sd.Revision=SECURITY_DESCRIPTOR_REVISION;
            sd.Control=SE_DACL_PRESENT;
            sd.Dacl=NULL; // assume everyone access
            SECURITY_ATTRIBUTES SecAttr={0};
            SecAttr.bInheritHandle=FALSE;
            SecAttr.nLength=sizeof(SECURITY_ATTRIBUTES);
            SecAttr.lpSecurityDescriptor=&sd;

            TCHAR pszPID[32];
            TCHAR psz[MAX_PATH];
            _stprintf(pszPID,_T("0x%X"),dwProcessId);
            _tcscpy(psz,APIOVERRIDE_EVENT_SINGLETHREADEDMAILSLOTSERVER_END_WAITING_INSTRUCTIONS_LOOP);
            _tcscat(psz,pszPID);
            this->hevtTlsHookEndLoop=CreateEvent(&SecAttr,FALSE,FALSE,psz);

            if (Pe.IsDynamicallyBased())
            {
                // do relocation
                EntryPointAddress = EntryPointAddress - Pe.NTHeader.OptionalHeader.ImageBase + (ULONG_PTR)ExeBaseAddress;
            }
        }
    }
    
    if (!*pTlsHook)
    {
        EntryPointAddress=ExeBaseAddress + Pe.NTHeader.OptionalHeader.AddressOfEntryPoint;
    }

    // assume memory page has read write execute attributes
    DWORD OldProtection;
    if (!this->HookEntryPointpProcessMemory->Protect((LPVOID)EntryPointAddress,SIZEOF_HOOK_PROXY,PAGE_EXECUTE_READWRITE,&OldProtection))
        return FALSE;

    // read original entry opcode
    if (!this->HookEntryPointpProcessMemory->Read(
                                                    (LPVOID)EntryPointAddress,
                                                    LocalOriginalOpCode,
                                                    SIZEOF_HOOK_PROXY,
                                                    &dwTransferedSize)
        )
        return FALSE;

    // allocate memory for the Hook
    RemoteHook=(PBYTE)this->HookEntryPointpProcessMemory->Alloc(SIZEOF_HOOK);
    if (!RemoteHook)
        return FALSE;

    // allocate memory in remote process to store Library name
    RemoteLibName=(PBYTE)this->HookEntryPointpProcessMemory->Alloc((_tcslen(LocalLibName)+1)*sizeof(TCHAR));
    if (!RemoteLibName)
        return FALSE;

    // copy Library name in remote process
    if (!this->HookEntryPointpProcessMemory->Write(
                                                    (LPVOID)RemoteLibName,
                                                    LocalLibName,
                                                    (_tcslen(LocalLibName)+1)*sizeof(TCHAR),
                                                    &dwTransferedSize)
        )
        return FALSE;

    //// code for absolute jump
    // #define SIZEOF_HOOK_PROXY 7
    // // jump Hook Address
    // LocalProxy[0]=0xB8;// mov eax,
    // memcpy(&LocalProxy[1],&RemoteHook,sizeof(DWORD));// Hook Address
    // LocalProxy[5]=0xFF;LocalProxy[6]=0xE0;//jmp eax absolute 

    // make a relative jump
    dw=(DWORD)(RemoteHook-EntryPointAddress-SIZEOF_HOOK_PROXY);
    // jump relative
    LocalProxy[0]=0xE9;
    memcpy(&LocalProxy[1],&dw,sizeof(DWORD));// Hook Address


    ///////////////////////
    // fill hook data
    // algorithm is the following :
    //
    //      reserve stack for return address
    //      save registers and flag registers
    //      fill return address
    //
    //      ///////////////////////////////////////
    //      ///// specifics operations to do 
    //      ///////////////////////////////////////
    //
    //      // load our spy library
    //      LoadLibrary(RemoteLibName)
    //
    //      ///////////////////////////////////////
    //      ///// End of specifics operations to do 
    //      ///////////////////////////////////////
    //
    //      //do some action that can tell the calling process that the hook is ending
    //      //  so it can free memory
    //      
    //      // suspend thread to allow monitoring files and overriding dll loading
    //
    //
    //      // restore registers and flag registers
    //
    //      // jump to Entry point
    //
    ///////////////////////

    BufferIndex=0;

    // reserve stack for return address
    LocalHook[BufferIndex++]=0x50;// push eax

    // save registers and flag registers
    LocalHook[BufferIndex++]=0x60;//pushad
    LocalHook[BufferIndex++]=0x9c;//pushfd

    //////////////////////////////////
    // fill return address
    //////////////////////////////////

    //mov eax, esp
    //add eax, 0x24 // sizeof pushad+pushfd
    //mov ebx, return address (Entry point)
    //mov [eax],ebx

    //mov eax, esp
    LocalHook[BufferIndex++]=0x8B;
    LocalHook[BufferIndex++]=0xC4;

    //add eax, 0x24
    LocalHook[BufferIndex++]=0x83;
    LocalHook[BufferIndex++]=0xC0;
    LocalHook[BufferIndex++]=0x24;

    //mov ebx, return address (Entry point)
    LocalHook[BufferIndex++]=0xBB;
    memcpy(&LocalHook[BufferIndex],&EntryPointAddress,sizeof(DWORD)); // return address (Entry point)   
    BufferIndex+=sizeof(DWORD);

    //mov [eax],ebx
    LocalHook[BufferIndex++]=0x89;
    LocalHook[BufferIndex++]=0x18;
    

    //////////////////////////////////
    // push libname
    //////////////////////////////////
    LocalHook[BufferIndex++]=0xB8; // mov eax,
    memcpy(&LocalHook[BufferIndex],&RemoteLibName,sizeof(DWORD)); // LibName Address   
    BufferIndex+=sizeof(DWORD);
    LocalHook[BufferIndex++]=0x50;// push eax

    // call load library
    LocalHook[BufferIndex++]=0xB8;// mov eax,
    memcpy(&LocalHook[BufferIndex],&pLoadLibrary,sizeof(DWORD)); // LoadLibrary Address
    BufferIndex+=sizeof(DWORD);
    LocalHook[BufferIndex++]=0xFF;LocalHook[BufferIndex++]=0xD0; // call eax

    // we are in stdcall --> parameters are removed from stack
     
    //////////////////////////////////////////////////////////////////////
    //do some action that can tell the calling process that the hook is ending
    //////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////
    // here we change some remotely allocated memory to signal end of hook
    // it's allow for remote process to do polling on this memory pointer
    // (Notice you can use Named event or whatever you want if you dislike
    // this way of doing)
    //
    //  so here we use the begin of RemoteHook and put first DWORD to dwHookEndFlag
    //////////////////////////////////////////////////////////////////////

    // mov eax,RemoteHook
    LocalHook[BufferIndex++]=0xB8;// mov eax,
    memcpy(&LocalHook[BufferIndex],&RemoteHook,sizeof(DWORD));
    BufferIndex+=sizeof(DWORD);

    // mov ebx,dwHookEndFlag
    LocalHook[BufferIndex++]=0xBB;// mov ebx,
    memcpy(&LocalHook[BufferIndex],&dwHookEndFlag,sizeof(DWORD));
    BufferIndex+=sizeof(DWORD);

    // *RemoteHook=dwHookEndFlag
    LocalHook[BufferIndex++]=0x89;LocalHook[BufferIndex++]=0x18;// mov dword ptr[eax],ebx


    //////////////////////////////////////////////////////////////////////
    // suspend thread until all injections are done
    //////////////////////////////////////////////////////////////////////

    // mov eax,pGetCurrentThread
    LocalHook[BufferIndex++]=0xB8;// mov eax,
    memcpy(&LocalHook[BufferIndex],&pGetCurrentThread,sizeof(DWORD));
    BufferIndex+=sizeof(DWORD);

    // call GetCurrentThread
    LocalHook[BufferIndex++]=0xFF;LocalHook[BufferIndex++]=0xD0; // call eax

    // we are in stdcall --> parameters are removed from stack

    // push eax (contains the thread handle
    LocalHook[BufferIndex++]=0x50;// push eax

    // mov eax,pSuspendThread
    LocalHook[BufferIndex++]=0xB8;// mov eax,
    memcpy(&LocalHook[BufferIndex],&pSuspendThread,sizeof(DWORD));
    BufferIndex+=sizeof(DWORD);

    // call SuspendThread
    LocalHook[BufferIndex++]=0xFF;LocalHook[BufferIndex++]=0xD0; // call eax

    // we are in stdcall --> parameters are removed from stack


    //////////////////////////////////////////////////////////////////////
    // restore registers and flag registers
    //////////////////////////////////////////////////////////////////////
    LocalHook[BufferIndex++]=0x9D;//popfd
    LocalHook[BufferIndex++]=0x61;//popad

    //////////////////////////////////////////////////////////////////////
    // jmp EntryPointAddress remember we have push return address on the stack
    // so just use ret
    //////////////////////////////////////////////////////////////////////
    LocalHook[BufferIndex++]=0xC3;//ret

    DWORD OldProtectionFlag;
    // copy hook data
    if (!this->HookEntryPointpProcessMemory->Write(
                                                    (LPVOID)RemoteHook,
                                                    LocalHook,
                                                    SIZEOF_HOOK,
                                                    &dwTransferedSize)
        )
        return FALSE;
    // mark new allocated memory as Executable
    if (!VirtualProtectEx(this->HookEntryPointpProcessMemory->GetProcessHandle(),
        RemoteHook,
        SIZEOF_HOOK,
        PAGE_EXECUTE_READWRITE,
        &OldProtectionFlag)
        )
        return FALSE;



    // remove memory protection
    if (!VirtualProtectEx(this->HookEntryPointpProcessMemory->GetProcessHandle(),
                            EntryPointAddress,
                            SIZEOF_HOOK_PROXY,
                            PAGE_EXECUTE_READWRITE,
                            &OldProtectionFlag)
        )
        return FALSE;

    // copy proxy data (assume that our hook is in remote process before jumping to it)
    if (!this->HookEntryPointpProcessMemory->Write(
                                                    (LPVOID)EntryPointAddress,
                                                    LocalProxy,
                                                    SIZEOF_HOOK_PROXY,
                                                    &dwTransferedSize)
        )
        return FALSE;

    // resume thread a first time to run our hook
    if(ResumeThread(hThreadHandle)==((DWORD)-1))
        return FALSE;

    // wait until hook has done it's job
    if (*pTlsHook)
    {
        // trick : wait for apioverride dll events instead of flags
        if (!this->WaitForInjectedDllToBeLoaded())
        {
            TCHAR pszMsg[2*MAX_PATH];
            _stprintf(pszMsg,
                _T("Error hooking application %s in suspended way"),
                pszFileName);
            MessageBox(this->hParentWindow,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);
            goto CleanUp;
        }
        // this->hevtAPIOverrideDllProcessAttachCompleted event has been reset by WaitForInjectedDllToBeLoaded,
        // so set it again
        SetEvent(this->hevtAPIOverrideDllProcessAttachCompleted);
    }
    else
    {
        DWORD OriginalTickCount=GetTickCount();
        DWORD CurrentTickCount;
        for(;;)
        {
            Sleep(HOOK_END_POOLING_IN_MS);
            // this injection way can fails for applications (like .net exe)
            CurrentTickCount=GetTickCount();
            if (OriginalTickCount+MAX_POOLING_TIME_IN_MS<CurrentTickCount)
            {
                TCHAR pszMsg[2*MAX_PATH];
                _stprintf(pszMsg,
                    _T("Error hooking application %s in suspended way\r\n")
                    _T("Use the \"Only After\" option"),
                    pszFileName);
                MessageBox(this->hParentWindow,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);
                goto CleanUp;
            }
            this->HookEntryPointpProcessMemory->Read( RemoteHook,&dw,sizeof(DWORD),&dwTransferedSize);

            if (dw==dwHookEndFlag)
                break;

            // if process has crash don't wait infinite
            if (!CProcessHelper::IsAlive(dwProcessId))
            {
                TCHAR pszMsg[2*MAX_PATH];
                _stprintf(pszMsg,_T("Error application %s seems to be closed"),pszFileName);
                MessageBox(this->hParentWindow,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);
                return FALSE;
            }
        }
    }

    // restore original opcode
    if (!this->HookEntryPointpProcessMemory->Write(
                                                    (LPVOID)EntryPointAddress,
                                                    LocalOriginalOpCode,
                                                    SIZEOF_HOOK_PROXY,
                                                    &dwTransferedSize)
        )
        return FALSE;

    // restore memory protection
    if (!VirtualProtectEx(this->HookEntryPointpProcessMemory->GetProcessHandle(),
                            EntryPointAddress,
                            SIZEOF_HOOK_PROXY,
                            OldProtectionFlag,
                            &OldProtectionFlag)
        )
        return FALSE;

    bSuccess=TRUE;
CleanUp:

    this->HookEntryPointRemoteHook          =RemoteHook;
    this->HookEntryPointRemoteLibName       =RemoteLibName;
    return bSuccess;
}
//-----------------------------------------------------------------------------
// Name: HookEntryPointFree
// Object: free memory allocated in remote process after a call of HookEntryPoint
//          must be called after caller of HookEntryPoint has call ResumeProcess
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CApiOverride::HookEntryPointFree()
{
    // don't use IsBadWritePointer here because this is remote process allocated memory
    if (this->HookEntryPointpProcessMemory==NULL)
        return FALSE;


    // wait a little to assume process don't need allocated memory anymore (only to be sure that the 3 asm instructions
    // required after the ResumeProcess are executed)
    Sleep(100);

    if (CProcessHelper::IsAlive(this->dwCurrentProcessId))
    {
        // free memory
        if (this->HookEntryPointRemoteHook)
            this->HookEntryPointpProcessMemory->Free(this->HookEntryPointRemoteHook);
        if (this->HookEntryPointRemoteLibName)
            this->HookEntryPointpProcessMemory->Free(this->HookEntryPointRemoteLibName);
    }

    this->HookEntryPointRemoteHook=NULL;
    this->HookEntryPointRemoteLibName=NULL;

    delete this->HookEntryPointpProcessMemory;
    this->HookEntryPointpProcessMemory=NULL;

    return TRUE;
}