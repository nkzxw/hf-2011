/*
Released under MIT License

Copyright (c) 2008 by Christoph Husse, SecurityRevolutions e.K.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial 
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Visit http://www.codeplex.com/easyhook for more information.
*/
#include "EasyHookDriver.h"

#ifndef _M_X64
	#error "This driver part is intended for 64-bit builds only."
#endif

/************************************************************************
*************************************** PgDeobfuscateTimerDpcEx
*************************************************************************

Description:

	Deobfuscates the given deferred context, according to PatchGuard 2/3.
	The keys used for decryption shall be passed as second and third
	parameter.

Disassembly:

	RBX: InTimer
	EDI: InPeriod
	R11: InTimerDpc

	nt!KeSetTimerEx+0x8c:
		mov     rax,qword ptr [nt!KiWaitNever (fffff800`019f90c8)]
		mov     rdx,qword ptr [nt!KiWaitAlways (fffff800`019f9168)]
		mov     dword ptr [rbx+38h],edi				; InTimer->Period = InPeriod;
		xor     rdx,r11								; RDX = InTimerDpc ^ [KiWaitAlways];
		mov     ecx,eax								; DWORD ECX = (DWORD)([KiWaitNever] & 0xFFFFFFFF);
		mov     byte ptr [rbx+1],0					; unrelated
		bswap   rdx									; RDX = byteswap64(RDX);
		xor     rdx,rbx								; RDX ^= InTimer;
		ror     rdx,cl								; RDX = rotate_right64(RDX, [KiWaitNever] & 0xFF)
		xor     rdx,rax								; RDX ^= [KiWaitNever]
		mov     rax,r10								; unrelated
		shr     rax,20h								; unrelated
		mov     qword ptr [rbx+30h],rdx				; InTimer->Dpc = RDX
		test    eax,eax								; unrelated
*/
KDPC* PgDeobfuscateTimerDpcEx(
			IN PKTIMER InTimer,
			IN ULONGLONG InKiWaitAlways,
			IN ULONGLONG InKiWaitNever)
{
	ULONGLONG			RDX = (ULONGLONG)InTimer->Dpc;

	RDX ^= InKiWaitNever;
	RDX = _rotl64(RDX, (UCHAR)(InKiWaitNever & 0xFF));
	RDX ^= (ULONGLONG)InTimer;
	RDX = _byteswap_uint64(RDX);
	RDX ^= InKiWaitAlways;

	return (KDPC*)RDX;
}

/************************************************************************
*************************************** PgDeobfuscateTimerDpc
*************************************************************************

Description:

	Deobfuscates the given deferred context, according to PatchGuard 2/3.
*/
KDPC* PgDeobfuscateTimerDpc(IN PKTIMER InTimer) 
{ 
	return PgDeobfuscateTimerDpcEx(InTimer, KiWaitAlways, KiWaitNever); 
}

/************************************************************************
*************************************** PgInstallTestHook
*************************************************************************

Description:

	Based on the work invested in KeCancelTimer_Showcase(), we can now
	provide an easy test hook to see whether patchguard has really been
	disabled, or is there at all...

	Please note that this will cause a bluescreen if PatchGuard is still
	enabled...

*/
void PgInstallTestHook()
{
	KTIMER					MyTimer;
	UCHAR					PatchCode[20] =
	{
		0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, KeCancelTimer_Showcase()
		0xFF, 0xE0, // jmp rax
	};

	*((ULONGLONG*)(PatchCode + 2)) = KeCancelTimer_Showcase;
	
	KeInitializeTimer(&MyTimer);

	memcpy(KeCancelTimer, PatchCode, 12);

	KeCancelTimer(&MyTimer);
}

