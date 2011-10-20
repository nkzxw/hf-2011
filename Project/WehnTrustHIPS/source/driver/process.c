/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "precomp.h"

typedef struct _PROCESS_MEMORY_STATE
{
	LIST_ENTRY          Entry;
	PPROCESS_OBJECT     ProcessObject;
	HANDLE              ProcessId;
	ULONG               ExemptionFlags;
	ULONG               ExecutionState;
	ULONG_PTR           StartAddress;
	ULONG_PTR           CurrentAddress;
	PIMAGE_SET_MAPPING  ExecutableMapping;

	//
	// NRER image mapping location.
	//
	NRER_DISPATCH_TABLE NrerDispatchTable;

	//
	// Execution flags that are passed to NRER for this process.
	//
	ULONG               NreExecutionFlags;

} PROCESS_MEMORY_STATE, *PPROCESS_MEMORY_STATE;

static VOID CreateProcessNotifyRoutine(
		IN HANDLE ParentId,
		IN HANDLE ProcessId,
		IN BOOLEAN Create);

static ULONG_PTR CreateProcessState(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG InitialRegionSize,
		IN ULONG ExemptionFlags OPTIONAL,
		IN ULONG ExecutionState OPTIONAL,
		IN PIMAGE_SET_MAPPING ExecutableMapping OPTIONAL,
		IN PNRER_DISPATCH_TABLE NrerDispatchTable OPTIONAL,
		IN ULONG NreExecutionFlags OPTIONAL,
		IN BOOLEAN LockResource);
static VOID UpdateOrCreateProcessState(
		IN PPROCESS_OBJECT ProcessObject,
		IN HANDLE ProcessId);
static VOID RemoveProcessState(
		IN PPROCESS_OBJECT ProcessObject,
		IN HANDLE ProcessId);
static VOID AdjustProcessNreExecutionFlags(
		IN PPROCESS_MEMORY_STATE MemoryState);

// 
// Globals
//
static LIST_ENTRY ProcessStateList;
static PERESOURCE ProcessStateListResource = NULL;

#pragma code_seg("INIT")

