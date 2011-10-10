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
// Object: manages the select hooked processes dialog
//-----------------------------------------------------------------------------


#pragma once


#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>

#include "resource.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../tools/GUI/ListBox/Listbox.h"
#include "../Tools/LinkList/LinkListSimple.h"
#include "../tools/Process/APIOverride/ApiOverride.h"

// ShowFilterDialog results
#define FILTERS_DLG_RES_FAILED 0
#define FILTERS_DLG_RES_OK     1    // user clicks OK
#define FILTERS_DLG_RES_CANCEL 2    // user clicks Cancel


class CSelectHookedProcess
{
private:
    CApiOverride* pobjApiOverride;
    HWND hWndDialog;
    HWND hwndSelectDialogListBoxHookedProcesses;
    void Select();
    void Close();
    void Init();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
    CSelectHookedProcess(void);
    ~CSelectHookedProcess(void);
    static CApiOverride* ShowDialog(HINSTANCE hInstance,HWND hWndDialog);
};
