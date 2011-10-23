//********************************************************************
//	created:	11:8:2008   4:02
//	file:		debug.cpp
//	author:		tiamo
//	purpose:	debug support
//********************************************************************

#include "stdafx.h"

//
// debug routine
//
BOOLEAN (*BdDebugRoutine)(__in PEXCEPTION_RECORD ExceptionRecord,__in CCHAR PreviousMode,__in PKTRAP_FRAME TrapFrame);

//
// dummy prcb
//
KPRCB													BdPrcb;

//
// debugger enabled
//
BOOLEAN													BdDebuggerEnabled;

//
// debug buffer
//
CHAR													DebugMessage[4096];

//
// debugger not present
//
extern BOOLEAN											BdDebuggerNotPresent;

//
// debug breakpoint
//
VOID __declspec(naked) DbgBreakPoint()
{
	__asm
	{
		int		3
		retn
	}
}

//
// user break point
//
VOID __declspec(naked) DbgUserBreakPoint()
{
	__asm
	{
		int		3
		retn
	}
}

//
// break with status
//
VOID __declspec(naked) DbgBreakPointWithStatus(__in NTSTATUS Status)
{
	__asm
	{
		mov     eax, [esp+4]
		int		3
		retn	4
	}
}

//
// break with status instruction
//
VOID __declspec(naked) RtlpBreakWithStatusInstruction(__in NTSTATUS Status)
{
	__asm
	{
		int		3
		retn	4
	}
}

//
// debug service
//
VOID __declspec(naked) DebugService(__in ULONG ServiceType,__in ULONG Info1,__in ULONG Info2,__in ULONG Info3,__in ULONG Info4)
{
	__asm
	{
		push	ebp
		mov		ebp,esp
		push    ecx
		push    ebx
		push    edi
		mov		eax,[ebp + 0x08]
		mov		ecx,[ebp + 0x0c]
		mov		edx,[ebp + 0x10]
		mov		ebx,[ebp + 0x14]
		mov		edi,[ebp + 0x18]
		int		0x2d
		int		3
		pop		edi
		pop		ebx
		leave
		retn	0x14
	}
}

//
// debug service2
//
VOID __declspec(naked) DebugService2(__in PVOID Info1,__in PVOID Info2,__in ULONG ServiceType)
{
	__asm
	{
		push	ebp
		mov		ebp,esp
		mov		eax,[ebp + 0x10]
		mov		ecx,[ebp + 0x08]
		mov		edx,[ebp + 0x0c]
		int		0x2d
		int		3
		leave
		retn	0x0c
	}
}

//
// debug print
//
VOID DebugPrint(__in PSTRING Output,__in ULONG ComponentId,__in ULONG Level)
{
	DebugService(BREAKPOINT_PRINT,reinterpret_cast<ULONG>(Output->Buffer),Output->Length,ComponentId,Level);
}

//
// load symbols
//
VOID DbgLoadImageSymbols(__in PSTRING FileName,__in PVOID ImageBase,__in ULONG ProcessId)
{
	KD_SYMBOLS_INFO Info;
	PIMAGE_NT_HEADERS ImageNtHeader						= RtlImageNtHeader(ImageBase);
	Info.BaseOfDll										= reinterpret_cast<ULONG>(ImageBase);
	Info.ProcessId										= ProcessId;
	Info.CheckSum										= ImageNtHeader ? ImageNtHeader->OptionalHeader.CheckSum : 0;
	Info.SizeOfImage									= ImageNtHeader ? ImageNtHeader->OptionalHeader.SizeOfImage : 0x100000;

	DebugService2(FileName,&Info,BREAKPOINT_LOAD_SYMBOLS);
}

//
// unload symbols
//
VOID DbgUnLoadImageSymbols(__in PSTRING FileName,__in PVOID ImageBase,__in ULONG ProcessId)
{
	KD_SYMBOLS_INFO Info;
	Info.BaseOfDll										= reinterpret_cast<ULONG>(ImageBase);
	Info.ProcessId										= ProcessId;
	Info.CheckSum										= 0;
	Info.SizeOfImage									= 0;

	DebugService2(FileName,&Info,BREAKPOINT_UNLOAD_SYMBOLS);
}

