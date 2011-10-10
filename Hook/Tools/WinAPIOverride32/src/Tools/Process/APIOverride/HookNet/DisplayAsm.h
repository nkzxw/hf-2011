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
// Object: display generated asm opcodes
//-----------------------------------------------------------------------------

#pragma once

#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <Windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "../../../LinkList/LinkListBase.h"

#define CDisplayAsm_DIALOG_MIN_WIDTH 200
#define CDisplayAsm_DIALOG_MIN_HEIGHT 100

class CDisplayAsm
{
public:
    CDisplayAsm(void);
    ~CDisplayAsm(void);
    static INT_PTR Show(TCHAR* Title,TCHAR* DisasmContent);
private:
    TCHAR* Content;
    TCHAR* Title;
    HINSTANCE hInstance;
    INT_PTR DialogResult;
    HWND hWndDialog;
    CLinkListItem* pItemDialog;
    HFONT hFont;

    static CDisplayAsm* GetAssociatedDialogObject(HWND hWndDialog);
    void Init();
    void Close();
    void OnSize();
    void OnSizing(RECT* pRect);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
