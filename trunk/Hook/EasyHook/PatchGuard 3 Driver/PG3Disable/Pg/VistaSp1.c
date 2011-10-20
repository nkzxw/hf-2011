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
*************************************** VistaSp1_KiTimerListExpirePatch
*************************************************************************

Description:

	Searches for KiTimerListExpire.
*/
MEMORY_PATCH_INFO VistaSp1_KiTimerListExpirePatch()
{
	UCHAR				Block[8 * 4 + 2] =
	{
		0x45, 0x8b, 0x4e, 0x04, 0x45, 0x8b, 0x06, 0x41, 
		0x89, 0xac, 0x24, 0xa0, 0x37, 0x00, 0x00, 0x48, 
		0x8b, 0x53, 0x08, 0x48, 0x8b, 0x4b, 0xf8, 0xff, 
		0x13, 0x40, 0x84, 0xff, 0x0f, 0x85, 0x6c, 0x8f, 
		0xfd, 0xff
	};
	MEMORY_PATCH_INFO	Result;

	/*
		Prepare memory search info...
	*/
	MEMORY_PATCH_INIT;

	return Result;
}

/************************************************************************
***************************************** VistaSp1_KiRetireDpcListPatch
*************************************************************************

Description:

	Searches for KiRetireDpcList.
*/
MEMORY_PATCH_INFO VistaSp1_KiRetireDpcListPatch()
{
	UCHAR				Block[8 * 3 + 1] =
	{
		0x4d, 0x8b, 0xce, 0x4d, 0x8b, 0xc5, 0x49, 0x8b, 
		0xd4, 0x48, 0x8b, 0xcb, 0xff, 0x54, 0x24, 0x70, 
		0x45, 0x84, 0xff, 0x0f, 0x85, 0x6e, 0x7f, 0xfd, 
		0xff
	};
	MEMORY_PATCH_INFO	Result;

	/*
		Prepare memory search info...
	*/
	MEMORY_PATCH_INIT;

	return Result;
}

/************************************************************************
***************************************** VistaSp1_ExpWorkerThreadPatch
*************************************************************************

Description:

	Searches for ExpWorkerThread.
*/
MEMORY_PATCH_INFO VistaSp1_ExpWorkerThreadPatch()
{
	/*
	nt!ExpWorkerThread+0x100:
		498b7c2418      mov     rdi,qword ptr [r12+18h]
		498b5c2410      mov     rbx,qword ptr [r12+10h]
		488bcf          mov     rcx,rdi
		ffd3            call    rbx
		4c8d9ee0020000  lea     r11,[rsi+2E0h]

	*/
	UCHAR				Block[8 * 2 + 6] =
	{
		0x49, 0x8b, 0x7c, 0x24, 0x18, 0x49, 0x8b, 0x5c, 
		0x24, 0x10, 0x48, 0x8b, 0xcf, 0xff, 0xd3, 0x4c, 
		0x8d, 0x9e, 0xe0, 0x02, 0x00, 0x00
	};
	MEMORY_PATCH_INFO	Result;

	/*
		Prepare memory search info...
	*/
	MEMORY_PATCH_INIT;

	return Result;
}

VOID VistaSp1_ExpWorkerThread_Fix(VOID* InValue);

/************************************************************************
***************************************** VistaSP1_ApplyPatches
*************************************************************************

Description:

	Returns TRUE if the current system is patchable with the
	Service Pack 1 configuration, FALSE otherwise.

*/
BOOLEAN VistaSP1_IsPatchable(
		PMEMORY_PATCH_INFO InKiTimerListExpirePatch,
		PMEMORY_PATCH_INFO InKiRetireDpcListPatch,
		PMEMORY_PATCH_INFO InExpWorkerThreadPatch)
{
	/*
		This way we can probe multiple patch vectors until one succeeds.
		So we will only need one driver for all supported systems...
	*/
	if((InKiTimerListExpirePatch->LocCount != 1) || 
			(InKiRetireDpcListPatch->LocCount != 1) ||
			(InExpWorkerThreadPatch->LocCount != 1))
		return FALSE;

	return TRUE;
}

