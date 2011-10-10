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
// Object: class helper for tooltip control
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

class CToolTip
{
private:
    HWND hwndToolTip;
public:
    enum TOOLTIP_ICON
    {
        ICON_NONE=0,ICON_INFO=1,ICON_WARNING=2,ICON_ERROR=3
    };
    CToolTip(HWND ControlHandle,TCHAR* pszText,BOOL BalloonTooltip);
    ~CToolTip();
    HWND GetControlHandle();
    LRESULT SetTitle(TCHAR* Title,TOOLTIP_ICON Icon);
    LRESULT SetTextColor(COLORREF Color);
    LRESULT SetBackGroundColor(COLORREF Color);
};
