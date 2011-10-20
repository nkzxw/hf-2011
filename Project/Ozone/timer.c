/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		timer.c
 *
 * Abstract:
 *
 *		This module implements various timer hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 25-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "timer.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitTimerHooks)
#endif


fpZwCreateTimer		OriginalNtCreateTimer = NULL;
fpZwOpenTimer		OriginalNtOpenTimer = NULL;


/*
 * HookedNtCreateTimer()
 *
 * Description:
 *		This function mediates the NtCreateTimer() system service and checks the
 *		provided timer name against the global and current process security policies.
 *
 *		NOTE: ZwCreateTimer creates or opens a timer object. [NAR]
 *
 * Parameters:
 *		Those of NtCreateTimer().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateTimer().
 */

NTSTATUS
NTAPI
HookedNtCreateTimer
(
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN TIMER_TYPE TimerType
)
{
	PCHAR	FunctionName = "HookedNtCreateTimer";


	HOOK_ROUTINE_START(TIMER);


	ASSERT(OriginalNtCreateTimer);

	rc = OriginalNtCreateTimer(TimerHandle, DesiredAccess, ObjectAttributes, TimerType);


	HOOK_ROUTINE_FINISH(TIMER);
}



/*
 * HookedNtOpenTimer()
 *
 * Description:
 *		This function mediates the NtOpenTimer() system service and checks the
 *		provided timer name against the global and current process security policies.
 *
 *		NOTE: ZwOpenTimer opens a timer object. [NAR]
 *
 * Parameters:
 *		Those of NtOpenTimer().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtOpenTimer().
 */

NTSTATUS
NTAPI
HookedNtOpenTimer
(
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
	PCHAR	FunctionName = "HookedNtOpenTimer";


	HOOK_ROUTINE_START(TIMER);


	ASSERT(OriginalNtOpenTimer);

	rc = OriginalNtOpenTimer(TimerHandle, DesiredAccess, ObjectAttributes);


	HOOK_ROUTINE_FINISH(TIMER);
}



/*
 * InitTimerHooks()
 *
 * Description:
 *		Initializes all the mediated timer operation pointers. The "OriginalFunction" pointers
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
InitTimerHooks()
{
	if ( (OriginalNtCreateTimer = (fpZwCreateTimer) ZwCalls[ZW_CREATE_TIMER_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_TIMER, LOG_PRIORITY_DEBUG, ("InitTimerHooks: OriginalNtCreateTimer is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtOpenTimer = (fpZwOpenTimer) ZwCalls[ZW_OPEN_TIMER_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_TIMER, LOG_PRIORITY_DEBUG, ("InitTimerHooks: OriginalNtOpenTimer is NULL\n"));
		return FALSE;
	}

	return TRUE;
}
