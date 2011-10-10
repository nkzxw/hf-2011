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
#include "HookNet.h"

BOOL bApiOverrideDllLoaded=FALSE;// tells if apioverride functions pointer have been loaded
HOOK_NET_INIT HookNetInfos={0};// contains pointers to apioverride functions 
HOOK_NET_OPTIONS HookNetOptions={0};// .Net hooking options
HANDLE hevtEndOfHookNetInit=NULL;// event raise when hook net dll can access apioverride dll function (HookNetInfos filled)
MONITORING_FILE_INFOS HookNetDll_IMetaDataImportMonitoringFileInfo={HOOK_NET_AUTO_PARSING};
HINSTANCE DllhInstance=NULL;
HANDLE hevtHookNetReady=NULL;
BOOL bAutoHooking=TRUE;
BOOL bCorLoaded=FALSE;
ICorProfilerInfo* pCurrentCorProfiler=NULL;
DWORD NetExceptionTlsIndex=0;
CLinkListSimple* pLinkListOpenDialogs=NULL;// link list of handle of open dialog windows
HANDLE hSemaphoreOpenDialogs=NULL;// semaphore that manages opened dialogs closing

// pCompiledFunctionList : link list of CFunctionInfo
// contains objects for all jitted (compiled) function (hooked or not : if hooked CFunctionInfo.pItemApiInfo is not NULL)
CLinkListSimple* pCompiledFunctionList=NULL;

// pHookedFunctionList : link list of CFunctionInfo
CLinkListSimple* pHookedFunctionList=NULL;// link list of hooked function : allow to quickly find hooked functions

// pManuallyRegisteredMonitoringAndOverridingHooks : link list of STATIC_NET_HOOK_INFOS
// contains information on static hooks (loaded .Net monitoring files and overriding dll)
CLinkList* pManuallyRegisteredMonitoringAndOverridingHooks=NULL;

DWORD NetEnterLeaveTlsIndex=0;

extern void FreeTlsEnterLeaveLinkedList();
extern CLinkListSimple* GetTlsEnterLeaveLinkedList();
extern CLinkListSimple* GetOrCreateTlsEnterLeaveLinkedList();

BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved )
{    
    UNREFERENCED_PARAMETER(lpReserved);
    // save off the instance handle for later use
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            DllhInstance=hInstance;
//            DisableThreadLibraryCalls(DllhInstance);
            NetExceptionTlsIndex=TlsAlloc();
            NetEnterLeaveTlsIndex=TlsAlloc();
            return Initialize();
        case DLL_PROCESS_DETACH:
            Destroy();
            TlsFree(NetExceptionTlsIndex);
            TlsFree(NetEnterLeaveTlsIndex);
            return TRUE;
        case DLL_THREAD_DETACH:
            FreeTlsEnterLeaveLinkedList();
            return TRUE;
    } 
        
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Initialize
// Object: like constructor : allocate memory and initialize
//         WARNING CALLED EVEN FOR DLL REGISTRATION
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL Initialize()
{
    TCHAR psz[MAX_PATH+50];
    TCHAR pszPID[50];
    DWORD CurrentProcessID;
    // get process ID
    CurrentProcessID=GetCurrentProcessId();
    // pid -> string
    _stprintf(pszPID,_T("0x%X"),CurrentProcessID);

    pLinkListOpenDialogs=new CLinkListSimple();
    pHookedFunctionList=new CLinkListSimple();


    ///////////////////////////////////////////////////////////////
    // create an event to report information to winapioverride.exe
    ///////////////////////////////////////////////////////////////

    // create a unique event name
    _tcscpy(psz,APIOVERRIDE_EVENT_HOOKNET_STARTED);
    _tcscat(psz,pszPID);
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

    // create an manual reset unsignaled event
    hevtHookNetReady=CreateEvent(&SecAttr,TRUE,FALSE,psz);

    pCompiledFunctionList=new CLinkListSimple();
    pManuallyRegisteredMonitoringAndOverridingHooks=new CLinkList(sizeof(STATIC_NET_HOOK_INFOS));

    hevtEndOfHookNetInit=CreateEvent(NULL,TRUE,FALSE,NULL);// uninitialized manual reset event
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Destroy
// Object:  like destructor : free memory
//         WARNING CALLED EVEN FOR DLL REGISTRATION
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL Destroy()
{
    if (bApiOverrideDllLoaded)
    {
        // send message to application to signal .Net function calling becomes useless
        TCHAR pszPID[50];
        DWORD CurrentProcessID;
        // get process ID
        CurrentProcessID=GetCurrentProcessId();
        // pid -> string
        _stprintf(pszPID,_T("0x%X"),CurrentProcessID);

        // by sending a CMD_NET_SHUTDOWN_HOOKNET_DLL command
        TCHAR szMailSlotName[MAX_PATH];
        _tcscpy(szMailSlotName,APIOVERRIDE_MAILSLOT_FROM_INJECTOR);
        _tcscat(szMailSlotName,pszPID);
        CMailSlotClient MailSlot(szMailSlotName);
        if (MailSlot.Open())
        {
            // fill command
            STRUCT_COMMAND Cmd;
            Cmd.dwCommand_ID=CMD_NET_SHUTDOWN_HOOKNET_DLL;
            // send command
            MailSlot.Write(&Cmd,sizeof(STRUCT_COMMAND));

            // close mailslot
            MailSlot.Close();
        }
    }

    if (pLinkListOpenDialogs)
    {
        CLinkListItem* pItem;
        if (pLinkListOpenDialogs->Head) // if at least a dialog is displayed
        {
            pLinkListOpenDialogs->LockWaitTime=5000;
            pLinkListOpenDialogs->Lock();
            // create semaphore
            hSemaphoreOpenDialogs=CreateSemaphore(NULL,0,pLinkListOpenDialogs->GetItemsCount(),NULL);
            // for each existing dialog
            for (pItem=pLinkListOpenDialogs->Head;pItem;pItem=pItem->NextItem)
            {
                // close dialog
                EndDialog((HWND)pItem->ItemData,0);
            }
            // wait for all dialog to be close during 5 sec
            DWORD dwRes;
            for (DWORD Cnt=0;Cnt<pLinkListOpenDialogs->GetItemsCount();Cnt++)
            {
                dwRes=WaitForSingleObject(hSemaphoreOpenDialogs,5000);
                if (dwRes==WAIT_OBJECT_0)
                    continue;
                else
                    break;
            }
            // sleep a little to allow thread having called ReleaseSemaphore to end
            Sleep(100);

            // close semaphore
            CloseHandle(hSemaphoreOpenDialogs);
            hSemaphoreOpenDialogs=NULL;

            pLinkListOpenDialogs->RemoveAllItems(TRUE);
            pLinkListOpenDialogs->Unlock();
        }
        delete pLinkListOpenDialogs;
    }

    if (hevtEndOfHookNetInit)
    {
        CloseHandle(hevtEndOfHookNetInit);
        hevtEndOfHookNetInit=NULL;
    }

    if (pCompiledFunctionList)
    {
        // delete pCompiledFunctionList
        delete pCompiledFunctionList;
        pCompiledFunctionList=NULL;
    }

    if (pHookedFunctionList)
    {
        delete pHookedFunctionList;
        pHookedFunctionList=NULL;
    }

    if (pManuallyRegisteredMonitoringAndOverridingHooks)
    {
        // delete pManuallyRegisteredMonitoringAndOverridingHooks
        delete pManuallyRegisteredMonitoringAndOverridingHooks;
        pManuallyRegisteredMonitoringAndOverridingHooks=NULL;
    }

    if (hevtHookNetReady)
    {
        CloseHandle(hevtHookNetReady);
        hevtHookNetReady=NULL;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CorInitialize
// Object: like constructor : allocate memory and initialize
//         called when Cor as loaded HookNet.dll as a profiler
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CorInitialize()
{
#ifdef _DEBUG
    // gives a friendly information and allow to attach debugger at cor startup
    MessageBox(0,_T("Hook Net CorInitialize"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
#endif

    TCHAR pszPID[50];
    TCHAR psz[MAX_PATH+50];
    DWORD CurrentProcessID;
    // get process ID
    CurrentProcessID=GetCurrentProcessId();
    // pid -> string
    _stprintf(pszPID,_T("0x%X"),CurrentProcessID);


    // set bCorLoaded only after having initialized global value (like pCompiledFunctionList)
    bCorLoaded=TRUE;

    // check if apioverride dll is loaded inside process
    if (GetModuleHandle(API_OVERRIDE_DLL_NAME))
    {
        // this case can occurs in standard application loading a .net dll/control
    }
    else
    {
        ///////////////////////////////////////////////
        // if process should be hooked at startup, events should be created
        // or will be created soon by winapioverride
        // So to know if process must be hooked at startup, do 
        // a check on a named event created by winapioverride
        ///////////////////////////////////////////////
        _tcscpy(psz,APIOVERRIDE_EVENT_ERROR);
        _tcscat(psz,pszPID);
        HANDLE hevtError=OpenEvent(EVENT_ALL_ACCESS,TRUE,psz);
        if (!hevtError)
        {
            // sleep a while in case of hook all processes at startup use
            // Notice : we can wait as we are blocking current thread and so .NET process startup
            Sleep(1000);
            hevtError=OpenEvent(EVENT_ALL_ACCESS,TRUE,psz);
        }

        // if event has been opened, process must be hooked now, so load apioverride dll
        if (hevtError)
        {
            // close unneeded handle (do it before loading apioverride dll)
            CloseHandle(hevtError);

            // load apioverride dll now (avoid a polling way that can loose first calls)
            // process must be hooked, so load apioverride dll
            TCHAR ApiOverrideFullPath[MAX_PATH];
            CStdFileOperations::GetModulePath(DllhInstance,ApiOverrideFullPath,MAX_PATH);
            _tcscat(ApiOverrideFullPath,API_OVERRIDE_DLL_NAME);

            if (!LoadLibrary(ApiOverrideFullPath))
                return FALSE;
        }
        //else 
        //{
        //    // process is not going to be hooked now, but could be hooked later
        //    // we have to load Net configuration now to initialized net engine (do nothing it will be done just under)
        //}
    }

    // we have to load Net configuration now, IN ALL CASES to initialized the .NET engine
    // some configuration informations are required inside the CorInitialize callback function
    // load winapioverride ini file directly using winapioverride Coption class
    TCHAR ConfigFileName[MAX_PATH];
    CStdFileOperations::GetModulePath(DllhInstance,ConfigFileName,MAX_PATH);
    _tcscat(ConfigFileName,COPTION_OPTION_FILENAME);

    COptions Options(ConfigFileName);
    Options.Load();
    SetHookNetOptions(&Options.NetOptions);


    if (GetModuleHandle(API_OVERRIDE_DLL_NAME))
    {
        // query apioverride dll to initialize hooknet dll
        // by sending a CMD_NET_INITIALIZE_HOOKNET_DLL command
        TCHAR szMailSlotName[MAX_PATH];
        _tcscpy(szMailSlotName,APIOVERRIDE_MAILSLOT_FROM_INJECTOR);
        _tcscat(szMailSlotName,pszPID);
        CMailSlotClient MailSlot(szMailSlotName);
        if (!MailSlot.Open())
            return FALSE;
        // fill command
        STRUCT_COMMAND Cmd;
        Cmd.dwCommand_ID=CMD_NET_INITIALIZE_HOOKNET_DLL;
        // send command
        if (!MailSlot.Write(&Cmd,sizeof(STRUCT_COMMAND)))
        {
            // close mailslot
            MailSlot.Close();
            return FALSE;
        }

        // close mailslot
        MailSlot.Close();

        // wait end of configuration
        WaitForSingleObject(hevtEndOfHookNetInit,MAX_END_OF_HOOK_NET_INIT_WAIT_IN_MS);
    }
    SetEvent(hevtHookNetReady);


#ifdef _DEBUG
    // gives a friendly information and allow to attach debugger at cor startup
    MessageBox(0,_T("Hook Net CorInitialized"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
#endif
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CorShutdown
// Object:  like destructor : free memory
//         called when Cor unloaded HookNet.dll
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CorShutdown()
{
    // on .Net shutdown remove all .Net hooks
    UnHookAllNetMethods();

    pHookedFunctionList->RemoveAllItems();
    pManuallyRegisteredMonitoringAndOverridingHooks->RemoveAllItems();

    bCorLoaded=FALSE;

    // delete all pFunctionInfo contained in pCompiledFunctionList
    pCompiledFunctionList->Lock();
    CLinkListItem* pItem;
    CFunctionInfoEx* pFunctionInfo;
    for (pItem=pCompiledFunctionList->Head;pItem;pItem=pItem->NextItem)
    {
        pFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;
        delete pFunctionInfo;
    }
    pCompiledFunctionList->RemoveAllItems(TRUE);
    pCompiledFunctionList->Unlock();
    
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ApiInfoItemDeletionCallback
// Object: called each time a hook is being to be removed
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void __stdcall ApiInfoItemDeletionCallback(CLinkListItem* pItemAPIInfo)
{
    if (!pCompiledFunctionList)
        return;
    if (!pManuallyRegisteredMonitoringAndOverridingHooks)
        return;

    CLinkListItem* pItem;
    CLinkListItem* pNextItem;
    CFunctionInfoEx* pParsedFunctionInfo;
    STATIC_NET_HOOK_INFOS* pStaticNetHook;

    // remove item from pCompiledFunctionList,pHookedFunctionList and pManuallyRegisteredMonitoringAndOverridingHooks
    pCompiledFunctionList->Lock(FALSE);
    for (pItem=pCompiledFunctionList->Head;pItem;pItem=pItem->NextItem)
    {
        pParsedFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;
        // if same token
        if (pParsedFunctionInfo->pItemApiInfo==pItemAPIInfo)
        {
            pParsedFunctionInfo->pItemApiInfo=NULL;
            pHookedFunctionList->RemoveItemFromItemData(pParsedFunctionInfo);
        }
    }
    pCompiledFunctionList->Unlock();

    pManuallyRegisteredMonitoringAndOverridingHooks->Lock(FALSE);
    for (pItem=pManuallyRegisteredMonitoringAndOverridingHooks->Head;pItem;pItem=pNextItem)
    {
        pNextItem=pItem->NextItem;
        pStaticNetHook=(STATIC_NET_HOOK_INFOS*)pItem->ItemData;
        // if same token
        if (pStaticNetHook->pItemApiInfo==pItemAPIInfo)
        {
            // as all hooking information are stored in API_INFO struct,
            // if API_INFO is freed, pStaticNetHook contains no more informations so it should be deleted
            
            pStaticNetHook->pItemApiInfo=NULL;
            pManuallyRegisteredMonitoringAndOverridingHooks->RemoveItem(pItem,TRUE);
        }
    }
    pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();
}

//-----------------------------------------------------------------------------
// Name: UnhookFunctionUsingAutoHooking
// Object:  unhook specified function only if hooked by auto hooking
// Parameters :
//     in  : CFunctionInfoEx* pFunctionInfo : function info to unhook
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL UnhookFunctionUsingAutoHooking(CFunctionInfoEx* pFunctionInfo)
{
    return UnhookFunction(pFunctionInfo,TRUE);
}
//-----------------------------------------------------------------------------
// Name: UnhookFunction
// Object:  unhook specified function
// Parameters :
//     in  : CFunctionInfoEx* pFunctionInfo : function info to unhook
//           BOOL CheckAutoHookingOwner : if TRUE remove hook only if hook has been installed by auto hooking
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL UnhookFunction(CFunctionInfoEx* pFunctionInfo,BOOL CheckAutoHookingOwner)
{
    BOOL bRet;

    if (!bApiOverrideDllLoaded)
        return FALSE;

    // if function wasn't hooked
    if (!pFunctionInfo->pItemApiInfo)
        return TRUE;

    API_INFO* pApiInfo=((API_INFO*)pFunctionInfo->pItemApiInfo->ItemData);
    if (IsBadReadPtr(pApiInfo,sizeof(API_INFO)))
        return TRUE;

    if (CheckAutoHookingOwner)
    {
        // if we are no more hook owner
        if (pApiInfo->pMonitoringFileInfos!=&HookNetDll_IMetaDataImportMonitoringFileInfo)
            return TRUE;
    }

    // remove from hooked list
    pHookedFunctionList->RemoveItemFromItemData(pFunctionInfo);

    // remove monitoring information
    pApiInfo->pMonitoringFileInfos=NULL;

    // try to unhook
    bRet=HookNetInfos.UnHookIfPossible(pFunctionInfo->pItemApiInfo,TRUE);

    pFunctionInfo->pItemApiInfo=NULL;// ok even if static hook (static hook will still have access to pItemApiInfo information)
    
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: ReportMessage
// Object:  report message to hooking manager application
// Parameters :
//     in  : tagReportMessageType MsgType : message type (info/error/warning)
//           TCHAR* MsgText : message content
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
void ReportMessage(tagReportMessageType MsgType,TCHAR* MsgText)
{
    // assume apioverride dll is loaded and we get pointer to it's internal functions
    if (!bApiOverrideDllLoaded)
        return;

    // use apioverride dll report message function
    HookNetInfos.ReportMessage(MsgType,MsgText);
}

//-----------------------------------------------------------------------------
// Name: ReportBadNetFunctionDescription
// Object: report bad description for a .Net hook 
// Parameters :
//     in  : TCHAR* pszFuncDescription : provideded bad description
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
void ReportBadNetFunctionDescription(TCHAR* pszFuncDescription)
{
    TCHAR sz[2*MAX_PATH];
    _sntprintf(sz,2*MAX_PATH,_T("Bad .Net function description:%s"),pszFuncDescription);
    sz[2*MAX_PATH-1]=0;
    ReportMessage(REPORT_MESSAGE_ERROR,sz);
}

//-----------------------------------------------------------------------------
// Name: CheckHookAvailability
// Object:  check if function can be hooked
// Parameters :
//     in  : CFunctionInfoEx* pFunctionInfo : function info to hook
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CheckHookAvailability(CFunctionInfoEx* pFunctionInfo)
{
    if (pFunctionInfo->AsmCodeSize<OPCODE_REPLACEMENT_SIZE)
    {
        TCHAR sz[MAX_PATH];
        _stprintf(sz,_T("Function %s can't be hooked (Too short). Check \"Disable Optimization\" in .NET options dialog to hook this function"),pFunctionInfo->szName);
        ReportMessage(REPORT_MESSAGE_WARNING,sz);
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: HookFunctionUsingAutoHooking
// Object:  hook specified function
// Parameters :
//     in  : CFunctionInfoEx* pFunctionInfo : function info to hook
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL HookFunctionUsingAutoHooking(CFunctionInfoEx* pFunctionInfo)
{
    BOOL bAlreadyHooked;
    TCHAR szFuncName[MAX_PATH];
    TCHAR szParamName[MAX_PATH];
    CLinkListItem* pItemApiInfo;
    API_INFO* pApiInfo;
    CLinkListItem* pItemParameterInfo;
    CParameterInfo* pParameterInfo;
    int ParameterIndex;

    if (!bApiOverrideDllLoaded)
        return FALSE;

    // check compiled function code size
    if (!CheckHookAvailability(pFunctionInfo))
        return FALSE;

    // get function name
    pFunctionInfo->GetName(szFuncName,MAX_PATH);

    // get pApiInfo
    pItemApiInfo=HookNetInfos.GetAssociatedItemAPIInfo(pFunctionInfo->AsmCodeStart,&bAlreadyHooked);
    
    pApiInfo=(API_INFO*)pItemApiInfo->ItemData;

    // if a hook was installed
    if (bAlreadyHooked)
    {
        // check if a monitoring hook was installed
        if (pApiInfo->pMonitoringFileInfos)
        {
            // don't associate pItemApiInfo to hooked method in case monitoring hook is owned by a static hook
            return TRUE; // nothing to do
        }
    }
    else
    {
        TCHAR pszFuncName[MAX_PATH];
        TCHAR pszModuleName[MAX_PATH];
        pFunctionInfo->GetName(pszFuncName,MAX_PATH);
        pFunctionInfo->GetModuleName(pszModuleName,MAX_PATH);
        if (!HookNetInfos.InitializeApiInfo(pApiInfo,pszModuleName,pszFuncName))
            return FALSE;
    }

    // associate pItemApiInfo to hooked method
    pFunctionInfo->pItemApiInfo=pItemApiInfo;

    pApiInfo->pMonitoringFileInfos=&HookNetDll_IMetaDataImportMonitoringFileInfo;
    pApiInfo->ParamDirectionType=PARAM_DIR_IN;

    //////////////////////////
    // fill parameters infos
    //////////////////////////
    pApiInfo->MonitoringParamCount=0;
    if (!bAlreadyHooked)
        pApiInfo->StackSize=0;
    
    // loop throw function parameters
    ParameterIndex=0;
    pFunctionInfo->pParameterInfoList->Lock();
    // for each parameter of method
    for (pItemParameterInfo=pFunctionInfo->pParameterInfoList->Head;
        (pItemParameterInfo) && (ParameterIndex<MAX_PARAM);
        pItemParameterInfo=pItemParameterInfo->NextItem)
    {
        pParameterInfo=(CParameterInfo*)pItemParameterInfo->ItemData;

        // if parameter has an out direction, set Out spying type
        if (pParameterInfo->IsOutParameter())
            // set function spying direction to OUT
            pApiInfo->ParamDirectionType=PARAM_DIR_OUT;

        pParameterInfo->GetName(szParamName,MAX_PATH);
        _tcsncpy(pApiInfo->ParamList[ParameterIndex].pszParameterName,szParamName,PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE);
        pApiInfo->ParamList[ParameterIndex].pszParameterName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE-1]=0;

        pApiInfo->ParamList[ParameterIndex].dwSizeOfData=pParameterInfo->GetStackSize();
        pApiInfo->ParamList[ParameterIndex].dwSizeOfPointedData=pParameterInfo->GetPointedSize();;
        pApiInfo->ParamList[ParameterIndex].dwType=pParameterInfo->GetWinAPIOverrideType();
        pApiInfo->ParamList[ParameterIndex].bSizeOfPointedDataDefinedByAnotherParameter=FALSE;
        pApiInfo->ParamList[ParameterIndex].pConditionalBreakContent=NULL;
        pApiInfo->ParamList[ParameterIndex].pConditionalLogContent=NULL;
        // if you want to add conditional break or log info use the following as
        //  HookNet.dll Heap can differ from ApiOverride.dll 
        //         --> standard memory allocation (malloc, new) done in HookNet.dll can't be deleted from ApiOverride.dll
        // HookNetInfos.CreateParameterConditionalLogContentListIfDoesntExist(&pApiInfo->ParamList[ParameterIndex]);
        // HookNetInfos.CreateParameterConditionalBreakContentListIfDoesntExist(&pApiInfo->ParamList[ParameterIndex]);

        if (!bAlreadyHooked)
        {
            // update pApiInfo
            if (pApiInfo->ParamList[ParameterIndex].dwSizeOfData)
                pApiInfo->StackSize+=pApiInfo->ParamList[ParameterIndex].dwSizeOfData;
            else // pointer or less than REGISTER_BYTE_SIZE
                // add default register size in byte
                pApiInfo->StackSize+=REGISTER_BYTE_SIZE;
            pApiInfo->MonitoringParamCount++;
        }

        // increase ParameIndex value
        ParameterIndex++;
    }
    pFunctionInfo->pParameterInfoList->Unlock();

    if (!bAlreadyHooked)
    {
        // as generated .Net calls can be relative we can't use
        // pApiInfo->FirstBytesCanExecuteAnywhereSize=pFunctionInfo->AsmCodeSize;
        pApiInfo->FirstBytesCanExecuteAnywhereSize=0;
        pApiInfo->HookType=HOOK_TYPE_NET;
        pApiInfo->HookTypeExtendedFunctionInfos.InfosForNET.FunctionToken=pFunctionInfo->FunctionToken;
        pApiInfo->CallingConvention=pFunctionInfo->GetWinApiOverrideCallingConvention();

        // hook api function
        if (!HookNetInfos.HookAPIFunction(pApiInfo))
        {
            // remove pItemApiInfo associated information
            pFunctionInfo->pItemApiInfo=NULL;

            // free memory associated to item
            HookNetInfos.FreeApiInfoItem(pItemApiInfo);

            return FALSE;
        }

        // add function to hooked list
        pHookedFunctionList->AddItem(pFunctionInfo);

        // add ItemApiInfo deletion callback
        if (!pApiInfo->DeletionCallback)
            pApiInfo->DeletionCallback=ApiInfoItemDeletionCallback;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: AddHookNetFromTokenForJittedFuntions
// Object:  add hooks for all specified function tokens for the specified assembly
// Parameters :
//     in  : HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM* pParam
//     out : 
//     return : number of functions successfully hooked
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) DWORD __stdcall AddHookNetFromTokenForJittedFuntions(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM* pParam)
{
	mdToken* pTokenArray = (mdToken*) (pParam->TokenArray);
	if (IsBadReadPtr(pTokenArray,pParam->TokensNumber*sizeof(mdToken)))
		return 0;

	DWORD FuncRet = 0;
	CFunctionInfoEx* pParsedFunctionInfo;
	for (DWORD Cnt=0;Cnt<pParam->TokensNumber;Cnt++)
	{
		pParsedFunctionInfo = GetFunctionInfo(pParam->AssemblyName,pTokenArray[Cnt],FALSE);
		if (!pParsedFunctionInfo)
			continue;
		if (HookFunctionUsingAutoHooking(pParsedFunctionInfo))
			FuncRet++;
	}

	return FuncRet;
}

//-----------------------------------------------------------------------------
// Name: RemoveHookNetFromTokenForJittedFuntion
// Object:  remove hooks for all specified function tokens for the specified assembly
// Parameters :
//     in  : HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM* pParam
//     out : 
//     return : number of functions successfully unhooked
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) DWORD __stdcall RemoveHookNetFromTokenForJittedFuntions(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM* pParam)
{
	mdToken* pTokenArray = (mdToken*) (pParam->TokenArray);
	if (IsBadReadPtr(pTokenArray,pParam->TokensNumber*sizeof(mdToken)))
		return 0;

	DWORD FuncRet = 0;
	CFunctionInfoEx* pParsedFunctionInfo;
	for (DWORD Cnt=0;Cnt<pParam->TokensNumber;Cnt++)
	{
		pParsedFunctionInfo = GetFunctionInfo(pParam->AssemblyName,pTokenArray[Cnt],FALSE);
		if (!pParsedFunctionInfo)
			continue;
		if (UnhookFunction(pParsedFunctionInfo,FALSE))
			FuncRet++;
	}
	
	return FuncRet;
}

//-----------------------------------------------------------------------------
// Name: UnHookAllNetMethods
// Object:  unhook all currently hooked NET methods
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall UnHookAllNetMethods()
{
    BOOL bRet=TRUE;

    if (!bApiOverrideDllLoaded)
        return FALSE;
    if (!bCorLoaded)
        return FALSE;
    if (!pCompiledFunctionList)
        return FALSE;

    // remove all hook found in pCompiledFunctionList
    pCompiledFunctionList->Lock();
    CLinkListItem* pItem;
    CFunctionInfoEx* pFunctionInfo;
    for (pItem=pCompiledFunctionList->Head;pItem;pItem=pItem->NextItem)
    {
        pFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;
        if (pFunctionInfo->pItemApiInfo!=NULL)
        {
            bRet&=UnhookFunctionUsingAutoHooking(pFunctionInfo);
        }
    }
    pCompiledFunctionList->Unlock();

    // remove all hook found in pCompiledFunctionList
    pManuallyRegisteredMonitoringAndOverridingHooks->Lock();
    API_INFO* pApiInfo;
    STATIC_NET_HOOK_INFOS* pStaticNetHook;
    for (pItem=pManuallyRegisteredMonitoringAndOverridingHooks->Head;pItem;pItem=pItem->NextItem)
    {
        pStaticNetHook=(STATIC_NET_HOOK_INFOS*)pItem->ItemData;
        if (IsBadReadPtr(pStaticNetHook,sizeof(STATIC_NET_HOOK_INFOS)))
            continue;
        if (IsBadReadPtr(pStaticNetHook->pItemApiInfo,sizeof(CLinkListItem)))
            continue;

        pApiInfo=(API_INFO*)pStaticNetHook->pItemApiInfo->ItemData;
        if (IsBadWritePtr(pApiInfo,sizeof(API_INFO)))
            continue;

        // pApiInfo contains a .NET hook, auto-monitoring hook are already removed
        // so hook must contain a static .NET hook, so it's safe to remove it's information 
        // and ask for a removal
        pApiInfo->pMonitoringFileInfos=NULL;
        pApiInfo->pFakeDllInfos=NULL;
        if (pStaticNetHook->pFunctionInfo)
            pStaticNetHook->pFunctionInfo->pItemApiInfo=NULL;
        bRet&=HookNetInfos.UnHookIfPossible(pStaticNetHook->pItemApiInfo,TRUE);
    }
    pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: RemoveHookNetMonitoringDefinition
// Object: ONLY REMOVE REGISTERD STATE OF HOOK not hook itself
//         remove hook info from associated list
//         UnHookIfPossible must be done by calling function to release hook and associated memory
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall RemoveHookNetMonitoringDefinition(MONITORING_FILE_INFOS* pMonitoringFileInfo)
{
    // remove all monitoring hook info found in pCompiledFunctionList
    pManuallyRegisteredMonitoringAndOverridingHooks->Lock();
    CLinkListItem* pItem;
    CLinkListItem* pNextItem;
    STATIC_NET_HOOK_INFOS* pStaticNetHook;
    for (pItem=pManuallyRegisteredMonitoringAndOverridingHooks->Head;pItem;pItem=pNextItem)
    {
        // store next item info in case of pItem removal
        pNextItem=pItem->NextItem;

        pStaticNetHook=(STATIC_NET_HOOK_INFOS*)pItem->ItemData;
        if (pStaticNetHook->pMonitoringFileInfo==pMonitoringFileInfo)
        {
            // empty monitoring info part
            pStaticNetHook->pMonitoringFileInfo=0;

            // if item contains no other informations
            if (pStaticNetHook->pFakingDllInfos==0)
            {
                pManuallyRegisteredMonitoringAndOverridingHooks->RemoveItem(pItem,TRUE);

                // UnHookIfPossible must be done by calling function to release hook and associated memory
                // bRet=HookNetInfos.UnHookIfPossible(pFunctionInfo->pItemApiInfo,TRUE);
            }
        }
    }
    pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RemoveHookNetFakingDefinition
// Object: ONLY REMOVE REGISTERD STATE OF HOOK not hook itself
//         remove hook info from associated list
//         UnHookIfPossible must be done by calling function to release hook and associated memory
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall RemoveHookNetFakingDefinition(FAKING_DLL_INFOS* pFakingDllInfos)
{
    // remove all faking hook info found in pCompiledFunctionList
    pManuallyRegisteredMonitoringAndOverridingHooks->Lock();
    CLinkListItem* pItem;
    CLinkListItem* pNextItem;
    STATIC_NET_HOOK_INFOS* pStaticNetHook;
    for (pItem=pManuallyRegisteredMonitoringAndOverridingHooks->Head;pItem;pItem=pNextItem)
    {
        // store next item info in case of pItem removal
        pNextItem=pItem->NextItem;

        pStaticNetHook=(STATIC_NET_HOOK_INFOS*)pItem->ItemData;
        if (pStaticNetHook->pFakingDllInfos==pFakingDllInfos)
        {
            // empty faking info part
            pStaticNetHook->pFakingDllInfos=0;

            // if item contains no other informations
            if (pStaticNetHook->pMonitoringFileInfo==0)
            {
                pManuallyRegisteredMonitoringAndOverridingHooks->RemoveItem(pItem,TRUE);

                // UnHookIfPossible must be done by calling function to release hook and associated memory
                // bRet=HookNetInfos.UnHookIfPossible(pFunctionInfo->pItemApiInfo,TRUE);
            }
        }
    }
    pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetHookNetOptions
// Object: set COM hooks options
// Parameters :
//     in  : HOOK_NET_OPTIONS* pHookNetOptions : net monitoring options
//     out : 
//     return : TRUE on success
//              FALSE if some options are wrong and so new options are not applied
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall SetHookNetOptions(HOOK_NET_OPTIONS* pHookNetOptions)
{
    if (IsBadReadPtr(pHookNetOptions,sizeof(HOOK_NET_OPTIONS)))
        return FALSE;

    // copy options to local storage
    memcpy(&HookNetOptions,pHookNetOptions,sizeof(HOOK_NET_OPTIONS));

    // apply profiler option that can be changed in running mode
    if (pCurrentCorProfiler)
    {
        DWORD EventMask=0;
        // get current exceptions mask
        pCurrentCorProfiler->GetEventMask(&EventMask);

        // change mask
        /* DO NOT CHANGE MonitorException mask has it is requiered for apioverridekernel
        if (HookNetOptions.MonitorException)
        {
            EventMask|= (COR_PRF_MONITOR_CLR_EXCEPTIONS | COR_PRF_MONITOR_EXCEPTIONS);
        }
        else
        {
            EventMask&=~COR_PRF_MONITOR_CLR_EXCEPTIONS;
            EventMask&=~COR_PRF_MONITOR_EXCEPTIONS;
        }
        */

        if (HookNetOptions.EnableFrameworkMonitoring)
        {
            EventMask |=COR_PRF_MONITOR_ENTERLEAVE;
        }
        else
        {
            EventMask&=~COR_PRF_MONITOR_ENTERLEAVE;
        }

        // set new mask
        pCurrentCorProfiler->SetEventMask(EventMask);
    }

    return TRUE;
}

void UserTypeParsingReportError(IN TCHAR* const ErrorMessage,IN PBYTE const UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);
    ReportMessage(REPORT_MESSAGE_ERROR,ErrorMessage);
}

//-----------------------------------------------------------------------------
// Name: InitializeHookNet
// Object: Initialize Hooknet dll
//         provide Hook net dll functions handler to ApiOverride.dll functions
// Parameters :
//     in  : HOOK_NET_INIT pInitHookNet
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall InitializeHookNet(HOOK_NET_INIT* pInitHookNet)
{
    if (IsBadReadPtr(pInitHookNet,sizeof(HOOK_NET_INIT)))
        return FALSE;

    // Initialize local HookComInfos struct
    memcpy(&HookNetInfos,pInitHookNet,sizeof(HOOK_NET_INIT));

    bApiOverrideDllLoaded=TRUE;
    SetEvent(hevtEndOfHookNetInit);

    // set error report for CSupportedParameters
    CSupportedParameters::SetErrorReport(UserTypeParsingReportError,NULL);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Uninitialize
// Object: Uninitialize hook net
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall Uninitialize()
{
    bApiOverrideDllLoaded=FALSE;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: StartAutoHooking
// Object: start net auto hooking
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall StartAutoHooking()
{
    if (!bApiOverrideDllLoaded)
        return FALSE;
    if (!bCorLoaded)
        return FALSE;
    if (!pCompiledFunctionList)
        return FALSE;

    bAutoHooking=TRUE;

    BOOL bRet=TRUE;
    if (!bApiOverrideDllLoaded)
        return FALSE;

    // for each compiled function
    pCompiledFunctionList->Lock();
    CLinkListItem* pItem;
    CFunctionInfoEx* pFunctionInfo;
    for (pItem=pCompiledFunctionList->Head;pItem;pItem=pItem->NextItem)
    {
        pFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;
        // if function not already hooked
        if (pFunctionInfo->pItemApiInfo==NULL)
            // hook function
            bRet&=HookFunctionUsingAutoHooking(pFunctionInfo);
    }
    pCompiledFunctionList->Unlock();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: StopAutoHooking
// Object: stop net auto hooking
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall StopAutoHooking()
{
    if (!bApiOverrideDllLoaded)
        return FALSE;
    if (!bCorLoaded)
        return FALSE;
    if (!pCompiledFunctionList)
        return FALSE;

    bAutoHooking=FALSE;
    // just don't hook new functions
    // don't unhook as CApiOverride query user choice to unhook or not
    // and if user want to unhook, another message is sent
    // return UnHookAllNetMethods();
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: GetManuallyRegisteredInfos
// Object: return information on a previously installed static hook
// Parameters :
//     in  : TCHAR* AssemblyName : name of assembly
//           mdToken FunctionToken : function token
//           BOOL bUserManagesLock : TRUE if user manages lock on pManuallyRegisteredMonitoringAndOverridingHooks list
//     out : 
//     return : STATIC_NET_HOOK_INFOS* on success, NULL on error
//-----------------------------------------------------------------------------
STATIC_NET_HOOK_INFOS* GetManuallyRegisteredInfos(TCHAR* AssemblyName,mdToken FunctionToken,BOOL bUserManagesLock)
{
    STATIC_NET_HOOK_INFOS* pStaticNetHook;
    TCHAR* AssemblyNameWithoutPath;
    BOOL bAssemblyNamesMatch;

    AssemblyNameWithoutPath=CStdFileOperations::GetFileName(AssemblyName);

    if (!bUserManagesLock)
        pManuallyRegisteredMonitoringAndOverridingHooks->Lock();

    // loop in monitoring and faking list to see if we found a matching AssemblyName/FunctionToken
    CLinkListItem* pItem;
    for (pItem=pManuallyRegisteredMonitoringAndOverridingHooks->Head;pItem;pItem=pItem->NextItem)
    {
        pStaticNetHook=(STATIC_NET_HOOK_INFOS*)pItem->ItemData;
        // if function id matchs
        if (pStaticNetHook->FunctionToken==FunctionToken)
        {
            if (CStdFileOperations::IsFullPath(pStaticNetHook->AssemblyName))
            {
                bAssemblyNamesMatch=(_tcsicmp(pStaticNetHook->AssemblyName,AssemblyName)==0);
            }
            else
            {
                bAssemblyNamesMatch=(_tcsicmp(pStaticNetHook->AssemblyName,AssemblyNameWithoutPath)==0);
            }
            // if assembly names match
            if (bAssemblyNamesMatch)
            {
                // item has been found : unlock list and return item
                if (!bUserManagesLock)
                    pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();
                
                return pStaticNetHook;
            }
        }

    }
    // item not found

    if (!bUserManagesLock)
        pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();

    return NULL;
}

//-----------------------------------------------------------------------------
// Name: InstallStaticHookIfRequired
// Object: install a static hook if required
//         check for static hooks in pManuallyRegisteredMonitoringAndOverridingHooks
//         (loop in monitoring and faking list to see if we found a matching AssemblyName/FunctionToken)
// Parameters :
//     in  : CFunctionInfoEx* pFunctionInfo : informations of compiled function
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL InstallStaticHookIfRequired(CFunctionInfoEx* pFunctionInfo)
{
    BOOL bRet=FALSE;
    STATIC_NET_HOOK_INFOS* pStaticNetHook;

    pManuallyRegisteredMonitoringAndOverridingHooks->Lock();

    // check in monitoring and faking list to see if we found a matching AssemblyName/FunctionToken
    pStaticNetHook=GetManuallyRegisteredInfos(pFunctionInfo->szModule,pFunctionInfo->FunctionToken,TRUE);

    // if no static hook associated to function
    if (pStaticNetHook==NULL)
    {
        pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();
        return TRUE;
    }

    // associate compiled function info to static hook
    pStaticNetHook->pFunctionInfo=pFunctionInfo;

    API_INFO* pApiInfo;
    pApiInfo=(API_INFO*)pStaticNetHook->pItemApiInfo->ItemData;
    // if current item is being removed
    if (pApiInfo->AskedToRemove || pApiInfo->FreeingMemory)
        return FALSE;

    // fill pFunctionInfo->pItemApiInfo
    pFunctionInfo->pItemApiInfo=pStaticNetHook->pItemApiInfo;

    // check compiled function code size
    if (CheckHookAvailability(pFunctionInfo))
    {
        // check if function has been rejitted
        if (pFunctionInfo->AsmCodeStart!=(LPBYTE)pApiInfo->APIAddress)
        {
            //if (pApiInfo->APIAddress!=0)
            //{
            //    // don't try to unhook because we still need ApiInfo data, 
            //    // and memory containing previous jitted code is already (or going to) be free --> don't need to unhook
            //}

            // update api info
            pApiInfo->APIAddress=(FARPROC)pFunctionInfo->AsmCodeStart;
        }

        // as .NET generated calling convention can change according to options each time it's jitted, 
        // update calling convention with the jitted informations
        pApiInfo->CallingConvention=pFunctionInfo->GetWinApiOverrideCallingConvention();

        // hook function
        HookNetInfos.HookAPIFunction(pApiInfo);

        // add to hooked function list
        pHookedFunctionList->AddItem(pFunctionInfo);

        // add ItemApiInfo deletion callback
        if (!pApiInfo->DeletionCallback)
            pApiInfo->DeletionCallback=ApiInfoItemDeletionCallback;

        // function can be hooked
        bRet=TRUE;
    }
    
    pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: CompiledFunctionCallBack
// Object: called each time a function has finished to be built
//         can called be more than once for the same function (rejitted func)
// Parameters :
//     in  : CFunctionInfoEx* pFunctionInfo : informations of compiled function
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CompiledFunctionCallBack(CFunctionInfoEx* pFunctionInfo)
{
    // store function informations
    if (!AddCompiledFunction(pFunctionInfo))
    {
        delete pFunctionInfo;
        return;
    }

    // if apioverride.dll not loaded
    if (!bApiOverrideDllLoaded)
        // do nothing at all
        return;

    // install a static hook if required
    InstallStaticHookIfRequired(pFunctionInfo);

    if (bAutoHooking)
    {
        BOOL bAddAutoMonitoringHook=TRUE;

        // if a static monitoring hook has been defined don't use auto parsing
        if (pFunctionInfo->pItemApiInfo)// check static hook install
        {
            API_INFO* pApiInfo=(API_INFO*)pFunctionInfo->pItemApiInfo->ItemData;
            if (pApiInfo->pMonitoringFileInfos)// check monitoring hook install
            {
                bAddAutoMonitoringHook=FALSE;
            }
        }

        // if static monitoring hook has not been installed
        if (bAddAutoMonitoringHook)
            // install an auto monitoring hook
            HookFunctionUsingAutoHooking(pFunctionInfo);
    }
}

//-----------------------------------------------------------------------------
// Name: AddCompiledFunction
// Object: add informations on a new compiled function into pCompiledFunctionList
//         Notice : if function was already compiled and hooked, remove hook & delete old object
// Parameters :
//     in  : CFunctionInfoEx* pFunctionInfo : informations of function to hook
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL AddCompiledFunction(IN CFunctionInfoEx* pFunctionInfo)
{
    pCompiledFunctionList->Lock();

    //////////////////////////////////////////////////////
    // remove old CFunctionInfo associated to function if any
    //////////////////////////////////////////////////////
    CLinkListItem* pItem;
    CFunctionInfoEx* pParsedFunctionInfo;
    for (pItem=pCompiledFunctionList->Head;pItem;pItem=pItem->NextItem)
    {
        pParsedFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;
        if (pParsedFunctionInfo->FunctionId==pFunctionInfo->FunctionId)
        {
            // unhook function
            UnhookFunctionUsingAutoHooking(pParsedFunctionInfo);

            // free memory of old CFunctionInfo object
            delete pParsedFunctionInfo;

            // remove old CFunctionInfo from pCompiledFunctionList
            pCompiledFunctionList->RemoveItem(pItem,TRUE);

            // stop searching CFunctionInfo
            break;
        }
    }

    //////////////////////////////////////////////////////
    // add new CFunctionInfo associated to function
    //////////////////////////////////////////////////////
    pItem=pCompiledFunctionList->AddItem(pFunctionInfo,TRUE);

    pCompiledFunctionList->Unlock();

    return (pItem!=NULL);
}

//-----------------------------------------------------------------------------
// Name: GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress
// Object: get from an instruction address
// Parameters :
//     in  : PBYTE Address : address of instruction
//           HMODULE* pCallingModuleHandle,
//           TCHAR* ModuleAndFuncName : string that will contain module and function name MUST BE AT LEAST MAX_PATH
//           PBYTE* RelativeAddressFromFunctionStart : relative addres from function start
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress(IN PBYTE Address,
                                                                                                         OUT HMODULE* pCallingModuleHandle,
                                                                                                         OUT TCHAR* ModuleAndFuncName,
                                                                                                         OUT PBYTE* RelativeAddressFromFunctionStart)
{
    if (!bApiOverrideDllLoaded)
        return FALSE;
    if (!bCorLoaded)
        return FALSE;
    if (!pCompiledFunctionList)
        return FALSE;

    _tcscpy(ModuleAndFuncName,_T("Not Found"));
    *RelativeAddressFromFunctionStart=0;

    pCompiledFunctionList->Lock(TRUE);

    //////////////////////////////////////////////////////
    // remove old CFunctionInfo associated to function if any
    //////////////////////////////////////////////////////
    CLinkListItem* pItem;
    CFunctionInfoEx* pParsedFunctionInfo;
    for (pItem=pCompiledFunctionList->Head;pItem;pItem=pItem->NextItem)
    {
        pParsedFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;
        if (   (pParsedFunctionInfo->AsmCodeStart<=Address)
            && (Address<=pParsedFunctionInfo->AsmCodeStart+pParsedFunctionInfo->AsmCodeSize))
        {
            // we have found matching item

            // get relative address from function start
            *RelativeAddressFromFunctionStart=(PBYTE)(Address-pParsedFunctionInfo->AsmCodeStart);

            // get module + function name
            pParsedFunctionInfo->GetName(ModuleAndFuncName,MAX_PATH);

            // get module base address
            *pCallingModuleHandle=(HMODULE)pParsedFunctionInfo->BaseAddress;

            pCompiledFunctionList->Unlock();
            return TRUE;
        }
    }

    //////////////////////////////////////////////////////
    // add new CFunctionInfo associated to function
    //////////////////////////////////////////////////////

    pCompiledFunctionList->Unlock();

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: GetNetCompiledFunctionSize
// Object: Get size of jitted function code (asm code length of the function)
// Parameters :
//     in  : PBYTE NetCompiledFunctionAddress : address provided by GetNetCompiledFunctionAddress
//     out : 
//     return : 0 on failure, function length in bytes on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) DWORD __stdcall GetNetCompiledFunctionSize(PBYTE NetCompiledFunctionAddress)
{
    if (!bApiOverrideDllLoaded)
        return NULL;
    if (!bCorLoaded)
        return NULL;
    if (!pCompiledFunctionList)
        return NULL;

    // loops throw list of compiled functions to find function size
    pCompiledFunctionList->Lock(FALSE);

    CLinkListItem* pItem;
    CFunctionInfoEx* pParsedFunctionInfo;
    for (pItem=pCompiledFunctionList->Head;pItem;pItem=pItem->NextItem)
    {
        pParsedFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;
        // if same AsmCodeStart
        if (pParsedFunctionInfo->AsmCodeStart==NetCompiledFunctionAddress)
        {
            // unlock list
            pCompiledFunctionList->Unlock();

            // return function info
            return pParsedFunctionInfo->AsmCodeSize;
        }
    }
    pCompiledFunctionList->Unlock();
    return 0;
}


//-----------------------------------------------------------------------------
// Name: GetNetCompiledFunctionAddress
// Object: Get address of jitted function code (asm code start of the function)
// Parameters :
//     in  : TCHAR* pszFuncDescription : like .NET@AssemblyName@functionToken
//     out : 
//     return : NULL on failure, function start address on success
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) PBYTE __stdcall GetNetCompiledFunctionAddress(TCHAR* pszFuncDescription)
{
    if (!bApiOverrideDllLoaded)
        return NULL;
    if (!bCorLoaded)
        return NULL;
    if (!pCompiledFunctionList)
        return NULL;

    PBYTE AsmFunctionAddress;
    TCHAR AssemblyName[MAX_PATH];
    mdToken FunctionToken;
    if (!GetAssemblyNameAndTokenFromFunctionDescription(pszFuncDescription,AssemblyName,MAX_PATH,&FunctionToken))
    {
        ReportBadNetFunctionDescription(pszFuncDescription);
        return NULL;
    }

    // definition is ok

    // find asm code start in pCompiledFunctionList
    AsmFunctionAddress=NULL;
    pCompiledFunctionList->Lock(FALSE);
    // find matching pParsedFunctionInfo in pCompiledFunctionList
    CFunctionInfoEx* pParsedFunctionInfo=GetFunctionInfo(AssemblyName,FunctionToken,TRUE);
    // in case of success
    if (pParsedFunctionInfo)
    {
        // get asm start address
        AsmFunctionAddress=pParsedFunctionInfo->AsmCodeStart;
    }
    pCompiledFunctionList->Unlock();

    return AsmFunctionAddress;
}

//-----------------------------------------------------------------------------
// Name: GetFunctionInfo
// Object: get CFunctionInfo object in pCompiledFunctionList
//         matching provided AssemblyName and FunctionToken
//         WARNING it looks only in compiled function, that means 
//         function can return NULL if function exists in the assembly but has never been called
// Parameters :
//     in  : 
//     out : 
//     return : CFunctionInfoEx* on success, NULL if no function has been found
//              for such AssemblyName and FunctionToken
//-----------------------------------------------------------------------------
CFunctionInfoEx* GetFunctionInfo(TCHAR* AssemblyName,mdToken FunctionToken,BOOL UserManagesLock)
{
    if (!UserManagesLock)
        pCompiledFunctionList->Lock(FALSE);

    TCHAR* AssemblyNameWithoutPath;
    BOOL bAssemblyNamesMatch;
    BOOL bFullPathChecking=CStdFileOperations::IsFullPath(AssemblyName);
    

    CLinkListItem* pItem;
    CFunctionInfoEx* pParsedFunctionInfo;
    for (pItem=pCompiledFunctionList->Head;pItem;pItem=pItem->NextItem)
    {
        pParsedFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;
        // if same token
        if (pParsedFunctionInfo->FunctionToken==FunctionToken)
        {
            if (bFullPathChecking)
            {
                bAssemblyNamesMatch=(_tcsicmp(pParsedFunctionInfo->szModule,AssemblyName)==0);
            }
            else
            {
                AssemblyNameWithoutPath=CStdFileOperations::GetFileName(pParsedFunctionInfo->szModule);
                bAssemblyNamesMatch=(_tcsicmp(AssemblyNameWithoutPath,AssemblyName)==0);
            }
            // if same assembly name
            if (bAssemblyNamesMatch)
            {
                if (!UserManagesLock)
                    pCompiledFunctionList->Unlock();

                // return function info
                return pParsedFunctionInfo;
            }
        }
    }

    if (!UserManagesLock)
        pCompiledFunctionList->Unlock();

    return NULL;
}

//-----------------------------------------------------------------------------
// Name: GetApiInfoFromFuncId
// Object: get associated API_INFO structure for an hook .Net function
// Parameters :
//     in  : FunctionID functionId : .NET function Id
//     return : pointer on associated API_INFO structure if hooked, NULL if function is not hooked
//-----------------------------------------------------------------------------
API_INFO* GetApiInfoFromFuncId(FunctionID functionId)
{
    API_INFO* pApiInfo=NULL;
    // for each compiled function
    CLinkListItem* pItem;
    CFunctionInfoEx* pParsedFunctionInfo;
    pHookedFunctionList->Lock();
    for (pItem=pHookedFunctionList->Head;pItem;pItem=pItem->NextItem)
    {
        pParsedFunctionInfo=(CFunctionInfoEx*)pItem->ItemData;
        // if same token
        if (pParsedFunctionInfo->FunctionId==functionId)
        {
            if (pParsedFunctionInfo->pItemApiInfo)
                pApiInfo=(API_INFO*)pParsedFunctionInfo->pItemApiInfo->ItemData;
        }
    }
    pHookedFunctionList->Unlock();
    return pApiInfo;
}

//-----------------------------------------------------------------------------
// Name: GetAssemblyNameAndTokenFromFunctionDescription
// Object: extract assembly name and function token from 
//         a function description like .NET@AssemblyName@functionToken
// Parameters :
//     in  : TCHAR* pszFuncDescription : like .NET@AssemblyName@functionToken
//           DWORD AssemblyNameMaxLen : max size of AssemblyName in tchar
//     in out : TCHAR* AssemblyName,
//     out : mdToken* pFunctionToken : found function token
//     return : FALSE on failure, TRUE on success
//-----------------------------------------------------------------------------
BOOL GetAssemblyNameAndTokenFromFunctionDescription(IN TCHAR* FuncDescription,IN OUT TCHAR* AssemblyName,DWORD AssemblyNameMaxLen,OUT mdToken* pFunctionToken)
{
    TCHAR* pszAssemblyName;
    TCHAR* pszFunctionTocken;

    // copy FuncDescription in local var to avoid original string changes
    TCHAR* pszLocalDefinition= (TCHAR*) _alloca((_tcslen(FuncDescription)+1)*sizeof(TCHAR));
    _tcscpy(pszLocalDefinition,FuncDescription);

    // get prefix size
    size_t PrefixLen=_tcslen(DLL_OR_EXE_NET_PREFIX);

    // check .NET@ prefix
    if (_tcsnicmp(pszLocalDefinition,DLL_OR_EXE_NET_PREFIX,PrefixLen)!=0)
        return FALSE;

    // point after prefix
    pszAssemblyName=pszLocalDefinition+PrefixLen;

    // find next @
    pszFunctionTocken=_tcschr(pszAssemblyName,'@');

    // if not found
    if (!pszFunctionTocken)
        return FALSE;

    // end Assembly name
    *pszFunctionTocken=0;

    // point after @
    pszFunctionTocken++;

    // copy pszAssemblyName name inside AssemblyName parameter
    _tcsncpy(AssemblyName,pszAssemblyName,AssemblyNameMaxLen);
    AssemblyName[AssemblyNameMaxLen-1]=0;

    // get token value
    if (!CStringConverter::StringToDWORD(pszFunctionTocken,(DWORD*)pFunctionToken))
        return FALSE;

    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: AddHookNetMonitoringDefinition
// Object: add a hook from a monitoring file (design only for static hook)
//         hook removal is done like a standard monitoring file inside ApiOverride.dll
// Parameters :
//     in  : MONITORING_FILE_INFOS* pMonitoringFileInfo : Monitoring file pointer stored in API_INFO struct associated to hook
//           TCHAR* pszFullDescription : full hook description (full line of monitoring file)
//           TCHAR* pszFileName : monitoring file name
//           DWORD dwLineNumber : monitoring line number currently parsed
//
//     out : MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo : contains monitoring file information owning the hook
//                                                                        if hook is already install
//                                                                        NULL if function wasn't already hooked
//           BOOL* pbParsingError : TRUE if a parsing error occurs
//     return : TRUE in case of success, FALSE in case of error
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) 
BOOL __stdcall AddHookNetMonitoringDefinition(MONITORING_FILE_INFOS* pMonitoringFileInfo,
                                              TCHAR* pszFullDescription,
                                              TCHAR* pszFileName,
                                              DWORD dwLineNumber,
                                              MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo,
                                              BOOL* pbParsingError)
{
    if (!bApiOverrideDllLoaded)
        return FALSE;
    if (!pManuallyRegisteredMonitoringAndOverridingHooks)
        return FALSE;

    TCHAR AssemblyName[MAX_PATH];
    TCHAR* pszFunctionName;
    TCHAR* pszParameters;
    TCHAR* pszOptions;
    tagCALLING_CONVENTION CallingConvention;
    BOOL bAPIInfoAlreadInitialized=FALSE;
    BOOL bAlreadyHooked=FALSE;
    CLinkListItem* pItemRegisteredHookCreated=NULL;
    STATIC_NET_HOOK_INFOS* pStaticNetHook;
    CFunctionInfoEx* pFunctionInfo;
    API_INFO* pApiInfo;

    *pbParsingError=FALSE;
    *ppAlreadyHookingMonitoringFileInfo=NULL;

    mdToken FunctionToken;
    if (!GetAssemblyNameAndTokenFromFunctionDescription(pszFullDescription,AssemblyName,MAX_PATH,&FunctionToken))
    {
        *pbParsingError=TRUE;
        return FALSE;
    }
    // point to func description
    TCHAR* pszFunctionDescription=_tcschr(pszFullDescription,FIELDS_SEPARATOR);
    if (!pszFunctionDescription)
    {
        *pbParsingError=TRUE;
        return FALSE;
    }
    // point after field separator
    pszFunctionDescription++;

    // parse function description
    if (!HookNetInfos.ParseFunctionDescription(pszFunctionDescription,&pszFunctionName,&pszParameters,&pszOptions,&CallingConvention))
    {
        *pbParsingError=TRUE;
        return FALSE;
    }

    pManuallyRegisteredMonitoringAndOverridingHooks->Lock();

    // check in monitoring and faking list to see if we found a matching AssemblyName/FunctionToken
    pStaticNetHook=GetManuallyRegisteredInfos(AssemblyName,FunctionToken,TRUE);
    // if a static hook has been already defined for this function
    if (pStaticNetHook)
    {
        bAPIInfoAlreadInitialized=FALSE;
        if (pStaticNetHook->pItemApiInfo)
        {
            pApiInfo=(API_INFO*)pStaticNetHook->pItemApiInfo->ItemData;
            if (pApiInfo)
            {
                // if not auto hooking
                if (pApiInfo->pMonitoringFileInfos)
                {
                    if (pApiInfo->pMonitoringFileInfos!=&HookNetDll_IMetaDataImportMonitoringFileInfo)
                    {
                        TCHAR szMsg[2*MAX_PATH];
                        _stprintf(szMsg,_T("Monitoring hook has already been defined by %s"),pApiInfo->pMonitoringFileInfos->szFileName);
                        ReportMessage(REPORT_MESSAGE_WARNING,szMsg);
                        pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();
                        return FALSE;
                    }
                }

                // user may want to overwrite auto hooking informations (to add some options as conditional logging)
                bAPIInfoAlreadInitialized=TRUE;
            }
        }
    }
    else // no static hook has been defined for this function
    {
        bAPIInfoAlreadInitialized=FALSE;
        // add new item to static hook list
        pItemRegisteredHookCreated=pManuallyRegisteredMonitoringAndOverridingHooks->AddItem(TRUE);

        // get pointer on created StaticNetHook struct
        pStaticNetHook=(STATIC_NET_HOOK_INFOS*)pItemRegisteredHookCreated->ItemData;

        // store assembly name and function token in new created struct
        _tcsncpy(pStaticNetHook->AssemblyName,AssemblyName,MAX_PATH);
        pStaticNetHook->AssemblyName[MAX_PATH-1]=0;
        pStaticNetHook->FunctionToken=FunctionToken;

        pStaticNetHook->pMonitoringFileInfo=pMonitoringFileInfo;
    }

    // search in compiled func list to know if function has been jitted (built)
    pFunctionInfo=GetFunctionInfo(AssemblyName,FunctionToken,FALSE);

    // get pApiInfo
    // if function is built, get pApiInfo from function address
    if (pFunctionInfo)
    {
        if (pStaticNetHook->pItemApiInfo)
        {
            pApiInfo=(API_INFO*)pStaticNetHook->pItemApiInfo->ItemData;
            pApiInfo->APIAddress=(FARPROC)pFunctionInfo->AsmCodeStart;
        }
        else
        {
            pStaticNetHook->pItemApiInfo=HookNetInfos.GetAssociatedItemAPIInfo(pFunctionInfo->AsmCodeStart,&bAlreadyHooked);
            bAPIInfoAlreadInitialized=bAlreadyHooked;
        }
    }
    // else, create an empty ApiInfo struct
    else
    {
        // avoid to loose some infos : create on only if not existing
        if (!pStaticNetHook->pItemApiInfo)
            pStaticNetHook->pItemApiInfo=HookNetInfos.QueryEmptyItemAPIInfo();
    }

    // on memory allocation error
    if (!pStaticNetHook->pItemApiInfo)
        goto OnError;

    pApiInfo=(API_INFO*)pStaticNetHook->pItemApiInfo->ItemData;

    // if a hook was prepared/installed
    if (bAPIInfoAlreadInitialized)
    {
        // remove monitoring parameters options if any
        // quite useless until only auto hooked monitoring can be override
        // as auto hooking don't put parameter conditional logging or breaking
        HookNetInfos.FreeOptionalParametersMemory(pApiInfo);
    }
    else // not initialized : function hasn't been built
    {
        if (!HookNetInfos.InitializeApiInfo(pApiInfo,AssemblyName,pszFunctionName))
            goto OnError;
        
        if (pFunctionInfo)
            pApiInfo->APIAddress=(FARPROC)pFunctionInfo->AsmCodeStart;

    }
    // fill hook informations ACCORDING TO PROVIDED INFORMATIONS (NOT USING METADATA INFO)
    if (!HookNetInfos.ParseParameters(pApiInfo,pszParameters,pszFileName,dwLineNumber))
    {
        *pbParsingError=TRUE;
        goto OnError;
    }
    if (!HookNetInfos.ParseOptions(pApiInfo,pszOptions,bAlreadyHooked,pszFileName,dwLineNumber))
    {
        *pbParsingError=TRUE;
        goto OnError;
    }

    pApiInfo->pMonitoringFileInfos=pMonitoringFileInfo;
    pApiInfo->HookType=HOOK_TYPE_NET;
    pApiInfo->HookTypeExtendedFunctionInfos.InfosForNET.FunctionToken=FunctionToken;

    // if function is already jitted without being hooked
    if (pFunctionInfo && (!bAlreadyHooked))
    {
        pApiInfo->CallingConvention=pFunctionInfo->GetWinApiOverrideCallingConvention();
        // hook it
        if(!HookNetInfos.HookAPIFunction(pApiInfo))
            goto OnError;
        // add ItemApiInfo deletion callback
        if (!pApiInfo->DeletionCallback)
            pApiInfo->DeletionCallback=ApiInfoItemDeletionCallback;
    }

    pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();

    return TRUE;

OnError:
    if (!bAlreadyHooked)
    {
        // free it BEFORE REMOVING pItemRegisteredHookCreated
        if (pStaticNetHook->pItemApiInfo)
            HookNetInfos.FreeApiInfoItem(pStaticNetHook->pItemApiInfo);

        // if item has been added to list
        if (pItemRegisteredHookCreated)
        {
            // remove created item from list
            pManuallyRegisteredMonitoringAndOverridingHooks->RemoveItem(pItemRegisteredHookCreated,TRUE);
        }
    }

    pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: AddHookNetFakingDefinition
// Object: add a faking hook or pre/post api call hook for Net objects
// Parameters :
//     in  : FAKING_DLL_INFOS* pFakingDllInfos : Faking file pointer stored in API_INFO struct associated to hook
//           STRUCT_FAKE_API_WITH_USERPARAM* pFakeApiInfos : full hook description
//           TCHAR* pszDllName : faking dll file name
//           DWORD dwFunctionIndex : 0 based index of faking function definition array of faking dll
//           tagFakingDllArray FakingType : faking way : faking, pre or post hook
//     out : 
//           FAKING_DLL_INFOS** ppAlreadyHookingFakingDllInfos : NULL if method wasn't already hooked
//                                                               faking dll information of dll owning the hook
//           BOOL* pbParsingError : TRUE if a parsing error occurs
//     return : TRUE in case of success, FALSE in case of error
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) 
BOOL __stdcall AddHookNetFakingDefinition(FAKING_DLL_INFOS* pFakingDllInfos,
                                          STRUCT_FAKE_API_WITH_USERPARAM* pFakeApiInfos,
                                          TCHAR* pszDllName,
                                          DWORD dwFunctionIndex,
                                          tagFakingDllArray FakingType,
                                          FAKING_DLL_INFOS** ppAlreadyHookingFakingDllInfos,
                                          BOOL* pbParsingError)
{
    UNREFERENCED_PARAMETER(dwFunctionIndex);
    UNREFERENCED_PARAMETER(pszDllName);
    if (!bApiOverrideDllLoaded)
        return FALSE;
    if (!pManuallyRegisteredMonitoringAndOverridingHooks)
        return FALSE;

    TCHAR AssemblyName[MAX_PATH];
    BOOL bAPIInfoAlreadInitialized=FALSE;
    BOOL bAlreadyHooked=FALSE;
    CLinkListItem* pItemRegisteredHookCreated=NULL;
    STATIC_NET_HOOK_INFOS* pStaticNetHook;
    CFunctionInfoEx* pFunctionInfo;
    API_INFO* pApiInfo;

    *pbParsingError=FALSE;
    *ppAlreadyHookingFakingDllInfos=NULL;

    mdToken FunctionToken;
    if (!GetAssemblyNameAndTokenFromFunctionDescription(pFakeApiInfos->pszModuleName,AssemblyName,MAX_PATH,&FunctionToken))
    {
        *pbParsingError=TRUE;
        return FALSE;
    }

    pManuallyRegisteredMonitoringAndOverridingHooks->Lock();

    // check in monitoring and faking list to see if we found a matching AssemblyName/FunctionToken
    pStaticNetHook=GetManuallyRegisteredInfos(AssemblyName,FunctionToken,TRUE);
    // if a static hook has been already defined for this function
    if (pStaticNetHook)
    {
        bAPIInfoAlreadInitialized=FALSE;
        if (pStaticNetHook->pItemApiInfo)
        {
            pApiInfo=(API_INFO*)pStaticNetHook->pItemApiInfo->ItemData;
            if (pApiInfo)
            {
                if (pApiInfo->FakeAPIAddress && (FakingType==FAKING_DLL_ARRAY_FAKING))
                {
                    *ppAlreadyHookingFakingDllInfos=pApiInfo->pFakeDllInfos;
                    pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();
                    return FALSE;
                }

                // user may want to overwrite auto hooking informations (to add some options as conditional logging)
                bAPIInfoAlreadInitialized=TRUE;
            }
        }
    }
    else // no static hook has been defined for this function
    {
        bAPIInfoAlreadInitialized=FALSE;
        // add new item to static hook list
        pItemRegisteredHookCreated=pManuallyRegisteredMonitoringAndOverridingHooks->AddItem(TRUE);

        // get pointer on created StaticNetHook struct
        pStaticNetHook=(STATIC_NET_HOOK_INFOS*)pItemRegisteredHookCreated->ItemData;

        // store assembly name and function token in new created struct
        _tcsncpy(pStaticNetHook->AssemblyName,AssemblyName,MAX_PATH);
        pStaticNetHook->AssemblyName[MAX_PATH-1]=0;
        pStaticNetHook->FunctionToken=FunctionToken;
        pStaticNetHook->pFakingDllInfos=pFakingDllInfos;
    }

    // search in compiled func list to know if function has been jitted (built)
    pFunctionInfo=GetFunctionInfo(AssemblyName,FunctionToken,FALSE);

    // get pApiInfo
    // if function is built, get pApiInfo from function address
    if (pFunctionInfo)
    {
        // if an overriding hook was already defined
        if (pStaticNetHook->pItemApiInfo)
        {
            pApiInfo=(API_INFO*)pStaticNetHook->pItemApiInfo->ItemData;
            pApiInfo->APIAddress=(FARPROC)pFunctionInfo->AsmCodeStart;
        }
        else
        {
            pStaticNetHook->pItemApiInfo=HookNetInfos.GetAssociatedItemAPIInfo(pFunctionInfo->AsmCodeStart,&bAlreadyHooked);
            bAPIInfoAlreadInitialized=bAlreadyHooked;
        }
    }
    // else, create an empty ApiInfo struct
    else
    {
        // avoid to loose some infos : create on only if not existing
        if (!pStaticNetHook->pItemApiInfo)
            pStaticNetHook->pItemApiInfo=HookNetInfos.QueryEmptyItemAPIInfo();
    }

    // on memory allocation error
    if (!pStaticNetHook->pItemApiInfo)
        goto OnError;

    pApiInfo=(API_INFO*)pStaticNetHook->pItemApiInfo->ItemData;

    // if a hook wasn't prepared/installed
    if (!bAPIInfoAlreadInitialized)
    // not initialized : function hasn't been built
    {
        if (!HookNetInfos.InitializeApiInfo(pApiInfo,AssemblyName,pFakeApiInfos->pszAPIName))
            goto OnError;
        
        if (pFunctionInfo)
            pApiInfo->APIAddress=(FARPROC)pFunctionInfo->AsmCodeStart;

        pApiInfo->HookType=HOOK_TYPE_NET;
        pApiInfo->HookTypeExtendedFunctionInfos.InfosForNET.FunctionToken=pFunctionInfo->FunctionToken;
        // fill stack size
        pApiInfo->StackSize=pFakeApiInfos->StackSize;

        if (pFakingDllInfos->ApiOverrideBuildVersionFramework>=6)
        {
            // for backward compatibility, 
            // pFakeApiInfos->FirstBytesCanExecuteAnywhereSize becomes an option flags and FirstBytesCanExecuteAnywhereSize is reduce to an unsigned 8 bits value (sufficient for our goal currently max size is 64 bits)

            // get FirstBytesCanExecuteAnywhereSize 
            pApiInfo->FirstBytesCanExecuteAnywhereSize = (pFakeApiInfos->AdditionalOptions & OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_SIZE_MASK);
            if ((pApiInfo->FirstBytesCanExecuteAnywhereSize & OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE)== OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE)
            {
                pApiInfo->FirstBytesCanExecuteAnywhereSize = (DWORD)(-1);
            }

            pApiInfo->DontCheckModulesFiltersForFaking = ((pFakeApiInfos->AdditionalOptions & OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS)== OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS);
        }
        else
        {
            // set the first bytes can execute anywhere option
            pApiInfo->FirstBytesCanExecuteAnywhereSize=pFakeApiInfos->AdditionalOptions;
        }

    }

    // check specific cases of pre and post hook informations
    switch(FakingType)
    {
    case FAKING_DLL_ARRAY_PRE_HOOK:
        if(!HookNetInfos.AddPreApiCallCallBack(pStaticNetHook->pItemApiInfo,(!bAlreadyHooked) && pFunctionInfo,pFakingDllInfos->hModule,(pfPreApiCallCallBack)pFakeApiInfos->FakeAPI,pFakeApiInfos->UserParam))
            goto OnError;

        // add ItemApiInfo deletion callback
        if (!pApiInfo->DeletionCallback)
            pApiInfo->DeletionCallback=ApiInfoItemDeletionCallback;

        break;

    case FAKING_DLL_ARRAY_POST_HOOK:
        if (!HookNetInfos.AddPostApiCallCallBack(pStaticNetHook->pItemApiInfo,(!bAlreadyHooked) && pFunctionInfo,pFakingDllInfos->hModule,(pfPostApiCallCallBack)pFakeApiInfos->FakeAPI,pFakeApiInfos->UserParam))
            goto OnError;

        // add ItemApiInfo deletion callback
        if (!pApiInfo->DeletionCallback)
            pApiInfo->DeletionCallback=ApiInfoItemDeletionCallback;

        break;

    case FAKING_DLL_ARRAY_FAKING:
        pApiInfo->FakeAPIAddress=pFakeApiInfos->FakeAPI;

        // if api was already hooked
        if (bAlreadyHooked)
        {
            pApiInfo->pFakeDllInfos=pFakingDllInfos;
            if (!pApiInfo->DeletionCallback)
                pApiInfo->DeletionCallback=ApiInfoItemDeletionCallback;
            break;
        }
        else
        {
            // if function is already jitted without being hooked
            if (pFunctionInfo)
            {
                pApiInfo->CallingConvention=pFunctionInfo->GetWinApiOverrideCallingConvention();
                // hook it
                if(!HookNetInfos.HookAPIFunction(pApiInfo))
                    goto OnError;
                pApiInfo->DeletionCallback=ApiInfoItemDeletionCallback;
            }
        }
        break;
    }

    pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();

    return TRUE;

OnError:
    if (!bAlreadyHooked)
    {
        // free it BEFORE REMOVING pItemRegisteredHookCreated
        if (pStaticNetHook->pItemApiInfo)
            HookNetInfos.FreeApiInfoItem(pStaticNetHook->pItemApiInfo);

        // if item has been added to list
        if (pItemRegisteredHookCreated)
        {
            // remove created item from list
            pManuallyRegisteredMonitoringAndOverridingHooks->RemoveItem(pItemRegisteredHookCreated,TRUE);
        }
    }

    pManuallyRegisteredMonitoringAndOverridingHooks->Unlock();

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: ClearUserDataTypeCache
// Object: clear user data type cache
//         called on each file change in UserTypes directory
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall ClearUserDataTypeCache()
{
    CSupportedParameters::ClearUserDataTypeCache();
    return TRUE;
}