//********************************************************************
//	created:	27:9:2008   22:26
//	file:		tcpxsum.cpp
//	author:		tiamo
//	purpose:	tcp checksum
//********************************************************************

#include "stdafx.h"

#if _TCP_CHECKSUM_SUPPORT_

//
// make sapce
//
#define MAKE_SPACE										__asm __emit	0
#define MAKE_SPACE2										MAKE_SPACE MAKE_SPACE
#define MAKE_SPACE4										MAKE_SPACE2 MAKE_SPACE2
#define MAKE_SPACE8										MAKE_SPACE4 MAKE_SPACE4
#define MAKE_SPACE16									MAKE_SPACE8 MAKE_SPACE8
#define MAKE_SPACE32									MAKE_SPACE16 MAKE_SPACE16
#define MAKE_SPACE64									MAKE_SPACE32 MAKE_SPACE32
#define MAKE_SPACE128									MAKE_SPACE64 MAKE_SPACE64

//
// make loop entry
//
#define MAKE_LOOP_ENTRY(x)								loop_entry_##x:__asm adc eax,edx __asm mov edx,[esi + x*4]
#define MAKE_LOOP_ENTRY_NO_LABEL(x)						__asm adc eax,edx __asm mov edx,[esi + x*4]

//
// make reference
//
#define MAKE_REFERENCE_START							__asm mov ecx,loop_entry + 4
#define MAKE_REFERENCE(x)								__asm mov eax,loop_entry_##x __asm mov [ecx],eax __asm add ecx,4
#define MAKE_REFERENCE_END								__asm jmp make_jump_table_done

