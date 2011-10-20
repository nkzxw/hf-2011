/*++

	This is the part of NGdbg kernel debugger

	command.cpp

	This file contains routines to process commands typed by user.

--*/

#include <ntifs.h>
#include "dbgeng.h"
#include <stdlib.h>
#include <stdio.h>
#include "gui.h"
typedef int BOOL;
extern "C"
{
#include "disasm.h"
}
#include "symbols.h"

//
// Declarations
//

int explode (char *input, char* symset, char** output, int maxarray);

UCHAR hexchr (char hx);
ULONG hextol (char *hex);
BOOLEAN isstrhex (char *str);

extern BOOLEAN ExceptionShouldBeDispatched;
extern BOOLEAN StopProcessingCommands;

extern "C" extern PVOID pNtBase;
//extern "C" extern PVOID pNtSymbols;

struct _KPRCB
{
	USHORT MinorVersion;
	USHORT MajorVersion;
	PKTHREAD CurrentThread;
};

BOOLEAN DisplayBuffer();

PVOID LastUnassemble;
PVOID LastDump;


PVOID
DisasmAtAddress (
	PVOID Address, 
	ULONG nCommands
	)

/*++
	
Routine Description

	This routine disassembles code at the specified address 
	 and prints listing to the main debugger output

 Arguments

	Address

		Address of code to be disassembled

	nCommands

		Number of commands to disassemble

Return Value

	Function returns virtual address where it has been stopped.

Environment

	This function can be called at any IRQL if code being disassembled
	 is locked in physical memory

--*/

{
	if (MmIsAddressValid(Address))
	{
		PUCHAR ptr = (PUCHAR) Address;
		TMnemonicOptions opts = {0};
		char buff[512];

		opts.RealtiveOffsets = FALSE;
		opts.AddHexDump = TRUE;
		opts.AlternativeAddres = 0;
		opts.AddAddresPart = FALSE;
		opts.MnemonicAlign = 23;
		
		for (ULONG line=0; line<nCommands; line++)
		{
			TDisCommand dis = {0};
			TInstruction instr = {0};

			char Symbol[32];
			ULONG symlen = sizeof(Symbol);
			ULONG disp = 0;

			if (SymGlobGetNearestSymbolByAddress (ptr, Symbol, &symlen, &disp) == 0)
			{
				if (disp)
					GuiPrintf("%s + 0x%x\n", Symbol, disp);
				else
					GuiPrintf("%s\n", Symbol);
			}

			ULONG len = InstrDecode (ptr, &instr, FALSE);
			if(!InstrDasm (&instr, &dis, FALSE))
			{
				GuiPrintf("%08X : invalid opcode\n", ptr);
			}
			else
			{
				MakeMnemonic (buff, &dis, &opts);
				GuiPrintf("%08X : %s\n", ptr, buff);
			}

			ptr += len;
		}

		return ptr;
	}
	else
	{
		GuiPrintf ("%08X : ???\n", Address);
		return NULL;
	}
}

BOOLEAN 
Sym (
	char *a, 
	PVOID *pSym
	)

/*++

Routine Description

	This routine lookups string to be used in commands like 'u' or 'dd'.
	It can be symbol, hexadecimal value or CPU register (not suppored yet).

Arguments

	a

		String, which value should be calculated.

	pSym

		Place where value should be stored.

Return Value

	TRUE if expression was evaluated successfully, FALSE otherwise

Environment

	This function can be called at any IRQL.
	Debugger environment should contain valid trap frame if specified string
	 is CPU register.
	Symbol tables should be locked in physical memory if specified string
	 is a symbol name.

--*/

{
	NTSTATUS Status;

	Status = SymGlobGetSymbolByName (a, (ULONG*)pSym);
	if (NT_SUCCESS(Status))
		return TRUE;

	if (isstrhex(a))
	{
		*pSym = (PVOID) hextol (a);
		return TRUE;
	}

	return FALSE;
}

