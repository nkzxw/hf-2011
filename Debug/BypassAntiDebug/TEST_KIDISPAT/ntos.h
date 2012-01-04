// ntos.h: interface for the ntos class.
//
//////////////////////////////////////////////////////////////////////



#if !defined(AFX_NTOS_H__6A0074D1_8BF0_4D6C_88FB_5FCB4F06B186__INCLUDED_)
#define AFX_NTOS_H__6A0074D1_8BF0_4D6C_88FB_5FCB4F06B186__INCLUDED_

#ifndef IN
#define IN
#define OUT
#endif

#ifdef FASTCALL
#undef FASTCALL
#endif

#ifndef FASTCALL
#define FASTCALL _fastcall
#endif


#pragma warning(disable: 4013)
//
// Process flags. Use interlocked operations with PS_SET_BITS, etc
// to modify these.
//

#include "ntdbg.h"



typedef ULONG WIN32_PROTECTION_MASK;
typedef PULONG PWIN32_PROTECTION_MASK;

#define MM_ZERO_ACCESS         0  // this value is not used.
#define MM_READONLY            1
#define MM_EXECUTE             2
#define MM_EXECUTE_READ        3
#define MM_READWRITE           4  // bit 2 is set if this is writable.
#define MM_WRITECOPY           5
#define MM_EXECUTE_READWRITE   6
#define MM_EXECUTE_WRITECOPY   7

#define MM_NOCACHE            0x8
#define MM_GUARD_PAGE         0x10
#define MM_DECOMMIT           0x10   // NO_ACCESS, Guard page
#define MM_NOACCESS           0x18   // NO_ACCESS, Guard_page, nocache.
#define MM_UNKNOWN_PROTECTION 0x100  // bigger than 5 bits!

#define MM_INVALID_PROTECTION ((ULONG)-1)  // bigger than 5 bits!

#define MM_PROTECTION_WRITE_MASK     4
#define MM_PROTECTION_COPY_MASK      1
#define MM_PROTECTION_OPERATION_MASK 7 // mask off guard page and nocache.
#define MM_PROTECTION_EXECUTE_MASK   2

#define MM_SECURE_DELETE_CHECK 0x55

#define PS_PROCESS_FLAGS_CREATE_REPORTED        0x00000001UL // Create process debug call has occurred
#define PS_PROCESS_FLAGS_NO_DEBUG_INHERIT       0x00000002UL // Don't inherit debug port
#define PS_PROCESS_FLAGS_PROCESS_EXITING        0x00000004UL // PspExitProcess entered
#define PS_PROCESS_FLAGS_PROCESS_DELETE         0x00000008UL // Delete process has been issued
#define PS_PROCESS_FLAGS_WOW64_SPLIT_PAGES      0x00000010UL // Wow64 split pages
#define PS_PROCESS_FLAGS_VM_DELETED             0x00000020UL // VM is deleted
#define PS_PROCESS_FLAGS_OUTSWAP_ENABLED        0x00000040UL // Outswap enabled
#define PS_PROCESS_FLAGS_OUTSWAPPED             0x00000080UL // Outswapped
#define PS_PROCESS_FLAGS_FORK_FAILED            0x00000100UL // Fork status
#define PS_PROCESS_FLAGS_WOW64_4GB_VA_SPACE     0x00000200UL // Wow64 process with 4gb virtual address space
#define PS_PROCESS_FLAGS_ADDRESS_SPACE1         0x00000400UL // Addr space state1
#define PS_PROCESS_FLAGS_ADDRESS_SPACE2         0x00000800UL // Addr space state2
#define PS_PROCESS_FLAGS_SET_TIMER_RESOLUTION   0x00001000UL // SetTimerResolution has been called
#define PS_PROCESS_FLAGS_BREAK_ON_TERMINATION   0x00002000UL // Break on process termination
#define PS_PROCESS_FLAGS_CREATING_SESSION       0x00004000UL // Process is creating a session
#define PS_PROCESS_FLAGS_USING_WRITE_WATCH      0x00008000UL // Process is using the write watch APIs
#define PS_PROCESS_FLAGS_IN_SESSION             0x00010000UL // Process is in a session
#define PS_PROCESS_FLAGS_OVERRIDE_ADDRESS_SPACE 0x00020000UL // Process must use native address space (Win64 only)
#define PS_PROCESS_FLAGS_HAS_ADDRESS_SPACE      0x00040000UL // This process has an address space
#define PS_PROCESS_FLAGS_LAUNCH_PREFETCHED      0x00080000UL // Process launch was prefetched
#define PS_PROCESS_INJECT_INPAGE_ERRORS         0x00100000UL // Process should be given inpage errors - hardcoded in trap.asm too
#define PS_PROCESS_FLAGS_VM_TOP_DOWN            0x00200000UL // Process memory allocations default to top-down
#define PS_PROCESS_FLAGS_IMAGE_NOTIFY_DONE      0x00400000UL // We have sent a message for this image
#define PS_PROCESS_FLAGS_PDE_UPDATE_NEEDED      0x00800000UL // The system PDEs need updating for this process (NT32 only)
#define PS_PROCESS_FLAGS_VDM_ALLOWED            0x01000000UL // Process allowed to invoke NTVDM support
#define PS_PROCESS_FLAGS_SMAP_ALLOWED           0x02000000UL // Process allowed to invoke SMAP support
#define PS_PROCESS_FLAGS_CREATE_FAILED          0x04000000UL // Process create failed

#define PS_PROCESS_FLAGS_DEFAULT_IO_PRIORITY    0x38000000UL // The default I/O priority for created threads. (3 bits)

#define PS_PROCESS_FLAGS_PRIORITY_SHIFT         27

#define PS_PROCESS_FLAGS_EXECUTE_SPARE1         0x40000000UL //
#define PS_PROCESS_FLAGS_EXECUTE_SPARE2         0x80000000UL //

//
// Debugger isn't shown this thread
//

#define PS_CROSS_THREAD_FLAGS_HIDEFROMDBG          0x00000004UL


//
// This thread should skip sending its create thread message
//
#define PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG    0x00000080UL

//
// This thread should skip sending its final thread termination message
//
#define PS_CROSS_THREAD_FLAGS_SKIP_TERMINATION_MSG 0x00000100UL
#define PS_CROSS_THREAD_FLAGS_DEADTHREAD           0x00000002UL

#define POOL_QUOTA_FAIL_INSTEAD_OF_RAISE 8
#define POOL_RAISE_IF_ALLOCATION_FAILURE 16               // ntifs
#define POOL_MM_ALLOCATION 0x80000000     // Note this cannot encode into the header.



//
// i386 Feature bit definitions
//
// N.B. The no execute feature flags must be identical on all platforms.

#define KF_V86_VIS          0x00000001
#define KF_RDTSC            0x00000002
#define KF_CR4              0x00000004
#define KF_CMOV             0x00000008
#define KF_GLOBAL_PAGE      0x00000010
#define KF_LARGE_PAGE       0x00000020
#define KF_MTRR             0x00000040
#define KF_CMPXCHG8B        0x00000080
#define KF_MMX              0x00000100
#define KF_WORKING_PTE      0x00000200
#define KF_PAT              0x00000400
#define KF_FXSR             0x00000800
#define KF_FAST_SYSCALL     0x00001000
#define KF_XMMI             0x00002000
#define KF_3DNOW            0x00004000
#define KF_AMDK6MTRR        0x00008000
#define KF_XMMI64           0x00010000
#define KF_DTS              0x00020000
#define KF_NOEXECUTE        0x20000000
#define KF_GLOBAL_32BIT_EXECUTE 0x40000000
#define KF_GLOBAL_32BIT_NOEXECUTE 0x80000000

#define try _try
#define except _except





// begin_ntsrv


// end_ntsrv
#define DBGKD_MAXSTREAM 16

// typedef struct _DBGKD_CONTROL_REPORT {
//     ULONG   Dr6;
//     ULONG   Dr7;
//     USHORT  InstructionCount;
//     USHORT  ReportFlags;
//     UCHAR   InstructionStream[DBGKD_MAXSTREAM];
//     USHORT  SegCs;
//     USHORT  SegDs;
//     USHORT  SegEs;
//     USHORT  SegFs;
//     ULONG   EFlags;
// } DBGKD_CONTROL_REPORT, *PDBGKD_CONTROL_REPORT;


#define REPORT_INCLUDES_SEGS    0x0001  // this is for backward compatibility

//
// DBGKD_CONTROL_SET
//
// This structure control value the debugger wants to set on every
// continue, and thus sets here to avoid packet traffic.
//

// typedef struct _DBGKD_CONTROL_SET {
//     ULONG   TraceFlag;                  // WARNING: This must NOT be a BOOLEAN,
// 	//     or host and target will end
// 	//     up with different alignments!
//     ULONG   Dr7;
//     ULONG   CurrentSymbolStart;         // Range in which to trace locally
//     ULONG   CurrentSymbolEnd;
// } DBGKD_CONTROL_SET, *PDBGKD_CONTROL_SET;

// begin_windbgkd


#define KI_EXCEPTION_INTERNAL               0x10000000
#define KI_EXCEPTION_GP_FAULT               (KI_EXCEPTION_INTERNAL | 0x1)
#define KI_EXCEPTION_INVALID_OP             (KI_EXCEPTION_INTERNAL | 0x2)
#define KI_EXCEPTION_INTEGER_DIVIDE_BY_ZERO (KI_EXCEPTION_INTERNAL | 0x3)
#define KI_EXCEPTION_ACCESS_VIOLATION       (KI_EXCEPTION_INTERNAL | 0x4)

#define EFLAGS_CF_MASK        0x00000001L
#define EFLAGS_PF_MASK        0x00000004L
#define EFLAGS_AF_MASK        0x00000010L
#define EFLAGS_ZF_MASK        0x00000040L
#define EFLAGS_SF_MASK        0x00000080L
#define EFLAGS_TF             0x00000100L
#define EFLAGS_INTERRUPT_MASK 0x00000200L
#define EFLAGS_DF_MASK        0x00000400L
#define EFLAGS_OF_MASK        0x00000800L
#define EFLAGS_IOPL_MASK      0x00003000L
#define EFLAGS_NT             0x00004000L
#define EFLAGS_RF             0x00010000L
#define EFLAGS_V86_MASK       0x00020000L
#define EFLAGS_ALIGN_CHECK    0x00040000L
#define EFLAGS_VIF            0x00080000L
#define EFLAGS_VIP            0x00100000L
#define EFLAGS_ID_MASK        0x00200000L

#define EFLAGS_USER_SANITIZE  0x003f4dd7L


#define RPL_MASK                                0x0003
#define MODE_MASK                               0x0001
#define KGDT_R0_CODE                            0x8
#define KGDT_R0_DATA                            0x10
#define KGDT_R3_CODE                            0x18
#define KGDT_R3_DATA                            0x20
#define KGDT_TSS                                0x28
#define KGDT_R0_PCR                             0x30
#define KGDT_R3_TEB                             0x38
#define KGDT_LDT                                0x48
#define KGDT_DF_TSS                             0x50
#define KGDT_NMI_TSS                            0x58


typedef struct _DESCRIPTOR // 3 elements, 0x8 bytes (sizeof) 
{                                                            
	/*0x000*/     UINT16       Pad;                                        
	/*0x002*/     UINT16       Limit;                                      
	/*0x004*/     ULONG32      Base;                                       
          }DESCRIPTOR, *PDESCRIPTOR;        

typedef struct _KSPECIAL_REGISTERS // 15 elements, 0x54 bytes (sizeof) 
{                                                                      
	/*0x000*/     ULONG32      Cr0;                                                  
	/*0x004*/     ULONG32      Cr2;                                                  
	/*0x008*/     ULONG32      Cr3;                                                  
	/*0x00C*/     ULONG32      Cr4;                                                  
	/*0x010*/     ULONG32      KernelDr0;                                            
	/*0x014*/     ULONG32      KernelDr1;                                            
	/*0x018*/     ULONG32      KernelDr2;                                            
	/*0x01C*/     ULONG32      KernelDr3;                                            
	/*0x020*/     ULONG32      KernelDr6;                                            
	/*0x024*/     ULONG32      KernelDr7;                                            
	/*0x028*/     struct _DESCRIPTOR Gdtr;       // 3 elements, 0x8 bytes (sizeof)   
	/*0x030*/     struct _DESCRIPTOR Idtr;       // 3 elements, 0x8 bytes (sizeof)   
	/*0x038*/     UINT16       Tr;                                                   
	/*0x03A*/     UINT16       Ldtr;                                                 
	/*0x03C*/     ULONG32      Reserved[6];                                          
          }KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;                              

