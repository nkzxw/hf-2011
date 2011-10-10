#include <windows.h>
#include <tlhelp32.h>
#if (!defined(_WIN32_WINNT))
    #define _WIN32_WINNT 0x0501
#endif
#include <Winternl.h>
#include <psapi.h>
#pragma comment (lib,"psapi")

#include "ProcessAndThreadID.h"

// this stuff just hide process and loaded dll of apioverride,
// this don't hides named event 
// --> don't think WinAPIOverride32 is fully hidden : it's only a proof of concept

// as we are never first process or first module, it's unused to fake Process32First 
// or Module32First


/////////////////////////////////
// structs and defines requiered for Process32NextA and Module32NextA
/////////////////////////////////
typedef BOOL (_stdcall *pfProcess32NextA)(HANDLE,PVOID);
typedef BOOL (_stdcall *pfProcess32NextW)(HANDLE,PVOID);
typedef BOOL (_stdcall *pfModule32NextA)(HANDLE,PVOID);
typedef BOOL (_stdcall *pfModule32NextW)(HANDLE,PVOID);

typedef struct tagMODULEENTRY32A
{
    DWORD   dwSize;
    DWORD   th32ModuleID;       // This module
    DWORD   th32ProcessID;      // owning process
    DWORD   GlblcntUsage;       // Global usage count on the module
    DWORD   ProccntUsage;       // Module usage count in th32ProcessID's context
    BYTE  * modBaseAddr;        // Base address of module in th32ProcessID's context
    DWORD   modBaseSize;        // Size in bytes of module starting at modBaseAddr
    HMODULE hModule;            // The hModule of this module in th32ProcessID's context
    char    szModule[MAX_MODULE_NAME32 + 1];
    char    szExePath[MAX_PATH];
} MODULEENTRY32A;
typedef MODULEENTRY32A *  PMODULEENTRY32A;
typedef MODULEENTRY32A *  LPMODULEENTRY32A;

typedef struct tagPROCESSENTRY32A
{
    DWORD   dwSize;
    DWORD   cntUsage;
    DWORD   th32ProcessID;          // this process
    ULONG_PTR th32DefaultHeapID;
    DWORD   th32ModuleID;           // associated exe
    DWORD   cntThreads;
    DWORD   th32ParentProcessID;    // this process's parent process
    LONG    pcPriClassBase;         // Base priority of process's threads
    DWORD   dwFlags;
    CHAR    szExeFile[MAX_PATH];    // Path
} PROCESSENTRY32A;
typedef PROCESSENTRY32A *  PPROCESSENTRY32A;
typedef PROCESSENTRY32A *  LPPROCESSENTRY32A;

/////////////////////////////////
// structs and defines requiered for NtQuerySystemInformation
/////////////////////////////////
#if (!defined(STATUS_SUCCESS))
    #define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L)
#endif

typedef DWORD (__stdcall *pfNtQuerySystemInformation)
 (SYSTEM_INFORMATION_CLASS SystemInformationClass,
  PVOID SystemInformation,
  ULONG SystemInformationLength,
  PULONG ReturnLength
);
typedef enum {
StateInitialized,
StateReady,
StateRunning,
StateStandby,
StateTerminated,
StateWait,
StateTransition,
StateUnknown
} THREAD_STATE;
typedef enum _KWAIT_REASON {
Executive,
FreePage,
PageIn,
PoolAllocation,
DelayExecution,
Suspended,
UserRequest,
WrExecutive,
WrFreePage,
WrPageIn,
WrPoolAllocation,
WrDelayExecution,
WrSuspended,
WrUserRequest,
WrEventPair,
WrQueue,
WrLpcReceive,
WrLpcReply,
WrVirtualMemory,
WrPageOut,
WrRendezvous,
Spare2,
Spare3,
Spare4,
Spare5,
Spare6,
WrKernel
} KWAIT_REASON;
typedef struct _SYSTEM_THREADS {
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
    ULONG ContextSwitchCount;
    THREAD_STATE State;
    KWAIT_REASON WaitReason;
} SYSTEM_THREADS, *PSYSTEM_THREADS;
typedef struct _SYSTEM_PROCESSES { // Information Class 5
    ULONG NextEntryDelta;
    ULONG ThreadCount;
    ULONG Reserved1[6];
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ProcessName;
    KPRIORITY BasePriority;
    ULONG ProcessId;
    ULONG InheritedFromProcessId;
    ULONG HandleCount;
    ULONG Reserved2[2];
    VM_COUNTERS VmCounters;
    IO_COUNTERS IoCounters; // Windows
    SYSTEM_THREADS Threads[1];
} SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;


/////////////////////////////////
// HideMe vars
/////////////////////////////////
pfProcess32NextA pProcess32NextA;
pfProcess32NextW pProcess32NextW;
pfModule32NextA pModule32NextA;
pfModule32NextW pModule32NextW;
pfNtQuerySystemInformation pNtQuerySystemInformation;

DWORD dwExplorerID=0;
DWORD dwCurrentProcessId=0;
CProcessAndThreadID* ProcessAndThreadID=NULL;