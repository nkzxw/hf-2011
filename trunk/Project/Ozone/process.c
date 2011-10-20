/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		process.c
 *
 * Abstract:
 *
 *		This module defines various types used by process and thread hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 23-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include "process.h"
#include "driver.h"
#include "policy.h"
#include "hookproc.h"
#include "userland.h"
#include "procname.h"
#include "accessmask.h"
#include "learn.h"
#include "misc.h"
#include "i386.h"
#include "log.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitProcessEntries)
#pragma alloc_text (PAGE, ProcessPostBootup)
#endif


fpZwCreateProcess	OriginalNtCreateProcess = NULL;
fpZwCreateProcessEx	OriginalNtCreateProcessEx = NULL;
fpZwOpenProcess		OriginalNtOpenProcess = NULL;

fpZwCreateThread	OriginalNtCreateThread = NULL;
fpZwOpenThread		OriginalNtOpenThread = NULL;


BOOLEAN				AllowProcessesWithPoliciesOnly = FALSE;

WCHAR				OzoneInstallPath[MAX_PATH];
USHORT				OzoneInstallPathSize = 0;


/* XXX this will not work on 64-bit architectures, due to assumption of size of ulong, pointers, etc */
typedef struct _CONTROL_AREA {      // must be quadword sized.
	ULONG	data[9];
    PFILE_OBJECT FilePointer;
} CONTROL_AREA, *PCONTROL_AREA;

typedef struct _SEGMENT {
    struct _CONTROL_AREA *ControlArea;
} SEGMENT, *PSEGMENT;

typedef struct _SECTION {
	ULONG_PTR	data[5];
    PSEGMENT Segment;
} SECTION, *PSECTION;



/*
 * rand()
 *
 * Description:
 *		Returns a random number that is calculated by XOR'ing together often changing system values.
 *
 * Parameters:
 *		randval - An optional random value.
 *
 * Returns:
 *		A random ULONG value.
 */

ULONG
rand(ULONG randval)
{
	LARGE_INTEGER	perf, TickCount;
	ULONG			r;


	KeQueryPerformanceCounter(&perf);
//	KdPrint(("perf: %d %d\n", perf.LowPart, perf.HighPart));

	KeQueryTickCount(&TickCount);
//	KdPrint(("tick: %d %d\n", TickCount.LowPart, TickCount.HighPart));

//	KdPrint(("int: %I64d\n", KeQueryInterruptTime()));

	r = randval ^
		perf.LowPart ^ perf.HighPart ^
		TickCount.HighPart ^ TickCount.LowPart ^
		(ULONG) KeQueryInterruptTime();

//	KdPrint(("rand = %x\n", r));


	return r;
}



/*
 * PostProcessNtCreateProcess()
 *
 * Description:
 *		This function is called by the mediated NtCreateProcess() system service.
 *		It is responsible for saving the information about the newly created process in a
 *		global process hash table.
 *
 * Parameters:
 *		ProcessHandle - process object handle initialized by NtCreateProcess().
 *		SectionHandle - specifies a handle to an image section that grants SECTION_MAP_EXECUTE
 *		    access. If this value is zero, the new process inherits the address space from the process
 *		    referred to by InheritFromProcessHandle. In Windows 2000 the lowest bit specifies
 *		    (when set) that the process should not be associated with the job of the
 *		    InheritFromProcessHandle process.
 *
 * Returns:
 *		ULONG indicating an action to take (ACTION_DENY to disallow process createion, ACTION_PERMIT to allow).
 */

