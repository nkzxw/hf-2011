/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		registry.c
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


#include <NTDDK.h>
#include "registry.h"
#include "policy.h"
#include "pathproc.h"
#include "hookproc.h"
#include "accessmask.h"
#include "learn.h"
#include "misc.h"
#include "log.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitRegistryHooks)
#endif


fpZwCreateKey		OriginalNtCreateKey = NULL;
fpZwOpenKey			OriginalNtOpenKey = NULL;

fpZwDeleteKey		OriginalNtDeleteKey = NULL;

fpZwSetValueKey		OriginalNtSetValueKey = NULL;
fpZwQueryValueKey	OriginalNtQueryValueKey = NULL;



/*
 * HookedNtCreateKey()
 *
 * Description:
 *		This function mediates the NtCreateKey() system service and checks the
 *		provided registry key against the global and current process security policies.
 *
 *		NOTE: ZwCreateKey creates or opens a registry key object. [NAR]
 *
 * Parameters:
 *		Those of NtCreateKey().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateKey().
 */

NTSTATUS
NTAPI
HookedNtCreateKey
(
    OUT PHANDLE  KeyHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes,
    IN ULONG  TitleIndex,
    IN PUNICODE_STRING  Class  OPTIONAL,
    IN ULONG  CreateOptions,
    OUT PULONG  Disposition  OPTIONAL
)
{
	PCHAR	FunctionName = "HookedNtCreateKey";


	HOOK_ROUTINE_START(REGISTRY);


	ASSERT(OriginalNtOpenKey);

	rc = OriginalNtCreateKey(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex,
								Class, CreateOptions, Disposition);


	HOOK_ROUTINE_FINISH(REGISTRY);
}



/*
 * HookedNtOpenKey()
 *
 * Description:
 *		This function mediates the NtOpenKey() system service and checks the
 *		provided registry key against the global and current process security policies.
 *
 *		NOTE: ZwOpenKey opens a registry key object. [NAR]
 *
 * Parameters:
 *		Those of NtOpenKey().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtOpenKey().
 */

NTSTATUS
NTAPI
HookedNtOpenKey
(
    OUT PHANDLE  KeyHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes
)
{
	PCHAR	FunctionName = "HookedNtOpenKey";


	HOOK_ROUTINE_START(REGISTRY);


	ASSERT(OriginalNtOpenKey);

	rc = OriginalNtOpenKey(KeyHandle, DesiredAccess, ObjectAttributes);


	HOOK_ROUTINE_FINISH(REGISTRY);
}



/*
 * HookedNtDeleteKey()
 *
 * Description:
 *		This function mediates the NtDeleteKey() system service and checks the
 *		provided registry key against the global and current process security policies.
 *
 *		NOTE: ZwDeleteKey deletes a key in the registry. [NAR]
 *
 * Parameters:
 *		Those of NtDeleteKey().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtDeleteKey().
 */

