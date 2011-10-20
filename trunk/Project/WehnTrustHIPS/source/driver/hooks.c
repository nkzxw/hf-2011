/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "precomp.h"

//
// Prototypes
//
NTSTATUS MmMapViewOfSectionHook(
		IN PSECTION_OBJECT SectionObject,
		IN PPROCESS_OBJECT ProcessObject,
		IN OUT PVOID *BaseAddress,
		IN ULONG ZeroBits,
		IN ULONG CommitSize,
		IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
		IN OUT PSIZE_T ViewSize,
		IN SECTION_INHERIT InheritDisposition,
		IN ULONG AllocationType,
		IN ULONG Protect);
NTSTATUS ZwAllocateVirtualMemoryHook(
		IN HANDLE ProcessHandle,
		IN OUT PVOID *BaseAddress,
		IN ULONG ZeroBits,
		IN OUT PULONG RegionSize,
		IN ULONG AllocationType,
		IN ULONG Protect);
NTSTATUS ZwSetSystemInformationHook(
		IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
		IN OUT PVOID SystemInformation,
		IN ULONG SystemInformationLength);
NTSTATUS NtCreateThreadHook(
		OUT PHANDLE ThreadHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN HANDLE ProcessHandle,
		OUT PCLIENT_ID ClientId,
		IN PCONTEXT ThreadContext,
		IN PUSER_STACK UserStack,
		IN BOOLEAN CreateSuspended);

typedef NTSTATUS (NTAPI *MmMapViewOfSectionProc)(
		IN PSECTION_OBJECT SectionObject,
		IN PPROCESS_OBJECT ProcessObject,
		IN OUT PVOID *BaseAddress,
		IN ULONG ZeroBits,
		IN ULONG CommitSize,
		IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
		IN OUT PSIZE_T ViewSize,
		IN SECTION_INHERIT InheritDisposition,
		IN ULONG AllocationType,
		IN ULONG Protect);
typedef NTSTATUS (NTAPI *ZwAllocateVirtualMemoryProc)(
		IN HANDLE ProcessHandle,
		IN OUT PVOID *BaseAddress,
		IN ULONG ZeroBits,
		IN OUT PULONG RegionSize,
		IN ULONG AllocationType,
		IN ULONG Protect);
typedef NTSTATUS (NTAPI *ZwSetSystemInformationProc)(
		IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
		IN OUT PVOID SystemInformation,
		IN ULONG SystemInformationLength);
typedef NTSTATUS (NTAPI *NtCreateThreadProc)(
		OUT PHANDLE ThreadHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN HANDLE ProcessHandle,
		OUT PCLIENT_ID ClientId,
		IN PCONTEXT ThreadContext,
		IN PUSER_STACK UserStack,
		IN BOOLEAN CreateSuspended);

//
// Globals
//
PCALL_LAYER MmMapViewOfSectionCallLayer      = NULL;
PCALL_LAYER ZwAllocateVirtualMemoryCallLayer = NULL;
PCALL_LAYER ZwSetSystemInformationCallLayer  = NULL;
PCALL_LAYER NtCreateThreadCallLayer          = NULL;

//
// Dependent symbols
//
NT_PROTECT_VIRTUAL_MEMORY NtProtectVirtualMemory = NULL;
NT_RESUME_THREAD          NtResumeThread = NULL;

//
// This table contains symbols that are depended upon by one or more hook
// routine (or perhaps elsewhere in the driver).
//
struct 
{
	PCHAR SymbolName;
	PVOID *SymbolAddress;
} DependentSymbolTable[] =
{
	//
	// Dependended on by NtCreateThreadHook
	//
	{ "NtProtectVirtualMemory", (PVOID *)&NtProtectVirtualMemory   },
	{ "NtResumeThread",         (PVOID *)&NtResumeThread           },
	{ NULL,                     NULL                               },
};

#pragma code_seg("INIT")

