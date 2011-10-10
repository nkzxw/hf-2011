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
    return (CMemoryUserInterface*)GetWindowLong(hWndDialog,GWLP_USERDATA);
}

CMemoryUserInterface::CMemoryUserInterface(void)
{
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);
    this->dwSystemPageSize=siSysInfo.dwPageSize;
}

CMemoryUserInterface::~CMemoryUserInterface(void)
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
DWORD WINAPI CMemoryUserInterface::ModelessDialogThread(PVOID lParam)
{
    CMemoryUserInterface* pMemoryUserInterface=new CMemoryUserInterface();
    DialogBoxParam((HINSTANCE)lParam,(LPTSTR)IDD_DIALOG_MEMORY,NULL,(DLGPROC)CMemoryUserInterface::UserInterfaceWndProc,(LPARAM)pMemoryUserInterface);
    delete pMemoryUserInterface;
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
void CMemoryUserInterface::Show(HINSTANCE hInstance,HWND ParentHandle)
{
    // show dialog
    // don't use hWndDialog to allow to put winapioverride to an upper Z-order
    UNREFERENCED_PARAMETER(ParentHandle);
    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CMemoryUserInterface::ModelessDialogThread,hInstance,0,NULL));
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMemoryUserInterface::Init()
{
    HANDLE hWndControl;

    this->RefreshProcessList();

    // create fixed with font
    this->MemoryhFont=CreateFont(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, NULL);

    // set hexa textboxes fonts (fixed width font)
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
void CMemoryUserInterface::Close()
{
    DeleteObject(this->MemoryhFont);
    EndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: RefreshProcessList
// Object: refresh processes list
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CMemoryUserInterface::RefreshProcessList()
{
    TCHAR psz[MAX_PATH];
    // get combo item handle
    HANDLE hWndControl=GetDlgItem(this->hWndDialog,IDC_COMBO_MEMORY_SELECT_PROCESS);

    SendMessage((HWND) hWndControl,(UINT) CB_RESETCONTENT,0,0);

    // create a snapshot and list processes
    PROCESSENTRY32 pe32 = {0};
    HANDLE hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (hSnap == INVALID_HANDLE_VALUE) 
    {
        CAPIError::ShowLastError();
        return; 
    }
    // Fill the size of the structure before using it. 
    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
    // Walk the process list of the system
    if (!Process32First(hSnap, &pe32))
    {
        CAPIError::ShowLastError();
        CloseHandle(hSnap);
        return;
    }
    do 
    {
        // don't show system processes
        if ((pe32.th32ProcessID==0)
            ||(pe32.th32ProcessID==4))
            continue;

        // add ProcessName(ProcessId) to combo
        _stprintf(psz,_T("%s (%u)"),pe32.szExeFile,pe32.th32ProcessID);
        SendMessage((HWND) hWndControl,(UINT) CB_ADDSTRING,0,(LPARAM)psz);
    } 
    while (Process32Next(hSnap, &pe32)); 
 
    // clean up the snapshot object. 
    CloseHandle (hSnap); 

    // select first process in combo
    SendMessage((HWND)hWndControl,(UINT) CB_SETCURSEL,0,0);
}
//-----------------------------------------------------------------------------
// Name: CheckIfProcessIsAlive
// Object: check if process is alive
// Parameters :
//     in  : DWORD dwProcessId : Id of process to check
//     out :
//     return : TRUE if process is alive
//-----------------------------------------------------------------------------
BOOL CMemoryUserInterface::CheckIfProcessIsAlive(DWORD dwProcessId)
{
    
    if (!CProcessHelper::IsAlive(dwProcessId))
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Process 0x%X doesn't exist anymore"),dwProcessId);
        MessageBox(this->hWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        this->RefreshProcessList();

        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetProcessId
// Object: get selected process ID
// Parameters :
//     in  : 
//     out :
//     return : selected Process ID
//-----------------------------------------------------------------------------
DWORD CMemoryUserInterface::GetProcessId()
{
    HANDLE hWndControl;
    TCHAR psz[MAX_PATH];
    TCHAR* pc;
    *psz=0;
    hWndControl=GetDlgItem(this->hWndDialog,IDC_COMBO_MEMORY_SELECT_PROCESS);
    if(SendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz)<=0)
        return 0;
    pc=_tcsrchr(psz,'(');
    if (!pc)
        return 0;
    pc++;

    return (DWORD)_ttol(pc);
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
    DWORD ProcessId=0;
    DWORD StartAddress=0;
    DWORD Size=0;
    HANDLE hWndControl;
    int iScanfRes;
    

    // retrieve process ID
    ProcessId=this->GetProcessId();
    if (!ProcessId)
        return;

    if(!this->CheckIfProcessIsAlive(ProcessId))
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

    // create a process memory object
    CProcessMemory ProcessMemory(ProcessId,FALSE);
    // allocate the memory
    StartAddress=(DWORD)ProcessMemory.Alloc(Size);

    // get handle to IDC_EDIT_MEMORY_ALLOCATE_MEMORY_ADDRESS
    hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_ALLOCATE_MEMORY_ADDRESS);


    if (!StartAddress)
    {
        // clear text
        *psz=0;
        SendMessage((HWND) hWndControl,(UINT) WM_SETTEXT,0,(LPARAM)psz);
        return;
    }
    // set text
    _stprintf(psz,_T("0x%.8x"),StartAddress);
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
void CMemoryUserInterface::MemoryFree()
{
    TCHAR psz[MAX_PATH];
    DWORD ProcessId=0;
    DWORD StartAddress=0;;
    HANDLE hWndControl;
    int iScanfRes;
    

    // retrieve process ID
    ProcessId=this->GetProcessId();
    if (!ProcessId)
        return;

    if(!this->CheckIfProcessIsAlive(ProcessId))
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

    // create CProcessMemory object
    CProcessMemory ProcessMemory(ProcessId,FALSE);

    // try yo free memory
    if (!ProcessMemory.Free((LPVOID)StartAddress))
        return;

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
void CMemoryUserInterface::MemoryRead()
{
    TCHAR psz[MAX_PATH];
    TCHAR* pc;
    DWORD ProcessId=0;
    DWORD StartAddress=0;;
    DWORD Size=0;
    HANDLE hWndControl;
    
    // clear previous result
    hWndControl=GetDlgItem(this->hWndDialog,IDC_EDIT_MEMORY_READ_RESULT);
    *psz=0;
    SendMessage((HWND) hWndControl,(UINT) WM_SETTEXT,0,(LPARAM)psz);
    
    // retrieve process ID
    ProcessId=this->GetProcessId();
    if (!ProcessId)
        return;

    if(!this->CheckIfProcessIsAlive(ProcessId))
        return;

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

    SIZE_T ReadSize=0;
    BYTE* Buffer=new BYTE[Size];
    // create CProcessMemory object
    CProcessMemory ProcessMemory(ProcessId,TRUE);
    // try to read memory
    if (!ProcessMemory.Read((LPCVOID)StartAddress,Buffer,Size,&ReadSize))
    {
        delete[] Buffer;
        return;
    }

    ////////////////////////
    // put Buffer into an Hexa representation 
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
void CMemoryUserInterface::MemoryWrite()
{
    TCHAR psz[MAX_PATH];
    TCHAR* pc;
    DWORD ProcessId=0;
    DWORD StartAddress=0;;
    DWORD Size=0;
    HANDLE hWndControl;
    
    // retrieve process ID
    ProcessId=this->GetProcessId();
    if (!ProcessId)
        return;

    if(!this->CheckIfProcessIsAlive(ProcessId))
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

    
    // get IDC_EDIT_MEMORY_WRITE_ENTRY controle hande 
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
    // translate hexa data to Byte
    for (PosInArray=0;PosInArray<Size;PosInArray++)
        Buffer[PosInArray]=CStrToHex::StrHexToByte(&pc[PosInArray*2]);

    SIZE_T ReadSize=0;
    // create CProcessMemory object
    CProcessMemory ProcessMemory(ProcessId,FALSE);
    // try to write remote process memory
    if (!ProcessMemory.Write((LPVOID)StartAddress,Buffer,Size,&ReadSize))
    {
        delete[] pc;
        delete[] Buffer;
        return;
    }

    // show information message
    MessageBox(this->hWndDialog,_T("Memory successfully written"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);

    delete[] pc;
    delete[] Buffer;
}

//-----------------------------------------------------------------------------
// Name: MemoryUserInterfaceWndProc
// Object: dialog callback of the dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CMemoryUserInterface::UserInterfaceWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CMemoryUserInterface* pDialogObject; 
    switch (uMsg)
    {
    case WM_INITDIALOG:

        pDialogObject=(CMemoryUserInterface*)lParam;
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pDialogObject);
        pDialogObject->hWndDialog=hWnd;
        pDialogObject->Init();
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

