/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		port.h
 *
 * Abstract:
 *
 *		This module defines various types used by port object hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 25-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __PORT_H__
#define __PORT_H__


#include <NTDDK.h>
#include "policy.h"
#include "pathproc.h"
#include "hookproc.h"
#include "accessmask.h"
#include "learn.h"
#include "log.h"


/*
 * ZwCreatePort creates a port object. [NAR]
 */

typedef NTSTATUS (*fpZwCreatePort) (
	OUT PHANDLE PortHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG MaxDataSize,
	IN ULONG MaxMessageSize,
	IN ULONG Reserved
	);

NTSTATUS
NTAPI
HookedNtCreatePort(
	OUT PHANDLE PortHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG MaxDataSize,
	IN ULONG MaxMessageSize,
	IN ULONG Reserved
	);


/*
 * ZwCreateWaitablePort creates a waitable port object. [NAR]
 */

typedef NTSTATUS (*fpZwCreateWaitablePort) (
	OUT PHANDLE PortHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG MaxDataSize,
	IN ULONG MaxMessageSize,
	IN ULONG Reserved
	);

NTSTATUS
NTAPI
HookedNtCreateWaitablePort(
	OUT PHANDLE PortHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG MaxDataSize,
	IN ULONG MaxMessageSize,
	IN ULONG Reserved
	);



typedef struct _PORT_SECTION_READ {
	ULONG Length;
	ULONG ViewSize;
	ULONG ViewBase;
} PORT_SECTION_READ, *PPORT_SECTION_READ;

typedef struct _PORT_SECTION_WRITE {
	ULONG Length;
	HANDLE SectionHandle;
	ULONG SectionOffset;
	ULONG ViewSize;
	PVOID ViewBase;
	PVOID TargetViewBase;
} PORT_SECTION_WRITE, *PPORT_SECTION_WRITE;


/*
 * ZwConnectPort creates a port connected to a named port. [NAR]
 */

typedef NTSTATUS (*fpZwConnectPort) (
	OUT PHANDLE PortHandle,
	IN PUNICODE_STRING PortName,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	IN OUT PPORT_SECTION_WRITE WriteSection OPTIONAL,
	IN OUT PPORT_SECTION_READ ReadSection OPTIONAL,
	OUT PULONG MaxMessageSize OPTIONAL,
	IN OUT PVOID ConnectData OPTIONAL,
	IN OUT PULONG ConnectDataLength OPTIONAL
	);

NTSTATUS
NTAPI
HookedNtConnectPort(
	OUT PHANDLE PortHandle,
	IN PUNICODE_STRING PortName,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	IN OUT PPORT_SECTION_WRITE WriteSection OPTIONAL,
	IN OUT PPORT_SECTION_READ ReadSection OPTIONAL,
	OUT PULONG MaxMessageSize OPTIONAL,
	IN OUT PVOID ConnectData OPTIONAL,
	IN OUT PULONG ConnectDataLength OPTIONAL
	);


/*
 * ZwSecureConnectPort creates a port connected to a named port. [NAR]
 */

typedef NTSTATUS (*fpZwSecureConnectPort) (
	OUT PHANDLE PortHandle,
	IN PUNICODE_STRING PortName,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	IN OUT PPORT_SECTION_WRITE WriteSection OPTIONAL,
	IN PSID ServerSid OPTIONAL,
	IN OUT PPORT_SECTION_READ ReadSection OPTIONAL,
	OUT PULONG MaxMessageSize OPTIONAL,
	IN OUT PVOID ConnectData OPTIONAL,
	IN OUT PULONG ConnectDataLength OPTIONAL
	);

NTSTATUS
NTAPI
HookedNtSecureConnectPort(
	OUT PHANDLE PortHandle,
	IN PUNICODE_STRING PortName,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	IN OUT PPORT_SECTION_WRITE WriteSection OPTIONAL,
	IN PSID ServerSid OPTIONAL,
	IN OUT PPORT_SECTION_READ ReadSection OPTIONAL,
	OUT PULONG MaxMessageSize OPTIONAL,
	IN OUT PVOID ConnectData OPTIONAL,
	IN OUT PULONG ConnectDataLength OPTIONAL
	);



BOOLEAN InitPortHooks();


#endif	/* __PORT_H__ */
