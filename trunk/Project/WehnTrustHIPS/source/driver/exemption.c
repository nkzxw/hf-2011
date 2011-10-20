/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#include "precomp.h"

static VOID CreateExemptionRegistryKeys();

//
// Registry manipulation
//
static NTSTATUS InitializeExemptionsFromKey(
		IN EXEMPTION_SCOPE Scope,
		IN PWSTR ExemptionKey);
static NTSTATUS AddExemptionFromRegistry(
		IN EXEMPTION_SCOPE Scope,
		IN HANDLE ParentKey,
		IN PUNICODE_STRING SubkeyName);
static NTSTATUS AddUpdateRemoveExemptionToRegistry(
		IN EXEMPTION_SCOPE Scope,
		IN EXEMPTION_TYPE Type,
		IN ULONG Flags,
		IN PUNICODE_STRING ExemptionPath,
		IN BOOLEAN Remove);

static VOID CalculateExemptionPathChecksum(
		IN PUNICODE_STRING ExemptionPath,
		OUT WCHAR Checksum[EXEMPTION_PATH_CHECKSUM_SIZE]);

static PEXEMPTION GetExemptionFromFilePath(
		IN PUNICODE_STRING SymbolicPath OPTIONAL,
		IN PUNICODE_STRING FullFilePath OPTIONAL,
		IN EXEMPTION_TYPE Type);

//
// Globals
//
static LIST_ENTRY ExemptionsList;
static PERESOURCE ExemptionsListResource = NULL;

//
// Global array of keys that must be created for exemptions to function properly
//
static const PWSTR ExemptionBaseKeys[] =
{
	WEHNTRUST_EXEMPTION_KEY,
	WEHNTRUST_GLOBAL_SCOPE_EXEMPTION_KEY,
	NULL
};

#pragma code_seg("INIT")

