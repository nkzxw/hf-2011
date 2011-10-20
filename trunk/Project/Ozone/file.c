/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		file.c
 *
 * Abstract:
 *
 *		This module implements various file hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 19-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include "file.h"
#include "policy.h"
#include "pathproc.h"
#include "hookproc.h"
#include "accessmask.h"
#include "learn.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitFileHooks)
#endif


fpZwCreateFile				OriginalNtCreateFile = NULL;
fpZwOpenFile				OriginalNtOpenFile = NULL;
fpZwDeleteFile				OriginalNtDeleteFile = NULL;
fpZwQueryAttributesFile		OriginalNtQueryAttributesFile = NULL;
fpZwQueryFullAttributesFile	OriginalNtQueryFullAttributesFile = NULL;
fpZwQueryDirectoryFile		OriginalNtQueryDirectoryFile = NULL;
fpZwSetInformationFile		OriginalNtSetInformationFile = NULL;

fpZwCreateMailslotFile		OriginalNtCreateMailslotFile = NULL;
fpZwCreateNamedPipeFile		OriginalNtCreateNamedPipeFile = NULL;



// XXX make sure that this still works with POSIX subsystem (inside windows 2000 describes how to start posix subsystem)

// XXX make sure streams don't screw anything up... do a search on a directory, observe NtCreateFile output..


/*
 * HookedNtCreateFile()
 *
 * Description:
 *		This function mediates the NtCreateFile() system service and checks the
 *		provided file name against the global and current process security policies.
 *
 *		NOTE: ZwCreateFile() creates or opens a file. [NAR]
 *
 * Parameters:
 *		Those of NtCreateFile().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateFile().
 */

NTSTATUS
NTAPI
HookedNtCreateFile
(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PLARGE_INTEGER AllocationSize OPTIONAL,
	IN ULONG FileAttributes,
	IN ULONG ShareAccess,
	IN ULONG CreateDisposition,
	IN ULONG CreateOptions,
	IN PVOID EaBuffer OPTIONAL,
	IN ULONG EaLength
)
{
	PCHAR		FunctionName = "HookedNtCreateFile";
	CHAR		BufferLongName[MAX_PATH], BufferShortName[MAX_PATH];
	PCHAR		FILENAME = BufferLongName;//BufferShortName;
	PCHAR		DIRECTORYNAME = BufferLongName;//BufferShortName;
	BOOLEAN		CreateDirectoryRequest = FALSE;


	HOOK_ROUTINE_ENTER();


	/* special handling for directories, look at flags to figure out whether we are dealing w/a directory */
	if ((CreateOptions & FILE_DIRECTORY_FILE) && (CreateDisposition & FILE_CREATE))
		CreateDirectoryRequest = TRUE;


	if (LearningMode == FALSE)
	{
		GetPathFromOA(ObjectAttributes, BufferLongName, MAX_PATH, RESOLVE_LINKS);

//		ConvertLongFileNameToShort(BufferLongName, BufferShortName, MAX_PATH);
//KdPrint(("%s\n%s\n", BufferLongName, BufferShortName));

		if (CreateDirectoryRequest == TRUE)
		{
			POLICY_CHECK_OPTYPE(DIRECTORY, OP_DIR_CREATE);
		}
		else
		{
			POLICY_CHECK_OPTYPE(FILE, Get_FILE_OperationType(DesiredAccess));
		}
	}

//XXX if resolved name's first character is not '\' then allow? to allow names such as IDE#CdRomNECVMWar_VMware..


/*
XXX
investigate

The FileId can be used to open the file, when the FILE_OPEN_BY_FILE_ID
CreateOption is specified in a call to ZwCreateFile.

whether this can be used to bypass name checking mechanism
*/
	if (CreateOptions & FILE_OPEN_BY_FILE_ID)
	{
		LOG(LOG_SS_FILE, LOG_PRIORITY_WARNING, ("%d HookedNtCreateFile: FILE_OPEN_BY_FILE_ID set\n", (ULONG) PsGetCurrentProcessId()));

		HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
	}


	ASSERT(OriginalNtCreateFile);

	rc = OriginalNtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
								AllocationSize, FileAttributes, ShareAccess, CreateDisposition,
								CreateOptions, EaBuffer, EaLength);


	if (CreateDirectoryRequest == TRUE)
	{
		HOOK_ROUTINE_FINISH_OPTYPE(DIRECTORY, OP_DIR_CREATE);
	}
	else
	{
		HOOK_ROUTINE_FINISH(FILE);
	}
}



