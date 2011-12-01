//APCFunc.h
#include <ntddk.h>

//=======================================================================================//
// 这里声明要用到的APC结构																//
//======================================================================================//
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

//=======================================================================================//
// 这里声明WDK中一些undocumented的API，编译时与lib文件链接								//
//======================================================================================//
extern "C" VOID
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

extern "C" BOOLEAN
KeInsertQueueApc (
				  PKAPC Apc,
				  PVOID SystemArgument1,
				  PVOID SystemArgument2,
				  KPRIORITY Increment
				  );

extern "C" NTSTATUS
PsLookupProcessByProcessId(
						   __in HANDLE ProcessId,
						   __deref_out PEPROCESS *Process
						   );

extern "C" UCHAR *
PsGetProcessImageFileName(
						  __in PEPROCESS Process
						  );
extern "C" VOID
KeStackAttachProcess (
					  IN PVOID Process,
					  OUT PRKAPC_STATE ApcState
					  );
extern "C" VOID
KeUnstackDetachProcess(
					   IN PRKAPC_STATE ApcState
					   );

//=======================================================================================//
// 这里是自定义函数																		//
//======================================================================================//
void 
ApcKernelRoutine(
				 IN struct _KAPC *Apc,
				 IN OUT PKNORMAL_ROUTINE *NormalRoutine, 
				 IN OUT PVOID *NormalContext, 
				 IN OUT PVOID *SystemArgument1, 
				 IN OUT PVOID *SystemArgument2
				 ); 

NTSTATUS 
InstallUserModeApc(
				   const char* DllFullPath, 
				   ULONG pTargetThread, 
				   ULONG pTargetProcess
				   );
void 
ApcUserCode(
			PVOID NormalContext, 
			PVOID  SystemArgument1, 
			PVOID SystemArgument2
			);
void 
ApcUserCodeEnd();