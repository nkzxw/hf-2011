#include "splice.h"

ULONG IntDisableWP()
{
	__asm
	{
		mov eax, cr0
		mov ecx, eax
		and eax, 0xFFFEFFFF
		mov cr0, eax
		mov eax, ecx
	}
}

VOID IntRestoreWP(ULONG _Cr0)
{
	__asm
	{
		mov eax, [_Cr0]
		mov cr0, eax
	}
}

#define EmitJumpCommand( From, To ) \
		{ \
			*(UCHAR*)(From) = 0xE9; \
			*(ULONG*)((ULONG)(From)+1) = (ULONG)(To) - (ULONG)(From) - 5; \
		}

ULONG
SpliceFunctionStart(
	IN	PVOID	OriginalAddress,
    IN	PVOID	HookFunction,
	OUT	PVOID	SplicingBuffer,
	IN	ULONG	MaxLength,
	OUT	PVOID	BackupBuffer,
	OUT	PULONG	BytesWritten,
	IN	BOOLEAN	WorkAtCurrentIrql
	)
{
	ULONG cr0;
	KIRQL Irql;
	ULONG Len;
	ULONG Ptr, NextAddress;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
//	ULONG CapturedBytesWritten;

	KdPrint(("Entering SpliceFunctionStart( 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x )\n",
		OriginalAddress,
		HookFunction,
		SplicingBuffer,
		sizeof(MaxLength),
		BytesWritten));

	KdPrint(("Saving state\n"));

	// Save
	cr0 = IntDisableWP( ); // Disable Write-Protection on system pages
	if( !WorkAtCurrentIrql )
		KeRaiseIrql( HIGH_LEVEL, &Irql ); // Raise IRQL

	KdPrint(("cr0=0x%08x\n", cr0));

	//
    // Copy integer number of instructions to the buffer
	//

	KdPrint(("Copying instructions from OriginalAddress=%08x\n", OriginalAddress));
	*BytesWritten = 0;
	for( Ptr = (ULONG)OriginalAddress; Ptr < ((ULONG)OriginalAddress+5); Ptr+=Len )
	{
		Len = size_of_code( (UCHAR*)Ptr );

		KdPrint(("Command decoded at address 0x%08x, length 0x%08x\n", Ptr, Len));
		
		KdPrint(("Ptr=%08x,OriginalAddress=%08x, MaxLength=%08x, Ptr-OriginalAddress+5=%08x\n", Ptr, OriginalAddress, MaxLength, (Ptr-(ULONG)OriginalAddress+5)));

		if( Ptr < ((ULONG)OriginalAddress+5) )
		{
			if( (Ptr-(ULONG)OriginalAddress+5) >= MaxLength )
			{
				KdPrint(("Error: buffer is too small\n"));

				Status = STATUS_INFO_LENGTH_MISMATCH;
				goto _exit;
			}

			memcpy( (PVOID)((ULONG)SplicingBuffer+(Ptr-(ULONG)OriginalAddress)), (PVOID)Ptr, Len );
			*BytesWritten += Len;
		}
	}

	NextAddress = Ptr;
	KdPrint(("*BytesWritten = 0x%08x, Ptr = 0x%08x, NextAddress = 0x%08x\n", *BytesWritten, Ptr, NextAddress));

	/*
	// Fixup buffer commands
	KdPrint(("Fixing up instructions\n"));
	CapturedBytesWritten = *BytesWritten;
	for( Ptr = (ULONG)SplicingBuffer; Ptr < ((ULONG)SplicingBuffer+CapturedBytesWritten); Ptr+=Len )
	{
		BOOLEAN FixupPrevious;
		ULONG	PreviousFixupOffset, PreviousPreviousFixupOffset;

 		Len = size_of_code( (BYTE*)Ptr );

		FixupPrevious = DcpFixupCommand( (PVOID)Ptr, (ULONG)OriginalAddress+Ptr-(ULONG)SplicingBuffer, SplicingBuffer, FALSE, 0, 0, BytesWritten, &Len, &PreviousFixupOffset );
		if( FixupPrevious )
		{
			// Fixup all previous commands
			ULONG TempPtr, TempLen;

			for( TempPtr = (ULONG)SplicingBuffer; TempPtr < Ptr; TempPtr+=TempLen )
			{
				TempLen = size_of_code( (BYTE*)TempPtr );

				if( DcpFixupCommand( (PVOID)TempPtr, (ULONG)OriginalAddress+TempPtr-(ULONG)SplicingBuffer, SplicingBuffer, TRUE, PreviousFixupOffset, Ptr, BytesWritten, &TempLen, &PreviousPreviousFixupOffset) )
				{
					KdPrint(("Nested fixing up is not supported (nested depth >2, PreviousPreviousFixupOffset=%d)\n", PreviousPreviousFixupOffset));
					// Hard case. Break

					Status = STATUS_INVALID_PARAMETER;
					goto _exit;
				}
				KdPrint((" -> Temp command fixed up at address 0x%08x, length 0x%08x\n", TempPtr, TempLen));
			}
			KdPrint((" -> Finished fixing up previous commands\n"));
		}

		KdPrint(("Command fixed up at address 0x%08x, length 0x%08x\n", Ptr, Len));
	}
	*/

	KdPrint(("Generating splicing buffer\n"));

	//
	// Emit splicing jump to the buffer
	//

	memcpy( BackupBuffer, OriginalAddress, 5 );

	EmitJumpCommand( OriginalAddress, HookFunction );

	KdPrint(("Original address bytes: %02x %02x %02x %02x %02x\n",
		((PUCHAR)OriginalAddress)[0],
		((PUCHAR)OriginalAddress)[1],
		((PUCHAR)OriginalAddress)[2],
		((PUCHAR)OriginalAddress)[3],
		((PUCHAR)OriginalAddress)[4]
		));

	//
	// Emit continuation jump to the function continuation
	//

	Ptr = ((ULONG)SplicingBuffer+*BytesWritten);
	EmitJumpCommand( Ptr, NextAddress );

	Status = STATUS_SUCCESS;

_exit:

	// Restore
	KdPrint(("Irql = %x, cr0 = %x\n", Irql, cr0));
	
	if( !WorkAtCurrentIrql )
		KeLowerIrql( Irql ); // Lower IRQL
	IntRestoreWP( cr0 ); // Restore Write-Protection bit

	return Status;
}


ULONG
UnspliceFunctionStart(
	IN	PVOID	OriginalAddress,
	IN	PVOID	BackupBuffer,
	IN	BOOLEAN	WorkAtCurrentIrql
	)
{
	ULONG cr0;
	KIRQL Irql;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	
	// Save
	cr0 = IntDisableWP( ); // Disable Write-Protection on system pages
	if( !WorkAtCurrentIrql )
		KeRaiseIrql( HIGH_LEVEL, &Irql ); // Raise IRQL

	// Reset splicing
	__try
	{
		memcpy( OriginalAddress, BackupBuffer, 5 );
		Status = STATUS_SUCCESS;
	}
	__except(EXCEPTION_EXECUTE_HANDLER ){
		Status = GetExceptionCode();
	}

	if( !WorkAtCurrentIrql )
		KeLowerIrql( Irql ); // Lower IRQL
	IntRestoreWP( cr0 ); // Restore Write-Protection bit

	return Status;
}
