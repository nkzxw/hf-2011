#pragma once

#define HOOK_NET_DLL_NAME _T("HookNet.dll")

#include "../HookNet/HookNetExport.h"
#include "FakeApiLoader.h"
#include "apimonitoringfileloader.h"

class CNET_Manager
{
private:
    HOOK_NET_OPTIONS HookNetOptions;
    HMODULE HookNetDllModule;
    BOOL bStarted;
    BOOL bIsNetApplication;

    pfInitializeHookNet                 f_InitializeHookNet;
    pfSetHookNetOptions                 f_SetHookNetOptions;
    pfNETGetModuleNameAndRelativeAddressFromCallerAbsoluteAddress f_GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress;
    pfNETStartAutoHooking               f_StartAutoHooking;
    pfNETStopAutoHooking                f_StopAutoHooking;
    pfUnHookAllNetMethods               f_UnHookAllNetMethods;
    pfGetNetCompiledFunctionAddress     f_GetNetCompiledFunctionAddress;
    pfAddHookNetFakingDefinition        f_AddHookNetFakingDefinition;
    pfAddHookNetMonitoringDefinition    f_AddHookNetMonitoringDefinition;
    pfRemoveHookNetFakingDefinition     f_RemoveHookNetFakingDefinition;
    pfRemoveHookNetMonitoringDefinition f_RemoveHookNetMonitoringDefinition;
    pfUninitialize                      f_Uninitialize;
    pfGetNetCompiledFunctionSize        f_GetNetCompiledFunctionSize;
    pfShowNetInteraction                f_ShowNetInteraction;
    pfClearUserDataTypeCache            f_ClearUserDataTypeCache;
	pfAddHookNetFromTokenForCompiledFuntions   f_AddHookNetFromTokenForCompiledFuntions;
	pfRemoveHookNetFromTokenForCompiledFuntion f_RemoveHookNetFromTokenForCompiledFuntion;

    BOOL ResetFunctionsPointer();
    void ReportNetHookingMustBeEnableBeforeAppStart();
    BOOL Uninitialize();// called by apioverride.dll to inform hooknet.dll of apioverride.dll unload (hooknet.dll must not use apioverride.dll exported functions anymore)
public:
    CNET_Manager(void);
    ~CNET_Manager(void);
    BOOL InitializeHookNetDll(); // called to initialize Hooknet.dll 
                                    // - called by hooknet.dll itself if apioverride.dll is loaded before hooknet.dll
                                    // - called by apioverride.dll if apioverride.dll is loaded after hooknet.dll
    BOOL ShutDownHookNetDll(); // called by hooknet.dll to inform apioverride.dll that hooknet.dll is going to be unloaded
    BOOL GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress(
                                                                IN PBYTE Address,
                                                                OUT HMODULE* pCallingModuleHandle,
                                                                OUT TCHAR* ModuleAndFuncName,
                                                                OUT PBYTE* RelativeAddressFromFunctionStart);
    BOOL StartAutoHooking();
    BOOL StopAutoHooking();
    BOOL SetOptions(HOOK_NET_OPTIONS* pOptions);
    BOOL UnHookAllNetMethods();
    BOOL ShowNetInteraction();
    PBYTE GetNetCompiledFunctionAddress(TCHAR* pszFuncDescription);
    DWORD GetNetCompiledFunctionSize(PBYTE NetCompiledFunctionAddress);
    BOOL AddHookNetFakingDefinition(FAKING_DLL_INFOS* pFakingDllInfos,
                                    STRUCT_FAKE_API_WITH_USERPARAM* pFakeApiInfos,
                                    TCHAR* pszDllName,
                                    DWORD dwFunctionIndex,
                                    tagFakingDllArray FakingType,
                                    FAKING_DLL_INFOS** ppAlreadyHookingFakingDllInfos,
                                    BOOL* pbParsingError);
    BOOL RemoveHookNetFakingDefinition(FAKING_DLL_INFOS* pFakingDllInfos);
    BOOL AddHookNetMonitoringDefinition(MONITORING_FILE_INFOS* pMonitoringFileInfo,
                                        TCHAR* pszFullDescription,
                                        TCHAR* pszFileName,
                                        DWORD dwLineNumber,
                                        MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo,
                                        BOOL* pbParsingError);
    BOOL RemoveHookNetMonitoringDefinition(MONITORING_FILE_INFOS* pMonitoringFileInfo);
    BOOL AreEnterLeaveSpied();
    BOOL ClearUserDataTypeCache();
	BOOL AddHookNetFromTokenForCompiledFuntions(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM*);
	BOOL RemoveHookNetFromTokenForCompiledFuntion(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM*);
};