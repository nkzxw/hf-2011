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
// Object: manages debug informations for modules
//-----------------------------------------------------------------------------
#include "moduleinfos.h"

CModuleInfos::CModuleInfos(void)
{
    this->Name=NULL;
    this->pLinkListFunctions=new CLinkListSimple();
    this->pLinkListLabels=new CLinkListSimple();
    this->pLinkListThunks=new CLinkListSimple();
}

CModuleInfos::~CModuleInfos(void)
{
    if (this->Name)
        free(this->Name);


    CLinkListItem* pItem;
    CFunctionInfos* pFunctionInfos;
    CLabelInfos* pLabelInfos;
    CThunkInfos* pThunkInfos;

    // for each item of this->pLinkListFunctions
    for (pItem=this->pLinkListFunctions->Head;pItem;pItem=pItem->NextItem)
    {
        pFunctionInfos=(CFunctionInfos*)pItem->ItemData;
        // delete item
        delete pFunctionInfos;
    }

    // delete pLinkListFunctions
    delete this->pLinkListFunctions;

    // for each item of this->pLinkListLabels
    for (pItem=this->pLinkListLabels->Head;pItem;pItem=pItem->NextItem)
    {
        pLabelInfos=(CLabelInfos*)pItem->ItemData;
        // delete item
        delete pLabelInfos;
    }

    // for each item of this->pLinkListLabels
    for (pItem=this->pLinkListThunks->Head;pItem;pItem=pItem->NextItem)
    {
        pThunkInfos=(CThunkInfos*)pItem->ItemData;
        // delete item
        delete pThunkInfos;
    }

    // delete pLinkListLabels
    delete this->pLinkListLabels;


    delete this->pLinkListThunks;
}

HRESULT CModuleInfos::Parse(IDiaSymbol *pSymbol)
{
    HRESULT hr;
    DWORD dwSymTag;

    hr=pSymbol->get_symTag(&dwSymTag);
    if(FAILED(hr))
        return hr;

    switch(dwSymTag)
    {
    case SymTagCompilandDetails:
        // provide information on module (Compilation language, target processor, compiler version)
        // PrintCompilandDetails(pSymbol);
        break;
    case SymTagCompilandEnv:
        // provide full path
        // PrintCompilandEnv(pSymbol);
        break;
    case SymTagFunction: // len : function code + function static vars
        {
            CFunctionInfos* pFunctionInfos;
            pFunctionInfos=new CFunctionInfos();
            if (pFunctionInfos->Parse(pSymbol))
                this->pLinkListFunctions->AddItem(pFunctionInfos);
            else
                delete pFunctionInfos;
        }
        
        break;
    case SymTagThunk:
        {
            CThunkInfos* pThunkInfos;
            // thunk are external functions, no parameters information provided 
            pThunkInfos=new CThunkInfos();
            if (pThunkInfos->Parse(pSymbol))
                this->pLinkListThunks->AddItem(pThunkInfos);
            else
                delete pThunkInfos;
        }
        break;
    case SymTagTypedef:
    case SymTagData:
        break;
    case SymTagLabel:
        {
            CLabelInfos* pLabelInfos=new CLabelInfos();
            if (pLabelInfos->Parse(pSymbol))
                this->pLinkListLabels->AddItem(pLabelInfos);
            else
                delete pLabelInfos;
        }
        break;
    default:
#ifdef _DEBUG
        if (IsDebuggerPresent())
            DebugBreak();
#endif
        break;
    }

    return TRUE;
}
