/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#include "NRER.h"

////
//
// Summary
// -------
//
// This DLL is responsible for implementing the randomization of non-relocatable
// executable image files.  It does this by mirroring the old executable region
// against the randomized region by hooking the exception dispatcher entry point
// that is called whenever an exception is thrown inside the process.  This
// allows the DLL to transparently translate references to the old region to the
// new and to detect any malicious attempts to reference the old region (such as
// by an exploitation attempt).
//
// It also provides the user-mode interface to detecting and preventing various
// types of attack vectors, such as SEH overwrites, stack overflows, and format
// string exploits.
//
////

////
//
// Defines and constants
//
////
#ifndef STATUS_ACCESS_VIOLATION
#define STATUS_ACCESS_VIOLATION 0xc0000005
#endif

#ifndef ULONG_PTR
#define ULONG_PTR ULONG
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

typedef enum _ACCESS_VIOLATION_TYPE
{
	AccessViolationRead, 
	AccessViolationWrite
} ACCESS_VIOLATION_TYPE, *PACCESS_VIOLATION_TYPE;

////
//
// Prototypes
//
////

//
// Macros for fixing up address references in the original executable
//
#define IsAddressInsideOriginalExecutableRegion(Address)   \
	(((ULONG_PTR)(Address) >= OrigExecutableImageBase) &&   \
    ((ULONG_PTR)(Address) < OrigExecutableImageBase + OrigExecutableImageSize))
#define FixupOriginalExecutableAddress(Address)            \
	NewExecutableImageBase + ((ULONG_PTR)(Address) - OrigExecutableImageBase)

//
// Core function prototypes
//
VOID KiUserExceptionDispatcherHook(
		IN PEXCEPTION_RECORD ExceptionRecord,
		IN PCONTEXT Context);

static BOOLEAN InspectException(
		IN PEXCEPTION_RECORD ExceptionRecord,
		IN PCONTEXT Context);
static BOOLEAN GetExceptionInstruction(
		IN ULONG_PTR InstructionPointer,
		OUT PINSTRUCTION Instruction);
static BOOLEAN CheckExploitationAttempt(
		IN PCONTEXT Context,
		IN PINSTRUCTION Instruction OPTIONAL,
		IN ULONG_PTR FaultAddress,
		IN ACCESS_VIOLATION_TYPE AvType,
		OUT PSELECTED_EXIT_METHOD ExitMethod OPTIONAL);

static BOOLEAN MirrorExecutableAddressReference(
		IN PCONTEXT Context,
		IN ULONG_PTR InstructionPointer,
		IN ULONG_PTR FaultAddress,
		IN ACCESS_VIOLATION_TYPE AvType,
		IN PINSTRUCTION Instruction);
static VOID FixupContextRegister(
		IN PCONTEXT Context,
		IN ULONG RegisterIndex);
static VOID FixupStaticAddressReference(
		IN PULONG StaticAddressReference);

static VOID CreateProcessParametersMirror();
static VOID CopyProcessParameters();
static BOOLEAN FixupProcessParametersReference(
		IN ULONG_PTR InstructionPointer,
		IN ULONG_PTR FaultAddress);

////
// 
// Exported Global Variables
//
////

//
// These globals contain information about the new and original address regions
// that the executable can be found at.  These are populated by the driver.
//
ULONG_PTR NewExecutableImageBase  = 0;
ULONG_PTR OrigExecutableImageBase = 0;
ULONG     OrigExecutableImageSize = 0;

//
// This global variable contains the execution flags that are used to drive how
// the user-mode code functions.  These flags contain what exploitation
// prevention mechanisms should be enforced and so on.
//
ULONG     NreExecutionFlags       = 0;

//
// Function pointers to the original KiUserExceptionDispatcher and
// LdrInitializeThunk routines.
//
static ULONG (NTAPI *OrigKiUserExceptionDispatcher)(
		IN PEXCEPTION_RECORD ExceptionRecord,
		IN PCONTEXT Context);
static ULONG (NTAPI *OrigLdrInitializeThunk)(
		IN PVOID Unused1,
		IN PVOID Unused2,
		IN PVOID Unused3);
static ULONG (NTAPI *OrigLdrLoadDll)(
		IN PWCHAR PathToFile OPTIONAL,
		IN ULONG Flags OPTIONAL,
		IN PUNICODE_STRING ModuleFileName,
		OUT PHANDLE ModuleHandle);