/*
 * HookedNtOpenFile()
 *
 * Description:
 *		This function mediates the NtOpenFile() system service and checks the
 *		provided file name against the global and current process security policies.
 *
 *		NOTE: ZwOpenFile() opens a file. [NAR]
 *
 * Parameters:
 *		Those of NtOpenFile().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtOpenFile().
 */

NTSTATUS
NTAPI
HookedNtOpenFile
(
	OUT PHANDLE  FileHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_ATTRIBUTES  ObjectAttributes,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	IN ULONG  ShareAccess,
	IN ULONG  OpenOptions
)
{
	PCHAR		FunctionName = "HookedNtOpenFile";
//	HOOK_ROUTINE_START(FILE);

	CHAR		BufferLongName[MAX_PATH], BufferShortName[MAX_PATH];
	PCHAR		FILENAME = BufferLongName;//BufferShortName;


	HOOK_ROUTINE_ENTER();


	if (LearningMode == FALSE)
	{
		GetPathFromOA(ObjectAttributes, BufferLongName, MAX_PATH, RESOLVE_LINKS);

//		ConvertLongFileNameToShort(BufferLongName, BufferShortName, MAX_PATH);
//KdPrint(("%s\n%s\n", BufferLongName, BufferShortName));

		POLICY_CHECK_OPTYPE(FILE, Get_FILE_OperationType(DesiredAccess));
	}


	ASSERT(OriginalNtOpenFile);

	rc = OriginalNtOpenFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
							ShareAccess, OpenOptions);


	HOOK_ROUTINE_FINISH(FILE);
}



/*
 * HookedNtDeleteFile()
 *
 * Description:
 *		This function mediates the NtDeleteFile() system service and checks the
 *		provided file name against the global and current process security policies.
 *
 *		NOTE: ZwDeleteFile deletes a file. [NAR]
 *
 * Parameters:
 *		Those of NtDeleteFile().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtDeleteFile().
 */

NTSTATUS
NTAPI
HookedNtDeleteFile
(
	IN POBJECT_ATTRIBUTES ObjectAttributes
)
{
	PCHAR		FunctionName = "HookedNtDeleteFile";


	HOOK_ROUTINE_START_OPTYPE(FILE, OP_DELETE);


	ASSERT(OriginalNtDeleteFile);

	rc = OriginalNtDeleteFile(ObjectAttributes);


	HOOK_ROUTINE_FINISH_OPTYPE(FILE, OP_DELETE);
}



/*
 * HookedNtQueryAttributesFile()
 *
 * Description:
 *		This function mediates the NtQueryAttributesFile() system service and checks the
 *		provided file name against the global and current process security policies.
 *
 *		NOTE: ZwQueryAttributesFile retrieves basic information about a file object. [NAR]
 *
 * Parameters:
 *		Those of NtQueryAttributesFile().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtQueryAttributesFile().
 */

NTSTATUS
NTAPI
HookedNtQueryAttributesFile
(
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PFILE_BASIC_INFORMATION FileInformation
)
{
	PCHAR		FunctionName = "HookedNtQueryAttributesFile";


	HOOK_ROUTINE_START_OPTYPE(FILE, OP_READ);


	ASSERT(OriginalNtQueryAttributesFile);

	rc = OriginalNtQueryAttributesFile(ObjectAttributes, FileInformation);


	HOOK_ROUTINE_FINISH_OPTYPE(FILE, OP_READ);
}



/*
 * HookedNtQueryFullAttributesFile()
 *
 * Description:
 *		This function mediates the NtQueryFullAttributesFile() system service and checks the
 *		provided file name against the global and current process security policies.
 *
 *		NOTE: ZwQueryFullAttributesFile retrieves extended information about a file object. [NAR]
 *
 * Parameters:
 *		Those of NtQueryFullAttributesFile().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtQueryFullAttributesFile().
 */

NTSTATUS
NTAPI
HookedNtQueryFullAttributesFile
(
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
)
{
	PCHAR		FunctionName = "HookedNtQueryFullAttributesFile";


	HOOK_ROUTINE_START_OPTYPE(FILE, OP_READ);


	ASSERT(OriginalNtQueryFullAttributesFile);

	rc = OriginalNtQueryFullAttributesFile(ObjectAttributes, FileInformation);


	HOOK_ROUTINE_FINISH_OPTYPE(FILE, OP_READ);
}



/*
 * HookedNtQueryDirectoryFile()
 *
 * Description:
 *		This function mediates the NtQueryDirectoryFile() system service and checks the
 *		provided file name against the global and current process security policies.
 *
 *		NOTE: ZwQueryDirectoryFile retrieves information about the contents of a directory. [NAR]
 *
 * Parameters:
 *		Those of NtQueryDirectoryFile().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtQueryDirectoryFile().
 */

