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

#include "BreakUserInterface.h"

#include "DynamicLoadedFuncs.h"
#include "MemoryUserInterface.h"
#include "RegistersUserInterface.h"
#include "DumpUserInterface.h"
#include "CallStackUserInterface.h"
#include "dialoginterfacemanager.h"
#include "../../../Disasm/CallInstructionSizeFromRetAddress/CallInstructionSizeFromRetAddress.h"

extern BOOL bBreakDialogDontBreakAPIOverrideThreads;
extern HINSTANCE DllhInstance;
extern DWORD dwCurrentProcessID;
extern DWORD APIOverrideThreads[NB_APIOVERRIDE_WORKER_THREADS];
extern BOOL CanCreateThread();
//-----------------------------------------------------------------------------
// Name: GetAssociatedObject
// Object: Get object associated to window handle
// Parameters :
//     in  : HWND hWndDialog : handle of the window
//     out :
//     return : associated object if found, NULL if not found
//-----------------------------------------------------------------------------
CBreakUserInterface* CBreakUserInterface::GetAssociatedObject(HWND hWndDialog)
{
    return (CBreakUserInterface*)DynamicGetWindowLong(hWndDialog,GWLP_USERDATA);
}

CBreakUserInterface::CBreakUserInterface(PAPI_INFO pAPIInfo,
                                         LOG_INFOS* pLogInfo,
                                         PBYTE StackParametersPointer,
                                         PREGISTERS pRegisters,
                                         double* pDoubleResult,
                                         PBYTE ReturnAddress,
                                         PBYTE CallerEbp,
                                         BOOL BeforeCall,
                                         CLinkListSingleThreaded* pLinkListTlsData)
{
    this->Dlg_Res=-1;
    this->pAPIInfo=pAPIInfo;
    // make a local copy to avoid to modify it
    this->pLogInfo=pLogInfo;
    this->BeforeCall=BeforeCall;
    this->StackParametersPointer=StackParametersPointer;
    this->pRegisters=pRegisters;
    this->pDoubleResult=pDoubleResult;
    this->ReturnAddress=ReturnAddress;
    this->CallerEbp=CallerEbp;
    this->pLinkListTlsData=pLinkListTlsData;
}

