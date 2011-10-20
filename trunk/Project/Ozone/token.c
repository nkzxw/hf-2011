/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		token.c
 *
 * Abstract:
 *
 *		This module implements various token hooking routines.
 *		Token objects encapsulate the privileges and access rights of an agent
 *		(a thread or process).
 *
 * Author:
 *
 *		Eugene Tsyrklevich 25-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "token.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitTokenHooks)
#endif


fpZwAdjustPrivilegesToken	OriginalNtAdjustPrivilegesToken = NULL;
fpZwSetInformationToken		OriginalNtSetInformationToken = NULL;



/*
 * HookedNtAdjustPrivilegesToken()
 *
 * Description:
 *		This function mediates the NtAdjustPrivilegesToken() system service and XXX.
 *
 *		NOTE: ZwAdjustPrivilegesToken adjusts the attributes of the privileges in a token. [NAR]
 *
 * Parameters:
 *		Those of NtAdjustPrivilegesToken().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtAdjustPrivilegesToken().
 */

NTSTATUS
NTAPI
HookedNtAdjustPrivilegesToken
(
	IN HANDLE TokenHandle,
	IN BOOLEAN DisableAllPrivileges,
	IN PTOKEN_PRIVILEGES NewState,
	IN ULONG BufferLength,
	OUT PTOKEN_PRIVILEGES PreviousState OPTIONAL,
	OUT PULONG ReturnLength
)
{
	PCHAR			FunctionName = "HookedNtAdjustPrivilegesToken";
	PCHAR			TOKENNAME = NULL;	/* allow the use of POLICY_CHECK_OPTYPE_NAME() macro */
	ULONG			i;


	HOOK_ROUTINE_ENTER();

/*
	if (LearningMode == FALSE && IsTokenModificationAllowed() == FALSE)
	{
		LOG(LOG_SS_TOKEN, LOG_PRIORITY_DEBUG, ("%d (%S) HookedNtAdjustPrivilegesToken: disallowing token modification\n", (ULONG) PsGetCurrentProcessId(), GetCurrentProcessName()));

		LogAlert(ALERT_SS_TOKEN, OP_MODIFY, ALERT_RULE_NONE, ACTION_DENY, ALERT_PRIORITY_MEDIUM, NULL, 0, NULL);

		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}
*/
	if (LearningMode == FALSE)
	{
		POLICY_CHECK_OPTYPE_NAME(TOKEN, OP_TOKEN_MODIFY);
	}


	if (KeGetPreviousMode() != KernelMode && DisableAllPrivileges == FALSE && ARGUMENT_PRESENT(NewState))
	{
		BOOLEAN		CaughtException;

		__try
		{
			// Probe to make sure the first ULONG (PrivilegeCount) is accessible
			ProbeForRead(NewState, sizeof(ULONG), sizeof(ULONG));

			// Now probe the entire structure
			ProbeForRead(NewState, sizeof(TOKEN_PRIVILEGES) +
						(NewState->PrivilegeCount - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES),
						sizeof(ULONG));
		}

		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			NTSTATUS status = GetExceptionCode();

			LOG(LOG_SS_TOKEN, LOG_PRIORITY_DEBUG, ("HookedNtAdjustPrivilegesToken(): caught an exception. status = 0x%x\n", status));

			CaughtException = TRUE;
		}


		LOG(LOG_SS_TOKEN, LOG_PRIORITY_VERBOSE, ("%d HookedNtAdjustPrivilegesToken: %S\n", (ULONG) PsGetCurrentProcessId(), GetCurrentProcessName()));


		//XXX replace with PID lookup
/*
		if (CaughtException == FALSE &&
			wcsstr(GetCurrentProcessName(), L"svchost.exe") == 0 &&
			wcsstr(GetCurrentProcessName(), L"services.exe") == 0)
		{
			LOG(LOG_SS_TOKEN, LOG_PRIORITY_DEBUG, ("%d HookedNtAdjustPrivilegesToken\n", (ULONG) PsGetCurrentProcessId()));

			for (i = 0; i < NewState->PrivilegeCount; i++)
			{
				if (NewState->Privileges[i].Luid.LowPart == SE_AUDIT_PRIVILEGE && NewState->Privileges[i].Luid.HighPart == 0)
					;
				else
					KdPrint(("priv %d: %x %x %x\n", i, NewState->Privileges[i].Attributes, NewState->Privileges[i].Luid.LowPart, NewState->Privileges[i].Luid.HighPart));
			}
		}
*/
	}


	ASSERT(OriginalNtAdjustPrivilegesToken);

	rc = OriginalNtAdjustPrivilegesToken(TokenHandle, DisableAllPrivileges, NewState, BufferLength,
											PreviousState, ReturnLength);


	if (LearningMode == TRUE)
	{
		AddRule(RULE_TOKEN, NULL, OP_TOKEN_MODIFY);
	}

	HOOK_ROUTINE_EXIT(rc);
}



