 /*++

	This is the part of NGdbg kernel debugger

	base.cpp

	Contains routines that calculate image base.

--*/

#include <ntifs.h>
#include "winnt.h"

extern "C"
{

BOOLEAN
FindBaseAndSize(
	IN PVOID SomePtr,
	OUT PVOID *BaseAddress OPTIONAL, 
	OUT ULONG *ImageSize OPTIONAL
	)
{
	ULONG SomeAddress = (ULONG) SomePtr;

	for ( SomeAddress &= 0xFFFFF000 ; ; SomeAddress -= PAGE_SIZE )
	{
		if(MmIsAddressValid ((PVOID)SomeAddress) && *(USHORT*)SomeAddress == IMAGE_DOS_SIGNATURE ) // MZ signature?
		{
			PVOID NtHeader = RtlImageNtHeader ((PVOID)SomeAddress);// + ((IMAGE_DOS_HEADER*)SomeAddress)->e_lfanew;

			if (MmIsAddressValid ((PVOID)NtHeader) && *(ULONG*)NtHeader == IMAGE_NT_SIGNATURE) // PE signature?
			{
				if (ARGUMENT_PRESENT (BaseAddress))
					*BaseAddress = (PVOID)SomeAddress;

				if (ARGUMENT_PRESENT (ImageSize))
					*ImageSize = ((IMAGE_NT_HEADERS*)NtHeader)->OptionalHeader.SizeOfImage;

				return TRUE;
			}
		}
	}

	return FALSE;
}

PVOID
FindSelfBase(
	)
/**	
	Find our module's base address and size
*/
{
	PVOID SomeAddress;
	PVOID g_SelfBase;
	__asm
	{
		call _1
_1:		pop [SomeAddress]
	}

	if (FindBaseAndSize (SomeAddress, &g_SelfBase, NULL))
		return g_SelfBase;
	return NULL;
}


}