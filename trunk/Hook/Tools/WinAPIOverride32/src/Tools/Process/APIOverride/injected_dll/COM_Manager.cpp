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
// Object: manages hook com dll
//         wrapper for calling hookcom.dll functions
//-----------------------------------------------------------------------------

#include "COM_Manager.h"

extern HANDLE hevtFreeProcess;
extern HINSTANCE DllhInstance;
extern BOOL __stdcall GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress(IN PBYTE pOriginAddress,
                                                                               OUT HMODULE* pCallingModuleHandle,
                                                                               OUT TCHAR* pszModuleName,
                                                                               OUT PBYTE* pRelativeAddress,
                                                                               OUT BOOL* pbShouldLog,
                                                                               BOOL TakeAnotherSnapshotIfNotFound,
                                                                               BOOL ContinueEvenIfShouldNotBeLogged);

//-----------------------------------------------------------------------------
// Name: ResetFunctionsPointer
// Object: empty hookcom dll functions pointer
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
void CCOM_Manager::ResetFunctionsPointer()
{
    this->f_InitializeHookCom=NULL;
    this->f_UnHookAllComObjects=NULL;
    this->f_SetHookComOptions=NULL;
    this->f_StartHookingCreatedCOMObjects=NULL;
    this->f_StopHookingCreatedCOMObjects=NULL;
    this->f_AddHookComFakingDefinition=NULL;
    this->f_AddHookComMonitoringDefinition=NULL;
    this->f_RemoveCOMObjectCreationCallBack=NULL;
    this->f_AddCOMObjectCreationCallBack=NULL;
    this->f_ReleaseCreatedCOMObjectsForStaticHooks=NULL;
    this->f_StartAutoHooking=NULL;
    this->f_StopAutoHooking=NULL;
    this->f_PrepareDllUnload=NULL;
    this->f_ClearUserDataTypeCache=NULL;
}
CCOM_Manager::CCOM_Manager()
{
    this->ResetFunctionsPointer();
    this->HookComDllModule=NULL;
    this->bStarted=FALSE;
    memset(&this->HookComOptions,0,sizeof(HOOK_COM_OPTIONS));

    // get current dll path
    CStdFileOperations::GetModulePath(DllhInstance,this->HookComDllPath,MAX_PATH);

    // forge hook com dll full path
    _tcscat(this->HookComDllPath,HOOK_COM_DLL_NAME);
    
}
CCOM_Manager::~CCOM_Manager()
{
    this->Stop();
}

