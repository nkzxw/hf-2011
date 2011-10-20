/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		policy.c
 *
 * Abstract:
 *
 *		This module implements various security policy parsing and enforcement routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 16-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */

// XXX rename all funcs as SpYYY ? (same for other modules?)

#include <NTDDK.h>
#include "policy.h"
#include "pathproc.h"
#include "procname.h"
#include "hookproc.h"
#include "media.h"
#include "learn.h"
#include "misc.h"
#include "i386.h"


#include "process.h"
#include "log.h"


BOOLEAN PolicyParseRule(OUT PSECURITY_POLICY pSecPolicy, IN PCHAR rule, OUT BOOLEAN *Critical);
BOOLEAN PolicyParsePolicyRule(OUT PSECURITY_POLICY pSecPolicy, IN PCHAR Operation, IN PCHAR rule, OUT BOOLEAN *Critical);
BOOLEAN PolicyParseObjectRule(PSECURITY_POLICY pSecPolicy, RULE_TYPE RuleType, PCHAR Operation, PCHAR rule);
BOOLEAN PolicyParseSyscallRule(PSECURITY_POLICY pSecPolicy, PCHAR SyscallName, PCHAR rule);
BOOLEAN	PolicyParseProtectionRule(PSECURITY_POLICY pSecPolicy, PCHAR Operation, PCHAR rule);
BOOLEAN	PolicyParseMediaRule(PSECURITY_POLICY pSecPolicy, PCHAR Operation, PCHAR rule);


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitPolicy)
#pragma alloc_text (PAGE, PolicyPostBootup)
#pragma alloc_text (PAGE, PolicyRemove)
#endif


/*
 * example:
 *
 * SystemRoot - \device\harddisk1\windows
 * SystemRootUnresolved - c:\windows
 * SystemRootDirectory - \windows
 * CDrive - \device\harddisk1
 */

CHAR			SystemDrive, SystemRoot[MAX_PATH], SystemRootUnresolved[MAX_PATH], *SystemRootDirectory, CDrive[MAX_PATH];
USHORT			SystemRootLength = 0, SystemRootUnresolvedLength = 0, SystemRootDirectoryLength = 0, CDriveLength = 0;

USHORT			gPolicyLineNumber;
PWSTR			gPolicyFilename, gFilePath;

// to be portable on 32 & 64 bit platforms
ULONG			NumberOfBitsInUlong, UlongBitShift;

/* LoadPolicy() can be used by only one thread at a time due to use of global variables */
KMUTEX			LoadPolicyMutex;

/* Global Security Policy */
SECURITY_POLICY	gSecPolicy;



/*
 * InitPolicy()
 *
 * Description:
 *		Initialize the policy engine. Load the global policy.
 *
 *		NOTE: Called once during driver initialization (DriverEntry()).
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		TRUE if everything is OK, FALSE if failed.
 */

BOOLEAN
InitPolicy()
{
	NumberOfBitsInUlong = sizeof(ULONG) * 8;
	UlongBitShift = sizeof(ULONG) == 4 ? 5 : 6;


	/* gets reinitialized correctly once bootup is complete (see PolicyPostBootup) */
	SystemDrive = 'C';

	if (ReadSymlinkValue(L"\\SystemRoot", SystemRootUnresolved, MAX_PATH) == FALSE)
	{
		LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("InitPolicy: ReadSymlinkValue failed\n"));
		return FALSE;
	}

	SystemRootUnresolvedLength = (USHORT) strlen(SystemRootUnresolved);

	ResolveFilename(SystemRootUnresolved, SystemRoot, MAX_PATH);


	/* extract the system directory name by itself (i.e. \windows) */
	SystemRootDirectory = strrchr(SystemRoot, '\\');

	if (SystemRootDirectory == NULL)
	{
		LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("InitPolicy: SystemRootDirectory is NULL\n"));
		return FALSE;
	}

	SystemRootDirectoryLength = (USHORT) strlen(SystemRootDirectory);


	if (ReadSymlinkValue(L"\\??\\C:", CDrive, MAX_PATH) == FALSE)
	{
		LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("InitPolicy: Failed to open C: symbolic link\n"));
		return FALSE;
	}

	CDriveLength = (USHORT) strlen(CDrive);


	if (PolicyPostBootup() == FALSE)
	{
		/*
		 * if boot process is not complete yet then we cannot get SystemRootUnresolved (i.e. c:\windows)
		 * because parts of registry are not initialized yet (see PolicyPostBootup)
		 *
		 * In that case, try to assemble SystemRootUnresolved manually
		 */

		SystemRootUnresolved[0] = SystemDrive;
		SystemRootUnresolved[1] = ':';

		strcpy(SystemRootUnresolved + 2, SystemRootDirectory);
	}


	LOG(LOG_SS_POLICY, LOG_PRIORITY_VERBOSE, ("InitPolicy: SystemRoot=%s (%s, %s)\n", SystemRoot, SystemRootUnresolved, SystemRootDirectory));


	KeInitializeMutex(&LoadPolicyMutex, 0);

	RtlZeroMemory(&gSecPolicy, sizeof(gSecPolicy));

	KeInitializeSpinLock(&gSecPolicy.SpinLock);


	if (LearningMode == TRUE)

		return TRUE;


	if (FindAndLoadSecurityPolicy(&gSecPolicy, L"computer", NULL) == FALSE)
	{
		LOG(LOG_SS_POLICY, LOG_PRIORITY_WARNING, ("InitPolicy: LoadSecurityPolicy(computer.policy) failed\n"));
		gSecPolicy.DefaultPolicyAction = ACTION_PERMIT_DEFAULT;
	}


	if (gSecPolicy.DefaultPolicyAction != ACTION_PERMIT_DEFAULT)
	{
		LOG(LOG_SS_POLICY, LOG_PRIORITY_WARNING, ("InitPolicy: Global policy default action must be permit\n"));
		gSecPolicy.DefaultPolicyAction = ACTION_PERMIT_DEFAULT;
	}


	return TRUE;
}



/*
 * PolicyPostBootup()
 *
 * Description:
 *		Finish initializing system variables once the bootup process is complete.
 *		We are unable to read the SystemRoot registry value before the bootup is complete since
 *		that part of the registry has not been initialized yet.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
PolicyPostBootup()
{
	ASSERT(BootingUp == FALSE);


	if (ReadStringRegistryValueA(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion", L"SystemRoot", SystemRootUnresolved, MAX_PATH) == FALSE)
	{
		LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("PolicyPostBootup: Failed to open SystemRoot registry key\n"));
		return FALSE;
	}

	SystemRootUnresolvedLength = (USHORT) strlen(SystemRootUnresolved);

	SystemDrive = (CHAR) toupper(SystemRootUnresolved[0]);


	return TRUE;
}



/*
 * PolicyRemove()
 *
 * Description:
 *		Shutdown the policy engine. Delete the global policy
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Nothing.
 */

void
PolicyRemove()
{
	PolicyDelete(&gSecPolicy);
}



/*
 * PolicyDelete()
 *
 * Description:
 *		Delete a security policy. Free all the rules associated with a policy.
 *
 * Parameters:
 *		pSecPolicy - pointer to a security policy to delete.
 *
 * Returns:
 *		Nothing.
 */

void
PolicyDelete(IN PSECURITY_POLICY pSecPolicy)
{
	PPOLICY_RULE		r, tmp;
	KIRQL				irql;
	UCHAR				i;


	if (pSecPolicy == NULL)
	{
		LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("PolicyDelete: pSecPolicy is NULL\n"));
		return;
	}


	if (pSecPolicy->Initialized == FALSE)
	{
		LOG(LOG_SS_POLICY, LOG_PRIORITY_VERBOSE, ("PolicyDelete: pSecPolicy is not initialized\n"));
		return;
	}


	KeAcquireSpinLock(&pSecPolicy->SpinLock, &irql);

	for (i = 0; i < RULE_LASTONE; i++)
	{
		r = pSecPolicy->RuleList[i];

		while (r)
		{
			tmp = r;
			r = (PPOLICY_RULE) r->Next;

			ExFreePoolWithTag(tmp, _POOL_TAG);
		}
	}

	if (pSecPolicy->Name)
	{
		ExFreePoolWithTag(pSecPolicy->Name, _POOL_TAG);
		pSecPolicy->Name = NULL;
	}

	pSecPolicy->Initialized = FALSE;

	RtlZeroMemory(pSecPolicy->RuleList, sizeof(pSecPolicy->RuleList));


	KeReleaseSpinLock(&pSecPolicy->SpinLock, irql);
}



/*
 * LoadSecurityPolicy()
 *
 * Description:
 *		Parses and loads a security policy.
 *
 * Parameters:
 *		pSecPolicy - pointer to a security policy to initialize.
 *		PolicyFile - string containing the policy filename to parse
 *		FilePath - string containing full program path of the file we are loading policy for
 *
 * Returns:
 *		TRUE if security policy was successfully parsed and loaded, FALSE otherwise.
 */

BOOLEAN
LoadSecurityPolicy(OUT PSECURITY_POLICY pSecPolicy, IN PWSTR PolicyFile, IN PWSTR FilePath)
{
	OBJECT_ATTRIBUTES		oa;
	HANDLE					hFile = 0;
	UNICODE_STRING			usPolicyFile;
	ULONG					size;
	NTSTATUS				status;
	IO_STATUS_BLOCK			isb;
	CHAR					*p, buffer[POLICY_MAX_RULE_LENGTH];
	INT64					offset;
	BOOLEAN					ret = TRUE, Critical = FALSE;


	if (pSecPolicy == NULL || PolicyFile == NULL)
	{
		LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("LoadSecurityPolicy(%x, %x, %x): NULL parameter\n", pSecPolicy, PolicyFile, FilePath));
		return FALSE;
	}


	pSecPolicy->Initialized = FALSE;
	pSecPolicy->DefaultPolicyAction = ACTION_NONE;
	pSecPolicy->ProtectionFlags = BootingUp ? PROTECTION_ALL_OFF : PROTECTION_ALL_ON;


	RtlInitUnicodeString(&usPolicyFile, PolicyFile);

	LOG(LOG_SS_POLICY, LOG_PRIORITY_VERBOSE, ("LoadSecurityPolicy: Parsing %S\n", usPolicyFile.Buffer));


	InitializeObjectAttributes(&oa, &usPolicyFile, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	if (!NT_SUCCESS(ZwCreateFile(&hFile, GENERIC_READ, &oa, &isb,
									NULL, 0, FILE_SHARE_READ, FILE_OPEN,
									FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0)))
	{
		LOG(LOG_SS_POLICY, LOG_PRIORITY_VERBOSE, ("LoadSecurityPolicy: Failed to open file %S\n", usPolicyFile.Buffer));
		return FALSE;
	}


	offset = 0;
	buffer[0] = 0;


	/* only one thread at a time can use LoadPolicyMutex due to use of global variables (PolicyLineNumber, buffer) */
	KeWaitForMutexObject(&LoadPolicyMutex, Executive, KernelMode, FALSE, NULL);


	gPolicyLineNumber = 1;
	gPolicyFilename = usPolicyFile.Buffer;
	gFilePath = FilePath;

	while (1)
	{
		status = ZwReadFile(hFile, NULL, NULL, NULL, &isb, (PVOID) buffer, sizeof(buffer) - 1,
							(PLARGE_INTEGER) &offset, NULL);

		if (! NT_SUCCESS(status))
		{
			if (status != STATUS_END_OF_FILE)
			{
				LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("LoadSecurityPolicy: ZwReadFile failed rc=%x\n", status));
				ret = FALSE;
				PolicyDelete(pSecPolicy);
			}

			break;
		}

		if (isb.Information == 0)
			break;

		buffer[isb.Information] = '\0';

		/*
		 * strchr() will return NULL when the line we read exceeds the size of the buffer or
		 * the last line was not '\n' terminated
		 */

		if ((p = strchr(buffer, '\n')) == NULL)
		{
			/* don't try to parse very long lines */

			if (isb.Information == sizeof(buffer) - 1)
			{
				LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_WARNING, ("LoadSecurityPolicy(%s:%d): Rule is too long\n", gPolicyFilename, gPolicyLineNumber));

				PolicyDelete(pSecPolicy);

				ret = FALSE;
				break;
			}


			/* the last rule was not '\n' terminated */

			if (PolicyParseRule(pSecPolicy, buffer, &Critical) == FALSE)
			{
				if (Critical == TRUE)
				{
					LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_DEBUG, ("LoadSecurityPolicy(%S:%d): Encountered a critical error. Aborting.\n", gPolicyFilename, gPolicyLineNumber));

					PolicyDelete(pSecPolicy);

					ret = FALSE;
					break;
				}
			}
			
			ret = TRUE;

			break;
		}

		*p = 0;

		if (p != buffer && *(p - 1) == '\r')
			*(p - 1) = 0;


		if (PolicyParseRule(pSecPolicy, buffer, &Critical) == FALSE)
		{
			if (Critical == TRUE)
			{
				LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_DEBUG, ("LoadSecurityPolicy(%S:%d): Encountered a critical error. Aborting.\n", gPolicyFilename, gPolicyLineNumber));

				PolicyDelete(pSecPolicy);

				ret = FALSE;
				break;
			}
		}


		offset += p - buffer + 1;


		if (++gPolicyLineNumber > 10000)
		{
			LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_WARNING, ("LoadSecurityPolicy: Policy '%S' is too long. Maximum number of lines is 10000.\n", gPolicyFilename));

			PolicyDelete(pSecPolicy);

			ret = FALSE;
			break;
		}
	}

	ZwClose(hFile);


	if (ret != FALSE)
	{
		pSecPolicy->Initialized = TRUE;

		if (pSecPolicy->DefaultPolicyAction == ACTION_NONE)
			pSecPolicy->DefaultPolicyAction = DEFAULT_POLICY_ACTION;
	}


	LOG(LOG_SS_POLICY, LOG_PRIORITY_VERBOSE, ("LoadSecurityPolicy: Done Parsing %S. Total number of lines %d. (ret=%d)\n", usPolicyFile.Buffer, gPolicyLineNumber, ret));


	KeReleaseMutex(&LoadPolicyMutex, FALSE);


	return ret;
}



