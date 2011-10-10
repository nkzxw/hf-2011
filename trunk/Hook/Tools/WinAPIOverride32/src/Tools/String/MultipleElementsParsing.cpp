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
// Object: allow to convert a string with format "str1;str2;str3" to an array of string
//         or a string with format "1;4;6-9;14-20;22;33" to an array of DWORD
//-----------------------------------------------------------------------------

#include "multipleelementsparsing.h"

//-----------------------------------------------------------------------------
// Name: GetValue
// Object: get a value from it's string representation
// Parameters :
//     in  : TCHAR* psz : string containing number representation in decimal or hex
//     out : DWORD* pValue : value
//     return : array of pointer to elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
BOOL CMultipleElementsParsing::GetValue(TCHAR* psz,DWORD* pValue)
{
    BOOL bSuccess=TRUE;
    int iScanfRes;
    *pValue=0;

    if(_tcsnicmp(psz,_T("0x"),2)==0)
        iScanfRes=_stscanf(psz+2,_T("%x"),pValue);
    else
        iScanfRes=_stscanf(psz,_T("%u"),pValue);
    if ((iScanfRes<=0)||(!pValue))
        bSuccess=FALSE;

    return bSuccess;
}
//-----------------------------------------------------------------------------
// Name: ParseDWORD
// Object: return an array of DWORD containing elements
//         CALLER HAVE TO FREE ARRAY calling "delete[] Array;"
// Parameters :
//     in  : TCHAR* pszText : string containing list of numbers likes "1;4;6-9;14-20;22;33"
//                            meaning list {1,4,6,7,8,9,14,15,16,17,18,19,20,22,33}
//     out : DWORD* pdwArraySize : returned array size
//     return : array of pointer to elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
DWORD* CMultipleElementsParsing::ParseDWORD(TCHAR* pszText,DWORD* pdwArraySize)
{
    // NOTICE until we use atol, return values are only positive LONG values not DWORD
    CLinkListSimple* pLinkList;
    TCHAR* pszOldPos=pszText;
    TCHAR* pszNewPos;
    TCHAR* pszPosSplitter2;
    DWORD dwBegin;
    DWORD dwEnd;
    DWORD dwCnt;
    DWORD* RetArray;

    if (IsBadWritePtr(pdwArraySize,sizeof(DWORD)))
        return NULL;
    *pdwArraySize=0;

    if (IsBadReadPtr(pszText,1))
        return NULL;

    pLinkList=new CLinkListSimple();
    LONG strSize=(LONG)_tcslen(pszText);
    // while we can found ;
    while (strSize>(pszOldPos-pszText))
    {
        pszNewPos=_tcschr(pszOldPos,CMULTIPLEELEMENTSPARSING_MAJOR_SPLITTER_CHAR);

        // search -
        pszPosSplitter2=_tcschr(pszOldPos,CMULTIPLEELEMENTSPARSING_MINOR_SPLITTER_CHAR);
        // if - exists and is before next ;
        if ((pszPosSplitter2)&&((pszPosSplitter2<pszNewPos)||(pszNewPos==NULL))&&(pszPosSplitter2>pszOldPos))
        {
            // add list of number between dwBegin and dwEnd
            CMultipleElementsParsing::GetValue(pszOldPos,&dwBegin);
            CMultipleElementsParsing::GetValue(pszPosSplitter2+1,&dwEnd);
            for(dwCnt=dwBegin;dwCnt<=dwEnd;dwCnt++)
                pLinkList->AddItem((PVOID)dwCnt);
        }
        else
        {
            // add only the number
            CMultipleElementsParsing::GetValue(pszOldPos,&dwBegin);
            pLinkList->AddItem((PVOID)dwBegin);
        }

        pszOldPos=pszNewPos+1;
        // if it was the last ;
        if (!pszNewPos)
            break;
    }
    // link list to DWORD*
    RetArray=(DWORD*)pLinkList->ToArray(pdwArraySize);

    // free memory
    delete pLinkList;

    // return allocated array
    return RetArray;
}
//-----------------------------------------------------------------------------
// Name: ParseStringArrayFree
// Object: free memory allocated during a ParseString call
// Parameters :
//     in  : TCHAR** pArray : array return by ParseString call
//           DWORD dwArraySize : array size return by ParseString call
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CMultipleElementsParsing::ParseStringArrayFree(TCHAR** pArray,DWORD dwArraySize)
{
    DWORD dwCnt;

    if (!pArray)
        return;

    for (dwCnt=0;dwCnt<dwArraySize;dwCnt++)
        free(pArray[dwCnt]);
    delete[] pArray;
}
//-----------------------------------------------------------------------------
// Name: ParseString
// Object: return an array of DWORD containing elements
//         CALLER HAVE TO FREE ARRAY calling ParseStringArrayFree(TCHAR** pArray,DWORD dwArraySize)
// Parameters :
//     in  : TCHAR* pszText : string containing list of string likes "str1;str2;str3"
//     out : DWORD* pdwArraySize : returned array size
//     return : array of pointer to elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
TCHAR** CMultipleElementsParsing::ParseString(TCHAR* pszText,DWORD* pdwArraySize)
{
    DWORD dwNbItems;
    DWORD dw;
    DWORD dwCnt;
    TCHAR** pArray;
    TCHAR* psz;
    TCHAR* pszLocalText;

    if (IsBadWritePtr(pdwArraySize,sizeof(DWORD)))
        return NULL;

    *pdwArraySize=0;
    pArray=NULL;

    // count major splitter number in field
    dwNbItems=0;
    dw=(DWORD)_tcslen(pszText);
    for (dwCnt=0;dwCnt<dw;dwCnt++)
    {
        if (pszText[dwCnt]==CMULTIPLEELEMENTSPARSING_MAJOR_SPLITTER_CHAR)
            dwNbItems++;
    }
    // allocate memory
    pArray=new TCHAR*[dwNbItems+1];

    // copy text for not modifying pszText
    pszLocalText=_tcsdup(pszText);

    // for each field
    psz=_tcstok(pszLocalText,CMULTIPLEELEMENTSPARSING_MAJOR_SPLITTER_STRING);
    while(psz)
    {
        // copy it
        pArray[*pdwArraySize]=_tcsdup(psz);
        // increase pszFiltersInclusion size
        (*pdwArraySize)++;
        psz = _tcstok( NULL, CMULTIPLEELEMENTSPARSING_MAJOR_SPLITTER_STRING );
    }
    // free allocated text
    free(pszLocalText);

    return pArray;
}
