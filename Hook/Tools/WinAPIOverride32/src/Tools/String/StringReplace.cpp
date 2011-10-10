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
// Object: replace a string by another one
//-----------------------------------------------------------------------------

#include "StringReplace.h"
//-----------------------------------------------------------------------------
// Name: Replace
// Object: replace pszOldStr by pszNewStr in pszInputString and put the result in 
//         pszOutputString.
//         Warning pszOutputString must be large enough no check is done
// Parameters :
//     in  : TCHAR* pszInputString : string to translate
//           TCHAR* pszOldStr : string to replace
//           TCHAR* pszNewStr : replacing string
//     out : TCHAR* pszOutputString : result string
//     return : 
//-----------------------------------------------------------------------------
void Replace(TCHAR* pszInputString,TCHAR* pszOutputString,TCHAR* pszOldStr,TCHAR* pszNewStr)
{
    return CStringReplace::Replace(pszInputString,pszOutputString,pszOldStr,pszNewStr,TRUE);
}

//-----------------------------------------------------------------------------
// Name: Replace
// Object: replace pszOldStr by pszNewStr in pszInputString and put the result in 
//         pszOutputString.
//         Warning pszOutputString must be large enough no check is done
// Parameters :
//     in  : TCHAR* pszInputString : string to translate
//           TCHAR* pszOldStr : string to replace
//           TCHAR* pszNewStr : replacing string
//           BOOL CaseSensitive : TRUE for case sensitive search, FALSE for case insensitive search
//     out : TCHAR* pszOutputString : result string
//     return : 
//-----------------------------------------------------------------------------
void CStringReplace::Replace(TCHAR* pszInputString,TCHAR* pszOutputString,TCHAR* pszOldStr,TCHAR* pszNewStr,BOOL CaseSensitive)
{
    TCHAR* pszPos;
    TCHAR* pszOldPos;
    SIZE_T SearchedItemSize;
    TCHAR* pszLocalInputString;
    TCHAR* pszLocalOldStr;

    if ((!pszInputString)||(!pszOutputString)||(!pszOldStr)||(!pszNewStr))
        return;

    *pszOutputString=0;

    if (CaseSensitive)
    {
        pszLocalInputString=pszInputString;
        pszLocalOldStr=pszOldStr;
    }
    else
    {
        pszLocalInputString=_tcsdup(pszInputString);
        _tcsupr(pszLocalInputString);
        pszLocalOldStr=_tcsdup(pszOldStr);
        _tcsupr(pszLocalOldStr);
    }

    pszOldPos=pszLocalInputString;
    // get searched item size
    SearchedItemSize=_tcslen(pszOldStr);
    // look for next string to replace
    pszPos=_tcsstr(pszLocalInputString,pszLocalOldStr);
    while(pszPos)
    {
        // copy unchanged data
        _tcsncat(pszOutputString,pszInputString+ (pszOldPos - pszLocalInputString),pszPos-pszOldPos);
        // copy replace string
        _tcscat(pszOutputString,pszNewStr);
        // update old position
        pszOldPos=pszPos+SearchedItemSize;
        // look for next string to replace
        pszPos=_tcsstr(pszOldPos,pszOldStr);
    }
    // copy remaining data
    _tcscat(pszOutputString,pszInputString+ (pszOldPos - pszLocalInputString));

    if (!CaseSensitive)
    {
        free(pszLocalInputString);
        free(pszLocalOldStr);
    }
}

//-----------------------------------------------------------------------------
// Name: ReplaceW
// Object: replace pszOldStr by pszNewStr in pszInputString and put the result in 
//         pszOutputString.
//         Warning pszOutputString must be large enough no check is done
// Parameters :
//     in  : WCHAR* pszInputString : string to translate
//           WCHAR* pszOldStr : string to replace
//           WCHAR* pszNewStr : replacing string
//     out : WCHAR* pszOutputString : result string
//     return : 
//-----------------------------------------------------------------------------
void CStringReplace::ReplaceW(WCHAR* pszInputString,WCHAR* pszOutputString,WCHAR* pszOldStr,WCHAR* pszNewStr)
{
    return CStringReplace::ReplaceW(pszInputString,pszOutputString,pszOldStr,pszNewStr,TRUE);
}

//-----------------------------------------------------------------------------
// Name: ReplaceW
// Object: replace pszOldStr by pszNewStr in pszInputString and put the result in 
//         pszOutputString.
//         Warning pszOutputString must be large enough no check is done
// Parameters :
//     in  : WCHAR* pszInputString : string to translate
//           WCHAR* pszOldStr : string to replace
//           WCHAR* pszNewStr : replacing string
//           BOOL CaseSensitive : TRUE for case sensitive search, FALSE for case insensitive search
//     out : WCHAR* pszOutputString : result string
//     return : 
//-----------------------------------------------------------------------------
void CStringReplace::ReplaceW(WCHAR* pszInputString,WCHAR* pszOutputString,WCHAR* pszOldStr,WCHAR* pszNewStr,BOOL CaseSensitive)
{
    WCHAR* pszPos;
    WCHAR* pszOldPos;
    SIZE_T SearchedItemSize;
    WCHAR* pszLocalInputString;
    WCHAR* pszLocalOldStr;

    if ((!pszInputString)||(!pszOutputString)||(!pszOldStr)||(!pszNewStr))
        return;

    *pszOutputString=0;

    if (CaseSensitive)
    {
        pszLocalInputString=pszInputString;
        pszLocalOldStr=pszOldStr;
    }
    else
    {
        pszLocalInputString=wcsdup(pszInputString);
        wcsupr(pszLocalInputString);
        pszLocalOldStr=wcsdup(pszOldStr);
        wcsupr(pszLocalOldStr);
    }

    pszOldPos=pszLocalInputString;
    // get searched item size
    SearchedItemSize=wcslen(pszOldStr);
    // look for next string to replace
    pszPos=wcsstr(pszLocalInputString,pszLocalOldStr);
    while(pszPos)
    {
        // copy unchanged data
        wcsncat(pszOutputString,pszInputString+ (pszOldPos - pszLocalInputString),pszPos-pszOldPos);
        // copy replace string
        wcscat(pszOutputString,pszNewStr);
        // update old position
        pszOldPos=pszPos+SearchedItemSize;
        // look for next string to replace
        pszPos=wcsstr(pszOldPos,pszOldStr);
    }
    // copy remaining data
    wcscat(pszOutputString,pszInputString+ (pszOldPos - pszLocalInputString));

    if (!CaseSensitive)
    {
        free(pszLocalInputString);
        free(pszLocalOldStr);
    }
}

