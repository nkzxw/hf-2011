/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		procname.c
 *
 * Abstract:
 *
 *		This module defines various types used by process id to process name conversion routines.
 *
 *		All processes are tracked in a global hash table.
 *		At startup ZwQuerySystemInformation(SystemProcessesAndThreadsInformation..) is used to
 *		enumerate all the existing processes. After that NtCreateProcess() is hooked and used
 *		to keep track of newly created processes while PsSetCreateProcessNotifyRoutine()
 *		callbacks are used to keep track of terminating processes.
 *
 *		See http://www.microsoft.com/msj/0199/nerd/nerd0199.aspx for more info.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 23-Feb-2004
 *
 * Revision History:
 *
 *		07-Apr-2004 ET - Copied from process.h
 */


#include <NTDDK.h>
#include "procname.h"
#include "hookproc.h"
#include "process.h"
#include "policy.h"
#include "sysinfo.h"
#include "learn.h"
#include "misc.h"
#include "log.h"


void	FindProcessNameOffset();


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitProcessNameEntries)
#pragma alloc_text (INIT, FindProcessNameOffset)
#pragma alloc_text (INIT, EnumerateExistingProcesses)
#pragma alloc_text (PAGE, RemoveProcessNameEntries)
#endif


ULONG			SystemProcessId;

/* 67 * 144 = 10 kilobytes */
IMAGE_PID_ENTRY	gImagePidHtbl[IMAGE_PID_HASHTABLE_SIZE];

//XXX investigate KeAcquireInStackQueuedSpinLock
KSPIN_LOCK		gImagePidHtblSpinLock;


/*
 * FindImagePidEntry()
 *
 * Description:
 *		Looks for a process entry in the global process hash table with a specified process id.
 *
 * Parameters:
 *		ProcessId - process id of the process to look for.
 *
 * Returns:
 *		Pointer to a process entry if one is found, NULL otherwise.
 */

PIMAGE_PID_ENTRY
FindImagePidEntry(ULONG ProcessId, ULONG ParentId)
{
	PIMAGE_PID_ENTRY	p;
	KIRQL				irql;


//LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("FindImagePidEntry(%d %d) 1\n", ProcessId, ParentId));
	KeAcquireSpinLock(&gImagePidHtblSpinLock, &irql);
//LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("FindImagePidEntry 2\n"));


	p = gImagePidHtbl[(ULONG) ProcessId % IMAGE_PID_HASHTABLE_SIZE].next;

	while (p)
	{
		if (p->ProcessId == ProcessId)
		{
			if (ParentId == 0 || p->ParentId == ParentId)
			{
				if (ParentId != 0)
				{
					LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d FindImagePidEntry(%d, %d) found an entry (%d)\n", CURRENT_PROCESS_PID, ProcessId, ParentId, p->ParentId));
				}

				KeReleaseSpinLock(&gImagePidHtblSpinLock, irql);
				return p;
			}

			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d FindImagePidEntry: ProcessId %d clash. Parent id %d vs %d\n", CURRENT_PROCESS_PID, ProcessId, ParentId, p->ParentId));
		}

		p = p->next;
	}


	KeReleaseSpinLock(&gImagePidHtblSpinLock, irql);


	return NULL;
}



