/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		timer.h
 *
 * Abstract:
 *
 *		This module defines various types used by timer object hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 25-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __TIMER_H__
#define __TIMER_H__


#include <NTDDK.h>
#include "policy.h"
#include "pathproc.h"
#include "hookproc.h"
#include "accessmask.h"
#include "learn.h"
#include "log.h"


/*
 * ZwCreateTimer creates or opens a timer object. [NAR]
 */

typedef NTSTATUS (*fpZwCreateTimer) (
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN TIMER_TYPE TimerType
	);

NTSTATUS
NTAPI
HookedNtCreateTimer(
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN TIMER_TYPE TimerType
	);


/*
 * ZwOpenTimer opens a timer object. [NAR]
 */

typedef NTSTATUS (*fpZwOpenTimer) (
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
NTAPI
HookedNtOpenTimer(
	OUT PHANDLE TimerHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);


BOOLEAN InitTimerHooks();


#endif	/* __TIMER_H__ */
