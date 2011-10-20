/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		job.c
 *
 * Abstract:
 *
 *		This module implements various job object hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 25-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include "job.h"
#include "policy.h"
#include "pathproc.h"
#include "hookproc.h"
#include "accessmask.h"
#include "learn.h"
#include "log.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitJobHooks)
#endif


fpZwCreateJobObject	OriginalNtCreateJobObject = NULL;
fpZwOpenJobObject	OriginalNtOpenJobObject = NULL;


/*
 * HookedNtCreateJobObject()
 *
 * Description:
 *		This function mediates the NtCreateJobObject() system service and checks the
 *		provided job object name against the global and current process security policies.
 *
 *		NOTE: ZwCreateJobObject creates or opens a job object. [NAR]
 *
 * Parameters:
 *		Those of NtCreateJobObject().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateJobObject().
 */

NTSTATUS
NTAPI
HookedNtCreateJobObject
(
	OUT PHANDLE JobHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
	PCHAR	FunctionName = "HookedNtCreateJobObject";


	HOOK_ROUTINE_START(JOB);


	ASSERT(OriginalNtCreateJobObject);

	rc = OriginalNtCreateJobObject(JobHandle, DesiredAccess, ObjectAttributes);


	HOOK_ROUTINE_FINISH(JOB);
}



/*
 * HookedNtOpenJobObject()
 *
 * Description:
 *		This function mediates the NtOpenJobObject() system service and checks the
 *		provided job object name against the global and current process security policies.
 *
 *		NOTE: ZwOpenJobObject opens a job object. [NAR]
 *
 * Parameters:
 *		Those of NtOpenJobObject().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtOpenJobObject().
 */

NTSTATUS
NTAPI
HookedNtOpenJobObject
(
	OUT PHANDLE JobHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
	PCHAR	FunctionName = "HookedNtOpenJobObject";


	HOOK_ROUTINE_START(JOB);


	ASSERT(OriginalNtOpenJobObject);

	rc = OriginalNtOpenJobObject(JobHandle, DesiredAccess, ObjectAttributes);


	HOOK_ROUTINE_FINISH(JOB);
}



/*
 * InitJobHooks()
 *
 * Description:
 *		Initializes all the mediated job object operation pointers. The "OriginalFunction" pointers
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
InitJobHooks()
{
	if ( (OriginalNtCreateJobObject = (fpZwCreateJobObject) ZwCalls[ZW_CREATE_JOBOBJECT_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_JOB, LOG_PRIORITY_DEBUG, ("InitJobObjectHooks: OriginalNtCreateJobObject is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtOpenJobObject = (fpZwOpenJobObject) ZwCalls[ZW_OPEN_JOBOBJECT_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_JOB, LOG_PRIORITY_DEBUG, ("InitJobObjectHooks: OriginalNtOpenJobObject is NULL\n"));
		return FALSE;
	}

	return TRUE;
}
