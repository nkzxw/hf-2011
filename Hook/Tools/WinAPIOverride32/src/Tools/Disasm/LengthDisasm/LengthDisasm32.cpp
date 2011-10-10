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
// Object: provides a 32bit len disasm
//-----------------------------------------------------------------------------

#include "LengthDisasm32.h"

CLengthDisasm32::CLengthDisasm32()
{
}

CLengthDisasm32::~CLengthDisasm32(void)
{

}


BOOL CLengthDisasm32::Disasm(BYTE* opcode0)
{
    this->LastDisasmResult=FALSE;

    BYTE* opcode = opcode0;

    DWORD defdata; // == C_66 ? 2 : 4
    DWORD defmem;  // == C_67 ? 2 : 4


    this->length = 0;
    this->flags = 0;
    this->datasize = 0;
    this->memsize = 0;
    defdata = 4;
    defmem = 4;

retry:
    this->opcode = *opcode++;

    switch (this->opcode)
    {
    case 0x00: case 0x01: case 0x02: case 0x03:
    case 0x08: case 0x09: case 0x0A: case 0x0B:
    case 0x10: case 0x11: case 0x12: case 0x13:
    case 0x18: case 0x19: case 0x1A: case 0x1B:
    case 0x20: case 0x21: case 0x22: case 0x23:
    case 0x28: case 0x29: case 0x2A: case 0x2B:
    case 0x30: case 0x31: case 0x32: case 0x33:
    case 0x38: case 0x39: case 0x3A: case 0x3B:
    case 0x62: case 0x63:
    case 0x84: case 0x85: case 0x86: case 0x87:
    case 0x88: case 0x89: case 0x8A: case 0x8B:
    case 0x8C: case 0x8D: case 0x8E: case 0x8F:
    case 0xC4: case 0xC5:
    case 0xD0: case 0xD1: case 0xD2: case 0xD3:
    case 0xD8: case 0xD9: case 0xDA: case 0xDB:
    case 0xDC: case 0xDD: case 0xDE: case 0xDF:
    case 0xFE: case 0xFF:
        this->flags |= C_MODRM;
        break;
    case 0xCD: this->datasize += *opcode==0x20 ? 1+4 : 1;
        break;
    case 0xF6:
    case 0xF7: this->flags |= C_MODRM;
        if (*opcode & 0x38) break;
        // continue if <test ..., xx>
    case 0x04: case 0x05: case 0x0C: case 0x0D:
    case 0x14: case 0x15: case 0x1C: case 0x1D:
    case 0x24: case 0x25: case 0x2C: case 0x2D:
    case 0x34: case 0x35: case 0x3C: case 0x3D:
        if (this->opcode & 1)
            this->datasize += defdata;
        else
            this->datasize++;
        break;
    case 0x6A:
    case 0xA8:
    case 0xB0: case 0xB1: case 0xB2: case 0xB3:
    case 0xB4: case 0xB5: case 0xB6: case 0xB7:
    case 0xD4: case 0xD5:
    case 0xE4: case 0xE5: case 0xE6: case 0xE7:
    case 0x70: case 0x71: case 0x72: case 0x73:
    case 0x74: case 0x75: case 0x76: case 0x77:
    case 0x78: case 0x79: case 0x7A: case 0x7B:
    case 0x7C: case 0x7D: case 0x7E: case 0x7F:
    case 0xEB:
    case 0xE0: case 0xE1: case 0xE2: case 0xE3:
        this->datasize++;
        break;
    case 0x26: case 0x2E: case 0x36: case 0x3E:
    case 0x64: case 0x65:
        if (this->flags & C_SEG) return FALSE;
        this->flags |= C_SEG;
        this->seg = this->opcode;
        goto retry;
    case 0xF0:
        if (this->flags & C_LOCK) return FALSE;
        this->flags |= C_LOCK;
        goto retry;
    case 0xF2: case 0xF3:
        if (this->flags & C_REP) return FALSE;
        this->flags |= C_REP;
        this->rep = this->opcode;
        goto retry;
    case 0x66:
        if (this->flags & C_66) return FALSE;
        this->flags |= C_66;
        defdata = 2;
        goto retry;
    case 0x67:
        if (this->flags & C_67) return FALSE;
        this->flags |= C_67;
        defmem = 2;
        goto retry;
    case 0x6B:
    case 0x80:
    case 0x82:
    case 0x83:
    case 0xC0:
    case 0xC1:
    case 0xC6: this->datasize++;
        this->flags |= C_MODRM;
        break;
    case 0x69:
    case 0x81:
    case 0xC7:
        this->datasize += defdata;
        this->flags |= C_MODRM;
        break;
    case 0x9A:
    case 0xEA: this->datasize += 2 + defdata;
        break;
    case 0xA0:
    case 0xA1:
    case 0xA2:
    case 0xA3: this->memsize += defmem;
        break;
    case 0x68:
    case 0xA9:
    case 0xB8: case 0xB9: case 0xBA: case 0xBB:
    case 0xBC: case 0xBD: case 0xBE: case 0xBF:
    case 0xE8:
    case 0xE9:
        this->datasize += defdata;
        break;
    case 0xC2:
    case 0xCA: this->datasize += 2;
        break;
    case 0xC8:
        this->datasize += 3;
        break;
    case 0xF1:
        return FALSE;
    case 0x0F:
        this->flags |= C_OPCODE2;
        this->opcode2 = *opcode++;
        switch (this->opcode2)
        {
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x90: case 0x91: case 0x92: case 0x93:
        case 0x94: case 0x95: case 0x96: case 0x97:
        case 0x98: case 0x99: case 0x9A: case 0x9B:
        case 0x9C: case 0x9D: case 0x9E: case 0x9F:
        case 0xA3:
        case 0xA5:
        case 0xAB:
        case 0xAD:
        case 0xAF:
        case 0xB0: case 0xB1: case 0xB2: case 0xB3:
        case 0xB4: case 0xB5: case 0xB6: case 0xB7:
        case 0xBB:
        case 0xBC: case 0xBD: case 0xBE: case 0xBF:
        case 0xC0:
        case 0xC1:
            this->flags |= C_MODRM;
            break;
        case 0x06:
        case 0x08: case 0x09: case 0x0A: case 0x0B:
        case 0xA0: case 0xA1: case 0xA2: case 0xA8:
        case 0xA9:
        case 0xAA:
        case 0xC8: case 0xC9: case 0xCA: case 0xCB:
        case 0xCC: case 0xCD: case 0xCE: case 0xCF:
            break;
        case 0x80: case 0x81: case 0x82: case 0x83:
        case 0x84: case 0x85: case 0x86: case 0x87:
        case 0x88: case 0x89: case 0x8A: case 0x8B:
        case 0x8C: case 0x8D: case 0x8E: case 0x8F:
            this->datasize += defdata;
            break;
        case 0xA4:
        case 0xAC:
        case 0xBA:
            this->datasize++;
            this->flags |= C_MODRM;
            break;
        default:
            return FALSE;
        } // 0F-switch
        break;

    } //switch

    if (this->flags & C_MODRM)
    {
        this->modrm = *opcode++;
        BYTE mod = this->modrm & 0xC0;
        BYTE rm  = this->modrm & 0x07;
        if (mod != 0xC0)
        {
            if (mod == 0x40) this->memsize++;
            if (mod == 0x80) this->memsize += defmem;
            if (defmem == 2)           // modrm16
            {
                if ((mod == 0x00)&&(rm == 0x06)) this->memsize+=2;
            }
            else                              // modrm32
            {
                if (rm==0x04)
                {
                    this->flags |= C_SIB;
                    this->sib = *opcode++;
                    rm = this->sib & 0x07;
                }
                if ((rm==0x05)&&(mod==0x00)) this->memsize+=4;
            }
        }
    } // C_MODRM

    DWORD i;
    for(i=0; i<this->memsize; i++)
        this->mem[i] = *opcode++;
    for(i=0; i<this->datasize; i++)
        this->data[i] = *opcode++;

    this->length = (DWORD)(opcode - opcode0);

    this->LastDisasmResult=TRUE;

    return TRUE;

}