//
// Initialize the process memory state management subsystem
//
NTSTATUS InitializeProcess()
{
	NTSTATUS Status;

	do
	{
		//
		// Initialize the process state list
		//
		InitializeListHead(
				&ProcessStateList);

		//
		// Initialize the process resource that we'll use for locking shared and
		// exclusive access to the process memory state list
		//
		if (!(ProcessStateListResource = (PERESOURCE)ExAllocatePoolWithTag(
				NonPagedPool,
				sizeof(ERESOURCE),
				ALLOC_TAG)))
		{
			DebugPrint(("InitializeProcess(): Failed to allocate memory for process list resource."));

			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		if (!NT_SUCCESS(Status = ExInitializeResourceLite(
				ProcessStateListResource)))
		{
			DebugPrint(("InitializeProcess(): ExInitializeResourceLite failed, %.8x.", 
					Status));
			break;
		}

#ifndef DISABLE_STACKHEAP_RANDOMIZATION
		//
		// Create the process notification routine that we'll use to clean out
		// process state when a process is terminated
		//
		if (!NT_SUCCESS(Status = PsSetCreateProcessNotifyRoutine(
				CreateProcessNotifyRoutine,
				FALSE)))
		{
			DebugPrint(("InitializeProcess(): PsSetCreateProcessNotifyRoutine failed, %.8x.",
					Status));
			break;
		}
#endif

	} while (0);

	return Status;
}

#pragma code_seg()

//
// Deinitialize the process memory state lists
//
NTSTATUS DeinitializeProcess()
{
	NTSTATUS Status;

#ifndef DISABLE_STACKHEAP_RANDOMIZATION
	//
	// Remove the process notification routine
	//
	if (!NT_SUCCESS(Status = PsSetCreateProcessNotifyRoutine(
			CreateProcessNotifyRoutine,
			TRUE)))
	{
		DebugPrint(("DeinitializeProcess(): PsSetCreateProcessNotifyRoutine failed, %.8x.",
				Status));
	}
#endif

	//
	// Deinitialize the process memory list resource
	//
	if (!NT_SUCCESS(Status = ExDeleteResourceLite(
			ProcessStateListResource)))
	{
		DebugPrint(("DeinitializeProcess(): ExDeleteResourceLite failed, %.8x.",
				Status));
	}

	ExFreePool(
			ProcessStateListResource);

	return Status;
}

//
// Flags the supplied process as being exempted from randomization
//
VOID FlagProcessAsExempted(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG ExemptionFlags)
{
	PPROCESS_MEMORY_STATE Current;
	BOOLEAN               Found = FALSE;
	KIRQL                 OldIrql;

	//
	// Acquire the state list resource for exclusive access
	//
	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceExclusiveLite(
			ProcessStateListResource,
			TRUE);

	for (Current = (PPROCESS_MEMORY_STATE)ProcessStateList.Flink;
	     Current != (PPROCESS_MEMORY_STATE)&ProcessStateList;
	     Current = (PPROCESS_MEMORY_STATE)Current->Entry.Flink)
	{
		if (Current->ProcessObject == ProcessObject)
		{
			Current->ExemptionFlags = ExemptionFlags;
			Found                   = TRUE;

			AdjustProcessNreExecutionFlags(
					Current);

			break;
		}
	}

	//
	// If we didn't find a match, create the process state.
	//
	if (!Found)
		CreateProcessState(
				ProcessObject,
				0,
				ExemptionFlags,
				0,
				NULL,
				NULL,
				0,
				FALSE);
	
	//
	// Release the state list resource
	//
	ExReleaseResourceLite(
			ProcessStateListResource);
	KeLowerIrql(OldIrql);
}

//
// Checks to see if the supplied process is flagged as being exempted
//
BOOLEAN IsProcessExempted(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG ExemptionFlag)
{
	PPROCESS_MEMORY_STATE Current;
	BOOLEAN               Exempted = FALSE;
	KIRQL                 OldIrql;

	//
	// Acquire the process memory state list resource with shared access
	//
	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceSharedLite(
			ProcessStateListResource,
			TRUE);

	for (Current = (PPROCESS_MEMORY_STATE)ProcessStateList.Flink;
	     Current != (PPROCESS_MEMORY_STATE)&ProcessStateList;
	     Current = (PPROCESS_MEMORY_STATE)Current->Entry.Flink)
	{
		if (Current->ProcessObject != ProcessObject)
			continue;

		Exempted = (Current->ExemptionFlags & ExemptionFlag)
			? TRUE
			: FALSE;

		break;
	}

	//
	// Release the process memory state list resource
	//
	ExReleaseResourceLite(
			ProcessStateListResource);
	KeLowerIrql(OldIrql);

	return Exempted;
}

//
// Set one or more of the process' execution state flags
//
VOID SetProcessExecutionState(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG ExecutionState,
		IN BOOLEAN Unset)
{
	PPROCESS_MEMORY_STATE Current;
	BOOLEAN               Found = FALSE;
	KIRQL                 OldIrql;

	//
	// Acquire the state list resource for exclusive access
	//
	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceExclusiveLite(
			ProcessStateListResource,
			TRUE);

	for (Current = (PPROCESS_MEMORY_STATE)ProcessStateList.Flink;
	     Current != (PPROCESS_MEMORY_STATE)&ProcessStateList;
	     Current = (PPROCESS_MEMORY_STATE)Current->Entry.Flink)
	{
		if (Current->ProcessObject == ProcessObject)
		{
			if (Unset)
				Current->ExecutionState &= ~ExecutionState;
			else
				Current->ExecutionState |= ExecutionState;

			Found = TRUE;
			break;
		}
	}

	//
	// If we didn't find a match, create the process state.
	//
	if (!Found)
		CreateProcessState(
				ProcessObject,
				0,
				0,
				ExecutionState,
				NULL,
				NULL,
				0,
				FALSE);
	
	//
	// Release the state list resource
	//
	ExReleaseResourceLite(
			ProcessStateListResource);
	KeLowerIrql(OldIrql);
}

//
// Check to see if one or more execution states are set for the supplied process
// object
//
BOOLEAN IsProcessExecutionState(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG ExecutionState)
{
	PPROCESS_MEMORY_STATE Current;
	BOOLEAN               Matched = FALSE;
	KIRQL                 OldIrql;

	//
	// Acquire the process memory state list resource with shared access
	//
	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceSharedLite(
			ProcessStateListResource,
			TRUE);

	for (Current = (PPROCESS_MEMORY_STATE)ProcessStateList.Flink;
	     Current != (PPROCESS_MEMORY_STATE)&ProcessStateList;
	     Current = (PPROCESS_MEMORY_STATE)Current->Entry.Flink)
	{
		if (Current->ProcessObject != ProcessObject)
			continue;

		Matched = ((Current->ExecutionState & ExecutionState) == ExecutionState)
			? TRUE
			: FALSE;

		break;
	}

	//
	// Release the process memory state list resource
	//
	ExReleaseResourceLite(
			ProcessStateListResource);
	KeLowerIrql(OldIrql);

	return Matched;
}

//
// Sets the image set mapping that is associated with the executable from which
// the process is executing.
//
VOID SetProcessExecutableMapping(
		IN PPROCESS_OBJECT ProcessObject,
		IN PIMAGE_SET_MAPPING Mapping)
{
	PPROCESS_MEMORY_STATE Current;
	BOOLEAN               Found = FALSE;
	KIRQL                 OldIrql;

	//
	// Acquire the state list resource for exclusive access
	//
	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceExclusiveLite(
			ProcessStateListResource,
			TRUE);

	for (Current = (PPROCESS_MEMORY_STATE)ProcessStateList.Flink;
	     Current != (PPROCESS_MEMORY_STATE)&ProcessStateList;
	     Current = (PPROCESS_MEMORY_STATE)Current->Entry.Flink)
	{
		if (Current->ProcessObject == ProcessObject)
		{
			//
			// If the process state does not already has an executable mapping
			// associated with it...
			//
			if (!Current->ExecutableMapping)
			{
				if (Mapping)
					ReferenceImageSetMapping(
							Mapping);
	
				Current->ExecutableMapping = Mapping;
			}

			Found = TRUE;
			break;
		}
	}

	//
	// If we didn't find a match, create the process state.
	//
	if (!Found)
		CreateProcessState(
				ProcessObject,
				0,
				0,
				0,
				Mapping,
				NULL,
				0,
				FALSE);
	
	//
	// Release the state list resource
	//
	ExReleaseResourceLite(
			ProcessStateListResource);
	KeLowerIrql(OldIrql);
}

//
// Gets a reference to the image set mapping that is associated with the
// process's executable image file.
//
PIMAGE_SET_MAPPING GetProcessExecutableMapping(
		IN PPROCESS_OBJECT ProcessObject)
{
	PPROCESS_MEMORY_STATE Current;
	PIMAGE_SET_MAPPING    ExecutableMapping = NULL;
	KIRQL                 OldIrql;

	//
	// Acquire the process memory state list resource with shared access
	//
	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceSharedLite(
			ProcessStateListResource,
			TRUE);

	for (Current = (PPROCESS_MEMORY_STATE)ProcessStateList.Flink;
	     Current != (PPROCESS_MEMORY_STATE)&ProcessStateList;
	     Current = (PPROCESS_MEMORY_STATE)Current->Entry.Flink)
	{
		if (Current->ProcessObject != ProcessObject)
			continue;

		//
		// If this process has an executable image set mapping, grab a reference
		// to it and return it to the caller
		//
		if (Current->ExecutableMapping)
		{
			ReferenceImageSetMapping(
					Current->ExecutableMapping);

			ExecutableMapping = Current->ExecutableMapping;
		}

		break;
	}

	//
	// Release the process memory state list resource
	//
	ExReleaseResourceLite(
			ProcessStateListResource);
	KeLowerIrql(OldIrql);

	return ExecutableMapping;
}

//
// Sets the NRER image location for the supplied process object.
//
VOID SetProcessNrerDispatchTable(
		IN PPROCESS_OBJECT ProcessObject,
		IN PNRER_DISPATCH_TABLE NrerDispatchTable)
{
	PPROCESS_MEMORY_STATE Current;
	BOOLEAN               Found = FALSE;
	KIRQL                 OldIrql;

	//
	// Acquire the state list resource for exclusive access
	//
	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceExclusiveLite(
			ProcessStateListResource,
			TRUE);

	for (Current = (PPROCESS_MEMORY_STATE)ProcessStateList.Flink;
	     Current != (PPROCESS_MEMORY_STATE)&ProcessStateList;
	     Current = (PPROCESS_MEMORY_STATE)Current->Entry.Flink)
	{
		if (Current->ProcessObject == ProcessObject)
		{
			RtlCopyMemory(
					&Current->NrerDispatchTable,
					NrerDispatchTable,
					sizeof(NRER_DISPATCH_TABLE));

			Found = TRUE;
			break;
		}
	}

	//
	// If we didn't find a match, create the process state.
	//
	if (!Found)
		CreateProcessState(
				ProcessObject,
				0,
				0,
				0,
				NULL,
				NrerDispatchTable,
				0,
				FALSE);
	
	//
	// Release the state list resource
	//
	ExReleaseResourceLite(
			ProcessStateListResource);
	KeLowerIrql(OldIrql);
}

//
// Returns the base location and size of the NRER image mapping for this
// process.
//
BOOLEAN GetProcessNrerDispatchTable(
		IN PPROCESS_OBJECT ProcessObject,
		OUT PNRER_DISPATCH_TABLE NrerDispatchTable)
{
	PPROCESS_MEMORY_STATE Current;
	BOOLEAN               Found = FALSE;
	KIRQL                 OldIrql;

	//
	// Acquire the process memory state list resource with shared access
	//
	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceSharedLite(
			ProcessStateListResource,
			TRUE);

	for (Current = (PPROCESS_MEMORY_STATE)ProcessStateList.Flink;
	     Current != (PPROCESS_MEMORY_STATE)&ProcessStateList;
	     Current = (PPROCESS_MEMORY_STATE)Current->Entry.Flink)
	{
		if (Current->ProcessObject != ProcessObject)
			continue;

		//
		// If this process hasn't currently initialized the NRER dispatch table,
		// then we'll indicate that it wasn't found.
		//
		if (!Current->NrerDispatchTable.NrerImageBase)
			break;

		//
		// Otherwise, we copy the dispatch table to the output pointer and carry
		// on.
		//
		RtlCopyMemory(
				NrerDispatchTable,
				&Current->NrerDispatchTable,
				sizeof(NRER_DISPATCH_TABLE));

		Found = TRUE;

		break;
	}

	//
	// Release the process memory state list resource
	//
	ExReleaseResourceLite(
			ProcessStateListResource);
	KeLowerIrql(OldIrql);

	return Found;
}

//
// Acquire the next available randomized base address within the context of a
// given process which will be used as the starting point for the vad traversal
// when attempting to acquire a region of memory.
//
ULONG_PTR GetNextRandomizedBaseForProcess(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG RegionSize,
		IN ULONG RegionIncrement)
{
	PPROCESS_MEMORY_STATE Current;
	ULONG_PTR             RandomizedBase = 0;
	BOOLEAN               Found = FALSE;
	ULONG                 RandPadding = RngRand() & 0x30000;
	KIRQL                 OldIrql;

	//
	// We align the region size to a 16 page boundary
	// such that the next allocation will start aligned.
	//
	RegionSize = Round16Page(RegionSize) + RandPadding;

	//
	// Acquire the process memory state list resource with shared access
	//
	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceSharedLite(
			ProcessStateListResource,
			TRUE);

	for (Current = (PPROCESS_MEMORY_STATE)ProcessStateList.Flink;
	     Current != (PPROCESS_MEMORY_STATE)&ProcessStateList;
	     Current = (PPROCESS_MEMORY_STATE)Current->Entry.Flink)
	{
		if (Current->ProcessObject != ProcessObject)
			continue;

		//
		// Increment based off the region increment prior to continuing
		//
		Current->CurrentAddress += RegionIncrement;

		//
		// Round our address to a 16page aligned boundary
		//
		Current->CurrentAddress = Round16Page(Current->CurrentAddress);

		//
		// If the current start point plus the region size is takes us off into
		// kernel address space, wrap the allocation back to 0x10000
		//
		if ((Current->CurrentAddress >= (ULONG_PTR)MM_HIGHEST_USER_ADDRESS) ||
		    (Current->CurrentAddress + RegionSize >= (ULONG_PTR)MM_HIGHEST_USER_ADDRESS))
			Current->CurrentAddress = 0x10000;

		//
		// Set the randomized base address to the current start point
		//
		RandomizedBase = Current->CurrentAddress;

		//
		// Increment the start point by the amount that is to be allocated
		//
		Current->CurrentAddress += RegionSize;

		//
		// Flag that we found the process
		//
		Found = TRUE;

		break;
	}

	//
	// Release the process memory state list resource
	//
	ExReleaseResourceLite(
			ProcessStateListResource);
	KeLowerIrql(OldIrql);

	//
	// If we failed to find a randomized base due to not having any process
	// state, create that state now.
	//
	if (!Found)
		RandomizedBase = CreateProcessState(
				ProcessObject,
				RegionSize,
				FALSE,
				0,
				NULL,
				NULL,
				0,
				TRUE);

	return RandomizedBase;
}

//
// Set one or more of the process' NRE execution flags.
//
VOID SetProcessNreExecutionFlag(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG NreExecutionFlag,
		IN BOOLEAN Unset)
{
	PPROCESS_MEMORY_STATE Current;
	BOOLEAN               Found = FALSE;
	KIRQL                 OldIrql;

	//
	// Acquire the state list resource for exclusive access
	//
	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceExclusiveLite(
			ProcessStateListResource,
			TRUE);

	for (Current = (PPROCESS_MEMORY_STATE)ProcessStateList.Flink;
	     Current != (PPROCESS_MEMORY_STATE)&ProcessStateList;
	     Current = (PPROCESS_MEMORY_STATE)Current->Entry.Flink)
	{
		if (Current->ProcessObject == ProcessObject)
		{
			if (Unset)
				Current->NreExecutionFlags &= ~NreExecutionFlag;
			else
				Current->NreExecutionFlags |= NreExecutionFlag;

			Found = TRUE;
			break;
		}
	}

	//
	// If we didn't find a match, create the process state.
	//
	if (!Found)
		CreateProcessState(
				ProcessObject,
				0,
				0,
				NreExecutionFlag,
				NULL,
				NULL,
				0,
				FALSE);
	
	//
	// Release the state list resource
	//
	ExReleaseResourceLite(
			ProcessStateListResource);
	KeLowerIrql(OldIrql);
}

//
// Check to see if one or more nre execution flags are set for the supplied process
// object
//
ULONG GetProcessNreExecutionFlags(
		IN PPROCESS_OBJECT ProcessObject)
{
	PPROCESS_MEMORY_STATE Current;
	ULONG                 Flags = 0;
	KIRQL                 OldIrql;

	//
	// Acquire the process memory state list resource with shared access
	//
	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceSharedLite(
			ProcessStateListResource,
			TRUE);

	for (Current = (PPROCESS_MEMORY_STATE)ProcessStateList.Flink;
	     Current != (PPROCESS_MEMORY_STATE)&ProcessStateList;
	     Current = (PPROCESS_MEMORY_STATE)Current->Entry.Flink)
	{
		if (Current->ProcessObject != ProcessObject)
			continue;

		Flags = Current->NreExecutionFlags;

		break;
	}

	//
	// Release the process memory state list resource
	//
	ExReleaseResourceLite(
			ProcessStateListResource);
	KeLowerIrql(OldIrql);

	return Flags;
}

//
// Entry point for process creation and destruction notifications
//
static VOID CreateProcessNotifyRoutine(
		IN HANDLE ParentId,
		IN HANDLE ProcessId,
		IN BOOLEAN Create)
{
	PPROCESS_OBJECT ProcessObject = NULL;

	PsLookupProcessByProcessId(
			ProcessId,
			&ProcessObject);

	DebugPrint(("CreateProcessNotifyRoutine(): Called with ParentId=%lu ProcessId=%lu ProcessObject=%p Create=%d.",
			ParentId,
			ProcessId,
			ProcessObject,
			Create));

	//
	// If the process is ending...
	//
	if (!Create)
	{
		//
		// Remove the state associated with the supplied process object
		//
		RemoveProcessState(
				ProcessObject,
				ProcessId);
	}
	//
	// Otherwise, if the process is starting, correlate the process identifier
	// with an already initialized process state or create a new process start
	//
	else
	{
		if (ProcessObject)
			UpdateOrCreateProcessState(
					ProcessObject,
					ProcessId);
		else
		{
			DebugPrint(("CreateProcessNotifyRoutine(%lu): Failed to open process object (UpdateOrCreate).",
					ProcessId));
		}
	}

	//
	// Dereference the process object if we found one
	//
	if (ProcessObject)
		ObDereferenceObject(
				ProcessObject);
}

//
// Creates the state object for tracking the process' allocation starting 
// point
//
#define DEFAULT_PREFERRED_PADDING 0x00f24000

static ULONG_PTR CreateProcessState(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG InitialRegionSize,
		IN ULONG ExemptionFlags OPTIONAL,
		IN ULONG ExecutionState OPTIONAL,
		IN PIMAGE_SET_MAPPING ExecutableMapping OPTIONAL,
		IN PNRER_DISPATCH_TABLE NrerDispatchTable OPTIONAL,
		IN ULONG NreExecutionFlags OPTIONAL,
		IN BOOLEAN LockResource)
{
	PPROCESS_MEMORY_STATE NewState;
	PIMAGE_SET            ImageSet = NULL;
	ULONG_PTR             StartAddress = 0;
	ULONG_PTR             HighStart;
	ULONG_PTR             LowStart;
	ULONG_PTR             HighPad;
	ULONG_PTR             LowPad;
	NTSTATUS              Status;
	ULONG                 Attempts = 0;
	KIRQL                 OldIrql;

	do
	{
		//
		// Allocate storage for the new process memory state
		//
		if (!(NewState = (PPROCESS_MEMORY_STATE)ExAllocatePoolWithTag(
				NonPagedPool,
				sizeof(PROCESS_MEMORY_STATE),
				ALLOC_TAG)))
		{
			DebugPrint(("CreateProcessState(): ExAllocatePoolWithTag failed."));
			break;
		}

		//
		// Zero out the context.
		//
		RtlZeroMemory(
				NewState,
				sizeof(PROCESS_MEMORY_STATE));

		//
		// Set the new state's process object to the one that was passed in
		//
		NewState->ProcessObject = ProcessObject;

		//
		// The types of thing this process is exempted from doing
		//
		NewState->ExemptionFlags = ExemptionFlags;

		//
		// The process' execution state, if any
		//
		NewState->ExecutionState = ExecutionState;

		//
		// Set this process' NRE execution flags.
		//
		NewState->NreExecutionFlags = (!NreExecutionFlags)
			? NRE_FLAG_ENFORCE_ALL
			: NreExecutionFlags;

		//
		// Adjust the NRE execution flags based on whatever exemptions may have
		// been applied to this process.
		//
		AdjustProcessNreExecutionFlags(
				NewState);

		//
		// Set the process' executable image set mapping if it was supplied
		//
		if (ExecutableMapping)
			ReferenceImageSetMapping(
					ExecutableMapping);

		NewState->ExecutableMapping = ExecutableMapping;

		//
		// Initialize the NRER image location if it was supplied
		//
		if (NrerDispatchTable)
			RtlCopyMemory(
					&NewState->NrerDispatchTable,
					NrerDispatchTable,
					sizeof(NRER_DISPATCH_TABLE));

		//
		// Acquire the image set for this process so that we can avoid range
		// clashes with the image set low/high start points.
		//
		if (!NT_SUCCESS(Status = GetImageSetForProcess(
				ProcessObject,
				&ImageSet)))
		{
			DebugPrint(("CreateProcessState(): GetImageSetForProcess failed, %.8x.",
					Status));
			break;
		}

		//
		// Get the low and high starting addresses.  We do not need to worry about
		// locking because this only needs to be a guestimate.
		//
		LowStart  = GetImageSetLowStartAddress(
				ImageSet);
		HighStart = GetImageSetHighStartAddress(
				ImageSet);

		do
		{
			NewState->StartAddress = RngRand() & 0x7fff0000;

			//
			// Don't allow memory allocations to start before 0x00600000 to avoid
			// problems with non-reloc executables and other shit
			//
			if (NewState->StartAddress < 0x00600000)
			{
				HighPad = 0;
				LowPad  = 0;
				continue;
			}

			//
			// Do not allow randomization to be near SharedUserData
			//
			if (NewState->StartAddress >= 0x7fd00000)
			{
				HighPad = 0;
				LowPad  = 0;
				continue;
			}

			//
			// If the starting point is within the mapping region of the image set,
			// we obviously wont use this as our starting point.
			//
			if ((NewState->StartAddress < HighStart) &&
			    (NewState->StartAddress >= LowStart))
			{
				HighPad = 0;
				LowPad  = 0;
				continue;
			}

			LowPad = (NewState->StartAddress >= LowStart) 
				? NewState->StartAddress - LowStart
				: LowStart - NewState->StartAddress;
			
			HighPad = (NewState->StartAddress >= HighStart) 
				? NewState->StartAddress - HighStart
				: HighStart - NewState->StartAddress;

		} while ((Attempts++ < 256) &&
		         ((HighPad < DEFAULT_PREFERRED_PADDING) &&
		          (LowPad < DEFAULT_PREFERRED_PADDING)));

		//
		// Assert if attempts was more than 256 so that we can catch conditions
		// where we fail to find a viable region during testing
		//
		ASSERT(Attempts < 256);

		DebugPrint(("CreateProcessState(%p): Using base %p after %lu attempts (LowPad=%p HighPad=%p)",
				ProcessObject,
				NewState->StartAddress, 
				Attempts,
				LowPad,
				HighPad));

		//
		// Set the current address in the context of the memory state to the start
		// address
		//
		NewState->CurrentAddress = NewState->StartAddress;

		//
		// Set the outbound start address to what we specify
		//
		StartAddress = NewState->StartAddress;

		//
		// If an initial region size was supplied, add it to the current start
		// point
		//
		if (InitialRegionSize)
			NewState->CurrentAddress += Round16Page(InitialRegionSize);

		//
		// Acquire the state list resource for exclusive access
		//
		if (LockResource)
		{
			KeRaiseIrql(APC_LEVEL, &OldIrql);
			ExAcquireResourceExclusiveLite(
					ProcessStateListResource,
					TRUE);
		}

		//
		// Insert the new process state into the process memory state list
		//
		InsertHeadList(
				&ProcessStateList,
				(PLIST_ENTRY)NewState);
	
		//
		// Release the state list resource
		//
		if (LockResource)
		{
			ExReleaseResourceLite(
					ProcessStateListResource);
			KeLowerIrql(OldIrql);
		}

	} while (0);

	//
	// If we acquired the process' image set, dereference it now.
	//
	if (ImageSet)
		DereferenceImageSet(
				ImageSet);

	return StartAddress;
}

//
// Updates an existing process memory state or creates a new one depending on
// whether or not one already exists in the list
//
static VOID UpdateOrCreateProcessState(
		IN PPROCESS_OBJECT ProcessObject,
		IN HANDLE ProcessId)
{
	PPROCESS_MEMORY_STATE Current;
	BOOLEAN               Found = FALSE;
	KIRQL                 OldIrql;

	//
	// Acquire the state list resource for exclusive access
	//
	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceExclusiveLite(
			ProcessStateListResource,
			TRUE);

	for (Current = (PPROCESS_MEMORY_STATE)ProcessStateList.Flink;
	     Current != (PPROCESS_MEMORY_STATE)&ProcessStateList;
	     Current = (PPROCESS_MEMORY_STATE)Current->Entry.Flink)
	{
		if (Current->ProcessObject == ProcessObject)
		{
			Current->ProcessId = ProcessId;
			Found              = TRUE;

			break;
		}
	}

	//
	// If we didn't find a match, create the process state.
	//
	if (!Found)
		CreateProcessState(
				ProcessObject,
				0,
				0,
				0,
				NULL,
				NULL,
				0,
				FALSE);
	
	//
	// Release the state list resource
	//
	ExReleaseResourceLite(
			ProcessStateListResource);
	KeLowerIrql(OldIrql);
}

//
// Removes the state associated with a given process object
//
static VOID RemoveProcessState(
		IN PPROCESS_OBJECT ProcessObject,
		IN HANDLE ProcessId)
{
	PPROCESS_MEMORY_STATE Current;
	PIMAGE_SET_MAPPING    Mapping = NULL;
	KIRQL                 OldIrql;

	//
	// Acquire the state list resource for exclusive access
	//
	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceExclusiveLite(
			ProcessStateListResource,
			TRUE);

	//
	// Enumerate the process memory state list entries
	//
	for (Current = (PPROCESS_MEMORY_STATE)ProcessStateList.Flink;
	     Current != (PPROCESS_MEMORY_STATE)&ProcessStateList;
	     Current = (PPROCESS_MEMORY_STATE)Current->Entry.Flink)
	{
		//
		// No match? Continue.
		//
		if ((Current->ProcessObject != ProcessObject) &&
		    (Current->ProcessId != ProcessId))
			continue;

		//
		// Once we've found a match, remove it from the list of process memory
		// states and deallocate the associated memory as we no longer need it.
		//
		RemoveEntryList(
				(PLIST_ENTRY)Current);

		//
		// Snag the process' state's executable mapping if one was set...
		//
		Mapping = Current->ExecutableMapping;

		ExFreePool(
				Current);
		
		DebugPrint(("RemoveProcessState(%p): Expunged process memory state information.",
				ProcessObject));

		break;
	}

	//
	// Release the state list resource
	//
	ExReleaseResourceLite(
			ProcessStateListResource);
	KeLowerIrql(OldIrql);

	//
	// If the process had an executable image set mapping associated with it,
	// dereference it
	//
	if (Mapping)
		DereferenceImageSetMapping(
				Mapping);
}

//
// Get rid of exempted execution flags based on exemptions for this
// process.
//
static VOID AdjustProcessNreExecutionFlags(
		IN PPROCESS_MEMORY_STATE MemoryState)
{
	if (MemoryState->ExemptionFlags & EXEMPT_NRE_SEH)
		MemoryState->NreExecutionFlags &= ~NRE_FLAG_ENFORCE_SEH;
	else if (MemoryState->ExemptionFlags & EXEMPT_NRE_STACK)
		MemoryState->NreExecutionFlags &= ~NRE_FLAG_ENFORCE_STACK;
	else if (MemoryState->ExemptionFlags & EXEMPT_NRE_FORMAT)
		MemoryState->NreExecutionFlags &= ~NRE_FLAG_ENFORCE_FMT;
}
