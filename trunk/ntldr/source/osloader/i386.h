//********************************************************************
//	created:	11:8:2008   12:39
//	file:		i386.h
//	author:		tiamo
//	purpose:	i386
//********************************************************************

#pragma once

//
// debug service break
//
#define BREAKPOINT_BREAK								0

//
// debug service print
//
#define BREAKPOINT_PRINT								1

//
// debug service prompt
//
#define BREAKPOINT_PROMPT								2

//
// debug service load symbols
//
#define BREAKPOINT_LOAD_SYMBOLS							3

//
// debug service unload symbols
//
#define BREAKPOINT_UNLOAD_SYMBOLS						4

//
// count of 80387 registers
//
#define SIZE_OF_80387_REGISTERS							80

//
// i386 context
//
#define CONTEXT_i386									0x00010000

//
// i486 have identical context records
//
#define CONTEXT_i486									0x00010000

//
// SS:SP, CS:IP, FLAGS, BP
//
#define CONTEXT_CONTROL									(CONTEXT_i386 | 0x00000001L)

//
// AX, BX, CX, DX, SI, DI
//
#define CONTEXT_INTEGER									(CONTEXT_i386 | 0x00000002L)

//
// DS, ES, FS, GS
//
#define CONTEXT_SEGMENTS								(CONTEXT_i386 | 0x00000004L)

//
// 387 state
//
#define CONTEXT_FLOATING_POINT							(CONTEXT_i386 | 0x00000008L)

//
// DB 0-3,6,7
//
#define CONTEXT_DEBUG_REGISTERS							(CONTEXT_i386 | 0x00000010L)

//
// without i387 registers
//
#define CONTEXT_FULL									(CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS)

#include <pshpack1.h>
//
// gdtr and idtr
//
typedef struct _DESCRIPTOR
{
	//
	// pading
	//
	USHORT												Pad;

	//
	// limit
	//
	USHORT												Limit;

	//
	// base
	//
	ULONG												Base;
}KDESCRIPTOR,*PKDESCRIPTOR;

//
// pte
//
typedef struct _HARDWARE_PTE
{
	//
	// valid
	//
	ULONG												Valid : 1;

	//
	// write
	//
	ULONG												Write : 1;

	//
	// owner
	//
	ULONG												Owner : 1;

	//
	// write through cache
	//
	ULONG												WriteThrough : 1;

	//
	// enable cache
	//
	ULONG												CacheDisable : 1;

	//
	// accessed
	//
	ULONG												Accessed : 1;

	//
	// writen
	//
	ULONG												Dirty : 1;

	//
	// 4MB page
	//
	ULONG												LargePage : 1;

	//
	// global page
	//
	ULONG												Global : 1;

	//
	// copy on write
	//
	ULONG												CopyOnWrite : 1;

	//
	// prototype pte
	//
	ULONG												Prototype : 1;

	//
	// reserved
	//
	ULONG												Reserved : 1;

	//
	// page frame number
	//
	ULONG												PageFrameNumber : 20;
}HARDWARE_PTE,*PHARDWARE_PTE;

//
// PTE when pae enabled
//
typedef struct _HARDWARE_PTE_PAE
{
	//
	// valid
	//
	ULONGLONG											Valid : 1;

	//
	// write
	//
	ULONGLONG											Write : 1;

	//
	// owner
	//
	ULONGLONG											Owner : 1;

	//
	// write through
	//
	ULONGLONG											WriteThrough : 1;

	//
	// disable cache
	//
	ULONGLONG											CacheDisable : 1;

	//
	// accessed
	//
	ULONGLONG											Accessed : 1;

	//
	// dirty
	//
	ULONGLONG											Dirty : 1;

	//
	// large page mode
	//
	ULONGLONG											LargePage : 1;

	//
	// global page
	//
	ULONGLONG											Global : 1;

	//
	// copy on write
	//
	ULONGLONG											CopyOnWrite : 1;

	//
	// proto type
	//
	ULONGLONG											Prototype : 1;

	//
	// reserved
	//
	ULONGLONG											Reserved0 : 1;

	//
	// page frame number
	//
	ULONGLONG											PageFrameNumber : 26;

	//
	// reserved
	//
	ULONGLONG											Reserved1 : 26;
}HARDWARE_PTE_PAE,*PHARDWARE_PTE_PAE;

