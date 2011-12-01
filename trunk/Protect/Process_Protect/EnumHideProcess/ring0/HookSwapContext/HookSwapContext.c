#include <ntddk.h>

#include "HookSwapContext.h"

ULONG pOldSwapContextAddr = 0;
ULONG OrigSwapContext = 0;
ULONG pBackAddr = 0;

PEPROCESS pEProcess[100] = {NULL};
ULONG count = 0;

NTSTATUS DriverEntry( PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath )
{
	NTSTATUS Status = STATUS_SUCCESS;
	pDriverObject->DriverUnload = UnLoad;

	DisplayInfo();

	return Status;
}

_declspec(naked) void MySwapContext()
{
	_asm
	{
		pushad
			pushfd
			cli
	}
	_asm
	{
		push esi
			push edi
			call ShowProcess
	}
	
	_asm
	{
		sti
			popfd
			popad	
	}
	_asm jmp DWORD PTR[pBackAddr]
		
}

void DisplayInfo()
{
	ULONG pJmpAddr = 0;

	pOldSwapContextAddr = GetSwapContextAddr();
	if( pOldSwapContextAddr == 0 )
		return;
	OrigSwapContext = *(PULONG)( pOldSwapContextAddr + 1 );
	pBackAddr = pOldSwapContextAddr + 5 + OrigSwapContext;
	_asm
	{
		cli
		push eax
		mov eax,cr0
		and eax,not 10000B
		mov cr0,eax
		pop eax
		
	}
	*(PULONG)(pOldSwapContextAddr + 1 ) = (ULONG)MySwapContext - pOldSwapContextAddr - 5;
	_asm
	{
		push eax
		mov eax,cr0
		or eax,10000B
		mov cr0,eax
		pop eax
		sti
	}


}

ULONG GetSwapContextAddr()
{
	PETHREAD pEThread = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	Status = PsLookupThreadByThreadId( 8, &pEThread );
	if( !NT_SUCCESS(Status) )
		return 0;
	if( MmIsAddressValid( (PULONG)((ULONG)pEThread+0x28) ) )
	{
		ULONG pCurrentStack = *(PULONG)((ULONG)pEThread + 0x28);
		return *(PULONG)(pCurrentStack+0x8) - 5;
	}
	else
		return 0;
}



void ShowProcess( PETHREAD pOldThread, PETHREAD pNewThread )
{
	ULONG index = 0;
	if( MmIsAddressValid( (PULONG)((ULONG)pNewThread+0x220) ) )
	{
		PEPROCESS pAddr = (PEPROCESS)(*(PULONG)((ULONG)pNewThread+0x220 ));
		if( MmIsAddressValid( pAddr ) )
		{
			for( index = 0; index < count; index++ )
			{
				if( pEProcess[index] == pAddr )
					break;
			}
			if( index == count )
			pEProcess[count++] = pAddr;
		}
		

	}
}

void UnLoad( PDRIVER_OBJECT pDriverobject )
{
	ULONG index = 0;
	_asm
	{
		cli
		push eax
		mov eax,cr0
		and eax,not 10000B
		mov cr0,eax
		pop eax
	}
	*(PULONG)(pOldSwapContextAddr+1) = OrigSwapContext;
	_asm
	{
		push eax
		mov eax,cr0
		or eax,10000B
		mov cr0,eax
		pop eax
		sti
	}

	for( index = 0; index < count; index++ )
	{
		ULONG pAddr = (ULONG)pEProcess[index];
		if( pAddr == 0 )
		{
			DbgPrint("PId:%d\tPath:%s\n", 0, "Idle" );
		}
		else
		{
			DbgPrint("PId:%d\tPath:%s\n", *(PULONG)(pAddr+0x84), (PUCHAR)(pAddr+0x174) );
		}
	}
}