ULONG
PostProcessNtCreateProcess(PHANDLE ProcessHandle, HANDLE SectionHandle)
{
	NTSTATUS					status, rc;
	PIMAGE_PID_ENTRY			p, prev, NewProcess;
	ULONG						ProcessId, Size;
	PULONG_PTR					Base;
	UNICODE_STRING				usPath;
	ANSI_STRING					asPath;
	KIRQL						irql;
	PROCESS_BASIC_INFORMATION	ProcessBasicInfo;
	CHAR						ProcessPathUnresolved[MAX_PATH];
	CHAR						PROCESSNAME[MAX_PATH];
	PCHAR						FunctionName = "PostProcessNtCreateProcess";


	if (ProcessHandle == NULL)
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_WARNING, ("%s: ProcessHandle is NULL\n", FunctionName));
		return ACTION_NONE;
	}


	/*
	 * Try to retrieve the image name from a section object
	 */

	if (!SectionHandle)
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_WARNING, ("%s: SectionHandle is NULL\n", FunctionName));
		return ACTION_NONE;
	}


	do
	{
		PSECTION		SectionToMap;
		PSEGMENT		seg;
		PCONTROL_AREA	pca;
		PFILE_OBJECT	pfo;


		status = ObReferenceObjectByHandle(
					SectionHandle,
					0,
					0,
					KernelMode,
					(PSECTION *) &SectionToMap,
					NULL
					);

/* macro shortcut for bailing out of PostProcessNtCreateProcess do {} while loop in case of an error */

#define	ABORT_PostProcessNtCreateProcess(msg)													\
		{																						\
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("Error occurred in %s:", FunctionName));	\
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, (msg));										\
			return ACTION_NONE; /* XXX */														\
		}

		if (! NT_SUCCESS(status))
			
			ABORT_PostProcessNtCreateProcess(("ObReferenceObjectByHandle(SectionHandle) failed"));


		if (SectionToMap == NULL)

			ABORT_PostProcessNtCreateProcess(("SectionToMap is NULL"));


		if ( (seg = ((PSECTION)SectionToMap)->Segment) == NULL)

			ABORT_PostProcessNtCreateProcess(("Segment is NULL"));

		if ( (pca = seg->ControlArea) == NULL)

			ABORT_PostProcessNtCreateProcess(("ControlArea is NULL"));


		if ( (pfo = pca->FilePointer) == NULL)

			ABORT_PostProcessNtCreateProcess(("FilePointer is NULL"));


		usPath = pfo->FileName;

		if (usPath.Length == 0)

			ABORT_PostProcessNtCreateProcess(("FileName length is 0"));


		_snprintf(ProcessPathUnresolved, MAX_PATH, "%S", usPath.Buffer);
		ProcessPathUnresolved[MAX_PATH - 1] = 0;


		ObDereferenceObject(SectionToMap);

	} while (0);


	/*
	 * Now verify the executable name against the security policy
	 */

	VerifyExecutableName(ProcessPathUnresolved);


	if (LearningMode == FALSE)
	{
		ACTION_TYPE Action;


		VerifyUserReturnAddress();


		FixupFilename(ProcessPathUnresolved, PROCESSNAME, MAX_PATH);


		/*
		 * We manually inc/dec HookedRoutineRunning since POLICY_CHECK_OPTYPE_NAME() can call
		 * HOOK_ROUTINE_EXIT() which will decrement HookedRoutineRunning and then it will get
		 * decremented the second time in HookedNtCreateProcess()
		 */

#if DBG
		InterlockedIncrement(&HookedRoutineRunning);
#endif

		POLICY_CHECK_OPTYPE_NAME(PROCESS, OP_PROC_EXECUTE);

#if DBG
		InterlockedDecrement(&HookedRoutineRunning);
#endif
	}
	else
	{
		// learning mode
		AddRule(RULE_PROCESS, ProcessPathUnresolved, OP_PROC_EXECUTE);
	}


	/*
	 * retrieve the Process ID
	 */

	status = ZwQueryInformationProcess(*ProcessHandle, ProcessBasicInformation, &ProcessBasicInfo, sizeof(ProcessBasicInfo), &Size);

	if (! NT_SUCCESS(status))
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("PostProcessNtCreateProcess: ZwQueryInformationProcess failed with status %x\n", status));
		return ACTION_NONE;
	}

	ProcessId = ProcessBasicInfo.UniqueProcessId;

	/*
	 * On win2k ProcessId is not available at this stage yet.
	 * ProcessId of 0 will get replaced by a real value in CreateProcessNotifyProc.
	 */


	/* once winlogon.exe executes, we consider the boot process to be complete */
	if (BootingUp == TRUE)
	{
		PCHAR	ProcessName;


		ProcessName = strrchr(ProcessPathUnresolved, '\\');

		if (ProcessName == NULL)
			ProcessName = ProcessPathUnresolved;
		else
			++ProcessName;	/* skip past the slash */

		if (_strnicmp(ProcessName, "winlogon.exe", 12) == 0)
		{
			BootingUp = FALSE;
			InitPostBootup();
		}
	}


	/*
	 * Now create a new process entry and load the associated security policy (if any)
	 */

	NewProcess = CreateNewProcessEntry(ProcessId, CURRENT_PROCESS_PID, &usPath, TRUE);


	if (ProcessInsertImagePidEntry(ProcessId, NewProcess) == FALSE)
	{
		ExFreePoolWithTag(NewProcess, _POOL_TAG);

		return ACTION_NONE;
	}


	/*
	 * Now find and load appropriate security policy.
	 *
	 * Look for a per-user policy first. To do that, we send an SID resolve request
	 * to userland Ozone Agent Service.
	 */

	if (LearningMode == FALSE)
	{
		PSID_RESOLVE_REPLY	pSidResolveReply = NULL;
		PWSTR				UserName = NULL;


 		if (IssueUserlandSidResolveRequest(NewProcess) != FALSE)
		{
			if (NewProcess->UserlandReply)
			{
				pSidResolveReply = (PSID_RESOLVE_REPLY) NewProcess->UserlandReply;

				if (pSidResolveReply->UserNameLength)
				{
					LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("PostProcessNtCreateProcess: SID resolved to %S\n", pSidResolveReply->UserName));
					UserName = pSidResolveReply->UserName;
				}
				else
				{
					LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("PostProcessNtCreateProcess(): SID resolve error\n"));
				}
			}
		}


		if (! FindAndLoadSecurityPolicy(&NewProcess->SecPolicy, usPath.Buffer, UserName))
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_WARNING, ("%d PostProcessNtCreateProcess(): no policy, %d, %S\n", CURRENT_PROCESS_PID, ProcessId, usPath.Buffer));

			//XXX have an option where only processes with existing policies (even if they are empty.. policy_default: allow) are allowed to run
			//interactive session is excluded?!?!
			if (AllowProcessesWithPoliciesOnly == TRUE)
			{
				ExFreePoolWithTag(NewProcess, _POOL_TAG);
				return ACTION_DENY;
			}
		}
		else
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_WARNING, ("%d PostProcessNtCreateProcess(): with policy, %d, %S\n", CURRENT_PROCESS_PID, ProcessId, usPath.Buffer));
		}


		if (NewProcess->UserlandReply)
		{
			ExFreePoolWithTag(NewProcess->UserlandReply, _POOL_TAG);
			NewProcess->UserlandReply = NULL;
		}
	}


	/*
	 * Stack Buffer Overflow Protection (Part 1).
	 *
	 * 1. Allocate/reserve a random (< 0x4000000) chunk of virtual memory starting at 0xFFFF.
	 * This causes the stack of the main thread to be moved by a random amount.
	 *
	 * (note1: first 64K of memory are allocated as a guard against NULL pointers dereferencing).
	 * (note2: main() is mapped at 0x4000000 and thus we cannot allocate anything that will cause the
	 *		   stack of the main thread to move past the 0x400000 boundary).
	 *
	 *
	 * Stack Buffer Overflow Protection (Part 2).
	 *
	 * 2. Allocate a random (> 4 Megs && < 10 Megs) chunk of virtual memory after the process code segment.
	 * This causes the heap and stacks non-main threads to be moved by a random amount.
	 *
	 * (note: as mentioned above, main() is mapped at 0x4000000, by reserving a large chunk of virtual
	 *		  we force Windows to find the first available address beyond the code segment and reserve
	 *		  a random amount of memory past it causing other thread stacks and heaps to shift).
	 */

