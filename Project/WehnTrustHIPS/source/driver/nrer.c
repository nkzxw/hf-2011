#include "precomp.h"


//
// Local types & definitions
//

//
// Prototypes
//
static NTSTATUS CacheSystemDllSymbols(
		IN PIMAGE_SET ImageSet,
		IN ULONG_PTR ImageBase,
		IN ULONG ImageSize);
static NTSTATUS InitializeNrerCoreForProcess(
		IN PPROCESS_OBJECT ProcessObject,
		IN PIMAGE_SET ImageSet,
		IN PVOID NtdllImageBase,
		IN ULONG NtdllImageSize,
		IN PVOID NrerImageBase,
		IN ULONG NrerImageSize,
		OUT PNRER_DISPATCH_TABLE NrerDispatchTable);
static NTSTATUS InitializeNrerExecutableMirrorForProcess(
		IN PPROCESS_OBJECT ProcessObject,
		IN PIMAGE_SET ImageSet,
		IN PIMAGE_SET_MAPPING ExecutableMapping,
		IN PVOID NtdllImageBase,
		IN ULONG NtdllImageSize,
		IN PVOID NrerImageBase,
		IN ULONG NrerImageSize);
static VOID CreateSehValidationFrameKernelModeApcRoutine(
		IN PKAPC Apc,
		IN OUT PKNORMAL_ROUTINE *NormalRoutine,
		IN OUT PVOID *NormalContext,
		IN OUT PVOID *SystemArgument1,
		IN OUT PVOID *SystemArgument2);
static NTSTATUS CallNrerUserModeRoutine(
		IN PTHREAD_OBJECT ThreadObject,
		IN PVOID RoutineAddress);

//
// A pointer to the section object that is associated with the non-randomized
// NRER user-mode DLL on disk.
//
static PSECTION_OBJECT NrerUserModeDllSectionObject = NULL;
static BOOLEAN         NrerInitialized = FALSE;

//
// Initializes the NRER subsystem by creating a file-backed section that is
// pointed at the NRER user-mode DLL on disk.  This section is used for
// randomization purposes.
//
NTSTATUS InitializeNreSubsystem()
{
	OBJECT_ATTRIBUTES Attributes;
	IO_STATUS_BLOCK   IoStatus;
	UNICODE_STRING    FilePath;
	NTSTATUS          Status = STATUS_SUCCESS;
	HANDLE            FileHandle;
	HANDLE            SectionHandle;

	do
	{
		//
		// Get a file handle to the user-mode DLL
		//
		RtlInitUnicodeString(
				&FilePath,
				NRER_USER_MODE_DLL_PATH);

		InitializeObjectAttributes(
				&Attributes,
				&FilePath,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);

		if (!NT_SUCCESS(Status = ZwOpenFile(
				&FileHandle,
				GENERIC_READ | GENERIC_EXECUTE,
				&Attributes,
				&IoStatus,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				0)))
		{
			DebugPrint(("InitializeNreSubsystem(): ZwCreateFile(%wZ) failed, %.8x.",
					&FilePath, 
					Status));
			break;
		}

		//
		// Create an image file-backed section that is associated with the DLL
		//
		if (!NT_SUCCESS(Status = ZwCreateSection(
				&SectionHandle,
				SECTION_ALL_ACCESS,
				NULL,
				NULL,
				PAGE_EXECUTE_WRITECOPY,
				SEC_IMAGE,
				FileHandle)))
		{
			DebugPrint(("InitializeNreSubsystem(): ZwCreateSection() failed, %.8x.",
					Status));
			break;
		}

		//
		// Get the section object that's associated with the section handle
		//
		if (!NT_SUCCESS(Status = ObReferenceObjectByHandle(
				SectionHandle,
				0,
				NULL,
				KernelMode,
				(PVOID *)&NrerUserModeDllSectionObject,
				NULL)))
		{
			DebugPrint(("InitializeNreSubsystem(): ObReferenceObjectByHandle() failed, %.8x.",
					Status));
			break;
		}

		DebugPrint(("InitializeNreSubsystem(): NRER user-mode DLL section at: %p.",
				NrerUserModeDllSectionObject));

		NrerInitialized = TRUE;

	} while (0);

	//
	// Close the handles that were opened
	//
	if (FileHandle)
		ZwClose(
				FileHandle);
	if (SectionHandle)
		ZwClose(
				SectionHandle);

	return Status;
}

