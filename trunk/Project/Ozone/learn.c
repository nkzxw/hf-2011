/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		learn.c
 *
 * Abstract:
 *
 *		This module implements various policy-auto-generation routines.
 *		Policy auto generation works by monitoring all system calls and remembering their
 *		argument values. These are then saved in a policy file.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 24-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include "learn.h"
#include "accessmask.h"
#include "hookproc.h"
#include "procname.h"
#include "process.h"
#include "log.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitLearningMode)
#endif


//XXX on a terminal server, global\* might need to be handled specially

WCHAR				ProcessToMonitor[MAX_PROCESS_NAME] = L"";

BOOLEAN				LearningMode = FALSE;
HANDLE				hFile;
INT64				offset;

BOOLEAN				IsGuiThread = FALSE;	/* does the process we are profiling contain any GUI threads? */

SECURITY_POLICY		NewPolicy;



/*
 * InitLearningMode()
 *
 * Description:
 *		Initialize a learning/training mode. Training mode is used to create process policies that
 *		describe all the resources used by a process.
 *
 *		Called during driver initialization (DriverEntry()) or from DriverDeviceControl().
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
InitLearningMode()
{
	LOG(LOG_SS_LEARN, LOG_PRIORITY_VERBOSE, ("InitLearningMode: Entered.\n"));


	/* initialize the NewPolicy */
	RtlZeroMemory(&NewPolicy, sizeof(NewPolicy));

	KeInitializeSpinLock(&NewPolicy.SpinLock);

	NewPolicy.ProtectionFlags = PROTECTION_ALL_ON;


	/*
	 * Load an existing policy (if there is one) and use it as a template
	 */

	if (FindAndLoadSecurityPolicy(&NewPolicy, ProcessToMonitor, NULL) == FALSE)
	{
		LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("InitLearningMode: Learning about '%S'. An existing policy not found.\n", ProcessToMonitor));

		NewPolicy.DefaultPolicyAction = ACTION_LOG;
	}
	else
	{
		LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("InitLearningMode: Learning about '%S'. Using a pre-existing policy.\n", ProcessToMonitor));
	}


	return TRUE;
}



/*************************************************************************************************
 *
 * Path Filters
 * Used to convert kernel object names into their userland representation
 * (i.e. registry \REGISTRY\USER will be converted to HKEY_USERS
 *
 *************************************************************************************************/



/*
 * FilterFilePath()
 *
 * Description:
 *		Convert kernel file paths to their userland representation (i.e. removing \??\, \DosDevices\, references).
 *
 * Parameters:
 *		path - Pointer to a source character file path.
 *
 * Returns:
 *		Pointer to a post-processed file path.
 */

PCHAR
FilterFilePath(PCHAR path)
{
	CHAR	buffer[MAX_PATH];


	if (path[0] != '\\' && path[0] != '%')
	{
//		KdPrint(("FilterFilePath: special filename %s\n", path));
//		return NULL;
	}


	if (_strnicmp(path, "\\??\\pipe", 8) == 0)
	{
		return path + 3;
	}

	if (_strnicmp(path, "\\??\\", 4) == 0)
	{
		path += 4;
	}
	else if (_strnicmp(path, "\\DosDevices\\", 12) == 0)
	{
		path += 12;
	}
	else if (_strnicmp(path, CDrive, CDriveLength) == 0 && CDriveLength)
	{
		/* replace any possible \device\harddiskvolumeX references with a DOS C:\ name */
		//XXX what about d:\ and so on?

		path[CDriveLength-2] = 'C';
		path[CDriveLength-1] = ':';

		path += CDriveLength - 2;
	}



	if (_strnicmp(path, SystemRootDirectory, SystemRootDirectoryLength) == 0 && SystemRootDirectoryLength)
	{
		/* replace \windows (systemroot) references with SystemRoot */

		strcpy(buffer, "%SystemRoot%");
		strcat(buffer, path + SystemRootDirectoryLength);

		path = buffer;
	}
	else if (_strnicmp(path, SystemRootUnresolved, SystemRootUnresolvedLength) == 0 && SystemRootUnresolvedLength)
	{
		/* replace c:\windows (systemroot) references with SystemRoot */

		strcpy(buffer, "%SystemRoot%");
		strcat(buffer, path + SystemRootUnresolvedLength);

		path = buffer;
	}
	else if (_strnicmp(path, "\\SystemRoot\\", 12) == 0)
	{
		strcpy(buffer, "%SystemRoot%");
		strcat(buffer, path + 11);

		path = buffer;
	}
	else if ((path[0] == SystemDrive || path[0] == (CHAR) tolower(SystemDrive)) && path[1] == ':')
	{
		/* replace any system drive references with "SystemDrive" */
		strcpy(buffer, "%SystemDrive%");
		strcat(buffer, path + 1);

		path = buffer;
	}


	return path;
}



