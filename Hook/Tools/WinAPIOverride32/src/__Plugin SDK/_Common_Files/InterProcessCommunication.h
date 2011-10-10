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

#pragma once

#include "Registers.h"

#define REGISTER_BYTE_SIZE sizeof(PBYTE)

// dll  name
#define API_OVERRIDE_DLL_NAME _T("ApiOverride.dll")
#define INJECTLIB_DLL_NAME _T("InjLib.dll")
#define HOOKNET_DLL_NAME _T("hooknet.dll")

#define APIOVERRIDE_MUTEX _T("APIOVERRIDE_MUTEX")
#define APIOVERRIDE_DEFINES_PATH _T("UserDefines\\")
#define APIOVERRIDE_USER_TYPES_PATH _T("UserTypes\\")

// MailSlots names. These names are followed by Target process Id (allow multiple instance of this soft)
#define APIOVERRIDE_MAILSLOT_TO_INJECTOR _T("\\\\.\\mailslot\\APIOVERRIDE_TO_INJECTOR")
#define APIOVERRIDE_MAILSLOT_FROM_INJECTOR _T("\\\\.\\mailslot\\APIOVERRIDE_FROM_INJECTOR")

// Events names. These names are followed by Target process Id (allow multiple instance of this soft)
//(Injector -> APIOverride)
#define APIOVERRIDE_EVENT_START_MONITORING _T("Global\\APIOVERRIDE_EVENT_START_MONITORING")
#define APIOVERRIDE_EVENT_STOP_MONITORING _T("Global\\APIOVERRIDE_EVENT_STOP_MONITORING")
#define APIOVERRIDE_EVENT_START_FAKING _T("Global\\APIOVERRIDE_EVENT_START_FAKING")
#define APIOVERRIDE_EVENT_STOP_FAKING _T("Global\\APIOVERRIDE_EVENT_STOP_FAKING")
#define APIOVERRIDE_EVENT_FREE_PROCESS _T("Global\\APIOVERRIDE_EVENT_FREE_PROCESS")
#define APIOVERRIDE_EVENT_SINGLETHREADEDMAILSLOTSERVER_END_WAITING_INSTRUCTIONS_LOOP _T("Global\\APIOVERRIDE_EVENT_END_SMS_LOOP")

// (APIOverride -> Injector)
#define APIOVERRIDE_EVENT_DLLPROCESS_ATTACH_COMPLETED _T("Global\\APIOVERRIDE_EVENT_DLLPROCESS_ATTACH_COMPLETED")
#define APIOVERRIDE_EVENT_DLL_DETACHED_COMPLETED _T("Global\\APIOVERRIDE_EVENT_DLLPROCESS_DETACHED_COMPLETED")
#define APIOVERRIDE_EVENT_PROCESS_FREE _T("Global\\APIOVERRIDE_EVENT_PROCESS_FREE")
#define APIOVERRIDE_EVENT_MONITORING_FILE_LOADED _T("Global\\APIOVERRIDE_EVENT_MONITORING_FILE_LOADED")
#define APIOVERRIDE_EVENT_MONITORING_FILE_UNLOADED _T("Global\\APIOVERRIDE_EVENT_MONITORING_FILE_UNLOADED")
#define APIOVERRIDE_EVENT_FAKE_API_DLL_LOADED _T("Global\\APIOVERRIDE_EVENT_FAKE_API_DLL_LOADED")
#define APIOVERRIDE_EVENT_FAKE_API_DLL_UNLOADED _T("Global\\APIOVERRIDE_EVENT_FAKE_API_DLL_UNLOADED")
#define APIOVERRIDE_EVENT_ERROR _T("Global\\APIOVERRIDE_EVENT_ERROR")
#define APIOVERRIDE_EVENT_CLIENT_MAILSLOT_OPEN _T("Global\\APIOVERRIDE_EVENT_MAILSLOT_OPEN")

// (Hook net -> Injector)
#define APIOVERRIDE_EVENT_HOOKNET_STARTED _T("Global\\APIOVERRIDE_EVENT_HOOKNET_STARTED")

// tag to specify internal address of software instead of a libname/funcname
#define EXE_INTERNAL_PREFIX _T("EXE_INTERNAL@0x")
#define EXE_INTERNAL_POINTER_PREFIX _T("EXE_INTERNAL_POINTER@0x")
#define EXE_INTERNAL_RVA_PREFIX _T("EXE_INTERNAL_RVA@0x")
#define EXE_INTERNAL_RVA_POINTER_PREFIX _T("EXE_INTERNAL_RVA_POINTER@0x")
// tag to specify internal address of a dll instead of an exported funcname
// allow to hook non exported function without knowing loaded bas address
#define DLL_INTERNAL_PREFIX _T("DLL_INTERNAL@0x")
#define DLL_INTERNAL_POINTER_PREFIX _T("DLL_INTERNAL_POINTER@0x")

// tag to specify ordinal exported address of a dll instead of an exported funcname
// allow to hook non exported function without knowing loaded bas address
#define DLL_ORDINAL_PREFIX _T("DLL_ORDINAL@0x")

