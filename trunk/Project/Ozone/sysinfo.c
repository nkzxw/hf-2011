/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		sysinfo.c
 *
 * Abstract:
 *
 *		This module defines various routines used for hooking ZwSetSystemInformation() routine.
 *		ZwSetSystemInformation's SystemLoadAndCallImage and SystemLoadImage parameters can be used
 *		to load code into kernel address space.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 01-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include "sysinfo.h"
#include "hookproc.h"
#include "procname.h"
#include "learn.h"
#include "time.h"
#include "log.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitSysInfoHooks)
#endif


fpZwSetSystemInformation		OriginalNtSetSystemInformation = NULL;


/*
 * HookedNtSetSystemInformation()
 *
 * Description:
 *		This function mediates the NtSetSystemInformation() system service and disallows access to
 *		Information Classes 26 (SystemLoadImage) and 38 (SystemLoadAndCallImage) which allow
 *		applications to load code into kernel memory.
 *
 *		NOTE: ZwSetSystemInformation sets information that affects the operation of the system. [NAR]
 *
 * Parameters:
 *		Those of NtSetSystemInformation().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if Information Class 26 or 38 is used.
 *		Otherwise, NTSTATUS returned by NtSetSystemInformation().
 */

NTSTATUS
NTAPI
HookedNtSetSystemInformation
(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength
)
{
	PCHAR			FunctionName = "HookedNtSetSystemInformation";


	HOOK_ROUTINE_ENTER();


	if (SystemInformationClass == SystemLoadImage ||
		SystemInformationClass == SystemLoadAndCallImage)
	{
		UNICODE_STRING		usImageName;
		ANSI_STRING			asImageName;
		CHAR				DriverNameUnresolved[MAX_PATH];


		if (!VerifyUnicodeString(SystemInformation, &usImageName))
		{
			LOG(LOG_SS_PORT, LOG_PRIORITY_DEBUG, ("%s: VerifyUnicodeString failed\n", FunctionName));
			HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
		}


		if (_snprintf(DriverNameUnresolved, MAX_PATH, "%S", usImageName.Buffer) < 0)
		{
			LOG(LOG_SS_DRIVER, LOG_PRIORITY_DEBUG, ("%s: Driver name '%S' is too long\n", FunctionName, usImageName.Buffer));
			HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
		}


		LOG(LOG_SS_SYSINFO, LOG_PRIORITY_VERBOSE, ("%d %s: SystemLoad %d %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, SystemInformationClass, DriverNameUnresolved));


		/*
		 * Verify the image name against the security policy
		 */

		if (LearningMode == FALSE)
		{
			CHAR		DRIVERNAME[MAX_PATH];


			FixupFilename(DriverNameUnresolved, DRIVERNAME, MAX_PATH);

			LOG(LOG_SS_SYSINFO, LOG_PRIORITY_VERBOSE, ("%d %s: SystemLoad %d %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, SystemInformationClass, DRIVERNAME));

			POLICY_CHECK_OPTYPE_NAME(DRIVER, OP_LOAD);
		}
		else
		{
			AddRule(RULE_DRIVER, DriverNameUnresolved, OP_LOAD);
		}
	}
	else if (SystemInformationClass == SystemUnloadImage)
	{
		LOG(LOG_SS_SYSINFO, LOG_PRIORITY_VERBOSE, ("%d HookedNtSetSystemInformation: SystemUnloadImage %x\n", (ULONG) PsGetCurrentProcessId(), SystemInformation));
	}
	else if (SystemInformationClass == SystemTimeAdjustment)
	{
		LOG(LOG_SS_SYSINFO, LOG_PRIORITY_VERBOSE, ("%d HookedNtSetSystemInformation: SystemTimeAdjustment\n", (ULONG) PsGetCurrentProcessId()));


		if (LearningMode == FALSE)
		{
			PCHAR		TIMENAME = NULL;	/* allow the use of POLICY_CHECK_OPTYPE_NAME() macro */

			POLICY_CHECK_OPTYPE_NAME(TIME, OP_TIME_CHANGE);
		}
		else if (LearningMode == TRUE)
		{
			AddRule(RULE_TIME, NULL, OP_TIME_CHANGE);
		}
	}
	else if (SystemInformationClass == SystemProcessesAndThreadsInformation)
	{
		LOG(LOG_SS_SYSINFO, LOG_PRIORITY_DEBUG, ("%d HookedNtSetSystemInformation: SystemProcessesAndThreadsInformation\n", (ULONG) PsGetCurrentProcessId()));
	}
	else if (SystemInformationClass == SystemModuleInformation)
	{
		LOG(LOG_SS_SYSINFO, LOG_PRIORITY_DEBUG, ("%d HookedNtSetSystemInformation: SystemModuleInformation\n", (ULONG) PsGetCurrentProcessId()));
	}
	else if (SystemInformationClass == SystemCreateSession)
	{
		LOG(LOG_SS_SYSINFO, LOG_PRIORITY_VERBOSE, ("%d HookedNtSetSystemInformation: SystemCreateSession %x %x\n", (ULONG) PsGetCurrentProcessId(), SystemInformation, *(PULONG) SystemInformation));
	}
	else if (SystemInformationClass == SystemDeleteSession)
	{
		LOG(LOG_SS_SYSINFO, LOG_PRIORITY_VERBOSE, ("%d HookedNtSetSystemInformation: SystemDeleteSession %x %x\n", (ULONG) PsGetCurrentProcessId(), SystemInformation, *(PULONG) SystemInformation));
	}
	else if (SystemInformationClass == SystemSessionProcessesInformation)
	{
		LOG(LOG_SS_SYSINFO, LOG_PRIORITY_DEBUG, ("%d HookedNtSetSystemInformation: SystemSessionProcessesInformation\n", (ULONG) PsGetCurrentProcessId()));
	}


	ASSERT(OriginalNtSetSystemInformation);

	rc = OriginalNtSetSystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength);


	HOOK_ROUTINE_EXIT(rc);
}



/*
 * InitSysInfoHooks()
 *
 * Description:
 *		Initializes all the mediated system information operation pointers. The "OriginalFunction" pointers
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
InitSysInfoHooks()
{
	if ((OriginalNtSetSystemInformation = (fpZwSetSystemInformation) ZwCalls[ZW_SET_SYSTEM_INFORMATION_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_SYSINFO, LOG_PRIORITY_DEBUG, ("InitSysInfoHooks: OriginalNtSetSystemInformation is NULL\n"));
		return FALSE;
	}

	return TRUE;
}