/*
 * ProcessInsertImagePidEntry()
 *
 * Description:
 *		Insert the new process entry into the global process hash table.
 *
 * Parameters:
 *		ProcessId - process id of the new process.
 *		NewProcessEntry - the entry to insert into the hash table.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
ProcessInsertImagePidEntry(ULONG ProcessId, PIMAGE_PID_ENTRY NewProcessEntry)
{
	PIMAGE_PID_ENTRY	p, prev;
	KIRQL				irql;


//LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("ProcessInsertImagePidEntry(%d %x) 1\n", ProcessId, NewProcessEntry));
	KeAcquireSpinLock(&gImagePidHtblSpinLock, &irql);
//LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("ProcessInsertImagePidEntry 2\n"));

	prev = &gImagePidHtbl[(ULONG) ProcessId % IMAGE_PID_HASHTABLE_SIZE];
	p = prev->next;

	while (p)
	{
		// if an entry with our ProcessId already exists, bail out

		if (p->ProcessId == (ULONG) ProcessId)
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d ProcessInsertImagePidEntry: ProcessId (%d) clash. New image name is '%S' (%d). Old image name is '%S' (%d)\n", CURRENT_PROCESS_PID, ProcessId, NewProcessEntry->ImageName, NewProcessEntry->ParentId, p->ImageName, p->ParentId));

			if (ProcessId != 0)
			{
				KeReleaseSpinLock(&gImagePidHtblSpinLock, irql);
				return FALSE;
			}
		}

		prev = p;
		p = p->next;
	}

	prev->next = NewProcessEntry;


	KeReleaseSpinLock(&gImagePidHtblSpinLock, irql);


	return TRUE;
}



/*
 * CreateNewProcessEntry()
 *
 * Description:
 *		Allocates and initializes a new process entry.
 *
 * Parameters:
 *		ProcessId - process id of the new process.
 *		ProcessName - process image name.
 *
 * Returns:
 *		Pointer to a heap allocated IMAGE_PID_ENTRY structure. NULL if failed.
 */

PIMAGE_PID_ENTRY
CreateNewProcessEntry(ULONG ProcessId, ULONG ParentId, PUNICODE_STRING ProcessName, BOOLEAN NewProcess)
{
	PIMAGE_PID_ENTRY	ProcessEntry;


	ASSERT(ProcessName);


	ProcessEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof(IMAGE_PID_ENTRY) + ProcessName->Length, _POOL_TAG);
	if (ProcessEntry == NULL)
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("CreateNewProcessEntry: Out of memory. Forgeting about process %d (%S)", ProcessId, ProcessName->Buffer));
		return NULL;
	}

	RtlZeroMemory(ProcessEntry, sizeof(IMAGE_PID_ENTRY));

	ProcessEntry->ProcessId = ProcessId;
	ProcessEntry->ParentId = ParentId;
	ProcessEntry->FirstThread = NewProcess;

	KeInitializeEvent(&ProcessEntry->UserlandRequestDoneEvent, SynchronizationEvent, FALSE);

	wcscpy(ProcessEntry->ImageName, ProcessName->Buffer);
	ProcessEntry->ImageName[ ProcessName->Length/sizeof(WCHAR) ] = L'\0';

	KeInitializeSpinLock(&ProcessEntry->SecPolicy.SpinLock);


	return ProcessEntry;
}



/*
 * CreateAndLoadNewProcessEntry()
 *
 * Description:
 *		Creates a new process entry and inserts it into the global process hash table.
 *
 * Parameters:
 *		ProcessId - process id of the new process.
 *		ProcessName - process image name.
 *
 * Returns:
 *		Pointer to a heap allocated IMAGE_PID_ENTRY structure. NULL if failed.
 */

PIMAGE_PID_ENTRY
CreateAndLoadNewProcessEntry(ULONG ProcessId, PUNICODE_STRING ProcessName, BOOLEAN NewProcess)
{
	PIMAGE_PID_ENTRY	ProcessEntry;


	ASSERT(ProcessName);


	ProcessEntry = CreateNewProcessEntry(ProcessId, 0, ProcessName, NewProcess);
	if (ProcessEntry == NULL)
		return NULL;


	if (LearningMode == FALSE && FindAndLoadSecurityPolicy(&ProcessEntry->SecPolicy, ProcessName->Buffer, NULL) == FALSE)
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_VERBOSE, ("CreateAndLoadNewProcessEntry: Failed to load security policy for %S\n", ProcessName->Buffer));
	}


	if (ProcessInsertImagePidEntry(ProcessId, ProcessEntry) == FALSE)
	{
		ExFreePoolWithTag(ProcessEntry, _POOL_TAG);
		return NULL;
	}


	return ProcessEntry;
}



