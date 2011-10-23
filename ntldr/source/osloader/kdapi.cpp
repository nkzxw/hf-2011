//********************************************************************
//	created:	11:8:2008   14:31
//	file:		kdapi.cpp
//	author:		tiamo
//	purpose:	kernel debug api
//********************************************************************

#include "stdafx.h"

//
// message buffer
//
CHAR													BdMessageBuffer[0x1000];

//
// control+c pressed
//
BOOLEAN													BdControlCPressed;

//
// ctrl+c pending
//
BOOLEAN													BdControlCPending;

//
// debugger not present
//
BOOLEAN													BdDebuggerNotPresent;

//
// next expected id
//
ULONG													BdPacketIdExpected;

//
// next send id
//
ULONG													BdNextPacketIdToSend;

//
// retries
//
ULONG													BdNumberRetries = 5;

//
// default retries count
//
ULONG													BdRetryCount = 5;

//
// breakpoint instruction byte
//
UCHAR													BdBreakpointInstruction;

//
// breakpoint table
//
BREAKPOINT_ENTRY										BdBreakpointTable[BREAKPOINT_TABLE_SIZE];

//
// send control packet
//
VOID													(*BdSendControlPacket)(__in ULONG PacketType,__in_opt ULONG PacketId);

//
// receive a packet
//
ULONG													(*BdReceivePacket)(__in ULONG PacketType,__out PSTRING Header,__out PSTRING Data,__out PULONG DataLength);

//
// send control packet
//
VOID													(*BdSendPacket)(__in ULONG PacketType,__in PSTRING MessageHeader,__in PSTRING MessageData);

//
// remote file handle
//
ULONG64													BdRemoteFiles[0x10];

//
// remote file api buffer
//
CHAR													BdFileTransferBuffer[0x2000];

//
// loaded module list head
//
LIST_ENTRY												BdModuleList;

//
// loader data entry
//
LDR_DATA_TABLE_ENTRY									BdModuleDataTableEntry;

//
// check pde valid
//
BOOLEAN BdCheckPdeValid(__in PVOID Buffer)
{
	if(PaeEnabled)
		return *Add2Ptr(PDE_BASE_X86PAE + ((Add2Ptr(Buffer,0,ULONG) >> 21) << 3),0,PUCHAR) & 1;

	return *Add2Ptr(PDE_BASE_X86 + ((Add2Ptr(Buffer,0,ULONG) >> 22) << 2),0,PUCHAR) & 1;
}

//
// check pte
//
BOOLEAN BdCheckPteValid(__in PVOID Buffer)
{
	if(PaeEnabled)
		return *Add2Ptr(PTE_BASE_X86 + ((Add2Ptr(Buffer,0,ULONG) >> 12) << 3),0,PUCHAR) & 1;

	return *Add2Ptr(PTE_BASE_X86 + ((Add2Ptr(Buffer,0,ULONG) >> 12) << 2),0,PUCHAR) & 1;
}

//
// check write access
//
PVOID BdWriteCheck(__in PVOID Buffer)
{
	//
	// pde and pte must be valid
	//
	if(BdCheckPdeValid(Buffer) && BdCheckPteValid(Buffer))
		return Buffer;

	return 0;
}

//
// transfer physical address
//
PVOID BdTranslatePhysicalAddress(__in ULONG64 Address)
{
	return reinterpret_cast<PVOID>(Address);
}

//
// copy memory
//
VOID BdCopyMemory(__in PVOID Dest,__in PVOID Src,__in ULONG Count)
{
	PUCHAR DstBuffer									= static_cast<PUCHAR>(Dest);
	PUCHAR SrcBuffer									= static_cast<PUCHAR>(Src);
	for(ULONG i = 0; i < Count; i ++)
		DstBuffer[i]									= SrcBuffer[i];
}

//
// move memory
//
ULONG BdMoveMemory(__in PVOID Dest,__in PVOID Src,__in ULONG Count)
{
	ULONG MoveLength									= Count > 0x1000 ? 0x1000 : Count;
	Count												= MoveLength;
	PUCHAR DestBuffer									= static_cast<PUCHAR>(Dest);
	PUCHAR SrcBuffer									= static_cast<PUCHAR>(Src);

	while(Add2Ptr(SrcBuffer,0,ULONG) & 3)
	{
		if(!MoveLength)
			break;

		PUCHAR WriteBuffer								= static_cast<PUCHAR>(BdWriteCheck(DestBuffer));
		PUCHAR ReadBuffer								= static_cast<PUCHAR>(BdWriteCheck(SrcBuffer));
		if(!WriteBuffer || !ReadBuffer)
			break;

		*WriteBuffer									= *ReadBuffer;
		DestBuffer										+= sizeof(UCHAR);
		SrcBuffer										+= sizeof(UCHAR);
		MoveLength										-= sizeof(UCHAR);
	}

	while(MoveLength > 3)
	{
		PULONG WriteBuffer								= static_cast<PULONG>(BdWriteCheck(DestBuffer));
		PULONG ReadBuffer								= static_cast<PULONG>(BdWriteCheck(SrcBuffer));
		if(!WriteBuffer || !ReadBuffer)
			break;

		*WriteBuffer									= *ReadBuffer;
		DestBuffer										+= sizeof(ULONG);
		SrcBuffer										+= sizeof(ULONG);
		MoveLength										-= sizeof(ULONG);
	}

	while(MoveLength)
	{
		PUCHAR WriteBuffer								= static_cast<PUCHAR>(BdWriteCheck(DestBuffer));
		PUCHAR ReadBuffer								= static_cast<PUCHAR>(BdWriteCheck(SrcBuffer));
		if(!WriteBuffer || !ReadBuffer)
			break;

		*WriteBuffer									= *ReadBuffer;
		DestBuffer										+= sizeof(UCHAR);
		SrcBuffer										+= sizeof(UCHAR);
		MoveLength										-= sizeof(UCHAR);
	}

	return Count - MoveLength;
}

//
// low write context
//
BOOLEAN BdLowWriteContent(__in ULONG Index)
{
	if(BdBreakpointTable[Index].Flags & KD_BREAKPOINT_NEEDS_WRITE)
	{
		//
		// the breakpoint was never written out.clear the flag and we are done.
		//
		BdBreakpointTable[Index].Flags					&= ~KD_BREAKPOINT_NEEDS_WRITE;
		return TRUE;
	}

	if(BdBreakpointTable[Index].Content == BdBreakpointInstruction)
	{
		//
		// the instruction is a breakpoint anyway.
		//
		return TRUE;
	}

	if(BdMoveMemory(reinterpret_cast<PVOID>(BdBreakpointTable[Index].Address),&BdBreakpointTable[Index].Content,sizeof(UCHAR)) != sizeof(UCHAR))
	{
		BdBreakpointTable[Index].Flags					|= KD_BREAKPOINT_NEEDS_REPLACE;

		return FALSE;
	}

	return TRUE;
}