#define	ONE_MEGABYTE	(1024 * 1024)

	if (IS_OVERFLOW_PROTECTION_ON(gSecPolicy) && IS_OVERFLOW_PROTECTION_ON(NewProcess->SecPolicy) && LearningMode == FALSE && BootingUp == FALSE)
	{
//XXX verify that the image entry is actually at 4 meg (not true for most of system processes)

		/*
		 * allocate up to 3 megs of virtual address space before the code segment,
		 * this affects main thread stack as well as some heaps
		 */

#define	FIRST_AVAILABLE_ADDRESS	0xFFFF

		Size = PAGE_SIZE + (rand(ProcessId) % (3 * ONE_MEGABYTE));
		Base = (PULONG_PTR) FIRST_AVAILABLE_ADDRESS;

		status = ZwAllocateVirtualMemory(*ProcessHandle, &Base, 0L, &Size, MEM_RESERVE, PAGE_NOACCESS);

		if (! NT_SUCCESS(status))
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("PostProcessNtCreateProcess: ZwAllocateVirtualMemory1(%x, %x) failed with status %x\n", Base, Size, status));
		else
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_VERBOSE, ("PostProcessNtCreateProcess: size=%u base=%x status=%d\n", Size, Base, status));

		/*
		 * allocate up to 10 megs of virtual address space after the code segment,
		 * this affects non-main thread stack as well as some heaps
		 */

#define	IMAGE_BASE	(4 * ONE_MEGABYTE)

		Size = IMAGE_BASE + (rand(ProcessId) % (10 * ONE_MEGABYTE));
		Base = (PULONG_PTR) NULL;

		status = ZwAllocateVirtualMemory(*ProcessHandle, &Base, 0L, &Size, MEM_RESERVE, PAGE_NOACCESS);

		LOG(LOG_SS_PROCESS, LOG_PRIORITY_VERBOSE, ("PostProcessNtCreateProcess: size=%u base=%x status=%d\n", Size, Base, status));

		if (! NT_SUCCESS(status))
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("PostProcessNtCreateProcess: ZwAllocateVirtualMemory2(%x, %x) failed with status %x\n", Base, Size, status));



		/*
		 * allocate the entire KnownDll space
		 */
//#if 0
//		Size = (4 * ONE_MEGABYTE) + (rand(ProcessId) % (100 * ONE_MEGABYTE));
//		Base = (PULONG_PTR) 0x71bf0000;

//		Size = 0x7000000;//(125 * ONE_MEGABYTE);
//		Base = (PULONG_PTR) 0x70000000;

#if HOOK_BOPROT
		if (strstr(ProcessPathUnresolved, "stack.exe") != NULL)
		{
			Size = PAGE_SIZE;	
	//		Base = (PULONG_PTR) 0x77d00000;	//user32
			Base = (PULONG_PTR) 0x77e30000;	//kernel32 on win2k3
//			Base = (PULONG_PTR) 0x77e80000;	//kernel32 on win2k

			status = ZwAllocateVirtualMemory(*ProcessHandle, &Base, 0L, &Size, MEM_RESERVE, PAGE_NOACCESS);

			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("PostProcessNtCreateProcess: kernel32.dll size=%u base=%x status=%d\n", Size, Base, status));

			if (! NT_SUCCESS(status))
				LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("PostProcessNtCreateProcess: ZwAllocateVirtualMemory1(%x, %x) failed with status %x\n", Base, Size, status));
		}
		else
			NewProcess->FirstThread = FALSE;//XXX remove
