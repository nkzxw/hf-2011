#include "HtmlViewerWindow.h"

CHtmlViewerWindow::CHtmlViewerWindow(DWORD Width,DWORD Height)
{
    this->CommonConstructor();
    this->x=0;
    this->y=0;
    this->Width=Width;
    this->Height=Height;
    this->CenterWindow=TRUE;
}
CHtmlViewerWindow::CHtmlViewerWindow(DWORD x,DWORD y,DWORD Width,DWORD Height)
{
    this->CommonConstructor();
    this->x=x;
    this->y=y;
    this->Width=Width;
    this->Height=Height;
    this->CenterWindow=FALSE;
}
// allow to specify a path for HtmlViewerDll (in case dll is not in the same path of the app)
CHtmlViewerWindow::CHtmlViewerWindow(DWORD Width,DWORD Height,TCHAR* HtmlViewerDllPath)
{
    this->CommonConstructor();
    this->x=0;
    this->y=0;
    this->Width=Width;
    this->Height=Height;
    this->CenterWindow=TRUE;
    _tcsncpy(this->szHtmlViewerDllPath,HtmlViewerDllPath,MAX_PATH);
    this->szHtmlViewerDllPath[MAX_PATH-1]=0;
}
CHtmlViewerWindow::CHtmlViewerWindow(DWORD x,DWORD y,DWORD Width,DWORD Height,TCHAR* HtmlViewerDllPath)
{
    this->CommonConstructor();
    this->x=x;
    this->y=y;
    this->Width=Width;
    this->Height=Height;
    this->CenterWindow=FALSE;
    _tcsncpy(this->szHtmlViewerDllPath,HtmlViewerDllPath,MAX_PATH);
    this->szHtmlViewerDllPath[MAX_PATH-1]=0;
}

