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

#define MAX_SYMBOL_COUNT	50

// these are the values that will by extracted by PgInitialize()
PROC_KiAcquireDispatcherLockRaiseToSynch*	KiAcquireDispatcherLockRaiseToSynch = NULL;
PROC_KeAcquireQueuedSpinLockAtDpcLevel*		KeAcquireQueuedSpinLockAtDpcLevel = NULL;
PROC_KeReleaseQueuedSpinLockFromDpcLevel*	KeReleaseQueuedSpinLockFromDpcLevel = NULL;
PROC_KiExitDispatcher*						KiExitDispatcher = NULL;
PROC_KiReleaseDispatcherLockFromSynchLevel*	KiReleaseDispatcherLockFromSynchLevel = NULL;
PKTIMER_TABLE_ENTRY							KiTimerTableListHead = NULL;
ULONGLONG									KiWaitAlways = 0;
ULONGLONG									KiWaitNever = 0;

/************************************************************************************
************************************** ExtractSymbolAddresses()
*************************************************************************************

Description:

	This method loops through the given entry point until it encounters
	a return instruction (0xC3). It writes at maximum MAX_SYMBOL_COUNT
	addresses into the given array. If the entry point contains more addresses,
	the method returns FALSE, TRUE otherwise.
*/
BOOLEAN ExtractSymbolAddresses(
	IN PVOID InEntryPoint,
	IN BOOLEAN IsTwoBytePrefix,
	IN SHORT InPrefix,
	OUT void** OutAddrArray)
{
	UCHAR*			CodePtr = (UCHAR*)InEntryPoint;
	ULONG			ArrayIndex = 0;

	memset(&OutAddrArray[0], 0, sizeof(OutAddrArray[0]) * MAX_SYMBOL_COUNT);

	while(TRUE)
	{
		int Length = GetInstructionLength_x64(CodePtr, 64);

		if(Length <= 0)
			return FALSE;

		if((!IsTwoBytePrefix && ((*CodePtr) == (InPrefix & 0xFF))) ||
			(IsTwoBytePrefix && ((*((SHORT*)CodePtr)) == InPrefix)))
		{
			// convert to absolute address
			if(ArrayIndex >= MAX_SYMBOL_COUNT)
				return FALSE;

			OutAddrArray[ArrayIndex++] = (void*)(CodePtr + Length + *((signed __int32*)(CodePtr + Length - 4)));
		}

		if((*CodePtr) == 0xC3)
			break;

		CodePtr += Length;
	}

	return TRUE;
}

/************************************************************************************
************************************** IntersectSymbolAddresses()
*************************************************************************************

Description:

	"OutIntersection" will contain all entries that are in
	"InArrayA" AND "InArrayB". The return value is the amount
	of stored entries.
*/
ULONG IntersectSymbolAddresses(
		IN void** InArrayA,
		IN void** InArrayB,
		OUT void** OutIntersection)
{
	ULONG				iSymbolA;
	ULONG				iSymbolB;
	ULONG				MatchCount;

	memset(&OutIntersection[0], 0, sizeof(OutIntersection[0]) * MAX_SYMBOL_COUNT);

	MatchCount = 0;

	for(iSymbolA = 0; iSymbolA < MAX_SYMBOL_COUNT; iSymbolA++)
	{
		if(InArrayA[iSymbolA] == NULL)
			continue;

		for(iSymbolB = 0; iSymbolB < MAX_SYMBOL_COUNT; iSymbolB++)
		{
			if(InArrayA[iSymbolA] == InArrayB[iSymbolB])
			{
				OutIntersection[MatchCount++] = InArrayA[iSymbolA];

				break;
			}
		}
	}

	return MatchCount;
}

/************************************************************************************
************************************** ValidateTimerTable()
*************************************************************************************

Description:

	Checks if the given symbol is a timer table. This validation is
	not accurate but better than nothing. Currently we won't need it at
	all, because KeCancelTimer() has only one symbol that matches
	our previous extraction parameters...
*/
BOOLEAN ValidateTimerTable(IN void* InSymbol)
{
	PKTIMER_TABLE_ENTRY		Table = (PKTIMER_TABLE_ENTRY)InSymbol;
	PKTIMER_TABLE_ENTRY		TableHead;
	ULONG					Index = 0;


	// the desired symbol must be in non-paged memory...
	if(!MmIsNonPagedSystemAddressValid(Table) || 
			!MmIsAddressValid(Table) || 
			!MmIsAddressValid(Table + TIMER_TABLE_SIZE))
		return FALSE;

	/*
		The following code does not cover all cases.
		It relies on the fact that there should always be one
		timer list that is empty. If this is not the case, we
		can't validate the pointer.

		Considering that the driver will usually be called during
		system start there is simply no chance to have no empty timer
		list which would incorrectly cause this method to return FALSE.

		Please note that we are dealing with non-pooled memory,
		so an exception handler is useless!
	*/
	for(Index = 0; Index < TIMER_TABLE_SIZE; Index++)
	{
		TableHead = &Table[Index];

		if(TableHead->Entry.Flink == (PLIST_ENTRY)TableHead)
			return TRUE;
	}

	return FALSE;
}

