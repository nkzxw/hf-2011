#include "net_manager.h"

#include "ModuleFilters.h"
#include "ReportMessage.h"
#include "ProcessInternalCallRequest.h"
#include "../../../File/StdFileOperations.h"
#include "../../../pe/PE.h"


extern HANDLE hevtFreeProcess;
extern HINSTANCE DllhInstance;
extern BOOL __stdcall GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress(IN PBYTE pOriginAddress,
                                                                               OUT HMODULE* pCallingModuleHandle,
                                                                               OUT TCHAR* pszModuleName,
                                                                               OUT PBYTE* pRelativeAddress,
                                                                               OUT BOOL* pbShouldLog,
                                                                               BOOL TakeAnotherSnapshotIfNotFound,
                                                                               BOOL ContinueEvenIfShouldNotBeLogged);

CNET_Manager::CNET_Manager(void)
{
    TCHAR AppPath[MAX_PATH];
    CStdFileOperations::GetAppName(AppPath,MAX_PATH);
    CPE Pe(AppPath);
    this->bIsNetApplication=Pe.IsNET();
    this->bStarted=FALSE;
    this->ResetFunctionsPointer();
    memset(&this->HookNetOptions,0,sizeof(HOOK_NET_OPTIONS));
    this->HookNetOptions.DisableOptimization=FALSE;
    this->HookNetOptions.EnableFrameworkMonitoring=FALSE;
    this->HookNetOptions.MonitorException=TRUE;
}

CNET_Manager::~CNET_Manager(void)
{
    this->StopAutoHooking();
    this->UnHookAllNetMethods();
    // signal hooknet.dll that we are going to unload apioverride.dll
    this->Uninitialize();
}

BOOL CNET_Manager::ResetFunctionsPointer()
{
    this->f_InitializeHookNet=NULL;
    this->f_SetHookNetOptions=NULL;
    this->f_GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress=NULL;
    this->f_StartAutoHooking=NULL;
    this->f_StopAutoHooking=NULL;
    this->f_UnHookAllNetMethods=NULL;
    this->f_GetNetCompiledFunctionAddress=NULL;
    this->f_GetNetCompiledFunctionSize=NULL;
    this->f_AddHookNetFakingDefinition=NULL;
    this->f_AddHookNetMonitoringDefinition=NULL;
    this->f_RemoveHookNetFakingDefinition=NULL;
    this->f_RemoveHookNetMonitoringDefinition=NULL;
    this->f_Uninitialize=NULL;
    this->f_ShowNetInteraction=NULL;
    this->f_ClearUserDataTypeCache=NULL;
    return TRUE;
}

