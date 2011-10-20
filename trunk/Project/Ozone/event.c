/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		event.c
 *
 * Abstract:
 *
 *		This module implements various event hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 09-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include "event.h"
#include "policy.h"
#include "pathproc.h"
#include "hookproc.h"
#include "accessmask.h"
#include "learn.h"
#include "log.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitEventHooks)
#endif


fpZwCreateEventPair	OriginalNtCreateEventPair = NULL;
fpZwOpenEventPair OriginalNtOpenEventPair = NULL;

fpZwCreateEvent	OriginalNtCreateEvent = NULL;
fpZwOpenEvent	OriginalNtOpenEvent = NULL;


/*
 * HookedNtCreateEvent()
 *
 * Description:
 *		This function mediates the NtCreateEvent() system service and checks the
 *		provided event name against the global and current process security policies.
 *
 *		NOTE: ZwCreateEvent creates or opens an event object. [NAR]
 *
 * Parameters:
 *		Those of NtCreateEvent().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateEvent().
 */

NTSTATUS
NTAPI
HookedNtCreateEvent
(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN EVENT_TYPE EventType,
	IN BOOLEAN InitialState
)
{
	PCHAR	FunctionName = "HookedNtCreateEvent";


	HOOK_ROUTINE_START(EVENT);


	ASSERT(OriginalNtCreateEvent);

	rc = OriginalNtCreateEvent(EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);


	HOOK_ROUTINE_FINISH(EVENT);
}



/*
 * HookedNtOpenEvent()
 *
 * Description:
 *		This function mediates the NtOpenEvent() system service and checks the
 *		provided event name against the global and current process security policies.
 *
 *		NOTE: ZwOpenEvent opens an event object. [NAR]
 *
 * Parameters:
 *		Those of NtOpenEvent().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtOpenEvent().
 */

NTSTATUS
NTAPI
HookedNtOpenEvent
(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
	PCHAR	FunctionName = "HookedNtOpenEvent";


	HOOK_ROUTINE_START(EVENT);


	ASSERT(OriginalNtOpenEvent);

	rc = OriginalNtOpenEvent(EventHandle, DesiredAccess, ObjectAttributes);


	HOOK_ROUTINE_FINISH(EVENT);
}



/*
 * HookedNtCreateEventPair()
 *
 * Description:
 *		This function mediates the NtCreateEventPair() system service and checks the
 *		provided eventpair name against the global and current process security policies.
 *
 *		NOTE: ZwCreateEventPair creates or opens an event pair object. [NAR]
 *
 * Parameters:
 *		Those of NtCreateEventPair().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateEventPair().
 */

NTSTATUS
NTAPI
HookedNtCreateEventPair
(
	OUT PHANDLE EventPairHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
	PCHAR	FunctionName = "HookedNtCreateEventPair";


	HOOK_ROUTINE_START(EVENT);


	ASSERT(OriginalNtCreateEventPair);

	rc = OriginalNtCreateEventPair(EventPairHandle, DesiredAccess, ObjectAttributes);


	HOOK_ROUTINE_FINISH(EVENT);
}




/*
 * HookedNtOpenEventPair()
 *
 * Description:
 *		This function mediates the NtOpenEventPair() system service and checks the
 *		provided event name against the global and current process security policies.
 *
 *		NOTE: ZwOpenEventPair opens an event pair object. [NAR]
 *
 * Parameters:
 *		Those of NtOpenEventPair().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtOpenEventPair().
 */

NTSTATUS
NTAPI
HookedNtOpenEventPair
(
	OUT PHANDLE EventPairHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
	PCHAR	FunctionName = "HookedNtOpenEventPair";


	HOOK_ROUTINE_START(EVENT);


	ASSERT(OriginalNtOpenEventPair);

	rc = OriginalNtOpenEventPair(EventPairHandle, DesiredAccess, ObjectAttributes);


	HOOK_ROUTINE_FINISH(EVENT);
}



/*
 * InitEventHooks()
 *
 * Description:
 *		Initializes all the mediated event operation pointers. The "OriginalFunction" pointers
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
InitEventHooks()
{
	if ( (OriginalNtCreateEventPair = (fpZwCreateEventPair) ZwCalls[ZW_CREATE_EVENT_PAIR_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_EVENT, LOG_PRIORITY_DEBUG, ("InitEventHooks: OriginalNtCreateEventPair is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtOpenEventPair = (fpZwOpenEventPair) ZwCalls[ZW_OPEN_EVENT_PAIR_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_EVENT, LOG_PRIORITY_DEBUG, ("InitEventHooks: OriginalNtOpenEventPair is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtCreateEvent = (fpZwCreateEvent) ZwCalls[ZW_CREATE_EVENT_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_EVENT, LOG_PRIORITY_DEBUG, ("InitEventHooks: OriginalNtCreateEvent is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtOpenEvent = (fpZwOpenEvent) ZwCalls[ZW_OPEN_EVENT_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_EVENT, LOG_PRIORITY_DEBUG, ("InitEventHooks: OriginalNtOpenEvent is NULL\n"));
		return FALSE;
	}

	return TRUE;
}
