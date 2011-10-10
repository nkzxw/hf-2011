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
// Object: manages the Convert String Bytes dialog
//-----------------------------------------------------------------------------

#include "StringBytesConvert.h"

CConvertStringBytes::CConvertStringBytes(void)
{

}

CConvertStringBytes::~CConvertStringBytes(void)
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
DWORD WINAPI CConvertStringBytes::ModelessDialogThread(PVOID lParam)
{
    CConvertStringBytes* pConvertStringBytes=new CConvertStringBytes();
    DialogBoxParam ((HINSTANCE)lParam,(LPCTSTR)IDD_DIALOG_CONVERT,NULL,(DLGPROC)CConvertStringBytes::WndProc,(LPARAM)pConvertStringBytes);
    delete pConvertStringBytes;
    return 0;
}
//-----------------------------------------------------------------------------
// Name: ShowDialog
// Object: create a modeless compare dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CConvertStringBytes::Show(HINSTANCE hInstance,HWND hWndDialog)
{
    // show dialog
    // don't use hWndDialog to allow to put winapioverride to an upper Z-order
    UNREFERENCED_PARAMETER(hWndDialog);
    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CConvertStringBytes::ModelessDialogThread,hInstance,0,NULL));
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CConvertStringBytes::Init()
{

}

//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CConvertStringBytes::Close()
{
    EndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback of the dump dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CConvertStringBytes::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // init dialog
            CConvertStringBytes* pConvertStringBytes;
            pConvertStringBytes=(CConvertStringBytes*)lParam;
            pConvertStringBytes->hWndDialog=hWnd;
            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pConvertStringBytes);

            pConvertStringBytes->Init();
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_CONVERT);
        }
        break;
    case WM_CLOSE:
        // close dialog
        ((CConvertStringBytes*)GetWindowLongPtr(hWnd,GWLP_USERDATA))->Close();
        break;
    case WM_COMMAND:
        {
            CConvertStringBytes* pConvertStringBytes=((CConvertStringBytes*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pConvertStringBytes)
                break;

            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    pConvertStringBytes->Close();
                    break;
                case IDC_BUTTON_CONVERT_TO_STRING:
                    pConvertStringBytes->ConvertToString();
                    break;
                case IDC_BUTTON_CONVERT_TO_BYTES:
                    pConvertStringBytes->ConvertToBytes();
                    break;
            }
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: ConvertToString
// Object: convert byte buffer (content of IDC_EDIT_CONVERT_BYTES) to string
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CConvertStringBytes::ConvertToString()
{
    HWND hControl;
    DWORD StrBytesSize;
    TCHAR* StrBytes;
    DWORD BufferSize;
    BYTE* Buffer;
    BYTE* TmpBuffer;

    // if no radio button is checked
    if ((IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_CONVERT_ASCII)!=BST_CHECKED)
        &&(IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_CONVERT_UNICODE)!=BST_CHECKED))
    {
        MessageBox(this->hWndDialog,_T("Please select a character encoding"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    hControl=GetDlgItem(this->hWndDialog,IDC_EDIT_CONVERT_BYTES);
    // get length of IDC_EDIT_CONVERT_BYTES content
    StrBytesSize=(DWORD)SendMessage(hControl,WM_GETTEXTLENGTH,0,0)+1;

    if (StrBytesSize==0)
    {
        MessageBox(this->hWndDialog,_T("Empty bytes to convert"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // get IDC_EDIT_CONVERT_BYTES content
    StrBytes=new TCHAR[StrBytesSize];
    SendMessage(hControl,WM_GETTEXT,StrBytesSize,(LPARAM)StrBytes);

    // convert content to byte buffer
    Buffer=CStrToHex::StrHexArrayToByteArray(StrBytes,&BufferSize);
    if (BufferSize==0)
    {
        delete[] StrBytes;
        MessageBox(this->hWndDialog,_T("No bytes to convert"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // assume length of unicode is multiple of 2 bytes
    if ((IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_CONVERT_UNICODE)==BST_CHECKED)&&(BufferSize%2))
    {
        MessageBox(this->hWndDialog,_T("Bad unicode byte buffer length (must be multiple of 2 bytes)"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        delete[] StrBytes;
        delete[] Buffer;
        return;
    }

    // depending selected character encoding set the IDC_EDIT_CONVERT_STRING text
    // notice we use directly ansi or unicode version of SendMessage to avoid a charcter encoding conversion
    hControl=GetDlgItem(this->hWndDialog,IDC_EDIT_CONVERT_STRING);
    if (IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_CONVERT_ASCII)==BST_CHECKED)
    {
        if ((BufferSize<1)||(Buffer[BufferSize-1]!=0))
        {
            MessageBox(this->hWndDialog,_T("String buffer not ended with \\0"),_T("Warning"),MB_OK|MB_ICONWARNING|MB_TOPMOST);
            TmpBuffer=new BYTE[BufferSize+1];
            TmpBuffer[BufferSize]=0;
            memcpy(TmpBuffer,Buffer,BufferSize);
            delete[] Buffer;
            Buffer=TmpBuffer;
        }
        SendMessageA(hControl,WM_SETTEXT,0,(LPARAM)Buffer);
    }
    else // if (IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_CONVERT_UNICODE)==BST_CHECKED)
    {
        
        if ((BufferSize<2)||(Buffer[BufferSize-1]!=0)||(Buffer[BufferSize-2]!=0))
        {
            MessageBox(this->hWndDialog,_T("String buffer not ended with \\0"),_T("Warning"),MB_OK|MB_ICONWARNING|MB_TOPMOST);
            TmpBuffer=new BYTE[BufferSize+2];
            TmpBuffer[BufferSize]=0;
            TmpBuffer[BufferSize+1]=0;
            memcpy(TmpBuffer,Buffer,BufferSize);
            delete[] Buffer;
            Buffer=TmpBuffer;
        }
        SendMessageW(hControl,WM_SETTEXT,0,(LPARAM)Buffer);
    }


    delete[] Buffer;
    delete[] StrBytes;

}

//-----------------------------------------------------------------------------
// Name: ConvertToBytes
// Object: convert string (content of IDC_EDIT_CONVERT_STRING) to byte buffer
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CConvertStringBytes::ConvertToBytes()
{
    DWORD StrSize;
    DWORD cnt;
    DWORD NbBytes;
    TCHAR* pszBytes;
    BYTE* pszStr;
    HWND hControl;


    // if no radio button is checked
    if ((IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_CONVERT_ASCII)!=BST_CHECKED)
        &&(IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_CONVERT_UNICODE)!=BST_CHECKED))
    {
        MessageBox(this->hWndDialog,_T("Please select a character encoding"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    hControl=GetDlgItem(this->hWndDialog,IDC_EDIT_CONVERT_STRING);
    // get length of IDC_EDIT_CONVERT_BYTES content
    StrSize=(DWORD)SendMessage(hControl,WM_GETTEXTLENGTH,0,0)+1;

    // use SendMessageA and SendMessageW to directly get wanted char encoding
    if (IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_CONVERT_ASCII)==BST_CHECKED)
    {
        NbBytes=StrSize*sizeof(char);
        pszStr=new BYTE[NbBytes];
        SendMessageA(hControl,WM_GETTEXT,(WPARAM)StrSize,(LPARAM)pszStr);
    }
    else // if (IsDlgButtonChecked(this->hWndDialog,IDC_RADIO_CONVERT_UNICODE)==BST_CHECKED)
    {
        NbBytes=StrSize*sizeof(wchar_t);
        pszStr=new BYTE[NbBytes];
        SendMessageW(hControl,WM_GETTEXT,(WPARAM)StrSize,(LPARAM)pszStr);
    }

    // only show bytes of pszStr buffer
    pszBytes=new TCHAR[NbBytes*3+1];
    for (cnt=0;cnt<NbBytes;cnt++)
        _stprintf(&pszBytes[3*cnt],_T("%.2X "),pszStr[cnt]);

    SetDlgItemText(this->hWndDialog,IDC_EDIT_CONVERT_BYTES,pszBytes);

    delete[] pszBytes;
    delete[] pszStr;
}