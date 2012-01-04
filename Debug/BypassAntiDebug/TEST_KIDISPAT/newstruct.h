#ifndef _____abc_____
#define _____abc_____


#include "ntos.h"
NTSYSAPI 
PIMAGE_NT_HEADERS
NTAPI
RtlImageNtHeader(
				 
				 
				 IN PVOID                ModuleAddress );
NTKERNELAPI
NTSTATUS
ObCreateObject (
				IN KPROCESSOR_MODE ObjectAttributesAccessMode OPTIONAL,
				IN POBJECT_TYPE ObjectType,
				IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
				IN KPROCESSOR_MODE AccessMode,
				IN PVOID Reserved,
				IN ULONG ObjectSizeToAllocate,
				IN ULONG PagedPoolCharge OPTIONAL,
				IN ULONG NonPagedPoolCharge OPTIONAL,
				OUT PVOID *Object
    );
NTSTATUS ObCloseHandle(
					   __in  HANDLE Handle,
					   __in  KPROCESSOR_MODE PreviousMode
);
NTSYSAPI NTSTATUS NTAPI ZwFlushInstructionCache	(	IN HANDLE 	ProcessHandle,
												 IN PVOID 	BaseAddress,
												 IN ULONG 	NumberOfBytesToFlush	 
);

#define KeGetPreviousMode() (KeGetCurrentThread()->PreviousMode)
#define LPC_MAX_CONNECTION_INFO_SIZE (16 * sizeof(ULONG_PTR))

#define PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH ((PORT_MAXIMUM_MESSAGE_LENGTH + sizeof(PORT_MESSAGE) + LPC_MAX_CONNECTION_INFO_SIZE + 0xf) & ~0xf)



typedef struct _LPC_MESSAGE {
	USHORT  DataSize;
	USHORT  MessageSize;
	USHORT  MessageType;
	USHORT  VirtualRangesOffset;
	CLIENT_ID  ClientId;
	ULONG  MessageId;
	ULONG  SectionSize;
	UCHAR  Data[ANYSIZE_ARRAY];
} LPC_MESSAGE, *PLPC_MESSAGE;

#define LPC_MESSAGE_BASE_SIZE	24

enum LPC_TYPE {
	LPC_NEW_MESSAGE,
		LPC_REQUEST,
		LPC_REPLY,
		LPC_DATAGRAM,
		LPC_LOST_REPLY,
		LPC_PORT_CLOSED,
		LPC_CLIENT_DIED,
		LPC_EXCEPTION,
		LPC_DEBUG_EVENT,
		LPC_ERROR_EVENT,
		LPC_CONNECTION_REQUEST,
		LPC_CONNECTION_REFUSED,
		LPC_MAXIMUM
};
typedef enum _PS_QUOTA_TYPE {
	PsNonPagedPool = 0,
		PsPagedPool    = 1,
		PsPageFile     = 2,
		PsQuotaTypes   = 3
} PS_QUOTA_TYPE, *PPS_QUOTA_TYPE;




//#define PS_TRACK_QUOTA 1

#define EPROCESS_QUOTA_TRACK_MAX 10000

typedef struct _EPROCESS_QUOTA_TRACK {
    SIZE_T Charge;
    PVOID Caller;
    PVOID FreeCaller;
    PVOID Process;
} EPROCESS_QUOTA_TRACK, *PEPROCESS_QUOTA_TRACK;



 typedef struct _KGDTENTRY
 {
 USHORT LimitLow;
 USHORT BaseLow;
 struct
 {
 UCHAR BaseMid;
 UCHAR Flags1; // bit0-4 - Type
 // bit5-6 - Dpl
 // bit7 - Pres
 UCHAR Flags2; // bit0-3 - LimitHi
 // bit4 - Sys
 // bit5 - Reserved_0
 // bit6 - Default_Big
 // bit7 - Granularity
 UCHAR BaseHi;
 } HighWord;
 } KGDTENTRY, *PKGDTENTRY;
 
 typedef struct _KIDTENTRY
 {
 USHORT Offset;
 USHORT Selector;
 USHORT Access;
 USHORT ExtendedOffset;
 } KIDTENTRY, *PKIDTENTRY;
 
typedef struct _KEXECUTE_OPTIONS {
    UCHAR ExecuteDisable : 1;
    UCHAR ExecuteEnable : 1;
    UCHAR DisableThunkEmulation : 1;
    UCHAR Permanent : 1;
    UCHAR ExecuteDispatchEnable : 1;
    UCHAR ImageDispatchEnable : 1;
    UCHAR Spare : 2;
} KEXECUTE_OPTIONS, PKEXECUTE_OPTIONS;

 
 typedef struct _EXCEPTION_REGISTRATION_RECORD
 {
	 struct _EXCEPTION_REGISTRATION_RECORD *Next;
	 PVOID Handler;
 } EXCEPTION_REGISTRATION_RECORD, *PEXCEPTION_REGISTRATION_RECORD;
 
typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SpareUl2;
    ULONG SpareUl3;
    ULONG PeakVirtualSize;
    ULONG VirtualSize;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    ULONG QuotaPeakPagedPoolUsage;
    ULONG QuotaPagedPoolUsage;
    ULONG QuotaPeakNonPagedPoolUsage;
    ULONG QuotaNonPagedPoolUsage;
    ULONG PagefileUsage;
    ULONG PeakPagefileUsage;
    ULONG PrivatePageCount;
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

 typedef struct _KTHREAD
{
	 DISPATCHER_HEADER Header;
	 LIST_ENTRY MutantListHead;
	 PVOID InitialStack;
	 PVOID StackLimit;
	 PVOID Teb;
	 PVOID TlsArray;
	 PVOID KernelStack;
	 UCHAR DebugActive;
	 UCHAR State;
	 UCHAR Alerted[2];
	 UCHAR Iopl;
	 UCHAR NpxState;
	 CHAR Saturation;
	 CHAR Priority;
	 KAPC_STATE ApcState;
	 UINT32 ContextSwitches;
	 UCHAR IdleSwapBlock;
	 UCHAR Spare0[3];
	 INT32 WaitStatus;
	 UCHAR WaitIrql;
	 CHAR WaitMode;
	 UCHAR WaitNext;
	 UCHAR WaitReason;
	 PKWAIT_BLOCK WaitBlockList;
	 union
	 {
	 LIST_ENTRY WaitListEntry;
	 SINGLE_LIST_ENTRY SwapListEntry;
	 };
	 UINT32 WaitTime;
	 CHAR BasePriority;
	 UCHAR DecrementCount;
	 CHAR PriorityDecrement;
	 CHAR Quantum;
	 KWAIT_BLOCK WaitBlock[4];
	 PVOID LegoData;
	 UINT32 KernelApcDisable;
	 UINT32 UserAffinity;
	 UCHAR SystemAffinityActive;
	 UCHAR PowerState;
	 UCHAR NpxIrql;
	 UCHAR InitialNode;
	 PVOID ServiceTable;
	 PKQUEUE Queue;
	 UINT32 ApcQueueLock;
	 KTIMER Timer;
	 LIST_ENTRY QueueListEntry;
	 UINT32 SoftAffinity;
	 UINT32 Affinity;
	 UCHAR Preempted;
	 UCHAR ProcessReadyQueue;
	 UCHAR KernelStackResident;
	 UCHAR NextProcessor;
	 PVOID CallbackStack;
	 PVOID Win32Thread;
	 PKTRAP_FRAME TrapFrame;
	 PKAPC_STATE ApcStatePointer[2];
	 CHAR PreviousMode;
	 UCHAR EnableStackSwap;
	 UCHAR LargeStack;
	 UCHAR ResourceIndex;
	 UINT32 KernelTime;
	 UINT32 UserTime;
	 KAPC_STATE SavedApcState;
	 UCHAR Alertable;
	 UCHAR ApcStateIndex;
	 UCHAR ApcQueueable;
	 UCHAR AutoAlignment;
	 PVOID StackBase;
	 KAPC SuspendApc;
	 KSEMAPHORE SuspendSemaphore;
	 LIST_ENTRY ThreadListEntry;
	 CHAR FreezeCount;
	 CHAR SuspendCount;
	 UCHAR IdealProcessor;
	 UCHAR DisableBoost;
 } KTHREAD;
 typedef struct _TERMINATION_PORT
 {
 struct _TERMINATION_PORT *Next;
 PVOID Port;
 }TERMINATION_PORT, *PTERMINATION_PORT;
 
 typedef struct _PS_IMPERSONATION_INFORMATION
 {
 PVOID Token;
 UCHAR CopyOnOpen;
 UCHAR EffectiveOnly;
 SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
 } PS_IMPERSONATION_INFORMATION, *PPS_IMPERSONATION_INFORMATION;
 

 typedef struct _ETHREAD                                      // 54 elements, 0x258 bytes (sizeof)
 {
	 /*0x000*/     struct _KTHREAD Tcb;                                     // 73 elements, 0x1C0 bytes (sizeof)
	 union                                                    // 2 elements, 0x8 bytes (sizeof)
	 {
		 /*0x1C0*/         union _LARGE_INTEGER CreateTime;                     // 4 elements, 0x8 bytes (sizeof)
		 struct                                               // 2 elements, 0x4 bytes (sizeof)
		 {
			 /*0x1C0*/             UINT32       NestedFaultCount : 2;               // 0 BitPosition
			 /*0x1C0*/             UINT32       ApcNeeded : 1;                      // 2 BitPosition
		 };
	 };
	 union                                                    // 3 elements, 0x8 bytes (sizeof)
	 {
		 /*0x1C8*/         union _LARGE_INTEGER ExitTime;                       // 4 elements, 0x8 bytes (sizeof)
		 /*0x1C8*/         struct _LIST_ENTRY LpcReplyChain;                    // 2 elements, 0x8 bytes (sizeof)
		 /*0x1C8*/         struct _LIST_ENTRY KeyedWaitChain;                   // 2 elements, 0x8 bytes (sizeof)
	 };
	 union                                                    // 2 elements, 0x4 bytes (sizeof)
	 {
		 /*0x1D0*/         LONG32       ExitStatus;
		 /*0x1D0*/         VOID*        OfsChain;
	 };
	 /*0x1D4*/     struct _LIST_ENTRY PostBlockList;                        // 2 elements, 0x8 bytes (sizeof)
	 union                                                    // 3 elements, 0x4 bytes (sizeof)
	 {
		 /*0x1DC*/         struct _TERMINATION_PORT* TerminationPort;
		 /*0x1DC*/         struct _ETHREAD* ReaperLink;
		 /*0x1DC*/         VOID*        KeyedWaitValue;
	 };
	 /*0x1E0*/     ULONG32      ActiveTimerListLock;
	 /*0x1E4*/     struct _LIST_ENTRY ActiveTimerListHead;                  // 2 elements, 0x8 bytes (sizeof)
	 /*0x1EC*/     struct _CLIENT_ID Cid;                                   // 2 elements, 0x8 bytes (sizeof)
	 union                                                    // 2 elements, 0x14 bytes (sizeof)
	 {
		 /*0x1F4*/         struct _KSEMAPHORE LpcReplySemaphore;                // 2 elements, 0x14 bytes (sizeof)
		 /*0x1F4*/         struct _KSEMAPHORE KeyedWaitSemaphore;               // 2 elements, 0x14 bytes (sizeof)
	 };
	 union                                                    // 2 elements, 0x4 bytes (sizeof)
	 {
		 /*0x208*/         VOID*        LpcReplyMessage;
		 /*0x208*/         VOID*        LpcWaitingOnPort;
	 };
	 /*0x20C*/     struct _PS_IMPERSONATION_INFORMATION* ImpersonationInfo;
	 /*0x210*/     struct _LIST_ENTRY IrpList;                              // 2 elements, 0x8 bytes (sizeof)
	 /*0x218*/     ULONG32      TopLevelIrp;
	 /*0x21C*/     struct _DEVICE_OBJECT* DeviceToVerify;
	 /*0x220*/     struct _EPROCESS* ThreadsProcess;
	 /*0x224*/     VOID*        StartAddress;
	 union                                                    // 2 elements, 0x4 bytes (sizeof)
	 {
		 /*0x228*/         VOID*        Win32StartAddress;
		 /*0x228*/         ULONG32      LpcReceivedMessageId;
	 };
	 /*0x22C*/     struct _LIST_ENTRY ThreadListEntry;                      // 2 elements, 0x8 bytes (sizeof)
	 /*0x234*/     struct _EX_RUNDOWN_REF RundownProtect;                   // 2 elements, 0x4 bytes (sizeof)
	 /*0x238*/     EX_PUSH_LOCK ThreadLock;                         // 5 elements, 0x4 bytes (sizeof)
	 /*0x23C*/     ULONG32      LpcReplyMessageId;
	 /*0x240*/     ULONG32      ReadClusterSize;
	 /*0x244*/     ULONG32      GrantedAccess;
	 union                                                    // 2 elements, 0x4 bytes (sizeof)
	 {
		 /*0x248*/         ULONG32      CrossThreadFlags;
		 struct                                               // 9 elements, 0x4 bytes (sizeof)
		 {
			 /*0x248*/             ULONG32      Terminated : 1;                     // 0 BitPosition
			 /*0x248*/             ULONG32      DeadThread : 1;                     // 1 BitPosition
			 /*0x248*/             ULONG32      HideFromDebugger : 1;               // 2 BitPosition
			 /*0x248*/             ULONG32      ActiveImpersonationInfo : 1;        // 3 BitPosition
			 /*0x248*/             ULONG32      SystemThread : 1;                   // 4 BitPosition
			 /*0x248*/             ULONG32      HardErrorsAreDisabled : 1;          // 5 BitPosition
			 /*0x248*/             ULONG32      BreakOnTermination : 1;             // 6 BitPosition
			 /*0x248*/             ULONG32      SkipCreationMsg : 1;                // 7 BitPosition
			 /*0x248*/             ULONG32      SkipTerminationMsg : 1;             // 8 BitPosition
		 };
	 };
	 union                                                    // 2 elements, 0x4 bytes (sizeof)
	 {
		 /*0x24C*/         ULONG32      SameThreadPassiveFlags;
		 struct                                               // 3 elements, 0x4 bytes (sizeof)
		 {
			 /*0x24C*/             ULONG32      ActiveExWorker : 1;                 // 0 BitPosition
			 /*0x24C*/             ULONG32      ExWorkerCanWaitUser : 1;            // 1 BitPosition
			 /*0x24C*/             ULONG32      MemoryMaker : 1;                    // 2 BitPosition
		 };
	 };
	 union                                                    // 2 elements, 0x4 bytes (sizeof)
	 {
		 /*0x250*/         ULONG32      SameThreadApcFlags;
		 struct                                               // 3 elements, 0x1 bytes (sizeof)
                  {
/*0x250*/             UINT8        LpcReceivedMsgIdValid : 1;          // 0 BitPosition
/*0x250*/             UINT8        LpcExitThreadCalled : 1;            // 1 BitPosition
/*0x250*/             UINT8        AddressSpaceOwner : 1;              // 2 BitPosition
                  };
              };
/*0x254*/     UINT8        ForwardClusterOnly;
/*0x255*/     UINT8        DisablePageFaultClustering;
/*0x256*/     UINT8        _PADDING0_[0x2];
          }ETHREAD2, *PETHREAD2;
 typedef struct _KTHREAD* PKTHREAD;

 typedef struct _PS_JOB_TOKEN_FILTER
 {
 UINT32 CapturedSidCount;
 PSID_AND_ATTRIBUTES CapturedSids;
 UINT32 CapturedSidsLength;
 UINT32 CapturedGroupCount;
 PSID_AND_ATTRIBUTES CapturedGroups;
 UINT32 CapturedGroupsLength;
 UINT32 CapturedPrivilegeCount;
 PLUID_AND_ATTRIBUTES CapturedPrivileges;
 UINT32 CapturedPrivilegesLength;
 } PS_JOB_TOKEN_FILTER, *PPS_JOB_TOKEN_FILTER;
 
 typedef struct _EJOB
 {
 KEVENT Event;
 LIST_ENTRY JobLinks;
 LIST_ENTRY ProcessListHead;
 ERESOURCE JobLock;
 LARGE_INTEGER TotalUserTime;
 LARGE_INTEGER TotalKernelTime;
 LARGE_INTEGER ThisPeriodTotalUserTime;
 LARGE_INTEGER ThisPeriodTotalKernelTime;
 UINT32 TotalPageFaultCount;
 UINT32 TotalProcesses;
 UINT32 ActiveProcesses;
 UINT32 TotalTerminatedProcesses;
 LARGE_INTEGER PerProcessUserTimeLimit;
 LARGE_INTEGER PerJobUserTimeLimit;
 UINT32 LimitFlags;
 UINT32 MinimumWorkingSetSize;
 UINT32 MaximumWorkingSetSize;
 UINT32 ActiveProcessLimit;
 UINT32 Affinity;
 UCHAR PriorityClass;
 UINT32 UIRestrictionsClass;
 UINT32 SecurityLimitFlags;
 PVOID Token;
 PPS_JOB_TOKEN_FILTER Filter;
 UINT32 EndOfJobTimeAction;
 PVOID CompletionPort;
 PVOID CompletionKey;
 UINT32 SessionId;
 UINT32 SchedulingClass;
 UINT64 ReadOperationCount;
 UINT64 WriteOperationCount;
 UINT64 OtherOperationCount;
 UINT64 ReadTransferCount;
 UINT64 WriteTransferCount;
 UINT64 OtherTransferCount;
 IO_COUNTERS IoInfo;
 UINT32 ProcessMemoryLimit;
 UINT32 JobMemoryLimit;
 UINT32 PeakProcessMemoryUsed;
 UINT32 PeakJobMemoryUsed;
 UINT32 CurrentJobMemoryUsed;
 FAST_MUTEX MemoryLimitsLock;
 LIST_ENTRY JobSetLinks;
 UINT32 MemberLevel;
 UINT32 JobFlags;
 } EJOB, *PEJOB;
 
 typedef struct _EPROCESS_QUOTA_ENTRY
 {
 UINT32 Usage;
 UINT32 Limit;
 UINT32 Peak;
 UINT32 Return;
 } EPROCESS_QUOTA_ENTRY, *PEPROCESS_QUOTA_ENTRY;
 
 typedef struct _EPROCESS_QUOTA_BLOCK
 {
 EPROCESS_QUOTA_ENTRY QuotaEntry;
 LIST_ENTRY QuotaList;
 UINT32 ReferenceCount;
 UINT32 ProcessCount;
 } EPROCESS_QUOTA_BLOCK, *PEPROCESS_QUOTA_BLOCK;
 
 typedef struct _PAGEFAULT_HISTORY
 {
 UINT32 CurrentIndex;
 UINT32 MaxIndex;
 UINT32 SpinLock;
 PVOID Reserved;
 PROCESS_WS_WATCH_INFORMATION WatchInfo[1];
 } PAGEFAULT_HISTORY, *PPAGEFAULT_HISTORY;
 
 typedef struct _EX_FAST_REF
 {
	 union{
	 PVOID Object;
	 UINT32 Value; //bit0-2 - RefCnt
	 };

 } EX_FAST_REF, *PEX_FAST_REF;
 
 typedef struct _SE_AUDIT_PROCESS_CREATION_INFO
 {
 POBJECT_NAME_INFORMATION ImageFileName;
 } SE_AUDIT_PROCESS_CREATION_INFO;
 
 typedef struct _MMSUPPORT_FLAGS
 {
 UINT32 Value; // bit0 - SessionSpace
 // bit1 - BeingTrimmed
 // bit2 - SessionLeader
 // bit3 - TrimHard
 // bit4 - WorkingSetHard
 // bit5 - AddressSpaceBeingDeleted
 // bit6-15 - Available
 // bit16-23 - AllowWorkingSetAdjustment
 // bit24-31 - MemoryPriority
 } MMSUPPORT_FLAGS;


 typedef struct _MMWSLE_HASH
 {
 PVOID Key;
 UINT32 Index;
 } MMWSLE_HASH, *PMMWSLE_HASH;
 
 typedef struct _MMWSL
 {
 UINT32 Quota;
 UINT32 FirstFree;
 UINT32 FirstDynamic;
 UINT32 LastEntry;
 UINT32 NextSlot;
 struct _MMWSLE* Wsle;
 UINT32 LastInitializedWsle;
 UINT32 NonDirectCount;
 PMMWSLE_HASH HashTable;
 UINT32 HashTableSize;
 UINT32 NumberOfCommittedPageTables;
 PVOID HashTableStart;
 PVOID HighestPermittedHashAddress;
 UINT32 NumberOfImageWaiters;
 UINT32 VadBitMapHint;
 union
 {
 USHORT UsedPageTableEntries[1536];
 UINT32 CommittedPageTables[48];
 };
 } MMWSL, *PMMWSL;
 typedef struct _MMWSLE* PMMWSLE;

 typedef struct _MMSUPPORT
 {
 LARGE_INTEGER LastTrimTime;
 MMSUPPORT_FLAGS Flags;
 UINT32 PageFaultCount;
 UINT32 PeakWorkingSetSize;
 UINT32 WorkingSetSize;
 UINT32 MinimumWorkingSetSize;
 UINT32 MaximumWorkingSetSize;
 PMMWSL VmWorkingSetList;
 LIST_ENTRY WorkingSetExpansionLinks;
 UINT32 Claim;
 UINT32 NextEstimationSlot;
 UINT32 NextAgingSlot;
 UINT32 EstimatedAvailable;
 UINT32 GrowthSinceLastEstimate;
 } MMSUPPORT;
 
 typedef struct _HANDLE_TRACE_DB_ENTRY
 {
 CLIENT_ID ClientId;
 PVOID Handle;
 UINT32 Type;
 PVOID StackTrace;
 } HANDLE_TRACE_DB_ENTRY, *PHANDLE_TRACE_DB_ENTRY;
 
 typedef struct _HANDLE_TRACE_DEBUG_INFO
 {
 UINT32 CurrentStackIndex;
 HANDLE_TRACE_DB_ENTRY TraceDb[4096];
 } HANDLE_TRACE_DEBUG_INFO, *PHANDLE_TRACE_DEBUG_INFO;
 
 typedef struct _HANDLE_TABLE_ENTRY
 {
 union
 {
 PVOID Object;
 UINT32 ObAttributes;
 struct _HANDLE_TABLE_ENTRY *InfoTable;
 UINT32 Value;
 };
 union
 {
 UINT32 GrantedAccess;
 struct
 {
 USHORT GrantedAccessIndex;
 USHORT CreatorBackTraceIndex;
 };
 INT32 NextFreeTableEntry;
 };
 } HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;
 
 typedef struct _HANDLE_TABLE
 {
 UINT32 TableCode;
 PEPROCESS QuotaProcess;
 PVOID UniqueProcessId;
 EX_PUSH_LOCK HandleTableLock[4];
 LIST_ENTRY HandleTableList;
 EX_PUSH_LOCK HandleContentionEvent;
 PHANDLE_TRACE_DEBUG_INFO DebugInfo;
 UINT32 FirstFree;
 UINT32 LastFree;
 UINT32 NextHandleNeedingPool;
 INT32 HandleCount;
 UINT32 Flags; // bit0 - StrictFIFO
 } HANDLE_TABLE, *PHANDLE_TABLE;
 
 
 typedef struct _KPROCESS                     // 29 elements, 0x6C bytes (sizeof) 
 {                                                                                
	 /*0x000*/     struct _DISPATCHER_HEADER Header;        // 6 elements, 0x10 bytes (sizeof)  
	 /*0x010*/     struct _LIST_ENTRY ProfileListHead;      // 2 elements, 0x8 bytes (sizeof)   
	 /*0x018*/     ULONG32      DirectoryTableBase[2];                                          
	 /*0x020*/     struct _LIST_ENTRY LdtDescriptor;         // 3 elements, 0x8 bytes (sizeof)   
	 /*0x028*/     struct _LIST_ENTRY Int21Descriptor;       // 4 elements, 0x8 bytes (sizeof)   
	 /*0x030*/     UINT16       IopmOffset;                                                     
	 /*0x032*/     UINT8        Iopl;                                                           
	 /*0x033*/     UINT8        Unused;                                                         
	 /*0x034*/     ULONG32      ActiveProcessors;                                               
	 /*0x038*/     ULONG32      KernelTime;                                                     
	 /*0x03C*/     ULONG32      UserTime;                                                       
	 /*0x040*/     struct _LIST_ENTRY ReadyListHead;        // 2 elements, 0x8 bytes (sizeof)   
	 /*0x048*/     struct _SINGLE_LIST_ENTRY SwapListEntry; // 1 elements, 0x4 bytes (sizeof)   
	 /*0x04C*/     VOID*        VdmTrapcHandler;                                                
	 /*0x050*/     struct _LIST_ENTRY ThreadListHead;       // 2 elements, 0x8 bytes (sizeof)   
	 /*0x058*/     ULONG32      ProcessLock;                                                    
	 /*0x05C*/     ULONG32      Affinity;                                                       
	 /*0x060*/     UINT16       StackCount;                                                     
	 /*0x062*/     CHAR         BasePriority;                                                   
	 /*0x063*/     CHAR         ThreadQuantum;                                                  
	 /*0x064*/     UINT8        AutoAlignment;                                                  
	 /*0x065*/     UINT8        State;                                                          
	 /*0x066*/     UINT8        ThreadSeed;                                                     
	 /*0x067*/     UINT8        DisableBoost;                                                   
	 /*0x068*/     UINT8        PowerState;                                                     
	 /*0x069*/     UINT8        DisableQuantum;                                                 
	 /*0x06A*/     UINT8        IdealNode;                                                      
	 union                                    // 2 elements, 0x1 bytes (sizeof)   
	 {                                                                            
		 	/*0x06B*/         struct _KEXECUTE_OPTIONS Flags;      // 7 elements, 0x1 bytes (sizeof)   
		 /*0x06B*/         UINT8        ExecuteOptions;                                             
	 };                                                                           
 }KPROCESS, *PKPROCESS; 


