/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#include "NRER.h"

//
// This dispatch table is populated by the kernel mode driver when the DLL is
// first loaded in the context of a process.  It contains the addresses of the
// symbols that are exported by NTDLL that are required to successfully
// implement mirroring of the executable's address region, among other things.
//
NRER_NT_DISPATCH_TABLE DispatchTable = 
{ 
	InlineInitializeStructure(sizeof(DispatchTable)),
	NULL, // NtdllImageBase
	NULL, // LdrGetProcedureAddress
	NULL, // LdrLoadDll
	NULL, // NtProtectVirtualMemory

	NULL, // LdrInitializeThunk
	NULL, // KiUserExceptionDispatcher
};

//
// Global variables holding dependent symbol addresses.
//
static ULONG (NTAPI *LdrUnloadDllProc)(
		IN HMODULE ModuleHandle) = NULL;
static ULONG (NTAPI *LdrGetDllHandleProc)(
		IN PWORD Path OPTIONAL,
		IN PVOID Unused OPTIONAL,
		IN PUNICODE_STRING ModuleFileName,
		OUT PHANDLE ModuleHandle) = NULL;
static ULONG (NTAPI *NtAllocateVirtualMemoryProc)(
		IN HANDLE ProcessHandle,
		IN OUT PVOID *BaseAddress,
		IN ULONG ZeroBits,
		IN OUT PULONG RegionSize,
		IN ULONG AllocationType,
		IN ULONG Protect) = NULL;
static ULONG (NTAPI *NtQueryVirtualMemoryProc)(
		IN HANDLE ProcessHandle,
		IN PVOID BaseAddress,
		IN MEMORY_INFORMATION_CLASS MemoryInformationClass,
		OUT PVOID MemoryInformation,
		IN ULONG MemoryInformationLength,
		OUT PULONG ResultLength OPTIONAL) = NULL;
static ULONG (NTAPI *NtFreeVirtualMemoryProc)(
		IN HANDLE ProcessHandle,
		IN OUT PVOID *BaseAddress,
		IN OUT PULONG RegionSize,
		IN ULONG FreeType) = NULL;
static ULONG (NTAPI *NtContinueProc)(
		IN PCONTEXT Context,
		IN BOOLEAN RemoveAlert) = NULL;
static ULONG (NTAPI *NtQueryInformationProcessProc)(
		IN HANDLE ProcessHandle,
		IN PROCESSINFOCLASS InformationClass,
		OUT PVOID ProcessInformation,
		IN ULONG ProcessInformationLength,
		OUT PULONG ReturnLength OPTIONAL) = NULL;
static ULONG (NTAPI *NtTerminateProcessProc)(
		IN HANDLE ProcessHandle,
		IN ULONG ExitCode) = NULL;
static ULONG (NTAPI *NtTerminateThreadProc)(
		IN HANDLE ThreadHandle,
		IN ULONG ExitCode) = NULL;
static ULONG (NTAPI *NtCreateFileProc)(
		OUT PHANDLE FileHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		OUT PIO_STATUS_BLOCK IoStatusBlock,
		IN PLARGE_INTEGER AllocationSize OPTIONAL,
		IN ULONG FileAttributes,
		IN ULONG ShareAccess,
		IN ULONG CreateDisposition,
		IN ULONG CreateOptions,
		IN PVOID EaBuffer OPTIONAL,
		IN ULONG EaLength) = NULL;
static ULONG (NTAPI *NtSetInformationFileProc)(
		IN HANDLE FileHandle,
		OUT PIO_STATUS_BLOCK IoStatusBlock,
		IN PVOID FileInformation,
		IN ULONG FileInformationLength,
		IN FILE_INFORMATION_CLASS FileInformationClass) = NULL;
