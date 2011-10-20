/*++

	This is the part of NGdbg kernel debugger

	halipi.cpp

	Implementation of IPIs for multiprocessor systems

--*/

#include <ntifs.h>

// Local APIC register (16 bit dummy)
union APIC_REGISTER
{
	UCHAR Dummy[16];
	ULONG Value;
};

// Local APIC register table
struct APIC_TABLE
{
	APIC_REGISTER Reserved1[2];

	// Local APIC ID register
	APIC_REGISTER Id;				// r/w

	// Local APIC Version regisger
	APIC_REGISTER Version;			// r/o

	APIC_REGISTER Reserved2[4];

	// Task Priority Register
	APIC_REGISTER TPR;				// r/w

	// Arbitration Priority Register
	APIC_REGISTER APR;				// r/o

	// Processor Prioirity Register
	APIC_REGISTER PPR;				// r/o

	// End-Of-Interrupt Register
	APIC_REGISTER EOI;				// w/o

	APIC_REGISTER Reserved3;

	APIC_REGISTER LocalDestination;	// r/w

	APIC_REGISTER DestinationFormat;

	APIC_REGISTER SpuriousInterruptVector;

	APIC_REGISTER InService[8];
	APIC_REGISTER TriggerMode[8];
	APIC_REGISTER InterruptRequest[8];
	
	APIC_REGISTER ErrorStatus;
	
	// 290-2E0
	APIC_REGISTER Reserved4[6];

	APIC_REGISTER LVT_CMCI;

	APIC_REGISTER InterruptCommand[2];

	APIC_REGISTER LVT_Timer;

	APIC_REGISTER LVT_ThermalSensor;

	APIC_REGISTER LVT_PerfMonitorCounters;

	APIC_REGISTER LVT_LINT0;
	APIC_REGISTER LVT_LINT1;

	APIC_REGISTER LVT_Error;

	APIC_REGISTER InitialCount;
	APIC_REGISTER CurrentCount;

	APIC_REGISTER Reserved5[4];

	APIC_REGISTER DivideConfiguration;

	APIC_REGISTER Reserved6;
};
const ULONG SizeOfApic = sizeof(APIC_TABLE);

// Local APIC mapping in Windows
#define APIC ((APIC_TABLE*)0xFFFE0000)

// Interrupt Command Register
union APIC_ICR
{
	struct
	{
		ULONGLONG Vector : 8;
		ULONGLONG DeliveryMode : 3;
		ULONGLONG DestinationMode : 1;
		ULONGLONG DeliveryStatus : 1;
		ULONGLONG Reserved : 1;
		ULONGLONG Level : 1;
		ULONGLONG TriggerMode : 1;
		ULONGLONG Reserved2 : 2;
		ULONGLONG DestinationShorthand : 2;
		ULONGLONG Reserved3 : 36;
		ULONGLONG Destination : 8;
	};
	LARGE_INTEGER LargeInteger;
};
const ULONG SizeOfIcr = sizeof(APIC_ICR);

//
// IDTR structure
//

#pragma pack(push, 2)
struct IDTR
{
    USHORT Limit;
    PVOID Table;
};
#pragma pack(pop)

VOID (*pKeFlushQueuedDpcs) ();

VOID
SendEachProcessorDpc (
	PKDEFERRED_ROUTINE Routine,
	PVOID Context, 
	PVOID SysArg1, 
	PVOID SysArg2
	)

/*++

Routine Description

	This routine sends DPC to each processor in multiprocessor system

Arguments

	Routine
	
		Deferred routine

	Context, SysArg1, SysArg2
	
		Parameters, see MSDN doc for KeInitializeDpc, KeInsertQueueDpc

Return Value

	None

--*/

{
	UNICODE_STRING u;
	RtlInitUnicodeString (&u, L"KeFlushQueuedDpcs");
	*(PVOID*)&pKeFlushQueuedDpcs = MmGetSystemRoutineAddress (&u);

	for (CCHAR i=0; i<KeNumberProcessors; i++)
	{
		KDPC Dpc;

		KdPrint(("SendEachProcessorDpc: processor [%d] in queue\n", i));

		KeInitializeDpc (&Dpc, Routine, Context);
		KeSetTargetProcessorDpc (&Dpc, i);
		KeInsertQueueDpc (&Dpc, SysArg1, SysArg2);

		KdPrint(("SendEachProcessorDpc: processor [%d] completed its DPC\n", i));
	}

	if (pKeFlushQueuedDpcs)
	{
		// Ensure that all DPCs are delivered.
		pKeFlushQueuedDpcs ();
	}
	else
	{
		KdPrint(("pKeFlushQueuedDpcs = NULL!!!\n"));
	}

	KdPrint(("SendEachProcessorDpc: all completed\n"));
}