/*
 * FindAndLoadSecurityPolicy()
 *
 * Description:
 *		Finds and loads a security policy associated with a specified executable filename.
 *
 * Parameters:
 *		pSecPolicy - pointer to a security policy to initialize.
 *		FilePath - string specifying the complete path to an executable
 *		UserName - optional username, if specified check for a policy in "username" directory first
 *
 * Returns:
 *		TRUE if security policy was successfully parsed and loaded, FALSE otherwise.
 */

BOOLEAN
FindAndLoadSecurityPolicy(OUT PSECURITY_POLICY pSecPolicy, IN PWSTR FilePath, IN PWSTR UserName)
{
	PWSTR		filename;
	WCHAR		PolicyPath[MAX_PATH];
	BOOLEAN		ret;
	int			len;


	if (pSecPolicy == NULL || FilePath == NULL)
	{
		LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("FindAndLoadSecurityPolicy: NULL argument %x %x\n", pSecPolicy, FilePath));
		return FALSE;
	}


	if (KeGetCurrentIrql() != 0)
	{
		LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("FindAndLoadSecurityPolicy(): irql=%d\n", KeGetCurrentIrql()));
		return FALSE;
	}


	filename = wcsrchr(FilePath, L'\\');

	if (filename == NULL)
		filename = FilePath;
	else
		++filename;


	/* if user policy load fails, we loop here again to load the global policy */
ReloadPolicy:

	if (UserName != NULL)
		_snwprintf(PolicyPath, MAX_PATH, L"\\??\\%s\\policy\\%s\\%s.policy", OzoneInstallPath, UserName, filename);
	else
		_snwprintf(PolicyPath, MAX_PATH, L"\\??\\%s\\policy\\%s.policy", OzoneInstallPath, filename);

	PolicyPath[MAX_PATH - 1] = 0;


	LOG(LOG_SS_POLICY, LOG_PRIORITY_VERBOSE, ("FindAndLoadSecurityPolicy: Loading policy for %S (%S)\n", PolicyPath, FilePath));


	ret = LoadSecurityPolicy(pSecPolicy, PolicyPath, FilePath);
	if (ret == FALSE)
	{
		/* If we can't find a policy specific to a user, try to load a global policy instead */
		if (UserName != NULL)
		{
			LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("FindAndLoadSecurityPolicy: Cannot find '%S' policy for user '%S'. Looking for a global policy..\n", filename, UserName));

			UserName = NULL;

			goto ReloadPolicy;
		}

		return FALSE;
	}


	/* allocate extra space for ".policy" string */
	len = wcslen(filename) + 7 + 1;

	pSecPolicy->Name = ExAllocatePoolWithTag(NonPagedPool, len * sizeof(WCHAR), _POOL_TAG);

	if (pSecPolicy->Name != NULL)
	{
		_snwprintf(pSecPolicy->Name, len, L"%s.policy", filename);
	}
	else
	{
		PolicyDelete(pSecPolicy);
		ret = FALSE;
	}

	return ret;
}



/*
 * PolicyParseRule()
 *
 * Description:
 *		Parses a specified rule.
 *
 * Parameters:
 *		pSecPolicy - pointer to a security policy that will contain the parsed rule.
 *		rule - string buffer containing a rule to parse.
 *		Critical - Boolean indicating whether the parser should abort parsing the policy due to a critical error.
 *
 * Returns:
 *		TRUE if the policy rule was successfully parsed and loaded, FALSE otherwise.
 */

#define	SKIP_WHITESPACE(str)	do { while(*(str) == ' ' || *(str) == '\t') ++(str); } while(0)
#define	IS_WHITESPACE(c)		((c) == ' ' || (c) == '\t')

/* macro shortcut for bailing out of PolicyParseRule() in case of an error */

#define	ABORT_PolicyParseRule(msg)									\
	do {															\
		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_WARNING, ("Encountered an error while parsing %S:%d :\n", gPolicyFilename, gPolicyLineNumber));		\
		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_WARNING, msg);		\
		return FALSE;												\
	} while (0)


static BOOLEAN
PolicyParseRule(OUT PSECURITY_POLICY pSecPolicy, IN PCHAR rule, OUT BOOLEAN *Critical)
{
	CHAR	ServiceName[POLICY_MAX_SERVICE_NAME_LENGTH];
	PCHAR	OriginalRule = rule;
	int		i = 0, SawSpace = 0, SawUnderscore = 0;


	*Critical = FALSE;


	SKIP_WHITESPACE(rule);


	/* skip empty lines */

	if (*rule == 0)
		return TRUE;


	/* comments start with '#' */

	if (*rule == '#')
		return TRUE;


	/*
	 * Parse the service name. Format:	"ServiceName:" or "ServiceName_OperationType:"
	 * where ServiceName can be "file", "registry", "event", "memory", etc
	 * and OperationType can be "read", "write", "rw"
	 */

	while (*rule != 0 && *rule != ':')
	{
		/* is the specified syscall name too long? */

		if (i > POLICY_MAX_SERVICE_NAME_LENGTH - 1)
			ABORT_PolicyParseRule(("Rule type specification is too long. Maximum rule type specification length is %d characters.\n", POLICY_MAX_SERVICE_NAME_LENGTH));


		/* allow whitespace before the colon */

		if (IS_WHITESPACE(*rule))
		{
			++rule;

			SawSpace = 1;

			continue;
		}
		

		/* Service Names are not allowed to contain a space */

		if (SawSpace)
			ABORT_PolicyParseRule(("Rule type specification cannot contain a space\n"));


		/* Expecting to see 1 underscore '_' */

		if (*rule == '_')
		{
			/* There can be only be 1 underscore char. and it cannot be the first char. */
			if (i == 0 || SawUnderscore)
				ABORT_PolicyParseRule(("Rule type specification cannot contain multiple underscore characters\n"));

			/* remember the underscore position */
			SawUnderscore = i;
		}


		ServiceName[i++] = *rule++;
	}


	/* Expecting to have read more than 1 character, finishing with a ':' */
	if (i == 0 || *rule++ != ':')
		ABORT_PolicyParseRule(("Colon not found. Rule type specification must end with a colon ':'.\n"));


	ServiceName[i] = 0;


	SKIP_WHITESPACE(rule);


	/* didn't see any underscores. assume "system_call_name:" format */

	if (SawUnderscore == 0)
		ABORT_PolicyParseRule(("Underscore not found. Rule type specification must contain an underscore.\n"));


	ServiceName[SawUnderscore] = 0;


	// file operation
	if (strlen(ServiceName) == 4 && _stricmp(ServiceName, "file") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_FILE, ServiceName + SawUnderscore + 1, rule);

	// directory operation
	if (strlen(ServiceName) == 9 && _stricmp(ServiceName, "directory") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_DIRECTORY, ServiceName + SawUnderscore + 1, rule);

	// registry operation
	if (strlen(ServiceName) == 8 && _stricmp(ServiceName, "registry") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_REGISTRY, ServiceName + SawUnderscore + 1, rule);

	// memory section operation
	if (strlen(ServiceName) == 7 && _stricmp(ServiceName, "section") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_SECTION, ServiceName + SawUnderscore + 1, rule);

	// memory section / dll operation
	if (strlen(ServiceName) == 3 && _stricmp(ServiceName, "dll") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_DLL, ServiceName + SawUnderscore + 1, rule);

	// event operation
	if (strlen(ServiceName) == 5 && _stricmp(ServiceName, "event") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_EVENT, ServiceName + SawUnderscore + 1, rule);

	// semaphore operation
	if (strlen(ServiceName) == 9 && _stricmp(ServiceName, "semaphore") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_SEMAPHORE, ServiceName + SawUnderscore + 1, rule);

	// mailslot operation
	if (strlen(ServiceName) == 8 && _stricmp(ServiceName, "mailslot") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_MAILSLOT, ServiceName + SawUnderscore + 1, rule);

	// named pipe operation
	if (strlen(ServiceName) == 9 && _stricmp(ServiceName, "namedpipe") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_NAMEDPIPE, ServiceName + SawUnderscore + 1, rule);

	// job object operation
	if (strlen(ServiceName) == 3 && _stricmp(ServiceName, "job") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_JOB, ServiceName + SawUnderscore + 1, rule);

	// mutant operation
	if (strlen(ServiceName) == 5 && _stricmp(ServiceName, "mutex") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_MUTANT, ServiceName + SawUnderscore + 1, rule);

	// port operation
	if (strlen(ServiceName) == 4 && _stricmp(ServiceName, "port") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_PORT, ServiceName + SawUnderscore + 1, rule);

	// symlink operation
	if (strlen(ServiceName) == 7 && _stricmp(ServiceName, "symlink") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_SYMLINK, ServiceName + SawUnderscore + 1, rule);

	// timer operation
	if (strlen(ServiceName) == 5 && _stricmp(ServiceName, "timer") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_TIMER, ServiceName + SawUnderscore + 1, rule);

	// process operation
	if (strlen(ServiceName) == 7 && _stricmp(ServiceName, "process") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_PROCESS, ServiceName + SawUnderscore + 1, rule);

	// driver operation
	if (strlen(ServiceName) == 6 && _stricmp(ServiceName, "driver") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_DRIVER, ServiceName + SawUnderscore + 1, rule);

	// object directory operation
	if (strlen(ServiceName) == 6 && _stricmp(ServiceName, "dirobj") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_DIROBJ, ServiceName + SawUnderscore + 1, rule);

	// atom operation
	if (strlen(ServiceName) == 4 && _stricmp(ServiceName, "atom") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_ATOM, ServiceName + SawUnderscore + 1, rule);

	// network operation
	if (strlen(ServiceName) == 7 && _stricmp(ServiceName, "network") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_NETWORK, ServiceName + SawUnderscore + 1, rule);

	// service operation
	if (strlen(ServiceName) == 7 && _stricmp(ServiceName, "service") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_SERVICE, ServiceName + SawUnderscore + 1, rule);

	// time operation
	if (strlen(ServiceName) == 4 && _stricmp(ServiceName, "time") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_TIME, ServiceName + SawUnderscore + 1, rule);

	// token operation
	if (strlen(ServiceName) == 5 && _stricmp(ServiceName, "token") == 0)
		return PolicyParseObjectRule(pSecPolicy, RULE_TOKEN, ServiceName + SawUnderscore + 1, rule);


	/*
	 * non object rules
	 */

	// syscall
	if (strlen(ServiceName) == 7 && _stricmp(ServiceName, "syscall") == 0)
		return PolicyParseSyscallRule(pSecPolicy, ServiceName + SawUnderscore + 1, rule);

	// policy
	if (strlen(ServiceName) == 6 && _stricmp(ServiceName, "policy") == 0)
		return PolicyParsePolicyRule(pSecPolicy, ServiceName + SawUnderscore + 1, rule, Critical);

	// protection
	if (strlen(ServiceName) == 10 && _stricmp(ServiceName, "protection") == 0)
		return PolicyParseProtectionRule(pSecPolicy, ServiceName + SawUnderscore + 1, rule);

	// media
	if (strlen(ServiceName) == 5 && _stricmp(ServiceName, "media") == 0)
		return PolicyParseMediaRule(pSecPolicy, ServiceName + SawUnderscore + 1, rule);



	ABORT_PolicyParseRule(("Invalid rule type specification: '%s'.\n", ServiceName));
}