/*
 * CreateProcessNotifyProc()
 *
 * Description:
 *		PsSetCreateProcessNotifyRoutine() callback. Used to remove process entries from the global
 *		hashtable of currently running processes. On Windows 2000, this routine also replaces PIDs
 *		of 0 with the real PIDs. This is necessary since win2k does not assign PIDs at a point
 *		where process.c!PostProcessNtCreateProcess() is called.
 *
 *		NOTE: PsSetCreateProcessNotifyRoutine() adds a driver-supplied callback routine to,
 *		or removes it from, a list of routines to be called whenever a process is created or deleted.
 *
 * Parameters:
 *		ParentId - the parent process ID.
 *		ProcessId - process ID that was created / deleted.
 *		Create - indicates whether the process was created (TRUE) or deleted (FALSE).
 *
 * Returns:
 *		Nothing.
 */

VOID
CreateProcessNotifyProc
(
    IN HANDLE  ParentId,
    IN HANDLE  ProcessId,
    IN BOOLEAN  Create
)
{
	PIMAGE_PID_ENTRY	p, prev, tmp;
	ULONG				RequiredPid;
	BOOLEAN				FoundNewProcess = FALSE;
	KIRQL				irql;


	LOG(LOG_SS_PROCESS, LOG_PRIORITY_VERBOSE, ("%d CreateProccessNotifyProc(%d, %d, %d)\n", CURRENT_PROCESS_PID, ParentId, ProcessId, Create));


	/* if the entry is being removed look for ProcessId, otherwise look for 0 */
	RequiredPid = Create == FALSE ? (ULONG) ProcessId : 0;


	/*
	 * find an entry with pid=ProcessId
	 */

	KeAcquireSpinLock(&gImagePidHtblSpinLock, &irql);

	prev = &gImagePidHtbl[(ULONG) RequiredPid % IMAGE_PID_HASHTABLE_SIZE];
	p = prev->next;

	while (p)
	{
		if (p->ProcessId != (ULONG) RequiredPid)
		{
			prev = p;
			p = p->next;
			continue;
		}

		if (p->ParentId != 0 && p->ParentId != (ULONG) ParentId)
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d CreateProccessNotifyProc(): ProcessId %d collision. %d vs %d\n", CURRENT_PROCESS_PID, p->ProcessId, p->ParentId, ParentId));

			prev = p;
			p = p->next;
			continue;
		}


		/* found the necessary entry */
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d CreateProccessNotifyProc(): Found (%s) %d %x %S\n", CURRENT_PROCESS_PID, Create == FALSE ? "delete" : "fix", p->ProcessId, p->next, p->ImageName));

		tmp = p;

		/* unlink the found entry */
		prev->next = p->next;

		if (Create == FALSE)
		{
			/* if the process is being deleted then free the entry */
			PolicyDelete(&tmp->SecPolicy);
			ExFreePoolWithTag(tmp, _POOL_TAG);
		}
		else
		{
			/*
			 * if the process is being executed and we found an entry with PID of 0 then
			 * replace 0 with the real pid
			 */
			tmp->ProcessId = (ULONG) ProcessId;
			tmp->next = NULL;
			FoundNewProcess = TRUE;
		}

		break;
	}

	KeReleaseSpinLock(&gImagePidHtblSpinLock, irql);


	/*
	 * If necessary, reinsert the new entry that used to have a PID of 0.
	 * We need to reinsert since the hashtable is indexed by PIDs.
	 */
	if (FoundNewProcess == TRUE)
	{
//XXX at this point no locks are held. if tmp->ProcessId process quits before ProcessInsertImagePidEntry()
		// executes, we will get a zombie entry (will never be removed)
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d Replacing 0 with %d for %S\n", CURRENT_PROCESS_PID, tmp->ProcessId, tmp->ImageName));
		ProcessInsertImagePidEntry(tmp->ProcessId, tmp);
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d Done replacing %d\n", CURRENT_PROCESS_PID, tmp->ProcessId, tmp->ImageName));
	}
}



