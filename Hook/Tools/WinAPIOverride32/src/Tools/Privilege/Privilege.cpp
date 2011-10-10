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
// Object: Class helper to adjust privileges
//-----------------------------------------------------------------------------

#include "privilege.h"

CPrivilege::CPrivilege(BOOL bShowErrors)
{
    this->bShowErrors=bShowErrors;
    if(!OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &this->hToken
            ))
    {
        if (this->bShowErrors)
            CAPIError::ShowLastError();
    }
}
CPrivilege::~CPrivilege(void)
{
    CloseHandle(this->hToken);
}


BOOL CPrivilege::SetPrivilege(
    LPCTSTR Privilege,    // Privilege to enable/disable
    BOOL bEnablePrivilege // TRUE to enable.  FALSE to disable
    )
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious;
    DWORD cbPrevious=sizeof(TOKEN_PRIVILEGES);

    memset(&tpPrevious,0,sizeof(TOKEN_PRIVILEGES));

    if(!LookupPrivilegeValue( NULL, Privilege, &luid )) return FALSE;

    // 
    // first pass.  get current privilege setting
    // 
    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid; 
    tp.Privileges[0].Attributes = 0;

    if (!AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            &tpPrevious,
            &cbPrevious
            ))
    {
        if (this->bShowErrors)
            CAPIError::ShowLastError();
        return FALSE;
    }

    // 
    // second pass.  set privilege based on previous setting
    // 
    tpPrevious.PrivilegeCount       = 1;
    tpPrevious.Privileges[0].Luid   = luid;

    if(bEnablePrivilege) {
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    }
    else {
        tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
            tpPrevious.Privileges[0].Attributes);
    }

    if (!AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tpPrevious,
            cbPrevious,
            NULL,
            NULL
            ))
    {
        if (this->bShowErrors)
            CAPIError::ShowLastError();
        return FALSE;
    }
    return TRUE;
}