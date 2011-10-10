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
// Object: manages the ComIidNameConvert dialog
//-----------------------------------------------------------------------------

#include "ComIidNameConvert.h"

CComIidNameConvert::CComIidNameConvert()
{
}
CComIidNameConvert::~CComIidNameConvert()
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
DWORD WINAPI CComIidNameConvert::ModelessDialogThread(PVOID lParam)
{
    CComIidNameConvert* pComIidNameConvert=new CComIidNameConvert();
    DialogBoxParam ((HINSTANCE)lParam,(LPCTSTR)IDD_DIALOG_COM_CLSID_NAME_CONVERT,NULL,(DLGPROC)CComIidNameConvert::WndProc,(LPARAM)pComIidNameConvert);
    delete pComIidNameConvert;
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
void CComIidNameConvert::Show(HINSTANCE hInstance,HWND hWndDialog)
{
    // show dialog
    // don't use hWndDialog to allow to put winapioverride to an upper Z-order
    UNREFERENCED_PARAMETER(hWndDialog);
    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CComIidNameConvert::ModelessDialogThread,hInstance,0,NULL));
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: init stats dialog
// Parameters :
//     in  : HWND hWndDialog : stat dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComIidNameConvert::Init(HWND hwnd)
{
    this->hWndDialog=hwnd;
    // set static displayed names
    SetWindowText(this->hWndDialog,_T("COM IID Name Convert"));
    SetDlgItemText(this->hWndDialog,IDC_STATIC_COM_CONVERT_ID,_T("COM Interface Identifier (IID)"));
    SetDlgItemText(this->hWndDialog,IDC_STATIC_COM_CONVERT_NAME,_T("Interface Name"));
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close stats dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CComIidNameConvert::Close()
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
LRESULT CALLBACK CComIidNameConvert::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // init dialog
        SetWindowLongPtr(hWnd,GWLP_USERDATA,lParam);
        CDialogHelper::SetIcon(hWnd,IDI_ICON_CLSIDIID_CONVERT);
        ((CComIidNameConvert*)lParam)->Init(hWnd);
        break;
    case WM_CLOSE:
        {
            // close dialog
            CComIidNameConvert* pComIidNameConvert=(CComIidNameConvert*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (pComIidNameConvert)
                pComIidNameConvert->Close();
            break;
        }
        break;
        case WM_COMMAND:
            {
                CComIidNameConvert* pComIidNameConvert=(CComIidNameConvert*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
                if (!pComIidNameConvert)
                    return FALSE;
                switch(LOWORD(wParam))
                {
                case IDCANCEL:
                    pComIidNameConvert->Close();
                    break;
                case IDC_BUTTON_COM_CLSID_NAME_CONVERT_TO_NAME:
                    pComIidNameConvert->IIDToName();
                    break;
                case IDC_BUTTON_COM_CLSID_NAME_CONVERT_TO_CLSID:
                    pComIidNameConvert->NameToIID();
                    break;
                }
            }
    default:
        return FALSE;
    }
    return TRUE;
}

void CComIidNameConvert::NameToIID()
{
    TCHAR QueriedInterfaceName[MAX_PATH];
    

    // clear IDC_EDIT_COM_CLSID_NAME_CONVERT_CLSID
    SetDlgItemText(this->hWndDialog,IDC_EDIT_COM_CLSID_NAME_CONVERT_CLSID,_T(""));

    // get IDC_EDIT_COM_CLSID_NAME_CONVERT_NAME
    GetDlgItemText(this->hWndDialog,IDC_EDIT_COM_CLSID_NAME_CONVERT_NAME,QueriedInterfaceName,MAX_PATH);
    QueriedInterfaceName[MAX_PATH-1]=0;

    CTrimString::TrimString(QueriedInterfaceName);

    this->IIDFromInterfaceName(QueriedInterfaceName);

}
void CComIidNameConvert::IIDToName()
{
    TCHAR pszIID[MAX_PATH];
    SetDlgItemText(this->hWndDialog,IDC_EDIT_COM_CLSID_NAME_CONVERT_NAME,_T(""));
    GetDlgItemText(this->hWndDialog,IDC_EDIT_COM_CLSID_NAME_CONVERT_CLSID,pszIID,MAX_PATH-2);
    pszIID[MAX_PATH-3]=0;

    CTrimString::TrimString(pszIID);

    // add } at the end of CLSID if necessary
    if (pszIID[_tcslen(pszIID)-1]!='}')
        _tcscat(pszIID,_T("}"));
    // add { at begin of CLSID if necessary
    if (*pszIID!='{')
    {
        memmove(&pszIID[1],pszIID,(_tcslen(pszIID)+1)*sizeof(TCHAR)); // +1 to keep \0
        *pszIID='{';
    }

    // get Interface name from registry
    TCHAR pszIIDName[MAX_PATH];
    if(!CGUIDStringConvert::GetInterfaceName(pszIID,pszIIDName,MAX_PATH))
    {
        MessageBox(this->hWndDialog,
            _T("Can't get interface name"),
            _T("Error"),
            MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // set dlg text
    SetDlgItemText(this->hWndDialog,IDC_EDIT_COM_CLSID_NAME_CONVERT_NAME,pszIIDName);
}

// an extended version of GUIDStringConvert class (get all IID associated to Interface Name)
void CComIidNameConvert::IIDFromInterfaceName(IN TCHAR* pszIIDName)
{

    DWORD cnt;
    TCHAR pszKey[100];
    TCHAR SubKeyName[CGUIDSTRINGCONVERT_STRING_GUID_SIZE];
    DWORD SubKeyNameSize;
    TCHAR IIDName[MAX_PATH];
    LONG IIDNameSize;
    DWORD NbSubKeys;
    HKEY hKeyInterfaces;
    LONG Ret;
    BOOL bInterfaceFound=FALSE;
    TCHAR* FullIID;
    size_t FullIIDSize=1024;
    FullIID=new TCHAR[FullIIDSize];
    *FullIID=0;

    _tcscpy(pszKey,_T("SOFTWARE\\Classes\\Interface\\"));
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,pszKey,0,KEY_READ,&hKeyInterfaces)!=ERROR_SUCCESS)
    {
        // error opening registry or not found
        MessageBox(this->hWndDialog,
                    _T("Error opening registry"),
                    _T("Error"),
                    MB_OK|MB_ICONERROR|MB_TOPMOST);
    }

    // Get the class name and the value count. 
    RegQueryInfoKey(hKeyInterfaces,// key handle 
        NULL,      // buffer for class name 
        NULL,      // size of class string 
        NULL,      // reserved 
        &NbSubKeys,// number of subkeys 
        NULL,      // longest subkey size 
        NULL,      // longest class string 
        NULL,      // number of values for this key 
        NULL,      // longest value name 
        NULL,      // longest value data 
        NULL,      // security descriptor 
        NULL);     // last write time 


    // Enumerate the child keys
    for (cnt = 0, Ret = ERROR_SUCCESS; 
        (Ret == ERROR_SUCCESS) && (cnt<NbSubKeys);
        cnt++) 
    { 
        SubKeyNameSize=CGUIDSTRINGCONVERT_STRING_GUID_SIZE;
        Ret = RegEnumKeyEx(hKeyInterfaces, 
            cnt, 
            SubKeyName, 
            &SubKeyNameSize, 
            NULL, 
            NULL, 
            NULL, 
            NULL); 
        if (Ret != ERROR_SUCCESS) 
            break;

        IIDNameSize=MAX_PATH;
        if (RegQueryValue(hKeyInterfaces,SubKeyName,IIDName,&IIDNameSize)!=ERROR_SUCCESS)
            continue;

        // if Interface name has been found
        if (_tcscmp(pszIIDName,IIDName)==0)
        {
            if (_tcslen(FullIID)+_tcslen(SubKeyName)+_tcslen(CComIidNameConvertINERFACE_GUID_SPLITTER)+1>FullIIDSize)
            {
                TCHAR* psz;
                psz=FullIID;

                FullIIDSize*=2;
                FullIID=new TCHAR[FullIIDSize];
                _tcscpy(FullIID,psz);
                delete[] psz;
            }
            // if not first interface
            if (bInterfaceFound)
            {
                _tcscat(FullIID,CComIidNameConvertINERFACE_GUID_SPLITTER);
            }
            _tcscat(FullIID,SubKeyName);
            bInterfaceFound=TRUE;
        }
    } 
    RegCloseKey(hKeyInterfaces);
    

    if (bInterfaceFound)
    {
        SetDlgItemText(this->hWndDialog,IDC_EDIT_COM_CLSID_NAME_CONVERT_CLSID,FullIID);
    }
    else
    {
        // error opening registry or not found
        MessageBox(this->hWndDialog,
                    _T("Can't find interface name"),
                    _T("Error"),
                    MB_OK|MB_ICONERROR|MB_TOPMOST);
    }
    delete[] FullIID;
}