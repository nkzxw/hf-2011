/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "precomp.h"

//
// Copy a UNICODE_STRING's contents to another UNICODE_STRING
//
NTSTATUS RtleCopyUnicodeString(
		OUT PUNICODE_STRING Destination,
		IN PUNICODE_STRING Source)
{
	NTSTATUS Status = STATUS_NO_MEMORY;

	//
	// Allocate storage for the destination buffer
	//
	if ((Destination->Buffer = (PWSTR)ExAllocatePoolWithTag(
			PagedPool,
			Source->Length, 
			ALLOC_TAG)))
	{
		USHORT CopyLength = Source->Length;

		//
		// Don't copy the NULL terminator if one exists
		//
		CopyLength = Source->Length;

		while ((CopyLength > 0) &&
		       (Source->Buffer[(CopyLength / sizeof(WCHAR)) - 1] == 0))
			CopyLength -= sizeof(WCHAR);

		//
		// Copy the source buffer to the destination buffer
		//
		RtlCopyMemory(
				Destination->Buffer,
				Source->Buffer,
				CopyLength);

		Destination->Length        = CopyLength;
		Destination->MaximumLength = CopyLength;

		Status = STATUS_SUCCESS;
	}
	else
	{
		Destination->Length        = 0;
		Destination->MaximumLength = 0;
	}

	return Status;
}

//
// Frees buffers allocated by Rtle functions 
//
NTSTATUS RtleFreeUnicodeString(
		IN PUNICODE_STRING UnicodeString)
{
	if (UnicodeString->Buffer)
		ExFreePool(UnicodeString->Buffer);

	UnicodeString->Buffer        = NULL;
	UnicodeString->Length        = 0;
	UnicodeString->MaximumLength = 0;

	return STATUS_SUCCESS;
}

//
// Searches memory for a needle in a haystack.  This is analogus to memmem on
// *nix variants.  If found, a pointer to the needle is returned.  Otherwise,
// NULL is returned.
//
PUCHAR RtleSearchMemory(
		IN PUCHAR Haystack,
		IN ULONG HaystackLength,
		IN PUCHAR Needle,
		IN ULONG NeedleLength)
{
	PUCHAR Result = NULL;
	ULONG  HaystackIndex, NeedleIndex; 

	do
	{
		//
		// If the needle is larger than the haystack, bomb.
		//
		if (NeedleLength > HaystackLength)
			break;

		//
		// Walk the haystack
		//
		for (HaystackIndex = 0;
		     HaystackIndex < HaystackLength - NeedleLength;
		     HaystackIndex++)
		{
			//
			// Compare the needle to the current offset into the haystack
			//
			for (NeedleIndex = 0;
			     NeedleIndex < NeedleLength;
			     NeedleIndex++)
			{
				if (Haystack[HaystackIndex + NeedleIndex] != Needle[NeedleIndex])
					break;
			}

			//
			// If the needle index is equal to the needle length it means we've
			// found a match.
			//
			if (NeedleIndex == NeedleLength)
			{
				Result = Haystack + HaystackIndex;
				break;
			}
		}
	
	} while (0);

	return Result;
}

//
// Convert a unicode string into an integer of a given base
//
ULONG RtleUnicodeStringToInteger(
		IN PUNICODE_STRING UnicodeString,
		IN ULONG Base)
{
	ULONG Accumulator, BaseAccumulator = 1, Value;
	LONG Index;
	
	if (!UnicodeString->Length)
		return 0;

	for (Index = (UnicodeString->Length / sizeof(WCHAR)) - 1, Accumulator = 0;
	     Index >= 0;
	     Index--, BaseAccumulator *= Base)
	{
		Value = (ULONG)UnicodeString->Buffer[Index];	

		if (Value > 255)
			continue;

		RtlCharToInteger(
				(PCSZ)&Value, 
				Base,
				&Value);
		
		Accumulator += BaseAccumulator * Value;
	}

	return Accumulator;
}

