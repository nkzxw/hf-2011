/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		policy.h
 *
 * Abstract:
 *
 *		This module defines various types used by security policy related routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 16-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __POLICY_H__
#define __POLICY_H__


#define	POLICY_MAX_SERVICE_NAME_LENGTH		64
#define	POLICY_MAX_OBJECT_NAME_LENGTH		192
#define	POLICY_MAX_RULE_LENGTH				256

// maximum number of '*' characters in a regex
#define	POLICY_TOTAL_NUMBER_OF_STARS		5

#define	isalpha(c)	( ((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') )

#if 0
typedef enum _ActionType
{
	ACTION_NONE=0,
	ACTION_PERMIT,
	ACTION_PERMIT_DEFAULT,
	ACTION_LOG,
	ACTION_LOG_DEFAULT,
	ACTION_PROCESS,				/* further processing is required									*/
	ACTION_RESERVED1,
	ACTION_RESERVED2,
	ACTION_RESERVED3,
	ACTION_TERMINATE,			/* terminate process												*/
	ACTION_ASK,					/* XXX user prompt (interactive session only?)						*/
	ACTION_ASK_PERMIT,			/* User chose permit												*/
	ACTION_ASK_LOG,				/* User chose log													*/
	ACTION_ASK_TERMINATE,		/* User chose terminate												*/
	ACTION_DENY,				/* all actions listed after ACTION_DENY are treated as DENY actions	*/
	ACTION_ASK_DENY,			/* User chose deny													*/
	ACTION_DENY_DEFAULT,		/* default deny policy action, used to distinguish between default and explicit deny actions */
	ACTION_QUIETDENY,			/* deny but do not log												*/
	ACTION_QUIETDENY_DEFAULT,	/* deny but do not log (default action)								*/

} ACTION_TYPE;
#endif


typedef unsigned char	ACTION_TYPE;

#define	ACTION_DEFAULT				(1 << 7)
#define	ACTION_DENY					(1 << 6)
#define	ACTION_PERMIT				(1 << 5)
#define	ACTION_LOG					(1 << 4)
#define	ACTION_TERMINATE			(1 << 3)

#define	ACTION_NONE					 0
#define	ACTION_ASK					 1
#define	ACTION_ASK_PERMIT			(ACTION_ASK | ACTION_PERMIT)
#define	ACTION_ASK_LOG				(ACTION_ASK | ACTION_LOG)
#define	ACTION_ASK_TERMINATE		(ACTION_ASK | ACTION_TERMINATE)
#define	ACTION_ASK_DENY				(ACTION_ASK | ACTION_DENY)
#define	ACTION_QUIETDENY			(2 | ACTION_DENY)
#define	ACTION_PROCESS				 3
#define	ACTION_RESERVED1			 4
#define	ACTION_RESERVED2			 5
#define	ACTION_RESERVED3			 6
#define	ACTION_RESERVED4			 7

#define	ACTION_DENY_DEFAULT			(ACTION_DENY | ACTION_DEFAULT)
#define	ACTION_PERMIT_DEFAULT		(ACTION_PERMIT | ACTION_DEFAULT)
#define	ACTION_LOG_DEFAULT			(ACTION_LOG | ACTION_DEFAULT)
#define	ACTION_QUIETDENY_DEFAULT	(ACTION_QUIETDENY | ACTION_DEFAULT)
#define	ACTION_ASK_DEFAULT			(ACTION_ASK | ACTION_DEFAULT)


#define	DEFAULT_POLICY_ACTION	ACTION_PERMIT_DEFAULT


/*
 * WARNING: ObjectParseOps (policy.c) && RuleTypeData (learn.c) structures depend on the order of the
 * following enum values
 *
 * RuleType enumerates all possible object types.
 * (in C++ we would have a separate class for each)
 */

