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
// Object: remove beginning and ending spaces of a string
//-----------------------------------------------------------------------------

#include "trimstring.h"
//-----------------------------------------------------------------------------
// Name: TrimString
// Object: remove begin and ending space of a string
// Parameters :
//     in  : TCHAR* pszStr : pointer to string not affected. Only content will change
//     return : TCHAR* pszStr
//-----------------------------------------------------------------------------
TCHAR* CTrimString::TrimString(TCHAR* pszStr)
{
    return CTrimString::TrimString(pszStr,FALSE);
}

//-----------------------------------------------------------------------------
// Name: TrimString
// Object: remove begin and ending space of a string
// Parameters :
//     in  : TCHAR* pszStr
//           BOOL bPreserveOriginalStringAndAllocateNewOne : pszStr won't be affected
//                                                           returned TCHAR must be free with free() call
//     return : TCHAR* pszStr
//-----------------------------------------------------------------------------
TCHAR* CTrimString::TrimString(TCHAR* pszStr,BOOL bPreserveOriginalStringAndAllocateNewOne)
{
    if (IsBadReadPtr(pszStr,sizeof(TCHAR)))
        return NULL;

    TCHAR* psz;
    TCHAR* pcBlankChar;
    TCHAR* pcFirstChar;

    if (bPreserveOriginalStringAndAllocateNewOne)
        psz=_tcsdup(pszStr);
    else
        psz=pszStr;

    // if empty string
    if (!*psz)
        return psz;

    pcBlankChar = psz + _tcslen(psz) - 1;

    // remove last spaces
    while ((*pcBlankChar == ' ') && (psz<=pcBlankChar))
    {
        *pcBlankChar = '\0';
        pcBlankChar--;
    }

    pcFirstChar=psz;
    // remove first spaces
    while (*pcFirstChar == ' ') pcFirstChar++;

    // if string begins with blank char
    if (pcFirstChar!=psz)
        memmove(psz,pcFirstChar,(_tcslen(pcFirstChar)+1)*sizeof(TCHAR));

    return psz;
}