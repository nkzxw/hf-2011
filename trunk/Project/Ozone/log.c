/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		log.c
 *
 * Abstract:
 *
 *		This module implements various alert logging routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 24-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "log.h"
#include "procname.h"
#include "policy.h"	// CDrive declaration


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitLog)
#pragma alloc_text (PAGE, ShutdownLog)
#pragma alloc_text (PAGE, LogPostBootup)
#endif


KSPIN_LOCK			gLogSpinLock;

PKEVENT				LogUserEvent = NULL;
HANDLE				LogUserEventHandle = NULL;

PSECURITY_ALERT		LogList = NULL;				/* alert queue */
PSECURITY_ALERT		LastAlert = NULL;			/* pointer to the last queued alert, used for quick inserts */

USHORT				NumberOfAlerts;				/* number of queued alerts */



/*
 * GetObjectAccessAlertPriority()
 *
 * Description:
 *		Figure out a priority for object access alert category.
 *
 * Parameters:
 *		AlertSubSystem
 *		Operation
 *		ActionTaken
 *
 * Returns:
 *		Chosen priority.
 */

ALERT_PRIORITY
GetObjectAccessAlertPriority(UCHAR AlertSubSystem, UCHAR Operation, ACTION_TYPE ActionTaken)
{
	switch (AlertSubSystem)
	{
		case RULE_FILE:
		case RULE_DIRECTORY:
		case RULE_NAMEDPIPE:
		case RULE_REGISTRY:
		case RULE_SECTION:
		case RULE_JOB:
		case RULE_PORT:
		case RULE_SYMLINK:

			/* default actions are given low priority */
			if (ActionTaken & ACTION_DEFAULT)
				return ALERT_PRIORITY_LOW;

			/* non-read operations are marked medium priority */
			if (Operation != OP_READ)
				return ALERT_PRIORITY_MEDIUM;

			/* whilst read operations are marked low priority */
			return ALERT_PRIORITY_LOW;


		/* high priority rules */
		case RULE_DLL:
		case RULE_PROCESS:
		case RULE_DRIVER:
		case RULE_NETWORK:
		case RULE_SERVICE:

			return ALERT_PRIORITY_HIGH;


		case RULE_TIME:
		case RULE_TOKEN:

			return ALERT_PRIORITY_MEDIUM;


		/* low priority rules */
		case RULE_MAILSLOT:
		case RULE_EVENT:
		case RULE_SEMAPHORE:
		case RULE_MUTANT:
		case RULE_DIROBJ:
		case RULE_ATOM:
		case RULE_SYSCALL:
		case RULE_TIMER:
			
			return ALERT_PRIORITY_LOW;


		default:
			LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("GetAlertPriority: Unknown alert subsystem %d\n", AlertSubSystem));
			return ALERT_PRIORITY_MEDIUM;
	}
}



/*
 * FilterObjectName()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		Nothing.
 */
//XXX move elsewhere
PCHAR
FilterObjectName(PCHAR ObjectName)
{
	if (_strnicmp(ObjectName, "\\??\\PIPE\\", 9) == 0)
		return ObjectName + 3;

	if (_strnicmp(ObjectName, "\\??\\", 4) == 0)
		return ObjectName + 4;

	if (_strnicmp(ObjectName, "\\DosDevices\\", 12) == 0)
		return ObjectName + 12;

	if (_strnicmp(ObjectName, "\\BaseNamedObjects\\", 18) == 0)
		return ObjectName + 18;

	if (_strnicmp(ObjectName, CDrive, CDriveLength) == 0 && CDriveLength)
	{
		/* replace any possible \device\harddiskvolumeX references with a DOS C:\ name */
		ObjectName[CDriveLength-2] = 'C';
		ObjectName[CDriveLength-1] = ':';

		ObjectName += CDriveLength - 2;
	}


	return ObjectName;
}



/*
 * LogAlert()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		Nothing.
 */

