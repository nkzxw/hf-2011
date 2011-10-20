/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		port.c
 *
 * Abstract:
 *
 *		This module implements various port object hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 25-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "port.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitPortHooks)
#endif


fpZwCreatePort			OriginalNtCreatePort = NULL;
fpZwCreateWaitablePort	OriginalNtCreateWaitablePort = NULL;

fpZwConnectPort			OriginalNtConnectPort = NULL;
fpZwSecureConnectPort	OriginalNtSecureConnectPort = NULL;


/*
 * HookedNtCreatePort()
 *
 * Description:
 *		This function mediates the NtCreatePort() system service and checks the
 *		provided port name against the global and current process security policies.
 *
 *		NOTE: ZwCreatePort creates a port object. [NAR]
 *
 * Parameters:
 *		Those of NtCreatePort().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreatePort().
 */

NTSTATUS
NTAPI
HookedNtCreatePort
(
	OUT PHANDLE PortHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG MaxDataSize,
	IN ULONG MaxMessageSize,
	IN ULONG Reserved
)
{
	PCHAR	FunctionName = "HookedNtCreatePort";


	HOOK_ROUTINE_START_OPTYPE(PORT, OP_PORT_CREATE);


	ASSERT(OriginalNtCreatePort);

	rc = OriginalNtCreatePort(PortHandle, ObjectAttributes, MaxDataSize, MaxMessageSize, Reserved);


	HOOK_ROUTINE_FINISH_OPTYPE(PORT, OP_PORT_CREATE);
}



/*
 * HookedNtCreateWaitablePort()
 *
 * Description:
 *		This function mediates the NtCreateWaitablePort() system service and checks the
 *		provided port name against the global and current process security policies.
 *
 *		NOTE: ZwCreateWaitablePort creates a waitable port object. [NAR]
 *
 * Parameters:
 *		Those of NtCreateWaitablePort().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateWaitablePort().
 */

NTSTATUS
NTAPI
HookedNtCreateWaitablePort
(
	OUT PHANDLE PortHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG MaxDataSize,
	IN ULONG MaxMessageSize,
	IN ULONG Reserved
)
{
	PCHAR	FunctionName = "HookedNtCreateWaitablePort";


	HOOK_ROUTINE_START_OPTYPE(PORT, OP_PORT_CREATE);


	ASSERT(OriginalNtCreateWaitablePort);

	rc = OriginalNtCreateWaitablePort(PortHandle, ObjectAttributes, MaxDataSize, MaxMessageSize, Reserved);


	HOOK_ROUTINE_FINISH_OPTYPE(PORT, OP_PORT_CREATE);
}



/*
 * HookedNtConnectPort()
 *
 * Description:
 *		This function mediates the NtConnectPort() system service and checks the
 *		provided port name against the global and current process security policies.
 *
 *		NOTE: ZwConnectPort creates a port connected to a named port. [NAR]
 *
 * Parameters:
 *		Those of NtConnectPort().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtConnectPort().
 */

NTSTATUS
NTAPI
HookedNtConnectPort
(
	OUT PHANDLE PortHandle,
	IN PUNICODE_STRING PortName,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	IN OUT PPORT_SECTION_WRITE WriteSection OPTIONAL,
	IN OUT PPORT_SECTION_READ ReadSection OPTIONAL,
	OUT PULONG MaxMessageSize OPTIONAL,
	IN OUT PVOID ConnectData OPTIONAL,
	IN OUT PULONG ConnectDataLength OPTIONAL
)
{
	PCHAR			FunctionName = "HookedNtConnectPort";
	UNICODE_STRING	usInputPortName;
	CHAR			PORTNAME[MAX_PATH];
	ANSI_STRING		asPortName;


	HOOK_ROUTINE_ENTER();


	if (!VerifyUnicodeString(PortName, &usInputPortName))
	{
		LOG(LOG_SS_PORT, LOG_PRIORITY_DEBUG, ("HookedNtConnectPort: VerifyUnicodeString failed\n"));
		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}


	if (_snprintf(PORTNAME, MAX_PATH, "%S", usInputPortName.Buffer) < 0)
	{
		LOG(LOG_SS_PORT, LOG_PRIORITY_DEBUG, ("%s: Port name '%S' is too long\n", FunctionName, usInputPortName.Buffer));
		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}


	if (LearningMode == FALSE)
	{
		POLICY_CHECK_OPTYPE_NAME(PORT, OP_PORT_CONNECT);
	}


	ASSERT(OriginalNtConnectPort);

	rc = OriginalNtConnectPort(PortHandle, PortName, SecurityQos, WriteSection, ReadSection, MaxMessageSize,
								ConnectData, ConnectDataLength);


	HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(PORT, PORTNAME, OP_PORT_CONNECT);
}