#if DBG
char* DeliveryModes[] = {
	"Fixed (000)",
	"Lowest Prioirity (001, ICR)",
	"SMI (010)",
	"reserved (011)",
	"NMI (100)",
	"INIT (101)",
	"Start Up (110, ICR)",
	"ExtINT (111)"
};

char *ShortHands[] = {
	"No Shorthand",
	"Self",
	"All Including Self",
	"All Excluding Self"
};

VOID
DumpIcr(
	APIC_ICR *e
	)

/*++

Routine Description

	This routine dumps Interrupt-Command-Register of Local APIC

Arguments

	e
		Pointer to APIC_ICR structure describing ICR

Return Value

	None

--*/

{
	KdPrint(("Dumping ICR\n"));
	KdPrint((
		"Vector %x Mode %s DstMode %s Status %s "
		"Level %s Trigger %s Shorthand %s Dst %x\n",
		(UCHAR) e->Vector,
		DeliveryModes[e->DeliveryMode],
		e->DestinationMode ? "Logical" : "Physical",
		e->DeliveryStatus ? "Pending" : "Idle",
		e->Level ? "Assert" : "De-Assert",
		e->TriggerMode ? "Level" : "Edge",
		ShortHands[e->DestinationShorthand],
		e->Destination
		));
}
#else
#define DumpIcr(E) NOTHING;
#endif

APIC_ICR 
ReadICR(
	)

/*++

Routine Description

	This routine reads ICR from Local APIC

Arguments

	None

Return Value

	APIC_ICR structure

--*/

{
	APIC_ICR Icr;

	Icr.LargeInteger.LowPart = APIC->InterruptCommand[0].Value;
	Icr.LargeInteger.HighPart = APIC->InterruptCommand[1].Value;

	return Icr;
}

// Maximum number of supported processors
#define MAX_PROCESSORS 16

KIRQL Irql;
KAFFINITY Affinity;
ULONG IpiVector;
PKINTERRUPT IntObjs[MAX_PROCESSORS];
LONG FreezeLock = 0;

//
// Inter-Processor Requests
//

#define IPI_REQ_FREEZE	1		// freeze execution
#define IPI_REQ_IGNORE	2		// ignore IPI

// IPI Request
ULONG IpiRequest = IPI_REQ_IGNORE;


BOOLEAN
 IpiService(
	PKINTERRUPT InterruptObject,
	PVOID Context
	)

/*++

Routine Description

	CALLBACK

	This is an IPI service handler, connected to the appropriate interrupt object
	 by IoConnectInterrupt.
	This routine is called each time debugger delivers an IPI.

	Currently, two requests are supported:

		IPI_REQ_IGNORE

			Ignore the request and return

		IPI_REQ_FREEZE

			Freeze execution of the processor while FreezeLock == 0

Arguments

	InterruptObject

		Pointer to KINTERRUPT object, created by IoConnectInterrupt

	Context

		IoConnectInterrupt context, always NULL

Return Value

	TRUE if next handlers should be called, FALSE Otherwise.
	This interrupt object has no chained handlers, so the return value is ignored.

Environment

	IPI interrupt handler, IRQL = DIRQL for IPI interrupt.

--*/

{
	switch (IpiRequest)
	{
	case IPI_REQ_IGNORE:
		KdPrint(("Ipi Service [%d]! : (%d) ignoring\n", KeGetCurrentProcessorNumber(), IpiRequest));
		break;

	case IPI_REQ_FREEZE:
		KdPrint(("Ipi Service [%d]! : (%d) freezing\n", KeGetCurrentProcessorNumber(), IpiRequest));

		while (InterlockedCompareExchange (&FreezeLock, 0, 0) == 0)
			NOTHING;

		KdPrint(("Ipi Service [%d]! : (%d) thawed\n", KeGetCurrentProcessorNumber(), IpiRequest));
		break;

	default:
		KdPrint(("Ipi Service [%d]! : (%d) unknown request\n", KeGetCurrentProcessorNumber(), IpiRequest));
	}

	return FALSE;
}


