/*
Copyright (C) 2010 Jacquelin POTIER <jacquelin.potier@free.fr>
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

#pragma once

#include "LauncherOptions.h"

#include "../../../../../Tools/gui/Dialog/DialogBase.h"

#define CLauncherConfigGUI_EMULATED_REGISTRY_CREATOR _T("EmulatedRegistryCreator.exe")
#define CLauncherConfigGUI_EMULATED_REGISTRY_EDITOR _T("EmulatedRegistryEditor.exe")
#define CLauncherConfigGUI_DEFAULT_FILTER_EXCLUDE_FILE _T("NotHookedModuleList.txt")
#define CLauncherConfigGUI_DEFAULT_FILTER_INCLUDE_FILE _T("HookedOnlyModuleList.txt")

class CLauncherConfigGUI:public CDialogBase
{
private:
    CLauncherOptions* pLauncherOptions;

    void OnOk();
    void OnSpy();
    void OnCreateRegistry();
    void OnBrowse(int EditId,TCHAR* Filter);
    BOOL GetAndCheckOptions(BOOL bSpyMode);
public:
    virtual void OnInit();
    virtual void OnCommand(WPARAM wParam,LPARAM lParam);
    virtual void OnDropFiles(WPARAM wParam,LPARAM lParam);


    static BOOL Show(HINSTANCE hInstance,IN OUT CLauncherOptions* pLauncherOptions);

};