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
// Object: class helper for Syslink control
//-----------------------------------------------------------------------------

#pragma once

// required lib : comctl32.lib
#pragma comment (lib,"comctl32")
// require manifest

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif

#include <windows.h>
#include <commctrl.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "../../String/AnsiUnicodeConvert.h"

// for dynamic loading
typedef HWND (WINAPI *ptrCreateWindowExW)(
    IN DWORD dwExStyle,
    IN LPCWSTR lpClassName,
    IN LPCWSTR lpWindowName,
    IN DWORD dwStyle,
    IN int X,
    IN int Y,
    IN int nWidth,
    IN int nHeight,
    IN HWND hWndParent,
    IN HMENU hMenu,
    IN HINSTANCE hInstance,
    IN LPVOID lpParam);

class CSysLink
{
private:
    ptrCreateWindowExW pCreateWindowExW;
    HMODULE hModuleHandle;
    BOOL bLibUser32Loaded;
    HWND hWindowHandle;
    BOOL bInitializeDone;
    void Initialize(HWND hWndParent,int ControlID,LPCWSTR Text,int XPos,int YPos,int Width,int Height);
public:
    
    CSysLink(HWND hWndParent,int ControlID,LPCSTR Text,int XPos,int YPos,int Width,int Height);
    CSysLink(HWND hWndParent,int ControlID,LPCWSTR Text,int XPos,int YPos,int Width,int Height);
    ~CSysLink(void);
    BOOL Notify(LPARAM lParam,LPWSTR IdName);
    BOOL OpenLink(LPCTSTR URL);
    HWND FORCEINLINE GetControlHandle(){return this->hWindowHandle;}
};