typedef struct _MMADDRESS_NODE {
    union {
        LONG_PTR Balance : 2;
        struct _MMADDRESS_NODE *Parent;
    } u1;
    struct _MMADDRESS_NODE *LeftChild;
    struct _MMADDRESS_NODE *RightChild;
    ULONG_PTR StartingVpn;
    ULONG_PTR EndingVpn;
} MMADDRESS_NODE, *PMMADDRESS_NODE;
typedef struct _MM_AVL_TABLE {
    MMADDRESS_NODE  BalancedRoot;
    ULONG_PTR DepthOfTree: 5;
    ULONG_PTR Unused: 3;
#if defined (_WIN64)
    ULONG_PTR NumberGenericTableElements: 56;
#else
    ULONG_PTR NumberGenericTableElements: 24;
#endif
    PVOID NodeHint;
    PVOID NodeFreeHint;
} MM_AVL_TABLE, *PMM_AVL_TABLE;


typedef struct _HARDWARE_PTE_X86 {
    ULONG Valid : 1;
    ULONG Write : 1;
    ULONG Owner : 1;
    ULONG WriteThrough : 1;
    ULONG CacheDisable : 1;
    ULONG Accessed : 1;
    ULONG Dirty : 1;
    ULONG LargePage : 1;
    ULONG Global : 1;
    ULONG CopyOnWrite : 1; // software field
    ULONG Prototype : 1;   // software field
    ULONG reserved : 1;  // software field
    ULONG PageFrameNumber : 20;
} HARDWARE_PTE_X86, *PHARDWARE_PTE_X86;

