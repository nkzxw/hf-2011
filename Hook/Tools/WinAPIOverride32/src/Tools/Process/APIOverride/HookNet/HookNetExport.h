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
// Object: defines exported struct, defines and functions
//-----------------------------------------------------------------------------

#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif

#include "../injected_dll/APIOverrideDllExport.h"
#include "HookNetOptions.h"

// if you change CLSID_NET_PROFILER_GUID you have to change CLSID_NET_PROFILER
// in HookNet.dll project, file ComStuff.cpp
#define CLSID_NET_PROFILER_GUID _T("{52AE91FC-569A-496f-A268-74D62B866D73}")

#define NET_MONITORING_FILES_RELATIVE_PATH      _T("monitoring files\\NET\\")
#define HOOK_NET_AUTO_PARSING                   _T("HookNet.dll IMetaDataImport parsing")

#define HOOK_NET_InitializeHookNet_EXPORTED_FUNC_NAME               "_InitializeHookNet@4"
#define HOOK_NET_SetHookNetOptions_EXPORTED_FUNC_NAME               "_SetHookNetOptions@4"
#define HOOK_NET_UnHookAllNetObjects_EXPORTED_FUNC_NAME             "_UnHookAllNetObjects@0"
#define HOOK_NET_ShowMethodsAddress                                 "_ShowMethodsAddress@0"
#define HOOK_NET_GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress "_GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress@16"
#define HOOK_NET_StartAutoHooking_EXPORTED_FUNC_NAME                "_StartAutoHooking@0"
#define HOOK_NET_StopAutoHooking_EXPORTED_FUNC_NAME                 "_StopAutoHooking@0"
#define HOOK_NET_UnHookAllNetMethods_EXPORTED_FUNC_NAME             "_UnHookAllNetMethods@0"
#define HOOK_NET_GetNetCompiledFunctionAddress_EXPORTED_FUNC_NAME   "_GetNetCompiledFunctionAddress@4"
#define HOOK_NET_GetNetCompiledFunctionSize_EXPORTED_FUNC_NAME      "_GetNetCompiledFunctionSize@4"
#define HOOK_NET_AddHookNetFakingDefinition_EXPORTED_FUNC_NAME      "_AddHookNetFakingDefinition@28"
#define HOOK_NET_AddHookNetMonitoringDefinition_EXPORTED_FUNC_NAME  "_AddHookNetMonitoringDefinition@24"
#define HOOK_NET_RemoveHookNetFakingDefinition_EXPORTED_FUNC_NAME   "_RemoveHookNetFakingDefinition@4"
#define HOOK_NET_RemoveHookNetMonitoringDefinition_EXPORTED_FUNC_NAME "_RemoveHookNetMonitoringDefinition@4"
#define HOOK_NET_Uninitialize_EXPORTED_FUNC_NAME                    "_Uninitialize@0"
#define HOOK_NET_ShowNetInteraction_EXPORTED_FUNC_NAME              "_ShowNetInteraction@0"
#define HOOK_NET_ClearUserDataTypeCache_EXPORTED_FUNC_NAME          "_ClearUserDataTypeCache@0"
#define HOOK_NET_AddHookNetFromTokenForCompiledFuntions_EXPORTED_FUNC_NAME 	  "_AddHookNetFromTokenForJittedFuntions@4"
#define HOOK_NET_RemoveHookNetFromTokenForCompiledFuntions_EXPORTED_FUNC_NAME "_RemoveHookNetFromTokenForJittedFuntions@4"