USHORT ProcessNameOffset = 0, ThreadServiceTableOffset = 0;


/*
 * FindProcessNameOffset()
 *
 * Description:
 *		Identifies process name offset in the EPROCESS structure.
 *
 *		The name offset is identified by searching for "System" string in the System EPROCESS structure
 *		(this function is called when the driver is loaded and thus runs in the "System" context).
 *
 *		ThreadServiceTableOffset is the offset of the pointer to a service descriptor table (syscall table)
 *		in the Thread Environment Block (TEB). Used to determine whether a thread is GUI based or not.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Nothing.
 */

void
FindProcessNameOffset()
{
	PEPROCESS	SystemProcess = PsGetCurrentProcess();		// current "System" process
	PETHREAD	SystemThread = PsGetCurrentThread();		// current "System" thread
	USHORT		i;


	/* Search for "System" process name in the current system process Process Environment Block */

	for (i = 0; i < 1024; i++)	// 372 on Windows XP SP1
	{
		if (_strnicmp((PCHAR) SystemProcess + i, "System", 6) == 0)
		{
			ProcessNameOffset = i;
			break;
		}
	}


	/* Search for KeServiceDescriptorTable address in the current system thread's Thread Environment Block */

	for (i = 0; i < 500; i++)	// 292 on Win2k3
	{
		if (* (PULONG) ((PCHAR) SystemThread + i) == (ULONG) &KeServiceDescriptorTable[0])
		{
			ThreadServiceTableOffset = i;
			break;
		}
	}
}



/*
 * GetCurrentProcessName()
 *
 * Description:
 *		Returns the current process pathname.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Pointer to the current process pathname ("Unknown" if not found).
 */

PWCHAR
GetCurrentProcessName()
{
	PIMAGE_PID_ENTRY	p;
	PWCHAR				name = NULL;


	p = FindImagePidEntry(CURRENT_PROCESS_PID, 0);

	if (p != NULL)
		/*
		 * NOTE: we are returning a pointer to a member of a structure which is not locked at this point!
		 * Should be ok though since this function is only called in the context of a current process which
		 * cannot disappear from underneath us.
		 */
		name = p->ImageName;
	else
		name = L"(Unknown)";


	return name;
}



/*
 * FindExistingProcesses()
 *
 * Description:
 *		Enumerates all the existing processes using ZwQuerySystemInformation(SystemProcessesAndThreadsInformation)
 *		and creates IMAGE_PID_ENTRY for all of them. Called once at startup.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Nothing.
 */