NTSTATUS
NTAPI
HookedNtDeleteKey
(
	IN HANDLE KeyHandle
)
{
	PCHAR			FunctionName = "HookedNtDeleteKey";
	CHAR			REGISTRYNAME[MAX_PATH];
	WCHAR			REGISTRYNAMEW[MAX_PATH];
	PWSTR			KeyName = NULL;


	HOOK_ROUTINE_ENTER();


	if ((KeyName = GetNameFromHandle(KeyHandle, REGISTRYNAMEW, sizeof(REGISTRYNAMEW))) != NULL)
	{
		sprintf(REGISTRYNAME, "%S", KeyName);

		LOG(LOG_SS_REGISTRY, LOG_PRIORITY_VERBOSE, ("%d %s: %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, REGISTRYNAME));


		if (LearningMode == FALSE)
		{
			POLICY_CHECK_OPTYPE_NAME(REGISTRY, OP_DELETE);
		}
	}


	ASSERT(OriginalNtDeleteKey);

	rc = OriginalNtDeleteKey(KeyHandle);


	if (KeyName)
	{
		HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(REGISTRY, REGISTRYNAME, OP_DELETE);
	}
	else
	{
		HOOK_ROUTINE_EXIT(rc);
	}
}



/*
 * HookedNtSetValueKey()
 *
 * Description:
 *		This function mediates the NtSetValueKey() system service and checks the
 *		provided registry key against the global and current process security policies.
 *
 *		NOTE: ZwSetValueKey updates or adds a value to a key. [NAR]
 *
 * Parameters:
 *		Those of NtSetValueKey().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtSetValueKey().
 */

NTSTATUS
NTAPI
HookedNtSetValueKey
(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	IN ULONG TitleIndex,
	IN ULONG Type,
	IN PVOID Data,
	IN ULONG DataSize
)
{
	CHAR			REGISTRYNAME[MAX_PATH];
	WCHAR			REGISTRYNAMEW[MAX_PATH];
	PCHAR			FunctionName = "HookedNtSetValueKey";
	UNICODE_STRING	usValueName;
	PWSTR			KeyName = NULL;


	HOOK_ROUTINE_ENTER();


	if (VerifyUnicodeString(ValueName, &usValueName) == TRUE)
	{
		if ((KeyName = GetNameFromHandle(KeyHandle, REGISTRYNAMEW, sizeof(REGISTRYNAMEW))) != NULL)
		{
			_snprintf(REGISTRYNAME, MAX_PATH, "%S\\%S", KeyName, ValueName->Buffer);
			REGISTRYNAME[MAX_PATH - 1] = 0;

			LOG(LOG_SS_REGISTRY, LOG_PRIORITY_VERBOSE, ("%d %s: %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, REGISTRYNAME));


			if (LearningMode == FALSE)
			{
				POLICY_CHECK_OPTYPE_NAME(REGISTRY, OP_WRITE);
			}
		}
	}


	ASSERT(OriginalNtSetValueKey);

	rc = OriginalNtSetValueKey(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);


	if (KeyName)
	{
		HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(REGISTRY, REGISTRYNAME, OP_WRITE);
	}
	else
	{
		HOOK_ROUTINE_EXIT(rc);
	}
}



/*
 * HookedNtQueryValueKey()
 *
 * Description:
 *		This function mediates the NtQueryValueKey() system service and checks the
 *		provided registry key against the global and current process security policies.
 *
 *		NOTE: ZwQueryValueKey retrieves information about a key value. [NAR]
 *
 * Parameters:
 *		Those of NtQueryValueKey().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtQueryValueKey().
 */

NTSTATUS
NTAPI
HookedNtQueryValueKey
(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	OUT PVOID KeyValueInformation,
	IN ULONG KeyValueInformationLength,
	OUT PULONG ResultLength
)
{
	CHAR			REGISTRYNAME[MAX_PATH];
	WCHAR			REGISTRYNAMEW[MAX_PATH];
	PCHAR			FunctionName = "HookedNtQueryValueKey";
	UNICODE_STRING	usValueName;
	PWSTR			KeyName = NULL;


	HOOK_ROUTINE_ENTER();


	if (VerifyUnicodeString(ValueName, &usValueName) == TRUE)
	{
		if ((KeyName = GetNameFromHandle(KeyHandle, REGISTRYNAMEW, sizeof(REGISTRYNAMEW))) != NULL)
		{
			_snprintf(REGISTRYNAME, MAX_PATH, "%S\\%S", KeyName, ValueName->Buffer);
			REGISTRYNAME[MAX_PATH - 1] = 0;

			LOG(LOG_SS_REGISTRY, LOG_PRIORITY_VERBOSE, ("%d %s: %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, REGISTRYNAME));


			if (LearningMode == FALSE)
			{
				POLICY_CHECK_OPTYPE_NAME(REGISTRY, OP_READ);
			}
		}
	}


	ASSERT(OriginalNtQueryValueKey);

	rc = OriginalNtQueryValueKey(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation,
									KeyValueInformationLength, ResultLength);


	if (KeyName)
	{
		HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(REGISTRY, REGISTRYNAME, OP_READ);
	}
	else
	{
		HOOK_ROUTINE_EXIT(rc);
	}
}



/*
 * InitRegistryHooks()
 *
 * Description:
 *		Initializes all the mediated registry operation pointers. The "OriginalFunction" pointers
 *		are initialized by InstallSyscallsHooks() that must be called prior to this function.
 *
 *		NOTE: Called once during driver initialization (DriverEntry()).
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
InitRegistryHooks()
{
	if ( (OriginalNtCreateKey = (fpZwCreateKey) ZwCalls[ZW_CREATE_KEY_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_REGISTRY, LOG_PRIORITY_DEBUG, ("InitRegistryHooks: OriginalNtCreateKey is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtOpenKey = (fpZwOpenKey) ZwCalls[ZW_OPEN_KEY_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_REGISTRY, LOG_PRIORITY_DEBUG, ("InitRegistryHooks: OriginalNtOpenKey is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtDeleteKey = (fpZwDeleteKey) ZwCalls[ZW_DELETE_KEY_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_REGISTRY, LOG_PRIORITY_DEBUG, ("InitRegistryHooks: OriginalNtDeleteKey is NULL\n"));
		return FALSE;
	}

//	XXX ZwDeleteValueKey
/*
	if ( (OriginalNtSetValueKey = (fpZwSetValueKey) ZwCalls[ZW_SET_VALUE_KEY_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_REGISTRY, LOG_PRIORITY_DEBUG, ("InitRegistryHooks: OriginalNtSetValueKey is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtQueryValueKey = (fpZwQueryValueKey) ZwCalls[ZW_QUERY_VALUE_KEY_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_REGISTRY, LOG_PRIORITY_DEBUG, ("InitRegistryHooks: OriginalNtQueryValueKey is NULL\n"));
		return FALSE;
	}
*/
	return TRUE;
}