typedef enum _RuleType
{
	RULE_FILE = 0,
	RULE_DIRECTORY,
	RULE_MAILSLOT,
	RULE_NAMEDPIPE,
	RULE_REGISTRY,
	RULE_SECTION,
	RULE_DLL,
	RULE_EVENT,
	RULE_SEMAPHORE,
	RULE_JOB,
	RULE_MUTANT,
	RULE_PORT,
	RULE_SYMLINK,
	RULE_TIMER,
	RULE_PROCESS,
	RULE_DRIVER,
	RULE_DIROBJ,
	RULE_ATOM,
	
	RULE_NETWORK,
	RULE_SERVICE,
	RULE_TIME,
	RULE_TOKEN,
	RULE_SYSCALL,
	RULE_LASTONE,		/* not a real rule, just a convinient way of iterating through all rules (i < RULE_LASTONE) */

} RULE_TYPE;


typedef enum _MatchType
{
	MATCH_SINGLE = 0,
	MATCH_WILDCARD,
	MATCH_ALL,
	MATCH_NONE

} MATCH_TYPE;


typedef enum _AlertPriority
{
	ALERT_PRIORITY_HIGH = 1,
	ALERT_PRIORITY_MEDIUM,
	ALERT_PRIORITY_LOW,
	ALERT_PRIORITY_INFO,

} ALERT_PRIORITY;


/*
 * Operation Types
 */


#define	OP_INVALID			0x00
#define	OP_NONE				0x00

// file ops

#define	OP_READ				0x01
#define	OP_WRITE			0x02
#define	OP_READ_WRITE		(OP_READ | OP_WRITE)
#define	OP_EXECUTE			0x04
#define	OP_DELETE			0x08
#define	OP_APPEND			0x10

// dirobj & job ops
#define	OP_CREATE			0x01
#define	OP_OPEN				0x02

// directory ops

#define OP_DIR_TRAVERSE		0x01
#define OP_DIR_CREATE		0x02

// process ops

#define	OP_PROC_EXECUTE		0x01
#define	OP_PROC_OPEN		0x02

// port ops

#define	OP_PORT_CONNECT		0x01
#define	OP_PORT_CREATE		0x02

// network ops

#define	OP_TCPCONNECT		0x01
#define	OP_UDPCONNECT		0x02
#define	OP_CONNECT			0x03
#define	OP_BIND				0x04

// atom ops

#define	OP_FIND				0x01
#define	OP_ADD				0x02

// service ops

#define	OP_SERVICE_START	0x01
#define	OP_SERVICE_STOP		0x02
#define	OP_SERVICE_CREATE	0x03
#define	OP_SERVICE_DELETE	0x04

// dll/driver ops

#define	OP_LOAD				0x01
#define	OP_REGLOAD			0x02
#define	OP_UNLOAD			0x03	// XXX 0x04?

// time change op

#define OP_TIME_CHANGE		0x01

// vdm ops

#define OP_VDM_USE			0x01

// debug ops

#define OP_DEBUG			0x01

// token ops

#define OP_TOKEN_MODIFY		0x01

// buffer overflow protection "virtual op"

#define OP_INVALIDCALL		0x01


#define	OP_ALL				0xFF


// forward declaration
typedef struct _SECURITY_POLICY SECURITY_POLICY, *PSECURITY_POLICY;


/* Rule should really be a class */

typedef struct _POLICY_RULE
{
	struct _POLICY_RULE		*Next;

	PSECURITY_POLICY		pSecurityPolicy;

	ACTION_TYPE				ActionType;
	MATCH_TYPE				MatchType;
	UCHAR					OperationType;

	UCHAR					RuleNumber;					/* is used to associate text descriptions with certain rules */

	USHORT					PolicyLineNumber;			/* line number in the policy file */

	/* 
	 * the majority of rules use the struct below to hold information about string objects they represent
	 * RULE_SYSCALL though does not have any names associated with it and uses ServiceBitArray to create
	 * a bit index for all system calls. Both Name & ServiceBitArray are allocated dynamically.
	 * (in C++ we would have 2 different classes for this)
	 */
	union
	{
		struct
		{
			USHORT			NameLength;
			CHAR			Name[ANYSIZE_ARRAY];
		};

		ULONG				ServiceBitArray[ANYSIZE_ARRAY];
	};

} POLICY_RULE, *PPOLICY_RULE;



