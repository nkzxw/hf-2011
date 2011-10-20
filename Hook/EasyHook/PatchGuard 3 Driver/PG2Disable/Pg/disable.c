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
*************************************** PgDisablePatchGuard
*************************************************************************

Description:

	If we would have direct access to non-exported kernel symbols,
	the following will be all we need to disable PatchGuard 2...
*/
BOOLEAN PgDisablePatchGuard(PDEVICE_OBJECT InDevice)
{
	KIRQL					OldIrql;
	ULONG					Index;
	PKSPIN_LOCK_QUEUE		LockQueue;
	PKTIMER_TABLE_ENTRY		TimerListHead;
	PLIST_ENTRY				TimerList;
	PKTIMER					Timer;
	PKDPC					TimerDpc;

	/*
		Lock the dispatcher database and loop through the timer list...

		We will cancel all timers that have a non-canonical DeferredContext.
	*/
	OldIrql = KiAcquireDispatcherLockRaiseToSynch();

	for(Index = 0; Index < TIMER_TABLE_SIZE; Index++)
	{
		LockQueue = KeTimerIndexToLockQueue((UCHAR)(Index & 0xFF));

		KeAcquireQueuedSpinLockAtDpcLevel(LockQueue);
		
		// now we can work with the timer list...
		TimerListHead = &KiTimerTableListHead[Index];
		TimerList = TimerListHead->Entry.Flink;

		while(TimerList != (PLIST_ENTRY)TimerListHead)
		{
			// is DPC patched?
			Timer = CONTAINING_RECORD(TimerList, KTIMER, TimerListEntry);
			TimerList = TimerList->Flink;
			TimerDpc = PgDeobfuscateTimerDpc(Timer);

			if(TimerDpc == NULL)
				continue;

			if(PgIsPatchGuardContext(TimerDpc->DeferredContext) && KeContainsSymbol(TimerDpc->DeferredRoutine))
			{
				// this will cancel the timer...
				Timer->Header.Inserted = FALSE;

				if(RemoveEntryList(&Timer->TimerListEntry))
					TimerListHead->Time.HighPart = 0xFFFFFFFF;
			}
		}

		KeReleaseQueuedSpinLockFromDpcLevel(LockQueue);
	}

	KiReleaseDispatcherLockFromSynchLevel();

	KiExitDispatcher(OldIrql);


	return TRUE;
}