typedef struct _KPROCESSOR_STATE                 // 2 elements, 0x320 bytes (sizeof)  
{                                                                                     
	/*0x000*/     struct _CONTEXT ContextFrame;                // 25 elements, 0x2CC bytes (sizeof) 
	/*0x2CC*/     struct _KSPECIAL_REGISTERS SpecialRegisters; // 15 elements, 0x54 bytes (sizeof)  
          }KPROCESSOR_STATE, *PKPROCESSOR_STATE;       


typedef struct _PP_LOOKASIDE_LIST // 2 elements, 0x8 bytes (sizeof) 
{                                                                   
	/*0x000*/     struct _GENERAL_LOOKASIDE* P;                                   
	/*0x004*/     struct _GENERAL_LOOKASIDE* L;                                   
          }PP_LOOKASIDE_LIST, *PPP_LOOKASIDE_LIST;    


typedef struct _FNSAVE_FORMAT      // 8 elements, 0x6C bytes (sizeof) 
{                                                                     
	/*0x000*/     ULONG32      ControlWord;                                         
	/*0x004*/     ULONG32      StatusWord;                                          
	/*0x008*/     ULONG32      TagWord;                                             
	/*0x00C*/     ULONG32      ErrorOffset;                                         
	/*0x010*/     ULONG32      ErrorSelector;                                       
	/*0x014*/     ULONG32      DataOffset;                                          
	/*0x018*/     ULONG32      DataSelector;                                        
	/*0x01C*/     UINT8        RegisterArea[80];                                    
          }FNSAVE_FORMAT, *PFNSAVE_FORMAT;   

typedef struct _FXSAVE_FORMAT       // 14 elements, 0x208 bytes (sizeof) 
{                                                                        
	/*0x000*/     UINT16       ControlWord;                                            
	/*0x002*/     UINT16       StatusWord;                                             
	/*0x004*/     UINT16       TagWord;                                                
	/*0x006*/     UINT16       ErrorOpcode;                                            
	/*0x008*/     ULONG32      ErrorOffset;                                            
	/*0x00C*/     ULONG32      ErrorSelector;                                          
	/*0x010*/     ULONG32      DataOffset;                                             
	/*0x014*/     ULONG32      DataSelector;                                           
	/*0x018*/     ULONG32      MXCsr;                                                  
	/*0x01C*/     ULONG32      MXCsrMask;                                              
	/*0x020*/     UINT8        RegisterArea[128];                                      
	/*0x0A0*/     UINT8        Reserved3[128];                                         
	/*0x120*/     UINT8        Reserved4[224];                                         
	/*0x200*/     UINT8        Align16Byte[8];                                         
          }FXSAVE_FORMAT, *PFXSAVE_FORMAT;             

typedef struct _FX_SAVE_AREA          // 3 elements, 0x210 bytes (sizeof)  
{                                                                          
	union                             // 2 elements, 0x208 bytes (sizeof)  
	{                                                                      
		/*0x000*/         struct _FNSAVE_FORMAT FnArea; // 8 elements, 0x6C bytes (sizeof)   
		/*0x000*/         struct _FXSAVE_FORMAT FxArea; // 14 elements, 0x208 bytes (sizeof) 
	}U;                                                                    
	/*0x208*/     ULONG32      NpxSavedCpu;                                              
	/*0x20C*/     ULONG32      Cr0NpxState;                                              
          }FX_SAVE_AREA, *PFX_SAVE_AREA; 


typedef struct _PROCESSOR_IDLE_TIMES     // 3 elements, 0x20 bytes (sizeof) 
{                                                                           
	/*0x000*/     UINT64       StartTime;                                                 
	/*0x008*/     UINT64       EndTime;                                                   
	/*0x010*/     ULONG32      IdleHandlerReserved[4];                                    
          }PROCESSOR_IDLE_TIMES, *PPROCESSOR_IDLE_TIMES;         

typedef struct _PROCESSOR_POWER_STATE          // 45 elements, 0x120 bytes (sizeof) 
{                                                                                   
	/*0x000*/     PVOID IdleFunction;                                                  
	/*0x004*/     ULONG32      Idle0KernelTimeLimit;                                              
	/*0x008*/     ULONG32      Idle0LastTime;                                                     
	/*0x00C*/     VOID*        IdleHandlers;                                                      
	/*0x010*/     VOID*        IdleState;                                                         
	/*0x014*/     ULONG32      IdleHandlersCount;                                                 
	/*0x018*/     UINT64       LastCheck;                                                         
	/*0x020*/     struct _PROCESSOR_IDLE_TIMES IdleTimes;    // 3 elements, 0x20 bytes (sizeof)   
	/*0x040*/     ULONG32      IdleTime1;                                                         
	/*0x044*/     ULONG32      PromotionCheck;                                                    
	/*0x048*/     ULONG32      IdleTime2;                                                         
	/*0x04C*/     UINT8        CurrentThrottle;                                                   
	/*0x04D*/     UINT8        ThermalThrottleLimit;                                              
	/*0x04E*/     UINT8        CurrentThrottleIndex;                                              
	/*0x04F*/     UINT8        ThermalThrottleIndex;                                              
	/*0x050*/     ULONG32      LastKernelUserTime;                                                
	/*0x054*/     ULONG32      LastIdleThreadKernelTime;                                          
	/*0x058*/     ULONG32      PackageIdleStartTime;                                              
	/*0x05C*/     ULONG32      PackageIdleTime;                                                   
	/*0x060*/     ULONG32      DebugCount;                                                        
	/*0x064*/     ULONG32      LastSysTime;                                                       
	/*0x068*/     UINT64       TotalIdleStateTime[3];                                             
	/*0x080*/     ULONG32      TotalIdleTransitions[3];                                           
	/*0x08C*/     UINT8        _PADDING0_[0x4];                                                   
	/*0x090*/     UINT64       PreviousC3StateTime;                                               
	/*0x098*/     UINT8        KneeThrottleIndex;                                                 
	/*0x099*/     UINT8        ThrottleLimitIndex;                                                
	/*0x09A*/     UINT8        PerfStatesCount;                                                   
	/*0x09B*/     UINT8        ProcessorMinThrottle;                                              
	/*0x09C*/     UINT8        ProcessorMaxThrottle;                                              
	/*0x09D*/     UINT8        EnableIdleAccounting;                                              
	/*0x09E*/     UINT8        LastC3Percentage;                                                  
	/*0x09F*/     UINT8        LastAdjustedBusyPercentage;                                        
	/*0x0A0*/     ULONG32      PromotionCount;                                                    
	/*0x0A4*/     ULONG32      DemotionCount;                                                     
	/*0x0A8*/     ULONG32      ErrorCount;                                                        
	/*0x0AC*/     ULONG32      RetryCount;                                                        
	/*0x0B0*/     ULONG32      Flags;                                                             
	/*0x0B4*/     UINT8        _PADDING1_[0x4];                                                   
	/*0x0B8*/     union _LARGE_INTEGER PerfCounterFrequency; // 4 elements, 0x8 bytes (sizeof)    
	/*0x0C0*/     ULONG32      PerfTickCount;                                                     
	/*0x0C4*/     UINT8        _PADDING2_[0x4];                                                   
	/*0x0C8*/     struct _KTIMER PerfTimer;                  // 5 elements, 0x28 bytes (sizeof)   
	/*0x0F0*/     struct _KDPC PerfDpc;                      // 9 elements, 0x20 bytes (sizeof)   
	/*0x110*/     PVOID PerfStates;                                       
	/*0x114*/     PVOID PerfSetThrottle;                                               
	/*0x118*/     ULONG32      LastC3KernelUserTime;                                              
	/*0x11C*/     ULONG32      LastPackageIdleTime;                                               
          }PROCESSOR_POWER_STATE, *PPROCESSOR_POWER_STATE;                 
// 
typedef struct _KPRCB                                    // 91 elements, 0xC50 bytes (sizeof) 
{                                                                                             
	/*0x000*/     UINT16       MinorVersion;                                                                
	/*0x002*/     UINT16       MajorVersion;                                                                
	/*0x004*/     struct _KTHREAD* CurrentThread;                                                           
	/*0x008*/     struct _KTHREAD* NextThread;                                                              
	/*0x00C*/     struct _KTHREAD* IdleThread;                                                              
	/*0x010*/     CHAR         Number;                                                                      
	/*0x011*/     CHAR         Reserved;                                                                    
	/*0x012*/     UINT16       BuildType;                                                                   
	/*0x014*/     ULONG32      SetMember;                                                                   
	/*0x018*/     CHAR         CpuType;                                                                     
	/*0x019*/     CHAR         CpuID;                                                                       
	/*0x01A*/     UINT16       CpuStep;                                                                     
	/*0x01C*/     struct _KPROCESSOR_STATE ProcessorState;             // 2 elements, 0x320 bytes (sizeof)  
	/*0x33C*/     ULONG32      KernelReserved[16];                                                          
	/*0x37C*/     ULONG32      HalReserved[16];                                                             
	/*0x3BC*/     UINT8        PrcbPad0[92];                                                                
	/*0x418*/     struct _KSPIN_LOCK_QUEUE LockQueue[16];                                                   
	/*0x498*/     UINT8        PrcbPad1[8];                                                                 
	/*0x4A0*/     struct _KTHREAD* NpxThread;                                                               
	/*0x4A4*/     ULONG32      InterruptCount;                                                              
	/*0x4A8*/     ULONG32      KernelTime;                                                                  
	/*0x4AC*/     ULONG32      UserTime;                                                                    
	/*0x4B0*/     ULONG32      DpcTime;                                                                     
	/*0x4B4*/     ULONG32      DebugDpcTime;                                                                
	/*0x4B8*/     ULONG32      InterruptTime;                                                               
	/*0x4BC*/     ULONG32      AdjustDpcThreshold;                                                          
	/*0x4C0*/     ULONG32      PageColor;                                                                   
	/*0x4C4*/     ULONG32      SkipTick;                                                                    
	/*0x4C8*/     UINT8        MultiThreadSetBusy;                                                          
	/*0x4C9*/     UINT8        Spare2[3];                                                                   
	/*0x4CC*/     PVOID ParentNode;                                                                
	/*0x4D0*/     ULONG32      MultiThreadProcessorSet;                                                     
	/*0x4D4*/     struct _KPRCB* MultiThreadSetMaster;                                                      
	/*0x4D8*/     ULONG32      ThreadStartCount[2];                                                         
	/*0x4E0*/     ULONG32      CcFastReadNoWait;                                                            
	/*0x4E4*/     ULONG32      CcFastReadWait;                                                              
	/*0x4E8*/     ULONG32      CcFastReadNotPossible;                                                       
	/*0x4EC*/     ULONG32      CcCopyReadNoWait;                                                            
	/*0x4F0*/     ULONG32      CcCopyReadWait;                                                              
	/*0x4F4*/     ULONG32      CcCopyReadNoWaitMiss;                                                        
	/*0x4F8*/     ULONG32      KeAlignmentFixupCount;                                                       
	/*0x4FC*/     ULONG32      KeContextSwitches;                                                           
	/*0x500*/     ULONG32      KeDcacheFlushCount;                                                          
	/*0x504*/     ULONG32      KeExceptionDispatchCount;                                                    
	/*0x508*/     ULONG32      KeFirstLevelTbFills;                                                         
	/*0x50C*/     ULONG32      KeFloatingEmulationCount;                                                    
	/*0x510*/     ULONG32      KeIcacheFlushCount;                                                          
	/*0x514*/     ULONG32      KeSecondLevelTbFills;                                                        
	/*0x518*/     ULONG32      KeSystemCalls;                                                               
	/*0x51C*/     ULONG32      SpareCounter0[1];                                                            
	/*0x520*/     struct _PP_LOOKASIDE_LIST PPLookasideList[16];                                            
	/*0x5A0*/     struct _PP_LOOKASIDE_LIST PPNPagedLookasideList[32];                                      
	/*0x6A0*/     struct _PP_LOOKASIDE_LIST PPPagedLookasideList[32];                                       
	/*0x7A0*/     ULONG32      PacketBarrier;                                                               
	/*0x7A4*/     ULONG32      ReverseStall;                                                                
	/*0x7A8*/     VOID*        IpiFrame;                                                                    
	/*0x7AC*/     UINT8        PrcbPad2[52];                                                                
	/*0x7E0*/     VOID*        CurrentPacket[3];                                                            
	/*0x7EC*/     ULONG32      TargetSet;                                                                   
	/*0x7F0*/     PVOID WorkerRoutine;                                                           
	/*0x7F4*/     ULONG32      IpiFrozen;                                                                   
	/*0x7F8*/     UINT8        PrcbPad3[40];                                                                
	/*0x820*/     ULONG32      RequestSummary;                                                              
	/*0x824*/     struct _KPRCB* SignalDone;                                                                
	/*0x828*/     UINT8        PrcbPad4[56];                                                                
	/*0x860*/     struct _LIST_ENTRY DpcListHead;                      // 2 elements, 0x8 bytes (sizeof)    
	/*0x868*/     VOID*        DpcStack;                                                                    
	/*0x86C*/     ULONG32      DpcCount;                                                                    
	/*0x870*/     ULONG32      DpcQueueDepth;                                                               
	/*0x874*/     ULONG32      DpcRoutineActive;                                                            
	/*0x878*/     ULONG32      DpcInterruptRequested;                                                       
	/*0x87C*/     ULONG32      DpcLastCount;                                                                
	/*0x880*/     ULONG32      DpcRequestRate;                                                              
	/*0x884*/     ULONG32      MaximumDpcQueueDepth;                                                        
	/*0x888*/     ULONG32      MinimumDpcRate;                                                              
	/*0x88C*/     ULONG32      QuantumEnd;                                                                  
	/*0x890*/     UINT8        PrcbPad5[16];                                                                
	/*0x8A0*/     ULONG32      DpcLock;                                                                     
	/*0x8A4*/     UINT8        PrcbPad6[28];                                                                
	/*0x8C0*/     struct _KDPC CallDpc;                                // 9 elements, 0x20 bytes (sizeof)   
	/*0x8E0*/     VOID*        ChainedInterruptList;                                                        
	/*0x8E4*/     LONG32       LookasideIrpFloat;                                                           
	/*0x8E8*/     ULONG32      SpareFields0[6];                                                             
	/*0x900*/     UINT8        VendorString[13];                                                            
	/*0x90D*/     UINT8        InitialApicId;                                                               
	/*0x90E*/     UINT8        LogicalProcessorsPerPhysicalProcessor;                                       
	/*0x90F*/     UINT8        _PADDING0_[0x1];                                                             
	/*0x910*/     ULONG32      MHz;                                                                         
	/*0x914*/     ULONG32      FeatureBits;                                                                 
	/*0x918*/     union _LARGE_INTEGER UpdateSignature;                // 4 elements, 0x8 bytes (sizeof)    
	/*0x920*/     struct _FX_SAVE_AREA NpxSaveArea;                    // 3 elements, 0x210 bytes (sizeof)  
	/*0xB30*/     struct _PROCESSOR_POWER_STATE PowerState;            // 45 elements, 0x120 bytes (sizeof) 
          }KPRCB, *PKPRCB;   