void CHtmlViewerWindow::CommonConstructor()
{
    this->bInitializationError=FALSE;
    *this->szHtmlViewerDllPath=0;
    this->bLoaded=FALSE;
    this->pHtmlViewerHelper=NULL;
    this->hWindowClose=CreateEvent(0,TRUE,FALSE,0);// manual reset event
    this->hWindowClosing=CreateEvent(0,TRUE,FALSE,0);// manual reset event
    this->hWindowInit=CreateEvent(0,FALSE,FALSE,0);
    this->hWaitForPageCompletedFinished=CreateEvent(0,TRUE,TRUE,0);// manual reset event, initially signaled
    this->hWaitForPageCompletedCancelEvent=CreateEvent(0,FALSE,FALSE,0);
    this->hWaitForPageCompletedInternalCancelEvent=CreateEvent(0,FALSE,FALSE,0);
    this->hWndDialog=NULL;
    this->hHtmlViewer=NULL;
    this->HtmlViewerhWnd=NULL;

    // force CoInitialize on current thread
    HMODULE hModuleOle;
    hModuleOle=GetModuleHandle(CHtmlViewerWindow_OLE_DLL_NAME);
    if (!hModuleOle)
        hModuleOle=LoadLibrary(CHtmlViewerWindow_OLE_DLL_NAME);

    pfOleInitialize pOleInitialize=(pfOleInitialize)GetProcAddress(hModuleOle,"OleInitialize");
    if (pOleInitialize)
        pOleInitialize(0);
    else
    {
        MessageBox(NULL, _T("Error loading Ole dynamic library"), _T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
    }

}

CHtmlViewerWindow::~CHtmlViewerWindow(void)
{
    CloseHandle(this->hWindowClosing);
    CloseHandle(this->hWindowClose);
    CloseHandle(this->hWindowInit);
    CloseHandle(this->hWaitForPageCompletedCancelEvent);
    CloseHandle(this->hWaitForPageCompletedInternalCancelEvent);
    CloseHandle(this->hWaitForPageCompletedFinished);
}

HWND CHtmlViewerWindow::GetControlHandle()
{
    return this->hWndDialog;
}

//-----------------------------------------------------------------------------
// Name: GetAssociatedObject
// Object: Get object associated to window handle
// Parameters :
//     in  : HWND hWndDialog : handle of the window
//     out :
//     return : associated object if found, NULL if not found
//-----------------------------------------------------------------------------
CHtmlViewerWindow* CHtmlViewerWindow::GetAssociatedObject(HWND hWndDialog)
{
    return (CHtmlViewerWindow*)GetWindowLongPtr(hWndDialog,GWLP_USERDATA);
}

//-----------------------------------------------------------------------------
// Name: ModelessDialogThread
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : PVOID lParam : CHexDisplay* current object
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CHtmlViewerWindow::ModelessDialogThread(PVOID lParam)
{
    CHtmlViewerWindow* pHtmlViewerWindow;
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    pHtmlViewerWindow=(CHtmlViewerWindow*)lParam;

    // if CHtmlViewerWindowClassName is not already registered
    if (!GetClassInfoEx(pHtmlViewerWindow->hInstance, CHtmlViewerWindowClassName, &wc))
    {
        // register it

        wc.cbSize        = sizeof(WNDCLASSEX);
        wc.style         = 0;
        wc.lpfnWndProc   = CHtmlViewerWindow::WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = pHtmlViewerWindow->hInstance;
        wc.hIcon         = pHtmlViewerWindow->hIcon;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_WINDOWFRAME;
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = CHtmlViewerWindowClassName;
        wc.hIconSm       = pHtmlViewerWindow->hIcon;
        if(!RegisterClassEx(&wc))
        {
            MessageBox(pHtmlViewerWindow->hParentHandle, _T("Window Registration Failed"), _T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            return (DWORD)-1;
        }
    }

    // compute center window if needed
    if (pHtmlViewerWindow->CenterWindow)
    {
        RECT Rect;
        if (pHtmlViewerWindow->hParentHandle)
        {
            GetWindowRect(pHtmlViewerWindow->hParentHandle,&Rect);
            pHtmlViewerWindow->x=Rect.left+((int)(Rect.right-Rect.left-pHtmlViewerWindow->Width))/2; // Warning : force int cast to make sign div
            pHtmlViewerWindow->y=Rect.top+((int)(Rect.bottom-Rect.top-pHtmlViewerWindow->Height))/2; // Warning : force int cast to make sign div
        }
        else
        {
            pHtmlViewerWindow->x=((int)(::GetSystemMetrics(SM_CXFULLSCREEN)-pHtmlViewerWindow->Width))/2; // Warning : force int cast to make sign div
            pHtmlViewerWindow->y=((int)(::GetSystemMetrics(SM_CYFULLSCREEN)-pHtmlViewerWindow->Height))/2; // Warning : force int cast to make sign div
        }
    }

    // create the dialog window
    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, //| WS_EX_APPWINDOW,
                          CHtmlViewerWindowClassName,
                          pHtmlViewerWindow->pszWindowTitle,
                          WS_OVERLAPPEDWINDOW,
                          pHtmlViewerWindow->x,
                          pHtmlViewerWindow->y, 
                          pHtmlViewerWindow->Width, 
                          pHtmlViewerWindow->Height,
                          HWND_DESKTOP, // pHtmlViewerWindow->hParentHandle, <-- doesn't display taskbar entry and multiple window 
                          NULL,
                          pHtmlViewerWindow->hInstance,
                          pHtmlViewerWindow);

    if (pHtmlViewerWindow->bInitializationError)
    {
        // warning pHtmlViewerWindow can be destroyed after SetEvent(pHtmlViewerWindow->hWindowInit); call
        SetEvent(pHtmlViewerWindow->hWindowInit);
        return (DWORD)-1;
    }

    SetEvent(pHtmlViewerWindow->hWindowInit);

    if(hwnd == NULL)
    {
        MessageBox(pHtmlViewerWindow->hParentHandle, _T("Window Creation Failed"), _T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return (DWORD)-1;
    }

    // show it
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    BOOL bRet;
    while( (bRet = GetMessage( &Msg, NULL, 0, 0 )) != 0)
    { 
        if (bRet == -1)
        {
            // handle the error and possibly exit
        }
        else
        {
            // MSDN : calling IOleInPlaceActiveObject::TranslateAccelerator from your container's message loop before doing any other translation
            if (Msg.message==WM_KEYDOWN)
            { 
                // MSDN : You should apply your own translation only when this method returns S_FALSE
                if (pHtmlViewerWindow->pHtmlViewerHelper)
                {
                    if (pHtmlViewerWindow->pHtmlViewerHelper->TranslateAccelerator(pHtmlViewerWindow->hHtmlViewer,&Msg))
                        continue;
                }
            }
            TranslateMessage(&Msg); 
            DispatchMessage(&Msg); 
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
BOOL CHtmlViewerWindow::Show(HINSTANCE hInstance,HWND hParent,TCHAR* pszWindowTitle)
{
    return this->Show(hInstance,hParent,pszWindowTitle,NULL);
}
BOOL CHtmlViewerWindow::Show(HINSTANCE hInstance,HWND hParent,TCHAR* pszWindowTitle,HMODULE hModule, DWORD IDIcon)
{
    return this->Show(hInstance,hParent,pszWindowTitle,LoadIcon(hModule,MAKEINTRESOURCE(IDIcon)));
}
BOOL CHtmlViewerWindow::Show(HINSTANCE hInstance,HWND hParent,TCHAR* pszWindowTitle,HICON hIcon)
{
    this->hInstance=hInstance;
    this->hParentHandle=hParent;
    this->pszWindowTitle=pszWindowTitle;
    this->hIcon=hIcon;

    // create thread for dialog
    CloseHandle(CreateThread(NULL,0,CHtmlViewerWindow::ModelessDialogThread,this,0,NULL));
    // wait dialog and html control to be fully initialized
    WaitForSingleObject(this->hWindowInit,INFINITE);

    if (this->bInitializationError)
        return FALSE;

    return TRUE;
}

void CHtmlViewerWindow::OnInit()
{
    // create html viewer helper
    this->pHtmlViewerHelper=new CHtmlViewerHelper();
    // load it
    if (!this->pHtmlViewerHelper->Load(this->szHtmlViewerDllPath))
    {
        TCHAR sz[2*MAX_PATH];
        _tcscpy(sz,_T("Error loading library "));
        _tcscat(sz,HTMLVIEWER_DLL_NAME);
        MessageBox(this->hWndDialog,sz,_T("Error"),MB_OK|MB_TOPMOST|MB_ICONERROR);
        // set event to avoid deadlock
        this->bInitializationError = TRUE;
        return;
    }
    this->bLoaded=TRUE;
    RECT rect;
    GetClientRect(this->hWndDialog,&rect);
    // create html control
    this->hHtmlViewer=this->pHtmlViewerHelper->CreateHtmlViewer(this->hWndDialog,0,0,rect.right,rect.bottom,&this->HtmlViewerhWnd);
}
void CHtmlViewerWindow::OnClose()
{
    // hide current window
    ShowWindow(this->hWndDialog,FALSE);

    // set closing event
    SetEvent(this->hWindowClosing);

    if (this->bLoaded)
    {
        this->bLoaded=FALSE;

        // assume html control has finished to work
        WaitForSingleObject(this->hWaitForPageCompletedFinished,60000);

        // destroy html control
        this->pHtmlViewerHelper->DestroyHtmlViewer(this->hHtmlViewer);
    }

    // free CHtmlViewerHelper object
    delete this->pHtmlViewerHelper;
    this->pHtmlViewerHelper=NULL;

    DestroyWindow(this->hWndDialog);

    // signal window closed
    SetEvent(this->hWindowClose);
}
void CHtmlViewerWindow::WaitWindowClose()
{
    WaitForSingleObject(this->hWindowClose,INFINITE);
}
void CHtmlViewerWindow::OnSize()
{
    RECT rect;
    GetClientRect(this->hWndDialog,&rect);
    MoveWindow(this->HtmlViewerhWnd,0,0,rect.right-rect.left,rect.bottom-rect.top,TRUE);
}

LRESULT CALLBACK CHtmlViewerWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
    case WM_CREATE:
        {
            CREATESTRUCT* pCreateStruct;
            CHtmlViewerWindow* pDialogObject; 
            pCreateStruct=(CREATESTRUCT*)lParam;
            pDialogObject=(CHtmlViewerWindow*)pCreateStruct->lpCreateParams;
            SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)pDialogObject);
            pDialogObject->hWndDialog=hwnd;
            pDialogObject->OnInit();
            if (pDialogObject->bInitializationError)
                return -1;
        }
        break;
    case WM_SIZE:
        {
            CHtmlViewerWindow* pDialogObject; 
            pDialogObject=CHtmlViewerWindow::GetAssociatedObject(hwnd);
            if (!pDialogObject)
                break;
            pDialogObject->OnSize();
        }
        break;
    case WM_CLOSE:
        {
            CHtmlViewerWindow* pDialogObject; 
            pDialogObject=CHtmlViewerWindow::GetAssociatedObject(hwnd);
            if (!pDialogObject)
                break;
            pDialogObject->OnClose();
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        {
            CHtmlViewerWindow* pDialogObject; 
            pDialogObject=CHtmlViewerWindow::GetAssociatedObject(hwnd);
            if (!pDialogObject)
                break;
            // force html control to be repaint (not always the case after GetSaveFileName by the way)
            RedrawWindow( pDialogObject->HtmlViewerhWnd,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
        }
        break;
    // Message used to call methods in the same thread of the control owner
    case WM_HTML_VIEWER_WINDOW_MESSAGE:
        {
            CHtmlViewerWindow* pDialogObject; 
            pDialogObject=CHtmlViewerWindow::GetAssociatedObject(hwnd);
            if (!pDialogObject)
                break;
            pDialogObject->ProcessHTMLViewerThreadedMessage((tagUserMessageValues)wParam,(PARAMETERS_STRUCT*)lParam);
        }
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
//                       redirect methods to dll                                                   //
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CHtmlViewerWindow::ThreadedLoadEmptyPageAndSetBodyContent(TCHAR* Content)
{
    if (!this->bLoaded)
        return FALSE;
    BOOL bRet=this->pHtmlViewerHelper->LoadEmptyPageAndSetBodyContent(this->hHtmlViewer,Content);
    SetEvent(this->hWaitForPageCompletedFinished);
    return bRet;
}
BOOL CHtmlViewerWindow::ThreadedAddHtmlContentToBody(TCHAR* Content)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->AddHtmlContentToBody(this->hHtmlViewer,Content);
}
BOOL CHtmlViewerWindow::ThreadedAddHtmlContentToElement(TCHAR* Id,TCHAR* Content)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->AddHtmlContentToElement(this->hHtmlViewer,Id,Content);
}
BOOL CHtmlViewerWindow::ThreadedSetElementSrc(TCHAR* Id,TCHAR* Src)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->SetElementSrc(this->hHtmlViewer,Id,Src);
}
BOOL CHtmlViewerWindow::ThreadedSetElementInnerHtml(TCHAR* Id,TCHAR* Html)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->SetElementInnerHtml(this->hHtmlViewer,Id,Html);
}


DWORD WINAPI CHtmlViewerWindow::WaitForPageCompletedCancelWatcherStatic(PVOID lParam)
{
    return ((CHtmlViewerWindow*)lParam)->WaitForPageCompletedCancelWatcher();
    
}
DWORD CHtmlViewerWindow::WaitForPageCompletedCancelWatcher()
{
    // cancel this->pHtmlViewerHelper->WaitForPageCompleted 
    // as soon as cancel event or current window closing occurs
    HANDLE ph[2]={this->hWindowClosing,this->hWaitForPageCompletedCancelEvent};
    WaitForMultipleObjects(2,ph,FALSE,INFINITE);
    SetEvent(this->hWaitForPageCompletedInternalCancelEvent);
    return 0;
}

BOOL CHtmlViewerWindow::ThreadedNavigate(TCHAR* Url)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->Navigate(this->hHtmlViewer,Url);
}
BOOL CHtmlViewerWindow::ThreadedSave()
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->Save(this->hHtmlViewer);
}
BOOL CHtmlViewerWindow::ThreadedSaveAs(TCHAR* FileName)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->SaveAs(this->hHtmlViewer,FileName);
}
BOOL CHtmlViewerWindow::ThreadedExecScript(TCHAR* ScriptContent,VARIANT* pScriptResult,EXCEPINFO* pExcepInfo)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->ExecScriptEx2(this->hHtmlViewer,ScriptContent,pScriptResult,pExcepInfo);
}
BOOL CHtmlViewerWindow::ThreadedExecScript(TCHAR* ScriptContent,VARIANT* pScriptResult)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->ExecScriptEx(this->hHtmlViewer,ScriptContent,pScriptResult);
}
BOOL CHtmlViewerWindow::ThreadedExecScript(TCHAR* ScriptContent)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->ExecScript(this->hHtmlViewer,ScriptContent);
}
BOOL CHtmlViewerWindow::ThreadedSetElementsEventsCallBack(pfElementEventsCallBack ElementEventsCallBack,LPVOID UserParam)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->SetElementsEventsCallBack(this->hHtmlViewer,ElementEventsCallBack,UserParam);
}
BOOL CHtmlViewerWindow::ThreadedSetElementsEventsCallBackEx(pfElementEventsCallBackEx ElementEventsCallBackEx,LPVOID UserParam)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->SetElementsEventsCallBackEx(this->hHtmlViewer,ElementEventsCallBackEx,UserParam);
}
BOOL CHtmlViewerWindow::ThreadedEnableContextMenu(BOOL Enable)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->EnableContextMenu(this->hHtmlViewer,Enable);
}
BOOL CHtmlViewerWindow::ThreadedEnableSelection(BOOL Enable)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->EnableSelection(this->hHtmlViewer,Enable);
}