void CNET_Manager::ReportNetHookingMustBeEnableBeforeAppStart()
{
    if (this->bIsNetApplication)
        DynamicMessageBoxInDefaultStation(NULL,_T(".NET hooking must be enable before application starting"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
    else
        DynamicMessageBoxInDefaultStation(NULL,_T("No .NET Module currently loaded"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
}

// load Hook net exported functions
// and set Net options
BOOL CNET_Manager::InitializeHookNetDll()
{
    TCHAR HookNetDllPath[MAX_PATH];
    // get current dll path
    CStdFileOperations::GetModulePath(DllhInstance,HookNetDllPath,MAX_PATH);
    // forge hook net dll full path
    _tcscat(HookNetDllPath,HOOK_NET_DLL_NAME);
    // get hook net module handle
    this->HookNetDllModule=GetModuleHandle(HookNetDllPath);
    // if HookNet dll not loaded
    if (!this->HookNetDllModule)
        return FALSE;

    // load net exported functions
    this->f_InitializeHookNet=(pfInitializeHookNet)GetProcAddress(this->HookNetDllModule,HOOK_NET_InitializeHookNet_EXPORTED_FUNC_NAME);
    this->f_SetHookNetOptions=(pfSetHookNetOptions)GetProcAddress(this->HookNetDllModule,HOOK_NET_SetHookNetOptions_EXPORTED_FUNC_NAME);
    this->f_GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress=(pfNETGetModuleNameAndRelativeAddressFromCallerAbsoluteAddress)GetProcAddress(this->HookNetDllModule,HOOK_NET_GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress);
    this->f_StartAutoHooking=(pfNETStartAutoHooking)GetProcAddress(this->HookNetDllModule,HOOK_NET_StartAutoHooking_EXPORTED_FUNC_NAME);
    this->f_StopAutoHooking=(pfNETStopAutoHooking)GetProcAddress(this->HookNetDllModule,HOOK_NET_StopAutoHooking_EXPORTED_FUNC_NAME);
    this->f_UnHookAllNetMethods=(pfUnHookAllNetMethods)GetProcAddress(this->HookNetDllModule,HOOK_NET_UnHookAllNetMethods_EXPORTED_FUNC_NAME);
    this->f_GetNetCompiledFunctionAddress=(pfGetNetCompiledFunctionAddress)GetProcAddress(this->HookNetDllModule,HOOK_NET_GetNetCompiledFunctionAddress_EXPORTED_FUNC_NAME);
    this->f_GetNetCompiledFunctionSize=(pfGetNetCompiledFunctionSize)GetProcAddress(this->HookNetDllModule,HOOK_NET_GetNetCompiledFunctionSize_EXPORTED_FUNC_NAME);
    this->f_AddHookNetFakingDefinition=(pfAddHookNetFakingDefinition)GetProcAddress(this->HookNetDllModule,HOOK_NET_AddHookNetFakingDefinition_EXPORTED_FUNC_NAME);
    this->f_AddHookNetMonitoringDefinition=(pfAddHookNetMonitoringDefinition)GetProcAddress(this->HookNetDllModule,HOOK_NET_AddHookNetMonitoringDefinition_EXPORTED_FUNC_NAME);
    this->f_RemoveHookNetFakingDefinition=(pfRemoveHookNetFakingDefinition)GetProcAddress(this->HookNetDllModule,HOOK_NET_RemoveHookNetFakingDefinition_EXPORTED_FUNC_NAME);
    this->f_RemoveHookNetMonitoringDefinition=(pfRemoveHookNetMonitoringDefinition)GetProcAddress(this->HookNetDllModule,HOOK_NET_RemoveHookNetMonitoringDefinition_EXPORTED_FUNC_NAME);
    this->f_Uninitialize=(pfUninitialize)GetProcAddress(this->HookNetDllModule,HOOK_NET_Uninitialize_EXPORTED_FUNC_NAME);
    this->f_ShowNetInteraction=(pfShowNetInteraction)GetProcAddress(this->HookNetDllModule,HOOK_NET_ShowNetInteraction_EXPORTED_FUNC_NAME);
    this->f_ClearUserDataTypeCache=(pfClearUserDataTypeCache)GetProcAddress(this->HookNetDllModule,HOOK_NET_ClearUserDataTypeCache_EXPORTED_FUNC_NAME);
	this->f_AddHookNetFromTokenForCompiledFuntions=(pfAddHookNetFromTokenForCompiledFuntions)GetProcAddress(this->HookNetDllModule,HOOK_NET_AddHookNetFromTokenForCompiledFuntions_EXPORTED_FUNC_NAME);
	this->f_RemoveHookNetFromTokenForCompiledFuntion=(pfRemoveHookNetFromTokenForCompiledFuntion)GetProcAddress(this->HookNetDllModule,HOOK_NET_RemoveHookNetFromTokenForCompiledFuntions_EXPORTED_FUNC_NAME);

    if (  (!this->f_InitializeHookNet)
        ||(!this->f_SetHookNetOptions)
        ||(!this->f_GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress)
        ||(!this->f_StartAutoHooking)
        ||(!this->f_StopAutoHooking)
        ||(!this->f_UnHookAllNetMethods)
        ||(!this->f_GetNetCompiledFunctionAddress)
        ||(!this->f_GetNetCompiledFunctionSize)
        ||(!this->f_AddHookNetFakingDefinition)
        ||(!this->f_AddHookNetMonitoringDefinition)
        ||(!this->f_RemoveHookNetFakingDefinition)
        ||(!this->f_RemoveHookNetMonitoringDefinition)
        ||(!this->f_Uninitialize)
        ||(!this->f_ShowNetInteraction)
        ||(!this->f_ClearUserDataTypeCache)
		||(!this->f_AddHookNetFromTokenForCompiledFuntions)
		||(!this->f_RemoveHookNetFromTokenForCompiledFuntion)
        )
    {
#ifdef _DEBUG
        OutputDebugString(_T("ERROR LOADING HOOKNET.DLL check if dll exists and its exports"));
#endif
        return FALSE;
    }

    HOOK_NET_INIT InitHookNet;

    InitHookNet.hevtFreeProcess=hevtFreeProcess;

    // get functions handlers
    InitHookNet.DynamicMessageBoxInDefaultStation=DynamicMessageBoxInDefaultStation;
    InitHookNet.FreeApiInfoItem=FreeApiInfoItem;
    InitHookNet.GetAssociatedItemAPIInfo=GetAssociatedItemAPIInfo;
    InitHookNet.HookAPIFunction=HookAPIFunction;
    InitHookNet.InitializeApiInfo=InitializeApiInfo;
    InitHookNet.ParseFunctionDescription=ParseFunctionDescription;
    InitHookNet.ParseOptions=ParseOptions;
    InitHookNet.ParseParameters=ParseParameters;
    InitHookNet.CreateParameterConditionalBreakContentListIfDoesntExist=CreateParameterConditionalBreakContentListIfDoesntExist;
    InitHookNet.CreateParameterConditionalLogContentListIfDoesntExist=CreateParameterConditionalLogContentListIfDoesntExist;
    InitHookNet.AddPostApiCallCallBack=AddPostApiCallCallBack;
    InitHookNet.RemovePostApiCallCallBack=RemovePostApiCallCallBack;
    InitHookNet.AddPreApiCallCallBack=AddPreApiCallCallBack;
    InitHookNet.RemovePreApiCallCallBack=RemovePreApiCallCallBack;
    InitHookNet.UnHookIfPossible=UnHookIfPossible;
    InitHookNet.ReportMessage=CReportMessage::ReportMessage;
    InitHookNet.GetWinAPIOverrideFunctionDescriptionAddress=GetWinAPIOverrideFunctionDescriptionAddress;
    InitHookNet.GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress=::GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress;
    InitHookNet.SetDefaultStation=CDialogInterfaceManager::SetDefaultStation;
    InitHookNet.RestoreStation=CDialogInterfaceManager::RestoreStation;
    InitHookNet.CanWindowInteract=CDialogInterfaceManager::CanWindowInteract;
    InitHookNet.AdjustThreadSecurityAndLaunchDialogThread=CDialogInterfaceManager::AdjustThreadSecurityAndLaunchDialogThread;
    InitHookNet.ProcessInternalCallRequestEx=CProcessInternalCallRequest::ProcessInternalCallRequestEx;
    InitHookNet.QueryEmptyItemAPIInfo=QueryEmptyItemAPIInfo;
    InitHookNet.FreeOptionalParametersMemory=FreeOptionalParametersMemory;
    InitHookNet.NetExceptionSearchFunctionLeaveCallBack=NetExceptionSearchFunctionLeaveCallBack;
    InitHookNet.NetExceptionCatcherEnterCallBack=NetExceptionCatcherEnterCallBack;
    InitHookNet.NetTlsRestoreHookAddress=NetTlsRestoreHookAddress;

    // set options
    if (!this->f_SetHookNetOptions(&this->HookNetOptions))
        return FALSE;

    // init dll MUST BE LAST FUNCTION CALLED because it's release the waiting state of hooknet.dll
    if (!this->f_InitializeHookNet(&InitHookNet))
        return FALSE;

    this->bStarted=TRUE;

    return TRUE;
}

// callback called when hook net is unloading (just before hooknet.dll unload)
BOOL CNET_Manager::ShutDownHookNetDll()
{
    this->bStarted=FALSE;
    this->ResetFunctionsPointer();
    return TRUE;
}

// TCHAR* ModuleAndFuncName : module name (without path) + function name
//          Buffer size should be >= MAX_PATH
// PBYTE* RelativeAddressFromFunctionStart : relative address from function start
BOOL CNET_Manager::GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress(
                        IN PBYTE Address,
                        OUT HMODULE* pCallingModuleHandle,
                        OUT TCHAR* ModuleAndFuncName,
                        OUT PBYTE* RelativeAddressFromFunctionStart)
{
    if (!this->bStarted)
        return FALSE;

    if (!this->f_GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress)
        return FALSE;
    return this->f_GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress(Address,pCallingModuleHandle,ModuleAndFuncName,RelativeAddressFromFunctionStart);
}

BOOL CNET_Manager::StartAutoHooking()
{
    if (!this->bStarted)
    {
        // try to initialize hook net dll
        if (!this->InitializeHookNetDll())
            return FALSE;
    }
    return this->f_StartAutoHooking();
}
BOOL CNET_Manager::StopAutoHooking()
{
    if (!this->bStarted)
        return FALSE;// don't initialize if not already done
    return this->f_StopAutoHooking();
}
BOOL CNET_Manager::SetOptions(HOOK_NET_OPTIONS* pOptions)
{
    if (this->HookNetOptions.EnableFrameworkMonitoring 
        && !pOptions->EnableFrameworkMonitoring)
    {
        NetTlsRestoreAllHookAddress();
    }
    memcpy(&this->HookNetOptions,pOptions,sizeof(HOOK_NET_OPTIONS));

    if (!this->bStarted)
    {
        // try to initialize hook net dll
        if (!this->InitializeHookNetDll())
            return FALSE;
    }

    return this->f_SetHookNetOptions(&this->HookNetOptions);
}
BOOL CNET_Manager::AreEnterLeaveSpied()
{
    return this->HookNetOptions.EnableFrameworkMonitoring;
}

BOOL CNET_Manager::UnHookAllNetMethods()
{
    if (!this->bStarted)
        return FALSE;// don't initialize if not already done
/*
// unhook all auto hooked
    BOOL bFunctionRet;
    API_INFO* pApiInfo;
    pLinkListAPIInfos->Lock();
begin:
    for (pItemAPIInfo =pLinkListAPIInfos->Head;pItemAPIInfo;pItemAPIInfo = pNextItemAPIInfo)
    {
        pApiInfo=(API_INFO*)pItemAPIInfo->ItemData;
        pNextItemAPIInfo = pItemAPIInfo->NextItem; // because pItemAPIInfo can be free by UnHookIfPossible

        if (pApiInfo->HookType==HOOK_TYPE_NET)
        {
            if (_tcscmp(pApiInfo->pMonitoringFileInfos->szFileName,HOOK_NET_AUTO_PARSING)==0)
            {
                pApiInfo->pMonitoringFileInfos=0;

                pLinkListAPIInfos->Unlock();

                // list must be unlocked because lock is needed by UnHookIfPossible
                bFunctionRet = UnHookIfPossible(pApiInfo,TRUE);
                bRet=bRet && bFunctionRet;

                pLinkListAPIInfos->Lock();
                if (!pLinkListAPIInfos->IsItemStillInList(pNextItemAPIInfo,TRUE))
                    goto begin;
            }
        }
    }
    pLinkListAPIInfos->Unlock();

    return TRUE;
*/
    return this->f_UnHookAllNetMethods();
}


// TCHAR* pszFuncDescription like .NET@AssemblyName@functionToken
PBYTE CNET_Manager::GetNetCompiledFunctionAddress(TCHAR* pszFuncDescription)
{
    if (!this->bStarted)
    {
        this->ReportNetHookingMustBeEnableBeforeAppStart();
        return FALSE;
    }

    return this->f_GetNetCompiledFunctionAddress(pszFuncDescription);
}

DWORD CNET_Manager::GetNetCompiledFunctionSize(PBYTE NetCompiledFunctionAddress)
{
    if (!this->bStarted)
    {
        this->ReportNetHookingMustBeEnableBeforeAppStart();
        return FALSE;
    }

    return this->f_GetNetCompiledFunctionSize(NetCompiledFunctionAddress);
}

BOOL CNET_Manager::ShowNetInteraction()
{
    if (!this->bStarted)
    {
        this->ReportNetHookingMustBeEnableBeforeAppStart();
        return FALSE;
    }

    return this->f_ShowNetInteraction();
}

BOOL CNET_Manager::AddHookNetFakingDefinition(FAKING_DLL_INFOS* pFakingDllInfos,
                                                STRUCT_FAKE_API_WITH_USERPARAM* pFakeApiInfos,
                                                TCHAR* pszDllName,
                                                DWORD dwFunctionIndex,
                                                tagFakingDllArray FakingType,
                                                FAKING_DLL_INFOS** ppAlreadyHookingFakingDllInfos,
                                                BOOL* pbParsingError)
{
    if (!this->bStarted)
    {
        this->ReportNetHookingMustBeEnableBeforeAppStart();
        return FALSE;
    }
    if (!this->f_AddHookNetFakingDefinition)
        return FALSE;

    return this->f_AddHookNetFakingDefinition(pFakingDllInfos,pFakeApiInfos,pszDllName,dwFunctionIndex,FakingType,ppAlreadyHookingFakingDllInfos,pbParsingError);
}
BOOL CNET_Manager::AddHookNetMonitoringDefinition(MONITORING_FILE_INFOS* pMonitoringFileInfo,
                                                    TCHAR* pszFullDescription,
                                                    TCHAR* pszFileName,
                                                    DWORD dwLineNumber,
                                                    MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo,
                                                    BOOL* pbParsingError)
{
    if (!this->bStarted)
    {
        this->ReportNetHookingMustBeEnableBeforeAppStart();
        return FALSE;
    }
    if (!this->f_AddHookNetMonitoringDefinition)
        return FALSE;

    return this->f_AddHookNetMonitoringDefinition(pMonitoringFileInfo,pszFullDescription,pszFileName,dwLineNumber,ppAlreadyHookingMonitoringFileInfo,pbParsingError);
}

BOOL CNET_Manager::RemoveHookNetMonitoringDefinition(MONITORING_FILE_INFOS* pMonitoringFileInfo)
{
    if (!this->bStarted)
    {
        // this->ReportNetHookingMustBeEnableBeforeAppStart();
        return FALSE;
    }
    if (!this->f_RemoveHookNetMonitoringDefinition)
        return FALSE;

    return this->f_RemoveHookNetMonitoringDefinition(pMonitoringFileInfo);
}
BOOL CNET_Manager::RemoveHookNetFakingDefinition(FAKING_DLL_INFOS* pFakingDllInfos)
{
    if (!this->bStarted)
    {
        // this->ReportNetHookingMustBeEnableBeforeAppStart();
        return FALSE;
    }
    if (!this->f_RemoveHookNetFakingDefinition)
        return FALSE;

    return this->f_RemoveHookNetFakingDefinition(pFakingDllInfos);
}

BOOL CNET_Manager::Uninitialize()
{
    if (!this->bStarted)
        return TRUE;

    if (!this->f_Uninitialize)
        return FALSE;

    return this->f_Uninitialize();
}
//-----------------------------------------------------------------------------
// Name: ClearUserDataTypeCache
// Object: clear user data type cache in hookNet dll
//         called on each file change in UserTypes directory
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CNET_Manager::ClearUserDataTypeCache()
{
    if(!this->bStarted)
        return FALSE;// don't try to load dll : this function is no use if dll is not loaded
    return this->f_ClearUserDataTypeCache();
}

BOOL CNET_Manager::AddHookNetFromTokenForCompiledFuntions(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM* Param)
{
	if(!this->bStarted)
		return FALSE;// don't try to load dll : this function is no use if dll is not loaded
	return this->f_AddHookNetFromTokenForCompiledFuntions(Param);
}
BOOL CNET_Manager::RemoveHookNetFromTokenForCompiledFuntion(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM* Param)
{
	if(!this->bStarted)
		return FALSE;// don't try to load dll : this function is no use if dll is not loaded
	return this->f_RemoveHookNetFromTokenForCompiledFuntion(Param);
}