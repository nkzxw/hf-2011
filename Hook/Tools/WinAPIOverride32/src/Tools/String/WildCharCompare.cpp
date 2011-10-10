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


THANKS TO OPEN SOURCE COMMUNITY
This code is based on an open source code, but
as more than once claiming to be author of this, 
I don't know the person to thanks :( 
--> please let original authors names !!!
*/

//-----------------------------------------------------------------------------
// Object: Check if a string match a pattern containing '*' and '?'
//-----------------------------------------------------------------------------

#include "wildcharcompare.h"

//-----------------------------------------------------------------------------
// Name: WildICmp
// Object: Insensitive comparison with wild char
// Parameters :
//     in  : TCHAR* Wild   : the pattern (*.xml)
//           TCHAR* String : the string to check (myDoc.xml)
//     out :
//     return : TRUE if String match pattern, FALSE else
//-----------------------------------------------------------------------------
BOOL CWildCharCompare::WildICmp(TCHAR* Wild,TCHAR* String)
{
    TCHAR* W;
    TCHAR* S;
    BOOL bRet;
    SIZE_T W_Size;
    SIZE_T S_Size;

    W_Size=(_tcslen(Wild)+1)*sizeof(TCHAR);
    S_Size=(_tcslen(String)+1)*sizeof(TCHAR);
    // make local copy as _tcsupr modify data
    W=(TCHAR*)_alloca(W_Size);
    memcpy(W,Wild,W_Size);
    S=(TCHAR*)_alloca(S_Size);
    memcpy(S,String,S_Size);
    // convert data to upper case
    _tcsupr(W);
    _tcsupr(S);

    // compare data
    bRet=CWildCharCompare::WildCmp(W,S);

    // return WildCmp status
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: WildCmp
// Object: Sensitive comparison with wild char
// Parameters :
//     in  : TCHAR* Wild   : the pattern (*.xml)
//           TCHAR* String : the string to check (myDoc.xml)
//     out :
//     return : TRUE if String match pattern, FALSE else
//-----------------------------------------------------------------------------
BOOL CWildCharCompare::WildCmp(TCHAR* Wild,TCHAR* String)
{
    TCHAR* cp=NULL;
    TCHAR* mp=NULL;

    // check data before first '*' in Wild string
    while ((*String) && (*Wild != '*'))
    {
        // if String doesn't match Wild and *Wild!='?'
        // notice : it checks for end of Wild (if *Wild==0 and as *String is !=0)
        //          job is done by *Wild != *String
        if ((*Wild != *String) && (*Wild != '?')) 
            // string don't match
            return FALSE;

        // increment pointers
        Wild++;
        String++;
    }

    // until String is not finished 
    while (*String)
    {
        if (*Wild == '*')
        {
            // if there's no other char in Wild after '*'
            if (!*++Wild)
                return TRUE;
            // else 
            
            // save pointers
            mp = Wild;
            cp = String+1;
        }
        // if strings match
        else if ((*Wild == *String) || (*Wild == '?'))
        {
            // increment pointers
            Wild++;
            String++;
        }
        else
        {
            // notice as we go through the first while, 
            // it's assume we have initialize mp and cp at the first loop of this while

            // notice : it checks for end of Wild too (if *Wild==0 and as *String is !=0)
            //          job is done by *Wild != *String

            // restore Wild pointer
            Wild = mp;
            // make String point to the char after the save pointer
            // doing this, we will check patterns again for remaining String data
            String = cp++;
        }
    }

    // as String is finished, we have to assume that 
    // next chars in Wild only contains '*'
    while (*Wild == '*')
        Wild++;

    // if first char !='*' is not the end string one ('\0'=0)
    //     return FALSE
    // else (*Wild==0)
    //     return TRUE
    return !*Wild;
}
