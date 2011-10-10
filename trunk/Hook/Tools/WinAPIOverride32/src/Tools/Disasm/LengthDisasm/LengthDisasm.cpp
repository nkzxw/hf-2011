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

#include "LengthDisasm.h"

CLengthDisasmBase::CLengthDisasmBase()
{
    this->ResetVars();
}

CLengthDisasmBase::~CLengthDisasmBase(void)
{

}
void CLengthDisasmBase::ResetVars()
{
    this->LastDisasmResult=FALSE;

    this->length=0;
    this->flags=0;
    this->memsize=0;
    memset(this->mem,0,sizeof(this->mem));
    this->datasize=0;
    memset(this->data,0,sizeof(this->data));
    this->seg=0;
    this->rep=0;
    this->opcode=0;
    this->opcode2=0;
    this->modrm=0;
    this->sib=0; 
}

// print and move buffer
void CLengthDisasmBase::PrintByte(TCHAR** ppsz,BYTE b)
{
    _stprintf(*ppsz,_T("%.2X "),b);
    *ppsz+=3;
}

BOOL CLengthDisasmBase::ToString(TCHAR* pszString)
{
    if (!this->LastDisasmResult)
        return FALSE;

    TCHAR* psz=pszString;

    if (this->flags & C_66) 
        PrintByte(&psz,0x66);

    if (this->flags & C_67)
        PrintByte(&psz,0x67);

    if (this->flags & C_LOCK)
        PrintByte(&psz,0xF0);

    if (this->flags & C_REP)
        PrintByte(&psz,this->rep);

    if (this->flags & C_SEG)
        PrintByte(&psz,this->seg);

    PrintByte(&psz,this->opcode);

    if (this->flags & C_OPCODE2)
        PrintByte(&psz,this->opcode2);

    if (this->flags & C_MODRM)
        PrintByte(&psz,this->modrm);

    if (this->flags & C_SIB)
        PrintByte(&psz,this->sib);

    DWORD i;
    for (i=0; i<this->memsize;  i++)
        PrintByte(&psz,this->mem[i]);

    for (i=0; i<this->datasize; i++)
        PrintByte(&psz,this->data[i]);

    return TRUE;
}