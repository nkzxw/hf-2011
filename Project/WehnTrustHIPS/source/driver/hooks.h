/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_HOOKS_H
#define _WEHNTRUST_DRIVER_HOOKS_H

NTSTATUS InstallHooks();
NTSTATUS UninstallHooks();

BOOLEAN  IsNtCreateThreadHooked();
NTSTATUS InstallNtCreateThreadHook(
		IN PVOID NtdllImageBase,
		IN ULONG NtdllImageSize,
		IN PVOID UserModeNtCreateThread);

NTSTATUS OrigMmMapViewOfSection(
		IN PSECTION_OBJECT SectionObject,
		IN PPROCESS_OBJECT ProcessObject,
		IN OUT PVOID *BaseAddress,
		IN ULONG ZeroBits,
		IN ULONG CommitSize,
		IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
		IN OUT PSIZE_T ViewSize,
		IN SECTION_INHERIT InheritDisposition,
		IN ULONG AllocationType,
		IN ULONG Protect);
NTSTATUS OrigZwAllocateVirtualMemory(
		IN HANDLE ProcessHandle,
		IN OUT PVOID *BaseAddress,
		IN ULONG ZeroBits,
		IN OUT PULONG RegionSize,
		IN ULONG AllocationType,
		IN ULONG Protect);
NTSTATUS OrigZwSetSystemInformation(
		IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
		IN OUT PVOID SystemInformation,
		IN ULONG SystemInformationLength);
NTSTATUS OrigNtCreateThread(
		OUT PHANDLE ThreadHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN HANDLE ProcessHandle,
		OUT PCLIENT_ID ClientId,
		IN PCONTEXT ThreadContext,
		IN PUSER_STACK UserStack,
		IN BOOLEAN CreateSuspended);

//
// Non-hooked dependent routines
//
typedef NTSTATUS (NTAPI *NT_RESUME_THREAD)(
		IN HANDLE ThreadHandle,
		OUT PULONG PreviousSuspendCount OPTIONAL);
typedef NTSTATUS (NTAPI *NT_PROTECT_VIRTUAL_MEMORY)(
		IN HANDLE ProcessHandle,
		IN OUT PVOID *BaseAddress,
		IN OUT PULONG ProtectSize,
		IN ULONG NewProtect,
		OUT PULONG OldProtect);

extern NT_RESUME_THREAD NtResumeThread;
extern NT_PROTECT_VIRTUAL_MEMORY NtProtectVirtualMemory;

#endif
