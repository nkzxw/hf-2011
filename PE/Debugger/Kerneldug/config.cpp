/*++

	This is the part of NGdbg kernel debugger

	config.cpp

	Contains routines that query confuguration information from the registry

--*/

#include <ntifs.h>
#include "config.h"

NTSTATUS
QueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )

/*++

Routine Description

	QueryRoutine for RtlQueryRegistryValues()

Arguments

	ValueName    Unicode string containing value name
	ValueType    Value type
	ValueData    Pointer to the data
	ValueLength  Data length
	Context      User-supplied context, here this is a pointer to USER_ARGUMENTS
	EntryContext Entry context, specified in RTL_QUERY_REGISTRY_TABLE, in out case is always NULL

Return Value

	STATUS_SUCCESS on success
	error status on error

This is a callback function

--*/

{
	DC_QUERY_REGISTRY_USER_ARGUMENTS* Args = (DC_QUERY_REGISTRY_USER_ARGUMENTS*) Context;

	if( ValueType == REG_DWORD )
	{
		if( *Args->BufferLength < 4 )
			return (Args->ReturnStatus = STATUS_BUFFER_TOO_SMALL);

		*(ULONG*)Args->Buffer = *(ULONG*)ValueData;
		*Args->BufferLength = 4;
		return ( Args->ReturnStatus = STATUS_SUCCESS );
	}
	else if( ValueType == REG_SZ )
	{
		if( *Args->BufferLength < ValueLength )
			return ( Args->ReturnStatus = STATUS_BUFFER_TOO_SMALL );

		RtlCopyMemory( Args->Buffer, ValueData, ValueLength );
		*Args->BufferLength = ValueLength;
		return ( Args->ReturnStatus = STATUS_SUCCESS );
	}

	return ( Args->ReturnStatus = STATUS_INVALID_PARAMETER );
}

extern "C"  extern int _cdecl _snwprintf (wchar_t*, int, ...);

extern "C" 
NTSTATUS
QueryRegistrySetting(
	IN PWSTR Path,
	IN PWSTR Name,
	IN ULONG Type,
	OUT PVOID Value,
	IN OUT PULONG BufferLength
	)

/*++

Routine Description

	This function loads debugger-specific information from the registry

Arguments

	Path   Unicode string containing registry path relative to \Registry\Machine\Software\NGdbg\ key
	Name   Unicode string containing registry value name
	Type   Value type (REG_xxx)
	Value  Output buffer receiving output value
	BufferLength  On input points to buffer size, on output contains number of actually written bytes

Return Value

	STATUS_SUCCESS on success
	error status on error

	WARNING! Function does not return error status when value (not key) was not found!

This function should be called at IRQL = PASSIVE_LEVEL

--*/

{
	wchar_t path[1024];
	NTSTATUS Status;
	DC_QUERY_REGISTRY_USER_ARGUMENTS UserArguments = { Value, BufferLength };
	RTL_QUERY_REGISTRY_TABLE Table[2] = {0};

	PAGED_CODE();

	_snwprintf( path, sizeof(path)/2, L"\\Registry\\Machine\\Software\\NGdbg\\%s", Path );

	Table[0].QueryRoutine = &QueryRoutine;
	Table[0].Flags = 0;
	Table[0].Name = Name;
	Table[0].EntryContext = NULL;
	Table[0].DefaultType = REG_NONE;

	Status = RtlQueryRegistryValues(
		RTL_REGISTRY_ABSOLUTE,
		path,
		Table,
		(PVOID) &UserArguments,
		(PVOID) NULL );

	if( !NT_SUCCESS(Status) )
		return Status;

    return UserArguments.ReturnStatus;
}