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

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "malloc.h"
#include "distorm64/distorm.h"
#pragma once

class CDisasm
{
public:
    typedef enum {Decode16Bits = 0, Decode32Bits = 1, Decode64Bits = 2} tagDecodeType;
    typedef BOOL (*pfDisasmCallback)(ULONGLONG InstructionAddress,DWORD InstructionsSize,TCHAR* InstructionHex,TCHAR* Mnemonic,TCHAR* Operands,PVOID UserParam);
    //// pfDisasmCallback content can be something like
    //printf("0x%p (%02d) %-24s %s%s%s\r\n", 
    //        InstructionAddress,
    //        InstructionsSize,
    //        InstructionHex,
    //        Mnemonic,
    //        *Operands != 0 ? " " : "",
    //        Operands);

    static BOOL Disasm(ULONGLONG BaseAddress,PBYTE CodePointer,DWORD CodeSize,tagDecodeType DecodeType,pfDisasmCallback pDisasmCallback,PVOID DisasmCallbackUserParam);
private:
    static BOOL LocalDisasmCallback(_DecodedInst* pDecodedInstructions,pfDisasmCallback pDisasmCallback,PVOID DisasmCallbackUserParam);
};
