/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_TYPES_H
#define _WEHNTRUST_DRIVER_TYPES_H

//
// The majority of these are undocumented internal structures that are needed in
// order to accomplish our goal.
//

#define SEC_FILE           0x800000
#define SEC_IMAGE         0x1000000
#define SEC_VLM           0x2000000
#define SEC_RESERVE       0x4000000
#define SEC_COMMIT        0x8000000
#define SEC_NOCACHE      0x10000000
#define MEM_DOS_LIM      0x40000000
#define MEM_IMAGE         SEC_IMAGE

#define CONTROL_AREA_FLAG_IMAGE 0x20
#define CONTROL_AREA_FLAG_BASED 0x40

//#define THREAD_SUSPEND_RESUME          0x0002
//#define THREAD_SET_CONTEXT             0x0010  
#define THREAD_QUERY_INFORMATION       0x0040

#define PROCESS_VM_OPERATION           0x0008
#define PROCESS_VM_READ                0x0010
#define PROCESS_VM_WRITE               0x0020

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

//
// Use NtCurrentProcess; redefine if not available.
//

#ifndef NtCurrentProcess
#define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)
#endif

#ifndef NtCurrentThread
#define NtCurrentThread() ((HANDLE)(LONG_PTR)-2)
#endif

typedef struct _KAPC_STATE
{
	LIST_ENTRY       ApcListHead[MaximumMode];
	struct _KPROCESS *Process;
	BOOLEAN          KernelApcInProgress;
	BOOLEAN          KernelApcPending;
	BOOLEAN          UserApcPending;
} KAPC_STATE, *PKAPC_STATE, *RESTRICTED_POINTER PRKAPC_STATE;

typedef VOID (*KAPC_USER_ROUTINE)(
		IN PVOID NormalContext,
		IN PVOID SystemArgument1,
		IN PVOID SystemArgument2);

//
// Standard exception registration, not defined here
//
typedef struct _EXCEPTION_REGISTRATION_RECORD * PEXCEPTION_REGISTRATION_RECORD; 

typedef VOID CONTROL_AREA, *PCONTROL_AREA;
typedef VOID SEGMENT, *PSEGMENT;
typedef VOID SECTION_OBJECT, *PSECTION_OBJECT;
typedef VOID MMVAD, *PMMVAD;
typedef VOID PROCESS_OBJECT, *PPROCESS_OBJECT;
typedef struct _KTHREAD THREAD_OBJECT, *PTHREAD_OBJECT;

