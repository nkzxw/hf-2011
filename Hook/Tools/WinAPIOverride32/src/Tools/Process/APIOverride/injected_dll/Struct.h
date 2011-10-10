/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
originally based from APISpy32 v2.1 from Yariv Kaplan @ WWW.INTERNALS.COM

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
// Object: common structs
//-----------------------------------------------------------------------------

#pragma once

#include "defines.h"
#include "../ExportedStructs.h"
#include "../InterProcessCommunication.h"
#include "../GenericFakeAPI.h"
#include "../../../LinkList/LinkList.h"

typedef struct tagCallStackItemInfoTemp
{
    PBYTE Address;
    PBYTE RelativeAddress;
    PBYTE ParametersPointer;
    TCHAR pszModuleName[MAX_PATH];
}CALLSTACK_ITEM_INFO_TEMP,*PCALLSTACK_ITEM_INFO_TEMP;


typedef struct _tagAPIOverrideInternalModuleLimits
{
    HMODULE hModule;
    BYTE* Start;
    BYTE* End;
}APIOVERRIDE_INTERNAL_MODULELIMITS,*PAPIOVERRIDE_INTERNAL_MODULELIMITS;

typedef struct tagAPILogBreakWay
{
    ULONG64 BreakBeforeCall:1,
            BreakAfterCall:1,
            BreakLogInputAfter:1,
            BreakLogOutputAfter:1,
            BreakAfterCallIfNullResult:1,
            BreakAfterCallIfNotNullResult:1,
            BreakOnFailure:1,
            BreakOnSuccess:1,
            LogIfNotNullResult:1,
            LogIfNullResult:1,
            LogOnFailure:1,
            LogOnSuccess:1,

            FailureIfNullRet:1,
            FailureIfNotNullRet:1,
            FailureIfRetValue:1,
            FailureIfNotRetValue:1,
            FailureIfNegativeRetValue:1,
            FailureIfPositiveRetValue:1,

            FailureIfNullFloatingRet:1,
            FailureIfNotNullFloatingRet:1,
            FailureIfFloatingRetValue:1,
            FailureIfNotFloatingRetValue:1,
            FailureIfFloatingNegativeRetValue:1,
            FailureIfFloatingPositiveRetValue:1,

            FailureIfSignedRetLess:1,
            FailureIfSignedRetUpper:1,
            FailureIfUnsignedRetLess:1,
            FailureIfUnsignedRetUpper:1,
            FailureIfFloatingRetLess:1,
            FailureIfFloatingRetUpper:1,

            FailureIfLastErrorValue:1,
            FailureIfNotLastErrorValue:1,
            FailureIfLastErrorValueLess:1,
            FailureIfLastErrorValueUpper:1,

            Unused:30;
   
}API_LOG_BREAK_WAY,*PAPI_LOG_BREAK_WAY;

typedef struct  tagPre_Post_Api_Call_Chain_Data
{
    PBYTE CallBack;      // pfPreApiCallCallBack or pfPostApiCallCallBack function pointer
    PVOID UserParam;     // a user parameter for call back
    HMODULE OwnerModule; // HMODULE of dll that install pre or post hook
}PRE_POST_API_CALL_CHAIN_DATA,*PPRE_POST_API_CALL_CHAIN_DATA;

typedef struct tagMonitoringFileInfos
{
    TCHAR szFileName[MAX_PATH];// File name of monitoring file or overriding dll
}MONITORING_FILE_INFOS,*PMONITORING_FILE_INFOS;

typedef struct tagFakingDllInfos
{
    HMODULE hModule;           // HMODULE of the dll. Allow to know dll name
    DWORD ApiOverrideBuildVersionFramework;
    pfCOMObjectCreationCallBack pCOMObjectCreationCallBack;
}FAKING_DLL_INFOS,*PFAKING_DLL_INFOS;

typedef struct tagDefineInfos
{
    TCHAR szFileName[MAX_PATH];// File name relative from Defines directory
}DEFINE_INFOS,*PDEFINE_INFOS;

typedef struct tagUserTypeInfos
{
    TCHAR szName[MAX_PATH];// Type name
    SIZE_T TypeSize;
    SIZE_T NbPointedTimes; // number of time a type is pointed ex for PBYTE: typedef BYTE *PBYTE -> NbPointedTimes = 1, TypeSize = 1
}USER_TYPE_INFOS,*PUSER_TYPE_INFOS;

