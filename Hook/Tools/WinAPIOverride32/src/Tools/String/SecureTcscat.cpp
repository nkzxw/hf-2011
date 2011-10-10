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
// Object: make a strcat function with reallocation if needed
//         limitation : string must be created with "new" and delete with "delete"
//-----------------------------------------------------------------------------

#include "SecureTcscat.h"

//-----------------------------------------------------------------------------
// Name: Secure_tcscat
// Object: make a strcat function with reallocation if needed
//         limitation : string must be created with "new" and delete with "delete"
// Parameters :
//     IN OUT TCHAR** pString : result string may be reallocated
//     TCHAR* StringToAdd : string to add to *pString
//     IN OUT SIZE_T* pStringMaxSize : IN *pString max size, OUT new *pString max size
//     return : *pString
//-----------------------------------------------------------------------------
TCHAR* CSecureTcscat::Secure_tcscat(IN OUT TCHAR** pString,TCHAR* StringToAdd,IN OUT SIZE_T* pStringMaxSize)
{
    if ( ( _tcslen(*pString) + _tcslen(StringToAdd) + 1) >= *pStringMaxSize)
    {
        TCHAR* TmpString = *pString;
        *pStringMaxSize = __max(*pStringMaxSize*2,*pStringMaxSize+_tcslen(StringToAdd)+1);
        *pString = new TCHAR[*pStringMaxSize];
        _tcscpy(*pString,TmpString);
        delete TmpString;
    }
    _tcscat(*pString,StringToAdd);
    return *pString;
}