#define DLL_OR_EXE_NET_PREFIX _T(".NET@")

// flags to store more information on file type
#define EXTENDED_TYPE_FLAG_NET_SINGLE_DIM_ARRAY     0x80000000 // used internally by injected dll only
#define EXTENDED_TYPE_FLAG_NET_MULTIPLE_DIM_ARRAY   0x40000000 // used internally by injected dll only
#define EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_DEFINE_VALUES_FILE 0x2000000 // used by winapioverride and injected dll
#define EXTENDED_TYPE_FLAG_HAS_ASSOCIATED_USER_DATA_TYPE_FILE 0x1000000 // used by winapioverride and injected dll
#define EXTENDED_TYPE_FLAG_MASK                     0xFF000000
#define SIMPLE_TYPE_FLAG_MASK                       0x00FFFFFF  // winapioverrride enum type mask (allow to use unused PARAMETER_INFOS.dwType first bytes as extended informations)

// struct for commands (Injector -> APIOverride)
enum tagAPIOverrideCommands
{
    CMD_LOAD_MONITORING_FILE,
    CMD_UNLOAD_MONITORING_FILE,
    CMD_LOAD_FAKE_API_DLL,
    CMD_UNLOAD_FAKE_API_DLL,
    CMD_FREE_PROCESS,
    CMD_MONITORING_LOG,
    CMD_PROCESS_INTERNAL_CALL_QUERY,
    CMD_PROCESS_INTERNAL_CALL_REPLY,
    CMD_START_LOG_ONLY_BASE_MODULE,
    CMD_STOP_LOG_ONLY_BASE_MODULE,
    CMD_START_MODULE_LOGGING,
    CMD_STOP_MODULE_LOGGING,
    CMD_SET_LOGGED_MODULE_LIST_FILTERS_WAY,
    CMD_CLEAR_LOGGED_MODULE_LIST_FILTERS,
    CMD_ENABLE_MODULE_FILTERS_FOR_MONITORING,
    CMD_DISABLE_MODULE_FILTERS_FOR_MONITORING,
    CMD_ENABLE_MODULE_FILTERS_FOR_FAKING,
    CMD_DISABLE_MODULE_FILTERS_FOR_FAKING,
    CMD_NOT_LOGGED_MODULE_LIST_QUERY,
    CMD_NOT_LOGGED_MODULE_LIST_REPLY,
    CMD_DUMP,
    CMD_MONITORING_FILE_DEBUG_MODE,
    CMD_CALLSTACK_RETRIEVAL,
    CMD_AUTOANALYSIS,
    CMD_BREAK_DONT_BREAK_APIOVERRIDE_THREADS,
    CMD_REPORT_MESSAGE,
    CMD_CLEAR_USER_DATA_TYPE_CACHE,
    // hook COM dll msg
    CMD_COM_HOOKING_START_STOP,
    CMD_COM_HOOKING_OPTIONS,
    CMD_COM_INTERACTION,
    CMD_COM_RELEASE_CREATED_COM_OBJECTS_FOR_STATIC_HOOKS,
    CMD_COM_RELEASE_AUTO_HOOKED_OBJECTS,
    // Hook Net dll msg
    CMD_NET_INITIALIZE_HOOKNET_DLL,
    CMD_NET_SHUTDOWN_HOOKNET_DLL,
    CMD_NET_HOOKING_START_STOP,
    CMD_NET_HOOKING_OPTIONS,
    CMD_NET_INTERACTION,
    CMD_NET_RELEASE_HOOKED_METHODS,

    // other message
    CMD_OVERRIDING_DLL_QUERY_TO_PLUGIN,
    CMD_PLUGIN_REPLY_TO_OVERRIDING_DLL_QUERY,

	// .Net plug in interactions
	CMD_NET_HOOK_FROM_TOKEN,
	CMD_NET_UNHOOK_FROM_TOKEN
};

// types of report messages
// DON'T CHANGE EXISTING VALUES TO AVOID TROUBLES RELOADING OLD MONITORING FILES
enum tagReportMessageType
{
    REPORT_MESSAGE_INFORMATION=1,
    REPORT_MESSAGE_WARNING=2,
    REPORT_MESSAGE_ERROR=3,
    REPORT_MESSAGE_EXCEPTION=4
};

enum tagFirstBytesAutoAnalysis
{
    FIRST_BYTES_AUTO_ANALYSIS_NONE,    // no first bytes analysis is done
    FIRST_BYTES_AUTO_ANALYSIS_SECURE,  // first bytes analysis is done and used only 
                                       // - if first instruction length is more than HOOK_SIZE
                                       // - if first bytes match a well known sequence
    FIRST_BYTES_AUTO_ANALYSIS_INSECURE// first bytes analysis is done and used even 
                                       // if first instruction length is less than HOOK_SIZE
};