typedef struct tagParameterInfos
{
    DWORD dwType;                   // type of parameter
    DWORD dwSizeOfPointedData;      // SizeOfPointedData (or number of depending parameter if bSizeOfPointedDataDefinedByAnotherParameter)
    DWORD dwSizeOfData;             // Size Of Parameter
    BOOL  bSizeOfPointedDataDefinedByAnotherParameter;// if TRUE, dwSizeOfPointedData is the index of parameter specifying size (0 based index)
    DWORD SizeOfPointedDataDefinedByAnotherParameterFactor;
    // by the way for ReadFile( IN HANDLE hFile, OUT LPVOID lpBuffer, IN DWORD nNumberOfBytesToRead, OUT LPDWORD lpNumberOfBytesRead, IN LPOVERLAPPED lpOverlapped)
    // dwSizeOfPointedData will be 2

    CLinkList* pConditionalLogContent;  // list of MONITORING_PARAMETER_OPTIONS, used to do parameter filtering
    CLinkList* pConditionalBreakContent;// list of MONITORING_PARAMETER_OPTIONS, used to do parameter filtering

    USER_TYPE_INFOS* pUserTypeInfos;
    DEFINE_INFOS* pDefineInfos;
    TCHAR pszParameterName[PARAMETER_LOG_INFOS_PARAM_NAME_MAX_SIZE];// parameter name (if specified in monitoring file)
}PARAMETER_INFOS,*PPARAMETER_INFOS;

// number of register potentially used by fastcall
#ifdef _WIN64
    #define FASTCALL_NUMBER_OF_PARAM_POTENTIALY_PASSED_BY_REGISTER 4
#else
    #define FASTCALL_NUMBER_OF_PARAM_POTENTIALY_PASSED_BY_REGISTER 2
#endif

typedef void (__stdcall *pfApiInfoItemDeletionCallback)(CLinkListItem* pItemAPIInfo);
typedef struct tagAPI_INFO
{
    BOOL FirstBytesCanExecuteAnywhereNeedRelativeAddressChange; // TRUE if a relative address need to be changed in the first byte executed at another place
    DWORD FirstBytesCanExecuteAnywhereSize; // Number of bytes that must be executed at another place

    BYTE pbHookCodes[MAX_OPCODE_REPLACEMENT_SIZE]; // hook opcodes, used to install hook, only make a jump to pbSecondHook (allow to overwrite only 5 opcode bytes)

    // Opcodes: original opcode for OPCODE_REPLACEMENT_SIZE first bytes (used to remove hook)
    // FIRST_OPCODES_MAX_SIZE next bytes used if FirstBytesCanExecuteAnyWhereSize!=0
    // (do a jump to address = original API address + OPCODE_REPLACEMENT_SIZE)
    BYTE Opcodes[MAX_OPCODE_REPLACEMENT_SIZE]; 
    BYTE OpcodesExecutedAtAnotherPlace[MAX_OPCODES_EXECUTEDATANOTHERPLACE_SIZE];// opcode executed at another place if FirstBytesCanExecuteAnywhereSize!=0
    PBYTE OpcodesExecutedAtAnotherPlaceExtended;// extension of OpcodesExecutedAtAnotherPlace if executed moved size is to big
    BYTE pbSecondHook[MAX_SECOND_HOOK_SIZE]; // asm code between jump and generic API Handler
    FARPROC APIAddress;                  // address of hooked api function if bFunctionPointer=FALSE
                                         // address of function pointer if bFunctionPointer=TRUE
    FARPROC FakeAPIAddress;              // address of function executed instead of API (0 if none defined)
    CLinkList* PreApiCallChain;          // linked list of functions called before api calling
    CLinkList* PostApiCallChain;         // linked list of functions called after api calling

    BOOL bFunctionPointer;// if TRUE, pAPIInfo->APIAddress points to a function pointer, not a function
                          // By the way you have to put it to TRUE for COM vtbl patching
    BYTE OpcodeReplacementSize; // size in byte of opcode changed at hook installation

    DWORD dwOldProtectionFlags; // old memory protection flags
    volatile LONG UseCount;           // number of time the function is being hooked
    HANDLE evtEndOfHook;        // event set at the end of the hook

    BOOL bOriginalOpcodes;      // true if opcode at api address are the originals ones

    TCHAR* szModuleName;        // dll name
    TCHAR* szAPIName;           // function name

    PARAMETER_INFOS ParamList[MAX_PARAM]; // list of parameters definition
    BYTE MonitoringParamCount;            // number of parameters
    BYTE ParamDirectionType;              // logging direction (in, out, inout)
    DWORD StackSize;                      // size of stack required by all parameters

    MONITORING_FILE_INFOS* pMonitoringFileInfos;  // pointer to informations on monitoring file associated with the hook
    FAKING_DLL_INFOS* pFakeDllInfos;              // pointer to informations on the faking dll associated with the hook

	BOOL DontCheckModulesFilters;// use for EXE_INTERNAL@0x or DLL_INTERNAL@0x, because this can be callback of Enum functions
								 // like EnumWindows, and in this case caller address is in the user32.dll memory space,
								 // so if we don't have this flag, callback will never appears until we stop using filters
                                 // version 5.5.1 and upper used by monitoring files flag "|DontCheckModulesFilters"
    BOOL DontCheckModulesFiltersForFaking; // introduce in version 5.5.1, allow for single faking API inside faking dll to respect or not modules filters
                                           // interesting by the way if you are spying some generics API using modules filtering but you want another api to be spied 
                                           // whatever the calling module is (like CreateProcess) 
    API_LOG_BREAK_WAY LogBreakWay;
    BOOL BlockingCall;          // TRUE if blocking call
    BOOL AskedToRemove;         // TRUE if hook has been queried to be unhooked
    BOOL FreeingMemory;         // TRUE if hook is currently freeing its memory (used for speed unhook+hook of the same function without waiting end of hooked functions)

    PBYTE FailureValue;         // reference failure value for integer failure (<, > and != are defined in LogBreakWay)
    double FloatingFailureValue;// reference failure value for floating failure (<, > and != are defined in LogBreakWay)
    DWORD FailureLastErrorValue;// reference failure value for last error code (<, > and != are defined in LogBreakWay)

    HOOK_TYPE HookType;// specify hook type (standard API, .NET,...)
    tagCALLING_CONVENTION CallingConvention;
    pfApiInfoItemDeletionCallback DeletionCallback;// called when a hooked API_INFO item is removed from hooked list
    tagExtendedFunctionInfosForHookType HookTypeExtendedFunctionInfos; // contains extended informations to uniquely identify function
}API_INFO,*PAPI_INFO;

