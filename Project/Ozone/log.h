/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		log.h
 *
 * Abstract:
 *
 *		This module defines various macros used for logging.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 17-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __LOG_H__
#define __LOG_H__


#include <NTDDK.h>
#include "ntproto.h"
#include "misc.h"


/* maximum number of alerts the kernel will queue */
#define	MAXIMUM_OUTSTANDING_ALERTS	1000

#define	LOG_USER_EVENT_NAME	L"\\BaseNamedObjects\\OzoneLogEvent"


/*
 * Logging SubSystems
 */

#define	LOG_SS_DRIVER_INTERNAL			(1 <<  0)
#define	LOG_SS_POLICY					(1 <<  1)
#define	LOG_SS_POLICY_PARSER			(1 <<  2)
#define	LOG_SS_FILE						(1 <<  3)
#define	LOG_SS_DIRECTORY				(1 <<  3)	// same as LOG_SS_FILE, used in HookedNtCreateFile
#define	LOG_SS_SEMAPHORE				(1 <<  4)
#define	LOG_SS_EVENT					(1 <<  5)
#define	LOG_SS_SECTION					(1 <<  6)
#define	LOG_SS_REGISTRY					(1 <<  7)
#define	LOG_SS_PROCESS					(1 <<  8)
#define	LOG_SS_HOOKPROC					(1 <<  9)
#define	LOG_SS_LEARN					(1 << 10)
#define	LOG_SS_PATHPROC					(1 << 11)
#define	LOG_SS_NETWORK					(1 << 12)
#define	LOG_SS_TIME						(1 << 13)
#define	LOG_SS_SYSINFO					(1 << 14)
#define	LOG_SS_JOB						(1 << 15)
#define	LOG_SS_MUTANT					(1 << 16)
#define	LOG_SS_PORT						(1 << 17)
#define	LOG_SS_SYMLINK					(1 << 18)
#define	LOG_SS_TIMER					(1 << 19)
#define	LOG_SS_TOKEN					(1 << 20)
#define	LOG_SS_NAMEDPIPE				(1 << 21)
#define	LOG_SS_MAILSLOT					(1 << 22)
#define	LOG_SS_DRIVER					(1 << 23)
#define	LOG_SS_DIROBJ					(1 << 24)
#define	LOG_SS_ATOM						(1 << 25)
#define	LOG_SS_VDM						(1 << 26)
#define	LOG_SS_DEBUG					(1 << 27)
#define	LOG_SS_DRIVE					(1 << 28)
#define	LOG_SS_MISC						(1 << 29)


/* log the following subsytems */

#define	LOG_SUBSYSTEMS						(LOG_SS_ATOM			|	\
											LOG_SS_DIROBJ			|	\
											LOG_SS_DRIVER			|	\
											LOG_SS_DRIVER_INTERNAL	|	\
											LOG_SS_EVENT			|	\
											LOG_SS_FILE				|	\
											LOG_SS_HOOKPROC			|	\
											LOG_SS_JOB				|	\
											LOG_SS_LEARN			|	\
											LOG_SS_MAILSLOT			|	\
											LOG_SS_MISC				|	\
											LOG_SS_MUTANT			|	\
											LOG_SS_NAMEDPIPE		|	\
											LOG_SS_NETWORK			|	\
											LOG_SS_PATHPROC			|	\
											LOG_SS_POLICY			|	\
											LOG_SS_POLICY_PARSER	|	\
											LOG_SS_PORT				|	\
											LOG_SS_PROCESS			|	\
											LOG_SS_REGISTRY			|	\
											LOG_SS_SECTION			|	\
											LOG_SS_SEMAPHORE		|	\
											LOG_SS_SYMLINK			|	\
											LOG_SS_SYSINFO			|	\
											LOG_SS_TIME				|	\
											LOG_SS_TIMER			|	\
											LOG_SS_TOKEN			|	\
											LOG_SS_VDM				|	\
											LOG_SS_DRIVE			|	\
											LOG_SS_DEBUG)

#define	LOG_PRIORITY_VERBOSE		1
#define	LOG_PRIORITY_DEBUG			2
#define	LOG_PRIORITY_WARNING		3
#define	LOG_PRIORITY_ERROR			4
#define	LOG_PRIORITY_CRITICAL		5


#define	MINIMUM_LOGGING_PRIORITY	LOG_PRIORITY_DEBUG


#define	LOG(subsystem, priority, msg)					\
	do {												\
		if (priority >= LOG_PRIORITY_WARNING) {			\
			DbgPrint msg;								\
		} else if (priority > MINIMUM_LOGGING_PRIORITY){\
			KdPrint(msg);								\
		} else if (subsystem & LOG_SUBSYSTEMS) {		\
			if (priority == MINIMUM_LOGGING_PRIORITY) {	\
				KdPrint(msg);							\
			}											\
		}												\
	} while(0)



