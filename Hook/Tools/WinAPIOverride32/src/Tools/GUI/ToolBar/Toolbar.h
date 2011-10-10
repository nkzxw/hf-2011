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
// Object: class helper for Toolbar control
//-----------------------------------------------------------------------------

#pragma once

// required lib : comctl32.lib
#pragma comment (lib,"comctl32")
// require manifest to make SetView work

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif

#include <windows.h>
#include <commctrl.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "../menu/popupmenu.h" // for drop down menu

#define CToolbar_DEFAULT_IMAGELIST_SIZE 20
#define CToolbar_IMAGELIST_GROW_SIZE 5

#define CToolbar_MARKER_PROP_NAME _T("CToolbarMarkerPropName")

class CToolbarButtonUserData
{
public:
    CToolbarButtonUserData(TCHAR* ToolTip,CPopUpMenu* PopUpMenu);
    ~CToolbarButtonUserData();
    void SetToolTip(TCHAR* ToolTip);
    TCHAR* ToolTip;
    CPopUpMenu* PopUpMenu;
};

class CToolbar
{
public:
    typedef void (*tagDropDownMenuCallBack)(CPopUpMenu* PopUpMenu,UINT MenuId,PVOID UserParam);
    enum tagCToolbarPosition
    {
        ToolbarPosition_USER=0,
        ToolbarPosition_TOP=CCS_TOP,
        ToolbarPosition_LEFT=CCS_LEFT,
        ToolbarPosition_BOTTOM=CCS_BOTTOM,
        ToolbarPosition_RIGHT=CCS_RIGHT
     };
private:
    HIMAGELIST hImgList;
    HIMAGELIST hImgListDisabled;
    HIMAGELIST hImgListHot;
    HWND hwndTB;
    HWND hwndToolTip;
    HINSTANCE hInstance;
    PVOID DropDownMenuUserParam;
    tagDropDownMenuCallBack pDropDownMenuCallBack;
    void Constructor(HINSTANCE hInstance,HWND hwndParent,BOOL bFlat,BOOL bListStyle,DWORD dwIconWidth,DWORD dwIconHeight);
public:
    enum ImageListType
    {
        ImageListTypeEnable,
        ImageListTypeDisable,
        ImageListTypeHot
    };

    CToolbar(HINSTANCE hInstance,HWND hwndParent);
    CToolbar(HINSTANCE hInstance,HWND hwndParent,BOOL bFlat,BOOL bListStyle);
    CToolbar(HINSTANCE hInstance,HWND hwndParent,BOOL bFlat,BOOL bListStyle,DWORD dwIconWidth,DWORD dwIconHeight);
    ~CToolbar(void);
    BOOL OnNotify(WPARAM wParam, LPARAM lParam);
    BOOL SetDropDownMenuCallBack(tagDropDownMenuCallBack pDropDownMenuCallBack,PVOID UserParam);
    BOOL GetDropDownMenuCallBack(tagDropDownMenuCallBack* ppDropDownMenuCallBack,PVOID* pUserParam);

    HWND GetControlHandle();
    void EnableButton(int ButtonID,BOOL bEnable);
    

    void AddSeparator();
    int GetButtonCount();

    BOOL RemoveButton(int ButtonID);
    int GetButtonIndex(int ButtonID);
    int GetButtonId(int ButtonIndex);
    BYTE GetButtonStyle(int ButtonID);
    BYTE GetButtonState(int ButtonID);
    BOOL SetButtonState(int ButtonID,BYTE State);

    BOOL AddButton(int ButtonID,TCHAR* ButtonText);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,TCHAR* ToolTip);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,int IdIcon);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,int IdIcon,TCHAR* ToolTip);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable,TCHAR* ToolTip);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable,int IdIconHot);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip);
    BOOL AddButton(int ButtonID,int IdIcon,TCHAR* ToolTip);
    BOOL AddButton(int ButtonID,int IdIconEnable,int IdIconDisable,TCHAR* ToolTip);
    BOOL AddButton(int ButtonID,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,BYTE ButtonStyle,BYTE ButtonState,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip);
    BOOL AddButton(int ButtonID,TCHAR* ButtonText,BYTE ButtonStyle,BYTE ButtonState,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip,CPopUpMenu* PopUpMenu);

    BOOL AddDropDownButton(int ButtonID,int IdIcon,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,BOOL bWholeDropDown);
    BOOL AddDropDownButton(int ButtonID,TCHAR* ButtonText,int IdIcon,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,BOOL bWholeDropDown);
    BOOL AddDropDownButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,BOOL bWholeDropDown);
    BOOL AddDropDownButton(int ButtonID,TCHAR* ButtonText,BYTE ButtonState,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,BOOL bWholeDropDown);
    CPopUpMenu* GetButtonDropDownMenu(int ButtonID);

    BOOL AddCheckButton(int ButtonID,int IdIcon,TCHAR* ToolTip);
    BOOL AddCheckButton(int ButtonID,int IdIcon,int IdIconDisabled,int IdIconHot,TCHAR* ToolTip);
    
    void Autosize();
    BOOL EnableParentAlign(BOOL bEnable);
    BOOL SetDirection(BOOL bHorizontal);
    BOOL EnableDivider(BOOL bEnable);
    BOOL SetPosition(tagCToolbarPosition Position);

    BOOL ReplaceIcon(int ButtonID,CToolbar::ImageListType ImgListType,int IdNewIcon);
    BOOL ReplaceText(int ButtonID,TCHAR* NewText);
    BOOL ReplaceToolTipText(int ButtonID,TCHAR* NewToolTipText);
};
