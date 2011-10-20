/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		hookproc.h
 *
 * Abstract:
 *
 *		This module definies various types used by service operation (system call) hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 16-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __HOOKPROC_H__
#define __HOOKPROC_H__


#include "userland.h"


/* should the following calls be intercepted? */

#define	HOOK_EVENT		1
#define	HOOK_FILE		1
#define	HOOK_DIROBJ		1
#define	HOOK_JOB		1
#define	HOOK_NETWORK	1
#define	HOOK_MUTANT		1
#define	HOOK_PORT		1
#define	HOOK_PROCESS	1
#define	HOOK_REGISTRY	1
#define	HOOK_SECTION	1
#define	HOOK_SEMAPHORE	1
#define	HOOK_SYMLINK	1
#define	HOOK_SYSINFO	1
#define	HOOK_TIME		1
#define	HOOK_TIMER		1
#define	HOOK_TOKEN		1
#define	HOOK_DRIVEROBJ	1
#define	HOOK_ATOM		1
#define	HOOK_VDM		1
#define	HOOK_SYSCALLS	0
#define	HOOK_DEBUG		1
#define	HOOK_MEDIA		1
#define	HOOK_BOPROT		0


#pragma pack(push, 1)
typedef struct _SERVICE_TABLE_DESCRIPTOR {

	PULONG	ServiceTableBase;		/* table of function pointers		*/
	PVOID	ServiceCounterTable;	/* used in checked build only		*/
	ULONG	NumberOfServices;		/* number of services in this table	*/
	/* extra LONG on IA64 goes here */
	PVOID	ParamTableBase;			/* number of parameters				*/

} SERVICE_TABLE_DESCRIPTOR, *PSERVICE_TABLE_DESCRIPTOR;
#pragma pack(pop)


/*
 * The Service Descriptor Table index (4 bytes following the mov opcode)
 *
 * The index format is as follows:
 *
 * Leading 18 bits are all zeroes
 * Following 2 bits are system service table index (3 bits on Win64)
 * Following 12 bits are service number
 */

#define	SERVICE_TABLE_INDEX_BITS	2
#define	NUMBER_SERVICE_TABLES		(1 << SERVICE_TABLE_INDEX_BITS)

#define	SERVICE_ID_NUMBER_BITS		12
#define	SERVICE_ID_NUMBER_MASK		((1 << SERVICE_ID_NUMBER_BITS) - 1)


/*
 * The kernel's service descriptor table, which is used to find the address
 * of the service dispatch tables to use for a service ID.
 *
 * Descriptor 0 is used for core services (NTDLL)
 * Descriptor 1 is used for GUI services (WIN32K)
 * Descriptors 2 and 3 are unused on current versions of Windows NT.
 */

__declspec(dllimport) SERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable[NUMBER_SERVICE_TABLES];


/*
 * not exported
 */

//PSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTableShadow;



