/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "precomp.h"

//
// At the moment, multiple image set support isn't entirely complete.  If added,
// these routines will be necessary in order to create jump tables.  This hack
// is needed due to being an external implementation.
//
#ifdef MULTIPLE_IMAGE_SETS

#define KEY_BASIC_INFORMATION_SIZE         4096
#define KEY_VALUE_PARTIAL_INFORMATION_SIZE 8192
#define IMAGE_TABLE_KEY_PATH               WEHNTRUST_REGISTRY_CONFIG_PATH L"\\Images"
#define RELATIVE_JUMP_SIZE                 5

#define PAGE_ALIGN_16(Address)             ((ULONG)Address & 0xffff0000)
#define PAGE_SIZE_16                       0x10000

NTSTATUS InitializeImage(
		IN PIMAGE Image,
		IN HANDLE ImageTableKey);
NTSTATUS InitializeImageAddressTable(
		IN PIMAGE Image,
		IN HANDLE ImageKey);
NTSTATUS InitializeImageAddressTableEntry(
		IN PIMAGE Image,
		IN ULONG  Index,
		IN PUNICODE_STRING EntryName,
		IN HANDLE AddressTableKey);
NTSTATUS InitializeImageExemptions(
		IN PIMAGE Image,
		IN HANDLE ImageKey);

LIST_ENTRY ImageTable;
FAST_MUTEX ImageTableMutex;

//
// Creates an entry in the image table and returns a referenced image instance.
//
PIMAGE CreateImage(
		IN PUNICODE_STRING ImageName)
{
	PIMAGE Image = NULL;

	do
	{
		//
		// Allocate the image context
		// 
		if (!(Image = (PIMAGE)ExAllocatePoolWithTag(
				PagedPool, 
				sizeof(IMAGE), 
				ALLOC_TAG)))
			break;

		//
		// Zero the context
		//
		RtlZeroMemory(
				Image, 
				sizeof(IMAGE));

		//
		// Copy the image's name
		//
		RtleCopyUnicodeString(
				&Image->Name,
				ImageName);

		//
		// Give the image its first reference
		//
		ReferenceImage(Image);

	} while (0);

	return Image;
}

//
// Locks the image list
//
VOID LockImages()
{
	ExAcquireFastMutex(
			&ImageTableMutex);
}

//
// Unlocks the image list
//
VOID UnlockImages()
{
	ExReleaseFastMutex(
			&ImageTableMutex);
}

//
// Increments the reference count on an image context.
//
PIMAGE ReferenceImage(
		IN PIMAGE Image)
{
	//
	// Increment the reference count
	//
	InterlockedIncrement(&Image->References);

	return Image;
}

//
// Decrements the reference count on an image context, returning TRUE if the
// reference count has dropped to zero and the image was destroyed.
//
BOOLEAN DereferenceImage(
		IN PIMAGE Image)
{
	BOOLEAN Deleted = FALSE;

	//
	// Make sure the reference count isn't already zero
	//
	ASSERT(Image->References != 0);

	//
	// Decrement the reference count
	//
	if (InterlockedDecrement(&Image->References) == 0)
	{
		//
		// Free the image name
		//
		if (Image->Name.Buffer)
			RtleFreeUnicodeString(&Image->Name);
		if (Image->Path.Buffer)
			RtleFreeUnicodeString(&Image->Path);

		//
		// Destroy the address table
		//
		if (Image->AddressTable.Table)
		{
			ULONG Index = 0;

			for (Index = 0;
			     Index < Image->AddressTable.TableSize;
			     Index++)
			{
				if (Image->AddressTable.Table[Index].Prepend)
					ExFreePool(Image->AddressTable.Table[Index].Prepend);
			}

			ExFreePool(Image->AddressTable.Table);
		}

		//
		// Free the image context
		//
		ExFreePool(Image);

		Deleted = TRUE;
	}

	return Deleted;
}

//
// Insert an image into the image table list
//
VOID InsertImageEntry(
		IN PIMAGE Image)
{
	//
	// Acquire a reference to the image for the list entry
	//
	ReferenceImage(Image);

	LockImages();

	InsertTailList(
			&ImageTable,
			(PLIST_ENTRY)Image);

	UnlockImages();
}

//
// Finds an image of a given name in the image table
//
PIMAGE FindImageEntryByPath(
		IN PUNICODE_STRING Path)
{
	PLIST_ENTRY Current;
	PIMAGE      Image = NULL;

	LockImages();

	//
	// Search the list of images for one that has a path that is inside the one
	// being supplied
	//
	for (Current = ImageTable.Flink;
	     Current != &ImageTable;
	     Current = Current->Flink)
	{
		PIMAGE CurrentImage = (PIMAGE)Current;

		//
		// The path stored in the registry for the image is an NT path and the one
		// used by the memory manager is typically not, so it is always safer to
		// assume that the path being mapped will be inside the path that is
		// listed in the registry, but not necessarily the other way around.
		//
		if (RtleFindUnicodeStringInUnicodeString(
				&CurrentImage->Path,
				Path))
		{
			Image = CurrentImage;

			//
			// Acquire a reference to the image
			//
			ReferenceImage(Image);

			break;
		}
	}

	UnlockImages();

	return Image;
}