static ULONG (NTAPI *NtFsControlFileProc)(
		IN HANDLE FileHandle,
		IN HANDLE Event OPTIONAL,
		IN PVOID ApcRoutine OPTIONAL,
		IN PVOID ApcContext OPTIONAL,
		OUT PIO_STATUS_BLOCK IoStatusBlock,
		IN ULONG FsControlCode,
		IN PVOID InputBuffer OPTIONAL,
		IN ULONG InputBufferLength,
		OUT PVOID OutputBuffer OPTIONAL,
		IN ULONG OutputBufferLength) = NULL;
static ULONG (NTAPI *NtCloseProc)(
		IN HANDLE Handle) = NULL;
static PVOID (NTAPI *RtlAllocateHeapProc)(
		IN HANDLE HeapHandle,
		IN ULONG Flags,
		IN ULONG Length) = NULL;
static VOID (NTAPI *RtlFreeHeapProc)(
		IN HANDLE HeapHandle,
		IN ULONG Flags,
		IN PVOID Buffer) = NULL;

////
//
// Native API and dependent symbol wrappers.
//
////

//
// Unicode string length calculator.
//
static ULONG NreWideCharLength(
		IN PWCHAR String)
{
	ULONG Length = 0;

	while (String[Length]) Length++;

	return Length;
}

//
// Initialize a unicode string.
//
static VOID NreInitUnicodeString(
		OUT PUNICODE_STRING UnicodeString,
		IN PWCHAR String)
{
	UnicodeString->Buffer        = String;
	UnicodeString->Length        = (USHORT)(NreWideCharLength(String) * 2);
	UnicodeString->MaximumLength = UnicodeString->Length;
}

//
// Initializes an ANSI string.
//
static VOID NreInitAnsiString(
		OUT PANSI_STRING AnsiString,
		IN PCHAR String)
{
	AnsiString->Buffer        = String;
	AnsiString->Length        = strlen(String);
	AnsiString->MaximumLength = strlen(String);
}

// 
// Continue execution.
//
ULONG NtContinue(
		IN PCONTEXT Context,
		IN BOOLEAN RemoveAlert)
{
	RESOLVE_SYMBOL(NULL, NtContinue);

	return NtContinueProc(
			Context,
			RemoveAlert);
}

//
// Returns the heap for the current process.
//
HANDLE NreGetProcessHeap()
{
	__asm
	{
		mov eax, fs:[0x18]
		mov eax, [eax+0x30]
		mov eax, [eax+0x18]
	}
}

//
// Allocates memory from the process heap.
//
PVOID NreAllocate(
		IN ULONG Length)
{
	PVOID Base = NULL;

	RESOLVE_SYMBOL_RV(NULL, NtAllocateVirtualMemory, PVOID);

	NtAllocateVirtualMemoryProc(
			(HANDLE)-1,
			&Base,
			0,
			&Length,
			MEM_COMMIT,
			PAGE_EXECUTE_READWRITE);

	return Base;
}

//
// Allocates no-access memory at a specific location in memory.
//
PVOID NreAllocateNoAccess(
		IN PVOID Address,
		IN ULONG Length)
{
	PVOID Base = Address;

	RESOLVE_SYMBOL_RV(NULL, NtAllocateVirtualMemory, PVOID);

	NtAllocateVirtualMemoryProc(
			(HANDLE)-1,
			&Base,
			0,
			&Length,
			MEM_COMMIT | MEM_RESERVE,
			PAGE_NOACCESS);

	return Base;
}

//
// Deallocates memory from the process heap.
//
ULONG NreFree(
		IN PVOID Buffer)
{
	RESOLVE_SYMBOL(NULL, NtFreeVirtualMemory);

	return NtFreeVirtualMemoryProc(
			(HANDLE)-1,
			&Buffer,
			NULL,
			MEM_RELEASE);
}

