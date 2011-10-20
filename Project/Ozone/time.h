/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		time.h
 *
 * Abstract:
 *
 *		This module defines various types used by time hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 10-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __TIME_H__
#define __TIME_H__


#include <NTDDK.h>


/*
 * ZwSetSystemTime sets the system time. [NAR]
 */

typedef NTSTATUS (*fpZwSetSystemTime)(
	IN PLARGE_INTEGER NewTime,
	OUT PLARGE_INTEGER OldTime OPTIONAL
	);

NTSTATUS
NTAPI
HookedNtSetSystemTime(
	IN PLARGE_INTEGER NewTime,
	OUT PLARGE_INTEGER OldTime OPTIONAL
	);


/*
 * ZwSetTimerResolution sets the resolution of the system timer. [NAR]
 */

typedef NTSTATUS (*fpZwSetTimerResolution)(
	IN ULONG RequestedResolution,
	IN BOOLEAN Set,
	OUT PULONG ActualResolution
	);

NTSTATUS
NTAPI
HookedNtSetTimerResolution(
	IN ULONG RequestedResolution,
	IN BOOLEAN Set,
	OUT PULONG ActualResolution
	);


BOOLEAN InitTimeHooks();


#endif	/* __TIME_H__ */
