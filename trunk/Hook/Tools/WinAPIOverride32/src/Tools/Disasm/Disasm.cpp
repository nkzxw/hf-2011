/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//-----------------------------------------------------------------------------
// Object: disasm helper
//         distorm.lib must be included to project
//         #pragma comment(lib, "distorm64/distorm.lib")
//-----------------------------------------------------------------------------
#include "disasm.h"

BOOL CDisasm::LocalDisasmCallback(_DecodedInst* pDecodedInstructions,pfDisasmCallback pDisasmCallback,PVOID DisasmCallbackUserParam)
{
    TCHAR* instructionHex;
    TCHAR* mnemonic;
    TCHAR* operands;
#if (defined(UNICODE)||defined(_UNICODE))
    SIZE_T Size;
    // convert instructionHex
    Size=strlen((char*)pDecodedInstructions->instructionHex.p)+1;
    instructionHex=(TCHAR*)_alloca(Size*sizeof(TCHAR));
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pDecodedInstructions->instructionHex.p,-1, instructionHex, Size);

    // convert mnemonic
    Size=strlen((char*)pDecodedInstructions->mnemonic.p)+1;
    mnemonic=(TCHAR*)_alloca(Size*sizeof(TCHAR));
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pDecodedInstructions->mnemonic.p,-1, mnemonic, Size);

    // convert operands
    Size=strlen((char*)pDecodedInstructions->operands.p)+1;
    operands=(TCHAR*)_alloca(Size*sizeof(TCHAR));
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pDecodedInstructions->operands.p,-1, operands, Size);
#else
    instructionHex=(char*)pDecodedInstructions->instructionHex.p;
    mnemonic=(char*)pDecodedInstructions->mnemonic.p;
    operands=(char*)pDecodedInstructions->operands.p;
#endif
    return pDisasmCallback(
                            pDecodedInstructions->offset,// doing this we can have bad offset when disasm 64bit from 32bit platform
                            pDecodedInstructions->size,
                            instructionHex,
                            mnemonic,
                            operands,
                            DisasmCallbackUserParam
                            );
}

// The number of the array of instructions the decoder function will use to return the disassembled instructions.
// Play with this value for performance...
#define MAX_INSTRUCTIONS (2048)

// ULONGLONG BaseAddress : address of first instruction (used only for displaying address, not asm instruction pointer)
// PBYTE Origin : pointer on first asm instruction
BOOL CDisasm::Disasm(ULONGLONG BaseAddress,PBYTE CodePointer,DWORD CodeSize,tagDecodeType DecodeType,pfDisasmCallback pDisasmCallback,PVOID DisasmCallbackUserParam)
{
    if (IsBadCodePtr((FARPROC)pDisasmCallback))
        return FALSE;
    // if (IsBadReadPtr(CodePointer,CodeSize)) return FALSE; // could be slow

    // Holds the result of the decoding.
    _DecodeResult res;
    // Decoded instruction information.
    _DecodedInst decodedInstructions[MAX_INSTRUCTIONS];
    // next is used for instruction's offset synchronization.
    // decodedInstructionsCount holds the count of filled instructions' array by the decoder.
    unsigned int decodedInstructionsCount = 0, i, next;

    // Default offset for buffer is 0, could be set in command line.
    _OffsetType offset = (_OffsetType)BaseAddress;

    // Decode the buffer at given offset (virtual address).
    for (;;) 
    {
        // If you get an unresolved external symbol linker error for the following line,
        // change the SUPPORT_64BIT_OFFSET in distorm.h.
        res = distorm_decode(offset, CodePointer, CodeSize,(_DecodeType)DecodeType, decodedInstructions, MAX_INSTRUCTIONS, &decodedInstructionsCount);
        // check for input buffer error
        if (res == DECRES_INPUTERR) 
            return FALSE;

        for (i = 0; i < decodedInstructionsCount; i++) 
        {
            if (!CDisasm::LocalDisasmCallback(&decodedInstructions[i],
                                            pDisasmCallback,
                                            DisasmCallbackUserParam
                                            )
                )
                break;
        }

        if (res == DECRES_SUCCESS) 
            break; // All instructions were decoded.
        else 
        {
            if (decodedInstructionsCount == 0) 
                break;
        }

        // Synchronize:
        next = (unsigned long)(decodedInstructions[decodedInstructionsCount-1].offset - offset);
        next += decodedInstructions[decodedInstructionsCount-1].size;
        // Advance ptr and recalc offset.
        CodePointer += next;
        CodeSize -= next;
        offset += next;
    }
    return TRUE;
}