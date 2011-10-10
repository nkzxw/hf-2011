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
// Object: class helper for dialog window
//-----------------------------------------------------------------------------

#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
class CDialogHelper
{
private:
    typedef struct tagMoveGroupStruct
    {
        POINT  Point;
        HWND   hClient;
    }MOVE_GROUP_STRUCT,*PMOVE_GROUP_STRUCT;

    static BOOL CallBackRedrawGroup(HWND Item,PVOID UserParam);
    static BOOL CallBackShowGroup(HWND Item,PVOID UserParam);
    static BOOL CallBackEnableGroup(HWND Item,PVOID UserParam);
    static BOOL CallBackMoveGroup(HWND Item,PVOID UserParam);

public:
    /////////////////
    // dialog icon actions
    /////////////////
    static void SetIcon(HINSTANCE hInstance,HWND hWnd,int IdIconBig,int IdIconSmall);
    static void SetIcon(HINSTANCE hInstance,HWND hWnd,int IdIcon);
    static void SetIcon(HWND hWnd,int IdIconBig,int IdIconSmall);
    static void SetIcon(HWND hWnd,int IdIcon);
    static void SetIcon(HWND hWnd,HICON hIcon);
    static void SetIcon(HWND hWnd,HICON hIconBig,HICON hIconSmall);

    /////////////////
    // instance
    /////////////////
    static HINSTANCE GetInstance(HWND hWnd);

    /////////////////
    // positioning
    /////////////////
    static BOOL ScreenToClient (HWND hDlg, LPRECT pRect);
    static BOOL ClientToScreen (HWND hDlg, LPRECT pRect);
    static BOOL GetClientWindowRect(HWND hDlg,HWND hItem,LPRECT lpRect);
    static BOOL SetWindowPosUnderItem(HWND hClient,HWND hWnd,HWND hWindowReferenceItem,DWORD SpaceBetweenControls);
    static BOOL SetWindowPosAtRightOfItem(HWND hClient,HWND hWnd,HWND hWindowReferenceItem,DWORD SpaceBetweenControls);

    static BOOL GetDialogInternalRect(HWND hDlg,LPRECT lpRect);
    
    /////////////////
    // checked button
    /////////////////
    static BOOL IsButtonChecked(HWND hWnd);
    static BOOL IsButtonChecked(HWND hWndDialog,int ButtonID);
    static void SetButtonCheckState(HWND hWnd,BOOL bChecked);
    static void SetButtonCheckState(HWND hWndDialog,int ButtonID,BOOL bChecked);

    /////////////////
    // redraw
    /////////////////
    static BOOL Redraw(HWND hItem);

    /////////////////
    // transparency and fading
    /////////////////
    static void SetTransparency(HWND hWnd,BYTE Opacity);
    static void Fading(HWND hWnd,BYTE BeginOpacity,BYTE EndOpacity,BYTE Step);
    static void SetTransparentColor(HWND hWnd,COLORREF crKey);

    /////////////////
    // group actions
    /////////////////
    static BOOL RedrawGroup(HWND hWndGroup);
    static BOOL ShowGroup(HWND hWndGroup,int CmdShow);
    static BOOL EnableGroup(HWND hWndGroup,BOOL bEnable);
    static BOOL MoveGroupTo(HWND hClient,HWND hWndGroup,POINT* pPoint);
    static BOOL SetGroupPosUnderWindow(HWND hClient,HWND hWnd,HWND hWindowReferenceItem,DWORD SpaceBetweenControls);
    static BOOL SetGroupPosAtRightOfWindow(HWND hClient,HWND hWnd,HWND hWindowReferenceItem,DWORD SpaceBetweenControls);

    typedef BOOL (*pfCallBackGroupAction)(HWND Item,PVOID UserParam);
    static BOOL ParseGroupForAction(HWND hWndGroup,CDialogHelper::pfCallBackGroupAction CallBackFunction,PVOID UserParam);

    /////////////////
    // controls creation
    /////////////////
    static HWND CreateButton(HWND hWndParent,int x,int y,int Width,int Height,int Identifier,LPCTSTR ButtonText);
    static HWND CreateStatic(HWND hWndParent,int x,int y,int Width,int Height,int Identifier,LPCTSTR StaticText);
    static HWND CreateStaticCentered(HWND hWndParent,int x,int y,int Width,int Height,int Identifier,LPCTSTR StaticText);
    static HWND CreateEdit(HWND hWndParent,int x,int y,int Width,int Height,int Identifier,LPCTSTR EditText);
    static HWND CreateEditMultiLines(HWND hWndParent,int x,int y,int Width,int Height,int Identifier,LPCTSTR EditText);

};