CBreakUserInterface::~CBreakUserInterface(void)
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
void CBreakUserInterface::Init(HWND hWndDialog)
{
    this->hWndDialog=hWndDialog;

    HWND hWndControl;
    // create fixed with font
    this->hFont=DynamicCreateFont(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, NULL);

    // set hexa textboxes fonts (fixed width font)
    if (this->hFont)
    {
        hWndControl=DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_BREAK_PARAMETERS);
        DynamicSendMessage((HWND) hWndControl,(UINT) WM_SETFONT,(WPARAM)this->hFont,FALSE);
    }

    TCHAR* pszLog;
    TCHAR* psz;
    TCHAR pszTmp[MAX_PATH];
    DWORD Cnt;
    PBYTE StackValue;
    TCHAR* ParamString;
    DWORD LogSize=0;
    DWORD MaxLogSize=10*MAX_PATH;
    DWORD ParamLogSize;
    pszLog=new TCHAR[MaxLogSize];

    // associate object to window handle
    DynamicSetWindowLongPtr(hWndDialog,GWLP_USERDATA,(LONG)this);
    
    // write break text
    _tcscpy(pszTmp,_T("Break "));
    if (this->BeforeCall)
        _tcscat(pszTmp,_T("before"));
    else
        _tcscat(pszTmp,_T("after"));

    _stprintf(pszLog,_T("%s call of function %s (%s)\r\n\r\n")
                  _T("Process: 0x%X") 
                  _T("     Thread: 0x%X") 
                  _T("     Caller Address: 0x%p"),
                  pszTmp,
                  this->pAPIInfo->szAPIName,
                  this->pAPIInfo->szModuleName,
                  dwCurrentProcessID,
                  GetCurrentThreadId(),
                  this->ReturnAddress - CCallInstructionSizeFromReturnAddress::GetCallInstructionSizeFromReturnAddress(this->ReturnAddress));
    _tcscat(pszLog,_T("\r\n\r\nYou can View/Modify registers values or memory if you want."));

    // set break text
    DynamicSendMessage(DynamicGetDlgItem(this->hWndDialog,IDC_STATIC_BREAK_TEXT),WM_SETTEXT,0,(LPARAM)pszLog);

    *pszLog=0;
    
    // copy func with params
   
    // show parameters and their address for not parsed params
    for (Cnt=0;Cnt<pAPIInfo->MonitoringParamCount;Cnt++)
    {
        // get param value from stack
        StackValue=this->StackParametersPointer+sizeof(PBYTE)*Cnt;

        _stprintf(pszTmp,_T(" Param%u: Addr:0x%p "),Cnt+1,StackValue);
        // add it to psz
        _tcscat(pszLog,pszTmp);

        // compute call size
        LogSize=(DWORD)_tcslen(pszLog);

        // fill log info param name as it's not done in case of non breaking hook to earn time
        _tcscpy(this->pLogInfo->ParamLogList[Cnt].pszParameterName,this->pAPIInfo->ParamList[Cnt].pszParameterName);

        // translate param to string
        CSupportedParameters::ParameterToString(this->pAPIInfo->szModuleName,&this->pLogInfo->ParamLogList[Cnt],&ParamString,TRUE);

        // check buffer space
        ParamLogSize=(DWORD)_tcslen(ParamString);
        if (LogSize+ParamLogSize+1>MaxLogSize-CBREAKUSERINTERFACE_RETURN_STRING_MAX_SIZE)
        {
            MaxLogSize=LogSize+ParamLogSize*2;
            psz=pszLog;
            pszLog=new TCHAR[MaxLogSize];
            _tcscpy(pszLog,psz);
            delete[] psz;
        }

        // add param to log
        _tcscat(pszLog,ParamString);

        // free string allocated by ParameterToString
        delete[] ParamString;

        // go to new line for next param
        _tcscat(pszLog,_T("\r\n"));
    }

    if (!this->BeforeCall)
    {
        // get returned value
        _stprintf(pszTmp,_T("\r\n Returned Value  : 0x%.8X"),this->pRegisters->eax);

        // add it to psz
        _tcscat(pszLog,pszTmp);

        // get returned floating value
        _stprintf(pszTmp,_T("\r\n Floating Return : %.19g"), *this->pDoubleResult);
        
        // add it to psz
        _tcscat(pszLog,pszTmp);
    }

    DynamicSendMessage(DynamicGetDlgItem(this->hWndDialog,IDC_EDIT_BREAK_PARAMETERS),WM_SETTEXT,0,(LPARAM)pszLog);

    delete[] pszLog;
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close like. EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CBreakUserInterface::Close()
{
    if (DynamicSendMessage(DynamicGetDlgItem(this->hWndDialog,IDC_CHECK_DONT_BREAK_AGAIN),BM_GETCHECK,0,0)==BST_CHECKED)
    {
        // clear all breaking flags
        this->pAPIInfo->LogBreakWay.BreakAfterCall=0;
        this->pAPIInfo->LogBreakWay.BreakAfterCallIfNotNullResult=0;
        this->pAPIInfo->LogBreakWay.BreakAfterCallIfNullResult=0;
        this->pAPIInfo->LogBreakWay.BreakBeforeCall=0;
        this->pAPIInfo->LogBreakWay.BreakLogInputAfter=0;
        this->pAPIInfo->LogBreakWay.BreakLogOutputAfter=0;
        this->pAPIInfo->LogBreakWay.BreakOnFailure=0;
        this->pAPIInfo->LogBreakWay.BreakOnSuccess=0;
    }

    DynamicDeleteObject(this->hFont);
    DynamicEndDialog(this->hWndDialog,0);
}


