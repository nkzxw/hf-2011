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
// Object: manages window station to display all WinAPIOverride dialogs into current user station
//          manages multiple dialogs in the same time
//          All injected dll must use this class to display a dialog
//-----------------------------------------------------------------------------

#include "dialoginterfacemanager.h"

static HWINSTA OriginalStation=NULL;
static DWORD NumberOfDialogInterface=0;
extern CSetUserWindowStation* pSetUserWindowStation;

//-----------------------------------------------------------------------------
// Name: SetDefaultStation
// Object: set process station and thread desktop to allow user to interact with dialogs
// Parameters :
//     out: OUT HWINSTA* pCurrentStation : current station
//          OUT HWINSTA* pOldStation : old station
//          OUT HDESK* pCurrentDesktop : current desktop
//          OUT HDESK* pOldDesktop : old desktop
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL __stdcall CDialogInterfaceManager::SetDefaultStation(OUT HWINSTA* pCurrentStation, OUT HWINSTA* pOldStation,OUT HDESK* pCurrentDesktop,OUT HDESK* pOldDesktop)
{
    *pOldDesktop=NULL;
    *pCurrentDesktop=NULL;
    *pOldStation=NULL;
    *pCurrentStation=NULL;
    BOOL bError=FALSE;
    BOOL bFunctionRet;

    NumberOfDialogInterface++;

    if (pSetUserWindowStation)
    {
        if (!pSetUserWindowStation->IsUserStation())
        {
            bFunctionRet = pSetUserWindowStation->SetProcessStation(pOldStation,pCurrentStation);
            bError=bError && bFunctionRet;
        }
        if (!pSetUserWindowStation->IsUserDesktop())
        {
            bFunctionRet = pSetUserWindowStation->SetThreadDesktop(pOldDesktop,pCurrentDesktop);
            bError=bError && bFunctionRet;
        }

        // save original station only if not previously modified
        if (NumberOfDialogInterface==1)
            OriginalStation=*pOldStation;
    }

    return !bError;
}

//-----------------------------------------------------------------------------
// Name: SetDefaultStation
// Object: restore process station and thread desktop
// Parameters :
//     in : HWINSTA CurrentStation : current station
//          HWINSTA OldStation : old station
//          HDESK CurrentDesktop : current desktop
//          HDESK OldDesktop : old desktop
// Return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL __stdcall CDialogInterfaceManager::RestoreStation(IN HWINSTA CurrentStation,IN HWINSTA OldStation,IN HDESK CurrentDesktop,IN HDESK OldDesktop)
{
    UNREFERENCED_PARAMETER(OldStation);
    if (pSetUserWindowStation)
    {
        if(OldDesktop)
            pSetUserWindowStation->RestorePreviousDesktop(OldDesktop,CurrentDesktop);

        if (OriginalStation)
        {
            // restore original station only if no other break dialog are displayed
            if (NumberOfDialogInterface==1)
            {
                pSetUserWindowStation->RestorePreviousStation(OriginalStation,CurrentStation);
            }
        }
    }
    NumberOfDialogInterface--;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CanWindowInteract
// Object: check if process can interact with user (= if dialog can be shown)
// Parameters :
//     in : 
// Return : TRUE if window can interact
//-----------------------------------------------------------------------------
BOOL __stdcall CDialogInterfaceManager::CanWindowInteract()
{
    return pSetUserWindowStation->CanProcessWindowInteract();
    // if service has no user interaction , 
    // it fails (at least on XP SP1) to try to give it interaction 
    // by setting WSF_VISIBLE attribute with SetUserObjectInformation

    // if you want to give it a try here's the function
    /*
    BOOL EnableProcessWindowInteract(BOOL Enable)
    {
    USEROBJECTFLAGS Flags;
    HWINSTA CurrentStation=GetProcessWindowStation();
    DWORD NeededSize=0;
    if (!GetUserObjectInformation(CurrentStation,UOI_FLAGS,&Flags,sizeof(USEROBJECTFLAGS),&NeededSize))
    return FALSE;

    if (Enable)
    {
    // add user interaction
    Flags.dwFlags|=WSF_VISIBLE;
    }
    else
    {
    // remove
    Flags.dwFlags&=~WSF_VISIBLE;
    }
    return SetUserObjectInformation(CurrentStation,UOI_FLAGS,&Flags,sizeof(USEROBJECTFLAGS));
    }
    */
}

//-----------------------------------------------------------------------------
// Name: AdjustThreadSecurityAndLaunchDialogThread
// Object: create a thread with adjusted security parameter to allow user to interact with dialog
//         (avoid to change full process security)
// Parameters :
//     in : 
// Return : HANDLE of created thread
//-----------------------------------------------------------------------------
HANDLE __stdcall CDialogInterfaceManager::AdjustThreadSecurityAndLaunchDialogThread(LPTHREAD_START_ROUTINE lpStartAddress,LPVOID lpParameter)
{
    // start a new thread to limit interactions with the desktop setting

    // set thread default attribute without security (avoid user interaction security limitations)
    SECURITY_DESCRIPTOR sd={0};
    sd.Revision=SECURITY_DESCRIPTOR_REVISION;
    sd.Control=SE_DACL_PRESENT;
    sd.Dacl=NULL; // assume everyone access
    SECURITY_ATTRIBUTES SecAttr={0};
    SecAttr.bInheritHandle=FALSE;
    SecAttr.nLength=sizeof(SECURITY_ATTRIBUTES);
    SecAttr.lpSecurityDescriptor=&sd;
    // start dialog thread

    return CreateThread(&SecAttr,0,lpStartAddress,lpParameter,0,0);
}