/************************************************************************
***************************************** VistaSP1_ApplyPatches
*************************************************************************

Description:

	It patches the DPC handlers to redirect all calls to VistaAll_DpcInterceptor().

*/
void VistaSP1_ApplyPatches(
		PMEMORY_PATCH_INFO InKiTimerListExpirePatch,
		PMEMORY_PATCH_INFO InKiRetireDpcListPatch,
		PMEMORY_PATCH_INFO InKiCustomAccessRoutinePatch,
		PMEMORY_PATCH_INFO InExpWorkerThreadPatch)
{
	UCHAR*			ExpWorkerThread = (UCHAR*)InExpWorkerThreadPatch->LocArray[0];
	UCHAR*			KiTimerListExpire = (UCHAR*)InKiTimerListExpirePatch->LocArray[0];
	UCHAR*			KiRetireDpcList = (UCHAR*)InKiRetireDpcListPatch->LocArray[0];
	UCHAR**			KiCustomAccessRoutines = (UCHAR**)InKiCustomAccessRoutinePatch->LocArray;
	LONGLONG		RelAddr;
	ULONGLONG		AtomicCache;
	UCHAR*			AtomicBytes = (UCHAR*)&AtomicCache;
	
	/****************************************
	
		Write a jump table into one of the redirected
		custom access routines...
	*/
	UCHAR			JumpTable[33] = 
	{
		0x48, 0x8b, 0x4b, 0xf8,			// mov rcx,qword ptr [rbx-8]
		0xeb, 0x03,						// jmp 3
		0x48, 0x8b, 0xcb,				// mov rcx,rbx
		0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, offset [PgDpcInterceptor]
		0xff, 0xe0,						// jmp rax

		// 21:
		0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, offset [VistaSp0_ExpWorkerThread_Fix]
		0xff, 0xe0,						// jmp		rax
	};

	
	RelAddr = (LONGLONG)VistaAll_DpcInterceptor;

	memcpy(KiCustomAccessRoutines[2] + 0x05, JumpTable, sizeof(JumpTable));
	memcpy(KiCustomAccessRoutines[2] + 0x05 + 11, &RelAddr, 8);

	RelAddr = (LONGLONG)VistaSp1_ExpWorkerThread_Fix;
	memcpy(KiCustomAccessRoutines[2] + 0x05 + 23, &RelAddr, 8);

	/*
	***********************************************
	nt!ExpWorkerThread+0x101:
		498b7c2418      mov     rdi,qword ptr [r12+18h]
		498b5c2410      mov     rbx,qword ptr [r12+10h]

		-----

			488bcf          mov     rcx,rdi
			ffd3            call    rbx

		=====>
			
			E8XXXXXXXX		call	KiCustomAccessRoutines[2] + 0x05 + 21	

		-----

		4c8d9ee0020000  lea     r11,[rsi+2E0h]
		4d391b          cmp     qword ptr [r11],r11
		0f850c020000    jne     nt!ExpWorkerThread+0x336 (fffff800`01870276)
	*/
	RelAddr = (LONGLONG)((KiCustomAccessRoutines[2] + 0x05 + 21) - (ExpWorkerThread + 10 + 5));

	ASSERT(RelAddr == (LONG)RelAddr);

	AtomicCache = *((ULONGLONG*)(ExpWorkerThread + 10));
	{
		AtomicBytes[0] = 0xE8;

		memcpy(AtomicBytes + 1, &RelAddr, 4);
	}
	*((ULONGLONG*)(ExpWorkerThread + 10)) = AtomicCache;

	/*
	***********************************************
	nt!KiRetireDpcList+0x107:
		4d8bce				mov     r9,r14
		4d8bc5				mov     r8,r13
		498bd4				mov     rdx,r12

		------

			488bcb			mov     rcx,rbx
			ff542470		call    qword ptr [rsp+70h]

		=>
			90				nop
			90				nop
			E8XXXXXXXX		call	KiCustomAccessRoutine0+0x05+0x03

		-------
		4584ff				test    r15b,r15b
		0f856e7ffdff		jne     nt! ?? ::FNODOBFM::`string'+0x39888 (fffff800`0183794e)
	*/
	RelAddr = (LONGLONG)((KiCustomAccessRoutines[2] + 0x05 + 0x06) - (KiRetireDpcList + 9 + 7));

	if(RelAddr != (LONG)RelAddr)
		KeBugCheck(INVALID_DATA_ACCESS_TRAP);

	AtomicCache = *((ULONGLONG*)(KiRetireDpcList + 9));
	{
		AtomicBytes[0] = 0x90;
		AtomicBytes[1] = 0x90;
		AtomicBytes[2] = 0xE8;

		memcpy(AtomicBytes + 3, &RelAddr, 4);
	}
	*((ULONGLONG*)(KiRetireDpcList + 9)) = AtomicCache;

	/***********************************************
	Nt!KiTimerListExpire+0x31a:
		458b4e04			mov     r9d,dword ptr [r14+4]
		458b06				mov     r8d,dword ptr [r14]
		4189ac24a0370000	mov     dword ptr [r12+37A0h],ebp
		488b5308			mov     rdx,qword ptr [rbx+8]
		-------

			488b4bf8		mov     rcx,qword ptr [rbx-8]
			ff13			call    qword ptr [rbx]
		=>
			90				nop
			E8XXXXXXXX		call	KiCustomAccessRoutine0+0x05

		-------
		4084ff				test	dil,dil
		0f856c8ffdff		jne		nt! ?? ::FNODOBFM::`string'+0x39742 (fffff800`01837828)
	*/

	RelAddr = (LONGLONG)((KiCustomAccessRoutines[2] + 0x05) - (KiTimerListExpire + 19 + 6));

	if(RelAddr != (LONG)RelAddr)
		KeBugCheck(INVALID_DATA_ACCESS_TRAP);

	AtomicCache = *((ULONGLONG*)(KiTimerListExpire + 19));
	{
		AtomicBytes[0] = 0x90;
		AtomicBytes[1] = 0xE8;

		memcpy(AtomicBytes + 2, &RelAddr, 4);
	}
	*((ULONGLONG*)(KiTimerListExpire + 19)) = AtomicCache;

	/*
		From now on ALL!! DPCs will be redirected to our interception method...
	*/
}

