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
// Object: manages query monitoring ownership
//-----------------------------------------------------------------------------
#include "QueryMonitoringOwnership.h"

extern HINSTANCE DllhInstance;
extern BOOL CanCreateThread();
//-----------------------------------------------------------------------------
// Name: GetAssociatedObject
// Object: Get object associated to window handle
// Parameters :
//     in  : HWND hWndDialog : handle of the window
//     out :
//     return : associated object if found, NULL if not found
//-----------------------------------------------------------------------------
CQueryMonitoringOwnerShip* CQueryMonitoringOwnerShip::GetAssociatedObject(HWND hWndDialog)
{
    return (CQueryMonitoringOwnerShip*)DynamicGetWindowLong(hWndDialog,GWLP_USERDATA);
}

CQueryMonitoringOwnerShip::CQueryMonitoringOwnerShip(TCHAR* pszPrefix,BOOL* pTakeSameAction,BOOL* pTakeOwnerShip, BOOL* pRemoveConditionalBreakAndLogParameters,BOOL* pQuestionAsked)
{
    this->Dlg_Res=-1;

    this->bDialogCreationError=FALSE;
    this->pszPrefix=pszPrefix;
    this->pTakeSameAction=pTakeSameAction;
    this->pTakeOwnerShip=pTakeOwnerShip;
    this->pRemoveConditionalBreakAndLogParameters=pRemoveConditionalBreakAndLogParameters;
    this->pQuestionAsked=pQuestionAsked;
}

CQueryMonitoringOwnerShip::~CQueryMonitoringOwnerShip(void)
{
}


//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CQueryMonitoringOwnerShip::Init(HWND hWndDialog)
{
    this->hWndDialog=hWndDialog;

    // associate object to window handle
    DynamicSetWindowLongPtr(hWndDialog,GWLP_USERDATA,(LONG)this);

    DynamicSendMessage(DynamicGetDlgItem(this->hWndDialog,IDC_STATIC_PREFIX),WM_SETTEXT,0,(LPARAM)this->pszPrefix);
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close like. EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CQueryMonitoringOwnerShip::Close()
{
    *this->pTakeSameAction=(DynamicSendMessage(DynamicGetDlgItem(this->hWndDialog,IDC_CHECK_TAKE_SAME_ACTION),BM_GETCHECK,0,0)==BST_CHECKED);
    *this->pTakeOwnerShip=(DynamicSendMessage(DynamicGetDlgItem(this->hWndDialog,IDC_CHECK_TAKE_OWNERSHIP),BM_GETCHECK,0,0)==BST_CHECKED);
    *this->pRemoveConditionalBreakAndLogParameters=(DynamicSendMessage(DynamicGetDlgItem(this->hWndDialog,IDC_CHECK_REMOVE_COND_PARAM),BM_GETCHECK,0,0)==BST_CHECKED);

    DynamicEndDialog(this->hWndDialog,0);
}


DWORD WINAPI CQueryMonitoringOwnerShip::ModelessDialogThread(PVOID lParam)
{
    HDESK OldDesktop=NULL;
    HDESK CurrentDesktop=NULL;
    HWINSTA OldStation=NULL;
    HWINSTA CurrentStation=NULL;
    // show dialog
    if (CDialogInterfaceManager::SetDefaultStation(&CurrentStation,&OldStation,&CurrentDesktop,&OldDesktop))
    {
        ((CQueryMonitoringOwnerShip*)lParam)->Dlg_Res=DynamicDialogBoxParam(DllhInstance,(LPTSTR)IDD_DIALOG_QUERY_MONITORING_OWNERSHIP,NULL,(DLGPROC)CQueryMonitoringOwnerShip::WndProc,(LPARAM)lParam);
    }
    else
        ((CQueryMonitoringOwnerShip*)lParam)->bDialogCreationError=TRUE;

    CDialogInterfaceManager::RestoreStation(CurrentStation,OldStation,CurrentDesktop,OldDesktop);

    return 0;
}

//-----------------------------------------------------------------------------
// Name: ShowDialog
// Object: show the filter dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : parent window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CQueryMonitoringOwnerShip::ShowDialog()
{
    INT_PTR Ret=-1;

    // check if service a user interaction
    if (!CDialogInterfaceManager::CanWindowInteract())
    {
        this->bDialogCreationError=TRUE;
    }
    else // if user interaction
    {
        if (!CanCreateThread())
        {
            // thread can't be created during Tls callback, just try yo show dialog
            // it's ok because Tls callback hooking mode is used only when winapioverride
            // creates the process, so we have full access rights on created process, and
            // created process uses user window station and desktop
            ModelessDialogThread(this);
        }
        else
        {
            HANDLE hThread=CDialogInterfaceManager::AdjustThreadSecurityAndLaunchDialogThread(ModelessDialogThread,this);
            if (hThread)
            {
                // wait the end of the dialog
                WaitForSingleObject(hThread,INFINITE);
                // close thread handle
                CloseHandle(hThread);
                // get dialog return
                Ret=this->Dlg_Res;
            }
            else
            {
                this->bDialogCreationError=TRUE;
            }
        }
    }

    if (this->bDialogCreationError)
    {
        BOOL bTakeOwnerShip=TRUE;
        BOOL bRemoveConditionalBreakAndLogParameters=TRUE;

        TCHAR pszMsg[3*MAX_PATH];
        _tcscpy(pszMsg,this->pszPrefix);
        _tcscat(pszMsg,_T("\r\nDo you want this new monitoring file take the ownership of the hook ?"));
        if(DynamicMessageBoxInDefaultStation(NULL,pszMsg,_T("API Override Warning"),MB_ICONWARNING|MB_YESNO|MB_TOPMOST)==IDNO)
            bTakeOwnerShip=FALSE;
        else
        {
            if(DynamicMessageBoxInDefaultStation(NULL,_T("Do you want to keep conditional log and break configuration ?\r\n")
                _T("(new conditional log and break values will be added to existing ones)"),
                _T("API Override Warning"),MB_ICONWARNING|MB_YESNO|MB_TOPMOST)==IDYES)
                bRemoveConditionalBreakAndLogParameters=FALSE;
        }

        if (!*this->pQuestionAsked)
        {
            *this->pQuestionAsked=TRUE;
            *this->pTakeSameAction=(DynamicMessageBoxInDefaultStation(NULL,_T("Do you want to take this action for all other multiple definitions"),_T("Question"),MB_ICONQUESTION|MB_YESNO)==IDYES);
            *this->pTakeOwnerShip=bTakeOwnerShip;
            *this->pRemoveConditionalBreakAndLogParameters=bRemoveConditionalBreakAndLogParameters;
        }
    }

    return Ret;
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback of the dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CQueryMonitoringOwnerShip::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CQueryMonitoringOwnerShip* pDialogObject; 
    switch (uMsg)
    {
    case WM_INITDIALOG:
        pDialogObject=(CQueryMonitoringOwnerShip*)lParam;
        pDialogObject->Init(hWnd);
        break;
    case WM_CLOSE:
        pDialogObject=CQueryMonitoringOwnerShip::GetAssociatedObject(hWnd);
        if (pDialogObject)
            pDialogObject->Close();
        break;
    case WM_COMMAND:

        pDialogObject=CQueryMonitoringOwnerShip::GetAssociatedObject(hWnd);
        if (!pDialogObject)
            break;
        switch (LOWORD(wParam))
        {
        case IDOK:
            pDialogObject->Close();
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}