IHTMLElement* CHtmlViewerWindow::ThreadedGetHTMLElement(TCHAR* Id)
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->GetHTMLElement(this->hHtmlViewer,Id);
}
IDispatch* CHtmlViewerWindow::ThreadedGetHTMLDocument()
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->GetHTMLDocument(this->hHtmlViewer);
}

BOOL CHtmlViewerWindow::ThreadedWaitForPageCompleted(HANDLE hEventCancel)
{
    if (!this->bLoaded)
        return FALSE;
    BOOL bRet;
    HANDLE hEvt;
    if (hEventCancel)
    {
        this->hWaitForPageCompletedCancelEvent=hEventCancel;
        CloseHandle(CreateThread(0,0,CHtmlViewerWindow::WaitForPageCompletedCancelWatcherStatic,this,0,0));
        hEvt=this->hWaitForPageCompletedInternalCancelEvent;
    }
    else
        hEvt=this->hWindowClosing;

    ResetEvent(this->hWaitForPageCompletedFinished);

    bRet=this->pHtmlViewerHelper->WaitForPageCompleted(this->hHtmlViewer,hEvt);

    SetEvent(this->hWaitForPageCompletedFinished);

    return bRet;
}
IWebBrowser2* CHtmlViewerWindow::GetWebBrowser()
{
    if (!this->bLoaded)
        return FALSE;
    return this->pHtmlViewerHelper->GetWebBrowser(this->hHtmlViewer);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
//             Following are only functions to allow threaded execution --> no interest            //
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CHtmlViewerWindow::ExecuteFunctionInHTMLViewerThread(pfThreadedFunction FunctionToBeCalled,LPVOID UserParam,DWORD* pFunctionResult)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.pFunction=(FARPROC)FunctionToBeCalled;
    Param.pVoid=UserParam;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_ExecuteFunctionInHTMLViewerThread,(LPARAM)&Param);
    *pFunctionResult=Param.Dword;
    return Param.bReturn;
}