//
// This callback is called from the custom section action code each time the
// system DLL, NTDLL.DLL, is mapped into a process' address space.  The reason
// this is necessary is because the NRER dispatch table must be initialized.
// Also, each time NTDLL.DLL is mapped into a process, the NRER user-mode DLL is
// mapped into the process' address space (if it has not already been mapped)
// and the exception dispatcher is hooked.
//
NTSTATUS NrerSystemDllHandler(
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN PSECTION_OBJECT NewSectionObject,
		IN PVOID ImageBase,
		IN ULONG ViewSize)
{
	NRER_DISPATCH_TABLE NrerDispatchTable;
	PIMAGE_SET_MAPPING  ExecutableMapping = NULL;
	PIMAGE_SET          ImageSet = NULL;
	NTSTATUS            Status = STATUS_SUCCESS;

	do
	{
		//
		// Get the image set that's associated with the specified process.
		//
		if (!NT_SUCCESS(Status = GetImageSetForProcess(
				ProcessObject,
				&ImageSet)))
		{
			DebugPrint(("NrerSystemDllHandler(): GetImageSetForProcess failed, %.8x.",
					Status));
			break;
		}

		//
		// Create the NRER image set mapping for this process if it has not
		// already been created.
		//
		if (!NT_SUCCESS(Status = CreateNrerImageSetMapping(
				ProcessObject,
				ImageSet,
				NULL,
				&NrerDispatchTable)))
		{
			DebugPrint(("NrerSystemDllHandler(): CreateNrerImageSetMapping failed, %.8x.",
					Status));
			break;
		}

#ifndef DISABLE_NRE_RANDOMIZATION
		//
		// If the process requires executable mirroring, do that now.
		//
		if (IsProcessExecutionState(
				ProcessObject,
				PROCESS_EXECUTION_STATE_NRER_MIRRORING_NEEDED))
		{
			//
			// Get the executable mapping that is associated with the supplied process.
			//
			if (!(ExecutableMapping = GetProcessExecutableMapping(
					ProcessObject)))
			{  
				DebugPrint(("NrerSystemDllHandler(): Process %p does not have an associated executable mapping.",
						ProcessObject));
				
				ASSERT(0);

				Status = STATUS_SUCCESS;
				break;
			}

			//
			// Create the executable mirror for this process.
			//
			__try
			{
				if (!NT_SUCCESS(Status = InitializeNrerExecutableMirrorForProcess(
						ProcessObject,
						ImageSet,
						ExecutableMapping,
						(ULONG_PTR)ImageBase,                       // NTDLL.DLL's ImageBase
						ViewSize,                                   // NTDLL.DLL's ImageSize
						(ULONG_PTR)NrerDispatchTable.NrerImageBase, // NRER.DLL's ImageBase
						NrerDispatchTable.NrerImageSize)))          // NRER.DLL's ImageSize
				{
					DebugPrint(("NrerSystemDllHandler(): InitializeNrerExecutableMirrorForProcess failed, %.8x.",
							Status));
					break;
				}
			} __except(EXCEPTION_EXECUTE_HANDLER)
			{
				Status = GetExceptionCode();

				DebugPrint(("NrerSystemDllHandler(): Exception caught during executable mirroring: %.8x.",
						Status));
				break;
			}

			//
			// Flag that this process no longer needs executable mirroring.
			//
			SetProcessExecutionState(
					ProcessObject,
					PROCESS_EXECUTION_STATE_NRER_MIRRORING_NEEDED,
					FALSE);
		}
#endif

	} while (0);

	//
	// Lose references.
	//
	if (ExecutableMapping)
		DereferenceImageSetMapping(
				ExecutableMapping);
	if (ImageSet)
		DereferenceImageSet(
				ImageSet);

	return Status;
}

