/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		userland.c
 *
 * Abstract:
 *
 *		This module defines various types routines used to interact with userland agent service.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 18-Apr-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "userland.h"
#include "procname.h"
#include "process.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitUserland)
#pragma alloc_text (PAGE, ShutdownUserland)
#pragma alloc_text (PAGE, UserlandPostBootup)
#endif


BOOLEAN						ActiveUserAgent = FALSE;

PKEVENT						UserlandRequestUserEvent = NULL;
HANDLE						UserlandRequestUserEventHandle = NULL;

PUSERLAND_REQUEST_HEADER	UserlandRequestList = NULL;
KSPIN_LOCK					gUserlandRequestListSpinLock;

BOOLEAN						CacheSid = TRUE;
ULONG						CachedSidSize = 0, CachedSidReplySize = 0;
PVOID						CachedSid = NULL;
PSID_RESOLVE_REPLY			CachedSidReply = NULL;

UCHAR						SeqId = 0;	/* global sequence id used for matching up userland requests & replies */



/*
 * UserlandPostBootup()
 *
 * Description:
 *		Initializes userland related data once computer is done booting up.
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
UserlandPostBootup()
{
    UNICODE_STRING  uszUserlandRequestEvent;


	ASSERT(BootingUp == FALSE);


	KeInitializeSpinLock(&gUserlandRequestListSpinLock);

	
	/* Create an event for userland agent service to monitor */
#define	USERLAND_REQUEST_EVENT_NAME	L"\\BaseNamedObjects\\OzoneUserlandRequestEvent"

    RtlInitUnicodeString(&uszUserlandRequestEvent, USERLAND_REQUEST_EVENT_NAME);


	if ((UserlandRequestUserEvent = IoCreateNotificationEvent(&uszUserlandRequestEvent, &UserlandRequestUserEventHandle)) == NULL)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("UserlandPostBootup: IoCreateNotificationEvent failed\n"));
		return FALSE;
	}

	KeClearEvent(UserlandRequestUserEvent);


	return TRUE;
}



/*
 * IssueUserlandSidResolveRequest()
 *
 * Description:
 *		Resolves binary SID to its ASCII representation by querying a userland agent.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		TRUE to indicate success, FALSE otherwise.
 */

BOOLEAN
IssueUserlandSidResolveRequest(PIMAGE_PID_ENTRY Process)
{
	PSID_RESOLVE_REQUEST	pSidResolveRequest;
	USHORT					Size;
	PCHAR					UserSidBuffer;
	KIRQL					irql;
	LARGE_INTEGER			liDelay;
	NTSTATUS				status;


	ASSERT(Process);


	if (!ActiveUserAgent)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_VERBOSE, ("IssueUserlandSidResolveRequest: no Agent Service\n"));
		return FALSE;
	}

	if (KeGetCurrentIrql() != PASSIVE_LEVEL)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandSidResolveRequest: Running at high IRQL\n"));
		return FALSE;
	}

	if (Process->ProcessId == SystemProcessId)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandSidResolveRequest: Cannot issue requests on behalf of SYSTEM process\n"));
