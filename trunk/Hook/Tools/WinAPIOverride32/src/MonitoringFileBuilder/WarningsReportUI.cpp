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
// Object: manages the Warnings and Errors UI
//-----------------------------------------------------------------------------

#include "WarningsReportUI.h"


//-----------------------------------------------------------------------------
// Name: CMonitoringFileBuilderUI
// Object: constructor
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
CWarningsReportUI::CWarningsReportUI(DWORD NbErrors,DWORD NbWarnings,TCHAR* pMessages)
{
    this->NbErrors=NbErrors;
    this->NbWarnings=NbWarnings;
    this->pMessages=pMessages;
}
CWarningsReportUI::~CWarningsReportUI()
{

}

//-----------------------------------------------------------------------------
// Name: Show
// Object: show monitoring file builder dialog
// Parameters :
//     in  : HINSTANCE Instance : instance of module containing resources
//           HWND hWndParent : parent window handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CWarningsReportUI::Show(HINSTANCE Instance,HWND hWndParent)
{
    this->hInstance=Instance;
    // show dialog
    DialogBoxParam(Instance, (LPCTSTR)IDD_DIALOG_WARNING_AND_ERRORS,hWndParent, (DLGPROC)WndProc,(LPARAM)this);
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: Init monitoring file builder UI
// Parameters :
//     in  : HWND hWnd : window handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CWarningsReportUI::Init(HWND hwnd)
{
    this->hWndDialog=hwnd;

    TCHAR* psz= new TCHAR[_tcslen(this->pMessages)+MAX_PATH];

    // display report with number of error and warnings
    TCHAR pszError[MAX_PATH];
    TCHAR pszWarning[MAX_PATH];

    _tcscpy(pszError,_T("error"));
    if (this->NbErrors)
        _tcscat(pszError,_T("s"));

    _tcscpy(pszWarning,_T("warning"));
    if (this->NbWarnings)
        _tcscat(pszWarning,_T("s"));

    _stprintf(psz,_T("%s\r\n%u %s, %u %s\r\n"),this->pMessages,this->NbErrors,pszError,this->NbWarnings,pszWarning);

    SetDlgItemText(this->hWndDialog,IDC_EDIT_WARNINGS_AND_ERRORS,psz);

    delete[] psz;
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: close monitoring file builder dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CWarningsReportUI::Close()
{
    // close dialog
    EndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: warning and error dialog window proc
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CWarningsReportUI::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetWindowLong(hWnd,GWLP_USERDATA,(LONG)lParam);
        // set Dialog icon
        CDialogHelper::SetIcon(hWnd,IDI_ICON_MONITORINGFILEBUILDER);
        // call the init method
        ((CWarningsReportUI*)lParam)->Init(hWnd);
        break;
    case WM_CLOSE:
        {
            // get CMonitoringFileBuilderUI object
            CWarningsReportUI* pWarningsReportUI=(CWarningsReportUI*)GetWindowLong(hWnd,GWLP_USERDATA);
            if (pWarningsReportUI)
                // call the close method
                pWarningsReportUI->Close();
            break;
        }
        break;
    case WM_COMMAND:
        {
            // get CMonitoringFileBuilderUI object
            CWarningsReportUI* pWarningsReportUI=(CWarningsReportUI*)GetWindowLong(hWnd,GWLP_USERDATA);
            if (pWarningsReportUI)
            {
                switch (LOWORD(wParam))
                {
                case IDOK:
                    // call the close method
                    pWarningsReportUI->Close();
                    break;
                }
            }
            break;
        }
    default:
        return FALSE;
    }
    return TRUE;
}