/*
 * FilterMailslotPath()
 *
 * Description:
 *		Convert kernel mailslot paths to their userland representation.
 *
 * Parameters:
 *		path - Pointer to a source character path.
 *
 * Returns:
 *		Pointer to a post-processed path.
 */

PCHAR
FilterMailslotPath(PCHAR path)
{
	if (_strnicmp(path, "\\mailslot\\", 10) == 0)
		return path + 10;

	if (_strnicmp(path, "\\??\\mailslot\\", 13) == 0)
		return path + 13;

	if (_strnicmp(path, "\\device\\mailslot\\", 17) == 0)
		return path + 17;

	return path;
}



/*
 * FilterNamedpipePath()
 *
 * Description:
 *		Convert kernel namedpipe paths to their userland representation.
 *
 * Parameters:
 *		path - Pointer to a source character path.
 *
 * Returns:
 *		Pointer to a post-processed path.
 */

PCHAR
FilterNamedpipePath(PCHAR path)
{
	if (_strnicmp(path, "\\pipe\\", 6) == 0)
		return path + 6;

	if (_strnicmp(path, "\\??\\pipe\\", 9) == 0)
		return path + 9;

	if (_strnicmp(path, "\\device\\namedpipe\\", 18) == 0)
		return path + 18;

	return path;
}



/*
 * FilterRegistryPath()
 *
 * Description:
 *		Convert kernel registry paths to their userland equivalents
 *		(i.e. replacing \REGISTRY\USER\ kernel path with its userland HKEY_USERS\ representation).
 *
 * Parameters:
 *		path - Pointer to a source character registry path.
 *
 * Returns:
 *		Pointer to a post-processed registry path.
 */

PCHAR
FilterRegistryPath(PCHAR path)
{
	static char	buffer[MAX_PATH];


//XXX use reverse symlink lookup for this?!?!?

	if (_strnicmp(path, "\\REGISTRY\\", 10) == 0)
	{
		/* replace \Registry\User\ with HKEY_USERS\ */
		if (_strnicmp(path + 10, "USER\\", 5) == 0)
		{
			strcpy(path + 4, "HKEY_USERS");
			path[14] = '\\';
			return path + 4;
		}

		/* replace \Registry\Machine\ with HKEY_LOCAL_MACHINE\ */
		if (_strnicmp(path + 10, "MACHINE\\", 8) == 0)
		{
			strcpy(buffer, "HKEY_LOCAL_MACHINE\\");
			strncat(buffer, path + 18, MAX_PATH - 20);

			return buffer;
		}
	}


	return path;
}



/*
 * FilterBaseNamedObjectsPath()
 *
 * Description:
 *		Convert kernel paths to their userland representation (by removing \BaseNamedObjects\ kernel reference).
 *
 * Parameters:
 *		path - Pointer to a source character event or semaphore path.
 *
 * Returns:
 *		Pointer to a post-processed path.
 */

