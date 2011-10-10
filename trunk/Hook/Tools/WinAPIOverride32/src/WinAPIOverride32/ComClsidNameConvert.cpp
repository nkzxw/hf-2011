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
// Object: manages the ComClsidNameConvert dialog
//-----------------------------------------------------------------------------

#include "ComClsidNameConvert.h"

CComClsidNameConvert::CComClsidNameConvert()
{
}
CComClsidNameConvert::~CComClsidNameConvert()
{
}

//-----------------------------------------------------------------------------
// Name: ModelessDialogThread
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : HINSTANCE hInstance : application instance
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CComClsidNameConvert::ModelessDialogThread(PVOID lParam)
{
    CComClsidNameConvert* pComClsidNameConvert=new CComClsidNameConvert();
    DialogBoxParam ((HINSTANCE)lParam,(LPCTSTR)IDD_DIALOG_COM_CLSID_NAME_CONVERT,NULL,(DLGPROC)CComClsidNameConvert::WndProc,(LPARAM)pComClsidNameConvert);
    delete pComClsidNameConvert;
    return 0;
}

//-----------------------------------------------------------------------------
// Name: Show
// Object: create a modeless compare dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComClsidNameConvert::Show(HINSTANCE hInstance,HWND hWndDialog)
{
    // show dialog
    // don't use hWndDialog to allow to put winapioverride to an upper Z-order
    UNREFERENCED_PARAMETER(hWndDialog);
    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CComClsidNameConvert::ModelessDialogThread,hInstance,0,NULL));
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: init stats dialog
// Parameters :
//     in  : HWND hWndDialog : stat dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComClsidNameConvert::Init(HWND hwnd)
{
    this->hWndDialog=hwnd;

}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close stats dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComClsidNameConvert::Close()
{
    EndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: stats dialog window proc
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CComClsidNameConvert::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // init dialog
        SetWindowLongPtr(hWnd,GWLP_USERDATA,lParam);
        CDialogHelper::SetIcon(hWnd,IDI_ICON_CLSIDIID_CONVERT);
        ((CComClsidNameConvert*)lParam)->Init(hWnd);
        break;
    case WM_CLOSE:
        {
            // close dialog
            CComClsidNameConvert* pComClsidNameConvert=(CComClsidNameConvert*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (pComClsidNameConvert)
                pComClsidNameConvert->Close();
            break;
        }
        break;
        case WM_COMMAND:
            {
                CComClsidNameConvert* pComClsidNameConvert=(CComClsidNameConvert*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
                if (!pComClsidNameConvert)
                    return FALSE;
                switch(LOWORD(wParam))
                {
                case IDCANCEL:
                    pComClsidNameConvert->Close();
                    break;
                case IDC_BUTTON_COM_CLSID_NAME_CONVERT_TO_NAME:
                    pComClsidNameConvert->CLSIDToName();
                    break;
                case IDC_BUTTON_COM_CLSID_NAME_CONVERT_TO_CLSID:
                    pComClsidNameConvert->NameToCLSID();
                    break;
                }
            }
    default:
        return FALSE;
    }
    return TRUE;
}
void CComClsidNameConvert::NameToCLSID()
{
    TCHAR pszProgId[MAX_PATH];
    TCHAR pszCLSID[CGUIDSTRINGCONVERT_STRING_GUID_SIZE];
    CLSID Clsid;
    // clear IDC_EDIT_COM_CLSID_NAME_CONVERT_CLSID
    SetDlgItemText(this->hWndDialog,IDC_EDIT_COM_CLSID_NAME_CONVERT_CLSID,_T(""));

    // get IDC_EDIT_COM_CLSID_NAME_CONVERT_NAME
    GetDlgItemText(this->hWndDialog,IDC_EDIT_COM_CLSID_NAME_CONVERT_NAME,pszProgId,MAX_PATH);
    pszProgId[MAX_PATH-1]=0;

    CTrimString::TrimString(pszProgId);
    
    if (!CGUIDStringConvert::CLSIDFromProgId(pszProgId,&Clsid))
    {
        MessageBox(this->hWndDialog,
                   _T("Error converting ProgID to CLSID"),
                   _T("Error"),
                   MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }
    
    if (!CGUIDStringConvert::TcharFromCLSID(&Clsid,pszCLSID))
    {
        MessageBox(this->hWndDialog,
                _T("Error converting ProgID to CLSID"),
                _T("Error"),
                MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    SetDlgItemText(this->hWndDialog,IDC_EDIT_COM_CLSID_NAME_CONVERT_CLSID,pszCLSID);
    
}
void CComClsidNameConvert::CLSIDToName()
{
    TCHAR pszCLSID[MAX_PATH];
    TCHAR pszProgId[MAX_PATH];  
    CLSID Clsid;

    // clear IDC_EDIT_COM_CLSID_NAME_CONVERT_CLSID
    SetDlgItemText(this->hWndDialog,IDC_EDIT_COM_CLSID_NAME_CONVERT_NAME,_T(""));
    // get IDC_EDIT_COM_CLSID_NAME_CONVERT_CLSID
    GetDlgItemText(this->hWndDialog,IDC_EDIT_COM_CLSID_NAME_CONVERT_CLSID,pszCLSID,MAX_PATH-2);
    pszCLSID[MAX_PATH-3]=0;

    CTrimString::TrimString(pszCLSID);

    // add } at the end of CLSID if necessary
    if (pszCLSID[_tcslen(pszCLSID)-1]!='}')
        _tcscat(pszCLSID,_T("}"));
    // add { at begin of CLSID if necessary
    if (*pszCLSID!='{')
    {
        memmove(&pszCLSID[1],pszCLSID,(_tcslen(pszCLSID)+1)*sizeof(TCHAR)); // +1 to keep \0
        *pszCLSID='{';
    }

    

    if (!CGUIDStringConvert::CLSIDFromTchar(pszCLSID,&Clsid))
    {
        MessageBox(this->hWndDialog,
                    _T("Bad CLSID value"),
                    _T("Error"),
                    MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }
    if (!CGUIDStringConvert::GetClassProgId(&Clsid,pszProgId,MAX_PATH))
    {
        if (CGUIDStringConvert::GetClassName(&Clsid,pszProgId,MAX_PATH))
        {
            TCHAR psz[2*MAX_PATH];
            _stprintf(psz,_T("No ProgId for class %s"),pszProgId);
            MessageBox(this->hWndDialog,
                        psz,
                        _T("Error"),
                        MB_OK|MB_ICONERROR|MB_TOPMOST);
        }

        else
            MessageBox(this->hWndDialog,
                        _T("Error converting CLSID to ProgID"),
                        _T("Error"),
                        MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    SetDlgItemText(this->hWndDialog,IDC_EDIT_COM_CLSID_NAME_CONVERT_NAME,pszProgId);

}