//
// Define Address of Processor Control Registers.
//
#ifndef KIPCR
// #define KREGION_INDEX 7
// 
// #define KADDRESS_BASE ((ULONGLONG)(KREGION_INDEX) << 61)
// 
// #define KIPCR ((ULONG_PTR)(KADDRESS_BASE + 0xffff0000))            // kernel address of first PCR

//
// Define Pointer to Processor Control Registers.
//


//#define _PCR   fs:[0] 

//#define PCR ((volatile KPCR * const)KIPCR)	//不要这个
inline	struct _KPRCB * KeGetCurrentPrcb()
{
	KPCR *	pcr=NULL;
	__asm mov eax,fs:[0] 
	__asm mov pcr,eax
	return pcr->Prcb;
}
//#define KeGetCurrentPrcb() PCR->Prcb
#endif

typedef struct _LDT_ENTRY {
    USHORT  LimitLow;
    USHORT  BaseLow;
    union {
        struct {
            UCHAR   BaseMid;
            UCHAR   Flags1;     // Declare as bytes to avoid alignment
            UCHAR   Flags2;     // Problems.
            UCHAR   BaseHi;
        } Bytes;
        struct {
            ULONG   BaseMid : 8;
            ULONG   Type : 5;
            ULONG   Dpl : 2;
            ULONG   Pres : 1;
            ULONG   LimitHi : 4;
            ULONG   Sys : 1;
            ULONG   Reserved_0 : 1;
            ULONG   Default_Big : 1;
            ULONG   Granularity : 1;
            ULONG   BaseHi : 8;
        } Bits;
    } HighWord;
} LDT_ENTRY, *PLDT_ENTRY;

typedef struct _DESCRIPTOR_TABLE_ENTRY {
    ULONG Selector;
    LDT_ENTRY Descriptor;
} DESCRIPTOR_TABLE_ENTRY, *PDESCRIPTOR_TABLE_ENTRY;

// end_windbgkd
#define LDRP_STATIC_LINK                0x00000002
#define LDRP_IMAGE_DLL                  0x00000004
#define LDRP_LOAD_IN_PROGRESS           0x00001000
#define LDRP_UNLOAD_IN_PROGRESS         0x00002000
#define LDRP_ENTRY_PROCESSED            0x00004000
#define LDRP_ENTRY_INSERTED             0x00008000
#define LDRP_CURRENT_LOAD               0x00010000
#define LDRP_FAILED_BUILTIN_LOAD        0x00020000
#define LDRP_DONT_CALL_FOR_THREADS      0x00040000
#define LDRP_PROCESS_ATTACH_CALLED      0x00080000
#define LDRP_DEBUG_SYMBOLS_LOADED       0x00100000
#define LDRP_IMAGE_NOT_AT_BASE          0x00200000
#define LDRP_COR_IMAGE                  0x00400000
#define LDRP_COR_OWNS_UNMAP             0x00800000
#define LDRP_SYSTEM_MAPPED              0x01000000
#define LDRP_IMAGE_VERIFYING            0x02000000
#define LDRP_DRIVER_DEPENDENT_DLL       0x04000000
#define LDRP_ENTRY_NATIVE               0x08000000
#define LDRP_REDIRECTED                 0x10000000
#define LDRP_NON_PAGED_DEBUG_INFO       0x20000000
#define LDRP_MM_LOADED                  0x40000000
#define LDRP_COMPAT_DATABASE_PROCESSED  0x80000000

//
// Loader Data Table. Used to track DLLs loaded into an
// image.
//

typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    union {
        LIST_ENTRY HashLinks;
        struct {
            PVOID SectionPointer;
            ULONG CheckSum;
        };
    };
    union {
        struct {
            ULONG TimeDateStamp;
        };
        struct {
            PVOID LoadedImports;
        };
    };
    PVOID EntryPointActivationContext;
	
    PVOID PatchInformation;
	
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;


                           
typedef struct _KTRAP_FRAME                               // 35 elements, 0x8C bytes (sizeof) 
          {                                                                                             
/*0x000*/     ULONG32      DbgEbp;                                                                      
/*0x004*/     ULONG32      DbgEip;                                                                      
/*0x008*/     ULONG32      DbgArgMark;                                                                  
/*0x00C*/     ULONG32      DbgArgPointer;                                                               
/*0x010*/     ULONG32      TempSegCs;                                                                   
/*0x014*/     ULONG32      TempEsp;                                                                     
/*0x018*/     ULONG32      Dr0;                                                                         
/*0x01C*/     ULONG32      Dr1;                                                                         
/*0x020*/     ULONG32      Dr2;                                                                         
/*0x024*/     ULONG32      Dr3;                                                                         
/*0x028*/     ULONG32      Dr6;                                                                         
/*0x02C*/     ULONG32      Dr7;                                                                         
/*0x030*/     ULONG32      SegGs;                                                                       
/*0x034*/     ULONG32      SegEs;                                                                       
/*0x038*/     ULONG32      SegDs;                                                                       
/*0x03C*/     ULONG32      Edx;                                                                         
/*0x040*/     ULONG32      Ecx;                                                                         
/*0x044*/     ULONG32      Eax;                                                                         
/*0x048*/     ULONG32      PreviousPreviousMode;                                                        
/*0x04C*/     struct _EXCEPTION_REGISTRATION_RECORD* ExceptionList;                                     
/*0x050*/     ULONG32      SegFs;                                                                       
/*0x054*/     ULONG32      Edi;                                                                         
/*0x058*/     ULONG32      Esi;                                                                         
/*0x05C*/     ULONG32      Ebx;                                                                         
/*0x060*/     ULONG32      Ebp;                                                                         
/*0x064*/     ULONG32      ErrCode;                                                                     
/*0x068*/     ULONG32      Eip;                                                                         
/*0x06C*/     ULONG32      SegCs;                                                                       
/*0x070*/     ULONG32      EFlags;                                                                      
/*0x074*/     ULONG32      HardwareEsp;                                                                 
/*0x078*/     ULONG32      HardwareSegSs;                                                               
/*0x07C*/     ULONG32      V86Es;                                                                       
/*0x080*/     ULONG32      V86Ds;                                                                       
/*0x084*/     ULONG32      V86Fs;                                                                       
/*0x088*/     ULONG32      V86Gs;                                                                       
          }KTRAP_FRAME, *PKTRAP_FRAME;               
		  
		  typedef KTRAP_FRAME *PKTRAP_FRAME;
typedef KTRAP_FRAME *PKEXCEPTION_FRAME;
typedef KTRAP_FRAME KEXCEPTION_FRAME;
		  
		  
#define KTRAP_FRAME_LENGTH  (sizeof(KTRAP_FRAME))
#define KTRAP_FRAME_ALIGN   (sizeof(ULONG))
#define KTRAP_FRAME_ROUND   (KTRAP_FRAME_ALIGN-1)
		  // Process Specific Access Rights
		  //
		  
#define PROCESS_TERMINATE         (0x0001)  // winnt
#define PROCESS_CREATE_THREAD     (0x0002)  // winnt
#define PROCESS_SET_SESSIONID     (0x0004)  // winnt
#define PROCESS_VM_OPERATION      (0x0008)  // winnt
#define PROCESS_VM_READ           (0x0010)  // winnt
#define PROCESS_VM_WRITE          (0x0020)  // winnt
		  // begin_ntddk begin_wdm begin_ntifs
#define PROCESS_DUP_HANDLE        (0x0040)  // winnt
		  // end_ntddk end_wdm end_ntifs
#define PROCESS_CREATE_PROCESS    (0x0080)  // winnt
#define PROCESS_SET_QUOTA         (0x0100)  // winnt
#define PROCESS_SET_INFORMATION   (0x0200)  // winnt
#define PROCESS_QUERY_INFORMATION (0x0400)  // winnt
#define PROCESS_SET_PORT          (0x0800)
#define PROCESS_SUSPEND_RESUME    (0x0800)  // winnt
		  
		  // begin_winnt begin_ntddk begin_wdm begin_ntifs
#define PROCESS_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
		  0xFFF)
		  // begin_nthal
		  
		  
		  
		  
		  
#ifdef _NTDRIVER_
		  extern POBJECT_TYPE *ExEventPairObjectType;
		  extern POBJECT_TYPE *PsProcessType;
		  extern POBJECT_TYPE *PsThreadType;
		  extern POBJECT_TYPE *PsJobType;
		  extern POBJECT_TYPE *LpcPortObjectType;
		  extern POBJECT_TYPE *LpcWaitablePortObjectType;
#else
		  extern POBJECT_TYPE ExEventPairObjectType;
		  
		  extern POBJECT_TYPE *PsProcessType;
	      extern POBJECT_TYPE KODbgkDebugObjectType;
		  extern POBJECT_TYPE *PsThreadType;
		  extern POBJECT_TYPE *MmSectionObjectType;
		  
		  extern POBJECT_TYPE PsJobType;
		  extern POBJECT_TYPE LpcPortObjectType;
		  extern POBJECT_TYPE LpcWaitablePortObjectType;
#endif // _NTDRIVER
		  

		  extern ULONG PsPrioritySeparation;
		  extern ULONG PsRawPrioritySeparation;
		  extern LIST_ENTRY PsActiveProcessHead;

		  extern PVOID PsSystemDllBase;

		  extern PVOID PsNtosImageBase;
			extern PVOID PsHalImageBase;
		  extern const UNICODE_STRING PsNtDllPathName;
		  extern ULONG DbgkDebugObjectType;

//		  		  extern PEPROCESS PsInitialSystemProcess;
// 
// typedef
// BOOLEAN
// (WINAPI *PKDEBUG_ROUTINE) (
// 						   IN PKTRAP_FRAME TrapFrame,
// 						   IN PKEXCEPTION_FRAME ExceptionFrame,
// 						   IN PEXCEPTION_RECORD ExceptionRecord,
// 						   IN PCONTEXT ContextRecord,
// 						   IN KPROCESSOR_MODE PreviousMode,
// 						   IN BOOLEAN SecondChance
// 					);

