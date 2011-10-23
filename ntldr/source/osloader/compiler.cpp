//********************************************************************
//	created:	15:8:2008   0:40
//	file:		compiler.cpp
//	author:		tiamo
//	purpose:	compiler lib,ugly,asm instead?
//********************************************************************

#include "stdafx.h"

//
// 64bit right shift
//
extern "C" int __declspec(naked) __cdecl _aullshr()
{
	__asm
	{
		cmp	cl,64
		jae	short RETSIGN

		cmp	cl, 32
		jae	short MORE32
		shrd	eax,edx,cl
		sar	edx,cl
		ret

	MORE32:
		mov	eax,edx
		sar	edx,31
		and	cl,31
		sar	eax,cl
		ret

	RETSIGN:
		sar	edx,31
		mov	eax,edx
		ret
	}
}

//
// 64bit div
//
extern "C" void __declspec(naked) __cdecl _alldiv()
{
	__asm
	{
		push	edi
		push	esi
		push	ebx

		xor	edi,edi

		mov	eax,[esp + 20]
		or	eax,eax
		jge	short L1

		inc	edi
		mov	edx,[esp + 16]
		neg	eax
		neg	edx
		sbb	eax,0
		mov	[esp + 20],eax
		mov	[esp + 16],edx
	L1:
		mov	eax,[esp + 28]
		or	eax,eax
		jge	short L2
		inc	edi
		mov	edx,[esp + 24]
		neg	eax
		neg	edx
		sbb	eax,0
		mov	[esp + 28],eax
		mov	[esp + 24],edx
	L2:
		or	eax,eax
		jnz	short L3
		mov	ecx,[esp + 24]
		mov	eax,[esp + 20]
		xor	edx,edx
		div	ecx
		mov	ebx,eax
		mov	eax,[esp + 16]
		div	ecx
		mov	edx,ebx
		jmp	short L4

	L3:
		mov	ebx,eax
		mov	ecx,[esp + 24]
		mov	edx,[esp + 20]
		mov	eax,[esp + 16]
	L5:
		shr	ebx,1
		rcr	ecx,1
		shr	edx,1
		rcr	eax,1
		or	ebx,ebx
		jnz	short L5
		div	ecx
		mov	esi,eax

		mul	dword ptr [esp + 28]
		mov	ecx,eax
		mov	eax,[esp + 24]
		mul	esi
		add	edx,ecx
		jc	short L6

		cmp	edx,[esp + 20]
		ja	short L6
		jb	short L7
		cmp	eax,[esp + 16]
		jbe	short L7
	L6:
		dec	esi
	L7:
		xor	edx,edx
		mov	eax,esi

	L4:
		dec	edi
		jnz	short L8
		neg	edx
		neg	eax
		sbb	edx,0

	L8:
		pop	ebx
		pop	esi
		pop	edi

		ret	16
	}
}

//
// ULONG64 div
//
extern "C" void __declspec(naked) __cdecl _aulldiv()
{
	__asm
	{
		push	ebx
		push	esi

		mov	eax,[esp + 24]
		or	eax,eax
		jnz	short L1

		mov	ecx,[esp + 20]
		mov	eax,[esp + 16]
		xor	edx,edx
		div	ecx
		mov	ebx,eax
		mov	eax,[esp + 12]
		div	ecx
		mov	edx,ebx
		jmp	short L2

	L1:
		mov	ecx,eax
		mov	ebx,[esp + 20]
		mov	edx,[esp + 14]
		mov	eax,[esp + 12]

	L3:
		shr	ecx,1
		rcr	ebx,1
		shr	edx,1
		rcr	eax,1
		or	ecx,ecx
		jnz	short L3
		div	ebx
		mov	esi,eax

		mul	dword ptr [esp + 24]
		mov	ecx,eax
		mov	eax,[esp + 20]
		mul	esi
		add	edx,ecx
		jc	short L4

		cmp	edx,[esp + 16]
		ja	short L4
		jb	short L5
		cmp	eax,[esp + 12]
		jbe	short L5
	L4:
		dec	esi
	L5:
		xor	edx,edx
		mov	eax,esi

	L2:

		pop	esi
		pop	ebx

		ret	16
	}
}