//
// Hooks the following routines for randomization purposes:
//
//   - MmMapViewOfSection
//   - ZwAllocateVirtualMemory
//   - ZwSetSystemInformation
//
NTSTATUS InstallHooks()
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING    ConfigKeyPath;
	UNICODE_STRING    ValueName;
	UNICODE_STRING    SymbolName;
	NTSTATUS          Status;
	HANDLE            ConfigKey = NULL;
	ULONG             ValueLength;
	PVOID             SymbolAddress;
	
	do
	{
#if !defined(DISABLE_LIBRARY_RANDOMIZATION) || !defined(DISABLE_STACKHEAP_RANDOMIZATION)
		//
		// Hook MmMapViewOfSection
		//
		RtlInitUnicodeString(
				&SymbolName,
				L"MmMapViewOfSection");

		if (!(SymbolAddress = MmGetSystemRoutineAddress(
				&SymbolName)))
		{
			DebugPrint(("InstallHooks(): Failed to resolve MmMapViewOfSection."));

			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (!NT_SUCCESS(Status = InstallCallLayer(
				(ULONG_PTR)SymbolAddress,
				(ULONG_PTR)MmMapViewOfSectionHook,
				&MmMapViewOfSectionCallLayer)))
		{
			DebugPrint(("InstallHooks(): InstallCallLayer on MmMapViewOfSection failed, %.8x.", 
					Status));
			break;
		}

		//
		// Hook ZwSetSystemInformation
		//
		RtlInitUnicodeString(
				&SymbolName,
				L"ZwSetSystemInformation");

		if (!(SymbolAddress = MmGetSystemRoutineAddress(
				&SymbolName)))
		{
			DebugPrint(("InstallHooks(): Failed to resolve ZwSetSystemInformation."));

			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (!NT_SUCCESS(Status = InstallCallLayer(
				(ULONG_PTR)SymbolAddress,
				(ULONG_PTR)ZwSetSystemInformationHook,
				&ZwSetSystemInformationCallLayer)))
		{
			DebugPrint(("InstallHooks(): InstallCallLayer on ZwSetSystemInformation failed, %.8x.", 
					Status));
			break;
		}

#else
		DebugPrint(("InstallHooks(): Library and Stack/Heap randomization is disabled."));
#endif

#ifndef DISABLE_STACKHEAP_RANDOMIZATION
		//
		// Hook NtAllocateVirtualMemory
		//
		RtlInitUnicodeString(
				&SymbolName,
				L"NtAllocateVirtualMemory");

		if (!(SymbolAddress = MmGetSystemRoutineAddress(
				&SymbolName)))
		{
			DebugPrint(("InstallHooks(): Failed to resolve NtAllocateVirtualMemory."));

			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (!NT_SUCCESS(Status = InstallCallLayer(
				(ULONG_PTR)SymbolAddress,
				(ULONG_PTR)ZwAllocateVirtualMemoryHook,
				&ZwAllocateVirtualMemoryCallLayer)))
		{
			DebugPrint(("InstallHooks(): InstallCallLayer on ZwAllocateVirtualMemory failed, %.8x.", 
					Status));
			break;
		}
#else
		DebugPrint(("InstallHooks(): Stack/Heap randomization is disabled."));
#endif

	} while (0);

	//
	// If the config key was opened, close it
	//
	if (ConfigKey)
		ZwClose(ConfigKey);

	return Status;
}

#pragma code_seg()

//
// Unhooks the functions hooked by InstallHooks.
//
NTSTATUS UninstallHooks()
{
	do
	{
#ifndef DISABLE_LIBRARY_RANDOMIZATION
		if (MmMapViewOfSectionCallLayer)
			UninstallCallLayer(
					MmMapViewOfSectionCallLayer);
		if (ZwSetSystemInformationCallLayer)
			UninstallCallLayer(
					ZwSetSystemInformationCallLayer);
#endif

#ifndef DISABLE_STACKHEAP_RANDOMIZATION
		if (ZwAllocateVirtualMemoryCallLayer)
			UninstallCallLayer(
					ZwAllocateVirtualMemoryCallLayer);
#endif

		if (NtCreateThreadCallLayer)
			UninstallCallLayer(
					NtCreateThreadCallLayer);

		MmMapViewOfSectionCallLayer      = NULL;
		ZwSetSystemInformationCallLayer  = NULL;
		ZwAllocateVirtualMemoryCallLayer = NULL;
		NtCreateThreadCallLayer          = NULL;

	} while (0);

	return STATUS_SUCCESS;
}

//
// Returns whether or not NtCreateThread has been hooked yet.  If this is the
// free version, this function always returns true because NtCreateThread is not
// hooked.
//
BOOLEAN IsNtCreateThreadHooked()
{
	return NtCreateThreadCallLayer ? TRUE : FALSE;
}

//
// Installs the NtCreateThread hook using the supplied user-mode NtCreateThread
// routine (from which the system call index is extracted).
//
// NOTE:
// This routine must be called in a __try / __except block as it will make
// reference of user-mode addresses.  With that said, this routine will
// currently only run the first time NTDLL is loaded which occurs in a trusted
// context.
//
NTSTATUS InstallNtCreateThreadHook(
		IN PVOID NtdllImageBase,
		IN ULONG NtdllViewSize,
		IN PVOID UserModeNtCreateThread)
{
	ULONG_PTR SymbolAddress = 0;
	NTSTATUS  Status;
	ULONG     Index;

	//
	// Resolve dependent symbols that are needed for NtCreateThread to succeed.
	//
	for (Index = 0;
	     DependentSymbolTable[Index].SymbolName;
	     Index++)
	{
		PVOID UserModeAddress = NULL;

		//
		// If the symbol is already resolved, then we shall not do it again.
		//
		if (*DependentSymbolTable[Index].SymbolAddress)
			continue;

		//
		// Resolve the user-mode symbol address.  If we fail, break out.
		//
		if (!NT_SUCCESS(Status = LdrGetProcAddress(
				(ULONG_PTR)NtdllImageBase,
				NtdllViewSize,
				DependentSymbolTable[Index].SymbolName,
				&UserModeAddress)))
			break;

		//
		// Next, get the kernel-mode symbol address for the routine associated
		// with this dependent symbol.
		//
		if (!NT_SUCCESS(Status = GetSystemCallRoutine(
				UserModeAddress,
				0,
				(PULONG_PTR)DependentSymbolTable[Index].SymbolAddress)))
			break;
	}

	//
	// If the symbol name at the current index is valid, it's an indication that
	// our loop did not complete.  Log such.
	//
	if (DependentSymbolTable[Index].SymbolName)
	{
		DebugPrint(("InstallNtCreateThreadHook(): Failed to resolve dependent symbol: %s (%.8x).",
				DependentSymbolTable[Index].SymbolName,
				Status));
		return Status;
	}

	//
	// Try to resolve the kernel-mode address for the supplied system call.  If
	// we get here, all the dependent symbols that will be needed within the hook
	// routine will have been successfully resolved.
	//
	if (NT_SUCCESS(Status = GetSystemCallRoutine(
			UserModeNtCreateThread,
			0,
			&SymbolAddress)))
	{
		//
		// Install the call layer.
		//
		if (!NT_SUCCESS(Status = InstallCallLayer(
				(ULONG_PTR)SymbolAddress,
				(ULONG_PTR)NtCreateThreadHook,
				&NtCreateThreadCallLayer)))
		{
			DebugPrint(("InstallNtCreateThreadHook(): InstallCallLayer on NtCreateThread failed, %.8x.",
					Status));
		}
	}
	else
	{
		DebugPrint(("InstallNtCreateThreadHook(): GetSystemCallRoutine(%p) failed, %.8x.",
				UserModeNtCreateThread,
				Status));
	}

	return Status;
}

//
// Hook implementation of MmMapViewOfSection.  This function is responsible for
// the randomization and relocation of library images, mapped memory regions,
// and non-image file mappings.
//
NTSTATUS MmMapViewOfSectionHook(
		IN PSECTION_OBJECT SectionObject,
		IN PPROCESS_OBJECT ProcessObject,
		IN OUT PVOID *BaseAddress,
		IN ULONG ZeroBits,
		IN ULONG CommitSize,
		IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
		IN OUT PSIZE_T ViewSize,
		IN SECTION_INHERIT InheritDisposition,
		IN ULONG AllocationType,
		IN ULONG Protect)
{
	PIMAGE_SET_MAPPING Mapping = NULL;
	PSECTION_OBJECT    NewSectionObject = NULL;
	PSECTION_OBJECT    OrigSectionObject = SectionObject;
	NTSTATUS           Status = STATUS_UNSUCCESSFUL;
	BOOLEAN            CreateRegionExemption = FALSE;
	BOOLEAN            Randomized = FALSE;

	//
	// Check to see if the section object is file-backed and it's an image
	// section
	//
	if ((GetSectionObjectControlAreaFlags(
			SectionObject) & CONTROL_AREA_FLAG_IMAGE) &&
		 (GetSectionObjectControlAreaFilePointer(
			SectionObject)))
	{
#ifndef DISABLE_LIBRARY_RANDOMIZATION
		ULONG_PTR NewBase = 0;
		BOOLEAN   CreateTransferAddressJump = TRUE;
		//
		// Get a randomized instance of the supplied section object
		//
		Status = GetRandomizedImageMapping(
				SectionObject,
				ProcessObject,
				*ViewSize,
				&NewSectionObject,
				&Mapping);

		if (NT_SUCCESS(Status))
		{
			DebugPrint(("MmMapViewOfSectionHook(): Got randomized section %p for %wZ (orig=%p).",
					NewSectionObject,
					&GetSectionObjectControlAreaFilePointer(
						SectionObject)->FileName,
					SectionObject));

			NewBase = (ULONG_PTR)GetSectionObjectSegmentBasedAddress(
				NewSectionObject);

#ifndef DISABLE_NRE_RANDOMIZATION
			//
			// If NRE is enabled for this mapping, allocate memory at the original
			// image base with PAGE_NOACCESS such that mirroring can be simulated
			// in user-mode.
			//
			if ((Mapping->NreMirrorEnabled) &&
			    (Mapping->NreOriginalImageBase) &&
			    (Mapping->NreOriginalImageSize))
			{
				KAPC_STATE ApcState;
				BOOLEAN    Attached = FALSE;
				PVOID      MirrorBase = (PVOID)Mapping->NreOriginalImageBase;
				ULONG      MirrorSize = Mapping->NreOriginalImageSize;

				//
				// If the target process does not equal the current process, attach
				// to it
				//
				if (ProcessObject != IoGetCurrentProcess())
				{
					KeStackAttachProcess(
							ProcessObject,
							&ApcState);

					Attached = TRUE;
				}

				DebugPrint(("MmMapViewOfSectionHook(): Creating mirrored region at %.8x/%lu for process %p.",
						MirrorBase,
						MirrorSize,
						ProcessObject));

				//
				// If we fail to create the mirrored region, disable the
				// randomization of this image and fall back to using the original
				// image.  The is to prevent very bad crashes that would result from
				// trying to execute an image that had been "relocated" but does not
				// have relocation information.
				//
				if (!NT_SUCCESS(Status = ZwAllocateVirtualMemory(
						(HANDLE)-1,
						(PVOID *)&MirrorBase,
						0,
						&MirrorSize,
						MEM_COMMIT | MEM_RESERVE,
						PAGE_NOACCESS)))
				{
					DebugPrint(("MmMapViewOfSectionHook(): Failed to create mirrored region at %.8x, status %.8x.",
							MirrorBase,
							Status));

					//
					// Lose our reference to the randomized section object and set it
					// to NULL so that randomization will not be attempted
					//
					ObDereferenceObject(
							NewSectionObject);

					NewSectionObject = NULL;

					//
					// Unset the flag that indicates that NRER is needed for this
					// process since we're unable to create the mirrored region
					//
					SetProcessExecutionState(
							ProcessObject,
							PROCESS_EXECUTION_STATE_NRER_MIRRORING_NEEDED,
							FALSE);
				}
				//
				// Otherwise, if we successfully create the mirrored region, do not
				// create the transfer address jump table entry because it's not
				// necessary.
				//
				else
				{
					CreateTransferAddressJump = FALSE;

					//
					// If we successfully created the mirrored region then everything
					// is still on track for using NRER for this process.  As such,
					// we need to associate the process with the executable image
					// mapping so that we can use this information to set up the
					// mirroring and initialize NRER.dll
					//
					SetProcessExecutableMapping(
							ProcessObject,
							Mapping);
				}

				//
				// If we were attached, detach
				//
				if (Attached)
					KeUnstackDetachProcess(
							&ApcState);
			}
#endif

			// 
			// If we're randomizing an executable we have to adjust the
			// ImageInformation.TransferAddress to point to the randomized entry
			// point of the executable instead of the old one.  We have to do this
			// on the OldSectionObject because things like RtlCreateUserProcess and
			// friends will call NtQuerySection to get information about the image
			// file (including the entry point).
			//
			// Only do this if it's necessary to also create the transfer address
			// jump.  It's not necessary to do this if we're doing a mirror against
			// the original address region.
			//
			if ((Mapping->IsExecutable) &&
			    (CreateTransferAddressJump))
			{
				PSECTION_IMAGE_INFORMATION ImageInformation;
				ULONG_PTR                  OldBase = 0;
				ULONG_PTR                  CurrentTransferAddress = 0;
				ULONG_PTR                  NewTransferAddress = 0;
				ULONG_PTR                  OldTransferAddress = 0;
				ULONG                      EntryPointOffset = 0;
				ULONG                      SectionViewSize = *ViewSize;

				if (!SectionViewSize)
					SectionViewSize = GetSectionObjectSegmentSize(
							SectionObject)->LowPart;

				ImageInformation = GetSectionObjectSegmentImageInformation(
						SectionObject);

				CurrentTransferAddress = (ULONG_PTR)ImageInformation->TransferAddress;
				OldBase = (ULONG_PTR)GetSectionObjectSegmentBasedAddress(
						SectionObject);

				// 
				// Calculate the address of the old and new (randomized) entry point
				//
				if ((CurrentTransferAddress >= OldBase) &&
				    (CurrentTransferAddress < OldBase + SectionViewSize))
					EntryPointOffset = CurrentTransferAddress - OldBase;
				else
					EntryPointOffset = CurrentTransferAddress - NewBase;

				OldTransferAddress = OldBase + EntryPointOffset;
				NewTransferAddress = NewBase + EntryPointOffset;

				DebugPrint(("OldBase=%p NewBase=%p OldTransferAddress=%p NewTransferAddress=%p CurrentTransferAddress=%p",
						OldBase, 
						NewBase, 
						OldTransferAddress, 
						NewTransferAddress, 
						CurrentTransferAddress));

				//
				// Build a jump table mapping for the old entry point to the new
				// entry point.  This is necessary because CreateProcessInternalW
				// calls NtQuerySection prior to mapping the image, thus we will not
				// have updated the TransferAddress by the time the caller obtains
				// it.
				//
				// This is not necessary for things created through
				// RtlCreateUserProcess as it queries in the opposite order.
				//
				if (!NT_SUCCESS(Status = BuildJumpTableSingleMapping(
						ProcessObject,
						(PVOID)OldTransferAddress,
						(PVOID)NewTransferAddress)))
				{
					DebugPrint(("MmMapViewOfSectionHook(): BuildJumpTableSingleMapping failed, %.8x.", 
							Status));
				}

				// 
				// Update the transfer address for future references
				//
				if (CurrentTransferAddress != NewTransferAddress)
					ImageInformation->TransferAddress = (PVOID)NewTransferAddress;
			}

			//
			// Change the section object to the randomized section object such that
			// the mapping will use the randomized section object
			//
			if (NewSectionObject)
			{
				SectionObject = NewSectionObject;
				*BaseAddress  = (PVOID)NewBase;
				Randomized    = TRUE;
			}
		}
#endif
	}
#ifndef DISABLE_STACKHEAP_RANDOMIZATION
	//
	// If the section is based then it expects its mapping to be at the same
	// address in every process.  As such, the region that it wants to be mapped
	// into should be exempted from further randomized allocations in order to
	// avoid bad things from happening.  A specific scenario where this is
	// necessary is with basesrv's shared section that is mapped into csrss
	// clients.
	//
	else if (GetSectionObjectControlAreaFlags(
			SectionObject) & CONTROL_AREA_FLAG_BASED)
	{
		//
		// If the number of mapped views is zero we shall randomize the base
		// address that is being loaded and add it to the list of region
		// exemptions such that it wont be used in the future.
		//
		if (GetSectionObjectControlAreaNumberOfMappedViews(
				SectionObject) == 0)
		{
			*BaseAddress = GetRandomizedBaseForProcess(
					ProcessObject,
					GetSectionObjectSegmentSize(
						SectionObject)->LowPart);

			CreateRegionExemption = TRUE;
			Randomized            = TRUE;
		}
	}
	//
	// If no base address was supplied and the supplied mapping is not associated
	// with an image file and is not based, randomize it for just this process.
	//
	else if (!*BaseAddress)
	{
		//
		// If memory allocation randomization is enabled and this process
		// isn't exempted...
		//
		if ((IsRandomizationSubsystemEnabled(
				RANDOMIZATION_SUBSYSTEM_ALLOCATIONS)) &&
		    (!IsProcessExempted(
				ProcessObject,
				EXEMPT_MEMORY_ALLOCATIONS)))
		{
			Randomized   = TRUE;
			*BaseAddress = GetRandomizedBaseForProcess(
					ProcessObject,
					(ViewSize && *ViewSize)  ?
						*ViewSize :
						GetSectionObjectSegmentSize(
							SectionObject)->LowPart);
	
			DebugPrint(("MmMapViewOfSectionHook(): Randomized non-image/non-based range to %p for process %p.",
					*BaseAddress,
					ProcessObject));
		}
	}
#endif

	//
	// Execute the mapping, potentially with a randomized section object instead
	// of the one that was passed in.
	//
	Status = OrigMmMapViewOfSection(
			SectionObject,
			ProcessObject,
			BaseAddress,
			ZeroBits,
			CommitSize,
			SectionOffset,
			ViewSize,
			InheritDisposition,
			AllocationType,
			Protect);

	//
	// Decrements the randomized section's reference count
	//
	if (NewSectionObject)
	{
		//
		// If the call failed due to conflicting addresses, allow the memory
		// manager to pick the base address for us
		//
		if (Status == STATUS_CONFLICTING_ADDRESSES)
		{
			*BaseAddress = NULL;

			Status = OrigMmMapViewOfSection(
					SectionObject,
					ProcessObject,
					BaseAddress,
					ZeroBits,
					CommitSize,
					SectionOffset,
					ViewSize,
					InheritDisposition,
					AllocationType,
					Protect);
		}

		//
		// If the operation succeeded but we did not mapped to the base address
		// that we desired, set STATUS_IMAGE_NOT_AT_BASE
		//
		if ((NT_SUCCESS(Status)) &&
		    (GetSectionObjectSegmentBasedAddress(
				NewSectionObject) != *BaseAddress))
		{
			//
			// If we're on XPSP2, we might need to switch to the PAE release since
			// the BasedAddress attribute will be located at a different offset
			// than what we're typically used to.  Try to detect this and call the
			// same comparison over again afterword.  If the results haven't
			// changed, then this is likely a legitemate not-at-base situation.
			//
			CheckSwitchToPae(
					NewSectionObject,
					*BaseAddress);

			//
			// If it still doesn't match after the check, set the status to image
			// not at base.
			//
			if (GetSectionObjectSegmentBasedAddress(
					NewSectionObject) != *BaseAddress)
			{
				Status = STATUS_IMAGE_NOT_AT_BASE;
			}
		}

		DebugPrint(("MmMapViewOfSectionHook(): Returning base %p to caller.", 
				*BaseAddress));

		//
		// If the mapping fails after a randomization optimization, blow up.
		//
		ASSERT(NT_SUCCESS(Status));

		//
		// Lose our reference to the section
		//
		ObDereferenceObject(
				NewSectionObject);
	}
	else if ((Randomized) &&
	         (!NT_SUCCESS(Status)))
	{
		//
		// If the randomization failed, let the operating system decide
		//
		Randomized   = FALSE;
		*BaseAddress = NULL;

		Status = OrigMmMapViewOfSection(
				OrigSectionObject,
				ProcessObject,
				BaseAddress,
				ZeroBits,
				CommitSize,
				SectionOffset,
				ViewSize,
				InheritDisposition,
				AllocationType,
				Protect);
	}

	//
	// Execute custom section actions for this mapping if necessary
	//
	ExecuteCustomSectionAction(
			ProcessObject,
			OrigSectionObject,
			NewSectionObject,
			*BaseAddress,
			*ViewSize);

	//
	// If we should create a region exemption and the mapping succeeded, do it
	// now.
	//
	if (Randomized)
	{
		if ((NT_SUCCESS(Status)) &&
		    (CreateRegionExemption))
		{
			DebugPrint(("MmMapViewOfSectionHook(): Creating region exemption for base %p size %lu.",
					*BaseAddress,
					*ViewSize));
	
			AddRegionExemption(
					*BaseAddress,
					*ViewSize);
		}
		//
		// If the operation was not successful, assert.
		//
		else if (!NT_SUCCESS(Status))
		{
			ASSERT(Status == STATUS_SUCCESS);
		}
	}

	//
	// Lose the reference to the image set mapping if we obtained one
	//
	if (Mapping)
		DereferenceImageSetMapping(
				Mapping);

	return Status;
}

//
// Hook implementation of ZwAllocateVirtualMemory.  This function is responsible
// for the randomization of user-mode stack and heap ranges.
//
NTSTATUS ZwAllocateVirtualMemoryHook(
		IN HANDLE ProcessHandle,
		IN OUT PVOID *BaseAddress,
		IN ULONG ZeroBits,
		IN OUT PULONG RegionSize,
		IN ULONG AllocationType,
		IN ULONG Protect)
{
	KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
	NTSTATUS        Status;
	BOOLEAN         Randomized = FALSE;
	ULONG           OrigAllocationType = AllocationType;
	ULONG           OrigZeroBits = ZeroBits;
	ULONG           MaxTries = 0;
	ULONG           CapturedRegionSize;
	PVOID           CapturedBaseAddress;

	//
	// Make sure we have valid arguments if we're using UserMode.
	//

	if (PreviousMode == UserMode)
	{
		__try
		{
			//
			// IA32 does not have strict alignment requirements, and as such we can
			// probe for aligning on a one byte boundary.  There are some
			// applications that will provide pointers that are unaligned.  We must
			// be able to handle these conditions.
			//
			ProbeForWrite(
					BaseAddress, 
					sizeof(PVOID), 
					ARCHITECTURE_ALIGNMENT_REQUIREMENT);
			ProbeForWrite(
					RegionSize, 
					sizeof(ULONG), 
					ARCHITECTURE_ALIGNMENT_REQUIREMENT);

			CapturedBaseAddress = *BaseAddress;
			CapturedRegionSize = *RegionSize;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return (NTSTATUS)GetExceptionCode();
		}
	}
	else
	{
		CapturedBaseAddress = *BaseAddress;
		CapturedRegionSize = *RegionSize;
	}

	//
	// If the requested base address is NULL and randomized allocations are
	// enabled, randomize the base address for the allocation.
	//
	if ((!CapturedBaseAddress) &&
	    (IsRandomizationSubsystemEnabled(
			RANDOMIZATION_SUBSYSTEM_ALLOCATIONS)))
	{
		PPROCESS_OBJECT ProcessObject = NULL;
		BOOLEAN         DerefProcessObject = FALSE;

		if (ProcessHandle == NtCurrentProcess())
			ProcessObject = IoGetCurrentProcess();
		else
		{
			ObReferenceObjectByHandle(
					ProcessHandle,
					PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
					*PsProcessType,
					PreviousMode,
					&ProcessObject,
					NULL);

			DerefProcessObject = TRUE;
		}

		//
		// If the process object is valid...
		//
		if (ProcessObject)
		{
			BOOLEAN CanRandomize = TRUE;

			//
			// If the process isn't exempted...
			//
			if (!IsProcessExempted(
					ProcessObject,
					EXEMPT_MEMORY_ALLOCATIONS))
			{
				//
				// Check to see if the ATI hack is required.  If it is and the required
				// memory region does not yet exist in the process, skip randomization.
				//
				if ((IsAtiHackRequired()) &&
					 (!CheckProcessRegionBaseExists(
						ProcessObject,
						(PVOID)0x00020000)))
					CanRandomize = FALSE;

				if (CanRandomize)
				{
					//
					// Get a randomized base if possible
					//
					CapturedBaseAddress = GetRandomizedBaseForProcess(
							ProcessObject,
							CapturedRegionSize);

					//
					// Increment the total number of randomized allocations for statistic
					// tracking purposes
					//
					IncrementExecutiveCounter(
							RandomizedAllocationsCounter,
							0);
				}

				if (CapturedBaseAddress)
				{
					DebugPrint(("ZwAllocateVirtualMemory(): Randomized to %p (process %p sz %.8x type %.8x prot %.8x ZB %d).", 
								CapturedBaseAddress, 
								ProcessObject,
								CapturedRegionSize,
								AllocationType,
								Protect,
								ZeroBits));

					ZeroBits   = 0;
					Randomized = TRUE;
					
					//
					// When mapping to a specific base address, we must reserve the memory
					//
					if (AllocationType == MEM_COMMIT)
						AllocationType |= MEM_RESERVE;
				}
			}

			//
			// If we obtained a reference to the process object via
			// ObReferenceObjectByHandle, lose that reference now.
			//
			if (DerefProcessObject)
				ObDereferenceObject(
						ProcessObject);
		}
	}

	//
	// Now we write out any modified parameters to caller memory for the real implementation.
	//
	// We don't want to use our (kernel) stack versions because if PreviousMode is UserMode,
	// then the routine will fail to capture the arguments (invalid UserMode pointers).
	//

	if (PreviousMode == UserMode)
	{
		__try
		{
			*BaseAddress = CapturedBaseAddress;
			*RegionSize = CapturedRegionSize;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return (NTSTATUS)GetExceptionCode();
		}
	}
	else
	{
		*BaseAddress = CapturedBaseAddress;
		*RegionSize = CapturedRegionSize;
	}

	//
	// There is a slight race condition between picking a random base address
	// and actually using it.  We could address this with a global mutex or
	// with a claimed queue or something.  Not too worried about it right now.
	// If the operation fails the worst case scenario is that the region is not
	// randomized.
	//
	Status = OrigZwAllocateVirtualMemory(
			ProcessHandle,
			BaseAddress,
			ZeroBits,
			RegionSize,
			AllocationType,
			Protect);

	//
	// If the allocation fails and we were using randomization, disable
	// randomization and try to allocate again
	//
	if ((!NT_SUCCESS(Status)) && 
	    (Randomized))
	{
		if (PreviousMode == UserMode)
		{
			__try
			{
				*BaseAddress = 0;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				return (NTSTATUS)GetExceptionCode();
			}
		}
		else
		{
			*BaseAddress = 0;
		}

		ZeroBits       = OrigZeroBits;
		AllocationType = OrigAllocationType;

		Status = OrigZwAllocateVirtualMemory(
				ProcessHandle,
				BaseAddress,
				ZeroBits,
				RegionSize,
				AllocationType,
				Protect);

		ASSERT(Status == STATUS_SUCCESS);
	}

	return Status;
}

//
// Hook implementation of ZwSetSystemInformation.  This is needed due to the
// fact that some things in the kernel will use ZwSetSystemInformation to load
// images into system address space and as such the driver must be ready to able
// to ignore randomization of these images.  If this is not done, bad things can
// and will happen.  For instance, the SVGA driver is loaded from a stub DLL via
// this mechanism and if relocation fixups are applied to the image by the
// randomization driver, bad things will happen due to the fact that the kernel
// will manually do fixups if the requested address (by the system call) differs
// from the one to which the image is mapped.
//
NTSTATUS ZwSetSystemInformationHook(
		IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
		IN OUT PVOID SystemInformation,
		IN ULONG SystemInformationLength)
{
	BOOLEAN  Exempted = FALSE;
	NTSTATUS Status;

	//
	// If the caller is requesting that an image be loaded, try to add the thread
	// to the list of exempted threads.  This will prevent randomization from
	// occuring for any images that are mapped in this thread during the time
	// period that it is exempted.
	//
	if (SystemInformationClass == SystemLoadImage)
		Exempted = AddThreadExemption(
				PsGetCurrentThread());

	//
	// Call the real function as if nothing were happening
	//
	Status = OrigZwSetSystemInformation(
			SystemInformationClass,
			SystemInformation,
			SystemInformationLength);

	//
	// If the thread was exempted, remove it from the exemption list
	//
	if (Exempted)
		RemoveThreadExemption(
				PsGetCurrentThread());

	return Status;
}

//
// This hook is responsible for intercepting user-mode thread creations and
// adjusting the stack so that partial address overwrites are not possible.  It
// is also responsible for initializing SEH overwrite protection for the thread.
//
NTSTATUS NtCreateThreadHook(
		OUT PHANDLE ThreadHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN HANDLE ProcessHandle,
		OUT PCLIENT_ID ClientId,
		IN PCONTEXT ThreadContext,
		IN PUSER_STACK UserStack,
		IN BOOLEAN CreateSuspended)
{
	NTSTATUS Status;
	NTSTATUS InnerStatus;

	//
	// We need at least access enough to resume the thread, so add that to the
	// access mask.
	//
	DesiredAccess |= THREAD_SUSPEND_RESUME;

	//
	// First, create the thread suspended using the exact same arguments that
	// were specified by the caller.  We have to create it suspended so that we
	// can alter the exception list.
	//
	Status = OrigNtCreateThread(
			ThreadHandle,
			DesiredAccess,
			ObjectAttributes,
			ProcessHandle,
			ClientId,
			ThreadContext,
			UserStack,
			TRUE);

	//
	// If the thread is created successfully, it's time to insert the validation
	// frame.
	//
	if (NT_SUCCESS(Status))
	{
		HANDLE CapturedThreadHandle = NULL;

		__try
		{
			//
			// Capture the thread handle we'll be working with.  We don't need to probe
			// because the original NtCreateThread will have done it for us.
			//
			CapturedThreadHandle = *ThreadHandle;

			//
			// Create the validation frame for this thread.
			//
			InnerStatus = CreateSehValidationFrameForThread(
					ProcessHandle,
					CapturedThreadHandle);

		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			InnerStatus = GetExceptionCode();
		
			DebugPrint(("NtCreateThreadHook(): Caught exception during CreateSehValidationFrameForThread, %.8x.",
					InnerStatus));
		}

		//
		// If we failed to create the validation frame for this thread for some
		// reason, log such and continue on with our merry business.
		//
		if (!NT_SUCCESS(InnerStatus))
		{
			DebugPrint(("NtCreateThreadHook(): Failed to create validation frame, %.8x.",
					InnerStatus));
		}

		//
		// Regardless of what happens, resume the thread and let things rock, but
		// only if the caller didn't request that the thread be created suspended.
		//
		if ((CapturedThreadHandle) &&
		    (!CreateSuspended) &&
		    (!NT_SUCCESS(InnerStatus = NtResumeThread(
				CapturedThreadHandle,
				NULL))))
		{
			DebugPrint(("NtCreateThreadHook(): Failed to resume thread handle 0x%.8x, %.8x.",
					CapturedThreadHandle,
					InnerStatus));
		}
	}
		
	return Status;
}

#if 0
	// 
	// If the caller supplied a thread context, then we shall inject some random
	// stack padding of no greater than 252 bytes.
	//
	// UPDATE: It seems like things calling NtCreateThread may keep a reference
	// to the stack outside of the context structure and then use that to set up
	// arguments.  How unfortunate.
	//
	if (ThreadContext)
	{
		ThreadContext->Esp -= RngRand() & 0xfc;

		DebugPrint(("NtCreateThreadHook(): New stack pointer is: %.8x",
				ThreadContext->Esp));
	}
#endif

//
// Call the real MmMapViewOfSection
//
NTSTATUS OrigMmMapViewOfSection(
		IN PSECTION_OBJECT SectionObject,
		IN PPROCESS_OBJECT ProcessObject,
		IN OUT PVOID *BaseAddress,
		IN ULONG ZeroBits,
		IN ULONG CommitSize,
		IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
		IN OUT PSIZE_T ViewSize,
		IN SECTION_INHERIT InheritDisposition,
		IN ULONG AllocationType,
		IN ULONG Protect)
{
	return (MmMapViewOfSectionCallLayer) 
		? ((MmMapViewOfSectionProc)(MmMapViewOfSectionCallLayer->RealFunction))(
				SectionObject,
				ProcessObject,
				BaseAddress,
				ZeroBits,
				CommitSize,
				SectionOffset,
				ViewSize,
				InheritDisposition,
				AllocationType,
				Protect)
		: STATUS_NOT_FOUND;
}

//
// Call the real ZwAllocateVirtualMemory
//
NTSTATUS OrigZwAllocateVirtualMemory(
		IN HANDLE ProcessHandle,
		IN OUT PVOID *BaseAddress,
		IN ULONG ZeroBits,
		IN OUT PULONG RegionSize,
		IN ULONG AllocationType,
		IN ULONG Protect)
{
	return (ZwAllocateVirtualMemoryCallLayer)
		? ((ZwAllocateVirtualMemoryProc)(ZwAllocateVirtualMemoryCallLayer->RealFunction))(
			ProcessHandle,
			BaseAddress,
			ZeroBits,
			RegionSize,
			AllocationType,
			Protect)
		: STATUS_NOT_FOUND;
}

//
// Call the real ZwSetSystemInformation
//
NTSTATUS OrigZwSetSystemInformation(
		IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
		IN OUT PVOID SystemInformation,
		IN ULONG SystemInformationLength)
{
	return (ZwSetSystemInformationCallLayer)
		? ((ZwSetSystemInformationProc)(ZwSetSystemInformationCallLayer->RealFunction))(
			SystemInformationClass,
			SystemInformation,
			SystemInformationLength)
		: STATUS_NOT_FOUND;
}

//
// Call the real NtCreateThread
//
NTSTATUS OrigNtCreateThread(
		OUT PHANDLE ThreadHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN HANDLE ProcessHandle,
		OUT PCLIENT_ID ClientId,
		IN PCONTEXT ThreadContext,
		IN PUSER_STACK UserStack,
		IN BOOLEAN CreateSuspended)
{
	return (NtCreateThreadCallLayer)
		? ((NtCreateThreadProc)(NtCreateThreadCallLayer->RealFunction))(
			ThreadHandle,
			DesiredAccess,
			ObjectAttributes,
			ProcessHandle,
			ClientId,
			ThreadContext,
			UserStack,
			CreateSuspended)
		: STATUS_NOT_FOUND;
}
