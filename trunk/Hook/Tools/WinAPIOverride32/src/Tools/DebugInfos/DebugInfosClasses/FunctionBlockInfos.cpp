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
// Object: manages debug informations for blocks
//-----------------------------------------------------------------------------
#include "functionblockinfos.h"

CFunctionBlockInfos::CFunctionBlockInfos(void)
{
    this->pLinkListVars=new CLinkListSimple();
    this->pLinkListBlocks=new CLinkListSimple();
    this->RelativeVirtualAddress=0;
    this->Length=0;
    this->SectionIndex=0;
    this->Offset=0;
}

CFunctionBlockInfos::~CFunctionBlockInfos(void)
{
    CLinkListItem* pItem;
    CFunctionBlockInfos* pFunctionBlockInfos;
    CTypeInfos* pVarInfos;

    // for each item of this->pLinkListParams
    for (pItem=this->pLinkListVars->Head;pItem;pItem=pItem->NextItem)
    {
        pVarInfos=(CTypeInfos*)pItem->ItemData;
        // delete item
        delete pVarInfos;
    }
    delete this->pLinkListVars;

    // for each item of this->pLinkListParams
    for (pItem=this->pLinkListBlocks->Head;pItem;pItem=pItem->NextItem)
    {
        pFunctionBlockInfos=(CFunctionBlockInfos*)pItem->ItemData;
        // delete item
        delete pFunctionBlockInfos;
    }
    delete this->pLinkListBlocks;
}

BOOL CFunctionBlockInfos::Parse(IDiaSymbol *pSymbol)
{
    DWORD dwLocationType;
    if(FAILED(pSymbol->get_locationType(&dwLocationType)))
        return FALSE;

    // function address should be static
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


    pSymbol->get_length(&this->Length);

    HRESULT hResult;
    IDiaEnumSymbols* pEnumChildren=NULL;

    hResult=pSymbol->findChildren(SymTagNull, NULL, nsNone, &pEnumChildren);
    if( FAILED(hResult) || (pEnumChildren==NULL) )
        return FALSE;

    DWORD dwSymTag;
    IDiaSymbol* pChild;
    ULONG celt = 0;

    while((pEnumChildren->Next(1, &pChild, &celt) == S_OK) && (celt == 1))
    {
        hResult=pChild->get_symTag(&dwSymTag);
        if(FAILED(hResult))
            return FALSE;

        // sym tag should be only local data and block
        switch (dwSymTag)
        {
        case SymTagBlock:
            CFunctionBlockInfos* pFunctionBlockInfos;
            pFunctionBlockInfos=new CFunctionBlockInfos();
            if (pFunctionBlockInfos->Parse(pChild))
                this->pLinkListBlocks->AddItem(pFunctionBlockInfos);
            else
                delete pFunctionBlockInfos;
            break;
        default: // blocks seems to contain types only
            {
                CTypeInfos* pTypeInfos=new CTypeInfos();
                if (pTypeInfos->Parse(pChild))
                    this->pLinkListVars->AddItem(pTypeInfos);
                else
                    delete pTypeInfos;
            }
            break;
        }
        pChild->Release();
    }
    pEnumChildren->Release();

    return TRUE;
}