//
// This routine creates the image set mapping that is to be associated with the
// NRER user-mode DLL and maps it into the supplied process.
//
NTSTATUS CreateNrerImageSetMapping(
		IN PPROCESS_OBJECT ProcessObject,
		IN PIMAGE_SET ImageSet OPTIONAL,
		OUT PIMAGE_SET_MAPPING *OutNrerMapping OPTIONAL,
		OUT PNRER_DISPATCH_TABLE OutNrerDispatchTable OPTIONAL)
{
	NRER_DISPATCH_TABLE NrerDispatchTable;
	PIMAGE_SET_MAPPING  NrerMapping = NULL;
	LARGE_INTEGER       SectionOffset = { 0 };
	KAPC_STATE          ApcState;
	NTSTATUS            Status;
	BOOLEAN             Attached = FALSE;
	BOOLEAN             DerefImageSet = FALSE;
	PVOID               NrerImageBase = NULL;
	ULONG               NrerImageSize = 0;

	do
	{
		//
		// If the NRER user-mode DLL has yet to have a section object cached for
		// it, do so now.
		//
		if (!NrerInitialized)
		{
			if (!NT_SUCCESS(Status = InitializeNreSubsystem()))
			{
				DebugPrint(("CreateNrerImageSetMapping(): InitializeNreSubsystem failed, %.8x.",
						Status));
				break;
			}
		}

		//
		// Get the image set that's associated with the process
		//
		if (!ImageSet)
		{
			if (!NT_SUCCESS(Status = GetImageSetForProcess(
					ProcessObject,
					&ImageSet)))
			{
				DebugPrint(("CreateNrerImageSetMapping(): GetImageSetForProcess failed, %.8x.",
						Status));
				break;
			}

			DerefImageSet = TRUE;
		}

		//
		// If the NRER dispatch table has not been initialized, do so now.
		//
		if (!ImageSet->NtDispatchTableInitialized)
		{
			//
			// If the calling process is not the same as the process object that
			// was passed to us, we must attach so that we can resolve symbols
			// inside of NTDLL.DLL
			//
			if (ProcessObject != IoGetCurrentProcess())
			{
				KeStackAttachProcess(
						ProcessObject,
						&ApcState);

				Attached = TRUE;
			}

			//
			// Cache the symbols that are associated with the system DLL so that
			// they can be used 
			//
			__try
			{
				if (!NT_SUCCESS(Status = CacheSystemDllSymbols(
						ImageSet,
						(ULONG_PTR)ImageSet->NtdllImageBase,
						ImageSet->NtdllImageSize)))
				{
					DebugPrint(("CreateNrerImageSetMapping(): CacheSystemDllSymbols failed, %.8x.",
							Status));
					break;
				}

			} __except(EXCEPTION_EXECUTE_HANDLER)
			{
				Status = GetExceptionCode();

				DebugPrint(("CreateNrerImageSetMapping(): Exception caught during CacheSystemDllSymbols call: %.8x.",
						Status));

				break;
			}

			// 
			// If we're attached, un-attach now
			//
			if (Attached)
			{
				KeUnstackDetachProcess(
						&ApcState);

				Attached = FALSE;
			}
		}
	
		//
		// If the DLL has already been initialized for this process, then we don't
		// need to worry about anything.
		//
		if (GetProcessNrerDispatchTable(
				ProcessObject,
				&NrerDispatchTable))
		{
			break;
		}

		//
		// With NRER enabled and the system dispatch table initialized for this
		// image set, map the NRER user-mode DLL into the context of the process.
		//
		if (!NT_SUCCESS(Status = LookupRandomizedImageMapping(
				ImageSet,
				ProcessObject,
				NrerUserModeDllSectionObject,
				GetSectionObjectSegmentSize(
					NrerUserModeDllSectionObject)->LowPart,
				&NrerMapping)))
		{
			DebugPrint(("CreateNrerImageSetMapping(): LookupRandomizedImageMapping failed, %.8x.",
					Status));
			break;
		}

		//
		// Once NRER.dll has had a randomized instance created, the next step is
		// to actually map it into this process' address space.
		//
		NrerImageBase = NrerMapping->ImageBaseAddress;
		NrerImageSize = NrerMapping->ViewSize;

		if (!NT_SUCCESS(Status = OrigMmMapViewOfSection(
				NrerMapping->SectionObject,
				ProcessObject,
				&NrerImageBase,
				0,
				0,
				&SectionOffset,
				&NrerImageSize,
				0,
				MEM_COMMIT,
				PAGE_EXECUTE_READWRITE)))
		{
			DebugPrint(("CreateNrerImageSetMapping(): OrigMmMapViewOfSection failed, %.8x.",
					Status));
			break;
		}
		
		//
		// If we haven't already attached to the process (and assuming it's
		// necessary), attach now.
		//
		if ((!Attached) &&
		    (ProcessObject != IoGetCurrentProcess()))
		{
			KeStackAttachProcess(
					ProcessObject,
					&ApcState);

			Attached = TRUE;
		}

		//
		// Now that we've gotten everything prepared, it's time to hook NTDLL's
		// KiUserExceptionDispatcher and initialize NRER.dll such that it can
		// perform its duties.
		//
		__try
		{
			//
			// If NRER.dll was not mapped at the address it was supposed to be mapped
			// at, we must perform relocations internally prior to using it.
			//
			if (NrerImageBase != NrerMapping->ImageBaseAddress)
			{
				if (!NT_SUCCESS(Status = LdrRelocateImage(
						NrerImageBase,
						NrerImageSize)))
				{
					DebugPrint(("NrerSystemDllHandler(): LdrRelocateImage(%p) failed, %.8x.",
							NrerImageBase,
							Status));
					break;
				}
			}

			//
			// Initialize NRER now that everything is mapped and ready to go.
			//
			if (!NT_SUCCESS(Status = InitializeNrerCoreForProcess(
					ProcessObject,
					ImageSet,
					ImageSet->NtdllImageBase,
					ImageSet->NtdllImageSize,
					NrerImageBase,
					NrerImageSize,
					&NrerDispatchTable)))
			{
				DebugPrint(("CreateNrerImageSetMapping(): InitializeNrerCoreForProcess failed, %.8x.",
						Status));
				break;
			}

		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = GetExceptionCode();

			DebugPrint(("CreateNrerImageSetMapping(): Exception caught during relocations: %.8x.",
					Status));
			break;
		}

		//
		// If we were attached, then let's un-attach now.
		//
		if (Attached)
		{
			KeUnstackDetachProcess(
					&ApcState);

			Attached = FALSE;
		}

		//
		// Set this process' NRER image base and size as it could have been forced
		// to be relocated for some reason.
		//
		SetProcessNrerDispatchTable(
				ProcessObject,
				&NrerDispatchTable);

		//
		// Set the flag that indicates that NRER is initialized for this process.
		//
		SetProcessExecutionState(
				ProcessObject,
				PROCESS_EXECUTION_STATE_NRER_DLL_LOADED,
				FALSE);

	} while (0);

	//
	// If we were attached, then let's un-attach now.
	//
	if (Attached)
		KeUnstackDetachProcess(
				&ApcState);

	//
	// Set output pointers.
	//
	if (OutNrerDispatchTable)
		RtlCopyMemory(
				OutNrerDispatchTable,
				&NrerDispatchTable,
				sizeof(NRER_DISPATCH_TABLE));

	//
	// Lose our reference to the image set mapping if we were given one
	//
	if (NrerMapping)
	{
		if (OutNrerMapping)
			*OutNrerMapping = NrerMapping;
		else
			DereferenceImageSetMapping(
					NrerMapping);
	}
	if (DerefImageSet)
		DereferenceImageSet(
				ImageSet);

	return Status;
}

