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
// Object: try to find function (name + start address) from dll name and relative address
//-----------------------------------------------------------------------------

#include "DllFindFuncNameFromRVA.h"

//-----------------------------------------------------------------------------
// Name: FindFuncNameFromRVA
// Object: try to find function (name + start address) from dll name and relative address
// Parameters :
//     in  : TCHAR* DllName : name of the dll
//           PBYTE RelativeAddress : relative address for which we wan't to try to get name
//           SIZE_T MaxpszFuncNameSize : max size in TCHAR of pszFuncName
//     out : TCHAR* pszFuncName : MAY the function name (only disassembly can assume it)
//           PBYTE* pFunctionStartAddress : associated found function start address
//     return : FALSE if no function name can be found
//-----------------------------------------------------------------------------
BOOL CDllFindFuncNameFromRVA::FindFuncNameFromRVA(TCHAR* DllName,PBYTE RelativeAddress, OUT TCHAR* pszFuncName,SIZE_T MaxpszFuncNameSize,OUT PBYTE* pFunctionStartAddress)
{
    // parse export table of the dll
    CPE Pe(DllName);
    if (!Pe.Parse(TRUE,FALSE))
        return FALSE;
    return CDllFindFuncNameFromRVA::FindFuncNameFromRVA(&Pe,RelativeAddress,pszFuncName,MaxpszFuncNameSize,pFunctionStartAddress);
}

//-----------------------------------------------------------------------------
// Name: FindFuncNameFromRVA
// Object: try to find function (name + start address) from dll name and relative address
// Parameters :
//     in  : CPE* pPe : pointer to a CPE object with export table already parsed
//           PBYTE RelativeAddress : relative address for which we wan't to try to get name
//           SIZE_T MaxpszFuncNameSize : max size in TCHAR of pszFuncName
//     out : TCHAR* pszFuncName : MAY the function name (only disassembly can assume it)
//           PBYTE* pFunctionStartAddress : associated found function start address
//     return : FALSE if no function name can be found
//-----------------------------------------------------------------------------
BOOL CDllFindFuncNameFromRVA::FindFuncNameFromRVA(CPE* pPe,PBYTE RelativeAddress, OUT TCHAR* pszFuncName,SIZE_T MaxpszFuncNameSize,OUT PBYTE* pFunctionStartAddress)
{
    *pFunctionStartAddress=0;
    // find the nearest lower exported function
    CPE::EXPORT_FUNCTION_ITEM* pNearestLower=NULL;
    CLinkListItem* pItem;
    pItem=pPe->pExportTable->Head;
    CPE::EXPORT_FUNCTION_ITEM* pCurrent;
    while(pItem)
    {
        pCurrent=(CPE::EXPORT_FUNCTION_ITEM*)pItem->ItemData;

        // if relative address of function is less than called address
        if ((PBYTE)pCurrent->FunctionAddressRVA<=RelativeAddress)
        {
            // if pNearestLower has already been defined
            if (pNearestLower==NULL)
                pNearestLower=pCurrent;
            // if relative address is upper than pNearestLower address
            else if (pCurrent->FunctionAddressRVA>(DWORD)pNearestLower->FunctionAddressRVA)
                pNearestLower=pCurrent;
        }

        pItem=pItem->NextItem;
    }

    if (!pNearestLower)
        return FALSE;

    // copy func name to pszFuncName
    _tcsncpy(pszFuncName,pNearestLower->FunctionName,MaxpszFuncNameSize-1);

    // if function has no name
    if (*pszFuncName==0)
        _sntprintf(pszFuncName,MaxpszFuncNameSize-1,_T("function ordinal 0x%X"),pNearestLower->ExportedOrdinal);

    *pFunctionStartAddress=(PBYTE)pNearestLower->FunctionAddressRVA;

    return TRUE;
}