enum tagCALLING_CONVENTION
{
    CALLING_CONVENTION_STDCALL_OR_CDECL=0,
    CALLING_CONVENTION_STDCALL,
    CALLING_CONVENTION_CDECL,
    CALLING_CONVENTION_FASTCALL,
    CALLING_CONVENTION_THISCALL,
    CALLING_CONVENTION_FASTCALL_PUSHED_LEFT_TO_RIGHT // used for process internal call only do not use this type for APIInfo field
};

enum tagFilteringWay
{
    FILTERING_WAY_ONLY_SPECIFIED_MODULES,
    FILTERING_WAY_NOT_SPECIFIED_MODULES,
    FILTERING_WAY_DONT_USE_LIST
};

#define MAX_CMD_PARAMS 10
typedef struct _STRUCT_COMMAND
{
    DWORD dwCommand_ID;// must be at first position // let DWORD : better for 32 - 64 bit cross injection
    union {
        TCHAR pszStringParam[MAX_PATH];
        DWORD Param[MAX_CMD_PARAMS];
    };
}STRUCT_COMMAND,*PSTRUCT_COMMAND;

// struct for api logging (APIOverride -> Injector)
enum tagParamDirectionType// Param direction type enum
{
    PARAM_DIR_TYPE_IN,
    PARAM_DIR_TYPE_OUT,
    PARAM_DIR_TYPE_IN_NO_RETURN
};

enum HOOK_TYPE // used in saved logged files : don't change values
{
    HOOK_TYPE_API=0, // must be 0 (used as default value, no need to be changed after an allocation with zeromemory)
    HOOK_TYPE_COM=1,
    HOOK_TYPE_NET=2
};

typedef struct tagExtendedFunctionInfosForNET
{
    ULONG32 FunctionToken;
}EXTENDED_FUNCTION_INFOS_FOR_NET;
typedef struct tagExtendedFunctionInfosForCOM
{
    CLSID ClassID;
    IID InterfaceID;
    DWORD VTBLIndex;
}EXTENDED_FUNCTION_INFOS_FOR_COM;

union tagExtendedFunctionInfosForHookType
{
    tagExtendedFunctionInfosForCOM InfosForCOM;
    tagExtendedFunctionInfosForNET InfosForNET;
};

typedef struct tagLogEntryFixedSize
{
    DWORD dwProcessId;
    DWORD dwThreadId;
    PBYTE pOriginAddress;
    PBYTE RelativeAddressFromCallingModuleName;
    REGISTERS RegistersBeforeCall;
    REGISTERS RegistersAfterCall;
    PBYTE ReturnValue;
    double DoubleResult;

    DWORD dwLastError;
    FILETIME CallTime;
    DWORD dwCallDuration;

    BOOLEAN bFailure;// don't use bSuccess because as memory is set to 0 by default, default value is FALSE
                     // Using bFailure allow to have a successful return for undefined Failure returned type
    BYTE bParamDirectionType;
    BYTE bNumberOfParameters;
    WORD CallStackSize;// number of caller found
    WORD CallStackEbpRetrievalSize;// (number of stack parameters to retrieve for each caller function found) * sizeof(PBYTE)

    FILETIME FirstHookedParentCallTime;// id of the first previous hooked caller, null if no parent 
                                        // use logged caller start time has it is unique by thread
                                        // this field is useful only for call stack analysis
    HOOK_TYPE HookType; // specify hook type API / COM / NET
}LOG_ENTRY_FIXED_SIZE,*PLOG_ENTRY_FIXED_SIZE;

#define PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE 40
typedef struct tagParameterLogInfos
{
    // NOTICE keep structure order

    DWORD dwType;
    PBYTE Value;// if (dwSizeOfPointedData >0 )
                //    {Value = pointer value} // data value is in pbValue
                // else
                //    {
                //      if (dwSizeOfData < REGISTER_BYTE_SIZE) // direct data value
                //          {Value = value of parameter}
                //      else 
                //           {value is stored in pbValue and Value = 0}
    DWORD dwSizeOfData;// size of Data. If <=REGISTER_BYTE_SIZE param value is stored in Value (no memory allocation) else in pbValue 
    DWORD dwSizeOfPointedValue;// size of pbValue.
    TCHAR pszParameterName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE];
    BYTE* pbValue;// content of data if dwSizeOfData > REGISTER_BYTE_SIZE
                  // content of pointer if dwSizeOfPointedData > 0
                  // NULL if (dwSizeOfData <= REGISTER_BYTE_SIZE) && (dwSizeOfPointedData==0)
    TCHAR* pszDefineNamesFile;   // currently not used in injected dll (information is already stored inside ApiInfo for injected dll)
    TCHAR* pszUserDataTypeName;// currently not used in injected dll (information is already stored inside ApiInfo for injected dll)
}PARAMETER_LOG_INFOS,*PPARAMETER_LOG_INFOS;

typedef struct _STRUCT_FUNC_PARAM
{
    BOOL bPassAsRef;    // true if param is pass as ref
    DWORD dwDataSize;   // size in byte
    PBYTE pData;        // pointer to data
}STRUCT_FUNC_PARAM,*PSTRUCT_FUNC_PARAM;
