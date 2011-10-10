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

#pragma once

#define HOOK_COM_DLL_NAME _T("HookCom.dll")

#include "../HookCom/HookComExport.h"
#include "FakeApiLoader.h"
#include "ModuleFilters.h"
#include "ReportMessage.h"
#include "ProcessInternalCallRequest.h"
#include "../../../File/StdFileOperations.h"

class CCOM_Manager
{
private:
    TCHAR HookComDllPath[MAX_PATH];
    HOOK_COM_OPTIONS HookComOptions;
    HMODULE HookComDllModule;
    BOOL bStarted;

    pfInitializeHookCom                     f_InitializeHookCom;
    pfUnHookAllComObjects                   f_UnHookAllComObjects;
    pfSetHookComOptions                     f_SetHookComOptions;
    pfStartHookingCreatedCOMObjects         f_StartHookingCreatedCOMObjects;
    pfStopHookingCreatedCOMObjects          f_StopHookingCreatedCOMObjects;
    pfAddHookComFakingDefinition            f_AddHookComFakingDefinition;
    pfAddHookComMonitoringDefinition        f_AddHookComMonitoringDefinition;
    pfRemoveCOMObjectCreationCallBack       f_RemoveCOMObjectCreationCallBack;
    pfAddCOMObjectCreationCallBack          f_AddCOMObjectCreationCallBack;
    pfShowCOMInteraction                    f_ShowCOMInteraction;
    pfReleaseCreatedCOMObjectsForStaticHooks f_ReleaseCreatedCOMObjectsForStaticHooks;
    pfStartAutoHooking                      f_StartAutoHooking;
    pfStartAutoHooking                      f_StopAutoHooking;
    pfPrepareDllUnload                      f_PrepareDllUnload;
    pfClearUserDataTypeCache                f_ClearUserDataTypeCache;

    void ResetFunctionsPointer();
public:
    CCOM_Manager();
    ~CCOM_Manager();
    BOOL SetOptions(HOOK_COM_OPTIONS* pHookComOptions);
    BOOL ShowCOMInteraction();
    BOOL StartAutoHooking();
    BOOL StopAutoHooking();
    BOOL StartHookingCreatedCOMObjects();
    BOOL StopHookingCreatedCOMObjects();
    BOOL UnHookAllComObjects();
    HMODULE GetHookComDllModuleHandle();
    BOOL AddHookComFakingDefinition(FAKING_DLL_INFOS* pFakingDllInfos,
                                    STRUCT_FAKE_API_WITH_USERPARAM* pFakeApiInfos,
                                    TCHAR* pszDllName,
                                    DWORD dwFunctionIndex,
                                    tagFakingDllArray FakingType,
                                    FAKING_DLL_INFOS** ppAlreadyHookingFakingDllInfos,
                                    BOOL* pbParsingError);
    BOOL AddHookComMonitoringDefinition(MONITORING_FILE_INFOS* pMonitoringFileInfo,
                                        TCHAR* pszFullDescription,
                                        TCHAR* pszFileName,
                                        DWORD dwLineNumber,
                                        MONITORING_FILE_INFOS** ppAlreadyHookingMonitoringFileInfo,
                                        BOOL* pbParsingError);

    BOOL RemoveCOMObjectCreationCallBack(pfCOMObjectCreationCallBack pCOMObjectCreationCallBack);
    BOOL AddCOMObjectCreationCallBack(pfCOMObjectCreationCallBack pCOMObjectCreationCallBack);
    BOOL ReleaseCreatedCOMObjectsForStaticHooks();

    BOOL Start();
    BOOL Stop();

    BOOL ClearUserDataTypeCache();
};
