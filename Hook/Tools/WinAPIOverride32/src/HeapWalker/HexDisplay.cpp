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
// Object: manages the memory dialog
//-----------------------------------------------------------------------------

#include "HexDisplay.h"

//-----------------------------------------------------------------------------
// Name: GetAssociatedObject
// Object: Get object associated to window handle
// Parameters :
//     in  : HWND hWndDialog : handle of the window
//     out :
//     return : associated object if found, NULL if not found
//-----------------------------------------------------------------------------
CHexDisplay* CHexDisplay::GetAssociatedObject(HWND hWndDialog)
{
    return (CHexDisplay*)GetWindowLongPtr(hWndDialog,GWLP_USERDATA);
}

CHexDisplay::CHexDisplay(PBYTE pByte,DWORD Size)
{
    this->Size=Size;
    this->pByte=new BYTE[this->Size];
    if (this->pByte)
        memcpy(this->pByte,pByte,this->Size);
}

CHexDisplay::~CHexDisplay(void)
{
    if (this->pByte)
    {
        delete this->pByte;
        this->pByte=NULL;
    }
}

//-----------------------------------------------------------------------------
// Name: ModelessDialogThread
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : CHexDisplay* current object
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CHexDisplay::ModelessDialogThread(PVOID lParam)
{
    CHexDisplay* pHexDisplay=(CHexDisplay*)lParam;
    // don't use pHexDisplay->hParentHandle to allow to put main window to an upper Z-order
    DialogBoxParam(pHexDisplay->hInstance,(LPTSTR)IDD_DIALOG_HEX_DISPLAY,NULL,(DLGPROC)CHexDisplay::UserInterfaceWndProc,(LPARAM)pHexDisplay);
    delete pHexDisplay;
    return 0;
}


//-----------------------------------------------------------------------------
// Name: ShowDialog
// Object: show the dialog box
// Parameters :
//     in  : HWND ParentHandle : parent window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CHexDisplay::Show(HINSTANCE hInstance,HWND ParentHandle,PBYTE pByte,DWORD Size)
{
    if (IsBadReadPtr(pByte,Size))
    {
        MessageBox(ParentHandle,_T("Empty data"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
        return;
    }
    // show dialog
    CHexDisplay* pHexDisplay=new CHexDisplay(pByte,Size);
    pHexDisplay->hInstance=hInstance;
    pHexDisplay->hParentHandle=ParentHandle;

    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CHexDisplay::ModelessDialogThread,pHexDisplay,0,NULL));
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CHexDisplay::Init()
{
    HWND hwndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_HEX_DATA);

    // create fixed with font
    this->MemoryhFont=CreateFont(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, NULL);

    // set hex text box font (fixed width font)
    if (this->MemoryhFont)
        SendMessage(hwndControl,(UINT) WM_SETFONT,(WPARAM)this->MemoryhFont,FALSE);

    this->HexDisplay();
    
    // MSDN Q96674 : avoid text selection
    SetFocus(hwndControl);
    PostMessage(hwndControl,EM_SETSEL,0,0);

}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close like. EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CHexDisplay::Close()
{
    DeleteObject(this->MemoryhFont);
    EndDialog(this->hWndDialog,0);
}



//-----------------------------------------------------------------------------
// Name: HexDisplay
// Object: show data to hex and ascii format
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CHexDisplay::HexDisplay()
{
    if (!this->pByte)
    {
        MessageBox(this->hWndDialog,_T("Error allocating memory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    TCHAR psz[MAX_PATH];
    TCHAR* pc;
    DWORD StartAddress=0;;
    
    ////////////////////////
    // put Buffer into an Hexa representation 
    // and put result in IDC_EDIT_MEMORY_READ_RESULT
    ////////////////////////


    #define NB_BYTES_PER_LINE 16 // 4 DWORD

    DWORD Cnt;
    DWORD Cnt2;
    DWORD NbLines=(this->Size/NB_BYTES_PER_LINE)+1;
    DWORD HexaBufferSize;
    TCHAR pszAscii[NB_BYTES_PER_LINE+1];
    BYTE CurrentByte;

    HexaBufferSize= (11// address "0x%.8x "
                     +3*NB_BYTES_PER_LINE// single byte "%.2x "
                     +NB_BYTES_PER_LINE// ascii representation
                     +2// \r\n
                     )*NbLines
                    +1; // \0

    pc=new TCHAR[HexaBufferSize];
    if (!pc)
    {
        MessageBox(this->hWndDialog,_T("Error allocating memory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    pszAscii[NB_BYTES_PER_LINE]=0;

    *pc=0;
    // for each line
    for (Cnt=0;Cnt<this->Size;Cnt+=NB_BYTES_PER_LINE)
    {
        // print line address start
        _stprintf(psz,_T("0x%.8X "),Cnt+StartAddress);
        _tcscat(pc,psz);

        // for each byte of the line
        for (Cnt2=0;(Cnt2<NB_BYTES_PER_LINE)&&(Cnt+Cnt2<Size);Cnt2++)
        {
            CurrentByte=this->pByte[Cnt+Cnt2];
            // print hex representation
            _stprintf(psz,_T("%.2X "),CurrentByte);
            _tcscat(pc,psz);

            if ((CurrentByte>=0x20)&&(CurrentByte<=0x7E))
                pszAscii[Cnt2]=CurrentByte;
            else
                pszAscii[Cnt2]='.';
        }

        // for last line check if we have to add spaces before ascii representation
        if (Cnt2<NB_BYTES_PER_LINE)
        {
            for (;Cnt2<NB_BYTES_PER_LINE;Cnt2++)
            {
                _tcscat(pc,_T("   "));
                pszAscii[Cnt2]=' ';
            }
        }
        // add ascii representation
        _tcscat(pc,pszAscii);

        // add \r\n
        _tcscat(pc,_T("\r\n"));
    }

    // set text 
    SetDlgItemText(this->hWndDialog,IDC_EDIT_HEX_DATA,pc);
    delete[] pc;
}

//-----------------------------------------------------------------------------
// Name: UserInterfaceWndProc
// Object: dialog callback of the dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CHexDisplay::UserInterfaceWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    CHexDisplay* pDialogObject; 
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // load dlg icons
        CDialogHelper::SetIcon(hWnd,IDI_ICON_MEMORY);

        pDialogObject=(CHexDisplay*)lParam;
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pDialogObject);
        pDialogObject->hWndDialog=hWnd;
        pDialogObject->Init();
        break;

    case WM_CLOSE:
        pDialogObject=CHexDisplay::GetAssociatedObject(hWnd);
        if (pDialogObject)
            pDialogObject->Close();
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