void SystemCallHandler0(); void SystemCallHandler1(); void SystemCallHandler2(); void SystemCallHandler3();
void SystemCallHandler4(); void SystemCallHandler5(); void SystemCallHandler6(); void SystemCallHandler7();
void SystemCallHandler8(); void SystemCallHandler9(); void SystemCallHandler10(); void SystemCallHandler11();
void SystemCallHandler12(); void SystemCallHandler13(); void SystemCallHandler14(); void SystemCallHandler15();
void SystemCallHandler16(); void SystemCallHandler17(); void SystemCallHandler18(); void SystemCallHandler19();
void SystemCallHandler20(); void SystemCallHandler21(); void SystemCallHandler22(); void SystemCallHandler23();
void SystemCallHandler24(); void SystemCallHandler25(); void SystemCallHandler26(); void SystemCallHandler27();
void SystemCallHandler28(); void SystemCallHandler29(); void SystemCallHandler30(); void SystemCallHandler31();
void SystemCallHandler32(); void SystemCallHandler33(); void SystemCallHandler34(); void SystemCallHandler35();
void SystemCallHandler36(); void SystemCallHandler37(); void SystemCallHandler38(); void SystemCallHandler39();
void SystemCallHandler40(); void SystemCallHandler41(); void SystemCallHandler42(); void SystemCallHandler43();
void SystemCallHandler44(); void SystemCallHandler45(); void SystemCallHandler46(); void SystemCallHandler47();
void SystemCallHandler48(); void SystemCallHandler49(); void SystemCallHandler50(); void SystemCallHandler51();
void SystemCallHandler52(); void SystemCallHandler53(); void SystemCallHandler54(); void SystemCallHandler55();
void SystemCallHandler56(); void SystemCallHandler57(); void SystemCallHandler58(); void SystemCallHandler59();
void SystemCallHandler60(); void SystemCallHandler61(); void SystemCallHandler62(); void SystemCallHandler63();
void SystemCallHandler64(); void SystemCallHandler65(); void SystemCallHandler66(); void SystemCallHandler67();
void SystemCallHandler68(); void SystemCallHandler69(); void SystemCallHandler70(); void SystemCallHandler71();
void SystemCallHandler72(); void SystemCallHandler73(); void SystemCallHandler74(); void SystemCallHandler75();
void SystemCallHandler76(); void SystemCallHandler77(); void SystemCallHandler78(); void SystemCallHandler79();
void SystemCallHandler80(); void SystemCallHandler81(); void SystemCallHandler82(); void SystemCallHandler83();
void SystemCallHandler84(); void SystemCallHandler85(); void SystemCallHandler86(); void SystemCallHandler87();
void SystemCallHandler88(); void SystemCallHandler89(); void SystemCallHandler90(); void SystemCallHandler91();
void SystemCallHandler92(); void SystemCallHandler93(); void SystemCallHandler94(); void SystemCallHandler95();
void SystemCallHandler96(); void SystemCallHandler97(); void SystemCallHandler98(); void SystemCallHandler99();
void SystemCallHandler100(); void SystemCallHandler101(); void SystemCallHandler102(); void SystemCallHandler103();
void SystemCallHandler104(); void SystemCallHandler105(); void SystemCallHandler106(); void SystemCallHandler107();
void SystemCallHandler108(); void SystemCallHandler109(); void SystemCallHandler110(); void SystemCallHandler111();
void SystemCallHandler112(); void SystemCallHandler113(); void SystemCallHandler114(); void SystemCallHandler115();
void SystemCallHandler116(); void SystemCallHandler117(); void SystemCallHandler118(); void SystemCallHandler119();
void SystemCallHandler120(); void SystemCallHandler121(); void SystemCallHandler122(); void SystemCallHandler123();
void SystemCallHandler124(); void SystemCallHandler125(); void SystemCallHandler126(); void SystemCallHandler127();
void SystemCallHandler128(); void SystemCallHandler129(); void SystemCallHandler130(); void SystemCallHandler131();
void SystemCallHandler132(); void SystemCallHandler133(); void SystemCallHandler134(); void SystemCallHandler135();
void SystemCallHandler136(); void SystemCallHandler137(); void SystemCallHandler138(); void SystemCallHandler139();
void SystemCallHandler140(); void SystemCallHandler141(); void SystemCallHandler142(); void SystemCallHandler143();
void SystemCallHandler144(); void SystemCallHandler145(); void SystemCallHandler146(); void SystemCallHandler147();
void SystemCallHandler148(); void SystemCallHandler149(); void SystemCallHandler150(); void SystemCallHandler151();
void SystemCallHandler152(); void SystemCallHandler153(); void SystemCallHandler154(); void SystemCallHandler155();
void SystemCallHandler156(); void SystemCallHandler157(); void SystemCallHandler158(); void SystemCallHandler159();
void SystemCallHandler160(); void SystemCallHandler161(); void SystemCallHandler162(); void SystemCallHandler163();
void SystemCallHandler164(); void SystemCallHandler165(); void SystemCallHandler166(); void SystemCallHandler167();
void SystemCallHandler168(); void SystemCallHandler169(); void SystemCallHandler170(); void SystemCallHandler171();
void SystemCallHandler172(); void SystemCallHandler173(); void SystemCallHandler174(); void SystemCallHandler175();
void SystemCallHandler176(); void SystemCallHandler177(); void SystemCallHandler178(); void SystemCallHandler179();
void SystemCallHandler180(); void SystemCallHandler181(); void SystemCallHandler182(); void SystemCallHandler183();
void SystemCallHandler184(); void SystemCallHandler185(); void SystemCallHandler186(); void SystemCallHandler187();
void SystemCallHandler188(); void SystemCallHandler189(); void SystemCallHandler190(); void SystemCallHandler191();
void SystemCallHandler192(); void SystemCallHandler193(); void SystemCallHandler194(); void SystemCallHandler195();
void SystemCallHandler196(); void SystemCallHandler197(); void SystemCallHandler198(); void SystemCallHandler199();
void SystemCallHandler200(); void SystemCallHandler201(); void SystemCallHandler202(); void SystemCallHandler203();
void SystemCallHandler204(); void SystemCallHandler205(); void SystemCallHandler206(); void SystemCallHandler207();
void SystemCallHandler208(); void SystemCallHandler209(); void SystemCallHandler210(); void SystemCallHandler211();
void SystemCallHandler212(); void SystemCallHandler213(); void SystemCallHandler214(); void SystemCallHandler215();
void SystemCallHandler216(); void SystemCallHandler217(); void SystemCallHandler218(); void SystemCallHandler219();
void SystemCallHandler220(); void SystemCallHandler221(); void SystemCallHandler222(); void SystemCallHandler223();
void SystemCallHandler224(); void SystemCallHandler225(); void SystemCallHandler226(); void SystemCallHandler227();
void SystemCallHandler228(); void SystemCallHandler229(); void SystemCallHandler230(); void SystemCallHandler231();
void SystemCallHandler232(); void SystemCallHandler233(); void SystemCallHandler234(); void SystemCallHandler235();
void SystemCallHandler236(); void SystemCallHandler237(); void SystemCallHandler238(); void SystemCallHandler239();
void SystemCallHandler240(); void SystemCallHandler241(); void SystemCallHandler242(); void SystemCallHandler243();
void SystemCallHandler244(); void SystemCallHandler245(); void SystemCallHandler246(); void SystemCallHandler247();
void SystemCallHandler248(); void SystemCallHandler249(); void SystemCallHandler250(); void SystemCallHandler251();
void SystemCallHandler252(); void SystemCallHandler253(); void SystemCallHandler254(); void SystemCallHandler255();
void SystemCallHandler256(); void SystemCallHandler257(); void SystemCallHandler258(); void SystemCallHandler259();
void SystemCallHandler260(); void SystemCallHandler261(); void SystemCallHandler262(); void SystemCallHandler263();
void SystemCallHandler264(); void SystemCallHandler265(); void SystemCallHandler266(); void SystemCallHandler267();
void SystemCallHandler268(); void SystemCallHandler269(); void SystemCallHandler270(); void SystemCallHandler271();
void SystemCallHandler272(); void SystemCallHandler273(); void SystemCallHandler274(); void SystemCallHandler275();
void SystemCallHandler276(); void SystemCallHandler277(); void SystemCallHandler278(); void SystemCallHandler279();
void SystemCallHandler280(); void SystemCallHandler281(); void SystemCallHandler282(); void SystemCallHandler283();
void SystemCallHandler284(); void SystemCallHandler285(); void SystemCallHandler286(); void SystemCallHandler287();
void SystemCallHandler288(); void SystemCallHandler289(); void SystemCallHandler290(); void SystemCallHandler291();
void SystemCallHandler292(); void SystemCallHandler293(); void SystemCallHandler294();



