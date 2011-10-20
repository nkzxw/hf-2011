/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Precomp.h"

typedef struct _THREAD_EXEMPTION
{
	LIST_ENTRY Entry;
	PVOID      Thread;
} THREAD_EXEMPTION, *PTHREAD_EXEMPTION;

typedef struct _REGION_EXEMPTION
{
	LIST_ENTRY Entry;
	ULONG_PTR  Base;
	ULONG      Size;
} REGION_EXEMPTION, *PREGION_EXEMPTION;

static PTHREAD_EXEMPTION FindThreadExemption(
		IN PVOID Thread,
		IN BOOLEAN LockList);
static PULONG GetCounterFromCounterType(
		IN COUNTER_TYPE Type);

static VOID ReadConfiguration(
		OUT PULONG EnabledRandomizationSubsystems);
static VOID SaveEnabledRandomizationSubsystems();

////
//
// Globals
//
////

//
// Exemptions
//
static LIST_ENTRY ThreadExemptionList;
static FAST_MUTEX ThreadExemptionListMutex;

static LIST_ENTRY RegionExemptionList;
static FAST_MUTEX RegionExemptionListMutex;

//
// Configuration flags
//
static ULONG EnabledRandomizationSubsystems = 0;

//
// Counters
//
static ULONG NumberOfRandomizedDlls         = 0;
static ULONG NumberOfImageSets              = 0;
static ULONG NumberOfRandomizationsExempted = 0;
static ULONG NumberOfRandomizedAllocations  = 0;

#pragma code_seg("INIT")

//
// Initialize the exemption subsystem
//
VOID InitializeExecutive()
{
	ULONG ConfigEnabledSubsystems = 0;

	//
	// Initialize the exemption list and synchronization objects
	//
	InitializeListHead(
			&ThreadExemptionList);
	ExInitializeFastMutex(
			&ThreadExemptionListMutex);

	//
	// Initialize the region exemption list and synchronization objects
	//
	InitializeListHead(
			&RegionExemptionList);
	ExInitializeFastMutex(
			&RegionExemptionListMutex);

	//
	// Initialize the randomization state based on what's been defined in the
	// configuration registry key
	//
	ReadConfiguration(
			&ConfigEnabledSubsystems);

	DebugPrint(("InitializeExecutive(): CONFIG: ConfigEnabledSubsystems mask is %.8x.",
			ConfigEnabledSubsystems));

	EnableRandomizationSubsystems(
			ConfigEnabledSubsystems,
			TRUE);
}

#pragma code_seg()

//
// Adds a thread to the list of exempted threads
//
BOOLEAN AddThreadExemption(
		IN PVOID Thread)
{
	PTHREAD_EXEMPTION NewExemption = NULL;
	BOOLEAN           Success = FALSE;

	ExAcquireFastMutex(
			&ThreadExemptionListMutex);

	do
	{
		// 
		// Allocate storage for the exemption
		//
		if (!(NewExemption = (PTHREAD_EXEMPTION)ExAllocatePoolWithTag(
				NonPagedPool,
				sizeof(THREAD_EXEMPTION),
				ALLOC_TAG)))
			break;

		//
		// Initialize and insert the exemption into the list
		//
		NewExemption->Thread = Thread;

		InsertHeadList(
				&ThreadExemptionList,
				(PLIST_ENTRY)NewExemption);

		Success = TRUE;

	} while (0);

	ExReleaseFastMutex(
			&ThreadExemptionListMutex);

	return Success;
}

//
// Checks to see if a given thread is exempted
//
BOOLEAN IsThreadExempted(
		IN PVOID Thread)
{
	return (FindThreadExemption(
				Thread,
				TRUE))
		? TRUE
		: FALSE;
}

//
// Removes a thread from the list of exempted threads
//
VOID RemoveThreadExemption(
		IN PVOID Thread)
{
	PTHREAD_EXEMPTION Exemption;

	ExAcquireFastMutex(
			&ThreadExemptionListMutex);

	if ((Exemption = FindThreadExemption(
			Thread,
			FALSE)))
	{
		//
		// Remove the entry from the list of exemptions and deallocate its context
		//
		RemoveEntryList((PLIST_ENTRY)Exemption);

		ExFreePool(Exemption);
	}
	
	ExReleaseFastMutex(
			&ThreadExemptionListMutex);
}