/*
 * HookedNtSetInformationToken()
 *
 * Description:
 *		This function mediates the NtSetInformationToken() system service and XXX.
 *
 *		NOTE: ZwSetInformationToken sets information affecting a token object. [NAR]
 *
 * Parameters:
 *		Those of NtSetInformationToken().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtSetInformationToken().
 */

NTSTATUS
NTAPI
HookedNtSetInformationToken
(
	IN HANDLE TokenHandle,
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	IN PVOID TokenInformation,
	IN ULONG TokenInformationLength
)
{
	PCHAR			FunctionName = "HookedNtSetInformationToken";
	PCHAR			TOKENNAME = NULL;	/* allow the use of POLICY_CHECK_OPTYPE_NAME() macro */


	HOOK_ROUTINE_ENTER();


	LOG(LOG_SS_TOKEN, LOG_PRIORITY_VERBOSE, ("%d HookedNtSetInformationToken %S %x %x %x\n", (ULONG) PsGetCurrentProcessId(), GetCurrentProcessName(), TokenInformationClass, TokenInformation, TokenInformationLength));

/*
	if (LearningMode == FALSE && IsTokenModificationAllowed() == FALSE)
	{
		LOG(LOG_SS_TOKEN, LOG_PRIORITY_DEBUG, ("%d (%S) HookedNtSetInformationToken: disallowing token modification\n", (ULONG) PsGetCurrentProcessId(), GetCurrentProcessName()));

		LogAlert(ALERT_SS_TOKEN, OP_MODIFY, ALERT_RULE_NONE, ACTION_DENY, ALERT_PRIORITY_MEDIUM, NULL, 0, NULL);

		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}
*/
	if (LearningMode == FALSE)
	{
		POLICY_CHECK_OPTYPE_NAME(TOKEN, OP_TOKEN_MODIFY);
	}


	ASSERT(OriginalNtSetInformationToken);

	rc = OriginalNtSetInformationToken(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength);


	if (LearningMode == TRUE)
	{
		AddRule(RULE_TOKEN, NULL, OP_TOKEN_MODIFY);
	}

	HOOK_ROUTINE_EXIT(rc);
}



/*
 * InitTokenHooks()
 *
 * Description:
 *		Initializes all the mediated token object operation pointers. The "OriginalFunction" pointers
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
InitTokenHooks()
{
	if ( (OriginalNtAdjustPrivilegesToken = (fpZwAdjustPrivilegesToken) ZwCalls[ZW_ADJUST_TOKEN_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_TOKEN, LOG_PRIORITY_DEBUG, ("InitTokenHooks: OriginalNtAdjustPrivilegesToken is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtSetInformationToken = (fpZwSetInformationToken) ZwCalls[ZW_SET_INFO_TOKEN_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_TOKEN, LOG_PRIORITY_DEBUG, ("InitTokenHooks: OriginalNtSetInformationToken is NULL\n"));
		return FALSE;
	}

	return TRUE;
}
