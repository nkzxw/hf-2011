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
// Object: manages the raw dump dialog
//         
//-----------------------------------------------------------------------------

#include "RawDump.h"

extern BOOL UserMode;
LRESULT CALLBACK RawDumpWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void RawDumpInit();
void RawDumpDump();
HWND RawDumphDlg;
HINSTANCE RawDumphInstance;
BOOL RawCheckIfProcessIsAlive(DWORD dwProcessId);
void RawRefreshProcesses();

//-----------------------------------------------------------------------------
// Name: RawCheckIfProcessIsAlive
// Object: check if process is alive
// Parameters :
//     in  : DWORD dwProcessId : Id of process to check
//     out :
//     return : TRUE if process is alive
//-----------------------------------------------------------------------------
BOOL RawCheckIfProcessIsAlive(DWORD dwProcessId)
{
    if (!CProcessHelper::IsAlive(dwProcessId))
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Process 0x%X doesn't exist anymore"),dwProcessId);
        MessageBox(RawDumphDlg,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        RawRefreshProcesses();

        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RawRefreshProcesses
// Object: refresh process list
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void RawRefreshProcesses()
{
    TCHAR psz[MAX_PATH];
    // get combo item handle
    HANDLE hWndControl=GetDlgItem(RawDumphDlg,IDC_COMBO_RAW_DUMP_SELECT_PROCESS);

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
// Name: RawDump
// Object: Show the raw dump dialog
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndParent : handle of parent window
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void RawDump(HINSTANCE hInstance,HWND hWndParent)
{
    RawDumphInstance=hInstance;
    DialogBox(hInstance, (LPCTSTR)IDD_DIALOG_RAW_DUMP, hWndParent, (DLGPROC)RawDumpWndProc);
}

//-----------------------------------------------------------------------------
// Name: RawDumpWndProc
// Object: raw dump dialog window proc
// Parameters :
//     in  : HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam
//     out : 
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK RawDumpWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		case WM_INITDIALOG:
            RawDumphDlg=hDlg;
            RawDumpInit();
			break;
        case WM_CLOSE:
            EndDialog(RawDumphDlg,0);
			break;
		case WM_COMMAND:
            switch(LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(RawDumphDlg,0);
				break;
            case IDOK:
                RawDumpDump();
                break;
            }
            break;
        default:
            return FALSE;
	}
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RawDumpInit
// Object: Initialize raw dump dialog
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void RawDumpInit()
{
    if (UserMode)
        RawRefreshProcesses();
    else
    {
        HWND hWndControl;

        // hide process combo box
        ShowWindow(GetDlgItem(RawDumphDlg,IDC_COMBO_RAW_DUMP_SELECT_PROCESS),FALSE);

        // change "process" caption to a warning
        hWndControl=GetDlgItem(RawDumphDlg,IDC_STATIC_PROCESS_RAW_DUMP);
        SetWindowText(hWndControl,_T("Kernel Memory Dump"));
        // change size of control
        RECT Rect;
        GetWindowRect(hWndControl,&Rect);
        SetWindowPos(hWndControl,HWND_NOTOPMOST,0,0,500,Rect.bottom-Rect.top,SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    }
}

//-----------------------------------------------------------------------------
// Name: RawDumpDump
// Object: make a raw dump for specified process, address and length
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void RawDumpDump()
{
    TCHAR psz[MAX_PATH];
    TCHAR* pc;
    DWORD ProcessId=0;
    PVOID StartAddress=0;;
    DWORD Size=0;
    HANDLE hWndControl;
    
    if (UserMode)
    {
        // retrieve process ID
        *psz=0;
        hWndControl=GetDlgItem(RawDumphDlg,IDC_COMBO_RAW_DUMP_SELECT_PROCESS);
        SendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);
        pc=_tcsrchr(psz,'(');
        if (!pc)
            return;
        pc++;
        ProcessId=(DWORD)_ttol(pc);

        if(!RawCheckIfProcessIsAlive(ProcessId))
            return;
    }

    // retrieve start address
    *psz=0;
    hWndControl=GetDlgItem(RawDumphDlg,IDC_EDIT_RAW_DUMP_START_ADDRESS);
    SendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);
    int iScanfRes;

    if(_tcsnicmp(psz,_T("0x"),2)==0)
        iScanfRes=_stscanf(psz,_T("0x%x"),&StartAddress);
    else
        iScanfRes=_stscanf(psz,_T("%u"),&StartAddress);
    if ((!iScanfRes)||(!StartAddress))
    {
        MessageBox(RawDumphDlg,_T("Bad Start Address"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // retrieve size
    hWndControl=GetDlgItem(RawDumphDlg,IDC_EDIT_RAW_DUMP_SIZE);
    SendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);

    if(_tcsnicmp(psz,_T("0x"),2)==0)
        iScanfRes=_stscanf(psz,_T("0x%x"),&Size);
    else
        iScanfRes=_stscanf(psz,_T("%u"),&Size);
    if ((!iScanfRes)||(!Size))
    {
        MessageBox(RawDumphDlg,_T("Bad Size"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }


    // make dump
    BYTE* Buffer=new BYTE[Size];
    SIZE_T ReadSize=0;

    if (UserMode)
    {
        CProcessMemory ProcessMemory(ProcessId,TRUE);
        if (!ProcessMemory.Read((LPCVOID)StartAddress,Buffer,Size,&ReadSize))
        {
            if (ReadSize==0)
            {
                MessageBox(RawDumphDlg,_T("Error reading memory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                delete[] Buffer;
                return;
            }

            // else
            Size=ReadSize;
            _stprintf(psz,_T("Error reading memory.\r\nOnly first 0x%X bytes are readable"),ReadSize);
            MessageBox(RawDumphDlg,psz,_T("Warning"),MB_OK|MB_ICONWARNING|MB_TOPMOST);
        }
    }
    else
    {
        CKernelMemoryAccessInterface KMem;
        KMem.StartDriver();
        KMem.OpenDriver();
        if (KMem.ReadMemory(StartAddress,Size,Buffer,&ReadSize))
        {
            if (ReadSize==0)
            {
                MessageBox(RawDumphDlg,_T("Error reading memory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                delete[] Buffer;
                return;
            }

            // else
            Size=ReadSize;
            _stprintf(psz,_T("Error reading memory.\r\nOnly first 0x%X bytes are readable"),ReadSize);
            MessageBox(RawDumphDlg,psz,_T("Warning"),MB_OK|MB_ICONWARNING|MB_TOPMOST);
        }
        KMem.CloseDriver();
    }
    /////////////////
    // save dump
    ////////////////

    OPENFILENAME ofn;
    TCHAR pszFileName[MAX_PATH];

    
    // save file dialog
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=RawDumphDlg;
    ofn.hInstance=RawDumphInstance;
    ofn.lpstrFilter=_T("dmp\0*.dmp\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt=_T("dmp");
    *pszFileName=0;
    ofn.lpstrFile=pszFileName;
    ofn.nMaxFile=MAX_PATH;
    
    if (!GetSaveFileName(&ofn))
    {
        delete[] Buffer;
        return;
    }

    // write dump
    HANDLE hFile = CreateFile(
        ofn.lpstrFile,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, 
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        CAPIError::ShowLastError();
        delete[] Buffer;
        return;
    }

    if (!WriteFile(hFile,
                    Buffer,
                    Size,
                    &Size,
                    NULL
                    ))
    {
        CAPIError::ShowLastError();
        CloseHandle(hFile);
        delete[] Buffer;
        return;
    }

    CloseHandle(hFile);
    delete[] Buffer;

    // show message information
    MessageBox(NULL,_T("Dump successfully completed"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);

    return;
}