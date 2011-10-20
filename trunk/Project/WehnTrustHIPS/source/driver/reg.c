#include "precomp.h"

//
// Set a REG_DWORD value on the supplied key
//
NTSTATUS RegSetValueLong(
		IN HANDLE Key,
		IN PWSTR ValueName,
		IN ULONG ValueData)
{
	UNICODE_STRING ValueNameUnicodeString;

	RtlInitUnicodeString(
			&ValueNameUnicodeString,
			ValueName);

	return ZwSetValueKey(
			Key,
			&ValueNameUnicodeString,
			0,
			REG_DWORD,
			&ValueData,
			sizeof(ValueData));
}

//
// Query a REG_DWORD value from the supplied key
//
NTSTATUS RegQueryValueLong(
		IN HANDLE Key,
		IN PWSTR ValueName,
		OUT PULONG ValueData)
{
	PKEY_VALUE_PARTIAL_INFORMATION Information;
	UNICODE_STRING                 ValueNameUnicodeString;
	NTSTATUS                       Status;
	UCHAR                          Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
	ULONG                          SizeRequired = 0;

	Information = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;

	RtlInitUnicodeString(
			&ValueNameUnicodeString,
			ValueName);

	if (NT_SUCCESS(Status = ZwQueryValueKey(
			Key,
			&ValueNameUnicodeString,
			KeyValuePartialInformation,
			Information,
			sizeof(Buffer),
			&SizeRequired)))
	{
		*ValueData = *(PULONG)Information->Data;
	}
	else
	{
		*ValueData = 0;
	}

	return Status;	
}

NTSTATUS RegQueryValueBinary(
		IN HANDLE Key,
		IN PWSTR ValueName,
		OUT PUCHAR ValueData,
		OUT PULONG ValueSize)
{
	PKEY_VALUE_PARTIAL_INFORMATION Information;
	UNICODE_STRING                 ValueNameUnicodeString;
	NTSTATUS                       Status;
	UCHAR                          Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256];
	ULONG                          SizeRequired = 0;

	//
	// Sanity check.
	//
	if (*ValueSize > 256)
	{
		ASSERT(0);

		DebugPrint(("RegQueryValueBinary doesn't support >256 bytes"));

		return STATUS_BUFFER_OVERFLOW;
	}

	Information = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;

	RtlInitUnicodeString(
			&ValueNameUnicodeString,
			ValueName);

	if (NT_SUCCESS(Status = ZwQueryValueKey(
			Key,
			&ValueNameUnicodeString,
			KeyValuePartialInformation,
			Information,
			sizeof(Buffer),
			&SizeRequired)))
	{
		*ValueSize = Information->DataLength;

		RtlCopyMemory(
				ValueData,
				Information->Data,
				Information->DataLength);
	}

	return Status;	
}

