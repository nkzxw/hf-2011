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
*********************************** VistaSp0_KiTimerListExpire_01_Patch
*************************************************************************

Description:

	Searches for KiTimerListExpire.
*/
MEMORY_PATCH_INFO VistaSp0_KiTimerListExpire_01_Patch()
{
	/*
	nt!KiTimerExpiration+0x888:
		488b5308        mov     rdx,qword ptr [rbx+8]
		488b4bf8        mov     rcx,qword ptr [rbx-8]
		4d8bcc          mov     r9,r12
		4c8bc7          mov     r8,rdi
		ff13            call    qword ptr [rbx]
		4084f6          test    sil,sil
		742c            je      nt!KiTimerExpiration+0x8d3 (fffff800`0184824d)
	*/
	UCHAR				Block[8 * 2 + 4] =
	{
		0x48, 0x8b, 0x53, 0x08, 0x48, 0x8b, 0x4b, 0xf8, 
		0x4d, 0x8b, 0xcc, 0x4c, 0x8b, 0xc7, 0xff, 0x13, 
		0x40, 0x84, 0xf6, 0x74, 
	};
	ULONGLONG			RangeArray[2] = {KernelImageStart, KernelImageEnd,};
	MEMORY_PATCH_INFO	Result;

	/*
		Prepare memory search info...
	*/
	MEMORY_PATCH_INIT;

	return Result;
}

/************************************************************************
*********************************** VistaSp0_KiTimerListExpire_02_Patch
*************************************************************************

Description:

	Searches for KiTimerListExpire. 
*/
MEMORY_PATCH_INFO VistaSp0_KiTimerListExpire_02_Patch()
{
	/*
	nt!KiTimerExpiration+0x7a2:
		488b5308        mov     rdx,qword ptr [rbx+8]
		488b4bf8        mov     rcx,qword ptr [rbx-8]
		4d8bcc          mov     r9,r12
		4d8bc5          mov     r8,r13
		ff13            call    qword ptr [rbx]
		4084ed          test    bpl,bpl
		742c            je      nt!KiTimerExpiration+0x7e5 (fffff800`0184815f)
	*/
	UCHAR				Block[8 * 2 + 4] =
	{
		0x48, 0x8b, 0x53, 0x08, 0x48, 0x8b, 0x4b, 0xf8, 
		0x4d, 0x8b, 0xcc, 0x4d, 0x8b, 0xc5, 0xff, 0x13, 
		0x40, 0x84, 0xed, 0x74,
	};
	ULONGLONG			RangeArray[2] = {KernelImageStart, KernelImageEnd,};
	MEMORY_PATCH_INFO	Result;

	/*
		Prepare memory search info...
	*/
	MEMORY_PATCH_INIT;

	return Result;
}

/************************************************************************
***************************************** VistaSp0_KiRetireDpcListPatch
*************************************************************************

Description:

	Searches for KiRetireDpcList.
*/
MEMORY_PATCH_INFO VistaSp0_KiRetireDpcListPatch()
{
	/*
	nt!KiRetireDpcList+0x145:
		4d8bcc          mov     r9,r12
		4c8bc5          mov     r8,rbp
		488bd6          mov     rdx,rsi
		488bcf          mov     rcx,rdi
		ff542470        call    qword ptr [rsp+70h]
		4584ff          test    r15b,r15b
		742b            je      nt!KiRetireDpcList+0x185 (fffff800`01850542)
	*/
	UCHAR				Block[8 * 2 + 4] =
	{
		0x4d, 0x8b, 0xcc, 0x4c, 0x8b, 0xc5, 0x48, 0x8b, 
		0xd6, 0x48, 0x8b, 0xcf, 0xff, 0x54, 0x24, 0x70, 
		0x45, 0x84, 0xff, 0x74, 
	};
	ULONGLONG			RangeArray[2] = {KernelImageStart, KernelImageEnd,};
	MEMORY_PATCH_INFO	Result;

	/*
		Prepare memory search info...
	*/
	MEMORY_PATCH_INIT;

	return Result;
}