BOOL CHtmlViewerWindow::LoadEmptyPageAndSetBodyContent(TCHAR* Content)
{
    ResetEvent(this->hWaitForPageCompletedFinished);
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.Str1=Content;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_LoadEmptyPageAndSetBodyContent,(LPARAM)&Param);
    return Param.bReturn;
}
BOOL CHtmlViewerWindow::AddHtmlContentToBody(TCHAR* Content)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.Str1=Content;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_AddHtmlContentToBody,(LPARAM)&Param);
    return Param.bReturn;
}

BOOL CHtmlViewerWindow::AddHtmlContentToElement(TCHAR* Id,TCHAR* Content)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.Str1=Id;
    Param.Str2=Content;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_AddHtmlContentToElement,(LPARAM)&Param);
    return Param.bReturn;
}
BOOL CHtmlViewerWindow::SetElementSrc(TCHAR* Id,TCHAR* Src)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.Str1=Id;
    Param.Str2=Src;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_SetElementSrc,(LPARAM)&Param);
    return Param.bReturn;
}
BOOL CHtmlViewerWindow::SetElementInnerHtml(TCHAR* Id,TCHAR* Html)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.Str1=Id;
    Param.Str2=Html;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_SetElementInnerHtml,(LPARAM)&Param);
    return Param.bReturn;
}

