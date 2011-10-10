/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
originally based from APISpy32 v2.1 from Yariv Kaplan @ WWW.INTERNALS.COM

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
// Object: registers
//-----------------------------------------------------------------------------
#pragma once

#include <windows.h>

#pragma pack(push)
#pragma pack(4)
typedef struct tagRegisters
{
    DWORD eax;
    DWORD ebx;
    DWORD ecx;
    DWORD edx;
    DWORD esi;
    DWORD edi;
    DWORD efl;
    DWORD es;
    DWORD fs;
    DWORD gs;
    DWORD esp;
    DWORD ebp;
}REGISTERS,*PREGISTERS;

#pragma pack(pop)