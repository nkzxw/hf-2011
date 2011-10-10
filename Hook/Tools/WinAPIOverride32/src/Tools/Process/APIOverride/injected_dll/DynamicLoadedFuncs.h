/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
originaly based from APISpy32 v2.1 from Yariv Kaplan @ WWW.INTERNALS.COM

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
// Object: avoid static linking with library other than kernel32
//-----------------------------------------------------------------------------

#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "../../../GUI/DynamicMessageBox/DynamicMessageBox.h"
#include "SetUserWindowStation.h"

int __stdcall DynamicMessageBoxInDefaultStation(IN HWND hWnd,IN TCHAR* lpText,IN TCHAR* lpCaption,IN UINT uType);

BOOL DynamicDeleteObject(HGDIOBJ hObject);

HFONT DynamicCreateFont(
  int nHeight,               // height of font
  int nWidth,                // average character width
  int nEscapement,           // angle of escapement
  int nOrientation,          // base-line orientation angle
  int fnWeight,              // font weight
  DWORD fdwItalic,           // italic attribute option
  DWORD fdwUnderline,        // underline attribute option
  DWORD fdwStrikeOut,        // strikeout attribute option
  DWORD fdwCharSet,          // character set identifier
  DWORD fdwOutputPrecision,  // output precision
  DWORD fdwClipPrecision,    // clipping precision
  DWORD fdwQuality,          // output quality
  DWORD fdwPitchAndFamily,   // pitch and family
  TCHAR* lpszFace           // typeface name
);

INT_PTR DynamicDialogBoxParam(HINSTANCE hInstance,
    TCHAR* lpTemplateName,
    HWND hWndParent,
    DLGPROC lpDialogFunc,
    LPARAM dwInitParam
);

BOOL DynamicEndDialog(HWND hDlg,
    INT_PTR nResult
);
HWND DynamicGetDlgItem(HWND hDlg,
    int nIDDlgItem
);
LRESULT DynamicSendMessage(HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
);
LONG DynamicGetWindowLong(          HWND hWnd,
    int nIndex
);
LONG DynamicSetWindowLongPtr(          HWND hWnd,
    int nIndex,
    LONG dwNewLong
);

BOOL DynamicEnableWindow(HWND hWnd,BOOL bEnable);