/*
 * ParseDllOperation()
 *
 * Description:
 *		Parses an operation (i.e. "load" in "dll_load") specified for a DLL object rule.
 *
 * Parameters:
 *		Operation - specified operation.
 *
 * Returns:
 *		OP_INVALID if a specified operation is invalid or an OP_* value corresponding to the parsed operation.
 */

UCHAR
ParseDllOperation(IN PCHAR Operation)
{
	if (strlen(Operation) == 4 && _stricmp(Operation, "load") == 0)
		return OP_LOAD;

	if (strlen(Operation) == 3 && _stricmp(Operation, "all") == 0)
		return OP_ALL;

	return OP_INVALID;
}



/*
 * ParseTimeOperation()
 *
 * Description:
 *		Parses an operation (i.e. "change" in "time_change") specified for a Time object rule.
 *
 * Parameters:
 *		Operation - specified operation.
 *
 * Returns:
 *		OP_INVALID if a specified operation is invalid or an OP_* value corresponding to the parsed operation.
 */

UCHAR
ParseTimeOperation(IN PCHAR Operation)
{
	if (strlen(Operation) == 6 && _stricmp(Operation, "change") == 0)
		return OP_LOAD;

	if (strlen(Operation) == 3 && _stricmp(Operation, "all") == 0)
		return OP_ALL;

	return OP_INVALID;
}



/*
 * ParseTokenOperation()
 *
 * Description:
 *		Parses an operation (i.e. "modify" in "token_modify") specified for a Token object rule.
 *
 * Parameters:
 *		Operation - specified operation.
 *
 * Returns:
 *		OP_INVALID if a specified operation is invalid or an OP_* value corresponding to the parsed operation.
 */

UCHAR
ParseTokenOperation(IN PCHAR Operation)
{
	if (strlen(Operation) == 6 && _stricmp(Operation, "modify") == 0)
		return OP_TOKEN_MODIFY;

	if (strlen(Operation) == 3 && _stricmp(Operation, "all") == 0)
		return OP_ALL;

	return OP_INVALID;
}



/*
 * ParsePortOperation()
 *
 * Description:
 *		Parses an operation (i.e. "create" in "port_create") specified for a port object rule.
 *
 * Parameters:
 *		Operation - specified operation.
 *
 * Returns:
 *		OP_INVALID if a specified operation is invalid or an OP_* value corresponding to the parsed operation.
 */

UCHAR
ParsePortOperation(IN PCHAR Operation)
{
	if (strlen(Operation) == 6 && _stricmp(Operation, "create") == 0)
		return OP_PORT_CREATE;

	if (strlen(Operation) == 7 && _stricmp(Operation, "connect") == 0)
		return OP_PORT_CONNECT;

	if (strlen(Operation) == 3 && _stricmp(Operation, "all") == 0)
		return OP_ALL;


	return OP_INVALID;
}



/*
 * ParseCreateOpenOperation()
 *
 * Description:
 *		Parses a create or an open operation (i.e. "create" in "dirobj_create").
 *
 * Parameters:
 *		Operation - specified operation.
 *
 * Returns:
 *		OP_INVALID if a specified operation is invalid or an OP_* value corresponding to the parsed operation.
 */

UCHAR
ParseCreateOpenOperation(IN PCHAR Operation)
{
	if (strlen(Operation) == 6 && _stricmp(Operation, "create") == 0)
		return OP_CREATE;

	if (strlen(Operation) == 4 && _stricmp(Operation, "open") == 0)
		return OP_OPEN;

	if (strlen(Operation) == 3 && _stricmp(Operation, "all") == 0)
		return OP_ALL;


	return OP_INVALID;
}



/*
 * ParseAtomOperation()
 *
 * Description:
 *		Parses an operation (i.e. "find" in "atom_find") specified for an atom object rule.
 *
 * Parameters:
 *		Operation - specified operation.
 *
 * Returns:
 *		OP_INVALID if a specified operation is invalid or an OP_* value corresponding to the parsed operation.
 */

UCHAR
ParseAtomOperation(IN PCHAR Operation)
{
	if (strlen(Operation) == 4 && _stricmp(Operation, "find") == 0)
		return OP_FIND;

	if (strlen(Operation) == 3 && _stricmp(Operation, "add") == 0)
		return OP_ADD;

	if (strlen(Operation) == 3 && _stricmp(Operation, "all") == 0)
		return OP_ALL;


	return OP_INVALID;
}



/*
 * ParseDriverOperation()
 *
 * Description:
 *		Parses an operation (i.e. "load" in "driver_load") specified for a driver object rule.
 *
 * Parameters:
 *		Operation - specified operation.
 *
 * Returns:
 *		OP_INVALID if a specified operation is invalid or an OP_* value corresponding to the parsed operation.
 */

UCHAR
ParseDriverOperation(IN PCHAR Operation)
{
	if (strlen(Operation) == 4 && _stricmp(Operation, "load") == 0)
		return OP_LOAD;

	if (strlen(Operation) == 7 && _stricmp(Operation, "regload") == 0)
		return OP_REGLOAD;

	if (strlen(Operation) == 3 && _stricmp(Operation, "all") == 0)
		return OP_ALL;

	return OP_INVALID;
}



/*
 * ParseDirectoryOperation()
 *
 * Description:
 *		Parses an operation (i.e. "create" in "directory_create") specified for a directory object rule.
 *
 * Parameters:
 *		Operation - specified operation.
 *
 * Returns:
 *		OP_INVALID if a specified operation is invalid or an OP_* value corresponding to the parsed operation.
 */

UCHAR
ParseDirectoryOperation(IN PCHAR Operation)
{
	if (strlen(Operation) == 6 && _stricmp(Operation, "create") == 0)
		return OP_DIR_CREATE;

	if (strlen(Operation) == 3 && _stricmp(Operation, "all") == 0)
		return OP_ALL;

	return OP_INVALID;
}



/*
 * ParseObjectOperation()
 *
 * Description:
 *		Parses an operation (i.e. "read" in "file_read") specified for an object (file, registry, etc) rule.
 *
 * Parameters:
 *		Operation - specified operation.
 *
 * Returns:
 *		OP_INVALID if a specified operation is invalid or an OP_* value corresponding to the parsed operation.
 */

UCHAR
ParseObjectOperation(IN PCHAR Operation)
{
	if (strlen(Operation) == 4 && _stricmp(Operation, "read") == 0)
		return OP_READ;

	if (strlen(Operation) == 5 && _stricmp(Operation, "write") == 0)
		return OP_WRITE;

	if (strlen(Operation) == 2 && _stricmp(Operation, "rw") == 0)
		return (OP_READ | OP_WRITE);

	if (strlen(Operation) == 3 && _stricmp(Operation, "all") == 0)
		return OP_ALL;

	//XXX valid for files only
//	if (strlen(Operation) == 6 && _stricmp(Operation, "append") == 0)
//		return OP_APPEND;

	if (strlen(Operation) == 7 && _stricmp(Operation, "execute") == 0)
		return OP_EXECUTE;

	if (strlen(Operation) == 6 && _stricmp(Operation, "delete") == 0)
		return OP_DELETE;


	return OP_INVALID;
}



/*
 * ParseProcessOperation()
 *
 * Description:
 *		Parses an operation (i.e. "execute" in "process_execute") specified for a process rule.
 *
 * Parameters:
 *		Operation - specified operation.
 *
 * Returns:
 *		OP_INVALID if a specified operation is invalid or an OP_* value corresponding to the parsed operation.
 */

UCHAR
ParseProcessOperation(IN PCHAR Operation)
{
	if (strlen(Operation) == 3 && _stricmp(Operation, "all") == 0)
		return OP_ALL;

	if (strlen(Operation) == 7 && _stricmp(Operation, "execute") == 0)
		return OP_PROC_EXECUTE;

	if (strlen(Operation) == 4 && _stricmp(Operation, "open") == 0)
		return OP_PROC_OPEN;


	return OP_INVALID;
}



/*
 * ParseServiceOperation()
 *
 * Description:
 *		Parses an operation (i.e. "start" in "service_start") specified for a service object rule.
 *
 * Parameters:
 *		Operation - specified operation.
 *
 * Returns:
 *		OP_INVALID if a specified operation is invalid or an OP_* value corresponding to the parsed operation.
 */

UCHAR
ParseServiceOperation(IN PCHAR Operation)
{
	if (strlen(Operation) == 3 && _stricmp(Operation, "all") == 0)
		return OP_ALL;

	/*
	 * We cannot distinguish between various service operations since service rules are actually
	 * enforced by the registry rules. Thus convert all operations to OP_ALL for now.
	 */

	if (strlen(Operation) == 5 && _stricmp(Operation, "start") == 0)
		return OP_ALL;//OP_SERVICE_START;

	if (strlen(Operation) == 4 && _stricmp(Operation, "stop") == 0)
		return OP_ALL;//OP_SERVICE_STOP;

	if (strlen(Operation) == 6 && _stricmp(Operation, "create") == 0)
		return OP_ALL;//OP_SERVICE_CREATE;

	if (strlen(Operation) == 6 && _stricmp(Operation, "delete") == 0)
		return OP_ALL;//OP_SERVICE_DELETE;

	return OP_INVALID;
}



/*
 * ParseNetworkOperation()
 *
 * Description:
 *		Parses an operation (i.e. "bind" in "network_bind") specified for a network object rule.
 *
 * Parameters:
 *		Operation - specified operation.
 *
 * Returns:
 *		OP_INVALID if a specified operation is invalid or an OP_* value corresponding to the parsed operation.
 */

UCHAR
ParseNetworkOperation(IN PCHAR Operation)
{
	if (strlen(Operation) == 3 && _stricmp(Operation, "all") == 0)
		return OP_ALL;

	if (strlen(Operation) == 10 && _stricmp(Operation, "tcpconnect") == 0)
		return OP_TCPCONNECT;

	if (strlen(Operation) == 10 && _stricmp(Operation, "udpconnect") == 0)
		return OP_UDPCONNECT;

	if (strlen(Operation) == 7 && _stricmp(Operation, "connect") == 0)
		return OP_CONNECT;

	if (strlen(Operation) == 4 && _stricmp(Operation, "bind") == 0)
		return OP_BIND;

	return OP_INVALID;
}