//
// dbg print
//
ULONG DbgPrint(__in PCCH Format,...)
{
	va_list list;
	va_start(list,Format);

	vsprintf(DebugMessage,Format,list);

	va_end(list);

	STRING Output;
	Output.Length										= strlen(DebugMessage);
	Output.MaximumLength								= sizeof(DebugMessage);
	Output.Buffer										= DebugMessage;

	DebugPrint(&Output,0,0);

	return Output.Length;
}

//
// return from exception handler
//
VOID __declspec(naked) BdExit()
{
	__asm
	{
		lea     esp, [ebp+30h]
		pop     gs
		pop     es
		pop     ds
		pop     edx
		pop     ecx
		pop     eax
		add     esp, 8																// skip exception list and previous previous mode
		pop     fs
		pop     edi
		pop     esi
		pop     ebx
		pop     ebp
		add     esp, 4																// skip error code
		iretd
	}
}

//
// common dispatch
//
VOID __declspec(naked) BdDispatch()
{
	__asm
	{
		sub     esp,size EXCEPTION_RECORD											// sizeof(EXCEPTION_RECORD) = 0x50
		mov     [esp+EXCEPTION_RECORD.ExceptionCode], eax							// ExceptionCode
		xor     eax, eax
		mov     [esp+EXCEPTION_RECORD.ExceptionFlags], eax							// ExceptionFlags
		mov     [esp+EXCEPTION_RECORD.ExceptionRecord], eax							// ExceptionRecord
		mov     [esp+EXCEPTION_RECORD.ExceptionAddress], ebx						// ExceptionAddress
		mov     [esp+EXCEPTION_RECORD.NumberParameters], ecx						// NumberParameters
		mov     [esp+EXCEPTION_RECORD.ExceptionInformation], edx					// ExceptionInformation0
		mov     [esp+EXCEPTION_RECORD.ExceptionInformation+4], edi					// ExceptionInformation1
		mov     [esp+EXCEPTION_RECORD.ExceptionInformation+8], esi					// ExceptionInformation2
		mov     eax, dr0
		mov     [ebp+KTRAP_FRAME.Dr0], eax											// dr0
		mov     eax, dr1
		mov     [ebp+KTRAP_FRAME.Dr1], eax											// dr1
		mov     eax, dr2
		mov     [ebp+KTRAP_FRAME.Dr2], eax											// dr2
		mov     eax, dr3
		mov     [ebp+KTRAP_FRAME.Dr3], eax											// dr3
		mov     eax, dr6
		mov     [ebp+KTRAP_FRAME.Dr6], eax											// dr6
		mov     eax, dr7
		mov     [ebp+KTRAP_FRAME.Dr7], eax											// dr7
		mov     ax, ss
		mov     [ebp+KTRAP_FRAME.TempSegCs], eax									// TempSegCs
		mov     [ebp+KTRAP_FRAME.TempEsp], ebp										// TempEsp
		add     dword ptr [ebp+KTRAP_FRAME.TempEsp],0x74							// 0x74 = FIELD_OFFSET(KTRAP_FRAME,HardwareEsp)
		mov     ecx, esp
		push    ebp																	// trap frame
		push    0																	// kernel mode
		push    ecx																	// exception record
		call    BdDebugRoutine
		add     esp,size EXCEPTION_RECORD
		retn
	}
}

//
// single step exception
//
VOID __declspec(naked) BdTrap01()
{
	__asm
	{
		push    0																	// dummy error code
		push    ebp
		push    ebx
		push    esi
		push    edi
		push    fs
		push    0ffffffffh															// ExceptionList
		push    0ffffffffh															// PreviousPreviousMode
		push    eax
		push    ecx
		push    edx
		push    ds
		push    es
		push    gs
		sub     esp, 30h
		mov     ebp, esp
		cld
		and     dword ptr [ebp+KTRAP_FRAME.EFlags], 0fffffeffh						// clear single step flag
		mov     eax, 80000004h														// EXCEPTION_SINGLE_STEP
		mov     ebx, [ebp+KTRAP_FRAME.Eip]											// exception address = Eip
		xor     ecx, ecx															// param count = 0
		call    BdDispatch
		jmp     BdExit
	}
}