//
// This flag indicates whether or not the mirror for the process parameters
// structure has been created.
//
static BOOLEAN ProcessParametersMirrorCreated = FALSE;

////
//
// Code
//
////

//
// Hooked entry point for LDR initialization.
//
static ULONG __declspec(naked) LdrInitializeThunkHook(
		IN PVOID Unused1,
		IN PVOID Unused2,
		IN PVOID Unused3)
{
	__asm
	{
		push [esp+0xc]
		push [esp+0xc]
		push [esp+0xc]
		mov  eax, [OrigLdrInitializeThunk]
		call eax
		push eax
		call NreHookEnforcementRoutines
		pop  eax
		retn 0xc
	}
}

//
// Hooked entry poitn for LdrLoadDll which is used to detect when certain DLLs
// are loaded, such as msvcrt.
//
static ULONG NTAPI LdrLoadDllHook(
		IN PWCHAR PathToFile OPTIONAL,
		IN ULONG Flags OPTIONAL,
		IN PUNICODE_STRING ModuleFileName,
		OUT PHANDLE ModuleHandle)
{
	ULONG Status = OrigLdrLoadDll(
			PathToFile,
			Flags,
			ModuleFileName,
			ModuleHandle);

	//
	// If the library is loaded successfully and has a valid module handle, check
	// to see if it's one of the modules that we care about.
	//
	if ((Status == ERROR_SUCCESS) &&
	    (*ModuleHandle))
		NreHookEnforcementRoutines();

	return Status;
}

//
// Hooked entry point for exception dispatching.
//
static VOID __declspec(naked) KiUserExceptionDispatcherHook(
		IN PEXCEPTION_RECORD ExceptionRecord,
		IN PCONTEXT Context)
{
	__asm
	{
		call InspectException                        // Inspect the exception that was passed to this routine
		test al, al                                  // Test if AL is zero
		jz   continue_execution                      // If AL==0, a fixup occurred and the thread should continue
		mov  eax, [OrigKiUserExceptionDispatcher]    // Get the original KiUserExceptionDispatcher pointer
		jmp  eax                                     // Jump into the orig exception dispatcher
continue_execution:
		push 0x0                                     // Set RemoveAlert to FALSE
		push dword ptr [esp + 8]                     // Push the CONTEXT pointer
		call NtContinue                              // Call NtContinue
		retn 0x8
	}
}

BYTE __declspec(naked) LdrInitialized()
{
	__asm
	{
		mov eax, fs:[0x30]
		cmp [eax + 0xc], 0x0
		setnz al
		ret
	}
}

//
// This exported routine is called when NRER is first mapped into memory after
// the first thread has been created.  The purpose of this routine hook all the
// required routines (such as KiUserExceptionDispatcher).
//
VOID WINAPI NreInitialize()
{
	//
	// If the Ldr has not been initialized, then it's not safe for us to do
	// anything.  This had to be added because of problems on Windows 2000.
	//
	if (!LdrInitialized())
		return;

	//
	// If we're enforcing format string prevention (and most likely others to
	// come) that would require hooking other functions, we do that here.
	//
	if (IsNreExecutionFlag(NRE_FLAG_ENFORCE_FMT))
	{
		//
		// Check to see if LdrInitializeThunk has already been called.  If it has,
		// initialize our extra hook routines.
		//
		if (NreGetProcessHeap())
			NreHookEnforcementRoutines();
		//
		// Otherwise, we hook LdrInitializeThunk so that we can hook our routines
		// after the Ldr has been initialized.  We also hook LdrLoadDll so that we
		// can detect is MSVCRT.DLL is loaded into memory.
		//
		else
		{
			NreHookRoutine(
					(PVOID)DispatchTable.LdrInitializeThunk,
					(PVOID)LdrInitializeThunkHook,
					FALSE,
					(PVOID *)&OrigLdrInitializeThunk);
			
			NreHookRoutine(
					(PVOID)DispatchTable.LdrLoadDll,
					(PVOID)LdrLoadDllHook,
					FALSE,
					(PVOID *)&OrigLdrLoadDll);
		}
	}

	//
	// For applications that may make reference to the
	// RTL_USER_PROCESS_PARAMETERS structure at 0x20000, we create a NOACCESS
	// page that will trigger an AV if it is referenced for reading or writing.
	// This has been seen in applications like Excel, and also in kernel-mode by
	// ATI drivers.
	//
	CreateProcessParametersMirror();

	//
	// We always hook the exception dispatcher in order to process process
	// parameter mirroring logic.  Prior to this, we only hooked the exception
	// dispatcher if SEH/stack detection was enabled for this process.  Now, that
	// check is done within the exception dispatcher hook.
	//
	NreHookRoutine(
			(PVOID)DispatchTable.KiUserExceptionDispatcher,
			(PVOID)KiUserExceptionDispatcherHook,
			FALSE,
			(PVOID *)&OrigKiUserExceptionDispatcher);
}

