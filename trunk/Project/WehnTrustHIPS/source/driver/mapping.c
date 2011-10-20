/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "precomp.h"

//
// Globals
//
extern PPROCESS_OBJECT SystemProcess;

//
// Indicates that an unreferenced cache flush should occur.
//
static BOOLEAN FlushUnrefImageSetCache = TRUE;

KTIMER ExpirationMonitorTimer;
KEVENT ExpirationMonitorStopEvent;
KEVENT ExpirationMonitorStopCompleteEvent;
HANDLE ExpirationMonitorThread = NULL;

// 
// TODO: Support multiple image sets
//
PIMAGE_SET GlobalImageSet = NULL;
KMUTEX     GlobalImageSetMutex;
ULONG      GlobalImageSetIdPool = 0;

#pragma code_seg("INIT")

//
// Initializes the global mutex that is used for creating the global image set
//
NTSTATUS ImageSetsInitialize()
{
	KeInitializeMutex(
			&GlobalImageSetMutex,
			0);

	return STATUS_SUCCESS;
}

#pragma code_seg()

//
// Obtains a pointer to the image set that should be used for the current
// process
//
NTSTATUS GetImageSetForProcess(
		IN PPROCESS_OBJECT ProcessObject,
		OUT PIMAGE_SET *ImageSet)
{
	//
	// TODO: Make this use list based creation
	// TODO: Make this pay attention to process privileges (config)
	// TODO: Make this process general config for when to create a new set
	//
	// If the global image set does not exist, create it.  See comment in
	// baserand.c:DriverEntry on how this can be improved.
	//
	if (!GlobalImageSet)
	{
		NTSTATUS Status;

		//
		// Wait for the global image set mutex object...
		//
	 	while (!NT_SUCCESS(Status = KeWaitForMutexObject(
				&GlobalImageSetMutex,
				Executive,
				KernelMode,
				FALSE,
				NULL)));

		//
		// If we acquired the mutex...
		//
		if (Status == STATUS_SUCCESS)
		{
			if (!GlobalImageSet)
				CreateImageSet(
						&GlobalImageSet);

			KeReleaseMutex(
					&GlobalImageSetMutex,
					FALSE);
		}
	}

	*ImageSet = GlobalImageSet;

	//
	// Increment the image set's reference count
	//
	if (*ImageSet)
		ReferenceImageSet(
				*ImageSet);

	return (*ImageSet)
		? STATUS_SUCCESS
		: STATUS_NOT_FOUND;
}

//
// Create an initialized instance of an image set
//
NTSTATUS CreateImageSet(
		OUT PIMAGE_SET *ImageSet)
{
	PIMAGE_SET NewSet = NULL;
	ULONG_PTR  CurrentAddress = 0;
	NTSTATUS   Status = STATUS_SUCCESS;
	BOOLEAN    Valid  = FALSE;

	//
	// Allocate the image set buffer
	//
	if ((NewSet = (PIMAGE_SET)ExAllocatePoolWithTag(
			PagedPool,
			sizeof(IMAGE_SET),
			ALLOC_TAG)))
	{
		//
		// Zero out the buffer
		//
		RtlZeroMemory(
				NewSet,
				sizeof(IMAGE_SET));

		//
		// Set the output image set.
		//
		*ImageSet = NewSet;

		//
		// Initialize the image mapping list
		//
		InitializeListHead(
				&NewSet->ImageMappingList);

		
		//
		// Initialize the image mapping list mutex
		//
		KeInitializeMutex(
				&NewSet->ImageMappingListMutex,
				0);

		//
		// Initialize the number of references to 1
		//
		ReferenceImageSet(
				NewSet);

		//
		// Increment the total number of allocated image sets
		//
		IncrementExecutiveCounter(
				ImageSetCounter,
				0);

		//
		// Set the image set's high and low start mapping address based of a
		// random start address
		//
		do
		{
			 CurrentAddress = RngRand() & BASE_RANDOMIZATION_MAPPING_MASK;

			 // 
			 // Do not start image mappings below 0x00600000 to avoid conflicts
			 // with non-relocated executables and other such things
			 //
			 if (CurrentAddress < 0x00600000)
				 continue;

			 //
			 // Do not start above an address that is near where the PEB/TEB and
			 // shareduserdata is mapped
			 //
			 if (CurrentAddress >= 0x7fd00000)
				 continue;

			 Valid = TRUE;

		} while (!Valid);
		
		NewSet->ImageMappingLowStartAddress  = CurrentAddress;
		NewSet->ImageMappingHighStartAddress = CurrentAddress;

		InitializeListHead(
				&NewSet->SortedAddressList);

		//
		// Set this image set's unique identifier
		//
		NewSet->Identifier = InterlockedIncrement(
				&GlobalImageSetIdPool);

		DebugPrint(("CreateImageSet(): Image Set %lu starts at %.8x.",
				NewSet->Identifier,
				NewSet->ImageMappingLowStartAddress));

		NewSet->QueryNameStringBufferSize = 2048;

		//
		// Create the storage directory for the image set
		//
		if (!NT_SUCCESS(Status = CreateImageSetStorageDirectory(
				NewSet)))
		{
			DebugPrint(("CreateImageSet(): CreateImageSetStorageDirectory failed, %.8x.",
					Status));
		}

		//
		// Storage directory flushing occurs later...
		//
		FlushUnrefImageSetCache = TRUE;

		Status = STATUS_SUCCESS;
	}

	// 
	// Cleanup on failure and determine the actual status
	//
	if (!NT_SUCCESS(Status))
	{
		DereferenceImageSet(
				NewSet);

		NewSet = NULL;
	}
	else if (!NewSet)
		Status = STATUS_NO_MEMORY;

	//
	// Set the outbound image set
	//
	*ImageSet = NewSet;

	return Status;
}

//
// Creates the storage directory on disk for storing randomized DLLs for a 
// given image set.
//
NTSTATUS CreateImageSetStorageDirectory(
		IN PIMAGE_SET ImageSet)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK   IoStatus;
	UNICODE_STRING    DirectoryPath;
	NTSTATUS          Status;
	HANDLE            DirectoryHandle = NULL;
	LONG              Written = 0;

	do
	{
		ImageSet->StorageDirectoryPath.MaximumLength = 1024;
		ImageSet->StorageDirectoryPath.Length        = 0;

		//
		// Allocate storage for the storage directory path
		//
		if (!(ImageSet->StorageDirectoryPath.Buffer = (PWSTR)ExAllocatePoolWithTag(
				PagedPool,
				ImageSet->StorageDirectoryPath.MaximumLength,
				ALLOC_TAG)))
		{
			DebugPrint(("CreateImageSetStorageDirectory(): ExAllocatePoolWithTag failed."));

			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		RtlZeroMemory(
				ImageSet->StorageDirectoryPath.Buffer,
				ImageSet->StorageDirectoryPath.MaximumLength);

		//
		// Build the unique path to the storage directory on disk
		//
		if ((Written = _snwprintf(
				ImageSet->StorageDirectoryPath.Buffer,
				ImageSet->StorageDirectoryPath.MaximumLength / sizeof(WCHAR) - sizeof(WCHAR),
				L"%s%lu", 
				RANDOMIZED_FILE_STORAGE_PATH,
				ImageSet->Identifier)) >= 0)
		{
			ImageSet->StorageDirectoryPath.Length = (USHORT)(Written * sizeof(WCHAR));
		}
		else
		{
			ASSERT(Written >= 0);
		}
				
		InitializeObjectAttributes(
				&ObjectAttributes,
				&ImageSet->StorageDirectoryPath,
				OBJ_KERNEL_HANDLE,
				NULL,
				NULL);

		//
		// Create the directory
		//
		if (!NT_SUCCESS(Status = ZwCreateFile(
				&DirectoryHandle,
				GENERIC_READ|GENERIC_WRITE,
				&ObjectAttributes,
				&IoStatus,
				NULL,
				FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				FILE_OPEN_IF,
				FILE_DIRECTORY_FILE,
				NULL,
				0)))
		{
			DebugPrint(("CreateImageSet(): ZwCreateFile(%wZ) failed, %.8x.",
					&ImageSet->StorageDirectoryPath,
					Status));
			break;
		}

	} while (0);


	//
	// Close the handle if it was opened
	//
	if (DirectoryHandle)
		ZwClose(
				DirectoryHandle);

	return Status;
}

//
// Enumerate through the list of image sets
//
NTSTATUS EnumerateImageSets(
		IN ULONG Index,
		OUT PIMAGE_SET *ImageSet)
{
	//
	// We don't support more than one image set
	//
	if ((Index > 0) ||
	    (!GlobalImageSet))
		return STATUS_NOT_FOUND;

	//
	// Right now we only support one global image set
	//
	ReferenceImageSet(
			GlobalImageSet);

	*ImageSet = GlobalImageSet;

	return STATUS_SUCCESS;
}

//
// Creates an initialized instance of an image set
//
NTSTATUS CreateImageSetMapping(
		IN PUNICODE_STRING FileName OPTIONAL,
		IN PUNICODE_STRING RandomizedFileName OPTIONAL,
		OUT PIMAGE_SET_MAPPING *Mapping)
{
	PIMAGE_SET_MAPPING NewMapping = NULL;

	//
	// Allocate the image set mapping buffer
	//
	if ((NewMapping = (PIMAGE_SET_MAPPING)ExAllocatePoolWithTag(
			PagedPool,
			sizeof(IMAGE_SET_MAPPING),
			ALLOC_TAG)))
	{
		//
		// Zero out the buffer
		//
		RtlZeroMemory(
				NewMapping,
				sizeof(IMAGE_SET_MAPPING));

		//
		// Initialize the image set mapping's state and event
		//
		KeInitializeEvent(
				&NewMapping->StateEvent,
				NotificationEvent,
				FALSE);

		NewMapping->State = ImageSetMappingInProgress;

		//
		// Duplicate the file name that was passed in, if any.
		//
		if (FileName)
			RtleCopyUnicodeString(
					&NewMapping->FileName,
					FileName);

		//
		// If a randomized file name is supplied, duplicate it
		//
		if (RandomizedFileName)
			RtleCopyUnicodeString(
					&NewMapping->RandomizedFileName,
					RandomizedFileName);

		//
		// Initialize the number of references to 1
		//
		ReferenceImageSetMapping(
				NewMapping);

		//
		// Increment the total number of randomized DLLs
		//
		IncrementExecutiveCounter(
				RandomizedDllCounter,
				0);
	}

	return ((*Mapping = NewMapping))
		? STATUS_SUCCESS
		: STATUS_NO_MEMORY;
}

