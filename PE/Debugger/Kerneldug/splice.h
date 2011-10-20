#ifndef _SPLICE_H_
#define _SPLICE_H_

extern "C"
{
#include <ntddk.h>
}

#include "ldasm.h"

ULONG
SpliceFunctionStart(
	IN	PVOID	OriginalAddress,
    IN	PVOID	HookFunction,
	OUT	PVOID	SplicingBuffer,
	IN	ULONG	MaxLength,
	OUT	PVOID	BackupBuffer,
	OUT	PULONG	BytesWritten,
	IN	BOOLEAN	WorkAtCurrentIrql
	);

ULONG
UnspliceFunctionStart(
	IN	PVOID	OriginalAddress,
	IN	PVOID	BackupBuffer,
	IN	BOOLEAN	WorkAtCurrentIrql
	);

#endif
