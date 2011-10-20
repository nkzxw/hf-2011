/*
 * WehnTrust
 *
 * SEH overflow detection code.
 *
 * Copyright (c) 2005, Wehnus.
 */
#include "NRER.h"

static VOID FlagValidationFrameInstalled();
static BOOLEAN IsValidationFrameInstalled();
static EXCEPTION_DISPOSITION SehValidationHandler(
		IN PEXCEPTION_RECORD ExceptionRecord,
		IN PVOID ExceptionFrame,
		IN PCONTEXT Context,
		IN PVOID DispatcherContext);

////
//
// Globals
//
////

//
// This global variable is the SEH validation frame that is used to ensure that
// the chain has not been invalidated.
//
static EXCEPTION_REGISTRATION_RECORD SehValidationFrame = 
{
	(PEXCEPTION_REGISTRATION_RECORD)-1,
	SehValidationHandler
};

#pragma warning(disable: 4733)

//
// This function inserts the SEH validation frame into the exception handler
// list for the calling thread.
//
ULONG InstallSehValidationFrame()
{
	PEXCEPTION_REGISTRATION_RECORD Current;
	BOOLEAN                        AlreadyInstalled = FALSE;
	ULONG                          Result = ERROR_SUCCESS;

	//
	// Get the first exception handler from NtTib->ExceptionList
	//
	__asm
	{
		mov eax, fs:[0]
		mov [Current], eax
	}

	//
	// Make sure there's at least one exception handler in the list
	//
	if ((Current) &&
	    (Current != (PEXCEPTION_REGISTRATION_RECORD)-1))
	{
		//
		// Walk the chain until we get to the last handler
		//
		while ((Current) &&
				 (Current->Next != (PEXCEPTION_REGISTRATION_RECORD)-1))
		{
			//
			// Sanity check to see if it's already installed
			//
			if (Current->Next == &SehValidationFrame)
			{
				AlreadyInstalled = TRUE;
				break;
			}

			Current = Current->Next;
		}

		// 
		// If the validation frame isn't already installed for this thread,
		// install it now as the last frame.
		//
		if (!AlreadyInstalled)
		{
			Current->Next = &SehValidationFrame;

			//
			// Flag the validation frame as having been installed
			//
	//		FlagValidationFrameInstalled();
		}
	}
	//
	// Otherwise, install ourself as the first exception handler.
	//
	else
	{
		__asm
		{
			lea eax, [SehValidationFrame]
			mov fs:[0], eax
		}
	}

	return Result;
}

//
// Checks to see whether or not the SEH chain is valid for the thread.
//
BOOLEAN IsSehChainValid(
		OUT PEXPLOIT_INFORMATION ExploitInformation)
{
	PEXCEPTION_REGISTRATION_RECORD Current;
	BOOLEAN                        ValidationHandlerFound = FALSE;
	BOOLEAN                        InvalidNextPointer = FALSE;
	PVOID                          InvalidSehFrameHandler = NULL;
	PVOID                          InvalidSehFrameNext = NULL;

	// 
	// Make sure that SEH protection is initialized for this thread.
	// If it isn't, bail out and don't attempt to validate.
	//
//	if (!IsValidationFrameInstalled())
//		return TRUE;

	// 
	// Grab the first handler in the chain
	//
	__asm
	{
		mov eax, fs:[0]
		mov [Current], eax
	}

	//
	// Walk the chain validating each of the Next pointers until we
	// either reach our validation handler or realize that something
	// bogus is happening
	//
	while (Current != (PEXCEPTION_REGISTRATION_RECORD)-1)
	{
		//
		// If there is a seemingly valid chain following this one
		//
		if (Current->Next != (PEXCEPTION_REGISTRATION_RECORD)-1)
		{
			MEMORY_BASIC_INFORMATION BasicInformation;

			//
			// If we either fail to acquire information about the region
			// or determine it to be a free region, then it's obviously
			// bogus.
			//
			if ((NreQueryVirtualMemory(
					Current->Next,
					MemoryBasicInformation,
					(PVOID)&BasicInformation,
					sizeof(BasicInformation),
					NULL) != 0) ||
			    (BasicInformation.State == MEM_FREE))
			{
				InvalidNextPointer = TRUE;

				//
				// This handler becomes our invalid target handler
				//
				InvalidSehFrameHandler = Current->Handler;
				InvalidSehFrameNext    = Current->Next;

				break;
			}
		}

		//
		// Check to see if this registration record is our validation
		// handler
		//
		if (Current == &SehValidationFrame)
		{
			ValidationHandlerFound = TRUE;
			break;
		}

		//
		// For tracking purposes, always use the first handler as the
		// invalid target handler for notification to the callback
		//
		if (!InvalidSehFrameHandler)
			InvalidSehFrameHandler = Current->Handler;
		if (!InvalidSehFrameNext)
			InvalidSehFrameNext = Current->Next;

		//
		// Move on to the next one
		//
		Current = Current->Next;
	}

	//
	// If the validation handler was not found and we were passed an exploit
	// information structure, initialize.
	//
	if ((!ValidationHandlerFound) &&
	    (ExploitInformation))
	{
		ULONG_PTR Next = (ULONG_PTR)InvalidSehFrameNext;

		//
		// Set the 
		//
		ExploitInformation->Type = SehOverwrite;

		//
		// Initialize the SEH overwrite-specific information.
		//
		ExploitInformation->Seh.InvalidFrameHandler     = InvalidSehFrameHandler;
		ExploitInformation->Seh.InvalidFrameNextPointer = InvalidSehFrameNext;

		//
		// Do some lame logic to figure out whether or not the next instruction
		// contains a short jump or not.
		//
		if (((Next & 0x0000ff) == 0x0000eb) ||
		    ((Next & 0x00ff00) == 0x00eb00) ||
		    ((Next & 0xff0000) == 0xeb0000))
			ExploitInformation->Seh.NextContainsShortJumpInstruction = TRUE;
		else
			ExploitInformation->Seh.NextContainsShortJumpInstruction = FALSE;
	}

	//
	// Return whether or not the validation handler was found.
	//
	return ValidationHandlerFound;
}

//
// Marks the calling thread as having had the SEH validation frame installed.
//
static VOID FlagValidationFrameInstalled()
{
	__asm
	{
		mov eax, fs:[0x18]
		mov byte ptr [eax + 0xffb], 0x1
	}
}

//
// Checks to see whether or not the calling thread has been marked as having its
// validation frame installed
//
static BOOLEAN IsValidationFrameInstalled()
{
	BOOLEAN Installed = FALSE;

	__asm
	{
		mov eax, fs:[0x18]
		cmp byte ptr [eax + 0xffb], 0x1
		jnz NotInstalled
		mov byte ptr [Installed], 0x1
	NotInstalled:
	}

	return Installed;
}

//
// This is the stub routine for the SEH validation frame that simply passes
// along any exceptions that come through it.
//
static EXCEPTION_DISPOSITION SehValidationHandler(
		IN PEXCEPTION_RECORD ExceptionRecord,
		IN PVOID ExceptionFrame,
		IN PCONTEXT Context,
		IN PVOID DispatcherContext)
{
	return ExceptionContinueSearch;
}
