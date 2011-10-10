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
// Object: try to get call address from return address
//-----------------------------------------------------------------------------
#include "CallInstructionSizeFromRetAddress.h"

#ifdef _DEBUG
    #include <stdio.h>
#endif

//-----------------------------------------------------------------------------
// Name: GetCallInstructionSize
// Object: try to get call instruction size from a quick table check
// Parameters :
//      in: PBYTE ReturnAddress : return address
// Return : size of call instruction before the return address
//-----------------------------------------------------------------------------
DWORD CCallInstructionSizeFromReturnAddress::GetCallInstructionSizeFromReturnAddress(PBYTE ReturnAddress)
{
#ifdef _WIN64
    TO_BE_IMPLEMENTED
#else

    // assume memory around return address is readable
    if (IsBadReadPtr(ReturnAddress-CCallInstructionSizeFromReturnAddress_ASM_CALL_INSRUCTION_MAX_SIZE,CCallInstructionSizeFromReturnAddress_ASM_CALL_INSRUCTION_MAX_SIZE))
        return 0; // keep return address

    BYTE RetAddress_7=*(ReturnAddress-7);
    BYTE RetAddress_6=*(ReturnAddress-6);
    BYTE RetAddress_5=*(ReturnAddress-5);
    BYTE RetAddress_4=*(ReturnAddress-4);
    BYTE RetAddress_3=*(ReturnAddress-3);
    BYTE RetAddress_2=*(ReturnAddress-2);
    BYTE RetAddress_1=*(ReturnAddress-1);

    //E8 CALL rel32 Call near, relative, displacement relative to next instruction
    if (RetAddress_5==0xE8)
        return 5;

    // FF 15 +4 bytes
    // FF 1D +4 bytes
    if ( (RetAddress_6==0xFF) && ( (RetAddress_5==0x15) || (RetAddress_5==0x1D) ) )
        return 6;

    // FF 10 call [eax]
    // FF 11 call [ecx]
    // FF D0 Call eax
    // FF D1 call ecx
    // FF D2 call edx
    // FF D3 call ebx
    // FF D4 call esp
    // FF D5 call ebp
    // FF D6 call esi
    // FF D7 call edi
    if ( (RetAddress_2==0xFF) 
         && (     ( (RetAddress_1>=0xD0) && (RetAddress_1<0xD8) )
               || ( (RetAddress_1>=0x10) && (RetAddress_1<0x20) )
            )
        )
        return 2;

    // FF 14 +5 bytes
    // FF 94 +5 bytes
    // FF 1C +5 bytes
    // FF 9C +5 bytes
    if ( (RetAddress_7==0xFF) && ( (RetAddress_6==0x14) || (RetAddress_6==0x94) || (RetAddress_6==0x9C) ) )
        return 7;

    //ff 50 byte call [eax+byte]
    //ff 51 byte call [ecx+byte]
    //ff 52 byte call [edx+byte]
    //ff 53 byte call [ebx+byte]

    //ff 55 byte call [ebp+byte]
    //ff 56 byte call [esi+byte]
    //ff 57 byte call [edi+byte]
    //ff 58 byte call far [eax+byte]
    if ( (RetAddress_3==0xFF) 
         && (    ( (RetAddress_2>=0x50) && (RetAddress_2<0x60) )
              || (RetAddress_2==0x14)
              || (RetAddress_2==0x1C)
             )
        )
        return 3;

    // ff 54 +2 bytes
    // ff 5C +2 bytes
    if (    (RetAddress_4==0xFF) 
         && ( (RetAddress_3==0x54) || (RetAddress_3==0x5C) ) )
        return 4;

    // FF 90 dword call [eax+dword]
    // FF 91 dword call [ecx+dword]
    if ( (RetAddress_6==0xFF) && ( (RetAddress_5>=0x90) && (RetAddress_5<0xa0) ) )
        return 6;

    // 9a word dword call word:dword
    if (RetAddress_7==0x9a)
        return 7;
#endif

#ifdef _DEBUG
    WCHAR Msg[256];
    WCHAR wcsb[64];
    BYTE b;
    swprintf(Msg,L"Can't find call sequence at address 0x%p in :",ReturnAddress);
    
    for (DWORD Cnt=0;Cnt<CCallInstructionSizeFromReturnAddress_ASM_CALL_INSRUCTION_MAX_SIZE;Cnt++)
    {
        b=*(ReturnAddress-(CCallInstructionSizeFromReturnAddress_ASM_CALL_INSRUCTION_MAX_SIZE-Cnt));
        swprintf(wcsb,L" 0x%.2X",b);
        wcscat(Msg,wcsb);
    }

    wcscat(Msg,L"\r\n");
    OutputDebugStringW(Msg);

#endif
    // no information found --> keep return address
    return 0;
}