/************************************************************************
***************************************** VistaSp0_ExpWorkerThreadPatch
*************************************************************************

Description:

	Searches for ExpWorkerThread.
*/
MEMORY_PATCH_INFO VistaSp0_ExpWorkerThreadPatch()
{
	/*
	nt!ExpWorkerThread+0x11a:
		5c              pop     rsp
		2470            and     al,70h
		4c8b6318        mov     r12,qword ptr [rbx+18h]
		488b7b10        mov     rdi,qword ptr [rbx+10h]
		498bcc          mov     rcx,r12
		ffd7            call    rdi
		4c8d9ee0020000  lea     r11,[rsi+2E0h]
		4d391b          cmp     qword ptr [r11],r11
	*/
	UCHAR				Block[8 * 1 + 7] =
	{
		0x24, 0x70, 0x4c, 0x8b, 0x63, 0x18, 0x48, 0x8b, 
		0x7b, 0x10, 0x49, 0x8b, 0xcc, 0xff, 0xd7,
	};
	ULONGLONG			RangeArray[2] = {KernelImageStart, KernelImageEnd,};
	MEMORY_PATCH_INFO	Result;

	/*
		Prepare memory search info...
	*/
	MEMORY_PATCH_INIT;

	return Result;
}

/************************************************************************
***************************************** VistaSP0_IsPatchable
*************************************************************************

Description:

	Returns true if we are running on a supported vista version
	without service pack.
	
*/
BOOLEAN VistaSP0_IsPatchable(
		PMEMORY_PATCH_INFO InKiTimerExpiration_01_Patch,
		PMEMORY_PATCH_INFO InKiTimerExpiration_02_Patch,
		PMEMORY_PATCH_INFO InKiRetireDpcListPatch,
		PMEMORY_PATCH_INFO InExpWorkerThreadPatch)
{
	/*
		This way we can probe multiple patch vectors until one succeeds.
		So we will only need one driver for all supported systems...
	*/
	if((InKiTimerExpiration_01_Patch->LocCount != 1) || 
			(InKiTimerExpiration_02_Patch->LocCount != 2) || 
			(InKiRetireDpcListPatch->LocCount != 1) ||
			(InExpWorkerThreadPatch->LocCount != 1))
		return FALSE;

	return TRUE;
}

VOID VistaSp0_ExpWorkerThread_Fix(VOID* InValue);

