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
#include <Tlhelp32.h>
#include <stdio.h>

#include "resource.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../Tools/GUI/Tab/TabControl.h"
#include "../Tools/File/StdFileOperations.h"
#include "Options.h"


class COptionsUI
{
public:
    enum tagDlgRes
    {
        // ShowFilterDialog results
        DLG_RES_FAILED=0,
        DLG_RES_OK,         // user clicks OK
        DLG_RES_CANCEL      // user clicks Cancel
    };
    COptionsUI(COptions* pOptions);
    ~COptionsUI();
    INT_PTR Show(HINSTANCE hInstance,HWND hWndDialog);
    static LRESULT CALLBACK TabWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
    COptions* pOptions;
    HWND hWndDialog;
    HWND hWndTab;
    HINSTANCE hInstance;
    CTabControl* pTabControl;
    HWND hWndGeneralOptions;
    HWND hWndCOMOptions;
    HWND hWndNETOptions;

    void LoadSettings();
    BOOL SaveSettings();
    void BrowseForFile(HWND hWnd,int Idd);
    void EditFile(HWND hWnd,int Idd);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void Close(tagDlgRes Res);
    void Init(HWND hwnd);
};

