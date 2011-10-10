/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

Adapted from
Z0mbie
http://z0mbie.host.sk/

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
// Object: Disasm base interface
//-----------------------------------------------------------------------------

#pragma once

#include <Windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)


// disasm_flag values:
#define C_66           0x00000001       // 66-prefix
#define C_67           0x00000002       // 67-prefix
#define C_LOCK         0x00000004       // lock
#define C_REP          0x00000008       // repz/repnz
#define C_SEG          0x00000010       // seg-prefix
#define C_OPCODE2      0x00000020       // 2nd opcode present (1st==0F)
#define C_MODRM        0x00000040       // modrm present
#define C_SIB          0x00000080       // sib present
#define C_ANYPREFIX    (C_66|C_67|C_LOCK|C_REP|C_SEG)

class CLengthDisasmBase
{
protected:
    void ResetVars();
    void PrintByte(TCHAR** ppsz,BYTE b);
    BOOL LastDisasmResult;
public:
    DWORD  length; // total instruction length in bytes, 0 if error 
    DWORD  flags;  // bitmask, flags, see C_xxx
                    // C_66  there is 66-prefix 
                    // C_67  there is 67-prefix 
                    // C_LOCK  there is LOCK-prefix (F0) 
                    // C_REP  there is REPZ- or REPNZ-prefix, exact value in disasm_rep 
                    // C_SEG  there is seg-prefix, exact value in disasm_seg 
                    // C_OPCODE2  there is 2nd opcode (1st one is 0x0F), value in disasm_opcode2 
                    // C_MODRM  there is modrm, value in disasm_modrm 
                    // C_SIB  there is sib, value in disasm_sib 
     
    DWORD memsize; // length of the memory address, if used in instruction, value in disasm_mem 
    BYTE  mem[16]; // memory address (length in disasm_memsize) 
    DWORD datasize; // length of data, used in instructions (in bytes), value in disasm_data 
    BYTE  data[16]; // data (length in disasm_datasize) 
    BYTE  seg; // C_SEG: seg-prefix (CS DS ES SS FS GS) 
    BYTE  rep; // C_REP: rep-prefix REPZ/REPNZ 
    BYTE  opcode; // opcode itself, not depending on flags 
    BYTE  opcode2; // C_OPCODE2: 2nd opcode (if 1st one is 0x0F) 
    BYTE  modrm; // C_MODRM: value of modxxxrm 
    BYTE  sib; // C_SIB: value of sib (scale-index-base) 

    CLengthDisasmBase();
    ~CLengthDisasmBase();
    
    virtual BOOL Disasm(BYTE* opcode0)=0;
    BOOL ToString(TCHAR* pszString);
};
