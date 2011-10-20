/*++

	This is the part of NGdbg kernel debugger

	dngeng.cpp

	This file contains general debug-engine routines.

Revision History:
	24-Dec-2008	Great	Fixed bug - ATTEMPTED_WRITE_TO_READ_ONLY_MEMORY bugcheck on 2003 R2 (DbgHookHalImports)

--*/

#include <ntifs.h>
#include "gui.h"
#include "dbgeng.h"
#include "ldasm.h"
#include "winnt.h"



PVOID
SetVector(
  IN UCHAR Interrupt,
  IN PVOID Handler,
  IN BOOLEAN MakeValid
  );
PVOID
GetVector(
  IN UCHAR Interrupt
  );
VOID
DelVector(
  IN UCHAR Interrupt
  );

PVOID FindImage (PWSTR);
PVOID *ppKiDispatchInterrupt;
VOID (NTAPI *KiDispatchInterrupt)();

BOOLEAN DbgEnteredDebugger;

struct DBGBP
{
	BOOLEAN Free;
	PVOID Address;
	UCHAR OldByte;
};

KSPIN_LOCK DbgCcLock;
DBGBP DbgCcBreakpoints[MAX_BREAKPOINTS];
PVOID DbgExpectedSingleStep;

BOOLEAN MmIsSystemAddressAccessable (PVOID);

NTSTATUS
DbgAddCcBreakpoint(
	IN PVOID Address,
	OUT PULONG Number
	)

/*++

Routine Description

	This routine adds a 0xCC (INT3) breakpoint

Arguments

	Address
		virtual address of code

	Number
		receives breakpoint number or -1

Return Value

	STATUS_SUCCESS on success, error status on failure

Enrivonment

	Debugger active

--*/

{
	NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;

	if (MmIsSystemAddressAccessable (Address) == FALSE)
	{
		return STATUS_ACCESS_VIOLATION;
	}

	KIRQL Irql = KeGetCurrentIrql();

	KdPrint(("Adding breakpoint (%X) at irql %x\n", Address, Irql));

	if (Irql < DISPATCH_LEVEL)
		KfRaiseIrql (DISPATCH_LEVEL);

	KeAcquireSpinLockAtDpcLevel (&DbgCcLock);

	*Number = -1;

	for (ULONG i=0; i<MAX_BREAKPOINTS; i++)
	{
		if (DbgCcBreakpoints[i].Free == TRUE)
		{
			DbgCcBreakpoints[i].Free = FALSE;
			DbgCcBreakpoints[i].Address = Address;
			DbgCcBreakpoints[i].OldByte = *(UCHAR*)Address;
			*(UCHAR*)Address = 0xCC;
			Status = STATUS_SUCCESS;
			*Number = i;
			KdPrint(("Added breakpoint #%x, oldbyte %x\n", i, DbgCcBreakpoints[i].OldByte));
			break;
		}
	}

	KeReleaseSpinLockFromDpcLevel (&DbgCcLock);

	if (Irql < DISPATCH_LEVEL)
		KfLowerIrql (Irql);

	return Status;
}

VOID
DbgDelCcBreakpoint(
	ULONG i
	)
{
	KIRQL Irql = KeGetCurrentIrql();

	KdPrint(("Deleting breakpoint #%x at irql %x\n", i, Irql));

	if (Irql < DISPATCH_LEVEL)
		KfRaiseIrql (DISPATCH_LEVEL);

	KeAcquireSpinLockAtDpcLevel (&DbgCcLock);

	if (DbgCcBreakpoints[i].Free == FALSE)
	{
		ASSERT (MmIsSystemAddressAccessable (DbgCcBreakpoints[i].Address) == TRUE);
		*(UCHAR*)DbgCcBreakpoints[i].Address = DbgCcBreakpoints[i].OldByte;
		DbgCcBreakpoints[i].Free = TRUE;
	}

	KeReleaseSpinLockFromDpcLevel (&DbgCcLock);

	if (Irql < DISPATCH_LEVEL)
		KfLowerIrql (Irql);
}


PVOID 
GetKernelAddress (
	PWSTR Name
	)

/*++

Routine Description

	This routine lookups NT kernel's export function
	by name.
	It simply calls MmGetSystemRoutineAddress, so see
	MmGetSystemRoutineAddress description in MSDN

Arguments

	Name

		Function name to be found

Return Value

	Entry point to the function or NULL

Environment

	This function is usually called at IRQL < DISPATCH_LEVEL

--*/