#endif
	}

	
	return ACTION_PERMIT;
}



/*
 * HookedNtCreateProcess()
 *
 * Description:
 *		This function mediates the NtCreateProcess() system service in order to keep track of all
 *		the newly created processes.
 *
 *		NOTE: ZwCreateProcess creates a process object. [NAR]
 *
 * Parameters:
 *		Those of NtCreateProcess().
 *
 * Returns:
 *		NTSTATUS returned by NtCreateProcess().
 */

NTSTATUS
NTAPI
HookedNtCreateProcess
(
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE InheritFromProcessHandle,
	IN BOOLEAN InheritHandles,
	IN HANDLE SectionHandle OPTIONAL,
	IN HANDLE DebugPort OPTIONAL,
	IN HANDLE ExceptionPort OPTIONAL
)
{
	HOOK_ROUTINE_ENTER();


	ASSERT(OriginalNtCreateProcess);

	rc = OriginalNtCreateProcess(ProcessHandle, DesiredAccess, ObjectAttributes, InheritFromProcessHandle,
									InheritHandles, SectionHandle, DebugPort, ExceptionPort);


	if (NT_SUCCESS(rc))
	{
		ULONG	ret = PostProcessNtCreateProcess(ProcessHandle, SectionHandle);

		if (ret == ACTION_DENY || ret == STATUS_ACCESS_DENIED)
		{
			ZwClose(*ProcessHandle);
			rc = STATUS_ACCESS_DENIED;
		}
	}


	HOOK_ROUTINE_EXIT(rc);
}



/*
 * HookedNtCreateProcessEx()
 *
 * Description:
 *		This function mediates the NtCreateProcessEx() system service in order to keep track of all
 *		the newly created processes.
 *
 *		NOTE: ZwCreateProcessEx creates a process object. [NAR]
 *
 * Parameters:
 *		Those of NtCreateProcessEx().
 *
 * Returns:
 *		NTSTATUS returned by NtCreateProcessEx().
 */

NTSTATUS
NTAPI
HookedNtCreateProcessEx
(
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE InheritFromProcessHandle,
	IN ULONG Unknown1,
	IN HANDLE SectionHandle OPTIONAL,
	IN HANDLE DebugPort OPTIONAL,
	IN HANDLE ExceptionPort OPTIONAL,
	IN ULONG Unknown2
)
{
	HOOK_ROUTINE_ENTER();


	ASSERT(OriginalNtCreateProcessEx);

	rc = OriginalNtCreateProcessEx(ProcessHandle, DesiredAccess, ObjectAttributes, InheritFromProcessHandle, 
									 Unknown1, SectionHandle, DebugPort, ExceptionPort, Unknown2);


	if (NT_SUCCESS(rc))
	{
		ULONG	ret = PostProcessNtCreateProcess(ProcessHandle, SectionHandle);

		if (ret == ACTION_DENY || ret == STATUS_ACCESS_DENIED)
		{
			ZwClose(*ProcessHandle);
			rc = STATUS_ACCESS_DENIED;
		}
	}


	HOOK_ROUTINE_EXIT(rc);
}



/*
 * HookedNtOpenProcess()
 *
 * Description:
 *		This function mediates the NtOpenProcess() system service and disallows certain operations such as
 *		PROCESS_VM_WRITE and PROCESS_CREATE_THREAD.
 *
 *		NOTE: ZwOpenProcess opens a process object. [NAR]
 *
 * Parameters:
 *		Those of NtOpenProcess().
 *
 * Returns:
 *		NTSTATUS returned by NtOpenProcess().
 */

