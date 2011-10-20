/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		dirobj.c
 *
 * Abstract:
 *
 *		This module implements various object directory hooking routines.
 *		These are not file system directories (see file.c) but rather containers
 *		for other objects.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 03-Sep-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "dirobj.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitDirobjHooks)
#endif


fpZwOpenDirectoryObject		OriginalNtOpenDirectoryObject = NULL;
fpZwCreateDirectoryObject	OriginalNtCreateDirectoryObject = NULL;



/*
 * HookedNtCreateDirectoryObject()
 *
 * Description:
 *		This function mediates the NtCreateDirectoryObject() system service and checks the
 *		provided directory object name against the global and current process security policies.
 *
 *		NOTE: ZwCreateDirectoryObject creates or opens an object directory. [NAR]
 *
 * Parameters:
 *		Those of NtCreateDirectoryObject().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateDirectoryObject().
 */

NTSTATUS
NTAPI
HookedNtCreateDirectoryObject
(
	OUT PHANDLE DirectoryHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
	PCHAR	FunctionName = "HookedNtCreateDirectoryObject";


	HOOK_ROUTINE_START(DIROBJ);


	ASSERT(OriginalNtCreateDirectoryObject);

	rc = OriginalNtCreateDirectoryObject(DirectoryHandle, DesiredAccess, ObjectAttributes);


	HOOK_ROUTINE_FINISH(DIROBJ);
}



/*
 * HookedNtOpenDirectoryObject()
 *
 * Description:
 *		This function mediates the NtOpenDirectoryObject() system service and checks the
 *		provided directory object name against the global and current process security policies.
 *
 *		NOTE: ZwOpenDirectoryObject opens an object directory. [NAR]
 *
 * Parameters:
 *		Those of NtOpenDirectoryObject().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtOpenDirectoryObject().
 */

NTSTATUS
NTAPI
HookedNtOpenDirectoryObject
(
	OUT PHANDLE DirectoryHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
	PCHAR	FunctionName = "HookedNtOpenDirectoryObject";


	HOOK_ROUTINE_START(DIROBJ);


	ASSERT(OriginalNtOpenDirectoryObject);

	rc = OriginalNtOpenDirectoryObject(DirectoryHandle, DesiredAccess, ObjectAttributes);


	HOOK_ROUTINE_FINISH(DIROBJ);
}



/*
 * InitDirobjHooks()
 *
 * Description:
 *		Initializes all the mediated driver object operation pointers. The "OriginalFunction" pointers
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
InitDirobjHooks()
{
	if ( (OriginalNtOpenDirectoryObject = (fpZwOpenDirectoryObject) ZwCalls[ZW_OPEN_DIRECTORYOBJECT_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_FILE, LOG_PRIORITY_DEBUG, ("InitDirobjHooks: OriginalNtOpenDirectoryObject is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtCreateDirectoryObject = (fpZwCreateDirectoryObject) ZwCalls[ZW_CREATE_DIRECTORYOBJECT_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_FILE, LOG_PRIORITY_DEBUG, ("InitDirobjHooks: OriginalNtCreateDirectoryObject is NULL\n"));
		return FALSE;
	}

	return TRUE;
}
