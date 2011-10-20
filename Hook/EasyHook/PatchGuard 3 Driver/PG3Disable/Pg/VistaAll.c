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


/****************************************************************************
*************************************** VistaAll_DpcInterceptor
*****************************************************************************

Description:

	Filters all DPCs wearing the typical PatchGuard DeferredContext.
	The search will be restricted to the kernel image and memory pool.

*/

void VistaAll_DpcInterceptor(
		PKDPC InDpc,
		PVOID InDeferredContext,
		PVOID InSystemArgument1,
		PVOID InSystemArgument2)
{
	ULONGLONG		Routine = (ULONGLONG)InDpc->DeferredRoutine;

	__try
	{
		if((Routine >= 0xFFFFFA8000000000) && (Routine <= 0xFFFFFAA000000000))
		{
		}
		else
		if(KeContainsSymbol((void*)Routine))
		{
			if(!PgIsPatchGuardContext(InDeferredContext))
				InDpc->DeferredRoutine(InDpc, InDeferredContext, InSystemArgument1, InSystemArgument2);
		}
		else
			InDpc->DeferredRoutine(InDpc, InDeferredContext, InSystemArgument1, InSystemArgument2);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}


VOID VistaAll_ExpWorkerThreadInterceptor(PWORKER_THREAD_ROUTINE InRoutine, VOID* InContext, VOID* InRSP)
{
	ULONGLONG		Val = (ULONGLONG)InRoutine;

	if((Val >= 0xfffffa8000000000) && (Val <= 0xfffffaa000000000))
		return;

	__try
	{
		InRoutine(InContext);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

/************************************************************************
******************************************* KeBugCheck_Hook
*************************************************************************

Description:

	

*/
ULONGLONG		RtlCaptureContext_Sym;
ULONGLONG		KeBugCheckEx_Sym;

ULONGLONG KeBugCheck_Hook(ULONGLONG InBugCode, ULONGLONG InCaller)
{
	FAST_MUTEX WaitAlways;

	if((InCaller >= KeBugCheckEx_Sym) && (InCaller <= KeBugCheckEx_Sym + 100))
	{
		if(InBugCode == CRITICAL_STRUCTURE_CORRUPTION)
		{
			// KeBugCheckEx disables interrupts before calling RtlCaptureContext()
			EnableInterrupts();

			ExInitializeFastMutex(&WaitAlways);
			ExAcquireFastMutex(&WaitAlways);
			ExAcquireFastMutex(&WaitAlways);
		}
	}

	return RtlCaptureContext_Sym + 14;
}

ULONGLONG RtlCaptureContext_Hook(ULONGLONG InCaller);


/************************************************************************
**************************** VistaAll_KiCustomAccessRoutinePatch
*************************************************************************

Description:

	Enumerates all custom access routines.

*/
MEMORY_PATCH_INFO VistaAll_KiCustomAccessRoutinePatch()
{
	UCHAR				Block[8 * 5] =
	{
		0x48, 0x83, 0xec, 0x28, 0x48, 0x8b, 0xd1, 0x83, 
		0xe1, 0x03, 0xff, 0xc1, 0x48, 0xd3, 0xc8, 0x4c, 
		0x33, 0xc0, 0x4c, 0x33, 0xc8, 0x4c, 0x33, 0xd0, 
		0x4c, 0x33, 0xd8, 0x33, 0xc0, 0xe8, 0xbe, 0xff, 
		0xff, 0xff, 0x90, 0x48, 0x83, 0xc4, 0x28, 0xc3,
	};
	ULONGLONG			RangeArray[2] = {KernelImageStart, KernelImageEnd};
	MEMORY_PATCH_INFO	Result;

	/*
		Prepare memory search info...
	*/
	MEMORY_PATCH_INIT;

	return Result;
}

/************************************************************************
**************************** VistaAll_FingerprintPatch
*************************************************************************

Description:

	Enumerates all fingerprints of PatchGuard 3.

*/
MEMORY_PATCH_INFO VistaAll_FingerprintPatch(BOOLEAN InCanReplace)
{
	MEMORY_PATCH_INFO	Result;
	UCHAR				Block[8] = { 0xf0, 0x48, 0x31, 0x11, 0x48, 0x31, 0x51, 0x08};
	ULONGLONG			RangeArray[4] = { KernelImageStart, KernelImageEnd, 0xFFFFFA8000000000, 0xFFFFFAA000000000 };

	MEMORY_PATCH_INIT;

	Result.CanReplace = InCanReplace;
	Result.ReplaceShift = -2;

	return Result;
}

/************************************************************************
******************************************* VistaAll_IsPatchable
*************************************************************************

Description:

	Just checks for the ten custom access routines and loads some
	required symbols...

*/
BOOLEAN VistaAll_IsPatchable(PMEMORY_PATCH_INFO InKiCustomAccessRoutinePatch)
{
	UNICODE_STRING		Symbol;
		
	if(InKiCustomAccessRoutinePatch->LocCount != 10)
		return FALSE;

	return TRUE;
}

/************************************************************************
******************************************* RedirectCustomAccessRoutine
*************************************************************************

Description:

	Install a jumper at "InFrom" so that execution is continued
	at "InTo".

*/
void RedirectCustomAccessRoutine(
				UCHAR* InFrom,
				UCHAR* InTo)
{
	LONGLONG		RelAddr = (LONGLONG)(InTo - (InFrom + 0x05));
	LONGLONG		AtomicCache;
	UCHAR*			AtomicBytes = (UCHAR*)&AtomicCache;

	if(RelAddr != (LONG)RelAddr)
		KeBugCheck(INVALID_DATA_ACCESS_TRAP);

	AtomicCache = *((ULONGLONG*)InFrom);
	{
		AtomicBytes[0] = 0xE9;

		memcpy(AtomicBytes + 1, &RelAddr, 4);
	}
	*((ULONGLONG*)InFrom) = AtomicCache;
}

/************************************************************************
******************************************* VistaAll_BeginPatches
*************************************************************************

Description:

	Redirects two of the ten custom access routines. This way the bytes
	can be used inside the service pack specific patches.

	This method should be called before a specific vista patch is applied.
*/
void VistaAll_BeginPatches(PMEMORY_PATCH_INFO InKiCustomAccessRoutinePatch)
{
	UCHAR**				KiCustomAccessRoutines = (UCHAR**)InKiCustomAccessRoutinePatch->LocArray;
	UNICODE_STRING		Symbol;
	UCHAR*				PatchBytes;

	// redirect some custom access routines to make them patchable...
	RedirectCustomAccessRoutine(KiCustomAccessRoutines[1], KiCustomAccessRoutines[0]);
	RedirectCustomAccessRoutine(KiCustomAccessRoutines[2], KiCustomAccessRoutines[0]);


	// Hook RtlCaptureContext...
	RtlInitUnicodeString(&Symbol, L"KeBugCheckEx");
	KeBugCheckEx_Sym = (ULONGLONG)MmGetSystemRoutineAddress(&Symbol);

	RtlInitUnicodeString(&Symbol, L"RtlCaptureContext");
	RtlCaptureContext_Sym = (ULONGLONG)MmGetSystemRoutineAddress(&Symbol);

	PatchBytes = RtlCaptureContext_Sym;
	PatchBytes[0] = 0x50;
	PatchBytes[1] = 0x48;
	PatchBytes[2] = 0xb8;
	*((ULONGLONG*)(RtlCaptureContext_Sym + 3)) = (ULONGLONG)RtlCaptureContext_Hook;
	PatchBytes[11] = 0xff;
	PatchBytes[12] = 0xe0;
}


/************************************************************************
******************************************* VistaAll_EndPatches
*************************************************************************

Description:

	Converts the non-SEH code path to throw an unhandled breakpoint
	exception that will be caught in the DPC interceptor.

	This method should be called after a specific vista patch
	has been applied.
*/
void VistaAll_EndPatches()
{
	MEMORY_PATCH_INFO		Patch = VistaAll_FingerprintPatch(TRUE);

	PgProcessMemoryPatchList(&Patch, 1);
}