//
// gdt entry
//
typedef struct _KGDTENTRY
{
	//
	// limit low 16bits
	//
	USHORT												LimitLow;

	//
	// base low word
	//
	USHORT												BaseLow;


	union
	{
		struct
		{
			//
			// base mid byte
			//
			UCHAR										BaseMid;

			//
			// flags1
			//
			UCHAR										Flags1;

			//
			// flags2
			//
			UCHAR										Flags2;

			//
			// base hi byte
			//
			UCHAR										BaseHi;
		}Bytes;

		struct
		{
			//
			// mid byte
			//
			ULONG										BaseMid : 8;

			//
			// type
			//
			ULONG										Type : 5;

			//
			// dpl
			//
			ULONG										Dpl : 2;

			//
			// pres
			//
			ULONG										Pres : 1;

			//
			// limit hi
			//
			ULONG										LimitHi : 4;

			//
			// system
			//
			ULONG										Sys : 1;

			//
			// reserved
			//
			ULONG										Reserved_0 : 1;

			//
			// big
			//
			ULONG										Default_Big : 1;

			//
			// size type
			//
			ULONG										Granularity : 1;

			//
			// base hi
			//
			ULONG										BaseHi : 8;
		}Bits;
	}HighWord;
}KGDTENTRY,*PKGDTENTRY;

//
// idt entry
//
typedef struct _KIDTENTRY
{
	//
	// offset
	//
	USHORT												Offset;

	//
	// selector
	//
	USHORT												Selector;

	//
	// access
	//
	USHORT												Access;

	//
	// extended offset
	//
	USHORT												ExtendedOffset;
}KIDTENTRY,*PKIDTENTRY;

//
// special register
//
typedef struct _KSPECIAL_REGISTERS
{
	//
	// cr0
	//
	ULONG												Cr0;

	//
	// cr2
	//
	ULONG												Cr2;

	//
	// cr3
	//
	ULONG												Cr3;

	//
	// cr4
	//
	ULONG												Cr4;

	//
	// kernel dr0
	//
	ULONG												KernelDr0;

	//
	// kernel dr1
	//
	ULONG												KernelDr1;

	//
	// kernel dr2
	//
	ULONG												KernelDr2;

	//
	// kernel dr3
	//
	ULONG												KernelDr3;

	//
	// kernel dr6
	//
	ULONG												KernelDr6;

	//
	// kernel dr7
	//
	ULONG												KernelDr7;

	//
	// gdtr
	//
	KDESCRIPTOR											Gdtr;

	//
	// idtr
	//
	KDESCRIPTOR											Idtr;

	//
	// tr
	//
	USHORT												Tr;

	//
	// ldtr
	//
	USHORT												Ldtr;

	//
	// padding
	//
	ULONG												Reserved[6];
}KSPECIAL_REGISTERS,*PKSPECIAL_REGISTERS;

//
// processor state
//
typedef struct _KPROCESSOR_STATE
{
	//
	// context frame
	//
	CONTEXT												ContextFrame;

	//
	// special register
	//
	KSPECIAL_REGISTERS									SpecialRegisters;
}KPROCESSOR_STATE,*PKPROCESSOR_STATE;