NTSTATUS
NTAPI
HookedNtOpenProcess
(
	OUT PHANDLE ProcessHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId OPTIONAL
)
{
	PCHAR	FunctionName = "HookedNtOpenProcess";
	CHAR	PROCESSNAME[MAX_PATH];


//taskmgr uses PROCESS_TERMINATE to kill processes
//XXX IPD disallows PROCESS_CREATE_THREAD|PROCESS_VM_WRITE

/*
PROCESS_TERMINATE Terminate process
PROCESS_CREATE_THREAD Create threads in process
PROCESS_SET_SESSIONID Set process session id
PROCESS_VM_OPERATION Protect and lock memory of process
PROCESS_VM_READ Read memory of process
PROCESS_VM_WRITE Write memory of process
PROCESS_DUP_HANDLE Duplicate handles of process
PROCESS_CREATE_PROCESS Bequeath address space and handles to new process
PROCESS_SET_QUOTA Set process quotas
PROCESS_SET_INFORMATION Set information about process
PROCESS_QUERY_INFORMATION Query information about process
PROCESS_SET_PORT Set process exception or debug port
PROCESS_ALL_ACCESS All of the preceding

find out who uses which flags, i.e. VM_READ, etc.. filter out accordingly
*/

	HOOK_ROUTINE_ENTER();


//	if (! IS_BIT_SET(DesiredAccess, PROCESS_QUERY_INFORMATION) &&
//		! IS_BIT_SET(DesiredAccess, PROCESS_DUP_HANDLE) &&
//		! IS_BIT_SET(DesiredAccess, SYNCHRONIZE) )


	if (! ARGUMENT_PRESENT(ClientId) || KeGetPreviousMode() != UserMode || KeGetCurrentIrql() != PASSIVE_LEVEL)
		goto done;


	__try
	{
		ProbeForRead(ClientId, sizeof(*ClientId), sizeof(PULONG_PTR));
	}

	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		NTSTATUS status = GetExceptionCode();

		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("%s: caught an exception. address=%x, status = 0x%x\n", FunctionName, ClientId, status));

		goto done;
	}


	if (PsGetCurrentProcessId() != ClientId->UniqueProcess &&
		(ULONG) PsGetCurrentProcessId() != SystemProcessId &&
		ClientId->UniqueProcess != 0)
	{
		PIMAGE_PID_ENTRY	p;

		/* can't access ClientId (pageable memory) while holding spinlonk */
		ULONG				RequestedProcessId = (ULONG) ClientId->UniqueProcess;


	//XXX
/*
		if (RequestedProcessId == UserAgentServicePid)
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_VERBOSE, ("%s: FindImagePidEntry(%d) UserAgent %x\n", FunctionName, RequestedProcessId, DesiredAccess));

			if (IS_BIT_SET(DesiredAccess, PROCESS_TERMINATE))
			{
				HOOK_ROUTINE_EXIT( STATUS_ACCESS_DENIED );
			}
		}
*/


		p  = FindImagePidEntry(RequestedProcessId, 0);

		if (p == NULL)
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d %s: FindImagePidEntry(%d) failed\n", CURRENT_PROCESS_PID, FunctionName, RequestedProcessId));
			goto done;
		}

/*
		if (DesiredAccess & PROCESS_CREATE_THREAD)
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d %s(%d): %x (PROCESS_CREATE_THREAD). (%S)\n", CURRENT_PROCESS_PID, FunctionName, RequestedProcessId, DesiredAccess, p->ImageName));
		}
		else if (DesiredAccess & PROCESS_VM_WRITE)
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d %s(%d): %x (PROCESS_VM_WRITE). (%S)\n", CURRENT_PROCESS_PID, FunctionName, RequestedProcessId, DesiredAccess, p->ImageName));
		}
		else
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_VERBOSE, ("%d %s(%d): %x. (%S)\n", CURRENT_PROCESS_PID, FunctionName, RequestedProcessId, DesiredAccess, p->ImageName));
		}
*/


		if (LearningMode == FALSE)
		{
			CHAR	ProcessPathUnresolved[MAX_PATH];


			_snprintf(ProcessPathUnresolved, MAX_PATH, "%S", p->ImageName);
			ProcessPathUnresolved[ MAX_PATH - 1 ] = 0;


			FixupFilename(ProcessPathUnresolved, PROCESSNAME, MAX_PATH);


			POLICY_CHECK_OPTYPE_NAME(PROCESS, OP_PROC_OPEN);
		}
		else
		{
			// learning mode
			_snprintf(PROCESSNAME, MAX_PATH, "%S", p->ImageName);
			PROCESSNAME[ MAX_PATH - 1 ] = 0;

			AddRule(RULE_PROCESS, PROCESSNAME, OP_PROC_OPEN);
		}
	}


	if (LearningMode == FALSE && GetPathFromOA(ObjectAttributes, PROCESSNAME, MAX_PATH, RESOLVE_LINKS))
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d %s: %s SPECIAL CASE\n", (ULONG) PsGetCurrentProcessId(), FunctionName, PROCESSNAME));
	}


done:

	ASSERT(OriginalNtOpenProcess);

	rc = OriginalNtOpenProcess(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);


	HOOK_ROUTINE_EXIT(rc);
}



/*
 * HookedNtCreateThread()
 *
 * Description:
 *		This function mediates the NtCreateThread() system service in order to randomize thread stack
 *		and inject userland dll into newly created main threads.
 *
 *		NOTE: ZwCreateThread creates a thread in a process. [NAR]
 *
 * Parameters:
 *		Those of NtCreateThread().
 *
 * Returns:
 *		NTSTATUS returned by NtCreateThread().
 */