/*
 * HookedNtSecureConnectPort()
 *
 * Description:
 *		This function mediates the NtSecureConnectPort() system service and checks the
 *		provided port name against the global and current process security policies.
 *
 *		NOTE: ZwSecureConnectPort creates a port connected to a named port. [NAR]
 *
 * Parameters:
 *		Those of NtSecureConnectPort().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtSecureConnectPort().
 */

NTSTATUS
NTAPI
HookedNtSecureConnectPort
(
	OUT PHANDLE PortHandle,
	IN PUNICODE_STRING PortName,
	IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	IN OUT PPORT_SECTION_WRITE WriteSection OPTIONAL,
	IN PSID ServerSid OPTIONAL,
	IN OUT PPORT_SECTION_READ ReadSection OPTIONAL,
	OUT PULONG MaxMessageSize OPTIONAL,
	IN OUT PVOID ConnectData OPTIONAL,
	IN OUT PULONG ConnectDataLength OPTIONAL
)
{
	PCHAR			FunctionName = "HookedNtSecureConnectPort";
	UNICODE_STRING	usInputPortName;
	CHAR			PORTNAME[MAX_PATH];
	ANSI_STRING		asPortName;


	HOOK_ROUTINE_ENTER();


	if (!VerifyUnicodeString(PortName, &usInputPortName))
	{
		LOG(LOG_SS_PORT, LOG_PRIORITY_DEBUG, ("HookedNtSecureConnectPort: VerifyUnicodeString failed\n"));
		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}


	asPortName.Length = 0;
	asPortName.MaximumLength = MAX_PATH - 1;
	asPortName.Buffer = PORTNAME;

	if (! NT_SUCCESS(RtlUnicodeStringToAnsiString(&asPortName, &usInputPortName, FALSE)))
	{
		LOG(LOG_SS_PORT, LOG_PRIORITY_DEBUG, ("HookedNtSecureConnectPort: RtlUnicodeStringToAnsiString failed\n"));
		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}

	PORTNAME[asPortName.Length] = 0;


	if (LearningMode == FALSE)
	{
		POLICY_CHECK_OPTYPE_NAME(PORT, OP_PORT_CONNECT);
	}


	ASSERT(OriginalNtSecureConnectPort);

	rc = OriginalNtSecureConnectPort(PortHandle, PortName, SecurityQos, WriteSection, ServerSid, ReadSection,
								MaxMessageSize,	ConnectData, ConnectDataLength);


	HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(PORT, PORTNAME, OP_PORT_CONNECT);
}



/*
 * InitPortHooks()
 *
 * Description:
 *		Initializes all the mediated port operation pointers. The "OriginalFunction" pointers
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
InitPortHooks()
{
	if ( (OriginalNtCreatePort = (fpZwCreatePort) ZwCalls[ZW_CREATE_PORT_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_PORT, LOG_PRIORITY_DEBUG, ("InitPortHooks: OriginalNtCreatePort is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtCreateWaitablePort = (fpZwCreateWaitablePort) ZwCalls[ZW_CREATE_WAITPORT_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_PORT, LOG_PRIORITY_DEBUG, ("InitPortHooks: OriginalNtCreateWaitablePort is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtConnectPort = (fpZwConnectPort) ZwCalls[ZW_CONNECT_PORT_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_PORT, LOG_PRIORITY_DEBUG, ("InitPortHooks: OriginalNtConnectPort is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtSecureConnectPort = (fpZwSecureConnectPort) ZwCalls[ZW_SECURECONNECT_PORT_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_PORT, LOG_PRIORITY_DEBUG, ("InitPortHooks: OriginalNtSecureConnectPort is NULL\n"));
		return FALSE;
	}

	return TRUE;
}
