
/*
Sample of use

CHtmlViewerWindow* pHtmlViewerWindow;
pHtmlViewerWindow= new CHtmlViewerWindow(400,400);

pHtmlViewerWindow->Show(mhInstance,mhWnd,_T("My Title"));

TCHAR ModuleName[MAX_PATH];
GetModuleFileName(NULL, ModuleName, MAX_PATH);
TCHAR URL[2*MAX_PATH];
_stprintf(URL,_T("res://%s/%d/%d"), ModuleName,RT_HTML, IDR_HTML1);

//pHtmlViewerWindow->LoadEmptyPageAndSetBodyContent(_T("Text content <a href=\"foo.htm\">foo</a>"));
pHtmlViewerWindow->Navigate(URL);
pHtmlViewerWindow->WaitForPageCompleted(NULL);
pHtmlViewerWindow->SetElementsEventsCallBack(ElementEventsCallBack,NULL);
pHtmlViewerWindow->SetElementsEventsCallBackEx(ElementEventsCallBackEx,NULL);

pHtmlViewerWindow->WaitWindowClose();
delete pHtmlViewerWindow;


// Saving Notes
// supported protocol for saving: res://, file://, (files extracted/copied to SavingName_files directory)
// html content between html tags <!--DONT_SAVE_BEGIN--> and <!--DONT_SAVE_END--> won't be saved

*/

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "HtmlViewerExport.h"

#define CHtmlViewerWindowClassName _T("CHtmlViewerWindowClassName")
#define CHtmlViewerWindow_OLE_DLL_NAME _T("Ole32.dll")
/*
MSDN :
The following are the ranges of message numbers
Range Meaning 
0 through WM_USER–1     Messages reserved for use by the system. 
WM_USER through 0x7FFF  Integer messages for use by private window classes. 
WM_APP through 0xBFFF   Messages available for use by applications. 
0xC000 through 0xFFFF   String messages for use by applications. 
Greater than 0xFFFF     Reserved by the system for future use. 
*/
// Message used to call methods in the same thread of the control owner
#define WM_HTML_VIEWER_WINDOW_MESSAGE WM_USER

class CHtmlViewerWindow
{
private:
    enum tagUserMessageValues
    {
        UserMessageValue_LoadEmptyPageAndSetBodyContent,
        UserMessageValue_AddHtmlContentToBody,
        UserMessageValue_AddHtmlContentToElement,
        UserMessageValue_SetElementSrc,
        UserMessageValue_SetElementInnerHtml,
        UserMessageValue_WaitForPageCompleted,
        UserMessageValue_Navigate,
        UserMessageValue_Save,
        UserMessageValue_SaveAs,
        UserMessageValue_ExecScript1,
        UserMessageValue_ExecScript2,
        UserMessageValue_ExecScript3,
        UserMessageValue_SetElementsEventsCallBack,
        UserMessageValue_SetElementsEventsCallBackEx,
        UserMessageValue_EnableContextMenu,
        UserMessageValue_EnableSelection,
        UserMessageValue_GetHTMLElement,
        UserMessageValue_GetHTMLDocument,
        UserMessageValue_ExecuteFunctionInHTMLViewerThread,
    };
    typedef struct tagParametersStruct
    {
        TCHAR* Str1;
        TCHAR* Str2;
        HANDLE Handle;
        VARIANT* pVariant;
        EXCEPINFO* pExcepInfo;
        FARPROC pFunction;
        PVOID pVoid;
        BOOL Bool;
        BOOL bReturn;
        DWORD Dword;
    }PARAMETERS_STRUCT,*PPARAMETERS_STRUCT;
    typedef HRESULT (__stdcall *pfOleInitialize) (LPVOID pvReserved);

    HINSTANCE hInstance;
    HWND hParentHandle; // parent window handle
    HWND hWndDialog;    // current dialog handle
    HANDLE hHtmlViewer; // pointer to web page object
    HWND HtmlViewerhWnd;// handle of web page window
    CHtmlViewerHelper* pHtmlViewerHelper;
    HANDLE hWindowClose;
    HANDLE hWindowInit;
    TCHAR* pszWindowTitle;
    HICON hIcon;
    int x;
    int y;
    DWORD Width;
    DWORD Height;
    BOOL CenterWindow;
    HANDLE hWaitForPageCompletedInternalCancelEvent;
    HANDLE hWaitForPageCompletedCancelEvent;
    HANDLE hWaitForPageCompletedFinished;
    HANDLE hWindowClosing;
    BOOL bLoaded;
    TCHAR szHtmlViewerDllPath[MAX_PATH];

    void OnInit();
    void OnClose();
    void OnSize();
    static DWORD WINAPI WaitForPageCompletedCancelWatcherStatic(PVOID lParam);
    DWORD WaitForPageCompletedCancelWatcher();
    static DWORD WINAPI ModelessDialogThread(PVOID lParam);
    static CHtmlViewerWindow* GetAssociatedObject(HWND hWndDialog);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void CommonConstructor();