/************************************************************************************
************************************** OnTestTimerInvokation()
*************************************************************************************

Description:

	Just a dummy for the test timer...
*/
VOID OnTestTimerInvokation(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
{
}

/************************************************************************************
************************************** ProbeAndSetKeys()
*************************************************************************************

Description:

	This method will validate the given key pair and if
	they are correct, it will also globally set them.
*/
BOOLEAN ProbeAndSetKeys(
			IN PKTIMER InTestTimer,
			IN ULONGLONG* InWaitAlways,
			IN ULONGLONG* InWaitNever)
{
	PKDPC			TimerDpc;
	ULONGLONG		KeyAlways;
	ULONGLONG		KeyNever;

	if(!MmIsNonPagedSystemAddressValid(InWaitAlways) || !MmIsNonPagedSystemAddressValid(InWaitNever))
		return FALSE;

	/*
		If we once validated non-paged addresses, we can be sure that it stays
		available, because they are hard-coded into the machine code (in our case). It would be
		very unlikely that we get a valid pointer that can be freed. And even if so,
		there is nearly no chance that it will be released within such a small time frame.
	*/
	if(!MmIsAddressValid(InWaitAlways)) 
		return FALSE;

	KeyAlways = *InWaitAlways;

	if(!MmIsAddressValid(InWaitNever))
		return FALSE;

	KeyNever = *InWaitNever;

	/*
		It is nearly impossible that the following code block will produce
		incorrect results...
	*/
	TimerDpc = PgDeobfuscateTimerDpcEx(
		InTestTimer,
		KeyAlways,
		KeyNever);

	// stack is always non-paged.
	if(!MmIsNonPagedSystemAddressValid(TimerDpc) || !MmIsAddressValid(TimerDpc))
		return FALSE;

	if((TimerDpc->DeferredContext != PgInitialize) ||
			(TimerDpc->DeferredRoutine != OnTestTimerInvokation))
		return FALSE;

	// now we can safely set the deobfuscation keys...
	KiWaitAlways = KeyAlways;
	KiWaitNever = KeyNever;

	return TRUE;
}


/************************************************************************************
************************************** ExtractPatchGuardVector()
*************************************************************************************

Description:

	Will initialize the symbols required in order to disable PatchGuard.

*/
NTSTATUS PgInitialize()
{
	void*				SymbolArray[MAX_SYMBOL_COUNT];
	void*				ValidationArray[MAX_SYMBOL_COUNT];
	void*				IntersectionArray[MAX_SYMBOL_COUNT];
	ULONG				iSymbol;
	ULONG				Index;
	ULONG				MatchCount;
	KTIMER				TestTimer;
	KDPC				TestTimerDpc;
	LARGE_INTEGER		TimerDueTime = {0};

	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

	/*
		Extract and validate methods...
	*/
	if(!ExtractSymbolAddresses((void*)KeCancelTimer, FALSE, 0xE8, OUT SymbolArray) ||
			!ExtractSymbolAddresses((void*)KeSetTimerEx, FALSE, 0xE8, OUT ValidationArray))
		return STATUS_NOT_SUPPORTED;
	
	if(IntersectSymbolAddresses(SymbolArray, ValidationArray, OUT IntersectionArray) != 5)
		return STATUS_NOT_SUPPORTED;

	KiAcquireDispatcherLockRaiseToSynch = IntersectionArray[0];
	KeAcquireQueuedSpinLockAtDpcLevel = IntersectionArray[1];
	KeReleaseQueuedSpinLockFromDpcLevel = IntersectionArray[2];
	KiReleaseDispatcherLockFromSynchLevel = IntersectionArray[3];
	KiExitDispatcher = IntersectionArray[4];

	/*
		Extract KiTimerTableListHead...
	*/
	if(!ExtractSymbolAddresses((void*)KeCancelTimer, TRUE, 0x8D48, OUT SymbolArray))
		return STATUS_NOT_SUPPORTED;

	if(!ExtractSymbolAddresses((void*)KeSetTimerEx, TRUE, 0x8D48, OUT ValidationArray))
		return STATUS_NOT_SUPPORTED;

	MatchCount = IntersectSymbolAddresses(SymbolArray, ValidationArray, OUT IntersectionArray);

	for(iSymbol = 0; iSymbol < MatchCount; iSymbol++)
	{
		if(ValidateTimerTable(IntersectionArray[iSymbol]))
		{
			// check if we found ambiguous symbol references
			KiTimerTableListHead = IntersectionArray[iSymbol];

			break;
		}
	}

	if(KiTimerTableListHead == NULL)
		return STATUS_NOT_SUPPORTED;

	/*
		Create test timer...

		We use a well known, probably unique DeferredContext for later identification...
	*/
	KeInitializeTimer(&TestTimer);
	KeInitializeDpc(&TestTimerDpc, OnTestTimerInvokation, (void*)PgInitialize);

	__try
	{
		KeSetTimerEx(&TestTimer, TimerDueTime, 10000, &TestTimerDpc);

		/*
			Extract KiWaitAlways and KiWaitNever
		*/
		if(!ExtractSymbolAddresses((void*)KeSetTimerEx, TRUE, 0x8B48, OUT SymbolArray))
			return STATUS_NOT_SUPPORTED;

		for(iSymbol = 0; iSymbol < MAX_SYMBOL_COUNT; iSymbol++)
		{
			if(SymbolArray[iSymbol] == NULL)
				break;

			for(Index = 0; Index < MAX_SYMBOL_COUNT; Index++)
			{
				if(SymbolArray[Index] == NULL)
					break;

				if(iSymbol == Index)
					continue;

				if(!ProbeAndSetKeys(
						&TestTimer,
						(ULONGLONG*)SymbolArray[iSymbol],
						(ULONGLONG*)SymbolArray[Index]))
					continue;

				return STATUS_SUCCESS;
			}
		}

		return STATUS_NOT_SUPPORTED;
	}
	__finally
	{
		// cleanup resources
		KeCancelTimer(&TestTimer);
	}
}