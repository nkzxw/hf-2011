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
// Object: manages the select hooked processes dialog
//-----------------------------------------------------------------------------

#include "selecthookedprocess.h"

extern CLinkListSimple* pApiOverrideList;

CSelectHookedProcess::CSelectHookedProcess(void)
{
    this->pobjApiOverride=NULL;
}

CSelectHookedProcess::~CSelectHookedProcess(void)
{
}

//-----------------------------------------------------------------------------
// Name: ShowDialog
// Object: show the filter dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
CApiOverride* CSelectHookedProcess::ShowDialog(HINSTANCE hInstance,HWND hWndDialog)
{
    CApiOverride* pApiOverride;
    CSelectHookedProcess* pSelectHookedProcess=new CSelectHookedProcess();
   
    // show dialog
    DialogBoxParam(hInstance,(LPCTSTR)IDD_DIALOG_SELECT_HOOKED_PROCESS,hWndDialog,(DLGPROC)CSelectHookedProcess::WndProc,(LPARAM)pSelectHookedProcess);

    pApiOverride=pSelectHookedProcess->pobjApiOverride;

    delete pSelectHookedProcess;

    return pApiOverride;
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSelectHookedProcess::Init()
{
    this->hwndSelectDialogListBoxHookedProcesses=GetDlgItem(this->hWndDialog,IDC_LIST_DUMP_SELECT_PROCESS);

    // retrieve hooked process list
    TCHAR pszProcName[2*MAX_PATH];
    TCHAR psz[2*MAX_PATH];
    CApiOverride* pApiOverride;

    CLinkListItem* pItem;
    // loop through pApiOverrideList
    pApiOverrideList->Lock(TRUE);
    for (pItem=pApiOverrideList->Head;pItem;pItem=pItem->NextItem)
    {
        pApiOverride=(CApiOverride*)pItem->ItemData;
        // get name and pid
        pApiOverride->GetProcessName(pszProcName,MAX_PATH);
        _stprintf(psz,_T("%s (0x%.8X"),pszProcName,pApiOverride->GetProcessID());
        // add to listbox
        CListbox::AddStringWithHScrollUpdate(this->hwndSelectDialogListBoxHookedProcesses,psz);
    }
    pApiOverrideList->Unlock();
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSelectHookedProcess::Close()
{
    EndDialog(this->hWndDialog,FILTERS_DLG_RES_CANCEL);
}


//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback of the dump dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CSelectHookedProcess::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            CSelectHookedProcess* pSelectHookedProcess=(CSelectHookedProcess*)lParam;
            pSelectHookedProcess->hWndDialog=hWnd;

            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pSelectHookedProcess);

            pSelectHookedProcess->Init();
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_DUMP);
        }
        break;
    case WM_CLOSE:
        {
            CSelectHookedProcess* pSelectHookedProcess=((CSelectHookedProcess*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pSelectHookedProcess)
                break;
            pSelectHookedProcess->Close();
        }
        break;
    case WM_COMMAND:
        {
            CSelectHookedProcess* pSelectHookedProcess=((CSelectHookedProcess*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pSelectHookedProcess)
                break;
            switch (LOWORD(wParam))
            {
            case IDOK:
                pSelectHookedProcess->Select();
                break;
            case IDCANCEL:
                pSelectHookedProcess->Close();
                break;
            }
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Select
// Object: get selected process
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSelectHookedProcess::Select()
{
    TCHAR psz[2*MAX_PATH];
    TCHAR* pPID;
    DWORD dwPID;
    CApiOverride* pApiOverride;
    int iSscanfNbRes;
    // find selected process
    LRESULT ItemIndex = SendMessage(this->hwndSelectDialogListBoxHookedProcesses,(UINT) LB_GETCURSEL,0,0);
    if (ItemIndex==LB_ERR)
    {
        MessageBox(this->hWndDialog,_T("No process selected"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    *psz=0;
    SendMessage(this->hwndSelectDialogListBoxHookedProcesses,(UINT)LB_GETTEXT,(WPARAM) ItemIndex,(LPARAM)psz);
    pPID=_tcsrchr(psz,'(');
    dwPID=0;
    // extract pid from selected listbox item text
    iSscanfNbRes=_stscanf(pPID,_T("(0x") _T("%.8X") _T(")"),&dwPID);
    if ((dwPID==0)||(iSscanfNbRes!=1))
        return;

    // find associated CApiOverride object (loop inside pApiOverrideList for matching PID)
    pApiOverride=NULL;
    CLinkListItem* pItem;
    pApiOverrideList->Lock(TRUE);
    for (pItem=pApiOverrideList->Head;pItem;pItem=pItem->NextItem)
    {
        // check process ID
        if (dwPID==((CApiOverride*)pItem->ItemData)->GetProcessID())
        {
            // we found it
            pApiOverride=(CApiOverride*)pItem->ItemData;
            break;
        }
    }
    pApiOverrideList->Unlock();

    if (!pApiOverride)
    {
        // process xxx is no more hooked
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Process 0x") _T("%.8X") _T(" no more hooked"),dwPID);
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);

        // remove item from list
        SendMessage(this->hwndSelectDialogListBoxHookedProcesses,LB_DELETESTRING,ItemIndex,0);

        return;
    }

    this->pobjApiOverride=pApiOverride;
    this->Close();
}