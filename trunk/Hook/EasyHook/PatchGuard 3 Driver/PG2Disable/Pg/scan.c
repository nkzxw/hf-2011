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

typedef struct _LOGENTRY_
{
	SINGLE_LIST_ENTRY		List;
	CHAR*					Text;
	ULONG					Length;
}LOGENTRY, *PLOGENTRY;

/************************************************************************
*************************************** PointerToString
*************************************************************************

Description:

	This is just to make the log file more readable.

*/
char* PointerToString(void* InPtr)
{
	if(!PgIsPatchGuardContext(InPtr))
	{
		if(MmIsAddressValid(InPtr))
			return "valid";
		else
			return "skipped"; // address is invalid but not considered to be PatchGuard specific...
	}
	else
	{
		return "patchguard"; // PatchGuard specific...
	}
}



/************************************************************************
*************************************** PgDumpTimerTable
*************************************************************************

Description:

	All PatchGuard 2 related timers will wear the "suspect" sttribute.

	ATTENTION: The code uses undocumented kernel APIs. Please keep in mind
	that you shouldn't change the code logic and remember that during
	enumeration your code will run at DISPATCH_LEVEL! 

*/
NTSTATUS PgDumpTimerTable()
{
	KIRQL					OldIrql;
	ULONG					Index;
	PKSPIN_LOCK_QUEUE		LockQueue;
	PKTIMER_TABLE_ENTRY		TimerListHead;
	PLIST_ENTRY				TimerList;
	PKTIMER					Timer;
	PKDPC					TimerDpc;
	CHAR					LogEntryText[2048];
	NTSTATUS				Result = STATUS_SUCCESS;
	HANDLE					hLogFile;
	UNICODE_STRING			LogFileName;
	OBJECT_ATTRIBUTES		ObjAttr;
	IO_STATUS_BLOCK			IOStatus;
	ULONG					LogEntryTextLen;
	SINGLE_LIST_ENTRY		LogListHead = {NULL};
	PSINGLE_LIST_ENTRY		LogList;
	LOGENTRY*				LogEntry;

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	/*
		Open log file...
	*/
	RtlInitUnicodeString(&LogFileName, L"\\??\\C:\\patchguard.log");

	InitializeObjectAttributes(
		&ObjAttr, 
		&LogFileName, 
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL, NULL)

	if(!NT_SUCCESS(Result = ZwCreateFile(
			&hLogFile,
			GENERIC_WRITE,
			&ObjAttr,
			&IOStatus,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			FILE_SHARE_READ,
			FILE_OVERWRITE_IF,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
			NULL, 0)))
	{
		KdPrint(("\r\n" "ERROR: Unable to open file \"\\??\\C:\\patchguard.log\". (NTSTATUS: 0x%p)\r\n", (void*)Result));

		return Result;
	}
	
	/*
		Lock the dispatcher database and loop through the timer list...
	*/
	Result = STATUS_SUCCESS;

	OldIrql = KiAcquireDispatcherLockRaiseToSynch();

	for(Index = 0; Index < TIMER_TABLE_SIZE; Index++)
	{
		// we have to emulate the windows timer bug "Index & 0xFF" for this to work...
		LockQueue = KeTimerIndexToLockQueue((UCHAR)(Index & 0xFF));

		KeAcquireQueuedSpinLockAtDpcLevel(LockQueue);
		
		// now we can work with the timer list...
		TimerListHead = &KiTimerTableListHead[Index];
		TimerList = TimerListHead->Entry.Flink;

		while(TimerList != (PLIST_ENTRY)TimerListHead)
		{
			Timer = CONTAINING_RECORD(TimerList, KTIMER, TimerListEntry);
			TimerDpc = PgDeobfuscateTimerDpc(Timer);
			TimerList = TimerList->Flink;

			if(TimerDpc != NULL)
			{
				memset(LogEntryText, 0, sizeof(LogEntryText));

				LogEntryTextLen = _snprintf(LogEntryText, sizeof(LogEntryText) - 1, 
					"<timer address=\"%p\" index=\"%d\" period=\"0x%p\" hand=\"%d\" duetime=\"0x%p\">\r\n"
					"%s"
					"    <dpc>\r\n"
					"        <DeferredContext value=\"0x%p\">%s</DeferredContext>\r\n"
					"        <DeferredRoutine>0x%p</DeferredRoutine>\r\n"
					"        <DpcListBlink value=\"0x%p\">%s</DpcListBlink>\r\n"
					"        <DpcListFlink value=\"0x%p\">%s</DpcListFlink>\r\n"
					"        <DpcData value=\"0x%p\">%s</DpcData>\r\n"
					"        <Importance>%d</Importance>\r\n"
					"        <Number>%d</Number>\r\n"
					"        <SystemArgument1 value=\"0x%p\">%s</SystemArgument1>\r\n"
					"        <SystemArgument2 value=\"0x%p\">%s</SystemArgument2>\r\n"
					"        <Type>%d</Type>\r\n"
					"    </dpc>\r\n"
					"</timer>\r\n\r\n",
					Timer,
					Index,
					(ULONGLONG)Timer->Period,
					(ULONG)Timer->Header.Hand,
					Timer->DueTime.QuadPart,
					PgIsPatchGuardContext(TimerDpc->DeferredContext)?"    <SUSPECT>true</SUSPECT>\t\n":"",
					TimerDpc->DeferredContext, PointerToString(TimerDpc->DeferredContext),
					TimerDpc->DeferredRoutine, 
					TimerDpc->DpcListEntry.Blink, PointerToString(TimerDpc->DpcListEntry.Blink),
					TimerDpc->DpcListEntry.Flink, PointerToString(TimerDpc->DpcListEntry.Flink),
					TimerDpc->DpcData, PointerToString(TimerDpc->DpcData),
					(ULONG)TimerDpc->Importance,
					(ULONG)TimerDpc->Number,
					TimerDpc->SystemArgument1, PointerToString(TimerDpc->SystemArgument1),
					TimerDpc->SystemArgument2, PointerToString(TimerDpc->SystemArgument2),
					(ULONG)TimerDpc->Type
				);

				// allocate memory and add log entry to list...
				if((LogEntry = (LOGENTRY*)ExAllocatePool(NonPagedPool, sizeof(LOGENTRY) + LogEntryTextLen + 1)) == NULL)
				{
					KeReleaseQueuedSpinLockFromDpcLevel(LockQueue);

					Result = STATUS_NO_MEMORY;

					DbgPrint("\r\n" "WARNING: Not enough non-paged memory to write suspect timer to file. Aborting enumeration...\r\n");

					break;
				}

				LogEntry->Text = (CHAR*)(LogEntry + 1);
				LogEntry->Length = LogEntryTextLen;

				memcpy(LogEntry->Text, LogEntryText, LogEntryTextLen);

				PushEntryList(&LogListHead, &LogEntry->List);
			}
		}

		KeReleaseQueuedSpinLockFromDpcLevel(LockQueue);
	}

	KiReleaseDispatcherLockFromSynchLevel();

	KiExitDispatcher(OldIrql);
		
	KdPrint(("\r\n" "INFORMATION: Completed PatchGuard scan...\r\n"));

	/*
		Loop through the log entries and flush them to disk...
		In case of an error during enumeration this actually won't write any
		files, but just free allocated memory...
	*/
	LogList = PopEntryList(&LogListHead);

	while(LogList != NULL)
	{
		LogEntry = CONTAINING_RECORD(LogList, LOGENTRY, List);

		if(NT_SUCCESS(Result))
		{
			Result = ZwWriteFile(
					hLogFile,
					NULL, NULL, NULL,
					&IOStatus,
					LogEntry->Text,
					LogEntry->Length,
					NULL, NULL);
		}

		ExFreePool(LogEntry);

		LogList = PopEntryList(&LogListHead);
	}

	ZwClose(hLogFile);

	return Result;
}