//
// This routine is called the first time NTDLL.DLL is mapped into memory.  It
// caches some of the symbols that are needed by NRER.DLL in order to accomplish
// its goal.  The symbols that are required are:
//
//		LdrGetProcedureAddress
//		LdrLoadDll
// 	NtQueryVirtualMemory
// 	NtProtectVirtualMemory
// 	NtContinue
//
static NTSTATUS CacheSystemDllSymbols(
		IN PIMAGE_SET ImageSet,
		IN ULONG_PTR ImageBase,
		IN ULONG ImageSize)
{
	NTSTATUS Status;

	do
	{
		//
		// Cache the base address of NTDLL
		//
		ImageSet->NtDispatchTable.NtdllImageBase = (PVOID)ImageBase;

		//
		// Resolve LdrGetProcedureAddress
		//
		if (!NT_SUCCESS(Status = LdrGetProcAddress(
				ImageBase,
				ImageSize,
				"LdrGetProcedureAddress",
				(PVOID)&ImageSet->NtDispatchTable.LdrGetProcedureAddress)))
		{
			DebugPrint(("CacheSystemDllSymbols(): Failed to resolve LdrGetProcedureAddress, %.8x.",
					Status));
			break;
		}
		
		//
		// Resolve LdrLoadDll
		//
		if (!NT_SUCCESS(Status = LdrGetProcAddress(
				ImageBase,
				ImageSize,
				"LdrLoadDll",
				(PVOID)&ImageSet->NtDispatchTable.LdrLoadDll)))
		{
			DebugPrint(("CacheSystemDllSymbols(): Failed to resolve LdrLoadDll, %.8x.",
					Status));
			break;
		}

		//
		// Resolve NtProtectVirtualMemory
		//
		if (!NT_SUCCESS(Status = LdrGetProcAddress(
				ImageBase,
				ImageSize,
				"NtProtectVirtualMemory",
				(PVOID)&ImageSet->NtDispatchTable.NtProtectVirtualMemory)))
		{
			DebugPrint(("CacheSystemDllSymbols(): Failed to resolve NtProtectVirtualMemory, %.8x.",
					Status));
			break;
		}

		//
		// Resolve LdrInitializeThunk and KiUserExceptionDispatcher
		//
		if (!NT_SUCCESS(Status = LdrGetProcAddress(
				ImageBase,
				ImageSize,
				"LdrInitializeThunk",
				(PVOID)&ImageSet->NtDispatchTable.LdrInitializeThunk)))
		{
			DebugPrint(("CacheSystemDllSymbols(): Failed to resolve LdrInitializeThunk, %.8x.",
					Status));
			break;
		}

		//
		// Resolve KiUserExceptionDispatcher
		//
		if (!NT_SUCCESS(Status = LdrGetProcAddress(
				ImageBase,
				ImageSize,
				"KiUserExceptionDispatcher",
				(PVOID)&ImageSet->NtDispatchTable.KiUserExceptionDispatcher)))
		{
			DebugPrint(("CacheSystemDllSymbols(): Failed to resolve KiUserExceptionDispatcher, %.8x.",
					Status));
			break;
		}

		//
		// Woop!
		//
		DebugPrint(("CacheSystemDllSymbols(): Successfully resolved system DLL symbols for image set %p.",
				ImageSet));

		ImageSet->NtDispatchTableInitialized = TRUE;

	} while (0);

	return Status;
}