typedef struct _HARDWARE_PTE_X86PAE {
    union {
        struct {
            ULONGLONG Valid : 1;
            ULONGLONG Write : 1;
            ULONGLONG Owner : 1;
            ULONGLONG WriteThrough : 1;
            ULONGLONG CacheDisable : 1;
            ULONGLONG Accessed : 1;
            ULONGLONG Dirty : 1;
            ULONGLONG LargePage : 1;
            ULONGLONG Global : 1;
            ULONGLONG CopyOnWrite : 1; // software field
            ULONGLONG Prototype : 1;   // software field
            ULONGLONG reserved0 : 1;  // software field
            ULONGLONG PageFrameNumber : 26;
            ULONGLONG reserved1 : 26;  // software field
        };
        struct {
            ULONG LowPart;
            ULONG HighPart;
        };
    };
} HARDWARE_PTE_X86PAE, *PHARDWARE_PTE_X86PAE;
#if defined (_NTSYM_HARDWARE_PTE_SYMBOL_)
#if !defined (_X86PAE_)
typedef struct _HARDWARE_PTE {
    ULONG Valid : 1;
    ULONG Write : 1;
    ULONG Owner : 1;
    ULONG WriteThrough : 1;
    ULONG CacheDisable : 1;
    ULONG Accessed : 1;
    ULONG Dirty : 1;
    ULONG LargePage : 1;
    ULONG Global : 1;
    ULONG CopyOnWrite : 1; // software field
    ULONG Prototype : 1;   // software field
    ULONG reserved : 1;  // software field
    ULONG PageFrameNumber : 20;
} HARDWARE_PTE, *PHARDWARE_PTE;

