/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		token.h
 *
 * Abstract:
 *
 *		This module defines various types used by token hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 25-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __TOKEN_H__
#define __TOKEN_H__


#include <NTDDK.h>
#include "policy.h"
#include "pathproc.h"
#include "hookproc.h"
#include "procname.h"
#include "learn.h"
#include "log.h"


/*
ZwAdjustGroupsToken
ZwCreateToken
ZwOpenProcessToken
ZwOpenProcessTokenEx
ZwOpenThreadToken
ZwOpenThreadTokenEx
*/


typedef struct _TOKEN_PRIVILEGES {
    DWORD PrivilegeCount;
    LUID_AND_ATTRIBUTES Privileges[ANYSIZE_ARRAY];
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES;


/*
 * ZwAdjustPrivilegesToken adjusts the attributes of the privileges in a token. [NAR]
 */

typedef NTSTATUS (*fpZwAdjustPrivilegesToken) (
	IN HANDLE TokenHandle,
	IN BOOLEAN DisableAllPrivileges,
	IN PTOKEN_PRIVILEGES NewState,
	IN ULONG BufferLength,
	OUT PTOKEN_PRIVILEGES PreviousState OPTIONAL,
	OUT PULONG ReturnLength
	);

NTSTATUS
NTAPI
HookedNtAdjustPrivilegesToken(
	IN HANDLE TokenHandle,
	IN BOOLEAN DisableAllPrivileges,
	IN PTOKEN_PRIVILEGES NewState,
	IN ULONG BufferLength,
	OUT PTOKEN_PRIVILEGES PreviousState OPTIONAL,
	OUT PULONG ReturnLength
	);


/*
 * ZwSetInformationToken sets information affecting a token object. [NAR]
 */

typedef NTSTATUS (*fpZwSetInformationToken) (
	IN HANDLE TokenHandle,
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	IN PVOID TokenInformation,
	IN ULONG TokenInformationLength
	);

NTSTATUS
NTAPI
HookedNtSetInformationToken(
	IN HANDLE TokenHandle,
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	IN PVOID TokenInformation,
	IN ULONG TokenInformationLength
	);


/*
 * ZwOpenProcessToken opens the token of a process. [NAR]
 */

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessToken(
	IN HANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	OUT PHANDLE TokenHandle
	);


/*
 * ZwOpenThreadToken opens the token of a thread. [NAR]
 */

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadToken(
	IN HANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN BOOLEAN OpenAsSelf,
	OUT PHANDLE TokenHandle
	);


/*
 * ZwQueryInformationToken retrieves information about a token object. [NAR]
 */

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationToken(
	IN HANDLE TokenHandle,
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	OUT PVOID TokenInformation,
	IN ULONG TokenInformationLength,
	OUT PULONG ReturnLength
	);


BOOLEAN InitTokenHooks();


#endif	/* __TOKEN_H__ */
