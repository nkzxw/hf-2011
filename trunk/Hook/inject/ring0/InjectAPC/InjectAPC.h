#include <ntddk.h>

#define MAX_PID 65535
/////////////////////////////////////////////////////////////////////////////////////////////////////////
NTSTATUS 
InstallUserModeApc(LPSTR DllFullPath, ULONG pTargetThread, ULONG pTargetProcess);
void ApcCreateProcessEnd();
void ApcCreateProcess(PVOID NormalContext, PVOID SystemArgument1, PVOID SystemArgument2);

typedef enum
{
		OriginalApcEnvironment,
		AttachedApcEnvironment,
		CurrentApcEnvironment
} KAPC_ENVIRONMENT;

typedef struct _KAPC_STATE {
		LIST_ENTRY ApcListHead[MaximumMode];
		struct _KPROCESS *Process;
		BOOLEAN KernelApcInProgress;
		BOOLEAN KernelApcPending;
		BOOLEAN UserApcPending;
} KAPC_STATE, *PKAPC_STATE, *PRKAPC_STATE;


VOID
KeInitializeApc (
	PKAPC Apc,
	PETHREAD Thread,
	KAPC_ENVIRONMENT Environment,
	PKKERNEL_ROUTINE KernelRoutine,
	PKRUNDOWN_ROUTINE RundownRoutine,
	PKNORMAL_ROUTINE NormalRoutine,
	KPROCESSOR_MODE ProcessorMode,
	PVOID NormalContext
	);

BOOLEAN
KeInsertQueueApc (
	PKAPC Apc,
	PVOID SystemArgument1,
	PVOID SystemArgument2,
	KPRIORITY Increment
	);

NTSTATUS
PsLookupProcessByProcessId(
    __in HANDLE ProcessId,
    __deref_out PEPROCESS *Process
    );

UCHAR *
PsGetProcessImageFileName(
    __in PEPROCESS Process
    );
VOID
KeStackAttachProcess (
    IN PVOID Process,
    OUT PRKAPC_STATE ApcState
    );
VOID
KeUnstackDetachProcess(
    IN PRKAPC_STATE ApcState
    );
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