//
// This routine initializes the NRER user-mode DLL and installs all of the hooks
// that are directly required by it.
//
// Must be called from within an SEH guard.
//
static NTSTATUS InitializeNrerCoreForProcess(
		IN PPROCESS_OBJECT ProcessObject,
		IN PIMAGE_SET ImageSet,
		IN PVOID NtdllImageBase,
		IN ULONG NtdllImageSize,
		IN PVOID NrerImageBase,
		IN ULONG NrerImageSize,
		OUT PNRER_DISPATCH_TABLE NrerDispatchTable)
{
	PNRER_NT_DISPATCH_TABLE DispatchTable = NULL;
	NTSTATUS                Status;
	PULONG                  NreExecutionFlags;
	PVOID                   NreInitialize = NULL;
	PVOID                   UmCreateSehValidationFrame = NULL;
	PVOID                   UmNreInitialize = NULL;
	ULONG                   CopySize;
	ULONG                   Offset;

	do
	{
		//
		// Initialize nrer!DispatchTable.
		//
		if (!NT_SUCCESS(Status = LdrGetProcAddress(
				(ULONG_PTR)NrerImageBase,
				NrerImageSize,
				"DispatchTable",
				(PVOID *)&DispatchTable)))
		{
			DebugPrint(("InitializeNrerCoreForProcess(): Failed to resolve symbol 'DispatchTable', %.8x.",
					Status));
			break;
		}

		//
		// We can't trust the dispatch table value we're provided.  We probe once
		// for sizeof(ULONG) and then again once we know the size we'll be
		// copying.
		//
		__try
		{
			ProbeForWrite(
					DispatchTable,
					sizeof(ULONG),
					1);

			CopySize = GetStructureSize(DispatchTable) - sizeof(ULONG);

			if (CopySize > sizeof(NRER_NT_DISPATCH_TABLE) - sizeof(ULONG))
				CopySize = sizeof(NRER_NT_DISPATCH_TABLE) - sizeof(ULONG);

			ProbeForWrite(
					DispatchTable,
					CopySize,
					1);

			RtlCopyMemory(
					(PVOID)((ULONG_PTR)DispatchTable + sizeof(ULONG)),
					(PVOID)((ULONG_PTR)&ImageSet->NtDispatchTable + sizeof(ULONG)),
					CopySize);

		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = GetExceptionCode();
			break;
		}


		//
		// Get the entry point of the NreInitialize routine.
		//
		if ((!NT_SUCCESS(Status = LdrGetProcAddress(
				(ULONG_PTR)NrerImageBase,
				NrerImageSize,
				"NreInitialize",
				(PVOID *)&UmNreInitialize))))
		{
			DebugPrint(("InitializeNrerCoreForProcess(): LdrGetProcAddress failed, %.8x.",
					Status));
			break;
		}

		//
		// Next, resolve the NRER dispatcher routines that we'll be using.
		//
		if (!NT_SUCCESS(Status = LdrGetProcAddress(
				(ULONG_PTR)NrerImageBase,
				NrerImageSize,
				"CreateSehValidationFrame",
				(PVOID *)&UmCreateSehValidationFrame)))
		{
			DebugPrint(("InitializeNrerCoreForProcess(): Failed to resolve dispatcher symbols, %.8x.",
					Status));
			break;
		}

		//
		// Finally, get the pointer to the NreExecutionFlags exported variable so
		// that we can tell NRER how to drive itself.
		//
		if (!NT_SUCCESS(Status = LdrGetProcAddress(
				(ULONG_PTR)NrerImageBase,
				NrerImageSize,
				"NreExecutionFlags",
				(PVOID *)&NreExecutionFlags)))
		{
			DebugPrint(("InitializeNrerCoreForProcess(): Failed to resolve NreExecutionFlags, %.8x.",
					Status));
			break;
		}

		//
		// Set the execution flags that are being enforced for this process.
		//
		__try
		{
			ProbeForWrite(
					NreExecutionFlags,
					sizeof(ULONG),
					1);

			*NreExecutionFlags = GetProcessNreExecutionFlags(
					ProcessObject);

		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = GetExceptionCode();
			break;
		}

		DebugPrint(("InitializeNrerCoreForProcess(): Initialized process %p with NRE flags %.8x.",
				ProcessObject,
				*NreExecutionFlags));

	} while (0);

	//
	// If we succeeded, initialize the NRER dispatch table with what we
	// determined.
	//
	if (NT_SUCCESS(Status))
	{
		NrerDispatchTable->NrerImageBase            = NrerImageBase;
		NrerDispatchTable->NrerImageSize            = NrerImageSize;
		NrerDispatchTable->CreateSehValidationFrame = UmCreateSehValidationFrame;
		NrerDispatchTable->NreInitialize            = UmNreInitialize;
	}

	return Status;
}