//		return FALSE;
	}


	/*
	 * the following code assumes that SID_RESOLVE_REQUESTS consists of USERLAND_REQUEST_HEADER
	 * followed by PSID_AND_ATTRIBUTES.
	 *
	 * GetCurrentUserSid() allocates memory for user sid + PSID_AND_ATTRIBUTES and converts SID
	 * from absolute into relative format.
	 */

	Size = sizeof(USERLAND_REQUEST_HEADER);
	UserSidBuffer = GetCurrentUserSid(&Size);

	if (UserSidBuffer == NULL)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandSidResolveRequest: out of memory\n"));
		return FALSE;
	}

	/* check for a previously resolved cached SID */
	while (CacheSid == TRUE)
	{
		int	SidSize = Size - sizeof(USERLAND_REQUEST_HEADER);


		/* cache hit? */
		if (CachedSid && CachedSidReply &&
			SidSize == CachedSidSize &&
			memcmp(UserSidBuffer + sizeof(USERLAND_REQUEST_HEADER), CachedSid, CachedSidSize) == 0)
		{
			Process->WaitingForUserRequestId = 0;
			Process->UserlandReply = ExAllocatePoolWithTag(PagedPool, CachedSidReplySize, _POOL_TAG);

			if (Process->UserlandReply == NULL)
			{
				ExFreePoolWithTag(UserSidBuffer, _POOL_TAG);
				return FALSE;
			}

			memcpy(Process->UserlandReply, CachedSidReply, CachedSidReplySize);

			LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandSidResolveRequest: Cache hit. Returning username %S\n", CachedSidReply->UserName));

			ExFreePoolWithTag(UserSidBuffer, _POOL_TAG);

			return TRUE;
		}


		/* cache miss */
		CachedSidSize = SidSize;

		if (CachedSid)
			ExFreePoolWithTag(CachedSid, _POOL_TAG);

		CachedSid = ExAllocatePoolWithTag(PagedPool, SidSize, _POOL_TAG);

		if (CachedSid == NULL)
		{
			ExFreePoolWithTag(UserSidBuffer, _POOL_TAG);
			return FALSE;
		}

		memcpy(CachedSid, UserSidBuffer + sizeof(USERLAND_REQUEST_HEADER), SidSize);

		if (CachedSidReply)
		{
			ExFreePoolWithTag(CachedSidReply, _POOL_TAG);
			CachedSidReply = NULL;
		}

		break;
	}


	pSidResolveRequest = (PSID_RESOLVE_REQUEST) UserSidBuffer;


	pSidResolveRequest->RequestHeader.RequestType = USERLAND_SID_RESOLVE_REQUEST;
	pSidResolveRequest->RequestHeader.RequestSize = Size;
	pSidResolveRequest->RequestHeader.ProcessId = Process->ProcessId;

	if (++SeqId == 0) SeqId = 1;
	pSidResolveRequest->RequestHeader.SeqId = SeqId;


	KeAcquireSpinLock(&gUserlandRequestListSpinLock, &irql);
	{
		pSidResolveRequest->RequestHeader.Next = UserlandRequestList;
		UserlandRequestList = (PUSERLAND_REQUEST_HEADER) pSidResolveRequest;
	}
	KeReleaseSpinLock(&gUserlandRequestListSpinLock, irql);


	Process->WaitingForUserRequestId = SeqId;
	Process->UserlandReply = NULL;


	KeClearEvent(&Process->UserlandRequestDoneEvent);


	/* signal the userland agent service */
	if (UserlandRequestUserEvent)
	{
		/* pulse twice since userland sometimes misses single pulses */
		KeSetEvent(UserlandRequestUserEvent, 0, FALSE);
		KeClearEvent(UserlandRequestUserEvent);

		KeSetEvent(UserlandRequestUserEvent, 0, FALSE);
		KeClearEvent(UserlandRequestUserEvent);
	}
	else
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandSidResolveRequest: UserlandRequestUserEvent is NULL\n"));
	}


	LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("%d IssueUserlandSidResolveRequest: Waiting for agent service\n", CURRENT_PROCESS_PID));


	/* wait for the userland service to reply (wait for maximum of USERLAND_REQUEST_TIMEOUT seconds) */
	liDelay.QuadPart = SECONDS(USERLAND_REQUEST_TIMEOUT);

	status = KeWaitForSingleObject(&Process->UserlandRequestDoneEvent, UserRequest, KernelMode, FALSE, &liDelay);


	Process->WaitingForUserRequestId = 0;

	if (status == STATUS_TIMEOUT)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandSidResolveRequest: KeWaitForSingleObject timed out\n"));
	}
	else
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandSidResolveRequest: KeWaitForSingleObject returned\n"));

		/* cache the resolved Sid */
		if (CacheSid == TRUE && Process->UserlandReply != NULL)
		{
			CachedSidReply = ExAllocatePoolWithTag(PagedPool, Process->UserlandReply->ReplySize, _POOL_TAG);

			if (CachedSidReply)
			{
				memcpy(CachedSidReply, Process->UserlandReply, Process->UserlandReply->ReplySize);

				CachedSidReplySize = Process->UserlandReply->ReplySize;
			}
		}
	}


	/* at this point UserSidBuffer/pSidResolveRequest has already been deallocated in driver.c!DriverDeviceControl */


	return TRUE;
}