// XXX
// SystemCallHandler macro depends on the size of this structure and the offset of the OriginalFunction!

extern struct _ZwCalls
{
	PCHAR			ZwName;				// System call name
	USHORT			ZwNameLength;		// System call name length
	USHORT			ServiceIDNumber;	// System call index (filled in at runtime)
	PULONG_PTR		HookFunction;		// Address of the hijacking function (function that will be called instead of the original system call)
	PULONG_PTR		OriginalFunction;	// PlaceHolder for the address of the original syscall address
	BOOLEAN			Hijacked;			// Flag indicating whether we already hijacked this system call
										// or whether this is a special system service that needs to be hijacked initially
};

extern struct _ZwCalls ZwCalls[];


#define	ZW_ADD_ATOM_INDEX				 8

#define	ZW_ADJUST_TOKEN_INDEX			12

#define	ZW_CONNECT_PORT_INDEX			33

#define	ZW_CREATE_DIRECTORYOBJECT_INDEX	36
#define	ZW_CREATE_EVENT_INDEX			37
#define	ZW_CREATE_EVENT_PAIR_INDEX		38
#define	ZW_CREATE_FILE_INDEX			39

#define	ZW_CREATE_JOBOBJECT_INDEX		41

#define	ZW_CREATE_KEY_INDEX				43

#define	ZW_CREATE_MAILSLOTFILE_INDEX	45
#define	ZW_CREATE_MUTANT_INDEX			46
#define	ZW_CREATE_NAMEDPIPEFILE_INDEX	47

