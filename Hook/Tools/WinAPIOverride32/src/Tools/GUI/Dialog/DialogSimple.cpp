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
// Object: allow to generate simple dialog window dynamically
//-----------------------------------------------------------------------------
#include "dialogsimple.h"

CDialogSimple::CDialogSimple(DWORD Width,DWORD Height,BOOL bModal)
{
    this->CommonConstructor();
    this->x=0;
    this->y=0;
    this->Width=Width;
    this->Height=Height;
    this->CenterWindow=TRUE;
    this->bModal=bModal;
}
CDialogSimple::CDialogSimple(DWORD x,DWORD y,DWORD Width,DWORD Height,BOOL bModal)
{
    this->CommonConstructor();
    this->x=x;
    this->y=y;
    this->Width=Width;
    this->Height=Height;
    this->CenterWindow=FALSE;
    this->bModal=bModal;
}

void CDialogSimple::CommonConstructor()
{
    this->hWndDialog=NULL;
    this->CreateCallback=NULL;
    this->CreateUserParam=NULL;
    this->DestroyCallback=NULL;
    this->DestroyUserParam=NULL;
    this->NotifyCallback=NULL;
    this->NotifyUserParam=NULL;
    this->CommandCallbackArray=NULL;
    this->CommandCallbackArraySize=NULL;
    this->CommandUserParam=NULL;
    this->hDefaultFont=NULL;
    this->DialogResult=0;
}

CDialogSimple::~CDialogSimple(void)
{

}

//-----------------------------------------------------------------------------
// Name: GetControlHandle
// Object: return the handle to the control
// Parameters :
//     in  : 
//     out :
//     return : HWND handle to the control
//-----------------------------------------------------------------------------
HWND CDialogSimple::GetControlHandle()
{
    return this->hWndDialog;
}

//-----------------------------------------------------------------------------
// Name: GetDialogResult
// Object: Get dialog result passed to Close methods, or 0 if user close window
// Parameters :
//     in  : 
//     out :
//     return : dialog result passed to Close methods, or 0 if user close window
//-----------------------------------------------------------------------------
INT_PTR CDialogSimple::GetDialogResult()
{
    return this->DialogResult;
}

//-----------------------------------------------------------------------------
// Name: GetAssociatedObject
// Object: Get object associated to window handle
// Parameters :
//     in  : HWND hWndDialog : handle of the window
//     out :
//     return : associated object if found, NULL if not found
//-----------------------------------------------------------------------------
CDialogSimple* CDialogSimple::GetAssociatedObject(HWND hWndDialog)
{
    return (CDialogSimple*)::GetWindowLongPtr(hWndDialog,GWLP_USERDATA);
}

