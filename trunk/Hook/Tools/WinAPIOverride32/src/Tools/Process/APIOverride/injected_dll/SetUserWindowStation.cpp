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

#include "setuserwindowstation.h"

CSetUserWindowStation::CSetUserWindowStation(void)
{
    this->FunctionsLoaded=FALSE;

    this->DynamicOpenWindowStationW=NULL;
    this->DynamicCloseWindowStation=NULL;
    this->DynamicSetProcessWindowStation=NULL;
    this->DynamicGetProcessWindowStation=NULL;
    this->DynamicOpenDesktopW=NULL;
    this->DynamicCloseDesktop=NULL;
    this->DynamicSetThreadDesktop=NULL;
    this->DynamicGetThreadDesktop=NULL;
    this->DynamicGetUserObjectInformationW=NULL;
}

CSetUserWindowStation::~CSetUserWindowStation(void)
{
}
//-----------------------------------------------------------------------------
// Name: CheckInit
// Object: check if LoadFunctions has been successfully called
// Parameters :
//      in: 
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSetUserWindowStation::CheckInit()
{
    if (!this->LoadFunctions())
        return FALSE;

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: LoadFunctions
// Object: load libraries and get required functions addresses
// Parameters :
//      in: 
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSetUserWindowStation::LoadFunctions()
{
    if (this->FunctionsLoaded)
        return TRUE;
    HMODULE HmodUser32;
    HmodUser32=GetModuleHandleW(L"user32.dll");
    if (!HmodUser32)
    {
        HmodUser32=LoadLibraryW(L"user32.dll");
        if (!HmodUser32)
            return FALSE;
    }
    
    this->DynamicOpenWindowStationW=(pfOpenWindowStationW)GetProcAddress(HmodUser32,"OpenWindowStationW");
    if (!this->DynamicOpenWindowStationW)
        return FALSE;

    this->DynamicCloseWindowStation=(pfCloseWindowStation)GetProcAddress(HmodUser32,"CloseWindowStation");
    if (!this->DynamicCloseWindowStation)
        return FALSE;

    this->DynamicSetProcessWindowStation=(pfSetProcessWindowStation)GetProcAddress(HmodUser32,"SetProcessWindowStation");
    if (!this->DynamicSetProcessWindowStation)
        return FALSE;

    this->DynamicGetProcessWindowStation=(pfGetProcessWindowStation)GetProcAddress(HmodUser32,"GetProcessWindowStation");
    if (!this->DynamicGetProcessWindowStation)
        return FALSE;

    this->DynamicOpenDesktopW=(pfOpenDesktopW)GetProcAddress(HmodUser32,"OpenDesktopW");
    if (!this->DynamicOpenDesktopW)
        return FALSE;

    this->DynamicCloseDesktop=(pfCloseDesktop)GetProcAddress(HmodUser32,"CloseDesktop");
    if (!this->DynamicCloseDesktop)
        return FALSE;

    this->DynamicSetThreadDesktop=(pfSetThreadDesktop)GetProcAddress(HmodUser32,"SetThreadDesktop");
    if (!this->DynamicSetThreadDesktop)
        return FALSE;

    this->DynamicGetThreadDesktop=(pfGetThreadDesktop)GetProcAddress(HmodUser32,"GetThreadDesktop");
    if (!this->DynamicGetThreadDesktop)
        return FALSE;

    this->DynamicGetUserObjectInformationW=(pfGetUserObjectInformationW)GetProcAddress(HmodUser32,"GetUserObjectInformationW");
    if (!this->DynamicGetUserObjectInformationW)
        return FALSE;

    this->FunctionsLoaded=TRUE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CanProcessWindowInteract
// Object: check if process can interact with current user desktop
// Parameters :
//      in: 
// Return : TRUE if process can interact with current user desktop
//-----------------------------------------------------------------------------
BOOL CSetUserWindowStation::CanProcessWindowInteract()
{
    if (!this->CheckInit())
        return FALSE;

    USEROBJECTFLAGS Flags;
    HWINSTA CurrentStation=this->DynamicGetProcessWindowStation();
    DWORD NeededSize=0;
    if (!this->DynamicGetUserObjectInformationW(CurrentStation,UOI_FLAGS,&Flags,sizeof(USEROBJECTFLAGS),&NeededSize))
        return FALSE;
    return ((Flags.dwFlags & WSF_VISIBLE)!=0);
}

//-----------------------------------------------------------------------------
// Name: IsUserStation
// Object: check if process runs with current user station
// Parameters :
//      in: 
// Return : TRUE if process runs with current user station
//-----------------------------------------------------------------------------
BOOL CSetUserWindowStation::IsUserStation()
{
    if (!this->CheckInit())
        return FALSE;

    WCHAR Name[10];
    DWORD NeededSize;
    // NeededSize: If this variable's value is greater than the value of the nLength parameter when the function returns,
    // the function returns FALSE, and none of the information is copied to the pvInfo buffer. 
    // If the value of the variable pointed to by lpnLengthNeeded is less than or equal to the value of nLength,
    // the entire information block is copied
    *Name=0;
    HWINSTA CurrentStation=this->DynamicGetProcessWindowStation();
    if (!CurrentStation)
        return FALSE;
    if(!this->DynamicGetUserObjectInformationW(CurrentStation,UOI_NAME,(PVOID)Name,10*sizeof(WCHAR),&NeededSize))
        return FALSE;// if NeededSize>10 we are sure that name doesn't match L"WinSta0"
    return (wcsicmp(L"WinSta0",Name)==0);
}

//-----------------------------------------------------------------------------
// Name: IsUserDesktop
// Object: check if process runs with current user desktop
// Parameters :
//      in: 
// Return : TRUE if process runs with current user desktop
//-----------------------------------------------------------------------------
BOOL CSetUserWindowStation::IsUserDesktop()
{
    if (!this->CheckInit())
        return FALSE;

    WCHAR Name[10];
    DWORD NeededSize;
    // NeededSize: If this variable's value is greater than the value of the nLength parameter when the function returns,
    // the function returns FALSE, and none of the information is copied to the pvInfo buffer. 
    // If the value of the variable pointed to by lpnLengthNeeded is less than or equal to the value of nLength,
    // the entire information block is copied
    *Name=0;
    HDESK CurrentThreadDesktop=this->DynamicGetThreadDesktop(GetCurrentThreadId());
    if (!CurrentThreadDesktop)
        return FALSE;
    if(!this->DynamicGetUserObjectInformationW(CurrentThreadDesktop,UOI_NAME,(PVOID)Name,10*sizeof(WCHAR),&NeededSize))
        return FALSE;// if NeededSize>10 we are sure that name doesn't match L"Default"
    return (wcsicmp(L"Default",Name)==0);
}


//-----------------------------------------------------------------------------
// Name: SetThreadDesktop
// Object: set default desktop for current thread
// Parameters :
//      out : OUT HDESK* pPreviousDesktop : previous desktop
//            OUT HDESK* pCurrentThreadDesktop : current desktop
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSetUserWindowStation::SetThreadDesktop(OUT HDESK* pPreviousDesktop,OUT HDESK* pCurrentThreadDesktop)
{
    if (!this->CheckInit())
        return FALSE;

    *pCurrentThreadDesktop = this->DynamicOpenDesktopW(L"Default", 0, TRUE, GENERIC_ALL);//MAXIMUM_ALLOWED);
    if (*pCurrentThreadDesktop == NULL) 
        return FALSE; 

    *pPreviousDesktop = this->DynamicGetThreadDesktop(GetCurrentThreadId()); 
    if (!this->DynamicSetThreadDesktop(*pCurrentThreadDesktop))
    {
        this->DynamicCloseDesktop(*pCurrentThreadDesktop);
        *pCurrentThreadDesktop=NULL;
        return FALSE;
    }
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: RestorePreviousDesktop
// Object: restore previous desktop for current thread
// Parameters :
//      in : HDESK PreviousDesktop : previous desktop
//           HDESK CurrentThreadDesktop : current desktop
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSetUserWindowStation::RestorePreviousDesktop(HDESK PreviousDesktop,HDESK CurrentThreadDesktop)
{
    BOOL bRet;
    if (!this->CheckInit())
        return FALSE;

    bRet=this->DynamicSetThreadDesktop(PreviousDesktop);
    if (bRet&&CurrentThreadDesktop)
        this->DynamicCloseDesktop(CurrentThreadDesktop);

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: RestorePreviousStation
// Object: set default station for current process
// Parameters :
//      out : HWINSTA* pPreviousStation : previous station
//            HWINSTA* pCurrentStation : current station
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSetUserWindowStation::SetProcessStation(OUT HWINSTA* pPreviousStation,OUT HWINSTA* pCurrentStation)
{
    if (!this->CheckInit())
        return FALSE;

    *pPreviousStation= this->DynamicGetProcessWindowStation(); 
    *pCurrentStation = this->DynamicOpenWindowStationW(L"WinSta0", FALSE, GENERIC_ALL);//MAXIMUM_ALLOWED); 
    if (*pCurrentStation == NULL) 
        return FALSE;

    if (!this->DynamicSetProcessWindowStation(*pCurrentStation))
    {
        this->DynamicCloseWindowStation(*pCurrentStation); 
        *pCurrentStation=NULL;
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RestorePreviousStation
// Object: restore previous station for current process
// Parameters :
//      in : HWINSTA PreviousStation: previous station
//           HWINSTA CurrentStation : current station
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSetUserWindowStation::RestorePreviousStation(HWINSTA PreviousStation,HWINSTA CurrentStation)
{
    if (!this->CheckInit())
        return FALSE;

    BOOL bRet;
    bRet=this->DynamicSetProcessWindowStation(PreviousStation); 
    if (bRet&&CurrentStation)
        this->DynamicCloseWindowStation(CurrentStation);
    return bRet;
}