VOID
EnumerateExistingProcesses()
{
	ULONG					size = 65535, i;
	PUCHAR					SystemInfo;
	PSYSTEM_PROCESSES		pSystemProcess;
	NTSTATUS				status;


	/*
	 * The format of the data returned to the SystemInformation buffer is a sequence of
	 * SYSTEM_PROCESSES structures, chained together via the NextEntryDelta member.
	 * The Threads member of each SYSTEM_PROCESSES structure is an array of ThreadCount
	 * SYSTEM_THREADS structures.The end of the process chain is marked by a NextEntryDelta
	 * value of zero.
	 */

	/* first, find out the total amount of SystemProcessesAndThreadsInformation to be returned */
/*
	status = ZwQuerySystemInformation(SystemProcessesAndThreadsInformation, &i, 0, &size);
	if (size == 0 || size > 1024*1024)
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("FindExistingProcesses: ZwQuerySystemInformation failed. status=%x size=%d\n", status, size));
		return;
	}
*/

	/* second, allocate the required amount of memory */

	SystemInfo = ExAllocatePoolWithTag(PagedPool, size, _POOL_TAG);
	if (SystemInfo == NULL)
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("FindExistingProcesses: out of memory (requested %d bytes)\n", size));
		return;
	}

	/* third, request the SystemProcessesAndThreadsInformation */

	ZwQuerySystemInformation(SystemProcessesAndThreadsInformation, SystemInfo, size, &i);


	pSystemProcess = (PSYSTEM_PROCESSES) SystemInfo;


	i = 0;

	while (pSystemProcess->NextEntryDelta != 0)
	{
		if (pSystemProcess->ProcessName.Length != 0)
		{
			/* create a new process entry and load the associated security policy (if any) */

			CreateAndLoadNewProcessEntry(pSystemProcess->ProcessId, &pSystemProcess->ProcessName, FALSE);

			++i;
		}

		pSystemProcess = (PSYSTEM_PROCESSES) ( ((PUCHAR) pSystemProcess) + pSystemProcess->NextEntryDelta);
	}

	ExFreePoolWithTag(SystemInfo, _POOL_TAG);


	/* if no processes exist, the computer must be booting up */
	if (i == 0)
	{
		UNICODE_STRING	System;


		BootingUp = TRUE;

		/*
		 * when booting up no processes are listed as existing even though "System" and "Idle"
		 * have already been initialized. Initialize "System" process entry since it will
		 * never be created otherwise.
		 */

		RtlInitUnicodeString(&System, L"System");

		CreateAndLoadNewProcessEntry(SystemProcessId, &System, FALSE);
	}
}



/*
 * InitProcessNameEntries()
 *
 * Description:
 *		Initializes process id to process name related data.
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
InitProcessNameEntries()
{
	memset(gImagePidHtbl, 0, sizeof(gImagePidHtbl));

	KeInitializeSpinLock(&gImagePidHtblSpinLock);

	SystemProcessId = (ULONG) PsGetCurrentProcessId();

	FindProcessNameOffset();


	/* XXX investigate
A driver's process-notify routine is also called with Create set to FALSE, usually when the last thread within a process has terminated and the process address space is about to be deleted. In very rare circumstances, for processes in which no thread was ever created, a driver's process-notify routine is called only at the destruction of the process. */
	if (! NT_SUCCESS( PsSetCreateProcessNotifyRoutine(CreateProcessNotifyProc, FALSE) ))
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("InitProcessNameEntries: PsSetCreateProcessNotifyRoutine() failed\n"));
		return FALSE;
	}


	return TRUE;
}



/*
 * RemoveProcessNameEntries()
 *
 * Description:
 *		Removes the global hash table process entries and PsSetCreateProcessNotifyRoutine() callback.
 *
 *		NOTE: Called once during driver unload (DriverUnload()).
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Nothing.
 */

VOID
RemoveProcessNameEntries()
{
	int					i;
	PIMAGE_PID_ENTRY	p, tmp;
	KIRQL				irql;


#if HOOK_PROCESS

	if (! NT_SUCCESS( PsSetCreateProcessNotifyRoutine(CreateProcessNotifyProc, TRUE) ))
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("RemoveProcessEntries: PsSetCreateProcessNotifyRoutine remove failed\n"));

#endif


	KeAcquireSpinLock(&gImagePidHtblSpinLock, &irql);

	for (i = 0; i < IMAGE_PID_HASHTABLE_SIZE; i++)
	{
		p = gImagePidHtbl[i].next;

		while (p)
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_VERBOSE, ("RemoveProcessEntries: Removing %d %x %S\n", p->ProcessId, p->next, p->ImageName));

			tmp = p;
			p = p->next;

			if (tmp->WaitingForUserRequestId != 0)
			{
				LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("RemoveProcessEntries: Process (pid=%d) is still waiting for a user request id %d!\n", tmp->ProcessId, tmp->WaitingForUserRequestId));
			}

			PolicyDelete(&tmp->SecPolicy);
			ExFreePoolWithTag(tmp, _POOL_TAG);
		}
	}

	KeReleaseSpinLock(&gImagePidHtblSpinLock, irql);
}