//
// This exported routine is called through an APC from user-mode whenever a new
// thread is created and needs to have its SEH validation frame initialized.
//
VOID WINAPI CreateSehValidationFrame(
		IN PVOID NormalContext,
		IN PVOID SystemArgument1,
		IN PVOID SystemArgument2)
{
	//
	// If we're enforcing SEH...
	//
	if (IsNreExecutionFlag(NRE_FLAG_ENFORCE_SEH))
		InstallSehValidationFrame();

	return;
}

//
// Inspects the exception record that is passed in to see if a fixup should be
// performed.  If a fixup is successfully performed, the return value is zero
// which indicates the original exception dispatcher should not be called.
// Otherwise, TRUE is returned to indicate that the exception should be passed
// on to the exception handlers.
//
BOOLEAN InspectException(
		IN PEXCEPTION_RECORD ExceptionRecord,
		IN PCONTEXT Context)
{
	ACCESS_VIOLATION_TYPE AvType;
	SELECTED_EXIT_METHOD  ExitMethod;
	PINSTRUCTION          Instruction;
	INSTRUCTION           InstructionBuffer;
	ULONG_PTR             InstructionPointer;
	ULONG_PTR             FaultAddress;
	BOOLEAN               ExploitationAttempt;
	BOOLEAN               PassException = TRUE;

	do
	{
		//
		// If the exception was not related to an access violation we do not
		// attempt to inspect it.  We also check illegal instructions, though.
		//
		if ((ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) &&
		    (ExceptionRecord->ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION))
			break;

		//
		// Determine what type of access violation it was
		//
		if (ExceptionRecord->ExceptionInformation[0])
			AvType = AccessViolationWrite;
		else
			AvType = AccessViolationRead;

		//
		// Extract the instruction from the instruction pointer that lead to the
		// access violation.
		//
		InstructionPointer = Context->Eip;
		FaultAddress       = ExceptionRecord->ExceptionInformation[1];

		//
		// Check to see if this is a reference inside the reserved process
		// parameters mapping.
		//
		if (FixupProcessParametersReference(
				InstructionPointer,
				FaultAddress))
			break;

		//
		// If we are not enforcing SEH/stack checks, then return now.
		//
		if ((!IsNreExecutionFlag(NRE_FLAG_ENFORCE_SEH)) &&
		    (!IsNreExecutionFlag(NRE_FLAG_ENFORCE_STACK)))
			break;

		//
		// If the instruction pointer is equal to the fault address or there is an
		// error in disassembling the instruction pointer, set the instruction to
		// NULL to symbolize the fact that it is unknown.
		//
		if ((InstructionPointer == FaultAddress) ||
		    (!GetExceptionInstruction(
				InstructionPointer,
				&InstructionBuffer)))
			Instruction = NULL;
		else
			Instruction = &InstructionBuffer;

		//
		// After obtaining the instruction, check to see if it may be associated
		// with some kind of exploitation attempt based on a number of heuristics.
		//
		if ((ExploitationAttempt = CheckExploitationAttempt(
				Context,
				Instruction,
				FaultAddress,
				AvType,
				&ExitMethod)))
		{
			NreExitWithMethod(
					ExitMethod);
		}
		//
		// If the access violation is not related to an exploitation attempt,
		// check to see if the address is inside the original executables image
		// region.  If it is, we may need to fix it up.
		//
		else if (IsAddressInsideOriginalExecutableRegion(
				FaultAddress))
		{
			//
			// If we don't have a valid original executable image base, then
			// mirroring is not enabled for this process.
			//
			if (!OrigExecutableImageBase)
				break;

			//
			// Mirror the address reference by correcting operands to the
			// instruction so that they are pointed to the new address region for
			// the executable.  If the mirroring succeeds, we will not pass this
			// exception on to the registered exception handlers.
			//
			if (MirrorExecutableAddressReference(
					Context,
					InstructionPointer,
					FaultAddress,
					AvType,
					Instruction))
			{
				//
				// Indicate that it should not be passed on
				//
				PassException = FALSE;
			}
		}

	} while (0);

	return PassException;
}