PCHAR
FilterBaseNamedObjectsPath(PCHAR path)
{
	if (_strnicmp(path, "\\BaseNamedObjects\\", 18) == 0)
	{
		return path + 18;
	}

	return path;
}



/*
 * FilterDll()
 *
 * Description:
 *		Convert kernel DLL path to its userland representation
 *		(i.e. removing \KnownDlls\ kernel reference).
 *
 * Parameters:
 *		path - Pointer to a source character DLL path.
 *
 * Returns:
 *		Pointer to a post-processed DLL path.
 */

PCHAR
FilterDll(PCHAR path)
{
	if (_strnicmp(path, "\\KnownDlls\\", 11) == 0)
	{
		return path + 11;
	}

	return path;
}



/*
 * FilterPath()
 *
 * Description:
 *		Filter stub.
 *
 * Parameters:
 *		path - Pointer to a source character path.
 *
 * Returns:
 *		Pointer to an unmodified path.
 */

PCHAR
FilterPath(PCHAR path)
{
	return path;
}



/*************************************************************************************************
 *
 * Operation Filters
 * Convert operations (such as OP_READ, OP_WRITE) into their ASCII representation.
 * (i.e. file OP_READ will be translated to "read" while atom OP_READ will become "find"
 *
 *************************************************************************************************/



/*
 * FilterOperation()
 *
 * Description:
 *		Convert operations (such as OP_READ, OP_WRITE) into their ASCII representation.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		Pointer to an ASCII string.
 */

PCHAR
FilterOperation(UCHAR OperationType)
{
	if (IS_BIT_SET(OperationType, OP_READ_WRITE) &&
		(IS_BIT_SET(OperationType, OP_DELETE) || IS_BIT_SET(OperationType, OP_EXECUTE)))

		return "all";


	if (IS_BIT_SET(OperationType, OP_DELETE))
		return "delete";

	if (IS_BIT_SET(OperationType, OP_READ_WRITE))
		return "rw";

	if (IS_BIT_SET(OperationType, OP_READ))
		return "read";

	if (IS_BIT_SET(OperationType, OP_WRITE))
		return "write";

	if (IS_BIT_SET(OperationType, OP_EXECUTE))
		return "execute";


	//XXX what about when multiple bits are set? read + delete?
	LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("FilterOperation: invalid operation type %d\n", OperationType));


	return "all";
}



/*
 * FilterSimpleOperation()
 *
 * Description:
 *		Convert operations (such as OP_READ, OP_WRITE) into their ASCII representation.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		Pointer to an ASCII string.
 */

PCHAR
FilterSimpleOperation(UCHAR OperationType)
{
	if (IS_BIT_SET(OperationType, OP_READ_WRITE))
		return "all";

	if (IS_BIT_SET(OperationType, OP_READ))
		return "read";

	if (IS_BIT_SET(OperationType, OP_WRITE))
		return "write";

	LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("FilterSimpleOperation: invalid operation type %d\n", OperationType));


	return "all";
}



/*
 * FilterCreateOpenOperation()
 *
 * Description:
 *		Convert OP_CREATE and OP_OPEN) into their ASCII representation.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		Pointer to an ASCII string.
 */

PCHAR
FilterCreateOpenOperation(UCHAR OperationType)
{
	if (IS_BIT_SET(OperationType, OP_CREATE) &&
		IS_BIT_SET(OperationType, OP_OPEN))

		return "all";

	if (IS_BIT_SET(OperationType, OP_CREATE))
		return "create";

	if (IS_BIT_SET(OperationType, OP_OPEN))
		return "open";


	LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("FilterCreateOpenOperation: invalid operation type %d\n", OperationType));


	return "all";
}



/*
 * FilterDirectoryOperation()
 *
 * Description:
 *		Convert operations (such as OP_DIR_CREATE) into their ASCII representation.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		Pointer to an ASCII string.
 */

