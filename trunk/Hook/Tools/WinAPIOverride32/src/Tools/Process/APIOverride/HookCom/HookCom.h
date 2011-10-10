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
// Object: manage com hook
//
// WARNING HookCom.dll Heap can differ from ApiOverride.dll 
//         --> standard memory allocation (malloc, new) done in HookCom.dll can't be deleted from ApiOverride.dll
//-----------------------------------------------------------------------------

#pragma once
#include <Windows.h>
#include "DefinesAndStructs.h"
#include "methodinfo.h"
#include "postapicallhooks.h"
#include "CLSIDFilters.h"
#include "../../../Com/GUIDStringConvert.h"
#include "../../../File/StdFileOperations.h"
#include "../../../File/TextFile.h"
#include "../../../String/AnsiUnicodeConvert.h"
#include "../../../String/StringConverter.h"
#include "../../../LinkList/LinkListSimple.h"
#include "../../../LinkList/LinkList.h"

class CHookedClass;
class CHookedInterface;

#include "HookedClass.h"
#include "HookedInterface.h"
#include "getidispatchwinapioverridefunctionsrepresentation.h"


extern "C" __declspec(dllexport) BOOL __stdcall InitializeHookCom(HOOK_COM_INIT* pInitHookCom);
extern "C" __declspec(dllexport) BOOL __stdcall SetHookComOptions(HOOK_COM_OPTIONS* pHookComOptions);
extern "C" __declspec(dllexport) BOOL __stdcall UnHookAllComObjects();
extern "C" __declspec(dllexport) BOOL __stdcall StartHookingCreatedCOMObjects();
extern "C" __declspec(dllexport) BOOL __stdcall StopHookingCreatedCOMObjects();
extern "C" __declspec(dllexport) BOOL __stdcall StartAutoHooking();
extern "C" __declspec(dllexport) BOOL __stdcall StopAutoHooking();
extern "C" __declspec(dllexport) BOOL __stdcall AddCOMObjectCreationCallBack(pfCOMObjectCreationCallBack pCOMObjectCreationCallBack);
extern "C" __declspec(dllexport) BOOL __stdcall RemoveCOMObjectCreationCallBack(pfCOMObjectCreationCallBack pCOMObjectCreationCallBack);

BOOL Initialize();
BOOL Destroy(BOOL CalledByFreeLibrary);
BOOL UnHookAllComObjects(BOOL CalledFromFreeLibrary);
BOOL CallCOMObjectCreationCallBacks(CLSID* pClsid,IID* pIid,PVOID pInterface,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos);
BOOL InitializeHook(TCHAR* pszModuleName,CMethodInfo* pMethodInfo,CLinkListItem** ppItemAPIInfo,BOOL* pbAlreadyHooked);
BOOL SetHookForObject(IUnknown* pObject,CLSID* pClsid,IID* pIid);
void MakeFullPathIfNeeded(IN OUT TCHAR* pszName);

BOOL GetHookDefinitionInfo(TCHAR* pszDefinition,IN IID* pCurrentIID,OUT HOOK_DEFINITION_INFOS* pHookDefinitionInfos);
BOOL GetHookAddress(CMethodInfo* pMethodInfo,OUT PBYTE* ppAddressToPatch,OUT BOOL* pbFunctionPointer);
BOOL GetModuleName(PBYTE FunctionAddress,TCHAR* pszModuleName,DWORD MaxModuleNameSize,HMODULE* pModuleHandle);

BOOL DecodeHookComDefinition(TCHAR* pszDefinition,OUT CLSID* pCLSID, OUT HOOK_DEFINITION_INFOS* pHookDefinitionInfos);

BOOL ReportParsingError(TCHAR* pszFileName,DWORD dwLineNumber);
void ReportNotSupportedInterface(CLSID* pClsid,IID* pIid);
void ReportBadFunctionAddress(CMethodInfo* pMethodInfo,PBYTE pAddress);
void StaticLoadingReportCOMCreationError(CLSID* pClsid,
                                         IID* pIid,
                                         TCHAR* pszFileName,
                                         DWORD dwLineNumberOrFunctionIndex,
                                         BOOL bMonitoring,
                                         tagFakingDllArray FakingType);
BOOL StartDllUnloadWatching();
BOOL StopDllUnloadWatching();
DWORD WINAPI WatchUnloadingDll(LPVOID lParam);

BOOL IsMethodHookSharedWithAnotherInterface(CMethodInfo* pMethodInfo,CHookedInterface* pLockedInterface,CHookedClass* pLockedInterfacesHookedClass,BOOL bLinkListHookedClassesLocked);

BOOL CreateInterfaceMonitoringFile(TCHAR* pszMonitoringFileName);
BOOL GetMonitoringFileName(IID* pIid,OUT TCHAR* pszMonitoringFileName);
void __stdcall ApiInfoItemDeletionCallback(CLinkListItem* pItemAPIInfo);

#define HOOK_COM_INTERFACE_MONITORING_FILE_HEADER _T(";COM Interface Monitoring File\r\n;For syntax, see documentation, COM/Auto Monitoring Files Syntax\r\n\r\n")