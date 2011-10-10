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
// Object: manages debug informations for labels
//-----------------------------------------------------------------------------
#include "labelinfos.h"

CLabelInfos::CLabelInfos(void)
{
    this->Name=NULL;
    this->RelativeVirtualAddress=0;
    this->SectionIndex=0;
    this->Offset=0;
}

CLabelInfos::~CLabelInfos(void)
{
    if (this->Name)
        free(this->Name);
}

BOOL CLabelInfos::Parse(IDiaSymbol *pSymbol)
{
    DWORD dwLocationType;
    if(FAILED(pSymbol->get_locationType(&dwLocationType)))
        return FALSE;

    // label address should be static
    if (dwLocationType==LocIsStatic)
    {
        // get relative virtual address
        // pSymbol->get_relativeVirtualAddress gives relative address from section
        // and pSymbol->get_virtualAddress gives relative address from image base <-- the one which interest us and is often called RVA
        pSymbol->get_virtualAddress(&this->RelativeVirtualAddress);
        pSymbol->get_addressSection(&this->SectionIndex);
        pSymbol->get_addressOffset(&this->Offset);
    }
#ifdef _DEBUG
    else
    {
        if (IsDebuggerPresent())
            DebugBreak();
    }
#endif
    BSTR Name=NULL;
    pSymbol->get_name(&Name);
    if (Name)
    {
#if (defined(UNICODE)||defined(_UNICODE))
        this->Name=_tcsdup(Name);
#else
        CAnsiUnicodeConvert::UnicodeToAnsi(Name,&this->Name);
#endif
        SysFreeString(Name);
    }
    else
        this->Name=_tcsdup(_T(""));

    return TRUE;
}