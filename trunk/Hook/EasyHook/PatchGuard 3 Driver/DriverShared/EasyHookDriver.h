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

#ifndef _M_X64
	#error "This driver part is intended for 64-bit builds only."
#endif


#ifndef _EASYHOOK_H
#define _EASYHOOK_H

#include <ntddk.h>
#include <ntstrsafe.h>
#include "..\DriverShared.h"

#pragma warning (disable:4054) // function pointer to data type...
#pragma warning (disable:4047) // differs in levels of indirection
#pragma warning (disable:4306) // type cast to greater size
#pragma warning (disable:4995) // deprecated
#pragma warning (disable:4152) // function/data conversion
#pragma warning (disable:4204) // non-constant aggregate initializer


extern BOOLEAN		IsPatchGuardDisabled;	
extern UCHAR*		KernelImageStart;
extern UCHAR*		KernelImageEnd;

BOOLEAN PgDisablePatchGuard(PDEVICE_OBJECT InDevice);

BOOLEAN KeContainsSymbol(void* InSymbol);

BOOLEAN PgIsPatchGuardContext(void* Ptr);

NTSTATUS ZwQuerySystemInformation(ULONG InType, PVOID InBuffer, ULONG InBufferSize, ULONG* OutRequiredSize);

#ifdef PATCHGUARD_v2
	/***************************************************************************************
	************************************************************* PatchGuard version 2
	****************************************************************************************

		Well, my approach to disable PatchGuard 2 comes with more code, but
		only because we have to extract unexported kernel symbols. Assuming
		that you already got their addresses it is quite simple to disable PG 2!

		The following methods and declarations are only defined in the PG2Disable-Project.
	*/

	/*
		Some helpful constants...
	*/
	#define LockQueueDispatcherLock			0
	#define LockQueueExpansionLock			1
	#define LockQueuePfnLock				2
	#define LockQueueSystemSpaceLock		3
	#define LockQueueVacbLock				4
	#define LockQueueMasterLock				5
	#define LockQueueNonPagedPoolLock		6
	#define LockQueueIoCancelLock			7
	#define LockQueueWorkQueueLock			8
	#define LockQueueIoVpbLock				9
	#define LockQueueIoDatabaseLock			10
	#define LockQueueIoCompletionLock		11
	#define LockQueueNtfsStructLock			12
	#define LockQueueAfdWorkQueueLock		13
	#define LockQueueBcbLock				14
	#define LockQueueMmNonPagedPoolLock		15
	#define LockQueueMax					15
	#define LockQueueTimerTableLock			17

	typedef KIRQL __fastcall PROC_KiAcquireDispatcherLockRaiseToSynch();
	typedef void __fastcall PROC_KiReleaseDispatcherLockFromSynchLevel();
	typedef void __fastcall PROC_KeAcquireQueuedSpinLockAtDpcLevel(IN OUT PKSPIN_LOCK_QUEUE RefLockQueue);
	typedef void __fastcall PROC_KeReleaseQueuedSpinLockFromDpcLevel(IN OUT PKSPIN_LOCK_QUEUE RefLockQueue);
	typedef void __fastcall PROC_KiExitDispatcher(IN KIRQL InOldIrql);

	typedef struct _KTIMER_TABLE_ENTRY_
	{
		LIST_ENTRY			Entry;
		LARGE_INTEGER		Time;
	}KTIMER_TABLE_ENTRY, *PKTIMER_TABLE_ENTRY;

	extern PROC_KiAcquireDispatcherLockRaiseToSynch*	KiAcquireDispatcherLockRaiseToSynch;
	extern PROC_KeAcquireQueuedSpinLockAtDpcLevel*		KeAcquireQueuedSpinLockAtDpcLevel;
	extern PROC_KeReleaseQueuedSpinLockFromDpcLevel*	KeReleaseQueuedSpinLockFromDpcLevel;
	extern PROC_KiExitDispatcher*						KiExitDispatcher;
	extern PROC_KiReleaseDispatcherLockFromSynchLevel*	KiReleaseDispatcherLockFromSynchLevel;
	extern PKTIMER_TABLE_ENTRY							KiTimerTableListHead;
	extern ULONGLONG									KiWaitAlways;
	extern ULONGLONG									KiWaitNever;	

	void PgInstallTestHook();

	BOOLEAN KeCancelTimer_Showcase(PKTIMER InTimer);

	NTSTATUS PgInitialize();

	NTSTATUS PgDumpTimerTable();

	KDPC* PgDeobfuscateTimerDpcEx(IN PKTIMER InTimer, IN ULONGLONG InKiWaitAlwaysAddr, IN ULONGLONG InKiWaitNeverAddr);

	KDPC* PgDeobfuscateTimerDpc(IN PKTIMER InTimer);

	ULONG PgIsSuspectTimer(PKTIMER InTimer);

	BOOLEAN PgIsCanonical(void* Ptr);

	PKSPIN_LOCK_QUEUE KeTimerIndexToLockQueue(UCHAR InTimerIndex);