//
// add breakpoint
//
ULONG BdAddBreakpoint(__in ULONG64 Address)
{
	UCHAR OldByte;
	BOOLEAN AddressValid								= BdMoveMemory(&OldByte,reinterpret_cast<PVOID>(Address),sizeof(OldByte)) == sizeof(OldByte);
	if(AddressValid)
	{
		if(!BdWriteCheck(reinterpret_cast<PVOID>(Address)))
			return 0;
	}

	ULONG Index = 0;
	for(; Index < ARRAYSIZE(BdBreakpointTable); Index ++)
	{
		if(!BdBreakpointTable[Index].Flags)
			break;
	}

	if(Index == ARRAYSIZE(BdBreakpointTable))
		return 0;

	BdBreakpointTable[Index].Address					= Address;

	if(AddressValid)
	{
		BdBreakpointTable[Index].Content				= OldByte;
		BdBreakpointTable[Index].Flags					= KD_BREAKPOINT_IN_USE;
		BdMoveMemory(reinterpret_cast<PVOID>(Address),&BdBreakpointInstruction,sizeof(BdBreakpointInstruction));
	}
	else
	{
		BdBreakpointTable[Index].Flags					= KD_BREAKPOINT_IN_USE | KD_BREAKPOINT_NEEDS_WRITE;
	}

	return Index + 1;
}

//
// delete breakpoint from index
//
BOOLEAN BdDeleteBreakpoint(__in ULONG Handle)
{
	ULONG Index											= Handle - 1;

	//
	// if the specified handle is not valid, then return FALSE.
	//
	if(Handle == 0 || Handle > BREAKPOINT_TABLE_SIZE)
		return FALSE;

	//
	// if the specified breakpoint table entry is not valid, then return FALSE.
	//
	if(BdBreakpointTable[Index].Flags == 0)
		return FALSE;

	//
	// if the breakpoint is already suspended, just delete it from the table.
	//
	BOOLEAN RemoveIt									= FALSE;
	if(!(BdBreakpointTable[Index].Flags & KD_BREAKPOINT_SUSPENDED) || BdBreakpointTable[Index].Flags & KD_BREAKPOINT_NEEDS_REPLACE)
		RemoveIt										= BdLowWriteContent(Index);
	else
		RemoveIt										= TRUE;

	if(RemoveIt)
	{
		//
		// delete breakpoint table entry
		//
		BdBreakpointTable[Index].Flags					= 0;
	}

	return TRUE;
}

//
// remove breakpoint in range
//
BOOLEAN BdDeleteBreakpointRange(__in ULONGLONG Lower,__in ULONGLONG Upper)
{
	//
	// examine each entry in the table in turn
	//
	BOOLEAN ReturnStatus								= FALSE;
	for(ULONG Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index ++)
	{
		if((BdBreakpointTable[Index].Flags & KD_BREAKPOINT_IN_USE) && BdBreakpointTable[Index].Address >= Lower && BdBreakpointTable[Index].Address <= Upper)
		{
			//
			// breakpoint is in use and falls in range, clear it.
			//
			ReturnStatus								= ReturnStatus || BdDeleteBreakpoint(Index + 1);
		}
	}

	return ReturnStatus;
}

//
// suspend breakpoint
//
VOID BdSuspendBreakpoint(__in ULONG Handle)
{
	ULONG Index											= Handle - 1;

	if((BdBreakpointTable[Index].Flags & KD_BREAKPOINT_IN_USE) && !(BdBreakpointTable[Index].Flags & KD_BREAKPOINT_SUSPENDED))
	{
		BdBreakpointTable[Index].Flags					|= KD_BREAKPOINT_SUSPENDED;
		BdLowWriteContent(Index);
	}
}

//
// suspend all breakpoints
//
VOID BdSuspendAllBreakpoints()
{
	for(ULONG i = 1; i <= BREAKPOINT_TABLE_SIZE; i ++)
		BdSuspendBreakpoint(i);
}

//
// set context state
//
VOID BdSetContextState(__inout PDBGKD_WAIT_STATE_CHANGE64 WaitStateChange,__in PCONTEXT Context)
{
	extern KPRCB BdPrcb;

	WaitStateChange->ControlReport.Dr6					= BdPrcb.ProcessorState.SpecialRegisters.KernelDr6;
	WaitStateChange->ControlReport.Dr7					= BdPrcb.ProcessorState.SpecialRegisters.KernelDr7;
	WaitStateChange->ControlReport.EFlags				= Context->EFlags;
	WaitStateChange->ControlReport.SegCs				= static_cast<USHORT>(Context->SegCs);
	WaitStateChange->ControlReport.SegDs				= static_cast<USHORT>(Context->SegDs);
	WaitStateChange->ControlReport.SegEs				= static_cast<USHORT>(Context->SegEs);
	WaitStateChange->ControlReport.SegFs				= static_cast<USHORT>(Context->SegFs);
	WaitStateChange->ControlReport.ReportFlags			= REPORT_INCLUDES_SEGS;
}

//
// extract continuation control data from Manipulate_State message
//
VOID BdGetStateChange(__in PDBGKD_MANIPULATE_STATE64 ManipulateState,__in PCONTEXT ContextRecord)
{
	if(!NT_SUCCESS(ManipulateState->Continue2.ContinueStatus))
		return;

	//
	// the debugger is doing a continue, and it makes sense to apply control changes.
	//
	if(ManipulateState->Continue2.ControlSet.TraceFlag == 1)
		ContextRecord->EFlags							|= 0x100L;
	else
		ContextRecord->EFlags							&= ~0x100L;

	extern KPRCB BdPrcb;
	BdPrcb.ProcessorState.SpecialRegisters.KernelDr7	= ManipulateState->Continue2.ControlSet.Dr7;
	BdPrcb.ProcessorState.SpecialRegisters.KernelDr6	= 0L;
}

//
// set state change
//
VOID BdSetStateChange(__inout PDBGKD_WAIT_STATE_CHANGE64 WaitStateChange,__in PEXCEPTION_RECORD ExceptionRecord,__in PCONTEXT Context)
{
	BdSetContextState(WaitStateChange,Context);
	WaitStateChange->u.Exception.FirstChance			= TRUE;
}

//
// set common state
//
VOID BdpSetCommonState(__in ULONG NewState,__in PCONTEXT Context,__inout PDBGKD_WAIT_STATE_CHANGE64 WaitStateChange)
{
	//
	// sign extend
	//
	WaitStateChange->ProgramCounter						= static_cast<ULONG64>(static_cast<LONG>(Context->Eip));
	WaitStateChange->NewState							= NewState;
	WaitStateChange->NumberProcessors					= 1;
	WaitStateChange->Thread								= 0;
	WaitStateChange->Processor							= 0;
	WaitStateChange->ProcessorLevel						= 0;
	RtlZeroMemory(&WaitStateChange->ControlReport,sizeof(WaitStateChange->ControlReport));

	//
	// copy instructions
	//
	WaitStateChange->ControlReport.InstructionCount		= static_cast<USHORT>(BdMoveMemory(WaitStateChange->ControlReport.InstructionStream,
																						   Add2Ptr(Context->Eip,0,PVOID),DBGKD_MAXSTREAM));

	//
	// delete breakpoint in this range
	// there were any breakpoints cleared, recopy the area without them
	//
	if(BdDeleteBreakpointRange(Context->Eip,WaitStateChange->ControlReport.InstructionCount + Context->Eip - 1))
		BdMoveMemory(WaitStateChange->ControlReport.InstructionStream,Add2Ptr(Context->Eip,0,PVOID),WaitStateChange->ControlReport.InstructionCount);
}

