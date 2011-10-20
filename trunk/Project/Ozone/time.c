/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		time.c
 *
 * Abstract:
 *
 *		This module defines various routines used for hooking time routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 10-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include "time.h"
#include "hookproc.h"
#include "procname.h"
#include "learn.h"
#include "misc.h"
#include "log.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitTimeHooks)
#endif


fpZwSetSystemTime			OriginalNtSetSystemTime = NULL;
fpZwSetTimerResolution		OriginalNtSetTimerResolution = NULL;



/*
 * HookedNtSetSystemTime()
 *
 * Description:
 *		This function mediates the NtSetSystemTime() system service and disallows applications
 *		to change the system time.
 *
 *		NOTE: ZwSetSystemTime sets the system time. [NAR]
 *
 * Parameters:
 *		Those of NtSetSystemTime().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if time changing is disabled.
 *		Otherwise, NTSTATUS returned by NtSetSystemTime().
 */

NTSTATUS
NTAPI
HookedNtSetSystemTime
(
	IN PLARGE_INTEGER NewTime,
	OUT PLARGE_INTEGER OldTime OPTIONAL
)
{
	PCHAR			FunctionName = "HookedNtSetSystemTime";
	PCHAR			TIMENAME = NULL;	/* allow the use of POLICY_CHECK_OPTYPE_NAME() macro */


	HOOK_ROUTINE_ENTER();


	LOG(LOG_SS_TIME, LOG_PRIORITY_DEBUG, ("%d (%S) HookedNtSetSystemTime\n", (ULONG) PsGetCurrentProcessId(), GetCurrentProcessName()));


	/* NOTE: same code is replicated in sysinfo.c */

	if (LearningMode == FALSE)
	{
		POLICY_CHECK_OPTYPE_NAME(TIME, OP_TIME_CHANGE);
	}


	ASSERT(OriginalNtSetSystemTime);

	rc = OriginalNtSetSystemTime(NewTime, OldTime);


	if (LearningMode == TRUE)
	{
		AddRule(RULE_TIME, NULL, OP_TIME_CHANGE);
	}

	HOOK_ROUTINE_EXIT(rc);
}



/*
 * HookedNtSetTimerResolution()
 *
 * Description:
 *		This function mediates the NtSetTimerResolution() system service and disallows applications
 *		to change the system time.
 *
 *		NOTE: ZwSetTimerResolution sets the resolution of the system timer. [NAR]
 *
 * Parameters:
 *		Those of NtSetTimerResolution().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if time changing is disabled.
 *		Otherwise, NTSTATUS returned by NtSetTimerResolution().
 */

NTSTATUS
NTAPI
HookedNtSetTimerResolution
(
	IN ULONG RequestedResolution,
	IN BOOLEAN Set,
	OUT PULONG ActualResolution
)
{
	PCHAR			FunctionName = "HookedNtSetTimerResolution";
	PCHAR			TIMENAME = NULL;	/* allow the use of POLICY_CHECK_OPTYPE_NAME() macro */


	HOOK_ROUTINE_ENTER();


	LOG(LOG_SS_TIME, LOG_PRIORITY_DEBUG, ("%d (%S) HookedNtSetTimerResolution\n", (ULONG) PsGetCurrentProcessId(), GetCurrentProcessName()));


	if (LearningMode == FALSE)
	{
		POLICY_CHECK_OPTYPE_NAME(TIME, OP_TIME_CHANGE);
	}


	ASSERT(OriginalNtSetTimerResolution);

	rc = OriginalNtSetTimerResolution(RequestedResolution, Set, ActualResolution);


	if (LearningMode == TRUE)
	{
		AddRule(RULE_TIME, NULL, OP_TIME_CHANGE);
	}

	HOOK_ROUTINE_EXIT(rc);
}



/*
 * InitTimeHooks()
 *
 * Description:
 *		Initializes all the mediated time operation pointers. The "OriginalFunction" pointers
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
InitTimeHooks()
{
	if ((OriginalNtSetSystemTime = (fpZwSetSystemTime) ZwCalls[ZW_SET_SYSTEM_TIME_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_TIME, LOG_PRIORITY_DEBUG, ("InitTimeHooks: OriginalNtSetSystemTime is NULL\n"));
		return FALSE;
	}

	/* a lot of applications seem to be calling this function thus don't intercept it */
/*
	if ((OriginalNtSetTimerResolution = (fpZwSetTimerResolution) ZwCalls[ZW_SET_TIMER_RESOLUTION_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_TIME, LOG_PRIORITY_DEBUG, ("InitTimeHooks: OriginalNtSetTimerResolution is NULL\n"));
		return FALSE;
	}
*/
	return TRUE;
}