PCHAR
FilterDirectoryOperation(UCHAR OperationType)
{
	if (IS_BIT_SET(OperationType, OP_DIR_CREATE))
		return "create";

	return "all";
}



/*
 * FilterProcessOperation()
 *
 * Description:
 *		Convert operations (such as OP_PROC_OPEN) into their ASCII representation.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		Pointer to an ASCII string.
 */

PCHAR
FilterProcessOperation(UCHAR OperationType)
{
	if (IS_BIT_SET(OperationType, OP_PROC_OPEN) &&
		IS_BIT_SET(OperationType, OP_PROC_EXECUTE))

		return "all";


	if (IS_BIT_SET(OperationType, OP_PROC_OPEN))
		return "open";

	if (IS_BIT_SET(OperationType, OP_PROC_EXECUTE))
		return "execute";


	return "all";
}



/*
 * FilterNetworkOperation()
 *
 * Description:
 *		Convert operations (such as OP_CONNECT) into their ASCII representation.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		Pointer to an ASCII string.
 */

PCHAR
FilterNetworkOperation(UCHAR OperationType)
{
	if (IS_BIT_SET(OperationType, OP_TCPCONNECT))
		return "tcpconnect";

	if (IS_BIT_SET(OperationType, OP_UDPCONNECT))
		return "udpconnect";

	if (IS_BIT_SET(OperationType, OP_CONNECT))
		return "connect";

	if (IS_BIT_SET(OperationType, OP_BIND))
		return "bind";


	LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("FilterNetworkOperation: invalid operation type %d\n", OperationType));


	return "all";
}



/*
 * FilterPortOperation()
 *
 * Description:
 *		Convert port operations (such as OP_PORT_CONNECT) into their ASCII representation.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		Pointer to an ASCII string.
 */

PCHAR
FilterPortOperation(UCHAR OperationType)
{
	if (IS_BIT_SET(OperationType, OP_PORT_CREATE) &&
		IS_BIT_SET(OperationType, OP_PORT_CONNECT))

		return "all";


	if (IS_BIT_SET(OperationType, OP_PORT_CREATE))
		return "create";

	if (IS_BIT_SET(OperationType, OP_PORT_CONNECT))
		return "connect";


	LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("FilterPortOperation: invalid operation type %d\n", OperationType));


	return "all";
}



/*
 * FilterAtomOperation()
 *
 * Description:
 *		Convert atom operations (such as OP_READ, OP_WRITE) into their ASCII representation.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		Pointer to an ASCII string.
 */

PCHAR
FilterAtomOperation(UCHAR OperationType)
{
	if (IS_BIT_SET(OperationType, OP_ADD) &&
		IS_BIT_SET(OperationType, OP_FIND))

		return "all";

	if (IS_BIT_SET(OperationType, OP_ADD))
		return "add";

	if (IS_BIT_SET(OperationType, OP_FIND))
		return "find";


	LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("FilterAtomOperation: invalid operation type %d\n", OperationType));


	return "all";
}



/*
 * FilterDriverOperation()
 *
 * Description:
 *		Convert driver object operations (such as OP_READ, OP_WRITE) into their ASCII representation.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		Pointer to an ASCII string.
 */

PCHAR
FilterDriverOperation(UCHAR OperationType)
{
	if (IS_BIT_SET(OperationType, OP_LOAD))
		return "load";

	if (IS_BIT_SET(OperationType, OP_REGLOAD))
		return "regload";

	LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("FilterDriverOperation: invalid operation type %d\n", OperationType));

	return "load";
}



/*
 * FilterDllOperation()
 *
 * Description:
 *		Convert DLL operations (such as OP_READ, OP_WRITE) into their ASCII representation.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		Pointer to an ASCII string.
 */

PCHAR
FilterDllOperation(UCHAR OperationType)
{
	return "load";
}



