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
// Object: manages debug informations for thunks
//-----------------------------------------------------------------------------
#include "thunkinfos.h"

CThunkInfos::CThunkInfos(void)
{
    this->Name=0;
    this->Offset=0;
    this->SectionIndex=0;
    this->Length=0;
    this->ThunkOrdinal=0;
    this->TargetOffset=0;
    this->TargetSectionIndex=0;
    this->TargetRelativeVirtualAddress=0;
}

CThunkInfos::~CThunkInfos(void)
{
    if (this->Name)
        free(this->Name);
}

TCHAR* CThunkInfos::GetOrdinalString()
{
    switch(this->ThunkOrdinal)
    {
    case THUNK_ORDINAL_NOTYPE:
        return _T("Standard thunk");
    case THUNK_ORDINAL_ADJUSTOR:
        return _T("A this adjustor thunk");
    case THUNK_ORDINAL_VCALL:
        return _T("Virtual call thunk");
    case THUNK_ORDINAL_PCODE:
        return _T("P-code thunk");
    case THUNK_ORDINAL_LOAD:
        return _T("Delay load thunk");
    }

    return _T("Unknown");
}

BOOL CThunkInfos::Parse(IDiaSymbol* pSymbol)
{
    // get name
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

    // get thunk infos
    pSymbol->get_addressSection(&this->SectionIndex);
    pSymbol->get_addressOffset(&this->Offset);
    pSymbol->get_thunkOrdinal(&this->ThunkOrdinal);
    pSymbol->get_length(&this->Length);

    // get target infos
    pSymbol->get_targetSection(&this->TargetSectionIndex);
    pSymbol->get_targetOffset(&this->TargetOffset);
    // pSymbol->get_targetRelativeVirtualAddress RVA from section index
    // RVA from image base
    pSymbol->get_targetVirtualAddress(&this->TargetRelativeVirtualAddress);

    return TRUE;
}