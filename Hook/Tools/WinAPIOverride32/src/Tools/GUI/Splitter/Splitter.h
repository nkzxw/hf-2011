/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

Based on Idea of CCSplitter by R.W.G. Hünen (rhunen@xs4all.nl) and Rob Pitt.

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
// Object: class for a Splitter control
//-----------------------------------------------------------------------------


#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <windows.h>
#include <commctrl.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#define CSPLITTER_SPLITTER_SIZE 3
#define CSPLITTER_SPLITTER_CLASSEX _T("CSplitter")
#define CSPLITTER_ID_STATIC_BAR 0
#define CSPLITTER_ID_STATIC_ICON 1
class CSplitter
{
public:
    typedef void (*tagMoveCallBack)(int LeftOrTopSplitterPos,int RightOrBottomSplitterPos,PVOID UserParam);
    typedef void (*tagCollapsedStateChange)(BOOL NewCollapsedState,PVOID UserParam);
    
private:
    HWND hWndParent;
    HWND hWndSplitter;
    HWND hWndSplitterBar;
    HWND hWndSplitterIcon;
    PVOID UserParam;
    PVOID CollapsedStateChangeUserParam;

    BOOL bCreated;
    int Width;// width of control (height for horizontal and width for vertical)
    BOOL bMouseOverIcon;
    BOOL bTopOrLeft;
    BOOL bCapture;
    BOOL bCollapsed;
    BOOL bVerticalSplitter;
    BOOL bLeftButtonDown;
    HCURSOR	hcurVert;		// Cursor for North/South split
    HCURSOR	hcurHorz;		// Cursor for East/West split
    HICON hIconExpanded;
    HICON hIconExpandedHot;
    HICON hIconExpandedDown;
    HICON hIconCollapsed;
    HICON hIconCollapsedHot;
    HICON hIconCollapsedDown;
    HBRUSH BackGroundBrush;
    HBRUSH SplitterBrush;
    int IconWidth;
    int IconHeight;
    float Percent;// current percent position from top or left
    float LastExpendedPercentPosition;// last expended position from top or left
    HINSTANCE hInstance;

    tagMoveCallBack pMoveCallBack;
    tagCollapsedStateChange pCollapsedStateChangeCallBack;
    
    BOOL CreateControl();

    void UpdatePercent(int Position);
    void GetSplitterRect(RECT* pOutSplitterRect);
    void GetSplitterBarRect(RECT* pSplitterRect,RECT* pRect);
    void GetIconRect(RECT* pSplitterRect,RECT* pRect);
    
    BOOL IsMouseOverIcon();
    BOOL IsMouseOverSplitter();

    void OnLButtonDown(WPARAM wParam, LPARAM lParam);
    void OnLButtonUp(WPARAM wParam, LPARAM lParam);
    void OnMouseMove(WPARAM wParam, LPARAM lParam);
    void OnMouseLeave(WPARAM wParam, LPARAM lParam);
       
    enum IconType{IconType_NORMAL,IconType_HOT,IconType_DOWN};
    void SetIconImg(IconType Type);
    void Show(BOOL bShow);
    void Draw(RECT* SplitterRect);
    static LRESULT CALLBACK WindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:

    int LeftMinFreeSpace;
    int RightMinFreeSpace;
    int TopMinFreeSpace;
    int BottomMinFreeSpace;

    BOOL AllowSizing;
    
    CSplitter(HINSTANCE hInstance,HWND hParent,BOOL bInitiallyVisible,
        BOOL Vertical,BOOL TopOrLeft,BOOL StartCollapsed,int DefaultExpandedPercent,
        DWORD ID_IconExpanded,DWORD ID_IconExpandedHot,DWORD ID_IconExpandedDown,
        DWORD ID_IconCollapsed,DWORD ID_IconCollapsedHot,DWORD ID_IconCollapsedDown,
        int IconWidth,int IconHeight);
    ~CSplitter(void);

    BOOL Show();
    BOOL Hide();
    void Redraw();// can be called parent WM_SIZE to auto recompute it's size
    void Collapse();
    void Expand();

    BOOL IsCollapsed();
    BOOL GetRect(RECT* pRect);
    int GetThickness();

    void SetMoveCallBack(tagMoveCallBack MoveCallBack,PVOID UserParam);
    void SetCollapsedStateChangeCallBack(tagCollapsedStateChange CollapsedStateChangeCallBack,PVOID UserParam);

    void SetBackGroundBrush(HBRUSH Brush);
    void SetSplitterBarBrush(HBRUSH Brush);
};

