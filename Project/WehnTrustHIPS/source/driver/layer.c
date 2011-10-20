/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include <ntddk.h>
#include "precomp.h"
#include "Layer.h"
#include "disasm.h"

extern ULONG **KeServiceDescriptorTable;

//
// Installs a call layer at the supplied address and redirects it to the hook
// address
//
NTSTATUS InstallCallLayer(
		IN ULONG_PTR FunctionToHookAddress, 
		IN ULONG_PTR HookFunctionAddress, 
		OUT PCALL_LAYER *Layer)
{
	PCALL_LAYER Result = NULL;
	NTSTATUS    Status = STATUS_INSUFFICIENT_RESOURCES;
	PUCHAR      JmpSlotBuffer = NULL;
	ULONG       JmpSlotBufferSize = 0;
	ULONG       PreambleSize;
	UCHAR       Overwrite[6];

	//
	// Determine the preamble size of the function we're hooking.
	//
	PreambleSize      = DisassembleUntil((PUCHAR)FunctionToHookAddress, 6);
	JmpSlotBufferSize = PreambleSize + 6;

	do
	{
		//
		// Allocate storage in non-paged pool for the call layer context.
		//
		if (!(Result = (PCALL_LAYER)ExAllocatePoolWithTag(
				NonPagedPool, 
				sizeof(CALL_LAYER),
				ALLOC_TAG)))
		{
			DebugPrint(("InstallCallLayer(): Failed to allocate call layer context."));
			break;
		}

		RtlZeroMemory(
				Result,
				sizeof(CALL_LAYER));

		//
		// Allocate storage in the non-paged pool for the jump slot buffer that is
		// to be associated with the function to be hooked.
		//
		if (!(JmpSlotBuffer = (PUCHAR)ExAllocatePoolWithTag(
				NonPagedPool, 
				JmpSlotBufferSize,
				ALLOC_TAG)))
		{
			DebugPrint(("InstallCallLayer(): Failed to allocate jmp slot buffer (%lu bytes).",
					JmpSlotBufferSize));
			break;
		}

		//
		// Copy the function's preamble to the start of the jump slot
		//
		RtlCopyMemory(
				JmpSlotBuffer,
				(PVOID)FunctionToHookAddress,
				PreambleSize);

		//
		// Store the address of the instruction immediately after the first n
		// bytes of the rpeamble in RealFunctionPostPreamble.
		//
		Result->RealFunctionPostPreamble = FunctionToHookAddress + PreambleSize;
		Result->FunctionToHookAddress    = FunctionToHookAddress;
		Result->HookFunctionAddress      = HookFunctionAddress;
		Result->Target                   = (ULONG_PTR)Result->RealFunction;
		Result->RealFunction             = JmpSlotBuffer;

		//
		// Initialize the relative jump to after the function's preamble.
		//
		JmpSlotBuffer[PreambleSize]     = 0xff;
		JmpSlotBuffer[PreambleSize + 1] = 0x25;
		*(PULONG_PTR)(JmpSlotBuffer + PreambleSize + 2) = (ULONG_PTR)&Result->RealFunctionPostPreamble;

		//
		// Preserve the first n bytes of the function's preamble since we're about
		// to overwrite them.
		//
		RtlCopyMemory(
				Result->SavePreamble,
				(PVOID)FunctionToHookAddress,
				6);

		//
		// Set up the indirect jump to overwrite the preamble with.
		//
		Overwrite[0] = 0xff;
		Overwrite[1] = 0x25;
		*(PULONG_PTR)(Overwrite + 2) = (ULONG_PTR)(&Result->HookFunctionAddress);

		//
		// Probe & lock the pages
		//
		if (!(Result->Mdl = MmCreateMdl(
				NULL,
				(PVOID)FunctionToHookAddress, 
				6)))
		{
			DebugPrint(("MmCreateMdl failed."));
			break;
		}

		MmBuildMdlForNonPagedPool(Result->Mdl);

		__try
		{
			if (!(Result->LockedAddress = MmMapLockedPages(
					Result->Mdl, 
					KernelMode)))
			{
				IoFreeMdl(Result->Mdl);

				DebugPrint(("MmMapLockedPages failed."));
				break;
			}
		} __except(EXCEPTION_EXECUTE_HANDLER)
		{
			Status = GetExceptionCode();
			break;
		}

		//
		// Copy over the preamble
		//
		RtlCopyMemory(
				Result->LockedAddress,
				Overwrite,
				6);

		DebugPrint(("InstallCallLayer(): Call Layer installed at %.8x (Real at %.8x).", 
				Result,
				Result->RealFunction));

		*Layer = Result;
		Status = STATUS_SUCCESS;

	} while (0);

	//
	// Cleanup on failure
	//
	if (!NT_SUCCESS(Status))
	{
		if (JmpSlotBuffer)
			ExFreePool(JmpSlotBuffer);

		if (Result)
		{
			if (Result->Mdl)
				IoFreeMdl(Result->Mdl);

			ExFreePool(Result);
		}
	}

	return Status;
}

//
// Uninstalls a previously installed call layer
//
NTSTATUS UninstallCallLayer(
		IN PCALL_LAYER Layer)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	do
	{
		if (!Layer)
			break;

		//
		// Restore the first 6 bytes of the preamble
		//
		RtlCopyMemory(
				(PVOID)Layer->LockedAddress,
				Layer->SavePreamble,
				6);

		//
		// Unmap the locked pages and free the Mdl
		//
		MmUnmapLockedPages(
				Layer->LockedAddress, 
				Layer->Mdl);

		IoFreeMdl(
				Layer->Mdl);
	
		DebugPrint(("Successfully removed call Layer at %.8x.", 
				Layer));

		//
		// Deallocate the Buffers
		//
		ExFreePool(
				Layer->RealFunction);
		ExFreePool(
				Layer);

		Status = STATUS_SUCCESS;

	} while (0);

	return Status;
}

//
// Returns the pointer to the kernel mode system call handler
//
NTSTATUS GetSystemCallRoutine(
      IN PVOID SystemCallToHook OPTIONAL,
      IN ULONG SystemCallIndex OPTIONAL,
      OUT PULONG_PTR SystemCallAddress)
{
	NTSTATUS Status;
	PULONG   SystemCallTable;

	//
	// If a system call virtual address was provided, use it in place of the
	// index
	//
	if (SystemCallToHook)
		SystemCallIndex = *(PULONG)((PCHAR)SystemCallToHook+1);
		
	//
	// Make sure we aren't supplied with an index that exceeds the maximum
	//
	if (SystemCallIndex < (ULONG)KeServiceDescriptorTable[2])
	{  
		SystemCallTable    = KeServiceDescriptorTable[0];
		*SystemCallAddress = SystemCallTable[SystemCallIndex];
		Status             = STATUS_SUCCESS;
	}
	else
	{  
		*SystemCallAddress = 0;
		Status             = STATUS_NOT_FOUND;
	}
		
	return Status;
}