// 
// #if defined(_AMD64_)
// 
// extern INVERTED_FUNCTION_TABLE PsInvertedFunctionTable;
// 
// #endif

extern PLIST_ENTRY PsLoadedModuleList;
extern ERESOURCE PsLoadedModuleResource;
//extern ALIGNED_SPINLOCK PsLoadedModuleSpinLock;
extern LCID PsDefaultSystemLocaleId;
extern LCID PsDefaultThreadLocaleId;
extern LANGID PsDefaultUILanguageId;
extern LANGID PsInstallUILanguageId;
extern PEPROCESS PsIdleProcess;
extern SINGLE_LIST_ENTRY PsReaperListHead;
extern WORK_QUEUE_ITEM PsReaperWorkItem;

extern BOOLEAN PsImageNotifyEnabled;
extern SINGLE_LIST_ENTRY KiProcessInSwapListHead;
extern SINGLE_LIST_ENTRY KiProcessOutSwapListHead;
extern KEVENT KiSwapEvent;
extern PKTHREAD KiSwappingThread;
extern LUID    SeDebugPrivilege;
extern PUCHAR KeI386XMMIPresent;
extern PULONG KeFeatureBits;
// extern PKDEBUG_ROUTINE KiDebugRoutine;
extern PVOID KeUserExceptionDispatcher;




//#define KeGetCurrentPrcb() PCR->Prcb
#ifndef MAXIMUM_PROCESSORS
#define MAXIMUM_PROCESSORS 32
#endif


#define THREAD_TERMINATE               (0x0001)  
#define THREAD_SUSPEND_RESUME          (0x0002)  
#define THREAD_GET_CONTEXT             (0x0008)  
#define THREAD_SET_CONTEXT             (0x0010)  
#define THREAD_SET_INFORMATION         (0x0020)  
#define THREAD_QUERY_INFORMATION       (0x0040)  
#define THREAD_SET_THREAD_TOKEN        (0x0080)
#define THREAD_IMPERSONATE             (0x0100)
#define THREAD_DIRECT_IMPERSONATION    (0x0200)


/* Local Memory Flags */
#define LMEM_FIXED          0x0000
#define LMEM_MOVEABLE       0x0002
#define LMEM_NOCOMPACT      0x0010
#define LMEM_NODISCARD      0x0020
#define LMEM_ZEROINIT       0x0040
#define LMEM_MODIFY         0x0080
#define LMEM_DISCARDABLE    0x0F00
#define LMEM_VALID_FLAGS    0x0F72
#define LMEM_INVALID_HANDLE 0x8000

#define LHND                (LMEM_MOVEABLE | LMEM_ZEROINIT)
#define LPTR                (LMEM_FIXED | LMEM_ZEROINIT)

#define NONZEROLHND         (LMEM_MOVEABLE)
#define NONZEROLPTR         (LMEM_FIXED)

#define LocalDiscard( h )   LocalReAlloc( (h), 0, LMEM_MOVEABLE )

/* Flags returned by LocalFlags (in addition to LMEM_DISCARDABLE) */
#define LMEM_DISCARDED      0x4000
#define LMEM_LOCKCOUNT      0x00FF

#define KiRequestSoftwareInterrupt(RequestIrql) \
    HalRequestSoftwareInterrupt( RequestIrql )

#define KeYieldProcessor()    __asm { rep nop }

#if defined(_AMD64_)

FORCEINLINE
VOID
ProbeForWriteHandle (
					 IN PHANDLE Address
					 )
					 
{
	
    if (Address >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {
        Address = (HANDLE * const)MM_USER_PROBE_ADDRESS;
    }
	
    *((volatile HANDLE *)Address) = *Address;
    return;
}

#else

#define ProbeForWriteHandle(Address) {                                       \
    if ((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {                \
	*(volatile HANDLE * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
	\
    *(volatile HANDLE *)(Address) = *(volatile HANDLE *)(Address);           \
}

#endif

//
// This is a block held on the local stack of the waiting threads.
//
typedef  struct DECLSPEC_ALIGN(16) _EX_PUSH_LOCK_WAIT_BLOCK *PEX_PUSH_LOCK_WAIT_BLOCK;

typedef struct DECLSPEC_ALIGN(16) _EX_PUSH_LOCK_WAIT_BLOCK {
    union {
        KGATE WakeGate;
        KEVENT WakeEvent;
    };
    PEX_PUSH_LOCK_WAIT_BLOCK Next;
    PEX_PUSH_LOCK_WAIT_BLOCK Last;
    PEX_PUSH_LOCK_WAIT_BLOCK Previous;
    LONG ShareCount;
	
#define EX_PUSH_LOCK_FLAGS_EXCLUSIVE  (0x1)
#define EX_PUSH_LOCK_FLAGS_SPINNING_V (0x1)
#define EX_PUSH_LOCK_FLAGS_SPINNING   (0x2)
    LONG Flags;
	
#if DBG
    BOOLEAN Signaled;
    PVOID OldValue;
    PVOID NewValue;
    PEX_PUSH_LOCK PushLock;
#endif
} DECLSPEC_ALIGN(16) EX_PUSH_LOCK_WAIT_BLOCK;


typedef
VOID
(*PPS_APC_ROUTINE) (
					__in_opt PVOID ApcArgument1,
					__in_opt PVOID ApcArgument2,
					__in_opt PVOID ApcArgument3
    );

#if defined(_WIN64)

#if (defined(_AMD64_) && !defined(_X86AMD64_))

#define KeFindFirstSetLeftAffinity(Set, Member) BitScanReverse64(Member, Set)

#else

#define KeFindFirstSetLeftAffinity(Set, Member) {                      \
    ULONG _Mask_;                                                      \
    ULONG _Offset_ = 32;                                               \
    if ((_Mask_ = (ULONG)(Set >> 32)) == 0) {                          \
	_Offset_ = 0;                                                  \
	_Mask_ = (ULONG)Set;                                           \
    }                                                                  \
    KeFindFirstSetLeftMember(_Mask_, Member);                          \
    *(Member) += _Offset_;                                             \
}

#endif
#endif


typedef struct _AUX_ACCESS_DATA {
    PPRIVILEGE_SET PrivilegesUsed;
    GENERIC_MAPPING GenericMapping;
    ACCESS_MASK AccessesToAudit;
    ACCESS_MASK MaximumAuditMask;
} AUX_ACCESS_DATA, *PAUX_ACCESS_DATA;

typedef enum _KPROCESS_STATE {
        ProcessInMemory,
		ProcessOutOfMemory,
		ProcessInTransition,
		ProcessOutTransition,
		ProcessInSwap,
		ProcessOutSwap
} KPROCESS_STATE;

typedef struct _USER_STACK
{
	
	PVOID  FixedStackBase;
	
	PVOID  FixedStackLimit;
	
	PVOID  ExpandableStackBase;
	
	PVOID  ExpandableStackLimit;
	
	PVOID  ExpandableStackBottom;
	
} USER_STACK, *PUSER_STACK;


#define CONTEXT_LENGTH  (sizeof(CONTEXT))
#define CONTEXT_ALIGN   (sizeof(ULONG))
#define CONTEXT_ROUND   (CONTEXT_ALIGN - 1)

typedef struct _INITIAL_TEB {
    struct {
        PVOID OldStackBase;
        PVOID OldStackLimit;
    } OldInitialTeb;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID StackAllocationBase;
} INITIAL_TEB, *PINITIAL_TEB;


//
// Sanitize segCS and eFlags based on a processor mode.
//
// If kernel mode,
//      force CPL == 0
//
// If user mode,
//      force CPL == 3
//

#define SANITIZE_SEG(segCS, mode) (\
    ((mode) == KernelMode ? \
	((0x00000000L) | ((segCS) & 0xfffc)) : \
((0x00000003L) | ((segCS) & 0xffff))))


#define PS_CROSS_THREAD_FLAGS_TERMINATED           0x00000001UL

//
// Thread create failed
//

#define PS_CROSS_THREAD_FLAGS_DEADTHREAD           0x00000002UL

//
// Debugger isn't shown this thread
//

#define PS_CROSS_THREAD_FLAGS_HIDEFROMDBG          0x00000004UL

//
// Thread is impersonating
//

#define PS_CROSS_THREAD_FLAGS_IMPERSONATING        0x00000008UL

//
// This is a system thread
//

#define PS_CROSS_THREAD_FLAGS_SYSTEM               0x00000010UL

//
// Hard errors are disabled for this thread
//

#define PS_CROSS_THREAD_FLAGS_HARD_ERRORS_DISABLED 0x00000020UL

//
// We should break in when this thread is terminated
//

#define PS_CROSS_THREAD_FLAGS_BREAK_ON_TERMINATION 0x00000040UL

//
// This thread should skip sending its create thread message
//
#define PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG    0x00000080UL

//
// This thread should skip sending its final thread termination message
//
#define PS_CROSS_THREAD_FLAGS_SKIP_TERMINATION_MSG 0x00000100UL


#define IS_SYSTEM_THREAD(Thread)  (((Thread)->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_SYSTEM) != 0)

#define PS_TEST_SET_BITS(Flags, Flag) \
    RtlInterlockedSetBits (Flags, Flag)


#define PS_SET_BITS(Flags, Flag) \
    RtlInterlockedSetBitsDiscardReturn (Flags, Flag)

 KIRQL __fastcall KeAcquireQueuedSpinLockRaiseToSynch(IN KSPIN_LOCK_QUEUE_NUMBER  Number);

#if defined(_AMD64_)

FORCEINLINE
VOID
ProbeForWritePointer (
					  IN PVOID *Address
					  )
					  
{
	
    if (Address >= (PVOID * const)MM_USER_PROBE_ADDRESS) {
        Address = (PVOID * const)MM_USER_PROBE_ADDRESS;
    }
	
    *((volatile PVOID *)Address) = *Address;
    return;
}

#else

#define ProbeForWritePointer(Address) {                                      \
    if ((PVOID *)(Address) >= (PVOID * const)MM_USER_PROBE_ADDRESS) {        \
	*(volatile PVOID * const)MM_USER_PROBE_ADDRESS = NULL;               \
    }                                                                        \
	\
    *(volatile PVOID *)(Address) = *(volatile PVOID *)(Address);             \
}

#endif


typedef enum _KAPC_ENVIRONMENT {
        OriginalApcEnvironment,
		AttachedApcEnvironment,
		CurrentApcEnvironment,
		InsertApcEnvironment
} KAPC_ENVIRONMENT;


NTKERNELAPI int ExSystemExceptionFilter(VOID);
typedef struct _OBJECT_TYPE_INITIALIZER
{
	USHORT Length;
	BOOLEAN UseDefaultObject;
	BOOLEAN CaseInsensitive;
	ULONG InvalidAttributes;
	GENERIC_MAPPING GenericMapping;
	ULONG ValidAccessMask;
	BOOLEAN SecurityRequired;
	BOOLEAN MaintainHandleCount;
	BOOLEAN MaintainTypeList;
	POOL_TYPE PoolType;
	ULONG DefaultPagedPoolCharge;
	ULONG DefaultNonPagedPoolCharge;
	PVOID DumpProcedure;
	PVOID OpenProcedure;
	PVOID CloseProcedure;
	PVOID DeleteProcedure;
	PVOID ParseProcedure;
	PVOID SecurityProcedure;
	PVOID QueryNameProcedure;
	PVOID OkayToCloseProcedure;
} OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER;

NTKERNELAPI NTSTATUS
ObCreateObjectType (
					__in PUNICODE_STRING TypeName,
					__in POBJECT_TYPE_INITIALIZER ObjectTypeInitializer,
					__in_opt PSECURITY_DESCRIPTOR SecurityDescriptor,
					__out POBJECT_TYPE *ObjectType
    );
NTKERNELAPI
NTSTATUS
ObOpenObjectByName(
				   __in POBJECT_ATTRIBUTES ObjectAttributes,
				   __in_opt POBJECT_TYPE ObjectType,
				   __in KPROCESSOR_MODE AccessMode,
				   __inout_opt PACCESS_STATE AccessState,
				   __in_opt ACCESS_MASK DesiredAccess,
				   __inout_opt PVOID ParseContext,
				   __out PHANDLE Handle
    );


NTKERNELAPI
VOID
NTAPI
ExRaiseException (
				  __in PEXCEPTION_RECORD ExceptionRecord
    );

NTKERNELAPI UCHAR * PsGetProcessImageFileName( __in PEPROCESS Process );

#define CONTEXT_ALIGNED_SIZE ((sizeof(CONTEXT) + CONTEXT_ROUND) & ~CONTEXT_ROUND)

#if defined(_AMD64_)

FORCEINLINE
VOID
ProbeForReadSmallStructure (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG Alignment
    )

/*++

Routine Description:

    Probes a structure for read access whose size is known at compile time.

    N.B. A NULL structure address is not allowed.

Arguments:

    Address - Supples a pointer to the structure.

    Size - Supplies the size of the structure.

    Alignment - Supplies the alignment of structure.

Return Value:

    None

--*/

{

    ASSERT((Alignment == 1) || (Alignment == 2) ||
           (Alignment == 4) || (Alignment == 8) ||
           (Alignment == 16));

    if ((Size == 0) || (Size >= 0x10000)) {

        ASSERT(0);

        ProbeForRead(Address, Size, Alignment);

    } else {
        if (((ULONG_PTR)Address & (Alignment - 1)) != 0) {
            ExRaiseDatatypeMisalignment();
        }

        if ((PUCHAR)Address >= (UCHAR * const)MM_USER_PROBE_ADDRESS) {
            Address = (UCHAR * const)MM_USER_PROBE_ADDRESS;
        }

        _ReadWriteBarrier();
        *(volatile UCHAR *)Address;
    }
}

#else

#define ProbeForReadSmallStructure(Address, Size, Alignment) {               \
    ASSERT(((Alignment) == 1) || ((Alignment) == 2) ||                       \
           ((Alignment) == 4) || ((Alignment) == 8) ||                       \
           ((Alignment) == 16));                                             \
    if ((Size == 0) || (Size > 0x10000)) {                                   \
        ASSERT(0);                                                           \
        ProbeForRead(Address, Size, Alignment);                              \
    } else {                                                                 \
        if (((ULONG_PTR)(Address) & ((Alignment) - 1)) != 0) {               \
            ExRaiseDatatypeMisalignment();                                   \
        }                                                                    \
        if ((ULONG_PTR)(Address) >= (ULONG_PTR)MM_USER_PROBE_ADDRESS) {      \
            *(volatile UCHAR * const)MM_USER_PROBE_ADDRESS = 0;              \
        }                                                                    \
    }                                                                        \
}