/*
 * FilterServiceOperation()
 *
 * Description:
 *		Convert service operations (such as OP_START, OP_WRITE) into their ASCII representation.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		Pointer to an ASCII string.
 */

PCHAR
FilterServiceOperation(UCHAR OperationType)
{
	if (IS_BIT_SET(OperationType, OP_SERVICE_START))
		return "start";

	if (IS_BIT_SET(OperationType, OP_SERVICE_STOP))
		return "stop";

	if (IS_BIT_SET(OperationType, OP_SERVICE_CREATE))
		return "create";

	if (IS_BIT_SET(OperationType, OP_SERVICE_DELETE))
		return "delete";


	LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("FilterServiceOperation: invalid operation type %d\n", OperationType));


	return "all";
}



/*
 * FilterTimeOperation()
 *
 * Description:
 *		Convert time operations (such as OP_TIME_CHANGE) into their ASCII representation.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		Pointer to an ASCII string.
 */

PCHAR
FilterTimeOperation(UCHAR OperationType)
{
	if (IS_BIT_SET(OperationType, OP_TIME_CHANGE))
		return "change";


	LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("FilterTimeOperation: invalid operation type %d\n", OperationType));


	return "all";
}



/*
 * FilterTokenOperation()
 *
 * Description:
 *		Convert token operations (such as OP_TOKEN_MODIFY) into their ASCII representation.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		Pointer to an ASCII string.
 */

PCHAR
FilterTokenOperation(UCHAR OperationType)
{
	if (IS_BIT_SET(OperationType, OP_TOKEN_MODIFY))
		return "modify";


	LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("FilterTokenOperation: invalid operation type %d\n", OperationType));


	return "all";
}



/*
 * FilterSyscallOperation()
 *
 * Description:
 *		This function is never supposed to be called.
 *
 * Parameters:
 *		OperationType - operation type.
 *
 * Returns:
 *		An error.
 */

PCHAR
FilterSyscallOperation(UCHAR OperationType)
{
	LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("FilterSyscallOperation: this function is not supposed to be called.\n"));

	return "error";
}



typedef PCHAR (*fpFilterObject) (PCHAR path);
typedef PCHAR (*fpFilterOperation) (UCHAR OperationType);


/* in C++ these would be member methods */

struct
{
	PCHAR					Prefix;
	fpFilterObject			FilterObjectProc;
	fpFilterOperation		FilterOperationProc;

} RuleTypeData[] =
{
	{ "file", FilterFilePath, FilterOperation },
	{ "directory", FilterFilePath, FilterDirectoryOperation },
	{ "mailslot", FilterMailslotPath, FilterSimpleOperation },
	{ "namedpipe", FilterNamedpipePath, FilterSimpleOperation },
	{ "registry", FilterRegistryPath, FilterOperation },
	{ "section", FilterBaseNamedObjectsPath, FilterOperation },
	{ "dll", FilterDll, FilterDllOperation },
	{ "event", FilterBaseNamedObjectsPath, FilterCreateOpenOperation },
	{ "semaphore", FilterBaseNamedObjectsPath, FilterCreateOpenOperation },
	{ "job", FilterBaseNamedObjectsPath, FilterCreateOpenOperation },
	{ "mutex", FilterBaseNamedObjectsPath, FilterCreateOpenOperation },
	{ "port", FilterPath, FilterPortOperation },
	{ "symlink", FilterPath, FilterCreateOpenOperation },
	{ "timer", FilterBaseNamedObjectsPath, FilterCreateOpenOperation },
	{ "process", FilterFilePath, FilterProcessOperation },
	{ "driver", FilterFilePath, FilterDriverOperation },
	{ "dirobj", FilterPath, FilterCreateOpenOperation },
	{ "atom", FilterPath, FilterAtomOperation },

	{ "network", FilterPath, FilterNetworkOperation },
	{ "service", FilterPath, FilterServiceOperation },
	{ "time", FilterPath, FilterTimeOperation },
	{ "token", FilterPath, FilterTokenOperation },
	{ "syscall", FilterPath, FilterSyscallOperation },
};



