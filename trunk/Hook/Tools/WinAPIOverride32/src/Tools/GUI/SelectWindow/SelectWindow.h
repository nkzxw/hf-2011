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
// Object: allows to graphically select a window with mouse
//         The GetHandleInfo method will fill public info fields like 
//           ParentWindowHandle, WindowControlID,WindowStyle,WindowRect,WindowProc,WindowProcessID ...
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CSelectWindow
{
private:
    HWND        hPictureBox;
    HBITMAP     hBmpCross;
    HBITMAP     hBmpBlank;
    HCURSOR     hCurCross;
    HCURSOR     hCurNormal;
    HINSTANCE   hInst;
    HWND        hDlg;
    HWND        hWndOld;
    BOOL        bCapture;
    BOOL        bInitializeDone;

    HWND SmallestWindowFromPoint( const POINT point );
    void HighlightWindow( HWND hwnd, BOOL fDraw );
    void FreeInitializedData();
public:
    BOOL bOwnerWindowSelectable;
    BOOL bSelectOnlyDialog;
    BOOL bMinimizeOwnerWindowWhenSelect;

    HWND ParentWindowHandle;
    TCHAR ParentWindowText[MAX_PATH];
    TCHAR ParentWindowClassName[MAX_PATH];

    HWND WindowHandle;
    DWORD WindowControlID;
    DWORD WindowStyle;
    RECT WindowRect;
    DWORD WindowExStyle;
    DWORD WindowProc;
    DWORD WindowHinst;
    DWORD WindowUserData;
    DWORD WindowProcessID;
    DWORD WindowThreadID;
    TCHAR WindowClassName[MAX_PATH];
    DWORD WindowDlgProc;
    DWORD WindowDlgMsgResult;
    DWORD WindowDlgUser;

    CSelectWindow(void);
    ~CSelectWindow(void);

    void Initialize(HINSTANCE hInst,HWND hDlg,
                    DWORD ID_PICTURE_BOX,
                    DWORD ID_BMP_BLANK,DWORD ID_BMP_CROSS,DWORD ID_CURSOR_TARGET);
    BOOL GetHandleInfo(HWND hWnd);
    BOOL MouseDown(POINT pt);
    BOOL MouseDown(LPARAM lParam);
    BOOL MouseUp();
    BOOL MouseMove(POINT pt,BOOL* pbWindowChanged);
    BOOL MouseMove(LPARAM lParam,BOOL* pbWindowChanged);
};