NTSTATUS
NTAPI
HookedNtQueryDirectoryFile
(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG FileInformationLength,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	IN BOOLEAN ReturnSingleEntry,
	IN PUNICODE_STRING FileName OPTIONAL,
	IN BOOLEAN RestartScan
)
{
	PCHAR			FunctionName = "HookedNtQueryDirectoryFile";
	UNICODE_STRING	usInputFileName;
	CHAR			FILENAME[MAX_PATH];
	ANSI_STRING		asFileName;


	HOOK_ROUTINE_ENTER();


	if (ARGUMENT_PRESENT(FileName))
	{
		if (!VerifyUnicodeString(FileName, &usInputFileName))
		{
			LOG(LOG_SS_FILE, LOG_PRIORITY_DEBUG, ("HookedNtQueryDirectoryFile: VerifyUnicodeString failed\n"));
			HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
		}


		_snprintf(FILENAME, MAX_PATH, "%S", usInputFileName.Buffer);
		FILENAME[ MAX_PATH - 1 ] = 0;

		LOG(LOG_SS_FILE, LOG_PRIORITY_DEBUG, ("HookedNtQueryDirectoryFile: %s\n", FILENAME));
	}


	if (LearningMode == FALSE)
	{
		//XXX
//		POLICY_CHECK_OPTYPE(FILE, OP_READ);
	}


	ASSERT(OriginalNtQueryDirectoryFile);

	rc = OriginalNtQueryDirectoryFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock,
										FileInformation, FileInformationLength, FileInformationClass,
										ReturnSingleEntry, FileName, RestartScan);


//	HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(FILE, FILENAME, OP_READ);
	HOOK_ROUTINE_EXIT(rc);
}



/*
 * HookedNtSetInformationFile()
 *
 * Description:
 *		This function mediates the NtSetInformationFile() system service and checks the
 *		provided file name against the global and current process security policies.
 *
 *		NOTE: ZwSetInformationFile sets information affecting a file object. [NAR]
 *
 * Parameters:
 *		Those of NtSetInformationFile().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtSetInformationFile().
 */

NTSTATUS
NTAPI
HookedNtSetInformationFile
(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID FileInformation,
	IN ULONG FileInformationLength,
	IN FILE_INFORMATION_CLASS FileInformationClass
)
{
	PCHAR			FunctionName = "HookedNtSetInformationFile";
	CHAR			FILENAME[MAX_PATH];
	WCHAR			FILENAMEW[MAX_PATH];
	PWSTR			FileName = NULL;
	UCHAR			Operation = OP_READ;


	HOOK_ROUTINE_ENTER();


	/* FileDispositionInformation is used to delete files */
	if (FileInformationClass == FileDispositionInformation)
		Operation = OP_DELETE;


	if ((FileName = GetNameFromHandle(FileHandle, FILENAMEW, sizeof(FILENAMEW))) != NULL)
	{
		sprintf(FILENAME, "%S", FileName);

		LOG(LOG_SS_FILE, LOG_PRIORITY_VERBOSE, ("%d %s: %s\n", (ULONG) PsGetCurrentProcessId(), FunctionName, FILENAME));

		if (LearningMode == FALSE)
		{
			POLICY_CHECK_OPTYPE_NAME(FILE, Operation);
		}
	}


	ASSERT(OriginalNtSetInformationFile);

	rc = OriginalNtSetInformationFile(FileHandle, IoStatusBlock, FileInformation, FileInformationLength, FileInformationClass);


	HOOK_ROUTINE_FINISH_OBJECTNAME_OPTYPE(FILE, FileName, Operation);
}



/*
 * HookedNtCreateNamedPipeFile()
 *
 * Description:
 *		This function mediates the NtCreateNamedPipeFile() system service and checks the
 *		provided named pipe name against the global and current process security policies.
 *
 *		NOTE: ZwCreateNamedPipeFile creates a named pipe. [NAR]
 *
 * Parameters:
 *		Those of NtCreateNamedPipeFile().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateNamedPipeFile().
 */