/************************************************************************
***************************************** VistaSP1_ApplyPatches
*************************************************************************

Description:

	It patches the DPC handlers to redirect all calls to VistaAll_DpcInterceptor().

*/
void VistaSP0_ApplyPatches(
		PMEMORY_PATCH_INFO InKiTimerExpiration_01_Patch,
		PMEMORY_PATCH_INFO InKiTimerExpiration_02_Patch,
		PMEMORY_PATCH_INFO InKiRetireDpcListPatch,
		PMEMORY_PATCH_INFO InKiCustomAccessRoutinePatch,
		PMEMORY_PATCH_INFO InExpWorkerThreadPatch)
{
	ULONG			Index;
	UCHAR*			KiTimerExpiration[3] = 
	{
		(UCHAR*)InKiTimerExpiration_01_Patch->LocArray[0],
		(UCHAR*)InKiTimerExpiration_02_Patch->LocArray[0],
		(UCHAR*)InKiTimerExpiration_02_Patch->LocArray[1],
	};
	UCHAR*			ExpWorkerThread = (UCHAR*)InExpWorkerThreadPatch->LocArray[0];
	UCHAR*			KiRetireDpcList = (UCHAR*)InKiRetireDpcListPatch->LocArray[0];
	UCHAR**			KiCustomAccessRoutines = (UCHAR**)InKiCustomAccessRoutinePatch->LocArray;
	LONGLONG		RelAddr;
	ULONGLONG		AtomicCache;
	UCHAR*			AtomicBytes = (UCHAR*)&AtomicCache;
	UCHAR			JumpTable[40] = 
	{
		0x4c, 0x8b, 0xc7,				// mov		r8,rdi
		0xeb, 0x08,						// jmp		13
		0x48, 0x8b, 0xcf,				// mov		rcx,rdi
		0xeb, 0x03,						// jmp		8
		0x4d, 0x8b, 0xc5,				// mov		r8,r13
		0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, offset [VistaAll_DpcInterceptor]
		0xff, 0xe0,						// jmp		rax

		// 25:
		0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov rax, offset [VistaSp0_ExpWorkerThread_Fix]
		0xff, 0xe0,						// jmp		rax
	};

	// jump table
	memcpy(KiCustomAccessRoutines[2] + 0x05, JumpTable, sizeof(JumpTable));

	RelAddr = (LONGLONG)VistaAll_DpcInterceptor;
	memcpy(KiCustomAccessRoutines[2] + 0x05 + 15, &RelAddr, 8);

	RelAddr = (LONGLONG)VistaSp0_ExpWorkerThread_Fix;
	memcpy(KiCustomAccessRoutines[2] + 0x05 + 27, &RelAddr, 8);


	/*
	***********************************************
	nt!ExpWorkerThread+0x11a:
		2470            and     al,70h
		4c8b6318        mov     r12,qword ptr [rbx+18h]
		488b7b10        mov     rdi,qword ptr [rbx+10h]

		-----

			498bcc          mov     rcx,r12
			ffd7            call    rdi
		
		=====>

			E8XXXXXXXX		call	KiCustomAccessRoutines[2] + 0x05 + 25		
		
		4c8d9ee0020000  lea     r11,[rsi+2E0h]
		4d391b          cmp     qword ptr [r11],r11
	*/
	RelAddr = (LONGLONG)((KiCustomAccessRoutines[2] + 0x05 + 25) - (ExpWorkerThread + 10 + 5));

	ASSERT(RelAddr == (LONG)RelAddr);

	AtomicCache = *((ULONGLONG*)(ExpWorkerThread + 10));
	{
		AtomicBytes[0] = 0xE8;

		memcpy(AtomicBytes + 1, &RelAddr, 4);
	}
	*((ULONGLONG*)(ExpWorkerThread + 10)) = AtomicCache;

	/*
	***********************************************
	nt!KiRetireDpcList+0x145:
		4d8bcc          mov     r9,r12
		4c8bc5          mov     r8,rbp
		488bd6          mov     rdx,rsi
		
		------

			488bcf          mov     rcx,rdi
			ff542470        call    qword ptr [rsp+70h]

		=>
			90				nop
			90				nop
			E8XXXXXXXX		call	KiCustomAccessRoutine0+0x05+0x03

		-------
		4584ff          test    r15b,r15b
		742b            je      nt!KiRetireDpcList+0x185 (fffff800`01850542)
	*/
	RelAddr = (LONGLONG)((KiCustomAccessRoutines[2] + 0x05 + 0x05) - (KiRetireDpcList + 9 + 7));

	ASSERT(RelAddr == (LONG)RelAddr);

	AtomicCache = *((ULONGLONG*)(KiRetireDpcList + 9));
	{
		AtomicBytes[0] = 0x90;
		AtomicBytes[1] = 0x90;
		AtomicBytes[2] = 0xE8;

		memcpy(AtomicBytes + 3, &RelAddr, 4);
	}
	*((ULONGLONG*)(KiRetireDpcList + 9)) = AtomicCache;

	/***********************************************
	nt!KiTimerExpiration+0x888:
		488b5308        mov     rdx,qword ptr [rbx+8]
		488b4bf8        mov     rcx,qword ptr [rbx-8]
		4d8bcc          mov     r9,r12

		---------

			4c8bc7          mov     r8,rdi
			ff13            call    qword ptr [rbx]

		=>

			E8XXXXXXXX		call	KiCustomAccessRoutine0+0x05+0x03

		---------

		4084f6          test    sil,sil
		742c            je      nt!KiTimerExpiration+0x8d3 (fffff800`0184824d)
	*/

	RelAddr = (LONGLONG)((KiCustomAccessRoutines[2] + 0x05) - (KiTimerExpiration[0] + 11 + 5));

	ASSERT(RelAddr == (LONG)RelAddr);

	AtomicCache = *((ULONGLONG*)(KiTimerExpiration[0] + 11));
	{
		AtomicBytes[0] = 0xE8;

		memcpy(AtomicBytes + 1, &RelAddr, 4);
	}
	*((ULONGLONG*)(KiTimerExpiration[0] + 11)) = AtomicCache;

	/***********************************************
	nt!KiTimerExpiration+0x7a2:
		488b5308        mov     rdx,qword ptr [rbx+8]
		488b4bf8        mov     rcx,qword ptr [rbx-8]
		4d8bcc          mov     r9,r12

		---------

			4d8bc5          mov     r8,r13
			ff13            call    qword ptr [rbx]

		=>

			E8XXXXXXXX		call	KiCustomAccessRoutine0+0x05+0x03

		---------

		4084ed          test    bpl,bpl
		742c            je      nt!KiTimerExpiration+0x7e5 (fffff800`0184815f)
	*/

	for(Index = 1; Index < 3; Index++)
	{
		RelAddr = (LONGLONG)((KiCustomAccessRoutines[2] + 5 + 10) - (KiTimerExpiration[Index] + 11 + 5));

		ASSERT(RelAddr == (LONG)RelAddr);

		AtomicCache = *((ULONGLONG*)(KiTimerExpiration[Index] + 11));
		{
			AtomicBytes[0] = 0xE8;

			memcpy(AtomicBytes + 1, &RelAddr, 4);
		}
		*((ULONGLONG*)(KiTimerExpiration[Index] + 11)) = AtomicCache;
	}

	/*
		From now on ALL!! DPCs will be redirected to our interception method...
	*/
}

