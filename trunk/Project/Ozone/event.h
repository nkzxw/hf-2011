/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		event.h
 *
 * Abstract:
 *
 *		This module defines various types used by event hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 09-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __EVENT_H__
#define __EVENT_H__



/*
 * ZwCreateEvent creates or opens an event object. [NAR]
 */

typedef NTSTATUS (*fpZwCreateEvent) (
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN EVENT_TYPE EventType,
	IN BOOLEAN InitialState
	);

NTSTATUS HookedNtCreateEvent(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN EVENT_TYPE EventType,
	IN BOOLEAN InitialState
	);


/*
 * ZwOpenEvent opens an event object. [NAR]
 */

typedef NTSTATUS (*fpZwOpenEvent) (
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS HookedNtOpenEvent(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);


/*
 * ZwCreateEventPair creates or opens an event pair object. [NAR]
 */

typedef NTSTATUS (*fpZwCreateEventPair) (
	OUT PHANDLE EventPairHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS HookedNtCreateEventPair(
	OUT PHANDLE EventPairHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);


/*
 * ZwOpenEventPair opens an event pair object. [NAR]
 */

typedef NTSTATUS (*fpZwOpenEventPair) (
	OUT PHANDLE EventPairHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
NTAPI
HookedNtOpenEventPair(
	OUT PHANDLE EventPairHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);



BOOLEAN InitEventHooks();


#endif	/* __EVENT_H__ */
