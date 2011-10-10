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
#include <malloc.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "../Menu/PopUpMenu.h"
#include "../toolbar/toolbar.h"// for drop down menu access

class CRebar
{
public:
    CRebar(HINSTANCE hInstance,HWND hwndParent);
    virtual ~CRebar(void);

    HWND GetControlHandle();
    UINT AddToolBarBand(HWND hwndToolBar);
    UINT AddToolBarBand(HWND hwndToolBar,TCHAR* Name);
    UINT AddToolBarBand(HWND hwndToolBar,TCHAR* Name,BOOL NewLine,BOOL FixedSize,BOOL NoGripper);

    UINT AddBand(HWND hwnd);
    UINT AddBand(HWND hwnd,TCHAR* Name);
    UINT AddBand(HWND hwnd,TCHAR* Name,BOOL NewLine,BOOL FixedSize,BOOL NoGripper);

    SIZE_T GetBandCount();
    UINT GetHeight();

    BOOL OnSize(WPARAM wParam, LPARAM lParam);
    BOOL OnNotify(WPARAM wParam, LPARAM lParam);
protected:
    HWND hwndRebar;
    UINT BandId;
    typedef struct CHEVRON_MENU_INFOS
    {
        CPopUpMenu* pToolBarDropDownMenu;// toolbar Menu
        UINT CommandId;// toolbar command Id 
        UINT MenuId;   // chevron menu id
        UINT OriginalMenuId; // original menu id (for drop down menu and sub menus)
        CPopUpMenu* pOriginalMenu;// original menu item is from

        // sub menus infos
        CPopUpMenu* pSubMenu; // sub menu associated to current menu item (if any)
        CHEVRON_MENU_INFOS* pSubMenuChevronInfosArray; // sub menu associated to current menu item infos (if any sub menu)
    }CHEVRON_MENU_INFOS,*PCHEVRON_MENU_INFOS;
    void ParseSubMenu(CPopUpMenu* pRootMenu,CPopUpMenu* pToolBarDropDownMenu,CPopUpMenu* pPopUpMenu,CHEVRON_MENU_INFOS* pChevronMenuInfos);
    void RestoreSubMenuIds(CHEVRON_MENU_INFOS* pChevronMenuInfos,BOOL bChevronMenu);
    BOOL GetChevronMenuInfos(IN UINT MenuId,IN CHEVRON_MENU_INFOS* pMenuInfos,OUT UINT* pMenuOrCommandId,OUT CPopUpMenu** ppDropDownMenu,OUT BOOL* pIsMenuId);
};