#define	ZW_CREATE_PORT_INDEX			49
#define	ZW_CREATE_PROCESS_INDEX			50
#define	ZW_CREATE_PROCESSEX_INDEX		51

#define	ZW_CREATE_SECTION_INDEX			53
#define	ZW_CREATE_SEMAPHORE_INDEX		54
#define	ZW_CREATE_SYMLINK_INDEX			55
#define	ZW_CREATE_THREAD_INDEX			56
#define	ZW_CREATE_TIMER_INDEX			57
#define	ZW_CREATE_TOKEN_INDEX			58
#define	ZW_CREATE_WAITPORT_INDEX		59
#define	ZW_DEBUG_ACTIVEPROCESS_INDEX	60

#define	ZW_DELETE_FILE_INDEX			66
#define	ZW_DELETE_KEY_INDEX				67

#define	ZW_FIND_ATOM_INDEX				81

#define	ZW_LOAD_DRIVER_INDEX			103

#define	ZW_MAPVIEW_SECTION_INDEX		115

#define	ZW_OPEN_DIRECTORYOBJECT_INDEX	121
#define	ZW_OPEN_EVENT_INDEX				122
#define	ZW_OPEN_EVENT_PAIR_INDEX		123
#define	ZW_OPEN_FILE_INDEX				124

#define	ZW_OPEN_JOBOBJECT_INDEX			126
#define	ZW_OPEN_KEY_INDEX				127

#define	ZW_OPEN_MUTANT_INDEX			129

#define	ZW_OPEN_PROCESS_INDEX			131

#define	ZW_OPEN_SECTION_INDEX			134
#define	ZW_OPEN_SEMAPHORE_INDEX			135
#define	ZW_OPEN_SYMLINK_INDEX			136
#define	ZW_OPEN_THREAD_INDEX			137

#define	ZW_OPEN_TIMER_INDEX				140

#define	ZW_QUERY_ATTRIBUTES_FILE_INDEX	148

#define	ZW_QUERY_DIRECTORYFILE_INDEX	154

#define	ZW_QUERY_FULLATTR_FILE_INDEX	159

#define	ZW_QUERY_VALUE_KEY_INDEX		189

#define	ZW_SECURECONNECT_PORT_INDEX		223

#define	ZW_SET_INFO_FILE_INDEX			238

#define	ZW_SET_INFO_TOKEN_INDEX			244

#define	ZW_SET_LDT_ENTRIES_INDEX		247

#define	ZW_SET_SYSTEM_INFORMATION_INDEX	254

#define	ZW_SET_SYSTEM_TIME_INDEX		256

#define	ZW_SET_TIMER_RESOLUTION_INDEX	259

#define	ZW_SET_VALUE_KEY_INDEX			261

#define	ZW_UNLOAD_DRIVER_INDEX			276

#define	ZW_VDM_CONTROL_INDEX			283


/*
 * make sure we don't try to unload the driver while a system call is in progress
 * still not atomic but we shouldn't be unloading this driver in any case
 */

#if DBG

extern int	HookedRoutineRunning;
#define	HOOK_ROUTINE_ENTER()			NTSTATUS rc; ACTION_TYPE Action; InterlockedIncrement(&HookedRoutineRunning);
#define	HOOK_ROUTINE_EXIT(status)		{ InterlockedDecrement(&HookedRoutineRunning); return ((status)); }

extern int	HookedTDIRunning;
#define	HOOK_TDI_ENTER()			NTSTATUS rc; ACTION_TYPE Action; InterlockedIncrement(&HookedTDIRunning);
#define	HOOK_TDI_ENTER_NORC()		InterlockedIncrement(&HookedTDIRunning);
#define	HOOK_TDI_EXIT(status)		{ InterlockedDecrement(&HookedTDIRunning); return ((status)); }


#else


#define	HOOK_ROUTINE_ENTER()			NTSTATUS rc; ACTION_TYPE Action; 
#define	HOOK_ROUTINE_EXIT(status)		{ return ((status)); }

#define	HOOK_TDI_ENTER()			NTSTATUS rc; ACTION_TYPE Action;
#define	HOOK_TDI_ENTER_NORC()		
#define	HOOK_TDI_EXIT(status)		{ return ((status)); }

#endif


/*
 * Various macros used by most of the hooking routines
 */

