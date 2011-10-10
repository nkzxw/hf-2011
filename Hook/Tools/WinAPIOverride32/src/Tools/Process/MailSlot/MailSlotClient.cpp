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
// Object: class helper for using a client Mailslot
//-----------------------------------------------------------------------------

#include "mailslotclient.h"

//-----------------------------------------------------------------------------
// Name: CMailSlotClient
// Object: Constructor
// Parameters :
//     in  : TCHAR* pszMailSlotName : mailslot name
//           \\.\mailslot\name Retrieves a client handle to a local mailslot. 
//           \\computername\mailslot\name Retrieves a client handle to a remote mailslot. 
//           \\domainname\mailslot\name Retrieves a client handle to all mailslots with the specified name in the specified domain. 
//           \\*\mailslot\name Retrieves a client handle to all mailslots with the specified name in the system's primary domain. 
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
CMailSlotClient::CMailSlotClient(TCHAR* pszMailSlotName)
{
    _tcsncpy(this->pszMailSlotName,pszMailSlotName,MAX_PATH);
    this->pszMailSlotName[MAX_PATH-1]=0;
    this->bOpen=FALSE;
    this->hMailslot=NULL;
}

CMailSlotClient::~CMailSlotClient(void)
{
    // assume mailslot is closed
    this->Close();
}

//-----------------------------------------------------------------------------
// Name: Open
// Object: Open mailslot to use it
// Parameters :
//     in  :
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CMailSlotClient::Open(void)
{
    // open mailslot
    this->hMailslot = CreateFile(this->pszMailSlotName, GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (this->hMailslot == INVALID_HANDLE_VALUE)
        return FALSE;
    // set bOpen flag
    this->bOpen=TRUE;
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: Close
// Object: Close mailslot
// Parameters :
//     in  :
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CMailSlotClient::Close(void)
{
    // check bOpen flag
    if (!this->bOpen)
        return TRUE;

    // close slot
    if (this->hMailslot)
    {
        CloseHandle(this->hMailslot);
        this->hMailslot=NULL;
    }

    // set bOpen flag
    this->bOpen=FALSE;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Write
// Object: Write data contained in pDAta buffer
// Parameters :
//     in  : PVOID pData : data buffer
//           DWORD DataSize : buffer size in byte
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CMailSlotClient::Write(PVOID pData,DWORD DataSize)
{
    if (!this->bOpen)
        return FALSE;

    DWORD dwBytesWritten=0;
    // write data
    BOOL bResult=WriteFile(this->hMailslot, pData, DataSize, &dwBytesWritten, NULL);
    // check result
    bResult=bResult&&(DataSize==dwBytesWritten);
    return bResult;
}
