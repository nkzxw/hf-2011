/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		driverobj.c
 *
 * Abstract:
 *
 *		This module implements various driver object hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 06-Apr-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "driverobj.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitDriverObjectHooks)
#endif


fpZwLoadDriver		OriginalNtLoadDriver = NULL;
fpZwUnloadDriver	OriginalNtUnloadDriver = NULL;


/*
 * HookedNtLoadDriver()
 *
 * Description:
 *		This function mediates the NtLoadDriver() system service and checks the
 *		provided driver object name against the global and current process security policies.
 *
 *		NOTE: ZwLoadDriver loads a device driver. [NAR]
 *
 * Parameters:
 *		Those of NtLoadDriver().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtLoadDriver().
 */

NTSTATUS
NTAPI
HookedNtLoadDriver
(
	IN PUNICODE_STRING DriverServiceName
)
{
	PCHAR			FunctionName = "HookedNtLoadDriver";
	UNICODE_STRING	usDriverName;
	ANSI_STRING		AnsiDriverName;
	CHAR			DRIVERNAME[MAX_PATH];


	HOOK_ROUTINE_ENTER();


	if (!VerifyUnicodeString(DriverServiceName, &usDriverName))
	{
		LOG(LOG_SS_DRIVER, LOG_PRIORITY_DEBUG, ("HookedNtLoadDriver: VerifyUnicodeString(%x) failed\n", DriverServiceName));
		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}


	if (_snprintf(DRIVERNAME, MAX_PATH, "%S", usDriverName.Buffer) < 0)
	{
		LOG(LOG_SS_DRIVER, LOG_PRIORITY_DEBUG, ("%s: Driver name '%S' is too long\n", FunctionName, usDriverName.Buffer));
		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}


	LOG(LOG_SS_DRIVER, LOG_PRIORITY_VERBOSE, ("HookedNtLoadDriver: %s\n", DRIVERNAME));


	if (LearningMode == FALSE)
	{
		POLICY_CHECK_OPTYPE_NAME(DRIVER, OP_REGLOAD);
	}


	ASSERT(OriginalNtLoadDriver);

	rc = OriginalNtLoadDriver(DriverServiceName);


	HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(DRIVER, DRIVERNAME, OP_REGLOAD);
}



/*
 * HookedNtUnloadDriver()
 *
 * Description:
 *		This function mediates the NtUnloadDriver() system service and checks the
 *		provided driver object name against the global and current process security policies.
 *
 *		NOTE: ZwUnloadDriver unloads a device driver. [NAR]
 *
 * Parameters:
 *		Those of NtUnloadDriver().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtUnloadDriver().
 */

//XXX cannot mediate this function if we want to be able to unload our own driver
/* uncomment originalfunction pointer code in Init() routine and hookproc.c hook to enable
NTSTATUS
NTAPI
HookedNtUnloadDriver
(
	IN PUNICODE_STRING DriverServiceName
)
{
	PCHAR			FunctionName = "HookedNtUnloadDriver";
	UNICODE_STRING	usDriverName;
	ANSI_STRING		AnsiDriverName;
	CHAR			DRIVERNAME[MAX_PATH];


	HOOK_ROUTINE_ENTER();


	if (!VerifyUnicodeString(DriverServiceName, &usDriverName))
	{
		LOG(LOG_SS_DRIVER, LOG_PRIORITY_DEBUG, ("HookedNtUnloadDriver: VerifyUnicodeString failed\n"));
		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}

	AnsiDriverName.Length = 0;
	AnsiDriverName.MaximumLength = MAX_PATH - 1;
	AnsiDriverName.Buffer = DRIVERNAME;

	if (! NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiDriverName, &usDriverName, FALSE)))
	{
		LOG(LOG_SS_DRIVER, LOG_PRIORITY_DEBUG, ("HookedNtUnloadDriver: RtlUnicodeStringToAnsiString failed\n"));
		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}

	DRIVERNAME[AnsiDriverName.Length] = 0;


	LOG(LOG_SS_DRIVER, LOG_PRIORITY_DEBUG, ("HookedNtUnloadDriver: %s\n", DRIVERNAME));


	if (LearningMode == FALSE)
	{
		POLICY_CHECK_OPTYPE_NAME(DRIVER, OP_UNLOAD);
	}


	ASSERT(OriginalNtUnloadDriver);

	rc = OriginalNtUnloadDriver(DriverServiceName);


	HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(DRIVER, DRIVERNAME, OP_UNLOAD);
}
*/


/*
 * InitDriverHooks()
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
InitDriverObjectHooks()
{
	if ( (OriginalNtLoadDriver = (fpZwLoadDriver) ZwCalls[ZW_LOAD_DRIVER_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_DRIVER, LOG_PRIORITY_DEBUG, ("InitDriverHooks: OriginalNtLoadDriver is NULL\n"));
		return FALSE;
	}
/*
	if ( (OriginalNtUnloadDriver = (fpZwUnloadDriver) ZwCalls[ZW_UNLOAD_DRIVER_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_DRIVER, LOG_PRIORITY_DEBUG, ("InitDriverHooks: OriginalNtUnloadDriver is NULL\n"));
		return FALSE;
	}
*/
	return TRUE;
}