//-----------------------------------------------------------------------------
// Name: ReplaceA
// Object: replace pszOldStr by pszNewStr in pszInputString and put the result in 
//         pszOutputString.
//         Warning pszOutputString must be large enough no check is done
// Parameters :
//     in  : CHAR* pszInputString : string to translate
//           CHAR* pszOldStr : string to replace
//           CHAR* pszNewStr : replacing string
//     out : CHAR* pszOutputString : result string
//     return : 
//-----------------------------------------------------------------------------
void CStringReplace::ReplaceA(CHAR* pszInputString,CHAR* pszOutputString,CHAR* pszOldStr,CHAR* pszNewStr)
{
    return CStringReplace::ReplaceA(pszInputString,pszOutputString,pszOldStr,pszNewStr,TRUE);
}
//-----------------------------------------------------------------------------
// Name: ReplaceA
// Object: replace pszOldStr by pszNewStr in pszInputString and put the result in 
//         pszOutputString.
//         Warning pszOutputString must be large enough no check is done
// Parameters :
//     in  : CHAR* pszInputString : string to translate
//           CHAR* pszOldStr : string to replace
//           CHAR* pszNewStr : replacing string
//           BOOL CaseSensitive : TRUE for case sensitive search, FALSE for case insensitive search
//     out : CHAR* pszOutputString : result string
//     return : 
//-----------------------------------------------------------------------------
void CStringReplace::ReplaceA(CHAR* pszInputString,CHAR* pszOutputString,CHAR* pszOldStr,CHAR* pszNewStr,BOOL CaseSensitive)
{
    CHAR* pszPos;
    CHAR* pszOldPos;
    SIZE_T SearchedItemSize;
    CHAR* pszLocalInputString;
    CHAR* pszLocalOldStr;

    if ((!pszInputString)||(!pszOutputString)||(!pszOldStr)||(!pszNewStr))
        return;

    *pszOutputString=0;

    if (CaseSensitive)
    {
        pszLocalInputString=pszInputString;
        pszLocalOldStr=pszOldStr;
    }
    else
    {
        pszLocalInputString=strdup(pszInputString);
        strupr(pszLocalInputString);
        pszLocalOldStr=strdup(pszOldStr);
        strupr(pszLocalOldStr);
    }

    pszOldPos=pszLocalInputString;
    // get searched item size
    SearchedItemSize=strlen(pszOldStr);
    // look for next string to replace
    pszPos=strstr(pszLocalInputString,pszLocalOldStr);
    while(pszPos)
    {
        // copy unchanged data
        strncat(pszOutputString,pszInputString+ (pszOldPos - pszLocalInputString),pszPos-pszOldPos);
        // copy replace string
        strcat(pszOutputString,pszNewStr);
        // update old position
        pszOldPos=pszPos+SearchedItemSize;
        // look for next string to replace
        pszPos=strstr(pszOldPos,pszOldStr);
    }
    // copy remaining data
    strcat(pszOutputString,pszInputString+ (pszOldPos - pszLocalInputString));

    if (!CaseSensitive)
    {
        free(pszLocalInputString);
        free(pszLocalOldStr);
    }
}

//-----------------------------------------------------------------------------
// Name: ComputeMaxRequieredSizeForReplace
// Object: compute max needed size in TCHAR of pszOutputString for a replacement
// Parameters :
//     in  : TCHAR* pszInputString : string to translate
//           TCHAR* pszOldStr : string to replace
//           TCHAR* pszNewStr : replacing string
//     out : 
//     return : max needed size in TCHAR of pszOutputString (including \0)
//-----------------------------------------------------------------------------
SIZE_T CStringReplace::ComputeMaxRequieredSizeForReplace(TCHAR* pszInputString,TCHAR* pszOldStr,TCHAR* pszNewStr)
{
    SIZE_T InputStringSize;
    SIZE_T OldStrSize;
    SIZE_T NewStrSize;
    SIZE_T NbMaxReplacement;
    SIZE_T NbRemainingChars;
    
    InputStringSize=_tcslen(pszInputString);
    OldStrSize=_tcslen(pszOldStr);
    NewStrSize=_tcslen(pszNewStr);
    
    if (NewStrSize<=OldStrSize)
    {
        // return the max case : no occurrence replaced
        return (InputStringSize+1); // +1 for \0
    }

    // at this point OldStrSize<NewStrSize

    // compute max number of replacements
    NbMaxReplacement=InputStringSize/OldStrSize;

    // compute remainder
    NbRemainingChars=InputStringSize%OldStrSize;

    return NbMaxReplacement*NewStrSize+NbRemainingChars+1; // +1 for \0
}