    BOOL ThreadedLoadEmptyPageAndSetBodyContent(TCHAR* Content);
    BOOL ThreadedAddHtmlContentToBody(TCHAR* Content);
    BOOL ThreadedAddHtmlContentToElement(TCHAR* Id,TCHAR* Content);
    BOOL ThreadedSetElementSrc(TCHAR* Id,TCHAR* Src);
    BOOL ThreadedSetElementInnerHtml(TCHAR* Id,TCHAR* Html);
    BOOL ThreadedWaitForPageCompleted(HANDLE hEventCancel);
    BOOL ThreadedNavigate(TCHAR* Url);
    BOOL ThreadedSave();
    BOOL ThreadedSaveAs(TCHAR* FileName);
    BOOL ThreadedExecScript(TCHAR* ScriptContent,VARIANT* pScriptResult,EXCEPINFO* pExcepInfo);
    BOOL ThreadedExecScript(TCHAR* ScriptContent,VARIANT* pScriptResult);
    BOOL ThreadedExecScript(TCHAR* ScriptContent);
    BOOL ThreadedSetElementsEventsCallBack(pfElementEventsCallBack ElementEventsCallBack,LPVOID UserParam);
    BOOL ThreadedSetElementsEventsCallBackEx(pfElementEventsCallBackEx ElementEventsCallBackEx,LPVOID UserParam);
    BOOL ThreadedEnableContextMenu(BOOL Enable);
    BOOL ThreadedEnableSelection(BOOL Enable);
    IHTMLElement* ThreadedGetHTMLElement(TCHAR* Id);
    IDispatch* ThreadedGetHTMLDocument();
    
public:

    typedef DWORD (*pfThreadedFunction)(LPVOID UserParam);

    CHtmlViewerWindow(DWORD Width,DWORD Height);
    CHtmlViewerWindow(DWORD x,DWORD y,DWORD Width,DWORD Height);
    CHtmlViewerWindow(DWORD Width,DWORD Height,TCHAR* HtmlViewerDllPath);
    CHtmlViewerWindow(DWORD x,DWORD y,DWORD Width,DWORD Height,TCHAR* HtmlViewerDllPath);
    ~CHtmlViewerWindow(void);
    BOOL Show(HINSTANCE hInstance,HWND hParent,TCHAR* pszWindowTitle);
    BOOL Show(HINSTANCE hInstance,HWND hParent,TCHAR* pszWindowTitle,HMODULE hModule, DWORD IDIcon);
    BOOL Show(HINSTANCE hInstance,HWND hParent,TCHAR* pszWindowTitle,HICON hIcon);
    void WaitWindowClose();
    HWND GetControlHandle();

    BOOL LoadEmptyPageAndSetBodyContent(TCHAR* Content);
    BOOL AddHtmlContentToBody(TCHAR* Content);
    BOOL AddHtmlContentToElement(TCHAR* Id,TCHAR* Content);
    BOOL SetElementSrc(TCHAR* Id,TCHAR* Src);
    BOOL SetElementInnerHtml(TCHAR* Id,TCHAR* Html);
    BOOL WaitForPageCompleted(HANDLE hEventCancel);
    BOOL Navigate(TCHAR* Url);
    BOOL Save();
    BOOL SaveAs(TCHAR* FileName);
    BOOL ExecScript(TCHAR* ScriptContent,VARIANT* pScriptResult,EXCEPINFO* pExcepInfo);
    BOOL ExecScript(TCHAR* ScriptContent,VARIANT* pScriptResult);
    BOOL ExecScript(TCHAR* ScriptContent);
    BOOL SetElementsEventsCallBack(pfElementEventsCallBack ElementEventsCallBack,LPVOID UserParam);
    BOOL SetElementsEventsCallBackEx(pfElementEventsCallBackEx ElementEventsCallBackEx,LPVOID UserParam);
    BOOL EnableContextMenu(BOOL Enable);
    BOOL EnableSelection(BOOL Enable);
    IHTMLElement* GetHTMLElement(TCHAR* Id);
    IDispatch* GetHTMLDocument();
    IWebBrowser2* GetWebBrowser();

    // Very important: allow to call functions in the same thread of the WebBrowser ActiveX
    // By the way access to IHTMLElement, IDispatch or IWebBrowser2 returned by GetHTMLElement, GetHTMLDocument or GetWebBrowser
    // must be done though the same thread so YOU HAVE TO USE THIS FUNCTION
    BOOL ExecuteFunctionInHTMLViewerThread(pfThreadedFunction FunctionToBeCalled,LPVOID UserParam,DWORD* pFunctionResult);

private:
    BOOL bInitializationError;
    void ProcessHTMLViewerThreadedMessage(tagUserMessageValues UserMessageValue,PARAMETERS_STRUCT* Parameters);
};
