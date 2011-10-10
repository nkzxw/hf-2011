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
// Object: manages the modules filters dialog
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>

#include "resource.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../tools/GUI/ListBox/Listbox.h"
#include "../tools/GUI/ListView/Listview.h"
#include "../Tools/LinkList/LinkListSimple.h"
#include "../tools/Process/APIOverride/ApiOverride.h"
#include "options.h"

// ShowFilterDialog results
#define FILTERS_DLG_RES_FAILED 0
#define FILTERS_DLG_RES_OK     1    // user clicks OK
#define FILTERS_DLG_RES_CANCEL 2    // user clicks Cancel

#define CCModulesFilters_DIALOG_MIN_WIDTH 500
#define CCModulesFilters_DIALOG_MIN_HEIGHT 200
#define CModulesFilters_SPACE_BETWEEN_BUTTONS 10
#define CModulesFilters_SPACE_BETWEEN_CONTROLS 5

class CModulesFilters
{
private:
    int State;
    void Close();
    void Init();
    void OnSize();
    void OnSizing(RECT* pRect);
    void SelectProcess();
    void SelectModules();
    void GetProcessModules();
    HWND hWndFiltersDialog;
    HWND hwndFilterDialogListBoxHookedProcesses;
    CListview* pListviewSelectedModules;
    CApiOverride* CurrentpApiOverride;
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
    CModulesFilters();
    ~CModulesFilters();
    INT_PTR ShowFilterDialog(HINSTANCE hInstance,HWND hWndDialog);
};