//
// Find a thread exemption in the list of exemptions and return its pointer
//
PTHREAD_EXEMPTION FindThreadExemption(
		IN PVOID Thread,
		IN BOOLEAN LockList)
{
	PTHREAD_EXEMPTION Exemption = NULL;
	PLIST_ENTRY       Current;

	if (LockList)
		ExAcquireFastMutex(
				&ThreadExemptionListMutex);

	for (Current = ThreadExemptionList.Flink;
	     Current != &ThreadExemptionList;
	     Current = Current->Flink)
	{
		PTHREAD_EXEMPTION CurrentExemption = (PTHREAD_EXEMPTION)Current;

		//
		// If the current entry's thread matches the one that is requested, return
		// its pointer
		//
		if (CurrentExemption->Thread == Thread)
		{
			Exemption = CurrentExemption;
			break;
		}
	}
	
	if (LockList)
		ExReleaseFastMutex(
				&ThreadExemptionListMutex);

	return Exemption;
}

//
// Adds a region to the region exemption list such that it will not be
// randomized into.
//
BOOLEAN AddRegionExemption(
		IN PVOID RegionBase,
		IN ULONG RegionSize)
{
	PREGION_EXEMPTION NewExemption = NULL;
	BOOLEAN           Result = FALSE;

	do
	{
		//
		// Allocate storage for the new region exemption
		//
		if (!(NewExemption = (PREGION_EXEMPTION)ExAllocatePoolWithTag(
				NonPagedPool,
				sizeof(REGION_EXEMPTION),
				ALLOC_TAG)))
			break;

		NewExemption->Base = (ULONG_PTR)RegionBase;
		NewExemption->Size = RegionSize;

		//
		// Acquire the region exemption list mutex
		//
		ExAcquireFastMutex(
				&RegionExemptionListMutex);

		//
		// Insert the region exemption into the exemption list
		//
		InsertHeadList(
				&RegionExemptionList,
				(PLIST_ENTRY)NewExemption);

		//
		// Release the region exemption list mutex
		//
		ExReleaseFastMutex(
				&RegionExemptionListMutex);

		Result = TRUE;

	} while (0);

	return Result;
}

//
// Checks to see if a supplied region is exempted from being allocated into
//
BOOLEAN IsRegionExempted(
		IN PVOID RegionBase,
		IN ULONG RegionSize,
		OUT PULONG EndDisplacement OPTIONAL)
{
	PLIST_ENTRY Current;
	BOOLEAN     Exempted = FALSE;

	//
	// Acquire the region exemption list mutex
	//
	ExAcquireFastMutex(
			&RegionExemptionListMutex);

	//
	// Enumerate through all of the region exemptions
	//
	for (Current = RegionExemptionList.Flink;
	     (Current != &RegionExemptionList) && (!Exempted);
	     Current = Current->Flink)
	{
		PREGION_EXEMPTION Exemption = (PREGION_EXEMPTION)Current;

		Exempted = (
			(IsAddressInsideRange(
				RegionBase,
				Exemption->Base,
				Exemption->Size)) ||
			(IsAddressInsideRange(
				Exemption->Base,
				RegionBase,
				RegionSize))) ? TRUE : FALSE;

		// 
		// If this region is exempted and the caller requested a displacement
		// calculation, pass one back to them such that they can optimize their
		// search by knowing how much to add to their region base to get outside
		// of the exempted region.
		//
		if ((Exempted) &&
		    (EndDisplacement))
		{
			//
			// The number of bytes that will have to be incremented by to get the
			// supplied region out of the area of the exempted region is calculated
			// by subtracting the end of the exempted region from the start of the
			// supplied region.
			//
			*EndDisplacement = (ULONG)((Exemption->Base + Exemption->Size) - (ULONG_PTR)RegionBase);

			//
			// Assert that the end of the exempted region is always greater than
			// the region base, which should always be true.
			//
			ASSERT((Exemption->Base + Exemption->Size) > (ULONG_PTR)RegionBase);
		}
	}

	//
	// Release the region exemption list mutex
	//
	ExReleaseFastMutex(
			&RegionExemptionListMutex);

	return Exempted;
}