/*****************************************************************************/




/*
 * ParseNetworkObject()
 *
 * Description:
 *		Parses the specified network address (i.e. "127.0.0.1:443").
 *
 * Parameters:
 *		name - string value to parse.
 *		Object - pointer to an Object where the final result will be saved
 *		wildcard - pointer to a BOOLEAN that will indicate whether the specified value contained any regular expressions.
 *
 * Returns:
 *		INVALID_OBJECT_SIZE if the specified network address is invalid. 0 to indicate SUCCESS.
 *		Network addresses do not require any additional memory to be allocated thus the returned size is 0.
 */

size_t
ParseNetworkObject(IN PCHAR name, OUT PCHAR *Object, OUT BOOLEAN *wildcard)
{
	PCHAR	colon;


	//XXX
	// for now connect format is "ipaddr" while bind format is "ipaddr:port"

	colon = strchr(name, ':');

	if (colon)
	{
		*Object = colon + 1;
//		if ((*Object = (PVOID) atoi(colon + 1)) == 0)
//			return INVALID_OBJECT_SIZE;
	}
	else
	{
		*Object = name;
//		if ((*Object = (PVOID) inet_addr(name)) == 0)
//			return INVALID_OBJECT_SIZE;
	}


	return strlen(*Object);
}



/*
 * ParseStub()
 *
 * Description:
 *		Parse stub for strings that do no require any further parsing.
 *
 * Parameters:
 *		name - string value to parse.
 *		Object - pointer to an Object where the final result will be saved.
 *		wildcard - pointer to a BOOLEAN that will indicate whether the specified value contained any regular expressions.
 *
 * Returns:
 *		Length of the specified string value.
 */

size_t
ParseStub(IN PCHAR name, OUT PCHAR *ObjectName, OUT BOOLEAN *wildcard)
{
	*ObjectName = name;

	return strlen(name);
}



/*
 * ParseRegistryObject()
 *
 * Description:
 *		Convert user land registry object names into their kernel land equivalents.
 *
 * Parameters:
 *		name - string value to parse.
 *		Object - pointer to an Object where the final result will be saved.
 *		wildcard - pointer to a BOOLEAN that will indicate whether the specified value contained any regular expressions.
 *
 * Returns:
 *		Length of the specified string value.
 */

size_t
ParseRegistryObject(IN PCHAR name, OUT PCHAR *ObjectName, OUT BOOLEAN *wildcard)
{
	PCHAR			key;
	static CHAR		buffer[MAX_PATH] = { 0 };


	if (_strnicmp(name, "HKEY_LOCAL_MACHINE\\", 19) == 0)
	{
		/* replace HKEY_LOCAL_MACHINE\ with kernel equivalent of \REGISTRY\MACHINE\ */

		strcpy(name + 1, "\\REGISTRY\\MACHINE");
		name[18] = '\\';

		key = name + 1;
	}
	else if (_strnicmp(name, "HKEY_USERS\\", 11) == 0)
	{
		/* replace HKEY_USERS\ with kernel equivalent of \REGISTRY\USER\ */

		strcpy(buffer, "\\REGISTRY\\USER\\");
		strncat(buffer, name + 11, MAX_PATH - 12);

		key = buffer;
	}
	else
	{
		key = name;
	}


	*ObjectName = key;


	return strlen(key);
}



/*
 * ParseFileObject()
 *
 * Description:
 *		Convert user land file object names into their kernel land equivalents.
 *
 * Parameters:
 *		name - string value to parse.
 *		Object - pointer to an Object where the final result will be saved.
 *		wildcard - pointer to a BOOLEAN that will indicate whether the specified value contained any regular expressions.
 *
 * Returns:
 *		Length of the specified string value.
 */

size_t
ParseFileObject(IN PCHAR name, OUT PCHAR *Object, OUT BOOLEAN *wildcard)
{
	PCHAR			filename;
	static CHAR		buffer[MAX_PATH] = { 0 };	//XXX not SMP safe


	if (_strnicmp(name, "%systemdrive%:", 14) == 0)
	{
		name[12] = SystemDrive;

		name += 12;
	}


	// match drive wildcards such as "?:" and "*:" with "\Device\*"
	if (name[1] == ':' && name[2] == '\\' && (name[0] == '?' || name[0] == '*'))
	{
#if 0
		ConvertLongFileNameToShort(name, buffer + 7, MAX_PATH - 7);

		strcpy(buffer, "\\Device\\*");
		buffer[9] = '\\';	/* replace the zero terminator */

		filename = buffer;
#endif

		strcpy(name - 7, "\\Device\\*");

		/*
		 * replace "\Device\*" terminating zero with a '\'
		 * since name is just a pointer to FullName+7, FullName now contains
		 * \Device\*\<user specified path>
		 */

		name[2] = '\\';

		filename = name - 7;


		// mark the rule as wildcard even if the user (mistakenly) used "eq"
		// XXX or should we throw an error if wildcard==0?
		*wildcard = TRUE;
	}
	else if (isalpha(name[0]) && name[1] == ':')
	{
#if 0
		CHAR	buffer2[MAX_PATH];


		ConvertLongFileNameToShort(name, buffer2 + 4, MAX_PATH - 4);

		buffer2[0] = '\\';
		buffer2[1] = '?';
		buffer2[2] = '?';
		buffer2[3] = '\\';

		if (ResolveFilename(buffer2, buffer, MAX_PATH) == FALSE)
			LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("ParseFileObject: ResolveFilename(%s) failed\n", name - 4));
#endif


		// match <letter>: drive specifications and prepend "\??\" to them

		*(name - 4) = '\\';
		*(name - 3) = '?';
		*(name - 2) = '?';
		*(name - 1) = '\\';

		if (ResolveFilename(name - 4, buffer, MAX_PATH) == FALSE)
			LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("ParseFileObject: ResolveFilename(%s) failed\n", name - 4));

		filename = buffer;
	}
	else if (_strnicmp(name, "%systemroot%\\", 13) == 0)
	{
		strcpy(buffer, SystemRoot);
		strcat(buffer, name + 12);

		filename = buffer;
	}
	else if (_strnicmp(name, "\\pipe\\", 6) == 0)
	{
		strcpy(buffer, "\\device\\namedpipe");
		strcat(buffer, name + 5);

		filename = buffer;
	}
	else
	{
		filename = name;
	}


	*Object = filename;

	return strlen(filename);
}



/*
 * ParseProcessObject()
 *
 * Description:
 *		Convert user land process object names into their kernel land equivalents (strip the drive specification).
 *
 * Parameters:
 *		name - string value to parse.
 *		Object - pointer to an Object where the final result will be saved.
 *		wildcard - pointer to a BOOLEAN that will indicate whether the specified value contained any regular expressions.
 *
 * Returns:
 *		Length of the specified string value.
 */

size_t
ParseProcessObject(IN PCHAR name, OUT PCHAR *Object, OUT BOOLEAN *wildcard)
{
	static CHAR		buffer[MAX_PATH] = { 0 };


	if ((name = StripFileMacros(name, buffer, MAX_PATH)) == NULL)
		return INVALID_OBJECT_SIZE;


	*Object = name;

	return strlen(name);
}



/*
 * ParseBaseNamedObjectsObject()
 *
 * Description:
 *		Convert user land object names into their kernel land equivalents.
 *
 * Parameters:
 *		name - string value to parse.
 *		Object - pointer to an Object where the final result will be saved.
 *		wildcard - pointer to a BOOLEAN that will indicate whether the specified value contained any regular expressions.
 *
 * Returns:
 *		Length of the specified string value.
 */

size_t
ParseBaseNamedObjectsObject(IN PCHAR name, OUT PCHAR *Object, OUT BOOLEAN *wildcard)
{
	PCHAR			ObjectName;


	/*
	 * if an object name does not start with a slash '\' then prepend '\BaseNamedObjects\' to it
	 */

	if (name[0] != '\\')
	{
		//XXX this is a hack, we are prepending to our buffer, knowing that there is space there
		strcpy(name - 18, "\\BaseNamedObjects");

		*(name - 1) = '\\';

		ObjectName = name - 18;
	}
	else
	{
		ObjectName = name;
	}


	*Object = ObjectName;

	return strlen(ObjectName);
}



/*
 * ParseMailslotObject()
 *
 * Description:
 *		Convert user land mailslot object names into their kernel land equivalents.
 *
 * Parameters:
 *		name - string value to parse.
 *		Object - pointer to an Object where the final result will be saved.
 *		wildcard - pointer to a BOOLEAN that will indicate whether the specified value contained any regular expressions.
 *
 * Returns:
 *		Length of the specified string value.
 */

size_t
ParseMailslotObject(IN PCHAR name, OUT PCHAR *Object, OUT BOOLEAN *wildcard)
{
	PCHAR			MailslotName;


	/*
	 * if the mailslot name does not start with a slash '\' then prepend '\Device\Mailslot\' to the name
	 */

	if (name[0] != '\\')
	{
		//XXX this is a hack, we are prepending to our buffer, knowing that there is space there
		strcpy(name - 17, "\\Device\\Mailslot");

		*(name - 1) = '\\';

		MailslotName = name - 17;
	}
	else
	{
		MailslotName = name;
	}


	*Object = MailslotName;

	return strlen(MailslotName);
}



/*
 * ParseNamedpipeObject()
 *
 * Description:
 *		Convert user land namedpipe object names into their kernel land equivalents.
 *
 * Parameters:
 *		name - string value to parse.
 *		Object - pointer to an Object where the final result will be saved.
 *		wildcard - pointer to a BOOLEAN that will indicate whether the specified value contained any regular expressions.
 *
 * Returns:
 *		Length of the specified string value.
 */

size_t
ParseNamedpipeObject(IN PCHAR name, OUT PCHAR *Object, OUT BOOLEAN *wildcard)
{
	PCHAR			NamedpipeName;


	/*
	 * if the namedpipe name does not start with a slash '\' then prepend '\Device\Namedpipe\' to the name
	 */

	if (name[0] != '\\')
	{
		//XXX this is a hack, we are prepending to our buffer, knowing that there is space there
		strcpy(name - 18, "\\Device\\Namedpipe");

		*(name - 1) = '\\';

		NamedpipeName = name - 18;
	}
	else
	{
		NamedpipeName = name;
	}


	*Object = NamedpipeName;

	return strlen(NamedpipeName);
}



/*
 * ParseDllObject()
 *
 * Description:
 *		Convert user land DLL object names into their kernel land equivalents.
 *		Since DLL rules are actually enforced by section rules, we just append DLL names to
 *		'\KnownDlls\' string which is used by section rules.
 *
 * Parameters:
 *		name - string value to parse.
 *		Object - pointer to an Object where the final result will be saved.
 *		wildcard - pointer to a BOOLEAN that will indicate whether the specified value contained any regular expressions.
 *
 * Returns:
 *		Length of the specified string value.
 */

size_t
ParseDllObject(IN PCHAR name, OUT PCHAR *Object, OUT BOOLEAN *wildcard)
{
	PCHAR			DllName;


	/*
	 * if the DLL name does not start with a slash '\' then prepend '\KnownDlls\' to the name
	 */

	if (name[0] != '\\')
	{
		strcpy(name - 11, "\\KnownDlls");

		*(name - 1) = '\\';

		DllName = name - 11;
	}
	else
	{
		DllName = name;
	}


	*Object = DllName;

	return strlen(DllName);
}



