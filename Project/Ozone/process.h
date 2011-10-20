/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		process.h
 *
 * Abstract:
 *
 *		This module defines various types used by process and thread hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 23-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */

#ifndef __PROCESS_H__
#define __PROCESS_H__


extern ULONG			SystemProcessId;

extern WCHAR			OzoneInstallPath[];
extern USHORT			OzoneInstallPathSize;


/*
 * ZwCreateProcess creates a process object. [NAR]
 */

typedef NTSTATUS (*fpZwCreateProcess) (
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE InheritFromProcessHandle,
	IN BOOLEAN InheritHandles,
	IN HANDLE SectionHandle OPTIONAL,
	IN HANDLE DebugPort OPTIONAL,
	IN HANDLE ExceptionPort OPTIONAL
	);

NTSTATUS
NTAPI
HookedNtCreateProcess(
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE InheritFromProcessHandle,
	IN BOOLEAN InheritHandles,
	IN HANDLE SectionHandle OPTIONAL,
	IN HANDLE DebugPort OPTIONAL,
	IN HANDLE ExceptionPort OPTIONAL
	);


typedef NTSTATUS (*fpZwCreateProcessEx) (
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE InheritFromProcessHandle,
	IN ULONG Unknown1,
	IN HANDLE SectionHandle OPTIONAL,
	IN HANDLE DebugPort OPTIONAL,
	IN HANDLE ExceptionPort OPTIONAL,
	IN ULONG Unknown2
	);

NTSTATUS
NTAPI
HookedNtCreateProcessEx(
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE InheritFromProcessHandle,
	IN ULONG Unknown1,
	IN HANDLE SectionHandle OPTIONAL,
	IN HANDLE DebugPort OPTIONAL,
	IN HANDLE ExceptionPort OPTIONAL,
	IN ULONG Unknown2
	);


/*
 * ZwOpenProcess opens a process object. [NAR]
 */

typedef NTSTATUS (*fpZwOpenProcess) (
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId OPTIONAL
	);

NTSTATUS
NTAPI
HookedNtOpenProcess(
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId OPTIONAL
	);


/*
 * ZwCreateThread creates a thread in a process. [NAR]
 */

typedef struct _USER_STACK {
	PVOID FixedStackBase;
	PVOID FixedStackLimit;
	PVOID ExpandableStackBase;
	PVOID ExpandableStackLimit;
	PVOID ExpandableStackBottom;
} USER_STACK, *PUSER_STACK;

typedef NTSTATUS (*fpZwCreateThread) (
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE ProcessHandle,
	OUT PCLIENT_ID ClientId,
	IN PCONTEXT ThreadContext,
	IN PUSER_STACK UserStack,
	IN BOOLEAN CreateSuspended
	);

NTSTATUS
NTAPI
HookedNtCreateThread(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE ProcessHandle,
	OUT PCLIENT_ID ClientId,
	IN PCONTEXT ThreadContext,
	IN PUSER_STACK UserStack,
	IN BOOLEAN CreateSuspended
	);


/*
 * ZwOpenThread opens a thread object. [NAR]
 */

typedef NTSTATUS (*fpZwOpenThread) (
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId
	);

NTSTATUS
NTAPI
HookedNtOpenThread(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId
	);


/*
 * ZwAllocateVirtualMemory allocates virtual memory in the user mode address range. [NAR]
 */

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateVirtualMemory(
	IN HANDLE ProcessHandle,
	IN OUT PVOID *BaseAddress,
	IN ULONG ZeroBits,
	IN OUT PULONG AllocationSize,
	IN ULONG AllocationType,
	IN ULONG Protect
	);


/*
 * ZwQueryInformationProcess retrieves information about a process object. [NAR]
 */

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);


VOID
KeAttachProcess(
    IN /*PRKPROCESS*/ PVOID Process
    );

VOID
KeDetachProcess (
    VOID
    );


BOOLEAN	InitProcessEntries();
VOID	RemoveProcessEntries();
VOID	ProcessPostBootup();


#endif	/* __PROCESS_H__ */