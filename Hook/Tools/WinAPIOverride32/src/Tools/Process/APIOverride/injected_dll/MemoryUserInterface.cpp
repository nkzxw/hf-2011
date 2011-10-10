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

#include "memoryuserinterface.h"

extern HINSTANCE DllhInstance;
extern DWORD dwSystemPageSize;

//-----------------------------------------------------------------------------
// Name: GetAssociatedObject
// Object: Get object associated to window handle
// Parameters :
//     in  : HWND hWndDialog : handle of the window
//     out :
//     return : associated object if found, NULL if not found
//-----------------------------------------------------------------------------
CMemoryUserInterface* CMemoryUserInterface::GetAssociatedObject(HWND hWndDialog)
{
    return (CMemoryUserInterface*)DynamicGetWindowLong(hWndDialog,GWLP_USERDATA);
}

CMemoryUserInterface::CMemoryUserInterface(void)
{
}

CMemoryUserInterface::~CMemoryUserInterface(void)
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
void CMemoryUserInterface::Init(HWND hWndDialog)
{
    HWND hWndControl;
    this->hWndDialog=hWndDialog;

    // associate object to window handle
    DynamicSetWindowLongPtr(hWndDialog,GWLP_USERDATA,(LONG)this);

    // create fixed with font
    this->MemoryhFont=DynamicCreateFont(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, NULL);

    // set hexa textboxes fonts (fixed width font)
    if (this->MemoryhFont)
    {
        hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_READ_RESULT);
        DynamicSendMessage((HWND) hWndControl,(UINT) WM_SETFONT,(WPARAM)this->MemoryhFont,FALSE);

        hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_WRITE_ENTRY);
        DynamicSendMessage((HWND) hWndControl,(UINT) WM_SETFONT,(WPARAM)this->MemoryhFont,FALSE);
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
void CMemoryUserInterface::Close()
{
    DynamicDeleteObject(this->MemoryhFont);
    DynamicEndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: MemoryAllocate
// Object: allocate memory according to user specified values
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMemoryUserInterface::MemoryAllocate()
{
    TCHAR psz[MAX_PATH];
    PBYTE StartAddress=0;;
    PBYTE Size=0;
    HANDLE hWndControl;

    // retrieve size
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_ALLOCATE_MEMORY_SIZE);
    DynamicSendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);

    if (!CStringConverter::StringToPBYTE(psz,&Size))
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Bad Size"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // allocate the memory
    StartAddress=(PBYTE)VirtualAlloc( NULL,(SIZE_T) Size, MEM_COMMIT, PAGE_READWRITE);

    // get handle to IDC_EDIT_MEMORY_ALLOCATE_MEMORY_ADDRESS
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_ALLOCATE_MEMORY_ADDRESS);


    if (!StartAddress)
    {
        // clear text
        *psz=0;
        DynamicSendMessage((HWND) this->hWndDialog,(UINT) WM_SETTEXT,0,(LPARAM)psz);
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Error allocating memory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }
    // set text
    _stprintf(psz,_T("0x%p"),StartAddress);
    DynamicSendMessage((HWND) hWndControl,(UINT) WM_SETTEXT,0,(LPARAM)psz);

    // show information message
    DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Memory successfully allocated"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);

}

//-----------------------------------------------------------------------------
// Name: MemoryFree
// Object: free memory according to user specified values
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMemoryUserInterface::MemoryFree()
{
    TCHAR psz[MAX_PATH];
    PBYTE StartAddress=0;;
    HANDLE hWndControl;

    // retrieve address
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_FREE_ALLOCATED_ADDRESS);
    DynamicSendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);

    if (!CStringConverter::StringToPBYTE(psz,&StartAddress))
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Bad Start Address"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // try yo free memory
    if (!VirtualFree( (LPVOID)StartAddress, 0, MEM_RELEASE))
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Error freeing memory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // show information message
    DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Memory successfully free"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
}

