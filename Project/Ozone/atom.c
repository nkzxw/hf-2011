/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		atom.c
 *
 * Abstract:
 *
 *		This module implements various atom hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 25-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "atom.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitAtomHooks)
#endif


fpZwAddAtom			OriginalNtAddAtom = NULL;
fpZwFindAtom		OriginalNtFindAtom = NULL;



/*
 * HookedNtCreateAtom()
 *
 * Description:
 *		This function mediates the NtAddAtom() system service and checks the
 *		provided atom name against the global and current process security policies.
 *
 *		NOTE: ZwAddAtom adds an atom to the global atom table. [NAR]
 *
 * Parameters:
 *		Those of NtAddAtom().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtAddAtom().
 */

NTSTATUS
NTAPI
HookedNtAddAtom
(
	IN PWSTR String,
	IN ULONG StringLength,
	OUT PUSHORT Atom
)
{
	PCHAR	FunctionName = "HookedNtAddAtom";
	CHAR	ATOMNAME[MAX_PATH];


	HOOK_ROUTINE_ENTER();


	if (!VerifyPwstr(String, StringLength))
	{
		LOG(LOG_SS_ATOM, LOG_PRIORITY_DEBUG, ("HookedNtAddAtom: VerifyPwstr(%x) failed\n", String));
		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}


	_snprintf(ATOMNAME, MAX_PATH, "%S", String);
	ATOMNAME[ MAX_PATH - 1 ] = 0;


	if (LearningMode == FALSE)
	{
		POLICY_CHECK_OPTYPE_NAME(ATOM, OP_WRITE);
	}


	ASSERT(OriginalNtAddAtom);

	rc = OriginalNtAddAtom(String, StringLength, Atom);


	HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(ATOM, ATOMNAME, OP_WRITE);
}



/*
 * HookedNtFindAtom()
 *
 * Description:
 *		This function mediates the NtFindAtom() system service and checks the
 *		provided atom name against the global and current process security policies.
 *
 *		NOTE: ZwFindAtom searches for an atom in the global atom table. [NAR]
 *
 * Parameters:
 *		Those of NtFindAtom().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtFindAtom().
 */

NTSTATUS
NTAPI
HookedNtFindAtom
(
	IN PWSTR String,
	IN ULONG StringLength,
	OUT PUSHORT Atom
)
{
	PCHAR	FunctionName = "HookedNtFindAtom";
	CHAR	ATOMNAME[MAX_PATH];


	HOOK_ROUTINE_ENTER();


	if (!VerifyPwstr(String, StringLength))
	{
		LOG(LOG_SS_ATOM, LOG_PRIORITY_DEBUG, ("HookedNtFindAtom: VerifyPwstr(%x) failed\n", String));
		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}


	_snprintf(ATOMNAME, MAX_PATH, "%S", String);
	ATOMNAME[ MAX_PATH - 1 ] = 0;


	if (LearningMode == FALSE)
	{
		POLICY_CHECK_OPTYPE_NAME(ATOM, OP_READ);
	}


	ASSERT(OriginalNtFindAtom);

	rc = OriginalNtFindAtom(String, StringLength, Atom);


	HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(ATOM, ATOMNAME, OP_READ);
}



/*
 * InitAtomHooks()
 *
 * Description:
 *		Initializes all the mediated atom operation pointers. The "OriginalFunction" pointers
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
InitAtomHooks()
{
	if ( (OriginalNtAddAtom = (fpZwAddAtom) ZwCalls[ZW_ADD_ATOM_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_ATOM, LOG_PRIORITY_DEBUG, ("InitAtomHooks: OriginalNtAddAtom is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtFindAtom = (fpZwFindAtom) ZwCalls[ZW_FIND_ATOM_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_ATOM, LOG_PRIORITY_DEBUG, ("InitAtomHooks: OriginalNtFindAtom is NULL\n"));
		return FALSE;
	}

	return TRUE;
}