typedef struct tagHookNetInit
{
    HANDLE hevtFreeProcess;// event set when user request to stop monitoring / overriding a process
    // functions of APIOverride dll used by HookCom dll
    pfHookAPIFunction HookAPIFunction;
    pfGetAssociatedItemAPIInfo GetAssociatedItemAPIInfo;
    pfInitializeApiInfo InitializeApiInfo;
    pfFreeApiInfoItem FreeApiInfoItem;
    pfParseFunctionDescription ParseFunctionDescription;
    pfParseParameters ParseParameters;
    pfParseOptions ParseOptions;
    pfDynamicMessageBoxInDefaultStation DynamicMessageBoxInDefaultStation;
    pfCreateParameterConditionalLogContentListIfDoesntExist CreateParameterConditionalLogContentListIfDoesntExist;
    pfCreateParameterConditionalBreakContentListIfDoesntExist CreateParameterConditionalBreakContentListIfDoesntExist;
    pfAddPostApiCallCallBack AddPostApiCallCallBack;
    pfRemovePostApiCallCallBack RemovePostApiCallCallBack;
    pfAddPreApiCallCallBack AddPreApiCallCallBack;
    pfRemovePreApiCallCallBack RemovePreApiCallCallBack;
    pfUnHookIfPossible UnHookIfPossible;
    pfGetWinAPIOverrideFunctionDescriptionAddress GetWinAPIOverrideFunctionDescriptionAddress;
    pfReportMessage ReportMessage;
    pfGetModuleNameAndRelativeAddressFromCallerAbsoluteAddress GetModuleNameAndRelativeAddressFromCallerAbsoluteAddress;
    pfSetDefaultStation SetDefaultStation;
    pfRestoreStation RestoreStation;
    pfCanWindowInteract CanWindowInteract;
    pfAdjustThreadSecurityAndLaunchDialogThread AdjustThreadSecurityAndLaunchDialogThread;
    pfProcessInternalCallRequestEx ProcessInternalCallRequestEx;
    pfQueryEmptyItemAPIInfo QueryEmptyItemAPIInfo;
    pfFreeOptionalParametersMemory FreeOptionalParametersMemory;
    pfNetExceptionCatcherEnterCallBack NetExceptionCatcherEnterCallBack;
    pfNetExceptionSearchFunctionLeaveCallBack NetExceptionSearchFunctionLeaveCallBack;
    pfNetTlsRestoreHookAddress NetTlsRestoreHookAddress;
}HOOK_NET_INIT,*PHOOK_NET_INIT;


typedef struct _HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM
{
	TCHAR AssemblyName[MAX_PATH];
	DWORD TokensNumber; // let DWORD : better for 32 - 64 bit cross injection
	// mdToken FunctionTokenArray[];
	ULONG32* TokenArray;
}HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM,*PHOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM;
///////////////////////////////////////////
// exported HookCom dll functions
///////////////////////////////////////////
typedef BOOL (__stdcall *pfInitializeHookNet)(HOOK_NET_INIT* pInitHookNet);
typedef BOOL (__stdcall *pfUnHookAllNetMethods)();
typedef BOOL (__stdcall *pfSetHookNetOptions)(HOOK_NET_OPTIONS* pHookNetOptions);
typedef BOOL (__stdcall *pfNETGetModuleNameAndRelativeAddressFromCallerAbsoluteAddress)(
                                                                                        IN PBYTE Address,
                                                                                        OUT HMODULE* pCallingModuleHandle,
                                                                                        OUT TCHAR* ModuleAndFuncName,
                                                                                        OUT PBYTE* RelativeAddressFromFunctionStart);
typedef BOOL (__stdcall *pfNETStartAutoHooking)();
typedef BOOL (__stdcall *pfNETStopAutoHooking)();
typedef BOOL (__stdcall *pfUnHookAllNetMethods)();
typedef PBYTE (__stdcall *pfGetNetCompiledFunctionAddress)(TCHAR* pszFuncDescription);
typedef DWORD (__stdcall *pfGetNetCompiledFunctionSize)(PBYTE NetCompiledFunctionAddress);

typedef BOOL (__stdcall *pfAddHookNetFakingDefinition)(FAKING_DLL_INFOS* pFakingDllInfos,
                                                       STRUCT_FAKE_API_WITH_USERPARAM* pFakeApiInfos,
                                                       TCHAR* pszDllName,
                                                       DWORD dwFunctionIndex,
                                                       tagFakingDllArray FakingType,
                                                       FAKING_DLL_INFOS** ppAlreadyHookingFakingDllInfos,
                                                       BOOL* pbParsingError);
typedef BOOL (__stdcall *pfAddHookNetMonitoringDefinition)(MONITORING_FILE_INFOS* pMonitoringFileInfo,
                                                           TCHAR* pszFullDescription,
                                                           TCHAR* pszFileName,
                                                           DWORD dwLineNumber,
                                                           MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo,
                                                           BOOL* pbParsingError);
typedef BOOL (__stdcall *pfRemoveHookNetFakingDefinition)(FAKING_DLL_INFOS* pFakingDllInfos);
typedef BOOL (__stdcall *pfRemoveHookNetMonitoringDefinition)(MONITORING_FILE_INFOS* pMonitoringFileInfo);

typedef BOOL (__stdcall *pfHookNetShowMethodsAddress)();
typedef BOOL (__stdcall *pfShowNetInteraction)();
typedef BOOL (__stdcall *pfAddHookNetFromTokenForCompiledFuntions)(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM*);
typedef BOOL (__stdcall *pfRemoveHookNetFromTokenForCompiledFuntion)(HOOK_NET_ADD_OR_REMOVE_HOOK_FROM_TOKEN_FOR_COMPILED_FUNTIONS_PARAM*);
typedef BOOL (__stdcall *pfUninitialize)();
typedef BOOL (__stdcall *pfClearUserDataTypeCache)();