//
// Removes a region from the exemption list 
//
VOID RemoveRegionExemption(
		IN PVOID RegionBase,
		IN ULONG RegionSize)
{
	PREGION_EXEMPTION Exemption;
	PLIST_ENTRY       Current;

	ExAcquireFastMutex(
			&RegionExemptionListMutex);

	//
	// Enumerate through all the region exemptions looking for the supplied
	// region such that it can be removed from the exemption list
	//
	for (Current = RegionExemptionList.Flink;
	     Current != &RegionExemptionList;
	     Current = Current->Flink)
	{
		PREGION_EXEMPTION Exemption = (PREGION_EXEMPTION)Current;

		if ((Exemption->Base != (ULONG_PTR)RegionBase) ||
		    (Exemption->Size != RegionSize))
			continue;

		//
		// Remove the entry from the list of exemptions and deallocate its context
		//
		RemoveEntryList(
				(PLIST_ENTRY)Exemption);

		ExFreePool(
				Exemption);

		break;
	}
	
	ExReleaseFastMutex(
			&RegionExemptionListMutex);
}

//
// Enables one or more randomization subsystem
//
VOID EnableRandomizationSubsystems(
		IN ULONG RandomizationSubsystems, 
		IN BOOLEAN Save)
{
	RtlInterlockedSetBits(
			&EnabledRandomizationSubsystems,
			RandomizationSubsystems);

	//
	// Save the enabled randomization subsystems to the registry for use at next
	// boot
	//
	if (Save)
		SaveEnabledRandomizationSubsystems();
}

//
// Checks to see if the supplied subsystems are currently enabled for
// randomization
//
BOOLEAN IsRandomizationSubsystemEnabled(
		IN ULONG RandomizationSubsystems)
{
	return ((EnabledRandomizationSubsystems & RandomizationSubsystems) == RandomizationSubsystems) 
		? TRUE
		: FALSE;
}

//
// Disables the supplied randomization subsystems
//
VOID DisableRandomizationSubsystems(
		IN ULONG RandomizationSubsystems)
{
	RtlInterlockedClearBits(
			&EnabledRandomizationSubsystems,
			RandomizationSubsystems);

	//
	// Save the enabled randomization subsystems to the registry for use at next
	// boot
	//
	SaveEnabledRandomizationSubsystems();
}

//
// Returns the address in memory of the counter that is associated with the
// supplied counter type
//
PULONG GetCounterFromCounterType(
		IN COUNTER_TYPE Type)
{
	PULONG Counter = NULL;

	switch (Type)
	{
		case RandomizedDllCounter:          Counter = &NumberOfRandomizedDlls; break;
		case ImageSetCounter:               Counter = &NumberOfImageSets; break;
		case RandomizationsExemptedCounter: Counter = &NumberOfRandomizationsExempted; break;
		case RandomizedAllocationsCounter:  Counter = &NumberOfRandomizedAllocations; break;
	}

	return Counter;
}

//
// Increment an executive counter
//
VOID IncrementExecutiveCounter(
		IN COUNTER_TYPE Type,
		IN ULONG Increment OPTIONAL)
{
	PULONG Counter;

	if ((Counter = GetCounterFromCounterType(Type)))
	{
		if (!Increment)
			InterlockedIncrement(
					Counter);
		else
			InterlockedExchangeAdd(
					Counter,
					Increment);
	}
}

//
// Decrement an executive counter, optionally by a given interval by a 
// given interval
//
VOID DecrementExecutiveCounter(
		IN COUNTER_TYPE Type,
		IN ULONG Decrement OPTIONAL)
{
	PULONG Counter;

	//
	// Add the negative value of the decrement
	//
	if ((Counter = GetCounterFromCounterType(Type)))
	{
		if (!Decrement)
			InterlockedDecrement(
					Counter);
		else
			InterlockedExchangeAdd(
					Counter,
					-((LONG)Decrement));
	}
}

//
// Reset the value of a given counter
//
VOID ResetExecutiveCounter(
		IN COUNTER_TYPE Type)
{
	PULONG Counter;

	if ((Counter = GetCounterFromCounterType(Type)))
		*Counter = 0;
}

//
// Returns information about the executive state of the driver in the output
// pointer passed in
//
NTSTATUS GetExecutiveStatistics(
		OUT PWEHNTRUST_STATISTICS Statistics)
{
	//
	// Zero out the context at this point
	//
	RtlZeroMemory(
		Statistics,
		sizeof(WEHNTRUST_STATISTICS));

	//
	// Store the current statistics
	//
	Statistics->Enabled                        = EnabledRandomizationSubsystems;
	Statistics->NumberOfImageSets              = NumberOfImageSets;
	Statistics->NumberOfRandomizedDlls         = NumberOfRandomizedDlls;
	Statistics->NumberOfRandomizationsExempted = NumberOfRandomizationsExempted;
	Statistics->NumberOfRandomizedAllocations  = NumberOfRandomizedAllocations;

	return STATUS_SUCCESS;
}