#include "i8042.h"

__declspec(noreturn)
VOID
RebootMachine(
	)

/*++

Routine Description

	This routine performs reset of the processor by signaling #RESET line

Arguments
	
	None

Return Value

	This function does not return

--*/

{
	do
	{
		I8xPutBytePolled (CommandPort, ControllerDevice, FALSE, (UCHAR) I8042_RESET);
	}
	while (TRUE);
}

extern PKTRAP_FRAME TrapFrame;

VOID WriteRegister (char *regname, ULONG Value)
{
	     if (!_stricmp (regname, "eax")) TrapFrame->Eax = Value;
	else if (!_stricmp (regname, "ecx")) TrapFrame->Ecx = Value;
	else if (!_stricmp (regname, "edx")) TrapFrame->Edx = Value;
	else if (!_stricmp (regname, "ebx")) TrapFrame->Ebx = Value;
	else if (!_stricmp (regname, "esi")) TrapFrame->Esi = Value;
	else if (!_stricmp (regname, "edi")) TrapFrame->Edi = Value;
	else if (!_stricmp (regname, "ebp")) TrapFrame->Ebp = Value;
	else if (!_stricmp (regname, "eip")) TrapFrame->Eip = Value;
	else if (!_stricmp (regname, "efl")) TrapFrame->EFlags = Value;
	else if (!_stricmp (regname, "cs")) TrapFrame->SegCs = Value;
	else if (!_stricmp (regname, "ds")) TrapFrame->SegDs = Value;
	else if (!_stricmp (regname, "es")) TrapFrame->SegEs = Value;
	else if (!_stricmp (regname, "fs")) TrapFrame->SegDs = Value;
	else if (!_stricmp (regname, "gs")) TrapFrame->SegGs = Value;
	else
	{
		GuiPrintf("Invalid reg name %s\n", regname);
	}
}

//
// Stack frame for function call
//

typedef struct STACK_FRAME
{
	UCHAR* ReturnAddress;
	ULONG Arguments[1];
} *PSTACK_FRAME;

//
// FPO frame for function call
//

typedef struct BASE_FRAME
{
	BASE_FRAME *SaveEbp;

	union
	{
		struct
		{
			UCHAR* ReturnAddress;
			ULONG Arguments[1];
		};
		STACK_FRAME StackFrame;
	};
} *PBASE_FRAME;


BOOLEAN
MmIsSystemAddressAccessable(
	PVOID VirtualAddress
	);

PBASE_FRAME 
LookupBaseFrame (
	PVOID esp, 
	PVOID ebp,
	PSTACK_FRAME StackFrame
	)

/*++

Routine Description

	This routine searches ebp frame for the specified stack frame if it exists
	EBP frame is a pointer to BASE_FRAME which has the following structure:

	base frame:
		+0	Saved EBP
	(stack frame:)
		+4	Return Address
		+8	Arguments

	This routine receives pointer to stack frame and checks if
	 base frame (StackFrame-4) exists. If so, its address is returned.
	If not, function returns null.
		
Arguments

	esp, ebp
	
		esp/ebp values

	StackFrame

		stack frame to search ebp frame for

Return Value

	Pointer to ebp frame (StackFrame-4) or null

--*/

{
	for (PBASE_FRAME Frame = (PBASE_FRAME) ebp;
		 MmIsSystemAddressAccessable (Frame);
		 Frame = Frame->SaveEbp)
	{
		if (&Frame->StackFrame == StackFrame)
			return Frame;
	}
	return NULL;
}


VOID
StackBacktrace (
	PVOID esp, 
	PVOID ebp, 
	PVOID eip,
	PVOID StackBase
	)

/*++

Routine Description

	 This function performs stack backtrace

Arguments


	esp, ebp, eip

		register values for backtrace

Return Value

	None

--*/
	