//
// Loads the supplied module file.
//
ULONG NreLoadLibrary(
		IN PWCHAR ModuleFileName,
		OUT HMODULE *ModuleHandle)
{
	UNICODE_STRING UnicodeString;

	NreInitUnicodeString(
			&UnicodeString,
			ModuleFileName);

	return DispatchTable.LdrLoadDll(
			NULL,
			0,
			&UnicodeString,
			ModuleHandle);
}

//
// Returns the procedure address associated with the supplied symbol name.
//
ULONG NreGetProcAddress(
		IN HMODULE ModuleHandle,
		IN PCHAR SymbolName,
		OUT PVOID *SymbolAddress)
{
	ANSI_STRING AnsiString;

	NreInitAnsiString(
			&AnsiString,
			SymbolName);

	return DispatchTable.LdrGetProcedureAddress(
			ModuleHandle,
			&AnsiString,
			0,
			SymbolAddress);
}

//
// Gets the handle that's associated with a given module.
//
ULONG NreGetModuleHandle(
		IN PWCHAR ModuleFileName,
		OUT HMODULE *ModuleHandle)
{
	UNICODE_STRING UnicodeString;

	RESOLVE_SYMBOL(NULL, LdrGetDllHandle);

	NreInitUnicodeString(
			&UnicodeString,
			ModuleFileName);

	return LdrGetDllHandleProc(
			NULL,
			NULL,
			&UnicodeString,
			ModuleHandle);
}

//
// Decrements the reference count on a loaded module and possibly unloads it.
//
ULONG NreFreeLibrary(
		IN HMODULE ModuleHandle)
{
	RESOLVE_SYMBOL(NULL, LdrUnloadDll);

	return LdrUnloadDllProc(
			ModuleHandle);
}

//
// Queries the properties of a virtual memory address.
//
ULONG NreQueryVirtualMemory(
		IN PVOID BaseAddress,
		IN MEMORY_INFORMATION_CLASS InformationClass,
		OUT PVOID MemoryInformation,
		IN ULONG MemoryInformationLength,
		OUT PULONG ReturnLength OPTIONAL)
{
	RESOLVE_SYMBOL(NULL, NtQueryVirtualMemory);

	return NtQueryVirtualMemoryProc(
			NtCurrentProcess(),
			BaseAddress,
			InformationClass,
			MemoryInformation,
			MemoryInformationLength,
			ReturnLength);
}

//
// Re-protects a virtual memory region.
//
ULONG NreProtectVirtualMemory(
		IN PVOID BaseAddress,
		IN ULONG RegionSize,
		IN ULONG Protection,
		OUT PULONG OldProtection OPTIONAL)
{
	ULONG LocalOldProtection;

	if (!OldProtection)
		OldProtection = &LocalOldProtection;

	return DispatchTable.NtProtectVirtualMemory(
			NtCurrentProcess(),
			&BaseAddress,
			&RegionSize,
			Protection,
			OldProtection);
}

//
// Exit using whatever method has been selected by the response.
//
ULONG NreExitWithMethod(
		IN SELECTED_EXIT_METHOD ExitMethod)
{
	switch (ExitMethod)
	{
		case UseExitProcess:
			NreExitProcess(
					0x4e484557);
			break;
		case UseExitThread:
		default:
			NreExitThread(
					0x4e484557);
			break;
	}

	return 0;
}

//
// Terminates the calling process.
//
ULONG NreExitProcess(
		IN ULONG ExitCode)
{
	RESOLVE_SYMBOL(NULL, NtTerminateProcess);

	return NtTerminateProcessProc(
			NtCurrentProcess(),
			ExitCode);
}

//
// Terminates the calling thread.
//
ULONG NreExitThread(
		IN ULONG ExitCode)
{
	RESOLVE_SYMBOL(NULL, NtTerminateThread);

	return NtTerminateThreadProc(
			NtCurrentThread(),
			ExitCode);
}

