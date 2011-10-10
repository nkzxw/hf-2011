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
#include "HookComOptions.h"

#define COM_DEFINITION_PREFIX                   _T("COM@")
#define COM_MONITORING_FILES_RELATIVE_PATH      _T("monitoring files\\COM\\")

#define HOOK_COM_InitializeHookCom_EXPORTED_FUNC_NAME               "_InitializeHookCom@4"
#define HOOK_COM_SetHookComOptions_EXPORTED_FUNC_NAME               "_SetHookComOptions@4"
#define HOOK_COM_StartHookingCreatedCOMObjects_EXPORTED_FUNC_NAME   "_StartHookingCreatedCOMObjects@0"
#define HOOK_COM_StopHookingCreatedCOMObjects_EXPORTED_FUNC_NAME    "_StopHookingCreatedCOMObjects@0"
#define HOOK_COM_AddHookComFakingDefinition_EXPORTED_FUNC_NAME      "_AddHookComFakingDefinition@28"
#define HOOK_COM_AddHookComMonitoringDefinition_EXPORTED_FUNC_NAME  "_AddHookComMonitoringDefinition@24"
#define HOOK_COM_UnHookAllComObjects_EXPORTED_FUNC_NAME             "_UnHookAllComObjects@0"
#define HOOK_COM_AddCOMObjectCreationCallBack_EXPORTED_FUNC_NAME    "_AddCOMObjectCreationCallBack@4"
#define HOOK_COM_RemoveCOMObjectCreationCallBack_EXPORTED_FUNC_NAME "_RemoveCOMObjectCreationCallBack@4"
#define HOOK_COM_ShowMethodsAddress                                 "_ShowMethodsAddress@0"
#define HOOK_COM_ShowMethodsAddressT                                _T("_ShowMethodsAddress@0")
#define HOOK_COM_ShowCOMInteraction_EXPORTED_FUNC_NAME              "_ShowCOMInteraction@0"
#define HOOK_COM_ReleaseCreatedCOMObjectsForStaticHooks_EXPORTED_FUNC_NAME "_ReleaseCreatedCOMObjectsForStaticHooks@0"
#define HOOK_COM_StartAutoHooking_EXPORTED_FUNC_NAME                "_StartAutoHooking@0"
#define HOOK_COM_StopAutoHooking_EXPORTED_FUNC_NAME                 "_StopAutoHooking@0"
#define HOOK_COM_PrepareDllUnload_EXPORTED_FUNC_NAME                "_PrepareDllUnload@0"
#define HOOK_COM_ClearUserDataTypeCache_EXPORTED_FUNC_NAME          "_ClearUserDataTypeCache@0"

typedef struct tagHookComInit  
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
}HOOK_COM_INIT,*PINIT_HOOK_COM;


enum tagHookFakingType
{
    HOOK_FAKING_TYPE_FAKE=FAKING_DLL_ARRAY_FAKING,
    HOOK_FAKING_TYPE_PRE_API_CALL=FAKING_DLL_ARRAY_PRE_HOOK,
    HOOK_FAKING_TYPE_POST_API_CALL=FAKING_DLL_ARRAY_POST_HOOK
};


enum tagHookType
{
    HOOK_TYPE_FAKING=HOOK_FAKING_TYPE_FAKE,
    HOOK_TYPE_PRE_API_CALL=HOOK_FAKING_TYPE_PRE_API_CALL,
    HOOK_TYPE_POST_API_CALL=HOOK_FAKING_TYPE_POST_API_CALL,
    HOOK_TYPE_MONITORING
};


///////////////////////////////////////////
// exported HookCom dll functions
///////////////////////////////////////////
typedef BOOL (__stdcall *pfInitializeHookCom)(HOOK_COM_INIT* pInitHookCom);
typedef BOOL (__stdcall *pfUnHookAllComObjects)();
typedef BOOL (__stdcall *pfSetHookComOptions)(HOOK_COM_OPTIONS* pHookComOptions);
typedef BOOL (__stdcall *pfStartHookingCreatedCOMObjects)();
typedef BOOL (__stdcall *pfStopHookingCreatedCOMObjects)();
typedef BOOL (__stdcall *pfAddHookComFakingDefinition)(FAKING_DLL_INFOS* pFakingDllInfos,
                                                        STRUCT_FAKE_API_WITH_USERPARAM* pFakeApiInfos,
                                                        TCHAR* pszDllName,
                                                        DWORD dwFunctionIndex,
                                                        tagFakingDllArray FakingType,
                                                        FAKING_DLL_INFOS** ppAlreadyHookingFakingDllInfos,
                                                        BOOL* pbParsingError);
typedef BOOL (__stdcall *pfAddHookComMonitoringDefinition)(MONITORING_FILE_INFOS* pMonitoringFileInfo,
                                              TCHAR* pszFullDescription,
                                              TCHAR* pszFileName,
                                              DWORD dwLineNumber,
                                              MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo,
                                              BOOL* pbParsingError);

typedef BOOL (__stdcall *pfRemoveCOMObjectCreationCallBack)(pfCOMObjectCreationCallBack pCOMObjectCreationCallBack);
typedef BOOL (__stdcall *pfAddCOMObjectCreationCallBack)(pfCOMObjectCreationCallBack pCOMObjectCreationCallBack);
typedef BOOL (__stdcall *pfHookComShowMethodsAddress)();
typedef BOOL (__stdcall *pfShowCOMInteraction)();
typedef BOOL (__stdcall *pfReleaseCreatedCOMObjectsForStaticHooks)();
typedef BOOL (__stdcall *pfStartAutoHooking)();
typedef BOOL (__stdcall *pfStopAutoHooking)();
typedef BOOL (__stdcall *pfPrepareDllUnload)();
typedef BOOL (__stdcall *pfClearUserDataTypeCache)();