typedef struct _THREAD_BASIC_INFORMATION
{
	NTSTATUS  ExitStatus;
	PNT_TIB   TebBaseAddress;
	CLIENT_ID ClientId;
	KAFFINITY AffinityMask;
	KPRIORITY Priority;
	KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef enum _KAPC_ENVIRONMENT
{
	OriginalApcEnvironment,
	AttachedApcEnvironment,
	CurrentApcEnvironment
} KAPC_ENVIRONMENT, *PKAPC_ENVIRONMENT;

typedef enum _MEMORY_INFORMATION_CLASS
{
	MemoryBasicInformation,
	MemoryWorkingSetList,
	MemorySectionName,
	MemoryBasicVlmInformation
} MEMORY_INFORMATION_CLASS, *PMEMORY_INFORMATION_CLASS;

//
// System info classes
//
typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemPerformanceInformation = 2,
	SystemTimeOfDayInformation   = 3,
	SystemProcessorStatistics    = 23,
	SystemLoadImage              = 26,
	SystemExceptionInformation   = 33,
	SystemLookasideInformation   = 45,
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_PERFORMANCE_INFORMATION 
{
	LARGE_INTEGER IdleTime;
	LARGE_INTEGER ReadTransferCount;
	LARGE_INTEGER WriteTransferCount;
	LARGE_INTEGER OtherTransferCount;
	ULONG         ReadOperationCount;
	ULONG         WriteOperationCount;
	ULONG         OtherOperationCount;
	ULONG         AvailablePages;
	ULONG         TotalCommittedPages;
	ULONG         TotalCommitLimit;
	ULONG         PeakCommitment;
	ULONG         PageFaults;
	ULONG         WriteCopyFaults;
	ULONG         TransitionFaults;
	ULONG         Reserved1;
	ULONG         DemandZeroFaults;
	ULONG         PagesRead;
	ULONG         PageReadIos;
	ULONG         Reserved2[2];
	ULONG         PagefilePagesWritten;
	ULONG         PagefilePageWriteIos;
	ULONG         MappedFilePagesWritten;
	ULONG         MappedFilePageWriteIos;
	ULONG         PagedPoolUsage;
	ULONG         NonPagedPoolUsage;
	ULONG         PagedPoolAllocs;
	ULONG         PagedPoolFrees;
	ULONG         NonPagedPoolAllocs;
	ULONG         NonPagedPoolFrees;
	ULONG         TotalFreeSystemPtes;
	ULONG         SystemCodePage;
	ULONG         TotalSystemDriverPages;
	ULONG         TotalSystemCodePages;
	ULONG         SmallNonPagedLookasideListAllocateHits;
	ULONG         SmallPagedLookasideListAllocateHits;
	ULONG         Reserved3;
	ULONG         MmSystemCachePage;
	ULONG         PagedPoolPage;
	ULONG         SystemDriverPage;
	ULONG         FastReadNoWait;
	ULONG         FastReadWait;
	ULONG         FastReadResourceMiss;
	ULONG         FastReadNotPossible;
	ULONG         FastMdlReadNoWait;
	ULONG         FastMdlReadWait;
	ULONG         FastMdlReadResourceMiss;
	ULONG         FastMdlReadNotPossible;
	ULONG         MapDataNoWait;
	ULONG         MapDataWait;
	ULONG         MapDataNoWaitMiss;
	ULONG         MapDataWaitMiss;
	ULONG         PinMappedDataCount;
	ULONG         PinReadNoWait;
	ULONG         PinReadWait;
	ULONG         PinReadNoWaitMiss;
	ULONG         PinReadWaitMiss;
	ULONG         CopyReadNoWait;
	ULONG         CopyReadWait;
	ULONG         CopyReadNoWaitMiss;
	ULONG         CopyReadWaitMiss;
	ULONG         MdlReadNoWait;
	ULONG         MdlReadWait;
	ULONG         MdlReadNoWaitMiss;
	ULONG         MdlReadWaitMiss;
	ULONG         ReadAheadIos;
	ULONG         LazyWriteIos;
	ULONG         LazyWritePages;
	ULONG         DataFlushes;
	ULONG         DataPages;
	ULONG         ContextSwitches;
	ULONG         FirstLevelTbFills;
	ULONG         SecondLevelTbFills;
	ULONG         SystemCalls;
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

typedef struct _SYSTEM_TIME_OF_DAY_INFORMATION 
{
	LARGE_INTEGER BootTime;
	LARGE_INTEGER CurrentTime;
	LARGE_INTEGER TimeZoneBias;
	ULONG         CurrentTimeZoneId;
} SYSTEM_TIME_OF_DAY_INFORMATION, *PSYSTEM_TIME_OF_DAY_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_STATISTICS
{
	ULONG ContextSwitches;
	ULONG DpcCount;
	ULONG DpcRequestRate;
	ULONG TimeIncrement;
	ULONG DpcBypassCount;
	ULONG ApcBypassCount;
} SYSTEM_PROCESSOR_STATISTICS, *PSYSTEM_PROCESSOR_STATISTICS;

typedef struct _SYSTEM_EXCEPTION_INFORMATION
{
	ULONG AlignmentFixupCount;
	ULONG ExceptionDispatchCount;
	ULONG FloatingEmulationCount;
	ULONG Reserved;
} SYSTEM_EXCEPTION_INFORMATION, *PSYSTEM_EXCEPTION_INFORMATION;

typedef struct _SYSTEM_LOOKASIDE_INFORMATION 
{ 
	USHORT    Depth;
	USHORT    MaximumDepth;
	ULONG     TotalAllocates;
	ULONG     AllocateMisses;
	ULONG     TotalFrees;
	ULONG     FreeMisses;
	POOL_TYPE Type;
	ULONG     Tag;
	ULONG     Size;
} SYSTEM_LOOKASIDE_INFORMATION, *PSYSTEM_LOOKASIDE_INFORMATION;

typedef struct _SYSTEM_LOAD_IMAGE 
{
   UNICODE_STRING ModuleName;
   PVOID          ModuleBase;
   PVOID          Unknown;
   PVOID          EntryPoint;
   PVOID          ExportDirectory;
} SYSTEM_LOAD_IMAGE, *PSYSTEM_LOAD_IMAGE;

typedef struct _FILE_DIRECTORY_INFORMATION 
{
	ULONG         NextEntryOffset;
	ULONG         Unknown;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG         FileAttributes;
	ULONG         FileNameLength;
	WCHAR         FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _USER_STACK
{
	PVOID FixedStackBase;
	PVOID FixedStackLimit;
	PVOID ExpandableStackBase;
	PVOID ExpandableStackLimit;
	PVOID ExpandableStackBottom;
} USER_STACK, *PUSER_STACK;

//
// Standard trap frame.
// We'll just manipulate this directly because KeKframesToContext isn't exported.
//

//
// 6001 DDK exposes this structure.  If you're compiling with an older DDK, 
// you may need to uncomment this.
//
#if 0
typedef struct _KTRAP_FRAME {
	ULONG							DbgEbp;
	ULONG							DbgEip;
	ULONG							DbgArgMark;
	ULONG							DbgArgPointer;
	ULONG							TempSegCs;
	ULONG							TempEsp;
	ULONG							Dr0;
	ULONG							Dr1;
	ULONG							Dr2;
	ULONG							Dr3;
	ULONG							Dr6;
	ULONG							Dr7;
	ULONG							SegGs;
	ULONG							SegEs;
	ULONG							SegDs;
	ULONG							Edx;
	ULONG							Ecx;
	ULONG							Eax;
	ULONG							PreviousPreviousMode; // MODE, but as a ULONG to ensure we have proper alignment (32-bit)
	PEXCEPTION_REGISTRATION_RECORD	ExceptionList;
	ULONG							SegFs;
	ULONG							Edi;
	ULONG							Esi;
	ULONG							Ebx;
	ULONG							Ebp;
	ULONG							ErrCode;
	ULONG							Eip;
	ULONG							SegCs;
	ULONG							EFlags;
	ULONG							HardwareEsp;
	ULONG							HardwareSegSs;
	ULONG							V86Es;
	ULONG							V86Ds;
	ULONG							V86Fs;
	ULONG							V86Gs;
} KTRAP_FRAME, * PKTRAP_FRAME;
#endif

//
// Unprototyped imports
//

extern NTSYSAPI NTSTATUS NTAPI ZwCreateSection(
		IN PHANDLE SectionHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES Attributes OPTIONAL,
		IN PLARGE_INTEGER MaximumSize OPTIONAL,
		IN ULONG SectionPageProtection,
		IN ULONG AllocationType,
		IN HANDLE FileHandle OPTIONAL);
extern NTSYSAPI NTSTATUS NTAPI MmMapViewOfSection(
		IN PSECTION_OBJECT SectionObject,
		IN PPROCESS_OBJECT ProcessObject,
		IN OUT PVOID *BaseAddress,
		IN ULONG ZeroBits,
		IN ULONG CommitSize,
		IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
		IN OUT PSIZE_T ViewSize,
		IN SECTION_INHERIT InheritDisposition,
		IN ULONG AllocationType,
		IN ULONG Protect);
extern NTSYSAPI NTSTATUS NTAPI MmUnmapViewOfSection(
		IN PPROCESS_OBJECT ProcessObject,
		IN PVOID BaseAddress);
extern NTSYSAPI NTSTATUS NTAPI ZwQueryDirectoryFile(
		IN HANDLE FileHandle,
		IN HANDLE Event OPTIONAL,
		IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
		IN PVOID ApcContext OPTIONAL,
		OUT PIO_STATUS_BLOCK IoStatusBlock,
		OUT PVOID FileInformation,
		IN ULONG FileInformationLength,
		IN FILE_INFORMATION_CLASS FileInformationClass,
		IN BOOLEAN ReturnSingleEntry,
		IN PUNICODE_STRING FileName OPTIONAL,
		IN BOOLEAN RestartScan);
extern NTSYSAPI NTSTATUS NTAPI ZwWaitForSingleObject(
		IN HANDLE Handle,
		IN BOOLEAN Alertable,
		IN PLARGE_INTEGER Timeout OPTIONAL);
extern NTSYSAPI NTSTATUS NTAPI ZwDeleteFile(
		IN POBJECT_ATTRIBUTES ObjectAttributes);
extern NTSYSAPI NTSTATUS NTAPI ZwAllocateVirtualMemory(
		IN HANDLE ProcessHandle,
		IN OUT PVOID *BaseAddress,
		IN ULONG ZeroBits,
		IN OUT PULONG RegionSize,
		IN ULONG AllocationType,
		IN ULONG Protect);
extern NTSYSAPI NTSTATUS NTAPI ZwFreeVirtualMemory(
		IN HANDLE ProcessHandle,
		IN OUT PVOID *BaseAddress,
		IN OUT PULONG RegionSize,
		IN ULONG FreeType);
extern NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation(
		IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
		IN OUT PVOID SystemInformation,
		IN ULONG SystemInformationLength,
		OUT PULONG ReturnLength OPTIONAL);
extern NTSYSAPI NTSTATUS NTAPI ZwSetSystemInformation(
		IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
		IN OUT PVOID SystemInformation,
		IN ULONG SystemInformationLength);
extern NTSYSAPI NTSTATUS NTAPI IoQueryFileInformation(
		IN PFILE_OBJECT FileObject,
		IN FILE_INFORMATION_CLASS FileInformationClass,
		IN ULONG Length,
		IN PVOID FileInformation,
		OUT PULONG LengthNeeded);
extern VOID KeStackAttachProcess(
		IN PVOID Process, 
		OUT PRKAPC_STATE ApcState);
extern VOID KeUnstackDetachProcess(
		IN PRKAPC_STATE ApcState);
extern NTSYSAPI KPROCESSOR_MODE NTAPI KeGetPreviousMode(
		VOID);
extern NTSYSAPI VOID NTAPI KeSetEventBoostPriority(
		IN PKEVENT Event,
		IN ULONG Priority);
extern NTSYSAPI NTSTATUS NTAPI ObOpenObjectByPointer(
		IN PVOID Object,
		IN ULONG HandleAttributes,
		IN PACCESS_STATE PassedAccessState OPTIONAL,
		IN ACCESS_MASK DesiredAccess OPTIONAL,
		IN POBJECT_TYPE ObjectType OPTIONAL,
		IN KPROCESSOR_MODE AccessMode,
		OUT PHANDLE Handle);
extern NTSYSAPI NTSTATUS NTAPI ObQueryNameString(
		IN PVOID Object,
		IN POBJECT_NAME_INFORMATION Information,
		IN ULONG MaximumLength,
		OUT PULONG ActualLength);
extern LONG _snwprintf(
		OUT PWSTR Buffer, 
		IN ULONG BufferSize, 
		IN PWSTR FormatString, 
		...);
extern PACCESS_TOKEN PsReferencePrimaryToken(
		IN PEPROCESS  Process); 
extern NTSYSAPI NTSTATUS NTAPI PsLookupProcessByProcessId(
		IN HANDLE ProcessId,
		OUT PPROCESS_OBJECT *ProcessObject);
extern NTSYSAPI PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader(
		IN PVOID Base);

extern BOOLEAN SeTokenIsAdmin(
		IN PACCESS_TOKEN  Token); 


extern NTSYSAPI VOID NTAPI KeInitializeApc(
		IN PRKAPC Apc,
		IN PKTHREAD Thread,
		IN KAPC_ENVIRONMENT Environment,
		IN PKKERNEL_ROUTINE KernelRoutine,
		IN PKRUNDOWN_ROUTINE RundownRoutine OPTIONAL,
		IN PKNORMAL_ROUTINE NormalRoutine OPTIONAL,
		IN KPROCESSOR_MODE ApcMode,
		IN PVOID NormalContext);
extern NTSYSAPI BOOLEAN NTAPI KeInsertQueueApc(
		IN PKAPC Apc,
		IN PVOID SystemArgument1,
		IN PVOID SystemArgument2,
		IN KAFFINITY Affinity);

//
// Global variables
//
extern PULONG InitSafeBootMode;
//extern POBJECT_TYPE PsProcessType;
//extern POBJECT_TYPE PsThreadType;

#endif
