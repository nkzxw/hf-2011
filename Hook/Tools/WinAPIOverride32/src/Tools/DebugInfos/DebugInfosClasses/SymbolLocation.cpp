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
// Object: manages debug informations for symbols location
//-----------------------------------------------------------------------------
#include "symbollocation.h"

extern DWORD g_RegistersMachineType;
CSymbolLocation::CSymbolLocation(void)
{
    this->RelativeVirtualAddress=0;
    this->SectionIndex=0;
    this->Offset=0;
    this->SymbolLocationType=LocIsNull;
    this->Register=0;
    this->RelativeOffset=0;
    this->BitFieldPos=0;
    this->Slot=0;
    this->BitFieldLen=0;
    this->ConstantValue.vt=VT_EMPTY;
}

CSymbolLocation::~CSymbolLocation(void)
{
}

TCHAR* CSymbolLocation::GetTypeString()
{
    switch(this->SymbolLocationType)
    {
    case LocIsNull:
        return _T("NULL");
    case LocIsStatic:
        return _T("static");
    case LocIsTLS:
        return _T("TLS");
    case LocIsRegRel:
        return _T("RegRel");
    case LocIsThisRel:
        return _T("ThisRel");
    case LocIsEnregistered:
        return _T("Enregistered");
    case LocIsBitField:
        return _T("BitField");
    case LocIsSlot:
        return _T("Slot");
    case LocIsIlRel:
        return _T("IL Relative");
    case LocInMetaData:
        return _T("In MetaData");
    case LocIsConstant:
        return _T("Constant");
    }
    return _T("Unknown");
}

// pszRegisterName should be at least TCHAR[64]
BOOL CSymbolLocation::GetRegisterName(TCHAR* pszRegisterName,DWORD pszRegisterNameMaxSize)
{
    switch(this->SymbolLocationType)
    {
    case LocIsRegRel:
    case LocIsEnregistered:
        {
            WCHAR* wcs=(WCHAR*)SzNameC7Reg((USHORT)this->Register,g_RegistersMachineType);
#if (defined(UNICODE)||defined(_UNICODE))
            _tcsncpy(pszRegisterName,wcs,pszRegisterNameMaxSize);
#else
            CAnsiUnicodeConvert::UnicodeToAnsi(wcs,pszRegisterName,pszRegisterNameMaxSize);
#endif
        }
        
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

BOOL CSymbolLocation::Parse(IDiaSymbol* pSymbol)
{
    if(FAILED(pSymbol->get_locationType(&this->SymbolLocationType)))
    {
        // It must be a symbol in optimized code
        return FALSE;
    }

    switch(this->SymbolLocationType)
    {
    case LocIsStatic: 
        // get relative virtual address
        // pSymbol->get_relativeVirtualAddress gives relative address from section
        // and pSymbol->get_virtualAddress gives relative address from image base <-- the one which interest us and is often called RVA
        pSymbol->get_virtualAddress(&this->RelativeVirtualAddress);
        pSymbol->get_addressSection(&this->SectionIndex);
        pSymbol->get_addressOffset(&this->Offset);
        // wprintf(L"%s, [0x%08x][0x%04x:0x%08x]",this->GetTypeString(), this->RelativeVirtualAddress, this->SectionIndex, this->Offset);
        break;
    case LocIsTLS:
    case LocInMetaData:
    case LocIsIlRel:
        pSymbol->get_addressSection(&this->SectionIndex);
        pSymbol->get_addressOffset(&this->Offset);
        // wprintf(L"%s, [0x%04x:0x%08x]",this->GetTypeString(), this->SectionIndex, this->Offset);
        break;
    case LocIsRegRel:
        pSymbol->get_registerId(&this->Register);
        pSymbol->get_offset(&this->RelativeOffset);
        // wprintf(L"%s Relative, [0x%08x]",SzNameC7Reg((USHORT)this->Register), this->RelativeOffset);
        break;
    case LocIsThisRel:
        pSymbol->get_offset(&this->RelativeOffset);
        // wprintf(L"this+0x%x", this->RelativeOffset);
        break;
    case LocIsBitField:
        pSymbol->get_offset(&this->RelativeOffset);
        pSymbol->get_bitPosition(&this->BitFieldPos);
        pSymbol->get_length(&this->BitFieldLen);
        // wprintf(L"this(bf)+0x%x:0x%x len(0x%x)",this->RelativeOffset, this->BitFieldPos, this->BitFieldLen);
        break;
    case LocIsEnregistered:
        pSymbol->get_registerId(&this->Register);
        // wprintf(L"enregistered %s", SzNameC7Reg((USHORT)this->Register));
        break;
    case LocIsNull:
        // wprintf(L"pure");
        break;
    case LocIsSlot:
        pSymbol->get_slot(&this->Slot);
        // wprintf(L"%s, [0x%08x]", this->GetTypeString(), this->Slot);
        break;
    case LocIsConstant:
        // wprintf(L"constant");
        pSymbol->get_value(&this->ConstantValue);
        // PrintVariant(this->ConstantValue);
        break;
    default :
        return FALSE;
    }
    return TRUE;
}