//
// trap frame
//
typedef struct _KTRAP_FRAME
{
	//
	// following 4 values are only used and defined for DBG systems,
	// but are always allocated to make switching from DBG to non-DBG and back quicker.
	// they are not DEVL because they have a non-0 performance impact.
	//

	//
	// copy of User EBP set up so KB will work.
	//
	ULONG												DbgEbp;

	//
	// EIP of caller to system call, again, for KB.
	//
	ULONG												DbgEip;

	//
	// marker to show no args here.
	//
	ULONG												DbgArgMark;

	//
	// Pointer to the actual args
	//
	ULONG												DbgArgPointer;

	//
	// temporary values used when frames are edited.
	// any code that want's ESP must materialize it, since it is not stored in the frame for kernel mode callers.
	// and code that sets ESP in a KERNEL mode frame, must put the new value in TempEsp,
	// make sure that TempSegCs holds the real SegCs value, and put a special marker value into SegCs.
	//
	ULONG												TempSegCs;

	//
	// temp esp
	//
	ULONG												TempEsp;

	//
	//  debug registers 0
	//
	ULONG												Dr0;

	//
	//  debug registers 1
	//
	ULONG												Dr1;

	//
	//  debug registers 2
	//
	ULONG												Dr2;

	//
	//  debug registers 3
	//
	ULONG												Dr3;

	//
	//  debug registers 6
	//
	ULONG												Dr6;

	//
	//  debug registers 7
	//
	ULONG												Dr7;

	//
	//  gs
	//
	ULONG												SegGs;

	//
	// es
	//
	ULONG												SegEs;

	//
	// ds
	//
	ULONG												SegDs;

	//
	// edx
	//
	ULONG												Edx;

	//
	// ecx
	//
	ULONG												Ecx;

	//
	// eax
	//
	ULONG												Eax;

	//
	// nesting state, not part of context record
	//
	ULONG												PreviousPreviousMode;

	//
	// exception list
	// trash if caller was user mode.
	// saved exception list if caller was kernel mode or we're in an interrupt.
	//
	PVOID												ExceptionList;

	//
	// fS is TIB/PCR pointer, is here to make save sequence easy
	//
	ULONG												SegFs;

	//
	//  edi
	//
	ULONG												Edi;

	//
	// esi
	//
	ULONG												Esi;

	//
	// ebx
	//
	ULONG												Ebx;

	//
	// ebp
	//
	ULONG												Ebp;

	//
	// error code,if hardware did not push an error code,we push a zero on the stack
	//
	ULONG												ErrCode;

	//
	// hardware saved eip
	//
	ULONG												Eip;

	//
	// hardware save cs
	//
	ULONG												SegCs;

	//
	// hardware saved eflags
	//
	ULONG												EFlags;

	//
	// WARNING - segSS:esp are only here for stack that involve a ring transition.
	//
	ULONG												HardwareEsp;

	//
	// hardware saved ss
	//
	ULONG												HardwareSegSs;

	//
	// v86 mode es
	//
	ULONG												V86Es;

	//
	// v86 mode ds
	//
	ULONG												V86Ds;

	//
	// v86 mode fs
	//
	ULONG												V86Fs;

	//
	// v86 mode gs
	//
	ULONG												V86Gs;
}KTRAP_FRAME,*PKTRAP_FRAME;

//
// processor control block (PRCB)
//
typedef struct _KPRCB
{
	//
	// minor version
	//
	USHORT												MinorVersion;

	//
	// major version
	//
	USHORT												MajorVersion;

	//
	// current thread
	//
	PVOID*												CurrentThread;

	//
	// next thread
	//
	PVOID												NextThread;

	//
	// idle thread
	//
	PVOID												IdleThread;

	//
	// number
	//
	CCHAR												Number;

	//
	// padding
	//
	CCHAR												Reserved;

	//
	// build type
	//
	USHORT												BuildType;

	//
	// set member
	//
	KAFFINITY											SetMember;

	//
	// cpu type
	//
	CCHAR												CpuType;

	//
	// cpu id
	//
	CCHAR												CpuID;

	//
	// cpu step
	//
	USHORT												CpuStep;

	//
	// processor state
	//
	KPROCESSOR_STATE									ProcessorState;

	//
	// reserved
	//
	UCHAR												Reserved1[0x914];
}KPRCB,*PKPRCB;

#include <PopPack.h>