//
// convert exception record
//
VOID ExceptionRecord32To64(__in PEXCEPTION_RECORD ExceptionRecord,__out PEXCEPTION_RECORD64 ExceptionRecord64)
{
	//
	// sign extend
	//
	ExceptionRecord64->ExceptionAddress					= static_cast<ULONG64>(reinterpret_cast<LONG>(ExceptionRecord->ExceptionAddress));
	ExceptionRecord64->ExceptionCode					= ExceptionRecord->ExceptionCode;
	ExceptionRecord64->ExceptionFlags					= ExceptionRecord->ExceptionFlags;
	ExceptionRecord64->ExceptionRecord					= reinterpret_cast<ULONG>(ExceptionRecord->ExceptionRecord);
	ExceptionRecord64->NumberParameters					= ExceptionRecord->NumberParameters;

	//
	// also sign extend
	//
	for(ULONG i = 0; i < ARRAYSIZE(ExceptionRecord->ExceptionInformation); i ++)
		ExceptionRecord64->ExceptionInformation[i]		= static_cast<ULONG64>(static_cast<LONG_PTR>(ExceptionRecord->ExceptionInformation[i]));
}

//
// compute checksum
//
ULONG BdComputeChecksum(__in PVOID Data,__in ULONG Length)
{
	ULONG Checksum										= 0;
	PUCHAR Buffer										= static_cast<PUCHAR>(Data);

	while(Length > 0)
	{
		Checksum										= Checksum + static_cast<ULONG>(*Buffer++);
		Length											-= 1;
	}

	return Checksum;
}

//
// read virtual memory
//
VOID BdReadVirtualMemory(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING MessageHeader;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_MANIPULATE_STATE64));
	MessageHeader.Buffer								= Add2Ptr(m,0,PCHAR);
	ULONG Length										= m->ReadMemory.TransferCount;
	PVOID Address										= reinterpret_cast<PVOID>(m->ReadMemory.TargetBaseAddress);
	if(Length > PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64))
		Length											= PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64);

	AdditionalData->Length								= static_cast<USHORT>(BdMoveMemory(AdditionalData->Buffer,Address,Length));
	m->ReturnStatus										= AdditionalData->Length == Length ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
	m->ReadMemory.ActualBytesRead						= AdditionalData->Length;

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&MessageHeader,AdditionalData);
}

//
// write virtual memory
//
VOID BdWriteVirtualMemory(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING MessageHeader;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_MANIPULATE_STATE64));
	MessageHeader.Buffer								= Add2Ptr(m,0,PCHAR);
	PVOID Address										= reinterpret_cast<PVOID>(m->WriteMemory.TargetBaseAddress);
	m->WriteMemory.ActualBytesWritten					= BdMoveMemory(Address,AdditionalData->Buffer,AdditionalData->Length);

	if(AdditionalData->Length == m->WriteMemory.ActualBytesWritten)
		m->ReturnStatus									= STATUS_SUCCESS;
	else
		m->ReturnStatus									= STATUS_UNSUCCESSFUL;

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&MessageHeader,0);
}

//
// get context
//
VOID BdGetContext(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING MessageHeader;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_MANIPULATE_STATE64));
	MessageHeader.Buffer								= Add2Ptr(m,0,PCHAR);
	m->ReturnStatus										= STATUS_SUCCESS;
	AdditionalData->Length								= sizeof(CONTEXT);

	BdCopyMemory(AdditionalData->Buffer,Context,sizeof(CONTEXT));

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&MessageHeader,AdditionalData);
}

//
// set context
//
VOID BdSetContext(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING MessageHeader;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_MANIPULATE_STATE64));
	MessageHeader.Buffer								= Add2Ptr(m,0,PCHAR);
	m->ReturnStatus										= STATUS_SUCCESS;

	BdCopyMemory(Context,AdditionalData->Buffer,sizeof(CONTEXT));

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&MessageHeader,0);
}

//
// write breakpoint
//
VOID BdWriteBreakpoint(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING MessageHeader;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_MANIPULATE_STATE64));
	MessageHeader.Buffer								= Add2Ptr(m,0,PCHAR);
	m->WriteBreakPoint.BreakPointHandle					= BdAddBreakpoint(m->WriteBreakPoint.BreakPointAddress);
	m->ReturnStatus										= m->WriteBreakPoint.BreakPointHandle ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&MessageHeader,0);
}

//
// restore breakpoint
//
VOID BdRestoreBreakpoint(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING MessageHeader;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_MANIPULATE_STATE64));
	MessageHeader.Buffer								= Add2Ptr(m,0,PCHAR);
	m->ReturnStatus										= BdDeleteBreakpoint(m->RestoreBreakPoint.BreakPointHandle) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&MessageHeader,0);
}

//
// read control space
//
VOID BdReadControlSpace(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING MessageHeader;
	PDBGKD_READ_MEMORY64 a								= &m->ReadMemory;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_MANIPULATE_STATE64));
	MessageHeader.Buffer								= Add2Ptr(m,0,PCHAR);
	ULONG Length										= a->TransferCount;
	if(a->TransferCount > PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64))
		Length											= PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64);

	if(static_cast<ULONG>(a->TargetBaseAddress) + Length <= sizeof(KPROCESSOR_STATE))
	{
		extern KPRCB BdPrcb;
		BdCopyMemory(AdditionalData->Buffer,Add2Ptr(&BdPrcb.ProcessorState.ContextFrame.ContextFlags,static_cast<ULONG>(a->TargetBaseAddress),PVOID),Length);
		AdditionalData->Length							= static_cast<USHORT>(Length);
		m->ReturnStatus									= STATUS_SUCCESS;
		a->ActualBytesRead								= Length;
	}
	else
	{
		AdditionalData->Length							= 0;
		m->ReturnStatus									= STATUS_UNSUCCESSFUL;
		a->ActualBytesRead								= 0;
	}

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&MessageHeader,AdditionalData);
}

//
// write control space
//
VOID BdWriteControlSpace(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING MessageHeader;
	PDBGKD_WRITE_MEMORY64 a								= &m->WriteMemory;
	ULONG Length										= a->TransferCount;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_MANIPULATE_STATE64));
	MessageHeader.Buffer								= Add2Ptr(m,0,PCHAR);
	if(Length >= AdditionalData->Length)
		Length											= AdditionalData->Length;

	if(static_cast<ULONG>(a->TargetBaseAddress) + Length <= sizeof(KPROCESSOR_STATE))
	{
		extern KPRCB BdPrcb;
		BdCopyMemory(Add2Ptr(&BdPrcb.ProcessorState.ContextFrame.ContextFlags,static_cast<ULONG>(a->TargetBaseAddress),PVOID),AdditionalData->Buffer,Length);
		m->ReturnStatus									= STATUS_SUCCESS;
		a->ActualBytesWritten							= Length;
	}
	else
	{
		m->ReturnStatus									= STATUS_UNSUCCESSFUL;
		a->ActualBytesWritten							= 0;
	}

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&MessageHeader,0);
}

//
// read port
//
VOID BdReadIoSpace(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING MessageHeader;
	PDBGKD_READ_WRITE_IO64 a							= &m->ReadWriteIo;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_MANIPULATE_STATE64));
	MessageHeader.Buffer								= Add2Ptr(m,0,PCHAR);
	m->ReturnStatus										= STATUS_SUCCESS;

	//
	// check size and alignment
	//
	switch(a->DataSize)
	{
	case 1:
		a->DataValue									= READ_PORT_UCHAR(reinterpret_cast<PUCHAR>(a->IoAddress));
		break;

	case 2:
		if(a->IoAddress & 1)
			m->ReturnStatus								= STATUS_DATATYPE_MISALIGNMENT;
		else
			a->DataValue								= READ_PORT_USHORT(reinterpret_cast<PUSHORT>(a->IoAddress));
		break;

	case 4:
		if(a->IoAddress & 3)
			m->ReturnStatus								= STATUS_DATATYPE_MISALIGNMENT;
		else
			a->DataValue								= READ_PORT_ULONG(reinterpret_cast<PULONG>(a->IoAddress));
		break;

	default:
		m->ReturnStatus									= STATUS_INVALID_PARAMETER;
		break;
	}

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&MessageHeader,0);
}

