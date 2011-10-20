/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		symlink.c
 *
 * Abstract:
 *
 *		This module implements various symbolic link object hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 25-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "symlink.h"
#include "media.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitSymlinkHooks)
#endif


fpZwCreateSymbolicLinkObject	OriginalNtCreateSymbolicLinkObject = NULL;
fpZwOpenSymbolicLinkObject		OriginalNtOpenSymbolicLinkObject = NULL;


/*
 * HookedNtCreateSymbolicLinkObject()
 *
 * Description:
 *		This function mediates the NtCreateSymbolicLinkObject() system service and checks the
 *		provided symbolic link object name against the global and current process security policies.
 *
 *		NOTE: ZwCreateSymbolicLinkObject creates or opens a symbolic link object. [NAR]
 *
 * Parameters:
 *		Those of NtCreateSymbolicLinkObject().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateSymbolicLinkObject().
 */

NTSTATUS
NTAPI
HookedNtCreateSymbolicLinkObject
(
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PUNICODE_STRING TargetName
)
{
	PCHAR			FunctionName = "HookedNtCreateSymbolicLinkObject";
	CHAR			SYMLINKNAME[MAX_PATH];


	HOOK_ROUTINE_ENTER();


	if (LearningMode == FALSE && GetPathFromOA(ObjectAttributes, SYMLINKNAME, MAX_PATH, RESOLVE_LINKS))
	{
		/* TargetName is not verified to be valid but it's ok as we don't use it for anything but printing (in debugging mode only) */
		if (TargetName)
			LOG(LOG_SS_SYMLINK, LOG_PRIORITY_VERBOSE, ("%d HookedNtCreateSymbolicLinkObject: %s -> %S\n", (ULONG) PsGetCurrentProcessId(), SYMLINKNAME, TargetName->Buffer));
		else
			LOG(LOG_SS_SYMLINK, LOG_PRIORITY_VERBOSE, ("%d HookedNtCreateSymbolicLinkObject: %s ->.\n", (ULONG) PsGetCurrentProcessId(), SYMLINKNAME));


		POLICY_CHECK_OPTYPE_NAME(SYMLINK, Get_SYMLINK_OperationType(DesiredAccess));
	}


	ASSERT(OriginalNtCreateSymbolicLinkObject);

	rc = OriginalNtCreateSymbolicLinkObject(SymbolicLinkHandle, DesiredAccess, ObjectAttributes, TargetName);


#if HOOK_MEDIA
	/* removable media hook */
	if (LearningMode == FALSE && NT_SUCCESS(rc) && KeGetPreviousMode() == KernelMode)
	{
		MonitorDriveLinks(SYMLINKNAME);
	}
#endif


	HOOK_ROUTINE_FINISH(SYMLINK);
}



/*
 * HookedNtOpenSymbolicLinkObject()
 *
 * Description:
 *		This function mediates the NtOpenSymbolicLinkObject() system service and checks the
 *		provided symbolic link object name against the global and current process security policies.
 *
 *		NOTE: ZwOpenSymbolicLinkObject opens a symbolic link object. [NAR]
 *
 * Parameters:
 *		Those of NtOpenSymbolicLinkObject().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtOpenSymbolicLinkObject().
 */

NTSTATUS
NTAPI
HookedNtOpenSymbolicLinkObject
(
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
	PCHAR			FunctionName = "HookedNtOpenSymbolicLinkObject";
	CHAR			SYMLINKNAME[MAX_PATH];


	HOOK_ROUTINE_ENTER();


	/* Cannot use RESOLVE_LINKS! (to avoid infinite recursion) */
	if (LearningMode == FALSE && GetPathFromOA(ObjectAttributes, SYMLINKNAME, MAX_PATH, DO_NOT_RESOLVE_LINKS))
	{
		POLICY_CHECK_OPTYPE_NAME(SYMLINK, Get_SYMLINK_OperationType(DesiredAccess));
	}


	ASSERT(OriginalNtOpenSymbolicLinkObject);

	rc = OriginalNtOpenSymbolicLinkObject(SymbolicLinkHandle, DesiredAccess, ObjectAttributes);


	HOOK_ROUTINE_FINISH(SYMLINK);
}



/*
 * InitSymlinkHooks()
 *
 * Description:
 *		Initializes all the mediated symbolic link object operation pointers. The "OriginalFunction" pointers
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
InitSymlinkHooks()
{
	if ( (OriginalNtCreateSymbolicLinkObject = (fpZwCreateSymbolicLinkObject) ZwCalls[ZW_CREATE_SYMLINK_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_SYMLINK, LOG_PRIORITY_DEBUG, ("InitSymlinkHooks: OriginalNtCreateSymbolicLinkObject is NULL\n"));
		return FALSE;
	}
/*
	disabled due to performance issues - this function is called by every system call (from ResolveFilename)

	if ( (OriginalNtOpenSymbolicLinkObject = (fpZwOpenSymbolicLinkObject) ZwCalls[ZW_OPEN_SYMLINK_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_SYMLINK, LOG_PRIORITY_DEBUG, ("InitSymlinkHooks: OriginalNtOpenSymbolicLinkObject is NULL\n"));
		return FALSE;
	}
*/
	return TRUE;
}
