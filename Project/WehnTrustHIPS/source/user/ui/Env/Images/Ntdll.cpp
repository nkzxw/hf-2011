/*
 * WenTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Precomp.h"

static struct 
{
	LPCTSTR SymbolName;
	BOOLEAN Required;
} Exports[] =
{
	{ TEXT("KiUserApcDispatcher"),            TRUE,   },
	{ TEXT("KiUserCallbackDispatcher"),       TRUE,   },
	{ TEXT("KiUserExceptionDispatcher"),      TRUE,   },
	{ TEXT("KiRaiseUserExceptionDispatcher"), TRUE,   },
	{ TEXT("KiFastSystemCall"),               FALSE,  },
	{ TEXT("KiFastSystemCallRet"),            FALSE,  },
	{ TEXT("KiIntSystemCall"),                FALSE,  },
	{ TEXT("DbgUiRemoteBreakin"),             FALSE,  },
	{ NULL,                                   FALSE,  },
};

Ntdll::Ntdll()
: Image(TEXT("NTDLL.DLL"))
{
}

Ntdll::~Ntdll()
{
}

//
// The NTDLL synchronize routine is responsible for building out the address
// table for the following symbols:
//
// 2000/XP/2003:
//
//   - KiUserApcDispatcher
//   - KiUserCallbackDispatcher
//   - KiUserExceptionDispatcher
//   - KiRaiseUserExceptionDispatcher
//   - LdrInitializeThunk
//
// XP/2003:
//
//   - DbgUiRemoteBreakin
//
// All of these symbols are exported.
//
DWORD Ntdll::Synchronize()
{
	PVOID VirtualAddress;
	DWORD Result;
	DWORD Index;

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
		// Start resolving symbols
		//
		for (Index = 0;
		     Exports[Index].SymbolName;
		     Index++)
		{
			//
			// If the symbol's virtual address cannot be resolved and it is a
			// required symbol, abort the synchronization.  Otherwise, continue on.
			//
			if ((!(VirtualAddress = GetProcAddress(
					Exports[Index].SymbolName))))
			{
				if (Exports[Index].Required)
				{
					Result = ERROR_INVALID_FUNCTION;
					break;
				}
				else
					continue;
			}

			//
			// Add the address table entry for the image
			//
			if ((Result = AddAddressTableEntry(
					VirtualAddress,
					Exports[Index].SymbolName)) != ERROR_SUCCESS)
				break;
		}

		//
		// If something failed in the above loop, abort.
		//
		if (Result != ERROR_SUCCESS)
			break;

		//
		// Now do special resolution for LdrInitializeThunk as it requires custom
		// prepend data
		//
		if ((VirtualAddress = GetProcAddress(
				TEXT("LdrInitializeThunk"))))
		{
			BYTE Prepend[8];

			// mov [esp + 8], ModuleBase
			*(LPDWORD)Prepend     = 0x082444c7;
			*(LPDWORD)(Prepend+4) = (DWORD)ImageModule;

			if ((Result = AddAddressTableEntry(
					VirtualAddress,
					TEXT("LdrInitializeThunk"),
					Prepend,
					sizeof(Prepend))) != ERROR_SUCCESS)
				break;
		}
		else
		{
			Result = ERROR_INVALID_FUNCTION;
			break;
		}

	} while (0);

	//
	// If the operation was successful, call the base class' Synchronize method
	// to continue with the synchronization process, otherwise simply return out
	//
	return (Result == ERROR_SUCCESS)
		? Image::Synchronize()
		: Result;
}