#elif defined(PATCHGUARD_v3)

	/***************************************************************************************
	************************************************************* PatchGuard version 3
	****************************************************************************************
	
		PatchGuard 3 can be easily disabled by using fingerprinting. 

		The following is all we need so far...
	*/

	#define MEMORY_PATCH_MAXLOC		50

	typedef struct _MEMORY_PATCH_INFO_
	{
		PVOID				LocArray[MEMORY_PATCH_MAXLOC];
		ULONG				LocCount;
		UCHAR				FindBytes[100];
		ULONG				ByteCount;
		UCHAR				ReplaceBytes[100];
		LONG				ReplaceShift;
		BOOLEAN				CanReplace;
	}MEMORY_PATCH_INFO, *PMEMORY_PATCH_INFO;

	#define MEMORY_PATCH_INIT\
		ASSERT(sizeof(Block) <= sizeof(Result.FindBytes));\
		\
		memset(&Result, 0, sizeof(Result));\
		memset(Result.ReplaceBytes, 0xCC, sizeof(Result.ReplaceBytes));\
		memcpy(Result.FindBytes, Block, sizeof(Block));\
		\
		Result.ByteCount = sizeof(Block);\
		\
		Result.LocCount = 0;\
		Result.ReplaceShift = 0;

	void EnableInterrupts();

	NTSTATUS PgDumpFingerprints();

	void PgDpcInterceptor(PKDPC InDpc);

	NTSTATUS PgProcessMemoryPatchList(
			PMEMORY_PATCH_INFO InPatchList,
			ULONG InPatchCount);

	///////////////////// All Vista versions
	BOOLEAN VistaAll_IsPatchable(PMEMORY_PATCH_INFO InKiCustomAccessRoutinePatch);

	void VistaAll_BeginPatches(PMEMORY_PATCH_INFO InKiCustomAccessRoutinePatch);

	void VistaAll_EndPatches();

	void VistaAll_DpcInterceptor(
			PKDPC InDpc,
			PVOID InDeferredContext,
			PVOID InSystemArgument1,
			PVOID InSystemArgument2);

	VOID VistaAll_ExpWorkerThreadInterceptor(PWORKER_THREAD_ROUTINE InRoutine, VOID* InContext, VOID* InRSP);

	MEMORY_PATCH_INFO VistaAll_FingerprintPatch(BOOLEAN InCanReplace);

	MEMORY_PATCH_INFO VistaAll_FingerprintPatchEx(BOOLEAN InCanReplace);

	MEMORY_PATCH_INFO VistaAll_KiCustomAccessRoutinePatch();

	//////////////////////// Service Pack 1 specific

	MEMORY_PATCH_INFO VistaSp1_ExpWorkerThreadPatch();

	BOOLEAN VistaSP1_IsPatchable(
		PMEMORY_PATCH_INFO InKiTimerListExpirePatch,
		PMEMORY_PATCH_INFO InKiRetireDpcListPatch,
		PMEMORY_PATCH_INFO InExpWorkerThreadPatch);

	MEMORY_PATCH_INFO VistaSp1_KiRetireDpcListPatch();

	MEMORY_PATCH_INFO VistaSp1_KiTimerListExpirePatch();

	void VistaSP1_ApplyPatches(
		PMEMORY_PATCH_INFO InKiTimerListExpirePatch,
		PMEMORY_PATCH_INFO InKiRetireDpcListPatch,
		PMEMORY_PATCH_INFO InKiCustomAccessRoutinePatch,
		PMEMORY_PATCH_INFO InExpWorkerThreadPatch);

	///////////////////////// Service Pack 0 specific

	MEMORY_PATCH_INFO VistaSp0_ExpWorkerThreadPatch();

	MEMORY_PATCH_INFO VistaSp0_KiRetireDpcListPatch();

	MEMORY_PATCH_INFO VistaSp0_KiTimerListExpire_01_Patch();

	MEMORY_PATCH_INFO VistaSp0_KiTimerListExpire_02_Patch();

	BOOLEAN VistaSP0_IsPatchable(
		PMEMORY_PATCH_INFO InKiTimerExpiration_01_Patch,
		PMEMORY_PATCH_INFO InKiTimerExpiration_02_Patch,
		PMEMORY_PATCH_INFO InKiRetireDpcListPatch,
		PMEMORY_PATCH_INFO InExpWorkerThreadPatch);

	void VistaSP0_ApplyPatches(
		PMEMORY_PATCH_INFO InKiTimerExpiration_01_Patch,
		PMEMORY_PATCH_INFO InKiTimerExpiration_02_Patch,
		PMEMORY_PATCH_INFO InKiRetireDpcListPatch,
		PMEMORY_PATCH_INFO InKiCustomAccessRoutinePatch,
		PMEMORY_PATCH_INFO InExpWorkerThreadPatch);

#else
	#error "Please specify the desired PatchGuard version you want to patch."
#endif

#endif