/*
 * ParseTimeObject()
 *
 * Description:
 *		Time rule specifications are not supposed to have any object names specified.
 *		Return an error.
 *
 * Parameters:
 *		name - string value to parse.
 *		Object - pointer to an Object where the final result will be saved.
 *		wildcard - pointer to a BOOLEAN that will indicate whether the specified value contained any regular expressions.
 *
 * Returns:
 *		An error.
 */

size_t
ParseTimeObject(IN PCHAR name, OUT PCHAR *Object, OUT BOOLEAN *wildcard)
{
	return INVALID_OBJECT_SIZE;
}



/*
 * ParseServiceObject()
 *
 * Description:
 *		Convert user land service object names into their kernel land equivalents.
 *		Since service rules are actually enforced by registry rules, we just append service names
 *		to '\Registry\Machine\System\*ControlSet*\Services\' string which is used by registry rules.
 *
 * Parameters:
 *		name - string value to parse.
 *		Object - pointer to an Object where the final result will be saved.
 *		wildcard - pointer to a BOOLEAN that will indicate whether the specified value contained any regular expressions.
 *
 * Returns:
 *		Length of the specified string value.
 *		INVALID_OBJECT_SIZE is service name is too long.
 */

size_t
ParseServiceObject(IN PCHAR name, OUT PCHAR *Object, OUT BOOLEAN *wildcard)
{
	static CHAR		buffer[MAX_PATH] = { 0 };	//XXX not SMP safe


	if (strlen(name) > 64)
	{
		return INVALID_OBJECT_SIZE;
	}

	strcpy(buffer, "\\Registry\\Machine\\System\\*ControlSet*\\Services\\");
	strcat(buffer, name);

	*wildcard = TRUE;

	*Object = buffer;


	return strlen(buffer);
}



typedef size_t (*OBJECT_PARSER)(IN PCHAR name, OUT PCHAR *Object, OUT BOOLEAN *wildcard);
typedef UCHAR (*OPERATION_TYPE_PARSER)(IN PCHAR name);


/* in C++ these would be member methods */

struct _ObjectParseOps
{
	RULE_TYPE				RuleType;
	OBJECT_PARSER			ObjectNameParser;
	OPERATION_TYPE_PARSER	OperationTypeParser;

} ObjectParseOps[] =
{
	{ RULE_FILE, ParseFileObject, ParseObjectOperation },
	{ RULE_DIRECTORY, ParseFileObject, ParseDirectoryOperation },
	{ RULE_MAILSLOT, ParseMailslotObject, ParseObjectOperation },
	{ RULE_NAMEDPIPE, ParseNamedpipeObject, ParseObjectOperation },
	{ RULE_REGISTRY, ParseRegistryObject, ParseObjectOperation },
	{ RULE_SECTION, ParseBaseNamedObjectsObject, ParseObjectOperation },
	{ RULE_DLL, ParseDllObject, ParseDllOperation },
	{ RULE_EVENT, ParseBaseNamedObjectsObject, ParseCreateOpenOperation },
	{ RULE_SEMAPHORE, ParseBaseNamedObjectsObject, ParseCreateOpenOperation },
	{ RULE_JOB, ParseBaseNamedObjectsObject, ParseCreateOpenOperation },
	{ RULE_MUTANT, ParseBaseNamedObjectsObject, ParseCreateOpenOperation },
	{ RULE_PORT, ParseStub, ParsePortOperation },
	{ RULE_SYMLINK, ParseStub, ParseCreateOpenOperation },
	{ RULE_TIMER, ParseBaseNamedObjectsObject, ParseCreateOpenOperation },
	{ RULE_PROCESS, ParseProcessObject, ParseProcessOperation },
	{ RULE_DRIVER, ParseProcessObject, ParseDriverOperation },
	{ RULE_DIROBJ, ParseStub, ParseCreateOpenOperation },
	{ RULE_ATOM, ParseStub, ParseAtomOperation },
	{ RULE_NETWORK, ParseNetworkObject, ParseNetworkOperation },
	{ RULE_SERVICE, ParseServiceObject, ParseServiceOperation },
	{ RULE_TIME, ParseTimeObject, ParseTimeOperation },
	{ RULE_TOKEN, ParseTimeObject, ParseTokenOperation },
};



/*
 * PolicyParseActionClause()
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
PolicyParseActionClause(PCHAR rule, ACTION_TYPE *ActionType, UCHAR *RuleNumber)
{
	UCHAR		len = 0, num = 0;


	SKIP_WHITESPACE(rule);


	if (_strnicmp(rule, "permit", 6) == 0)
	{
		rule += 6;

		*ActionType = ACTION_PERMIT;
	}
	else if (_strnicmp(rule, "deny", 4) == 0)
	{
		rule += 4;

		*ActionType = ACTION_DENY;
	}
	else if (_strnicmp(rule, "quietdeny", 9) == 0)
	{
		rule += 9;

		*ActionType = ACTION_QUIETDENY;
	}
	else if (_strnicmp(rule, "log", 3) == 0)
	{
		rule += 3;

		*ActionType = ACTION_LOG;
	}
	else if (_strnicmp(rule, "ask", 3) == 0)
	{
		rule += 3;

		*ActionType = ACTION_ASK;
	}
	else
	{
		ABORT_PolicyParseRule(("Expecting to see 'permit', 'deny', 'quitedeny', 'log' or 'ask' clause. Got '%s'\n", rule));
	}


	SKIP_WHITESPACE(rule);


	/* EOL? */
	if (*rule == 0)
	{
		if (RuleNumber)
			*RuleNumber = 0;

		return TRUE;
	}


	/* if it is not EOL then we expect to see "[rule DIGIT]" clause */
	if (_strnicmp(rule, "[rule", 5) == 0)
	{
		rule += 5;
	}
	else
	{
		ABORT_PolicyParseRule(("Expecting to see a rule clause. Got '%s'\n", rule));
	}


	if (! IS_WHITESPACE(*rule))
	{
		ABORT_PolicyParseRule(("Expecting to see white space. Got '%s'\n", rule));
	}

	SKIP_WHITESPACE(rule);


	/* parse the rule number (a decimal digit) */
	while (*rule >= '0' && *rule <= '9')
	{
		/* don't overflow UCHAR values */
		if (++len > 2)
		{
			ABORT_PolicyParseRule(("The rule number is too long.\n"));
		}

		num = num*10 + (*rule - '0');

		++rule;
	}


	SKIP_WHITESPACE(rule);


	if (*rule != ']')
	{
		ABORT_PolicyParseRule(("Invalid rule clause: '%s'\n", rule));
	}


	++rule;
	SKIP_WHITESPACE(rule);


	/* expecting an EOL */
	if (*rule != 0)
	{
		ABORT_PolicyParseRule(("Expecting to see end of line. Got '%s'\n", rule));
	}


	if (RuleNumber)
		*RuleNumber = num;


	return TRUE;
}



/*
 * PolicyParseOnOffClause()
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
PolicyParseOnOffClause(PCHAR rule, BOOLEAN *OnOff)
{
	SKIP_WHITESPACE(rule);


	if (_strnicmp(rule, "on", 2) == 0)
	{
		rule += 2;

		*OnOff = TRUE;
	}
	else if (_strnicmp(rule, "off", 3) == 0)
	{
		rule += 3;

		*OnOff = FALSE;
	}
	else
	{
		ABORT_PolicyParseRule(("Expecting to see 'on' or 'off' clause. Got '%s'\n", rule));
	}


	SKIP_WHITESPACE(rule);

	/* expecting an EOL */
	if (*rule != 0)
	{
		ABORT_PolicyParseRule(("Expecting to see end of line. Got '%s'\n", rule));
	}

	return TRUE;
}



/*
 * VerifyToken2()
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

UCHAR
VerifyToken2(PCHAR *rule, PCHAR token1, PCHAR token2)
{
	USHORT	Token1Length, Token2Length;


	ASSERT(rule && *rule);
	ASSERT(token1);
	ASSERT(token2);


	SKIP_WHITESPACE(*rule);


	Token1Length = (USHORT) strlen(token1);
	Token2Length = (USHORT) strlen(token2);

	if (_strnicmp(*rule, token1, Token1Length) == 0)
	{
		*rule += Token1Length;

        if (! IS_WHITESPACE(**rule))
		{
//			LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_WARNING, ("Expecting to see whitespace. Got '%s'\n", *rule));
			return FALSE;
		}

		SKIP_WHITESPACE(*rule);

		return 1;
	}


	if (_strnicmp(*rule, token2, Token2Length) == 0)
	{
		*rule += Token2Length;

        if (! IS_WHITESPACE(**rule))
		{
//			LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_WARNING, ("Expecting to see whitespace. Got '%s'\n", *rule));
			return FALSE;
		}

		SKIP_WHITESPACE(*rule);

		return 2;
	}


	return 0;
}



/*
 * PolicyParsePolicyRule()
 *
 * Description:
 *		Parse a policy rule and adjust the policy default action.
 *
 * Parameters:
 *		pSecPolicy - security policy that the rule is going to be added to.
 *		Operation - ASCII operation (valid options are 'default' and 'path').
 *		Rule - ASCII rule to parse.
 *		Critical - Boolean indicating whether the parser should abort parsing the policy due to a critical error.
 *
 * Returns:
 *		Nothing.
 */

BOOLEAN
PolicyParsePolicyRule(OUT PSECURITY_POLICY pSecPolicy, IN PCHAR Operation, IN PCHAR rule, OUT BOOLEAN *Critical)
{
	ACTION_TYPE		ActionType;
	UCHAR			RuleNumber;


	*Critical = FALSE;


	if (strlen(Operation) == 7 && _stricmp(Operation, "default") == 0)
	{
		if (PolicyParseActionClause(rule, &ActionType, &RuleNumber) == FALSE)

			ABORT_PolicyParseRule(("Invalid default policy action specification. Format: \"policy_default: (permit|deny|quietdeny|log|ask)\"\n"));


		/* did we initialize default policy action already? */
		if (pSecPolicy->DefaultPolicyAction != ACTION_NONE)

			ABORT_PolicyParseRule(("Duplicate default policy action specification.\n"));


		/* this still allows "policy_default: permit [rule 0]", oh well */
		if (RuleNumber != 0)

			ABORT_PolicyParseRule(("Rule clause cannot appear in default policy action specification.\n"));


		pSecPolicy->DefaultPolicyAction = ActionType | ACTION_DEFAULT;

		return TRUE;
	}


	if (strlen(Operation) == 4 && _stricmp(Operation, "path") == 0)
	{
		CHAR	szPath[MAX_PATH];
		CHAR	szPolicyPath[MAX_PATH];


		_snprintf(szPath, MAX_PATH, "%S", gFilePath);
		szPath[MAX_PATH - 1] = 0;


		rule = StripFileMacros(rule, szPolicyPath, MAX_PATH);
		

		KdPrint(("%s\n%s\n", szPath, rule));

		if (WildcardMatch(szPath, rule) == WILDCARD_MATCH)
		{
			KdPrint(("paths match\n"));
			return TRUE;
		}


		if (LearningMode == FALSE)
			*Critical = TRUE;

		KdPrint(("paths do not match\n"));


		return FALSE;
	}


	ABORT_PolicyParseRule(("Invalid policy operation '%s'. Valid options are 'default' and 'path'.\n", Operation));
}



/*
 * PolicyParseProtectionRule()
 *
 * Description:
 *		Parse a protection rule and adjust the protection options such as buffer overflow protection
 *		and userland (dll injection) protection being on and off
 *
 * Parameters:
 *		pSecPolicy - security policy that the rule is going to be added to.
 *		Operation - ASCII operation. Valid options are 'overflow', 'userland', 'debugging', 'dos16', 'keyboard', 'modem', 'sniffer', 'extension' and 'all'.
 *		Rule - ASCII rule to parse.
 *
 * Returns:
 *		Nothing.
 */