/*******************************************************************
****************************************** KeCancelTimer
********************************************************************

Source code info:
	After some debugging and disassembling I was able to decompile
	this compact and sweet method. With not that much work involved we
	get many information about the timer and dispatcher internals
	we wouldn't find anywhere else in the net. Even googling for most
	of the explored APIs will provide no useable results!

	ATTENTION: This is no (in)offical windows source code, but was
	reverse engineered only!

	Now we are able to properly walk interlocked through the timer table and scan
	for PatchGuard timers...
*/
BOOLEAN KeCancelTimer_Showcase(PKTIMER InTimer)
{
	// push    rbx
	// sub     rsp,20h
	KIRQL					OldIrql = 0;
	BOOLEAN					Existed = FALSE;
	PKSPIN_LOCK_QUEUE		LockArray = NULL;
	ULONG					LockIndex = 0;
	KTIMER_TABLE_ENTRY*		TimerEntry = NULL;

	// mov     r9,rcx
	// call    nt!KiAcquireDispatcherLockRaiseToSynch (fffff800`01c59940)
	OldIrql = KiAcquireDispatcherLockRaiseToSynch();

	// mov     bl,byte ptr [r9+3]		; in fact R9 will stay unchanged in any case...
	// test    bl,bl
	// mov     r10b,al					; OldIrql
	Existed = InTimer->Header.Inserted;

	// je      nt!KeCancelTimer+0x76 (fffff800`01c9c176)
	if(Existed)
	{
	// nt!KeCancelTimer+0x19:
		// mov     rcx,qword ptr gs:[28h]
		LockArray = KeGetPcr()->LockArray;

		// movzx   r8d, byte ptr [r9+2]
		// mov     eax,r8d
		// shr     eax,4
		// and     eax,0Fh
		// add     eax,11h

		LockIndex = ((InTimer->Header.Hand / sizeof(KSPIN_LOCK_QUEUE)) & 0x0F) + LockQueueTimerTableLock;

		// shl     rax,4			; multiply by sizeof(KSPIN_LOCK_QUEUE)
		// add     rcx,rax			; select pointer...
		// call    nt!KeAcquireQueuedSpinLockAtDpcLevel (fffff800`01c5c470)
		KeAcquireQueuedSpinLockAtDpcLevel(&LockArray[LockIndex]);

		// mov     byte ptr [r9+3],0
		InTimer->Header.Inserted = FALSE;

		// mov     rax,qword ptr [r9+28h]
		// mov     rdx,qword ptr [r9+20h]
		// cmp     rdx,rax			; belongs to the next IF statement...
		// mov     qword ptr [rax],rdx
		// mov     qword ptr [rdx+8],rax
		//jne     nt!KeCancelTimer+0x71 (fffff800`01c9c171)
		if(RemoveEntryList(&InTimer->TimerListEntry))
		{
		//nt!KeCancelTimer+0x58:
			// lea     rdx,[r8+r8*2]
			// lea     rax,[nt!KiTimerTableListHead (fffff800`01d50800)]
			// lea     r8,[rax+rdx*8]
			TimerEntry = &KiTimerTableListHead[InTimer->Header.Hand];

			// cmp     r8,qword ptr [r8]
			//jne     nt!KeCancelTimer+0x71 (fffff800`01c9c171)
			if(TimerEntry == (KTIMER_TABLE_ENTRY*)TimerEntry->Entry.Flink)
			{
			//nt!KeCancelTimer+0x6c:
				// or      dword ptr [r8+14h],0FFFFFFFFh
				TimerEntry->Time.HighPart = 0xFFFFFFFF;
			}
		}

	//nt!KeCancelTimer+0x71:
		//call    nt!KeReleaseQueuedSpinLockFromDpcLevel (fffff800`01c46300)
		KeReleaseQueuedSpinLockFromDpcLevel(&LockArray[LockIndex]);
	}
//nt!KeCancelTimer+0x76:
	//call    nt!KiReleaseDispatcherLockFromSynchLevel (fffff800`01c482f0)
	KiReleaseDispatcherLockFromSynchLevel();

	// mov     cl,r10b
	// call    nt!KiExitDispatcher (fffff800`01c5a950)
	KiExitDispatcher(OldIrql);

	// mov     al,bl
	return Existed;

	// add     rsp,20h
	// pop     rbx
	// ret
}

/*************************************************************************
************************************** KeTimerIndexToLockQueue()
**************************************************************************

Description:
	This method simply computes the lock entry for a given timer list index.

Disassembly:

	EBX: The timer index, i.e. the "Hand" value

	nt!KeCheckForTimer+0x42:
		mov     r8,qword ptr gs:[28h]
		mov     ecx,ebx
		shr     ecx,4
		and     ecx,0Fh
		add     ecx,11h
		shl     rcx,4
		add     r8,rcx
		mov     rcx,r8
		call    nt!KeAcquireQueuedSpinLockAtDpcLevel (fffff800`0185cab0)
*/
PKSPIN_LOCK_QUEUE KeTimerIndexToLockQueue(UCHAR InTimerIndex)
{
	return &(KeGetPcr()->LockArray[((InTimerIndex / sizeof(KSPIN_LOCK_QUEUE)) & 0x0F) + LockQueueTimerTableLock]);
}