#else
typedef struct _HARDWARE_PTE {
    union {
        struct {
            ULONGLONG Valid : 1;
            ULONGLONG Write : 1;
            ULONGLONG Owner : 1;
            ULONGLONG WriteThrough : 1;
            ULONGLONG CacheDisable : 1;
            ULONGLONG Accessed : 1;
            ULONGLONG Dirty : 1;
            ULONGLONG LargePage : 1;
            ULONGLONG Global : 1;
            ULONGLONG CopyOnWrite : 1; // software field
            ULONGLONG Prototype : 1;   // software field
            ULONGLONG reserved0 : 1;  // software field
            ULONGLONG PageFrameNumber : 26;
            ULONGLONG reserved1 : 26;  // software field
        };
        struct {
            ULONG LowPart;
            ULONG HighPart;
        };
    };
} HARDWARE_PTE, *PHARDWARE_PTE;
#endif

#else

#if !defined (_X86PAE_)
typedef HARDWARE_PTE_X86 HARDWARE_PTE;
typedef PHARDWARE_PTE_X86 PHARDWARE_PTE;
#else
typedef HARDWARE_PTE_X86PAE HARDWARE_PTE;
typedef PHARDWARE_PTE_X86PAE PHARDWARE_PTE;
#endif
#endif


typedef struct _EPROCESS {
    KPROCESS                            Pcb; // +0x000
    EX_PUSH_LOCK                     ProcessLock; // +0x06c
    LARGE_INTEGER                   CreateTime; // +0x070
    LARGE_INTEGER                   ExitTime; // +0x078
    EX_RUNDOWN_REF                RundownProtect; // +0x080
    HANDLE                                  UniqueProcessId; // +0x084
    LIST_ENTRY                          ActiveProcessLinks; // +0x088
    ULONG                                    QuotaUsage[3]; // +0x090
    ULONG                                   QuotaPeak[3]; // +0x09c
    ULONG                                   CommitCharge; // +0x0a8
    ULONG                                   PeakVirtualSize; // +0x0ac
    ULONG                                   VirtualSize; // +0x0b0
    LIST_ENTRY                          SessionProcessLinks; // +0x0b4
    PVOID                                    DebugPort; // +0x0bc
    PVOID                                   ExceptionPort; // +0x0c0
    PHANDLE_TABLE                   ObjectTable; // +0x0c4
    EX_FAST_REF                     Token; // +0x0c8
    FAST_MUTEX                        WorkingSetLock; // +0x0cc
   ULONG                                      WorkingSetPage; // +0x0ec
    KGUARDED_MUTEX               AddressCreationLock; // +0x0f0
    ULONG                                   HyperSpaceLock; // +0x110
    PETHREAD                           ForkInProgress; // +0x114
    ULONG                                HardwareTrigger; // +0x118
   PVOID                                     VadRoot;// +0x11c
   PVOID                                     VadHint;// +0x120
    PVOID                                 CloneRoot; // +0x124
    ULONG                                  NumberOfPrivatePages; // +0x128
    ULONG                                NumberOfLockedPages; // +0x12c
    PVOID                                   Win32Process; // +0x130
    PEJOB                                Job; // +0x134
    PVOID                                 SectionObject; // +0x138
    PVOID                                 SectionBaseAddress; // +0x13c
    PEPROCESS_QUOTA_BLOCK           QuotaBlock; // +0x140
    PPAGEFAULT_HISTORY                    WorkingSetWatch; // +0x144
    PVOID                           Win32WindowStation; // +0x148
    ULONG                           InheritedFromUniqueProcessId; // +0x14c
    PVOID                           LdtInformation; // +0x150
    PVOID                           VadFreeHint; // +0x154
    PVOID                           VdmObjects; // +0x158
    PVOID                           DeviceMap; // +0x15c
   LIST_ENTRY                  PhysicalVadList; //+0x160
    union {
        HARDWARE_PTE                PageDirectoryPte; // +0x168
        UINT64                      Filler; // +0x168
    };
    PVOID                           Session; // +0x170
    UCHAR                           ImageFileName[16]; // +0x174
    LIST_ENTRY                      JobLinks; // +0x184
    PVOID                           LockedPagesList; // +0x18c
    LIST_ENTRY                      ThreadListHead; // +0x190
    PVOID                           SecurityPort; // +0x198
    PVOID                           PaeTop; // +0x19c
    ULONG                           ActiveThreads; // +0x1a0
    ULONG                           GrantedAccess; // +0x1a4
    ULONG                           DefaultHardErrorProcessing; // +0x1a8
    SHORT                           LastThreadExitStatus; // +0x1ac
    PPEB                            Peb; // +0x1b0
    EX_FAST_REF                     PrefetchTrace; // +0x1b4
    LARGE_INTEGER                   ReadOperationCount; // +0x1b8
    LARGE_INTEGER                   WriteOperationCount; // +0x1c0
    LARGE_INTEGER                   OtherOperationCount; // +0x1c8
    LARGE_INTEGER                   ReadTransferCount; // +0x1d0
    LARGE_INTEGER                   WriteTransferCount; // +0x1d8
    LARGE_INTEGER                   OtherTransferCount; // +0x1e0
    ULONG                           CommitChargeLimit; // +0x1e8
    ULONG                           CommitChargePeak; // +0x1ec
    PVOID                           AweInfo; // +0x1f0
    SE_AUDIT_PROCESS_CREATION_INFO SeAuditProcessCreationInfo; // +0x1f4
    MMSUPPORT                       Vm; // +0x1f8
   ULONG                            LastFaultCount; // 0x238
    ULONG                           ModifiedPageCount; // +0x23c
    ULONG                          NumberOfVads; // 0x240
    ULONG                           JobStatus; // +0x244
    union {
        ULONG                       Flags; // 0x248
        struct {
            ULONG                   CreateReported              : 1;
            ULONG                   NoDebugInherit              : 1;
            ULONG                   ProcessExiting              : 1;
            ULONG                   ProcessDelete               : 1;
            ULONG                   Wow64SplitPages             : 1;
            ULONG                   VmDeleted                   : 1;
            ULONG                   OutswapEnabled              : 1;
            ULONG                   Outswapped                  : 1;
            ULONG                   ForkFailed                  : 1;
            ULONG                   Wow64VaSpace4Gb             : 1;
            ULONG                   AddressSpaceInitialized     : 2;
            ULONG                   SetTimerResolution          : 1;
            ULONG                   BreakOnTermination          : 1;
            ULONG                   SessionCreationUnderway     : 1;
            ULONG                   WriteWatch                  : 1;
            ULONG                   ProcessInSession            : 1;
            ULONG                   OverrideAddressSpace        : 1;
            ULONG                   HasAddressSpace             : 1;
            ULONG                   LaunchPrefetched            : 1;
            ULONG                   InjectInpageErrors          : 1;
            ULONG                   VmTopDown                   : 1;
            ULONG                   ImageNotifyDone             : 1;
            ULONG                   PdeUpdateNeeded             : 1;
            ULONG                   VdmAllowed                  : 1;
            ULONG                   Unused                      : 7;
        };
    }u1;
    NTSTATUS                        ExitStatus; // +0x24c
    USHORT                          NextPageColor; // +0x250
    union {
        struct {
            UCHAR                   SubSystemMinorVersion; // +0x252
            UCHAR                   SubSystemMajorVersion; // +0x253
        };
        USHORT                      SubSystemVersion; // +0x252
    };
    UCHAR                           PriorityClass; // +0x254
UCHAR        WorkingSetAcquiredUnsafe; // 0x255
ULONG        Cookie; // +0x258
} EPROCESS, *PEPROCESS;