//
// outport io
//
VOID BdWriteIoSpace(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING MessageHeader;
	PDBGKD_READ_WRITE_IO64 a							= &m->ReadWriteIo;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_MANIPULATE_STATE64));
	MessageHeader.Buffer								= Add2Ptr(m,0,PCHAR);
	m->ReturnStatus										= STATUS_SUCCESS;

	//
	// check size and alignment
	//
	switch(a->DataSize)
	{
	case 1:
		WRITE_PORT_UCHAR(reinterpret_cast<PUCHAR>(a->IoAddress),static_cast<UCHAR>(a->DataValue & 0xff));
		break;

	case 2:
		if(a->IoAddress & 1)
			m->ReturnStatus								= STATUS_DATATYPE_MISALIGNMENT;
		else
			WRITE_PORT_USHORT(reinterpret_cast<PUSHORT	>(a->IoAddress),static_cast<USHORT>(a->DataValue & 0xffff));
		break;

	case 4:
		if(a->IoAddress & 3)
			m->ReturnStatus								= STATUS_DATATYPE_MISALIGNMENT;
		else
			WRITE_PORT_ULONG(reinterpret_cast<PULONG>(a->IoAddress),a->DataValue);
		break;

	default:
		m->ReturnStatus									= STATUS_INVALID_PARAMETER;
		break;
	}

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&MessageHeader,0);
}

//
// reboot
//
VOID BdReboot()
{
	HalpReboot();

	while(1);
}

//
// read physical memory
//
VOID BdReadPhysicalMemory(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING MessageHeader;
	PDBGKD_READ_MEMORY64 a								= &m->ReadMemory;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_MANIPULATE_STATE64));
	MessageHeader.Buffer								= Add2Ptr(m,0,PCHAR);
	ULONG StartAddress									= static_cast<ULONG>(a->TargetBaseAddress);
	ULONG ReadLength									= a->TransferCount;
	if(ReadLength > PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64))
		ReadLength										= PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64);

	ULONG EndAddress									= StartAddress + ReadLength;
	ULONG Count											= 0;

	if(PAGE_ALIGN(StartAddress) == PAGE_ALIGN(EndAddress))
	{
		PVOID Address									= BdTranslatePhysicalAddress(StartAddress);
		if(Address)
			AdditionalData->Length						= static_cast<USHORT>(BdMoveMemory(AdditionalData->Buffer,Address,ReadLength));
		else
			AdditionalData->Length						= 0;
	}
	else
	{
		do
		{
			ULONG LeftCount								= ReadLength;
			PCHAR Buffer								= AdditionalData->Buffer;
			PVOID Address								= BdTranslatePhysicalAddress(StartAddress);
			if(!Address)
			{
				AdditionalData->Length					= 0;
				break;
			}

			ULONG LengthThisRun							= PAGE_SIZE - BYTE_OFFSET(Address);
			AdditionalData->Length						+= static_cast<USHORT>(BdMoveMemory(Buffer,Address,LengthThisRun));
			LeftCount									-= LengthThisRun;
			StartAddress								+= LengthThisRun;
			Buffer										+= LengthThisRun;

			while(LeftCount)
			{
				Address									= BdTranslatePhysicalAddress(StartAddress);
				if(!Address)
					break;

				LengthThisRun							= PAGE_SIZE > LeftCount ? PAGE_SIZE : LeftCount;
				AdditionalData->Length					+= static_cast<USHORT>(BdMoveMemory(Buffer,Address,LengthThisRun));
				LeftCount								-= LengthThisRun;
				StartAddress							+= LengthThisRun;
				Buffer									+= LengthThisRun;
			}
		}while(0);
	}

	a->ActualBytesRead									= AdditionalData->Length;
	m->ReturnStatus										= ReadLength == AdditionalData->Length ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&MessageHeader,AdditionalData);
}

//
// write physical memory
//
VOID BdWritePhysicalMemory(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING MessageHeader;
	PDBGKD_WRITE_MEMORY64 a								= &m->WriteMemory;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_MANIPULATE_STATE64));
	MessageHeader.Buffer								= Add2Ptr(m,0,PCHAR);
	ULONG StartAddress									= static_cast<ULONG>(a->TargetBaseAddress);
	ULONG EndAddress									= StartAddress + static_cast<USHORT>(a->TransferCount);
	ULONG Count											= 0;

	if(PAGE_ALIGN(StartAddress) == PAGE_ALIGN(EndAddress))
	{
		Count											= BdMoveMemory(BdTranslatePhysicalAddress(StartAddress),AdditionalData->Buffer,a->TransferCount);
	}
	else
	{
		ULONG LeftCount									= a->TransferCount;
		PCHAR Buffer									= AdditionalData->Buffer;
		PVOID Address									= BdTranslatePhysicalAddress(StartAddress);
		ULONG LengthThisRun								= PAGE_SIZE - BYTE_OFFSET(Address);
		ULONG ThisRun									= BdMoveMemory(Address,Buffer,LengthThisRun);
		Count											+= ThisRun;
		LeftCount										-= LengthThisRun;
		StartAddress									+= LengthThisRun;
		Buffer											+= LengthThisRun;

		while(LeftCount)
		{
			LengthThisRun								= PAGE_SIZE > LeftCount ? PAGE_SIZE : LeftCount;
			ThisRun										= BdMoveMemory(BdTranslatePhysicalAddress(StartAddress),Buffer,LengthThisRun);
			Count										+= ThisRun;
			LeftCount									-= LengthThisRun;
			StartAddress								+= LengthThisRun;
			Buffer										+= LengthThisRun;
		}
	}

	a->ActualBytesWritten								= Count;
	m->ReturnStatus										= Count == AdditionalData->Length ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&MessageHeader,0);
}

//
// get version
//
VOID BdGetVersion(__in PDBGKD_MANIPULATE_STATE64 ManipulateState)
{
	RtlZeroMemory(&ManipulateState->GetVersion64,sizeof(ManipulateState->GetVersion64));

	ManipulateState->GetVersion64.MajorVersion			= static_cast<USHORT>(0x400 | (NtBuildNumber >> 0x1c));
	ManipulateState->GetVersion64.MinorVersion			= static_cast<USHORT>(NtBuildNumber & 0x0000ffff);
	ManipulateState->GetVersion64.ProtocolVersion		= DBGKD_64BIT_PROTOCOL_VERSION2;
	ManipulateState->GetVersion64.KdSecondaryVersion	= 0;
	ManipulateState->GetVersion64.Flags					= DBGKD_VERS_FLAG_DATA;
	ManipulateState->GetVersion64.MachineType			= IMAGE_FILE_MACHINE_I386;
	ManipulateState->GetVersion64.MaxPacketType			= PACKET_TYPE_MAX;
	ManipulateState->GetVersion64.MaxStateChange		= DbgKdCommandStringStateChange & 0xff;
	ManipulateState->GetVersion64.MaxManipulate			= DbgKdCheckLowMemoryApi & 0xff;
	ManipulateState->GetVersion64.DebuggerDataList		= 0;
	ManipulateState->GetVersion64.PsLoadedModuleList	= reinterpret_cast<ULONG64>(&BdModuleList);
	ManipulateState->GetVersion64.KernBase				= reinterpret_cast<ULONG64>(BdModuleDataTableEntry.DllBase);
	ManipulateState->ReturnStatus						= STATUS_SUCCESS;

	STRING Header;
	Header.Length										= sizeof(DBGKD_MANIPULATE_STATE64);
	Header.Buffer										= Add2Ptr(ManipulateState,0,PCHAR);

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&Header,0);
}

