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
// Object: manages the hex display dialog
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "resource.h"
#include "../tools/String/StrToHex.h"
#include "../tools/APIError/ApiError.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"

// number of bytes visible in the read text box
#define NB_BYTES_PER_LINE 16 // 4 DWORD

class CHexDisplay
{
private:
    HINSTANCE hInstance;
    HWND hParentHandle;

    HWND hWndDialog;
    HFONT MemoryhFont;
    PBYTE pByte;
    DWORD Size;
    void HexDisplay();

    void Init();
    void Close();
    static DWORD WINAPI ModelessDialogThread(PVOID lParam);
    static CHexDisplay* GetAssociatedObject(HWND hWndDialog);
    static LRESULT CALLBACK UserInterfaceWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
    CHexDisplay(PBYTE pByte,DWORD Size);
    ~CHexDisplay(void);
    static void Show(HINSTANCE hInstance,HWND ParentHandle,PBYTE pByte,DWORD Size);
};