/*
 * Alert SubSystems (sorted in the same order as RULEs in policy.h)
 *
 * We have an alert subsystem for each category of alert that Ozone generates.
 * (These are mostly the same as RuleTypes plus several extra ones)
 */

#define	ALERT_SS_FILE							0
#define	ALERT_SS_DIRECTORY						1
#define	ALERT_SS_MAILSLOT					    2
#define	ALERT_SS_NAMEDPIPE					    3
#define	ALERT_SS_REGISTRY						4
#define	ALERT_SS_SECTION						5
#define	ALERT_SS_DLL							6
#define	ALERT_SS_EVENT							7
#define	ALERT_SS_SEMAPHORE						8
#define	ALERT_SS_JOB						    9
#define	ALERT_SS_MUTANT						   10
#define	ALERT_SS_PORT						   11
#define	ALERT_SS_SYMLINK					   12
#define	ALERT_SS_TIMER						   13
#define	ALERT_SS_PROCESS					   14
#define	ALERT_SS_DRIVER						   15
#define	ALERT_SS_DIROBJ						   16
#define	ALERT_SS_ATOM						   17

#define	ALERT_SS_NETWORK					   18
#define	ALERT_SS_SERVICE					   19
#define	ALERT_SS_TIME						   20
#define	ALERT_SS_TOKEN						   21
#define	ALERT_SS_SYSCALL					   22
#define	ALERT_SS_VDM						   23
#define	ALERT_SS_DEBUG						   24
#define	ALERT_SS_BOPROT						   25


/* 
 * Alert Rules
 */

#define	ALERT_RULE_NONE							0

#define	ALERT_RULE_PROCESS_EXEC_2EXTS			1		// Executing a binary with more than one extension
#define	ALERT_RULE_PROCESS_EXEC_UNKNOWN			2		// Executing a binary with an unknown extension
#define	ALERT_RULE_PROCESS_EXEC_NOEXT			3		// Executing binary without an extension

#define	ALERT_RULE_BOPROT_INVALIDCALL			1		// System call originating directly from user code


/* defined in policy.h */
typedef unsigned char ACTION_TYPE;
typedef struct _POLICY_RULE POLICY_RULE, *PPOLICY_RULE;
typedef enum _AlertPriority ALERT_PRIORITY;


#pragma pack(push, 1)
typedef	struct _SECURITY_ALERT
{
	struct _SECURITY_ALERT	*Next;

	/* size of the entire alert = sizeof(SECURITY_ALERT) + strlen(ObjectName) + sizeof(SID) */
	USHORT					Size;
	UCHAR					AlertSubsystem;
	UCHAR					AlertType;
	UCHAR					AlertRuleNumber;
	UCHAR/*ALERT_PRIORITY*/	Priority;
	UCHAR/*ACTION_TYPE*/	Action;				/* UINT8 Action that was taken (denied, logged) */
	ULONG					ProcessId;
	USHORT					ObjectNameLength;
	USHORT					ProcessNameLength;
	USHORT					PolicyNameLength;
	USHORT					PolicyLineNumber;

	/* space for ObjectName, ProcessName, PolicyName and SID are dynamically allocated */
	WCHAR					ObjectName[ANYSIZE_ARRAY];

	/* ProcessName follows the zero-terminated ObjectName */
//	WCHAR					ProcessName[ANYSIZE_ARRAY];

	/* PolicyName follows the zero-terminated ProcessName */
//	WCHAR					PolicyName[ANYSIZE_ARRAY];

	/* SID follows the zero-terminated PolicyName */
//	SID_AND_ATTRIBUTES		UserInfo;

} SECURITY_ALERT, *PSECURITY_ALERT;
#pragma pack(pop)


extern KSPIN_LOCK			gLogSpinLock;
extern PSECURITY_ALERT		LogList;
extern PSECURITY_ALERT		LastAlert;
extern USHORT				NumberOfAlerts;


BOOLEAN			InitLog();
VOID			ShutdownLog();
VOID			LogAlert(UCHAR AlertSubSystem, UCHAR OperationType, UCHAR AlertRuleNumber, ACTION_TYPE ActionTaken, ALERT_PRIORITY AlertPriority, PWSTR PolicyFilename, USHORT PolicyLineNumber, PCHAR ObjectName);
ALERT_PRIORITY	GetObjectAccessAlertPriority(UCHAR AlertSubSystem, UCHAR Operation, ACTION_TYPE ActionTaken);
BOOLEAN			LogPostBootup();

PCHAR			FilterObjectName(PCHAR ObjectName);


#endif	/* __LOG_H__ */