//
// Remove an image from the image table list
//
VOID RemoveImageEntry(
		IN PIMAGE Image)
{
	LockImages();

	//
	// Remove the entry from the list
	//
	RemoveEntryList((PLIST_ENTRY)Image);

	UnlockImages();

	//
	// Lose the list referene
	//
	DereferenceImage(Image);
}

//
// Initializes the image table context
//
NTSTATUS InitializeImageTable()
{
	//
	// Initialize the image table
	//
	InitializeListHead(&ImageTable);

	//
	// Initialize the image table mutex
	//
	ExInitializeFastMutex(&ImageTableMutex);

	//
	// Refresh the image table by synchronizing with the registry
	//
	return RefreshImageTable();
}

//
// Refreshes the image table and synchronizes with the registry
//
NTSTATUS RefreshImageTable()
{
	PKEY_BASIC_INFORMATION ImageKeyInformation = NULL;
	OBJECT_ATTRIBUTES      Attributes;
	UNICODE_STRING         ImageTableKeyPath;
	NTSTATUS               Status = STATUS_UNSUCCESSFUL;
	HANDLE                 ImageTableKey = NULL;
	ULONG                  ImageKeyInformationSize = 0;
	ULONG                  Index = 0;

	//
	// Flush all existing entries from the image table
	//
	FlushImageTable();

	do
	{
		//
		// Initialize the registry key path
		//
		RtlInitUnicodeString(
				&ImageTableKeyPath,
				IMAGE_TABLE_KEY_PATH);

		InitializeObjectAttributes(
				&Attributes,
				&ImageTableKeyPath,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);

		//
		// Allocate storage for the KEY_BASIC_INFORMATION structure
		//
		if (!(ImageKeyInformation = (PKEY_BASIC_INFORMATION)ExAllocatePoolWithTag(
				NonPagedPool, 
				KEY_BASIC_INFORMATION_SIZE, 
				ALLOC_TAG)))
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		//
		// Open the image table key
		//
		if (!NT_SUCCESS(Status = ZwOpenKey(
				&ImageTableKey, 
				STANDARD_RIGHTS_READ,
				&Attributes)))
		{
			DebugPrint(("RefreshImageTable(): ZwOpenKey(%wZ) failed, %.8x.",
					&ImageTableKeyPath,
					Status));
			break;
		}
		
		//
		// Begin the enumeration of the images
		//
		while (NT_SUCCESS(ZwEnumerateKey(
				ImageTableKey,
				Index++,
				KeyBasicInformation,
				ImageKeyInformation,
				KEY_BASIC_INFORMATION_SIZE,
				&ImageKeyInformationSize)))
		{
			UNICODE_STRING ImageName;
			PIMAGE         Image;

			ImageName.Buffer        = ImageKeyInformation->Name;
			ImageName.Length        = (USHORT)ImageKeyInformation->NameLength;
			ImageName.MaximumLength = (USHORT)ImageKeyInformation->NameLength;

			if (ImageName.Buffer[(ImageName.Length / sizeof(WCHAR)) - 1] == 0)
			{
				ImageName.Length        -= sizeof(WCHAR);
				ImageName.MaximumLength -= sizeof(WCHAR);
			}

			//
			// Create an image instance
			//
			if (!(Image = CreateImage(
					&ImageName)))
			{
				DebugPrint(("RefreshImageTable(): Failed to create image: %wZ",
						ImageName.Buffer));
				continue;
			}

			//
			// Initialize the image instance from the registry
			//
			if (!NT_SUCCESS(Status = InitializeImage(
					Image,
					ImageTableKey)))
			{
				DebugPrint(("RefreshImageTable(): InitializeImage(%wZ) failed, %.8x.",
						&ImageName, Status));

				//
				// Destroy the image
				//
				DereferenceImage(Image);

				break;
			}

			//
			// Insert the image into the image table list
			//
			InsertImageEntry(Image);

			DebugPrint(("RefreshImageTable(): Successfully initialized image %wZ.",
					&Image->Name));
			
			//
			// Lose our reference to the image as we no longer need it
			//
			DereferenceImage(Image);
			
		}

		//
		// If a failure status was encountered while initializing the images,
		// abort
		//
		if (!NT_SUCCESS(Status))
		{
			DebugPrint(("RefreshImageTable(): An image failed to initialize, aborting."));
			break;
		}

		DebugPrint(("RefreshImageTable(): Initialized %lu images.",
				(Index) ? Index - 1 : 0));

		Status = STATUS_SUCCESS;

	} while (0);

	//
	// Clean up
	//
	if (ImageTableKey)
		ZwClose(ImageTableKey);
	if (ImageKeyInformation)
		ExFreePool(ImageKeyInformation);

	return Status;
}