//
// Reads exemptions from the registry and initializes them
//
NTSTATUS InitializeExemptions()
{
	NTSTATUS Status;

	do
	{
		//
		// Initialize the exemptions list
		//
		InitializeListHead(
				&ExemptionsList);

		//
		// Allocate the exemptions list resource
		//
		if (!(ExemptionsListResource = (PERESOURCE)ExAllocatePoolWithTag(
				NonPagedPool,
				sizeof(ERESOURCE),
				ALLOC_TAG)))
		{
			DebugPrint(("InitializeExemptions(): ExAllocatePoolWithTag failed."));

			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		if (!NT_SUCCESS(Status = ExInitializeResourceLite(
				ExemptionsListResource)))
		{
			DebugPrint(("InitializeExemptions(): ExInitializeResourceLite failed, %.8x.",
					Status));
			break;
		}

		//
		// Create the exemption registry keys
		//
		CreateExemptionRegistryKeys();

		//
		// Initialize global exemptions
		//
		if (!NT_SUCCESS(Status = InitializeExemptionsFromKey(
				GlobalScope,
				WEHNTRUST_GLOBAL_SCOPE_EXEMPTION_KEY)))
		{
			DebugPrint(("InitializeExemptions(): InitializeExemptionsFromKey(global) failed, %.8x.",
					Status));
		}
		
	} while (0);

	return STATUS_SUCCESS;
}

#pragma code_seg()

//
// Checks to see if the supplied file object is exempted
//
BOOLEAN IsImageFileExempted(
		IN PPROCESS_OBJECT ProcessObject,
		IN PFILE_OBJECT FileObject)
{
	POBJECT_NAME_INFORMATION NameInformation = NULL;
	PEXEMPTION               Exemption;
	NTSTATUS                 Status;
	BOOLEAN                  IsExempted = FALSE;
	KIRQL                    OldIrql;

	do
	{
		//
		// Get the file path associated with the file object
		//
		if (!NT_SUCCESS(Status = RtleGetFilePath(
				FileObject,
				&NameInformation)))
		{
			DebugPrint(("IsImageFileExempted(): RtleGetFilePath failed, %.8x.",
					Status));
			break;
		}

		//
		// Acquire the exemptions list for shared access
		//
		KeRaiseIrql(APC_LEVEL, &OldIrql);
		ExAcquireResourceSharedLite(
				ExemptionsListResource,
				TRUE);

		//
		// Check to see if there's even an exemption for this file 
		//
		if ((Exemption = GetExemptionFromFilePath(
				NULL,
				&NameInformation->Name,
				AnyExemption)))
		{
			DebugPrint(("IsImageFileExempted(): Image file %wZ is exempted for %d",
					&NameInformation->Name,
					Exemption->Type));

			//
			// If this image file represents an application exemption, flag this
			// process state context as being exempted.  We also do this if a
			// directory exemption is in place and this file is an executable.
			//
			// FIXME:
			//
			// This check should be improved at the future, but at this point there
			// is no context in which to determine whether or not the supplied file
			// path is actually an executable.
			//
			if ((Exemption->Type & ApplicationExemption) ||
			    ((Exemption->Type & DirectoryExemption)) &&
			     (RtleFindStringInUnicodeString(
					&NameInformation->Name,
					L".EXE")))
			{
				FlagProcessAsExempted(
						ProcessObject,
						Exemption->Flags);
			}

			IsExempted = TRUE;
		}

		//
		// Release the exemptions list resource
		//
		ExReleaseResourceLite(
				ExemptionsListResource);
		KeLowerIrql(OldIrql);

	} while (0);

	//
	// Cleanup
	//
	if (NameInformation)
		RtleFreeFilePath(
				NameInformation);

	return IsExempted;
}

//
// Adds an exemption of a given type
//
NTSTATUS AddExemption(
		IN EXEMPTION_TYPE Type,
		IN EXEMPTION_SCOPE Scope,
		IN ULONG Flags,
		IN PUNICODE_STRING SymbolicPath,
		IN BOOLEAN AddToRegistry)
{
	PEXEMPTION Exemption;
	NTSTATUS   Status = STATUS_SUCCESS;
	BOOLEAN    WasNew = FALSE;
	KIRQL      OldIrql;

	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceExclusiveLite(
			ExemptionsListResource,
			TRUE);

	//
	// If there is an existing exemption for this file path, add the supplied
	// type to the list of exemptions for it
	//
	if ((Exemption = GetExemptionFromFilePath(
			SymbolicPath,
			NULL,
			AnyExemption)))
	{
		Exemption->Type  |= Type;
		Exemption->Flags  = Flags;

		Status = STATUS_SUCCESS;
	}
	//
	// Otherwise, add the exemption
	//
	else
	{
		do
		{
			//
			// Allocate storage for the exemption
			//
			if (!(Exemption = (PEXEMPTION)ExAllocatePoolWithTag(
					NonPagedPool,
					sizeof(EXEMPTION),
					ALLOC_TAG)))
			{
				DebugPrint(("AddExemption(): ExAllocatePoolWithTag failed."));

				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			RtlZeroMemory(
					Exemption,
					sizeof(EXEMPTION));

			//
			// Initialize the exemption
			//
			Exemption->Scope = Scope;
			Exemption->Type  = Type;
			Exemption->Flags = Flags;

			if (!NT_SUCCESS(Status = RtleCopyUnicodeString(
					&Exemption->SymbolicPath,
					SymbolicPath)))
			{
				DebugPrint(("AddExemption(): RtleCopyUnicodeString failed, %.8x.",
						Status));
				break;
			}

			//
			// Insert the exemption to the tail of the list
			//
			InsertTailList(
					&ExemptionsList,
					(PLIST_ENTRY)Exemption);

		} while (0);

		//
		// Cleanup on failure
		//
		if (!NT_SUCCESS(Status))
		{
			if (Exemption)
				ExFreePool(
						Exemption);

			Exemption = NULL;
		}
	}

	//
	// If we found a matching exemption, synchronize it to the registry
	//
	if (Exemption)
	{
		AddUpdateRemoveExemptionToRegistry(
				Exemption->Scope,
				Exemption->Type,
				Exemption->Flags,
				&Exemption->SymbolicPath,
				FALSE);
	}

#if DBG
	if (NT_SUCCESS(Status))
	{
		DebugPrint(("AddExemption(): Added exemption: %d, %d, %.8x, %wZ",
				Scope,
				Type,
				Flags,
				&Exemption->SymbolicPath));
	}
#endif
	
	ExReleaseResourceLite(
			ExemptionsListResource);
	KeLowerIrql(OldIrql);

	return Status;
}

//
// Removes an exemption of a given type
//
NTSTATUS RemoveExemption(
		IN EXEMPTION_TYPE Type,
		IN EXEMPTION_SCOPE Scope,
		IN ULONG Flags,
		IN PUNICODE_STRING SymbolicPath,
		IN BOOLEAN RemoveFromRegistry)
{
	PEXEMPTION Exemption;
	NTSTATUS   Status = STATUS_NOT_FOUND;
	KIRQL      OldIrql;

	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceExclusiveLite(
			ExemptionsListResource,
			TRUE);

	//
	// Find the exemption based on the file path
	//
	if ((Exemption = GetExemptionFromFilePath(
			SymbolicPath,
			NULL,
			Type)))
	{
		BOOLEAN Remove = FALSE;

		//
		// Unset this type as a valid exemption
		//
		Exemption->Type &= ~(Type);

#if DBG
		DebugPrint(("RemoveExemption(): Removed exemption type %lu for file path %wZ.",
				Type,
				&Exemption->SymbolicPath));
#endif

		//
		// If this file has no more exemption types associated with it, remove it
		// from the list
		//
		if (Exemption->Type == 0)
		{
			RemoveEntryList(
					&Exemption->Entry);

			if (Exemption->SymbolicPath.Buffer)
				RtleFreeUnicodeString(
						&Exemption->SymbolicPath);
			ExFreePool(
					Exemption);

			Remove = TRUE;
		}

		//
		// If the caller wants us to remove the entry from the registry, do so now
		//
		if (RemoveFromRegistry)
		{
			AddUpdateRemoveExemptionToRegistry(
					Scope,
					Type,
					Flags,
					SymbolicPath,
					Remove);
		}

		Status = STATUS_SUCCESS;
	}

	ExReleaseResourceLite(
			ExemptionsListResource);
	KeLowerIrql(OldIrql);

	return Status;
}

//
// Flush all exemptions for a given exemption type
//
NTSTATUS FlushExemptions(
		IN EXEMPTION_TYPE Type,
		IN EXEMPTION_SCOPE Scope,
		IN BOOLEAN RemoveFromRegistry)
{
	PEXEMPTION Current, Next;
	KIRQL      OldIrql;

	ASSERT(Type != AnyExemption);

	KeRaiseIrql(APC_LEVEL, &OldIrql);
	ExAcquireResourceExclusiveLite(
			ExemptionsListResource,
			TRUE);

	//
	// Enumerate through all of the exemptions, flushing out all exemptions that
	// match a the given type
	//
	for (Current = (PEXEMPTION)ExemptionsList.Flink;
	     Current != (PEXEMPTION)&ExemptionsList;
	    )
	{
		Next = (PEXEMPTION)Current->Entry.Flink;

		//
		// If this entry matches the exemption type that is being flushed...
		//
		if (Current->Type & (ULONG)Type)
		{
			BOOLEAN Remove = FALSE;

			//
			// Second, unset the type for this exemption
			//
			Current->Type &= ~(Type);

			//
			// If this exemption has no more types associated with it, it's time to
			// remove it from the list of exemptions
			//
			if (Current->Type == 0)
				Remove = TRUE;

			AddUpdateRemoveExemptionToRegistry(
					Current->Scope,
					Current->Type,
					Current->Flags,
					&Current->SymbolicPath,
					Remove);

			if (Remove)
			{
				RemoveEntryList(
						(PLIST_ENTRY)Current);

				if (Current->SymbolicPath.Buffer)
					RtleFreeUnicodeString(
							&Current->SymbolicPath);
				ExFreePool(
						Current);

				Remove = TRUE;
			}
		}

		Current = Next;
	}
	
	ExReleaseResourceLite(
			ExemptionsListResource);
	KeLowerIrql(OldIrql);

	return STATUS_SUCCESS;
}

////
//
// Internal routines
//
////

//
// Ensures that all of the exemption keys are created
//
static VOID CreateExemptionRegistryKeys()
{
	OBJECT_ATTRIBUTES Attributes;
	UNICODE_STRING    KeyName;
	NTSTATUS          Status;
	HANDLE            Key;
	ULONG             Index;

	for (Index = 0;
	     ExemptionBaseKeys[Index];
	     Index++)
	{
		//
		// Initialize the key name
		//
		RtlInitUnicodeString(
				&KeyName,
				ExemptionBaseKeys[Index]);

		InitializeObjectAttributes(
				&Attributes,
				&KeyName,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);

		// 
		// Create the key
		//
		if (NT_SUCCESS(Status = ZwCreateKey(
				&Key,
				KEY_READ,
				&Attributes,
				0,
				NULL,
				0,
				NULL)))
			ZwClose(
					Key);
		else
		{
			DebugPrint(("CreateExemptionRegistryKeys(): WARNING: ZwCreateKey(%wZ) failed, %.8x.",
					&KeyName,
					Status));
		}
	}
}

//
// Initializes a given exemption type from a registry key
//
static NTSTATUS InitializeExemptionsFromKey(
		IN EXEMPTION_SCOPE Scope,
		IN PWSTR ExemptionKey)
{
	PKEY_BASIC_INFORMATION KeyInformation = NULL;
	OBJECT_ATTRIBUTES      Attributes;
	UNICODE_STRING         ExemptionKeyUnicodeString;
	UNICODE_STRING         SubkeyName;
	NTSTATUS               Status;
	ULONG                  KeyInformationSize = 4096;
	ULONG                  KeyInformationNeededSize;
	ULONG                  GrowTries = 0;
	ULONG                  Index = 0;
	HANDLE                 Key = NULL;

	do
	{
		//
		// Initialize the object attributes an exemption key string
		//
		RtlInitUnicodeString(
				&ExemptionKeyUnicodeString,
				ExemptionKey);

		InitializeObjectAttributes(
				&Attributes,
				&ExemptionKeyUnicodeString,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);

		//
		// Open the exemption key
		//
		if (!NT_SUCCESS(Status = ZwOpenKey(
				&Key,
				KEY_READ,
				&Attributes)))
		{
			DebugPrint(("InitializeExemptionsFromKey(): ZwOpenKey(%p) failed, %.8x.",
					ExemptionKey, 
					Status));
			break;
		}

	
		// 
		// Allocate storage for the value data
		//
		if (!(KeyInformation = (PKEY_BASIC_INFORMATION)ExAllocatePoolWithTag(
				NonPagedPool,
				KeyInformationSize,
				ALLOC_TAG)))
		{
			DebugPrint(("InitializeExemptionsFromKey(): ExAllocatePoolWithTag failed."));
			break;
		}

		//
		// Enumerate all of the sub keys
		//
		while (1)
		{
			Status = ZwEnumerateKey(
					Key,
					Index,
					KeyBasicInformation,
					KeyInformation,
					KeyInformationSize,
					&KeyInformationNeededSize);

			//
			// If the enumeration was not successful...
			//
			if (!NT_SUCCESS(Status))
			{
				//
				// If the buffer was too small...grow it
				//
				if ((Status == STATUS_BUFFER_TOO_SMALL) ||
				    (Status == STATUS_BUFFER_OVERFLOW))
				{
					//
					// If we tried to grow the buffer three times and it was still
					// not large enough, skip this entry
					//
					if (++GrowTries > 3)
					{
						DebugPrint(("InitializeExemptionsFromKey(): Could not grow buffer large enough for exemption %lu.",
								Index));

						GrowTries = 0;

						Index++;

						continue;
					}

					//
					// Grow the buffer to the amount specified
					//
					KeyInformationSize = KeyInformationNeededSize;

					GrowTries++;

					ExFreePool(
							KeyInformation);

					if (!(KeyInformation = (PKEY_BASIC_INFORMATION)ExAllocatePoolWithTag(
							NonPagedPool,
							KeyInformationSize,
							ALLOC_TAG)))
					{
						DebugPrint(("InitializeExemptionsFromKey(): ExAllocatePoolWithTag failed (2)."));
						break;
					}

					continue;
				}
				//
				// If the enumeration failed for a reason other than no more
				// entries...
				//
				else if (Status != STATUS_NO_MORE_ENTRIES)
				{
					DebugPrint(("InitializeExemptionsFromKey(): ZwEnumerateKey failed, %.8x.",
							Status));
					break;
				}
				//
				// Finally, if we're at the end, break out
				//
				else
				{
					Status = STATUS_SUCCESS;
					break;
				}
			}

			//
			// Initialize the sub key name
			//
			SubkeyName.Buffer        = KeyInformation->Name;
			SubkeyName.Length        = (USHORT)KeyInformation->NameLength;
			SubkeyName.MaximumLength = (USHORT)KeyInformation->NameLength;

			//
			// Initialize the exemption from this subkey for the current scope
			//
			AddExemptionFromRegistry(
					Scope,
					Key,
					&SubkeyName);

			Index++;
		}

	} while (0);

	//
	// Cleanup
	//
	if (KeyInformation)
		ExFreePool(
				KeyInformation);
	if (Key)
		ZwClose(
				Key);

	return Status;
}

//
// Initializes a given exemption from a given registry key path for a given
// scope
//
static NTSTATUS AddExemptionFromRegistry(
		IN EXEMPTION_SCOPE Scope,
		IN HANDLE ParentKey,
		IN PUNICODE_STRING SubkeyName)
{
	OBJECT_ATTRIBUTES Attributes;
	UNICODE_STRING    ExemptionPath = { 0 };
	EXEMPTION_TYPE    ExemptionType = 0;
	NTSTATUS          Status = STATUS_SUCCESS;
	HANDLE            Key = NULL;
	ULONG             ExemptionFlags = 0;

	do
	{
		//
		// Initialize and open the subkey
		//
		InitializeObjectAttributes(
				&Attributes,
				SubkeyName,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				ParentKey,
				NULL);
	
		if (!NT_SUCCESS(Status = ZwOpenKey(
				&Key,
				KEY_READ,
				&Attributes)))
		{
			DebugPrint(("InitializeExemptionFromKey(): ZwOpenKey failed, %.8x.",
					Status));
			break;
		}

		//
		// Query the exemption's attributes
		//
		if (!NT_SUCCESS(Status = RegQueryValueSz(
				Key,
				L"Path",
				&ExemptionPath)))
		{
			DebugPrint(("InitializeExemptionFromKey(): No path existed for exemption %wZ.",
					&SubkeyName));
			break;
		}

		//
		// Query the type and flags for this exemption
		//
		if (!NT_SUCCESS(RegQueryValueLong(
				Key,
				L"Type",
				&ExemptionType)))
		{
			DebugPrint(("InitializeExemptionFromKey(): Defaulting to application exemption for %wZ.",
					&SubkeyName));

			ExemptionType = ApplicationExemption;
		}

		RegQueryValueLong(
				Key,
				L"Flags",
				&ExemptionFlags);

		//
		// Add the exemption
		//
		Status = AddExemption(
				ExemptionType,
				Scope,
				ExemptionFlags,
				&ExemptionPath,
				FALSE);

	} while (0);

	//
	// Cleanup
	//
	if (ExemptionPath.Buffer)
		RtleFreeUnicodeString(
				&ExemptionPath);
	if (Key)
		ZwClose(
				Key);

	return Status;
}

//
// Adds, updates, or removes an exemption to the registry for the specified scope
//
static NTSTATUS AddUpdateRemoveExemptionToRegistry(
		IN EXEMPTION_SCOPE Scope,
		IN EXEMPTION_TYPE Type,
		IN ULONG Flags,
		IN PUNICODE_STRING ExemptionPath,
		IN BOOLEAN Remove)
{
	OBJECT_ATTRIBUTES Attributes;
	UNICODE_STRING    TempUnicodeString;
	NTSTATUS          Status = STATUS_SUCCESS;
	HANDLE            ExemptionKey = NULL;
	HANDLE            ScopeKey = NULL;
	WCHAR             ExemptionPathChecksum[EXEMPTION_PATH_CHECKSUM_SIZE];
	PWSTR             ScopeKeyName = NULL;

	do
	{
		//
		// First, determine the scope's key name
		//
		switch (Scope)
		{
			case GlobalScope:
				ScopeKeyName = WEHNTRUST_GLOBAL_SCOPE_EXEMPTION_KEY;
				break;
			default:
				DebugPrint(("AddUpdateRemoveExemptionToRegistry() Unknown scope: %d.",
						Scope));
				break;
		}

		if (!ScopeKeyName)
			break;

		//
		// Open the scope key
		//
		RtlInitUnicodeString(
				&TempUnicodeString,
				ScopeKeyName);

		InitializeObjectAttributes(
				&Attributes,
				&TempUnicodeString,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);

		if (!NT_SUCCESS(Status = ZwOpenKey(
				&ScopeKey,
				KEY_WRITE,
				&Attributes)))
		{
			DebugPrint(("AddUpdateRemoveExemptionToRegistry(): ZwOpenKey(%wZ) failed, %.8x.",
					&TempUnicodeString,
					Status));
			break;
		}

		//
		// Calculate the checksum of the exemption path passed in
		//
		CalculateExemptionPathChecksum(
				ExemptionPath,
				ExemptionPathChecksum);

		//
		// Create or open the existing exemption key based on the checksum of the
		// file path
		//
		TempUnicodeString.Buffer        = ExemptionPathChecksum;
		TempUnicodeString.Length        = sizeof(ExemptionPathChecksum);
		TempUnicodeString.MaximumLength = sizeof(ExemptionPathChecksum);

		InitializeObjectAttributes(
				&Attributes,
				&TempUnicodeString,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				ScopeKey,
				NULL);

		if (!NT_SUCCESS(Status = ZwCreateKey(
				&ExemptionKey,
				KEY_ALL_ACCESS,
				&Attributes,
				0,
				NULL,
				0,
				NULL)))
		{
			DebugPrint(("AddUpdateRemoveExemptionToRegistry(): ZwCreateKey(%wZ) failed, %.8x.",
					&TempUnicodeString,
					Status));
			break;
		}

		//
		// Now, perform the action that we were meant to perform
		//
		if (Remove)
		{
			RegDeleteValue(
					ExemptionKey,
					L"Path");
			RegDeleteValue(
					ExemptionKey,
					L"Type");
			RegDeleteValue(
					ExemptionKey,
					L"Flags");
			
			if (!NT_SUCCESS(Status = ZwDeleteKey(
					ExemptionKey)))
			{
				DebugPrint(("AddUpdateRemoveExemptionToRegistry(): ZwDeleteKey failed, %.8x.",
						Status));
			}
		}
		else
		{
			RegSetValueLong(
					ExemptionKey,
					L"Type",
					Type);
			RegSetValueLong(
					ExemptionKey,
					L"Flags",
					Flags);
			RegSetValueSz(
					ExemptionKey,
					L"Path",
					ExemptionPath);
		}

	} while (0);

	if (ExemptionKey)
		ZwClose(
				ExemptionKey);
	if (ScopeKey)
		ZwClose(
				ScopeKey);

	return Status;
}

//
// Calculates the checksum of the supplied path
//
static VOID CalculateExemptionPathChecksum(
		IN PUNICODE_STRING ExemptionPath,
		OUT WCHAR Checksum[EXEMPTION_PATH_CHECKSUM_SIZE])
{
	SHA1_CTX ShaContext;
	UCHAR    ShaDigest[SHA1_HASH_SIZE];
	ULONG    Length = ExemptionPath->Length;
	ULONG    Index  = Length / sizeof(WCHAR);

	//
	// If the buffer is null terminated, do not include the NULL terminator in
	// the checksum calculation
	//
	if (Index > 0)
	{
		Index--;

		while ((Index) && (!ExemptionPath->Buffer[Index]))
		{
			Length -= sizeof(WCHAR);
			Index--;
		}
	}
		
	//
	// Calculate the SHA1 hash of the file path that was provided to us
	//
	SHA1_Init(
			&ShaContext);

	SHA1_Update(
			&ShaContext,
			(PUCHAR)ExemptionPath->Buffer,
			Length);

	SHA1_Final(
			&ShaContext,
			ShaDigest);

	//
	// Build the string version of the checksum
	//
	_snwprintf(
			Checksum,
			EXEMPTION_PATH_CHECKSUM_SIZE,
			L"%.8x%.8x%.8x%.8x%.8x",
			ShaContext.A,
			ShaContext.B,
			ShaContext.C,
			ShaContext.D,
			ShaContext.E);
}

//
// Gets the exemption context that is associated with the supplied file path.
//
// This function assumes that it is called in a locked context.
//
static PEXEMPTION GetExemptionFromFilePath(
		IN PUNICODE_STRING SymbolicPath OPTIONAL,
		IN PUNICODE_STRING FullFilePath OPTIONAL,
		IN EXEMPTION_TYPE Type)
{
	PEXEMPTION Exemption = NULL, Current = NULL;

	for (Current = (PEXEMPTION)ExemptionsList.Flink;
	     Current != (PEXEMPTION)&ExemptionsList;
	     Current = (PEXEMPTION)Current->Entry.Flink)
	{
		if (SymbolicPath)
		{
			DebugPrint(("GetExemptionFromFilePath(): Comparing symbolic '%wZ' to '%wZ' (%.8x %.8x)...",
					SymbolicPath,
					&Current->SymbolicPath,
					Current->Type,
					Type));

			if ((!RtlCompareUnicodeString(
					SymbolicPath,
					&Current->SymbolicPath,
					TRUE)) &&
				 (Current->Type & (ULONG)Type))
			{
				DebugPrint(("GetExemptionFromFilePath(): Found match %p.",
						Current));

				Exemption = Current;
				break;
			}
		}
		else if (FullFilePath)
		{
			DebugPrint(("GetExemptionFromFilePath(): Comparing full '%wZ' to '%wZ'...",
					FullFilePath,
					&Current->SymbolicPath));

			if ((RtleCompareNtPathToPhysicalPath(
					&Current->SymbolicPath,
					FullFilePath,
					Current->Type & DirectoryExemption
						? TRUE
						: FALSE)))
			{
				Exemption = Current;
				break;
			}
		}
	}

	return Exemption;
}