NTSTATUS
NTAPI
HookedNtCreateNamedPipeFile
(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG ShareAccess,
	IN ULONG CreateDisposition,
	IN ULONG CreateOptions,
	IN ULONG TypeMessage,
	IN ULONG ReadmodeMessage,
	IN ULONG Nonblocking,
	IN ULONG MaxInstances,
	IN ULONG InBufferSize,
	IN ULONG OutBufferSize,
	IN PLARGE_INTEGER DefaultTimeout OPTIONAL
)
{
	PCHAR	FunctionName = "HookedNtCreateNamedPipeFile";


	HOOK_ROUTINE_START(NAMEDPIPE);


	ASSERT(OriginalNtCreateNamedPipeFile);

	rc = OriginalNtCreateNamedPipeFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
										ShareAccess, CreateDisposition, CreateOptions, TypeMessage,
										ReadmodeMessage, Nonblocking, MaxInstances, InBufferSize,
										OutBufferSize, DefaultTimeout);


	HOOK_ROUTINE_FINISH(NAMEDPIPE);
}



/*
 * HookedNtCreateMailslotFile()
 *
 * Description:
 *		This function mediates the NtCreateMailslotFile() system service and checks the
 *		provided mailslot name against the global and current process security policies.
 *
 *		NOTE: ZwCreateMailslotFile creates a mailslot. [NAR]
 *
 * Parameters:
 *		Those of NtCreateMailslotFile().
 *
 * Returns:
 *		STATUS_ACCESS_DENIED if the call does not pass the security policy check.
 *		Otherwise, NTSTATUS returned by NtCreateMailslotFile().
 */

NTSTATUS
NTAPI
HookedNtCreateMailslotFile
(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CreateOptions,
	IN ULONG InBufferSize,
	IN ULONG MaxMessageSize,
	IN PLARGE_INTEGER ReadTimeout OPTIONAL
)
{
	PCHAR	FunctionName = "HookedNtCreateMailslotFile";


	HOOK_ROUTINE_START(MAILSLOT);


	ASSERT(OriginalNtCreateMailslotFile);

	rc = OriginalNtCreateMailslotFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
										CreateOptions, InBufferSize, MaxMessageSize, ReadTimeout);


	HOOK_ROUTINE_FINISH(MAILSLOT);
}



/*
 * InitFileHooks()
 *
 * Description:
 *		Initializes all the mediated file operation pointers. The "OriginalFunction" pointers
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
InitFileHooks()
{
	if ( (OriginalNtCreateFile = (fpZwCreateFile) ZwCalls[ZW_CREATE_FILE_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_FILE, LOG_PRIORITY_DEBUG, ("InitFileHooks: OriginalNtCreateFile is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtOpenFile = (fpZwOpenFile) ZwCalls[ZW_OPEN_FILE_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_FILE, LOG_PRIORITY_DEBUG, ("InitFileHooks: OriginalNtOpenFile is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtDeleteFile = (fpZwDeleteFile) ZwCalls[ZW_DELETE_FILE_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_FILE, LOG_PRIORITY_DEBUG, ("InitFileHooks: OriginalNtDeleteFile is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtQueryAttributesFile = (fpZwQueryAttributesFile) ZwCalls[ZW_QUERY_ATTRIBUTES_FILE_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_FILE, LOG_PRIORITY_DEBUG, ("InitFileHooks: OriginalNtQueryAttributesFile is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtQueryFullAttributesFile = (fpZwQueryFullAttributesFile) ZwCalls[ZW_QUERY_FULLATTR_FILE_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_FILE, LOG_PRIORITY_DEBUG, ("InitFileHooks: OriginalNtQueryFullAttributesFile is NULL\n"));
		return FALSE;
	}
/*
	if ( (OriginalNtQueryDirectoryFile = (fpZwQueryDirectoryFile) ZwCalls[ZW_QUERY_DIRECTORYFILE_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_FILE, LOG_PRIORITY_DEBUG, ("InitFileHooks: OriginalNtQueryDirectoryFile is NULL\n"));
		return FALSE;
	}
*/
	if ( (OriginalNtSetInformationFile = (fpZwSetInformationFile) ZwCalls[ZW_SET_INFO_FILE_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_FILE, LOG_PRIORITY_DEBUG, ("InitFileHooks: OriginalNtSetInformationFile is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtCreateNamedPipeFile = (fpZwCreateNamedPipeFile) ZwCalls[ZW_CREATE_NAMEDPIPEFILE_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_FILE, LOG_PRIORITY_DEBUG, ("InitFileHooks: OriginalNtCreateNamedPipeFile is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtCreateMailslotFile = (fpZwCreateMailslotFile) ZwCalls[ZW_CREATE_MAILSLOTFILE_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_FILE, LOG_PRIORITY_DEBUG, ("InitFileHooks: OriginalNtCreateMailslotFile is NULL\n"));
		return FALSE;
	}

	return TRUE;
}
