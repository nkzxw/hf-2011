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

#include "kernelmemoryuserinterface.h"
//-----------------------------------------------------------------------------
// Object: global var
//-----------------------------------------------------------------------------
BOOL bKernelMemoryWarningAlreadyDisplay=FALSE;

//-----------------------------------------------------------------------------
// Name: GetAssociatedObject
// Object: Get object associated to window handle
// Parameters :
//     in  : HWND hWndDialog : handle of the window
//     out :
//     return : associated object if found, NULL if not found
//-----------------------------------------------------------------------------
CKernelMemoryUserInterface* CKernelMemoryUserInterface::GetAssociatedObject(HWND hWndDialog)
{
    return (CKernelMemoryUserInterface*)GetWindowLong(hWndDialog,GWLP_USERDATA);
}

CKernelMemoryUserInterface::CKernelMemoryUserInterface(void)
{
    this->bKernelDriverStartSuccess=FALSE;
    this->pKernelMemoryAccessInterface=new CKernelMemoryAccessInterface();
    if (this->pKernelMemoryAccessInterface->StartDriver())
    {
        if (this->pKernelMemoryAccessInterface->OpenDriver())
        {
            this->bKernelDriverStartSuccess=TRUE;
        }
    }
}

CKernelMemoryUserInterface::~CKernelMemoryUserInterface(void)
{
    this->pKernelMemoryAccessInterface->CloseDriver();
}

//-----------------------------------------------------------------------------
// Name: ModelessDialogThread
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : HINSTANCE hInstance : application instance
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CKernelMemoryUserInterface::ModelessDialogThread(PVOID lParam)
{
    CKernelMemoryUserInterface* pKernelMemoryUserInterface=new CKernelMemoryUserInterface();
    if (pKernelMemoryUserInterface->bKernelDriverStartSuccess)
    {
        DialogBoxParam((HINSTANCE)lParam,(LPTSTR)IDD_DIALOG_MEMORY,NULL,(DLGPROC)CKernelMemoryUserInterface::UserInterfaceWndProc,(LPARAM)pKernelMemoryUserInterface);
    }
    delete pKernelMemoryUserInterface;
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
void CKernelMemoryUserInterface::Show(HINSTANCE hInstance,HWND ParentHandle)
{
    if (!bKernelMemoryWarningAlreadyDisplay)
    {
        if (MessageBox(ParentHandle,
                    _T("Warning : playing with kernel memory is dangerous.\r\n")
                    _T("You can get blue screen or damage your hardware,\r\n")
                    _T("so ONLY DO IT IF YOU REALLY KNOW WHAT YOU ARE DOING.\r\n")
                    _T("\r\nPLEASE SAVE YOUR WORK.\r\n")
                    _T("\r\nTHE AUTHOR CAN'T BE HELD RESPONSIBLE OF LOST OF DATA OR HARDWARE DAMAGES.\r\n")
                    _T("\r\nI understant and agree\r\n"),
                    _T("Warning"),MB_YESNO|MB_ICONWARNING|MB_TOPMOST)==IDNO)
                    return;
        bKernelMemoryWarningAlreadyDisplay=TRUE;
    }
    // show dialog
    // don't use hWndDialog to allow to put winapioverride to an upper Z-order
    UNREFERENCED_PARAMETER(ParentHandle);
    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CKernelMemoryUserInterface::ModelessDialogThread,hInstance,0,NULL));
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CKernelMemoryUserInterface::Init()
{
    HWND hWndControl;

    // hide process combo box
    ShowWindow(GetDlgItem(this->hWndDialog,IDC_COMBO_MEMORY_SELECT_PROCESS),FALSE);

    // change "process" caption to a warning
    hWndControl=GetDlgItem(this->hWndDialog,IDC_STATIC_PROCESS_CAPTION);
    SetWindowText(hWndControl,_T("Warning Kernel Memory Access"));
    // change size of control
    RECT Rect;
    GetWindowRect(hWndControl,&Rect);
    SetWindowPos(hWndControl,HWND_NOTOPMOST,0,0,500,Rect.bottom-Rect.top,SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);

    // create fixed with font
    this->MemoryhFont=CreateFont(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, NULL);

    // set hex text boxes fonts (fixed width font)
    if (this->MemoryhFont)
    {
        hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_READ_RESULT);
        SendMessage((HWND) hWndControl,(UINT) WM_SETFONT,(WPARAM)this->MemoryhFont,FALSE);

        hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_WRITE_ENTRY);
        SendMessage((HWND) hWndControl,(UINT) WM_SETFONT,(WPARAM)this->MemoryhFont,FALSE);
    }
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close like. EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CKernelMemoryUserInterface::Close()
{
    DeleteObject(this->MemoryhFont);
    EndDialog(this->hWndDialog,0);
}


