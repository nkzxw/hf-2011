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
// Object: manage COM object creation hooking
//         parse COM object creation file and hook all specified functions
//-----------------------------------------------------------------------------


#pragma once

#include "HookComExport.h"
#include "HookCom.h"
#include "../../../File/StdFileOperations.h"
#include "../../../File/TextFile.h"
#include "../../../String/AnsiUnicodeConvert.h"
#include "../../../String/StringConverter.h"
#include "../../../String/TrimString.h"
/*
#ifndef StackSizeOf
#define StackSizeOf(Type) ((sizeof(Type)<sizeof(PBYTE))?sizeof(PBYTE):(sizeof(Type)))
#endif
*/
#define COM_OBJECT_CREATION_API_STACK_SIZE                  _T("StackSize=")
#define COM_OBJECT_CREATION_API_POBJECT_STACK_INDEX         _T("ObjectStackIndex=")
#define COM_OBJECT_CREATION_API_POBJECT_IS_RETURNED_VALUE   _T("ObjectIsReturnedValue")
#define COM_OBJECT_CREATION_API_PMULTI_STACK_INDEX          _T("ObjectArrayStackIndex=")
#define COM_OBJECT_CREATION_API_PMULTI_SIZE_STACK_INDEX     _T("ObjectArraySizeStackIndex=")
#define COM_OBJECT_CREATION_API_CLSID_VALUE                 _T("CLSIDValue=")
#define COM_OBJECT_CREATION_API_CLSID_VALUE_CURRENT         _T("CurrentObjectCLSID")
#define COM_OBJECT_CREATION_API_CLSID_STACK_INDEX           _T("CLSIDStackIndex=")
#define COM_OBJECT_CREATION_API_REFCLSID_STACK_INDEX        _T("REFCLSIDStackIndex=")
#define COM_OBJECT_CREATION_API_IID_VALUE                   _T("IIDValue=")
#define COM_OBJECT_CREATION_API_IID_STACK_INDEX             _T("IIDStackIndex=")
#define COM_OBJECT_CREATION_API_REFIID_STACK_INDEX          _T("REFIIDStackIndex=")


class CCOMCreationPostApiCallHooks
{
private:
    BOOL bStarted;// Flag to know if com creation functions are currently hooked
    CLinkList* pLinkListPostApiCallHooks;// link list of LINKLIST_POSTAPICALL_HOOKDATA

    static BOOL ParseConfigFileLineCallBack(TCHAR* pszFileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam);
    static BOOL __stdcall COMObjectsArrayCreationPostHook(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);
    static BOOL __stdcall COMObjectCreationPostHook(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);
    static BOOL __stdcall COMObjectCreationUsingReturnedValuePostHook(PBYTE pEspArgs,REGISTERS* pAfterCallRegisters,PRE_POST_API_CALL_HOOK_INFOS* pHookInfos,PVOID UserParam);
    static BOOL GetCLSID(PBYTE pEspArgs,LINKLIST_POSTAPICALL_HOOKDATA* pHookData,OUT CLSID* pClsid);
    static BOOL GetIID(PBYTE pEspArgs,LINKLIST_POSTAPICALL_HOOKDATA* pHookData,OUT IID* pIid);
    
public:
    CCOMCreationPostApiCallHooks(void);
    ~CCOMCreationPostApiCallHooks(void);

    BOOL Start(TCHAR* pszConfigFileName);
    BOOL Stop();
    BOOL IsStarted();

    BOOL InstallPostHook(TCHAR* DllName,TCHAR* ApiName,PBYTE HookedFunctionAddress,BOOL bFunctionPointer,DWORD StackSize,LINKLIST_POSTAPICALL_HOOKDATA* pHookData);
    BOOL ParseComCreationParameters(TCHAR* Parameters,CLSID* pCurrentCLSID,BOOL StackSizeMustBeSpecified,OUT LINKLIST_POSTAPICALL_HOOKDATA* pHookData,DWORD* pStackSize);
};
