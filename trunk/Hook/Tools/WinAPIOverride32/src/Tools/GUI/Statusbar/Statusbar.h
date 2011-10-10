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
// Object: class helper for StatusBar control
//-----------------------------------------------------------------------------
#pragma once

// required lib : comctl32.lib
#pragma comment (lib,"comctl32")

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif

#include <windows.h>
#include <commctrl.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#define CStatusbar_MAX_PARTS 256 // MSDN 256 parts max
#define CStatusbar_DEFAULT_SIZE (-1)
class CStatusbar
{

public:
    // int PartIndex : 0 based part PartIndex
    // POINT* pMousePoint : control relative screen coordinates [add GetWindowRect(hwndStatus,&Rect);]
    typedef void (*pfMouseClickCallBack)(int PartIndex,POINT* pMousePoint,PVOID UserParam);

    CStatusbar(HINSTANCE hInstance,HWND hwndParent);
    ~CStatusbar(void);
    HWND GetControlHandle();
    BOOL OnSize(WPARAM wParam, LPARAM lParam);
    BOOL OnNotify(WPARAM wParam, LPARAM lParam);

    int AddPart();
    int AddPart(TCHAR* PartText);
    int AddPart(TCHAR* PartText,TCHAR* ToolTip);
    int AddPart(TCHAR* PartText,HICON hIcon);
    int AddPart(TCHAR* PartText,HINSTANCE hInstance,int IdIcon);
    int AddPart(HICON hIcon,TCHAR* ToolTip);
    int AddPart(HINSTANCE hInstance,int IdIcon,TCHAR* ToolTip);
    int AddPart(int Size);
    int AddPart(TCHAR* PartText,int Size);
    int AddPart(TCHAR* PartText,TCHAR* ToolTip,int Size);
    int AddPart(TCHAR* PartText,HICON hIcon,int Size);
    int AddPart(TCHAR* PartText,HINSTANCE hInstance,int IdIcon,int Size);
    int AddPart(HICON hIcon,TCHAR* ToolTip,int Size);

    BOOL SetIcon(int PartIndex,HICON hIcon);
    BOOL SetIcon(int PartIndex,HINSTANCE hInstance,int IdIcon);
    BOOL SetText(int PartIndex,TCHAR* PartText);
    BOOL SetToolTip(int PartIndex,TCHAR* ToolTip);
    BOOL SetSize(int PartIndex,int Size);

    void SetClickCallBack(pfMouseClickCallBack CallBack,PVOID UserParam);
    void SetRightClickCallBack(pfMouseClickCallBack CallBack,PVOID UserParam);
    void SetDoubleClickCallBack(pfMouseClickCallBack CallBack,PVOID UserParam);
    void SetDoubleRightClickCallBack(pfMouseClickCallBack CallBack,PVOID UserParam);

private:
    HINSTANCE hInstance;
    HWND hwndStatus;
    int PartsWidths[CStatusbar_MAX_PARTS];
    int NbParts;
    pfMouseClickCallBack ClickCallBack;
    PVOID ClickCallBackUserParam;
    pfMouseClickCallBack DoubleClickCallBack;
    PVOID DoubleClickCallBackUserParam;
    pfMouseClickCallBack RightClickCallBack;
    PVOID RightClickCallBackUserParam;
    pfMouseClickCallBack DoubleRightClickCallBack;
    PVOID DoubleRightClickCallBackUserParam;
};