//
// Find a unicode string inside another unicode string
//
PWSTR RtleFindUnicodeStringInUnicodeString(
		IN PUNICODE_STRING Haystack,
		IN PUNICODE_STRING Needle)
{
	USHORT OutterOffset, InnerOffset;
	USHORT RealHaystackLength = Haystack->Length;
	USHORT RealNeedleLength = Needle->Length;
	ULONG  InnerNumCharacters;
	ULONG  OutterNumCharacters;
	PWSTR  Result = NULL;

	do
	{
		//
		// Zero sized needle/haystack? Bogus.
		//
		if ((!RealNeedleLength) ||
		    (!RealHaystackLength))
			break;

		//
		// If the last byte of the needle is a null terminator or the last byte of
		// the haystack is a null terminator, drop the length down by one for
		// either.
		//
		if (!Needle->Buffer[(RealNeedleLength / sizeof(WCHAR)) - 1])
			RealNeedleLength -= sizeof(WCHAR);
		if (!Haystack->Buffer[(RealHaystackLength / sizeof(WCHAR)) - 1])
			RealHaystackLength -= sizeof(WCHAR);

		//
		// If the needle length is larger than the haystack length, we can't
		// possibly have a match
		//
		if (RealNeedleLength > RealHaystackLength)
			break;

		InnerNumCharacters  = RealNeedleLength / sizeof(WCHAR);
		OutterNumCharacters = ((RealHaystackLength - RealNeedleLength) / sizeof(WCHAR)) + 1;

		for (OutterOffset = 0;
		     OutterOffset < OutterNumCharacters;
		     OutterOffset++)
		{
			for (InnerOffset = 0;
			     InnerOffset < InnerNumCharacters;
			     InnerOffset++)
			{
				if (RtlUpcaseUnicodeChar(Needle->Buffer[InnerOffset]) != RtlUpcaseUnicodeChar(Haystack->Buffer[OutterOffset + InnerOffset]))
					break;
			}

			if (InnerOffset == InnerNumCharacters)
			{
				Result = Haystack->Buffer + OutterOffset;
				break;
			}
		}

	} while (0);

	return Result;
}

//
// Find a raw unicode string inside a unicode string
//
PWSTR RtleFindStringInUnicodeString(
		IN PUNICODE_STRING Haystack,
		IN PWSTR Needle)
{
	UNICODE_STRING NeedleString;

	RtlInitUnicodeString(
			&NeedleString,
			Needle);

	return RtleFindUnicodeStringInUnicodeString(
			Haystack,
			&NeedleString);
}

//
// Obtains the modification time for the provided file object
//
NTSTATUS RtleGetFileModificationTime(
		IN PFILE_OBJECT FileObject,
		OUT PLARGE_INTEGER ModificationTime)
{
	FILE_BASIC_INFORMATION BasicInformation;
	NTSTATUS               Status;
	ULONG                  Needed = 0;

	do
	{
		//
		// Query the file object for basic information such that we can obtained
		// the LastWriteTime
		//
		if (!NT_SUCCESS(Status = IoQueryFileInformation(
				FileObject,
				FileBasicInformation,
				sizeof(BasicInformation),
				&BasicInformation,
				&Needed)))
		{
			DebugPrint(("RtleGetFileModificationTime(): IoQueryFileInformation failed, %.8x.",
					Status));
			break;
		}

		//
		// Set the modification time for the file such that it can be compared
		// with an existing entry
		//
		*ModificationTime = BasicInformation.LastWriteTime;

	} while (0);

	return Status;
}

//
// Checks to see whether or not a file exists on disk
//
BOOLEAN RtleDoesFileExist(
		IN PUNICODE_STRING FilePath)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK   IoStatus;
	BOOLEAN           Exists = FALSE;
	HANDLE            Handle = NULL;

	InitializeObjectAttributes(
			&ObjectAttributes,
			FilePath,
			OBJ_KERNEL_HANDLE,
			NULL,
			NULL);

	if (NT_SUCCESS(ZwCreateFile(
			&Handle,
			GENERIC_READ,
			&ObjectAttributes,
			&IoStatus,
			NULL,
			0,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			FILE_OPEN,
			0,
			NULL,
			0)))
	{
		Exists = TRUE;

		ZwClose(
				Handle);
	}

	return Exists;
}