/*
 * DecodeAction()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		.
 */

PCHAR
DecodeAction(UCHAR ActionType)
{
	switch (ActionType)
	{
		case ACTION_ASK:
		case ACTION_ASK_DEFAULT:
				return "ask";

		case ACTION_PERMIT:
		case ACTION_ASK_PERMIT:
		case ACTION_PERMIT_DEFAULT:
				return "permit";

		case ACTION_DENY:
		case ACTION_ASK_DENY:
		case ACTION_DENY_DEFAULT:
				return "deny";

		case ACTION_LOG:
		case ACTION_ASK_LOG:
		case ACTION_LOG_DEFAULT:
				return "log";

		case ACTION_QUIETDENY:
		case ACTION_QUIETDENY_DEFAULT:
				return "quietdeny";

		case ACTION_TERMINATE:
		case ACTION_ASK_TERMINATE:
				return "log";

		default:
				return "unknown";
	}
}



/*
 * CreateRule()
 *
 * Description:
 *		Generate an ASCII rule for rule 'r' of type RuleType.
 *
 * Parameters:
 *		RuleType - rule type.
 *		r - rule itself.
 *		buffer - output buffer.
 *
 * Returns:
 *		TRUE if a rule was created, FALSE otherwise.
 */

BOOLEAN
CreateRule(RULE_TYPE RuleType, PPOLICY_RULE r, PCHAR buffer)
{
	PCHAR	name;
	

	ASSERT(r);
	ASSERT(buffer);


	if (r->MatchType != MATCH_ALL)
	{
		name = RuleTypeData[ RuleType ].FilterObjectProc(r->Name);

		/* if name is NULL there is no need to remember this rule */
		if (name == NULL)
			return FALSE;

		if (strlen(name) > MAX_PATH)
		{
			LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("CreateRule: name '%s' is too long\n", name));
			return FALSE;
		}

	}


	/*
	 * construct the "objecttype_operation: " part first
	 */
	
	strcpy(buffer, RuleTypeData[ RuleType ].Prefix);

	strcat(buffer, "_");

	strcat(buffer, RuleTypeData[ RuleType ].FilterOperationProc(r->OperationType));

	strcat(buffer, ": ");


	/*
	 * now construct the action part
	 */

	if (r->MatchType != MATCH_ALL)
	{
		strcat(buffer, "name ");

		if (r->MatchType == MATCH_WILDCARD)
			strcat(buffer, "match \"");
		else
			strcat(buffer, "eq \"");

		strcat(buffer, name);

		strcat(buffer, "\" then ");
	}

	strcat(buffer, DecodeAction(r->ActionType));


	/* add optional rule clause */
	if (r->RuleNumber != 0)
	{
		CHAR	rule[16];

		sprintf(rule, " [rule %d]", r->RuleNumber);

		strcat(buffer, rule);
	}


	return TRUE;
}



/*
 * CreateServiceRule()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		.
 */

BOOLEAN
CreateServiceRule(PPOLICY_RULE r, PCHAR buffer)
{
	strcpy(buffer, "service_");
	strcat(buffer, r->Name + 2);
	strcat(buffer, ": permit");

	return TRUE;
}



BOOLEAN WriteRule(PCHAR rule);
BOOLEAN WritePolicyFile(PCHAR buffer);


/*
 * FlushPolicy()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		Nothing.
 */