DWORD WINAPI CBreakUserInterface::ModelessDialogThread(PVOID lParam)
{
    HDESK OldDesktop=NULL;
    HDESK CurrentDesktop=NULL;
    HWINSTA OldStation=NULL;
    HWINSTA CurrentStation=NULL;
    // show dialog
    if (CDialogInterfaceManager::SetDefaultStation(&CurrentStation,&OldStation,&CurrentDesktop,&OldDesktop))
    {
        ((CBreakUserInterface*)lParam)->Dlg_Res=DynamicDialogBoxParam(DllhInstance,(LPTSTR)IDD_DIALOG_BREAK,NULL,(DLGPROC)CBreakUserInterface::WndProc,(LPARAM)lParam);
    }
    else
        DynamicMessageBoxInDefaultStation(NULL,_T("Error setting default station : can't display break dialog"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);

    CDialogInterfaceManager::RestoreStation(CurrentStation,OldStation,CurrentDesktop,OldDesktop);

    return 0;
}

//-----------------------------------------------------------------------------
// Name: IsThreadApiOverrideOne
// Object: check if provided thread id is a thread created by ApiOverride dll
//         for WinApiOverride work
// Parameters :
//     in  : DWORD ThreadId : Id of thread to check
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CBreakUserInterface::IsThreadApiOverrideOne(DWORD ThreadId)
{
    // search id through apioverride worker threads
    for (DWORD cnt=0;cnt<NB_APIOVERRIDE_WORKER_THREADS;cnt++)
    {
        // if thread id is one of the apioverride worker threads
        if (ThreadId==APIOverrideThreads[cnt])
            return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: ShowDialog
// Object: show the filter dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : parent window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CBreakUserInterface::ShowDialog()
{
    INT_PTR Ret=-1;
    HANDLE hSnapshot;
    HANDLE hThread;
    THREADENTRY32 Threadentry32={0};
    Threadentry32.dwSize=sizeof(THREADENTRY32);
    DWORD dwCurrentThreadId=GetCurrentThreadId();

    // suspend all other thread to avoid loosing API call during Break
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

                if (bBreakDialogDontBreakAPIOverrideThreads)
                {
                    if (this->IsThreadApiOverrideOne(Threadentry32.th32ThreadID))
                        continue;
                }
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

    // check if service a user interaction
    if (!CDialogInterfaceManager::CanWindowInteract())
    {
        DynamicMessageBoxInDefaultStation(
            NULL,
            _T("Process can't interact with user interface, so break window can't be displayed.\r\n")
            _T("Your process is currently breaked, to allow you to do some operation\r\n")
            _T("Click ok to resume it")
            _T("\r\n\r\nNotice: To use break dialog with services, please refer to documentation,\r\n")
            _T("in FAQ section: How to use break dialog with services ?"),
            _T("Error"),
            MB_OK|MB_ICONERROR|MB_TOPMOST);
    }
    else // if user interaction
    {
        if (!CanCreateThread())
        {
            // thread can't be created during Tls callback, just try yo show dialog
            // it's ok because Tls callback hooking mode is used only when winapioverride
            // creates the process, so we have full access rights on created process, and
            // created process uses user window station and desktop
            ModelessDialogThread(this);
        }
        else
        {
            hThread=CDialogInterfaceManager::AdjustThreadSecurityAndLaunchDialogThread(ModelessDialogThread,this);
            if (hThread)
            {
                // wait the end of the dialog
                WaitForSingleObject(hThread,INFINITE);
                // close thread handle
                CloseHandle(hThread);
                // get dialog return
                Ret=this->Dlg_Res;
            }
            else
            {
                Ret=0;
                DynamicMessageBoxInDefaultStation(
                    NULL,
                    _T("Error creating thread for the break dialog.\r\n")
                    _T("Your process is currently breaked, to allow you to do some operation\r\n")
                    _T("Click ok to resume it"),
                    _T("Error"),
                    MB_OK|MB_ICONERROR|MB_TOPMOST);
            }
        }
    }


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

                if (bBreakDialogDontBreakAPIOverrideThreads)
                {
                    if (this->IsThreadApiOverrideOne(Threadentry32.th32ThreadID))
                        continue;
                }

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

    return Ret;
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog callback of the dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CBreakUserInterface::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CBreakUserInterface* pDialogObject; 
    switch (uMsg)
    {
    case WM_INITDIALOG:
        pDialogObject=(CBreakUserInterface*)lParam;
        pDialogObject->Init(hWnd);
        break;
    case WM_CLOSE:
        pDialogObject=CBreakUserInterface::GetAssociatedObject(hWnd);
        if (pDialogObject)
            pDialogObject->Close();
        break;
    case WM_COMMAND:
        
        pDialogObject=CBreakUserInterface::GetAssociatedObject(hWnd);
        if (!pDialogObject)
            break;
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON_CONTINUE:
            pDialogObject->Close();
            break;
        case IDC_BUTTON_MEMORY:
            {
                CMemoryUserInterface* pMemUI=new CMemoryUserInterface();
                pMemUI->ShowDialog(hWnd);
                delete pMemUI;
            }
            break;
        case IDC_BUTTON_REGISTERS:
            {
                CRegistersUserInterface* pRegUI=new CRegistersUserInterface(pDialogObject->pRegisters,pDialogObject->pDoubleResult,pDialogObject->BeforeCall);
                pRegUI->ShowDialog(hWnd);
                delete pRegUI;
            }
            break;
        case IDC_BUTTON_BREAK_DUMP:
            {
                CDumpUserInterface* pDumpUI=new CDumpUserInterface();
                pDumpUI->ShowDialog(hWnd);
                delete pDumpUI;
            }
            break;
        case IDC_BUTTON_CALLSTACK:
            {
                CCallStackUserInterface* pCallStackUI=new CCallStackUserInterface(pDialogObject->CallerEbp,pDialogObject->ReturnAddress,pDialogObject->pLinkListTlsData);
                pCallStackUI->ShowDialog(hWnd);
                delete pCallStackUI;
            }
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}