//2003?? debugport +cc
// 
// 
// typedef struct _EPROCESS {
//     KPROCESS Pcb;
// 
//     //
//     // Lock used to protect:
//     // The list of threads in the process.
//     // Process token.
//     // Win32 process field.
//     // Process and thread affinity setting.
//     //
// 
//     EX_PUSH_LOCK ProcessLock;
// 
//     LARGE_INTEGER CreateTime;
//     LARGE_INTEGER ExitTime;
// 
//     //
//     // Structure to allow lock free cross process access to the process
//     // handle table, process section and address space. Acquire rundown
//     // protection with this if you do cross process handle table, process
//     // section or address space references.
//     //
// 
//     EX_RUNDOWN_REF RundownProtect;
// 
//     HANDLE UniqueProcessId;
// 
//     //
//     // Global list of all processes in the system. Processes are removed
//     // from this list in the object deletion routine.  References to
//     // processes in this list must be done with ObReferenceObjectSafe
//     // because of this.
//     //
// 
//     LIST_ENTRY ActiveProcessLinks;
// 
//     //
//     // Quota Fields.
//     //
// 
//     SIZE_T QuotaUsage[PsQuotaTypes];
//     SIZE_T QuotaPeak[PsQuotaTypes];
//     SIZE_T CommitCharge;
// 
//     //
//     // VmCounters.
//     //
// 
//     SIZE_T PeakVirtualSize;
//     SIZE_T VirtualSize;
// 
//     LIST_ENTRY SessionProcessLinks;
// 
//     PVOID DebugPort;
//     PVOID ExceptionPort;
//     PHANDLE_TABLE ObjectTable;
// 
//     //
//     // Security.
//     //
// 
//     EX_FAST_REF Token;
// 
//     PFN_NUMBER WorkingSetPage;
//     KGUARDED_MUTEX AddressCreationLock;
//     KSPIN_LOCK HyperSpaceLock;
// 
//     struct _ETHREAD *ForkInProgress;
//     ULONG_PTR HardwareTrigger;
// 
//     PMM_AVL_TABLE PhysicalVadRoot;
//     PVOID CloneRoot;
//     PFN_NUMBER NumberOfPrivatePages;
//     PFN_NUMBER NumberOfLockedPages;
//     PVOID Win32Process;
//     struct _EJOB *Job;
//     PVOID SectionObject;
// 
//     PVOID SectionBaseAddress;
// 
//     PEPROCESS_QUOTA_BLOCK QuotaBlock;
// 
//     PPAGEFAULT_HISTORY WorkingSetWatch;
//     HANDLE Win32WindowStation;
//     HANDLE InheritedFromUniqueProcessId;
// 
//     PVOID LdtInformation;
//     PVOID VadFreeHint;
//     PVOID VdmObjects;
//     PVOID DeviceMap;
// 
//     PVOID Spare0[3];
//     union {
//         HARDWARE_PTE PageDirectoryPte;
//         ULONGLONG Filler;
//     };
//     PVOID Session;
//     UCHAR ImageFileName[ 16 ];
// 
//     LIST_ENTRY JobLinks;
//     PVOID LockedPagesList;
// 
//     LIST_ENTRY ThreadListHead;
// 
//     //
//     // Used by rdr/security for authentication.
//     //
// 
//     PVOID SecurityPort;
// 
// #ifdef _WIN64
//     PWOW64_PROCESS Wow64Process;
// #else
//     PVOID PaeTop;
// #endif
// 
//     ULONG ActiveThreads;
// 
//     ACCESS_MASK GrantedAccess;
// 
//     ULONG DefaultHardErrorProcessing;
// 
//     NTSTATUS LastThreadExitStatus;
// 
//     //
//     // Peb
//     //
// 
//     PPEB Peb;
// 
//     //
//     // Pointer to the prefetches trace block.
//     //
//     EX_FAST_REF PrefetchTrace;
// 
//     LARGE_INTEGER ReadOperationCount;
//     LARGE_INTEGER WriteOperationCount;
//     LARGE_INTEGER OtherOperationCount;
//     LARGE_INTEGER ReadTransferCount;
//     LARGE_INTEGER WriteTransferCount;
//     LARGE_INTEGER OtherTransferCount;
// 
//     SIZE_T CommitChargeLimit;
//     SIZE_T CommitChargePeak;
// 
//     PVOID AweInfo;
// 
//     //
//     // This is used for SeAuditProcessCreation.
//     // It contains the full path to the image file.
//     //
// 
//     SE_AUDIT_PROCESS_CREATION_INFO SeAuditProcessCreationInfo;
// 
//     MMSUPPORT Vm;
// 
// #if !defined(_WIN64)
//     LIST_ENTRY MmProcessLinks;
// #else
//     ULONG Spares[2];
// #endif
// 
//     ULONG ModifiedPageCount;
// 
//     #define PS_JOB_STATUS_NOT_REALLY_ACTIVE      0x00000001UL
//     #define PS_JOB_STATUS_ACCOUNTING_FOLDED      0x00000002UL
//     #define PS_JOB_STATUS_NEW_PROCESS_REPORTED   0x00000004UL
//     #define PS_JOB_STATUS_EXIT_PROCESS_REPORTED  0x00000008UL
//     #define PS_JOB_STATUS_REPORT_COMMIT_CHANGES  0x00000010UL
//     #define PS_JOB_STATUS_LAST_REPORT_MEMORY     0x00000020UL
//     #define PS_JOB_STATUS_REPORT_PHYSICAL_PAGE_CHANGES  0x00000040UL
// 
//     ULONG JobStatus;
// 
// 
//     //
//     // Process flags. Use interlocked operations with PS_SET_BITS, etc
//     // to modify these.
//     //
// 
//     #define PS_PROCESS_FLAGS_CREATE_REPORTED        0x00000001UL // Create process debug call has occurred
//     #define PS_PROCESS_FLAGS_NO_DEBUG_INHERIT       0x00000002UL // Don't inherit debug port
//     #define PS_PROCESS_FLAGS_PROCESS_EXITING        0x00000004UL // PspExitProcess entered
//     #define PS_PROCESS_FLAGS_PROCESS_DELETE         0x00000008UL // Delete process has been issued
//     #define PS_PROCESS_FLAGS_WOW64_SPLIT_PAGES      0x00000010UL // Wow64 split pages
//     #define PS_PROCESS_FLAGS_VM_DELETED             0x00000020UL // VM is deleted
//     #define PS_PROCESS_FLAGS_OUTSWAP_ENABLED        0x00000040UL // Outswap enabled
//     #define PS_PROCESS_FLAGS_OUTSWAPPED             0x00000080UL // Outswapped
//     #define PS_PROCESS_FLAGS_FORK_FAILED            0x00000100UL // Fork status
//     #define PS_PROCESS_FLAGS_WOW64_4GB_VA_SPACE     0x00000200UL // Wow64 process with 4gb virtual address space
//     #define PS_PROCESS_FLAGS_ADDRESS_SPACE1         0x00000400UL // Addr space state1
//     #define PS_PROCESS_FLAGS_ADDRESS_SPACE2         0x00000800UL // Addr space state2
//     #define PS_PROCESS_FLAGS_SET_TIMER_RESOLUTION   0x00001000UL // SetTimerResolution has been called
//     #define PS_PROCESS_FLAGS_BREAK_ON_TERMINATION   0x00002000UL // Break on process termination
//     #define PS_PROCESS_FLAGS_CREATING_SESSION       0x00004000UL // Process is creating a session
//     #define PS_PROCESS_FLAGS_USING_WRITE_WATCH      0x00008000UL // Process is using the write watch APIs
//     #define PS_PROCESS_FLAGS_IN_SESSION             0x00010000UL // Process is in a session
//     #define PS_PROCESS_FLAGS_OVERRIDE_ADDRESS_SPACE 0x00020000UL // Process must use native address space (Win64 only)
//     #define PS_PROCESS_FLAGS_HAS_ADDRESS_SPACE      0x00040000UL // This process has an address space
//     #define PS_PROCESS_FLAGS_LAUNCH_PREFETCHED      0x00080000UL // Process launch was prefetched
//     #define PS_PROCESS_INJECT_INPAGE_ERRORS         0x00100000UL // Process should be given inpage errors - hardcoded in trap.asm too
//     #define PS_PROCESS_FLAGS_VM_TOP_DOWN            0x00200000UL // Process memory allocations default to top-down
//     #define PS_PROCESS_FLAGS_IMAGE_NOTIFY_DONE      0x00400000UL // We have sent a message for this image
//     #define PS_PROCESS_FLAGS_PDE_UPDATE_NEEDED      0x00800000UL // The system PDEs need updating for this process (NT32 only)
//     #define PS_PROCESS_FLAGS_VDM_ALLOWED            0x01000000UL // Process allowed to invoke NTVDM support
//     #define PS_PROCESS_FLAGS_SMAP_ALLOWED           0x02000000UL // Process allowed to invoke SMAP support
//     #define PS_PROCESS_FLAGS_CREATE_FAILED          0x04000000UL // Process create failed
// 
//     #define PS_PROCESS_FLAGS_DEFAULT_IO_PRIORITY    0x38000000UL // The default I/O priority for created threads. (3 bits)
// 
//     #define PS_PROCESS_FLAGS_PRIORITY_SHIFT         27
//     
//     #define PS_PROCESS_FLAGS_EXECUTE_SPARE1         0x40000000UL //
//     #define PS_PROCESS_FLAGS_EXECUTE_SPARE2         0x80000000UL //
// 
// 
//     union {
// 
//         ULONG Flags;
// 
//         //
//         // Fields can only be set by the PS_SET_BITS and other interlocked
//         // macros.  Reading fields is best done via the bit definitions so
//         // references are easy to locate.
//         //
// 
//         struct {
//             ULONG CreateReported            : 1;
//             ULONG NoDebugInherit            : 1;
//             ULONG ProcessExiting            : 1;
//             ULONG ProcessDelete             : 1;
//             ULONG Wow64SplitPages           : 1;
//             ULONG VmDeleted                 : 1;
//             ULONG OutswapEnabled            : 1;
//             ULONG Outswapped                : 1;
//             ULONG ForkFailed                : 1;
//             ULONG Wow64VaSpace4Gb           : 1;
//             ULONG AddressSpaceInitialized   : 2;
//             ULONG SetTimerResolution        : 1;
//             ULONG BreakOnTermination        : 1;
//             ULONG SessionCreationUnderway   : 1;
//             ULONG WriteWatch                : 1;
//             ULONG ProcessInSession          : 1;
//             ULONG OverrideAddressSpace      : 1;
//             ULONG HasAddressSpace           : 1;
//             ULONG LaunchPrefetched          : 1;
//             ULONG InjectInpageErrors        : 1;
//             ULONG VmTopDown                 : 1;
//             ULONG ImageNotifyDone           : 1;
//             ULONG PdeUpdateNeeded           : 1;    // NT32 only
//             ULONG VdmAllowed                : 1;
//             ULONG SmapAllowed               : 1;
//             ULONG CreateFailed              : 1;
//             ULONG DefaultIoPriority         : 3;
//             ULONG Spare1                    : 1;
//             ULONG Spare2                    : 1;
//         };
//     }u1;
// 
//     NTSTATUS ExitStatus;
// 
//     USHORT NextPageColor;
//     union {
//         struct {
//             UCHAR SubSystemMinorVersion;
//             UCHAR SubSystemMajorVersion;
//         };
//         USHORT SubSystemVersion;
//     };
//     UCHAR PriorityClass;
// 
//     MM_AVL_TABLE VadRoot;
// 
//     ULONG Cookie;
// 
// } EPROCESS, *PEPROCESS; 




#endif