NTSTATUS
NTAPI
HookedNtCreateThread
(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN HANDLE ProcessHandle,
	OUT PCLIENT_ID ClientId,
	IN PCONTEXT ThreadContext,
	IN PUSER_STACK UserStack,
	IN BOOLEAN CreateSuspended
)
{
	PEPROCESS			proc = NULL;
	USHORT				StackOffset = 0;
	PCHAR				FunctionName = "HookedNtCreateThread";


	HOOK_ROUTINE_ENTER();


	if (ARGUMENT_PRESENT(ThreadContext) && KeGetPreviousMode() == UserMode && LearningMode == FALSE && BootingUp == FALSE)
	{
		NTSTATUS					status;
		ULONG						ProcessId;
		PCHAR						InstructionAddress;
		ULONG						Size;
		PCHAR						Base;
		PROCESS_BASIC_INFORMATION	ProcessBasicInfo;
		ULONG						ret;
		PIMAGE_PID_ENTRY			p;


		VerifyUserReturnAddress();


		/* verify userland parameter threadcontext */
		__try
		{
			ProbeForRead(ThreadContext, sizeof(*ThreadContext), sizeof(ULONG));
			ProbeForWrite(ThreadContext, sizeof(*ThreadContext), sizeof(ULONG));
		}

		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			NTSTATUS status = GetExceptionCode();

			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%s: caught an exception. status = 0x%x\n", FunctionName, status));

			goto done;
		}


		if (ThreadContext->Eax > SystemAddressStart)
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%s: eax=%x > %x (SystemAddressStart)\n", FunctionName, ThreadContext->Eax, SystemAddressStart));
			goto done;
		}


		/* retrieve the Process ID */
		status = ZwQueryInformationProcess(ProcessHandle, ProcessBasicInformation, &ProcessBasicInfo, sizeof(ProcessBasicInfo), &Size);

		if (! NT_SUCCESS(status))
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%s: ZwQueryInformationProcess failed with status %x\n", FunctionName, status));
			goto done;
		}

		ProcessId = ProcessBasicInfo.UniqueProcessId;

		/*
		 * if ProcessId is 0 then the pid has not been assigned yet and we are the primary thread.
		 * in that case pass our pid (we are still running in the context of the parent process) as the ParentId
		 * to make sure we find the right process (since theoretically there can be more than one process with a
		 * pid of 0)
		 */
		p = FindImagePidEntry(ProcessId, ProcessId == 0 ? CURRENT_PROCESS_PID : 0);

		if (p == NULL)
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d %s: FindImagePidEntry(%d) failed\n", CURRENT_PROCESS_PID, FunctionName, ProcessId));
			goto done;
		}


		/*
		 * Stack Buffer Overflow Protection (Part 3).
		 *
		 * Allocate/reserve a random (< 1024 bytes) part of a thread's stack space.
		 * This causes the least significant 10 bits to be randomized as well.
		 * (without this, the least significant 16 bits are always the same).
		 */

		/* save the stackoffset for now since we are holding a spinlock and cannot touch pageable memory */
//XXX we are not holding the spinlock here but are still accessing various p-> fields
		if (IS_OVERFLOW_PROTECTION_ON(gSecPolicy) && IS_OVERFLOW_PROTECTION_ON(p->SecPolicy))

			StackOffset = (USHORT) (16 + (rand(ThreadContext->Eax) % 63) * 16);


		if (! IS_USERLAND_PROTECTION_ON(gSecPolicy) || ! IS_USERLAND_PROTECTION_ON(p->SecPolicy))
			goto done;


		/* Userland DLL needs to be loaded only once (by the first/main thread) */

		if (p->FirstThread != TRUE)
			goto done;

		p->FirstThread = FALSE;