//
// Gets the mask of randomization subsystems that are enabled from the
// configuration registry key.  If no enabled subsystems have been defined the
// default is to enabled all randomization subsystems.
//
VOID ReadConfiguration(
		OUT PULONG EnabledRandomizationSubsystems)
{
	PKEY_VALUE_PARTIAL_INFORMATION ValueInformation;
	OBJECT_ATTRIBUTES              Attributes;
	UNICODE_STRING                 ConfigKeyPath, ValueName;
	NTSTATUS                       Status;
	PUCHAR                         Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
	HANDLE                         ConfigKey = NULL;
	ULONG                          SavedEnabledRandomizationSubsystems = RANDOMIZATION_SUBSYSTEM_ALL;
	ULONG                          ValueInformationSize = sizeof(Buffer);

	ValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;

	do
	{
		//
		// Initialize the configuration unicode string
		//
		RtlInitUnicodeString(
				&ConfigKeyPath,
				WEHNTRUST_REGISTRY_CONFIG_PATH);

		InitializeObjectAttributes(
				&Attributes,
				&ConfigKeyPath,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);

		//
		// Open the configuration key
		//
		if (!NT_SUCCESS(Status = ZwOpenKey(
				&ConfigKey,
				STANDARD_RIGHTS_READ | STANDARD_RIGHTS_WRITE,
				&Attributes)))
		{
			DebugPrint(("ReadConfiguration(): ZwOpenKey failed, %.8x.",
					Status));
			break;
		}

		//
		// Query for the value of the enabled subsystem mask
		//
		RtlInitUnicodeString(
				&ValueName,
				L"EnabledRandomizationSubsystems");

		if (!NT_SUCCESS(Status = ZwQueryValueKey(
				ConfigKey,
				&ValueName,
				KeyValuePartialInformation,
				ValueInformation,
				ValueInformationSize,
				&ValueInformationSize)))
		{
			DebugPrint(("ReadConfiguration(): ZwQueryValueKey failed, %.8x.",
					Status));
			break;
		}

		//
		// Make sure the value's type is a REG_DWORD
		//
		if (ValueInformation->Type != REG_DWORD)
		{
			DebugPrint(("ReadConfiguration(): The value is not a DWORD, %d.",
					ValueInformation->Type));
			break;
		}

		//
		// Set the enabled randomization subsystems based on what we found
		//
		SavedEnabledRandomizationSubsystems = *(PULONG)ValueInformation->Data;

	} while (0);

	//
	// Close the configuration key if we opened it
	//
	if (ConfigKey)
		ZwClose(
				ConfigKey);

	//
	// Set the output parameters that were requested.
	//
	if (EnabledRandomizationSubsystems)
		*EnabledRandomizationSubsystems = SavedEnabledRandomizationSubsystems;
}

//
// Saves the enabled randomization subsystems bitvector to the registry such
// that it can be referenced the next time the computer is started.
//
VOID SaveEnabledRandomizationSubsystems()
{
	OBJECT_ATTRIBUTES Attributes;
	UNICODE_STRING    ConfigKeyPath, ValueName;
	NTSTATUS          Status;
	HANDLE            ConfigKey = NULL;

	do
	{
		//
		// Initialize the configuration unicode string
		//
		RtlInitUnicodeString(
				&ConfigKeyPath,
				WEHNTRUST_REGISTRY_CONFIG_PATH);

		InitializeObjectAttributes(
				&Attributes,
				&ConfigKeyPath,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);

		//
		// Open the configuration key
		//
		if (!NT_SUCCESS(Status = ZwOpenKey(
				&ConfigKey,
				STANDARD_RIGHTS_WRITE,
				&Attributes)))
		{
			DebugPrint(("SaveEnabledRandomizationSubsystems(): ZwOpenKey failed, %.8x.",
					Status));
			break;
		}

		//
		// Save the current value of the enabled randomization subsystems
		// bitvector
		//
		RtlInitUnicodeString(
				&ValueName,
				L"EnabledRandomizationSubsystems");

		if (!NT_SUCCESS(Status = ZwSetValueKey(
				ConfigKey,
				&ValueName,
				0,
				REG_DWORD,
				&EnabledRandomizationSubsystems,
				sizeof(ULONG))))
		{
			DebugPrint(("SaveEnabledRandomizationSubsystems(): ZwSetValueKey failed, %.8x.",
					Status));
			break;
		}

	} while (0);

	//
	// Close the configuration key if we opened it
	//
	if (ConfigKey)
		ZwClose(
				ConfigKey);
}
