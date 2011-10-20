/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		semaphore.h
 *
 * Abstract:
 *
 *		This module defines various types used by semaphore hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 09-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__



/*
 * ZwCreateSemaphore creates or opens a semaphore object. [NAR]
 */

typedef NTSTATUS (*fpZwCreateSemaphore) (
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN LONG InitialCount,
	IN LONG MaximumCount
	);

NTSTATUS HookedNtCreateSemaphore(
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN LONG InitialCount,
	IN LONG MaximumCount
	);


/*
 * ZwOpenSemaphore opens a semaphore object. [NAR]
 */

typedef NTSTATUS (*fpZwOpenSemaphore) (
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS HookedNtOpenSemaphore(
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);



BOOLEAN InitSemaphoreHooks();


#endif	/* __SEMAPHORE_H__ */