//
// Flush all entries from the image table
//
VOID FlushImageTable()
{
	PIMAGE CurrentImage;

	LockImages();

	// 
	// Flush all of the entries from the list
	//
	while ((CurrentImage = (PIMAGE)RemoveHeadList(&ImageTable)) != (PIMAGE)&ImageTable)
		DereferenceImage(CurrentImage);

	UnlockImages();
}

//
// Initialize a given image from the registry
//
NTSTATUS InitializeImage(
		IN PIMAGE Image,
		IN HANDLE ImageTableKey)
{
	PKEY_VALUE_PARTIAL_INFORMATION ValueInformation = NULL;
	OBJECT_ATTRIBUTES              Attributes;
	UNICODE_STRING                 ValueName;
	NTSTATUS                       Status;
	HANDLE                         ImageKey = NULL;
	ULONG                          ValueInformationSize;

	do
	{
		InitializeObjectAttributes(
				&Attributes,
				&Image->Name,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				ImageTableKey,
				NULL);

		//
		// Open the image's key
		//
		if (!NT_SUCCESS(Status = ZwOpenKey(
				&ImageKey,
				STANDARD_RIGHTS_READ,
				&Attributes)))
		{
			DebugPrint(("InitializeImage(): ZwOpenKey failed, %.8x.", Status));
			break;
		}

		//
		// Allocate the value buffer for use with subsequent value obtainment
		//
		if (!(ValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(
				PagedPool,
				KEY_VALUE_PARTIAL_INFORMATION_SIZE,
				ALLOC_TAG)))
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		//
		// Get the image's ``OriginalBaseAddress''
		//
		RtlInitUnicodeString(
				&ValueName,
				L"OriginalBaseAddress");

		if (NT_SUCCESS(ZwQueryValueKey(
				ImageKey,
				&ValueName,
				KeyValuePartialInformation,
				ValueInformation,
				KEY_VALUE_PARTIAL_INFORMATION_SIZE,
				&ValueInformationSize)))
		{
			if (ValueInformation->Type == REG_DWORD)
				Image->OriginalBaseAddress = *(PULONG)ValueInformation->Data;
		}

		//
		// Get the image's ``Path'' that is used for comparison when an image is
		// being mapped into memory by a process.  If the file being mapped
		// matches the path supplied in the registry, a the jump table described
		// in the registry for the given image will be mapped in as well.  This
		// path should be in the form of an NT path, such as:
		//
		// \??\C:\Windows\System32\NTDLL.DLL
		//
		// It must be in this format in order to be able to do checksum validation
		//
		RtlInitUnicodeString(
				&ValueName,
				L"Path");

		if (NT_SUCCESS(ZwQueryValueKey(
				ImageKey,
				&ValueName,
				KeyValuePartialInformation,
				ValueInformation,
				KEY_VALUE_PARTIAL_INFORMATION_SIZE,
				&ValueInformationSize)))
		{
			UNICODE_STRING Path;

			Path.Buffer        = (PWSTR)ValueInformation->Data;
			Path.Length        = (USHORT)ValueInformation->DataLength;
			Path.MaximumLength = (USHORT)ValueInformation->DataLength;

			RtleCopyUnicodeString(
					&Image->Path,
					&Path);
		}

		//
		// Get the image's ``Flags'' that are used to tell the driver how
		// instances of the image that are mapped into memory are to be handled
		// (such as how the jump table is to be constructed)
		//
		RtlInitUnicodeString(
				&ValueName,
				L"Flags");

		if (NT_SUCCESS(ZwQueryValueKey(
				ImageKey,
				&ValueName,
				KeyValuePartialInformation,
				ValueInformation,
				KEY_VALUE_PARTIAL_INFORMATION_SIZE,
				&ValueInformationSize)))
		{
			if (ValueInformation->Type == REG_DWORD)
				Image->Flags = *(PULONG)ValueInformation->Data;
		}
		
		//
		// Get the image's ``Checksum'' that is used to ensure that the jump table
		// supplied in the registry is accurate against the latest copy of the
		// file when a system boot or registry synchronization occurs.
		//
		RtlInitUnicodeString(
				&ValueName,
				L"Checksum");

		if ((NT_SUCCESS(ZwQueryValueKey(
				ImageKey,
				&ValueName,
				KeyValuePartialInformation,
				ValueInformation,
				KEY_VALUE_PARTIAL_INFORMATION_SIZE,
				&ValueInformationSize))) &&
		    (ValueInformation->DataLength >= 1))
		{
			Image->Checksum.Type = (IMAGE_CHECKSUM_TYPE)ValueInformation->Data[0];

			switch (Image->Checksum.Type)
			{
				case IMAGE_CHECKSUM_TYPE_NONE:
					break;
				case IMAGE_CHECKSUM_TYPE_ROR16:
					if (ValueInformation->DataLength >= sizeof(ULONG) + 1)
						Image->Checksum.Expected.Ror16 = *(PULONG)(ValueInformation->Data + 1);
					else
					{
						Image->Checksum.Expected.Ror16 = 0;

						DebugPrint(("InitializeImage(): Invalid ROR16 hash."));

						Image->Checksum.Type = IMAGE_CHECKSUM_TYPE_NONE;
					}
					break;
				case IMAGE_CHECKSUM_TYPE_SHA1:
					if (ValueInformation->DataLength >= SHA1_HASH_SIZE + 1)
						RtlCopyMemory(
								Image->Checksum.Expected.Sha1,
								ValueInformation->Data + 1,
								SHA1_HASH_SIZE);
					else
					{
						RtlZeroMemory(
								Image->Checksum.Expected.Sha1,
								SHA1_HASH_SIZE);

						DebugPrint(("InitializeImage(): Invalid SHA1 hash."));

						Image->Checksum.Type = IMAGE_CHECKSUM_TYPE_NONE;
					}
					break;
				default:
					DebugPrint(("InitializeImage(): Unsupported checksum algorithm: %.2x",
							Image->Checksum.Type));

					Image->Checksum.Type = IMAGE_CHECKSUM_TYPE_NONE;
					break;
			}
		}

		//
		// Initialize the image's address table
		//
		if (!NT_SUCCESS(Status = InitializeImageAddressTable(
				Image,
				ImageKey)))
			break;

		//
		// Initialize the image's exemptions table
		//
		if (!NT_SUCCESS(Status = InitializeImageExemptions(
				Image,
				ImageKey)))
			break;

		//
		// Validate the image's checksum
		//
		if (!NT_SUCCESS(Status = ValidateImageChecksum(
				Image)))
		{
			DebugPrint(("InitializeImage(): ValidateImageChecksum failed, %.8x.",
					Status));
			break;
		}

		//
		// We win
		//
		Status = STATUS_SUCCESS;

	} while (0);

	//
	// Clean up
	//
	if (ImageKey)
		ZwClose(ImageKey);
	if (ValueInformation)
		ExFreePool(ValueInformation);

	return Status;
}