//
// Returns information about the current process to the caller.
//
ULONG NreGetCurrentProcessInformation(
		OUT PULONG ProcessId,
		OUT PWCHAR ProcessFileName,
		IN ULONG ProcessFileNameLength)
{
	PROCESS_BASIC_INFORMATION BasicInformation;
	ULONG                     Status;

	RESOLVE_SYMBOL(NULL, NtQueryInformationProcess);

	if (ProcessId)
	{
		//
		// Extract the process identifier
		//
		if (NT_SUCCESS(Status = NtQueryInformationProcessProc(
				NtCurrentProcess(),
				ProcessBasicInformation,
				&BasicInformation,
				sizeof(BasicInformation),
				NULL)))
			*ProcessId = (ULONG)BasicInformation.UniqueProcessId;
		else
			*ProcessId = 0;
	}

	if (ProcessFileName)
	{
		PUNICODE_STRING NameUnicodeString;
		WCHAR           TempBuffer[1024];
		
		NameUnicodeString = (PUNICODE_STRING)TempBuffer;
		
		//
		// Extract the process image file name
		//
		if (NT_SUCCESS(Status = NtQueryInformationProcessProc(
				NtCurrentProcess(),
				ProcessImageFileName,
				NameUnicodeString,
				sizeof(TempBuffer),
				NULL)))
		{
			PWCHAR Current;
			ULONG  Chars = NameUnicodeString->Length / sizeof(WCHAR);
			ULONG  Offset = Chars - 1;
			ULONG  Length = 0;

			//
			// Scan the path to find the executable name.
			//
			for (Current = NameUnicodeString->Buffer + Offset;
			     Offset > 0;
			     Current--, Offset--, Length += sizeof(WCHAR))
			{
				if (*Current == L'\\')
				{
					Current++;
					break;
				}
			}

			//
			// Copy just the executable name, if possible.
			//
			nrememcpy(
					ProcessFileName,
					Current,
					Length > ProcessFileNameLength 
						? ProcessFileNameLength
						: Length);
		}
	}

	return Status;
}

//
// Returns the address of the process parameters structure.
//
PVOID NreGetProcessParameters()
{
	__asm
	{
		mov eax, fs:[0x30]
		mov eax, [eax+0x10]
	}
}

//
// Logs that an exploitation event occurred in whatever manner has currently
// been selected.
//
ULONG NreLogExploitationEvent(
		IN PEXPLOIT_INFORMATION ExploitInformation,
		OUT PWEHNSERV_RESPONSE Response OPTIONAL)
{
	WEHNSERV_REQUEST Request;

	Request.Type = WehnServExploitInformation;

	nrememcpy(
			&Request.ExploitInformation,
			ExploitInformation,
			sizeof(EXPLOIT_INFORMATION));

	return NreSendRequest(
			&Request,
			Response);
}