BOOLEAN
PolicyParseProtectionRule(PSECURITY_POLICY pSecPolicy, PCHAR Operation, PCHAR rule)
{
	BOOLEAN		OnOff;


	if (strlen(Operation) == 8 && _stricmp(Operation, "overflow") == 0)
	{
		if (PolicyParseOnOffClause(rule, &OnOff) == FALSE)
			return FALSE;

		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_VERBOSE, ("PolicyParseProtectionRule: Turning overflow protection %s\n", OnOff ? "on" : "off"));

		if (OnOff)
			pSecPolicy->ProtectionFlags |= PROTECTION_OVERFLOW;
		else
			pSecPolicy->ProtectionFlags &= ~PROTECTION_OVERFLOW;

		return TRUE;
	}


	if (strlen(Operation) == 8 && _stricmp(Operation, "userland") == 0)
	{
		if (PolicyParseOnOffClause(rule, &OnOff) == FALSE)
			return FALSE;

		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_VERBOSE, ("PolicyParseProtectionRule: Turning userland protection %s\n", OnOff ? "on" : "off"));

		if (OnOff)
			pSecPolicy->ProtectionFlags |= PROTECTION_USERLAND;
		else
			pSecPolicy->ProtectionFlags &= ~PROTECTION_USERLAND;

		return TRUE;
	}


	if (strlen(Operation) == 9 && _stricmp(Operation, "debugging") == 0)
	{
		if (PolicyParseOnOffClause(rule, &OnOff) == FALSE)
			return FALSE;

		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_VERBOSE, ("PolicyParseProtectionRule: Turning debugging protection %s\n", OnOff ? "on" : "off"));

		if (OnOff)
			pSecPolicy->ProtectionFlags |= PROTECTION_DEBUGGING;
		else
			pSecPolicy->ProtectionFlags &= ~PROTECTION_DEBUGGING;

		return TRUE;
	}


	if (strlen(Operation) == 5 && _stricmp(Operation, "dos16") == 0)
	{
		if (PolicyParseOnOffClause(rule, &OnOff) == FALSE)
			return FALSE;

		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_VERBOSE, ("PolicyParseProtectionRule: Turning dos16/vdm protection %s\n", OnOff ? "on" : "off"));

		if (OnOff)
			pSecPolicy->ProtectionFlags |= PROTECTION_VDM;
		else
			pSecPolicy->ProtectionFlags &= ~PROTECTION_VDM;

		return TRUE;
	}


	if (strlen(Operation) == 8 && _stricmp(Operation, "keyboard") == 0)
	{
		if (PolicyParseOnOffClause(rule, &OnOff) == FALSE)
			return FALSE;

		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_VERBOSE, ("PolicyParseProtectionRule: Turning keyboard logger protection %s\n", OnOff ? "on" : "off"));

		if (OnOff)
			pSecPolicy->ProtectionFlags |= PROTECTION_KEYBOARD;
		else
			pSecPolicy->ProtectionFlags &= ~PROTECTION_KEYBOARD;

		return TRUE;
	}


	if (strlen(Operation) == 5 && _stricmp(Operation, "modem") == 0)
	{
		if (PolicyParseOnOffClause(rule, &OnOff) == FALSE)
			return FALSE;

		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_VERBOSE, ("PolicyParseProtectionRule: Turning modem protection %s\n", OnOff ? "on" : "off"));

		if (OnOff)
			pSecPolicy->ProtectionFlags |= PROTECTION_MODEM;
		else
			pSecPolicy->ProtectionFlags &= ~PROTECTION_MODEM;

		return TRUE;
	}


	if (strlen(Operation) == 7 && _stricmp(Operation, "sniffer") == 0)
	{
		if (PolicyParseOnOffClause(rule, &OnOff) == FALSE)
			return FALSE;

		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_VERBOSE, ("PolicyParseProtectionRule: Turning sniffer protection %s\n", OnOff ? "on" : "off"));

		if (OnOff)
			pSecPolicy->ProtectionFlags |= PROTECTION_SNIFFER;
		else
			pSecPolicy->ProtectionFlags &= ~PROTECTION_SNIFFER;

		return TRUE;
	}


	if (strlen(Operation) == 9 && _stricmp(Operation, "extension") == 0)
	{
		if (PolicyParseOnOffClause(rule, &OnOff) == FALSE)
			return FALSE;

		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_VERBOSE, ("PolicyParseProtectionRule: Turning extension protection %s\n", OnOff ? "on" : "off"));

		if (OnOff)
			pSecPolicy->ProtectionFlags |= PROTECTION_EXTENSION;
		else
			pSecPolicy->ProtectionFlags &= ~PROTECTION_EXTENSION;

		return TRUE;
	}


	if (strlen(Operation) == 3 && _stricmp(Operation, "all") == 0)
	{
		if (PolicyParseOnOffClause(rule, &OnOff) == FALSE)
			return FALSE;

		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_VERBOSE, ("PolicyParseProtectionRule: Turning all protection %s\n", OnOff ? "on" : "off"));

		pSecPolicy->ProtectionFlags = OnOff == TRUE ? PROTECTION_ALL_ON : PROTECTION_ALL_OFF;

		return TRUE;
	}


	ABORT_PolicyParseRule(("Invalid protection operation '%s'. Valid options are 'overflow', 'userland', 'debugging', 'dos16', 'keyboard', 'modem', 'sniffer', 'extension' and 'all'.\n", Operation));
}



/*
 * PolicyParseMediaRule()
 *
 * Description:
 *		Parse a media rule.
 *
 * Parameters:
 *		pSecPolicy - security policy that the rule is going to be added to.
 *		Operation - ASCII operation. The only valid option is 'access'.
 *		Rule - ASCII rule to parse.
 *
 * Returns:
 *		Nothing.
 */

BOOLEAN
PolicyParseMediaRule(PSECURITY_POLICY pSecPolicy, PCHAR Operation, PCHAR rule)
{
	if (pSecPolicy != &gSecPolicy)
	{
		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_WARNING, ("PolicyParseMediaRule: Media rules can be setup only in a global policy\n"));
		return TRUE;
	}


	if (strlen(Operation) != 6 || _stricmp(Operation, "access") != 0)
		ABORT_PolicyParseRule(("Invalid media operation '%s'. The only valid option is 'access'.\n", Operation));


	if (strlen(rule) == 6 && _stricmp(rule, "permit") == 0)
	{
		MediaRemovableFlags = MEDIA_REMOVABLE_PERMIT;
		return TRUE;
	}

	if (strlen(rule) == 4 && _stricmp(rule, "deny") == 0)
	{
		MediaRemovableFlags |= MEDIA_REMOVABLE_DISABLE;
		return TRUE;
	}

	if (strlen(rule) == 8 && _stricmp(rule, "readonly") == 0)
	{
		MediaRemovableFlags |= MEDIA_REMOVABLE_READONLY;
		return TRUE;
	}

	if (strlen(rule) == 9 && _stricmp(rule, "noexecute") == 0)
	{
		MediaRemovableFlags |= MEDIA_REMOVABLE_NOEXECUTE;
		return TRUE;
	}


	ABORT_PolicyParseRule(("Expecting to see 'permit', 'deny', 'readonly', or 'noexecute' action. Got '%s'\n", rule));
}



/*
 * InsertPolicyRule()
 *
 * Description:
 *		Adds a rule to a specified security policy (FIFO order).
 *
 * Parameters:
 *		pSecPolicy - security policy that the rule is going to be added to.
 *		PolicyRule - rule to add.
 *		RuleType - rule type (file, network, etc).
 *
 * Returns:
 *		Nothing.
 */

VOID
InsertPolicyRule(PSECURITY_POLICY pSecPolicy, PPOLICY_RULE PolicyRule, RULE_TYPE RuleType)
{
	KIRQL			irql;
	PPOLICY_RULE	tmp;


	ASSERT(RuleType < RULE_LASTONE);


	KeAcquireSpinLock(&pSecPolicy->SpinLock, &irql);


	if (pSecPolicy->RuleList[RuleType] == NULL)
	{
		pSecPolicy->RuleList[RuleType] = PolicyRule;

		KeReleaseSpinLock(&pSecPolicy->SpinLock, irql);

		return;
	}

	/* find the last rule and link the new rule to it */
	tmp = pSecPolicy->RuleList[RuleType];

	while (tmp->Next)
	{
		tmp = tmp->Next;
	}

	tmp->Next = PolicyRule;


	KeReleaseSpinLock(&pSecPolicy->SpinLock, irql);
}



/*
 * PolicyParseObjectRule()
 *
 * Description:
 *		Parse an object rule of the following format:
 *		(name|address) (eq|match) "<objectname>" then (deny|quitedeny|permit|log)
 *		Rule can also consist of just (deny|quitedeny|permit|log)
 *
 * example1:	name match "c:\file*" then deny
 * example2:	address eq "192.168.0.1" then log
 * example3:	quietdeny
 *
 * Parameters:
 *		pSecPolicy - security policy that the rule is going to be added to.
 *		RuleType - rule type (file, network, etc).
 *		Operation - ASCII operation (read, write, etc).
 *		Rule - ASCII rule to parse. NOTE: this field gets clobbered.
 *
 * Returns:
 *		Nothing.
 */

BOOLEAN
PolicyParseObjectRule(PSECURITY_POLICY pSecPolicy, RULE_TYPE RuleType, PCHAR Operation, PCHAR rule)
{
	PCHAR			name = NULL;
	PCHAR			OriginalRule = rule;
	int				i, TotalStars;
	BOOLEAN			wildcard, WildcardWarning = FALSE;
	BOOLEAN			ParseLastToken = FALSE;
	ACTION_TYPE		ActionType;
	MATCH_TYPE		MatchType;
	PPOLICY_RULE	PolicyRule;
	UCHAR			OperationType;
	PCHAR			ObjectName = NULL;
	size_t			ObjectSize = 0;
	UCHAR			RuleNumber;


	LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_VERBOSE, ("PolicyParseObjectRule: Parsing rule '%s': '%s'\n", Operation, rule));


	/*
	 * First token can be "name" or "address" for network rules
	 * Alternatively, the entire rule can consist of an action clause (such as permit, deny, quietdeny or log)
	 */

	switch (VerifyToken2(&rule, "name", "address"))
	{
		/* matched token1 - "name" */
		case 1: break;

		/* matched token2 - "address" */
		case 2:
		{
			/* only network rules can substitute "address" for "name" */

			if (RuleType != RULE_NETWORK)

				ABORT_PolicyParseRule(("Expecting to see 'name'. Got 'address'\n"));

			break;
		}

		/* didn't match "name" or "address". try to parse as an action clause */
		default:
		{
			ParseLastToken = TRUE;
			goto ParseLastToken;
		}
	}


	/*
	 * Second token should be "eq" or "match"
	 */

	switch (VerifyToken2(&rule, "eq", "match"))
	{
		/* matched token1 - "eq" */
		case 1: wildcard = FALSE; break;

		/* matched token2 - "match" */
		case 2: wildcard = TRUE; break;

		/* didn't match "eq" or "match" */
		default: ABORT_PolicyParseRule(("Expecting to see 'eq' or 'match'. Got '%s'\n", rule));
	}


	/*
	 * Third token is the object names in quotes
	 */

	/* parse the object name surrounded by quotes: "<object name>" */

	if (*rule++ != '"')
		ABORT_PolicyParseRule(("Initial quote character not found. Object names should be surrounded by quotes.\n"));


	name = rule;

	TotalStars = i = 0;


	while (*rule != 0 && *rule != '"')
	{
		if (i >= POLICY_MAX_OBJECT_NAME_LENGTH-1)
			ABORT_PolicyParseRule(("Object name '%s' is too long.\nMaximum name length is %d characters.\n", rule - POLICY_MAX_OBJECT_NAME_LENGTH, POLICY_MAX_OBJECT_NAME_LENGTH));

		// fail bad regexes
		if (*rule == '*')
		{
			if (++TotalStars > POLICY_TOTAL_NUMBER_OF_STARS)
				ABORT_PolicyParseRule(("Invalid regular expression. Maximum of %d stars are allowed\n", POLICY_TOTAL_NUMBER_OF_STARS));
		}


		if (wildcard == FALSE && (*rule == '*' || *rule == '?') && WildcardWarning == FALSE)
		{
			LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_WARNING, ("%S:%d:\n", gPolicyFilename, gPolicyLineNumber));
			LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_WARNING, ("Found a regular expression with an 'eq' operator. Use 'match' operator to enable regular expressions.\n"));
			WildcardWarning = TRUE;
		}


		++i;
		++rule;
	}


	if (i == 0 || *rule++ != '"')
		ABORT_PolicyParseRule(("Final quote character not found. Object names should be surrounded by quotes.\n"));

	name[i] = 0;


	if (! IS_WHITESPACE(*rule))
	{
		ABORT_PolicyParseRule(("Expecting to see white space. Got '%s'\n", rule));
	}

	SKIP_WHITESPACE(rule);


	/*
	 * Fourth token is "then"
	 */

	if (VerifyToken2(&rule, "then", "") != 1)
		ABORT_PolicyParseRule(("Expecting to see 'then'. Got '%s'\n", rule));

	/*
	 * Fifth/Last token is "permit", "deny", "quietdeny", "log" or "ask"
	 */