//
// breakpoint exception
//
VOID __declspec(naked) BdTrap03()
{
	__asm
	{
		push    0
		push    ebp
		push    ebx
		push    esi
		push    edi
		push    fs
		push    0ffffffffh											// ExceptionList
		push    0ffffffffh											// PreviousPreviousMode
		push    eax
		push    ecx
		push    edx
		push    ds
		push    es
		push    gs
		sub     esp, 30h
		mov     ebp, esp
		cld
		dec		dword ptr [ebp+KTRAP_FRAME.Eip]						// point to breakpoint instruction
		mov     eax, 80000003h										// EXCEPTION_BREAKPOINT
		mov     ebx, [ebp+KTRAP_FRAME.Eip]							// exception address = Eip
		mov		ecx,0												// param count
		xor     edx, edx
		call    BdDispatch
		jmp     BdExit
	}
}

//
// general protection
//
VOID __declspec(naked) BdTrap0d()
{
	__asm
	{
		push    ebp
		push    ebx
		push    esi
		push    edi
		push    fs
		push    0ffffffffh
		push    0ffffffffh
		push    eax
		push    ecx
		push    edx
		push    ds
		push    es
		push    gs
		sub     esp, 30h
		mov     ebp, esp
		cld
	loop_for:
		mov     eax, 0c0000005h										// EXCEPTION_ACCESS_VIOLATION
		mov     ebx, [ebp+KTRAP_FRAME.Eip]							// exception address = eip
		mov     ecx, 1												// param count = 1
		mov     edx, [ebp+KTRAP_FRAME.ErrCode]						// hardware error code
		and     edx, 0ffffh
		call    BdDispatch
		jmp     loop_for
	}
}

//
// page fault
//
VOID __declspec(naked) BdTrap0e()
{
	__asm
	{
		push    ebp
		push    ebx
		push    esi
		push    edi
		push    fs
		push    0ffffffffh
		push    0ffffffffh
		push    eax
		push    ecx
		push    edx
		push    ds
		push    es
		push    gs
		sub     esp, 30h
		mov     ebp, esp
		cld
	loop_for:
		mov     eax, 0c0000005h										// EXCEPTION_ACCESS_VIOLATION
		mov     ebx, [ebp+KTRAP_FRAME.Eip]							// exception address = eip
		mov     ecx, 3												// param count = 3
		mov     edx, [ebp+KTRAP_FRAME.ErrCode]						// hardware error code
		and     edx, 2												// read or write
		mov     edi, cr2											// reference memory location
		xor		esi,esi
		call    BdDispatch
		jmp     loop_for
	}
}

//
// debug service
//
VOID __declspec(naked) BdTrap2d()
{
	__asm
	{
		push    0
		push    ebp
		push    ebx
		push    esi
		push    edi
		push    fs
		push    0ffffffffh
		push    0ffffffffh
		push    eax
		push    ecx
		push    edx
		push    ds
		push    es
		push    gs
		sub     esp, 30h
		mov     ebp, esp
		cld
		mov     eax, 80000003h										// EXCEPTION_BREAKPOINT
		mov     ebx, [ebp+KTRAP_FRAME.Eip]							// exception address = eip
		mov		ecx,3												// param count = 3
		xor     edx, edx
		mov     edx, [ebp+KTRAP_FRAME.Eax]							// edx = eax = debug service type
		mov     edi, [ebp+KTRAP_FRAME.Ecx]							// edi = ecx = debug service param1
		mov     esi, [ebp+KTRAP_FRAME.Edx]							// esi = edx = debug service param2
		call    BdDispatch
		jmp     BdExit
	}
}