/*
 * IssueUserlandAskUserRequest()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		.
 */

ACTION_TYPE
IssueUserlandAskUserRequest(RULE_TYPE RuleType, UCHAR OperationType, PCHAR ObjectName)
{
	ACTION_TYPE				Action = ACTION_DENY_DEFAULT;
	PIMAGE_PID_ENTRY		Process;
	PASK_USER_REQUEST		pAskUserRequest;
	USHORT					Size;
	KIRQL					irql;
	LARGE_INTEGER			liDelay;
	NTSTATUS				status;

#define	NAME_BUFFER_SIZE	256
	ANSI_STRING				asObjectName;
	UNICODE_STRING			usObjectName;
	WCHAR					ObjectNameW[NAME_BUFFER_SIZE] = { 0 };


	ASSERT(ObjectName);


	if (!ActiveUserAgent)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandAskUserRequest: no Agent Service\n"));
		return Action;
	}

	if (KeGetCurrentIrql() != PASSIVE_LEVEL)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandAskUserRequest: Running at high IRQL\n"));
		return FALSE;
	}


	if (CURRENT_PROCESS_PID == SystemProcessId)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandAskUserRequest: Cannot issue requests on behalf of SYSTEM process\n"));
		return FALSE;
	}


	Process = FindImagePidEntry(CURRENT_PROCESS_PID, 0);
	if (Process == NULL)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandAskUserRequest: Process %d not found\n", CURRENT_PROCESS_PID));
		return Action;
	}


	/* \KnownDlls section rules need to be converted to DLLs */
	if (RuleType == RULE_SECTION)
	{
		if (_strnicmp(ObjectName, "\\KnownDlls\\", 11) == 0)
		{
			ObjectName += 11;
			RuleType = RULE_DLL;
			OperationType = OP_LOAD;
		}
	}


	ObjectName = FilterObjectName(ObjectName);

	usObjectName.Length = 0;
	usObjectName.MaximumLength = sizeof(ObjectNameW);
	usObjectName.Buffer = ObjectNameW;

	RtlInitAnsiString(&asObjectName, ObjectName);

	RtlAnsiStringToUnicodeString(&usObjectName, &asObjectName, FALSE);
	ObjectNameW[ asObjectName.Length ] = 0;


	/* extra +1 for ProcessName zero termination */
	Size = sizeof(ASK_USER_REQUEST) + (wcslen(ObjectNameW) + wcslen(Process->ImageName) + 1) * sizeof(WCHAR);

	pAskUserRequest = ExAllocatePoolWithTag(NonPagedPool, Size, _POOL_TAG);

	if (pAskUserRequest == NULL)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandAskUserRequest: out of memory\n"));
		return Action;
	}


	pAskUserRequest->RequestHeader.RequestType = USERLAND_ASK_USER_REQUEST;
	pAskUserRequest->RequestHeader.RequestSize = Size;
	pAskUserRequest->RequestHeader.ProcessId = Process->ProcessId;

	if (++SeqId == 0) SeqId = 1;
	pAskUserRequest->RequestHeader.SeqId = SeqId;

	pAskUserRequest->RuleType = RuleType;
	pAskUserRequest->OperationType = OperationType;
	pAskUserRequest->ObjectNameLength = (USHORT) wcslen(ObjectNameW);
	pAskUserRequest->ProcessNameLength = (USHORT) wcslen(Process->ImageName);

	wcscpy(pAskUserRequest->ObjectName, ObjectNameW);

	/* process name follows the object name */
	wcscpy(pAskUserRequest->ObjectName + pAskUserRequest->ObjectNameLength + 1, Process->ImageName);


	KeAcquireSpinLock(&gUserlandRequestListSpinLock, &irql);
	{
		pAskUserRequest->RequestHeader.Next = UserlandRequestList;
		UserlandRequestList = (PUSERLAND_REQUEST_HEADER) pAskUserRequest;
	}
	KeReleaseSpinLock(&gUserlandRequestListSpinLock, irql);


	Process->WaitingForUserRequestId = SeqId;
	Process->UserlandReply = NULL;


	KeClearEvent(&Process->UserlandRequestDoneEvent);


	/* signal the userland agent service */
	if (UserlandRequestUserEvent)
	{
		/* pulse twice since userland sometimes misses single pulses */
		KeSetEvent(UserlandRequestUserEvent, 0, FALSE);
		KeClearEvent(UserlandRequestUserEvent);

		KeSetEvent(UserlandRequestUserEvent, 0, FALSE);
		KeClearEvent(UserlandRequestUserEvent);
	}
	else
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandAskUserRequest: UserlandRequestUserEvent is NULL\n"));
	}


	LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("%d IssueUserlandAskUserRequest: Waiting for agent service\n", CURRENT_PROCESS_PID));


	/* wait for the user/userland service to reply (wait for maximum of USERLAND_REQUEST_TIMEOUT seconds) */
	liDelay.QuadPart = SECONDS(USERLAND_REQUEST_TIMEOUT);

	status = KeWaitForSingleObject(&Process->UserlandRequestDoneEvent, UserRequest, KernelMode, FALSE, &liDelay);


	Process->WaitingForUserRequestId = 0;

	if (status != STATUS_TIMEOUT)
	{
		PASK_USER_REPLY		pAskUserReply = (PASK_USER_REPLY) Process->UserlandReply;

		if (pAskUserReply)
		{
			Action = pAskUserReply->Action;

			LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandAskUserRequest: received back action %d\n", Action));

			ExFreePoolWithTag(Process->UserlandReply, _POOL_TAG);
			Process->UserlandReply = NULL;
		}

		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandAskUserRequest: IssueUserlandAskUserRequest returned\n"));
	}
	else
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("IssueUserlandAskUserRequest: KeWaitForSingleObject timed out\n"));

		//XXX need to remove pAskUserRequest from the list
	}


	/* at this point pAskUserRequest has already been deallocated in driver.c!DriverDeviceControl */


	return Action;
}



/*
 * InitUserland()
 *
 * Description:
 *		Initializes all the userland related data.
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
InitUserland()
{
	if (BootingUp == FALSE)
	{
		if (UserlandPostBootup() == FALSE)

			return FALSE;
	}


	return TRUE;
}



/*
 * ShutdownUserland()
 *
 * Description:
 *		XXX.
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
ShutdownUserland()
{
	PUSERLAND_REQUEST_HEADER	tmp;
	KIRQL						irql;


	if (UserlandRequestUserEventHandle)
		ZwClose(UserlandRequestUserEventHandle);


	KeAcquireSpinLock(&gUserlandRequestListSpinLock, &irql);
	{
		while (UserlandRequestList)
		{
			tmp = UserlandRequestList;
			UserlandRequestList = UserlandRequestList->Next;

			ExFreePoolWithTag(tmp, _POOL_TAG);
		}
	}
	KeReleaseSpinLock(&gUserlandRequestListSpinLock, irql);


	if (CachedSid)
	{
		ExFreePoolWithTag(CachedSid, _POOL_TAG);
		CachedSid = NULL;
	}

	if (CachedSidReply)
	{
		ExFreePoolWithTag(CachedSidReply, _POOL_TAG);
		CachedSidReply = NULL;
	}
}