//
// Initializes an image's address jump table for use with relocations
//
NTSTATUS InitializeImageAddressTable(
		IN PIMAGE Image,
		IN HANDLE ImageKey)
{
	PKEY_BASIC_INFORMATION AddressTableEntryInformation = NULL;
	PKEY_FULL_INFORMATION  AddressTableInformation = NULL;
	OBJECT_ATTRIBUTES      Attributes;
	UNICODE_STRING         AddressTableKeyName;
	NTSTATUS               Status = STATUS_UNSUCCESSFUL;
	HANDLE                 AddressTableKey = NULL;
	PUCHAR                 KeyBuffer = NULL;
	ULONG                  AddressTableEntryInformationSize = 0;
	ULONG                  AddressTableInformationSize = 0;
	ULONG                  Index = 0;

	do
	{
		RtlInitUnicodeString(
				&AddressTableKeyName,
				L"AddressTable");

		InitializeObjectAttributes(
				&Attributes,
				&AddressTableKeyName,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				ImageKey,
				NULL);

		//
		// Open the AddressTable key
		//
		if (!NT_SUCCESS(Status = ZwOpenKey(
				&AddressTableKey,
				STANDARD_RIGHTS_READ,
				&Attributes)))
		{
			DebugPrint(("InitializeImageAddressTable(): Failed to open AddressTable for image %wZ.",
					&Image->Name));
			break;
		}

		//
		// Allocate storage for the enumeration
		//
		if (!(KeyBuffer = (PUCHAR)ExAllocatePoolWithTag(
				PagedPool,
				KEY_BASIC_INFORMATION_SIZE,
				ALLOC_TAG)))
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		//
		// Query the address table to figure out how many entries there will be in
		// the array
		//
		AddressTableInformation = (PKEY_FULL_INFORMATION)KeyBuffer;

		if (!NT_SUCCESS(Status = ZwQueryKey(
				AddressTableKey,
				KeyFullInformation,
				AddressTableInformation,
				KEY_BASIC_INFORMATION_SIZE,
				&AddressTableInformationSize)))
			break;

		//
		// If there are no address table entries for this image, break out
		//
		if (AddressTableInformation->SubKeys == 0)
		{
			DebugPrint(("InitializeImageAddressTable(): Image %wZ has no address table entries.",
					&Image->Name));

			Status = STATUS_SUCCESS;

			break;
		}

		//
		// Allocate the address table array for this image
		//
		if (!(Image->AddressTable.Table = (PIMAGE_ADDRESS)ExAllocatePoolWithTag(
				PagedPool,
				sizeof(IMAGE_ADDRESS) * AddressTableInformation->SubKeys,
				ALLOC_TAG)))
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		//
		// Zero out the address table
		//
		RtlZeroMemory(
				Image->AddressTable.Table,
				AddressTableInformation->SubKeys * sizeof(IMAGE_ADDRESS));

		Image->AddressTable.TableSize = AddressTableInformation->SubKeys;

		//
		// Begin the enumeration of the address table entries
		//
		AddressTableEntryInformation = (PKEY_BASIC_INFORMATION)KeyBuffer;

		while (NT_SUCCESS(ZwEnumerateKey(
				AddressTableKey,
				Index,
				KeyBasicInformation,
				AddressTableEntryInformation,
				KEY_BASIC_INFORMATION_SIZE,
				&AddressTableEntryInformationSize)))
		{
			UNICODE_STRING AddressUnicodeString;
			UNICODE_STRING ValueName;

			AddressUnicodeString.Buffer        = (PWSTR)AddressTableEntryInformation->Name;
			AddressUnicodeString.Length        = (USHORT)AddressTableEntryInformation->NameLength;
			AddressUnicodeString.MaximumLength = (USHORT)AddressTableEntryInformation->NameLength;

			//
			// Have we gone past the end of the address table?
			//
			if (Index >= Image->AddressTable.TableSize)
				break;

			//
			// Convert the address string into an unsigned long
			//
			Image->AddressTable.Table[Index].Address = RtleUnicodeStringToInteger(
					&AddressUnicodeString,
					16);

			//
			// Initialize the image's address table entry
			//
			InitializeImageAddressTableEntry(
					Image,
					Index,
					&AddressUnicodeString,
					AddressTableKey);

			// 
			// We have to allocate on 16-page aligned boundaries due to the way VAD
			// reservation works.  It will only allow allocations at 64k
			// granularity, which sucks, but we don't have any other option.
			//
			
			//
			// Determine the region info that will need to be allocated for this
			// jump table entry
			//
			Image->AddressTable.Table[Index].RegionBase = (ULONG)PAGE_ALIGN_16((PVOID)Image->AddressTable.Table[Index].Address);
			Image->AddressTable.Table[Index].RegionSize = PAGE_SIZE_16;

			//
			// Add this region to the list of exempted regions such that it will
			// not be allocated into
			//
			if (!AddRegionExemption(
					(PVOID)Image->AddressTable.Table[Index].RegionBase,
					Image->AddressTable.Table[Index].RegionSize))
			{
				DebugPrint(("InitializeImageAddressTable(): Warning: failed to add region exemption."));
			}

			//
			// If the Address plus the relative jump size plus the prepend size in
			// bytes goes past the end of a single page boundary, allocate another
			// page.
			//
			if ((Image->AddressTable.Table[Index].Address + RELATIVE_JUMP_SIZE + 
			     Image->AddressTable.Table[Index].PrependSize) > 
			    (Image->AddressTable.Table[Index].RegionBase + PAGE_SIZE_16))
				Image->AddressTable.Table[Index].RegionSize += PAGE_SIZE_16;

			//
			// Increment the index
			//
			Index++;
		}
		
		DebugPrint(("InitializeImageAddressTable(): Initialized %lu address table entries for %wZ.",
				Index, &Image->Name));

		Status = STATUS_SUCCESS;

	} while (0);

	//
	// Clean up
	//
	if (KeyBuffer)
		ExFreePool(KeyBuffer);
	if (AddressTableKey)
		ZwClose(AddressTableKey);

	return Status;
}