//
// Transmits a packet to the WehnServ service so that it can be handled in
// whatever manner is necessary.
//
ULONG NreSendRequest(
		IN PWEHNSERV_REQUEST Request,
		OUT PWEHNSERV_RESPONSE Response OPTIONAL)
{
	FILE_PIPE_INFORMATION PipeInformation;
	OBJECT_ATTRIBUTES     Attributes;
	WEHNSERV_RESPONSE     LocalResponse;
	IO_STATUS_BLOCK       IoStatus;
	UNICODE_STRING        PipeName;
	HANDLE                WehnServ = NULL;
	ULONG                 Status;

	//
	// If no output response buffer was supplied, use a local one.
	//
	if (!Response)
		Response = &LocalResponse;

	//
	// Default to ExitProcess for the exit method.
	//
	Response->ExploitInformation.ExitMethod = UseExitProcess;

	//
	// Resolve the symbols we need to transmit the packet to the service.
	//
	RESOLVE_SYMBOL(NULL, NtCreateFile);
	RESOLVE_SYMBOL(NULL, NtSetInformationFile);
	RESOLVE_SYMBOL(NULL, NtFsControlFile);
	RESOLVE_SYMBOL(NULL, NtClose);

	//
	// Get our process identifier.
	//
	NreGetCurrentProcessInformation(
			&Request->SourceProcessId,
			NULL,
			0);

	do 
	{
		//
		// Open the named pipe.
		//
		PipeName.Buffer = WEHNSERV_PIPE_NT_NAME;
		PipeName.Length = PipeName.MaximumLength = WEHNSERV_PIPE_NT_NAME_SZ;

		nrememset(
				&Attributes,
				0,
				sizeof(Attributes));

		Attributes.Length     = sizeof(Attributes);
		Attributes.ObjectName = &PipeName;

		if (!NT_SUCCESS(Status = NtCreateFileProc(
				&WehnServ,
				GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
				&Attributes,
				&IoStatus,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				FILE_OPEN,
				FILE_SYNCHRONOUS_IO_NONALERT,
				NULL,
				0)))
			break;

		//
		// Switch it to message-mode.
		//
		PipeInformation.ReadModeMessage  = 1;
		PipeInformation.WaitModeBlocking = 1;

		if (!NT_SUCCESS(Status = NtSetInformationFileProc(
				WehnServ,
				&IoStatus,
				&PipeInformation,
				sizeof(PipeInformation),
				FilePipeInformation)))
			break;

		if (!NT_SUCCESS(Status = NtFsControlFileProc(
				WehnServ,
				NULL,
				NULL,
				NULL,
				&IoStatus,
				0x11C017,
				Request,
				sizeof(WEHNSERV_REQUEST),
				Response,
				sizeof(WEHNSERV_RESPONSE))))
			break;

	} while (0);

	//
	// Close the handle to the pipe.
	//
	if (WehnServ)
		NtCloseProc(
				WehnServ);

	return 0;
}

//
// This routine hooks all the routines that are needed for added security
// enforcements, such as all format string routines and other stuff of that
// sort.
//
VOID NreHookEnforcementRoutines()
{
	static BOOLEAN NtdllRoutinesHooked = FALSE;
	static BOOLEAN MsvcrtRoutinesHooked = FALSE;

	//
	// If we haven't already hooked our enforcement routines, do so now.
	//
	if (!NtdllRoutinesHooked)
	{
		NtdllRoutinesHooked = TRUE;

		HookNtdllFormatStringRoutines();
	}

	//
	// Next, check to see if MSVCRT routines haven't been hooked
	//
	if (!MsvcrtRoutinesHooked)
	{
		HMODULE MsvcrtHandle = NULL;
		ULONG   Status = ERROR_NOT_FOUND;

		NreGetModuleHandle(
				L"msvcrt",
				&MsvcrtHandle);

		//
		// If the msvcrt handle is valid, try to hook the msvcrt routines
		//
		if (MsvcrtHandle)
			Status = HookMsvcrtFormatStringRoutines();

		//
		// Flag the MSVCRT routines as having been hooked successfully.
		//
		if (Status == ERROR_SUCCESS)
			MsvcrtRoutinesHooked = TRUE;
	}
}

