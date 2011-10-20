/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#ifndef _WEHNTRUST_NRER_NATIVE_H
#define _WEHNTRUST_NRER_NATIVE_H

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0
#endif

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (Status == 0)
#endif

#define FILE_OPEN                       0x00000001
#define FILE_SYNCHRONOUS_IO_ALERT       0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT    0x00000020

typedef enum _MEMORY_INFORMATION_CLASS
{
	MemoryBasicInformation,
	MemoryWorkingSetList,
	MemorySectionName,
	MemoryBasicVlmInformation
} MEMORY_INFORMATION_CLASS, *PMEMORY_INFORMATION_CLASS;

typedef enum _PROCESSINFOCLASS
{
	ProcessBasicInformation = 0,
	ProcessImageFileName    = 27
} PROCESSINFOCLASS, *PPROCESSINFOCLASS;

typedef enum _FILE_INFORMATION_CLASS 
{
	FilePipeInformation = 23
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _FILE_PIPE_INFORMATION 
{
   ULONG ReadModeMessage;
   ULONG WaitModeBlocking;
} FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;

typedef struct _PROCESS_BASIC_INFORMATION
{
	ULONG  ExitStatus;
	PVOID  PebBaseAddress;
	ULONG  AffinityMask;
	ULONG  BasePriority;
	HANDLE UniqueProcessId;
	HANDLE InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _IO_STATUS_BLOCK 
{
    union 
	 {
        ULONG Status;
        PVOID Pointer;
    };
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef struct _OBJECT_ATTRIBUTES 
{
   ULONG Length;
   HANDLE RootDirectory;
   PUNICODE_STRING ObjectName;
   ULONG Attributes;
   PVOID SecurityDescriptor;
   PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef enum _NRE_EVENT_TYPE
{
	NreErrorEvent
} NRE_EVENT_TYPE, *PNRE_EVENT_TYPE;

// 
// Direct native API
//
ULONG NtContinue(
		IN PCONTEXT Context,
		IN BOOLEAN RemoveAlert);
HANDLE NreGetProcessHeap();

//
// Memory allocation
//
PVOID NreAllocate(
		IN ULONG Length);
PVOID NreAllocateNoAccess(
		IN PVOID Address,
		IN ULONG Length);
ULONG NreFree(
		IN PVOID Buffer);

//
// Library loading & interaction
//
ULONG NreLoadLibrary(
		IN PWCHAR ModuleFileName,
		OUT HMODULE *ModuleHandle);
ULONG NreGetProcAddress(
		IN HMODULE ModuleHandle,
		IN PCHAR SymbolName,
		OUT PVOID *SymbolAddress);
ULONG NreGetModuleHandle(
		IN PWCHAR ModuleFileName,
		OUT HMODULE *ModuleHandle);
ULONG NreFreeLibrary(
		IN HMODULE ModuleHandle);

//
// Memory management
//
ULONG NreQueryVirtualMemory(
		IN PVOID BaseAddress,
		IN MEMORY_INFORMATION_CLASS InformationClass,
		OUT PVOID MemoryInformation,
		IN ULONG MemoryInformationLength,
		OUT PULONG ReturnLength OPTIONAL);
ULONG NreProtectVirtualMemory(
		IN PVOID BaseAddress,
		IN ULONG RegionSize,
		IN ULONG Protection,
		OUT PULONG OldProtection OPTIONAL);

//
// Process
//
ULONG NreExitWithMethod(
		IN SELECTED_EXIT_METHOD ExitMethod);
ULONG NreExitProcess(
		IN ULONG ExitCode);
ULONG NreExitThread(
		IN ULONG ExitCode);
ULONG NreGetCurrentProcessInformation(
		OUT PULONG ProcessId,
		OUT PWCHAR ProcessFileName,
		IN ULONG ProcessFileNameLength);
PVOID NreGetProcessParameters();

//
// Logging
//
ULONG NreLogExploitationEvent(
		IN PEXPLOIT_INFORMATION ExploitInformation,
		OUT PWEHNSERV_RESPONSE Response OPTIONAL);
ULONG NreSendRequest(
		IN PWEHNSERV_REQUEST Request,
		OUT PWEHNSERV_RESPONSE Response OPTIONAL);

#if 0
ULONG NreLogEvent(
		IN NRE_EVENT_TYPE EventType,
		IN WORD CatId,
		IN DWORD EventId,
		IN WORD NumberOfStrings,
		IN DWORD DataSize,
		IN LPCWSTR *String,
		LPVOID RawData OPTIONAL);
#endif

//
// Internal routines
// 
VOID NreHookEnforcementRoutines();
ULONG NreHookRoutine(
		IN PVOID FunctionToHook,
		IN PVOID HookFunction,
		IN BOOLEAN UseHeapForStorage,
		OUT PVOID *OrigFunction);

//
// Symbol resolution
//
#define RESOLVE_SYMBOL_RV(Lib, SymbolName, P)  \
	if (!SymbolName##Proc)                      \
	{                                           \
		ULONG Result;                            \
		if ((Result = ResolveDependentSymbol(    \
				Lib,                               \
				#SymbolName,                       \
				(PVOID *)&SymbolName##Proc)) != 0) \
			return (P)Result;                     \
	}
#define RESOLVE_SYMBOL(Lib, SymbolName)        \
	RESOLVE_SYMBOL_RV(Lib, SymbolName, ULONG)

ULONG ResolveDependentSymbol(
		IN PWCHAR ModuleFileName,
		IN PCHAR SymbolName,
		OUT PVOID *SymbolAddress);

#endif