//
// Gets the instruction that is associated with the supplied instruction pointer
// if the address being referenced is a valid memory address.
//
BOOLEAN GetExceptionInstruction(
		IN ULONG_PTR InstructionPointer,
		OUT PINSTRUCTION Instruction)
{
	BOOLEAN ValidInstruction = TRUE;

	if (get_instruction(
			Instruction,
			(LPBYTE)InstructionPointer,
			MODE_32) <= 0)
		ValidInstruction = FALSE;

	return ValidInstruction;
}

//
// Checks to see if an exploitation attempt may be occurring such that the user
// can be notified and the process can be terminated if necessary.
//
BOOLEAN CheckExploitationAttempt(
		IN PCONTEXT Context,
		IN PINSTRUCTION Instruction OPTIONAL,
		IN ULONG_PTR FaultAddress,
		IN ACCESS_VIOLATION_TYPE AvType,
		OUT PSELECTED_EXIT_METHOD ExitMethod OPTIONAL)
{
	EXPLOIT_INFORMATION ExploitInformation;
	BOOLEAN             ExploitationAttempt = FALSE;

	nrememset(&ExploitInformation, 0, sizeof(ExploitInformation));

	do
	{
		////
		//
		// CHECK: SEH Overwrite
		//
		////

		//
		// Is the SEH chain valid for this thread?
		//
		if ((IsNreExecutionFlag(NRE_FLAG_ENFORCE_SEH)) &&
		    (!IsSehChainValid(
				&ExploitInformation)))
		{
			ExploitationAttempt = TRUE;
			break;
		}

		////
		//
		// CHECK: Stack overflow
		//
		////

		//
		// If there is no valid instruction and the access violation was due to a
		// read operation, that means the program referenced an invalid memory
		// address for execution.  This may be a likely suspect for an
		// exploitation attempt.
		//
		if ((!Instruction) &&
		    (AvType == AccessViolationRead))
		{
			ULONG_PTR ThreadStackBase;
			ULONG_PTR ThreadStackLimit;

			if (IsNreExecutionFlag(NRE_FLAG_ENFORCE_STACK))
			{
				////
				//
				// CHECK: EBP points to invalid memory
				//
				// This check is used to detect potential stack overflows by checking
				// to see if EBP points to an invalid memory region.  In general, EBP
				// must point to an address that is on the stack.  This means we can
				// check to see if EBP is within the range of the stack for this
				// thread.
				//
				////
				__asm
				{
					mov eax, fs:[0x4]
					mov [ThreadStackBase], eax
					mov eax, fs:[0x8]
					mov [ThreadStackLimit], eax
				}

				if ((Context->Ebp < ThreadStackLimit) ||
					 (Context->Ebp > ThreadStackBase))
				{
					ExploitInformation.Type = StackOverflow;
					ExploitInformation.Stack.InvalidFramePointer = (PVOID)Context->Ebp;
					ExploitInformation.Stack.FaultAddress        = (PVOID)FaultAddress;

					ExploitationAttempt = TRUE;
					break;
				}
			}
		}

	} while (0);

	if (ExploitationAttempt)
	{
		WEHNSERV_RESPONSE Response;

		//
		// Don't pass the exception along if we've detected an exploitation
		// attempt.
		//
		if (!NT_SUCCESS(NreLogExploitationEvent(
				&ExploitInformation,
				&Response)))
			Response.ExploitInformation.ExitMethod = UseExitProcess;

		//
		// If an exit method was supplied, pass it back to the caller.
		//
		if (ExitMethod)
			*ExitMethod = Response.ExploitInformation.ExitMethod;
	}

	return ExploitationAttempt;
}

