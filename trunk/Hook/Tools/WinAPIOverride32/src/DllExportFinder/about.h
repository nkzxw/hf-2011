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
// Object: manages the about dialog
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <windowsx.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "resource.h"
#include "../tools/GUI/SysLink/SysLink.h"
#include "../tools/GUI/Dialog/DialogHelper.h"
#include "../Tools/Version/Version.h"
#include "../Tools/File/StdFileOperations.h"

#define ID_ABOUT_SYSLINK 10001

#define ABOUT_SYSLINK_TEXT _T("For more information, see the <A ID=\"IdHomePage\">project homepage</A>\r\nor <A ID=\"IdUserDoc\">user manual</A>\r\nAuthor : Jacquelin POTIER\r\n               <A ID=\"IdMailto\">jacquelin.potier@free.fr</A>")
#define ABOUT_SYSLINK_PROJECT_LINK _T("http://jacquelin.potier.free.fr/DllExportFinder/")
#define ABOUT_SYSLINK_PROJECT_USER_DOC_LINK _T("http://jacquelin.potier.free.fr/DllExportFinder/index.php#documentation_user")
#define ABOUT_SYSLINK_EMAIL_LINK _T("mailto:jacquelin.potier@free.fr")

#define ABOUT_SYSLINK_XPOS 20
#define ABOUT_SYSLINK_YPOS 60
#define ABOUT_SYSLINK_WIDTH 350
#define ABOUT_SYSLINK_HEIGHT 65

#define ABOUT_OPACITY 95
#define ABOUT_FADING_STEP 4

#define ABOUT_TITLE_FONT_HEIGHT 18
#define ABOUT_LINK_FONT_HEIGHT 16


void ShowAboutDialog(HINSTANCE hInstance,HWND hWndDialog);
void AboutDlgExit();
LRESULT CALLBACK AboutWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);