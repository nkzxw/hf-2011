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
// Object: manages the processes filters dialog
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <windowsx.h>
#include <Tlhelp32.h>
#include <stdio.h>

#include "resource.h"
#include "Options.h"
#include "defines.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"

class CFilters
{
public:
    enum tagDlgRes
    {
        // ShowFilterDialog results
        DLG_RES_FAILED=0,
        DLG_RES_OK,         // user clicks OK
        DLG_RES_CANCEL      // user clicks Cancel
    };

    CFilters();
    ~CFilters();
    static INT_PTR Show(HINSTANCE hInstance,HWND hWndDialog,COptions* pOptions);

private:
    HWND hWndDialog;
    HINSTANCE hInstance;
    COptions* pOptions;

    BOOL GetValue(int ControlID,DWORD* pValue);
    void Init();
    void LoadOptions();
    BOOL SaveOptions();
    void Close(tagDlgRes DlgRes);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