void
FlushPolicy()
{
	PPOLICY_RULE	r;
	KIRQL			irql;
	char			buffer[MAX_PATH*2];
	UCHAR			i;
	BOOLEAN			ValidRule;


	// cannot write to files while holding spinlock (irql != PASSIVE_LEVEL)
//	KeAcquireSpinLock(&NewPolicy.SpinLock, &irql);

	sprintf(buffer, "\r\n# %S Default Process Policy\r\n", ProcessToMonitor);
	WriteRule(buffer);

	sprintf(buffer, "policy_default: %s\r\n", DecodeAction(NewPolicy.DefaultPolicyAction));
	WriteRule(buffer);

	if (! IS_OVERFLOW_PROTECTION_ON(NewPolicy))
		WriteRule("protection_overflow: off");

	if (! IS_USERLAND_PROTECTION_ON(NewPolicy))
		WriteRule("protection_userland: off");

	if (! IS_EXTENSION_PROTECTION_ON(NewPolicy))
		WriteRule("protection_extension: off");

	if (! IS_DEBUGGING_PROTECTION_ON(NewPolicy))
		WriteRule("protection_debugging: off");

	if (! IS_VDM_PROTECTION_ON(NewPolicy))
		WriteRule("protection_dos16: off");


//	KdPrint(("%s process\n", IsGuiThread ? "GUI" : "NON GUI"));


	for (i = 0; i < RULE_LASTONE; i++)
	{
		r = NewPolicy.RuleList[i];

		if (r)
		{
			WritePolicyFile("\r\n\r\n# ");
			WritePolicyFile(RuleTypeData[i].Prefix);
			WritePolicyFile(" related operations\r\n\r\n");
		}

		while (r)
		{
			if (i != RULE_SYSCALL)
				ValidRule = CreateRule(i, r, buffer);
			else
				ValidRule = CreateServiceRule(r, buffer);

			if (ValidRule == TRUE)
				WriteRule(buffer);

			r = (PPOLICY_RULE) r->Next;
		}
	}


//	KeReleaseSpinLock(&NewPolicy.SpinLock, irql);
}



/*
 * ShutdownLearningMode()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		Nothing.
 */

BOOLEAN
ShutdownLearningMode()
{
	UNICODE_STRING			pathname;
	OBJECT_ATTRIBUTES		oa;
	IO_STATUS_BLOCK			isb;
	WCHAR					PolicyPath[MAX_PATH];


	/* now open a file where the new policy will be written, possibly clobbering the old policy */
	//XXX should really copy an existing policy to a .bak file

//	_snwprintf(PolicyPath, MAX_PATH, L"\\??\\c:\\policy\\%s.policy", ProcessToMonitor);
	_snwprintf(PolicyPath, MAX_PATH, L"\\??\\%s\\policy\\%s.policy", OzoneInstallPath, ProcessToMonitor);
	PolicyPath[MAX_PATH - 1] = 0;


	LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("ShutdownLearningMode: Writing policy to %S\n", PolicyPath));


	RtlInitUnicodeString(&pathname, PolicyPath);

	InitializeObjectAttributes(&oa, &pathname, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	if (!NT_SUCCESS(ZwCreateFile(&hFile, GENERIC_WRITE, &oa, &isb,
									NULL, 0, 0, FILE_SUPERSEDE,
									FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0)))
	{
		LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("ShutdownLearningMode: Failed to open file %S\n", pathname.Buffer));
		return FALSE;
	}

	offset = 0;

	FlushPolicy();

	PolicyDelete(&NewPolicy);

	ZwClose(hFile);
	hFile = 0;


	return TRUE;
}



/*
 * WritePolicyFile()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		Nothing.
 */

BOOLEAN
WritePolicyFile(PCHAR buffer)
{
	int					len = strlen(buffer);
	IO_STATUS_BLOCK		isb;


	if (!NT_SUCCESS(ZwWriteFile(hFile, NULL, NULL, NULL, &isb, (PVOID) buffer, len, (PLARGE_INTEGER) &offset, NULL)))
	{
		LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("WritePolicyFile(): ZwReadFile failed\n"));
		return FALSE;
	}

	if (isb.Information != len)
	{
		LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("WritePolicyFile(): Asked to write %d bytes. Wrote only %d\n", len, isb.Information));
	}

	offset += isb.Information;


	return TRUE;
}



