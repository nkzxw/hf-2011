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
// Object: display generated asm opcodes
//-----------------------------------------------------------------------------

#include "NetInteraction.h"
#pragma comment (lib,"comctl32.lib")

extern HINSTANCE DllhInstance;
extern CLinkListSimple* pLinkListOpenDialogs;
extern HANDLE hSemaphoreOpenDialogs;


CDisplayAsm::CDisplayAsm(void)
{
    this->hInstance=DllhInstance;
    this->DialogResult=0;
    this->hWndDialog=0;
    this->Content=0;
    this->Title=0;
}
CDisplayAsm::~CDisplayAsm(void)
{

}

//-----------------------------------------------------------------------------
// Name: Show
// Object: show the dialog box
// Parameters :
//     in  : TCHAR* Title : dialog caption
//           TCHAR* DisasmContent : content
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CDisplayAsm::Show(TCHAR* Title,TCHAR* DisasmContent)
{
    INT_PTR Ret;
    CDisplayAsm* pDialog=new CDisplayAsm();
    pDialog->hInstance=DllhInstance;
    pDialog->Content=DisasmContent;
    pDialog->Title=Title;

    Ret=DialogBoxParam(DllhInstance,(LPCTSTR)IDD_DIALOG_ASM,NULL,(DLGPROC)CDisplayAsm::WndProc,(LPARAM)pDialog);

    delete pDialog;
    return Ret;
}

//-----------------------------------------------------------------------------
// Name: GetAssociatedDialogObject
// Object: Get object associated to window handle
// Parameters :
//     in  : HWND hWndDialog : handle of the window
//     out :
//     return : associated object if found, NULL if not found
//-----------------------------------------------------------------------------
CDisplayAsm* CDisplayAsm::GetAssociatedDialogObject(HWND hWndDialog)
{
    return (CDisplayAsm*)GetWindowLongPtr(hWndDialog,GWLP_USERDATA);
}

//-----------------------------------------------------------------------------
// Name: OnSizing
// Object: check dialog box size
// Parameters :
//     in out  : RECT* pRect : pointer to dialog rect
//     return : 
//-----------------------------------------------------------------------------
void CDisplayAsm::OnSizing(RECT* pRect)
{
    // check min width and min height
    if ((pRect->right-pRect->left)<CDisplayAsm_DIALOG_MIN_WIDTH)
        pRect->right=pRect->left+CDisplayAsm_DIALOG_MIN_WIDTH;
    if ((pRect->bottom-pRect->top)<CDisplayAsm_DIALOG_MIN_HEIGHT)
        pRect->bottom=pRect->top+CDisplayAsm_DIALOG_MIN_HEIGHT;
}

//-----------------------------------------------------------------------------
// Name: OnSize
// Object: Resize controls
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDisplayAsm::OnSize()
{
    RECT RectWindow;
    RECT Rect;
    DWORD SpaceBetweenControls;

    // get window Rect
    GetClientRect(this->hWndDialog,&RectWindow);


    ///////////////
    // edit 
    ///////////////
    HWND hItem=GetDlgItem(this->hWndDialog,IDC_EDIT_ASM);
    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    SpaceBetweenControls=Rect.left;
    SetWindowPos(hItem,HWND_NOTOPMOST,
        0,0,
        (RectWindow.right-RectWindow.left)-2*SpaceBetweenControls,
        RectWindow.bottom-RectWindow.top-SpaceBetweenControls,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);

    // redraw dialog
    CDialogHelper::Redraw(this->hWndDialog);
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDisplayAsm::Init()
{
    this->pItemDialog=pLinkListOpenDialogs->AddItem(this->hWndDialog);

    
    SetWindowText(this->hWndDialog,this->Title);

    HWND hItem=GetDlgItem(this->hWndDialog,IDC_EDIT_ASM);
    SetWindowText(hItem,this->Content);

    // MSDN Q96674 : avoid text selection
    SetFocus(hItem);
    PostMessage(hItem,EM_SETSEL,0,0);
    this->hFont=CreateFont(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, NULL);
    PostMessage((HWND) hItem,(UINT) WM_SETFONT,(WPARAM)this->hFont,FALSE);

    // render layout
    this->OnSize();
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDisplayAsm::Close()
{
    DeleteObject(this->hFont);
    // if pLinkListOpenDialogs is not locked by dll unload
    if (!hSemaphoreOpenDialogs)
        pLinkListOpenDialogs->RemoveItem(this->pItemDialog);

    EndDialog(this->hWndDialog,this->DialogResult);

    return;
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback of the dump dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CDisplayAsm::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            CDisplayAsm* pDialog=(CDisplayAsm*)lParam;
            pDialog->hWndDialog=hWnd;

            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pDialog);

            pDialog->Init();
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_ASM);
        }
        break;
    case WM_CLOSE:
        {
            CDisplayAsm* pDialog=CDisplayAsm::GetAssociatedDialogObject(hWnd);
            if (!pDialog)
                break;
            pDialog->Close();
        }
        break;
    case WM_SIZING:
        {
            CDisplayAsm* pDialog=CDisplayAsm::GetAssociatedDialogObject(hWnd);
            if (!pDialog)
                break;
            pDialog->OnSizing((RECT*)lParam);
        }
        break;
    case WM_SIZE:
        {
            CDisplayAsm* pDialog=CDisplayAsm::GetAssociatedDialogObject(hWnd);
            if (!pDialog)
                break;
            pDialog->OnSize();
        }
        break;
    //case WM_COMMAND:
    //    {
    //        CDisplayAsm* pDialog=CDisplayAsm::GetAssociatedDialogObject(hWnd);
    //        if (!pDialog)
    //            break;
    //        switch (LOWORD(wParam))
    //        {

    //        }
    //    }
    //    break;
    default:
        return FALSE;
    }
    return TRUE;
}