//
// write breakpoint ext
//
NTSTATUS BdWriteBreakPointEx(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING Header;
	PDBGKD_BREAKPOINTEX a								= &m->BreakPointEx;
	Header.Length										= sizeof(DBGKD_MANIPULATE_STATE64);
	Header.Buffer										= Add2Ptr(m,0,PCHAR);

	//
	// verify that the packet size is correct
	//
	if(AdditionalData->Length != a->BreakPointCount * sizeof(DBGKD_WRITE_BREAKPOINT64))
	{
		m->ReturnStatus									= STATUS_UNSUCCESSFUL;
		BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&Header,AdditionalData);
		return STATUS_UNSUCCESSFUL;
	}

	DBGKD_WRITE_BREAKPOINT64 BpBuf[BREAKPOINT_TABLE_SIZE];
	BdMoveMemory(BpBuf,AdditionalData->Buffer,a->BreakPointCount * sizeof(DBGKD_WRITE_BREAKPOINT64));

	m->ReturnStatus										= STATUS_SUCCESS;
	PDBGKD_WRITE_BREAKPOINT64 b							= BpBuf;
	for(ULONG i = 0; i < a->BreakPointCount; i ++, b ++)
	{
		if(b->BreakPointHandle)
		{
			if(!BdDeleteBreakpoint(b->BreakPointHandle))
				m->ReturnStatus							= STATUS_UNSUCCESSFUL;

			b->BreakPointHandle							= 0;
		}
	}

	b													= BpBuf;
	for(ULONG i = 0; i < a->BreakPointCount; i ++, b ++)
	{
		if(b->BreakPointAddress)
		{
			b->BreakPointHandle							= BdAddBreakpoint(b->BreakPointAddress);
			if(!b->BreakPointHandle)
				m->ReturnStatus							= STATUS_UNSUCCESSFUL;
		}
	}

	BdMoveMemory(AdditionalData->Buffer,BpBuf,a->BreakPointCount * sizeof(DBGKD_WRITE_BREAKPOINT64));

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&Header,AdditionalData);

	return a->ContinueStatus;
}

//
// restore breakpoint ex
//
VOID BdRestoreBreakPointEx(__in PDBGKD_MANIPULATE_STATE64 m,__in PSTRING AdditionalData,__in PCONTEXT Context)
{
	STRING Header;
	PDBGKD_BREAKPOINTEX a								= &m->BreakPointEx;
	Header.Length										= sizeof(DBGKD_MANIPULATE_STATE64);
	Header.Buffer										= Add2Ptr(m,0,PCHAR);

	//
	// verify that the packet size is correct
	//
	if(AdditionalData->Length != a->BreakPointCount * sizeof(DBGKD_RESTORE_BREAKPOINT))
	{
		m->ReturnStatus									= STATUS_UNSUCCESSFUL;
		BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&Header,AdditionalData);
		return;
	}

	DBGKD_RESTORE_BREAKPOINT BpBuf[BREAKPOINT_TABLE_SIZE];
	BdMoveMemory(BpBuf,AdditionalData->Buffer,a->BreakPointCount * sizeof(DBGKD_RESTORE_BREAKPOINT));

	m->ReturnStatus										= STATUS_SUCCESS;
	PDBGKD_RESTORE_BREAKPOINT b							= BpBuf;

	for(ULONG i = 0; i < a->BreakPointCount; i ++, b ++)
	{
		if(!BdDeleteBreakpoint(b->BreakPointHandle))
			m->ReturnStatus								= STATUS_UNSUCCESSFUL;
	}

	BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&Header,AdditionalData);
}

//
// send and wait
//
KCONTINUE_STATUS BdSendWaitContinue(__in ULONG PacketType,__in PSTRING OutputHead,__in PSTRING OutputData,__in PCONTEXT Context)
{
	STRING InputHead;
	STRING InputData;
	DBGKD_MANIPULATE_STATE64 ManipulateState;
	InputHead.MaximumLength								= sizeof(DBGKD_MANIPULATE_STATE64);
	InputHead.Buffer									= Add2Ptr(&ManipulateState,0,PCHAR);
	InputData.MaximumLength								= sizeof(BdMessageBuffer);
	InputData.Buffer									= BdMessageBuffer;
	BOOLEAN SendOutputPacket							= TRUE;

	while(1)
	{
		if(SendOutputPacket)
		{
			//
			// send output packet
			//
			BdSendPacket(PacketType,OutputHead,OutputData);

			//
			// debugger not present
			//
			if(BdDebuggerNotPresent)
				return ContinueSuccess;
		}

		ULONG ReplyCode									= KDP_PACKET_TIMEOUT;
		ULONG Length									= 0;
		SendOutputPacket								= FALSE;

		do
		{
			//
			// wait a reply packet
			//
			ReplyCode									= BdReceivePacket(PACKET_TYPE_KD_STATE_MANIPULATE,&InputHead,&InputData,&Length);

			//
			// peer wants us to resend output packat
			//
			if(ReplyCode == KDP_PACKET_RESEND)
				SendOutputPacket						= TRUE;

			//
			// loop if timeout
			//
		}while(ReplyCode == KDP_PACKET_TIMEOUT);

		//
		// resend packet
		//
		if(SendOutputPacket)
			continue;

		//
		// case on api number
		//
		switch(ManipulateState.ApiNumber)
		{
		case DbgKdReadVirtualMemoryApi:
			BdReadVirtualMemory(&ManipulateState,&InputData,Context);
			break;

		case DbgKdWriteVirtualMemoryApi:
			BdWriteVirtualMemory(&ManipulateState,&InputData,Context);
			break;

		case DbgKdGetContextApi:
			BdGetContext(&ManipulateState,&InputData,Context);
			break;

		case DbgKdSetContextApi:
			BdSetContext(&ManipulateState,&InputData,Context);
			break;

		case DbgKdWriteBreakPointApi:
			BdWriteBreakpoint(&ManipulateState,&InputData,Context);
			break;

		case DbgKdRestoreBreakPointApi:
			BdRestoreBreakpoint(&ManipulateState,&InputData,Context);
			break;

		case DbgKdContinueApi:
			return NT_SUCCESS(ManipulateState.Continue.ContinueStatus) ? ContinueSuccess : ContinueError;
			break;

		case DbgKdReadControlSpaceApi:
			BdReadControlSpace(&ManipulateState,&InputData,Context);
			break;

		case DbgKdWriteControlSpaceApi:
			BdWriteControlSpace(&ManipulateState,&InputData,Context);
			break;

		case DbgKdReadIoSpaceApi:
			BdReadIoSpace(&ManipulateState,&InputData,Context);
			break;

		case DbgKdWriteIoSpaceApi:
			BdWriteIoSpace(&ManipulateState,&InputData,Context);
			break;

		case DbgKdRebootApi:
			BdReboot();
			break;

		case DbgKdContinueApi2:
			if(!NT_SUCCESS(ManipulateState.Continue2.ContinueStatus))
				return ContinueError;

			BdGetStateChange(&ManipulateState,Context);
			return ContinueSuccess;
			break;

		case DbgKdReadPhysicalMemoryApi:
			BdReadPhysicalMemory(&ManipulateState,&InputData,Context);
			break;

		case DbgKdWritePhysicalMemoryApi:
			BdWritePhysicalMemory(&ManipulateState,&InputData,Context);
			break;

		case DbgKdGetVersionApi:
			BdGetVersion(&ManipulateState);
			break;

		case DbgKdWriteBreakPointExApi:
			if(BdWriteBreakPointEx(&ManipulateState,&InputData,Context) != STATUS_SUCCESS)
				return ContinueError;
			break;

		case DbgKdRestoreBreakPointExApi:
			BdRestoreBreakPointEx(&ManipulateState,&InputData,Context);
			break;

		default:
			InputData.Length							= 0;
			ManipulateState.ReturnStatus				= STATUS_UNSUCCESSFUL;
			BdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,&InputHead,&InputData);
			break;
		}
	}
}