#define	POLICY_CHECK_OPTYPE_NAME(OBJECTTYPE, OPTYPE)								\
	while (KeGetPreviousMode() == UserMode) {										\
		UCHAR			OpType = (OPTYPE);											\
		PWSTR			PolicyFilename = NULL;										\
		USHORT			PolicyLinenumber = 0;										\
		UCHAR			RuleNumber = 0;												\
		LOG(LOG_SS_##OBJECTTYPE, LOG_PRIORITY_VERBOSE, ("%d %s: %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, OBJECTTYPE##NAME));						\
		Action = PolicyCheck(RULE_##OBJECTTYPE, OBJECTTYPE##NAME, OpType, &RuleNumber, &PolicyFilename, &PolicyLinenumber);\
		if (Action & ACTION_ASK)													\
		{																			\
			LOG(LOG_SS_##OBJECTTYPE, LOG_PRIORITY_DEBUG, ("%d %s: (ask) access to %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, OBJECTTYPE##NAME));		\
			/*XXX GetPathFromOA(ObjectAttributes, OBJECTTYPE##NAME, MAX_PATH, DO_NOT_RESOLVE_LINKS);*/	\
			Action = IssueUserlandAskUserRequest(RULE_##OBJECTTYPE, OpType, OBJECTTYPE##NAME);	\
		}																			\
		if ((Action & ACTION_QUIETDENY) == ACTION_QUIETDENY)						\
		{																			\
			LOG(LOG_SS_##OBJECTTYPE, LOG_PRIORITY_VERBOSE, ("%d %s: quitely denying access to %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, OBJECTTYPE##NAME));		\
			HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );								\
		}																			\
		else if (Action & ACTION_DENY)												\
		{																			\
			LOG(LOG_SS_##OBJECTTYPE, LOG_PRIORITY_VERBOSE, ("%d %s: denying access to %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, OBJECTTYPE##NAME));	\
			LogAlert(ALERT_SS_##OBJECTTYPE, OpType, RuleNumber, Action,				\
			  GetObjectAccessAlertPriority(ALERT_SS_##OBJECTTYPE, OpType, Action), PolicyFilename, PolicyLinenumber, OBJECTTYPE##NAME);\
			HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );								\
		}																			\
		else if (Action & ACTION_LOG)												\
		{																			\
			LOG(LOG_SS_##OBJECTTYPE, LOG_PRIORITY_VERBOSE, ("%d %s: (log) access to %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, OBJECTTYPE##NAME));		\
			LogAlert(ALERT_SS_##OBJECTTYPE, OpType, RuleNumber, Action,				\
			  GetObjectAccessAlertPriority(ALERT_SS_##OBJECTTYPE, OpType, Action), PolicyFilename, PolicyLinenumber, OBJECTTYPE##NAME);\
		}																			\
		break;																		\
	}


#define	POLICY_CHECK_OPTYPE(OBJECTTYPE, OPTYPE)										\
	if (KeGetPreviousMode() == UserMode && GetPathFromOA(ObjectAttributes, OBJECTTYPE##NAME, MAX_PATH, RESOLVE_LINKS) )\
	{																				\
		UCHAR			OpType = (OPTYPE);											\
		PWSTR			PolicyFilename = NULL;										\
		USHORT			PolicyLinenumber = 0;										\
		UCHAR			RuleNumber = 0;												\
		LOG(LOG_SS_##OBJECTTYPE, LOG_PRIORITY_VERBOSE, ("%d %s: %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, OBJECTTYPE##NAME));						\
		Action = PolicyCheck(RULE_##OBJECTTYPE, OBJECTTYPE##NAME, OpType, &RuleNumber, &PolicyFilename, &PolicyLinenumber);\
		if (Action & ACTION_ASK)													\
		{																			\
			LOG(LOG_SS_##OBJECTTYPE, LOG_PRIORITY_DEBUG, ("%d %s: (ask) access to %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, OBJECTTYPE##NAME));		\
			GetPathFromOA(ObjectAttributes, OBJECTTYPE##NAME, MAX_PATH, DO_NOT_RESOLVE_LINKS);	\
			Action = IssueUserlandAskUserRequest(RULE_##OBJECTTYPE, OpType, OBJECTTYPE##NAME);	\
		}																			\
		if ((Action & ACTION_QUIETDENY) == ACTION_QUIETDENY)						\
		{																			\
			LOG(LOG_SS_##OBJECTTYPE, LOG_PRIORITY_VERBOSE, ("%d %s: quitely denying access to %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, OBJECTTYPE##NAME));		\
			HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );								\
		}																			\
		else if (Action & ACTION_DENY)												\
		{																			\
			LOG(LOG_SS_##OBJECTTYPE, LOG_PRIORITY_VERBOSE, ("%d %s: denying access to %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, OBJECTTYPE##NAME));	\
			GetPathFromOA(ObjectAttributes, OBJECTTYPE##NAME, MAX_PATH, DO_NOT_RESOLVE_LINKS);	\
			LogAlert(ALERT_SS_##OBJECTTYPE, OpType, RuleNumber, Action,				\
			  GetObjectAccessAlertPriority(ALERT_SS_##OBJECTTYPE, OpType, Action), PolicyFilename, PolicyLinenumber, OBJECTTYPE##NAME);\
			HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );								\
		}																			\
		else if (Action & ACTION_LOG)												\
		{																			\
			LOG(LOG_SS_##OBJECTTYPE, LOG_PRIORITY_VERBOSE, ("%d %s: (log) access to %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, OBJECTTYPE##NAME));		\
			GetPathFromOA(ObjectAttributes, OBJECTTYPE##NAME, MAX_PATH, DO_NOT_RESOLVE_LINKS);	\
			LogAlert(ALERT_SS_##OBJECTTYPE, OpType, RuleNumber, Action,				\
			  GetObjectAccessAlertPriority(ALERT_SS_##OBJECTTYPE, OpType, Action), PolicyFilename, PolicyLinenumber, OBJECTTYPE##NAME);\
		}																			\
	}


#define	POLICY_CHECK(OBJECTTYPE)	POLICY_CHECK_OPTYPE(OBJECTTYPE, Get_##OBJECTTYPE##_OperationType(DesiredAccess))



#define	HOOK_ROUTINE_START_OPTYPE(OBJECTTYPE, OPTYPE)										\
	CHAR		OBJECTTYPE##NAME[MAX_PATH];													\
	HOOK_ROUTINE_ENTER();																	\
	if (LearningMode == FALSE)																\
	{																						\
		POLICY_CHECK_OPTYPE(OBJECTTYPE, OPTYPE);											\
	}


#define	HOOK_ROUTINE_START(OBJECTTYPE)	HOOK_ROUTINE_START_OPTYPE(OBJECTTYPE, Get_##OBJECTTYPE##_OperationType(DesiredAccess))


#define	HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(OBJECTTYPE, OBJECTNAME, OPTYPE)								\
	if (LearningMode == TRUE /*&& NT_SUCCESS(rc)*/)															\
	{																										\
		if (OBJECTNAME)																						\
		{																									\
			AddRule(RULE_##OBJECTTYPE, OBJECTTYPE##NAME, OPTYPE);											\
		}																									\
		else																								\
		{																									\
			/*LOG(LOG_SS_##OBJECTTYPE, LOG_PRIORITY_DEBUG, ("%d %s: GetPathFromOA() failed. status=%x\n", (ULONG) PsGetCurrentProcessId(), FunctionName, rc));*/	\
		}																									\
	}																										\
	HOOK_ROUTINE_EXIT(rc);


#define	HOOK_ROUTINE_FINISH_OPTYPE(OBJECTTYPE, OPTYPE)												\
		HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(OBJECTTYPE,											\
											  GetPathFromOA(ObjectAttributes, OBJECTTYPE##NAME, MAX_PATH, RESOLVE_LINKS),	\
											  OPTYPE)

#define	HOOK_ROUTINE_FINISH(OBJECTTYPE)																\
		HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(OBJECTTYPE,											\
											  GetPathFromOA(ObjectAttributes, OBJECTTYPE##NAME, MAX_PATH, RESOLVE_LINKS),	\
											  Get_##OBJECTTYPE##_OperationType(DesiredAccess))



//#define	USE_DEFAULT_HOOK_FUNCTION	NULL


extern PCHAR	NTDLL_Base;
extern int		ZwCallsNumber;


PVOID	HookSystemService(PVOID OldService, PVOID NewService);
PVOID	HookSystemServiceByIndex(ULONG ServiceIDNumber, PVOID NewService);
BOOLEAN	HookSystemServiceByName(PCHAR ServiceName, PULONG_PTR HookFunction);

BOOLEAN	InitSyscallsHooks();
BOOLEAN	InstallSyscallsHooks();
void	RemoveSyscallsHooks();

int		FindZwFunctionIndex(PCSTR Name);
PVOID	FindFunctionBase(PCHAR ImageBase, PCSTR Name);
ULONG	FindSystemServiceNumber(PCHAR ServiceName);


#endif	/* __HOOKPROC_H__ */