//
// This routine initializes the attributes required to perform user-mode page
// mirroring.  This is not currently supported.
//
static NTSTATUS InitializeNrerExecutableMirrorForProcess(
		IN PPROCESS_OBJECT ProcessObject,
		IN PIMAGE_SET ImageSet,
		IN PIMAGE_SET_MAPPING ExecutableMapping,
		IN PVOID NtdllImageBase,
		IN ULONG NtdllImageSize,
		IN PVOID NrerImageBase,
		IN ULONG NrerImageSize)
{
	NTSTATUS Status;
	PVOID    *NewExecutableImageBase = NULL;
	PVOID    *OrigExecutableImageBase = NULL;
	PULONG   OrigExecutableImageSize = 0;

	do 
	{
		// 
		// Initialize the mirroring information for this process' executable
		//
		if ((!NT_SUCCESS(Status = LdrGetProcAddress(
				(ULONG_PTR)NrerImageBase,
				NrerImageSize,
				"NewExecutableImageBase",
				(PVOID *)&NewExecutableImageBase))) ||
		    (!NT_SUCCESS(Status = LdrGetProcAddress(
				(ULONG_PTR)NrerImageBase,
				NrerImageSize,
				"OrigExecutableImageBase",
				(PVOID *)&OrigExecutableImageBase))) ||
		    (!NT_SUCCESS(Status = LdrGetProcAddress(
				(ULONG_PTR)NrerImageBase,
				NrerImageSize,
				"OrigExecutableImageSize",
				(PVOID *)&OrigExecutableImageSize))))
		{
			DebugPrint(("InitializeNrerExecutableMirrorForProcess(): Failed to resolve mirroring information, %.8x.",
					Status));
			break;
		}

		__try
		{

			ProbeForWrite(
					NewExecutableImageBase,
					sizeof(PVOID),
					1);
			ProbeForWrite(
					OrigExecutableImageBase,
					sizeof(PVOID),
					1);
			ProbeForWrite(
					OrigExecutableImageSize,
					sizeof(ULONG),
					1);


			*NewExecutableImageBase  = (PVOID)ExecutableMapping->ImageBaseAddress;
			*OrigExecutableImageBase = (PVOID)ExecutableMapping->NreOriginalImageBase;
			*OrigExecutableImageSize = ExecutableMapping->NreOriginalImageSize;

		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = GetExceptionCode();
			break;
		}

	} while (0);

	return Status;
}