//
// Get the file object's path name
//
NTSTATUS RtleGetFilePath(
		IN PFILE_OBJECT FileObject,
		OUT POBJECT_NAME_INFORMATION *FilePath)
{
	POBJECT_NAME_INFORMATION NameInformation = NULL;
	NTSTATUS                 Status = STATUS_SUCCESS;
	ULONG                    NameInformationSize = 8192;
	ULONG                    Tried = 0;

	do
	{
allocate_again:
		// 
		// Allocate storage for the name information
		//
		if (!(NameInformation = (POBJECT_NAME_INFORMATION)ExAllocatePoolWithTag(
				NonPagedPool,
				NameInformationSize,
				ALLOC_TAG)))
		{
			DebugPrint(("RtleGetFilePath(): ExAllocatePoolWithTag failed."));
			break;	
		}

		//
		// Query the file object's name
		//
		if (!NT_SUCCESS(Status = ObQueryNameString(
				FileObject,
				NameInformation,
				NameInformationSize,
				&NameInformationSize)))
		{
			if (Status != STATUS_BUFFER_TOO_SMALL)
			{
				DebugPrint(("RtleGetFilePath(): ObQueryNameString failed, %.8x.",
						Status));
				break;
			}

			//
			// Free the buffer so that it can be allocated again
			//
			ExFreePool(
					NameInformation);

			NameInformation = NULL;

			//
			// Try again
			//
			if (++Tried < 2)
				goto allocate_again;
			else
				break;
		}

	} while (0);

	//
	// If we failed, cleanup
	//
	if (!NT_SUCCESS(Status))
	{
		if (NameInformation)
			ExFreePool(
					NameInformation);

		NameInformation = NULL;
	}

	//
	// Set the out pointer
	//
	if (FilePath)
		*FilePath = NameInformation;
	else if (NameInformation)
		ExFreePool(
				NameInformation);

	return Status;
}

//
// Free a path name that was allocated by a previous call to RtleGetFilePath
//
VOID RtleFreeFilePath(
		IN POBJECT_NAME_INFORMATION FilePath)
{
	ExFreePool(
			FilePath);
}

//
// Compares an NT file path to a physical file path in a wildcard fashion.  NT
// paths are formatted like:
//
// \??\C:\windows\system32\calc.exe
//
// Physical file paths are formatted like:
//
// \Device\HarddiskVolume1\Windows\system32\calc.exe
//
BOOLEAN RtleCompareNtPathToPhysicalPath(
		IN PUNICODE_STRING NtFilePath,
		IN PUNICODE_STRING PhysicalFilePath,
		IN BOOLEAN IsDirectory)
{
	BOOLEAN IsMatch = FALSE;
	PWCHAR  CurrentNtFilePath, CurrentPhysicalFilePath;
	USHORT  CurrentNtFileOffset, CurrentPhysicalFileOffset;

	do
	{
		//
		// If the supplied NT path is a directory, we have to do a special
		// comparison
		//
		if (IsDirectory)
		{
			UNICODE_STRING NtPathNoPrefix;
			PWSTR          Current = NtFilePath->Buffer;
			USHORT         Offset  = 0;

			while (Offset < NtFilePath->Length)
			{
				Offset += sizeof(WCHAR);

				if (*Current++ == ':')
					break;
			}

			if (Offset < NtFilePath->Length)
			{
				NtPathNoPrefix.Buffer        = Current;
				NtPathNoPrefix.Length        = NtFilePath->Length - Offset;
				NtPathNoPrefix.MaximumLength = NtFilePath->Length - Offset;
			}
			else
			{
				NtPathNoPrefix.Buffer        = NtFilePath->Buffer;
				NtPathNoPrefix.Length        = NtFilePath->Length;
				NtPathNoPrefix.MaximumLength = NtFilePath->MaximumLength;
			}

			//
			// Search for the directory without a prefix in the physical path to
			// see if we should exempt this thing or not.  This has a bug in that
			// it could exempt directories across multiple physical volumes, though
			// it should be unlikely to occur.
			//
			if (RtleFindUnicodeStringInUnicodeString(
					PhysicalFilePath,
					&NtPathNoPrefix))
			{
				IsMatch = TRUE;
			}

			break;
		}

		// 
		// We have something to work with, right?
		//
		if ((!NtFilePath->Length) ||
		    (!PhysicalFilePath->Length))
			break;

		//
		// Initialize the index offset that we're working with
		//
		CurrentNtFileOffset       = (NtFilePath->Length - sizeof(WCHAR)) / sizeof(WCHAR);
		CurrentPhysicalFileOffset = (PhysicalFilePath->Length - sizeof(WCHAR)) / sizeof(WCHAR);

		if ((!CurrentNtFileOffset) ||
		    (!CurrentPhysicalFileOffset))
			break;

		//
		// Initialize the current path pointers
		//
		CurrentNtFilePath       = NtFilePath->Buffer;
		CurrentPhysicalFilePath = PhysicalFilePath->Buffer;

		//
		// Adjust the offset if there is a null terminator
		//
		if (!CurrentNtFilePath[CurrentNtFileOffset])
			CurrentNtFileOffset--;
		if (!CurrentPhysicalFilePath[CurrentPhysicalFileOffset])
			CurrentPhysicalFileOffset--;

		do
		{
			//
			// If the upper case compare of the two file paths does not match at
			// this offset...
			//
			if (RtlUpcaseUnicodeChar(CurrentNtFilePath[CurrentNtFileOffset]) !=
			    RtlUpcaseUnicodeChar(CurrentPhysicalFilePath[CurrentPhysicalFileOffset]))
			{
				//
				// If the portion of the NT file path is a colon, flag the paths as
				// having been matched because that means everything up to the colon
				// matched.
				//
				if (CurrentNtFilePath[CurrentNtFileOffset] == ':')
					IsMatch = TRUE;

				break;
			}

		} while ((CurrentNtFileOffset-- > 0) &&
		         (CurrentPhysicalFileOffset-- > 0));

		//
		// Also, if both the NT file path and the physical file path offset are
		// zero, that means the entire strings matched, which is coo.
		//
		if ((!CurrentNtFileOffset) &&
		    (!CurrentPhysicalFileOffset))
			IsMatch = TRUE;

	} while (0);

	return IsMatch;
}

