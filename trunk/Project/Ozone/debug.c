/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		debug.c
 *
 * Abstract:
 *
 *		This module implements various debug hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 23-Apr-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include "debug.h"
#include "hookproc.h"
#include "procname.h"
#include "learn.h"
#include "log.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitDebugHooks)
#endif


fpZwDebugActiveProcess	OriginalNtDebugActiveProcess = NULL;


//XXX http://www.nsfocus.net/index.php?act=magazine&do=view&mid=2108


/*
 * IsDebuggingAllowed()
 *
 * Description:
 *		Check whether the current process is allowed to use debugging functionality.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		FALSE if debugging is disabled. TRUE otherwise.
 */

BOOLEAN
IsDebuggingAllowed()
{
	PIMAGE_PID_ENTRY	CurrentProcess;
	BOOLEAN				DebuggingAllowed = FALSE;


	/* check the global policy first */
	if (! IS_DEBUGGING_PROTECTION_ON(gSecPolicy))
		return TRUE;


	/* now check the process specific policy */
	CurrentProcess = FindImagePidEntry(CURRENT_PROCESS_PID, 0);

	if (CurrentProcess != NULL)
	{
		DebuggingAllowed = ! IS_DEBUGGING_PROTECTION_ON(CurrentProcess->SecPolicy);
	}
	else
	{
		LOG(LOG_SS_DEBUG, LOG_PRIORITY_DEBUG, ("%d IsDebuggingAllowed: CurrentProcess = NULL!\n", CURRENT_PROCESS_PID));
	}


	return DebuggingAllowed;
}



/*
 * HookedNtDebugActiveProcess()
 *
 * Description:
 *		This function mediates the NtDebugActiveProcess() system service and disallows
 *		debugging.
 *
 * Parameters:
 *		Those of NtDebugActiveProcess().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtDebugActiveProcess().
 */

NTSTATUS
NTAPI
HookedNtDebugActiveProcess
(
	UINT32	Unknown1,
	UINT32	Unknown2
)
{
	HOOK_ROUTINE_ENTER();


	LOG(LOG_SS_DEBUG, LOG_PRIORITY_DEBUG, ("HookedNtDebugActiveProcess(%x %x)\n", Unknown1, Unknown2));

	if (LearningMode == FALSE && IsDebuggingAllowed() == FALSE)
	{
		LOG(LOG_SS_DEBUG, LOG_PRIORITY_DEBUG, ("%d (%S) HookedNtDebugActiveProcess: disallowing debugging\n", (ULONG) PsGetCurrentProcessId(), GetCurrentProcessName()));

		LogAlert(ALERT_SS_DEBUG, OP_DEBUG, ALERT_RULE_NONE, ACTION_DENY, ALERT_PRIORITY_MEDIUM, NULL, 0, NULL);

		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}


	ASSERT(OriginalNtDebugActiveProcess);

	rc = OriginalNtDebugActiveProcess(Unknown1, Unknown2);


	if (LearningMode == TRUE)
		TURN_DEBUGGING_PROTECTION_OFF(NewPolicy);


	HOOK_ROUTINE_EXIT(rc);
}



/*
 * InitDebugHooks()
 *
 * Description:
 *		Initializes all the mediated debug operation pointers. The "OriginalFunction" pointers
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
InitDebugHooks()
{
	if ( (OriginalNtDebugActiveProcess = (fpZwDebugActiveProcess) ZwCalls[ZW_DEBUG_ACTIVEPROCESS_INDEX].OriginalFunction) == NULL)
	{
		/* does not exist on Win2K */
		LOG(LOG_SS_DEBUG, LOG_PRIORITY_DEBUG, ("InitDebugHooks: OriginalNtDebugActiveProcess is NULL\n"));
	}

	return TRUE;
}