////
//
// SEH Overwrite protection
//
////

//
// This routine is responsible for creating and initialization the SEH
// validation frame for the current thread.  This validation frame is used to
// ensure that the exception handler chain for the thread has not been corrupted
// whenever an exception is issued.
//
// The way this is accomplished is by creating and initializing an APC that
// creates and initializes the validation frame for the new thread handle.  This
// APC runs before the actual entry point of the thread, so there should be no
// race conditions or other odd scenarios.
//
NTSTATUS CreateSehValidationFrameForThread(
		IN HANDLE ProcessHandle,
		IN HANDLE ThreadHandle)
{
	NRER_DISPATCH_TABLE NrerDispatchTable;
	PPROCESS_OBJECT     ProcessObject = NULL;
	PTHREAD_OBJECT      ThreadObject = NULL;
	NTSTATUS            Status;

	do 
	{
		//
		// Get the thread and process objects associated with the supplied
		// handles.
		//
		if ((!NT_SUCCESS(Status = RtleGetThreadObject(
				ThreadHandle,
				THREAD_SET_CONTEXT,
				&ThreadObject))) ||
		    (!NT_SUCCESS(Status = RtleGetProcessObject(
				ProcessHandle,
				PROCESS_VM_READ | PROCESS_VM_WRITE,
				&ProcessObject))))
		{
			DebugPrint(("CreateSehValidationFrameForThread(): Failed to acquire thread/process object, %.8x.",
					Status));
			break;
		}

		//
		// Next, we need to figure out if NRER.dll has been mapped into the
		// user-mode process and, if so, where it's been mapped at for this
		// process' image set.  Furthermore, we need to acquire the address of the
		// user-mode symbol that will take care of handling the APC that we
		// dispatch.
		//
		if (!NT_SUCCESS(Status = CreateNrerImageSetMapping(
				ProcessObject,
				NULL,
				NULL,
				&NrerDispatchTable)))
		{
			DebugPrint(("CreateSehValidationFrameForThread(): CreateNrerImageSetMapping failed, %.8x.",
					Status));
			break;
		}

		//
		// If the process has yet to initialize, queue an APC to initialize the
		// library.
		//
		if (!IsProcessExecutionState(
				ProcessObject,
				PROCESS_EXECUTION_STATE_NRER_DLL_INITIALIZED))
		{
			DebugPrint(("CreateSehValidationFrameForThread(): Initializing NRER DLL in process %p...",
					ProcessObject));

			if (!NT_SUCCESS(Status = CallNrerUserModeRoutine(
					ThreadObject,
					NrerDispatchTable.NreInitialize)))
			{
				DebugPrint(("CreateSehValidationFrameForThread(): Warning: Failed to call NreInitailize, %.8x.",
						Status));
			}

			SetProcessExecutionState(
					ProcessObject,
					PROCESS_EXECUTION_STATE_NRER_DLL_INITIALIZED,
					FALSE);
		}

		//
		// Create the SEH validation frame for this thread.
		//
		if (!NT_SUCCESS(Status = CallNrerUserModeRoutine(
					ThreadObject,
				NrerDispatchTable.CreateSehValidationFrame)))
		{
			DebugPrint(("CreateSehValidationFrameForThread(): CallNrerUserModeRoutine failed, %.8x.",
					Status));
			break;
		}

	} while (0);

	//
	// Lose references to things we no longer need.
	//
	if (ThreadObject)
		ObDereferenceObject(
				ThreadObject);
	if (ProcessObject)
		ObDereferenceObject(
				ProcessObject);

	return Status;
}

