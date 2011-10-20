/*
 * WenTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Precomp.h"

User32::User32()
: Image(TEXT("USER32.DLL"))
{
}

User32::~User32()
{
}

//
// The synchronize routine for user32 is a bit more complicated than the other
// two simply because the method by which the addresses that require jump table
// entries is entirely undocumented.  The reason user32 needs a jump table is
// because it supplies win32k.sys with an array of callbacks that are used for
// handling notification messages on a per-session basis.  These callbacks are
// expected to be the same across processes (or so it seems) and thus a jump
// table must be built to handle the condition where it might not be.
//
DWORD User32::Synchronize()
{
	DWORD Result;

	do
	{
		//
		// Open the registry key and load the image into memory
		//
		if (((Result = Open()) != ERROR_SUCCESS) ||
		    ((Result = LoadLibrary()) != ERROR_SUCCESS))
			break;

		//
		// Flush the existing address table
		//
		FlushAddressTable();

		//
		// If something failed...bust out.
		//
		if (Result != ERROR_SUCCESS)
			break;

		//
		// Get the win32k entry table symbols that are passed to the win32k once
		// per-session.
		//
		if ((Result = GetWin32EntryTableSymbols()) != ERROR_SUCCESS)
			break;

		//
		// The USER32.DLL image mapping cannot be unloaded from an image set for
		// now.  This is to prevent having to keep track of the initial pfn array
		// setting logic that user32 goes through the first time it is loaded in a
		// session.
		//
		if ((Result = SetFlags(
				IMAGE_FLAG_CANNOT_UNLOAD)) != ERROR_SUCCESS)
			break;

	} while (0);

	//
	// If the operation was successful, call the base class' Synchronize method
	// to continue with the synchronization process, otherwise simply return out
	//
	return (Result == ERROR_SUCCESS)
		? Image::Synchronize()
		: Result;
}

////
//
// Protected methods
//
////

//
// This routine is responsible for locating the array of symbols that are passed
// to win32k.sys to be used as window messaging callbacks.
//
DWORD User32::GetWin32EntryTableSymbols()
{
#if 0
	INSTRUCTION Instruction;
	PULONG      DispatchArray = NULL;
	PBYTE       InitializeWin32EntryTable = NULL;
	DWORD       Result;
	DWORD       Index = 0;
	BOOL        WantRetn = FALSE;
	int         InstructionSize, TotalSize = 0;

	do
	{
		//
		// Get the address of InitializeWin32EntryTable
		//
		if (!(InitializeWin32EntryTable = (PBYTE)GetProcAddress(
				TEXT("InitializeWin32EntryTable"))))
		{
			Result = ERROR_NOT_FOUND;
			break;
		}

		//
		// Start disassembling the routine looking for this:
		//
		// movzx eax, ds:[pfndispatch]
		// retn
		//
		while (TotalSize < 512)
		{
			InstructionSize = get_instruction(
					&Instruction,
					InitializeWin32EntryTable,
					0,
					MODE_32,
					FORMAT_INTEL);

			//
			// Disassembly failure?
			//
			if (InstructionSize <= 0)
				break;

			InitializeWin32EntryTable += InstructionSize;
			TotalSize                 += InstructionSize;

			//
			// If a retn is desired and the current instruction eqauls it, we've
			// found our array.
			//
			if (WantRetn)
			{
				if (!strcmp(Instruction.mnemonic, "retn"))
					break;

				WantRetn = FALSE;
			}
			    
			//
			// If this isn't a movzx instruction, move along.
			//
			if (strcmp(Instruction.mnemonic, "movzx"))
				continue;

			DispatchArray = (PULONG)Instruction.op2.displacement;
			WantRetn      = TRUE;
		}

		//
		// If the dispatch array is NULL, we failed to find the array
		//
		if (!DispatchArray)
		{
			Result = ERROR_NOT_FOUND;
			break;
		}

		//
		// Enumerate through all of the entries in the dispatch array
		//
		for (Index = 1;
		     ((!IsBadReadPtr(
					(PVOID)DispatchArray[Index],
					sizeof(ULONG))) &&
		     (DispatchArray[Index] > (ULONG)ImageModule));
			  Index++)
		{
			if ((Result = AddAddressTableEntry(
					(PVOID)DispatchArray[Index])) != ERROR_SUCCESS)
				break;
		}

	} while (0);

	return Result;
#endif

	return 0;
}
