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
// Object: manages the break dialog
//-----------------------------------------------------------------------------

#include "DumpUserInterface.h"

extern HINSTANCE DllhInstance;
extern CLinkList* pLinkListAPIInfos;
extern DWORD dwCurrentProcessID;

//-----------------------------------------------------------------------------
// Name: GetAssociatedObject
// Object: Get object associated to window handle
// Parameters :
//     in  : HWND hWndDialog : handle of the window
//     out :
//     return : associated object if found, NULL if not found
//-----------------------------------------------------------------------------
CDumpUserInterface* CDumpUserInterface::GetAssociatedObject(HWND hWndDialog)
{
    return (CDumpUserInterface*)DynamicGetWindowLong(hWndDialog,GWLP_USERDATA);
}

CDumpUserInterface::CDumpUserInterface()
{
}

CDumpUserInterface::~CDumpUserInterface(void)
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
void CDumpUserInterface::Init(HWND hWndDialog)
{
    TCHAR psz[MAX_PATH];
    TCHAR* pc;
    MODULEENTRY32 me32 = {0}; 
    HANDLE hSnap;

    this->hWndDialog=hWndDialog;

    // associate object to window handle
    DynamicSetWindowLongPtr(this->hWndDialog,GWLP_USERDATA,(LONG)this);

    // get combo item handle
    HANDLE hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_COMBO_DUMP_MODULE);

    // get modules informations
    hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,dwCurrentProcessID);
    if (hSnap == INVALID_HANDLE_VALUE) 
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Error taking snapshot of the process"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return; 
    }
    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32); 
 
    // Walk the module list of the process
    if (!Module32First(hSnap, &me32))
    {
        CloseHandle(hSnap);
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Error parsing modules of the process"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }
    do 
    { 
        // remove path
        pc=_tcsrchr(me32.szExePath,'\\');
        if (pc)
            pc++;
        else
            pc=me32.szExePath;
        _stprintf(psz,_T("%s <0x%p-0x%p>"),pc,me32.modBaseAddr,(UINT_PTR)me32.modBaseAddr+me32.modBaseSize);
        DynamicSendMessage((HWND) hWndControl,(UINT) CB_ADDSTRING,0,(LPARAM)psz);
    } 
    while (Module32Next(hSnap, &me32)); 
 
    // clean up the snapshot object. 
    CloseHandle (hSnap); 

    // select first process in combo
    DynamicSendMessage((HWND)hWndControl,(UINT) CB_SETCURSEL,0,0);
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close like. EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDumpUserInterface::Close()
{
    DynamicEndDialog(this->hWndDialog,0);
}

//-----------------------------------------------------------------------------
// Name: ShowFilterDialog
// Object: show the filter dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : parent window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CDumpUserInterface::ShowDialog(HWND ParentHandle)
{
    // show dialog
    return DynamicDialogBoxParam(DllhInstance,(LPTSTR)IDD_DIALOG_DUMP,ParentHandle,(DLGPROC)CDumpUserInterface::WndProc,(LPARAM)this);
}

//-----------------------------------------------------------------------------
// Name: BreakWndProc
// Object: dialog callback of the dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CDumpUserInterface::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CDumpUserInterface* pDialogObject; 
    switch (uMsg)
    {
    case WM_INITDIALOG:
        pDialogObject=(CDumpUserInterface*)lParam;
        pDialogObject->Init(hWnd);
        break;
    case WM_CLOSE:
        pDialogObject=CDumpUserInterface::GetAssociatedObject(hWnd);
        if (pDialogObject)
            pDialogObject->Close();
        break;
    case WM_COMMAND:

        pDialogObject=CDumpUserInterface::GetAssociatedObject(hWnd);
        if (!pDialogObject)
            break;
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON_DUMP_RAW_DUMP:
            pDialogObject->ButtonRawDump();
            break;
        case IDC_BUTTON_DUMP:
            pDialogObject->ButtonDump();
            break;
        case IDCANCEL:
            pDialogObject->Close();
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ButtonDump
// Object: dump the specified module
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDumpUserInterface::ButtonDump()
{
    TCHAR psz[MAX_PATH];
    TCHAR* pc;
    PBYTE StartAddress=0;
    PBYTE EndAddress=0;
    PBYTE Size=0;
    HANDLE hWndControl;
    int iScanfRes;

    // get combo item handle
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_COMBO_DUMP_MODULE);
    *psz=0;
    if(DynamicSendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz)<=0)
        return;

    // get module start and end address
    // combo content is like "module name <0xStartAddress-0xEndAddress>"
    pc=_tcsrchr(psz,'<');
    if (!pc)
        return;
    pc++;

    iScanfRes=_stscanf(pc,_T("0x%p-0x%p>"),&StartAddress,&EndAddress);

    if ((iScanfRes!=2)||(!StartAddress)||(!EndAddress))
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Error retreiving module informations"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }
    // compute size
    Size=(PBYTE)((UINT_PTR)EndAddress-(UINT_PTR)StartAddress);

    // retrieve output path
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_DUMP_OUTPUT_FILE);
    if(DynamicSendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz)<=0)
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("No output file specified"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // make dump
    CDumpUserInterface::Dump(StartAddress,Size,psz);
}

