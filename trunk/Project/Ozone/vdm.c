/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		vdm.c
 *
 * Abstract:
 *
 *		This module implements various VDM (Virtual Dos Machine) hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 06-Apr-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include "vdm.h"
#include "policy.h"
#include "hookproc.h"
#include "procname.h"
#include "policy.h"
#include "learn.h"
#include "log.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitVdmHooks)
#endif


fpZwSetLdtEntries		OriginalNtSetLdtEntries = NULL;
fpZwVdmControl			OriginalNtVdmControl = NULL;



/*
 * IsVdmAllowed()
 *
 * Description:
 *		Check whether the current process is allowed to use dos16/VDM functionality.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		FALSE if VDM is disabled. TRUE otherwise.
 */

BOOLEAN
IsVdmAllowed()
{
	PIMAGE_PID_ENTRY	CurrentProcess;
	BOOLEAN				VdmAllowed = FALSE;


	/* check the global policy first */
	if (! IS_VDM_PROTECTION_ON(gSecPolicy))
		return TRUE;


	/* now check the process specific policy */
	CurrentProcess = FindImagePidEntry(CURRENT_PROCESS_PID, 0);

	if (CurrentProcess != NULL)
	{
		VdmAllowed = ! IS_VDM_PROTECTION_ON(CurrentProcess->SecPolicy);
	}
	else
	{
		LOG(LOG_SS_VDM, LOG_PRIORITY_DEBUG, ("%d IsVdmAllowed: CurrentProcess = NULL!\n", CURRENT_PROCESS_PID));
	}


	return VdmAllowed;
}



/*
 * HookedNtSetLdtEntries()
 *
 * Description:
 *		This function mediates the NtSetLdtEntries() system service and disallows access to it.
 *
 *		NOTE: ZwSetLdtEntries sets Local Descriptor Table (LDT) entries for a Virtual DOS Machine (VDM). [NAR]
 *
 * Parameters:
 *		Those of NtSetLdtEntries().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if 16-bit applications are disabled.
 *		Otherwise, NTSTATUS returned by NtSetLdtEntries().
 */

NTSTATUS
NTAPI
HookedNtSetLdtEntries
(
    IN ULONG Selector0,
    IN ULONG Entry0Low,
    IN ULONG Entry0Hi,
    IN ULONG Selector1,
    IN ULONG Entry1Low,
    IN ULONG Entry1Hi
)
{
	HOOK_ROUTINE_ENTER();


	LOG(LOG_SS_VDM, LOG_PRIORITY_VERBOSE, ("%d (%S) HookedNtSetLdtEntries(%x %x %x)\n", (ULONG) PsGetCurrentProcessId(), GetCurrentProcessName(), Selector0, Entry0Low, Entry0Hi));

	if (LearningMode == FALSE && IsVdmAllowed() == FALSE)
	{
		LOG(LOG_SS_VDM, LOG_PRIORITY_DEBUG, ("%d (%S) HookedNtSetLdtEntries: disallowing VDM access\n", (ULONG) PsGetCurrentProcessId(), GetCurrentProcessName()));

		LogAlert(ALERT_SS_VDM, OP_VDM_USE, ALERT_RULE_NONE, ACTION_DENY, ALERT_PRIORITY_MEDIUM, NULL, 0, NULL);

		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}


	ASSERT(OriginalNtSetLdtEntries);

	rc = OriginalNtSetLdtEntries(Selector0, Entry0Low, Entry0Hi, Selector1, Entry1Low, Entry1Hi);


	if (LearningMode == TRUE)
		TURN_VDM_PROTECTION_OFF(NewPolicy);


	HOOK_ROUTINE_EXIT(rc);
}



/*
 * HookedNtVdmControl()
 *
 * Description:
 *		This function mediates the NtVdmControl() system service and disallows access to it.
 *
 *		NOTE: ZwVdmControl performs a control operation on a VDM. [NAR]
 *
 * Parameters:
 *		Those of NtVdmControl().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if 16-bit applications are disabled.
 *		Otherwise, NTSTATUS returned by NtVdmControl().
 */

NTSTATUS
NTAPI
HookedNtVdmControl
(
	IN ULONG ControlCode,
	IN PVOID ControlData
)
{
	HOOK_ROUTINE_ENTER();


	LOG(LOG_SS_VDM, LOG_PRIORITY_VERBOSE, ("%d (%S) HookedNtVdmControl(%x %x)\n", (ULONG) PsGetCurrentProcessId(), GetCurrentProcessName(), ControlCode, ControlData));

	if (LearningMode == FALSE && IsVdmAllowed() == FALSE)
	{
		LOG(LOG_SS_VDM, LOG_PRIORITY_DEBUG, ("%d (%S) HookedNtVdmControl: disallowing VDM access\n", (ULONG) PsGetCurrentProcessId(), GetCurrentProcessName()));

		LogAlert(ALERT_SS_VDM, OP_VDM_USE, ALERT_RULE_NONE, ACTION_DENY, ALERT_PRIORITY_MEDIUM, NULL, 0, NULL);

		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}


	ASSERT(OriginalNtVdmControl);

	rc = OriginalNtVdmControl(ControlCode, ControlData);


	if (LearningMode == TRUE)
		TURN_VDM_PROTECTION_OFF(NewPolicy);


	HOOK_ROUTINE_EXIT(rc);
}



/*
 * InitVdmHooks()
 *
 * Description:
 *		Initializes all the mediated vdm operation pointers. The "OriginalFunction" pointers
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
InitVdmHooks()
{
	if ( (OriginalNtSetLdtEntries = (fpZwSetLdtEntries) ZwCalls[ZW_SET_LDT_ENTRIES_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_VDM, LOG_PRIORITY_DEBUG, ("InitVdmHooks: OriginalNtSetLdtEntries is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtVdmControl = (fpZwVdmControl) ZwCalls[ZW_VDM_CONTROL_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_VDM, LOG_PRIORITY_DEBUG, ("InitVdmHooks: OriginalNtVdmControl is NULL\n"));
		return FALSE;
	}

	return TRUE;
}
