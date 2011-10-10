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
// Object: Generic data and functions used by fake api
//-----------------------------------------------------------------------------

#pragma once
#include <windows.h>
#include "ExportedStructs.h"
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#define OVERRIDING_DLL_API_OVERRIDE_BUILD_VERSION 6
#define OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_FIRST_BYTES_CAN_EXECUTE_ANYWHERE_SIZE_MASK 0x000000FF
#define OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_DONT_CHECK_MODULES_FILTERS                 0x80000000
#define OVERRIDING_DLL_API_OVERRIDE_EXTENDED_OPTION_FIRST_BYTES_CANT_EXECUTE_ANYWHERE          0x40000000
///////////////////////////////////////////////////////////////////
// struct definition 
///////////////////////////////////////////////////////////////////

// assume that structs share between WinAPIOverride and the FakeAPI dll
// will have the same alignment
#pragma pack(push)
#pragma pack(4)

typedef struct _STRUCT_FAKE_API
{
    TCHAR pszModuleName[MAX_PATH];
    TCHAR pszAPIName[MAX_PATH];
    FARPROC FakeAPI;
    DWORD StackSize;// necessary for allocating enough stack size if you put a fake api without monitoring it
    DWORD FirstBytesCanExecuteAnywhereSize;
}STRUCT_FAKE_API,*PSTRUCT_FAKE_API;

typedef struct _STRUCT_FAKE_API_WITH_USERPARAM
{
    // common STRUCT_FAKE_API struct
    TCHAR pszModuleName[MAX_PATH];
    TCHAR pszAPIName[MAX_PATH];
    FARPROC FakeAPI;
    DWORD StackSize;// necessary for allocating enough stack size if you put a fake api without monitoring it
    DWORD AdditionalOptions;
    // user param
    PVOID UserParam;
}STRUCT_FAKE_API_WITH_USERPARAM,*PSTRUCT_FAKE_API_WITH_USERPARAM;


///////////////////////////////////////////////////////////////////
// the following structs are defined to allow a Unicode version of WinApiOverride to load an Ansi Fake DLL
// and an ANSI version of WinApiOverride to load an UNICODE fake dll
///////////////////////////////////////////////////////////////////
typedef struct _STRUCT_FAKE_API_ANSI
{
    char pszModuleName[MAX_PATH];
    char pszAPIName[MAX_PATH];
    FARPROC FakeAPI;
    DWORD StackSize;// necessary for allocating enough stack size if you put a fake api without monitoring it
    DWORD FirstBytesCanExecuteAnywhereSize;
}STRUCT_FAKE_API_ANSI,*PSTRUCT_FAKE_API_ANSI;
typedef struct _STRUCT_FAKE_API_UNICODE
{
    wchar_t pszModuleName[MAX_PATH];
    wchar_t pszAPIName[MAX_PATH];
    FARPROC FakeAPI;
    DWORD StackSize;// necessary for allocating enough stack size if you put a fake api without monitoring it
    DWORD FirstBytesCanExecuteAnywhereSize;
}STRUCT_FAKE_API_UNICODE,*PSTRUCT_FAKE_API_UNICODE;

typedef struct _STRUCT_FAKE_API_ANSI_WITH_USERPARAM
{
    // common STRUCT_FAKE_API struct
    char pszModuleName[MAX_PATH];
    char pszAPIName[MAX_PATH];
    FARPROC FakeAPI;
    DWORD StackSize;// necessary for allocating enough stack size if you put a fake api without monitoring it
    DWORD FirstBytesCanExecuteAnywhereSize;
    // user param
    PVOID UserParam;
}STRUCT_FAKE_API_ANSI_WITH_USERPARAM,*PSTRUCT_FAKE_API_ANSI_WITH_USERPARAM;
typedef struct _STRUCT_FAKE_API_UNICODE_WITH_USERPARAM
{
    // common STRUCT_FAKE_API struct
    wchar_t pszModuleName[MAX_PATH];
    wchar_t pszAPIName[MAX_PATH];
    FARPROC FakeAPI;
    DWORD StackSize;// necessary for allocating enough stack size if you put a fake api without monitoring it
    DWORD FirstBytesCanExecuteAnywhereSize;
    // user param
    PVOID UserParam;
}STRUCT_FAKE_API_UNICODE_WITH_USERPARAM,*PSTRUCT_FAKE_API_UNICODE_WITH_USERPARAM;


typedef BOOL (__stdcall *tagOverridingDllSendDataToPlugin)(IN TCHAR* PluginName,IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize);
typedef BOOL (__stdcall *tagOverridingDllSendDataToPluginAndWaitReply)(IN TCHAR* PluginName,IN PBYTE DataToPlugin,IN SIZE_T DataToPluginSize,OUT PBYTE* pDataFromPlugin,OUT SIZE_T* pDataFromPluginSize,IN DWORD WaitTimeInMs /* INFINITE for infinite wait */);
typedef void (__stdcall *tagOverridingDllSendDataToPluginAndWaitReplyFreeReceivedData)(IN PBYTE pDataFromPluginToBeFree);
typedef struct _APIOVERRIDE_EXPORTED_FUNCTIONS
{
    tagOverridingDllSendDataToPlugin pOverridingDllSendDataToPlugin;
    tagOverridingDllSendDataToPluginAndWaitReply pOverridingDllSendDataToPluginAndWaitReply;
    tagOverridingDllSendDataToPluginAndWaitReplyFreeReceivedData pOverridingDllSendDataToPluginAndWaitReplyFreeReceivedData;
}APIOVERRIDE_EXPORTED_FUNCTIONS;

#pragma pack(pop)

// encoding type enum
enum tagFakeAPIEncoding
{
    FakeAPIEncodingANSI,
    FakeAPIEncodingUNICODE
};

// macro to know the stack sized used by a type or struct
#ifndef StackSizeOf
    #define StackSizeOf(Type) ((sizeof(Type)<sizeof(PBYTE))?sizeof(PBYTE):(sizeof(Type)))
#endif 

typedef void* (__stdcall *tagGetFakeAPIArrayFuncPointer)(void);
typedef int (__stdcall *tagGetFakeAPIEncoding)(void);
typedef int (__stdcall *tagGetAPIOverrideBuildVersion)(void);
typedef void (__stdcall *tagInitializeFakeDllFuncPointer)(APIOVERRIDE_EXPORTED_FUNCTIONS* pApiOverrideExportedFunc);