//
// Performs the mirroring operation between the original executable region and
// the new one based on the information provided.
//
BOOLEAN MirrorExecutableAddressReference(
		IN PCONTEXT Context,
		IN ULONG_PTR InstructionPointer,
		IN ULONG_PTR FaultAddress,
		IN ACCESS_VIOLATION_TYPE AvType,
		IN PINSTRUCTION Instruction)
{
	BOOLEAN Success = TRUE;

	//
	// If the fault occurred at EIP, fixup EIP to point to the new executable
	// region.
	//
	if ((InstructionPointer == FaultAddress) &&
	    (AvType == AccessViolationRead))
	{
		Context->Eip = FixupOriginalExecutableAddress(
				Context->Eip);
	}
	//
	// Otherwise, if we have a valid instruction, fixup the operands to the
	// instruction that are making reference to the old executable region with
	// the new one.
	//
	else if (Instruction)
	{
		POPERAND Operand = NULL;

		//
		// We pick the operand that is making a memory reference.  It's times like
		// these that we thank Intel for not allowing mem,mem.
		//
		if (Instruction->op1.type == OPERAND_TYPE_MEMORY)
			Operand = &Instruction->op1;
		else if (Instruction->op2.type == OPERAND_TYPE_MEMORY)
			Operand = &Instruction->op2;
		//
		// If we fail to find an operand that doesn't reference memory, such as an
		// instruction that has implicit operands (string functions), then we do
		// some more checks...
		//
		else
		{
			if (Instruction->type == INSTRUCTION_TYPE_STOS)
				FixupContextRegister(
						Context,
						REGISTER_EDI);
			else if (Instruction->type == INSTRUCTION_TYPE_LODS)
				FixupContextRegister(
						Context,
						REGISTER_ESI);
			else if (Instruction->type == INSTRUCTION_TYPE_SCAS)
				FixupContextRegister(
						Context,
						REGISTER_EDI);
			else if (Instruction->type == INSTRUCTION_TYPE_MOVS)
			{
				FixupContextRegister(
						Context,
						REGISTER_ESI);
				FixupContextRegister(
						Context,
						REGISTER_EDI);
			}
			else if (Instruction->type == INSTRUCTION_TYPE_CMPS)
			{
				FixupContextRegister(
						Context,
						REGISTER_ESI);
				FixupContextRegister(
						Context,
						REGISTER_EDI);
			}
			else
			{
				__asm int 3

				Success = FALSE;
			}

			goto MirrorOut;
		}

		//
		// Fixup registers if they make reference to the old executable address
		// region
		//
		if (Operand->reg != REGISTER_NOP)
			FixupContextRegister(
					Context,
					Operand->reg);
		if (Operand->basereg != REGISTER_NOP)
			FixupContextRegister(
					Context,
					Operand->basereg);

		//
		// If displacement is being used, check to see if it's inside the
		// executables address region.  If so, we need to patch the instruction
		// manually to reference the new base address.  This is common for global
		// variables and dispatch tables.
		//
		if ((Operand->displacement > 0) &&
		    (Operand->dispbytes == 4) &&
		    (IsAddressInsideOriginalExecutableRegion(
				Operand->displacement)))
		{
			FixupStaticAddressReference(
					(PULONG)(Context->Eip + Operand->dispoffset));
		}
	}

MirrorOut:
	return Success;
}

//
// Fixes up the context register supplied in RegisterIndex if it's inside the
// original executable's address region
//
VOID FixupContextRegister(
		IN PCONTEXT Context,
		IN ULONG RegisterIndex)
{
	PULONG Register = NULL;

	switch (RegisterIndex)
	{
		case REGISTER_EAX: Register = &Context->Eax; break;
		case REGISTER_EBX: Register = &Context->Ebx; break;
		case REGISTER_ECX: Register = &Context->Ecx; break;
		case REGISTER_EDX: Register = &Context->Edx; break;
		case REGISTER_EDI: Register = &Context->Edi; break;
		case REGISTER_ESI: Register = &Context->Esi; break;
		case REGISTER_EBP: Register = &Context->Ebp; break;
		case REGISTER_ESP: Register = &Context->Esp; break;
		default: break;
	}

	//
	// If the register is valid and it's inside the original executable's address
	// region, fix it up!
	//
	if ((Register) &&
	    (IsAddressInsideOriginalExecutableRegion(
			*Register)))
	{
		*Register = FixupOriginalExecutableAddress(
				*Register);
	}
}