//
// Chops off the NULL terminator if one exists in the supplied unicode string
//
VOID RtleTruncateUnicodeTerminator(
		IN PUNICODE_STRING UnicodeString)
{
	if (UnicodeString->Length > 0)
	{
		USHORT NullOffset = (UnicodeString->Length / sizeof(WCHAR)) - 1;

		if (!UnicodeString->Buffer[NullOffset])
		{
			UnicodeString->Length        -= sizeof(WCHAR);
			UnicodeString->MaximumLength -= sizeof(WCHAR);
		}
	}
}

//
// Flushes the contents of a directory
//
#define DIRECTORY_LIST_BUFFER_SIZE 32768

NTSTATUS RtleFlushDirectory(
		IN PUNICODE_STRING DirectoryPath,
		IN BOOLEAN IgnoreInUse)
{
	PFILE_DIRECTORY_INFORMATION DirectoryList = NULL, DirectoryEntry, LastEntry = NULL;
	OBJECT_ATTRIBUTES           ObjectAttributes;
	IO_STATUS_BLOCK             IoStatus;
	UNICODE_STRING              FileName;
	NTSTATUS                    Status;
	BOOLEAN                     RestartScan = TRUE;
	HANDLE                      DirectoryFileHandle = NULL;

	do
	{
		//
		// Open the directory for content querying
		//
		InitializeObjectAttributes(
				&ObjectAttributes,
				DirectoryPath,
				COMPAT_OBJ_KERNEL_HANDLE,
				NULL,
				0);

		if (!NT_SUCCESS(Status = ZwOpenFile(
				&DirectoryFileHandle,
				FILE_LIST_DIRECTORY | FILE_TRAVERSE | SYNCHRONIZE,
				&ObjectAttributes,
				&IoStatus,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_DIRECTORY_FILE)))
		{
			DebugPrint(("RtleFlushDirectory(): Failed to open directory %wZ, %.8x.",
					DirectoryPath,
					Status));
			break;
		}

		//
		// Allocate storage for holding the individual files in the directory
		//
		if (!(DirectoryList = (PFILE_DIRECTORY_INFORMATION)ExAllocatePoolWithTag(
				PagedPool,
				DIRECTORY_LIST_BUFFER_SIZE,
				ALLOC_TAG)))
		{
			DebugPrint(("RtleFlushDirectory(): ExAllocatePoolWithTag failed."));

			Status = STATUS_NO_MEMORY;
			break;
		}

		//
		// Keep looping until we run out of files...
		//
		while (1)
		{
			Status = ZwQueryDirectoryFile(
					DirectoryFileHandle,
					0,
					0,
					0,
					&IoStatus,
					DirectoryList,
					DIRECTORY_LIST_BUFFER_SIZE,
					FileDirectoryInformation,
					FALSE,
					0,
					RestartScan);

			RestartScan = FALSE;

			if (Status == STATUS_PENDING)
			{
				Status = ZwWaitForSingleObject(
						DirectoryFileHandle,
						FALSE,
						0);

				if (Status != STATUS_WAIT_0)
				{
					DebugPrint(("RtleFlushDirectory(): ZwWaitForSingleObject returned expectedly, %.8x.",
							Status));

					if (NT_SUCCESS(Status))
						Status = STATUS_UNSUCCESSFUL;

					break;
				}

				Status = IoStatus.Status;
			}

			//
			// Out of items?
			//
			if (Status == STATUS_NO_MORE_FILES)
			{
				Status = STATUS_SUCCESS;
				break;
			}

			if (!NT_SUCCESS(Status))
			{
				DebugPrint(("RtleFlushDirectory(): ZwQueryDirectoryFile failed, %.8x.",
						Status));
				break;
			}

			//
			// Enumerate all of the entries that were given to us and delete them
			// assuming they aren't themselves directories
			//
			for (DirectoryEntry = DirectoryList;
			     LastEntry != DirectoryEntry;
			     LastEntry = DirectoryEntry, 
			     DirectoryEntry = (PFILE_DIRECTORY_INFORMATION)((PUCHAR)DirectoryEntry + DirectoryEntry->NextEntryOffset))
			{
				//
				// Skip directories
				//
				if (DirectoryEntry->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					continue;

				FileName.Buffer        = DirectoryEntry->FileName;
				FileName.Length        = (USHORT)DirectoryEntry->FileNameLength;
				FileName.MaximumLength = (USHORT)DirectoryEntry->FileNameLength;

				InitializeObjectAttributes(
						&ObjectAttributes,
						&FileName,
						COMPAT_OBJ_KERNEL_HANDLE,
						DirectoryFileHandle,
						0);

				Status = ZwDeleteFile(
						&ObjectAttributes);

				//
				// If the delete failed...
				//
				if ((!NT_SUCCESS(Status)) &&
				    ((!IgnoreInUse) ||
				     ((IgnoreInUse) && (Status != STATUS_SHARING_VIOLATION))))
				{
					DebugPrint(("RtleFlushDirectory(): Failed to remove %wZ, %.8x.",
							&FileName, 
							Status));
				}
			}
		}

	} while (0);

	//
	// Close handles and free memory
	//
	if (DirectoryList)
		ExFreePool(
				DirectoryList);
	if (DirectoryFileHandle)
		ZwClose(
				DirectoryFileHandle);

	return Status;
}

//
// Returns the thread object associated with the supplied thread handle if one
// exists.  The reference count to the object is incremented by one.  Callers
// must call ObDereferenceObject on the output object when they are done using
// it.
//
NTSTATUS RtleGetThreadObject(
		IN HANDLE ThreadHandle,
		IN ACCESS_MASK DesiredAccess,
		OUT PTHREAD_OBJECT *ThreadObject)
{
	PTHREAD_OBJECT Object = NULL;
	NTSTATUS       Status = STATUS_SUCCESS;

	//
	// If the thread is equivalent to the current thread, we can use
	// PsGetCurrentThread
	//
	if (ThreadHandle == NtCurrentThread())
	{
		Object = (PTHREAD_OBJECT)PsGetCurrentThread();

		//
		// Increment the reference count to the thread object.
		//
		ObReferenceObject(
				Object);
	}
	//
	// Otherwise, reference the object via the handle.
	//
	else
	{
		Status = ObReferenceObjectByHandle(
				ThreadHandle,
				DesiredAccess,
				*PsThreadType,
				KeGetPreviousMode(),
				(PVOID *)&Object,
				NULL);
	}

	if (Object)
		*ThreadObject = Object;

	return Status;
}

//
// Returns the object associated with the supplied process handle.  The
// reference count to the object is incremented by one.  Callers must call
// ObDereferenceObject on the output object when they are done using it.
//
NTSTATUS RtleGetProcessObject(
		IN HANDLE ProcessHandle,
		IN ACCESS_MASK DesiredAccess,
		OUT PPROCESS_OBJECT *ProcessObject)
{
	PPROCESS_OBJECT Object = NULL;
	NTSTATUS        Status = STATUS_SUCCESS;

	//
	// If the process is equivalent to the current process, we can use
	// PsGetCurrentProcess
	//
	if (ProcessHandle == NtCurrentProcess())
	{
		Object = PsGetCurrentProcess();

		//
		// Increment the reference count to the process object.
		//
		ObReferenceObject(
				Object);
	}
	//
	// Otherwise, reference the object via the handle.
	//
	else
	{
		Status = ObReferenceObjectByHandle(
				ProcessHandle,
				DesiredAccess,
				*PsProcessType,
				KeGetPreviousMode(),
				(PVOID *)&Object,
				NULL);
	}

	if (Object)
		*ProcessObject = Object;

	return Status;
}
