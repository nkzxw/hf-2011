/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		i386.c
 *
 * Abstract:
 *
 *		This module implements various x86 processor dependant routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 07-Apr-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include "hookproc.h"
#include "i386.h"
#include "misc.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitI386)
#endif


ULONG			SystemAddressStart;
static ULONG	SharedUserDataAddress, TssAddress;
ULONG			MajorVersion, MinorVersion;


/*
 * InitI386()
 *
 * Description:
 *		Verify that we are running on Win2k, XP or 2003 on x86 platform.
 *		Also initialize various i386 related variables. Since Windows Advanced & Datacenter editions
 *		support a boot-time option that allows 3-GB user address spaces we cannot rely
 *		on static addresses for predefined structures.
 *
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Nothing.
 */

BOOLEAN
InitI386()
{
	CHAR				Gdtr[6];
	USHORT				TssOffset;
	PKGDTENTRY			TssGdtEntry;


	PsGetVersion(&MajorVersion, &MinorVersion, NULL, NULL);


	/*
	 * Right now we only support Windows 2000 (5.0), XP (5.1), 2003 (5.2)
	 */

	if (MajorVersion != 5 || MinorVersion > 2)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("InitI386: Version = %d.%d\n", MajorVersion, MinorVersion));
		return FALSE;
	}


	/*
	 * Find the Shared User Data address.
	 */

	if (* (PULONG) MmHighestUserAddress == 0x7FFEFFFF)
	{
		SharedUserDataAddress = 0x7FFE0000;
		SystemAddressStart = 0x80000000;
	}
	else if (* (PULONG) MmHighestUserAddress == 0xBFFEFFFF)
	{
		SharedUserDataAddress = 0xBFFE0000;
		SystemAddressStart = 0xC0000000;
	}
	else
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("InitI386: Unknown MmHighestUserAddress=%x\n", * (PULONG) MmHighestUserAddress));
		return FALSE;
	}


	/*
	 * Find the TSS address.
	 *
	 * STR - Stores the segment selector from the task register (TR) in the destination operand. The
	 * destination operand can be a general-purpose register or a memory location. The segment selector
	 * stored with this instruction points to the task state segment (TSS) for the currently running task.
	 *
	 * SGDT - Stores the content of the global descriptor table register (GDTR) in the destination operand.
	 * The destination operand specifies a 6-byte memory location.
	 */

	_asm
	{
		str		TssOffset
		sgdt	Gdtr
	}

	TssGdtEntry = (PKGDTENTRY) * (PULONG) (Gdtr + 2);		/* Extract the GDT address */
	(PCHAR) TssGdtEntry += TssOffset;

	TssAddress = TssGdtEntry->BaseLow | ( ((TssGdtEntry->HighWord.Bytes.BaseHi) << 24) | ((TssGdtEntry->HighWord.Bytes.BaseMid) << 16) );


	return TRUE;
}



/*
 * VerifyUserReturnAddress()
 *
 * Description:
 *		Verifies whether a specified userland return address is valid.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Nothing.
 */

VOID
VerifyUserReturnAddress()
{
	PKTSS		tss = (PKTSS) TssAddress;
	ULONG		UserEip, UserEsp;


	/*
	 * this feature is not supported on Windows 2000 as it can make
	 * system calls from anywhere, not just ntdll.dll (it uses int 0x2e instead of sysenter)
	 */
	if (MinorVersion == 0)
		return;

	if (KeGetPreviousMode() != UserMode)
		return;


	// EIP is 5 DWORDs before ESP0 on stack
#define EIP_OFFSET	5

	UserEip = * (PULONG) (tss->Esp0 - EIP_OFFSET * sizeof(DWORD));


#define STACK_POINTER_OFFSET	2

	UserEsp = * (PULONG) (tss->Esp0 - STACK_POINTER_OFFSET * sizeof(DWORD));
	UserEsp -= 4;


	//XXX verify that the return address is not on a writable page
	//(might be used with Win2K which can make calls from anywhere)

	if (UserEip < (ULONG) NTDLL_Base)
	{
		LOG(LOG_SS_MISC, LOG_PRIORITY_DEBUG, ("%d VerifyUserReturnAddress: Abnormal return user EIP=%x ESP0=%x (NTDLL_Base=%x)\n", (ULONG) PsGetCurrentProcessId(), UserEip, tss->Esp0, NTDLL_Base));

		LogAlert(ALERT_SS_BOPROT, OP_INVALIDCALL, ALERT_RULE_BOPROT_INVALIDCALL, ACTION_LOG, ALERT_PRIORITY_HIGH, NULL, 0, NULL);
	}
}