//
// Patches a static address reference to the original memory region.
//
VOID FixupStaticAddressReference(
		IN PULONG StaticAddressReference)
{
	LPVOID Base = (LPVOID)((ULONG_PTR)StaticAddressReference & ~(PAGE_SIZE - 1));
	ULONG  OldProtection = 0;
	ULONG  Size = PAGE_SIZE * 2;

	//
	// If re-protecting the address succeeds...
	//
	if (NreProtectVirtualMemory(
			Base,
			Size,
			PAGE_EXECUTE_READWRITE,
			&OldProtection) == 0)
	{
		*StaticAddressReference = FixupOriginalExecutableAddress(
				*StaticAddressReference);
	}
}

//
// The default address at which RTL_USER_PROCESS_PARAMETERS resides.
//
#define RTL_USER_PROCESS_PARAMETERS_ADDRESS 0x20000

//
// Creates a no-access page at the location that the process parameters
// structure is expected to be allocated.  This effectively reserves the
// allocation for later use if it is referenced by an application.
//
static VOID CreateProcessParametersMirror()
{
	PULONG ProcessParameters = (PULONG)NreGetProcessParameters();

	//
	// If the process parameters structure is not already at 0x20000, create a
	// no-access mapping at that location.
	//
	if (((ULONG_PTR)ProcessParameters != RTL_USER_PROCESS_PARAMETERS_ADDRESS) &&
	    (NreAllocateNoAccess(
			(PVOID)RTL_USER_PROCESS_PARAMETERS_ADDRESS,
			ProcessParameters ? ProcessParameters[0] : 0x1000)))
	{
		ProcessParametersMirrorCreated = TRUE;
	}
}

//
// Copys the contents of the process parameters structure to the expected memory
// location such that it can be referenced.
//
static VOID CopyProcessParameters()
{
	PULONG ProcessParameters = (PULONG)NreGetProcessParameters();
	ULONG  Length = ProcessParameters ? ProcessParameters[0] : 0x1000;
	ULONG  Old;

	//
	// If the mirror was successfully created prior to this, copy the real
	// process parameters structure to its expected location.
	//
	if ((ProcessParametersMirrorCreated) &&
	    (NreProtectVirtualMemory(
			(PVOID)RTL_USER_PROCESS_PARAMETERS_ADDRESS,
			Length,
			PAGE_READWRITE,
			&Old) == STATUS_SUCCESS))
	{
		nrememcpy(
				(PVOID)RTL_USER_PROCESS_PARAMETERS_ADDRESS,
				ProcessParameters,
				Length);
	}
}

//
// Fixes references to the process parameters structure by copying the process
// parameters structure to its expected location from where it currently
// resides.
//
static BOOLEAN FixupProcessParametersReference(
		IN ULONG_PTR InstructionPointer,
		IN ULONG_PTR FaultAddress)
{
	PULONG ProcessParameters = (PULONG)NreGetProcessParameters();
	ULONG  Length = ProcessParameters ? ProcessParameters[0] : 0x1000;

	//
	// If the fault occurred within the region that we created to mirror the
	// process parameters structure, then go ahead and copy it to its expected
	// location.
	//
	if (((FaultAddress  < ((ULONG_PTR)RTL_USER_PROCESS_PARAMETERS_ADDRESS) + Length) &&
	     (FaultAddress >= (ULONG_PTR)RTL_USER_PROCESS_PARAMETERS_ADDRESS)) &&
	    (InstructionPointer != FaultAddress))
	{
		CopyProcessParameters();

		return TRUE;
	}

	return FALSE;
}

//
// To make the compiler happy...
//
void nrememcpy(void *dst, void *src, int len)
{
	int remain = len % 4;

	len >>= 2;

	while (len-- > 0)
	{
		*(unsigned long *)dst = *(unsigned long *)src;

		(char *)dst += 4;
		(char *)src += 4;
	}

	while (remain-- > 0)
	{
		*(unsigned char *)dst = *(unsigned char *)src;

		((char *)dst)++;
		((char *)src)++;
	}
}

void nrememset(void *dst, int val, int len)
{
	while (len-- > 0)
		((char *)dst)[len] = val;
}
