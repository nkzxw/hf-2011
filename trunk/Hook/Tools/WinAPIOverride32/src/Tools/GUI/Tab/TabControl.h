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
// Object: class helper for tab control
//-----------------------------------------------------------------------------

#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#include <vector>
#include <uxtheme.h>
// dynamic loading for win 2k compatibility --> remove static informations
// #pragma comment(lib,"UxTheme") 

class CTabControl
{
private:
    HWND hWndTabCtrl;
    RECT MaxChildDialogRect;
    RECT TabExternalRect;
    RECT TabInternalRect;
    int TabPosX;
    int TabPosY;
    BOOL SetTabXpStyle();
    HWND hwndDisplayedTab;
    HWND hDialog;
    SIZE_T SpaceBetweenControls;
    DLGTEMPLATE* WINAPI LockDialogResource(HMODULE hModule,LPCTSTR lpszResName);
    typedef struct TabInfos
    {
        DLGTEMPLATE* pDlgTemplate;
        HWND hWindow;
    }TAB_INFOS;
    std::vector<TAB_INFOS> TabDialogs;
    typedef HRESULT (__stdcall *pfEnableThemeDialogTexture)(HWND hwnd,DWORD dwFlags);
    pfEnableThemeDialogTexture pEnableThemeDialogTexture;

    void OnTabChanged();

public:
    CTabControl(HWND hDialog,HWND hWndTab,int TabPosX,int TabPosY,SIZE_T SpaceBetweenControls);
    ~CTabControl(void);

    typedef enum UNDER_BUTTONS_POSITION
    {
        UNDER_BUTTONS_POSITION_HORIZONTALLY_CENTERED_ON_TAB_CONTROL,
        UNDER_BUTTONS_POSITION_HORIZONTALLY_CENTERED_ON_DIALOG,
    };
    
    int GetItemCount();
    HWND GetControlHandle();

    int AddTab(TCHAR* pszTabName,HMODULE hModule,SIZE_T AssociatedDlgIDC,DLGPROC DialogProc,LPARAM lParamInit);
    int InsertTab(TCHAR* pszTabName,int Index,HMODULE hModule,SIZE_T AssociatedDlgIDC,DLGPROC DialogProc,LPARAM lParamInit);
    BOOL SetUnderButtonsAndAutoSizeDialog(HWND* phwndButtons,SIZE_T NbButtons,UNDER_BUTTONS_POSITION ButtonsPos);
    BOOL OnNotify(WPARAM wParam, LPARAM lParam);
    HWND GetTabItemWindowHandle(int ItemIndex);
};