//-----------------------------------------------------------------------------
// Name: ModelessDialogThread
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : CHexDisplay* current object
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CDialogSimple::ModelessDialogThread(PVOID lParam)
{
    CDialogSimple* pDialogSimple;
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    pDialogSimple=(CDialogSimple*)lParam;

    // if CDialogSimpleClassName is not already registered
    if (!::GetClassInfoEx(pDialogSimple->hInstance, CDialogSimpleClassName, &wc))
    {
        // register it

        wc.cbSize        = sizeof(WNDCLASSEX);
        wc.style         = 0;
        wc.lpfnWndProc   = CDialogSimple::WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = pDialogSimple->hInstance;
        wc.hIcon         = pDialogSimple->hIcon;
        wc.hCursor       = ::LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE+1;
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = CDialogSimpleClassName;
        wc.hIconSm       = pDialogSimple->hIcon;
        if(!::RegisterClassEx(&wc))
        {
            ::MessageBox(pDialogSimple->hParentHandle, _T("Window Registration Failed"), _T("Error"),MB_OK|MB_ICONERROR);
            return (DWORD)-1;
        }
    }

    // compute center window if needed
    if (pDialogSimple->CenterWindow)
    {
        RECT Rect;
        if (pDialogSimple->hParentHandle)
        {
            ::GetWindowRect(pDialogSimple->hParentHandle,&Rect);
            pDialogSimple->x=Rect.left+((int)(Rect.right-Rect.left-pDialogSimple->Width))/2; // Warning : force int cast to make sign div
            pDialogSimple->y=Rect.top+((int)(Rect.bottom-Rect.top-pDialogSimple->Height))/2; // Warning : force int cast to make sign div
        }
        else
        {
            pDialogSimple->x=((int)(::GetSystemMetrics(SM_CXFULLSCREEN)-pDialogSimple->Width))/2; // Warning : force int cast to make sign div
            pDialogSimple->y=((int)(::GetSystemMetrics(SM_CYFULLSCREEN)-pDialogSimple->Height))/2; // Warning : force int cast to make sign div
        }
    }

    // create the dialog window
    hwnd = ::CreateWindowEx(0, //| WS_EX_APPWINDOW,
        CDialogSimpleClassName,
        pDialogSimple->pszWindowTitle,
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        pDialogSimple->x,
        pDialogSimple->y, 
        pDialogSimple->Width, 
        pDialogSimple->Height,
        pDialogSimple->hParentHandle, // <-- doesn't display taskbar entry and multiple window (HWND_DESKTOP can be used instead)
        NULL,
        pDialogSimple->hInstance,
        pDialogSimple);

    if(hwnd == NULL)
    {
        ::MessageBox(pDialogSimple->hParentHandle, _T("Window Creation Failed"), _T("Error"),MB_OK|MB_ICONERROR);
        return (DWORD)-1;
    }

    // disable parent window (if any) for modal dialogs
    if (pDialogSimple->bModal && pDialogSimple->hParentHandle)
        ::EnableWindow(pDialogSimple->hParentHandle,FALSE);

    // message loop
    BOOL bRet;
    while( (bRet = ::GetMessage( &Msg, NULL, 0, 0 )) != 0)
    { 
        if (bRet == -1)
        {
            // handle the error and possibly exit
        }
        else
        {
            if (!::IsDialogMessage(hwnd,&Msg)) // filter msg and allow dialog key default action
            {
                ::TranslateMessage(&Msg); 
                ::DispatchMessage(&Msg); 
            }
        }
    }

    return (DWORD)Msg.wParam;
}

//-----------------------------------------------------------------------------
// Name: Show
// Object: show the dialog box
// Parameters :
//     in  : HWND ParentHandle : parent window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDialogSimple::Show(HINSTANCE hInstance,HWND hParent,TCHAR* pszWindowTitle)
{
    this->Show(hInstance,hParent,pszWindowTitle,NULL);
}
void CDialogSimple::Show(HINSTANCE hInstance,HWND hParent,TCHAR* pszWindowTitle,HMODULE hModule, DWORD IDIcon)
{
    this->Show(hInstance,hParent,pszWindowTitle,LoadIcon(hModule,MAKEINTRESOURCE(IDIcon)));
}
void CDialogSimple::Show(HINSTANCE hInstance,HWND hParent,TCHAR* pszWindowTitle,HICON hIcon)
{
    this->hInstance=hInstance;
    this->hParentHandle=hParent;
    this->pszWindowTitle=pszWindowTitle;
    this->hIcon=hIcon;

    if (this->bModal)
        CDialogSimple::ModelessDialogThread(this);
    else
        // create thread for dialog
        ::CloseHandle(CreateThread(NULL,0,CDialogSimple::ModelessDialogThread,this,0,NULL));
}