//
// save processor context
//
VOID __declspec(naked) KiSaveProcessorControlState(__in PKPROCESSOR_STATE Context)
{
	__asm
	{
		mov     edx, [esp+4]
		xor     ecx, ecx
		mov     eax, cr0
		mov     [edx+KPROCESSOR_STATE.SpecialRegisters.Cr0], eax
		mov     eax, cr2
		mov     [edx+KPROCESSOR_STATE.SpecialRegisters.Cr2], eax
		mov     eax, cr3
		mov     [edx+KPROCESSOR_STATE.SpecialRegisters.Cr3], eax
		mov     [edx+KPROCESSOR_STATE.SpecialRegisters.Cr4], ecx
		mov     eax, dr0
		mov     [edx+KPROCESSOR_STATE.SpecialRegisters.KernelDr0], eax
		mov     eax, dr1
		mov     [edx+KPROCESSOR_STATE.SpecialRegisters.KernelDr1], eax
		mov     eax, dr2
		mov     [edx+KPROCESSOR_STATE.SpecialRegisters.KernelDr2], eax
		mov     eax, dr3
		mov     [edx+KPROCESSOR_STATE.SpecialRegisters.KernelDr3], eax
		mov     eax, dr6
		mov     [edx+KPROCESSOR_STATE.SpecialRegisters.KernelDr6], eax
		mov     eax, dr7
		mov     dr7, ecx
		mov     [edx+KPROCESSOR_STATE.SpecialRegisters.KernelDr7], eax
		sgdt    fword ptr [edx+KPROCESSOR_STATE.SpecialRegisters.Gdtr.Limit]
		sidt    fword ptr [edx+KPROCESSOR_STATE.SpecialRegisters.Idtr.Limit]
		str     word ptr [edx+KPROCESSOR_STATE.SpecialRegisters.Tr]
		sldt    word ptr [edx+KPROCESSOR_STATE.SpecialRegisters.Ldtr]
		retn    4
	}
}

//
// save trapframe
//
VOID BdSaveKframe(__in PKTRAP_FRAME TrapFrame,__in PCONTEXT Context)
{
	Context->Ebp										= TrapFrame->Ebp;
	Context->Eip										= TrapFrame->Eip;
	Context->SegCs										= TrapFrame->SegCs;
	Context->EFlags										= TrapFrame->EFlags;
	Context->Esp										= TrapFrame->TempEsp;
	Context->SegSs										= TrapFrame->TempSegCs;
	Context->SegDs										= TrapFrame->SegDs;
	Context->SegEs										= TrapFrame->SegEs;
	Context->SegFs										= TrapFrame->SegFs;
	Context->SegGs										= TrapFrame->SegGs;
	Context->Eax										= TrapFrame->Eax;
	Context->Ebx										= TrapFrame->Ebx;
	Context->Ecx										= TrapFrame->Ecx;
	Context->Edx										= TrapFrame->Edx;
	Context->Edi										= TrapFrame->Edi;
	Context->Esi										= TrapFrame->Esi;
	Context->Dr0										= TrapFrame->Dr0;
	Context->Dr1										= TrapFrame->Dr1;
	Context->Dr2										= TrapFrame->Dr2;
	Context->Dr3										= TrapFrame->Dr3;
	Context->Dr6										= TrapFrame->Dr6;
	Context->Dr7										= TrapFrame->Dr7;

	KiSaveProcessorControlState(&BdPrcb.ProcessorState);
}

//
// restore processor context
//
VOID __declspec(naked) KiRestoreProcessorControlState(__in PKPROCESSOR_STATE State)
{
	__asm
	{
		mov     edx, [esp+4]
		mov     eax, [edx+KPROCESSOR_STATE.SpecialRegisters.Cr0]
		mov     cr0, eax
		mov     eax, [edx+KPROCESSOR_STATE.SpecialRegisters.Cr2]
		mov     cr2, eax
		mov     eax, [edx+KPROCESSOR_STATE.SpecialRegisters.Cr3]
		mov     cr3, eax
		mov     eax, [edx+KPROCESSOR_STATE.SpecialRegisters.KernelDr0]
		mov     dr0, eax
		mov     eax, [edx+KPROCESSOR_STATE.SpecialRegisters.KernelDr1]
		mov     dr1, eax
		mov     eax, [edx+KPROCESSOR_STATE.SpecialRegisters.KernelDr2]
		mov     dr2, eax
		mov     eax, [edx+KPROCESSOR_STATE.SpecialRegisters.KernelDr3]
		mov     dr3, eax
		mov     eax, [edx+KPROCESSOR_STATE.SpecialRegisters.KernelDr6]
		mov     dr6, eax
		mov     eax, [edx+KPROCESSOR_STATE.SpecialRegisters.KernelDr7]
		mov     dr7, eax
		lgdt    fword ptr [edx+KPROCESSOR_STATE.SpecialRegisters.Gdtr.Limit]
		lidt    fword ptr [edx+KPROCESSOR_STATE.SpecialRegisters.Idtr.Limit]
		lldt     word ptr [edx+KPROCESSOR_STATE.SpecialRegisters.Ldtr]
		retn    4
	}
}