BOOL CHtmlViewerWindow::Navigate(TCHAR* Url)
{
    ResetEvent(this->hWaitForPageCompletedFinished);
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.Str1=Url;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_Navigate,(LPARAM)&Param);
    return Param.bReturn;
}

BOOL CHtmlViewerWindow::Save()
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_Save,(LPARAM)&Param);
    return Param.bReturn;
}
BOOL CHtmlViewerWindow::SaveAs(TCHAR* FileName)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.Str1=FileName;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_SaveAs,(LPARAM)&Param);
    return Param.bReturn;
}
BOOL CHtmlViewerWindow::ExecScript(TCHAR* ScriptContent,VARIANT* pScriptResult,EXCEPINFO* pExcepInfo)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.Str1=ScriptContent;
    Param.pVariant=pScriptResult;
    Param.pExcepInfo=pExcepInfo;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_ExecScript3,(LPARAM)&Param);
    return Param.bReturn;
}
BOOL CHtmlViewerWindow::ExecScript(TCHAR* ScriptContent,VARIANT* pScriptResult)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.Str1=ScriptContent;
    Param.pVariant=pScriptResult;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_ExecScript2,(LPARAM)&Param);
    return Param.bReturn;
}
BOOL CHtmlViewerWindow::ExecScript(TCHAR* ScriptContent)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.Str1=ScriptContent;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_ExecScript1,(LPARAM)&Param);
    return Param.bReturn;
}
BOOL CHtmlViewerWindow::SetElementsEventsCallBack(pfElementEventsCallBack ElementEventsCallBack,LPVOID UserParam)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.pFunction=(FARPROC)ElementEventsCallBack;
    Param.pVoid=UserParam;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_SetElementsEventsCallBack,(LPARAM)&Param);
    return Param.bReturn;
}
BOOL CHtmlViewerWindow::SetElementsEventsCallBackEx(pfElementEventsCallBackEx ElementEventsCallBackEx,LPVOID UserParam)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.pFunction=(FARPROC)ElementEventsCallBackEx;
    Param.pVoid=UserParam;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_SetElementsEventsCallBackEx,(LPARAM)&Param);
    return Param.bReturn;
}
BOOL CHtmlViewerWindow::EnableContextMenu(BOOL Enable)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.Bool=Enable;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_EnableContextMenu,(LPARAM)&Param);
    return Param.bReturn;
}
BOOL CHtmlViewerWindow::EnableSelection(BOOL Enable)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.Bool=Enable;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_EnableSelection,(LPARAM)&Param);
    return Param.bReturn;
}
IHTMLElement* CHtmlViewerWindow::GetHTMLElement(TCHAR* Id)
{
    PARAMETERS_STRUCT Param;
    Param.Str1=Id;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_GetHTMLElement,(LPARAM)&Param);
    return (IHTMLElement*)Param.pVoid;
}
IDispatch* CHtmlViewerWindow::GetHTMLDocument()
{
    PARAMETERS_STRUCT Param;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_GetHTMLDocument,(LPARAM)&Param);
    return (IDispatch*)Param.pVoid;
}
BOOL CHtmlViewerWindow::WaitForPageCompleted(HANDLE hEventCancel)
{
    PARAMETERS_STRUCT Param;
    Param.bReturn=FALSE;
    Param.Handle=hEventCancel;
    SendMessage(this->hWndDialog,WM_HTML_VIEWER_WINDOW_MESSAGE,UserMessageValue_WaitForPageCompleted,(LPARAM)&Param);
    return Param.bReturn;
}