//
// report exception state change
//
VOID BdReportExceptionStateChange(__in PEXCEPTION_RECORD ExceptionRecord,__in PCONTEXT Context)
{
	KCONTINUE_STATUS Status;

	do
	{
		//
		// setup common field
		//
		DBGKD_WAIT_STATE_CHANGE64 WaitStateChange;
		BdpSetCommonState(DbgKdExceptionStateChange,Context,&WaitStateChange);

		//
		// convert 32bits to 64bits
		//
		ExceptionRecord32To64(ExceptionRecord,&WaitStateChange.u.Exception.ExceptionRecord);

		//
		// setup control report
		//
		BdSetStateChange(&WaitStateChange,ExceptionRecord,Context);

		STRING MessageHeader;
		MessageHeader.Length							= sizeof(DBGKD_WAIT_STATE_CHANGE64);
		MessageHeader.Buffer							= Add2Ptr(&WaitStateChange,0,PCHAR);

		STRING MessageData;
		MessageData.Length								= 0;
		MessageData.MaximumLength						= 0;

		//
		// send it,and wait a reply
		//
		Status											= BdSendWaitContinue(PACKET_TYPE_KD_STATE_CHANGE64,&MessageHeader,&MessageData,Context);
	}while(Status == ContinueProcessorReselected);
}

//
// report load symbols state change
//
VOID BdReportLoadSymbolsStateChange(__in PSTRING Name,__in PKD_SYMBOLS_INFO SymInfo,__in BOOLEAN Unload,__in PCONTEXT Context)
{
	KCONTINUE_STATUS Status;

	do
	{
		//
		// setup common field
		//
		DBGKD_WAIT_STATE_CHANGE64 WaitStateChange;
		BdpSetCommonState(DbgKdLoadSymbolsStateChange,Context,&WaitStateChange);

		//
		// setup control report
		//
		BdSetContextState(&WaitStateChange,Context);

		STRING MessageHeader;
		MessageHeader.Length							= sizeof(DBGKD_WAIT_STATE_CHANGE64);
		MessageHeader.Buffer							= Add2Ptr(&WaitStateChange,0,PCHAR);

		STRING MessageData;
		MessageData.Length								= 0;
		MessageData.Buffer								= 0;

		if(Name)
		{
			ULONG Length								= BdMoveMemory(BdMessageBuffer,Name->Buffer,Name->Length);
			BdMessageBuffer[Length]						= 0;
			MessageData.Buffer							= BdMessageBuffer;
			MessageData.Length							= static_cast<USHORT>(Length + 1);
		}

		WaitStateChange.u.LoadSymbols.PathNameLength	= MessageData.Length;
		WaitStateChange.u.LoadSymbols.UnloadSymbols		= Unload;
		WaitStateChange.u.LoadSymbols.BaseOfDll			= static_cast<ULONG64>(static_cast<LONG>(SymInfo->BaseOfDll));
		WaitStateChange.u.LoadSymbols.CheckSum			= SymInfo->CheckSum;
		WaitStateChange.u.LoadSymbols.ProcessId			= SymInfo->ProcessId;
		WaitStateChange.u.LoadSymbols.SizeOfImage		= SymInfo->SizeOfImage;

		//
		// send it,and wait a reply
		//
		Status											= BdSendWaitContinue(PACKET_TYPE_KD_STATE_CHANGE64,&MessageHeader,&MessageData,Context);
	}while(Status == ContinueProcessorReselected);
}

//
// poll break in
//
BOOLEAN BdPollBreakIn()
{
	//
	// if the debugger is enabled, see if a breakin by the kernel debugger is pending.
	//
	BOOLEAN BreakIn										= FALSE;
	if(BdDebuggerEnabled)
	{
		if(BdControlCPending)
		{
			BreakIn										= TRUE;
			BdControlCPending							= FALSE;
		}
		else
		{
			BreakIn										= BdReceivePacket(PACKET_TYPE_KD_POLL_BREAKIN,0,0,0) == KDP_PACKET_RECEIVED;
		}

		if(BreakIn)
			BdControlCPressed							= TRUE;
	}

	return BreakIn;
}

//
// poll connection
//
VOID BdPollConnection()
{
	if(!BdPollBreakIn())
		return;

	DbgPrint("User requested boot debugger break!\r\n");
	DbgBreakPoint();
}
//
// print string
//
BOOLEAN BdPrintString(__in PSTRING String)
{
	//
	// move the output string to the message buffer.
	//
	ULONG Length										= BdMoveMemory(BdMessageBuffer,String->Buffer,String->Length);

	//
	// if the total message length is greater than the maximum packet size,then truncate the output string.
	//
	if(sizeof(DBGKD_DEBUG_IO) + Length > PACKET_MAX_SIZE)
		Length											= PACKET_MAX_SIZE - sizeof(DBGKD_DEBUG_IO);

	//
	// construct the print string message and message descriptor.
	//
	DBGKD_DEBUG_IO DebugIo;
	DebugIo.ApiNumber									= DbgKdPrintStringApi;
	DebugIo.ProcessorLevel								= 0;
	DebugIo.Processor									= 0;
	DebugIo.u.PrintString.LengthOfString				= Length;

	//
	// construct the print string header
	//
	STRING MessageHeader;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_DEBUG_IO));
	MessageHeader.Buffer								= Add2Ptr(&DebugIo,0,PCHAR);

	//
	// construct the print string data and data descriptor.
	//
	STRING MessageData;
	MessageData.Length									= static_cast<USHORT>(Length);
	MessageData.Buffer									= BdMessageBuffer;

	//
	// send packet to the kernel debugger on the host machine.
	//
	BdSendPacket(PACKET_TYPE_KD_DEBUG_IO,&MessageHeader,&MessageData);

	return BdPollBreakIn();
}

