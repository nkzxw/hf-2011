//********************************************************************
//	created:	11:8:2008   19:36
//	file:		portio.cpp
//	author:		tiamo
//	purpose:	port io
//********************************************************************

#include "stdafx.h"

//
// read port byte
//
UCHAR __declspec(naked) READ_PORT_UCHAR(__in PUCHAR Port)
{
	__asm
	{
		mov     dx, [esp+4]
		in      al, dx
		retn    4
	}
}

//
// read port word
//
USHORT __declspec(naked) READ_PORT_USHORT(__in PUSHORT Port)
{
	__asm
	{
		mov     dx, [esp+4]
		in      ax, dx
		retn    4
	}
}

//
// read port dword
//
ULONG __declspec(naked) READ_PORT_ULONG(__in PULONG Port)
{
	__asm
	{
		mov     dx,  [esp+4]
		in      eax, dx
		retn    4
	}
}

//
// read port buffer byte
//
VOID __declspec(naked) READ_PORT_BUFFER_UCHAR(__in PUCHAR Port,__out PUCHAR Buffer,__in ULONG Count)
{
	__asm
	{
		mov     dx,  [esp+4]
		mov     ecx, [esp+0ch]
		push    edi
		mov     edi, [esp+0ch]
		rep		insb
		pop     edi
		retn    0ch
	}
}

//
// read port buffer short
//
VOID __declspec(naked) READ_PORT_BUFFER_USHORT(__in PUSHORT Port,__out PUSHORT Buffer,__in ULONG Count)
{
	__asm
	{
		mov     dx,  [esp+4]
		mov     ecx, [esp+0ch]
		push    edi
		mov     edi, [esp+0ch]
		rep		insw
		pop     edi
		retn    0ch
	}
}

//
// read port buffer long
//
VOID __declspec(naked) READ_PORT_BUFFER_ULONG(__in PULONG Port,__out PULONG Buffer,__in ULONG Count)
{
	__asm
	{
		mov     dx,  [esp+4]
		mov     ecx, [esp+0ch]
		push    edi
		mov     edi, [esp+0ch]
		rep		insd
		pop     edi
		retn    0ch
	}
}

//
// write port byte
//
VOID __declspec(naked) WRITE_PORT_UCHAR(__in PUCHAR Port,__in UCHAR Value)
{
	__asm
	{
		mov     dx, [esp+4]
		mov     al, [esp+8]
		out     dx, al
		retn    8
	}
}

//
// write port short
//
VOID __declspec(naked) WRITE_PORT_USHORT(__in PUSHORT Port,__in USHORT Value)
{
	__asm
	{
		mov     dx, [esp+4]
		mov     ax, [esp+8]
		out     dx, ax
		retn    8
	}
}

//
// write port byte
//
VOID __declspec(naked) WRITE_PORT_ULONG(__in PULONG Port,__in ULONG Value)
{
	__asm
	{
		mov     dx, [esp+4]
		mov     eax,[esp+8]
		out     dx, eax
		retn    8
	}
}

//
// read port buffer byte
//
VOID __declspec(naked) WRITE_PORT_BUFFER_UCHAR(__in PUCHAR Port,__in PUCHAR Buffer,__in ULONG Count)
{
	__asm
	{
		mov     dx,  [esp+4]
		mov     ecx, [esp+0ch]
		push    edi
		mov     edi, [esp+0ch]
		rep		outsb
		pop     edi
		retn    0ch
	}
}

//
// read port buffer short
//
VOID __declspec(naked) WRITE_PORT_BUFFER_USHORT(__in PUSHORT Port,__in PUSHORT Buffer,__in ULONG Count)
{
	__asm
	{
		mov     dx,  [esp+4]
		mov     ecx, [esp+0ch]
		push    edi
		mov     edi, [esp+0ch]
		rep		outsw
		pop     edi
		retn    0ch
	}
}

//
// write port buffer long
//
VOID __declspec(naked) WRITE_PORT_BUFFER_ULONG(__in PULONG Port,__in PULONG Buffer,__in ULONG Count)
{
	__asm
	{
		mov     dx,  [esp+4]
		mov     ecx, [esp+0ch]
		push    edi
		mov     edi, [esp+0ch]
		rep		outsd
		pop     edi
		retn    0ch
	}
}

//
// read register byte
//
UCHAR __declspec(naked) READ_REGISTER_UCHAR(__in PUCHAR Port)
{
	__asm
	{
		mov     edx, [esp+4]
		mov     al, [edx]
		retn    4
	}
}

//
// read register word
//
USHORT __declspec(naked) READ_REGISTER_USHORT(__in PUSHORT Port)
{
	__asm
	{
		mov     edx, [esp+4]
		mov     ax, [edx]
		retn    4
	}
}

//
// read register dword
//
ULONG __declspec(naked) READ_REGISTER_ULONG(__in PULONG Port)
{
	__asm
	{
		mov     edx, [esp+4]
		mov     eax, [edx]
		retn    4
	}
}

//
// read register buffer byte
//
VOID __declspec(naked) READ_REGISTER_BUFFER_UCHAR(__in PUCHAR Port,__out PUCHAR Buffer,__in ULONG Count)
{
	__asm
	{
		mov     eax, esi
		mov     edx, edi
		mov     ecx, [esp+0ch]
		mov     esi, [esp+4]
		mov     edi, [esp+8]
		rep		movsb
		mov     edi, edx
		mov     esi, eax
		retn    0ch
	}
}

