/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		userland.h
 *
 * Abstract:
 *
 *		This module defines various types used by userland interacting routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 18-Apr-2004
 *
 * Revision History:
 *
 *		None.
 */

#ifndef __USERLAND_H__
#define __USERLAND_H__


#include <NTDDK.h>
#include "policy.h"
#include "misc.h"


/* number of seconds to wait for userland agent to reply */
#define	USERLAND_REQUEST_TIMEOUT		5


#define	USERLAND_SID_RESOLVE_REQUEST	1
#define	USERLAND_ASK_USER_REQUEST		2


/*
 * all userland requests start with the following header
 */

typedef struct _USERLAND_REQUEST_HEADER
{
	struct _USERLAND_REQUEST_HEADER	*Next;

	USHORT						RequestType;
	USHORT						RequestSize;
	ULONG						ProcessId;
	UCHAR						SeqId;				/* Sequence id, will roll over but that's fine */

} USERLAND_REQUEST_HEADER, *PUSERLAND_REQUEST_HEADER;


/* binary SID -> ASCII name resolve request */

typedef struct _SID_RESOLVE_REQUEST
{
	USERLAND_REQUEST_HEADER		RequestHeader;
	PSID_AND_ATTRIBUTES			PUserSidAndAttributes;

} SID_RESOLVE_REQUEST, *PSID_RESOLVE_REQUEST;


/* Ask user request */

typedef struct _ASK_USER_REQUEST
{
	USERLAND_REQUEST_HEADER		RequestHeader;
	RULE_TYPE					RuleType;
	UCHAR						OperationType;
	USHORT						ObjectNameLength;
	USHORT						ProcessNameLength;

	WCHAR						ObjectName[ANYSIZE_ARRAY];

	/* ProcessName follows the zero-terminated ObjectName */
//	WCHAR						ProcessName[ANYSIZE_ARRAY];

} ASK_USER_REQUEST, *PASK_USER_REQUEST;



/*
 * all userland replies start with the following header
 */

typedef struct _USERLAND_REPLY_HEADER
{
	ULONG						ProcessId;
	USHORT						ReplySize;
	UCHAR						SeqId;				/* Sequence id, will roll over but that's fine */

} USERLAND_REPLY_HEADER, *PUSERLAND_REPLY_HEADER;


/* binary SID -> ASCII name resolve reply */

typedef struct _SID_RESOLVE_REPLY
{
	USERLAND_REPLY_HEADER		ReplyHeader;
	USHORT						UserNameLength;
	WCHAR						UserName[ANYSIZE_ARRAY];

} SID_RESOLVE_REPLY, *PSID_RESOLVE_REPLY;


/* Ask user reply */

typedef struct _ASK_USER_REPLY
{
	USERLAND_REPLY_HEADER		ReplyHeader;
	ACTION_TYPE					Action;

} ASK_USER_REPLY, *PASK_USER_REPLY;


extern BOOLEAN						ActiveUserAgent;
extern PUSERLAND_REQUEST_HEADER		UserlandRequestList;
extern KSPIN_LOCK					gUserlandRequestListSpinLock;
extern PKEVENT						UserlandRequestUserEvent;


BOOLEAN	InitUserland();
BOOLEAN	UserlandPostBootup();
VOID	ShutdownUserland();

typedef struct _IMAGE_PID_ENTRY *PIMAGE_PID_ENTRY;

BOOLEAN	IssueUserlandSidResolveRequest(PIMAGE_PID_ENTRY Process);
ACTION_TYPE	IssueUserlandAskUserRequest(RULE_TYPE RuleType, UCHAR OperationType, PCHAR ObjectName);


#endif	/* __USERLAND_H__ */