//-----------------------------------------------------------------------------
// Name: SetCreateCallback
// Object: register a callback for the WM_CREATE message
// Parameters :
//     in  : pfCreateCallback pCreateCallback : callback function
//           PVOID UserParam : user parameter passed to callback
//     out :
//     return : TRUE on Success
//-----------------------------------------------------------------------------
BOOL CDialogSimple::SetCreateCallback(pfCreateCallback pCreateCallback,PVOID UserParam)
{
    // check callback validity
    if (::IsBadCodePtr((FARPROC)pCreateCallback))
        return FALSE;

    this->CreateCallback=pCreateCallback;
    this->CreateUserParam=UserParam;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetDestroyCallback
// Object: register a callback for the WM_DESTROY message
// Parameters :
//     in  : pfDestroyCallback pDestroyCallback : callback function
//           PVOID UserParam : user parameter passed to callback
//     out :
//     return : TRUE on Success
//-----------------------------------------------------------------------------
BOOL CDialogSimple::SetDestroyCallback(pfDestroyCallback pDestroyCallback,PVOID UserParam)
{
    // check callback validity
    if (::IsBadCodePtr((FARPROC)pDestroyCallback))
        return FALSE;

    this->DestroyCallback=pDestroyCallback;
    this->DestroyUserParam=UserParam;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetDestroyCallback
// Object: register a callback for the WM_DESTROY message
// Parameters :
//     in  : pfDestroyCallback pDestroyCallback : callback function
//           PVOID UserParam : user parameter passed to callback
//     out :
//     return : TRUE on Success
//-----------------------------------------------------------------------------
BOOL CDialogSimple::SetNotifyCallback(pfNotifyCallback pNotifyCallback,PVOID UserParam)
{
    // check callback validity
    if (::IsBadCodePtr((FARPROC)pNotifyCallback))
        return FALSE;

    this->NotifyCallback=pNotifyCallback;
    this->NotifyUserParam=UserParam;
    
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetDestroyCallback
// Object: register a callback for the WM_COMMAND message
// Parameters :
//     in  : COMMANDS_CALLBACK_ARRAY_ITEM* Array : array if {control ID, associated callback function}
//           DWORD NbItems : number of item in array
//           PVOID UserParam : user parameter passed to callbacks
//     out :
//     return : TRUE on Success
//-----------------------------------------------------------------------------
BOOL CDialogSimple::SetCommandsCallBacks(COMMANDS_CALLBACK_ARRAY_ITEM* Array,DWORD NbItems,PVOID UserParam)
{
    this->CommandCallbackArraySize=NbItems;
    this->CommandUserParam=UserParam;

    // check callbacks validity
    for (DWORD Cnt=0;Cnt<NbItems;Cnt++)
    {
        if (::IsBadCodePtr((FARPROC)Array[Cnt].pCommandCallback))
            return FALSE;
    }
    this->CommandCallbackArray=Array;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: close current dialog
// Parameters :
//     in  : 
//     out :
//     return : TRUE on Success
//-----------------------------------------------------------------------------
BOOL CDialogSimple::Close()
{
    return this->Close(0);
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: close current dialog
// Parameters :
//     in  : INT_PTR DialogResult : a result accessible after dialog destruction
//                                  by GetDialogResult()
//     out :
//     return : TRUE on Success
//-----------------------------------------------------------------------------
BOOL CDialogSimple::Close(INT_PTR DialogResult)
{
    // ::EnableWindow must be called before ::DestroyWindow
    //MSDN :
    //A window must be enabled before it can be activated. 
    //For example, if an application is displaying a modeless dialog box and has disabled its main window,
    //the application must enable the main window before destroying the dialog box. 
    //Otherwise, another window will receive the keyboard focus and be activated. 
    if (this->bModal && this->hParentHandle)
        ::EnableWindow(this->hParentHandle,TRUE);

    // store result
    this->DialogResult=DialogResult;

    // destroy window
    return ::DestroyWindow(this->hWndDialog);
}

//-----------------------------------------------------------------------------
// Name: OnCreate
// Object: called on WM_CREATE message
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDialogSimple::OnCreate()
{
    if (this->CreateCallback)
    {
        // call creation callback
        this->CreateCallback(this,this->CreateUserParam);
    }

    // get default font
    NONCLIENTMETRICS ncm;
    ncm.cbSize=sizeof(NONCLIENTMETRICS);
    if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,0,&ncm,0))
    {
        this->hDefaultFont=::CreateFontIndirect(&ncm.lfMessageFont);
    }

    if (!this->hDefaultFont)
    {
        this->hDefaultFont=(HFONT)::GetStockObject(DEFAULT_GUI_FONT);
    }

    // applies default font to all children
    HWND hChild=::FindWindowEx(this->hWndDialog,0,0,0);
    while (hChild)
    {
        ::SendMessage(hChild,WM_SETFONT,(WPARAM)hDefaultFont,0);
        hChild=::FindWindowEx(this->hWndDialog,hChild,0,0);
    }
    
}

//-----------------------------------------------------------------------------
// Name: OnCreate
// Object: called on WM_DESTROY message
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDialogSimple::OnDestroy()
{
    // hide current window
    ::ShowWindow(this->hWndDialog,FALSE);

    if (this->DestroyCallback)
        this->DestroyCallback(this,this->DestroyUserParam);

    // destroy font if any
    if (this->hDefaultFont)
        ::DeleteObject(this->hDefaultFont);
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: dialog winow proc
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CDialogSimple::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_CREATE:
        {
            CREATESTRUCT* pCreateStruct;
            CDialogSimple* pDialogObject;
            // store CDialogSimple object in window user data
            pCreateStruct=(CREATESTRUCT*)lParam;
            pDialogObject=(CDialogSimple*)pCreateStruct->lpCreateParams;
            ::SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)pDialogObject);

            // save dialog handle
            pDialogObject->hWndDialog=hwnd;

            // call OnCreate method
            pDialogObject->OnCreate();
        }
        break;
    case WM_CLOSE:
        {
            // get CDialogSimple object
            CDialogSimple* pDialogObject; 
            pDialogObject=CDialogSimple::GetAssociatedObject(hwnd);
            if (!pDialogObject)
                break;

            // call close method
            pDialogObject->Close();
        }
        break;
    case WM_COMMAND:
        {
            // get CDialogSimple object
            CDialogSimple* pDialogObject; 
            pDialogObject=CDialogSimple::GetAssociatedObject(hwnd);
            if (!pDialogObject)
                break;

            // if callbacks have been defined for WM_COMMAND
            if (pDialogObject->CommandCallbackArray)
            {
                // search for a matching command id in CommandCallbackArray
                WORD CommandId=LOWORD(wParam);
                for (DWORD Cnt=0;Cnt<pDialogObject->CommandCallbackArraySize;Cnt++)
                {
                    if (pDialogObject->CommandCallbackArray[Cnt].CommandID==CommandId)
                    {
                        // call callback associated to id
                        pDialogObject->CommandCallbackArray[Cnt].pCommandCallback(pDialogObject,pDialogObject->CommandUserParam);
                        break;// to earn time (and as a single id is supposed to have only one callback)
                    }
                }
            }
        }
        break;
    case WM_NOTIFY:
        {
            // get CDialogSimple object
            CDialogSimple* pDialogObject; 
            pDialogObject=CDialogSimple::GetAssociatedObject(hwnd);
            if (!pDialogObject)
                break;

            if (pDialogObject->NotifyCallback)
                pDialogObject->NotifyCallback(pDialogObject,wParam,lParam,pDialogObject->NotifyUserParam);
        }
        break;
    case WM_DESTROY:
        {
            // get CDialogSimple object
            CDialogSimple* pDialogObject; 
            pDialogObject=CDialogSimple::GetAssociatedObject(hwnd);
            if (!pDialogObject)
                break;

            // call OnDestroy method
            pDialogObject->OnDestroy();
        }
        ::PostQuitMessage(0);
        break;

    default:
        return ::DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