ParseLastToken:

	if (PolicyParseActionClause(rule, &ActionType, &RuleNumber) == FALSE)

		return FALSE;


	/*
	 * Rule parsed ok. Create a new rule.
	 */

	if (RuleType > sizeof(ObjectParseOps) / sizeof(ObjectParseOps[0]))
		ABORT_PolicyParseRule(("Invalid rule type\n"));


	if (name)
	{
		if ((ObjectSize = ObjectParseOps[RuleType].ObjectNameParser(name, &ObjectName, &wildcard)) == INVALID_OBJECT_SIZE)
			ABORT_PolicyParseRule(("Invalid object name '%s'\n", name));

		MatchType = (wildcard == TRUE ? MATCH_WILDCARD : MATCH_SINGLE);
	}
	else
	{
		ObjectSize = 0;
		MatchType = MATCH_ALL;
	}


	if ((OperationType = ObjectParseOps[RuleType].OperationTypeParser(Operation)) == OP_INVALID)
		ABORT_PolicyParseRule(("Invalid operation '%s'\n", Operation));


	/* POLICY_RULE already includes 1 character for the name buffer */

	PolicyRule = ExAllocatePoolWithTag(NonPagedPool, sizeof(POLICY_RULE) + ObjectSize, _POOL_TAG);
	if (PolicyRule == NULL)
	{
		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_WARNING, ("Object policy parser is out of memory\n"));
		return FALSE;
	}

	RtlZeroMemory(PolicyRule, sizeof(POLICY_RULE));

	PolicyRule->ActionType = ActionType;
	PolicyRule->MatchType = MatchType;
	PolicyRule->OperationType = OperationType;

	PolicyRule->RuleNumber = RuleNumber;
	PolicyRule->PolicyLineNumber = gPolicyLineNumber;
	PolicyRule->pSecurityPolicy = pSecPolicy;


	/* ObjectSize can be 0 for MATCH_ALL rules */
	if (ObjectSize)
	{
		PolicyRule->NameLength = (USHORT) ObjectSize;
		strcpy(PolicyRule->Name, (PCHAR) ObjectName);
	}


	/*
	 * Some rules (such as DLL) are actually enforced by different rules (i.e. section).
	 * If LearningMode = TRUE then there is no need to convert
	 */

	if (LearningMode == FALSE)
	{
		/* DLL rules are enforced by section rules. */
		if (RuleType == RULE_DLL)
			RuleType = RULE_SECTION;

		/* Service rules are enforced by registry rules. */
		if (RuleType == RULE_SERVICE)
			RuleType = RULE_REGISTRY;
	}


	InsertPolicyRule(pSecPolicy, PolicyRule, RuleType);


	return TRUE;
}



/*
 * PolicyParseSyscallRule()
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
PolicyParseSyscallRule(PSECURITY_POLICY pSecPolicy, PCHAR SyscallName, PCHAR rule)
{
	PPOLICY_RULE	PolicyRule;
	KIRQL			irql;
	ACTION_TYPE		ActionType;
	ULONG			SyscallNameIndex;
	BOOLEAN			AcceptAll = FALSE;


#if HOOK_SYSCALLS

	LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_VERBOSE, ("PolicyParseSyscallRule: Parsing syscall '%s' rule: '%s'\n", SyscallName, rule));


	/* expecting to see "permit", "deny", "quietdeny" or "log" */
	if (PolicyParseActionClause(rule, &ActionType, NULL) == FALSE)
	{
//		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_DEBUG, ("PolicyParseSyscallRule: PolicyParseActionClause failed\n"));
		return FALSE;
	}

#if 0
	/*
	 * all the special system calls such as OpenFile and CreateProcess have already been hooked, it should
	 * be safe to hook everything else. If a special system call is specified then HookSystemServiceByName
	 * will just silently fail since it's already hooked.
	 */

	if (HookSystemServiceByName(SyscallName, NULL/*USE_DEFAULT_HOOK_FUNCTION*/) == FALSE)
	{
		LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_WARNING, ("Unknown syscall '%s'\n", SyscallName));
		return FALSE;
	}
#endif


	if (strlen(SyscallName) == 3 && _stricmp(SyscallName, "all") == 0)
	{
		AcceptAll = TRUE;
	}
	else
	{
		SyscallNameIndex = FindSystemServiceNumber(SyscallName);
		if (SyscallNameIndex == -1)
		{
			LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_DEBUG, ("PolicyParseSyscallRule: Syscall name '%s' not found\n", SyscallName));
			return FALSE;
		}
	}


	/* allocate enough memory to hold a bit array for all existing system calls */

	if (pSecPolicy->RuleList[ RULE_SYSCALL ] == NULL)
	{
		/*
		 * Take into account 1 ULONG that is already preallocated in POLICY_RULE.
		 */

		USHORT Size = 1 + (USHORT) (ZwCallsNumber - NumberOfBitsInUlong) / 8;


		PolicyRule = ExAllocatePoolWithTag(NonPagedPool, sizeof(POLICY_RULE) + Size, _POOL_TAG);
		if (rule == NULL)
		{
			LOG(LOG_SS_POLICY_PARSER, LOG_PRIORITY_WARNING, ("Policy parser is out of memory\n"));
			return FALSE;
		}

		RtlZeroMemory(PolicyRule, sizeof(POLICY_RULE) + Size);


		/* if default action is permit, then fill the entire bit array with 1's */
		if (pSecPolicy->DefaultPolicyAction == ACTION_PERMIT)
		{
			RtlFillMemory(&PolicyRule->ServiceBitArray, sizeof(PolicyRule->ServiceBitArray) + Size, 0xFF);
		}


		KeAcquireSpinLock(&pSecPolicy->SpinLock, &irql);

		pSecPolicy->RuleList[ RULE_SYSCALL ] = PolicyRule;

		KeReleaseSpinLock(&pSecPolicy->SpinLock, irql);
	}
	else
	{
		PolicyRule = pSecPolicy->RuleList[ RULE_SYSCALL ];
	}


	// syscall_all: permit
	// syscall_all: deny

	// syscall_blah: permit
	// syscall_all: deny

	if (AcceptAll == TRUE)
		;//XXX

	{
		/* Calculate the bit array ULONG we need to modify (bit array consists of a bunch of ULONGs) */
		ULONG	UlongCount = SyscallNameIndex >> UlongBitShift;

		/* Choose the correct bit array ULONG */
		PULONG	BitArray = &PolicyRule->ServiceBitArray[0] + UlongCount;


		//XXX what about log?
		if (ActionType == ACTION_PERMIT)
		{
			/* set the bit */
			*BitArray |= 1 << (SyscallNameIndex - (UlongCount << UlongBitShift));
		}
		else if (ActionType >= ACTION_DENY)
		{
			/* reset the bit */
			*BitArray &= ~( 1 << (SyscallNameIndex - (UlongCount << UlongBitShift)) );
		}
	}

#endif


	return TRUE;
}



/*
 * WildcardMatch()
 *
 * Description:
 *		Simple regex algorithm. Supports '?' for a single character match (does not match EOF) and
 *		'*' for multiple characters (must reside in the same directory).
 *
 *		i.e. c:\temp\*\blah will match c:\temp\temp2\blah but not c:\temp\temp2\temp3\blah
 *
 * NOTE: WildcardMatch() is at least 10x slower than simple _stricmp().
 *
 * Parameters:
 *		path - ASCII pathname.
 *		regex - regular expression to match
 *
 * Returns:
 *		WILDCARD_MATCH (0) in case of a match, WILDCARD_NO_MATCH (1) otherwise.
 */

int
WildcardMatch(PCHAR path, PCHAR regex)
{
	BOOLEAN		MultipleDirectoryMatch = FALSE, SkippedOverWhitespace = FALSE, ShortFileName = FALSE;


	if (path == NULL || regex == NULL)
		return WILDCARD_NO_MATCH;


	while (*regex)
	{
		/*
		 * SPECIAL CASE.
		 * Try to deal with short names (longna~1.txt vs long name.txt).
		 * when we encounter a ~X where X is a digit we skip over it and rewind regex
		 * to either the next '\' or whatever the next path character is.
		 *
		 * The reason for this is when we encounter a long directory it will be abbreviated as follows
		 * c:\Documents and Settings\... -> c:\DOCUME~1\...
		 * Thus by matching "DOCUME" and then skipping over ~1 in the pathname and by rewinding regex
         * until the next '\' we match the two directories.
		 *
		 * The long filenames are matched and abbreviated as follows
		 * c:\longfilename.txt -> c:\longfi~1.txt
		 * Thus we match "longfi", skip over ~1 in the pathname and rewind regex until we match
		 * the next path character which is '.', then we match ".txt"
		 *
		 * The following scheme was mostly designed to deal with "c:\documents and settings" and
		 * "c:\program files" directories since they are two very common long names. It breaks
		 * in a lot of different cases. Consider the following abbreviation list
		 *
		 * LONGNA~1     long name1
		 * LONGNA~2     long name2
		 * LONGNA~3     long name3
		 * LONGNA~4     long name4
		 * LOA926~1     long name5
		 * LOB926~1     long name6
		 *
		 * When more than four names match to the same short name prefix, Windows switches to an
		 * alternative hash based schemes which is not handled by our code.
		 *
		 * Another restriction (there are probably more) has to do with long extensions which are abbreviated
		 * as follows: file.html.text -> filete~1.htm. The following code does not handle this case either.
		 *
		 * All in all, the following code is an ugly hack that is designed to work for most common cases
		 * with a least amount of effort.
		 */

		if (*path == '~' && isdigit(*(path + 1)))
		{
			/* rewind regex until we see '\' or whichever character (ie '.' as in ~1.txt) follows ~X in the path */
			while (*regex && *regex != '\\' && *regex != *(path + 2) /*&& *regex != '*' && *regex != '?'*/)
				++regex;

			/* skip over ~X */
			path += 2;

			ShortFileName = TRUE;
		}


		if (*regex == '*')
		{
			PCHAR	str;

			/*
			 * match one or more characters
			 */

			/* if regular expression ends with '*', automatically match the rest of the pathname */
			if (*(regex + 1) == 0)
				return WILDCARD_MATCH;

			if (*(regex + 1) == '*')
			{
				++regex;
				MultipleDirectoryMatch = TRUE;
			}

			str = path;
			while (*str)
			{
				if (WildcardMatch(str, regex+1) == WILDCARD_MATCH)
					return WILDCARD_MATCH;

				if (MultipleDirectoryMatch == FALSE && *str == '\\')
					break;

				++str;
			}
		}
		else if (*regex == '?')
		{
			/*
			 * match one character
			 */

			if (*path == 0 || *path == '\\')
				return WILDCARD_NO_MATCH;

			++path;
		}
		else if (*regex == '\\')
		{
			/* if we skipped over whitespace but did not match a short name then bail out as not skipping over
			   whitespace would not get us this far anyway */
			if (SkippedOverWhitespace == TRUE && ShortFileName == FALSE)
				return WILDCARD_NO_MATCH;

			ShortFileName = FALSE;
			SkippedOverWhitespace = FALSE;

			/*
			 * match one or more slashes
			 */

			if (*path++ != '\\')
				return WILDCARD_NO_MATCH;

			while (*path == '\\')
				++path;
		}
		else
		{
			/*
			 * match all other characters
			 */

//			if (toupper(*path++) != toupper(*regex))
//				return WILDCARD_NO_MATCH;

			if (toupper(*path) != toupper(*regex))
			{
				/*
				 * Skip over whitespace to match long filenames which are abbreviated with all
				 * whitespace stripped. In order not to match two filenames with different amount
				 * of whitespace, we insert an additional check when we reach '\' once we found out
				 * whether we are working with an abbreviated name.
				 */
				if (*regex == ' ')
					SkippedOverWhitespace = TRUE;
				else
					return WILDCARD_NO_MATCH;
			}
			else
			{
				++path;
			}
		}

		++regex;
	}


	/* make sure pathname is not longer than the regex */
	return *path == 0 ? WILDCARD_MATCH : WILDCARD_NO_MATCH;
}