//
// Kernel-mode entry point for the SEH validation frame creation APC.  This
// routine simply deallocates the kernel-mode APC structure.
//
static VOID CreateSehValidationFrameKernelModeApcRoutine(
		IN PKAPC Apc,
		IN OUT PKNORMAL_ROUTINE *NormalRoutine,
		IN OUT PVOID *NormalContext,
		IN OUT PVOID *SystemArgument1,
		IN OUT PVOID *SystemArgument2)
{
	ExFreePool(
			Apc);
}

//
// This routine calls a NRER user-mode routine in the context of a given
// process' thread.
//
static NTSTATUS CallNrerUserModeRoutine(
		IN PTHREAD_OBJECT ThreadObject,
		IN PVOID RoutineAddress)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PKAPC    Apc = NULL;

	do
	{
		//
		// Allocate storage for the APC
		//
		if (!(Apc = (PKAPC)ExAllocatePoolWithTag(
				NonPagedPool,
				sizeof(KAPC),
				ALLOC_TAG)))
		{
			DebugPrint(("CallNrerUserModeRoutine(): ExAllocatePoolWithTag failed."));

			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		//
		// Initialize the APC context.
		//
		KeInitializeApc(
				Apc,
				ThreadObject,
				OriginalApcEnvironment,
				CreateSehValidationFrameKernelModeApcRoutine,
				NULL,
				RoutineAddress,
				UserMode,
				NULL);

		DebugPrint(("CallNrerUserModeRoutine(): Queuing APC for thread %p at %p...",
				ThreadObject,
				RoutineAddress));

		//
		// Queue the APC for execution.  Since the APC will be running within the
		// context of the thread we're wanting to initialize protection for, we
		// don't need to pass any sort of handle information.
		//
		if (!KeInsertQueueApc(
				Apc,
				0,
				0,
				0))
		{
			DebugPrint(("CallNrerUserModeRoutine(): KeInsertQueueApc failed."));

			Status = STATUS_NOT_SUPPORTED;
			break;
		}

	} while (0);

	return Status;
}