void CHtmlViewerWindow::ProcessHTMLViewerThreadedMessage(tagUserMessageValues UserMessageValue,PARAMETERS_STRUCT* Parameters)
{
    switch(UserMessageValue)
    {
        case UserMessageValue_LoadEmptyPageAndSetBodyContent:
            Parameters->bReturn=this->ThreadedLoadEmptyPageAndSetBodyContent(Parameters->Str1);
            break;
        case UserMessageValue_AddHtmlContentToBody:
            Parameters->bReturn=this->ThreadedAddHtmlContentToBody(Parameters->Str1);
            break;
        case UserMessageValue_AddHtmlContentToElement:
            Parameters->bReturn=this->ThreadedAddHtmlContentToElement(Parameters->Str1,Parameters->Str2);
            break;
        case UserMessageValue_SetElementSrc:
            Parameters->bReturn=this->ThreadedSetElementSrc(Parameters->Str1,Parameters->Str2);
            break;
        case UserMessageValue_SetElementInnerHtml:
            Parameters->bReturn=this->ThreadedSetElementInnerHtml(Parameters->Str1,Parameters->Str2);
            break;
        case UserMessageValue_WaitForPageCompleted:
            Parameters->bReturn=this->ThreadedWaitForPageCompleted(Parameters->Handle);
            break;
        case UserMessageValue_Navigate:
            Parameters->bReturn=this->ThreadedNavigate(Parameters->Str1);
            break;
        case UserMessageValue_Save:
            Parameters->bReturn=this->ThreadedSave();
            break;
        case UserMessageValue_SaveAs:
            Parameters->bReturn=this->ThreadedSaveAs(Parameters->Str1);
            break;
        case UserMessageValue_ExecScript1:
            Parameters->bReturn=this->ThreadedExecScript(Parameters->Str1);
            break;
        case UserMessageValue_ExecScript2:
            Parameters->bReturn=this->ThreadedExecScript(Parameters->Str1,Parameters->pVariant);
            break;
        case UserMessageValue_ExecScript3:
            Parameters->bReturn=this->ThreadedExecScript(Parameters->Str1,Parameters->pVariant,Parameters->pExcepInfo);
            break;
        case UserMessageValue_SetElementsEventsCallBack:
            Parameters->bReturn=this->ThreadedSetElementsEventsCallBack((pfElementEventsCallBack)Parameters->pFunction,Parameters->pVoid);
            break;
        case UserMessageValue_SetElementsEventsCallBackEx:
            Parameters->bReturn=this->ThreadedSetElementsEventsCallBackEx((pfElementEventsCallBackEx)Parameters->pFunction,Parameters->pVoid);
            break;
        case UserMessageValue_EnableContextMenu:
            Parameters->bReturn=this->ThreadedEnableContextMenu(Parameters->Bool);
            break;
        case UserMessageValue_EnableSelection:
            Parameters->bReturn=this->ThreadedEnableSelection(Parameters->Bool);
            break;
        case UserMessageValue_GetHTMLElement:
            Parameters->pVoid=this->ThreadedGetHTMLElement(Parameters->Str1);
            break;
        case UserMessageValue_GetHTMLDocument:
            Parameters->pVoid=this->ThreadedGetHTMLDocument();
            break;
        case UserMessageValue_ExecuteFunctionInHTMLViewerThread:
            pfThreadedFunction ThreadedFunction=(pfThreadedFunction)Parameters->pFunction;
            Parameters->Dword=ThreadedFunction(Parameters->pVoid);
            break;
    }
}