#ifndef _CONFIG_H_
#define _CONFIG_H_

NTSTATUS
QueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

struct DC_QUERY_REGISTRY_USER_ARGUMENTS {
	PVOID Buffer;
	PULONG BufferLength;
	NTSTATUS ReturnStatus;
};

extern "C" 
NTSTATUS
QueryRegistrySetting(
	IN PWSTR Path,
	IN PWSTR Name,
	IN ULONG Type,
	OUT PVOID Value,
	IN OUT PULONG BufferLength
	);

#endif
