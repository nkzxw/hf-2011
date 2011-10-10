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
// Object: class helper for using a Mailslot server
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "../../APIError/APIError.h"

#define CMAILSLOTSERVER_CLOSINGSERVER_MAX_WAIT 3000 // time in ms

typedef void (*pfnMailSlotCallback)(PVOID pData,DWORD dwDataSize,PVOID pUserData);


class CMailSlotServer
{
private:
    HANDLE hevtStopEvent;
    HANDLE hevtThreadStop;
    HANDLE hThreadHandle;
    HANDLE hevtDataArrival;
    BOOL bStarted;

    HANDLE hMailslot;
    TCHAR pszMailSlotName[MAX_PATH];
    pfnMailSlotCallback CallbackFunc;
    PVOID pUserData;
    static DWORD WINAPI ThreadListener(LPVOID lpParam);
    PBYTE pMailSlotData;
    HANDLE hOverlappedEvent;
    DWORD ServerThreadID;
public:
    CMailSlotServer(TCHAR* pszMailSlotName,pfnMailSlotCallback CallbackFunc,PVOID pUserData);
    ~CMailSlotServer(void);
    BOOL Start(void);
    BOOL Start(BOOL AllowAccessToAllUsers);
    BOOL Stop(void);
    BOOL WaitUntilNoMessageDuringSpecifiedTime(DWORD TimeInMs,HANDLE hCancelEvent);

    BOOL GetServerThreadId(OUT DWORD* pServerThreadID);
};
