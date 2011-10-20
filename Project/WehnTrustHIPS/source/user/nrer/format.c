/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#include <stdio.h>
#include "NRER.h"

//
// Format string protections are applied to all exported, public variadic
// routines that take a format specifier.  These routines include:
//
// NTDLL:
//   sprintf
//   swprintf
//   _snprintf
//   _snwprintf
//   
// MSVCRT (if loaded):
//   sprintf
//   swprintf
//   _snprintf
//   _snwprintf
//   printf
//   wprintf
//   fprintf
//   fwprintf
//

////
//
// Prototypes
//
////
static VOID _stdcall ValidateFormatString(
		IN PVOID ReturnAddress,
		IN PCHAR FormatString,
		IN ULONG FormatStringArgumentOffset,
		IN BOOLEAN UnicodeFmtString);
static ULONG GetNumberOfFormatSpecifiersA(
		IN PCHAR FormatString,
		OUT PULONG FormatStringBytes);
static ULONG GetNumberOfFormatSpecifiersW(
		IN PWCHAR FormatString,
		OUT PULONG FormatStringBytes);

//
// These macros serve as the implementations for the format string hook
// routines.
//
#define CALL_ORIG_FUNCTION(OrigFunction)            \
	__asm mov  eax, [OrigFunction]                   \
	__asm jmp  eax
#define CALL_FORMAT_CHECK_ARG(OrigFunction, Off, U) \
	__asm push U                                     \
	__asm push Off                                   \
	__asm push [esp + Off + 0x8]                     \
	__asm push [esp + 0xc]                           \
	__asm call ValidateFormatString                  \
	CALL_ORIG_FUNCTION(OrigFunction)               
#define CALL_FORMAT_CHECK_ARG0(OrigFunction)        \
	CALL_FORMAT_CHECK_ARG(OrigFunction, 0x04, 0)
#define CALL_FORMAT_CHECK_ARG1(OrigFunction)        \
	CALL_FORMAT_CHECK_ARG(OrigFunction, 0x08, 0)
#define CALL_FORMAT_CHECK_ARG2(OrigFunction)        \
	CALL_FORMAT_CHECK_ARG(OrigFunction, 0x0c, 0)
#define CALL_FORMAT_CHECK_ARG0_U(OrigFunction)      \
	CALL_FORMAT_CHECK_ARG(OrigFunction, 0x04, 1)
#define CALL_FORMAT_CHECK_ARG1_U(OrigFunction)      \
	CALL_FORMAT_CHECK_ARG(OrigFunction, 0x08, 1)
#define CALL_FORMAT_CHECK_ARG2_U(OrigFunction)      \
	CALL_FORMAT_CHECK_ARG(OrigFunction, 0x0c, 1)

////
//
// NTDLL Hook implementations
//
////

static INT (_cdecl *NtdllSprintfOrig)(
		IN LPSTR Buffer,
		IN LPCSTR Format,
		...) = NULL;
static INT (_cdecl *NtdllSwprintfOrig)(
		IN LPSTR Buffer,
		IN LPCWSTR Format,
		...) = NULL;
static INT (_cdecl *NtdllSnprintfOrig)(
		IN LPSTR Buffer,
		IN UINT Size,
		LPCSTR Format,
		...) = NULL;
static INT (_cdecl *NtdllSnwprintfOrig)(
		IN LPSTR Buffer,
		IN UINT Size,
		LPCWSTR Format,
		...) = NULL;

static INT __declspec(naked) NtdllSprintfHook(
		LPSTR Buffer,
		LPCSTR Format,
		...)
{
	CALL_FORMAT_CHECK_ARG1(NtdllSprintfOrig);
}

static INT __declspec(naked) NtdllSwprintfHook(
		IN LPSTR Buffer,
		IN LPCWSTR Format,
		...)
{
	CALL_FORMAT_CHECK_ARG1_U(NtdllSwprintfOrig);
}

static INT __declspec(naked) NtdllSnprintfHook(
		IN LPSTR Buffer,
		IN UINT Size,
		LPCSTR Format,
		...)
{
	CALL_FORMAT_CHECK_ARG2(NtdllSnprintfOrig);
}

static INT __declspec(naked) NtdllSnwprintfHook(
		IN LPSTR Buffer,
		IN UINT Size,
		LPCWSTR Format,
		...)
{
	CALL_FORMAT_CHECK_ARG2_U(NtdllSnwprintfOrig);
}

////
//
// MSVCRT
//
////
//   printf
//   wprintf
//   fprintf
//   fwprintf

static INT (_cdecl *MsvcrtSprintfOrig)(
		IN LPSTR Buffer,
		IN LPCSTR Format,
		...) = NULL;
static INT (_cdecl *MsvcrtSwprintfOrig)(
		IN LPSTR Buffer,
		IN LPCWSTR Format,
		...) = NULL;