//
// restore trap frame
//
VOID BdRestoreKframe(__in PKTRAP_FRAME TrapFrame,__in PCONTEXT Context)
{
	TrapFrame->Ebp										= Context->Ebp;
	TrapFrame->Eip										= Context->Eip;
	TrapFrame->SegCs									= Context->SegCs;
	TrapFrame->EFlags									= Context->EFlags;
	TrapFrame->SegDs									= Context->SegDs;
	TrapFrame->SegEs									= Context->SegEs;
	TrapFrame->SegFs									= Context->SegFs;
	TrapFrame->SegGs									= Context->SegGs;
	TrapFrame->Edi										= Context->Edi;
	TrapFrame->Esi										= Context->Esi;
	TrapFrame->Eax										= Context->Eax;
	TrapFrame->Ebx										= Context->Ebx;
	TrapFrame->Ecx										= Context->Ecx;
	TrapFrame->Edx										= Context->Edx;

	KiRestoreProcessorControlState(&BdPrcb.ProcessorState);
}

#pragma optimize("",off)
//
// default debug routine
//
BOOLEAN BdStub(__in PEXCEPTION_RECORD ExceptionRecord,__in CCHAR PreviousMode,__in PKTRAP_FRAME TrapFrame)
{
	if(ExceptionRecord->ExceptionCode != STATUS_BREAKPOINT || ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_BREAK)
		return FALSE;

	//
	// if this is a load/unload symbols and debug print,we skip the next int3 instruction
	//
	// DebugService looks like this
	//
	//	mov eax,ServiceType
	//	int 2d
	//	int 3
	//
	ULONG DebugServiceType								= ExceptionRecord->ExceptionInformation[0];
	if(DebugServiceType != BREAKPOINT_LOAD_SYMBOLS && DebugServiceType != BREAKPOINT_UNLOAD_SYMBOLS && DebugServiceType != BREAKPOINT_PRINT)
		return FALSE;

	TrapFrame->Eip										+= 1;
	return TRUE;
}

//
// debug routine used when debugger is enabled
//
BOOLEAN BdTrap(__in PEXCEPTION_RECORD ExceptionRecord,__in CCHAR PreviousMode,__in PKTRAP_FRAME TrapFrame)
{
	BdPrcb.ProcessorState.ContextFrame.ContextFlags		= CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;

	if(ExceptionRecord->ExceptionCode != STATUS_BREAKPOINT || ExceptionRecord->ExceptionInformation[0] == BREAKPOINT_BREAK)
	{
		//
		// save trap frame
		//
		BdSaveKframe(TrapFrame,&BdPrcb.ProcessorState.ContextFrame);

		//
		// debugger loop
		//
		BdReportExceptionStateChange(ExceptionRecord,&BdPrcb.ProcessorState.ContextFrame);

		//
		// restore trap frame
		//
		BdRestoreKframe(TrapFrame,&BdPrcb.ProcessorState.ContextFrame);

		//
		// reset control+c flags
		//
		extern BOOLEAN BdControlCPressed;
		BdControlCPressed								= FALSE;

		return TRUE;
	}

	ULONG DebugServiceType								= ExceptionRecord->ExceptionInformation[0];
	switch(DebugServiceType)
	{
	case BREAKPOINT_PRINT:
		{
			STRING String;
			String.Length								= static_cast<USHORT>(ExceptionRecord->ExceptionInformation[2]);
			String.Buffer								= Add2Ptr(ExceptionRecord->ExceptionInformation[1],0,PCHAR);
			if(BdDebuggerNotPresent)
				TrapFrame->Eax							= STATUS_DEVICE_NOT_CONNECTED;
			else
				TrapFrame->Eax							= BdPrintString(&String) ? STATUS_BREAKPOINT : STATUS_SUCCESS;

			TrapFrame->Eip								+= 1;
		}
		break;

	case BREAKPOINT_PROMPT:
		{
			STRING InputString;
			InputString.Length							= static_cast<USHORT>(ExceptionRecord->ExceptionInformation[2]);
			InputString.Buffer							= Add2Ptr(ExceptionRecord->ExceptionInformation[1],0,PCHAR);

			STRING OutputString;
			OutputString.MaximumLength					= static_cast<USHORT>(TrapFrame->Edi);
			OutputString.Buffer							= Add2Ptr(TrapFrame->Ebx,0,PCHAR);

			while(BdPromptString(&InputString,&OutputString));

			TrapFrame->Eax								= OutputString.Length;

			TrapFrame->Eip								+= 1;
		}
		break;

	case BREAKPOINT_LOAD_SYMBOLS:
	case BREAKPOINT_UNLOAD_SYMBOLS:
		{
			//
			// save trap frame
			//
			BdSaveKframe(TrapFrame,&BdPrcb.ProcessorState.ContextFrame);

			ULONG SavedContextEip						= BdPrcb.ProcessorState.ContextFrame.Eip;

			if(!BdDebuggerNotPresent)
			{
				PSTRING Name							= Add2Ptr(ExceptionRecord->ExceptionInformation[1],0,PSTRING);
				PKD_SYMBOLS_INFO SymInfo				= Add2Ptr(ExceptionRecord->ExceptionInformation[2],0,PKD_SYMBOLS_INFO);
				BOOLEAN Unload							= DebugServiceType == BREAKPOINT_UNLOAD_SYMBOLS;
				BdReportLoadSymbolsStateChange(Name,SymInfo,Unload,&BdPrcb.ProcessorState.ContextFrame);
			}

			if(SavedContextEip == BdPrcb.ProcessorState.ContextFrame.Eip)
				BdPrcb.ProcessorState.ContextFrame.Eip	+= 1;

			//
			// restore trap frame
			//
			BdRestoreKframe(TrapFrame,&BdPrcb.ProcessorState.ContextFrame);
		}
		break;

	default:
		return FALSE;
		break;
	}

	return TRUE;
}