//
// Set a REG_SZ value on the supplied key
//
NTSTATUS RegSetValueSz(
		IN HANDLE Key,
		IN PWSTR ValueName,
		IN PUNICODE_STRING UnicodeString)
{
	UNICODE_STRING ValueNameUnicodeString;
	NTSTATUS       Status;
	BOOLEAN        Allocated = FALSE;
	PWSTR          ValueString = NULL;
	ULONG          ValueStringLength = 0;

	RtlInitUnicodeString(
			&ValueNameUnicodeString,
			ValueName);

	do
	{
		// 
		// Check to see if the supplied unicode string is null terminated
		//
		if (UnicodeString->Length > 0)
		{
			ULONG Index = (UnicodeString->Length / sizeof(WCHAR)) - 1;

			//
			// If the string is not null terminated, allocate a temporary buffer
			// that is null terminated and copy the supplied buffer into it.
			//
			if (UnicodeString->Buffer[Index])
			{
				ValueStringLength = UnicodeString->Length + sizeof(WCHAR);

				if (!(ValueString = (PWSTR)ExAllocatePoolWithTag(
						NonPagedPool,
						ValueStringLength,
						ALLOC_TAG)))
				{
					DebugPrint(("RegSetValueSz(%wZ): ExAllocatePoolWithTag failed.",
							&ValueNameUnicodeString));

					Status = STATUS_INSUFFICIENT_RESOURCES;
					break;
				}

				ValueString[Index + 1] = 0;

				RtlCopyMemory(
						ValueString,
						UnicodeString->Buffer,
						UnicodeString->Length);

				Allocated = TRUE;
			}
			else
			{
				ValueString       = UnicodeString->Buffer;
				ValueStringLength = UnicodeString->Length;
			}
		}

		//
		// No love.
		//
		if (!ValueString)
		{
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		Status = ZwSetValueKey(
				Key,
				&ValueNameUnicodeString,
				0,
				REG_SZ,
				ValueString,
				ValueStringLength);

	} while (0);

	//
	// Free up the value string if we had to allocate it in order to null
	// terminate it
	//
	if (Allocated)
		ExFreePool(
				ValueString);

	return Status;
}

//
// Query a REG_SZ value from the supplied key
//
NTSTATUS RegQueryValueSz(
		IN HANDLE Key,
		IN PWSTR ValueName,
		OUT PUNICODE_STRING UnicodeString)
{
	PKEY_VALUE_PARTIAL_INFORMATION Information = NULL;
	UNICODE_STRING                 ValueNameUnicodeString;
	UNICODE_STRING                 TempUnicodeString;
	NTSTATUS                       Status;
	ULONG                          InformationSize = 4096;
	ULONG                          Attempts = 0;

	RtlInitUnicodeString(
			&ValueNameUnicodeString,
			ValueName);

	//
	// Initialize the output buffer to NULL
	//
	UnicodeString->Buffer = NULL;
	UnicodeString->Length = 0;

	//
	// Try three times if our buffer is too small
	//
	while (Attempts++ < 3)
	{
		//
		// Allocate storage based off the current size
		//
		if (Information)
			ExFreePool(
					Information);

		if (!(Information = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(
				NonPagedPool,
				InformationSize,
				ALLOC_TAG)))
		{
			DebugPrint(("RegQueryValueSz(%wZ): ExAllocatePoolWithTag failed.",
					&ValueNameUnicodeString));

			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		//
		// Execute the query
		//
		if (!NT_SUCCESS(Status = ZwQueryValueKey(
				Key,
				&ValueNameUnicodeString,
				KeyValuePartialInformation,
				(PVOID)Information,
				InformationSize,
				&InformationSize)))
		{
			//
			// If we failed due to buffer problems, try again...
			//
			if ((Status == STATUS_BUFFER_TOO_SMALL) ||
			    (Status == STATUS_BUFFER_OVERFLOW))
				continue;
			else
			{
				DebugPrint(("RegQueryValueSz(%wZ): ZwQueryValueKey failed, %.8x.",
						&ValueNameUnicodeString,
						Status));
				break;
			}
		}
			
		//
		// If we were successful, copy the string into the output unicode string
		//
		TempUnicodeString.Buffer        = (PWSTR)Information->Data;
		TempUnicodeString.Length        = (USHORT)Information->DataLength;
		TempUnicodeString.MaximumLength = (USHORT)Information->DataLength;

		RtleCopyUnicodeString(
				UnicodeString,
				&TempUnicodeString);

		break;
	}

	//
	// Free the information buffer if it's valid
	//
	if (Information)
		ExFreePool(
				Information);

	return Status;
}

//
// Deletes a given registry value
//
NTSTATUS RegDeleteValue(
		IN HANDLE Key,
		IN PWSTR ValueName)
{
	UNICODE_STRING ValueNameUnicodeString;

	RtlInitUnicodeString(
			&ValueNameUnicodeString,
			ValueName);

	return ZwDeleteValueKey(
			Key,
			&ValueNameUnicodeString);
}