#endif


#if defined(_AMD64_)

FORCEINLINE
VOID
ProbeForWriteUlong (
					IN PULONG Address
					)
					
{
	
    if (Address >= (ULONG * const)MM_USER_PROBE_ADDRESS) {
        Address = (ULONG * const)MM_USER_PROBE_ADDRESS;
    }
	
    *((volatile ULONG *)Address) = *Address;
    return;
}

#else

#define ProbeForWriteUlong(Address) {                                        \
    if ((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) {                 \
	*(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
	\
    *(volatile ULONG *)(Address) = *(volatile ULONG *)(Address);             \
}

#endif

#define PsGetCurrentProcessByThread(xCurrentThread) (ASSERT((xCurrentThread) == PsGetCurrentThread ()),CONTAINING_RECORD(((xCurrentThread)->Tcb.ApcState.Process),EPROCESS,Pcb))


#define KAPC_STATE_ACTUAL_LENGTH                                             \
    (FIELD_OFFSET(KAPC_STATE, UserApcPending) + sizeof(BOOLEAN))

typedef enum _KTHREAD_STATE {
    Initialized,
		Ready,
		Running,
		Standby,
		Terminated,
		Waiting,
		Transition,
		DeferredReady,
		GateWait
} KTHREAD_STATE;


#if defined(_AMD64_)

FORCEINLINE
VOID
ProbeForWriteUlong_ptr (
						IN PULONG_PTR Address
						)
						
{
	
    if (Address >= (ULONG_PTR * const)MM_USER_PROBE_ADDRESS) {
        Address = (ULONG_PTR * const)MM_USER_PROBE_ADDRESS;
    }
	
    *((volatile ULONG_PTR *)Address) = *Address;
    return;
}

#else

#define ProbeForWriteUlong_ptr(Address) {                                    \
    if ((Address) >= (ULONG_PTR * const)MM_USER_PROBE_ADDRESS) {             \
	*(volatile ULONG_PTR * const)MM_USER_PROBE_ADDRESS = 0;              \
    }                                                                        \
	\
    *(volatile ULONG_PTR *)(Address) = *(volatile ULONG_PTR *)(Address);     \
}

#endif


typedef enum _KOBJECTS {
    EventNotificationObject = 0,
		EventSynchronizationObject = 1,
		MutantObject = 2,
		ProcessObject = 3,
		QueueObject = 4,
		SemaphoreObject = 5,
		ThreadObject = 6,
		GateObject = 7,
		TimerNotificationObject = 8,
		TimerSynchronizationObject = 9,
		Spare2Object = 10,
		Spare3Object = 11,
		Spare4Object = 12,
		Spare5Object = 13,
		Spare6Object = 14,
		Spare7Object = 15,
		Spare8Object = 16,
		Spare9Object = 17,
		ApcObject,
		DpcObject,
		DeviceQueueObject,
		EventPairObject,
		InterruptObject,
		ProfileObject,
		ThreadedDpcObject,
		MaximumKernelObject
} KOBJECTS;
extern "C"
VOID  ProbeForWriteSmallStructure (
							 IN PVOID Address,
							 IN SIZE_T Size,
							 IN ULONG Alignment
							 );

extern 
NTSTATUS  (WINAPI *PsResumeThread)(IN PETHREAD Thread,OUT PULONG PreviousSuspendCount OPTIONAL);
extern 
NTSTATUS  (WINAPI *PsTerminateProcess)(IN PEPROCESS Process,IN NTSTATUS Status);
extern 
PEPROCESS (WINAPI *PsGetNextProcess) (IN PEPROCESS Process);
extern 
PETHREAD  (WINAPI *PsGetNextProcessThread) (IN PEPROCESS Process,IN PETHREAD Thread);
extern NTSTATUS  (WINAPI *PsSuspendThread) (IN PETHREAD Thread,OUT PULONG PreviousSuspendCount );
extern NTSTATUS  (WINAPI *MmGetFileNameForAddress) (IN PVOID ProcessVa,OUT PUNICODE_STRING FileName);
extern NTSTATUS  (WINAPI *ObDuplicateObject) (IN PEPROCESS SourceProcess,IN HANDLE SourceHandle,IN PEPROCESS TargetProcess OPTIONAL,OUT PHANDLE TargetHandle OPTIONAL,IN ACCESS_MASK DesiredAccess,IN ULONG HandleAttributes,IN ULONG Options,IN KPROCESSOR_MODE PreviousMode );

extern NTSTATUS (WINAPI *LpcRequestWaitReplyPortEx) (IN PVOID PortAddress,IN PPORT_MESSAGE RequestMessage,OUT PPORT_MESSAGE ReplyMessage);
extern VOID      (WINAPI *PsQuitNextProcessThread) (IN PETHREAD Thread);
extern	VOID      (WINAPI *PsCallImageNotifyRoutines)(IN PUNICODE_STRING FullImageName,IN HANDLE ProcessId, IN PIMAGE_INFO ImageInfo );
extern FAST_MUTEX *DbgkpProcessDebugPortMutex;
extern	VOID      (WINAPI *KeFreezeAllThreads) (VOID);
extern	VOID      (WINAPI *KeThawAllThreads) (VOID);
extern	VOID      (FASTCALL  *KiReadyThread)(IN PKTHREAD Thread);
extern	VOID      (WINAPI * KiSetSwapEvent)();
extern	BOOLEAN   (WINAPI *KiSwapProcess) (IN PKPROCESS NewProcess,IN PKPROCESS OldProcess);
extern	int       (FASTCALL *KiSwapThread) ();
extern	NTSTATUS  (FASTCALL *MiMakeProtectionMask)(unsigned int a1);
extern	NTSTATUS  (WINAPI*MiProtectVirtualMemory) (IN PEPROCESS Process,IN PVOID *BaseAddress,IN PULONG RegionSize,IN ULONG NewProtectWin32,IN PULONG LastProtect);
extern	NTSTATUS  (WINAPI *PspCreateThread)(OUT PHANDLE ThreadHandle,IN ACCESS_MASK DesiredAccess,IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,IN HANDLE ProcessHandle,IN PEPROCESS ProcessPointer,OUT PCLIENT_ID ClientId OPTIONAL,IN PCONTEXT ThreadContext OPTIONAL,IN PINITIAL_TEB InitialTeb OPTIONAL,IN BOOLEAN CreateSuspended,IN PKSTART_ROUTINE StartRoutine OPTIONAL,IN PVOID StartContext);
extern	void      (WINAPI *IopDeallocateApc)(PVOID P, int a2, int a3, int a4, int a5);


extern NTSTATUS  (WINAPI *MmGetFileNameForSection) (IN PVOID SectionObject,OUT POBJECT_NAME_INFORMATION *FileNameInfo);
extern VOID (FASTCALL *HalRequestSoftwareInterrupt) (KIRQL RequestIrql);
extern VOID (FASTCALL *KiUnlockDispatcherDatabase)(KIRQL irql);
extern NTSTATUS  (WINAPI * MmCopyVirtualMemory)(IN PEPROCESS FromProcess,IN CONST VOID *FromAddress,IN PEPROCESS ToProcess,OUT PVOID ToAddress,IN SIZE_T BufferSize,IN KPROCESSOR_MODE PreviousMode,OUT PSIZE_T NumberOfBytesCopied);

extern VOID  (WINAPI *KeContextFromKframes)(IN PKTRAP_FRAME TrapFrame,PKEXCEPTION_FRAME ExceptionFrame,PCONTEXT ContextFrame);
extern VOID (WINAPI *KeContextToKframes) ( PKTRAP_FRAME TrapFrame,PKEXCEPTION_FRAME ExceptionFrame,IN PCONTEXT ContextFrame,IN ULONG ContextFlags,IN KPROCESSOR_MODE PreviousMode);
extern BOOLEAN (WINAPI *KiCheckForAtlThunk) (IN PEXCEPTION_RECORD ExceptionRecord,IN PCONTEXT Context);
extern BOOLEAN (WINAPI *RtlDispatchException) (IN PEXCEPTION_RECORD ExceptionRecord,IN PCONTEXT ContextRecord);


extern BOOLEAN (WINAPI *KdIsThisAKdTrap) (IN PEXCEPTION_RECORD ExceptionRecord,IN PCONTEXT ContextRecord,IN KPROCESSOR_MODE PreviousMode);
extern VOID (WINAPI *KiSegSsToTrapFrame ) (IN PKTRAP_FRAME TrapFrame,IN ULONG SegSs);
extern VOID (WINAPI *KiEspToTrapFrame)(IN PKTRAP_FRAME TrapFrame,IN ULONG Esp);
extern ULONG (WINAPI *KiCopyInformation) (IN OUT PEXCEPTION_RECORD ExceptionRecord1,IN PEXCEPTION_RECORD ExceptionRecord2);
extern BOOLEAN
(WINAPI *KiDebugRoutine) (
						  IN PKTRAP_FRAME TrapFrame,
						  IN PKEXCEPTION_FRAME ExceptionFrame,
						  IN PEXCEPTION_RECORD ExceptionRecord,
						  IN PCONTEXT ContextRecord,
						  IN KPROCESSOR_MODE PreviousMode,
						  IN BOOLEAN SecondChance
						  );


#define ASSERT_PROCESS(object) ASSERT((object)->Header.Type == ProcessObject)

VOID WINAPI HxKeStackAttachProcess (__inout PKPROCESS Process,__out PKAPC_STATE ApcState);
VOID WINAPI HxKiAttachProcess ( __inout PRKTHREAD Thread,__in PKPROCESS Process,__in PKIRQL irql,__out PKAPC_STATE SavedApcState);
VOID WINAPI HxKeUnstackDetachProcess (IN PKAPC_STATE ApcState);

NTSTATUS WINAPI HxNtReadVirtualMemory(__in HANDLE ProcessHandle,__in_opt PVOID BaseAddress,__out_bcount(BufferSize) PVOID Buffer,__in SIZE_T BufferSize,__out_opt PSIZE_T NumberOfBytesRead );
NTSTATUS WINAPI HxNtWriteVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress,PVOID Buffer,ULONG BufferSize,PULONG ReturnLength);
NTSTATUS WINAPI HxNtProtectVirtualMemory(__in HANDLE ProcessHandle,__inout PVOID *BaseAddress,__inout PULONG ProtectSize,__in ULONG NewProtect, __out PULONG OldProtect);
NTSTATUS WINAPI HxReadProcessMemory( PEPROCESS Process, PUCHAR pbyBuff, PVOID BaseAddress, ULONG ulLen);
NTSTATUS WINAPI HxWriteProcessMemory( PEPROCESS Process, PUCHAR pbyBuff, PVOID BaseAddress, ULONG ulLen);
NTSTATUS WINAPI HxNtOpenProcess (__out PHANDLE ProcessHandle, __in ACCESS_MASK DesiredAccess,__in POBJECT_ATTRIBUTES ObjectAttributes,__in_opt PCLIENT_ID ClientId);
NTSTATUS WINAPI HxNtOpenThread (__out PHANDLE ThreadHandle,__in ACCESS_MASK DesiredAccess,__in POBJECT_ATTRIBUTES ObjectAttributes,__in_opt PCLIENT_ID ClientId);
NTSTATUS WINAPI HxNtOpenSection(__out PHANDLE SectionHandle,__in ACCESS_MASK DesiredAccess,__in POBJECT_ATTRIBUTES ObjectAttributes);
NTSTATUS WINAPI HxNtCreateThread(__out PHANDLE ThreadHandle,__in ACCESS_MASK DesiredAccess,__in_opt POBJECT_ATTRIBUTES ObjectAttributes,__in HANDLE ProcessHandle,__out PCLIENT_ID ClientId,__in PCONTEXT ThreadContext,__in PINITIAL_TEB InitialTeb,__in BOOLEAN CreateSuspended);
VOID     WINAPI HxKeInitializeApc (__out PRKAPC Apc,__in PRKTHREAD Thread,__in KAPC_ENVIRONMENT Environment,__in PKKERNEL_ROUTINE KernelRoutine,__in_opt PKRUNDOWN_ROUTINE RundownRoutine,__in_opt PKNORMAL_ROUTINE NormalRoutine,__in_opt KPROCESSOR_MODE ApcMode,__in_opt PVOID NormalContext);
NTSTATUS WINAPI HxNtQueueApcThread( __in HANDLE ThreadHandle,__in PPS_APC_ROUTINE ApcRoutine,__in_opt PVOID ApcArgument1, __in_opt PVOID ApcArgument2,__in_opt PVOID ApcArgument3);