//-----------------------------------------------------------------------------
// Name: MemoryAllocate
// Object: allocate memory according to user specified values
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CKernelMemoryUserInterface::MemoryAllocate()
{
    TCHAR psz[MAX_PATH];
    PVOID StartAddress=0;
    DWORD Size=0;
    HANDLE hWndControl;
    int iScanfRes;
    
    if (!this->bKernelDriverStartSuccess)
        return;

    // retrieve size
    *psz=0;
    hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_ALLOCATE_MEMORY_SIZE);
    SendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);

    if(_tcsnicmp(psz,_T("0x"),2)==0)
        iScanfRes=_stscanf(psz+2,_T("%x"),&Size);
    else
        iScanfRes=_stscanf(psz,_T("%u"),&Size);
    if ((!iScanfRes)||(!Size))
    {
        MessageBox(this->hWndDialog,_T("Bad Size"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // get handle to IDC_EDIT_MEMORY_ALLOCATE_MEMORY_ADDRESS
    hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_ALLOCATE_MEMORY_ADDRESS);
    if (!this->pKernelMemoryAccessInterface->AllocateMemory(Size,&StartAddress))
    {
        // clear text
        *psz=0;
        SendMessage((HWND) hWndControl,(UINT) WM_SETTEXT,0,(LPARAM)psz);
        MessageBox(this->hWndDialog,_T("Error allocating memory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }
    // set text
    _stprintf(psz,_T("0x%p"),StartAddress);
    SendMessage((HWND) hWndControl,(UINT) WM_SETTEXT,0,(LPARAM)psz);

    // show information message
    MessageBox(this->hWndDialog,_T("Memory successfully allocated"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);

}

//-----------------------------------------------------------------------------
// Name: MemoryFree
// Object: free memory according to user specified values
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CKernelMemoryUserInterface::MemoryFree()
{
    TCHAR psz[MAX_PATH];
    DWORD StartAddress=0;;
    HANDLE hWndControl;
    int iScanfRes;

    if (!this->bKernelDriverStartSuccess)
        return;

    // retrieve address
    *psz=0;
    hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_FREE_ALLOCATED_ADDRESS);
    SendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);

    if(_tcsnicmp(psz,_T("0x"),2)==0)
        iScanfRes=_stscanf(psz+2,_T("%x"),&StartAddress);
    else
        iScanfRes=_stscanf(psz,_T("%u"),&StartAddress);
    if ((!iScanfRes)||(!StartAddress))
    {
        MessageBox(this->hWndDialog,_T("Bad Start Address"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // try yo free memory
    if (!this->pKernelMemoryAccessInterface->FreeMemory((PVOID)StartAddress))
    {
        MessageBox(this->hWndDialog,_T("Error freeing memory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // show information message
    MessageBox(this->hWndDialog,_T("Memory successfully free"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
}

//-----------------------------------------------------------------------------
// Name: MemoryRead
// Object: read memory according to user specified values
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CKernelMemoryUserInterface::MemoryRead()
{
    TCHAR psz[MAX_PATH];
    TCHAR* pc;
    DWORD StartAddress=0;;
    DWORD Size=0;
    HANDLE hWndControl;
    
    if (!this->bKernelDriverStartSuccess)
        return;

    // clear previous result
    hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_READ_RESULT);
    *psz=0;
    SendMessage((HWND) hWndControl,(UINT) WM_SETTEXT,0,(LPARAM)psz);
    
    // retrieve start address
    hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_READ_START_ADDRESS);
    SendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);
    int iScanfRes;

    if(_tcsnicmp(psz,_T("0x"),2)==0)
        iScanfRes=_stscanf(psz+2,_T("%x"),&StartAddress);
    else
        iScanfRes=_stscanf(psz,_T("%u"),&StartAddress);
    if ((!iScanfRes)||(!StartAddress))
    {
        MessageBox(this->hWndDialog,_T("Bad Start Address"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // retrieve size
    hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_READ_SIZE);
    SendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);

    if(_tcsnicmp(psz,_T("0x"),2)==0)
        iScanfRes=_stscanf(psz+2,_T("%x"),&Size);
    else
        iScanfRes=_stscanf(psz,_T("%u"),&Size);
    if ((!iScanfRes)||(!Size))
    {
        MessageBox(this->hWndDialog,_T("Bad Size"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    BYTE* Buffer=new BYTE[Size];
    ULONG ReadSize;

    // try to read memory
    if (!this->pKernelMemoryAccessInterface->ReadMemory((LPVOID)StartAddress,Size,Buffer,&ReadSize))
    {
        if (ReadSize==0)
        {
            MessageBox(this->hWndDialog,_T("Error reading memory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            delete[] Buffer;
            return;
        }

        // else
        Size=ReadSize;
        _stprintf(psz,_T("Error reading memory.\r\nOnly first 0x%X bytes are readable"),ReadSize);
        MessageBox(this->hWndDialog,psz,_T("Warning"),MB_OK|MB_ICONWARNING|MB_TOPMOST);
    }

    ////////////////////////
    // put Buffer into an hex representation 
    // and put result in IDC_EDIT_MEMORY_READ_RESULT
    ////////////////////////

    #define NB_BYTES_PER_LINE 16 // 4 DWORD

    DWORD Cnt;
    DWORD Cnt2;
    DWORD NbLines=(Size/NB_BYTES_PER_LINE)+1;
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
    pszAscii[NB_BYTES_PER_LINE]=0;

    *pc=0;
    // for each line
    for (Cnt=0;Cnt<Size;Cnt+=NB_BYTES_PER_LINE)
    {
        // print line address start
        _stprintf(psz,_T("0x%.8X "),Cnt+StartAddress);
        _tcscat(pc,psz);

        // for each byte of the line
        for (Cnt2=0;(Cnt2<NB_BYTES_PER_LINE)&&(Cnt+Cnt2<Size);Cnt2++)
        {
            CurrentByte=Buffer[Cnt+Cnt2];
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
    hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_READ_RESULT);
    SendMessage((HWND) hWndControl,(UINT) WM_SETTEXT,0,(LPARAM)pc);

    delete[] pc;
    delete[] Buffer;
}

//-----------------------------------------------------------------------------
// Name: MemoryWrite
// Object: write memory according to user specified values
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CKernelMemoryUserInterface::MemoryWrite()
{
    TCHAR psz[MAX_PATH];
    TCHAR* pc;
    DWORD StartAddress=0;;
    DWORD Size=0;
    HANDLE hWndControl;
    
    if (!this->bKernelDriverStartSuccess)
        return;

    // retrieve start address
    *psz=0;
    hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_WRITE_START_ADDRESS);
    SendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);
    int iScanfRes;

    if(_tcsnicmp(psz,_T("0x"),2)==0)
        iScanfRes=_stscanf(psz+2,_T("%x"),&StartAddress);
    else
        iScanfRes=_stscanf(psz,_T("%u"),&StartAddress);
    if ((!iScanfRes)||(!StartAddress))
    {
        MessageBox(this->hWndDialog,_T("Bad Start Address"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    
    // get IDC_EDIT_MEMORY_WRITE_ENTRY control handle 
    hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_WRITE_ENTRY);
    // retrieve text size
    Size=(DWORD)SendMessage((HWND) hWndControl,(UINT) WM_GETTEXTLENGTH,0,0);
    pc=new TCHAR[Size+1];// +1 for \0
    // retrieve text
    SendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,Size+1,(LPARAM)pc);


    // remove non alpha num char
    DWORD PosInArray;
    DWORD PosInRemovedArray=0;
    for (PosInArray=0;PosInArray<Size;PosInArray++)
    {
        if (((pc[PosInArray]>='a')&&(pc[PosInArray]<='f'))
            ||((pc[PosInArray]>='A')&&(pc[PosInArray]<='F'))
            ||((pc[PosInArray]>='0')&&(pc[PosInArray]<='9'))
            )
        {
            pc[PosInRemovedArray]=pc[PosInArray];
            PosInRemovedArray++;
        }
    }
    // ends string
    pc[PosInRemovedArray]=0;

    // compute size of data
    Size=(DWORD)_tcslen(pc)/2;
    BYTE* Buffer=new BYTE[Size];
    // translate hex data to Byte
    for (PosInArray=0;PosInArray<Size;PosInArray++)
        Buffer[PosInArray]=CStrToHex::StrHexToByte(&pc[PosInArray*2]);

    // try to write memory
    ULONG WrittenSize;
    if (!this->pKernelMemoryAccessInterface->WriteMemory((LPVOID)StartAddress,Buffer,Size,&WrittenSize))
    {
        if (WrittenSize==0)
        {
            MessageBox(this->hWndDialog,_T("Error writing memory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            delete[] pc;
            delete[] Buffer;
            return;
        }

        // else
        Size=WrittenSize;
        _stprintf(psz,_T("Error writing memory.\r\nOnly first 0x%X bytes are writable and have been modified"),WrittenSize);
        MessageBox(this->hWndDialog,psz,_T("Warning"),MB_OK|MB_ICONWARNING|MB_TOPMOST);
    }
    else
        // show information message
        MessageBox(this->hWndDialog,_T("Memory successfully written"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);

    delete[] pc;
    delete[] Buffer;
}

//-----------------------------------------------------------------------------
// Name: KernelMemoryUserInterfaceWndProc
// Object: dialog callback of the dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CKernelMemoryUserInterface::UserInterfaceWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CKernelMemoryUserInterface* pDialogObject; 
    switch (uMsg)
    {
    case WM_INITDIALOG:

        pDialogObject=(CKernelMemoryUserInterface*)lParam;
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pDialogObject);
        pDialogObject->hWndDialog=hWnd;
        pDialogObject->Init();
        break;

    case WM_CLOSE:
        pDialogObject=CKernelMemoryUserInterface::GetAssociatedObject(hWnd);
        if (pDialogObject)
            pDialogObject->Close();
        break;
    case WM_COMMAND:

        pDialogObject=CKernelMemoryUserInterface::GetAssociatedObject(hWnd);
        if (!pDialogObject)
            break;
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
            pDialogObject->Close();
			break;
        case IDC_BUTTON_MEMORY_ALLOCATE:
            pDialogObject->MemoryAllocate();
            break;
        case IDC_BUTTON_MEMORY_FREE:
            pDialogObject->MemoryFree();
            break;
        case IDC_BUTTON_MEMORY_READ:
            pDialogObject->MemoryRead();
            break;
        case IDC_BUTTON_MEMORY_WRITE:
            pDialogObject->MemoryWrite();
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

