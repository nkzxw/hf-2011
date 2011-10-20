/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		mutant.c
 *
 * Abstract:
 *
 *		This module implements various mutant (mutex) hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 25-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "mutant.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitMutantHooks)
#endif


fpZwCreateMutant	OriginalNtCreateMutant = NULL;
fpZwOpenMutant		OriginalNtOpenMutant = NULL;


/*
 * HookedNtCreateMutant()
 *
 * Description:
 *		This function mediates the NtCreateMutant() system service and checks the
 *		provided mutant name against the global and current process security policies.
 *
 *		NOTE: ZwCreateMutant creates or opens a mutant object. [NAR]
 *
 * Parameters:
 *		Those of NtCreateMutant().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateMutant().
 */

NTSTATUS
NTAPI
HookedNtCreateMutant
(
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOLEAN InitialOwner
)
{
	PCHAR	FunctionName = "HookedNtCreateMutant";


	HOOK_ROUTINE_START(MUTANT);


	ASSERT(OriginalNtCreateMutant);

	rc = OriginalNtCreateMutant(MutantHandle, DesiredAccess, ObjectAttributes, InitialOwner);


	HOOK_ROUTINE_FINISH(MUTANT);
}



/*
 * HookedNtOpenMutant()
 *
 * Description:
 *		This function mediates the NtOpenMutant() system service and checks the
 *		provided mutant name against the global and current process security policies.
 *
 *		NOTE: ZwOpenMutant opens a mutant object. [NAR]
 *
 * Parameters:
 *		Those of NtOpenMutant().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtOpenMutant().
 */

NTSTATUS
NTAPI
HookedNtOpenMutant
(
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
	PCHAR	FunctionName = "HookedNtOpenMutant";


	HOOK_ROUTINE_START(MUTANT);


	ASSERT(OriginalNtOpenMutant);

	rc = OriginalNtOpenMutant(MutantHandle, DesiredAccess, ObjectAttributes);


	HOOK_ROUTINE_FINISH(MUTANT);
}



/*
 * InitMutantHooks()
 *
 * Description:
 *		Initializes all the mediated mutant operation pointers. The "OriginalFunction" pointers
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
InitMutantHooks()
{
	if ( (OriginalNtCreateMutant = (fpZwCreateMutant) ZwCalls[ZW_CREATE_MUTANT_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_MUTANT, LOG_PRIORITY_DEBUG, ("InitMutantHooks: OriginalNtCreateMutant is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtOpenMutant = (fpZwOpenMutant) ZwCalls[ZW_OPEN_MUTANT_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_MUTANT, LOG_PRIORITY_DEBUG, ("InitMutantHooks: OriginalNtOpenMutant is NULL\n"));
		return FALSE;
	}

	return TRUE;
}