DECLSPEC_NOINLINE
ULONG64
KxWaitForLockOwnerShip (
						__inout PKSPIN_LOCK_QUEUE LockQueue,
						__inout PKSPIN_LOCK_QUEUE TailQueue
    );
__forceinline
VOID
KxAcquireQueuedSpinLock (
						 __inout PKSPIN_LOCK_QUEUE LockQueue,
						 __inout PKSPIN_LOCK SpinLock
    );

VOID
KiAcquireDispatcherLockAtSynchLevel (
									 VOID
    );




DECLSPEC_NOINLINE
PKSPIN_LOCK_QUEUE
KxWaitForLockChainValid (
						 __inout PKSPIN_LOCK_QUEUE LockQueue
    );

VOID
KiMoveApcState (
				__in PKAPC_STATE Source,
				__out PKAPC_STATE Destination
    );
FORCEINLINE
PSINGLE_LIST_ENTRY
InterlockedPushEntrySingleList (
								IN PSINGLE_LIST_ENTRY ListHead,
								IN PSINGLE_LIST_ENTRY Entry
    );




// 
// #if defined(NT_UP)
// 
// #define KiLockDispatcherDatabaseAtSynchLevel()
// #define KiUnlockDispatcherDatabaseFromSynchLevel()
// 
// #else
// 
// #if defined(_AMD64_)
// 
// VOID
// KiAcquireDispatcherLockAtSynchLevel (
// 									 VOID
// 									 );
// 
// VOID
// KiReleaseDispatcherLockFromSynchLevel (
// 									   VOID
// 									   );
// 
// #define KiLockDispatcherDatabaseAtSynchLevel()                               \
// KiAcquireDispatcherLockAtSynchLevel()
// 
// #define KiUnlockDispatcherDatabaseFromSynchLevel()                           \
// KiReleaseDispatcherLockFromSynchLevel()
// 
// #else
// 
// #define KiLockDispatcherDatabaseAtSynchLevel()                               \
// KeAcquireQueuedSpinLockAtDpcLevel(&KeGetCurrentPrcb()->LockQueue[LockQueueDispatcherLock])
// 
// #define KiUnlockDispatcherDatabaseFromSynchLevel()                           \
// KeReleaseQueuedSpinLockFromDpcLevel(&KeGetCurrentPrcb()->LockQueue[LockQueueDispatcherLock])
// 
// #endif
// 
// #endif
// //
// // Thread scheduling states.
// //
// 
// typedef enum _KTHREAD_STATE {
//     Initialized,
// 		Ready,
// 		Running,
// 		Standby,
// 		Terminated,
// 		Waiting,
// 		Transition,
// 		DeferredReady,
// 		GateWait
// } KTHREAD_STATE;
// 
// 
// typedef enum _ADJUST_REASON {
//     AdjustNone = 0,
// 		AdjustUnwait = 1,
// 		AdjustBoost = 2
// } ADJUST_REASON;
// 
// #define CLOCK_QUANTUM_DECREMENT 3
// #define WAIT_QUANTUM_DECREMENT 1
// #define LOCK_OWNERSHIP_QUANTUM (WAIT_QUANTUM_DECREMENT * 4)
// 
// //
// // Define the default ready skip and thread quantum values.
// //
// 
// #define READY_SKIP_QUANTUM 2
// #define THREAD_QUANTUM (READY_SKIP_QUANTUM * CLOCK_QUANTUM_DECREMENT)
// 
// //
// // Define the round trip decrement count.
// //
// 
// #define ROUND_TRIP_DECREMENT_COUNT 16
// 
// typedef enum _KOBJECTS {
//     EventNotificationObject = 0,
// 		EventSynchronizationObject = 1,
// 		MutantObject = 2,
// 		ProcessObject = 3,
// 		QueueObject = 4,
// 		SemaphoreObject = 5,
// 		ThreadObject = 6,
// 		GateObject = 7,
// 		TimerNotificationObject = 8,
// 		TimerSynchronizationObject = 9,
// 		Spare2Object = 10,
// 		Spare3Object = 11,
// 		Spare4Object = 12,
// 		Spare5Object = 13,
// 		Spare6Object = 14,
// 		Spare7Object = 15,
// 		Spare8Object = 16,
// 		Spare9Object = 17,
// 		ApcObject,
// 		DpcObject,
// 		DeviceQueueObject,
// 		EventPairObject,
// 		InterruptObject,
// 		ProfileObject,
// 		ThreadedDpcObject,
// 		MaximumKernelObject
// } KOBJECTS;
// 
// #define KOBJECT_LOCK_BIT 0x80
// #define KOBJECT_LOCK_BIT_NUMBER 7
// #define KOBJECT_TYPE_MASK 0x7f
// 
// 
// 
// 
// #define MAXIMUM_CCNUMA_NODES    16
// 
// // end_nthal
// //
// // Define node structure for multinode systems.
// //
// // N.B. The x86 SLIST_HEADER is a single quadword.
// //      The AMD64 SLIST_HEADER is 16-byte aligned and contains quadword
// //      header - the region field is not used. The below packing for AMD64
// //      allows the NUMA node structure to fit in a single cache line.
// //
// 
// #define KeGetCurrentNode() (KeGetCurrentPrcb()->ParentNode)
// 
// typedef struct DECLSPEC_CACHEALIGN _KNODE {
//     SLIST_HEADER DeadStackList;         // node dead stack list
// 	
// #if defined(_AMD64_)
// 	
//     union {
//         SLIST_HEADER PfnDereferenceSListHead; // node deferred PFN freelist
//         struct {
//             ULONGLONG Alignment;
//             KAFFINITY ProcessorMask;
//         };
//     };
// 	
// #else
// 	
//     SLIST_HEADER PfnDereferenceSListHead; // node deferred PFN freelist
//     KAFFINITY ProcessorMask;
// 	
// #endif
// 	
//     UCHAR Color;                        // zero based node color
//     UCHAR Seed;                         // ideal processor seed
//     UCHAR NodeNumber;
//     struct _flags {
//         UCHAR Removable : 1;            // node can be removed
//         UCHAR Fill : 7;
//     } Flags;
// 	
//     ULONG MmShiftedColor;               // private shifted color
//     PFN_NUMBER FreeCount[2];            // number of colored pages free
//     PSLIST_ENTRY PfnDeferredList;       // node deferred PFN list
// } KNODE, *PKNODE;
// 
// 
// #define ALERT_INCREMENT 2           // Alerted unwait priority increment
// #define BALANCE_INCREMENT 10        // Balance set priority increment
// #define RESUME_INCREMENT 0          // Resume thread priority increment
// #define TIMER_EXPIRE_INCREMENT 0    // Timer expiration priority increment
// 
// //
// // Define time critical priority class base.
// //
// 
// #define TIME_CRITICAL_PRIORITY_BOUND 14
// 
// 
// 
// #define MEMORY_PRIORITY_BACKGROUND 0
// #define MEMORY_PRIORITY_WASFOREGROUND 1
// #define MEMORY_PRIORITY_FOREGROUND 2
// 
// 
// 
// #if defined(_NTHAL_) || defined(_NTOSP_) || defined(_AMD64_)
// 
// #define AFFINITY_MASK(n) ((ULONG_PTR)1 << (n))
// 
// #else
// 
// 
// //
// // Processor Control Block (PRCB)
// //
// 
// #define PRCB_MAJOR_VERSION 1
// #define PRCB_MINOR_VERSION 1
// #define PRCB_BUILD_DEBUG        0x0001
// #define PRCB_BUILD_UNIPROCESSOR 0x0002
// 
// //////////////////////////////////////////////////////////////////////////
// 
// #define  KDESCRIPTOR ULONG
// 
// 
// typedef struct _KSPECIAL_REGISTERS {
//     ULONG Cr0;
//     ULONG Cr2;
//     ULONG Cr3;
//     ULONG Cr4;
//     ULONG KernelDr0;
//     ULONG KernelDr1;
//     ULONG KernelDr2;
//     ULONG KernelDr3;
//     ULONG KernelDr6;
//     ULONG KernelDr7;
//     KDESCRIPTOR Gdtr;
//     KDESCRIPTOR Idtr;
//     USHORT Tr;
//     USHORT Ldtr;
//     ULONG Reserved[6];
// } KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;
// 
// typedef struct _PP_LOOKASIDE_LIST {
//     struct _GENERAL_LOOKASIDE *P;
//     struct _GENERAL_LOOKASIDE *L;
// } PP_LOOKASIDE_LIST, *PPP_LOOKASIDE_LIST;
// 
// typedef struct _KPROCESSOR_STATE {
//     struct _CONTEXT ContextFrame;
//     struct _KSPECIAL_REGISTERS SpecialRegisters;
// } KPROCESSOR_STATE, *PKPROCESSOR_STATE;
// 
// #endif // _X86_
// 
// // end_windbgkd
// 
// //
// // DPC data structure definition.
// //
// 
// typedef struct _KDPC_DATA {
//     LIST_ENTRY DpcListHead;
//     KSPIN_LOCK DpcLock;
//     volatile ULONG DpcQueueDepth;
//     ULONG DpcCount;
// } KDPC_DATA, *PKDPC_DATA;
// 
// 
// typedef struct _FX_SAVE_AREA{
// 	UCHAR U[0x208];//               : __unnamed
// 	ULONG NpxSavedCpu;//      : Uint4B
// 	ULONG Cr0NpxState;//      : Uint4B
// }FX_SAVE_AREA,*PFX_SAVE_AREA;
// 
// #define POOL_SMALL_LISTS 0x32
// 
// typedef struct _KPRCB {
// 
// //
// // Start of the architecturally defined section of the PRCB. This section
// // may be directly addressed by vendor/platform specific HAL code and will
// // not change from version to version of NT.
// //
//     USHORT MinorVersion;
//     USHORT MajorVersion;
// 
//     struct _KTHREAD *CurrentThread;
//     struct _KTHREAD *NextThread;
//     struct _KTHREAD *IdleThread;
// 
//     CCHAR  Number;
//     CCHAR  Reserved;
//     USHORT BuildType;
//     KAFFINITY SetMember;
// 
//     CCHAR   CpuType;
//     CCHAR   CpuID;
//     USHORT  CpuStep;
// 
//     struct _KPROCESSOR_STATE ProcessorState;
// 
//     ULONG   KernelReserved[16];         // For use by the kernel
//     ULONG   HalReserved[16];            // For use by Hal
// 
// //
// // Per processor lock queue entries.
// //
// // N.B. The following padding is such that the first lock entry falls in the
// //      last eight bytes of a cache line. This makes the dispatcher lock and
// //      the context swap lock lie in separate cache lines.
// //
// 
//     UCHAR PrcbPad0[28 + 64];
//     KSPIN_LOCK_QUEUE LockQueue[LockQueueMaximumLock];
// 
// // End of the architecturally defined section of the PRCB.
// // end_nthal end_ntosp
// 
// //
// // Miscellaneous counters - 64-byte aligned.
// //
// 
//     struct _KTHREAD *NpxThread;
//     ULONG   InterruptCount;
//     ULONG   KernelTime;
//     ULONG   UserTime;
//     ULONG   DpcTime;
//     ULONG   DebugDpcTime;
//     ULONG   InterruptTime;
//     ULONG   AdjustDpcThreshold;
//     ULONG   PageColor;
//     BOOLEAN SkipTick;
//     KIRQL   DebuggerSavedIRQL;
//     UCHAR   NodeColor;
//     UCHAR   Spare1;
//     ULONG   NodeShiftedColor;
//     struct _KNODE *ParentNode;
//     KAFFINITY MultiThreadProcessorSet;
//     struct _KPRCB * MultiThreadSetMaster;
//     ULONG   SecondaryColorMask;
//     LONG    Sleeping;
// 
// //
// // Performance counters - 64-byte aligned.
// //
// // Cache manager performance counters.
// //
// 
//     ULONG CcFastReadNoWait;
//     ULONG CcFastReadWait;
//     ULONG CcFastReadNotPossible;
//     ULONG CcCopyReadNoWait;
//     ULONG CcCopyReadWait;
//     ULONG CcCopyReadNoWaitMiss;
// 
// //
// //  Kernel performance counters.
// //
// 
//     ULONG KeAlignmentFixupCount;
//     ULONG SpareCounter0;
//     ULONG KeDcacheFlushCount;
//     ULONG KeExceptionDispatchCount;
//     ULONG KeFirstLevelTbFills;
//     ULONG KeFloatingEmulationCount;
//     ULONG KeIcacheFlushCount;
//     ULONG KeSecondLevelTbFills;
//     ULONG KeSystemCalls;
// 
// //
// // I/O system counters.
// //
// 
//     volatile LONG IoReadOperationCount;
//     volatile LONG IoWriteOperationCount;
//     volatile LONG IoOtherOperationCount;
//     LARGE_INTEGER IoReadTransferCount;
//     LARGE_INTEGER IoWriteTransferCount;
//     LARGE_INTEGER IoOtherTransferCount;
//     ULONG SpareCounter1[8];
// 
// //
// // Nonpaged per processor lookaside lists - 64-byte aligned.
// //
// 
//     PP_LOOKASIDE_LIST PPLookasideList[16];
// 
// //
// // Nonpaged per processor small pool lookaside lists - 64-byte aligned.
// //
// 
//     PP_LOOKASIDE_LIST PPNPagedLookasideList[POOL_SMALL_LISTS];
// 
// //
// // Paged per processor small pool lookaside lists - 64-byte aligned.
// //
// 
//     PP_LOOKASIDE_LIST PPPagedLookasideList[POOL_SMALL_LISTS];
// 
// //
// // MP interprocessor request packet barrier - 64-byte aligned.
// //
// 
//     volatile KAFFINITY PacketBarrier;
//     volatile ULONG ReverseStall;
//     PVOID IpiFrame;
//     UCHAR PrcbPad2[52];
// 
// //
// // MP interprocessor request packet and summary - 64-byte aligned.
// //
// 
//     volatile PVOID CurrentPacket[3];
//     volatile KAFFINITY TargetSet;
//     volatile PKIPI_WORKER WorkerRoutine;
//     volatile ULONG IpiFrozen;
//     UCHAR PrcbPad3[40];
// 
// //
// // MP interprocessor request summary and packet address - 64-byte aligned.
// //
// 
//     volatile ULONG RequestSummary;
//     volatile struct _KPRCB *SignalDone;
//     UCHAR PrcbPad4[56];
// 
// //
// // DPC listhead, counts, and batching parameters - 64-byte aligned.
// //
// 
//     KDPC_DATA DpcData[2];
//     PVOID DpcStack;
//     ULONG MaximumDpcQueueDepth;
//     ULONG DpcRequestRate;
//     ULONG MinimumDpcRate;
//     volatile BOOLEAN DpcInterruptRequested;
//     volatile BOOLEAN DpcThreadRequested;
// 
// //
// // N.B. the following two fields must be on a word boundary.
// //
// 
//     volatile BOOLEAN DpcRoutineActive;
//     volatile BOOLEAN DpcThreadActive;
//     KSPIN_LOCK PrcbLock;
//     ULONG DpcLastCount;
//     volatile ULONG TimerHand;
//     volatile ULONG TimerRequest;
//     PVOID DpcThread;
//     KEVENT DpcEvent;
//     BOOLEAN ThreadDpcEnable;
//     volatile BOOLEAN QuantumEnd;
//     UCHAR PrcbPad50;
//     volatile BOOLEAN IdleSchedule;
//     LONG DpcSetEventRequest;
//     UCHAR PrcbPad5[18];
// 
// //
// // Number of 100ns units remaining before a tick completes on this processor.
// //
// 
//     LONG TickOffset;
// 
// //
// // Generic call DPC - 64-byte aligned.
// //
// 
//     KDPC CallDpc;
//     ULONG PrcbPad7[8];
// 
// //
// // Per-processor ready summary and ready queues - 64-byte aligned.
// //
// // N.B. Ready summary is in the first cache line as the queue for priority
// //      zero is never used.
// //
// 
//     LIST_ENTRY WaitListHead;
//     ULONG ReadySummary;
//     ULONG QueueIndex;
//     LIST_ENTRY DispatcherReadyListHead[MAXIMUM_PRIORITY];
//     SINGLE_LIST_ENTRY DeferredReadyListHead;
//     ULONG PrcbPad72[11];
// 
// //
// // Per processor chained interrupt list - 64-byte aligned.
// //
// 
//     PVOID ChainedInterruptList;
// 
// //
// // I/O IRP float.
// //
// 
//     LONG LookasideIrpFloat;
// 
// //
// // Memory management counters.
// //
// 
//     volatile LONG MmPageFaultCount;
//     volatile LONG MmCopyOnWriteCount;
//     volatile LONG MmTransitionCount;
//     volatile LONG MmCacheTransitionCount;
//     volatile LONG MmDemandZeroCount;
//     volatile LONG MmPageReadCount;
//     volatile LONG MmPageReadIoCount;
//     volatile LONG MmCacheReadCount;
//     volatile LONG MmCacheIoCount;
//     volatile LONG MmDirtyPagesWriteCount;
//     volatile LONG MmDirtyWriteIoCount;
//     volatile LONG MmMappedPagesWriteCount;
//     volatile LONG MmMappedWriteIoCount;
//     
// //
// // Spare fields.
// //
// 
//     ULONG   SpareFields0[1];
// 
// //
// // Processor information.
// //
// 
//     UCHAR VendorString[13];
//     UCHAR InitialApicId;
//     UCHAR LogicalProcessorsPerPhysicalProcessor;
//     ULONG MHz;
//     ULONG FeatureBits;
//     LARGE_INTEGER UpdateSignature;
// 
// //
// // ISR timing data.
// //
// 
//     volatile ULONGLONG IsrTime;
//     ULONGLONG SpareField1;
// 
// //
// // Npx save area - 16-byte aligned.
// //
// 
//     FX_SAVE_AREA NpxSaveArea;
// 
// //
// // Processors power state
// //
// 
//     /*PROCESSOR_POWER_STATE*/ 
// 	UCHAR PowerState[0x120];
// 
// // begin_nthal begin_ntosp
// 
// } KPRCB, *PKPRCB, *RESTRICTED_POINTER PRKPRCB;
// 
// 
// 
// FORCEINLINE
// VOID
// KiSendSoftwareInterrupt (
// 						 IN KAFFINITY Affinity,
// 						 IN KIRQL Level
// 						 );
// 
// 
// 
// #if defined(NT_UP)
// 
// #define KiRequestDispatchInterrupt(Processor)
// 
// #else
// 
// #define KiRequestDispatchInterrupt(Processor)             \
//     if (KeGetCurrentProcessorNumber() != Processor) {     \
// 	KiSendSoftwareInterrupt(AFFINITY_MASK(Processor), DISPATCH_LEVEL);     \
//     }
// 
// #endif
// 
// 
// #define KeIsIdleHaltSet(Prcb, Number) ((Prcb)->Sleeping != 0)
// 
// 
// #if defined(_AMD64_)
// 
// #define KiQueryLowTickCount() SharedUserData->TickCount.LowPart
// 
// #else
// 
// #define KiQueryLowTickCount() KeTickCount.LowPart
// 
// #endif
// 
// 
// #if defined(_AMD64_)
// 
// #define PRIORITY_MASK(n) ((ULONG)1 << (n))
// 
// #else
// 
// extern const ULONG KiMask32Array[];
// 
// #define PRIORITY_MASK(n) (KiMask32Array[n])
// 
// #endif
// 
// 
// #define KiRequestSoftwareInterrupt(RequestIrql) \
//     HalRequestSoftwareInterrupt( RequestIrql )
// 
// 
// NTSTATUS ObCreateObjectType(
// 				   __in PUNICODE_STRING TypeName,
// 				   __in POBJECT_TYPE_INITIALIZER ObjectTypeInitializer,
// 				   __in_opt PSECURITY_DESCRIPTOR SecurityDescriptor,
// 				   __out POBJECT_TYPE *ObjectType
// 				   );
// 
// NTSTATUS PsResumeThread (
// 				IN PETHREAD Thread,
// 				OUT PULONG PreviousSuspendCount OPTIONAL
//     );
// 
// NTSTATUS PsTerminateProcess(
// 				   IN PEPROCESS Process,
// 				   IN NTSTATUS Status
// 				   );
// 
// 
// PETHREAD
// PsGetNextProcessThread (
// 						IN PEPROCESS Process,
// 						IN PETHREAD Thread
// 						);
// 
// BOOLEAN
// FASTCALL
// ObReferenceObjectSafe (
// 					   IN PVOID Object
//     );
// 
// 
// VOID
// FORCEINLINE
// PspUnlockProcessShared (
// 						IN PEPROCESS Process,
// 						IN PETHREAD CurrentThread
// 						);
// 
// VOID
// FORCEINLINE
// PspLockProcessShared (
// 					  IN PEPROCESS Process,
// 					  IN PETHREAD CurrentThread
// 					  );
// 
// 
// NTKERNELAPI
// VOID
// DECLSPEC_NOINLINE
// FASTCALL
// ExfAcquirePushLockShared (
// 						  __inout PEX_PUSH_LOCK PushLock
//      );
// 
// 
// 
// VOID
// FORCEINLINE
// ExAcquirePushLockShared (
// 						 IN PEX_PUSH_LOCK PushLock
//      );
// 
// 
// 
// VOID
// FORCEINLINE
// ExReleasePushLockShared (
// 						 IN PEX_PUSH_LOCK PushLock
//      );
// 
// 
// 
// VOID
// FASTCALL
// ExpOptimizePushLockList (
// 						 IN PEX_PUSH_LOCK PushLock,
// 						 IN EX_PUSH_LOCK TopValue
//     );
// 
// 
// NTKERNELAPI
// VOID
// FASTCALL
// ExfWakePushLock (
// 				 IN PEX_PUSH_LOCK PushLock,
// 				 IN EX_PUSH_LOCK TopValue
//     );
// 
// 
// VOID
// FASTCALL
// KeSignalGateBoostPriority (
// 						   __inout PKGATE Gate
//     );
// 
// 
// FORCEINLINE
// VOID
// KiAcquireKobjectLock (
// 					  IN PVOID Object
//     );
// 
// 
// FORCEINLINE
// VOID
// KiReleaseKobjectLock (
// 					  IN PVOID Object
//     );
// 
// 
// FORCEINLINE
// BOOLEAN
// KiTryToAcquireThreadLock (
// 						  IN PKTHREAD Thread
// 						  );
// 
// FORCEINLINE
// VOID
// KiReleaseThreadLock (
// 					 IN PKTHREAD Thread
//     );
// 
// 
// FORCEINLINE
// BOOLEAN
// KzTryToAcquireSpinLock (
// 						IN PKSPIN_LOCK SpinLock
// 						);
// 
// 
// VOID
// FASTCALL
// KiDeferredReadyThread (
// 					   IN PKTHREAD Thread
//     );
// 
// 
// VOID
// FASTCALL
// KiExitDispatcher (
// 				  IN KIRQL OldIrql
//     );
// 
// 
// FORCEINLINE
// VOID
// KiAcquireThreadLock (
// 					 IN PKTHREAD Thread
//     );
// 
// 
// FORCEINLINE
// VOID
// KiReleaseThreadLock (
// 					 IN PKTHREAD Thread
// 					 );
// 
// 
// FORCEINLINE
// VOID
// KzAcquireSpinLock (
// 				   IN PKSPIN_LOCK SpinLock
// 				   );
// 
// 
// 
// FORCEINLINE
// SCHAR
// KiComputeNewPriority (
// 					  IN PKTHREAD Thread,
// 					  IN SCHAR Adjustment
//     );
// 
// 
// FORCEINLINE
// VOID
// KiReleaseTwoPrcbLocks (
// 					   IN PKPRCB FirstPrcb,
// 					   IN PKPRCB SecondPrcb
//     );
// 
// FORCEINLINE
// VOID
// KiAcquireTwoPrcbLocks (
// 					   IN PKPRCB FirstPrcb,
// 					   IN PKPRCB SecondPrcb
//     );
// 
// FORCEINLINE
// VOID
// KiReleasePrcbLock (
// 				   IN PKPRCB Prcb
//     );
// 
// FORCEINLINE
// VOID
// KiClearIdleSummary (
// 					IN KAFFINITY Mask
// 					);
// FORCEINLINE
// VOID
// KiClearSMTSummary (
// 				   IN KAFFINITY Mask
// 				   );
// 
// VOID
// FASTCALL
// KiProcessDeferredReadyList (
// 							IN PKPRCB CurrentPrcb
// 							);
// 
// FORCEINLINE
// VOID
// KiAcquirePrcbLock (
// 				   IN PKPRCB Prcb
//     );
// 
// FORCEINLINE
// VOID
// KiSetContextSwapBusy (
// 					  IN PKTHREAD Thread
//     );
// 
// FORCEINLINE
// VOID
// KxQueueReadyThread (
// 					IN PKTHREAD Thread,
// 					IN PKPRCB Prcb
// 					);
// 
// BOOLEAN
// KiSwapContext (
// 			   IN PKTHREAD OldThread,
// 			   IN PKTHREAD NewThread
// 				   );
// 
// 
// 
// FORCEINLINE
// PKPRCB
// NTAPI
// KeGetCurrentPrcb (VOID)
// {
// 	#if (_MSC_VER >= 13012035)
// 		return (PKPRCB) (ULONG_PTR) __readfsdword (FIELD_OFFSET (KPCR, Prcb));
// 	#else
// 		__asm {  mov eax, _PCR KPCR.Prcb     }
// 	#endif
// }
// 
// 
// VOID
// FASTCALL
// KeInitializeGate (
// 				  __out PKGATE Gate
//     );
// NTKERNELAPI
// VOID DECLSPEC_NOINLINE FASTCALL (*ExfReleasePushLockShared) ( PEX_PUSH_LOCK PushLock);

