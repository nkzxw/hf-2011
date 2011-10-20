/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		sysinfo.h
 *
 * Abstract:
 *
 *		This module defines various types used by ZwSetSystemInformation() hooking routines.
 *		ZwSetSystemInformation's SystemLoadAndCallImage and SystemLoadImage parameters can be used
 *		to load code into kernel address space.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 01-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __SYSINFO_H__
#define __SYSINFO_H__



/*
 * ZwSetSystemInformation sets information that affects the operation of the system. [NAR]
 */
											// # Query Set
typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation,					// 0	Y	N
	SystemProcessorInformation,				// 1	Y	N
	SystemPerformanceInformation,			// 2	Y	N
	SystemTimeOfDayInformation,				// 3	Y	N
	SystemNotImplemented1,					// 4	Y	N	// SystemPathInformation
	SystemProcessesAndThreadsInformation,	// 5	Y	N
	SystemCallCounts,						// 6	Y	N
	SystemConfigurationInformation,			// 7	Y	N
	SystemProcessorTimes,					// 8	Y	N
	SystemGlobalFlag,						// 9	Y	Y
	SystemNotImplemented2,					// 10	Y	N	// SystemCallTimeInformation
	SystemModuleInformation,				// 11	Y	N
	SystemLockInformation,					// 12	Y	N
	SystemNotImplemented3,					// 13	Y	N	// SystemStackTraceInformation
	SystemNotImplemented4,					// 14	Y	N	// SystemPagedPoolInformation
	SystemNotImplemented5,					// 15	Y	N	// SystemNonPagedPoolInformation
	SystemHandleInformation,				// 16	Y	N
	SystemObjectInformation,				// 17	Y	N
	SystemPagefileInformation,				// 18	Y	N
	SystemInstructionEmulationCounts,		// 19	Y	N
	SystemInvalidInfoClass1,				// 20
	SystemCacheInformation,					// 21	Y	Y
	SystemPoolTagInformation,				// 22	Y	N
	SystemProcessorStatistics,				// 23	Y	N
	SystemDpcInformation,					// 24	Y	Y
	SystemNotImplemented6,					// 25	Y	N	// SystemFullMemoryInformation
	SystemLoadImage,						// 26	N	Y	// SystemLoadGdiDriverInformation
	SystemUnloadImage,						// 27	N	Y
	SystemTimeAdjustment,					// 28	Y	Y
	SystemNotImplemented7,					// 29	Y	N	// SystemSummaryMemoryInformation
	SystemNotImplemented8,					// 30	Y	N	// SystemNextEventIdInformation
	SystemNotImplemented9,					// 31	Y	N	// SystemEventIdsInformation
	SystemCrashDumpInformation,				// 32	Y	N
	SystemExceptionInformation,				// 33	Y	N
	SystemCrashDumpStateInformation,		// 34	Y	Y/N
	SystemKernelDebuggerInformation,		// 35	Y	N
	SystemContextSwitchInformation,			// 36	Y	N
	SystemRegistryQuotaInformation,			// 37	Y	Y
	SystemLoadAndCallImage,					// 38	N	Y	// SystemExtendServiceTableInformation
	SystemPrioritySeparation,				// 39	N	Y
	SystemNotImplemented10,					// 40	Y	N	// SystemPlugPlayBusInformation
	SystemNotImplemented11,					// 41	Y	N	// SystemDockInformation
	SystemInvalidInfoClass2,				// 42			// SystemPowerInformation
	SystemInvalidInfoClass3,				// 43			// SystemProcessorSpeedInformation
	SystemTimeZoneInformation,				// 44	Y	N
	SystemLookasideInformation,				// 45	Y	N
	SystemSetTimeSlipEvent,					// 46	N	Y
	SystemCreateSession,					// 47	N	Y
	SystemDeleteSession,					// 48	N	Y
	SystemInvalidInfoClass4,				// 49
	SystemRangeStartInformation,			// 50	Y	N
	SystemVerifierInformation,				// 51	Y	Y
	SystemAddVerifier,						// 52	N	Y
	SystemSessionProcessesInformation		// 53	Y	N
} SYSTEM_INFORMATION_CLASS;


/*
 * Information Class 5
 */

typedef enum {
	StateInitialized,
	StateReady,
	StateRunning,
	StateStandby,
	StateTerminated,
	StateWait,
	StateTransition,
	StateUnknown
} THREAD_STATE;

typedef struct _SYSTEM_THREADS {
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER CreateTime;
	ULONG WaitTime;
	PVOID StartAddress;
	CLIENT_ID ClientId;
	KPRIORITY Priority;
	KPRIORITY BasePriority;
	ULONG ContextSwitchCount;
	THREAD_STATE State;
	KWAIT_REASON WaitReason;
} SYSTEM_THREADS, *PSYSTEM_THREADS;

typedef struct _SYSTEM_PROCESSES {
	ULONG NextEntryDelta;
	ULONG ThreadCount;
	ULONG Reserved1[6];
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;
	UNICODE_STRING ProcessName;
	KPRIORITY BasePriority;
	ULONG ProcessId;
	ULONG InheritedFromProcessId;
	ULONG HandleCount;
	ULONG Reserved2[2];
	VM_COUNTERS VmCounters;
	IO_COUNTERS IoCounters; // Windows 2000 only
	SYSTEM_THREADS Threads[1];
} SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;


NTSTATUS
NTAPI
HookedNtSetSystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength
	);


/*
 * ZwQuerySystemInformation queries information about the system. [NAR]
 */

NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);


typedef NTSTATUS (*fpZwSetSystemInformation)
(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength
);


/*
 * Information Class 38
 *
 * "This information class can only be set. Rather than setting any information (in a narrow
 * sense of “setting”), it performs the operation of loading a module into the kernel
 * address space and calling its entry point." [NAR]
 */

typedef struct _SYSTEM_LOAD_AND_CALL_IMAGE {

	UNICODE_STRING ModuleName;		/* The full path in the native NT format of the module to load. */

} SYSTEM_LOAD_AND_CALL_IMAGE, *PSYSTEM_LOAD_AND_CALL_IMAGE;



BOOLEAN InitSysInfoHooks();


#endif	/* __SYSINFO_H__ */