typedef struct tagBlockingCall
{
    HANDLE evtThreadStop;
    PAPI_INFO pApiInfo;
}BLOCKING_CALL,*PBLOCKING_CALL;

typedef struct tagMonitoringParameterOptions
{
    PBYTE Value;
    PBYTE pbPointedValue;
    DWORD dwPointedValueSize;
    DWORD dwValueSize;
}MONITORING_PARAMETER_OPTIONS,*PMONITORING_PARAMETER_OPTIONS;


typedef struct tagLogInfos
{
    BOOL ParmeterParsed;
    DWORD NbParameters;
    PARAMETER_LOG_INFOS ParamLogList[MAX_PARAM];
    DWORD dwLastErrorCode;
	FILETIME CallTime;
    DWORD dwCallDuration;
}LOG_INFOS,*PLOG_INFOS;

typedef struct tagFreeAPIInfo
{
    CLinkListItem* pItemAPIInfo;
    DWORD InitialTickCount;
    BOOL ForceLog;
}FREE_APIINFO,*PFREE_APIINFO;

typedef struct tagAPIHandlerTlsData
{
    REGISTERS OriginalRegisters;
    PBYTE AddressOfOriginalReturnAddress;
    PBYTE OriginalReturnAddress;
    API_INFO* pAPIInfo;
    PBYTE FS0Value;
    PBYTE OriginalExceptionHandler;
    PBYTE OriginalExceptionHandlerAddress;
    BOOL  bExceptionHookError;

    HANDLE BlockingCallThread;
    BLOCKING_CALL BlockingCallArg;
    HMODULE CallingModuleHandle;

    LARGE_INTEGER TickCountBeforeCall;
    PBYTE RelativeAddressFromCallingModule;
    REGISTERS LogOriginalRegisters;
    int OriginalThreadPriority;
    LOG_INFOS LogInfoIn;
    BOOL bLogInputParameters;
    BOOL bMatchMonitoringFilters;
    BOOL bMatchFakingFilters;
    BOOL bFakingApplied;
    TCHAR szCallingModuleName[MAX_PATH];
    PBYTE pParametersPointer;
    BOOL UnhookedDuringCall;
    BOOL ExecutionCompleted;
}API_HANDLER_TLS_DATA,*PAPI_HANDLER_TLS_DATA;

typedef struct tagLoadedDll
{
    TCHAR Name[MAX_PATH];
    HMODULE hModule;
}LOADED_DLL,*PLOADED_DLL;