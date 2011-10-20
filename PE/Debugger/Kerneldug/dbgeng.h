#ifndef _DBGENG_H_
#define _DBGENG_H

extern "C"
{

NTSTATUS
DbgDispatchException(
	PEXCEPTION_RECORD Record
	);

VOID
DbgInitialize(
	);

VOID
DbgCleanup(
	);

extern "C"
VOID
DbgTrap03(
	);

//
// Windows trap frame
//
typedef struct _KTRAP_FRAME
{
  /*<thisrel this+0x0>*/ /*|0x4|*/ ULONG DbgEbp;
  /*<thisrel this+0x4>*/ /*|0x4|*/ ULONG DbgEip;
  /*<thisrel this+0x8>*/ /*|0x4|*/ ULONG DbgArgMark;
  /*<thisrel this+0xc>*/ /*|0x4|*/ ULONG DbgArgPointer;
  /*<thisrel this+0x10>*/ /*|0x4|*/ ULONG TempSegCs;
  /*<thisrel this+0x14>*/ /*|0x4|*/ ULONG TempEsp;
  /*<thisrel this+0x18>*/ /*|0x4|*/ ULONG Dr0;
  /*<thisrel this+0x1c>*/ /*|0x4|*/ ULONG Dr1;
  /*<thisrel this+0x20>*/ /*|0x4|*/ ULONG Dr2;
  /*<thisrel this+0x24>*/ /*|0x4|*/ ULONG Dr3;
  /*<thisrel this+0x28>*/ /*|0x4|*/ ULONG Dr6;
  /*<thisrel this+0x2c>*/ /*|0x4|*/ ULONG Dr7;
  /*<thisrel this+0x30>*/ /*|0x4|*/ ULONG SegGs;
  /*<thisrel this+0x34>*/ /*|0x4|*/ ULONG SegEs;
  /*<thisrel this+0x38>*/ /*|0x4|*/ ULONG SegDs;
  /*<thisrel this+0x3c>*/ /*|0x4|*/ ULONG Edx;
  /*<thisrel this+0x40>*/ /*|0x4|*/ ULONG Ecx;
  /*<thisrel this+0x44>*/ /*|0x4|*/ ULONG Eax;
  /*<thisrel this+0x48>*/ /*|0x4|*/ ULONG PreviousPreviousMode;
  /*<thisrel this+0x4c>*/ /*|0x4|*/ PEXCEPTION_REGISTRATION_RECORD ExceptionList;
  /*<thisrel this+0x50>*/ /*|0x4|*/ ULONG SegFs;
  /*<thisrel this+0x54>*/ /*|0x4|*/ ULONG Edi;
  /*<thisrel this+0x58>*/ /*|0x4|*/ ULONG Esi;
  /*<thisrel this+0x5c>*/ /*|0x4|*/ ULONG Ebx;
  /*<thisrel this+0x60>*/ /*|0x4|*/ ULONG Ebp;
  /*<thisrel this+0x64>*/ /*|0x4|*/ ULONG ErrCode;
  /*<thisrel this+0x68>*/ /*|0x4|*/ ULONG Eip;
  /*<thisrel this+0x6c>*/ /*|0x4|*/ ULONG SegCs;
  /*<thisrel this+0x70>*/ /*|0x4|*/ ULONG EFlags;
  /*<thisrel this+0x74>*/ /*|0x4|*/ ULONG HardwareEsp;
  /*<thisrel this+0x78>*/ /*|0x4|*/ ULONG HardwareSegSs;
  /*<thisrel this+0x7c>*/ /*|0x4|*/ ULONG V86Es;
  /*<thisrel this+0x80>*/ /*|0x4|*/ ULONG V86Ds;
  /*<thisrel this+0x84>*/ /*|0x4|*/ ULONG V86Fs;
  /*<thisrel this+0x88>*/ /*|0x4|*/ ULONG V86Gs;
} KTRAP_FRAME, *PKTRAP_FRAME;
// <size 0x8c>

#define SIZEOF_TRAPFRAME_AND_FXSAVEAREA		0x29c


VOID
TrapPossibleVdm(
	PKTRAP_FRAME TrapFrame
	);

VOID
TrapMustBeRestored(
	PKTRAP_FRAME TrapFrame
	);

extern UCHAR KTHREAD_DebugActive;

VOID
DbgTrapBreakPoint(
	PKTRAP_FRAME TrapFrame
	);

#define NGDBG_INITIATED_CRASH (0x10000000 | MANUALLY_INITIATED_CRASH)

#define NGDBG_TRAP_POSSIBLE_VDM			0x00000001
#define NGDBG_TRAP_MUST_BE_RESTORED		0x00000002

extern PVOID Kei386EoiHelper;
extern UCHAR KTHREAD_DebugActive;

VOID ExRaiseException (PEXCEPTION_RECORD);

typedef struct EXCEPTION_DISPATCH
{
	IN EXCEPTION_RECORD *Record;
	IN BOOLEAN SecondChance;
	OUT NTSTATUS Status;
} *PEXCEPTION_DISPATCH;

typedef 
VOID
(NTAPI
 *PDBG_CALLBACK)(
	BOOLEAN In,
	PVOID Argument,
	BOOLEAN DispatchException
	);

extern BOOLEAN I3HereUser;
extern BOOLEAN I3HereKernel;

VOID
DbgFreezeProcessors(
	);

VOID
DbgThawProcessors(
	);

extern "C" extern PVOID pNtBase;

#define MAX_BREAKPOINTS 128

VOID
DbgHalInitializeMP(
	);

VOID
DbgHalCleanupMP(
	);

}


#endif