//
// Initialize a specific address table entry that may or may not have extended
// parameters needed for building the address jump table
//
NTSTATUS InitializeImageAddressTableEntry(
		IN PIMAGE Image,
		IN ULONG Index,
		IN PUNICODE_STRING EntryName,
		IN HANDLE AddressTableKey)
{
	PKEY_VALUE_PARTIAL_INFORMATION ValueInformation = NULL;
	OBJECT_ATTRIBUTES              Attributes;
	UNICODE_STRING                 ValueName;
	NTSTATUS                       Status;
	HANDLE                         AddressTableEntryKey = NULL;
	ULONG                          ValueInformationSize;

	do
	{
		InitializeObjectAttributes(
				&Attributes,
				EntryName,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				AddressTableKey,
				NULL);

		//
		// Open the address table entry key
		//
		if (!NT_SUCCESS(Status = ZwOpenKey(
				&AddressTableEntryKey,
				STANDARD_RIGHTS_READ,
				&Attributes)))
			break;

		//
		// Allocate the value buffer for use with subsequent value obtainment
		//
		if (!(ValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(
				PagedPool,
				KEY_VALUE_PARTIAL_INFORMATION_SIZE,
				ALLOC_TAG)))
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		//
		// Find out if the entry has any prepend bytes
		//
		RtlInitUnicodeString(
				&ValueName,
				L"Prepend");

		if ((NT_SUCCESS(ZwQueryValueKey(
				AddressTableEntryKey,
				&ValueName,
				KeyValuePartialInformation,
				ValueInformation,
				KEY_VALUE_PARTIAL_INFORMATION_SIZE,
				&ValueInformationSize))) &&
		    (ValueInformation->Type == REG_BINARY))
		{
			Image->AddressTable.Table[Index].Prepend = ExAllocatePoolWithTag(
					PagedPool,
					ValueInformation->DataLength,
					ALLOC_TAG);

			if (Image->AddressTable.Table[Index].Prepend)
			{
				RtlCopyMemory(
						Image->AddressTable.Table[Index].Prepend,
						ValueInformation->Data,
						ValueInformation->DataLength);
		
				Image->AddressTable.Table[Index].PrependSize = ValueInformation->DataLength;
			}
		}

		Status = STATUS_SUCCESS;

	} while (0);

	//
	// Clean up
	//
	if (ValueInformation)
		ExFreePool(ValueInformation);
	if (AddressTableEntryKey)
		ZwClose(AddressTableEntryKey);

	return Status;
}