//-----------------------------------------------------------------------------
// Name: ButtonRawDump
// Object: make a raw dump according to user interface
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDumpUserInterface::ButtonRawDump()
{
    TCHAR psz[MAX_PATH];
    PBYTE StartAddress=0;
    PBYTE Size=0;
    HANDLE hWndControl;

    // retrieve start address
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_DUMP_START_ADDRESS);
    DynamicSendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);

    if (!CStringConverter::StringToPBYTE(psz,&StartAddress))
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Bad Start Address"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // retrieve size
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_DUMP_SIZE);
    DynamicSendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz);

    if (!CStringConverter::StringToPBYTE(psz,&Size))
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Bad Size"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // retrieve output path
    hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_DUMP_OUTPUT_FILE);
    if(DynamicSendMessage((HWND) hWndControl,(UINT) WM_GETTEXT,MAX_PATH,(LPARAM)psz)<=0)
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("No output file specified"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // check memory validity
    if(IsBadReadPtr(StartAddress,(UINT_PTR)Size))
    {
        DynamicMessageBoxInDefaultStation(this->hWndDialog,_T("Bad Read Pointer"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // make dump
    CDumpUserInterface::Dump(StartAddress,Size,psz);
}

//-----------------------------------------------------------------------------
// Name: Dump
// Object: write dump from StartAddress to StartAddress+Size in pszOutputPath
// Parameters :
//     in  : DWORD StartAddress : start address
//           DWORD Size : size of dump
//           TCHAR* pszOutputPath : output file
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDumpUserInterface::Dump(PBYTE StartAddress,PBYTE Size,TCHAR* pszOutputPath)
{
    BOOL bSuccess;
    HANDLE hFile;
    HANDLE hSnapshot;
    HANDLE hThread;
    DWORD dwReallyRead=0;
    CLinkListItem* pItemAPIInfo;
    API_INFO *pAPIInfo;
    THREADENTRY32 Threadentry32={0};
    Threadentry32.dwSize=sizeof(THREADENTRY32);
    DWORD dwCurrentThreadId=GetCurrentThreadId();

    // suspend all other thread during Dump
    hSnapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        // do a loop throw all treads of the system
        if (Thread32First(hSnapshot,&Threadentry32))
        {
            do
            {
                // check process Id
                if (Threadentry32.th32OwnerProcessID!=dwCurrentProcessID)
                    continue;

                // check thread Id 
                if (Threadentry32.th32ThreadID==dwCurrentThreadId)
                    continue;
                // open thread
                hThread=OpenThread( THREAD_ALL_ACCESS,// THREAD_SUSPEND_RESUME 
                                    FALSE,
                                    Threadentry32.th32ThreadID
                                    );
                // suspend it
                SuspendThread(hThread);

                // close handle of opened thread
                CloseHandle(hThread);

            }while(Thread32Next(hSnapshot,&Threadentry32));
        }
    }


    //  restore original opcode for all functions
    pLinkListAPIInfos->Lock();
    for (pItemAPIInfo = pLinkListAPIInfos->Head;pItemAPIInfo;pItemAPIInfo=pItemAPIInfo->NextItem)
    {
        pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;

        // if dll hasn't been unloaded without hook removal
        if (!IsBadCodePtr((FARPROC)pAPIInfo->APIAddress))
        {
            // restore bytes only if not already done
            if (memcmp(pAPIInfo->APIAddress, pAPIInfo->Opcodes, pAPIInfo->OpcodeReplacementSize))
            {
                // assume opcodes is our one !
                // it can appear that for COM dll are unloaded and next reloaded at the same space,
                // if it's done too quickly or during COM unhooking, we can have original bytes
                // with original memory protection (due to reloading of dll), so pAPIInfo->APIAddress can be write protected
                if (memcmp(pAPIInfo->APIAddress,pAPIInfo->pbHookCodes, pAPIInfo->OpcodeReplacementSize)==0)
                {
                    // restore original opcodes
                    if (!IsBadWritePtr(pAPIInfo->APIAddress,pAPIInfo->OpcodeReplacementSize))
                        memcpy(pAPIInfo->APIAddress, pAPIInfo->Opcodes, pAPIInfo->OpcodeReplacementSize);
                }
            }
        }
    }
    pLinkListAPIInfos->Unlock();


    bSuccess=TRUE;

    // write dump

    // NOTICE: Dont try on GetSaveFileName here, because as we have suspended threads 
    // (here or in break interface) it will fail
    hFile = CreateFile(
        pszOutputPath,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, 
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        bSuccess=FALSE;
    }
    else
    {
        // ok as we are in process address space, only copy memory to file
        if (!WriteFile(hFile,
                    StartAddress,
                    (DWORD)Size,
                    &dwReallyRead,
                    NULL
                    ))
        {
            bSuccess=FALSE;
        }
        CloseHandle(hFile);
    }
    if(bSuccess)
        DynamicMessageBoxInDefaultStation(NULL,_T("Dump successfully completed"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
    else
        DynamicMessageBoxInDefaultStation(NULL,_T("Error writing dump"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);

    //  restore original hook opcode for all functions according to pAPIInfo->bOriginalOpcodes
    pLinkListAPIInfos->Lock();
    for (pItemAPIInfo = pLinkListAPIInfos->Head;pItemAPIInfo;pItemAPIInfo=pItemAPIInfo->NextItem)
    {
        pAPIInfo=(API_INFO*)pItemAPIInfo->ItemData;
        // as all is suspended, don't care critical section
        // as there are already original opcode restore only hooks
        if (!pAPIInfo->bOriginalOpcodes)
        {
            // if dll hasn't be removed
            if (!IsBadWritePtr(pAPIInfo->APIAddress,pAPIInfo->OpcodeReplacementSize))
                memcpy(pAPIInfo->APIAddress,pAPIInfo->pbHookCodes,pAPIInfo->OpcodeReplacementSize);
        }
    }
    pLinkListAPIInfos->Unlock();


    // Resume other suspended threads
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        // do a loop throw all treads of the system
        if (Thread32First(hSnapshot,&Threadentry32))
        {
            do
            {
                // check process Id
                if (Threadentry32.th32OwnerProcessID!=dwCurrentProcessID)
                    continue;

                // check thread Id
                if (Threadentry32.th32ThreadID==dwCurrentThreadId)
                    continue;
                // open thread
                hThread=OpenThread( THREAD_ALL_ACCESS,// THREAD_SUSPEND_RESUME 
                                    FALSE,
                                    Threadentry32.th32ThreadID
                                    );
                // suspend it
                ResumeThread(hThread);

                // close handle of opened thread
                CloseHandle(hThread);

            }while(Thread32Next(hSnapshot,&Threadentry32));
        }
    }
    // close snapshot
    CloseHandle(hSnapshot);
}