{
	PBASE_FRAME Frame = (PBASE_FRAME) ebp;

	//__asm int 3;

	GuiPrintf("Stack backtrace ESP %x EBP %x EIP %x Base %x\n", esp, ebp, eip, StackBase);
	KdPrint(("Stack backtrace ESP %x EBP %x EIP %x Base %x\n", esp, ebp, eip, StackBase));
	GuiPrintf("Return frames:\n");
	GuiPrintf("EBP       ChildEBP  Return    Arguments\n");
	KdPrint(("EBP       ChildEBP  Return    Arguments\n"));

	PVOID pFunction = eip;

	for ( UCHAR** StackPointer = (UCHAR**)esp;
		  MmIsSystemAddressAccessable (StackPointer) && 
		    (ULONG) StackPointer < (ULONG) StackBase;
		  StackPointer++ )
	{
		PSTACK_FRAME Frame = (PSTACK_FRAME) StackPointer;

		UCHAR* Return = Frame->ReturnAddress;

		if ((ULONG)Return <  0x10000)
			continue;

		if (!MmIsSystemAddressAccessable (Return-5))
			continue;

//		KdPrint(("%08x: MmIsSystemAddressAccessable(%x) = %d\n",
//			StackPointer,
//			Return-5,
//			MmIsSystemAddressAccessable (Return-5)
//			));
			

		// Call?
		if (Return[-5] == 0xE8 ||
			Return[-2] == 0xFF ||
			Return[-3] == 0xFF ||
			(Return[-7] == 0xFF && Return[-6] == 0x15))
		{
//			if (!IsAddressInCodeSection(Return))
//				printf("Return address is not within any known module\n");

			PUCHAR AddressToCheck = NULL;

			if (Return[-2] == 0xFF || Return[-3] == 0xFF)
			{
				KdPrint(("0xFF opcode is not fully supported yet\n"));
				GuiPrintf("0xFF opcode is not fully supported yet\n");
			}
			else if (Return[-5] == 0xE8) // CALL XXX
			{
				AddressToCheck = Return + *(ULONG*)&Return[-4];
			}
			else if (Return[-7] == 0xFF && Return[-6] == 0x15) // CALL DWORD PTR DS:[XXX]
			{
				PUCHAR* IndirectCall = *(PUCHAR**)&Return[-4];
				if (!MmIsSystemAddressAccessable (IndirectCall))
					continue;

				AddressToCheck = *IndirectCall;
			}

			if (!MmIsSystemAddressAccessable (AddressToCheck))
				continue;

			char sym[128] = "";
			ULONG disp, len = sizeof(sym);
			NTSTATUS Status;

			sprintf (sym, "0x%x", pFunction);

			Status = SymGlobGetNearestSymbolByAddress (
				pFunction,
				sym,
				&len,
				&disp
				);
			if (NT_SUCCESS(Status))
			{
				if (disp)
					sprintf (sym+strlen(sym), "+0x%x", disp);
			}

			PBASE_FRAME BaseFrame = LookupBaseFrame (esp, ebp, Frame);
			
			if (BaseFrame)
			{
				Frame = &BaseFrame->StackFrame;
			}

			GuiPrintf("%08x: %08x  %08x  %08x %08x %08x %08x %s\n",
				Frame, 
				BaseFrame ? BaseFrame->SaveEbp : NULL,
				Return,
				Frame->Arguments[0],
				Frame->Arguments[1],
				Frame->Arguments[2],
				Frame->Arguments[3],
				sym
				);

			KdPrint(("%08x: %08x  %08x  %08x %08x %08x %08x %s\n",
				Frame, 
				BaseFrame ? BaseFrame->SaveEbp : NULL,
				Return,
				Frame->Arguments[0],
				Frame->Arguments[1],
				Frame->Arguments[2],
				Frame->Arguments[3],
				sym
				));

			pFunction = Frame->ReturnAddress;
		}
	}

	char sym[128] = "";
	ULONG disp;
	ULONG len = sizeof(sym);
	NTSTATUS Status;

	sprintf (sym, "0x%x", pFunction);

	Status = SymGlobGetNearestSymbolByAddress (
		pFunction,
		sym,
		&len,
		&disp
		);
	if (NT_SUCCESS(Status))
	{
		if (disp)
			sprintf (sym+strlen(sym), "+0x%x", disp);
	}

	GuiPrintf("%08x: %08x  %08x  %08x %08x %08x %08x 0x%08x\n",
		0, 
		0,
		0,
		0,
		0,
		0,
		0,
		pFunction
		);
}

