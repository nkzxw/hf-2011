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
// Object: load fake api dll
//-----------------------------------------------------------------------------

#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "ApiInfo.h"
#include "APIOverrideKernel.h"
#include "../../../LinkList/LinkList.h"
#include "../../../String/AnsiUnicodeConvert.h"
#include "../../../File/StdFileOperations.h"
#include "../GenericFakeAPI.h"
#include "QueryFakingOwnership.h"

#define GET_FAKE_API_ARRAY_FUNCTION_NAME            _T("_GetFakeAPIArray@0")
#define GET_FAKE_API_ENCODING_FUNCTION_NAME         _T("_GetFakeAPIEncoding@0")
#define GET_APIOVERRIDE_BUILD_VERSION_FUNCTION_NAME _T("_GetAPIOverrideBuildVersion@0")
#define GET_PRE_API_CALL_ARRAY_FUNCTION_NAME        _T("_GetPreAPICallArray@0")
#define GET_POST_API_CALL_ARRAY_FUNCTION_NAME       _T("_GetPostAPICallArray@0")
#define COM_OBJECT_CREATION_CALLBACK_EXPORTED_NAME  _T("_COMObjectCreationCallBack@16")
#define INITIALIZE_FAKE_DLL_FUNCTION_NAME           _T("_InitializeFakeDll@4")

#define OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_SIZE_MASK 0x000000FF
#define OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS                 0x80000000
#define OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE          0x40000000

BOOL LoadFakeApiDll(TCHAR* szFakeAPIDLLFullPath);
BOOL UnloadFakeApiDll(TCHAR* szFakeAPIDLLFullPath);

BOOL UnloadAllFakeApiDlls();
BOOL GetFakeApiDllName(FAKING_DLL_INFOS* pFakingDllInfos,TCHAR* pszFileName);
BOOL GetFakeApiDllInfos(TCHAR* pszFileName,FAKING_DLL_INFOS** ppFakingDllInfos);

BOOL __stdcall AddPreApiCallCallBack(CLinkListItem* pItemAPIInfo,BOOL bNeedToBeHooked,HMODULE OwnerModule,pfPreApiCallCallBack FunctionPointer,PVOID UserParam);
BOOL __stdcall RemovePreApiCallCallBack(CLinkListItem* pItemAPIInfo,pfPreApiCallCallBack FunctionPointer,BOOL bRestoreOriginalBytes);
BOOL __stdcall AddPostApiCallCallBack(CLinkListItem* pItemAPIInfo,BOOL bNeedToBeHooked,HMODULE OwnerModule,pfPostApiCallCallBack FunctionPointer,PVOID UserParam);
BOOL __stdcall RemovePostApiCallCallBack(CLinkListItem* pItemAPIInfo,pfPostApiCallCallBack FunctionPointer,BOOL bRestoreOriginalBytes);
BOOL LoadFakeAPIDefinition(FAKING_DLL_INFOS* pFakingDllInfos,STRUCT_FAKE_API_WITH_USERPARAM*  pFakeApiInfos,tagFakingDllArray FakingDllArray,DWORD ApiOverrideBuildVersionFramework);
BOOL ConvertFakeAPIUnicodeToAnsi(STRUCT_FAKE_API_UNICODE_WITH_USERPARAM* pFakeApiUnicode,OUT STRUCT_FAKE_API_ANSI_WITH_USERPARAM* pFakeApiAnsi,BOOL bHasUserParam);
BOOL ConvertFakeAPIAnsiToUnicode(STRUCT_FAKE_API_ANSI_WITH_USERPARAM* pFakeApiAnsi,OUT STRUCT_FAKE_API_UNICODE_WITH_USERPARAM* pFakeApiUnicode,BOOL bHasUserParam);
BOOL LoadFakeArray(FAKING_DLL_INFOS* pFakingDllInfos,TCHAR* szFakeAPIDLLFullPath,PVOID FakeArray,int iFakeAPIEncoding,tagFakingDllArray FakingDllArray,DWORD ApiOverrideBuildVersionFramework);