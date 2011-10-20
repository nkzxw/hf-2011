/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		i386.h
 *
 * Abstract:
 *
 *		This module definies various types and macros used by x86 specific routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 24-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __I386_H__
#define __I386_H__



typedef struct _KTSS {

    USHORT  Backlink;
    USHORT  Reserved0;

    ULONG   Esp0;
    USHORT  Ss0;
    USHORT  Reserved1;

    ULONG   NotUsed1[4];

    ULONG   CR3;

    ULONG   Eip;

    ULONG   NotUsed2[9];

    USHORT  Es;
    USHORT  Reserved2;

    USHORT  Cs;
    USHORT  Reserved3;

    USHORT  Ss;
    USHORT  Reserved4;

    USHORT  Ds;
    USHORT  Reserved5;

    USHORT  Fs;
    USHORT  Reserved6;

    USHORT  Gs;
    USHORT  Reserved7;

    USHORT  LDT;
    USHORT  Reserved8;

    USHORT  Flags;

    USHORT  IoMapBase;

	/* IO/INT MAPS go here */

} KTSS, *PKTSS;


typedef struct _KGDTENTRY {
    USHORT  LimitLow;
    USHORT  BaseLow;
    union {
        struct {
            UCHAR   BaseMid;
            UCHAR   Flags1;     // Declare as bytes to avoid alignment
            UCHAR   Flags2;     // Problems.
            UCHAR   BaseHi;
        } Bytes;
        struct {
            ULONG   BaseMid : 8;
            ULONG   Type : 5;
            ULONG   Dpl : 2;
            ULONG   Pres : 1;
            ULONG   LimitHi : 4;
            ULONG   Sys : 1;
            ULONG   Reserved_0 : 1;
            ULONG   Default_Big : 1;
            ULONG   Granularity : 1;
            ULONG   BaseHi : 8;
        } Bits;
    } HighWord;
} KGDTENTRY, *PKGDTENTRY;


#define	INTERRUPTS_OFF()		_asm { cli }
#define	INTERRUPTS_ON()			_asm { sti }


/*
 * WP Write Protect (bit 16 of CR0).
 * Inhibits supervisor-level procedures from writing into user-level read-only pages when set;
 * allows supervisor-level procedures to write into user-level read-only pages when clear.
 * This flag facilitates implementation of the copyon-write method of creating a new process (forking)
 * used by operating systems such as UNIX.
 */

#define	CR0_WP_BIT	(0x10000)

#define	MEMORY_PROTECTION_OFF()		\
	__asm	mov eax, cr0			\
	__asm	and eax, NOT CR0_WP_BIT	\
	__asm	mov cr0, eax

#define	MEMORY_PROTECTION_ON()		\
	__asm	mov eax, cr0			\
	__asm	or	eax, CR0_WP_BIT		\
	__asm	mov cr0, eax



/* x86 opcodes */

#define	X86_OPCODE_PUSH					0x68
#define	X86_OPCODE_MOV_EAX_VALUE		0xB8

#define	X86_OPCODE_CALL_EAX				0xD0FF
#define	X86_OPCODE_JMP_DWORD_PTR		0x25FF



/*
 * Save a value on stack:
 *
 * push	PushValue
 */

#define	ASM_PUSH(CodeAddress, PushValue)											\
					* (PCHAR) (CodeAddress)++ = X86_OPCODE_PUSH;					\
					* (PULONG) (CodeAddress) = (ULONG) (PushValue);					\
					(PCHAR) (CodeAddress) += 4;

/*
 * Call a function:
 *
 * mov	eax, FunctionAddress
 * call	eax
 */

#define	ASM_CALL(CodeAddress, FunctionAddress)										\
					* (PCHAR) (CodeAddress)++ = X86_OPCODE_MOV_EAX_VALUE;			\
					* (PULONG) (CodeAddress) = (ULONG) (FunctionAddress);			\
					(PCHAR) (CodeAddress) += 4;										\
					* ((PUSHORT) (CodeAddress))++ = X86_OPCODE_CALL_EAX;


/*
 * Jump to a specified address:
 *
 * jmp dword ptr [next_4_bytes]
 * *(next_4_bytes) = address
 *
 * NOTE XXX: this should be converted to a direct jmp address but i
 * can't figure out how that instruction is encoded (opcode 0xE9)
 */

#define	ASM_JMP(CodeAddress, JmpAddress)											\
					* ((PUSHORT) (CodeAddress))++ = X86_OPCODE_JMP_DWORD_PTR;		\
					* (PULONG) (CodeAddress) = (ULONG) (JmpAddress);				\
					(PCHAR) (CodeAddress) += 4;


extern ULONG	SystemAddressStart;


BOOLEAN InitI386();
VOID VerifyUserReturnAddress();


#endif	/* __I386_H__ */