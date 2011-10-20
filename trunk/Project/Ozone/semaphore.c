/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		semaphore.c
 *
 * Abstract:
 *
 *		This module implements various semaphore hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 09-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include "semaphore.h"
#include "policy.h"
#include "pathproc.h"
#include "hookproc.h"
#include "accessmask.h"
#include "learn.h"
#include "log.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitSemaphoreHooks)
#endif


fpZwCreateSemaphore	OriginalNtCreateSemaphore = NULL;
fpZwOpenSemaphore	OriginalNtOpenSemaphore = NULL;



/*
 * HookedNtCreateSemaphore()
 *
 * Description:
 *		This function mediates the NtCreateSemaphore() system service and checks the
 *		provided semaphore name against the global and current process security policies.
 *
 *		NOTE: ZwOpenSemaphore opens a semaphore object. [NAR]
 *
 * Parameters:
 *		Those of NtCreateSemaphore().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateSemaphore().
 */

NTSTATUS
NTAPI
HookedNtCreateSemaphore
(
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN LONG InitialCount,
	IN LONG MaximumCount
)
{
	PCHAR	FunctionName = "HookedNtCreateSemaphore";


	HOOK_ROUTINE_START(SEMAPHORE);


	ASSERT(OriginalNtCreateSemaphore);

	rc = OriginalNtCreateSemaphore(SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount);


	HOOK_ROUTINE_FINISH(SEMAPHORE);
}




/*
 * HookedNtOpenSemaphore()
 *
 * Description:
 *		This function mediates the NtOpenSemaphore() system service and checks the
 *		provided semaphore name against the global and current process security policies.
 *
 *		NOTE: ZwOpenSemaphore opens a semaphore object. [NAR]
 *
 * Parameters:
 *		Those of NtOpenSemaphore().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtOpenSemaphore().
 */

NTSTATUS
NTAPI
HookedNtOpenSemaphore
(
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
	PCHAR	FunctionName = "HookedNtOpenSemaphore";


	HOOK_ROUTINE_START(SEMAPHORE);


	ASSERT(OriginalNtOpenSemaphore);

	rc = OriginalNtOpenSemaphore(SemaphoreHandle, DesiredAccess, ObjectAttributes);


	HOOK_ROUTINE_FINISH(SEMAPHORE);
}



/*
 * InitSemaphoreHooks()
 *
 * Description:
 *		Initializes all the mediated semaphore operation pointers. The "OriginalFunction" pointers
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
InitSemaphoreHooks()
{
	if ( (OriginalNtCreateSemaphore = (fpZwCreateSemaphore) ZwCalls[ZW_CREATE_SEMAPHORE_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_SEMAPHORE, LOG_PRIORITY_DEBUG, ("InstallSemaphoreHooks: OriginalNtCreateSemaphore is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtOpenSemaphore = (fpZwOpenSemaphore) ZwCalls[ZW_OPEN_SEMAPHORE_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_SEMAPHORE, LOG_PRIORITY_DEBUG, ("InstallSemaphoreHooks: OriginalNtOpenSemaphore is NULL\n"));
		return FALSE;
	}

	return TRUE;
}