VOID
  SetIpiHandlerRoutine(
    IN struct _KDPC  *Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    )

/*++

Routine Description

	CALLBACK

	This is a deferred routine for DPC, which is delivered for each processor in MP system
	 to initialize IPI support.

	This routine connects an IPI interrupt handler

Arguments

	Dpc, DeferredContext, SystemArgument1, SystemArgument2

		Normal parameters of CustomDpc routine, see CustomDpc,KeInitializeDpc,KeInsertQueueDpc in MSDN.

Return Value

	None

--*/

{
	IDTR Idtr;
	__asm
	{
		sidt fword ptr [Idtr]
	}

	ULONG Proc = KeGetCurrentProcessorNumber();
	KdPrint(("[%d] IDT = %x\n", Proc, Idtr.Table));

	if (DeferredContext == NULL)
	{
		NTSTATUS Status;

		Status = IoConnectInterrupt (
			&IntObjs[Proc],
			IpiService,
			NULL,
			NULL,
			IpiVector,
			Irql,
			Irql,
			Latched,
			TRUE,
			Affinity,
			FALSE
			);

		KdPrint(("[%d] IoConnectInterrupt: %x\n", Proc, Status));

		if (!NT_SUCCESS(Status))
			IntObjs[Proc] = NULL;
	}
	else
	{
		if (IntObjs[Proc])
			IoDisconnectInterrupt (IntObjs[Proc]);
		KdPrint(("[%d] Interrupt disconnected\n", Proc));
	}
}


VOID 
RequestIpi (
	APIC_ICR *Icr
	)

/*++

Routine Description

	This routine issues an IPI request to ICR register in Local APIC.
	Writing to lowest 32 bits of ICR actually issues an IPI.
	Caller should pass prepared ICR register

Arguments

	Icr

		Pointer to prepared interrupt-command-register with IPI request.

Return Value

	None

--*/

{
	// Wait while ICR is busy with previous request
	while ( ((APIC_ICR*)&APIC->InterruptCommand[0])->DeliveryStatus );

	APIC->InterruptCommand[1].Value = Icr->LargeInteger.HighPart;
	APIC->InterruptCommand[0].Value = Icr->LargeInteger.LowPart;	// Issue an IPI

	// Wait while ICR is busy with our request
	while ( ((APIC_ICR*)&APIC->InterruptCommand[0])->DeliveryStatus );
}


enum IPI_TYPE
{
	// Send IPI to all processors in the system including self
	IpiSendAll,

	// Send IPI to all processors in the system excluding self
	IpiSendAllExceptSelf,

	// Send IPI to the specified set of processors
	IpiSendByMask,

	// Send IPI to the specified set of processors (set = All processors including self)
	IpiSendAllByMask,

	// Send IPI to the specified set of processors (set = All processors excluding self)
	IpiSendAllExceptSelfByMask
};


VOID
SendIpi(
	IPI_TYPE IpiType,
	UCHAR TargetMask,
	ULONG IpiRequest
	)

/*++

Routine Description

	This is high-level routine to request an IPI of the specified type.
	It prepares ICR and calls RequestIpi() to issue an IPI request in Local APIC's ICR.

Arguments

	IpiType
		
		Type of the IPI to be sent. See IPI_TYPE enum above

	TargetMask

		Set of target processors if IpiType == IpiSendByMask

	IpiRequest

		Request to be sent

Return Value

	None

--*/