typedef struct _SECURITY_POLICY
{
	PPOLICY_RULE		RuleList[RULE_LASTONE];

	KSPIN_LOCK			SpinLock;

	BOOLEAN				Initialized;				/* Has this policy been initialized already? */


#define	PROTECTION_OVERFLOW			(1 << 0)
#define	PROTECTION_USERLAND			(1 << 1)
#define	PROTECTION_DEBUGGING		(1 << 2)
#define	PROTECTION_VDM				(1 << 3)
#define	PROTECTION_KEYBOARD			(1 << 4)
#define	PROTECTION_MODEM			(1 << 5)
#define	PROTECTION_SNIFFER			(1 << 6)
#define	PROTECTION_EXTENSION		(1 << 7)

	USHORT				ProtectionFlags;

	ACTION_TYPE			DefaultPolicyAction;

	PWSTR				Name;

} SECURITY_POLICY, *PSECURITY_POLICY;


#define	IS_OVERFLOW_PROTECTION_ON(SecPolicy)	(((SecPolicy).ProtectionFlags & PROTECTION_OVERFLOW) == PROTECTION_OVERFLOW)
#define	IS_USERLAND_PROTECTION_ON(SecPolicy)	(((SecPolicy).ProtectionFlags & PROTECTION_USERLAND) == PROTECTION_USERLAND)
#define	IS_DEBUGGING_PROTECTION_ON(SecPolicy)	(((SecPolicy).ProtectionFlags & PROTECTION_DEBUGGING) == PROTECTION_DEBUGGING)
#define	IS_VDM_PROTECTION_ON(SecPolicy)			(((SecPolicy).ProtectionFlags & PROTECTION_VDM) == PROTECTION_VDM)
#define	IS_EXTENSION_PROTECTION_ON(SecPolicy)	(((SecPolicy).ProtectionFlags & PROTECTION_EXTENSION) == PROTECTION_EXTENSION)

#define	TURN_DEBUGGING_PROTECTION_OFF(SecPolicy) ((SecPolicy).ProtectionFlags &= ~PROTECTION_DEBUGGING)
#define	TURN_VDM_PROTECTION_OFF(SecPolicy)		((SecPolicy).ProtectionFlags &= ~PROTECTION_VDM)
#define	TURN_EXTENSION_PROTECTION_OFF(SecPolicy) ((SecPolicy).ProtectionFlags &= ~PROTECTION_EXTENSION)


#define	PROTECTION_ALL_ON	0xFFFF
#define	PROTECTION_ALL_OFF	0x0000

#define	INVALID_OBJECT_SIZE	(-1)


extern SECURITY_POLICY	gSecPolicy;
extern CHAR				SystemDrive, SystemRoot[], SystemRootUnresolved[], *SystemRootDirectory, CDrive[];
extern USHORT			SystemRootLength, SystemRootUnresolvedLength, SystemRootDirectoryLength, CDriveLength;
extern ULONG			NumberOfBitsInUlong, UlongBitShift;


#define	WILDCARD_MATCH		1
#define	WILDCARD_NO_MATCH	0


BOOLEAN	InitPolicy();
void	PolicyRemove();
void	PolicyDelete(IN PSECURITY_POLICY pSecPolicy);
BOOLEAN	LoadSecurityPolicy(OUT PSECURITY_POLICY pSecPolicy, IN PWSTR PolicyFile, IN PWSTR FilePath);
BOOLEAN	FindAndLoadSecurityPolicy(OUT PSECURITY_POLICY pSecPolicy, IN PWSTR filename, IN PWSTR UserName);
ACTION_TYPE	PolicyCheck(RULE_TYPE RuleType, PCHAR Object, UCHAR OperationType, UCHAR *RuleNumber, PWSTR *PolicyFilename, USHORT *PolicyLineNumber);
BOOLEAN	PolicyParseObjectRule(PSECURITY_POLICY pSecPolicy, RULE_TYPE RuleType, PCHAR Operation, PCHAR rule);
VOID	InsertPolicyRule(PSECURITY_POLICY pSecPolicy, PPOLICY_RULE PolicyRule, RULE_TYPE RuleType);
BOOLEAN	PolicyPostBootup();
int		WildcardMatch(PCHAR path, PCHAR regex);


#endif	/* __POLICY_H__ */