static INT (_cdecl *MsvcrtSnprintfOrig)(
		IN LPSTR Buffer,
		IN UINT Size,
		LPCSTR Format,
		...) = NULL;
static INT (_cdecl *MsvcrtSnwprintfOrig)(
		IN LPSTR Buffer,
		IN UINT Size,
		LPCWSTR Format,
		...) = NULL;
static INT (_cdecl *MsvcrtPrintfOrig)(
		LPCSTR Format,
		...) = NULL;
static INT (_cdecl *MsvcrtWprintfOrig)(
		LPCWSTR Format,
		...) = NULL;
static INT (_cdecl *MsvcrtFprintfOrig)(
		FILE *fd,
		LPCSTR Format,
		...) = NULL;
static INT (_cdecl *MsvcrtFwprintfOrig)(
		FILE *fd,
		LPCWSTR Format,
		...) = NULL;

static INT __declspec(naked) MsvcrtSprintfHook(
		LPSTR Buffer,
		LPCSTR Format,
		...)
{
	CALL_FORMAT_CHECK_ARG1(MsvcrtSprintfOrig);
}

static INT __declspec(naked) MsvcrtSwprintfHook(
		IN LPSTR Buffer,
		IN LPCWSTR Format,
		...)
{
	CALL_FORMAT_CHECK_ARG1_U(MsvcrtSwprintfOrig);
}

static INT __declspec(naked) MsvcrtSnprintfHook(
		IN LPSTR Buffer,
		IN UINT Size,
		LPCSTR Format,
		...)
{
	CALL_FORMAT_CHECK_ARG2(MsvcrtSnprintfOrig);
}

static INT __declspec(naked) MsvcrtSnwprintfHook(
		IN LPSTR Buffer,
		IN UINT Size,
		LPCWSTR Format,
		...)
{
	CALL_FORMAT_CHECK_ARG2_U(MsvcrtSnwprintfOrig);
}

static INT __declspec(naked) MsvcrtPrintfHook(
		IN LPCSTR Format,
		...)
{
	CALL_FORMAT_CHECK_ARG0(MsvcrtPrintfOrig);
}

static INT __declspec(naked) MsvcrtWprintfHook(
		IN LPCWSTR Format,
		...)
{
	CALL_FORMAT_CHECK_ARG0_U(MsvcrtWprintfOrig);
}

static INT __declspec(naked) MsvcrtFprintfHook(
		FILE *fd,
		IN LPCSTR Format,
		...)
{
	CALL_FORMAT_CHECK_ARG1(MsvcrtFprintfOrig);
}

static INT __declspec(naked) MsvcrtFwprintfHook(
		FILE *fd,
		IN LPCWSTR Format,
		...)
{
	CALL_FORMAT_CHECK_ARG1_U(MsvcrtFwprintfOrig);
}

//
// This routine hooks as many format string routines as it can so that the
// format strings can be validated.  It only imports routines from NTDLL.DLL.
//
ULONG HookNtdllFormatStringRoutines()
{
	PVOID FmtFunctionSprintf = NULL;
	ULONG Index;
	struct
	{
		PCHAR SymbolName;
		PVOID SymbolAddress;
		PVOID HookFunction;
		PVOID *OrigFunction;
	} FunctionsToHook[] =
	{
		{ "sprintf",    NULL, NtdllSprintfHook,   (PVOID *)&NtdllSprintfOrig   },
		{ "swprintf",   NULL, NtdllSwprintfHook,  (PVOID *)&NtdllSwprintfOrig  },
		{ "_snprintf",  NULL, NtdllSnprintfHook,  (PVOID *)&NtdllSnprintfOrig  },
		{ "_snwprintf", NULL, NtdllSnwprintfHook, (PVOID *)&NtdllSnwprintfOrig },
		{ NULL,         NULL, NULL,               NULL                         },
	};

	//
	// Resolve all the dependent symbols.
	//
	for (Index = 0;
	     FunctionsToHook[Index].SymbolName;
	     Index++)
	{
		ULONG Result;

		if (((Result = ResolveDependentSymbol(
				NULL,
				FunctionsToHook[Index].SymbolName,
				&FunctionsToHook[Index].SymbolAddress))) != 0)
			continue;
	
		NreHookRoutine(
				FunctionsToHook[Index].SymbolAddress,
				FunctionsToHook[Index].HookFunction,
				TRUE,
				FunctionsToHook[Index].OrigFunction);
	}

	return ERROR_SUCCESS;
}

