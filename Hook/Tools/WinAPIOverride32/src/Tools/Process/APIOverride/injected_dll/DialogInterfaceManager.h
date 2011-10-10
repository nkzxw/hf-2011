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
#pragma once

#include "DynamicLoadedFuncs.h"
#include <windows.h>
#include <Tlhelp32.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CDialogInterfaceManager
{
public:
    static BOOL __stdcall SetDefaultStation(OUT HWINSTA* pCurrentStation, OUT HWINSTA* pOldStation,OUT HDESK* pCurrentDesktop,OUT HDESK* pOldDesktop);
    static BOOL __stdcall RestoreStation(IN HWINSTA CurrentStation,IN HWINSTA OldStation,IN HDESK CurrentDesktop,IN HDESK OldDesktop);
    static BOOL __stdcall CanWindowInteract();
    static HANDLE __stdcall AdjustThreadSecurityAndLaunchDialogThread(LPTHREAD_START_ROUTINE lpStartAddress,LPVOID lpParameter);
};