VOID
LogAlert(UCHAR AlertSubSystem, UCHAR OperationType, UCHAR AlertRuleNumber, ACTION_TYPE ActionTaken, ALERT_PRIORITY AlertPriority, PWSTR PolicyFilename, USHORT PolicyLineNumber, PCHAR ObjectName)
{
	USHORT				Size;
	PSECURITY_ALERT		pAlert;
	KIRQL				irql;

#define	NAME_BUFFER_SIZE	256
	ANSI_STRING			asObjectName;
	UNICODE_STRING		usObjectName;
	WCHAR				ProcessName[NAME_BUFFER_SIZE], ObjectNameW[NAME_BUFFER_SIZE] = { 0 };
	USHORT				ObjectNameLength = 0, ProcessNameLength = 0, PolicyNameLength = 0;

/*
	if (PolicyFilename == NULL)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, (("LogAlert: NULL PolicyFilename\n"));
		return;
	}
*/

	/* \KnownDlls section rules need to be converted to DLLs */
	if (AlertSubSystem == RULE_SECTION)
	{
		if (_strnicmp(ObjectName, "\\KnownDlls\\", 11) == 0)
		{
			ObjectName += 11;
			AlertSubSystem = RULE_DLL;
			OperationType = OP_LOAD;
		}
	}


	if (ObjectName)
	{
		ObjectName = FilterObjectName(ObjectName);

		usObjectName.Length = 0;
		usObjectName.MaximumLength = sizeof(ObjectNameW);
		usObjectName.Buffer = ObjectNameW;

		RtlInitAnsiString(&asObjectName, ObjectName);

		RtlAnsiStringToUnicodeString(&usObjectName, &asObjectName, FALSE);
		ObjectNameW[ asObjectName.Length ] = 0;
	}


	wcsncpy(ProcessName, GetCurrentProcessName(), NAME_BUFFER_SIZE - 1);
	ProcessName[NAME_BUFFER_SIZE - 1] = 0;


	/*
	 * extra +1 for ProcessName & PolicyName zero termination
	 * (ObjectName zero is covered by SECURITY_ALERT wchar[1] declaration)
	 */

	if (ObjectName)
		ObjectNameLength = (USHORT) wcslen(ObjectNameW);

	ProcessNameLength = (USHORT) wcslen(ProcessName);

	if (PolicyFilename)
		PolicyNameLength = (USHORT) wcslen(PolicyFilename);

	Size = sizeof(SECURITY_ALERT) +	(ObjectNameLength + ProcessNameLength + 1 + PolicyNameLength + 1) * sizeof(WCHAR);


	if ((pAlert = (PSECURITY_ALERT) GetCurrentUserSid(&Size)) == NULL)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("NULL UserInfo. Security Alert\n"));
		return;
	}


	pAlert->Next = NULL;
	pAlert->Size = Size;
	pAlert->AlertSubsystem = AlertSubSystem;
	pAlert->AlertType = OperationType;
	pAlert->AlertRuleNumber = AlertRuleNumber;
	pAlert->Priority = AlertPriority;
	pAlert->ObjectNameLength = ObjectNameLength;
	pAlert->ProcessNameLength = ProcessNameLength;
	pAlert->PolicyNameLength = PolicyNameLength;
	pAlert->ProcessId = (ULONG) PsGetCurrentProcessId();
	pAlert->Action = ActionTaken;
	pAlert->PolicyLineNumber = PolicyLineNumber;


	if (ObjectName)
		wcscpy(pAlert->ObjectName, ObjectNameW);

	/* process name follows the object name */
	wcscpy(pAlert->ObjectName + ObjectNameLength + 1, ProcessName);

	/* policy name follows the process name */
	if (PolicyFilename)
		wcscpy(pAlert->ObjectName + ObjectNameLength + 1 + ProcessNameLength + 1, PolicyFilename);

	/* save the alert for user agent to pick-up */
	if (ObjectName)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("%S Alert. Name %S. Object type %d. Alert type %d.\n", ProcessName, pAlert->ObjectName, pAlert->AlertSubsystem, pAlert->AlertType));
	}
	else
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("%S Alert. Object type %d. Alert type %d.\n", ProcessName, pAlert->AlertSubsystem, pAlert->AlertType));
	}


	KeAcquireSpinLock(&gLogSpinLock, &irql);
	{
		/*
		 * Put a ceiling on how many alerts we can queue, otherwise we can run out of memory
		 * if userland service is down
		 */

		if (NumberOfAlerts > MAXIMUM_OUTSTANDING_ALERTS)
		{
			LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("LogAlert: Exceeded maximum number of queued alerts. Dropping.\n"));

			ExFreePoolWithTag(pAlert, _POOL_TAG);

			KeReleaseSpinLock(&gLogSpinLock, irql);

			return;
		}

		++NumberOfAlerts;

		/*
		 * Append alerts to the end of the list so that they show up in a proper order when
		 * picked up by the userland service
		 */

		if (LogList == NULL)
		{
			/* first alert on the list */
			LastAlert = LogList = pAlert;
		}
		else
		{
			LastAlert->Next = pAlert;
			LastAlert = pAlert;
		}

//		pAlert->Next = LogList;
//		LogList = pAlert;
	}
	KeReleaseSpinLock(&gLogSpinLock, irql);


	/*
	 * Pulse the event to notify the user agent.
	 * User-mode apps can't reset a kernel mode event, which is why we're pulsing it here.
	 */

	if (LogUserEvent)
	{
		KeSetEvent(LogUserEvent, 0, FALSE);
		KeClearEvent(LogUserEvent);
	}
}



/*
 * LogPostBootup()
 *
 * Description:
 *		Initializes logging related data once computer is done booting up.
 *
 *		\BaseNamedObjects object directory is not created until the Win32® system initializes.
 *		Therefore, drivers that are loaded at boot time cannot create event objects in their DriverEntry
 *		routines that are visible to user-mode programs.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
LogPostBootup()
{
    UNICODE_STRING  uszLogEventString;


	ASSERT(BootingUp == FALSE);


    /* Create events for user-mode processes to monitor */
    RtlInitUnicodeString(&uszLogEventString, LOG_USER_EVENT_NAME);


	if ((LogUserEvent = IoCreateNotificationEvent(&uszLogEventString, &LogUserEventHandle)) == NULL)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("LogPostBootup: IoCreateNotificationEvent failed\n"));
		return FALSE;
	}

	KeClearEvent(LogUserEvent);


	return TRUE;
}



/*
 * InitLog()
 *
 * Description:
 *		Initializes logging related data.
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
InitLog()
{
	KeInitializeSpinLock(&gLogSpinLock);


	if (BootingUp == FALSE)

		return LogPostBootup();


	return TRUE;
}



/*
 * ShutdownLog()
 *
 * Description:
 *		Clean up the logging subsystem - close all the handles & delete any outstanding alerts.
 *
 *		NOTE: Called once during driver unload (DriverUnload()).
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Nothing.
 */

VOID
ShutdownLog()
{
	PSECURITY_ALERT		tmp;
	KIRQL				irql;


	if (LogUserEventHandle)
		ZwClose(LogUserEventHandle);


	/* XXX logs will need to be flushed to a file and then sent to MC */
	KeAcquireSpinLock(&gLogSpinLock, &irql);
	{
		while (LogList)
		{
			tmp = LogList;
			LogList = LogList->Next;

			ExFreePoolWithTag(tmp, _POOL_TAG);
		}
	}
	KeReleaseSpinLock(&gLogSpinLock, irql);
}