//
// This function hooks the format strings found in msvcrt.dll.
//
ULONG HookMsvcrtFormatStringRoutines()
{
	PVOID FmtFunctionSprintf = NULL;
	ULONG Index;
	struct
	{
		PCHAR SymbolName;
		PVOID SymbolAddress;
		PVOID HookFunction;
		PVOID *OrigFunction;
	} FunctionsToHook[] =
	{
		{ "sprintf",    NULL, MsvcrtSprintfHook,   (PVOID *)&MsvcrtSprintfOrig   },
		{ "swprintf",   NULL, MsvcrtSwprintfHook,  (PVOID *)&MsvcrtSwprintfOrig  },
		{ "_snprintf",  NULL, MsvcrtSnprintfHook,  (PVOID *)&MsvcrtSnprintfOrig  },
		{ "_snwprintf", NULL, MsvcrtSnwprintfHook, (PVOID *)&MsvcrtSnwprintfOrig },
		{ "printf",     NULL, MsvcrtPrintfHook,    (PVOID *)&MsvcrtPrintfOrig    },
		{ "wprintf",    NULL, MsvcrtWprintfHook,   (PVOID *)&MsvcrtWprintfOrig   },
		{ "fprintf",    NULL, MsvcrtFprintfHook,   (PVOID *)&MsvcrtFprintfOrig   },
		{ "fwprintf",   NULL, MsvcrtFwprintfHook,  (PVOID *)&MsvcrtFwprintfOrig  },
		{ NULL,         NULL, NULL,                NULL                          },
	};

	//
	// Resolve all the dependent symbols.
	//
	for (Index = 0;
	     FunctionsToHook[Index].SymbolName;
	     Index++)
	{
		ULONG Result;

		if (((Result = ResolveDependentSymbol(
				L"MSVCRT.DLL",
				FunctionsToHook[Index].SymbolName,
				&FunctionsToHook[Index].SymbolAddress))) != 0)
			continue;
	
		NreHookRoutine(
				FunctionsToHook[Index].SymbolAddress,
				FunctionsToHook[Index].HookFunction,
				TRUE,
				FunctionsToHook[Index].OrigFunction);
	}

	return ERROR_SUCCESS;
}

//
// This routine validates format strings in a generic fashion.  The basic
// algorithm taken to do this is two fold.  First, the format string is checked
// to see whether or not it is writable.  If it is, the number of format
// specifiers included in the format string as well as the number of arguments
// pushed are calculated and compared to see whether or not they match.  If they
// do not match then the format string is deemed invalid and the thread/process
// are terminated.
//
#define MAX_INSTRUCTIONS 6

