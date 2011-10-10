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
#include "tabcontrol.h"
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

CTabControl::CTabControl(HWND hDialog,HWND hWndTab,int TabPosX,int TabPosY,SIZE_T SpaceBetweenControls)
{
    this->pEnableThemeDialogTexture = NULL;
    HMODULE hModule = ::LoadLibrary(_T("UxTheme.dll"));
    if (hModule)
        this->pEnableThemeDialogTexture = (pfEnableThemeDialogTexture)::GetProcAddress(hModule,"EnableThemeDialogTexture");

    this->hWndTabCtrl=hWndTab;
    ::SetRectEmpty(&this->MaxChildDialogRect);
    ::SetRectEmpty(&this->TabExternalRect);
    this->TabPosX = TabPosX;
    this->TabPosY = TabPosY;
    this->hwndDisplayedTab = NULL;
    this->hDialog = hDialog;
    this->SpaceBetweenControls = SpaceBetweenControls;
    this->SetTabXpStyle();
}

CTabControl::~CTabControl(void)
{
    SIZE_T Cnt;
    for (Cnt = 0; Cnt< this->TabDialogs.size();Cnt++)
    {
        ::DestroyWindow(this->TabDialogs[Cnt].hWindow);
    }
}

int CTabControl::GetItemCount()
{
    return TabCtrl_GetItemCount(this->hWndTabCtrl);

}
HWND CTabControl::GetControlHandle()
{
    return this->hWndTabCtrl;
}

BOOL CTabControl::SetTabXpStyle()
{
    if (this->pEnableThemeDialogTexture)
        return (SUCCEEDED(this->pEnableThemeDialogTexture(this->hWndTabCtrl,ETDT_ENABLETAB)));
    else 
        return FALSE;
}

// LockDialogResource - loads and locks a dialog box template resource. 
// Returns the address of the locked resource. 
// lpszResName - name of the resource 
DLGTEMPLATE * WINAPI CTabControl::LockDialogResource(HMODULE hModule,LPCTSTR lpszResName) 
{ 
    HRSRC hrsrc = FindResource(hModule, lpszResName, RT_DIALOG); 
    HGLOBAL hglb = LoadResource(hModule, hrsrc); 
    return (DLGTEMPLATE *) LockResource(hglb); 
} 

// return tab index
int CTabControl::AddTab(TCHAR* pszTabName,HMODULE hModule,SIZE_T AssociatedDlgIDC,DLGPROC DialogProc,LPARAM lParamInit)
{
    return this->InsertTab(pszTabName,this->GetItemCount(),hModule,AssociatedDlgIDC,DialogProc,lParamInit);
}

// return tab index
int CTabControl::InsertTab(TCHAR* pszTabName,int Index,HMODULE hModule,SIZE_T AssociatedDlgIDC,DLGPROC DialogProc,LPARAM lParamInit)
{
    TAB_INFOS TabInfos={0};

    TCITEM tie; 
    tie.mask = TCIF_TEXT; 
    tie.pszText = pszTabName; 
    Index=TabCtrl_InsertItem(this->hWndTabCtrl, Index, &tie);

    // Lock the resources for the dialog box. 
    TabInfos.pDlgTemplate=this->LockDialogResource(hModule,MAKEINTRESOURCE(AssociatedDlgIDC)); 
    TabInfos.hWindow = CreateDialogIndirectParam(hModule, TabInfos.pDlgTemplate, this->hDialog, DialogProc, lParamInit); 
    if (this->pEnableThemeDialogTexture)
        this->pEnableThemeDialogTexture(TabInfos.hWindow,ETDT_ENABLETAB);

    if (Index>=(int)this->TabDialogs.size())
        this->TabDialogs.push_back(TabInfos);
    else
        this->TabDialogs.insert(this->TabDialogs.begin()+Index,TabInfos);

    RECT CurrentChildRect;
    ::GetWindowRect(TabInfos.hWindow,&CurrentChildRect);
    // get larger rect
    ::UnionRect(&this->MaxChildDialogRect,&this->MaxChildDialogRect,&CurrentChildRect);
 
    // Calculate how large to make the tab control, so 
    // the display area can accommodate all the child dialog boxes. 
    this->TabExternalRect = this->MaxChildDialogRect;
    TabCtrl_AdjustRect(this->hWndTabCtrl, TRUE, &this->TabExternalRect);
    int Dx =(int)(this->TabPosX + this->SpaceBetweenControls - this->TabExternalRect.left);
    int Dy =(int)(this->TabPosY + this->SpaceBetweenControls - this->TabExternalRect.top);
    ::OffsetRect(&this->TabExternalRect, Dx,Dy);

    ::SetWindowPos(this->hWndTabCtrl, 
                    NULL,
                    this->TabExternalRect.left,
                    this->TabExternalRect.top, 
                    this->TabExternalRect.right - this->TabExternalRect.left,
                    this->TabExternalRect.bottom - this->TabExternalRect.top, 
                    SWP_NOZORDER
                    );

    return Index;
}

