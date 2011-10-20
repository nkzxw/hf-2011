/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		registry.h
 *
 * Abstract:
 *
 *		This module defines various types used by registry hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 20-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __REGISTRY_H__
#define __REGISTRY_H__


/*
 * ZwCreateKey creates or opens a registry key object. [NAR]
 */

typedef NTSTATUS (*fpZwCreateKey) (
    OUT PHANDLE  KeyHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes,
    IN ULONG  TitleIndex,
    IN PUNICODE_STRING  Class  OPTIONAL,
    IN ULONG  CreateOptions,
    OUT PULONG  Disposition  OPTIONAL
    );

NTSTATUS
NTAPI
HookedNtCreateKey(
    OUT PHANDLE  KeyHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes,
    IN ULONG  TitleIndex,
    IN PUNICODE_STRING  Class  OPTIONAL,
    IN ULONG  CreateOptions,
    OUT PULONG  Disposition  OPTIONAL
	);


/*
 * ZwOpenKey opens a registry key object. [NAR]
 */

typedef NTSTATUS (*fpZwOpenKey) (
    OUT PHANDLE  KeyHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes
    );

NTSTATUS
NTAPI
HookedNtOpenKey(
    OUT PHANDLE  KeyHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes
	);


/*
 * ZwSetValueKey updates or adds a value to a key. [NAR]
 */

typedef NTSTATUS (*fpZwSetValueKey) (
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	IN ULONG TitleIndex,
	IN ULONG Type,
	IN PVOID Data,
	IN ULONG DataSize
	);

NTSTATUS
NTAPI
HookedNtSetValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	IN ULONG TitleIndex,
	IN ULONG Type,
	IN PVOID Data,
	IN ULONG DataSize
	);


/*
 * ZwQueryValueKey retrieves information about a key value. [NAR]
 */

typedef NTSTATUS (*fpZwQueryValueKey) (
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	OUT PVOID KeyValueInformation,
	IN ULONG KeyValueInformationLength,
	OUT PULONG ResultLength
	);

NTSTATUS
NTAPI
HookedNtQueryValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	OUT PVOID KeyValueInformation,
	IN ULONG KeyValueInformationLength,
	OUT PULONG ResultLength
	);


/*
 * ZwDeleteKey deletes a key in the registry. [NAR]
 */

typedef NTSTATUS (*fpZwDeleteKey) (
	IN HANDLE KeyHandle
	);

NTSTATUS
NTAPI
HookedNtDeleteKey(
	IN HANDLE KeyHandle
	);


BOOLEAN InitRegistryHooks();


#endif	/* __REGISTRY_H__ */