/*
 * WriteRule()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		.
 *
 * Returns:
 *		Nothing.
 */

BOOLEAN
WriteRule(PCHAR rule)
{
	BOOLEAN		ret, ret2;

	ret = WritePolicyFile(rule);
	ret2 = WritePolicyFile("\r\n");

	return ret && ret2;
}



/*
 * RememberRule()
 *
 * Description:
 *		Create a new rule.
 *
 * Parameters:
 *		RuleType - rule type.
 *		ObjectName - name of an object associated with the current rule.
 *		OperationType - operation type.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
RememberRule(RULE_TYPE RuleType, PCHAR ObjectName, UCHAR OperationType)
{
	PPOLICY_RULE	rule = NewPolicy.RuleList[RuleType];
	KIRQL			irql;
	int				len = 0;


	if (ObjectName)
		len = strlen(ObjectName);


	KeAcquireSpinLock(&NewPolicy.SpinLock, &irql);


	/*
	 * don't save duplicate rules
	 */

	while (rule != NULL)
	{
		if ( (rule->MatchType == MATCH_ALL) ||
			 (rule->MatchType == MATCH_SINGLE && len == rule->NameLength && _stricmp(ObjectName, rule->Name) == 0) ||
			 (rule->MatchType == MATCH_WILDCARD && WildcardMatch(ObjectName, rule->Name) == WILDCARD_MATCH) )
		{
			rule->OperationType |= OperationType;

			KeReleaseSpinLock(&NewPolicy.SpinLock, irql);

			return TRUE;
		}

		rule = (PPOLICY_RULE) rule->Next;
	}


	rule = ExAllocatePoolWithTag(NonPagedPool, sizeof(POLICY_RULE) + len, _POOL_TAG);
	if (rule == NULL)
	{
		LOG(LOG_SS_LEARN, LOG_PRIORITY_DEBUG, ("RememberRule: out of memory\n"));
		return FALSE;
	}

	RtlZeroMemory(rule, sizeof(POLICY_RULE));

	rule->ActionType = ACTION_PERMIT;
	rule->OperationType = OperationType;

	if (ObjectName)
	{
		rule->NameLength = (USHORT) len;
		strcpy(rule->Name, ObjectName);
		rule->MatchType = MATCH_SINGLE;
	}
	else
	{
		rule->MatchType = MATCH_ALL;
	}

	rule->Next = NewPolicy.RuleList[RuleType];
	NewPolicy.RuleList[RuleType] = rule;


	KeReleaseSpinLock(&NewPolicy.SpinLock, irql);


	return TRUE;
}



/*
 * DetermineThreadType()
 *
 * Description:
 *		Determine whether a thread is GUI enabled or not. Done by checking which
 *		system call table the thread is using.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Nothing.
 */

void
DetermineThreadType()
{
	if (ThreadServiceTableOffset == 0)
		return;

	if (* (PULONG) ((PCHAR) PsGetCurrentThread() + ThreadServiceTableOffset) != (ULONG) &KeServiceDescriptorTable[0])
		IsGuiThread = TRUE;
}



/*
 * AddRule()
 *
 * Description:
 *		Create a new rule for a particular rule type, operation type and object.
 *
 * Parameters:
 *		RuleType - rule type.
 *		str - optional character object name.
 *		OperationType - operation taking place.
 *
 * Returns:
 *		Nothing.
 */

BOOLEAN
AddRule(RULE_TYPE RuleType, PCHAR str, UCHAR OperationType)
{
	PWSTR		filename;


	filename = wcsrchr(GetCurrentProcessName(), L'\\');
	if (filename == NULL)
		filename = GetCurrentProcessName();
	else
		++filename;

	if (_wcsicmp(filename, ProcessToMonitor) != 0)
		return TRUE;

//	DetermineThreadType();

	return RememberRule(RuleType, str, OperationType);
}