//
// Insert a mapping into the list of mappings for the image set
//
NTSTATUS InsertImageSetMapping(
		IN PIMAGE_SET ImageSet,
		IN PIMAGE_SET_MAPPING Mapping,
		IN BOOLEAN LockList)
{
	//
	// Increment the mapping's reference count as it's being tossed into the list
	//
	ReferenceImageSetMapping(
			Mapping);

	//
	// Acquire the image mapping list mutex and insert the mapping into the list
	//
	if (LockList)
		LockImageSetMappingList(
				ImageSet);

	InsertHeadList(
			(PLIST_ENTRY)&ImageSet->ImageMappingList,
			(PLIST_ENTRY)Mapping);

	if (LockList)
		UnlockImageSetMappingList(
				ImageSet);

	return STATUS_SUCCESS;
}

//
// Remove an image mapping from the list of mappings and decrement its reference
// count.  This function expects to be called in a lock context.
//
NTSTATUS RemoveImageSetMapping(
		IN PIMAGE_SET ImageSet,
		IN PIMAGE_SET_MAPPING Mapping)
{
	//
	// Remove the entry from the list
	//
	RemoveEntryList(
			(PLIST_ENTRY)Mapping);

	//
	// Unclaim the region that the image set mapping occupies
	//
	UnclaimImageSetMappingAddress(
			ImageSet,
			(ULONG_PTR)Mapping->MappingBaseAddress);

	//
	// Set the state of the image set mapping to expired, not that it really
	// matters as nothing will reference after this point due to the fact that it
	// wont be resolvable via the image set mapping list.
	//
	Mapping->State = ImageSetMappingExpired;

	// 
	// Decrement its reference count
	//
	DereferenceImageSetMapping(
			Mapping);

	return STATUS_SUCCESS;
}

//
// Acquires the image set mapping list lock
//
VOID LockImageSetMappingList(
		IN PIMAGE_SET ImageSet)
{
	ASSERT(KeGetCurrentIrql() <= PASSIVE_LEVEL);

	KeWaitForMutexObject(
			&ImageSet->ImageMappingListMutex,
			Executive,
			KernelMode,
			FALSE,
			NULL);
}

//
// Acquires the image set mapping list lock
//
VOID UnlockImageSetMappingList(
		IN PIMAGE_SET ImageSet)
{
	ASSERT(KeGetCurrentIrql() <= PASSIVE_LEVEL);

	KeReleaseMutex(
			&ImageSet->ImageMappingListMutex,
			FALSE);
}

//
// Increment the reference count on the image set
//
VOID ReferenceImageSet(
		IN PIMAGE_SET ImageSet)
{
	InterlockedIncrement(
			&ImageSet->References);
}

//
// Decrement the image set's reference count, potentially destroying it
//
VOID DereferenceImageSet(
		IN PIMAGE_SET ImageSet)
{
	//
	// Blow up if the number of references is <= 0
	//
	ASSERT(ImageSet->References > 0);

	//
	// If the reference count drops to zero, clean up
	//
	if (InterlockedDecrement(
			&ImageSet->References) == 0)
	{
		PIMAGE_SET_MAPPING Mapping = NULL;

		//
		// Flush out the image mapping list
		//
		while ((Mapping = (PIMAGE_SET_MAPPING)RemoveHeadList(
				&ImageSet->ImageMappingList)) != (PIMAGE_SET_MAPPING)&ImageSet->ImageMappingList)
			DereferenceImageSetMapping(Mapping);

		//
		// Free the scratch buffer for querying name strings if it was
		// allocated
		//
		if (ImageSet->QueryNameStringBuffer)
			ExFreePool(
					ImageSet->QueryNameStringBuffer);

		//
		// Free the storage directory path
		//
		if (ImageSet->StorageDirectoryPath.Buffer)
			ExFreePool(
					ImageSet->StorageDirectoryPath.Buffer);

		//
		// Deallocate the image set context
		//
		ExFreePool(
				ImageSet);

		//
		// Decrement the total number of image sets
		//
		DecrementExecutiveCounter(
				ImageSetCounter,
				0);
	}
}

// 
// Returns the next available mapping address for use with creating a new image
// set mapping.  This function assumes that it's called in a locked context.
//
ULONG_PTR ClaimNextImageSetMappingAddress(
		IN PIMAGE_SET ImageSet,
		IN ULONG ImageSize)
{
	PSORTED_ADDRESS CurrentAddress, PreviousAddress = NULL, NewAddress = NULL;
	ULONG_PTR       MappingAddress = 0;
	BOOLEAN         UpdateHigh = FALSE;
	ULONG           RandPadding = (RngRand() & 0xf) * 0x10000;

	//
	// Allocate storage for the new sorted address entry
	//
	if (!(NewAddress = (PSORTED_ADDRESS)ExAllocatePoolWithTag(
			NonPagedPool,
			sizeof(SORTED_ADDRESS),
			ALLOC_TAG)))
	{
		DebugPrint(("ClaimNextImageSetMappingAddress(): ExAllocatePoolWIthTag failed."));
		return 0;
	}

	//
	// Determine the image size that includes the random padding
	//
	ImageSize = Round16Page(ImageSize) + RandPadding;

	ASSERT((ImageSize & 0xffff) == 0);

	// 
	// Check to see if this image size would cause us to go past
	// MmHighestUserAddress based on the current high start point
	//
	if ((ImageSet->ImageMappingHighStartAddress + ImageSize) >= (ULONG_PTR)MM_HIGHEST_USER_ADDRESS)
	{
		MappingAddress = ImageSet->ImageMappingLowStartAddress - ImageSize;

		NewAddress->BeginAddress              = MappingAddress;
		NewAddress->EndAddress                = MappingAddress + ImageSize;
		NewAddress->Size                      = ImageSize;
		ImageSet->ImageMappingLowStartAddress = NewAddress->BeginAddress;
	}
	else
	{
		// 
		// Walk the sorted address list searching for a gap that is big enough to
		// hold an image of the supplied size.
		//
		for (CurrentAddress = (PSORTED_ADDRESS)ImageSet->SortedAddressList.Flink;
			  CurrentAddress != (PSORTED_ADDRESS)&ImageSet->SortedAddressList;
			  PreviousAddress = CurrentAddress, 
			  CurrentAddress = (PSORTED_ADDRESS)CurrentAddress->Entry.Flink)
		{
			ULONG Difference;

			//
			// No previous address? Then we have nothing to compare against.
			//
			if (!PreviousAddress)
				continue;

			// 
			// If the addresses wrap, skip the calculation.  This should never happen
			// because the list is sorted, but should it occur, this is will act as a
			// fail-safe.
			//
			if (CurrentAddress->BeginAddress < PreviousAddress->EndAddress)
			{
				ASSERT(CurrentAddress->BeginAddress >= PreviousAddress->EndAddress);
				continue;
			}

			Difference = CurrentAddress->BeginAddress - PreviousAddress->EndAddress;

			//
			// If the difference between the current image base address and the
			// previous image's aligned end address is greater than the image size,
			// we'll use the ending aligned address as our start point.
			//
			if (Difference > ImageSize)
			{
				InsertEntryList(
						PreviousAddress, 
						CurrentAddress, 
						NewAddress);

				NewAddress->BeginAddress      = PreviousAddress->EndAddress;
				NewAddress->EndAddress        = NewAddress->BeginAddress + ImageSize;
				NewAddress->Size              = ImageSize;
				MappingAddress                = NewAddress->BeginAddress;
				break;
			}
		}
	}

	// 
	// If we were unable to find a location to squeeze the image into, allocate
	// it from the highest starting address.
	//
	if (!MappingAddress)
	{
		MappingAddress = ImageSet->ImageMappingHighStartAddress;

		InsertTailList(
   			&ImageSet->SortedAddressList,
				(PLIST_ENTRY)NewAddress);

		// 
		// Initialize the new address entry
		//
		NewAddress->BeginAddress      = MappingAddress;
		NewAddress->EndAddress        = MappingAddress + ImageSize;
		NewAddress->Size              = ImageSize;

		//
		// Update the high start address
		//
		ImageSet->ImageMappingHighStartAddress = NewAddress->EndAddress;
   }

	return MappingAddress;
}

//
// Unclaims the supplied address in an image set by removing it from the sorted
// address list such that future image mappings can claim it if necessary.
//
VOID UnclaimImageSetMappingAddress(
		IN PIMAGE_SET ImageSet,
		IN ULONG_PTR BaseAddress)
{
	PSORTED_ADDRESS CurrentAddress, PreviousAddress = NULL;

	//
	// Walk the sorted address list searching for an address entry that matches
	// the supplied base address
	//
	for (CurrentAddress = (PSORTED_ADDRESS)ImageSet->SortedAddressList.Flink;
	     CurrentAddress != (PSORTED_ADDRESS)&ImageSet->SortedAddressList;
	     PreviousAddress = CurrentAddress,
	     CurrentAddress = (PSORTED_ADDRESS)CurrentAddress->Entry.Flink)
	{
		//
		// If the current base address matches the one we're removing, well,
		// remove it!
		//
		if (CurrentAddress->BeginAddress == BaseAddress)
		{
			//
			// If the base address being removed is the high start address,
			// condense it to the previous address end aligned address or use the
			// low address as the high address
			//
			if (CurrentAddress->EndAddress == ImageSet->ImageMappingHighStartAddress)
			{
				ImageSet->ImageMappingHighStartAddress = (PreviousAddress) 
					? PreviousAddress->EndAddress 
					: ImageSet->ImageMappingLowStartAddress;
			}
			//
			// If the base address being removed is the low start address, move the
			// low start address up, but only if there's a next image
			//
			else if (CurrentAddress->BeginAddress == ImageSet->ImageMappingLowStartAddress)
			{
				PSORTED_ADDRESS NextAddress = (PSORTED_ADDRESS)CurrentAddress->Entry.Flink;

				if (NextAddress != (PSORTED_ADDRESS)&ImageSet->SortedAddressList)
					ImageSet->ImageMappingLowStartAddress = NextAddress->BeginAddress;
			}

			RemoveEntryList(
					(PLIST_ENTRY)CurrentAddress);

			ExFreePool(
					CurrentAddress);

			break;
		}
	}
}