//
// prompt string
//
BOOLEAN BdPromptString(__in PSTRING InputString,__in PSTRING OutputString)
{
	//
	// move the output string to the message buffer.
	//
	ULONG Length										= BdMoveMemory(BdMessageBuffer,InputString->Buffer,InputString->Length);

	//
	// if the total message length is greater than the maximum packet size,then truncate the output string.
	//
	if(sizeof(DBGKD_DEBUG_IO) + Length > PACKET_MAX_SIZE)
		Length											= PACKET_MAX_SIZE - sizeof(DBGKD_DEBUG_IO);

	//
	// construct the prompt string message and message descriptor.
	//
	DBGKD_DEBUG_IO DebugIo;
	DebugIo.ApiNumber									= DbgKdGetStringApi;
	DebugIo.ProcessorLevel								= 0;
	DebugIo.Processor									= 0;
	DebugIo.u.GetString.LengthOfPromptString			= Length;
	DebugIo.u.GetString.LengthOfStringRead				= OutputString->MaximumLength;

	//
	// construct the prompt string header
	//
	STRING MessageHeader;
	MessageHeader.Length								= static_cast<USHORT>(sizeof(DBGKD_DEBUG_IO));
	MessageHeader.Buffer								= Add2Ptr(&DebugIo,0,PCHAR);

	//
	// construct the prompt string data and data descriptor.
	//
	STRING MessageData;
	MessageData.Length									= static_cast<USHORT>(Length);
	MessageData.Buffer									= BdMessageBuffer;

	//
	// send packet to the kernel debugger on the host machine.
	//
	BdSendPacket(PACKET_TYPE_KD_DEBUG_IO,&MessageHeader,&MessageData);

	//
	// receive packet from the kernel debugger on the host machine.
	//
	MessageHeader.MaximumLength							= sizeof(DBGKD_DEBUG_IO);
	MessageData.MaximumLength							= sizeof(BdMessageBuffer);

	ULONG ReplyCode										= KDP_PACKET_TIMEOUT;
	do
	{
		ReplyCode = BdReceivePacket(PACKET_TYPE_KD_DEBUG_IO,&MessageHeader,&MessageData,&Length);

		//
		// let BdTrap recall us again
		//
		if(ReplyCode == KDP_PACKET_RESEND)
			return TRUE;

	}while(ReplyCode != KDP_PACKET_RECEIVED);

	//
	// copy to output buffer
	//
	if(Length > OutputString->MaximumLength)
		Length											= OutputString->MaximumLength;

	OutputString->Length								= static_cast<USHORT>(BdMoveMemory(OutputString->Buffer,BdMessageBuffer,Length));

	return FALSE;
}

//
// create remote file
//
NTSTATUS BdCreateRemoteFile(__out PULONG RemoteFileId,__out PULONG64 FileSize,__in PCHAR FileName,__in ULONG DesiredAccess,
							__in ULONG FileAttribute,__in ULONG ShareAccess,__in ULONG CreateDisposition,__in ULONG CreateOptions)
{
	//
	// debugger should be enabled
	//
	if(!BdDebuggerEnabled)
		return STATUS_DEBUGGER_INACTIVE;

	//
	// file name is too long
	//
	if(!FileName || strlen(FileName) > PACKET_MAX_SIZE)
	{
		DbgPrint("BdCreateRemoteFile: Bad parameter\n");
		return STATUS_INVALID_PARAMETER;
	}

	//
	// debugger is not present
	//
	if(BdDebuggerNotPresent)
		return STATUS_DEBUGGER_INACTIVE;

	//
	// find an empty slot
	//
	ULONG RemoteFileIndex								= 0xffffffff;
	for(ULONG i = 0; i < ARRAYSIZE(BdRemoteFiles); i ++)
	{
		if(!BdRemoteFiles[i])
		{
			RemoteFileIndex								= i;
			break;
		}
	}

	//
	// unable to find an empty slot
	//
	if(RemoteFileIndex >= ARRAYSIZE(BdRemoteFiles))
	{
		DbgPrint("BdCreateRemoteFile: No more empty handles available for this file.\n");
		return STATUS_NO_MEMORY;
	}

	//
	// setup packet
	//
	STRING FileNameString;
	RtlInitAnsiString(&FileNameString,FileName);

	UNICODE_STRING UnicodeFileNameString;
	UnicodeFileNameString.Buffer						= static_cast<PWCHAR>(static_cast<PVOID>(BdFileTransferBuffer));
	UnicodeFileNameString.MaximumLength					= sizeof(BdFileTransferBuffer);

	RtlAnsiStringToUnicodeString(&UnicodeFileNameString,&FileNameString,FALSE);

	DBGKD_FILE_IO FileIo;
	FileIo.ApiNumber									= DbgKdCreateFileApi;
	FileIo.ReturnStatus									= STATUS_UNSUCCESSFUL;
	FileIo.CreateFile.CreateDisposition					= CreateDisposition;
	FileIo.CreateFile.CreateOptions						= CreateOptions;
	FileIo.CreateFile.DesiredAccess						= DesiredAccess;
	FileIo.CreateFile.FileAttributes					= FileAttribute;
	FileIo.CreateFile.ShareAccess						= ShareAccess;
	FileIo.CreateFile.Handle							= 0;
	FileIo.CreateFile.Length							= 0;

	STRING MessageHeader;
	MessageHeader.Length								= sizeof(FileIo);
	MessageHeader.MaximumLength							= MessageHeader.Length;
	MessageHeader.Buffer								= static_cast<PCHAR>(static_cast<PVOID>(&FileIo));

	STRING MessageData;
	MessageData.Length									= static_cast<USHORT>(strlen(FileName) * sizeof(WCHAR) + sizeof(WCHAR));
	MessageData.Buffer									= BdFileTransferBuffer;

	//
	// send packet
	//
	BdSendPacket(PACKET_TYPE_KD_FILE_IO,&MessageHeader,&MessageData);
	if(BdDebuggerNotPresent)
		return STATUS_DEBUGGER_INACTIVE;

	//
	// wait reply
	//
	ULONG RetryCount;
	MessageData.Buffer									= BdFileTransferBuffer;
	MessageData.MaximumLength							= sizeof(BdFileTransferBuffer);
	ULONG Length										= 0;
	ULONG ReplyCode										= KDP_PACKET_TIMEOUT;
	for(RetryCount = 0; RetryCount < 10; RetryCount ++)
	{
		ReplyCode										= BdReceivePacket(PACKET_TYPE_KD_FILE_IO,&MessageHeader,&MessageData,&Length);
		if(ReplyCode != KDP_PACKET_TIMEOUT)
			break;
	}

	//
	// failed
	//
	if(ReplyCode != KDP_PACKET_RECEIVED || !NT_SUCCESS(FileIo.ReturnStatus))
		return STATUS_INVALID_PARAMETER;

	//
	// save file handle
	//
	BdRemoteFiles[RemoteFileIndex]						= FileIo.CreateFile.Handle;

	//
	// file id is index + 1
	//
	*RemoteFileId										= RemoteFileIndex + 1;

	//
	// return file size
	//
	*FileSize											= FileIo.CreateFile.Length;

	return STATUS_SUCCESS;
}