BOOL CTabControl::SetUnderButtonsAndAutoSizeDialog(HWND* phwndButtons,SIZE_T NbButtons,UNDER_BUTTONS_POSITION ButtonsPos)
{
    SIZE_T Cnt;
    SIZE_T AllButtonsWidth;
    SIZE_T FirstButtonXPos;
    SIZE_T CurrentButtonXPos;
    SIZE_T ButtonHeight;
    SIZE_T MaxButtonHeight;
    SIZE_T MinWidth;
    SIZE_T TabWidth;
    RECT ButtonRect;

    // get full button with
    AllButtonsWidth = 0;
    MaxButtonHeight = 0;
    for (Cnt = 0;Cnt<NbButtons ; Cnt++)
    {
        ::GetWindowRect(phwndButtons[Cnt], &ButtonRect); 
        AllButtonsWidth+= (ButtonRect.right - ButtonRect.left);
        ButtonHeight = (ButtonRect.bottom - ButtonRect.top);
        if(ButtonHeight>MaxButtonHeight)
            MaxButtonHeight = ButtonHeight;
        if (Cnt)
            AllButtonsWidth+=this->SpaceBetweenControls;
    }

    // get first button x position
    TabWidth = this->TabExternalRect.right - this->TabExternalRect.left;
    if (ButtonsPos == UNDER_BUTTONS_POSITION_HORIZONTALLY_CENTERED_ON_TAB_CONTROL)
    {
        if ( TabWidth >AllButtonsWidth )
            FirstButtonXPos = (TabWidth - AllButtonsWidth) / 2 + this->TabPosX;
        else
            FirstButtonXPos = this->SpaceBetweenControls + this->TabPosX;
    }
    else // if (ButtonsPos == UNDER_BUTTONS_POSITION_HORIZONTALLY_CENTERED_ON_DIALOG)
    {
        if ( TabWidth + this->TabPosX >AllButtonsWidth )
            FirstButtonXPos = (TabWidth + this->TabPosX - AllButtonsWidth) / 2;
        else
            FirstButtonXPos = this->SpaceBetweenControls;
    }
    MinWidth = __max(AllButtonsWidth, TabWidth + this->TabPosX);
    MinWidth += 2*this->SpaceBetweenControls;

    // set buttons positions
    CurrentButtonXPos = FirstButtonXPos;
    for (Cnt = 0;Cnt<NbButtons ; Cnt++)
    {
        ::SetWindowPos(phwndButtons[Cnt], NULL, CurrentButtonXPos, this->TabExternalRect.bottom + this->SpaceBetweenControls, 0, 0,SWP_NOSIZE | SWP_NOZORDER); 
        ::GetWindowRect(phwndButtons[Cnt], &ButtonRect); 
        CurrentButtonXPos+= (ButtonRect.right - ButtonRect.left)+this->SpaceBetweenControls;
    }

    // set child dialog positions
    RECT TabInternalRect;
    RECT CurrentChildRect;
    TabInternalRect = this->TabExternalRect;
    TabCtrl_AdjustRect(this->hWndTabCtrl, FALSE, &TabInternalRect);
    for (Cnt = 0; Cnt< this->TabDialogs.size();Cnt++)
    {
        ::GetWindowRect(this->TabDialogs[Cnt].hWindow,&CurrentChildRect);
        ::SetWindowPos(this->TabDialogs[Cnt].hWindow,
                       HWND_TOP, 
                       TabInternalRect.left
                       // + to center child horizontally
                       +((TabInternalRect.right - TabInternalRect.left) - (CurrentChildRect.right - CurrentChildRect.left)) /2, 
                       TabInternalRect.top, 
                       0, 
                       0, 
                       SWP_NOSIZE
                       );
    }
    

    // Size the dialog box. 
    ::SetWindowPos(this->hDialog, NULL,
        0, 
        0, 
        MinWidth + 2 * ::GetSystemMetrics(SM_CXDLGFRAME), 
        this->TabExternalRect.bottom + MaxButtonHeight + 2 * this->SpaceBetweenControls + ::GetSystemMetrics(SM_CYCAPTION) + ::GetSystemMetrics(SM_CYDLGFRAME) ,
        SWP_NOMOVE | SWP_NOZORDER); 

    // Simulate selection of the first item. 
    this->OnTabChanged();

    return TRUE;
}

HWND CTabControl::GetTabItemWindowHandle(int ItemIndex)
{
    if (ItemIndex>=(int)this->TabDialogs.size())
        return NULL;

    return this->TabDialogs[ItemIndex].hWindow;
}

void CTabControl::OnTabChanged()
{
    int iSel = TabCtrl_GetCurSel(this->hWndTabCtrl); 
    if (iSel == -1)
        iSel = 0;

    // Destroy the current child dialog box, if any. 
    if (this->hwndDisplayedTab != NULL) 
        ::ShowWindow(this->hwndDisplayedTab,FALSE);

    if ((int)this->TabDialogs.size()>=iSel)
    {
        this->hwndDisplayedTab = this->TabDialogs[iSel].hWindow;
        ::ShowWindow(this->hwndDisplayedTab,TRUE);
    }
}

//-----------------------------------------------------------------------------
// Name: OnNotify
// Object: a WM_NOTIFY message helper for tab 
//         Must be called in the WndProc when receiving a WM_NOTIFY message 
// Parameters :
//     in  : WPARAM wParam : WndProc wParam
//           LPARAM lParam : WndProc lParam
//     out : 
//     return : TRUE if message have been internally proceed
//-----------------------------------------------------------------------------
BOOL CTabControl::OnNotify(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    NMHDR* pNmhdr=(NMHDR*)lParam;
    if (pNmhdr->hwndFrom==this->hWndTabCtrl)
    {
        switch (pNmhdr->code) 
        { 
        case TCN_SELCHANGE: 
            this->OnTabChanged();
            return TRUE;
        } 

    }
    return FALSE;
}