//
// read port buffer short
//
VOID __declspec(naked) READ_REGISTER_BUFFER_USHORT(__in PUSHORT Port,__out PUSHORT Buffer,__in ULONG Count)
{
	__asm
	{
		mov     eax, esi
		mov     edx, edi
		mov     ecx, [esp+0ch]
		mov     esi, [esp+4]
		mov     edi, [esp+8]
		rep		movsw
		mov     edi, edx
		mov     esi, eax
		retn    0ch
	}
}

//
// read port buffer long
//
VOID __declspec(naked) READ_REGISTER_BUFFER_ULONG(__in PULONG Port,__out PULONG Buffer,__in ULONG Count)
{
	__asm
	{
		mov     eax, esi
		mov     edx, edi
		mov     ecx, [esp+0ch]
		mov     esi, [esp+4]
		mov     edi, [esp+8]
		rep		movsd
		mov     edi, edx
		mov     esi, eax
		retn    0ch
	}
}

//
// write port byte
//
VOID __declspec(naked) WRITE_REGISTER_UCHAR(__in PUCHAR Port,__in UCHAR Value)
{
	__asm
	{
		mov     edx, [esp+4]
		mov     al, [esp+8]
		mov     [edx], al
		lock or [esp+4], edx
		retn    8
	}
}

//
// write port short
//
VOID __declspec(naked) WRITE_REGISTER_USHORT(__in PUSHORT Port,__in USHORT Value)
{
	__asm
	{
		mov     edx, [esp+4]
		mov     ax, [esp+8]
		mov     [edx], ax
		lock or [esp+4], edx
		retn    8
	}
}

//
// write port byte
//
VOID __declspec(naked) WRITE_REGISTER_ULONG(__in PULONG Port,__in ULONG Value)
{
	__asm
	{
		mov     edx, [esp+4]
		mov     eax, [esp+8]
		mov     [edx], eax
		lock or [esp+4], edx
		retn    8
	}
}

//
// write port buffer byte
//
VOID __declspec(naked) WRITE_REGISTER_BUFFER_UCHAR(__in PUCHAR Port,__in PUCHAR Buffer,__in ULONG Count)
{
	__asm
	{
		mov     eax, esi
		mov     edx, edi
		mov     ecx, [esp+0Ch]
		mov     esi, [esp+8]
		mov     edi, [esp+4]
		rep		movsb
		lock or [esp+4], ecx
		mov     edi, edx
		mov     esi, eax
		retn    0ch
	}
}

//
// write port buffer short
//
VOID __declspec(naked) WRITE_REGISTER_BUFFER_USHORT(__in PUSHORT Port,__in PUSHORT Buffer,__in ULONG Count)
{
	__asm
	{
		mov     eax, esi
		mov     edx, edi
		mov     ecx, [esp+0Ch]
		mov     esi, [esp+8]
		mov     edi, [esp+4]
		rep		movsw
		lock or [esp+4], ecx
		mov     edi, edx
		mov     esi, eax
		retn    0ch
	}
}

//
// write port buffer long
//
VOID __declspec(naked) WRITE_REGISTER_BUFFER_ULONG(__in PULONG Port,__in PULONG Buffer,__in ULONG Count)
{
	__asm
	{
		mov     eax, esi
		mov     edx, edi
		mov     ecx, [esp+0Ch]
		mov     esi, [esp+8]
		mov     edi, [esp+4]
		rep		movsd
		lock or [esp+4], ecx
		mov     edi, edx
		mov     esi, eax
		retn    0ch
	}
}

//
// yield processor
//
VOID __declspec(naked) KeYieldProcessor()
{
	__asm
	{
		pause
		retn
	}
}

VOID __declspec(naked) __fastcall RDMSR(__in ULONG Index)
{
	__asm
	{
		rdmsr
		retn
	}
}

VOID __declspec(naked) WRMSR(__in ULONG Index,__in ULONG _EAX,__in ULONG _EDX)
{
	__asm
	{
		mov     ecx, [esp+4]
		mov     eax, [esp+8]
		mov     edx, [esp+0ch]
		wrmsr
		retn    0ch
	}
}

VOID HalpReboot()
{
	WRITE_PORT_UCHAR(reinterpret_cast<PUCHAR>(0x70),0x0b);
	FwStallExecution(1);

	UCHAR Value											= READ_PORT_UCHAR(reinterpret_cast<PUCHAR>(0x71));
	FwStallExecution(1);

	Value												&= ~0x40;
	WRITE_PORT_UCHAR(reinterpret_cast<PUCHAR>(0x71),Value);
	FwStallExecution(1);

	WRITE_PORT_UCHAR(reinterpret_cast<PUCHAR>(0x70),0x0a);
	FwStallExecution(1);

	Value												= READ_PORT_UCHAR(reinterpret_cast<PUCHAR>(0x71));
	Value												&= ~9;
	Value												|= 6;
	WRITE_PORT_UCHAR(reinterpret_cast<PUCHAR>(0x71),Value);
	FwStallExecution(1);

	WRITE_PORT_UCHAR(reinterpret_cast<PUCHAR>(0x70),0x15);
	FwStallExecution(1);

	WRITE_PORT_UCHAR(reinterpret_cast<PUCHAR>(0x64),0xfe);
}

//
// stop floppy
//
VOID MdShutoffFloppy()
{
	WRITE_PORT_UCHAR(reinterpret_cast<PUCHAR>(0x3f2),0xC);
}