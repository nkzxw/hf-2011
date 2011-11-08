//======================================================================
// 
// Ia64stub.h
//
// Copyright (C) 1996-2002 Mark Russinovich and Bryce Cogswell
// Sysinternals - wwww.sysinternals.com
//
//======================================================================


//
// Defined in readgp.s
//
ULONG64
ReadGpRegister(
    VOID
    );

//
// Defined in patchstb.c
//
VOID
PatchStub( 
    ULONGLONG GpReg,
    PVOID StubProc 
    );

//
// Stubs defined in ia64stub.s
//

NTSTATUS 
NTAPI
StubHookRegOpenKey( 
    IN OUT PHANDLE pHandle, 
    IN ACCESS_MASK ReqAccess, 
    IN POBJECT_ATTRIBUTES pOpenInfo 
    );
NTSTATUS 
NTAPI
HookRegOpenKey( 
    IN OUT PHANDLE pHandle, 
    IN ACCESS_MASK ReqAccess, 
    IN POBJECT_ATTRIBUTES pOpenInfo 
    );
NTSTATUS 
NTAPI
StubHookRegCreateKey( 
    OUT PHANDLE pHandle, 
    IN ACCESS_MASK ReqAccess,
    IN POBJECT_ATTRIBUTES pOpenInfo, 
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class, 
    IN ULONG CreateOptions, 
    OUT PULONG Disposition 
    );
NTSTATUS 
NTAPI
HookRegCreateKey( 
    OUT PHANDLE pHandle, 
    IN ACCESS_MASK ReqAccess,
    IN POBJECT_ATTRIBUTES pOpenInfo, 
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class, 
    IN ULONG CreateOptions, 
    OUT PULONG Disposition 
    );
NTSTATUS 
NTAPI
StubHookRegCloseKey( 
    IN HANDLE Handle 
    );
NTSTATUS 
NTAPI
HookRegCloseKey( 
    IN HANDLE Handle 
    );
NTSTATUS 
NTAPI
StubHookRegFlushKey( 
    IN HANDLE Handle 
    );
NTSTATUS 
NTAPI
HookRegFlushKey( 
    IN HANDLE Handle 
    );
NTSTATUS 
NTAPI
StubHookRegDeleteKey( 
    IN HANDLE Handle 
    );
NTSTATUS 
NTAPI
HookRegDeleteKey( 
    IN HANDLE Handle 
    );
NTSTATUS 
NTAPI
StubHookRegDeleteValueKey( 
    IN HANDLE Handle, 
    PUNICODE_STRING Name 
    );
NTSTATUS 
NTAPI
HookRegDeleteValueKey( 
    IN HANDLE Handle, 
    PUNICODE_STRING Name 
    );
NTSTATUS 
NTAPI
StubHookRegSetValueKey( 
    IN HANDLE KeyHandle, 
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex, 
    IN ULONG Type, 
    IN PVOID Data, 
    IN ULONG DataSize 
    );
NTSTATUS 
NTAPI
HookRegSetValueKey( 
    IN HANDLE KeyHandle, 
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex, 
    IN ULONG Type, 
    IN PVOID Data, 
    IN ULONG DataSize 
    );
NTSTATUS 
NTAPI
StubHookRegEnumerateKey( 
    IN HANDLE KeyHandle, 
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation, 
    IN ULONG Length, 
    OUT PULONG pResultLength 
    );
NTSTATUS 
NTAPI
HookRegEnumerateKey( 
    IN HANDLE KeyHandle, 
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation, 
    IN ULONG Length, 
    OUT PULONG pResultLength 
    );
NTSTATUS 
NTAPI
StubHookRegQueryKey( 
    IN HANDLE  KeyHandle, 
    IN KEY_INFORMATION_CLASS  KeyInformationClass,
    OUT PVOID  KeyInformation, 
    IN ULONG  Length, 
    OUT PULONG  pResultLength 
    );
NTSTATUS 
NTAPI
HookRegQueryKey( 
    IN HANDLE  KeyHandle, 
    IN KEY_INFORMATION_CLASS  KeyInformationClass,
    OUT PVOID  KeyInformation, 
    IN ULONG  Length, 
    OUT PULONG  pResultLength 
    );
NTSTATUS 
NTAPI
StubHookRegEnumerateValueKey( 
    IN HANDLE KeyHandle, 
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation, 
    IN ULONG Length,
    OUT PULONG  pResultLength 
    );
NTSTATUS 
NTAPI
HookRegEnumerateValueKey( 
    IN HANDLE KeyHandle, 
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation, 
    IN ULONG Length,
    OUT PULONG  pResultLength 
    );
NTSTATUS 
NTAPI
StubHookRegQueryValueKey( 
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation, 
    IN ULONG Length,
    OUT PULONG  pResultLength 
    );
NTSTATUS 
NTAPI
HookRegQueryValueKey( 
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation, 
    IN ULONG Length,
    OUT PULONG  pResultLength 
    );
NTSTATUS 
NTAPI
StubHookRegLoadKey( 
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES HiveFile 
    );
NTSTATUS 
NTAPI
HookRegLoadKey( 
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES HiveFile 
    );
NTSTATUS 
NTAPI
StubHookRegUnloadKey( 
    IN POBJECT_ATTRIBUTES TargetKey 
    );
NTSTATUS 
NTAPI
HookRegUnloadKey( 
    IN POBJECT_ATTRIBUTES TargetKey 
    );