//
// ULONG rem
//
extern "C" void __declspec(naked) __cdecl _aullrem()
{
	__asm
	{
		push	ebx

		mov	eax,[esp + 16][4]
		or	eax,eax
		jnz	short L1

		mov	ecx,[esp + 16]
		mov	eax,[esp + 8][4]
		xor	edx,edx
		div	ecx
		mov	eax,[esp + 8]
		div	ecx
		mov	eax,edx
		xor	edx,edx
		jmp	short L2
	L1:
		mov	ecx,eax
		mov	ebx,[esp + 16]
		mov	edx,[esp + 8][4]
		mov	eax,[esp + 8]
	L3:
		shr	ecx,1
		rcr	ebx,1
		shr	edx,1
		rcr	eax,1
		or	ecx,ecx
		jnz	short L3

		div	ebx
		mov	ecx,eax
		mul	dword ptr [esp + 16][4]
		xchg	ecx,eax
		mul	dword ptr [esp + 16]
		add	edx,ecx
		jc	short L4

		cmp	edx,[esp + 8][4]
		ja	short L4
		jb	short L5

		cmp	eax,[esp + 8]
		jbe	short L5
	L4:
		sub	eax,[esp + 16]
		sbb	edx,[esp + 16][4]
	L5:
		sub	eax,[esp + 8]
		sbb	edx,[esp + 8][4]
		neg	edx
		neg	eax
		sbb	edx,0
	L2:
		pop	ebx
		ret	16
	}
}

//
// 64bits shift left
//
extern "C" void __declspec(naked) __cdecl _allshl()
{
	__asm
	{
		cmp	cl, 64
		jae	short RETZERO

		cmp	cl, 32
		jae	short MORE32
		shld	edx,eax,cl
		shl	eax,cl
		ret

	MORE32:
		mov	edx,eax
		xor	eax,eax
		and	cl,31
		shl	edx,cl
		ret

	RETZERO:
		xor	eax,eax
		xor	edx,edx
		ret
	}
}

//
// LONG64 rem
//
extern "C" void __declspec(naked) __cdecl _allrem()
{
	__asm
	{
		push	ebx
		push	edi
		xor	edi,edi

		mov	eax,[esp + 12][4]
		or	eax,eax
		jge	short L1
		inc	edi
		mov	edx,[esp + 12]
		neg	eax
		neg	edx
		sbb	eax,0
		mov	[esp + 12][4],eax
		mov	[esp + 12],edx

	L1:
		mov	eax,[esp + 20][4]
		or	eax,eax
		jge	short L2
		mov	edx,[esp + 20]
		neg	eax
		neg	edx
		sbb	eax,0
		mov	[esp + 20][4],eax
		mov	[esp + 20],edx

	L2:
		or	eax,eax
		jnz	short L3
		mov	ecx,[esp + 20]
		mov	eax,[esp + 12][4]
		xor	edx,edx
		div	ecx
		mov	eax,[esp + 12]
		div	ecx
		mov	eax,edx
		xor	edx,edx
		dec	edi
		jns	short L4
		jmp	short L8

	L3:
		mov	ebx,eax
		mov	ecx,[esp + 20]
		mov	edx,[esp + 12][4]
		mov	eax,[esp + 12]
		L5:
		shr	ebx,1
		rcr	ecx,1
		shr	edx,1
		rcr	eax,1
		or	ebx,ebx
		jnz	short L5

		div	ecx
		mov	ecx,eax
		mul	dword ptr [esp + 20][4]
		xchg	ecx,eax
		mul	dword ptr [esp + 20]
		add	edx,ecx
		jc	short L6

		cmp	edx,[esp + 12][4]
		ja	short L6
		jb	short L7
		cmp	eax,[esp + 12]
		jbe	short L7

	L6:
		sub	eax,[esp + 20]
		sbb	edx,[esp + 20][4]

	L7:
		sub	eax,[esp + 12]
		sbb	edx,[esp + 12][4]
		dec	edi
		jns	short L8
		L4:
		neg	edx
		neg	eax
		sbb	edx,0

	L8:
		pop	edi
		pop	ebx

		ret	16
	}
}