//XXX investigate MEM_WRITE_WATCH (supported on i386?)


		/*
		 * Inject the userland DLL into the process.
		 *
		 * This is achieved by allocating 1 page of memory in the address space of a "victim" process.
		 * Then the LdrLoadDll(our_dll) code is written to the allocated page and victim's thread EIP
		 * is changed to point to our code.
		 * As soon as the thread executes, it will load our DLL and then jump to the original entry point.
		 *
		 * When not given explicit directions, the Microsoft linker creates each DLL with a
		 * preferred load address of 0x10000000. By loading our DLL first, we cause the rest of
		 * the application DLLs to load at a different address.
		 */

		/* allocate 1 page of commited rwx memory */

		Size = PAGE_SIZE;
		Base = NULL;

		status = ZwAllocateVirtualMemory(ProcessHandle, &Base, 0L, &Size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

		if (! NT_SUCCESS(status))
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%s: ZwAllocateVirtualMemory(%x, %x) failed with status %x\n", FunctionName, Base, Size, status));
			goto done;
		}


		status = ObReferenceObjectByHandle(ProcessHandle, PROCESS_ALL_ACCESS, 0, KernelMode, &proc, NULL);

		if (! NT_SUCCESS(status))
		{
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%s: ObReferenceObjectByHandle(ProcessHandle) failed with status %x\n", FunctionName, status));
			goto done;
		}


		try
		{
			ULONG	CodeAddress, DllPathAddress;
			ULONG	ThreadEntryPoint = ThreadContext->Eax;	/* thread entry point is stored in the EAX register */
			PWSTR	InjectDllName = L"ozoneusr.dll";
			USHORT	InjectDllNameSize = (wcslen(InjectDllName) + 1) * sizeof(WCHAR);


			/*
			 * Execute the DLL inject in the memory context of a "victim" process
			 */

			KeAttachProcess(proc);
			{
				/* probe the memory, just in case */
				ProbeForRead(Base, PAGE_SIZE, 1);
				ProbeForWrite(Base, PAGE_SIZE, 1);


				/*
				 * Memory Layout used for DLL injection
				 *
				 * Byte		Value
				 *
				 * 0..4		Original EIP
				 * 4..8		LdrLoadDll() output handle
				 * 8..16	LdrLoadDll() DllName UNICODE_STRING structure 
				 * 16..?	DllName.Buffer WCHAR string
				 * ?..?		DllPath WCHAR string
				 * ....		assembler code (to call LdrLoadDll() and then jmp to the original EIP)
				 */

#define	BASE_PTR					Base
#define	ADDRESS_ORIGINAL_EIP		(BASE_PTR + 0)
#define	ADDRESS_DLL_HANDLE			(BASE_PTR + 4)
#define	ADDRESS_DLL_NAME			(BASE_PTR + 8)


				/* save the original thread entry point */
				* (PULONG) ADDRESS_ORIGINAL_EIP = ThreadEntryPoint;

				/* skip past eip and output handle (8 bytes) */
				InstructionAddress = ADDRESS_DLL_NAME;

				/*
				 * Create a UNICODE_STRING structure
				 */

				* ((PUSHORT) InstructionAddress)++ = InjectDllNameSize;	/* UNICODE_STRING.Length */
				* ((PUSHORT) InstructionAddress)++ = InjectDllNameSize;	/* UNICODE_STRING.MaximumLength */

				/* UNICODE_STRING.Buffer (points to unicode string directly following the buffer) */

				* ((PULONG) InstructionAddress)++ = (ULONG) (InstructionAddress + sizeof(PWSTR));


				/* the actual DllName.Buffer value */

				wcscpy((PWSTR) InstructionAddress, InjectDllName);

				InstructionAddress += InjectDllNameSize;


				/* DllPathValue value */

				DllPathAddress = (ULONG) InstructionAddress;

				wcscpy((PWSTR) InstructionAddress, OzoneInstallPath);

				InstructionAddress += OzoneInstallPathSize;


				CodeAddress = (ULONG) InstructionAddress;


				/*
				 * Generate code that will call LdrLoadDll and then jmp to the original entry point.
				 */

				/*
				 *	NTSTATUS
				 *	LdrLoadDll (
				 *		IN PWSTR DllPath OPTIONAL,
				 *		IN PULONG DllCharacteristics OPTIONAL,
				 *		IN PUNICODE_STRING DllName,
				 *		OUT PVOID *DllHandle
				 *		);
				 *
				 * Save LdrLoadDll parameters on stack (last to first).
				 */

				ASM_PUSH(InstructionAddress, ADDRESS_DLL_HANDLE);		/* DllHandle			*/
				ASM_PUSH(InstructionAddress, ADDRESS_DLL_NAME);			/* DllName				*/
				ASM_PUSH(InstructionAddress, 0);						/* DllCharacteristics	*/
//				ASM_PUSH(InstructionAddress, NULL);						/* DllPath				*/
				ASM_PUSH(InstructionAddress, DllPathAddress);			/* DllPath				*/

				ASM_CALL(InstructionAddress, FindFunctionBase(NTDLL_Base, "LdrLoadDll"));


				//XXX now clear out all the data up to this instruction
				//be careful first 4 bytes are used by the next instruction
				/*
				mov	ecx, 16
				lea edi, Base + 4
				rep	stosw
				*/

				ASM_JMP(InstructionAddress, Base);
			}
			KeDetachProcess();

			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d %s: Replacing original thread entry point %x with %x\n", CURRENT_PROCESS_PID, FunctionName, ThreadContext->Eax, CodeAddress));

			/* hijack the thread instruction pointer (saved in EAX) */
//			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("eip %x eax %x\n", ThreadContext->Eip, ThreadContext->Eax));

			ThreadContext->Eax = (ULONG) CodeAddress;

			ThreadContext->Eip = ThreadContext->Eax;

			ObDereferenceObject(proc);
		}

		except(EXCEPTION_EXECUTE_HANDLER)
		{
			NTSTATUS status = GetExceptionCode();

			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%s: caught an exception. status = 0x%x\n", FunctionName, status));

			KeDetachProcess();

			ObDereferenceObject(proc);
		}
	}


done:

	/*
	 * finally adjust the stack.
	 * could not have done it before since we were holding a spinlock and weren't allowed to access pageable memory
	 */

	if (StackOffset)

		ThreadContext->Esp -= StackOffset;


	ASSERT(OriginalNtCreateThread);

	rc = OriginalNtCreateThread(ThreadHandle, DesiredAccess, ObjectAttributes, ProcessHandle,
								ClientId, ThreadContext, UserStack, CreateSuspended);


	HOOK_ROUTINE_EXIT(rc);
}



/*
 * HookedNtOpenThread()
 *
 * Description:
 *		This function mediates the NtOpenThread() system service in order to XXX
 *		.
 *
 *		NOTE: ZwOpenThread opens a thread object. [NAR]
 *
 * Parameters:
 *		Those of NtOpenThread().
 *
 * Returns:
 *		NTSTATUS returned by NtOpenThread().
 */