//
// Initialize the image's process exemption list
//
// XXX: Unimplemented
//
NTSTATUS InitializeImageExemptions(
		IN PIMAGE Image,
		IN HANDLE ImageKey)
{
	DebugPrint(("InitializeImageExemptions unimplemented!"));

	return STATUS_SUCCESS;
}

//
// Validates the checksum of an individual image.  If the checksum supplied in
// the registry does not match the one that is on disk, the randomization driver
// cannot safely assume that it can continue and thus must queue the user-mode
// determiner application to be executed at the next bootup such that it can
// determine the actual addresses and update the checksum for the image.
//
NTSTATUS ValidateImageChecksum(
		IN PIMAGE Image)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK   IoStatus;
	NTSTATUS          Status = STATUS_SUCCESS;
	SHA1_CTX          Sha1Context;
	PUCHAR            FileBuffer = NULL;
	HANDLE            FileHandle = NULL;

	do
	{
		//
		// If the image has no checksum, no need to validate it.  This should,
		// however, not be the common case.
		//
		if (Image->Checksum.Type == IMAGE_CHECKSUM_TYPE_NONE)
		{
			Status = STATUS_SUCCESS;
			break;
		}
		//
		// Otherwise, if SHA1 is being used, initialize the context.
		//
		else if (Image->Checksum.Type == IMAGE_CHECKSUM_TYPE_SHA1)
		{
			SHA1_Init(
					&Sha1Context);
		}

		//
		// Note: the image path cannot contain a null terminator, else it will
		// piss off ZwOpenFile.
		//
		InitializeObjectAttributes(
				&ObjectAttributes,
				&Image->Path,
				OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
				NULL,
				NULL);

		//
		// Open the file for reading
		//
		if (!NT_SUCCESS(Status = ZwOpenFile(
				&FileHandle,
				FILE_READ_DATA | STANDARD_RIGHTS_READ,
				&ObjectAttributes,
				&IoStatus,
				FILE_SHARE_READ,
				FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_ALERT)))
		{
			DebugPrint(("ValidateImageChecksum(%wZ,%d): ZwOpenFile() failed, %.8x.",
					&Image->Path,
					Image->Path.Length / sizeof(WCHAR),
					Status));
			break;
		}


		//
		// Allocate the buffer that will be used for holding file data 
		//
		if (!(FileBuffer = (PUCHAR)ExAllocatePoolWithTag(
				PagedPool,
				CHECKSUM_BUFFER_SIZE,
				ALLOC_TAG)))
		{
			DebugPrint(("ValidateImageChecksum(): ExAllocatePoolWithTag failed."));

			Status = STATUS_NO_MEMORY;
			break;
		}

		//
		// Keep reading from the file until the end of the file is reached or an
		// error is encountered
		//
		while (NT_SUCCESS(ZwReadFile(
				FileHandle,
				NULL,
				NULL,
				NULL,
				&IoStatus,
				FileBuffer,
				CHECKSUM_BUFFER_SIZE,
				NULL,
				NULL)))
		{
			switch (Image->Checksum.Type)
			{
				case IMAGE_CHECKSUM_TYPE_SHA1:
					SHA1_Update(
							&Sha1Context, 
							FileBuffer, 
							IoStatus.Information);
					break;
				default:
					break;
			}
		}

		//
		// Finally, compare the expected checksum with the actual checksum for the
		// given algorithm
		//
		switch (Image->Checksum.Type)
		{
			case IMAGE_CHECKSUM_TYPE_SHA1:
				//
				// Finalize and obtain the SHA1 digest
				//
				SHA1_Final(
						&Sha1Context, 
						Image->Checksum.Actual.Sha1);

				//
				// Compare the expected digest with the actual digest
				//
				if (RtlCompareMemory(
						Image->Checksum.Expected.Sha1,
						Image->Checksum.Actual.Sha1,
						SHA1_HASH_SIZE) != SHA1_HASH_SIZE)
				{
					DebugPrint((
							"ValidateImageChecksum(%wZ): SHA1 checksums mismatch: %.8x:%.8x:%.8x:%.8x:%.8x != %.8x:%.8x:%.8x:%.8x:%.8x.",
							&Image->Path,
							*((PULONG)Image->Checksum.Expected.Sha1+0),
							*((PULONG)Image->Checksum.Expected.Sha1+1),
							*((PULONG)Image->Checksum.Expected.Sha1+2),
							*((PULONG)Image->Checksum.Expected.Sha1+3),
							*((PULONG)Image->Checksum.Expected.Sha1+4),
							*((PULONG)Image->Checksum.Actual.Sha1+0),
							*((PULONG)Image->Checksum.Actual.Sha1+1),
							*((PULONG)Image->Checksum.Actual.Sha1+2),
							*((PULONG)Image->Checksum.Actual.Sha1+3),
							*((PULONG)Image->Checksum.Actual.Sha1+4)));

					Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
				}
				else
					Status = STATUS_SUCCESS;
				break;
			default:
				Status = STATUS_SUCCESS;
				break;
		}

	} while (0);

	//
	// Cleanup
	//
	if (FileBuffer)
		ExFreePool(
				FileBuffer);
	if (FileHandle)
		ZwClose(
				FileHandle);

	return Status;
}