//
// LONG64 mul
//
extern "C" void __declspec(naked) __cdecl _allmul()
{
	__asm
	{
		mov	eax,[esp + 4][4]
		mov	ecx,[esp + 12][4]
		or	ecx,eax
		mov	ecx,[esp + 12]
		jnz	short hard

		mov	eax,[esp + 4]
		mul	ecx
		ret	16

hard:
		push	ebx
		mul	ecx
		mov	ebx,eax
		mov	eax,[esp + 8]
		mul	dword ptr [esp + 16][4]
		add	ebx,eax
		mov	eax,[esp + 8]
		mul	ecx
		add	edx,ebx
		pop	ebx

		ret	16
	}
}

//
// LONG64 shr
//
extern "C" void	__declspec(naked) __cdecl _allshr()
{
	__asm
	{
		cmp	cl,64
		jae	short RETSIGN

		cmp	cl, 32
		jae	short MORE32
		shrd	eax,edx,cl
		sar	edx,cl
		ret

	MORE32:
		mov	eax,edx
		sar	edx,31
		and	cl,31
		sar	eax,cl
		ret

	RETSIGN:
		sar	edx,31
		mov	eax,edx
		ret
	}
}

extern "C" ULONG __security_cookie;
ULONG __security_cookie;

extern "C" VOID __fastcall __security_check_cookie(PVOID )
{
}

//
// this function will never be called
//
extern "C" int __cdecl _except_handler3(__in PVOID exception_record,__in PVOID registration,__in PCONTEXT context,__in PVOID dispatcher)
{
	return 0;
}

extern "C" VOID __declspec(naked) __cdecl _SEH_epilog()
{
	__asm
	{
		pop     ecx
		pop     edi
		pop     esi
		pop     ebx
		leave
		push    ecx
		retn
	}
}

extern "C" VOID __declspec(naked) __cdecl _SEH_prolog()
{
	__asm
	{
		push    0
		xor		eax,eax
		push    eax
		mov     eax, [esp+0x10]
		mov     [esp+0x10], ebp
		lea     ebp, [esp+0x10]
		sub     esp, eax
		push    ebx
		push    esi
		push    edi
		mov     eax, [ebp-8]
		mov     [ebp-18h], esp
		push    eax
		mov     eax, [ebp-4]
		mov     dword ptr [ebp-4], 0FFFFFFFFh
		mov     [ebp-8], eax
		lea     eax, [ebp-10h]
		retn
	}
}

extern "C" VOID __declspec(naked) __cdecl _aulldvrm()
{
	__asm
	{
		push    esi

        mov     eax,[esp + 16][4]
        or      eax,eax
        jnz     short L1
        mov     ecx,[esp + 16]
        mov     eax,[esp + 8][4]
        xor     edx,edx
        div     ecx
        mov     ebx,eax
        mov     eax,[esp + 8]
        div     ecx
        mov     esi,eax

        mov     eax,ebx
        mul     dword ptr [esp + 16]
        mov     ecx,eax
        mov     eax,esi
        mul     dword ptr [esp + 16]
        add     edx,ecx
        jmp     short L2
L1:
        mov     ecx,eax
        mov     ebx,[esp + 16]
        mov     edx,[esp + 8][4]
        mov     eax,[esp + 8]
L3:
        shr     ecx,1
        rcr     ebx,1
        shr     edx,1
        rcr     eax,1
        or      ecx,ecx
        jnz     short L3
        div     ebx
        mov     esi,eax

        mul     dword ptr [esp + 16][4]
        mov     ecx,eax
        mov     eax,[esp + 16]
        mul     esi
        add     edx,ecx
        jc      short L4

        cmp     edx,[esp + 8][4]
        ja      short L4
        jb      short L5
        cmp     eax,[esp + 8]
        jbe     short L5
L4:
        dec     esi
        sub     eax,[esp + 16]
        sbb     edx,[esp + 16][4]
L5:
        xor     ebx,ebx

L2:
        sub     eax,[esp + 8]
        sbb     edx,[esp + 8][4]
        neg     edx
        neg     eax
        sbb     edx,0

        mov     ecx,edx
        mov     edx,ebx
        mov     ebx,ecx
        mov     ecx,eax
        mov     eax,esi

        pop     esi
        ret     16
	}
}

