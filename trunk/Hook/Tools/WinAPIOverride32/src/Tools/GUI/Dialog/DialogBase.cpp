/*
Copyright (C) 2010 Jacquelin POTIER <jacquelin.potier@free.fr>
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

#include "DialogBase.h"

CDialogBase::CDialogBase(void)
{
    this->hWndDialog=NULL;
    this->IconResourceId=NULL;
    this->hInstance=NULL;
    this->bDragAcceptFileRequired=FALSE;
    this->bDragAcceptFileStatusEnabled=FALSE;
}

void CDialogBase::EnableDragAndDrop(BOOL bEnable)
{
    if (!this->hWndDialog)
    {
        this->bDragAcceptFileRequired = TRUE;
        return;
    }
    
    HMODULE hModule = ::GetModuleHandle(CDialogBase_SHELL_DLL);
    if (!hModule)
    {
        hModule =::LoadLibrary(CDialogBase_SHELL_DLL);
        if (!hModule)
            return;
    }

    typedef void (__stdcall *pfDragAcceptFiles)(HWND hWnd, BOOL fAccept);
    pfDragAcceptFiles pDragAcceptFiles;
    pDragAcceptFiles = (pfDragAcceptFiles) ::GetProcAddress(hModule,"DragAcceptFiles");
    if (!pDragAcceptFiles)
        return;
    pDragAcceptFiles(this->hWndDialog,bEnable);
    this->bDragAcceptFileStatusEnabled = bEnable;
}

CDialogBase::~CDialogBase(void)
{
}

//-----------------------------------------------------------------------------
// Name: ReportMessage
// Object: add an information message into the listview and log list
// Parameters :
//     in  : CApiOverride* pApiOverride : Api override object for which the message applies
//           TCHAR* pszMsg : message to display
//           ReportMessageTypes MsgType : type of message (information, error , warning)
//     return : 
//-----------------------------------------------------------------------------
void CDialogBase::ReportMessage(TCHAR* pszMsg, ReportMessageTypes MsgType)
{
    TCHAR* pszCaption;
    UINT MessageBoxFlag = 0;
    switch(MsgType)
    {
    default:
    case ReportMessageType_ERROR:
        pszCaption = _T("Error");
        MessageBoxFlag = MB_ICONERROR;
        break;
    case ReportMessageType_WARNING:
        pszCaption = _T("Warning");
        MessageBoxFlag = MB_ICONWARNING;
        break;
    case ReportMessageType_INFO:
        pszCaption = _T("Information");
        MessageBoxFlag = MB_ICONINFORMATION;
        break;
    }

    ::MessageBox(this->hWndDialog,pszMsg,pszCaption,MB_OK|MessageBoxFlag);
        
}
void CDialogBase::ReportError(TCHAR* pszMsg)
{
    return this->ReportMessage(pszMsg,ReportMessageType_ERROR);
}

//-----------------------------------------------------------------------------
// Name: Show
// Object: show the filter dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CDialogBase::Show(HINSTANCE hInstance,HWND hWndParentDialog,int ResourceId,int IconResourceId)
{
    this->IconResourceId = IconResourceId;
    this->hInstance = hInstance;
    return ::DialogBoxParam(hInstance,(LPCTSTR)ResourceId,hWndParentDialog,(DLGPROC)CDialogBase::WndProc,(LPARAM)this);
}

void CDialogBase::Close(int Code)
{
    if (this->bDragAcceptFileStatusEnabled)
        this->EnableDragAndDrop(FALSE);

    this->OnClose();

    ::SetWindowLongPtr(this->hWndDialog,GWLP_USERDATA,(LONG_PTR)NULL);
    ::EndDialog(this->hWndDialog,Code);
}


//-----------------------------------------------------------------------------
// Name: GetAssociatedObject
// Object: Get object associated to window handle
// Parameters :
//     in  : HWND hWndDialog : handle of the window
//     out :
//     return : associated object if found, NULL if not found
//-----------------------------------------------------------------------------
CDialogBase* CDialogBase::GetAssociatedObject(HWND hWndDialog)
{
    return (CDialogBase*)GetWindowLongPtr(hWndDialog,GWLP_USERDATA);
}
//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback of the dump dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CDialogBase::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            CDialogBase* pDialogBase=(CDialogBase*)lParam;
            pDialogBase->hWndDialog=hWnd;

            ::SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pDialogBase);

            if (pDialogBase->bDragAcceptFileRequired)
                pDialogBase->EnableDragAndDrop(TRUE);

            // load dlg icons
            ::SendMessage(hWnd, WM_SETICON, ICON_BIG,
                (LPARAM)::LoadImage(pDialogBase->hInstance, MAKEINTRESOURCE(pDialogBase->IconResourceId),IMAGE_ICON,0,0,LR_SHARED)
                );
            ::SendMessage(hWnd, WM_SETICON, ICON_SMALL,
                (LPARAM)::LoadImage(pDialogBase->hInstance, MAKEINTRESOURCE(pDialogBase->IconResourceId),IMAGE_ICON,0,0,LR_SHARED)
                );

            pDialogBase->OnInit();
        }
        break;
    case WM_CLOSE:
        {
            CDialogBase* pDialogBase=CDialogBase::GetAssociatedObject(hWnd);
            if (!pDialogBase)
                break;
            // 1) specific case as EDIT control in multi lines configuration send WM_CLOSE directly when edit has focus and VK_ESCAPE is pressed (known windows bug for backward compatibility)
            // 
            //LRESULT CDialogBaseChild::OnMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
            //{
            //    // specific case as EDIT control in multi lines configuration send WM_CLOSE directly when edit has focus and VK_ESCAPE is pressed (known windows bug for backward compatibility)
            //    switch (uMsg)
            //    {
            //    case WM_CLOSE:
            //        return TRUE;
            //    case WM_SYSCOMMAND:
            //        {
            //            if ( wParam == SC_CLOSE) 
            //            {
            //                this->Close();
            //            }
            //        }
            //    }
            //    return FALSE;
            //}

            // 2) take care to of buttons with IDCANCEL because they send WM_CLOSE too
            if (pDialogBase->OnMessage(uMsg,wParam,lParam)) // if WM_CLOSE returns TRUE, we should not treat it
                return TRUE;
            pDialogBase->Close();
        }
        break;

    case WM_COMMAND:
        {
            CDialogBase* pDialogBase=CDialogBase::GetAssociatedObject(hWnd);
            if (!pDialogBase)
                break;
            pDialogBase->OnCommand(wParam,lParam);
        }
        break;

    case WM_NOTIFY:
        {
            CDialogBase* pDialogBase=CDialogBase::GetAssociatedObject(hWnd);
            if (!pDialogBase)
                break;
            pDialogBase->OnNotify(wParam,lParam);
        }
        break;
    case WM_DROPFILES:
        {
            CDialogBase* pDialogBase=CDialogBase::GetAssociatedObject(hWnd);
            if (!pDialogBase)
                break;
            pDialogBase->OnDropFiles(wParam,lParam);
        }
        break;
    case WM_LBUTTONDOWN:
        {
            CDialogBase* pDialogBase=CDialogBase::GetAssociatedObject(hWnd);
            if (!pDialogBase)
                break;
            pDialogBase->OnMouseDown(wParam,lParam);
        }
        break;

    case WM_LBUTTONUP:
    case WM_KILLFOCUS:
        {
            CDialogBase* pDialogBase=CDialogBase::GetAssociatedObject(hWnd);
            if (!pDialogBase)
                break;
            pDialogBase->OnMouseUp(wParam,lParam);
        }
        break;


    case WM_MOUSEMOVE:
        {
            CDialogBase* pDialogBase=CDialogBase::GetAssociatedObject(hWnd);
            if (!pDialogBase)
                break;
            pDialogBase->OnMouseMove(wParam,lParam);
        }
        break;
    default:
        {
            CDialogBase* pDialogBase=CDialogBase::GetAssociatedObject(hWnd);
            if (!pDialogBase)
                break;
            return pDialogBase->OnMessage(uMsg,wParam,lParam);
        }
        // return FALSE;
    }
    return TRUE;
}