//
// Builds the jump table for a given image relative to the old and new base
// address.
//
NTSTATUS BuildJumpTableForImage(
		IN PIMAGE Image,
		IN PPROCESS_OBJECT ProcessObject OPTIONAL,
		IN PVOID NewImageBase)
{
	KAPC_STATE ApcState;
	NTSTATUS   Status = STATUS_SUCCESS;
	BOOLEAN    Attached = FALSE;
	HANDLE     ProcessHandle = (HANDLE)-1;
	ULONG      Index;

	DebugPrint(("BuildJumpTableForImage(%p): Process %p, new base %p.",
			Image,
			ProcessObject,
			NewImageBase));

	//
	// If a process was supplied, attach to it.
	//
	if (ProcessObject)
	{
		//
		// Try to get a handle to the process first
		//
		if (!NT_SUCCESS(Status = ObOpenObjectByPointer(
				ProcessObject,
				OBJ_KERNEL_HANDLE,
				0,
				0,
				PsProcessType,
				KernelMode,
				&ProcessHandle)))
		{
			DebugPrint(("BuildJumpTableForImage(%p): Failed to get process handle for %p.",
					Image,
					ProcessObject));
			
			ASSERT(0);
		}
		//
		// Otherwise, attach to it.
		//
		else
		{
			ProcessHandle = (HANDLE)-1;
			
			KeStackAttachProcess(
					ProcessObject,
					&ApcState);

			Attached = TRUE;
		}
	}

	//
	// Enumerate the address jump table, allocating and constructing the virtual
	// jumps to the new destination
	//
	for (Index = 0;
		  Index < Image->AddressTable.TableSize;
		  Index++)
	{
		BOOLEAN SkipAllocation;
		PVOID   Address = (PVOID)Image->AddressTable.Table[Index].Address;
		ULONG   Displacement = 0;
		ULONG   Offset = 0;
		ULONG   RegionBase = Image->AddressTable.Table[Index].RegionBase;
		ULONG   RegionSize = Image->AddressTable.Table[Index].RegionSize;
		ULONG   BeforeIndex = 0;
	
		//
		// Check to see if this range has already been allocated.  If so, skip the
		// allocation and write the memory out.
		//
		for (BeforeIndex = 0, SkipAllocation = FALSE;
		     BeforeIndex < Index;
		     BeforeIndex++)
		{
			if ((RegionBase >= Image->AddressTable.Table[BeforeIndex].RegionBase) &&
			    (RegionBase + RegionSize <= Image->AddressTable.Table[BeforeIndex].RegionBase + Image->AddressTable.Table[BeforeIndex].RegionSize))
				SkipAllocation = TRUE;
		}

		//
		// If the allocation should not be skipped, do it now.
		//
		if (!SkipAllocation)
		{
			DebugPrint(("BuildJumpTableForImage(): Allocating region base %.8x size %.8x.",
					RegionBase,
					RegionSize));

			//
			// Allocate the region
			//
			if (!NT_SUCCESS(Status = ZwAllocateVirtualMemory(
					ProcessHandle,
					(PVOID *)&RegionBase,
					0,
					&RegionSize,
					MEM_COMMIT | MEM_RESERVE,
					PAGE_EXECUTE_READWRITE)))
			{
				DebugPrint(("BuildJumpTableForImage(): ZwAllocateVirtualMemory failed, %.8x.", 
						Status));

				ASSERT(0);
			
				break;
			}
		}

		//
		// If the this jump table entry has bytes that need to be prepended...
		//
		if (Image->AddressTable.Table[Index].PrependSize)
		{
			ULONG FixupIndex = 0;

			memcpy(
					Address,
					Image->AddressTable.Table[Index].Prepend,
					Image->AddressTable.Table[Index].PrependSize);

			//
			// TODO: Make this more robust - like using offset hints or something
			//
			// If the prepend data has a reference to the old base address, fix it
			// up to point to the new one
			//
			for (FixupIndex = 0;
			     FixupIndex <= Image->AddressTable.Table[Index].PrependSize - sizeof(ULONG);
			     FixupIndex++)
			{
				PULONG FixupBuffer = (PULONG)((PUCHAR)Address + FixupIndex);

				if ((*FixupBuffer) == Image->OriginalBaseAddress)
				{
					DebugPrint(("BuildJumpTableForImage(): Fixed up prepend data to point to new base."));

					(*FixupBuffer) = (ULONG)NewImageBase;
				}
			}

			//
			// Update the offset for the subsequent jump
			//
			Offset += Image->AddressTable.Table[Index].PrependSize;
		}

		// 
		// Calculate the relative displacement between the new range and the
		// old range
		//
		Displacement = (ULONG)(((ULONG)NewImageBase - Image->OriginalBaseAddress) - 5);

		//
		// Build the jump instruction
		//
		*((PUCHAR)Address + Offset)             = 0xe9;
		*(PULONG)((PUCHAR)Address + Offset + 1) = Displacement;
		
		DebugPrint(("BuildJumpTableForImage(): Built jump from %.8x -> %.8x for %wZ.",
				Address,
				(ULONG)NewImageBase + ((ULONG)Address - Image->OriginalBaseAddress),
				&Image->Name));
	}

	//
	// Detach from the process if provided
	//
	if (ProcessObject)
	{
		//
		// If we got a process handle from ObOpenObjectByPointer...
		//
		if (ProcessHandle != (HANDLE)-1)
			ZwClose(
					ProcessHandle);

		//
		// If we attached, detach
		//
		if (Attached)
			KeUnstackDetachProcess(
					&ApcState);
	}

	return Status;
}
#endif

