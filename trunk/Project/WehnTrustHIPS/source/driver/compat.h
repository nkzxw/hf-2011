/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_COMPAT_H
#define _WEHNTRUST_DRIVER_COMPAT_H

NTSTATUS CompatInitialize();

typedef struct _SECTION_IMAGE_INFORMATION
{
	PVOID TransferAddress;
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

//
// Run-time determined functions pointers
//

extern VOID (NTAPI * KiCheckForKernelApcDelivery)(VOID);

//
// Process object accessors
//
extern VOID (*AcquireProcessRegionLock)(
		IN PPROCESS_OBJECT ProcessObject);
extern VOID (*ReleaseProcessRegionLock)(
		IN PPROCESS_OBJECT ProcessObject);
extern PMMVAD (*GetProcessVadRoot)(
		IN PPROCESS_OBJECT ProcessObject);

//
// Vad object accessors
//
extern PVOID (*GetVadStartingVpn)(
		IN PMMVAD Vad);
extern PVOID (*GetVadEndingVpn)(
		IN PMMVAD Vad);
extern PMMVAD (*GetVadLeftChild)(
		IN PMMVAD Vad);
extern PMMVAD (*GetVadRightChild)(
		IN PMMVAD Vad);

//
// Section object accessors
//
extern ULONG (*GetSectionObjectControlAreaFlags)(
		IN PSECTION_OBJECT SectionObject);
extern PFILE_OBJECT (*GetSectionObjectControlAreaFilePointer)(
		IN PSECTION_OBJECT SectionObject);
extern PVOID (*GetSectionObjectSegmentBasedAddress)(
		IN PSECTION_OBJECT SectionObject);
extern VOID (*SetSectionObjectSegmentBasedAddress)(
		IN PSECTION_OBJECT SectionObject,
		IN PVOID BasedAddress);
extern ULONG (*GetSectionObjectControlAreaNumberOfMappedViews)(
		IN PSECTION_OBJECT SectionObject);
extern ULONG (*GetSectionObjectControlAreaNumberOfSectionReferences)(
		IN PSECTION_OBJECT SectionObject);
extern PLARGE_INTEGER (*GetSectionObjectSegmentSize)(
		IN PSECTION_OBJECT SectionObject);
extern PSECTION_IMAGE_INFORMATION (*GetSectionObjectSegmentImageInformation)(
		IN PSECTION_OBJECT SectionObject);

#define GetSectionObjectSegmentFirstMappedVa GetSectionObjectSegmentImageInformation

////
//
// Platform specific structure definitions
//
////

//
// Windows 2000
//
struct _SEGMENT_W2K;

typedef struct _CONTROL_AREA_W2K
{
	struct _SEGMENT_W2K        *Segment;
	LIST_ENTRY                 DereferenceList;
	ULONG                      NumberOfSectionReferences;
	ULONG                      NumberOfPfnReferences;
	ULONG                      NumberOfMappedViews;
	USHORT                     NumberOfSubsections;
	USHORT                     FlushInProgress;
	ULONG                      NumberOfUserReferences;
	ULONG                      Flags;
	PFILE_OBJECT               FilePointer;
	PVOID                      WaitingForDeletion;
	USHORT                     ModifiedWriteCount;
	USHORT                     NumberOfSystemCacheViews;
} CONTROL_AREA_W2K, *PCONTROL_AREA_W2K;

//
// Not entirely sure on the format of this structure
//
typedef struct _SEGMENT_W2K
{
	PCONTROL_AREA_W2K          ControlArea;
	ULONG                      TotalNumberOfPtes;
	ULONG                      NonExtendedPtes;
	ULONG                      WritableUserReferences;
	LARGE_INTEGER              SizeOfSegment;
	PVOID                      SegmentPteTemplate; // _MMPTE
	union {
		PSECTION_IMAGE_INFORMATION ImageInformation;
	} u2;
	ULONG                      NumberOfCommitedPages;
	PVOID                      ExtendInfo;
	PVOID                      ImageBaseAddress;
	PVOID                      BasedAddress;
	PVOID                      u1;
	PVOID                      PrototypePte;
	PVOID                      ThePtes; // _MMPTE
} SEGMENT_W2K, *PSEGMENT_W2K;

typedef struct _SECTION_OBJECT_W2K
{
	PVOID                      StartingVa;
	PVOID                      EndingVa;
	struct _SECTION_OBJECT_W2K *Parent;
	struct _SECTION_OBJECT_W2K *LeftChild;
	struct _SECTION_OBJECT_W2K *RightChild;
	PSEGMENT_W2K               Segment;
} SECTION_OBJECT_W2K, *PSECTION_OBJECT_W2K;

//
// Windows XP
//
struct _SEGMENT_WXP;

typedef struct _CONTROL_AREA_WXP
{
	struct _SEGMENT_WXP        *Segment;
	LIST_ENTRY                 DereferenceList;
	ULONG                      NumberOfSectionReferences;
	ULONG                      NumberOfPfnReferences;
	ULONG                      NumberOfMappedViews;
	USHORT                     NumberOfSubsections;
	USHORT                     FlushInProgress;
	ULONG                      NumberOfUserReferences;
	ULONG                      Flags;
	PFILE_OBJECT               FilePointer;
	PVOID                      WaitingForDeletion;
	USHORT                     ModifiedWriteCount;
	USHORT                     NumberOfSystemCacheViews;
} CONTROL_AREA_WXP, *PCONTROL_AREA_WXP;

typedef struct _SEGMENT_WXP
{
	PCONTROL_AREA_WXP          ControlArea;
	ULONG                      TotalNumberOfPtes;
	ULONG                      NonExtendedPtes;
	ULONG                      WritableUserReferences;
	LARGE_INTEGER              SizeOfSegment;
	PVOID                      SegmentPteTemplate; // _MMPTE
	ULONG                      NumberOfCommitedPages;
	PVOID                      ExtendInfo;
	PVOID                      SystemImageBase;
	PVOID                      BasedAddress;
	PVOID                      u1;
	union {
		PSECTION_IMAGE_INFORMATION ImageInformation;
	} u2;
	PVOID                      PrototypePte;
	PVOID                      ThePtes; // _MMPTE
} SEGMENT_WXP, *PSEGMENT_WXP;

//
// The SegmentPteTemplate attribute is 8 bytes long in the PAE kernel in XPSP2.
//
typedef struct _SEGMENT_WXP_XPSP2_PAE
{
	PCONTROL_AREA_WXP          ControlArea;
	ULONG                      TotalNumberOfPtes;
	ULONG                      NonExtendedPtes;
	ULONG                      WritableUserReferences;
	LARGE_INTEGER              SizeOfSegment;
	ULONGLONG                  SegmentPteTemplate; // _MMPTE 
	ULONG                      NumberOfCommitedPages;
	PVOID                      ExtendInfo;
	PVOID                      SystemImageBase;
	PVOID                      BasedAddress;
	PVOID                      u1;
	union {
		PSECTION_IMAGE_INFORMATION ImageInformation;
	} u2;
	PVOID                      PrototypePte;
	PVOID                      ThePtes; // _MMPTE
} SEGMENT_WXP_XPSP2_PAE, *PSEGMENT_WXP_XPSP2_PAE;

typedef struct _SECTION_OBJECT_WXP
{
	PVOID                      StartingVa;
	PVOID                      EndingVa;
	struct _SECTION_OBJECT_WXP *Parent;
	struct _SECTION_OBJECT_WXP *LeftChild;
	struct _SECTION_OBJECT_WXP *RightChild;
	PSEGMENT_WXP               Segment;
} SECTION_OBJECT_WXP, *PSECTION_OBJECT_WXP;

typedef struct _MMVAD_WXP
{
	PVOID                      StartingVpn;
	PVOID                      EndingVpn;
	struct _MMVAD_WXP          *Parent;
	struct _MMVAD_WXP          *LeftChild;
	struct _MMVAD_WXP          *RightChild;
	union {
		ULONG                   LongFlags;
		struct {
			ULONG                CommitCharge : 16;
			ULONG                OtherFlags   : 16;
		} VadFlags;
	} u;
	PCONTROL_AREA_WXP          ControlArea;
	PVOID                      FirstPrototypePte;
	PVOID                      LastContiguousPte;
	PVOID                      Unknown2;
} MMVAD_WXP, *PMMVAD_WXP;

typedef struct _EPROCESS_WXP
{
	CHAR                       Garbage[0xcc];
	FAST_MUTEX                 WorkingSetLock;
	ULONG                      WorkingSetPage;
	FAST_MUTEX                 AddressCreationLock;
	CHAR                       Garbage2[0xc];
	PMMVAD_WXP                 VadRoot;
	CHAR                       Garbage3[0x90];
	PPEB                       Peb;
} EPROCESS_WXP, *PEPROCESS_WXP;

typedef struct _EPROCESS_WXP PROCESS_OBJECT_WXP, *PPROCESS_OBJECT_WXP;

//
// Windows 2000
//

typedef struct _EPROCESS_W2K
{
	CHAR                       Pad[0x130];
	FAST_MUTEX                 WorkingSetLock;
	ULONG                      WorkingSetPage;
	ULONG                      SomeFlags;
	FAST_MUTEX                 AddressCreationLock;
	CHAR                       Pad2[0x20];
	PMMVAD_WXP                 VadRoot;
} EPROCESS_W2K, *PEPROCESS_W2K;

typedef struct _EPROCESS_W2K PROCESS_OBJECT_W2K, *PPROCESS_OBJECT_W2K;


//
// Windows 2003 Server
//
struct _SEGMENT_2K3;

typedef struct _CONTROL_AREA_2K3
{
	struct _SEGMENT_2K3           *Segment;
	LIST_ENTRY                    DereferenceList;
	ULONG                         NumberOfSectionReferences;
	ULONG                         NumberOfPfnReferences;
	ULONG                         NumberOfMappedViews;
	ULONG                         NumberOfSystemCacheViews;
	ULONG                         NumberOfUserReferences;
	ULONG                         Flags;
	PFILE_OBJECT                  FilePointer;
	ULONG                         EventCounter;
	USHORT                        ModifiedWriteCount;
	USHORT                        FlushInProgressCount;
} CONTROL_AREA_2K3, *PCONTROL_AREA_2K3;

typedef struct _SEGMENT_2K3
{
	PCONTROL_AREA_2K3             ControlArea;
	ULONG                         TotalNumberOfPtes;
	ULONG                         NonExtendedPtes;
	ULONG                         WritableUserReferences;
	LARGE_INTEGER                 SizeOfSegment;
	PVOID                         SegmentPteTemplate; // _MMPTE
	ULONG                         NumberOfCommitedPages;
	PVOID                         ExtendInfo;
	PVOID                         SystemImageBase;
	PVOID                         BasedAddress;
	PVOID                         u1;
	union {
		PSECTION_IMAGE_INFORMATION ImageInformation;
	} u2;
	PVOID                         PrototypePte;
	PVOID                         ThePtes; // _MMPTE
} SEGMENT_2K3, *PSEGMENT_2K3;

typedef struct _SECTION_OBJECT_2K3
{
	PVOID                         StartingVa;
	PVOID                         EndingVa;
	struct _SECTION_OBJECT_2K3    *Parent;
	struct _SECTION_OBJECT_2K3    *LeftChild;
	struct _SECTION_OBJECT_2K3    *RightChild;
	PSEGMENT_2K3                  Segment;
} SECTION_OBJECT_2K3, *PSECTION_OBJECT_2K3;

typedef struct _KGUARDED_MUTEX_2K3
{
	LONG                          Count;
	PKTHREAD                      Owner;
	ULONG                         Contention;
	KEVENT                        Event;
	USHORT                        KernelApcDisable;
	USHORT                        SpecialApcDisable;
} KGUARDED_MUTEX_2K3, *PKGUARDED_MUTEX_2K3;

typedef struct _MMADDRESS_NODE_2K3
{
	union
	{
		ULONG                      Balance;
		struct _MMADDRESS_NODE_2K3 *Parent;
	} u1;
	struct _MMADDRESS_NODE_2K3    *LeftChild;
	struct _MMADDRESS_NODE_2K3    *RightChild;
	PVOID                         StartingVpn;
	PVOID                         EndingVpn;
	union {
		ULONG                      LongFlags;
		struct {
			ULONG                   CommitCharge : 16;
			ULONG                   OtherFlags   : 16;
		} VadFlags;
	} u;
} MMADDRESS_NODE_2K3, *PMMADDRESS_NODE_2K3;

typedef struct _MM_AVL_TABLE_2K3
{
	MMADDRESS_NODE_2K3            BalancedRoot;
	ULONG                         DepthOfTree : 5;
	ULONG                         Unused : 3;
	ULONG                         NumberOfGenericTableElements : 24;
	PVOID                         NodeHint;
	PVOID                         NodeFreeHint;
} MM_AVL_TABLE_2K3, *PMM_AVL_TABLE_2K3;

typedef struct _EPROCESS_2K3
{
	CHAR                          Pad[0xd0];
	KGUARDED_MUTEX_2K3            AddressCreationLock;
	CHAR                          Pad2[0x128];
	KGUARDED_MUTEX_2K3            WorkingSetMutex;
	CHAR                          Pad3[0x20];
	MM_AVL_TABLE_2K3              VadRoot;
} EPROCESS_2K3, *PEPROCESS_2K3;

typedef struct _EPROCESS_2K3 PROCESS_OBJECT_2K3, *PPROCESS_OBJECT_2K3;

typedef struct _KTHREAD_2K3
{
	DISPATCHER_HEADER             Header;
	LIST_ENTRY                    MutantListHead;
	PVOID                         InitialStack;
	PVOID                         StackLimit;
	PVOID                         KernelStack;
	KSPIN_LOCK                    ThreadLock;
	ULONG                         ContextSwitches;
	UCHAR                         State;
	UCHAR                         NpxState;
	KIRQL                         WaitIrql;
	KPROCESSOR_MODE               WaitMode;
	struct _TEB*                  Teb;
	KAPC_STATE                    ApcState;
	KSPIN_LOCK                    ApcQueueLock;
	NTSTATUS                      WaitStatus;
	PVOID                         WaitBlockList;
	BOOLEAN                       Alertable;
	UCHAR                         WaitNext;
	UCHAR                         WaitReason;
	CCHAR                         Priority;
	BOOLEAN                       EnableStackSwap;
	BOOLEAN                       SwapBusy;
	BOOLEAN                       Alerted[MaximumMode];
	LIST_ENTRY                    WaitListEntry;
	SLIST_ENTRY                   SwapListEntry;
	PVOID                         Queue;
	ULONG                         WaitTime;
	union {
		CSHORT                    KernelApcDisable;
		CSHORT                    SpecialApcDisable;
		ULONG                     CombinedApcDisable;
	};
	KTIMER                        Timer;
	KWAIT_BLOCK                   WaitBlock[4]; /* Hrm, not 3 (THREAD_WAIT_OBJECTS)? */
	LIST_ENTRY                    QueueListEntry;
	UCHAR                         ApcStateIndex;
	BOOLEAN                       ApcQueueable;
	BOOLEAN                       Preempted;
	UCHAR                         ProcessReadyQueue;
	BOOLEAN                       KernelStackResident;
	CCHAR                         Saturation;
	UCHAR                         IdealProcessor;
	UCHAR                         NextProcessor;
	CCHAR                         BasePriority;
	UCHAR                         Spare4;
	CCHAR                         PriorityDecrement;
	CCHAR                         Quantum;
	BOOLEAN                       SystemAffinityActive;
	KPROCESSOR_MODE               PreviousMode;
	UCHAR                         ResourceIndex;
	BOOLEAN                       DisableBoost;
	KAFFINITY                     UserAffinity;
	PVOID                         Process;
	KAFFINITY                     Affinity;
	PVOID                         ServiceTable;
	PVOID                         ApcStatePointer;
	KAPC_STATE                    SavedApcState;
	PVOID                         CallbackStack;
	PVOID                         Win32Thread;
	struct _KTRAP_FRAME*          TrapFrame;
	ULONG                         KernelTime;
	ULONG                         UserTime;
	PVOID                         StackBase;
	KAPC                          SuspendApc;
	KSEMAPHORE                    SuspendSemaphore;
	PVOID                         TlsArray;
	PVOID                         LegoData;
	LIST_ENTRY                    ThreadListEntry;
	BOOLEAN                       LargeStack;
	UCHAR                         PowerState;
	KIRQL                         NpxIrql;
	UCHAR                         Spare5;
	BOOLEAN                       AutoAlignment;
	UCHAR                         Iopl;
	CCHAR                         FreezeCount;
	CCHAR                         SuspendCount;
	UCHAR                         Spare0;
	UCHAR                         UserIdealProcessor;
	UCHAR                         DeferredProcessor;
	UCHAR                         AdjustReason;
	CCHAR                         AdjustIncrement;
	UCHAR                         Spare2;
} KTHREAD_2K3, * PKTHREAD_2K3;

typedef struct _KTHREAD_2K3 THREAD_OBJECT_2K3, *PTHREAD_OBJECT_2K3;

//
// Whew, hope I got those fields right...
//

#define WR_MUTEX ((KWAIT_REASON)0x1D)

extern ULONG COMPAT_OBJ_KERNEL_HANDLE;

VOID CheckSwitchToPae(
		IN PSECTION_OBJECT_WXP SectionObject,
		IN PVOID ExpectedBaseAddress);

#endif