static VOID _stdcall ValidateFormatString(
		IN PVOID ReturnAddress,
		IN PCHAR FormatString,
		IN ULONG FormatStringArgumentOffset,
		IN BOOLEAN UnicodeFmtString)
{
	MEMORY_BASIC_INFORMATION MemoryInformation;
	INSTRUCTION              Instruction;
	BOOLEAN                  Found = FALSE;
	ULONG                    InstructionLength;
	PBYTE                    CurrentInstruction = (PBYTE)ReturnAddress;
	ULONG                    NumberOfFormatSpecifiers = 0;
	ULONG                    NumberOfArguments = 0;
	ULONG                    NumberOfInstructions = 0;
	ULONG                    FormatStringBytes = 0;

	do
	{
		//
		// If we fail to query the format string for some reason, then that sucks.
		//
		if (!NT_SUCCESS(NreQueryVirtualMemory(
				FormatString,
				MemoryBasicInformation,
				&MemoryInformation,
				sizeof(MemoryInformation),
				NULL)))
			break;

		//
		// Not writable?  Cool.
		//
		if (!(MemoryInformation.Protect & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE|PAGE_WRITECOMBINE)))
			break;

		//
		// Get the number of format specifiers included in the format string.
		//
		if (UnicodeFmtString)
			NumberOfFormatSpecifiers = GetNumberOfFormatSpecifiersW(
					(PWCHAR)FormatString,
					&FormatStringBytes);
		else
			NumberOfFormatSpecifiers = GetNumberOfFormatSpecifiersA(
					FormatString,
					&FormatStringBytes);

		//
		// Now, disassemble the return address to try to determine the number of
		// format arguments that were pushed.
		//
		while ((NumberOfInstructions < MAX_INSTRUCTIONS) &&
		       (InstructionLength = get_instruction(
				&Instruction,
				CurrentInstruction,
				MODE_32)) > 0)
		{
			//
			// Is this the add instruction we're looking for?  Specifically, we're
			// looking for an:
			//
			// add esp, XX
			//
			if ((Instruction.type == INSTRUCTION_TYPE_ADD) &&
			    (Instruction.op1.type == OPERAND_TYPE_REGISTER) &&
			    (Instruction.op1.reg == REGISTER_ESP))
			{
				ULONG AdditionBytes = Instruction.op2.immediate;

				//
				// Sanity check to make sure the number of bytes we're adding is at
				// least greater than the format string offset
				//
				if (AdditionBytes < FormatStringArgumentOffset)
					return;

				//
				// Calculate the total number of arguments.
				//
				NumberOfArguments = (AdditionBytes - FormatStringArgumentOffset) / 4;

				//
				// If we found the number of arguments, flag the boolean indicating
				// such.
				//
				Found = TRUE;

				//
				// Finally, break out.
				//
				break;
			}

			//
			// Go to the next instruction to disassemble.
			//
			CurrentInstruction += InstructionLength;

			//
			// Increment the total number of instructions.
			//
			NumberOfInstructions++;
		}

		//
		// If we successfully determined the number of arguments, check to see if
		// they match with what the format specifier is indicating.  If that's the
		// case, then we know we've found something suspicious, and it's time to
		// log an event and terminate the process.
		//
		if ((Found) &&
		    (NumberOfArguments < NumberOfFormatSpecifiers))
		{
			EXPLOIT_INFORMATION ExploitInformation;
			WEHNSERV_RESPONSE   Response;

			nrememset(&ExploitInformation, 0, sizeof(ExploitInformation));

			//
			// Initialize the exploit information structure.
			//
			ExploitInformation.Type                                  = FormatStringBug;
			ExploitInformation.FormatString.UnicodeFormatString      = UnicodeFmtString;
			ExploitInformation.FormatString.NumberOfFormatSpecifiers = NumberOfFormatSpecifiers;
			ExploitInformation.FormatString.NumberOfArguments        = NumberOfArguments;
			ExploitInformation.FormatString.ReturnAddress            = ReturnAddress;

			//
			// Copy the format string.
			//
			if (UnicodeFmtString)
			{
				ULONG Bytes = FormatStringBytes;
					
				if (FormatStringBytes > sizeof(ExploitInformation.FormatString.String.w))
					Bytes = sizeof(ExploitInformation.FormatString.String.w);

				nrememcpy(
						ExploitInformation.FormatString.String.w,
						(PWCHAR)FormatString,
						Bytes);
			}
			else
			{
				ULONG Bytes = FormatStringBytes;
					
				if (FormatStringBytes > sizeof(ExploitInformation.FormatString.String.a))
					Bytes = sizeof(ExploitInformation.FormatString.String.a);

				nrememcpy(
						ExploitInformation.FormatString.String.a,
						FormatString,
						Bytes);
			}

			//
			// Log the event.
			//
			if (!NT_SUCCESS(NreLogExploitationEvent(
					&ExploitInformation,
					&Response)))
				Response.ExploitInformation.ExitMethod = UseExitProcess;

			//
			// Exit using whatever method we selected.
			//
			NreExitWithMethod(
					Response.ExploitInformation.ExitMethod);
		}

	} while (0);
}

//
// Returns the number of format specifiers in the ANSI format string.
//
static ULONG GetNumberOfFormatSpecifiersA(
		IN PCHAR FormatString,
		OUT PULONG FormatStringBytes)
{	
	ULONG NumberOfFormatSpecifiers = 0;
	PCHAR Current = FormatString;

	//
	// Calculate the number of format specifiers in the format string.
	//
	for (Current = FormatString;
	     *Current;
	    )
	{
		//
		// If it's a format character that is not followed by the NULL
		// terminator or another format specifier, increment the total number
		// of format specifiers.  Fixed logic for BUG#7.
		//
		if ((Current[0] == '%') && 
		    (Current[1]) &&
		    (Current[1] != '%'))
		{
			if ((Current != FormatString) &&
		       (Current[-1] != '%'))
			{
				NumberOfFormatSpecifiers++;
				Current += 2;
			}
			else
				Current++;
		}
		else
			Current++;
	}
	
	*FormatStringBytes = (ULONG)(Current - FormatString);

	return NumberOfFormatSpecifiers;
}

//
// Returns the number of format specifiers in the UNICODE format string.
//
static ULONG GetNumberOfFormatSpecifiersW(
		IN PWCHAR FormatString,
		OUT PULONG FormatStringBytes)
{
	PWCHAR Current = FormatString;
	ULONG  NumberOfFormatSpecifiers = 0;

	//
	// Calculate the number of format specifiers in the format string.
	//
	for (Current = FormatString;
	     *Current;
	    )
	{
		//
		// If it's a format character that is not followed by the NULL
		// terminator or another format specifier, increment the total number
		// of format specifiers.  Fixed logic for BUG#7.
		//
		if ((Current[0] == '%') && 
		    (Current[1]) &&
		    (Current[1] != '%'))
		{
			if ((Current != FormatString) &&
			    (Current[-1] != '%'))
			{
				NumberOfFormatSpecifiers++;
				Current += 2;
			}
			else
				Current++;
		}
		else
			Current++;
	
	}
		
	*FormatStringBytes = (ULONG)((PCHAR)Current - (PCHAR)FormatString);

	return NumberOfFormatSpecifiers;
}