//
// Build a single jump table mapping for a given process
//
NTSTATUS BuildJumpTableSingleMapping(
		IN PPROCESS_OBJECT ProcessObject OPTIONAL,
		IN PVOID OldAddress,
		IN PVOID NewAddress)
{
	KAPC_STATE ApcState;
	NTSTATUS   Status = STATUS_SUCCESS;
	BOOLEAN    Attached = FALSE;
	PVOID      OldBase = NULL;
	ULONG      RegionSize = 0x10000;
	ULONG      Displacement = 0;

	//
	// If a process was supplied, attach to it.
	//
	if ((ProcessObject) &&
	    (ProcessObject != IoGetCurrentProcess()))
	{
		KeStackAttachProcess(
				ProcessObject,
				&ApcState);

		Attached = TRUE;
	}
	
	do
	{
		OldBase = (PVOID)((ULONG_PTR)OldAddress & 0xffff0000);

		//
		// Allocate the region
		//
		if (!NT_SUCCESS(Status = ZwAllocateVirtualMemory(
				(HANDLE)-1,
				(PVOID *)&OldBase,
				0,
				&RegionSize,
				MEM_COMMIT | MEM_RESERVE,
				PAGE_EXECUTE_READWRITE)))
		{
			break;
		}

		Displacement = (ULONG)(((ULONG_PTR)NewAddress - (ULONG_PTR)OldAddress) - 5);

		//
		// Build the jump instruction
		//
		*((PUCHAR)OldAddress)             = 0xe9;
		*(PULONG)((PUCHAR)OldAddress + 1) = Displacement;

		DebugPrint(("BuildJumpTableSingleMapping(): Built single jump table: %p->%p",
				OldAddress,
				NewAddress));

	} while (0);


	//
	// Detach from the process if provided
	//
	if (Attached)
	{
		KeUnstackDetachProcess(
				&ApcState);
	}

	return Status;
}