//
// read remote file
//
NTSTATUS BdReadRemoteFile(__in ULONG RemoteFileId,__in ULONG64 Offset,__out PVOID Buffer,__in ULONG Length,__out PULONG ActualCount)
{
	*ActualCount										= 0;

	//
	// debugger should be enabled
	//
	if(!BdDebuggerEnabled)
		return STATUS_DEBUGGER_INACTIVE;

	//
	// check remote file id
	//
	ULONG RemoteFileIndex								= RemoteFileId - 1;
	if(RemoteFileIndex >= ARRAYSIZE(BdRemoteFiles) || !BdRemoteFiles[RemoteFileIndex])
	{
		DbgPrint("BdReadRemoteFile: Bad parameters!\n");
		return STATUS_INVALID_PARAMETER;
	}

	//
	// setup packet
	//
	DBGKD_FILE_IO FileIo;
	FileIo.ApiNumber									= DbgKdReadFileApi;
	FileIo.ReturnStatus									= STATUS_UNSUCCESSFUL;
	FileIo.ReadFile.Handle								= BdRemoteFiles[RemoteFileIndex];
	FileIo.ReadFile.Offset								= Offset;

	STRING MessageHeader;
	MessageHeader.Length								= sizeof(FileIo);
	MessageHeader.MaximumLength							= MessageHeader.Length;
	MessageHeader.Buffer								= static_cast<PCHAR>(static_cast<PVOID>(&FileIo));

	while(Length)
	{
		//
		// set read length
		//
		FileIo.ReadFile.Length							= Length > PACKET_MAX_SIZE - sizeof(FileIo) ? PACKET_MAX_SIZE - sizeof(FileIo) : Length;

		//
		// send packet
		//
		BdSendPacket(PACKET_TYPE_KD_FILE_IO,&MessageHeader,0);

		//
		// setup data
		//
		STRING MessageData;
		MessageData.Buffer								= static_cast<PCHAR>(Buffer);
		MessageData.MaximumLength						= static_cast<USHORT>(FileIo.ReadFile.Length);

		//
		// wait reply
		//
		while(1)
		{
			ULONG ReadLength							= 0;
			ULONG ReplyCode								= BdReceivePacket(PACKET_TYPE_KD_FILE_IO,&MessageHeader,&MessageData,&ReadLength);
			if(ReplyCode == KDP_PACKET_TIMEOUT)
				continue;

			//
			// retry current chunk
			//
			if(ReplyCode != KDP_PACKET_RECEIVED)
				break;

			//
			// failed
			//
			if(!NT_SUCCESS(FileIo.ReturnStatus))
				return FileIo.ReturnStatus;

			//
			// read next chunk
			//
			Length										-= ReadLength;
			FileIo.ReadFile.Offset						+= ReadLength;
			Buffer										= Add2Ptr(Buffer,ReadLength,PVOID);
			*ActualCount								+= ReadLength;
			break;
		}
	}

	return STATUS_SUCCESS;
}

//
// close remote file
//
NTSTATUS BdCloseRemoteFile(__in ULONG RemoteFileId)
{
	//
	// debugger should be enabled
	//
	if(!BdDebuggerEnabled)
		return STATUS_DEBUGGER_INACTIVE;

	//
	// check remote file id
	//
	ULONG RemoteFileIndex								= RemoteFileId - 1;
	if(RemoteFileIndex >= ARRAYSIZE(BdRemoteFiles) || !BdRemoteFiles[RemoteFileIndex])
		return STATUS_INVALID_PARAMETER;

	//
	// setup packet
	//
	DBGKD_FILE_IO FileIo;
	FileIo.ApiNumber									= DbgKdCloseFileApi;
	FileIo.ReturnStatus									= STATUS_UNSUCCESSFUL;
	FileIo.CloseFile.Handle								= BdRemoteFiles[RemoteFileIndex];

	STRING MessageHeader;
	MessageHeader.Length								= sizeof(FileIo);
	MessageHeader.MaximumLength							= MessageHeader.Length;
	MessageHeader.Buffer								= static_cast<PCHAR>(static_cast<PVOID>(&FileIo));

	while(1)
	{
		//
		// send packet
		//
		BdSendPacket(PACKET_TYPE_KD_FILE_IO,&MessageHeader,0);

		//
		// wait reply
		//
		STRING MessageData;
		MessageData.MaximumLength						= sizeof(BdMessageBuffer);
		MessageData.Buffer								= BdMessageBuffer;

		while(1)
		{
			ULONG Length								= 0;
			ULONG ReplyCode								= BdReceivePacket(PACKET_TYPE_KD_FILE_IO,&MessageHeader,&MessageData,&Length);
			if(ReplyCode == KDP_PACKET_TIMEOUT)
				continue;

			//
			// resend close packet
			//
			if(ReplyCode != KDP_PACKET_RECEIVED)
				break;

			//
			// clear handle array
			//
			if(NT_SUCCESS(FileIo.ReturnStatus))
				BdRemoteFiles[RemoteFileIndex]			= 0;

			return FileIo.ReturnStatus;
		}
	}

	return STATUS_SUCCESS;
}

//
// pull remote file
//
NTSTATUS BdPullRemoteFile(__in PCHAR FileName,__in ULONG Arg4,__in ULONG Arg8,__in ULONG ArgC,__in ULONG FileId)
{
	//
	// debugger should be enabled
	//
	if(!BdDebuggerEnabled)
		return STATUS_DEBUGGER_INACTIVE;

	ULONG RemoteFileId									= 0;
	ULONG BasePage										= 0;
	NTSTATUS Status										= STATUS_SUCCESS;

	__try
	{
		//
		// try to open the remote file
		//	FILE_GENERIC_READ		= 0x120089
		//	FILE_ATTRIBUTE_NORMAL	= 0x80
		//	FILE_SHARE_READ			= 0x01
		//	FILE_OPEN				= 0x01
		//
		ULONG64 FileSize								= 0;
		Status											= BdCreateRemoteFile(&RemoteFileId,&FileSize,FileName,0x120089,0x80,1,1,0);
		if(!NT_SUCCESS(Status))
			try_leave(NOTHING);

		//
		// allocate memory
		//
		ULONG Count										= static_cast<ULONG>((FileSize + PAGE_SIZE - 1) >> PAGE_SHIFT);
		ARC_STATUS ArcStatus							= BlAllocateAlignedDescriptor(LoaderFirmwareTemporary,0,Count,1,&BasePage);
		if(ArcStatus != ESUCCESS)
			try_leave(DbgPrint("BdPullRemoteFile: BlAllocateAlignedDescriptor failed! (%x)\n",ArcStatus);Status = STATUS_INSUFFICIENT_RESOURCES);

		//
		// read all file data in
		//
		ULONG64 Offset									= 0;
		PVOID Buffer									= reinterpret_cast<PVOID>(BasePage << PAGE_SHIFT);
		while(FileSize > Offset)
		{
			//
			// max read size is two pages
			//
			ULONG64 ReadSize							= FileSize - Offset;
			if(ReadSize > PAGE_SIZE * 2)
				ReadSize								= PAGE_SIZE * 2;

			//
			// read file
			//
			ULONG ActualSize							= 0;
			Status										= BdReadRemoteFile(RemoteFileId,Offset,Buffer,static_cast<ULONG>(ReadSize),&ActualSize);
			if(!NT_SUCCESS(Status) || !ActualSize)
				try_leave(DbgPrint("BdPullRemoteFile: BdReadRemoteFile failed! (%x)\n",Status);Status = STATUS_IO_DEVICE_ERROR);

			Offset										+= ActualSize;
			Buffer										= Add2Ptr(Buffer,ActualSize,PVOID);
		}

		//
		// update file table
		// HACKHACKHACK,should we define a debug fs device entry instead of reusing net device entry?
		//
		extern BL_DEVICE_ENTRY_TABLE NetDeviceEntryTable;

		BlFileTable[FileId].Flags.Open					= TRUE;
		BlFileTable[FileId].Flags.Read					= TRUE;
		BlFileTable[FileId].Position.QuadPart			= 0;
		BlFileTable[FileId].DeviceId					= 'dten';
		BlFileTable[FileId].StructureContext			= 0;
		BlFileTable[FileId].u.NetFileContext.FileSize	= static_cast<ULONG>(FileSize);
		BlFileTable[FileId].u.NetFileContext.FileBuffer	= reinterpret_cast<PVOID>(BasePage << PAGE_SHIFT);
		BlFileTable[FileId].DeviceEntryTable			= &NetDeviceEntryTable;

		NetInitialize();

		DbgPrint("<?dml?><col fg=\"emphfg\">BD: Loaded remote file %s</col>\n",FileName);
	}
	__finally
	{
		if(!NT_SUCCESS(Status))
		{
			if(RemoteFileId)
				BdCloseRemoteFile(RemoteFileId);

			if(BasePage)
				BlFreeDescriptor(BasePage);
		}
	}

	return Status;
}