//-----------------------------------------------------------------------------
// Name: MemoryRead
// Object: read memory according to user specified values
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMemoryUserInterface::MemoryRead()
{
    TCHAR psz[MAX_PATH];
    TCHAR* pc;
    PBYTE StartAddress=0;;
    PBYTE Size=0;
    HANDLE hWndControl;
    
    // clear previous result
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_READ_RESULT);
    *psz=0;
    DynamicSendMessage((HWND) hWndControl,(UINT) WM_SETTEXT,0,(LPARAM)psz);
    

    // retrieve start address
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_READ_START_ADDRESS);
    DynamicSendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);

    if (!CStringConverter::StringToPBYTE(psz,&StartAddress))
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Bad Start Address"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // retrieve size
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_READ_SIZE);
    DynamicSendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);

    if (!CStringConverter::StringToPBYTE(psz,&Size))
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Bad Size"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    if (IsBadReadPtr(StartAddress,(UINT_PTR)Size))
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Bad read pointer"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    BYTE* Buffer=new BYTE[(size_t)Size];
    memcpy(Buffer,StartAddress,(size_t)Size);

    ////////////////////////
    // put Buffer into an Hexa representation 
    // and put result in IDC_EDIT_MEMORY_READ_RESULT
    ////////////////////////

    DWORD Cnt;
    DWORD Cnt2;
    UINT_PTR NbLines=(((UINT_PTR)Size)/NB_BYTES_PER_LINE)+1;
    UINT_PTR HexaBufferSize;
    TCHAR pszAscii[NB_BYTES_PER_LINE+1];
    BYTE CurrentByte;

    HexaBufferSize= (
#ifdef _WIN64
                    19
#else
                    11// address "0x%.8x "
#endif
                     +3*NB_BYTES_PER_LINE// single byte "%.2x "
                     +NB_BYTES_PER_LINE// ascii representation
                     +2// \r\n
                     )*NbLines
                    +1; // \0

    pc=new TCHAR[HexaBufferSize];
    pszAscii[NB_BYTES_PER_LINE]=0;

    *pc=0;
    // for each line
    for (Cnt=0;Cnt<(UINT_PTR)Size;Cnt+=NB_BYTES_PER_LINE)
    {
        // print line address start
        _stprintf(psz,_T("0x%p "),Cnt+StartAddress);
        _tcscat(pc,psz);

        // for each byte of the line
        for (Cnt2=0;(Cnt2<NB_BYTES_PER_LINE)&&(Cnt+Cnt2<(UINT_PTR)Size);Cnt2++)
        {
            CurrentByte=Buffer[Cnt+Cnt2];
            // print hexa representation
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
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_READ_RESULT);
    DynamicSendMessage((HWND) hWndControl,(UINT) WM_SETTEXT,0,(LPARAM)pc);

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
void CMemoryUserInterface::MemoryWrite()
{
    TCHAR psz[MAX_PATH];
    PBYTE StartAddress=0;
    DWORD Size=0;
    HANDLE hWndControl;
    TCHAR* pc;
    DWORD dwProtectionFlags=0;
    DWORD dwScratch;

    // retrieve start address
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_WRITE_START_ADDRESS);
    DynamicSendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);

    if (!CStringConverter::StringToPBYTE(psz,&StartAddress))
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Bad Start Address"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }
    
    // get IDC_EDIT_MEMORY_WRITE_ENTRY control handle 
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_WRITE_ENTRY);
    // retrieve text size
    Size=(DWORD)DynamicSendMessage((HWND) hWndControl,(UINT) WM_GETTEXTLENGTH,0,0);
    pc=new TCHAR[Size+1];// +1 for \0
    // retrieve text
    DynamicSendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,Size+1,(LPARAM)pc);


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
    // translate hexa data to Byte
    for (PosInArray=0;PosInArray<Size;PosInArray++)
        Buffer[PosInArray]=CStrToHex::StrHexToByte(&pc[PosInArray*2]);


    BOOL bMemoryProtectionRemoved=FALSE;
    // try to write remote process memory
    if (IsBadWritePtr((LPVOID)StartAddress,Size))
    {
        // remove memory protection in case memory is locked (static string in .data for example) 
        // as StartAddress is not system page rounded, do not use "if (!VirtualProtect(StartAddress, dwSystemPageSize,..."
        if (!VirtualProtect((LPVOID)StartAddress, Size, PAGE_EXECUTE_READWRITE, &dwProtectionFlags))
        {
            DynamicMessageBoxInDefaultStation(NULL,_T("Bad write pointer. Can't remove memory protection"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            delete[] pc;
            delete[] Buffer;
            return;
        }

        if(DynamicMessageBoxInDefaultStation(NULL,_T("Warning you're going to write data in a protected memory page.\r\nDo you want to continue ?"),_T("Warning"),MB_YESNO|MB_ICONWARNING|MB_TOPMOST)==IDNO)
        {
            // restore original memory protection
            VirtualProtect((LPVOID)StartAddress, Size, dwProtectionFlags, &dwScratch);
            delete[] pc;
            delete[] Buffer;
            return;
        }

        bMemoryProtectionRemoved=TRUE;
    }

    // copy data
    memcpy((LPVOID)StartAddress,Buffer,Size);

    if (bMemoryProtectionRemoved)
    {
        // restore original memory protection
        VirtualProtect((LPVOID)StartAddress, Size, dwProtectionFlags, &dwScratch);
    }

    // show information message
    DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Memory successfully written"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);

    delete[] pc;
    delete[] Buffer;
}

//-----------------------------------------------------------------------------
// Name: ShowDialog
// Object: show the dialog box
// Parameters :
//     in  : HWND ParentHandle : parent window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CMemoryUserInterface::ShowDialog(HWND ParentHandle)
{
    // show dialog
    return DynamicDialogBoxParam(DllhInstance,(LPTSTR)IDD_DIALOG_MEMORY,ParentHandle,(DLGPROC)CMemoryUserInterface::WndProc,(LPARAM)this);
}

//-----------------------------------------------------------------------------
// Name: MemoryWndProc
// Object: dialog callback of the dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CMemoryUserInterface::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CMemoryUserInterface* pDialogObject; 
    switch (uMsg)
    {
    case WM_INITDIALOG:
        pDialogObject=(CMemoryUserInterface*)lParam;
        pDialogObject->Init(hWnd);
        break;
    case WM_CLOSE:
        pDialogObject=CMemoryUserInterface::GetAssociatedObject(hWnd);
        if (pDialogObject)
            pDialogObject->Close();
        break;
    case WM_COMMAND:

        pDialogObject=CMemoryUserInterface::GetAssociatedObject(hWnd);
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