//
// Try to locate a mapping for the supplied section object.  If one already
// exists in the image set supplied, use it, otherwise create a new one.
//
NTSTATUS LookupRandomizedImageMapping(
		IN PIMAGE_SET ImageSet,
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN ULONG ViewSize,
		OUT PIMAGE_SET_MAPPING *Mapping)
{
	PIMAGE_SET_MAPPING FoundMapping = NULL;
	PFILE_OBJECT       FilePointer;
	NTSTATUS           Status;
	BOOLEAN            Create = FALSE;
	
	FilePointer = GetSectionObjectControlAreaFilePointer(
			SectionObject);

	//
	// Acquire the image set's image mapping list lock
	//
	LockImageSetMappingList(
			ImageSet);

	//
	// Try to locate the image mapping for the supplied section object in the
	// list of already mapped entries.  If it cannot be found, create a mapping
	// for it in this image set.
	//
	if (!NT_SUCCESS(Status = FindRandomizedImageMapping(
			ImageSet,
			FilePointer,
			&FoundMapping)))
	{
		Create = TRUE;
	}
	else
	{
		//
		// If we find an existing instance of an image, we must check to see if
		// the contents of the file differ from that which is stored in the
		// randomized instance of the image.  If it is different and the image in
		// memory can be expired, we do so and then create a new randomized
		// instance.  Otherwise, we simply re-use the existing instance.
		//
		Create = ExpireImageMappingIfNecessary(
				ImageSet,
				FoundMapping,
				FilePointer);
	}

	//
	// If we are supposed to create an image set mapping, create the initial
	// mapping context and then finish the process after releasing the image set
	// mapping list lock
	//
	if (Create)
	{
		//
		// If we're creating a new mapping but we already found an existing one,
		// lose our reference to the existing one such that it can be destroyed.
		//
		if (FoundMapping)
		{
			DereferenceImageSetMapping(
					FoundMapping);

			FoundMapping = NULL;
		}

		//
		// Create the image set mapping context, but do not create the randomized
		// image mapping.  This operation inserts the mapping into the image set
		// mapping list such that it can be found by subsequent calls to
		// FindRandomizedImageMapping
		//
		if (NT_SUCCESS(Status = CreateImageSetMapping(
				NULL,
				NULL,
				&FoundMapping)))
		{
			InsertImageSetMapping(
					ImageSet,
					FoundMapping,
					FALSE);
		}

		//
		// Set the logical file name that is associated with the image set mapping
		//
		SetImageSetMappingLogicalFileName(
				FoundMapping,
				FilePointer);
	}

	//
	// Release the lock on the image set's mapping list
	//
	UnlockImageSetMappingList(
			ImageSet);

	//
	// If we failed in one way or another, do not continue.
	//
	if (!NT_SUCCESS(Status))
	{
		*Mapping = NULL;

		return Status;
	}

	// 
	// If the create flag is set, attempt to create a randomized image mapping.
	// This flag will be set if a mapping wasn't found or if a previously
	// existing mapping was expired in favor of a more up-to-date image on disk.
	//
	if (Create)
	{
		//
		// Create the initial image mapping state.  If it fails, destroy it and
		// return NULL to the caller.
		//
		if (!NT_SUCCESS(Status = CreateRandomizedImageMapping(
				ImageSet,
				ProcessObject,
				SectionObject,
				ViewSize,
				FoundMapping)))
		{
			DebugPrint(("LookupRandomizedImageMapping(): CreateRandomizedImageMapping failed, %.8x.",
					Status));

			// 
			// Remove the mapping from the image set
			//
			RemoveImageSetMapping(
					ImageSet,
					FoundMapping);

			//
			// Lose our reference to the image set mapping
			//
			DereferenceImageSetMapping(
					FoundMapping);

			FoundMapping = NULL;
		}
	}
	//
	// If the image mapping was not created, wait for it to enter a state that is
	// usable by us.  If it never enters that state, we lose our reference and
	// pass back NULL.
	//
	else
	{
		NTSTATUS WaitStatus;
		BOOLEAN  Completed = TRUE;

		//
		// If the mapping is currently in progress, set the completed flag to
		// FALSE such that we will wait for completion.  Otherwise, we can avoid
		// calling KeWaitForSingleObject.
		//
		if (FoundMapping->State == ImageSetMappingInProgress)
			Completed = FALSE;

		//
		// Keep waiting until we get information about whether or not the image
		// set mapping was successfully created.  Once the state has changed to
		// either expired or initialized, the mapping's event will remain signaled
		// indefinitely.
		//
		while (!Completed)
		{
			switch ((WaitStatus = KeWaitForSingleObject(
					&FoundMapping->StateEvent,
					Executive,
					KernelMode,
					TRUE,
					NULL)))
			{
				case STATUS_SUCCESS:
					//
					// If the mapping's state is no longer in progress, we can stop
					// waiting on the event.
					//
					if (FoundMapping->State != ImageSetMappingInProgress)
						Completed = TRUE;
					break;
				default:
					DebugPrint(("LookupRandomizedImageMapping(): KeWaitForSingleObject failed, %.8x.",
							WaitStatus));
					break;
			}
		}

		//
		// If the mapping is initialized, pass it back to the caller
		//
		if (FoundMapping->State == ImageSetMappingInitialized)
			Status    = STATUS_SUCCESS;
		//
		// Otherwise, if the mapping has expired or failed to initialize,
		// pass back NULL to the caller.
		//
		else if (FoundMapping->State == ImageSetMappingExpired)
		{
			DereferenceImageSetMapping(
					FoundMapping);

			FoundMapping = NULL;
			Status       = STATUS_UNSUCCESSFUL;
		}
	}
		
	//
	// Set the outbound pointer to the mapping that was located/created
	//
	*Mapping = FoundMapping;

	return Status;
}

//
// TODO: Make this a non-linear search if necessary (PERF)
//
// Locate a given image mapping in the list of mapped images for an image set.
//
// This function is called from a lock context.
//
NTSTATUS FindRandomizedImageMapping(
		IN PIMAGE_SET ImageSet,
		IN PFILE_OBJECT FileObject,
		OUT PIMAGE_SET_MAPPING *Mapping)
{
	PLIST_ENTRY CurrentEntry;
	NTSTATUS    Status;
	ULONG       ActualSize;

	//
	// Zero out the output pointer
	//
	*Mapping = NULL;

	//
	// If no query name string buffer has been allocated for this
	// image set, do it now.
	//
	if ((!ImageSet->QueryNameStringBuffer) &&
	    (!(ImageSet->QueryNameStringBuffer = (POBJECT_NAME_INFORMATION)ExAllocatePoolWithTag(
			PagedPool,
			ImageSet->QueryNameStringBufferSize,
			ALLOC_TAG))))
	{
		DebugPrint(("FindRandomizedImageMapping(): Failed to allocate query name string storage."));

		return STATUS_NO_MEMORY;
	}
	else if (!NT_SUCCESS(Status = ObQueryNameString(
			FileObject,
			ImageSet->QueryNameStringBuffer,
			ImageSet->QueryNameStringBufferSize,
			&ActualSize)))
	{
		DebugPrint(("FindRandomizedImageMapping(): ObQueryNameString failed, %.8x.",
				Status));

		return Status;
	}

	//
	// Reset the status to STATUS_NOT_FOUND
	//
	Status = STATUS_NOT_FOUND;

	//
	// Enumerate through the list of image mappings, comparing each with the
	// provided file name
	//
	for (CurrentEntry = ImageSet->ImageMappingList.Flink;
	     CurrentEntry != &ImageSet->ImageMappingList;
	     CurrentEntry = CurrentEntry->Flink)
	{
		PIMAGE_SET_MAPPING CurrentMapping = (PIMAGE_SET_MAPPING)CurrentEntry;

		//
		// If the file name's match, return the mapping in the output pointer and
		// set the status to success
		//
		if (RtlEqualUnicodeString(
				&ImageSet->QueryNameStringBuffer->Name,
				&CurrentMapping->FileName,
				TRUE))
		{
			//
			// Increment the mapping's reference count
			//
			ReferenceImageSetMapping(
					CurrentMapping);

			*Mapping = CurrentMapping;

			Status = STATUS_SUCCESS;

			break;
		}
	}

	return Status;
}