//
// tcp checksum
//
ULONG __declspec(naked) tcpxsum(__in ULONG IntialValue,__in PVOID Buffer,__in ULONG Length)
{
	__asm
	{
		//
		// save nonvolatile register
		//
        push    ebx
        push    esi

		//
		// HACK HACK HACK
		//
		mov		eax,loop_entry + 4
		cmp		[eax],0
		jz		make_jump_table

make_jump_table_done:
        mov     ecx,[esp + 0x14]						// get length in bytes
        sub     eax,eax									// clear computed checksum
        test    ecx,ecx									// any bytes to checksum at all?
        jz      checksum_done							// no bytes to checksum

		//
		// if the checksum buffer is not word aligned, then add the first byte of the buffer to the input checksum.
		//
        mov     esi,[esp + 0x10]						// get source address
        sub     edx,edx									// set up to load word into EDX below
        test    esi,1									// check if buffer word aligned
        jz      short checksum_word_aligned				// if zf, buffer word aligned
        mov     ah,[esi]								// get first byte (we know we'll have to swap at the end)
        inc     esi										// increment buffer address
        dec     ecx										// decrement number of bytes
        jz      checksum_done							// if zf set, no more bytes

		//
		// if the buffer is not an even number of of bytes, then initialize the computed checksum with the last byte of the buffer.
		//
checksum_word_aligned:
        shr     ecx,1									// convert to word count
        jnc     short checksum_start					// if nc, even number of bytes
        mov     al,[esi+ecx*2]							// initialize the computed checksum
        jz      checksum_done							// if zf set, no more bytes

		//
		// compute checksum in large blocks of dwords, with one partial word up front if necessary to get dword alignment,
		// and another partial word at the end if needed.
		//

		//
		// compute checksum on the leading word, if that's necessary to get dword  alignment.
		//
checksum_start:
        test    esi,2									// check if source dword aligned
        jz      short checksum_dword_aligned			// source is already dword aligned
        mov     dx,[esi]								// get first word to checksum
        add     esi,2									// update source address
        add     eax,edx									// update partial checksum (no carry is possible, because EAX and EDX are both 16-bit values)
        dec     ecx										// count off this word (zero case gets picked up below)

		//
		// checksum as many words as possible by processing a dword at a time.
		//
checksum_dword_aligned:
        push    ecx										// so we can tell if there's a trailing word later
        shr     ecx,1									// # of dwords to checksum
        jz      checksum_last_word						// no dwords to checksum

        mov     edx,[esi]								// preload the first dword
        add     esi,4									// point to the next dword
        dec     ecx										// count off the dword we just loaded
        jz      checksum_dword_loop_done				// skip the loop if that was the only dword

        mov     ebx,ecx									// EBX = # of dwords left to checksum
        add     ecx,0x1f								// round up loop count
        shr     ecx,5									// convert from word count to unrolled loop count
        and     ebx,0x1f								// # of partial dwords to do in first loop
        jz      short checksum_dword_loop				// special-case when no partial loop,
														// because fixup below doesn't work in that case (carry flag is cleared at this point, as required at loop entry)

        lea     esi,[esi + ebx*4 - 0x80]				// adjust buffer pointer back to compensate for hardwired displacement at loop entry point
        jmp     loop_entry[ebx*4]						// enter the loop to do the first,partial iteration, after which we can just do 64-word blocks
	}

checksum_dword_loop:
	MAKE_LOOP_ENTRY_NO_LABEL(0x00)
	MAKE_LOOP_ENTRY(0x01)
	MAKE_LOOP_ENTRY(0x02)
	MAKE_LOOP_ENTRY(0x03)
	MAKE_LOOP_ENTRY(0x04)
	MAKE_LOOP_ENTRY(0x05)
	MAKE_LOOP_ENTRY(0x06)
	MAKE_LOOP_ENTRY(0x07)
	MAKE_LOOP_ENTRY(0x08)
	MAKE_LOOP_ENTRY(0x09)
	MAKE_LOOP_ENTRY(0x0a)
	MAKE_LOOP_ENTRY(0x0b)
	MAKE_LOOP_ENTRY(0x0c)
	MAKE_LOOP_ENTRY(0x0d)
	MAKE_LOOP_ENTRY(0x0e)
	MAKE_LOOP_ENTRY(0x0f)
	MAKE_LOOP_ENTRY(0x10)
	MAKE_LOOP_ENTRY(0x11)
	MAKE_LOOP_ENTRY(0x12)
	MAKE_LOOP_ENTRY(0x13)
	MAKE_LOOP_ENTRY(0x14)
	MAKE_LOOP_ENTRY(0x15)
	MAKE_LOOP_ENTRY(0x16)
	MAKE_LOOP_ENTRY(0x17)
	MAKE_LOOP_ENTRY(0x18)
	MAKE_LOOP_ENTRY(0x19)
	MAKE_LOOP_ENTRY(0x1a)
	MAKE_LOOP_ENTRY(0x1b)
	MAKE_LOOP_ENTRY(0x1c)
	MAKE_LOOP_ENTRY(0x1d)
	MAKE_LOOP_ENTRY(0x1e)
	MAKE_LOOP_ENTRY(0x1f)

	__asm
	{
//checksum_dword_loop_end:
        lea     esi,[esi + 0x80]						// update source address
        dec     ecx										// count off unrolled loop iteration
        jnz     checksum_dword_loop						// do more blocks

checksum_dword_loop_done:
        adc     eax,edx									// finish dword checksum
        mov     edx,0									// prepare to load trailing word
        adc     eax,edx

		//
		// compute checksum on the trailing word, if there is one,high word of EDX = 0 at this point carry flag set iff there's a trailing word to do at this point
		//
checksum_last_word:
        pop     ecx										// get back word count
        test    ecx,1									// is there a trailing word?
        jz      short checksum_done						// no trailing word
        add     ax,[esi]								// add in the trailing word
        adc     eax,0

checksum_done:
        mov     ecx,eax									// fold the checksum to 16 bits
        ror     ecx,16
        add     eax,ecx
        mov     ebx,[esp + 0x10]
        shr     eax,16
        test    ebx,1									// check if buffer word aligned
        jz      short checksum_combine					// if zf set, buffer word aligned
        ror     ax,8									// byte aligned--swap bytes back

checksum_combine:
        add     ax,[esp + 0x0c]							// combine checksums
        pop     esi
        adc     eax,0
        pop     ebx
        retn	12
	}

loop_entry:
	MAKE_SPACE128

make_jump_table:
	MAKE_REFERENCE_START
	MAKE_REFERENCE(0x1f)
	MAKE_REFERENCE(0x1e)
	MAKE_REFERENCE(0x1d)
	MAKE_REFERENCE(0x1c)
	MAKE_REFERENCE(0x1b)
	MAKE_REFERENCE(0x1a)
	MAKE_REFERENCE(0x19)
	MAKE_REFERENCE(0x18)
	MAKE_REFERENCE(0x17)
	MAKE_REFERENCE(0x16)
	MAKE_REFERENCE(0x15)
	MAKE_REFERENCE(0x14)
	MAKE_REFERENCE(0x13)
	MAKE_REFERENCE(0x12)
	MAKE_REFERENCE(0x11)
	MAKE_REFERENCE(0x10)
	MAKE_REFERENCE(0x0f)
	MAKE_REFERENCE(0x0e)
	MAKE_REFERENCE(0x0d)
	MAKE_REFERENCE(0x0c)
	MAKE_REFERENCE(0x0b)
	MAKE_REFERENCE(0x0a)
	MAKE_REFERENCE(0x09)
	MAKE_REFERENCE(0x08)
	MAKE_REFERENCE(0x07)
	MAKE_REFERENCE(0x06)
	MAKE_REFERENCE(0x05)
	MAKE_REFERENCE(0x04)
	MAKE_REFERENCE(0x03)
	MAKE_REFERENCE(0x02)
	MAKE_REFERENCE(0x01)
	MAKE_REFERENCE_END
}
#endif