// 
// FORCEINLINE
// VOID
// KeEnterGuardedRegionThread (
// 							IN PKTHREAD Thread
//     );
// 
// 
// #define OB_FLAG_NEW_OBJECT              0x01
// #define OB_FLAG_KERNEL_OBJECT           0x02
// #define OB_FLAG_CREATOR_INFO            0x04
// #define OB_FLAG_EXCLUSIVE_OBJECT        0x08
// #define OB_FLAG_PERMANENT_OBJECT        0x10
// #define OB_FLAG_DEFAULT_SECURITY_QUOTA  0x20
// #define OB_FLAG_SINGLE_HANDLE_ENTRY     0x40
// #define OB_FLAG_DELETED_INLINE          0x80
// 
// // begin_ntosp
// #define OBJECT_TO_OBJECT_HEADER( o ) \
//     CONTAINING_RECORD( (o), OBJECT_HEADER, Body )
// 
// 
// FORCEINLINE
// BOOLEAN
// ObpSafeInterlockedIncrement(
// 							IN OUT LONG_PTR volatile *lpValue
// 							);
// 
// ULONG
// KeResumeThread (
// 				__inout PKTHREAD Thread
//     );
// 
// 
// VOID
// FORCEINLINE
// PspUnlockProcessList (
// 					  IN PETHREAD CurrentThread
// 					  );
// FORCEINLINE
// VOID
// KeLeaveGuardedRegionThread (
// 							IN PKTHREAD Thread
//     );
// 
// NTSTATUS
// PspTerminateThreadByPointer(
// 							IN PETHREAD Thread,
// 							IN NTSTATUS ExitStatus,
// 							IN BOOLEAN DirectTerminate
// 							);
// 
// VOID
// ObClearProcessHandleTable (
// 						   PEPROCESS Process
//     );
// 
// 
// #define PS_SET_BITS(Flags, Flag) \
//     RtlInterlockedSetBitsDiscardReturn (Flags, Flag)
// 
// 
// #define ObpInterlockedCompareExchange InterlockedCompareExchange64
// 
// 
// VOID
// KiCheckForKernelApcDelivery (
// 							 VOID
// 							 );
// 
// #define PS_CROSS_THREAD_FLAGS_TERMINATED           0x00000001UL
// 
// //
// // Thread create failed
// //
// 
// #define PS_CROSS_THREAD_FLAGS_DEADTHREAD           0x00000002UL
// 
// //
// // Debugger isn't shown this thread
// //
// 
// #define PS_CROSS_THREAD_FLAGS_HIDEFROMDBG          0x00000004UL
// 
// //
// // Thread is impersonating
// //
// 
// #define PS_CROSS_THREAD_FLAGS_IMPERSONATING        0x00000008UL
// 
// //
// // This is a system thread
// //
// 
// #define PS_CROSS_THREAD_FLAGS_SYSTEM               0x00000010UL
// 
// //
// // Hard errors are disabled for this thread
// //
// 
// #define PS_CROSS_THREAD_FLAGS_HARD_ERRORS_DISABLED 0x00000020UL
// 
// //
// // We should break in when this thread is terminated
// //
// 
// #define PS_CROSS_THREAD_FLAGS_BREAK_ON_TERMINATION 0x00000040UL
// 
// //
// // This thread should skip sending its create thread message
// //
// #define PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG    0x00000080UL
// 
// //
// // This thread should skip sending its final thread termination message
// //
//     #define PS_CROSS_THREAD_FLAGS_SKIP_TERMINATION_MSG 0x00000100UL
// 
// 
// #define THREAD_TO_PROCESS(Thread) ((Thread)->ThreadsProcess)
// 
// DECLSPEC_NORETURN
// VOID
// PspExitThread(
// 			  IN NTSTATUS ExitStatus
//     );
// 
// PKTHREAD
// FORCEINLINE
// PsGetKernelThread(
// 				  IN PETHREAD ThreadObject
// 				  );
// 
// 
// typedef enum _KAPC_ENVIRONMENT {
//     OriginalApcEnvironment,
// 		AttachedApcEnvironment,
// 		CurrentApcEnvironment,
// 		InsertApcEnvironment
// } KAPC_ENVIRONMENT;
// 
// VOID
// PsExitSpecialApc(
// 				 IN PKAPC Apc,
// 				 IN PKNORMAL_ROUTINE *NormalRoutine,
// 				 IN PVOID *NormalContext,
// 				 IN PVOID *SystemArgument1,
// 				 IN PVOID *SystemArgument2
// 				 );
// 
// VOID
// PspExitApcRundown(
// 				  IN PKAPC Apc
// 				  );
// 
// 
// VOID
// PspExitNormalApc(
// 				 IN PVOID NormalContext,
// 				 IN PVOID SystemArgument1,
// 				 IN PVOID SystemArgument2
// 				 );
// 
// 
// ULONG
// KeForceResumeThread (
// 					 __inout PKTHREAD Thread
//     );
// typedef struct _OBP_SWEEP_CONTEXT {
//     PHANDLE_TABLE HandleTable;
//     KPROCESSOR_MODE PreviousMode;
// } OBP_SWEEP_CONTEXT, *POBP_SWEEP_CONTEXT;
// 
// 
// PHANDLE_TABLE
// ObReferenceProcessHandleTable (
// 							   PEPROCESS SourceProcess
//     );
// 
// #define PsGetCurrentProcessByThread(xCurrentThread) (ASSERT((xCurrentThread) == PsGetCurrentThread ()),CONTAINING_RECORD(((xCurrentThread)->Tcb.ApcState.Process),EPROCESS,Pcb))
// 
// 
// BOOLEAN
// ObpCloseHandleProcedure (
// 						 IN PHANDLE_TABLE_ENTRY HandleTableEntry,
// 						 IN HANDLE Handle,
// 						 IN PVOID EnumParameter
//     );
// 
// NTSTATUS
// ObpCloseHandleTableEntry (
// 						  IN PHANDLE_TABLE ObjectTable,
// 						  IN PHANDLE_TABLE_ENTRY ObjectTableEntry,
// 						  IN HANDLE Handle,
// 						  IN KPROCESSOR_MODE PreviousMode,
// 						  IN BOOLEAN Rundown
//     );
// 
// NTKERNELAPI
// VOID
// ExSweepHandleTable (
// 					__in PHANDLE_TABLE HandleTable,
// 					__in EX_ENUMERATE_HANDLE_ROUTINE EnumHandleProcedure,
// 					__in PVOID EnumParameter
//     );
// 
// VOID
// ObDereferenceProcessHandleTable (
// 								 PEPROCESS SourceProcess
//     );
// 
// #define PoRundownThread(Thread)     \
//         PopCleanupPowerState(&Thread->Tcb.PowerState)
// 
// typedef struct _EX_CALLBACK {
//     EX_FAST_REF RoutineBlock;
// } EX_CALLBACK, *PEX_CALLBACK;
// 
// typedef struct _EX_CALLBACK_ROUTINE_BLOCK {
//     EX_RUNDOWN_REF        RundownProtect;
//     PEX_CALLBACK_FUNCTION Function;
//     PVOID                 Context;
// } EX_CALLBACK_ROUTINE_BLOCK, *PEX_CALLBACK_ROUTINE_BLOCK;
// 
// 
// 
// #define PSP_MAX_CREATE_PROCESS_NOTIFY 8
// 
// //
// // Define process callouts. These are of type PCREATE_PROCESS_NOTIFY_ROUTINE 
// // Called on process create and delete.
// //
// ULONG PspCreateProcessNotifyRoutineCount;
// EX_CALLBACK PspCreateProcessNotifyRoutine[PSP_MAX_CREATE_PROCESS_NOTIFY];
// 
// #define PSP_MAX_CREATE_THREAD_NOTIFY 8
// 
// 
// PEX_CALLBACK_ROUTINE_BLOCK
// ExReferenceCallBackBlock (
// 						  IN OUT PEX_CALLBACK CallBack
//     );
// 
// PEX_CALLBACK_FUNCTION
// ExGetCallBackBlockRoutine (
// 						   IN PEX_CALLBACK_ROUTINE_BLOCK CallBackBlock
//     );
// 
// VOID
// ExDereferenceCallBackBlock (
// 							IN OUT PEX_CALLBACK CallBack,
// 							IN PEX_CALLBACK_ROUTINE_BLOCK CallBackBlock
//     );
// 
// VOID
// FORCEINLINE
// PspLockProcessExclusive (
// 						 IN PEPROCESS Process,
// 						 IN PETHREAD CurrentThread
// 						 );
// 
// VOID
// FORCEINLINE
// PspUnlockProcessExclusive (
// 						   IN PEPROCESS Process,
// 						   IN PETHREAD CurrentThread
// 						   );
// 
// BOOLEAN
// KeReadStateThread (
// 				   __in PKTHREAD Thread
//     );





//////////////////////////////////////////////////////////////////////////
extern "C"
VOID WINAPI
KiDispatchException (
					   IN PEXCEPTION_RECORD ExceptionRecord,
					   IN PKEXCEPTION_FRAME ExceptionFrame,
					   IN PKTRAP_FRAME TrapFrame,
					   IN KPROCESSOR_MODE PreviousMode,
					   IN BOOLEAN FirstChance
					   );




#endif // !defined(AFX_NTOS_H__6A0074D1_8BF0_4D6C_88FB_5FCB4F06B186__INCLUDED_)