{
	UNICODE_STRING uName;
	RtlInitUnicodeString (&uName, Name);

	return MmGetSystemRoutineAddress (&uName);
}

LONG DbgCanUnloadNow = 0;

__declspec(naked)
VOID 
DbgDispatchInterrupt(
	)

/*++

Routine Description

	CALLBACK

	This is the hook routine for KiDispatchInterrupt.
	HAL's IAT (import address table) is hooked and 
	 address of KiDispatchInterrupt is replaced by address
	 of this function.
	HAL calls KiDispatchInterrupt generally in two cases:

	HalpDipatchInterrupt
		KiDispatchInterrupt
			-> SwapContext
			
	HalpClockInterrupt
		KeUpdateSystemTime
			HalEndSystemInterrupt
				KiDispatchInterrupt
					-> SwapContext

	When debugger is active, dbgeng can sometimes enable
	 interrupts and current thread should NOT be swapped.
	So we hook KiDispatchInterrupt and simply ignore this call
	when debugger is active.

Arguments

	None

Return Value

	None

Environment

	This routine is called generally from hal!HalpDispatchInterrupt or
	 hal!HalEndSystemInterrupt

--*/
					
{
	__asm
	{
		pushad
	}

	InterlockedIncrement (&DbgCanUnloadNow);
	if (DbgEnteredDebugger == FALSE)
	{
		__asm
		{
			popad
		}

		KiDispatchInterrupt ();		
	}
	else
	{
		__asm
		{
			popad
		}
	}
	InterlockedDecrement (&DbgCanUnloadNow);

	__asm ret
}

//
// These routines hook or unhook HAL iat
//

VOID DbgUnhookHalImports()
{
	KdPrint(("DbgUnhookHalImports: [%x] = %x, KI %x\n",
		ppKiDispatchInterrupt,
		*ppKiDispatchInterrupt,
		KiDispatchInterrupt
		));

	*ppKiDispatchInterrupt = KiDispatchInterrupt;

	KdPrint(("DbgUnhookHalImports: [%x] = %x\n",
		ppKiDispatchInterrupt,
		*ppKiDispatchInterrupt
		));

	KdPrint(("Waiting for unload\n"));

//	while (InterlockedCompareExchange (&DbgCanUnloadNow,
//		0, 
//		0) == 0)
	for (;;)
	{
		LONG t = DbgCanUnloadNow;
		if (t == 0)
			break;

		KdPrint(("[t=%d]", t));

		KeStallExecutionProcessor (100);
	}

	KdPrint(("\nOkay to unload\n"));
}