/*
 * PolicyCheckPolicy()
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

ACTION_TYPE
PolicyCheckPolicy(PSECURITY_POLICY pSecPolicy, RULE_TYPE RuleType, PCHAR Object, UCHAR OperationType, ACTION_TYPE DefaultAction, UCHAR *RuleNumber, PWSTR *PolicyFilename, USHORT *PolicyLineNumber)
{
	PPOLICY_RULE		r;
	ACTION_TYPE			ret = DefaultAction;
	KIRQL				irql;
	size_t				len;


	ASSERT(pSecPolicy);
	ASSERT(PolicyFilename && PolicyLineNumber);
	ASSERT(RuleNumber);


	if (Object)
		len = strlen(Object);

	*PolicyFilename = NULL;
	*PolicyLineNumber = 0;
	*RuleNumber = 0;


	if (pSecPolicy->Initialized == FALSE)
		
		return ACTION_NONE;


	KeAcquireSpinLock(&pSecPolicy->SpinLock, &irql);


	*PolicyFilename = pSecPolicy->Name;


	r = pSecPolicy->RuleList[RuleType];

	while (r)
	{
//XXX at least for registry keys there is no point in comparing the first 10 characters since they will always be \Registry\ ?

		if ( (r->MatchType == MATCH_ALL) ||
			 (r->MatchType == MATCH_SINGLE && len == r->NameLength && _stricmp(Object, r->Name) == 0) ||
			 (r->MatchType == MATCH_WILDCARD && WildcardMatch(Object, r->Name) == WILDCARD_MATCH) )
		{
			if (Object)
				LOG(LOG_SS_POLICY, LOG_PRIORITY_VERBOSE, ("%d PolicyCheckPolicy: matched '%s' (%s) (%x %x %x)\n", CURRENT_PROCESS_PID, Object, r->Name, r->MatchType, r->OperationType, OperationType));
			else
				LOG(LOG_SS_POLICY, LOG_PRIORITY_VERBOSE, ("%d PolicyCheckPolicy: matched RuleType %d MatchType %d OpType %d\n", CURRENT_PROCESS_PID, RuleType, r->MatchType, r->OperationType));


//				if (r->OperationType == OP_APPEND)
//				{
//					ret = ACTION_PROCESS;
//					break;
//				}


			/*
				* (r->OperationType & OperationType) == r->OperationType
				* (r->OperationType & OperationType)
				*
				* policy_default: deny
				* file_read: file.txt
				*
				* del file.txt (opens file for read + write)
				*
				* (read & read_write) = read
				*
				* successfully overwrites the file!
				*/

			/*
				* (r->OperationType & OperationType) == OperationType
				*
				* policy_default: permit
				* file_write: file.txt deny
				* file_read: file.txt
				*
				* del file.txt (opens file for read + write)
				*
				* (write & read_write) != read_write
				*
				* successfully deletes the file!
				*/

			// XXX if we only allow reading but both read+execute are requested, the following
			// if will match! (bad in deny all scenario!)
			if (r->OperationType == OP_ALL || r->OperationType & OperationType)
//				if (r->OperationType == OP_ALL || (r->OperationType & OperationType) == r->OperationType)
//				if (r->OperationType == OP_ALL || (r->OperationType & OperationType) == OperationType)
			{
				ret = r->ActionType;


				/* remember which rule caused this alert */
				*PolicyLineNumber = r->PolicyLineNumber;
				*RuleNumber = r->RuleNumber;


				LOG(LOG_SS_POLICY, LOG_PRIORITY_VERBOSE, ("%d PolicyCheckPolicy: %s access to %d\n",
								CURRENT_PROCESS_PID, ret >= ACTION_DENY ? "deny" : "permit",
								(ULONG) PsGetCurrentProcessId()));

				break;
			}
		}

		r = (PPOLICY_RULE) r->Next;
	}

	KeReleaseSpinLock(&pSecPolicy->SpinLock, irql);


	return ret;
}



/*
 * PolicyCheck()
 *
 * Description:
 *		Verifies whether a specified action (rule (RuleType) + object (arg) ) are allowed
 *		by a global and current process security policies.
 *
 * Parameters:
 *		RuleType - rule type (file rule, registry, etc).
 *		Object - Object name.
 *		OperationType - type of operation carried out (read, write, etc).
 *		RuleNumber - number of the rule that triggered the alert (if it did)
 *		PolicyFilename - name of the policy where the rule, that triggered the alert, was specified
 *		PolicyLineNumber - policy line where the rule, that triggered the alert, was specified
 *
 * Returns:
 *		ACCESS_NONE, ACCESS_PERMIT, ACCESS_DENY or ACCESS_QUIETDENY depending on a security policy.
 */

ACTION_TYPE
PolicyCheck(RULE_TYPE RuleType, PCHAR Object, UCHAR OperationType, UCHAR *RuleNumber, PWSTR *PolicyFilename, USHORT *PolicyLineNumber)
{
	PIMAGE_PID_ENTRY	p, prev;
	PWSTR				GlobalPolicyFilename, ProcessPolicyFilename;
	USHORT				GlobalPolicyLineNumber, ProcessPolicyLineNumber;
	UCHAR				GlobalRuleNumber, ProcessRuleNumber;
	ACTION_TYPE			GlobalAction, ProcessAction = ACTION_PERMIT_DEFAULT, ReturnAction;
	ULONG				ProcessId = CURRENT_PROCESS_PID;


	/* don't mess with kernel initiated calls */

	if (KeGetPreviousMode() != UserMode || KeGetCurrentIrql() != PASSIVE_LEVEL)

		return ACTION_PERMIT;


	/*
	 * As most of system calls PolicyCheck() this is a convinient place to verify
	 * user return address since it needs to be done for all calls
	 */

	VerifyUserReturnAddress();


	/*
	 * First verify against the global security policy
	 */

	GlobalAction = PolicyCheckPolicy(&gSecPolicy, RuleType, Object, OperationType, gSecPolicy.DefaultPolicyAction, &GlobalRuleNumber, &GlobalPolicyFilename, &GlobalPolicyLineNumber);

	/*
	 * Then against process specific policy
	 */

	/* find the process specific policy */

	p = FindImagePidEntry(ProcessId, 0);

	if (p)
	{
		ProcessAction = PolicyCheckPolicy(&p->SecPolicy, RuleType, Object, OperationType, p->SecPolicy.DefaultPolicyAction, &ProcessRuleNumber, &ProcessPolicyFilename, &ProcessPolicyLineNumber);
	}


//	KdPrint(("object %s %d %d action %x %x", Object, RuleType, OperationType, GlobalAction, ProcessAction));

	/*
	 * return the most stringent possible action
	 */

	/* Exception #1: explicit process permit/log overrides general global deny */
	if ((ProcessAction == ACTION_PERMIT || ProcessAction == ACTION_LOG) &&
		(GlobalAction & (ACTION_ASK | ACTION_DENY)))
	{
		*PolicyFilename = ProcessPolicyFilename;
		*PolicyLineNumber = ProcessPolicyLineNumber;
		*RuleNumber = ProcessRuleNumber;
		return ProcessAction;
	}

	/* Exception #2: explicit global permit overrides process default deny */
	if (GlobalAction == ACTION_PERMIT && (ProcessAction & ACTION_DEFAULT))
	{
		*PolicyFilename = GlobalPolicyFilename;
		*PolicyLineNumber = GlobalPolicyLineNumber;
		*RuleNumber = GlobalRuleNumber;
		return GlobalAction;
	}

	if (GlobalAction & ACTION_DENY)
	{ *PolicyFilename = GlobalPolicyFilename; *PolicyLineNumber = GlobalPolicyLineNumber; *RuleNumber = GlobalRuleNumber; ReturnAction = GlobalAction; goto done; }

	if (ProcessAction & ACTION_DENY)
	{ *PolicyFilename = ProcessPolicyFilename; *PolicyLineNumber = ProcessPolicyLineNumber; *RuleNumber = ProcessRuleNumber; ReturnAction = ProcessAction; goto done; }


	if (GlobalAction & ACTION_LOG)
	{ *PolicyFilename = GlobalPolicyFilename; *PolicyLineNumber = GlobalPolicyLineNumber; *RuleNumber = GlobalRuleNumber; ReturnAction = GlobalAction; goto done; }

	if (ProcessAction & ACTION_LOG)
	{ *PolicyFilename = ProcessPolicyFilename; *PolicyLineNumber = ProcessPolicyLineNumber; *RuleNumber = ProcessRuleNumber; ReturnAction = ProcessAction; goto done; }


	if (GlobalAction & ACTION_ASK)
	{ *PolicyFilename = GlobalPolicyFilename; *PolicyLineNumber = GlobalPolicyLineNumber; *RuleNumber = GlobalRuleNumber; ReturnAction = GlobalAction; goto done; }

	if (ProcessAction & ACTION_ASK)
	{ *PolicyFilename = ProcessPolicyFilename; *PolicyLineNumber = ProcessPolicyLineNumber; *RuleNumber = ProcessRuleNumber; ReturnAction = ProcessAction; goto done; }


	if (GlobalAction & ACTION_PERMIT)
	{ *PolicyFilename = GlobalPolicyFilename; *PolicyLineNumber = GlobalPolicyLineNumber; *RuleNumber = GlobalRuleNumber; ReturnAction = GlobalAction; goto done; }

	if (ProcessAction & ACTION_PERMIT)
	{ *PolicyFilename = ProcessPolicyFilename; *PolicyLineNumber = ProcessPolicyLineNumber; *RuleNumber = ProcessRuleNumber; ReturnAction = ProcessAction; goto done; }


	LOG(LOG_SS_POLICY, LOG_PRIORITY_DEBUG, ("object %s %d %d action %x %x\n", Object, RuleType, OperationType, GlobalAction, ProcessAction));

	*PolicyFilename = NULL;
	*PolicyLineNumber = 0;


	return ProcessAction;


done:

	/*
 	 * if we are booting up then don't deny any system calls to prevent a machine
	 * from not booting up
	 */

	if (BootingUp == TRUE && (ReturnAction == ACTION_ASK || (ReturnAction & ACTION_DENY)))
		ReturnAction = ACTION_LOG;

	return ReturnAction;
}