#pragma optimize("",off)

//
// init debugger
//
VOID BdInitDebugger(__in PCHAR ImageName,__in ULONG BaseAddress,__in PCHAR LoadOptions)
{
	//
	// debugger already enabled
	//
	if(BdDebuggerEnabled)
		return;

	//
	// default debug routine
	//
	BdDebugRoutine										= &BdStub;

	//
	// option is null
	//
	if(!LoadOptions)
		return;

	//
	// convert it to uppercase
	//
	_strupr(LoadOptions);

	//
	// check NODEBUG
	//
	if(strstr(LoadOptions,"NODEBUG"))
		return;

	extern BOOLEAN BdPortInitialize(__in ULONG Baudrate,__in ULONG PortNumber);
	extern BOOLEAN Bd1394Initialize(__in ULONG Channel,__in_opt UCHAR HardwareType,__in_opt UCHAR NoBuses);

	//
	// check debug port
	//
	PCHAR DebugPortString								= strstr(LoadOptions,"DEBUGPORT");
	if(DebugPortString)
	{
		//
		// skip =
		//
		DebugPortString									+= 10;

		//
		// check debug port type
		//
		if(!strncmp(DebugPortString,"COM",3))
		{
			//
			// get port number
			//
			ULONG PortNumber							= atol(DebugPortString + 3);
			if(PortNumber)
			{
				//
				// get baudrate
				//
				PCHAR BaudrateString					= strstr(LoadOptions,"BAUDRATE");
				ULONG Baudrate							= 115200;
				if(BaudrateString)
					Baudrate							= atol(BaudrateString + 9);

				BdDebuggerEnabled						= BdPortInitialize(Baudrate,PortNumber);
			}
		}
		else if(!strncmp(DebugPortString,"1394",4))
		{
			//
			// over 1394
			//
			ULONG Channel								= 0;
			PCHAR ChannelString							= strstr(LoadOptions,"CHANNEL");
			if(ChannelString)
				Channel									= atol(ChannelString + 8);

			PCHAR HardwareTypeString					= strstr(LoadOptions,"HARDWARETYPE");
			UCHAR HardwareType							= HardwareTypeString ? static_cast<UCHAR>(atol(HardwareTypeString + 13)) : 1;

			PCHAR NoBusesString							= strstr(LoadOptions,"NOBUSES");
			UCHAR NoBuses								= NoBusesString ? static_cast<UCHAR>(atol(NoBusesString	+ 8)) : 0x10;
			BdDebuggerEnabled							= Bd1394Initialize(Channel,HardwareType,NoBuses);
		}
	}
	else
	{
		//
		// check debug
		//
		if(strstr(LoadOptions,"DEBUG"))
		{
			//
			// use default com port
			//
			BdDebuggerEnabled							= BdPortInitialize(115200,1);
		}
	}

	if(!BdDebuggerEnabled)
		return;

	extern UCHAR BdBreakpointInstruction;
	BdBreakpointInstruction								= 0xcc;
	BdDebugRoutine										= &BdTrap;

	//
	// initialize breakpoint table
	//
	extern BREAKPOINT_ENTRY BdBreakpointTable[BREAKPOINT_TABLE_SIZE];
	for(ULONG i = 0; i < BREAKPOINT_TABLE_SIZE; i ++)
	{
		BdBreakpointTable[i].Flags						= 0;
		BdBreakpointTable[i].Address					= 0;
	}

	PIMAGE_NT_HEADERS NtHeaders							= RtlImageNtHeader(reinterpret_cast<PVOID>(BaseAddress));
	if(!NtHeaders)
		BlPrint("Unable to find NtImageHeaders!\r\nSome information will not be available to debugger.\r\n");

	//
	// setup a dummy module list
	//
	extern LIST_ENTRY BdModuleList;
	extern LDR_DATA_TABLE_ENTRY	BdModuleDataTableEntry;

	//
	// link them together
	//
	BdModuleList.Flink									= &BdModuleDataTableEntry.InLoadOrderLinks;
	BdModuleList.Blink									= &BdModuleDataTableEntry.InLoadOrderLinks;
	BdModuleDataTableEntry.InLoadOrderLinks.Flink		= &BdModuleList;
	BdModuleDataTableEntry.InLoadOrderLinks.Blink		= &BdModuleList;

	//
	// setup data table entry
	// BUG BUG BUG use caller provided file name instead?
	//
	BdModuleDataTableEntry.Flags						= 0;
	BdModuleDataTableEntry.LoadCount					= 1;
	BdModuleDataTableEntry.DllBase						= reinterpret_cast<PVOID>(BaseAddress);
	BdModuleDataTableEntry.SizeOfImage					= NtHeaders ? NtHeaders->OptionalHeader.SizeOfImage : 0x100000;
	BdModuleDataTableEntry.EntryPoint					= NtHeaders ? Add2Ptr(BdModuleDataTableEntry.DllBase,NtHeaders->OptionalHeader.AddressOfEntryPoint,PVOID) : 0;
	BdModuleDataTableEntry.CheckSum						= NtHeaders ? NtHeaders->OptionalHeader.CheckSum : 0;
	BdModuleDataTableEntry.BaseDllName.Buffer			= L"osloader.exe";
	BdModuleDataTableEntry.BaseDllName.MaximumLength	= sizeof(L"osloader.exe");
	BdModuleDataTableEntry.BaseDllName.Length			= BdModuleDataTableEntry.BaseDllName.MaximumLength - sizeof(WCHAR);
	BdModuleDataTableEntry.FullDllName					= BdModuleDataTableEntry.BaseDllName;

	for(ULONG i = 0; i < 10; i ++)
	{
		BdDebuggerNotPresent							= FALSE;
		DbgPrint("<?dml?><col fg=\"changed\">BD: Boot Debugger Initialized</col>\n");
		DbgPrint("<?dml?><col fg=\"changed\">BD: %p %s</col>\n",BaseAddress,ImageName);

		if(!BdDebuggerNotPresent)
			break;
	}

	//
	// let debugger load symbols
	//
	STRING Name;
	Name.Length											= static_cast<USHORT>(strlen(ImageName));
	Name.Buffer											= ImageName;
	Name.MaximumLength									= Name.Length + sizeof(CHAR);
	DbgLoadImageSymbols(&Name,reinterpret_cast<PVOID>(BaseAddress),-1);

	//
	// boot break?
	//
	if(strstr(LoadOptions,"DEBUGSTOP"))
		DbgBreakPoint();
	else
		BdPollConnection();
}

//
// stop debugger
//
VOID BdStopDebugger()
{
	if(!BdDebuggerEnabled)
		return;

	//
	// suspend all breakpoints
	//
	BdSuspendAllBreakpoints();

	//
	// special unload symbols packet to tell debugger we are stopping
	//
	DbgUnLoadImageSymbols(0,reinterpret_cast<PVOID>(0xffffffff),0);

	BdDebuggerEnabled									= FALSE;
	BdDebuggerNotPresent								= TRUE;
	BdDebugRoutine										= &BdStub;
}