//
// This function hooks other function implementations.
//
ULONG NreHookRoutine(
		IN PVOID FunctionToHook,
		IN PVOID HookFunction,
		IN BOOLEAN UseHeapForStorage,
		OUT PVOID *OrigFunction)
{
	INSTRUCTION Instruction;
	PUCHAR      HookBuffer = NULL;
	ULONG       Status = ERROR_SUCCESS;
	ULONG       PreambleSize = 0;
	ULONG       InstSize = 0;
	PBYTE       Preamble = (PBYTE)FunctionToHook;
	ULONG       TotalSize;
	ULONG       Alignment;
	ULONG       OrigProtection = PAGE_EXECUTE_READ;
	
	do
	{
		//
		// Get the size of the function's preamble
		//
		while ((InstSize = get_instruction(
				&Instruction,
				Preamble + PreambleSize,
				MODE_32)) &&
		       ((PreambleSize += InstSize) < 6));

		//
		// Re-protect the function we're hooking to execute/read/write.
		//
		if (!NT_SUCCESS(Status = NreProtectVirtualMemory(
				FunctionToHook,
				PreambleSize,
				PAGE_EXECUTE_READWRITE,
				&OrigProtection)))
			break;

		// Four byte alignment of the preamble plus two to account for the jmp
		// indirect.
		Alignment = (PreambleSize + 0x5) & ~0x3;

		//
		// Calculate the total amount of memory we're going to need.
		//
		TotalSize = Alignment + (sizeof(PVOID) * 3);

		//
		// We always use NreAllocate because we have to be careful to work on
		// machines that support hardware enforced DEP.  If we were to use the
		// process heap or our own internal data segment, we could run into
		// race conditions with reprotecting the memory region and other
		// not-so-fun stuff.
		//
		if (!(HookBuffer = (PUCHAR)NreAllocate(
				TotalSize)))
		{
			Status = ERROR_NOT_ENOUGH_MEMORY;
			break;
		}

		//
		// Copy the preamble.
		//
		nrememcpy(
				HookBuffer,
				Preamble,
				PreambleSize);

		//
		// Append the indirect jump back to the original function.
		//
		HookBuffer[PreambleSize]                     = 0xff;
		HookBuffer[PreambleSize + 1]                 = 0x25;
		*((PVOID *)(HookBuffer + PreambleSize + 2))  = (PVOID)(HookBuffer + Alignment + 4);

		//
		// Next comes the pointer to the original function.
		//
		*((PVOID *)(HookBuffer + Alignment + 4))  = (PVOID)(Preamble + PreambleSize);

		//
		// And last, the pointer to our hook routine for use in our indirect jump
		// that we overwrite the preamble with.
		//
		*((PVOID *)(HookBuffer + Alignment + 8)) = HookFunction;

		//
		// Set the HookBuffer's protection to PAGE_EXECUTE_READ so that we don't
		// get into trouble with DEP.
		//
		if (!NT_SUCCESS(Status = NreProtectVirtualMemory(
				HookBuffer,
				TotalSize,
				PAGE_EXECUTE_READ,
				NULL)))
			break;

		//
		// Now, let's overwrite the preamble.
		//
		Preamble[0] = 0xff;
		Preamble[1] = 0x25;
		*((PVOID *)(Preamble + 2)) = (PVOID)(HookBuffer + Alignment + 8);

		//
		// Set the output pointer.
		//
		if (OrigFunction)
			*OrigFunction = (PVOID)HookBuffer;

		//
		// Successful.
		//
		Status = STATUS_SUCCESS;

	} while (0);

	//
	// Try to re-protect the page back to execute/read (no write) even
	// if we might have failed somewhere before that step.
	//
	NreProtectVirtualMemory(
			FunctionToHook,
			PreambleSize,
			OrigProtection,
			&OrigProtection);

	return Status;
}

////
//
// Resolution routines
//
////

//
// Resolves a dependent symbol.
//
ULONG ResolveDependentSymbol(
		IN PWCHAR ModuleFileName,
		IN PCHAR SymbolName,
		OUT PVOID *SymbolAddress)
{
	HMODULE ModuleHandle = NULL;
	ULONG   Status = ERROR_SUCCESS;

	if (ModuleFileName)
	{
		//
		// Get the base address of the module as it should already have been
		// loaded.
		//
		Status = NreGetModuleHandle(
				ModuleFileName,
				&ModuleHandle);
	}
	else
		ModuleHandle = DispatchTable.NtdllImageBase;

	//
	// If we successfully loaded the library and have a module handle...
	//
	if (NT_SUCCESS(Status))
		Status = NreGetProcAddress(
				ModuleHandle,
				SymbolName,
				SymbolAddress);

	return Status;
}
