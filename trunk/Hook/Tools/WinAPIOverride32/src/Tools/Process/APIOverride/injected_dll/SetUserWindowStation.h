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
// Object: manages window station to display dialogs into current user station
//-----------------------------------------------------------------------------

#pragma once

#include <Windows.h>

typedef HWINSTA (WINAPI *pfOpenWindowStationW)(
    IN LPCWSTR lpszWinSta,
    IN BOOL fInherit,
    IN ACCESS_MASK dwDesiredAccess);
typedef BOOL (WINAPI *pfCloseWindowStation)(IN HWINSTA hWinSta);
typedef BOOL (WINAPI *pfSetProcessWindowStation)(IN HWINSTA hWinSta);
typedef HWINSTA (WINAPI *pfGetProcessWindowStation)(VOID);

typedef HDESK (WINAPI *pfOpenDesktopW)(
                                       IN LPCWSTR lpszDesktop,
                                       IN DWORD dwFlags,
                                       IN BOOL fInherit,
                                       IN ACCESS_MASK dwDesiredAccess);
typedef BOOL (WINAPI *pfCloseDesktop)(IN HDESK hDesktop);
typedef BOOL (WINAPI *pfSetThreadDesktop)(IN HDESK hDesktop);
typedef HDESK (WINAPI *pfGetThreadDesktop)(IN DWORD dwThreadId);
typedef BOOL (WINAPI *pfGetUserObjectInformationW)(HANDLE hObj,int nIndex,PVOID pvInfo,DWORD nLength,LPDWORD lpnLengthNeeded);



class CSetUserWindowStation
{
public:
    CSetUserWindowStation(void);
    ~CSetUserWindowStation(void);
    BOOL SetProcessStation(OUT HWINSTA* pPreviousStation,OUT HWINSTA* pCurrentStation);
    BOOL RestorePreviousStation(HWINSTA PreviousStation,HWINSTA CurrentStation);
    BOOL SetThreadDesktop(OUT HDESK* pPreviousDesktop,OUT HDESK* pCurrentThreadDesktop);
    BOOL RestorePreviousDesktop(HDESK PreviousDesktop,HDESK CurrentThreadDesktop);
    BOOL IsUserStation();
    BOOL IsUserDesktop();
    BOOL CanProcessWindowInteract();

private:
    BOOL FunctionsLoaded;
    HWINSTA hwinstaUser; 
    HDESK hdeskUser;

    BOOL LoadFunctions();
    BOOL CheckInit();


    pfOpenWindowStationW DynamicOpenWindowStationW;
    pfCloseWindowStation DynamicCloseWindowStation;
    pfSetProcessWindowStation DynamicSetProcessWindowStation;
    pfGetProcessWindowStation DynamicGetProcessWindowStation;
    pfOpenDesktopW DynamicOpenDesktopW;
    pfCloseDesktop DynamicCloseDesktop;
    pfSetThreadDesktop DynamicSetThreadDesktop;
    pfGetThreadDesktop DynamicGetThreadDesktop;
    pfGetUserObjectInformationW DynamicGetUserObjectInformationW;
};