//
// Create a new randomized image mapping and inserts it into the image set.  By
// default, this randomization will occur in the context of the System process
// to prevent abuse from user-mode applications.  However, for compatibility
// reasons, AV products (such as McAfee) have required in-process randomization
// to prevent deadlocks.  This requirement may very likely introduce subtle
// local privilege escalation vulnerabilities.
//
NTSTATUS CreateRandomizedImageMapping(
		IN PIMAGE_SET ImageSet,
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		IN ULONG ViewSize,
		IN PIMAGE_SET_MAPPING NewMapping)
{
	OBJECT_ATTRIBUTES        ObjectAttributes;
	IO_STATUS_BLOCK          IoStatus;
	PUNICODE_STRING          RandomizedImageFilePath = NULL;
	PSECTION_OBJECT          RawImageFileSectionObject = NULL;
	PPROCESS_OBJECT          RandomizeWithinProcess = SystemProcess;
	LARGE_INTEGER            FileModificationTime = { 0 };
	LARGE_INTEGER            SectionOffset = { 0 };
	LARGE_INTEGER            SectionSize = { 0 };
	PFILE_OBJECT             RawFileObject = NULL;
	KAPC_STATE               ApcState;
	NTSTATUS                 Status = STATUS_SUCCESS;
	BOOLEAN                  Attached = FALSE;
	BOOLEAN                  IsExecutable = FALSE;
	HANDLE                   RawImageFileSectionHandle = NULL;
	HANDLE                   RawImageFileHandle = NULL;
	HANDLE                   RandomizedImageFileHandle = NULL;
	HANDLE                   RandomizedSectionHandle = NULL;
	ULONG                    TempViewSize = ViewSize;
	PVOID                    RawImageFileBase = NULL;
	PVOID                    RandomizedImageBase = NULL;

	SectionSize.LowPart = ViewSize;

	//
	// Get the file object associated with the section being mapped
	//
	RawFileObject = GetSectionObjectControlAreaFilePointer(
			SectionObject);

	//
	// Get the image file's modification time
	//
	RtleGetFileModificationTime(
			RawFileObject,
			&FileModificationTime);

	do
	{
		//
		// Make sure the image set mapping has a logical file name associated with
		// it
		//
		if (!NewMapping->FileName.Buffer)
		{
			DebugPrint(("CreateRandomizedImageMapping(): Image mapping does not have a file name associated with it."));

			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		//
		// If the passed in process' state indicates that we should randomize
		// within the process' context, then we shall do so now.
		//
		if (IsProcessExecutionState(
				ProcessObject,
				PROCESS_EXECUTION_STATE_REQUIRES_IN_PROCESS_MAPPING))
			RandomizeWithinProcess = ProcessObject;
		else
		{
			KeStackAttachProcess(
					SystemProcess,
					&ApcState);

			Attached = TRUE;
		}

		//
		// If we aren't attached but the current process differs from the one we
		// need to reference addresses in, then we attach.
		//
		if ((!Attached) &&
			 (RandomizeWithinProcess != IoGetCurrentProcess()))
		{
			KeStackAttachProcess(
					RandomizeWithinProcess,
					&ApcState);

			Attached = TRUE;
		}

		//
		// Initialize some of the mapping's attributes, such as modification time
		// and segment view size
		//
		NewMapping->FileModificationTime = FileModificationTime;
		NewMapping->ViewSize             = GetSectionObjectSegmentSize(
				SectionObject)->LowPart;
		//
		// Open the file associated with the section object being mapped for
		// read access
		//
		InitializeObjectAttributes(
				&ObjectAttributes,
				&NewMapping->FileName,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);

		if (!NT_SUCCESS(Status = ZwOpenFile(
				&RawImageFileHandle,
				GENERIC_READ,
				&ObjectAttributes,
				&IoStatus,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				0)))
		{
			DebugPrint(("CreateRandomizedImageMapping(): ZwOpenFile(%wZ) failed, %.8x.",
					&NewMapping->FileName,
					Status));
			break;
		}

		//
		// Create a temporary non-image section object for use with mapping a 
		// view of the raw contents of the file
		//
		InitializeObjectAttributes(
				&ObjectAttributes,
				NULL,
				OBJ_KERNEL_HANDLE,
				NULL,
				NULL);

		SectionSize.LowPart = 0;

		if (!NT_SUCCESS(Status = ZwCreateSection(
				&RawImageFileSectionHandle,
				SECTION_ALL_ACCESS,
				&ObjectAttributes,
				&SectionSize,
				PAGE_WRITECOPY,
				SEC_COMMIT,
				RawImageFileHandle)))
		{
			DebugPrint(("CreateRandomizedImageMapping(): ZwCreateSection(%wZ) failed, %.8x.",
					&NewMapping->FileName,
					Status));
			break;
		}

		//
		// Get the section object associated with the temporary non-image section
		// that was created
		//
		if (!NT_SUCCESS(Status = ObReferenceObjectByHandle(
				RawImageFileSectionHandle,
				0,
				NULL,
				KernelMode,
				&RawImageFileSectionObject,
				NULL)))
		{
			DebugPrint(("CreateRandomizedImageMapping(): ObReferenceObjectByHandle(%wZ) failed, %.8x.",
					&NewMapping->FileName,
					Status));
			break;
		}

		//
		// Let me start out by prefixing this with: I hate McAfee.
		//
		// This code has been changed to always attach to the system process to
		// circumvent some annoying logic in McAfee 8.0 consumer.  The bug was
		// that McAfee was attempting to open an object by its pointer (to obtain
		// a handle) before the object had been inserted via ObInsertObject.  This
		// lead to the object being in an unexpected state whereby the
		// ObjectCreateInfo attribute of the OBJECT_HEADER was not valid because
		// it had been substituted with an EPROCESS_QUOTA_BLOCK structure.
		//
		Status = PerformImageFileRandomization(
				RandomizeWithinProcess,
				ImageSet,
				NewMapping,
				RawImageFileSectionObject,
				GetSectionObjectSegmentSize(
					SectionObject)->LowPart);

		//
		// If either path failed, note it as such.
		//
		if (!NT_SUCCESS(Status))
		{
			DebugPrint(("CreateRandomizedImageMapping(): Failed to randomize image, %.8x (process %p==%p).",
					Status,
					ProcessObject,
					IoGetCurrentProcess()));
			break;
		}

		//
		// Acquire a read-only file handle for the randomized image file
		//
		InitializeObjectAttributes(
				&ObjectAttributes,
				&NewMapping->RandomizedFileName,
				OBJ_KERNEL_HANDLE,
				NULL,
				NULL);

		if (!NT_SUCCESS(Status = ZwCreateFile(
				&RandomizedImageFileHandle,
				GENERIC_READ,
				&ObjectAttributes,
				&IoStatus,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ,
				FILE_OPEN,
				0,
				NULL,
				0)))
		{
			DebugPrint(("CreateRandomizedImageMapping(): ZwCreateFile(%wZ) failed, %.8x.",
					NewMapping->RandomizedFileName,
					Status));
			break;
		}

		//
		// Create a section that is associated with the randomized image file
		// which will be used for subsequent calls to MmMapViewOfSection.
		//
		InitializeObjectAttributes(
				&ObjectAttributes,
				NULL,
				OBJ_KERNEL_HANDLE,
				NULL,
				NULL);

		if (!NT_SUCCESS(Status = ZwCreateSection(
				&RandomizedSectionHandle,
				SECTION_ALL_ACCESS,
				&ObjectAttributes,
				NULL,
				PAGE_EXECUTE,
				SEC_IMAGE,
				RandomizedImageFileHandle)))
		{
			DebugPrint(("CreateRandomizedImageMapping(): ZwCreateSection(%wZ) failed, %.8x.",
					RandomizedImageFilePath,
					Status));
			break;
		}

		//
		// Get the section object associated with the section handle
		//
		if (!NT_SUCCESS(Status = ObReferenceObjectByHandle(
				RandomizedSectionHandle,
				0,
				NULL,
				KernelMode,
				(PVOID *)&NewMapping->SectionObject,
				NULL)))
		{
			DebugPrint(("CreateRandomizedImageMapping(): ObReferenceObjectByHandle failed, %.8x.",
					Status));
			break;
		}

		//
		// Add the region associated with this mapping to the exemption list so
		// that it wont be mapped to in the future
		//
		if (!AddRegionExemption(
				(PVOID)NewMapping->MappingBaseAddress,
				NewMapping->ViewSize))
		{
			DebugPrint(("CreateRandomizedImageMapping(): Warning: failed to create region exemption."));
		}

		DebugPrint(("CreateRandomizedImageMapping(): Created randomized mapping for %wZ at %p.",
				&NewMapping->FileName,
				NewMapping->ImageBaseAddress));

	} while (0);

	//
	// Cleanup and lose references
	//
	if (RandomizedImageFileHandle)
		ZwClose(
				RandomizedImageFileHandle);
	if (RandomizedSectionHandle)
		ZwClose(
				RandomizedSectionHandle);
	if (RawImageFileSectionObject)
		ObDereferenceObject(
				RawImageFileSectionObject);
	if (RawImageFileSectionHandle)
		ZwClose(
				RawImageFileSectionHandle);
	if (RawImageFileHandle)
		ZwClose(
				RawImageFileHandle);
	if (RandomizedImageFilePath)
		ExFreePool(
				RandomizedImageFilePath);

	if (Attached)
	{
		KeUnstackDetachProcess(
				&ApcState);

		Attached = FALSE;
	}

	//
	// Depending on how successful we were, set the state to either initialized
	// or expired to indicate success or failure.
	//
	if (NT_SUCCESS(Status))
	{
		NewMapping->State = ImageSetMappingInitialized;

	}
	else
		NewMapping->State = ImageSetMappingExpired;

	//
	// Notify any waiters that the mapping has completed and it can now be used,
	// regardless of whether or not we succeeded
	//
	KeSetEvent(
			&NewMapping->StateEvent,
			IO_NO_INCREMENT,
			FALSE);

	return Status;
}

//
// Expires an image mapping if the file object passed in has different contents
// that that which is stored in the existing mapping and the existing mapping
// has no open references to it.  If the mapping is expired, TRUE is returned.
// Otherwise, FALSE is returned.
//
BOOLEAN ExpireImageMappingIfNecessary(
		IN PIMAGE_SET ImageSet,
		IN PIMAGE_SET_MAPPING Mapping,
		IN PFILE_OBJECT FileObject)
{
	LARGE_INTEGER CurrentFileModificationTime;
	BOOLEAN       Expired = FALSE;

	//
	// If the current file object's modification time is different than the one
	// that was obtained when the mapping was originally created, attempt to
	// expire it from the image set.  This requires that there be no dangling
	// references to the mapping itself.  Also, check to ensure that the mapping
	// is not associated with an image that has the CANNOT_UNLOAD flag set.  If
	// this is the case, the mapping can never be safely expired.  Lastly, do not
	// attempt to expire a mapping if it is in the 'InProgress' state.
	//
	if ((NT_SUCCESS(RtleGetFileModificationTime(
			FileObject,
			&CurrentFileModificationTime))) &&
	    (!RtlLargeIntegerEqualTo(
			CurrentFileModificationTime,
			Mapping->FileModificationTime)) &&
	    (!(Mapping->ImageFlags & IMAGE_FLAG_CANNOT_UNLOAD)) &&
	    (Mapping->State != ImageSetMappingInProgress) &&
	    (Mapping->SectionObject))
	{
		//
		// If there are no mapped views and the number of section references is
		// one (for the image set mapping reference), we can safely remove it.  We
		// don't have to worry about synchronization here because we're called in
		// a lock context.
		//
		if ((GetSectionObjectControlAreaNumberOfMappedViews(
				Mapping->SectionObject) == 0) &&
		    (GetSectionObjectControlAreaNumberOfSectionReferences(
				Mapping->SectionObject) == 1) &&
		    (Mapping->NumberOfTimesMapped > 0))
		{
			DebugPrint(("ExpireImageMappingIfNecessary(): Expiring unreferenced image mapping in favor of new instance for %wZ.",
					&Mapping->FileName));

			//
			// Remove the mapping from the image set.
			//
			RemoveImageSetMapping(
					ImageSet,
					Mapping);

			//
			// Set the expired flag to TRUE
			//
			Expired = TRUE;
		}
	}

	return Expired;
}

//
// Sets the image mapping's logical file name based on the name that is
// associated with the file object that is passed in
//
VOID SetImageSetMappingLogicalFileName(
		IN PIMAGE_SET_MAPPING Mapping,
		IN PFILE_OBJECT FileObject)
{
	POBJECT_NAME_INFORMATION ObjectInformation = NULL;
	NTSTATUS                 Status;
	ULONG                    ObjectInformationSize = 0;

	do
	{
		//
		// Allocate storage for ObQueryNameString
		//
		ObjectInformationSize = 4096;

		if (!(ObjectInformation = (POBJECT_NAME_INFORMATION)ExAllocatePoolWithTag(
				PagedPool,
				ObjectInformationSize,
				ALLOC_TAG)))
		{
			DebugPrint(("SetImageSetMappingLogicalFileName(): ExAllocatePoolWithTag failed."));

			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		//
		// Query the name of the file object
		//
		RtlZeroMemory(
				ObjectInformation,
				ObjectInformationSize);

		if (!NT_SUCCESS(Status = ObQueryNameString(
				FileObject,
				ObjectInformation,
				ObjectInformationSize,
				&ObjectInformationSize)))
		{
			DebugPrint(("SetImageSetMappingLogicalFileName(): ObQueryNameString failed, %.8x.",
					Status));
			break;

		}

		//
		// Copy the object's name 
		//
		RtleCopyUnicodeString(
				&Mapping->FileName,
				&ObjectInformation->Name);

	} while (0);

	//
	// Free the temporary buffer
	//
	ExFreePool(
			ObjectInformation);
}

//
// Increment the image set mapping's reference count
//
VOID ReferenceImageSetMapping(
		IN PIMAGE_SET_MAPPING Mapping)
{
	InterlockedIncrement(
			&Mapping->References);
}

//
// Decrement the image set mapping's reference count and destroy it if it's
// dropped to zero
//
VOID DereferenceImageSetMapping(
		IN PIMAGE_SET_MAPPING Mapping)
{
	//
	// Blow up if the reference count is <= 0
	//
	ASSERT(Mapping->References > 0);

	//
	// Decrement the reference count
	//
	if (InterlockedDecrement(
			&Mapping->References) == 0)
	{
		OBJECT_ATTRIBUTES ObjectAttributes;
		NTSTATUS          Status;

		DebugPrint(("DereferenceImageSetMapping(%p): Destroying mapping for %wZ (%wZ), section %p.",
				Mapping,
				&Mapping->FileName,
				&Mapping->RandomizedFileName,
				Mapping->SectionObject));

		//
		// Remove the base address associated with this region from the exemption
		// list
		//
		RemoveRegionExemption(
				(PVOID)Mapping->MappingBaseAddress,
				Mapping->ViewSize);

		//
		// If the mapping has an associated file name, remove it
		//
		if (Mapping->FileName.Buffer)
			RtleFreeUnicodeString(
					&Mapping->FileName);

		//
		// Dereference the section object
		//
		if (Mapping->SectionObject)
			ObDereferenceObject(
					Mapping->SectionObject);

		//
		// If the mapping has an associated randomized file name, remove it
		//
		if (Mapping->RandomizedFileName.Buffer)
		{
			//
			// Cleanup the file on disk
			//
			InitializeObjectAttributes(
					&ObjectAttributes,
					&Mapping->RandomizedFileName,
					OBJ_CASE_INSENSITIVE,
					NULL,
					NULL);

			if (!NT_SUCCESS(Status = ZwDeleteFile(
					&ObjectAttributes)))
			{
				DebugPrint(("DereferenceImageSetMapping(%p): Failed to delete file from disk, %.8x.",
						Mapping,
						Status));
			}

			//
			// Free the memory associated with the randomized file name
			//
			if (Mapping->RandomizedFileName.Buffer)
				RtleFreeUnicodeString(
						&Mapping->RandomizedFileName);
		}

		//
		// Free the mapping context
		//
		ExFreePool(
				Mapping);
	
		//
		// Decrement the total number of randomized DLLs
		//
		DecrementExecutiveCounter(
				RandomizedDllCounter,
				0);
	}
}

//
// Get the randomized section and image set mapping associated with the supplied
// section object.  If such a mapping does not already exist, create it.
//
NTSTATUS GetRandomizedImageMapping(
		IN PSECTION_OBJECT SectionObject,
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG ViewSize,
		OUT PSECTION_OBJECT *NewSectionObject,
		OUT PIMAGE_SET_MAPPING *OutMapping)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PIMAGE   Image  = NULL;

	//
	// If the mapping can be randomized, let's do our work
	//
	if (IsRandomizableSectionMapping(
			ProcessObject,
			SectionObject,
			&Image))
	{
		PIMAGE_SET_MAPPING Mapping = NULL;
		PLARGE_INTEGER     SegmentSize;
		PIMAGE_SET         ImageSet = NULL;

		if (!ViewSize)
		{
			SegmentSize = GetSectionObjectSegmentSize(
					SectionObject);

			ViewSize = SegmentSize->LowPart;
		}

		do
		{
			//
			// Lookup the image set for this process
			//
			if (!NT_SUCCESS(Status = GetImageSetForProcess(
					ProcessObject,
					&ImageSet)))
			{
				DebugPrint(("GetRandomizedImageMapping(): GetImageSetForProcess failed, %.8x.",
						Status));
				break;
			}

			//
			// Lookup the image mapping
			//
			if (!NT_SUCCESS(Status = LookupRandomizedImageMapping(
					ImageSet,
					ProcessObject,
					SectionObject,
					ViewSize,
					&Mapping)))
			{
				DebugPrint(("GetRandomizedImageMapping(): LookupRandomizedImageMapping failed, %.8x.",
						Status));
				break;
			}

			//
			// Check to see if the image being mapped requires a custom jump table
			//
			if (Image)
			{
#ifdef MULTIPLE_IMAGE_SETS

				#error "Need more logic for building imageset<->imageset jump tables

				//
				// Build the custom jump table for the supplied image relative to the
				// two base addresses
				//
				if (!NT_SUCCESS(Status = BuildJumpTableForImage(
						Image,
						ProcessObject,
						Mapping->ImageBaseAddress)))
				{
					DebugPrint(("GetRandomizedImageMapping(): BuildJumpTableForImage failed, %.8x.",
							Status));
					break;
				}
#endif
				//
				// Keep track of the image's flags, as they will need to be used to
				// determine whether or not a mapping can be expired, for instance.
				//
				Mapping->ImageFlags = Image->Flags;
			}
			else
				Mapping->ImageFlags = 0;

			//
			// Set the outbound section object and increment its reference count
			//
			ObReferenceObject(
					Mapping->SectionObject);

			*NewSectionObject = Mapping->SectionObject;

			//
			// We win.
			//
			InterlockedIncrement(
					&Mapping->NumberOfTimesMapped);

			Status = STATUS_SUCCESS;

		} while (0);

		//
		// Lose referenes that are no longer needed
		//
		if (Mapping)
		{
			//
			// If the caller requested a reference to the mapping, pass the one we
			// already obtained to it
			//
			if (OutMapping)
				*OutMapping = Mapping;
			else
				DereferenceImageSetMapping(
						Mapping);
		}
		if (ImageSet)
			DereferenceImageSet(
					ImageSet);
	}
	else
		Status = STATUS_NOT_SUPPORTED;
	
#ifdef MULTIPLE_IMAGE_SETS
	if (Image)
		DereferenceImage(
				Image);
#endif

	return Status;
}


//
// Check to see if the supplied section object can be randomized
//
BOOLEAN IsRandomizableSectionMapping(
		IN PPROCESS_OBJECT ProcessObject,
		IN PSECTION_OBJECT SectionObject,
		OUT PIMAGE *OutImage)
{
	PFILE_OBJECT FilePointer;
	BOOLEAN      res = FALSE;
	PIMAGE       Image = NULL;

	FilePointer = GetSectionObjectControlAreaFilePointer(
			SectionObject);

#ifdef MULTIPLE_IMAGE_SETS
	//
	// Check to see if the section has an associated image 
	//
	Image = FindImageEntryByPath(
			&FilePointer->FileName);
#endif

	//
	// First, check to see if randomization is even enabled for DLLs.  We cannot
	// prevent the randomization of DLLs that have jump tables even if the user
	// has opt'd for randomization to be disabled.
	//
	if ((!Image) &&
	    (!IsRandomizationSubsystemEnabled(
			RANDOMIZATION_SUBSYSTEM_DLL)))
	{
		DebugPrint(("IsRandomizableSectionMapping(): DLL randomizations are disabled."));
	}
	//
	// Second, make sure that the calling thread is not currently exempted from
	// mapping randomized images
	//
	else if (IsThreadExempted(
			PsGetCurrentThread()))
	{
		DebugPrint(("IsRandomizableSectionMapping(): Skipping randomization for exempted thread %p.",
				PsGetCurrentThread()));

		//
		// Increment the number of images that have been exempted
		//
		IncrementExecutiveCounter(
				RandomizationsExemptedCounter,
				0);
	}
	//
	// If the supplied process or image file is exempted, then we shant randomize
	// it.
	//
	else if ((IsProcessExempted(
			ProcessObject,
			EXEMPT_IMAGE_FILES)) ||
	         (IsImageFileExempted(
			ProcessObject,
			FilePointer)))
	{
		//
		// Do not allow the exemption of NTDLL, KERNEL32, and USER32
		//
		if ((RtleFindStringInUnicodeString(
				&FilePointer->FileName,
				L"NTDLL.DLL")) ||
		    (RtleFindStringInUnicodeString(
				&FilePointer->FileName,
				L"KERNEL32.DLL")) ||
		    (RtleFindStringInUnicodeString(
				&FilePointer->FileName,
				L"USER32.DLL")))
		{
			DebugPrint(("IsRandomizableSectionMapping(): Not allowing special image to be exempted."));

			res = TRUE;
		}
		else
		{
			DebugPrint(("IsRandomizableSectionMapping(): Randomization exempted for image: %wZ.",
					&FilePointer->FileName));
		
			//
			// Increment the exemptions counter
			//
			IncrementExecutiveCounter(
					RandomizationsExemptedCounter,
					0);
		}
	}
	//
	// If the process object passed to MmMapViewOfSection is the system process
	// and the image being mapped is not NTDLL, exempted it from being
	// randomized.
	//
	else if ((ProcessObject == SystemProcess) &&
	         (!RtleFindStringInUnicodeString(
			&FilePointer->FileName,
			L"NTDLL.DLL")))
	{
		DebugPrint(("IsRandomizedSectionMapping(): Skipping system process randomization of %wZ.",
				&FilePointer->FileName));
	}
	else
		res = TRUE;

	//
	// Set the image context on the outbound
	//
	if (OutImage)
		*OutImage = Image;

	return res;
}

//
// Acquires a unique randomized range from an image set
//
// This function is called in a locked context.
//
NTSTATUS ImageSetAcquireRandomizedRange(
		IN PIMAGE_SET ImageSet,
		IN ULONG ViewSize,
		OUT PVOID *MappingBase,
		OUT PULONG BasePadding)
{
	PLIST_ENTRY CurrentEntry = NULL;
	NTSTATUS    Status;
	BOOLEAN     Collision = TRUE;
	PVOID       NewImageBase = NULL;
	PVOID       NewMappingBase = NULL;
	ULONG       Attempts = 0;

	//
	// Keep looping until an un-used range is found
	//
	while ((Collision) &&
	       (Attempts++ < 255))
	{
		//
		// Mask the address to the image base
		//
		NewImageBase = (PVOID)ClaimNextImageSetMappingAddress(
				ImageSet,
				ViewSize);
		NewMappingBase = NewImageBase;

		// 
		// If base padding is desired, calculate the difference
		//
		if (BasePadding)
			*BasePadding = (ULONG)((ULONG_PTR)NewImageBase - (ULONG_PTR)NewMappingBase);

		// Make sure that the region doesn't go past the highest user address
		//
		if ((PVOID)((PUCHAR)NewImageBase + ViewSize) > MM_HIGHEST_USER_ADDRESS)
			continue;

		//
		// If the region is exempted, try another one
		//
		if (IsRegionExempted(
				NewMappingBase,
				ViewSize,
				NULL))
			continue;

		Collision = FALSE;
	}

	//
	// If the collision flag is not set, we won.
	//
	if (!Collision)
	{
		*MappingBase = NewMappingBase;
		Status       = STATUS_SUCCESS;
	}
	else
	{
		Status = STATUS_UNSUCCESSFUL;

		DebugPrint(("ImageSetAcquireRandomizedRange(%p): Failed to acquire range.",
				ImageSet));
	}

	//
	// Make sure the number of attempts is less than 256
	//
	ASSERT(Attempts < 255);

	return Status;
}

//
// Fixes up the time date stamp in the header of the image such that imports
// will be recalculated
//
// Note: If this is called on a USER MAPPED IMAGE, then the caller MUST use
// SEH to guard against a malicious program unmapping the image under us and
// causing a fault here !!!
//
NTSTATUS FixupRelocatedImageHeaders(
		IN PVOID RandomizedImageBase,
		IN PVOID RawImageBase)
{
	PCHAR               RawImageBaseCast = (PCHAR)RawImageBase;
	PIMAGE_DOS_HEADER   Dos = (PIMAGE_DOS_HEADER)RawImageBaseCast;
	PIMAGE_NT_HEADERS32 Nt = (PIMAGE_NT_HEADERS32)(RawImageBaseCast + Dos->e_lfanew);
	NTSTATUS            Status = STATUS_INVALID_IMAGE_FORMAT;
	
	do
	{
		if ((Dos->e_magic != IMAGE_DOS_SIGNATURE) ||
		    (Nt->Signature != IMAGE_NT_SIGNATURE))
			break;

		if (!Nt->FileHeader.SizeOfOptionalHeader)
			break;

		//
		// Increment the TimeDateStamp to force imports to be re-calculated from
		// this image
		//
		// TODO: A better option is to look for bound imports in any image we are
		// mapping: if we find one, and the timestamps + checksums match, then we
		// just loop through the bound imports block, adding a fixed delta to the
		// bound VAs.  This will make it so that bound imports still grant some
		// performance increase over regular imports.  With this system, they never
		// get used since they are always invalidated.
		//
		Nt->FileHeader.TimeDateStamp++;

		//
		// Change the base address of the image so that if a user-mode process
		// attempts to do relocation after we already have the math will be
		// correct
		//
		Nt->OptionalHeader.ImageBase = (ULONG)RandomizedImageBase;

		Status = STATUS_SUCCESS;

	} while (0);

	return Status;
}

//
// Checks to see if an image has relocation information.  
//
// This method must be wrapped in SEH!
//
BOOLEAN IsImageRelocateable(
		IN PPROCESS_OBJECT ProcessObject,
		IN PIMAGE_SET_MAPPING Mapping,
		IN PVOID ImageBase,
		IN ULONG ImageSize,
		OUT PBOOLEAN IsExecutable)
{
	PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)((PUCHAR)ImageBase + DosHeader->e_lfanew);
	BOOLEAN           Executable = FALSE;
	BOOLEAN           Result = FALSE;
	BOOLEAN           Sane = FALSE;

	do
	{
		//
		// Verify that the NT header is a valid user-mode address
		//
		ProbeForRead(
				NtHeader,
				sizeof(IMAGE_NT_HEADERS),
				1);

		//
		// Check to see if the image is an executable
		//
		if (!(NtHeader->FileHeader.Characteristics & IMAGE_FILE_DLL))
			Executable = TRUE;

		//
		// Check to see if the preferred load address is in user-mode or
		// kernel-mode
		//
		if (NtHeader->OptionalHeader.ImageBase >= (ULONG)MM_HIGHEST_USER_ADDRESS)
			break;

		Sane = TRUE;

		//
		// Check to see if relocation information has been explicitly stripped.
		// If it does, we shant relocate it.
		//
		if (NtHeader->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
			break;

		//
		// Check to see if the image is importing symbols from ntoskrnl.exe.  If
		// it is, we need to flag this image as being non-relocatable as we don't
		// attempt to do randomization for kernel-mode images.
		//
		if (LdrCheckImportedDll(
				(ULONG_PTR)ImageBase,
				ImageSize,
				NtHeader,
				"NTOSKRNL.EXE"))
		{
			DebugPrint(("IsImageRelocatable(): Image imports NTOSKRNL.EXE, marking as non-relocatable."));
			break;
		}

		//
		// Check to see if the image has any incompatible sections.  If it does,
		// flag it as not being relocatable.
		//
		if (LdrCheckIncompatibleSections(
				(ULONG_PTR)ImageBase,
				ImageSize,
				NtHeader))
		{
			DebugPrint(("IsImageRelocatable(): Image contains incompatible sections."));
			break;
		}

		//
		// If the relocation table has entries in it, the executable has
		// relocation information
		//
		if (NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
			Result = TRUE;

	} while (0);

#ifndef DISABLE_NRE_RANDOMIZATION
	//
	// If relocations are not supported by this image and it's an executable,
	// check to see if the NRE subsystem is enabled.  If so, enable NRE mirroring
	// for this image and indicate that, yes, it is relocatable, even though it
	// really isn't.
	//
	if ((!Result) &&
	    (Executable) &&
	    (Sane))
	{
		//
		// If NRE is enabled, set the process' execution state to NRER needed
		// so that when NTDLL.DLL is mapped in, the NRER user-mode DLL will
		// also be mapped
		//
		if (IsRandomizationSubsystemEnabled(
				RANDOMIZATION_SUBSYSTEM_NRE))
		{
			SetProcessExecutionState(
					ProcessObject,
					PROCESS_EXECUTION_STATE_NRER_MIRRORING_NEEDED,
					TRUE);

			//
			// Flag this mapping as requiring a mirrored copy of the original
			// region
			//
			Mapping->NreMirrorEnabled     = TRUE;
			Mapping->NreOriginalImageBase = (ULONG_PTR)NtHeader->OptionalHeader.ImageBase;
			Mapping->NreOriginalImageSize = ImageSize;

			Result = TRUE;
		}
	}
#endif

	//
	// Set whether or not the image is an executable
	//
	if (IsExecutable)
		*IsExecutable = Executable;

	return Result;
}

//
// Creates and starts the mapping expiration monitor that is used to remove
// unreferenced mappings from image sets
//
NTSTATUS StartMappingExpirationMonitor()
{
	LARGE_INTEGER DueTime = { 0 };

	// 
	// Initialize the event & timer objects
	//
	KeInitializeEvent(
			&ExpirationMonitorStopEvent,
			NotificationEvent,
			FALSE);
	KeInitializeEvent(
			&ExpirationMonitorStopCompleteEvent,
			NotificationEvent,
			FALSE);
	KeInitializeTimerEx(
			&ExpirationMonitorTimer,
			SynchronizationTimer);

	//
	// Set the timer's period
	//
	KeSetTimerEx(
			&ExpirationMonitorTimer,
			DueTime,
			MAPPING_EXPIRATION_PERIOD,
			NULL);

	if (ExpirationMonitorThread)
		ExpirationMonitorThread = NULL;

	//
	// Start the monitor thread
	//
	return PsCreateSystemThread(
			&ExpirationMonitorThread,
			0,
			NULL,
			NULL,
			NULL,
			(PKSTART_ROUTINE)ExpireUnreferencedImageSetMappings,
			NULL);
}

//
// Stops the mapping expiration monitor
//
NTSTATUS StopMappingExpirationMonitor()
{
	LARGE_INTEGER Timeout = { 0 };
	NTSTATUS      Status = STATUS_SUCCESS;

	do
	{
		//
		// If the expiration monitor stop event is valid, set the event and wait
		// for the StopComplete event to be set
		//
		Timeout.LowPart = 100000000;

		if (!NT_SUCCESS(Status = KeSetEvent(
				&ExpirationMonitorStopEvent,
				0,
				FALSE)))
		{
			DebugPrint(("StopMappingExpirationMonitor(): KeSetEvent failed, %.8x.",
					Status));
			break;
		}

		//
		// Wait for the completion event
		//
		if (!NT_SUCCESS(Status = KeWaitForSingleObject(
				&ExpirationMonitorStopCompleteEvent,
				Executive,
				KernelMode,
				FALSE,
				&Timeout)))
		{
			DebugPrint(("StopMappingExpirationMonitor(): KeWaitForSingleObject failed, %.8x.",
					Status));
			break;
		}

		//
		// Reset the completion event
		//
		KeResetEvent(
				&ExpirationMonitorStopCompleteEvent);

	} while (0);

	return Status;
}

//
// Expires unreferenced image set mappings from image sets such that memory is
// not wasted
//
VOID ExpireUnreferencedImageSetMappings(
		IN PVOID Context)
{
	PIMAGE_SET ImageSet;
	NTSTATUS   Status;
	PVOID      Objects[2];

	Objects[0] = &ExpirationMonitorStopEvent;
	Objects[1] = &ExpirationMonitorTimer;

	//
	// Wait for the stop or timer event to be satisified
	//
	while (NT_SUCCESS(Status = KeWaitForMultipleObjects(
			2,
			Objects,
			WaitAny,
			Executive,
			KernelMode,
			FALSE,
			NULL,
			NULL)))
	{
		PIMAGE_SET_MAPPING CurrentMapping;
		PLIST_ENTRY        CurrentEntry;
		ULONG              Index = (Status & 0xff);

		//
		// If the stop event was set, break out of the loop
		//
		if (Index == 0)
		{
			KeClearEvent(
					&ExpirationMonitorStopEvent);
			break;
		}

		//
		// If this is our first time through the expiration thread, then we will
		// try to flush all unreferenced entries in the randomization cache by
		// trying to remove them.
		//
		if (FlushUnrefImageSetCache)
		{
			DebugPrint(("ExpireUnreferencedImageSetMappings(): Expiring cache from previous boot..."));

			//
			// If the timer was satisfied, expire unreferenced image set mappings
			// from a previous boot.
			//
			for (Index = 0;
			     NT_SUCCESS(EnumerateImageSets(
						  Index,
						  &ImageSet));
			     Index++)
			{
				RtleFlushDirectory(
						&ImageSet->StorageDirectoryPath,
						TRUE);
			
				DereferenceImageSet(
						ImageSet);
			}

			//
			// Reset the flush cache flag.
			//
			FlushUnrefImageSetCache = FALSE;
		}

		//
		// If the timer was satisfied, expire unreferenced image set mappings
		//
		for (Index = 0;
		     NT_SUCCESS(EnumerateImageSets(
					  Index,
					  &ImageSet));
		     Index++)
		{
			//
			// TODO: PERF
			//
			// Acquire the mapping list lock as we'll be enumerating the list of
			// mappings.  This could be bad, as we don't want to hold the image set
			// mapping lock for a very long period of time as it blocks mapping of
			// images.
			//
			LockImageSetMappingList(
					ImageSet);

			for (CurrentEntry = ImageSet->ImageMappingList.Flink;
			     CurrentEntry != &ImageSet->ImageMappingList;
			    )
			{
				PLIST_ENTRY NextEntry = CurrentEntry->Flink;

				//
				// Cast
				//
				CurrentMapping = (PIMAGE_SET_MAPPING)CurrentEntry;

				//
				// If the mapping cannot be expired, skip it.
				//
				if (CurrentMapping->ImageFlags & IMAGE_FLAG_CANNOT_UNLOAD)
				{
					CurrentEntry = NextEntry;
					continue;
				}

				//
				// If the mapping is still in progress, skip it.
				//
				if (CurrentMapping->State == ImageSetMappingInProgress)
				{
					CurrentEntry = NextEntry;
					continue;
				}

				//
				// If the number of mapped views for this section is zero and the
				// section itself only has one reference (as is held by the list),
				// we shall flush it out.
				//
				if ((GetSectionObjectControlAreaNumberOfMappedViews(
						CurrentMapping->SectionObject) == 0) &&
					 (GetSectionObjectControlAreaNumberOfSectionReferences(
						CurrentMapping->SectionObject) == 1) &&
				    (CurrentMapping->NumberOfTimesMapped > 0))
				{
					DebugPrint(("ExpireUnreferencedImageSetMappings(): Expiring randomized mapping for %wZ.", &CurrentMapping->FileName));

					//
					// Remove the mapping from the image set.
					//
					RemoveImageSetMapping(
							ImageSet,
							CurrentMapping);
				}

				//
				// Point to the next entry.  We do this outside of the loop in case
				// the current entry is removed.
				//
				CurrentEntry = NextEntry;
			}

			//
			// Release the mapping list lock
			//
			UnlockImageSetMappingList(
					ImageSet);

			//
			// Drop the reference to the image set
			//
			DereferenceImageSet(
					ImageSet);
		}
	}

	//
	// Set the stop complete event
	//
	KeSetEvent(
			&ExpirationMonitorStopCompleteEvent,
			0,
			FALSE);

	PsTerminateSystemThread(
			Status);
}

//
// Gets a base address in the context of a given process that has at least
// RegionSize of free space following it
//
PVOID GetRandomizedBaseForProcess(
		IN PPROCESS_OBJECT ProcessObject,
		IN ULONG RegionSize)
{
	ULONG RegionIncrement = 0;
	ULONG Attempts = 0;
	PVOID Base = NULL;
#if 0
	ULONG Begin = 0;
	ULONG End = 0;

	//
	// Acquire the lock on the process' address space layout
	//
	AcquireProcessRegionLock(
			ProcessObject);
#endif

	//
	// Keep searching for a base address that we can use
	//
	while ((!Base) &&
	       (Attempts++ < 256))
	{
		ULONG_PTR CurrentBase;
#if 0
		PMMVAD    Vad = GetProcessVadRoot(ProcessObject);
#endif
		
		CurrentBase = GetNextRandomizedBaseForProcess(
				ProcessObject,
				RegionSize,
				RegionIncrement);

		RegionIncrement = 0;

		//
		// If the region has been exempted, skip it.
		//
		if (IsRegionExempted(
				(PVOID)CurrentBase,
				RegionSize,
				&RegionIncrement))
			continue;

		Base = (PVOID)CurrentBase;

#if 0
		//
		// A note on why this is aligned to 16 page boundaries:
		//
		// There seems to be some code (such as in smss) that assumes that the
		// base address returned from ZwAllocateVirtualMemory will be aligned on a
		// 16 page boundary.  It most likely assumes this due to the
		// dwAllocationGranularity associated with the system.  I have yet to
		// pinpoint the code that makes this assumption, but suffice to say that
		// if it gets an address that is misaligned it will surely crash and burn.
		//

		while (Vad)
		{
			Begin = (ULONG)GetVadStartingVpn(Vad) * PAGE_SIZE;
			End   = ((ULONG)GetVadEndingVpn(Vad) * PAGE_SIZE) + PAGE_SIZE;

			if ((CurrentBase < Begin) &&
			    (CurrentBase + RegionSize < Begin))
				Vad = GetVadLeftChild(Vad);
			else if (CurrentBase > End)
				Vad = GetVadRightChild(Vad);
			else
				break;
		}

		//
		// No vad entry? We're set.
		//
		if (!Vad)
			Base = (PVOID)CurrentBase;
		else
			RegionIncrement = End - Begin;
#endif
	}

#if 0
	ReleaseProcessRegionLock(
			ProcessObject);
#endif

	ASSERT(Attempts < 256);

	return Base;
}

//
// Checks to see if a region with the supplied base address exists in the
// process supplied.
//
BOOLEAN CheckProcessRegionBaseExists(
		IN PPROCESS_OBJECT ProcessObject,
		IN PVOID RegionBase)
{
	BOOLEAN Exists = FALSE;
	PMMVAD  Vad;
	
	//
	// Acquire the lock on the process' address space layout
	//
	AcquireProcessRegionLock(
			ProcessObject);
	
	Vad = GetProcessVadRoot(
			ProcessObject);

	while (Vad)
	{
		ULONG RightBegin = 0;
		ULONG Begin      = (ULONG)GetVadStartingVpn(Vad) * PAGE_SIZE;
		ULONG End        = ((ULONG)GetVadEndingVpn(Vad) * PAGE_SIZE) + PAGE_SIZE;

		if ((ULONG_PTR)RegionBase < Begin)
			Vad = GetVadLeftChild(Vad);
		else if ((ULONG_PTR)RegionBase > End)
			Vad = GetVadRightChild(Vad);
		else
		{
			Exists = TRUE;

			break;
		}
	}

	//
	// Release the lock on the process' address layout
	//
	ReleaseProcessRegionLock(
			ProcessObject);

	return Exists;
}

//
// Randomize the highest user address such that the PEB/TEB occur at random
// offsets
//
NTSTATUS RandomizeHighestUserAddress()
{
	UNICODE_STRING Symbol;
	NTSTATUS       Status = STATUS_SUCCESS;
	ULONG          Displacement = RngRand() & 0xf000;
	PVOID *        HighestAddress;

	//
	// To work with the latest DDK, we need to resolve MmHighestUserAddress so
	// that we can modify its value rather than being given its value.
	//
	RtlInitUnicodeString(
			&Symbol,
			L"MmHighestUserAddress");
	
	HighestAddress = (PVOID *)MmGetSystemRoutineAddress(
			&Symbol);

	// Handle the condition where the displacement is zero to prevent int
	// overflow.
	if (!Displacement)
		Displacement = 0x2000;

	//
	// FIXME:
	//
	// This *cannot* randomize below SharedUserData which is at 0x7ffe0000
	//
	*(ULONG_PTR *)HighestAddress -= (Displacement - 0x1000);

	//
	// Do not drop below SharedUserData
	//
	if (*(ULONG_PTR *)HighestAddress < 0x7ffe3fff)
		*(ULONG_PTR *)HighestAddress = 0x7ffe3fff;

	DebugPrint(("RandomizeHighestUserAddress(): Randomized highest user address to %p.",
			MM_HIGHEST_USER_ADDRESS));

	return Status;
}

//
// Performs the randomization of the supplied image set mapping.  This will
// either be called from the system process or from a user process.  In either
// case, the addresses will be directly referencable.
//
NTSTATUS PerformImageFileRandomization(
		IN PPROCESS_OBJECT ProcessObject,
		IN PIMAGE_SET ImageSet,
		IN PIMAGE_SET_MAPPING NewMapping,
		IN PSECTION_OBJECT RawImageFileSectionObject,
		IN ULONG ViewSize)
{
	PUNICODE_STRING RandomizedImageFilePath = NULL;
	LARGE_INTEGER   SectionOffset = { 0 };
	NTSTATUS        Status;
	BOOLEAN         IsExecutable = FALSE;
	PVOID           RawImageFileBase = NULL;
	ULONG           TempViewSize = 0;

	do
	{
		//
		// Map a WRITECOPY view of the non-image section object such that it can
		// be checked to see if the image is relocateable and, if so, used to
		// apply relocation fixups to the non-image mapped contents of the file.
		//
		if (!NT_SUCCESS(Status = OrigMmMapViewOfSection(
				RawImageFileSectionObject,
				ProcessObject,
				&RawImageFileBase,
				0,
				0,
				&SectionOffset,
				&TempViewSize, 
				0,
				MEM_COMMIT,
				PAGE_WRITECOPY)))
		{
			DebugPrint(("PerformImageFileRandomization(): OrigMmMapViewOfSection failed, %.8x.",
					Status));
			break;
		}

		//
		// Check to see if the image is relocateable
		//
		__try
		{
			if (!IsImageRelocateable(
					ProcessObject,
					NewMapping,
					RawImageFileBase,
					TempViewSize,
					&IsExecutable))
			{
				DebugPrint(("PerformImageFileRandomization(): Image %wZ is not relocateable.",
						&NewMapping->FileName));

				Status = STATUS_NOT_SUPPORTED;
				break;
			}
		
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = GetExceptionCode();

			DebugPrint(("PerformImageFileRandomization(): Exception caught during reloc check, %.8x.",
					Status));
			break;
		}

		//
		// Build the randomized image file path based on the current image set and
		// the physical file name on disk.
		//
		if (!NT_SUCCESS(Status = BuildRandomizedImageMappingFilePath(
				ImageSet,
				&NewMapping->FileName,
				&RandomizedImageFilePath)))
		{
			DebugPrint(("PerformImageFileRandomization(): BuildRandomizedImageMappingFilePath failed, %.8x.",
					Status));
			break;
		}

		//
		// Once we've determine that the image is indeed relocateable, it's time 
		// to create the image set mapping for it
		//
		RtleCopyUnicodeString(
				&NewMapping->RandomizedFileName,
				RandomizedImageFilePath);

		//
		// Try to acquire a randomized base address 
		//
		if (!NT_SUCCESS(Status = ImageSetAcquireRandomizedRange(
				ImageSet,
				ViewSize,
				&NewMapping->MappingBaseAddress,
				&NewMapping->BasePadding)))
		{
			DebugPrint(("PerformImageFileRandomization(): ImageSetAcquireRandomizedRange failed, %.8x.",
					Status));
			break;
		}


		//
		// Initialize the new mapping
		//
		NewMapping->ImageBaseAddress = NewMapping->MappingBaseAddress;
		NewMapping->IsExecutable     = IsExecutable;

		//
		// If the randomized file does not already exist, create it and randomize
		// it on disk.
		//
		if (!NT_SUCCESS(Status = CreateRandomizedImageFile(
				ImageSet,
				NewMapping,
				RandomizedImageFilePath,
				RawImageFileBase,
				TempViewSize)))
		{
			DebugPrint(("PerformImageFileRandomization(): CreateRandomizedImageFile(%wZ) failed, %.8x.",
					RandomizedImageFilePath,
					Status));
			break;
		}

	} while (0);

	//
	// Cleanup
	//
	if (RawImageFileBase)
		MmUnmapViewOfSection(
				ProcessObject,
				RawImageFileBase);
	if (RandomizedImageFilePath)
		ExFreePool(
				RandomizedImageFilePath);

	return Status;
}

//
// Builds a randomized file path based on the supplied image set and physical
// file path.  This ends up being:
//
// [RANDOMIZED_FILE_STORAGE_PATH]\[ImageSetId]\[Sha1OfFilePath]_[FileName].ext
//
NTSTATUS BuildRandomizedImageMappingFilePath(
		IN PIMAGE_SET ImageSet,
		IN PUNICODE_STRING FilePath,
		OUT PUNICODE_STRING *RandomizedFilePath)
{
	PUNICODE_STRING LocalRandomizedFilePath = NULL;
	NTSTATUS        Status = STATUS_SUCCESS;
	SHA1_CTX        Sha1;
	ULONG           RandomizedFilePathSize = 0;
	UCHAR           Sha1Digest[SHA1_HASH_SIZE];
	LONG            Written;

	SHA1_Init(&Sha1);

	do
	{
		//
		// Allocate storage for the randomized file path
		//
		RandomizedFilePathSize = 1024;

		if (!(LocalRandomizedFilePath = (PUNICODE_STRING)ExAllocatePoolWithTag(
				PagedPool,
				RandomizedFilePathSize,
				ALLOC_TAG)))
		{
			DebugPrint(("BuildRandomizedImageMappingFilePath(): ExAllocatePoolWithTag failed."));

			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		//
		// Initialize the unicode string that is to hold the file path information
		//
		LocalRandomizedFilePath->Buffer        = (PWSTR)((PUCHAR)LocalRandomizedFilePath + sizeof(UNICODE_STRING));
		LocalRandomizedFilePath->Length        = 0;
		LocalRandomizedFilePath->MaximumLength = (USHORT)(RandomizedFilePathSize - sizeof(UNICODE_STRING) - sizeof(WCHAR));

		// 
		// Calculate & finalize the SHA1 hash of the file path
		//
		SHA1_Update(
				&Sha1,
				(PUCHAR)FilePath->Buffer,
				FilePath->Length);

		SHA1_Final(
				&Sha1,
				Sha1Digest);

		//
		// Build the full file path
		//
		// TODO:
		//
		//   - Add the filename to the end of the string as well so that it's easy
		//     to identify which DLL has been randomized.
		//
		if ((Written = _snwprintf(
				LocalRandomizedFilePath->Buffer,
				(RandomizedFilePathSize - sizeof(UNICODE_STRING)) / sizeof(WCHAR),
				L"%wZ\\%.8x%.8x%.8x%.8x%.8x.dll",
				&ImageSet->StorageDirectoryPath,
				((PULONG)Sha1Digest)[0],
				((PULONG)Sha1Digest)[1],
				((PULONG)Sha1Digest)[2],
				((PULONG)Sha1Digest)[3],
				((PULONG)Sha1Digest)[4])) >= 0)
		{
			LocalRandomizedFilePath->Length = (USHORT)(Written * sizeof(WCHAR));
		}
		else
		{
			ASSERT(Written >= 0);
		}

	} while (0);

	//
	// Cleanup on failure
	//
	if (!NT_SUCCESS(Status))
	{
		if (LocalRandomizedFilePath)
			ExFreePool(
					LocalRandomizedFilePath);

		LocalRandomizedFilePath = NULL;
	}

	*RandomizedFilePath = LocalRandomizedFilePath;

	return Status;
}

//
// Creates a relocated instance of a raw image in memory and then writes 
// it to the supplied file.
//
NTSTATUS CreateRandomizedImageFile(
		IN PIMAGE_SET ImageSet,
		IN PIMAGE_SET_MAPPING Mapping,
		IN PUNICODE_STRING RandomizedFilePath,
		IN PVOID RawImageBase,
		IN ULONG RawImageSize)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK   IoStatus;
	NTSTATUS          Status;
	HANDLE            RandomizedFileHandle = NULL;
	ULONG             Offset = 0;

	do
	{
		__try
		{
			//
			// If we're not doing mirroring on this mapping, relocate the image
			//
			if (!Mapping->NreMirrorEnabled)
			{
				//
				// Relocate the raw image in memory
				//
				if (!NT_SUCCESS(Status = LdrRelocateRawImage(
						Mapping->ImageBaseAddress,
						RawImageBase,
						RawImageSize)))
				{
					DebugPrint(("CreateRandomizedImageFile(): LdrRelocateRawImage failed, %.8x.",
							Status));
					break;
				}
			}

			//
			// Fixup the image header to use the new randomized base address and to
			// offset the link time
			//
			if (!NT_SUCCESS(Status = FixupRelocatedImageHeaders(
					Mapping->ImageBaseAddress,
					RawImageBase)))
			{
				DebugPrint(("CreateRandomizedImageFile(): FixupRelocatedImageHeaders failed, %.8x.",
						Status));
				break;
			}

		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = GetExceptionCode();

			DebugPrint(("CreateRandomizedImageFile(): Caught exception during reloc, %.8x.",
					Status));
			break;
		}

		//
		// Create the randomized image file on disk and prepare to copy
		// the relocated instance into it.
		//
		InitializeObjectAttributes(
				&ObjectAttributes,
				RandomizedFilePath,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);

		if (!NT_SUCCESS(Status = ZwCreateFile(
				&RandomizedFileHandle,
				FILE_WRITE_DATA,
				&ObjectAttributes,
				&IoStatus,
				NULL,
				FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN,
				FILE_SHARE_READ,
				FILE_SUPERSEDE,
				FILE_SYNCHRONOUS_IO_NONALERT,
				NULL,
				0)))
		{
			DebugPrint(("CreateRandomizedImageFile(): ZwCreateFile(%wZ) failed, %.8x.",
					RandomizedFilePath,
					Status));
			break;
		}
			
		//
		// Write the relocateable contents from memory to the randomized image 
		// file
		//
		while (RawImageSize > 0)
		{
			//
			// Must wrap the write operation around SEH guard since we reference a
			// mapped pointer
			//
			__try
			{
				if (!NT_SUCCESS(Status = ZwWriteFile(
						RandomizedFileHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						(PUCHAR)RawImageBase + Offset,
						RawImageSize,
						NULL,
						NULL)))
				{
					DebugPrint(("CreateRandomizedImageFile(): ZwWriteFile failed, %.8x.",
							Status));
					break;
				}
			} __except(EXCEPTION_EXECUTE_HANDLER)
			{
				Status = GetExceptionCode();

				DebugPrint(("CreateRandomizedImageFile(): Caught exception during write, %.8x.",
						Status));
				break;
			}

			RawImageSize -= (ULONG)IoStatus.Information;
			Offset       += (ULONG)IoStatus.Information;
		}

	} while (0);

	if (RandomizedFileHandle)
		ZwClose(
				RandomizedFileHandle);

	return Status;
}