NTSTATUS DbgHookHalImports()
{
	PVOID pHal = FindImage (L"hal.dll");
	NTSTATUS Status = STATUS_NOT_FOUND;

	// Get hal headers
	PIMAGE_NT_HEADERS HalNtHeaders = (PIMAGE_NT_HEADERS) RtlImageNtHeader (pHal);
	ULONG IatSize = 0;

	// Get hal import
	PULONG Iat = (PULONG) 
		RtlImageDirectoryEntryToData( pHal, TRUE, IMAGE_DIRECTORY_ENTRY_IAT, &IatSize);

	*(PVOID*)&KiDispatchInterrupt = GetKernelAddress (L"KiDispatchInterrupt");

	KdPrint(("IAT = %X  Size = %X\n", Iat, IatSize));

	if (Iat == NULL)
	{
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	for ( ULONG i=0; i < IatSize/4; i++ )
	{
		KdPrint(("Function = %X\n", Iat[i]));

		if (Iat[i] == (ULONG)KiDispatchInterrupt)
		{
			KdPrint(("Found KiDispatchInterrupt\n"));

			ppKiDispatchInterrupt = (PVOID*) &Iat[i];
			*(ULONG*)&KiDispatchInterrupt = Iat[i];

			__asm
			{
				mov eax, cr0
				push eax
				and eax, 0xFFFEFFFF
				mov cr0, eax
			}

			Iat[i] = (ULONG) DbgDispatchInterrupt;

			__asm 
			{
				pop eax
				mov cr0, eax
			}

			KdPrint(("KiDispatchInterrupt hooked\n"));

			return STATUS_SUCCESS;
		}
	}

	return Status;
}

typedef struct _KEXCEPTION_FRAME *PKEXCEPTION_FRAME;

BOOLEAN
DbgTrap (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
    );

PVOID *KiDebugRoutine;

BOOLEAN
(NTAPI * KdpTrapOrStub)(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
    );

extern ULONG MinorVersion;

BOOLEAN
DbgHookKiDebugRoutine(
	)

/*++

Routine Description

	This routine hooks KiDebugRoutine.
	See DbgTrap for details

Arguments

	None

Return Value

	TRUE if hook was successful, FALSE otherwise

--*/

{
	PVOID KdDisableDebugger = GetKernelAddress (L"KdDisableDebugger");
	PVOID KdDebuggerEnabled = GetKernelAddress (L"KdDebuggerEnabled");

	// On Windows XP KdDisableDebugger performs all work
	PVOID KdDisableDebuggerWorker = KdDisableDebugger;

	// KdDisableDebugger in XP ends with C3 (RETN)
	UCHAR RetOpCode = 0xC3;
	
	if (MinorVersion == 2)
	{
		// On Windows 2003 Server KdDisableDebugger simply calls 
		// KdDisableDebuggerWithLock
		// On Windows XP KdDisableDebugger performs all work.
		// So, on 2003 we should get address of KdDisableDebuggerWithLock;

		KdPrint(("DBG: Making corrections for 2003 server\n"));

		RetOpCode = 0xC2; // KdDisableDebuggerWithLock on 2003 ends with RETN 4

		PVOID KdDisableDebuggerWithLock = NULL;

		for	(UCHAR *p = (UCHAR*)KdDisableDebugger;
			 *p != 0xC3; // retn
			 p += size_of_code (p))
		{
			if (p[0] == 0xE8) // CALL KdDisableDebuggerWithLock
			{
				// destination           = address of call +    offset
				KdDisableDebuggerWithLock = (PUCHAR)&p[5] + *(ULONG*)&p[1];
				KdPrint(("DBG: Found KdDisableDebuggerWithLock at %X\n", KdDisableDebuggerWithLock));

				KdDisableDebuggerWorker = KdDisableDebuggerWithLock;
				goto _search;
			}
		}

		KdPrint(("DBG: Call KdDisableDebuggerWithLock NOT found!\n"));
		return FALSE;
	}

_search:
	UCHAR *prev = NULL;

	for (UCHAR *p = (UCHAR*)KdDisableDebuggerWorker;
		*p != RetOpCode; // retn
		p += size_of_code (p))
	{
		//
		// Search for
		//
		// and byte ptr [KdDebuggerEnabled], 0		// win xp sp1 (mov KiDebugRoutine,XXX is the NEXT)
		//  or
		// mov byte ptr [KdDebuggerEnabled], 0		// win xp sp2 (mov KiDebugRoutine,XXX is the PREVIOUS)
		//  or
		// mov byte ptr [KdDebuggerEnabled], bl		// win 2003 (mov KiDebugRoutine,XXX is the PREVIOUS)
		//

		if( p[0] == 0x80 &&
			p[1] == 0x25 &&
			*(PVOID*)&p[2] == KdDebuggerEnabled &&
			p[6] == 0
			)
		{
			// Windows XP SP1 instruction:
			// and byte ptr [KdDebuggerEnabled], 0
			prev = p + size_of_code (p); // mov KiDebugRoutine,XXX is the NEXT
			goto _found;
		}

		if (
			// Windows XP instruction (mov [], 0)
			(MinorVersion == 1 &&
			 p[0] == 0xC6 &&
			 p[1] == 0x05 &&
			 *(PVOID*)&p[2] == KdDebuggerEnabled &&
			 p[6] == 0) 
			||
			// For windows 2003 (mov [], bl)
			(MinorVersion == 2 &&
			 p[0] == 0x88 &&
			 p[1] == 0x1d &&
			 *(PVOID*)&p[2] == KdDebuggerEnabled)
			)
		{

_found:
			KdPrint(("DBG: Found MOV BYTE PTR [KdDebuggerEnabled],0 at %X\n", p));
			KdPrint(("DBG: Previous instruction at %X\n", prev));

			if (prev[0] == 0xC7 &&
				prev[1] == 0x05)
			{
				KdPrint(("DBG: Previous is MOV DWORD PTR [mem32], imm32\n"));

				KiDebugRoutine = *(PVOID**)&prev[2];

				KdPrint(("DBG: KiDebugRoutine is %X\n", KiDebugRoutine));

				*(PVOID*)&KdpTrapOrStub = *KiDebugRoutine;
				*KiDebugRoutine = DbgTrap;

				KdPrint(("DBG: KiDebugRoutine hooked, KdpTrapOrStub = %X\n", KdpTrapOrStub));
				return TRUE;
			}
		}

		// save pointer to previous instruction
		prev = p;
	}

	KdPrint(("DBG: KiDebugRoutine NOT hooked!\n"));

	return FALSE;
}

VOID
DbgUnhookKiDebugRoutine(
	)
{
	*KiDebugRoutine = KdpTrapOrStub;
}


PVOID KiTrap03;
BOOLEAN I3HereUser = TRUE;
BOOLEAN I3HereKernel = FALSE;

//
// Debugger engine
//

PVOID Kei386EoiHelper;
UCHAR KTHREAD_DebugActive = 0x2c;

// We don't use interrupt hooking now, but there is a code
// to do this.
#if 0
PKINTERRUPT DbIntObj;

//
// esp + 0x20 or &FirstArg + 0x1c
//

#define INT_TRAP_IN_STACK_OFFSET	0x1c

BOOLEAN
DbgIntBreak(
	PKINTERRUPT InterruptObject,
	PVOID Context
	)
{
	PKTRAP_FRAME TrapFrame = (PKTRAP_FRAME)((PUCHAR)&InterruptObject + INT_TRAP_IN_STACK_OFFSET);

	KdPrint(("INT3! TrapFrame %X DbgArkMark %X current irql %X\n", TrapFrame, TrapFrame->DbgArgMark, KeGetCurrentIrql()));

	DbgTrapBreakPoint (TrapFrame);

	return FALSE;
}

//UCHAR DbVector = 3;
UCHAR DbVector = 0xF3;
#endif

#if 0
BOOLEAN 
(*KeFreezeExecution) (
    IN PVOID TrapFrame,
    IN PVOID ExceptionFrame
    );

VOID
(*KeThawExecution) (
    IN BOOLEAN Enable
    );

VOID
DbgFindFreezeThaw(
	)
{
	PIMAGE_NT_HEADERS NtHeaders = (PIMAGE_NT_HEADERS) RtlImageNtHeader (pNtBase);
	PIMAGE_SECTION_HEADER Sections = (PIMAGE_SECTION_HEADER) NULL;

	for (ULONG i=0; i<NtHeaders->FileHeader.NumberOfSections; i++)
	{
	}
}
#endif

VOID
DbgInitialize(
	)

/*++

Routine Description

	This is initialization routine for debugger engine.
	This routine is called from DriverEntry

Arguments

	None

Return Value

	None, debugger engine cannot fail initialization

--*/

{
#if 0
	KIRQL Irql;
	KAFFINITY Affinity;
	ULONG TempVector = HalGetInterruptVector (Internal, 0, 0, 0, &Irql, &Affinity);

	KdPrint(("TempVector = %x, Irql = %x, Affinity = %x\n", TempVector, Irql, Affinity));

	NTSTATUS Status = IoConnectInterrupt (
		&DbIntObj,
		DbgIntBreak,
		NULL,
		NULL,
		TempVector,
		Irql,
		Irql,
		Latched,
		TRUE,
		Affinity,
		FALSE
		);
	KdPrint(("IoConnectInterrupt %x\n", Status));

	if (!NT_SUCCESS(Status))
	{
		KdPrint(("Cannot connect interrupt\n"));
	}
	else
	{
		KdPrint(("Dispatch code %X\n", DbIntObj->DispatchCode));

		if (DbVector == 0x03)
		{
			KiTrap03 = SetVector (0x03, &DbIntObj->DispatchCode, TRUE);
		}
		else
		{
			SetVector (DbVector, &DbIntObj->DispatchCode, TRUE);
		}
	}
#endif
	
	Kei386EoiHelper = GetKernelAddress (L"Kei386EoiHelper");

	if (!NT_SUCCESS(DbgHookHalImports()))
	{
		KdPrint(("Could not hook hal imports\n"));
		ASSERT (FALSE);
	}

	if (!DbgHookKiDebugRoutine())
	{
		KdPrint(("Could not hook KiDebugRoutine\n"));
		ASSERT (FALSE);
	}
}

VOID
DbgCleanup(
	)

/*++

Routine description

	This function cleans up all hooks set by DbgInitialize()
	It is called from DriverUnload

Arguments

	None

Return Value

	None

--*/

{
	KdPrint(("DbgCleanup enter [irql %x]\n", KeGetCurrentIrql()));

	if (KiDebugRoutine)
	{
		DbgUnhookKiDebugRoutine ();
	}
	else
	{
		KdPrint(("KiDebugRoutine was not hooked!\n"));
	}

	if (ppKiDispatchInterrupt)
	{
		DbgUnhookHalImports();
	}
	else
	{
		KdPrint(("Hal import was not hooked!\n"));
	}

	KdPrint(("DbgCleanup exit\n"));

#if 0
	if (DbIntObj)
	{
		IoDisconnectInterrupt (DbIntObj);

		if (DbVector != 0x03)
		{
			DelVector (DbVector);
		}
		else
		{
			SetVector (0x03, KiTrap03, FALSE);
		}
	}
	else
	{
		KdPrint(("DbIntObj was not connected!\n"));
	}
#endif
}

#if 0
VOID
TrapPossibleVdm(
	PKTRAP_FRAME TrapFrame
	)
{
	KdPrint(("TRAP : possible VDM at KTRAP_FRAME %X\n", TrapFrame));

	KeBugCheckEx (
		NGDBG_INITIATED_CRASH,
		NGDBG_TRAP_POSSIBLE_VDM,
		(ULONG_PTR) TrapFrame,
		0,
		0
		);
}

VOID
TrapMustBeRestored(
	PKTRAP_FRAME TrapFrame
	)
{
	KdPrint(("TRAP : must be restored at KTRAP_FRAME %X\n", TrapFrame));

	KeBugCheckEx (
		NGDBG_INITIATED_CRASH,
		NGDBG_TRAP_MUST_BE_RESTORED,
		(ULONG_PTR) TrapFrame,
		0,
		0
		);
}
#endif

#if 0
VOID
DbgTrapBreakPoint(
	PKTRAP_FRAME TrapFrame
	)
{
	EXCEPTION_RECORD Record;
	NTSTATUS Status;
	KIRQL Irql;

	KeRaiseIrql (HIGH_LEVEL, &Irql);

	KdPrint(("Breakpoint trap at %x\n", TrapFrame->Eip));

	// Increment EIP in stack so it will point to the next
	//  instruction after 0xCC

	if (DbVector == 3)
	{
		TrapFrame->Eip ++;
	}

	Record.ExceptionCode = STATUS_BREAKPOINT;
	Record.ExceptionAddress = (PVOID) TrapFrame->Eip;
	Record.ExceptionFlags = 0;
	Record.NumberParameters = 0;
	Record.ExceptionInformation[0] = 0;

	Status = DbgDispatchException (&Record);

	if (Status == STATUS_SUCCESS)
	{
		//
		// Return to DbgTrap03
		//

		return;
	}

	//
	// Raise system exception
	//

	ExRaiseException (&Record);
}
#endif



VOID WR_ENTER_DEBUGGER(BOOLEAN UserInitiated, PDBG_CALLBACK Callback, PVOID Argument);

/*
VOID
DbgBreakPointCallback(
	BOOLEAN In,
	PVOID Argument,
	BOOLEAN DispatchException
	)
{
	PEXCEPTION_DISPATCH Dispatch = (PEXCEPTION_DISPATCH) Argument;
	PEXCEPTION_RECORD Record = Dispatch->Record;

	if (!In)
	{
		// Successful dispatch, don't raise an exception
		Dispatch->Status = STATUS_SUCCESS;
		return;
	}

	KdPrint(( __FUNCTION__ ": enter (Record %X CODE %X)\n", Record, Record->ExceptionCode));
	GuiPrintf(" -> int3 embedded breakpoint at %x, breaking through..\n", Record->ExceptionAddress);
}
*/

PVOID DisasmAtAddress (PVOID Address, ULONG nCommands);

VOID
DbgAccessViolationCallback(
	BOOLEAN In,
	PVOID Argument,
	BOOLEAN DispatchException
	)

/*++

Routine Description

	CALLBACK

	This routine is called from WR_ENTER_DEBUGGER as callback 
	 (see WR_ENTER_DEBUGGER for details)

Arguments

	In

		Specifies if callback is called before entering of after exit

	Argument

		Callback argument (pointer to EXCEPTION_DISPATCH)

	DispatchException

		If In==FALSE specifies whenever exception should be marked at dispatched
		 or not.
		If In==TRUE, undefined

Return Value

	None

Environment

	Called from WR_ENTER_DEBUGGER at raised IRQL

--*/

{
	PEXCEPTION_DISPATCH Dispatch = (PEXCEPTION_DISPATCH) Argument;
	PEXCEPTION_RECORD Record = Dispatch->Record;

	if (!In)
	{
		// Successful dispatch, don't raise an exception

		if (DispatchException)
		{
			KdPrint(( __FUNCTION__ ": exit (exception dispatched)\n"));
			Dispatch->Status = STATUS_SUCCESS;
		}
		else
		{
			KdPrint(( __FUNCTION__ ": exit (exception NOT dispatched)\n"));
			Dispatch->Status = STATUS_UNSUCCESSFUL;
		}

		return;
	}

	KdPrint(( __FUNCTION__ ": enter (Record %X CODE %X)\n", Record, Record->ExceptionCode));

	GuiPrintf(" -> access violation (%s chance) at %x, the memory %08X could not be %s ..\n", 
		Dispatch->SecondChance ? "second" : "first",
		Record->ExceptionAddress,
		Record->ExceptionInformation[1],
		(Record->ExceptionInformation[0] == 1 ? "written" : "read")
		);

	DisasmAtAddress (Record->ExceptionAddress, 10);
}

NTSTATUS
DbgDispatchException(
	PEXCEPTION_RECORD Record,
	BOOLEAN SecondChance
	)

/*++

Routine Description

	This routine tries to dispatch an exception.
	It calls WR_ENTER_DEBUGGER to wake up debugger

Arguments

	Record

		pointer to EXCEPTION_RECORD of the exception being dispatched

	SecondChance

		Specifies whenever exception is first or second chance

Return Value

	NTSTATUS of dispatch

Environment

	This routine is called from DbgTrap

--*/

{
	EXCEPTION_DISPATCH Dispatch;
	Dispatch.Record = Record;
	Dispatch.SecondChance = SecondChance;
	Dispatch.Status = STATUS_NOT_IMPLEMENTED;

	VOID
	(*pExceptionCallback)(
		BOOLEAN In,
		PVOID Argument,
		BOOLEAN DispatchException
		) = NULL;

	switch (Record->ExceptionCode)
	{	
//	case STATUS_BREAKPOINT:
//		pExceptionCallback = DbgBreakPointCallback;
//		break;

	case STATUS_ACCESS_VIOLATION:
		pExceptionCallback = DbgAccessViolationCallback;
		break;
	}

	// set other exception callbacks for codes such as
	// status_access_violation, etc.

	if (pExceptionCallback)
	{
		WR_ENTER_DEBUGGER (FALSE, pExceptionCallback, &Dispatch);
	}

	return Dispatch.Status;
}

PKTRAP_FRAME TrapFrame;

//
// This routine replaces general KiDebugRoutine
//
// Usually this pointer points to KdpStub or KdpTrap
//

BOOLEAN
DbgTrap (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN SecondChance
    )

/*++

Routine Description:

	CALLBACK

	This routine is called whenever a exception is dispatched and the kernel
    debugger is active.

	This is a hook routine for KiDebugRoutine.
	Usually KiDebugRoutine points to KdpStub (if kd is inactive)
	 or KdpTrap (if kd is enabled).

Arguments:

    TrapFrame - Supplies a pointer to a trap frame that describes the
        trap.

    ExceptionFrame - Supplies a pointer to a exception frame that describes
        the trap.

    ExceptionRecord - Supplies a pointer to an exception record that
        describes the exception.

    ContextRecord - Supplies the context at the time of the exception.

    PreviousMode - Supplies the previous processor mode.

    SecondChance - Supplies a boolean value that determines whether this is
        the second chance (TRUE) that the exception has been raised.

Return Value:

    A value of TRUE is returned if the exception is handled. Otherwise a
    value of FALSE is returned.

--*/

{
	if (!DbgEnteredDebugger)
	{
		::TrapFrame = TrapFrame;

		switch (ExceptionRecord->ExceptionCode)
		{
		case STATUS_ACCESS_VIOLATION:
		case STATUS_SINGLE_STEP:
		case STATUS_BREAKPOINT:
			{
				NTSTATUS Status;
				KIRQL Irql;

				KeRaiseIrql (HIGH_LEVEL, &Irql);

				Status = DbgDispatchException (ExceptionRecord, SecondChance);

				KeLowerIrql (Irql);

				return NT_SUCCESS(Status);
			}
		}
	}

	return KdpTrapOrStub (TrapFrame, ExceptionFrame, ExceptionRecord, ContextRecord, PreviousMode, SecondChance);
}