//-----------------------------------------------------------------------------
// Name: Start
// Object: load hook com dll and initialize it providing function pointer to current dll functions
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::Start()
{
    if (this->bStarted)
        return TRUE;

    if (!this->HookComDllModule)
        this->HookComDllModule=LoadLibrary(this->HookComDllPath);

    if (!this->HookComDllModule)
        return FALSE;

    // signal library as an internal module
    AddAPIOverrideInternalModule(this->HookComDllModule);

    this->f_InitializeHookCom=(pfInitializeHookCom)GetProcAddress(this->HookComDllModule,HOOK_COM_InitializeHookCom_EXPORTED_FUNC_NAME);
    this->f_UnHookAllComObjects=(pfUnHookAllComObjects)GetProcAddress(this->HookComDllModule,HOOK_COM_UnHookAllComObjects_EXPORTED_FUNC_NAME);
    this->f_SetHookComOptions=(pfSetHookComOptions)GetProcAddress(this->HookComDllModule,HOOK_COM_SetHookComOptions_EXPORTED_FUNC_NAME);
    this->f_StartHookingCreatedCOMObjects=(pfStartHookingCreatedCOMObjects)GetProcAddress(this->HookComDllModule,HOOK_COM_StartHookingCreatedCOMObjects_EXPORTED_FUNC_NAME);
    this->f_StopHookingCreatedCOMObjects=(pfStopHookingCreatedCOMObjects)GetProcAddress(this->HookComDllModule,HOOK_COM_StopHookingCreatedCOMObjects_EXPORTED_FUNC_NAME);
    this->f_AddHookComFakingDefinition=(pfAddHookComFakingDefinition)GetProcAddress(this->HookComDllModule,HOOK_COM_AddHookComFakingDefinition_EXPORTED_FUNC_NAME);
    this->f_AddHookComMonitoringDefinition=(pfAddHookComMonitoringDefinition)GetProcAddress(this->HookComDllModule,HOOK_COM_AddHookComMonitoringDefinition_EXPORTED_FUNC_NAME);
    this->f_RemoveCOMObjectCreationCallBack=(pfRemoveCOMObjectCreationCallBack)GetProcAddress(this->HookComDllModule,HOOK_COM_RemoveCOMObjectCreationCallBack_EXPORTED_FUNC_NAME);
    this->f_AddCOMObjectCreationCallBack=(pfAddCOMObjectCreationCallBack)GetProcAddress(this->HookComDllModule,HOOK_COM_AddCOMObjectCreationCallBack_EXPORTED_FUNC_NAME);
    this->f_ShowCOMInteraction=(pfShowCOMInteraction)GetProcAddress(this->HookComDllModule,HOOK_COM_ShowCOMInteraction_EXPORTED_FUNC_NAME);
    this->f_ReleaseCreatedCOMObjectsForStaticHooks=(pfReleaseCreatedCOMObjectsForStaticHooks)GetProcAddress(this->HookComDllModule,HOOK_COM_ReleaseCreatedCOMObjectsForStaticHooks_EXPORTED_FUNC_NAME);
    this->f_StartAutoHooking=(pfStartAutoHooking)GetProcAddress(this->HookComDllModule,HOOK_COM_StartAutoHooking_EXPORTED_FUNC_NAME);
    this->f_StopAutoHooking=(pfStopAutoHooking)GetProcAddress(this->HookComDllModule,HOOK_COM_StopAutoHooking_EXPORTED_FUNC_NAME);
    this->f_PrepareDllUnload=(pfPrepareDllUnload)GetProcAddress(this->HookComDllModule,HOOK_COM_PrepareDllUnload_EXPORTED_FUNC_NAME);
    this->f_ClearUserDataTypeCache=(pfClearUserDataTypeCache)GetProcAddress(this->HookComDllModule,HOOK_COM_ClearUserDataTypeCache_EXPORTED_FUNC_NAME);

    if (  (!this->f_InitializeHookCom)
        ||(!this->f_UnHookAllComObjects)
        ||(!this->f_SetHookComOptions)
        ||(!this->f_StartHookingCreatedCOMObjects)
        ||(!this->f_StopHookingCreatedCOMObjects)
        ||(!this->f_AddHookComFakingDefinition)
        ||(!this->f_AddHookComMonitoringDefinition)
        ||(!this->f_RemoveCOMObjectCreationCallBack)
        ||(!this->f_AddCOMObjectCreationCallBack)
        ||(!this->f_ShowCOMInteraction)
        ||(!this->f_ReleaseCreatedCOMObjectsForStaticHooks)
        ||(!this->f_StartAutoHooking)
        ||(!this->f_StopAutoHooking)
        ||(!this->f_PrepareDllUnload)
        ||(!this->f_ClearUserDataTypeCache)
        )
    {
#ifdef _DEBUG
        OutputDebugString(_T("ERROR LOADING HOOKCOM.DLL check if dll exists and its exports"));
#endif
        return FALSE;
    }

    HOOK_COM_INIT InitHookCom;

    InitHookCom.hevtFreeProcess=hevtFreeProcess;

    // get functions handlers
    InitHookCom.DynamicMessageBoxInDefaultStation=DynamicMessageBoxInDefaultStation;
    InitHookCom.FreeApiInfoItem=FreeApiInfoItem;
    InitHookCom.GetAssociatedItemAPIInfo=GetAssociatedItemAPIInfo;
    InitHookCom.HookAPIFunction=HookAPIFunction;
    InitHookCom.InitializeApiInfo=InitializeApiInfo;
    InitHookCom.ParseFunctionDescription=ParseFunctionDescription;
    InitHookCom.ParseOptions=ParseOptions;
    InitHookCom.ParseParameters=ParseParameters;
    InitHookCom.CreateParameterConditionalBreakContentListIfDoesntExist=CreateParameterConditionalBreakContentListIfDoesntExist;
    InitHookCom.CreateParameterConditionalLogContentListIfDoesntExist=CreateParameterConditionalLogContentListIfDoesntExist;
    InitHookCom.AddPostApiCallCallBack=AddPostApiCallCallBack;
    InitHookCom.RemovePostApiCallCallBack=RemovePostApiCallCallBack;
    InitHookCom.AddPreApiCallCallBack=AddPreApiCallCallBack;
    InitHookCom.RemovePreApiCallCallBack=RemovePreApiCallCallBack;
    InitHookCom.UnHookIfPossible=UnHookIfPossible;
    InitHookCom.ReportMessage=CReportMessage::ReportMessage;
    InitHookCom.GetWinAPIOverrideFunctionDescriptionAddress=GetWinAPIOverrideFunctionDescriptionAddress;
    InitHookCom.GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress=GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress;
    InitHookCom.SetDefaultStation=CDialogInterfaceManager::SetDefaultStation;
    InitHookCom.RestoreStation=CDialogInterfaceManager::RestoreStation;
    InitHookCom.CanWindowInteract=CDialogInterfaceManager::CanWindowInteract;
    InitHookCom.AdjustThreadSecurityAndLaunchDialogThread=CDialogInterfaceManager::AdjustThreadSecurityAndLaunchDialogThread;
    InitHookCom.ProcessInternalCallRequestEx=CProcessInternalCallRequest::ProcessInternalCallRequestEx;


    // init dll
    if (!this->f_InitializeHookCom(&InitHookCom))
        return FALSE;

    // init options
    if (!this->f_SetHookComOptions(&this->HookComOptions))
        return FALSE;

    this->bStarted=TRUE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Stop
// Object: unload hook com dll
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::Stop()
{
    if(!this->bStarted)
        return TRUE;

    if(!this->StopAutoHooking())
        return FALSE;
    
    // stop to watch for new created com objects
    if(!this->StopHookingCreatedCOMObjects())
        return FALSE;

    // unhook all com objects
    if (!this->UnHookAllComObjects())
        return FALSE;

    // free hookcom module
    if (this->HookComDllModule)
    {
        // cleanest way than calling FreeLibrary directly
        // we can wait for our threads to finish (and so avoid potential deadlock)
        this->f_PrepareDllUnload();

        if (!FreeLibrary(this->HookComDllModule))
            return FALSE;
        this->HookComDllModule=NULL;
    }

    // remove the filter module informations
    RemoveAPIOverrideInternalModule(this->HookComDllModule);

    // reset values
    this->ResetFunctionsPointer();

    this->bStarted=FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetOptions
// Object: update COM options
// Parameters :
//     in : HOOK_COM_OPTIONS* pHookComOptions
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::SetOptions(HOOK_COM_OPTIONS* pHookComOptions)
{
    BOOL bRet=TRUE;
    TCHAR* psz=_tcsdup(this->HookComOptions.pszConfigFileComObjectCreationHookedFunctions);
    memcpy(&this->HookComOptions,pHookComOptions,sizeof(HOOK_COM_OPTIONS));

    // if com hooking is not started
    if(!this->bStarted)
    {
        free(psz);
        return FALSE;
    }

    // set option
    if (!this->f_SetHookComOptions(&this->HookComOptions))
    {
        free(psz);
        return FALSE;
    }

    // check if config file of api com object creation has changed, 
    // (as f_SetHookComOptions return TRUE, that means that config file exist)
    if(_tcsicmp(psz,this->HookComOptions.pszConfigFileComObjectCreationHookedFunctions)!=0)
    {
        // if config file has changed load the new one
        bRet=this->StartHookingCreatedCOMObjects();
    }
    free(psz);

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: GetHookComDllModuleHandle
// Object: get module handle of hook com dll
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
HMODULE CCOM_Manager::GetHookComDllModuleHandle()
{
    return GetModuleHandle(this->HookComDllPath);
}

//-----------------------------------------------------------------------------
// Name: StartHookingCreatedCOMObjects
// Object: start spying created COM object 
//         for com auto hooking and COM objects creation callback
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::StartHookingCreatedCOMObjects()
{
    // load library if needed
    if(!this->bStarted)
    {
        if(!this->Start())
            return FALSE;
    }

    if (!this->f_StartHookingCreatedCOMObjects)
        return FALSE;

    return this->f_StartHookingCreatedCOMObjects();
}

//-----------------------------------------------------------------------------
// Name: StopHookingCreatedCOMObjects
// Object: stop spying created COM object 
//         affect com auto hooking and COM objects creation callback
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::StopHookingCreatedCOMObjects()
{
    if(!this->bStarted)
        return FALSE;

    if (!this->f_StopHookingCreatedCOMObjects)
        return FALSE;
    return this->f_StopHookingCreatedCOMObjects();
}

//-----------------------------------------------------------------------------
// Name: UnHookAllComObjects
// Object: unhook auto hooked COM objects
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::UnHookAllComObjects()
{
    if(!this->bStarted)
        return FALSE;

    if (!this->f_UnHookAllComObjects)
        return FALSE;

    return this->f_UnHookAllComObjects();
}

//-----------------------------------------------------------------------------
// Name: AddHookComFakingDefinition
// Object: add COM static faking hook
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::AddHookComFakingDefinition(FAKING_DLL_INFOS* pFakingDllInfos,
                                                STRUCT_FAKE_API_WITH_USERPARAM* pFakeApiInfos,
                                                TCHAR* pszDllName,
                                                DWORD dwFunctionIndex,
                                                tagFakingDllArray FakingType,
                                                FAKING_DLL_INFOS** ppAlreadyHookingFakingDllInfos,
                                                BOOL* pbParsingError)
{
    // load library if needed
    if(!this->bStarted)
        if(!this->Start())
            return FALSE;

    if (!this->f_AddHookComFakingDefinition)
        return FALSE;

    return this->f_AddHookComFakingDefinition(
                    pFakingDllInfos,
                    pFakeApiInfos,
                    pszDllName,
                    dwFunctionIndex,
                    FakingType,
                    ppAlreadyHookingFakingDllInfos,
                    pbParsingError
                    );
}

//-----------------------------------------------------------------------------
// Name: AddHookComMonitoringDefinition
// Object: add COM static monitoring hook
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::AddHookComMonitoringDefinition(MONITORING_FILE_INFOS* pMonitoringFileInfo,
                                    TCHAR* pszFullDescription,
                                    TCHAR* pszFileName,
                                    DWORD dwLineNumber,
                                    MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo,
                                    BOOL* pbParsingError)
{
    // load library if needed
    if(!this->bStarted)
        if(!this->Start())
            return FALSE;

    if (!this->f_AddHookComMonitoringDefinition)
        return FALSE;

    return this->f_AddHookComMonitoringDefinition(
                    pMonitoringFileInfo,
                    pszFullDescription,
                    pszFileName,
                    dwLineNumber,
                    ppAlreadyHookingMonitoringFileInfo,
                    pbParsingError);
}

//-----------------------------------------------------------------------------
// Name: RemoveCOMObjectCreationCallBack
// Object: remove an installed callback for COM object creation
// Parameters :
//     in : pfCOMObjectCreationCallBack pCOMObjectCreationCallBack : installed callback to remove
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::RemoveCOMObjectCreationCallBack(pfCOMObjectCreationCallBack pCOMObjectCreationCallBack)
{
    // load library if needed
    if(!this->bStarted)
        return FALSE;

    if (pCOMObjectCreationCallBack==NULL)
        return TRUE;

    if(!this->bStarted)
        return FALSE;
    if (!this->f_RemoveCOMObjectCreationCallBack)
        return FALSE;

    return this->f_RemoveCOMObjectCreationCallBack(pCOMObjectCreationCallBack);
}

//-----------------------------------------------------------------------------
// Name: AddCOMObjectCreationCallBack
// Object: install a callback for COM object creation
// Parameters :
//     in : pfCOMObjectCreationCallBack pCOMObjectCreationCallBack : callback to install
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::AddCOMObjectCreationCallBack(pfCOMObjectCreationCallBack pCOMObjectCreationCallBack)
{
    // load library if needed
    if(!this->bStarted)
        if(!this->Start())
            return FALSE;

    if (pCOMObjectCreationCallBack==NULL)
        return TRUE;

    // load library if needed
    if(!this->bStarted)
        if(!this->Start())
            return FALSE;

    if (!this->f_AddCOMObjectCreationCallBack)
        return FALSE;

    return this->f_AddCOMObjectCreationCallBack(pCOMObjectCreationCallBack);
}

//-----------------------------------------------------------------------------
// Name: ShowCOMInteraction
// Object: display COM interaction dialog
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::ShowCOMInteraction()
{
    // load library if needed
    if(!this->bStarted)
        if(!this->Start())
            return FALSE;

    if (!this->f_ShowCOMInteraction)
        return FALSE;

    return this->f_ShowCOMInteraction();
}

//-----------------------------------------------------------------------------
// Name: ReleaseCreatedCOMObjectsForStaticHooks
// Object: destroy com object created for static hooking
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::ReleaseCreatedCOMObjectsForStaticHooks()
{
    if(!this->bStarted)
        return FALSE;

    if (!this->f_ReleaseCreatedCOMObjectsForStaticHooks)
        return FALSE;

    return this->f_ReleaseCreatedCOMObjectsForStaticHooks();
}
//-----------------------------------------------------------------------------
// Name: StartAutoHooking
// Object: start com auto hooking
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::StartAutoHooking()
{
    if(!this->bStarted)
        if(!this->Start())
            return FALSE;

    if (!this->f_StartAutoHooking)
        return FALSE;

    return this->f_StartAutoHooking();
}

//-----------------------------------------------------------------------------
// Name: StopAutoHooking
// Object: stop com auto hooking
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::StopAutoHooking()
{
    if(!this->bStarted)
        return FALSE;

    if (!this->f_StopAutoHooking)
        return FALSE;

    return this->f_StopAutoHooking();
}

//-----------------------------------------------------------------------------
// Name: ClearUserDataTypeCache
// Object: clear user data type cache in hookCom dll
//         called on each file change in UserTypes directory
// Parameters :
//     in : 
// Return : 
//-----------------------------------------------------------------------------
BOOL CCOM_Manager::ClearUserDataTypeCache()
{
    if(!this->bStarted)
        return FALSE;// don't try to load dll : this function is no use is dll is not loaded
    return this->f_ClearUserDataTypeCache();
}