NTSTATUS
NTAPI
HookedNtOpenThread
(
	OUT PHANDLE ThreadHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PCLIENT_ID ClientId
)
{
	CHAR	THREADNAME[MAX_PATH];

	HOOK_ROUTINE_ENTER();


	if (LearningMode == FALSE && GetPathFromOA(ObjectAttributes, THREADNAME, MAX_PATH, RESOLVE_LINKS))
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d HookedNtOpenThread: %s\n", (ULONG) PsGetCurrentProcessId(), THREADNAME));
	}


	if (! IS_BIT_SET(DesiredAccess, THREAD_QUERY_INFORMATION))
	{
		// ClientId->UniqueProcess = 0 if process opens a thread in the same process (wmplayer.exe)
		if (ClientId)
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d HookedNtOpenThread(%d %d): %x\n", (ULONG) PsGetCurrentProcessId(), ClientId->UniqueProcess, ClientId->UniqueThread, DesiredAccess));
		else
			LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("%d HookedNtOpenThread(): %x\n", (ULONG) PsGetCurrentProcessId(), DesiredAccess));
	}

	ASSERT(OriginalNtOpenThread);

	rc = OriginalNtOpenThread(ThreadHandle, DesiredAccess, ObjectAttributes, ClientId);


	HOOK_ROUTINE_EXIT(rc);
}



/*
 * ProcessPostBootup()
 *
 * Description:
 *		Find out where userland Ozone files are installed once the bootup process is complete.
 *		We are unable to read some parts of the registry before the bootup is complete since
 *		they have not been initialized yet.
 *
 *		ProcessPostBootup() might run even before bootup is complete in order to setup up the
 *		default values. When ProcessPostBootup() runs again after bootup, the default values
 *		will be overwritten with the correct ones.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Nothing.
 */

VOID
ProcessPostBootup()
{
	if (ReadStringRegistryValueW(L"\\Registry\\Machine\\Software\\Security Architects\\Ozone Agent", L"InstallPath", OzoneInstallPath, MAX_PATH) == FALSE)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("ProcessPostBootup: Failed to open InstallPath registry key\n"));

		// use the default path
		wcscpy(OzoneInstallPath, L"C:\\Program Files\\Security Architects\\Ozone Agent");
	}

	OzoneInstallPathSize = (wcslen(OzoneInstallPath) + 1) * sizeof(WCHAR);
}



/*
 * InitProcessEntries()
 *
 * Description:
 *		Initializes all the mediated process operation pointers. The "OriginalFunction" pointers
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
/*
VOID
LoadImageNotifyProc
(
    IN PUNICODE_STRING  FullImageName,
    IN HANDLE  ProcessId, // where image is mapped
    IN PIMAGE_INFO  ImageInfo
)
{
	if (FullImageName && ImageInfo)
	{
		KdPrint(("LoadImageNotifyProc: %d %S %x %x\n", ProcessId, FullImageName->Buffer, ImageInfo->ImageBase, ImageInfo->ImageSize));
	}
	else
	{
		KdPrint(("LoadImageNotifyProc: NULL Pointer %x %x\n", FullImageName, ImageInfo));
	}
}
*/

BOOLEAN
InitProcessEntries()
{
//	PsSetLoadImageNotifyRoutine(LoadImageNotifyProc);

	if ( (OriginalNtCreateProcess = (fpZwCreateProcess) ZwCalls[ZW_CREATE_PROCESS_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("InitProcessEntries: OriginalNtCreateProcess is NULL\n"));
		return FALSE;
	}
	
	if ( (OriginalNtCreateProcessEx = (fpZwCreateProcessEx) ZwCalls[ZW_CREATE_PROCESSEX_INDEX].OriginalFunction) == NULL)
	{
		/* does not exist on Win2K */
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("InitProcessEntries: OriginalNtCreateProcessEx is NULL\n"));
	}

	if ( (OriginalNtOpenProcess = (fpZwOpenProcess) ZwCalls[ZW_OPEN_PROCESS_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("InitProcessEntries: OriginalNtOpenProcess is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtCreateThread = (fpZwCreateThread) ZwCalls[ZW_CREATE_THREAD_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("InitProcessEntries: OriginalNtCreateThread is NULL\n"));
		return FALSE;
	}

	if ( (OriginalNtOpenThread = (fpZwOpenThread) ZwCalls[ZW_OPEN_THREAD_INDEX].OriginalFunction) == NULL)
	{
		LOG(LOG_SS_PROCESS, LOG_PRIORITY_DEBUG, ("InitProcessEntries: OriginalNtOpenThread is NULL\n"));
		return FALSE;
	}


	/*
	 * run ProcessPostBootup() even if we are still booting up, this way we will at least setup
	 * the default values. When ProcessPostBootup() runs again after bootup, the default values
	 * will be overwritten with the correct ones.
	 */

	ProcessPostBootup();


	return TRUE;
}
