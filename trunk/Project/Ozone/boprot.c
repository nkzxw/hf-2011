/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		boport.c
 *
 * Abstract:
 *
 *		This module implements buffer overflow protection related routines.
 *		Specifically, kernel32.dll randomization. The rest of buffer overflow
 *		code is in process.c
 *
 * Author:
 *
 *		Eugene Tsyrklevich 08-Jun-2004
 *
 * Revision History:
 *
 *		None.
 */


#include "boprot.h"
#include "hookproc.h"
#include "i386.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitBufferOverflowProtection)
#endif


ULONG	Kernel32Offset = 0, User32Offset = 0;


/*
 * InitBufferOverflowProtection()
 *
 * Description:
 *		.
 *
 *		NOTE: Called once during driver initialization (DriverEntry()).
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
InitBufferOverflowProtection()
{
	ULONG		addr;


	if (NTDLL_Base == NULL)
		return FALSE;


	__try
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("searching for kernel32.dll (%x)\n", NTDLL_Base));
		for (addr = (ULONG) NTDLL_Base; addr < 0x77ff9fff; addr++)
		{
			if (_wcsnicmp((PWSTR) addr, L"kernel32.dll", 12) == 0)
			{
				LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("InitBufferOverflowProtection: found kernel32.dll string at offset %x\n", addr));
				Kernel32Offset = addr;
				if (User32Offset)
					break;
			}

			if (_wcsnicmp((PWSTR) addr, L"user32.dll", 12) == 0)
			{
				LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("InitBufferOverflowProtection: found user32.dll string at offset %x\n", addr));
				User32Offset = addr;
				if (Kernel32Offset)
					break;
			}
		}

		/* kernel32.dll and user32.dll strings are supposed to follow each other */
		if (!Kernel32Offset || !User32Offset || (abs(User32Offset - Kernel32Offset) > 32))
		{
			LOG(LOG_SS_MISC, LOG_PRIORITY_WARNING, ("InitBufferOverflowProtection: incorrect kernel32.dll (%x) and user32.dll (%x) offsets\n", Kernel32Offset, User32Offset));
			Kernel32Offset = 0;
			User32Offset = 0;
			return FALSE;
		}

		if (Kernel32Offset)
		{
//XXX convert to use Mdl routines
			INTERRUPTS_OFF();
			MEMORY_PROTECTION_OFF();

			/* overwrite the first WCHAR of L"kernel32.dll" string with a zero */
			* (PWCHAR) Kernel32Offset = 0;

			MEMORY_PROTECTION_ON();
			INTERRUPTS_ON();
		}
#if 0
		if (User32Offset)
		{
			INTERRUPTS_OFF();
			MEMORY_PROTECTION_OFF();

			* (PWCHAR) User32Offset = 0;

			MEMORY_PROTECTION_ON();
			INTERRUPTS_ON();
		}
#endif

	} // __try

	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		NTSTATUS status = GetExceptionCode();
		LOG(LOG_SS_MISC, LOG_PRIORITY_WARNING, ("InitBufferOverflowProtection: caught an exception. status = 0x%x\n", status));

		return FALSE;
	}


	return TRUE;
}



/*
 * ShutdownBufferOverflowProtection()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Nothing.
 */

VOID
ShutdownBufferOverflowProtection()
{
	if (Kernel32Offset)
	{
		INTERRUPTS_OFF();
		MEMORY_PROTECTION_OFF();

		/* restore the first WCHAR of L"kernel32.dll" string */
		* (PWCHAR) Kernel32Offset = L'k';

		MEMORY_PROTECTION_ON();
		INTERRUPTS_ON();
	}
#if 0
	if (User32Offset)
	{
		INTERRUPTS_OFF();
		MEMORY_PROTECTION_OFF();

		* (PWCHAR) User32Offset = L'u';

		MEMORY_PROTECTION_ON();
		INTERRUPTS_ON();
	}
#endif
}
