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
// Object: get WinAPIOverride function description from an IDispatch parsing result
//-----------------------------------------------------------------------------

#include "getidispatchwinapioverridefunctionsrepresentation.h"


//-----------------------------------------------------------------------------
// Name: GetIDispatchWinApiOverrideFunctionsRepresentation
// Object:  get WinAPIOverride function representation like from an IDispatch parsing result
// Parameters :
//     in  : CHookedClass* pHookedClass
//           pfIDispatchFunctionsRepresentation pIDispatchFunctionsRepresentation : callback for function representation
//           BOOL AddAdvancedInformation : TRUE if IDispatch advanced information must be display 
//                                         if TRUE, representation is incompatible with real monitoring file syntax
//                                         if FALSE, representation is compatible with real monitoring file syntax
//           LPVOID UserParam : user param
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CGetIDispatchWinApiOverrideFunctionsRepresentation::GetIDispatchWinApiOverrideFunctionsRepresentation(CHookedClass* pHookedClass,pfIDispatchFunctionsRepresentation pIDispatchFunctionsRepresentation,BOOL AddAdvancedInformation,LPVOID UserParam)
{
    if(!pHookedClass->bIDispatchParsingSuccessFull)
        return FALSE;

    if (!pHookedClass->pInterfaceExposedByIDispatch)
        return FALSE;

    CLinkListItem* pItemMethod;
    CMethodInfo* pMethodInfo;
    TCHAR pszFuncDesc[2048];

    // for each method of IDispatch
    pHookedClass->pInterfaceExposedByIDispatch->pMethodInfoList->Lock();
    for(pItemMethod=pHookedClass->pInterfaceExposedByIDispatch->pMethodInfoList->Head;pItemMethod;pItemMethod=pItemMethod->NextItem)
    {
        pMethodInfo=(CMethodInfo*)pItemMethod->ItemData;
        // assume pointer validity
        if (IsBadReadPtr(pMethodInfo,sizeof(CMethodInfo)))
            continue;

        // get text content
        pMethodInfo->ToString(AddAdvancedInformation,pszFuncDesc);

        // call call back
        if (!pIDispatchFunctionsRepresentation(pszFuncDesc,pMethodInfo,UserParam))
            // if callback wants to stop parsing
            break;
    }
    pHookedClass->pInterfaceExposedByIDispatch->pMethodInfoList->Unlock();

    return TRUE;
}