VOID
ProcessCommand(
	CHAR* Command
	)

/*++

Routine Description

	Process user command.
	This function is called from WR_ENTER_DEBUGGER when
	 it receives the whole string from 8042 PS/2 minidriver

Arguments
	
	Command

		Command received from keyboard.

Return Value

	None

--*/
	
{
	char *output[20];
	int nItems = explode (Command, " \r\n", output, 20);

	char *cmd = output[0];

	_strlwr (cmd);

	if (!_stricmp (cmd, "i3hereuser"))
	{
		if (nItems < 2)
		{
			GuiTextOut ("this command requires an argument\n");
			return;
		}
		ULONG Value = atoi (output[1]);

		I3HereUser = !!(Value);
	}
	else if (!_stricmp (cmd, "i3herekernel"))
	{
		if (nItems < 2)
		{
			GuiTextOut ("this command requires an argument\n");
			return;
		}
		ULONG Value = atoi (output[1]);

		I3HereKernel = !!(Value);
	}
	else if (!_stricmp (cmd, "de"))
	{
		GuiTextOut ("Dispatching exception\n");
		ExceptionShouldBeDispatched = TRUE;
		StopProcessingCommands = TRUE;
	}
	else if (!_stricmp (cmd, "r"))
	{
		if (MmIsAddressValid(TrapFrame))
		{
			if (nItems == 3)
			{
				PVOID Address;

				if(!Sym(output[2], &Address))
				{
					GuiPrintf("Could not find symbol %s\n", output[2]);
				}
				else
				{
					WriteRegister (output[1], (ULONG) Address);
				}
			}
			else
			{
				GuiPrintf("%d args not supported for r\n", nItems-1);
			}
		}
		else
		{
			GuiPrintf("TrapFrame %X is not valid\n", TrapFrame);
		}
	}
	else if (!_stricmp (cmd, "trap"))
	{
		if (MmIsAddressValid(TrapFrame))
		{
			GuiPrintf(
				"Trap frame at %X\n"
				"DbgArgMark %08X     DbgEbp %08X        DbgEip %08X\n"
				"Dr0        %08X     Dr1    %08X        Dr2    %08X\n"
				"Dr3        %08X     Dr6    %08X        Dr7    %08X\n"
				"SegGs      %08X     SegEs  %08X        SegDs  %08X\n"
				"Edx        %08X     Ecx    %08X        Eax    %08X\n"
				"PrevMode   %08X    ExcList %08X        SegFs  %08X\n"
				"Edi        %08X     Esi    %08X        Ebx    %08X\n"
				"Ebp        %08X    ErrCode %08X        Eip    %08X\n"
				"SegCs           %08X       EFlags          %08X\n"
				"HardwareEsp     %08X       HardwareSegSs   %08X\n"
				,
				TrapFrame, TrapFrame->DbgArgMark, TrapFrame->DbgEbp,
				TrapFrame->DbgEip, TrapFrame->Dr0, TrapFrame->Dr1,
				TrapFrame->Dr2, TrapFrame->Dr3, TrapFrame->Dr6, TrapFrame->Dr7,
				TrapFrame->SegGs, TrapFrame->SegEs, TrapFrame->SegDs,
				TrapFrame->Edx, TrapFrame->Ecx, TrapFrame->Eax,
				TrapFrame->PreviousPreviousMode, TrapFrame->ExceptionList, TrapFrame->SegFs,
				TrapFrame->Edi, TrapFrame->Esi, TrapFrame->Ebx,
				TrapFrame->Ebp, TrapFrame->ErrCode, TrapFrame->Eip,
				TrapFrame->SegCs, TrapFrame->EFlags,
				TrapFrame->HardwareEsp, TrapFrame->HardwareSegSs
			);
			
		}
		else
		{
			GuiPrintf("TrapFrame %X is not valid\n", TrapFrame);
		}
	}
	else if (!_stricmp (cmd, "bugcheck"))
	{
		ULONG Code = 0xE2, Par1=0, Par2=0, Par3=0, Par4=0;

		if (nItems > 1)
			Code = hextol (output[1]);
		
		if (nItems > 2)
			Par1 = hextol (output[2]);

		if (nItems > 3)
			Par2 = hextol (output[3]);

		if (nItems > 4)
			Par3 = hextol (output[4]);

		if (nItems > 5)
			Par4 = hextol (output[5]);

		KeBugCheckEx (Code, Par1, Par2, Par3, Par4);
	}
	else if (!_stricmp (cmd, "g"))
	{
		StopProcessingCommands = TRUE;
	}
	else if (!_stricmp (cmd, "kb"))
	{
		KdPrint(("TrapFrame = %x, EBP %x EIP %x CS %x\n",
			TrapFrame,
			TrapFrame->Ebp,
			TrapFrame->Eip,
			TrapFrame->SegCs
			));

		ULONG ebp = TrapFrame->Ebp;
		ULONG esp = 0;
		ULONG eip = TrapFrame->Eip;
		PKPCR Pcr = (PKPCR) KIP0PCRADDRESS;
		ULONG StackBase = (ULONG) Pcr->NtTib.StackBase;

		KdPrint(("segcs = %x\n", TrapFrame->SegCs));

		if ((TrapFrame->SegCs & 3) == 0)
		{
			// Call from kernel-mode
			esp = (ULONG) &TrapFrame->HardwareEsp;
			KdPrint(("Kernel-mode esp used\n"));
		}
		else
		{
			// Call from user-mode
			esp = TrapFrame->HardwareEsp;
			KdPrint(("User-mode esp used\n"));
		}

		KdPrint(("esp %x ebp %x eip %x stackbase %x\n",
			esp,
			ebp,
			eip,
			StackBase
			));

		StackBacktrace (
			(PVOID) esp,
			(PVOID) ebp,
			(PVOID) eip,
			(PVOID) StackBase
			);
	}
	else if (!_stricmp (cmd, "?"))
	{
		PVOID Address;

		if (nItems < 2)
		{
			GuiTextOut("This command requires an argument\n");
		}
		else
		{
			if (!Sym(output[1], &Address))
			{
				GuiPrintf("Could not find symbol %s\n", output[1]);
			}
			else
			{
				GuiPrintf("%s = %x\n", output[1], Address);
			}
		}
	}
	else if (!_stricmp (cmd, "dd"))
	{
		PVOID Address;

		if (nItems < 2)
		{
			Address = LastDump;
		}
		else
		{
			if (!Sym(output[1], &Address))
			{
				GuiPrintf("could not find symbol %s\n", output[1]);
				return;
			}
		}
		
		GuiPrintf ("dumping memory at %08X\n", Address);

		if (MmIsAddressValid(Address))
		{
			PULONG ptr = (PULONG) Address;

			for (ULONG line=0; line<5; line++)
			{
				char Symbol[32];
				ULONG symlen = sizeof(Symbol);
				ULONG disp = 0;

				if (SymGlobGetNearestSymbolByAddress (ptr, Symbol, &symlen, &disp) == 0)
				{
					if (disp)
						GuiPrintf("%s + 0x%x\n", Symbol, disp);
					else
						GuiPrintf("%s\n", Symbol);
				}

				GuiPrintf("%08X : %08X %08X %08X %08X\n",
					ptr, ptr[0], ptr[1], ptr[2], ptr[3]);

				ptr += 4;
			}
		
			LastDump = (PVOID)ptr;
		}
		else
		{
			GuiPrintf ("%08X : ???\n", Address);
		}
		GuiTextOut ("End of dump\n");
	}
	else if (!_stricmp(cmd, "u"))
	{
		PVOID Address;

		if (nItems < 2)
		{
			Address = LastUnassemble;
		}
		else
		{
			if (!Sym(output[1], &Address))
			{
				GuiPrintf("could not find symbol %s\n", output[1]);
			}
		}
		GuiPrintf ("disassembly dump at %08X\n", Address);

		PVOID p = DisasmAtAddress (Address, 10);
		if (p)
			LastUnassemble = p;

		GuiTextOut ("End of dump\n");
	}
	else if (!_stricmp(cmd, "reboot"))
	{
		RebootMachine ();
	}
	else if (!_stricmp(cmd, "prcb"))
	{
		PKPCR Pcr = (PKPCR) KIP0PCRADDRESS;

		GuiPrintf(
			"Processor control region %08X:\n"
			" NT_TIB\n"
			"  ExceptionList   %08X\n"
			"  StackBase       %08X\n"
			"  StackLimit      %08X\n"
			"  SumSystemTib    %08X\n"
			"  FiberData/Ver   %08X\n"
			"  ArbUserPointer  %08X\n"
			"  Teb             %08X\n"
			" SelfPcr         %08X\n"
			" Prcb            %08X\n"
			" Irql            %02X\n"
			" IRR             %08X\n"
			" IrrActive       %08X\n"
			" IDR             %08X\n"
			" KdVersionBlock  %08X\n"
			" IDT             %08X\n"
			" GDT             %08X\n"
			" TSS             %08X\n"
			" MajorVersion    %04X\n"
			" MinorVersion    %04X\n"
			" Affinity        %08X\n"
			" DebugActive     %02X\n"
			" Number          %02X\n"
			"Processor control block at %08X:\n"
			"  CurrentThread    %08X\n"
			,
			Pcr, Pcr->NtTib.ExceptionList, Pcr->NtTib.StackBase,
			Pcr->NtTib.StackLimit, Pcr->NtTib.SubSystemTib,
			Pcr->NtTib.FiberData, Pcr->NtTib.ArbitraryUserPointer,
			Pcr->NtTib.Self, Pcr->SelfPcr, Pcr->Prcb, Pcr->Irql,
			Pcr->IRR, Pcr->IrrActive, Pcr->IDR, Pcr->KdVersionBlock,
			Pcr->IDT, Pcr->GDT, Pcr->TSS, Pcr->MajorVersion,
			Pcr->MinorVersion, Pcr->SetMember, Pcr->DebugActive,
			Pcr->Number, Pcr->Prcb, Pcr->Prcb->CurrentThread
			);
	}
	else if (!_stricmp(cmd, "help"))
	{
		GuiTextOut (
			"NGdbg debugger command help\n"
			"Available commands:\n"
			" u [ADDRESS]     display disassemble dump at the specified address\n"
			" dd [ADDRESS]    display raw ULONG dump at the specified address\n"
			"< db ADDRESS      display raw UCHAR dump at the specified address>\n"
			" prcb            display KPRCB dump\n"
			" g               go (if within exception, does not handle it)\n"
			" de              dispatch the exception\n"
			" i3hereuser B    sets action for usermode INT3's (B = 0 or 1)\n"
			" i3herekernel B  sets action for kernelmode INT3's (B = 0 or 1)\n"
			" r reg value     set register value\n"
			" ? exp           evaluate expression (only symbols supported)\n"
			" trap            show caller's trap frame\n"
			" bugcheck c 1234 crash system with code c and params 1,2,3,4\n"
			" reboot          reboot machine\n"
			);
	}
	else
	{
		GuiPrintf ("Unknown command: %s\n", cmd);
	}
}