{
	APIC_ICR Icr = {0};

	if (IpiType >= IpiSendByMask)
	{
		//
		// Send IPI to the specified set of processors
		//

		Icr.Vector = (IpiVector & 0xFF);
		Icr.DeliveryMode = 0;	// fixed
		Icr.DestinationMode = 1; // logical
		Icr.Level = 0;
		Icr.TriggerMode = 0; // edge
		Icr.DestinationShorthand = 0; // no shorthand

		UCHAR ProcessorMask = (1 << KeNumberProcessors) - 1;
		UCHAR CurrentProcessor = (UCHAR)(1 << KeGetCurrentProcessorNumber());

		switch (IpiType)
		{
		case IpiSendByMask:
			ProcessorMask = TargetMask;
			break;

		case IpiSendAllExceptSelfByMask:
			ProcessorMask &= ~CurrentProcessor;

		case IpiSendAllByMask:
			break;
		}

		Icr.Destination = ProcessorMask;
	}
	else
	{
		Icr.Vector = (IpiVector & 0xFF);
		Icr.DeliveryMode = 0;	// fixed
		Icr.DestinationMode = 0; // physical
		Icr.Level = 0;
		Icr.TriggerMode = 0; // edge

		UCHAR Shorthand = 0;
		switch (IpiType)
		{
		case IpiSendAll:
			Shorthand = 2; // all including self
			break;

		case IpiSendAllExceptSelf:
			Shorthand = 3; // all excluding self
			break;
		}
		
		Icr.DestinationShorthand = Shorthand;
		Icr.Destination = 0xFF;
	}

	// Save IPI request type
	::IpiRequest = IpiRequest;

	// Dump ICR contents
	DumpIcr (&Icr);

	// Issue an IPI
	RequestIpi (&Icr);
}

extern "C"
{

VOID
DbgFreezeProcessors(
	)

/*++

Routine Description

	This routine freezes all processors in MP system excluding self.
	The code until DbgThawProcessors() call is executed exclusively on the
	 current processor, all other processors are freezed (they spins in IpiService)

Arguments

	None

Return Value

	None

--*/

{
	if (KeNumberProcessors > 1)
	{
		KdPrint(("Freezing MP system...\n"));

		// Lock freeze lock
		InterlockedExchange (&FreezeLock, 0);
		
		// Send Freeze-IPI to all processors excluding self
		SendIpi (IpiSendAllExceptSelf, 0, IPI_REQ_FREEZE);
	}
}

VOID
DbgThawProcessors(
	)

/*++

Routine Description

	This routine thaws processors freezed by DbgFreezeProcessors call.

Arguments

	None

Return Value

	None

--*/

{
	if (KeNumberProcessors > 1)
	{
		KdPrint(("Thawing MP system...\n"));

		// Ignore following IPIs
		IpiRequest = IPI_REQ_IGNORE;

		// Unlock freeze-lock
		InterlockedExchange (&FreezeLock, 1);
	}
}

VOID
DbgHalInitializeMP(
	)

/*++

Routine Description

	This routine initializes an IPI support in MultiProcessor system

Arguments

	None

Return Value

	None

--*/

{
	if (KeNumberProcessors > MAX_PROCESSORS)
	{
		KdPrint(("Number of processors (%d) is greater that maximum supported number %d\n",
			KeNumberProcessors,
			MAX_PROCESSORS
			));

		ASSERT (FALSE);
	}

	if (KeNumberProcessors > 1)
	{
		KdPrint(("Initializing IPIs for MP system\n"));

		// Get free interrupt vector for IPI service
		IpiVector = HalGetInterruptVector (Internal, 0, 0, 0, &Irql, &Affinity);
		KdPrint(("Allocated vector %x\n", IpiVector));

		// Set IPI handlers
		KdPrint(("Setting up handlers\n"));
		SendEachProcessorDpc (SetIpiHandlerRoutine, NULL, NULL, NULL);
	}
	else
	{
		KdPrint(("DbgHalInitializeMP: nothing to initialize on UP system\n"));
	}
}

VOID
DbgHalCleanupMP(
	)

/*++

Routine Description

	This routine cleans up an IPI support in MultiProcessor system

Arguments

	None

Return Value

	None

--*/

{
	if (KeNumberProcessors > 1)
	{
		// Remove IPI handlers
		KdPrint(("Cleaning IPIs on MP system\n"));
		SendEachProcessorDpc (SetIpiHandlerRoutine, (PVOID) TRUE, NULL, NULL);

		// Stall until all DPCs are executed and execution is returned from all IpiService's
		KdPrint(("Stalling...\n"));
		KeStallExecutionProcessor (50000);
	}
	else
	{
		KdPrint(("DbgHalCleanupMP: nothing to clean up on UP system\n"));
	}
}

}