extern "C" VOID __declspec(naked) __cdecl _alldvrm()
{
	__asm
	{
		push    edi
        push    esi
        push    ebp

        xor     edi,edi
        xor     ebp,ebp

        mov     eax,[esp + 16][4]
        or      eax,eax
        jge     short L1
        inc     edi
        inc     ebp
        mov     edx,[esp + 16]
        neg     eax
        neg     edx
        sbb     eax,0
        mov     [esp + 16][4],eax
        mov     [esp + 16],edx
L1:
        mov     eax,[esp + 24][4]
        or      eax,eax
        jge     short L2
        inc     edi
        mov     edx,[esp + 24]
        neg     eax
        neg     edx
        sbb     eax,0
        mov     [esp + 24][4],eax
        mov     [esp + 24],edx
L2:
        or      eax,eax
        jnz     short L3
        mov     ecx,[esp + 24]
        mov     eax,[esp + 16][4]
        xor     edx,edx
        div     ecx
        mov     ebx,eax
        mov     eax,[esp + 16]
        div     ecx
        mov     esi,eax

        mov     eax,ebx
        mul     dword ptr [esp + 24]
        mov     ecx,eax
        mov     eax,esi
        mul     dword ptr [esp + 24]
        add     edx,ecx
        jmp     short L4

L3:
        mov     ebx,eax
        mov     ecx,[esp + 24]
        mov     edx,[esp + 16][4]
        mov     eax,[esp + 16]
L5:
        shr     ebx,1
        rcr     ecx,1
        shr     edx,1
        rcr     eax,1
        or      ebx,ebx
        jnz     short L5
        div     ecx
        mov     esi,eax

        mul     dword ptr [esp + 24][4]
        mov     ecx,eax
        mov     eax,[esp + 24]
        mul     esi
        add     edx,ecx
        jc      short L6

        cmp     edx,[esp + 16][4]
        ja      short L6
        jb      short L7
        cmp     eax,[esp + 16]
        jbe     short L7
L6:
        dec     esi
        sub     eax,[esp + 24]
        sbb     edx,[esp + 24][4]
L7:
        xor     ebx,ebx

L4:

        sub     eax,[esp + 16]
        sbb     edx,[esp + 16][4]

		dec     ebp
        jns     short L9
        neg     edx
        neg     eax
        sbb     edx,0

L9:
        mov     ecx,edx
        mov     edx,ebx
        mov     ebx,ecx
        mov     ecx,eax
        mov     eax,esi

		dec     edi
        jnz     short L8
        neg     edx
        neg     eax
        sbb     edx,0

L8:
        pop     ebp
        pop     esi
        pop     edi

        ret     16
	}
}

extern "C" VOID __declspec(naked) __cdecl _chkstk()
{
	__asm
	{
		cmp     eax, 1000h
		jnb     short big_stack

		neg     eax
		add     eax, esp
		add     eax, 4
		test    [eax], eax
		xchg    eax, esp
		mov     eax, [eax]
		push    eax
		retn

	big_stack:
		push    ecx
		lea     ecx, [esp+8]

	touch_stack:
		sub     ecx, 1000h
		sub     eax, 1000h
		test    [ecx], eax
		cmp     eax, 1000h
		jnb     short touch_stack

		sub     ecx, eax
		mov     eax, esp
		test    [ecx], eax
		mov     esp, ecx
		mov     ecx, [eax]
		mov     eax, [eax+4]
		push    eax
		retn
	}
}