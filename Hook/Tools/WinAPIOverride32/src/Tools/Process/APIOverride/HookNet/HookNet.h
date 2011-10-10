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
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <windows.h>
#include "HookNetExport.h"
#include "FunctionInfoEx.h"
#include "../ApiOverride.h"
#include "../../../File/StdFileOperations.h"
#include "../../MailSlot/MailSlotClient.h"
#include "../../../../WinAPIOverride32\Options.h"
#include "../../../String/StringConverter.h"

using namespace NET;
//using NET::CFunctionInfoEx;
//using NET::CFunctionInfo;
//using NET::CParameterInfo;

#define MAX_END_OF_HOOK_NET_INIT_WAIT_IN_MS 5000

typedef struct tagStaticNetHookInfos 
{
    CLinkListItem*  pItemApiInfo;// API_INFO* pApiInfo=(API_INFO*)pItemApiInfo->ItemData; 
                                 // contains all needed informations for the hook
    CFunctionInfoEx* pFunctionInfo;// compiled function info associated to static hook
    TCHAR           AssemblyName[MAX_PATH]; // assembly name for which we want to install static hook
    mdToken         FunctionToken;          // function token for which we want to install static hook
    MONITORING_FILE_INFOS*  pMonitoringFileInfo;
    FAKING_DLL_INFOS*       pFakingDllInfos;
}STATIC_NET_HOOK_INFOS,*PSTATIC_NET_HOOK_INFOS;

BOOL Initialize();
BOOL Destroy();
BOOL CorInitialize();
BOOL CorShutdown();
BOOL HookFunctionUsingAutoHooking(CFunctionInfoEx* pFunctionInfo);
BOOL UnhookFunctionUsingAutoHooking(CFunctionInfoEx* pFunctionInfo);
BOOL UnhookFunction(CFunctionInfoEx* pFunctionInfo,BOOL CheckAutoHookingOwner);
BOOL AddCompiledFunction(CFunctionInfoEx* pFunctionInfo);
void ReportMessage(tagReportMessageType MsgType,TCHAR* MsgText);
void CompiledFunctionCallBack(CFunctionInfoEx* pFunctionInfo);
void ReportBadNetFunctionDescription(TCHAR* pszFuncDescription);
BOOL CheckHookAvailability(CFunctionInfoEx* pFunctionInfo);
API_INFO* GetApiInfoFromFuncId(FunctionID functionId);
CFunctionInfoEx* GetFunctionInfo(TCHAR* AssemblyName,mdToken FunctionToken,BOOL UserManagesLock);
BOOL GetAssemblyNameAndTokenFromFunctionDescription(IN TCHAR* FuncDescription,IN OUT TCHAR* AssemblyName,DWORD AssemblyNameMaxLen,OUT mdToken* pFunctionToken);
STATIC_NET_HOOK_INFOS* GetManuallyRegisteredInfos(TCHAR* AssemblyName,mdToken FunctionToken,BOOL bUserManagesLock);
void __stdcall ApiInfoItemDeletionCallback(CLinkListItem* pItemAPIInfo);
extern "C" __declspec(dllexport) BOOL __stdcall InitializeHookNet(HOOK_NET_INIT* pInitHookNet);
extern "C" __declspec(dllexport) BOOL __stdcall SetHookNetOptions(HOOK_NET_OPTIONS* pHookNetOptions);
extern "C" __declspec(dllexport) BOOL __stdcall UnHookAllNetMethods();
extern "C" __declspec(dllexport) BOOL __stdcall Uninitialize();
extern "C" __declspec(dllexport) PBYTE __stdcall GetNetCompiledFunctionAddress(TCHAR* pszFuncDescription);