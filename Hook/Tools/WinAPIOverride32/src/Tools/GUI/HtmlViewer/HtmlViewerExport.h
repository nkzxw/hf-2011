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
// Object: allow to display html content
//-----------------------------------------------------------------------------

// include for DLL import
#pragma once
#include <windows.h>
#include <mshtml.h>
#include <mshtmdid.h> // optional only to have full list of mshtml events define for the event callback
#include <Exdisp.h>

#define HTMLVIEWER_DLL_NAME _T("HtmlViewer.dll")
#define HTMLVIEWER_DONT_SAVE_BEGIN         _T("<!--DONT_SAVE_BEGIN-->")
#define HTMLVIEWER_DONT_SAVE_END           _T("<!--DONT_SAVE_END-->")

// exported func name
#define HTMLVIEWER_ADD_HTML_CONTENT_TO_ELEMENTA "_AddHtmlContentToElementA@12"
#define HTMLVIEWER_ADD_HTML_CONTENT_TO_ELEMENTW "_AddHtmlContentToElementW@12"
#define HTMLVIEWER_ADD_HTML_CONTENT_TO_BODYA "_AddHtmlContentToBodyA@8"
#define HTMLVIEWER_ADD_HTML_CONTENT_TO_BODYW "_AddHtmlContentToBodyW@8"
#define HTMLVIEWER_CREATE_HTMLVIEWER_FROM_HWND "_CreateHtmlViewerFromHwnd@4"
#define HTMLVIEWER_CREATE_HTMLVIEWER "_CreateHtmlViewer@24"
#define HTMLVIEWER_DESTROYHTMLVIEWER "_DestroyHtmlViewer@4"
#define HTMLVIEWER_LOAD_EMPTY_PAGE_AND_SET_BODY_CONTENTA "_LoadEmptyPageAndSetBodyContentA@8"
#define HTMLVIEWER_LOAD_EMPTY_PAGE_AND_SET_BODY_CONTENTW "_LoadEmptyPageAndSetBodyContentW@8"
#define HTMLVIEWER_NAVIGATEA "_NavigateA@8"
#define HTMLVIEWER_NAVIGATEW "_NavigateW@8"
#define HTMLVIEWER_SET_ELEMENT_INNERHTMLA "_SetElementInnerHtmlA@12"
#define HTMLVIEWER_SET_ELEMENT_INNERHTMLW "_SetElementInnerHtmlW@12"
#define HTMLVIEWER_SET_ELEMENT_SRCA "_SetElementSrcA@12"
#define HTMLVIEWER_SET_ELEMENT_SRCW "_SetElementSrcW@12"
#define HTMLVIEWER_WAIT_FOR_PAGE_COMPLETED "_WaitForPageCompleted@8"
#define HTMLVIEWER_SAVE "_Save@4"
#define HTMLVIEWER_SAVEASA "_SaveAsA@8"
#define HTMLVIEWER_SAVEASW "_SaveAsW@8"
#define HTMLVIEWER_EXECSCRIPTA "_ExecScriptA@8"
#define HTMLVIEWER_EXECSCRIPTW "_ExecScriptW@8"
#define HTMLVIEWER_EXECSCRIPTEXA "_ExecScriptExA@12"
#define HTMLVIEWER_EXECSCRIPTEXW "_ExecScriptExW@12"
#define HTMLVIEWER_EXECSCRIPTEX2A "_ExecScriptEx2A@16"
#define HTMLVIEWER_EXECSCRIPTEX2W "_ExecScriptEx2W@16"
#define HTMLVIEWER_SET_ELEMENTS_EVENTS_CALLBACK     "_SetElementsEventsCallBack@12"
#define HTMLVIEWER_SET_ELEMENTS_EVENTS_CALLBACKEX   "_SetElementsEventsCallBackEx@12"
#define HTMLVIEWER_ENABLE_CONTEXT_MENU              "_EnableContextMenu@8"
#define HTMLVIEWER_ENABLE_SELECTION                 "_EnableSelection@8"
#define HTMLVIEWER_GET_HTML_DOCUMENT                "_GetHTMLDocument@4"
#define HTMLVIEWER_GET_HTML_ELEMENTA                "_GetHTMLElementA@8"
#define HTMLVIEWER_GET_HTML_ELEMENTW                "_GetHTMLElementW@8"
#define HTMLVIEWER_TRANSLATE_ACCELERATOR            "_TranslateAcceleratorForWebBrowser@8"
#define HTMLVIEWER_GET_WEB_BROWSER              "_GetWebBrowser@4"

// callbacks
typedef void (__stdcall *pfElementEventsCallBack)(DISPID dispidMember,WCHAR* ElementId,LPVOID UserParam);
typedef void (__stdcall *pfElementEventsCallBackEx)(DISPID dispidMember,IHTMLEventObj* pEvtObj,LPVOID UserParam);

// exported function definitions
typedef HANDLE (__stdcall *pCreateHtmlViewerFromHwnd)(HWND hWnd);
typedef HANDLE (__stdcall *pCreateHtmlViewer)(HWND hWndParent,DWORD X, DWORD Y, DWORD Width, DWORD Height, OUT HWND* pHtmlViewerhWnd);
typedef BOOL (__stdcall *pDestroyHtmlViewer)(HANDLE hHtmlViewer);
typedef BOOL (__stdcall *pNavigateA)(HANDLE hHtmlViewer,CHAR* Url);
typedef BOOL (__stdcall *pNavigateW)(HANDLE hHtmlViewer,WCHAR* Url);
typedef BOOL (__stdcall *pWaitForPageCompleted)(HANDLE hHtmlViewer,HANDLE hEventCancel);
typedef BOOL (__stdcall *pSetElementInnerHtmlA)(HANDLE hHtmlViewer,CHAR* Id,CHAR* Html);
typedef BOOL (__stdcall *pSetElementInnerHtmlW)(HANDLE hHtmlViewer,WCHAR* Id,WCHAR* Html);
typedef BOOL (__stdcall *pSetElementSrcA)(HANDLE hHtmlViewer,CHAR* Id,CHAR* Src);
typedef BOOL (__stdcall *pSetElementSrcW)(HANDLE hHtmlViewer,WCHAR* Id,WCHAR* Src);
typedef BOOL (__stdcall *pAddHtmlContentToElementA)(HANDLE hHtmlViewer,CHAR* Id,CHAR* Content);
typedef BOOL (__stdcall *pAddHtmlContentToElementW)(HANDLE hHtmlViewer,WCHAR* Id,WCHAR* Content);
typedef BOOL (__stdcall *pAddHtmlContentToBodyA)(HANDLE hHtmlViewer,CHAR* Content);
typedef BOOL (__stdcall *pAddHtmlContentToBodyW)(HANDLE hHtmlViewer,WCHAR* Content);
typedef BOOL (__stdcall *pLoadEmptyPageAndSetBodyContentA)(HANDLE hHtmlViewer,CHAR* Content);
typedef BOOL (__stdcall *pLoadEmptyPageAndSetBodyContentW)(HANDLE hHtmlViewer,WCHAR* Content);
typedef BOOL (__stdcall *pSave)(HANDLE hHtmlViewer);
typedef BOOL (__stdcall *pSaveAsW)(HANDLE hHtmlViewer,WCHAR* FileName);
typedef BOOL (__stdcall *pSaveAsA)(HANDLE hHtmlViewer,CHAR* FileName);
typedef BOOL (__stdcall *pExecScriptW)(HANDLE hHtmlViewer,WCHAR* ScriptContent);
typedef BOOL (__stdcall *pExecScriptA)(HANDLE hHtmlViewer,CHAR* ScriptContent);
typedef BOOL (__stdcall *pExecScriptExW)(HANDLE hHtmlViewer,WCHAR* ScriptContent,VARIANT* pScriptResult);
typedef BOOL (__stdcall *pExecScriptExA)(HANDLE hHtmlViewer,CHAR* ScriptContent,VARIANT* pScriptResult);
typedef BOOL (__stdcall *pExecScriptEx2W)(HANDLE hHtmlViewer,WCHAR* ScriptContent,VARIANT* pScriptResult,EXCEPINFO* pExcepInfo);
typedef BOOL (__stdcall *pExecScriptEx2A)(HANDLE hHtmlViewer,CHAR* ScriptContent,VARIANT* pScriptResult,EXCEPINFO* pExcepInfo);
typedef BOOL (__stdcall *pSetElementsEventsCallBack)(HANDLE hHtmlViewer,pfElementEventsCallBack ElementEventsCallBack,LPVOID UserParam);
typedef BOOL (__stdcall *pSetElementsEventsCallBackEx)(HANDLE hHtmlViewer,pfElementEventsCallBackEx ElementEventsCallBackEx,LPVOID UserParam);
typedef BOOL (__stdcall *pEnableContextMenu)(HANDLE hHtmlViewer,BOOL Enable);
typedef BOOL (__stdcall *pEnableSelection)(HANDLE hHtmlViewer,BOOL Enable);
typedef IDispatch* (__stdcall *pGetHTMLDocument)(HANDLE hHtmlViewer);
typedef IHTMLElement* (__stdcall *pGetHTMLElementA)(HANDLE hHtmlViewer,CHAR* Id);
typedef IHTMLElement* (__stdcall *pGetHTMLElementW)(HANDLE hHtmlViewer,WCHAR* Id);
typedef BOOL (__stdcall *pTranslateAccelerator)(HANDLE hHtmlViewer,MSG* pMsg);
typedef IWebBrowser2* (__stdcall *pGetWebBrowser)(HANDLE hHtmlViewer);

#if (defined(UNICODE)||defined(_UNICODE))
#define pNavigate                        pNavigateW
#define pSaveAs                          pSaveAsW
#define pSetElementInnerHtml             pSetElementInnerHtmlW
#define pSetElementSrc                   pSetElementSrcW
#define pAddHtmlContentToElement         pAddHtmlContentToElementW
#define pAddHtmlContentToBody            pAddHtmlContentToBodyW
#define pLoadEmptyPageAndSetBodyContent  pLoadEmptyPageAndSetBodyContentW
#define pExecScript                      pExecScriptW
#define pExecScriptEx                    pExecScriptExW
#define pExecScriptEx2                   pExecScriptEx2W
#define pGetHTMLElement                  pGetHTMLElementW

#define HTMLVIEWER_ADD_HTML_CONTENT_TO_ELEMENT             HTMLVIEWER_ADD_HTML_CONTENT_TO_ELEMENTW
#define HTMLVIEWER_ADD_HTML_CONTENT_TO_BODY       HTMLVIEWER_ADD_HTML_CONTENT_TO_BODYW
#define HTMLVIEWER_LOAD_EMPTY_PAGE_AND_SET_BODY_CONTENT    HTMLVIEWER_LOAD_EMPTY_PAGE_AND_SET_BODY_CONTENTW
#define HTMLVIEWER_NAVIGATE                                HTMLVIEWER_NAVIGATEW
#define HTMLVIEWER_SET_ELEMENT_INNERHTML                   HTMLVIEWER_SET_ELEMENT_INNERHTMLW 
#define HTMLVIEWER_SET_ELEMENT_SRC                         HTMLVIEWER_SET_ELEMENT_SRCW
#define HTMLVIEWER_SAVEAS                                  HTMLVIEWER_SAVEASW
#define HTMLVIEWER_EXECSCRIPT                              HTMLVIEWER_EXECSCRIPTW 
#define HTMLVIEWER_EXECSCRIPTEX                            HTMLVIEWER_EXECSCRIPTEXW
#define HTMLVIEWER_EXECSCRIPTEX2                           HTMLVIEWER_EXECSCRIPTEX2W
#define HTMLVIEWER_GET_HTML_ELEMENT                        HTMLVIEWER_GET_HTML_ELEMENTW

#else
#define HTMLVIEWER_ADD_HTML_CONTENT_TO_ELEMENT             HTMLVIEWER_ADD_HTML_CONTENT_TO_ELEMENTA
#define HTMLVIEWER_ADD_HTML_CONTENT_TO_BODY       HTMLVIEWER_ADD_HTML_CONTENT_TO_BODYA
#define HTMLVIEWER_LOAD_EMPTY_PAGE_AND_SET_BODY_CONTENT    HTMLVIEWER_LOAD_EMPTY_PAGE_AND_SET_BODY_CONTENTA
#define HTMLVIEWER_NAVIGATE                                HTMLVIEWER_NAVIGATEA
#define HTMLVIEWER_SET_ELEMENT_INNERHTML                   HTMLVIEWER_SET_ELEMENT_INNERHTMLA
#define HTMLVIEWER_SET_ELEMENT_SRC                         HTMLVIEWER_SET_ELEMENT_SRCA
#define HTMLVIEWER_SAVEAS                                  HTMLVIEWER_SAVEASA
#define HTMLVIEWER_EXECSCRIPT                              HTMLVIEWER_EXECSCRIPTA 
#define HTMLVIEWER_EXECSCRIPTEX                            HTMLVIEWER_EXECSCRIPTEXA 
#define HTMLVIEWER_EXECSCRIPTEX2                           HTMLVIEWER_EXECSCRIPTEX2A 
#define HTMLVIEWER_GET_HTML_ELEMENT                        HTMLVIEWER_GET_HTML_ELEMENTA


#define pNavigate                        pNavigateA
#define pSaveAs                          pSaveAsA
#define pSetElementInnerHtml             pSetElementInnerHtmlA
#define pSetElementSrc                   pSetElementSrcA
#define pAddHtmlContentToElement         pAddHtmlContentToElementA
#define pAddHtmlContentToBody            pAddHtmlContentToBodyA
#define pLoadEmptyPageAndSetBodyContent  pLoadEmptyPageAndSetBodyContentA
#define pExecScript                      pExecScriptA
#define pExecScriptEx                    pExecScriptExA
#define pExecScriptEx2                   pExecScriptEx2A
#define pGetHTMLElement                  pGetHTMLElementA
#endif
    
//// CHtmlViewerHelper Sample of use
// 
//    // WARNING ALL CALLS MUST BE DONE IN THE SAME THREAD (the WndProc of the window container)
//
//    HWND hWndDialog=NULL;
//    pHtmlViewerHelper=new CHtmlViewerHelper();
//    pHtmlViewerHelper->Load();
//    RECT rect;
//    GetClientRect(hWndDialog,&rect);
//    hHtmlViewer=pHtmlViewerHelper->CreateHtmlViewer(hWndDialog,0,0,rect.right,rect.bottom,&HtmlViewerhWnd);
//    pHtmlViewerHelper->SetElementsEventsCallBack(hHtmlViewer,ElementEventsCallBack,NULL);
//
//    // 1) to load a resource page
//    TCHAR ModuleName[MAX_PATH];
//    GetModuleFileName(NULL, ModuleName, MAX_PATH);
//    TCHAR URL[2*MAX_PATH];
//    _stprintf(URL,_T("res://%s/%d/%d"), ModuleName,RT_HTML, IDR_HTML1);// IDR_HTML1 your own resource html page ID
//    pHtmlViewerHelper->Navigate(hHtmlViewer,URL);
//    pHtmlViewerHelper->WaitForPageCompleted(hHtmlViewer,NULL);
//
//    // 2) to directly generate code without resource page
//    //pHtmlViewerHelper->LoadEmptyPageAndSetBodyContent(hHtmlViewer,_T("Text content <a href=\"foo.htm\">foo</a>"));

// Saving Notes
//  - supported protocol for saving: res://, file://, (files extracted/copied to "SavingName_files" directory)
//  - content between html comments <!--DONT_SAVE_BEGIN--> and <!--DONT_SAVE_END--> won't be saved


// TranslateAccelerator Notes
// to allow ctr+c, ctr + xxx to work under current web browser presentation
//       MUST BE USED LIKE THE FOLLOWING from your window message loop
//        //translate messages until window closing
//        BOOL bRet;
//        while( (bRet = GetMessage( &Msg, NULL, 0, 0 )) != 0)
//        { 
//            if (bRet == -1)
//            {
//                // handle the error and possibly exit
//            }
//            else
//            {
//                // MSDN : calling IOleInPlaceActiveObject::TranslateAccelerator from your container's message loop before doing any other translation
//                if (Msg.message==WM_KEYDOWN)
//                { 
//                    // MSDN : You should apply your own translation only when this method returns S_FALSE
//                    if (pHtmlViewerHelper->TranslateAccelerator(&Msg))
//                        continue;
//                }
//                TranslateMessage(&Msg); 
//                DispatchMessage(&Msg); 
//            }
//        }
//     Parameter : MSG* pMsg : pointer to the message being processing
//     return : TRUE if message has been processed by TranslateAccelerator and you shouldn't apply your own translation
//              FALSE if message hasn't been processed and you should apply your own translation 
class CHtmlViewerHelper
{
private:
    BOOL Loaded;
    HMODULE hModule;
public:
    pCreateHtmlViewerFromHwnd CreateHtmlViewerFromHwnd;
    pCreateHtmlViewer CreateHtmlViewer;
    pDestroyHtmlViewer DestroyHtmlViewer;
    pNavigate Navigate;
    pWaitForPageCompleted WaitForPageCompleted;
    pSetElementInnerHtml SetElementInnerHtml;
    pSetElementSrc SetElementSrc;
    pAddHtmlContentToElement AddHtmlContentToElement;
    pAddHtmlContentToBody AddHtmlContentToBody;
    pLoadEmptyPageAndSetBodyContent LoadEmptyPageAndSetBodyContent;
    pSave Save;
    pSaveAs SaveAs;
    pExecScript ExecScript;
    pExecScriptEx ExecScriptEx;
    pExecScriptEx2 ExecScriptEx2;
    pSetElementsEventsCallBack SetElementsEventsCallBack;
    pSetElementsEventsCallBackEx SetElementsEventsCallBackEx;
    pEnableContextMenu EnableContextMenu;
    pEnableSelection  EnableSelection;
    pGetHTMLElement GetHTMLElement;
    pGetHTMLDocument GetHTMLDocument;
    pTranslateAccelerator TranslateAccelerator;
    pGetWebBrowser GetWebBrowser;

    CHtmlViewerHelper()
    {
        this->Loaded=FALSE;
        this->hModule=NULL;
        this->CreateHtmlViewerFromHwnd=NULL;
        this->CreateHtmlViewer=NULL;
        this->DestroyHtmlViewer=NULL;
        this->Navigate=NULL;
        this->WaitForPageCompleted=NULL;
        this->SetElementInnerHtml=NULL;
        this->SetElementSrc=NULL;
        this->AddHtmlContentToElement=NULL;
        this->AddHtmlContentToBody=NULL;
        this->LoadEmptyPageAndSetBodyContent=NULL;
        this->Save=NULL;
        this->SaveAs=NULL;
        this->ExecScript=NULL; 
        this->ExecScriptEx =NULL; 
        this->ExecScriptEx2=NULL;
        this->SetElementsEventsCallBack  =NULL;
        this->SetElementsEventsCallBackEx=NULL;
        this->EnableContextMenu =NULL;
        this->EnableSelection =NULL;
        this->GetHTMLElement=NULL;
        this->GetHTMLDocument=NULL;
        this->TranslateAccelerator=NULL;
        this->GetWebBrowser=NULL;
    }
    ~CHtmlViewerHelper()
    {
        if (this->Loaded)
            this->Unload();
    }

    BOOL Load()
    {
        return this->Load(_T(""));
    }

    BOOL Load(TCHAR* HtmlViewerDllPath)
    {
        if (this->Loaded)
            return TRUE;
        if (!HtmlViewerDllPath)
            return FALSE;

        TCHAR Path[MAX_PATH];
        _tcscpy(Path,HtmlViewerDllPath);
        _tcscat(Path,HTMLVIEWER_DLL_NAME);
        this->hModule=LoadLibrary(Path);
        if (!this->hModule)
            return FALSE;

        this->CreateHtmlViewerFromHwnd=(pCreateHtmlViewerFromHwnd)GetProcAddress(this->hModule,HTMLVIEWER_CREATE_HTMLVIEWER_FROM_HWND);
        this->CreateHtmlViewer=(pCreateHtmlViewer)GetProcAddress(this->hModule,HTMLVIEWER_CREATE_HTMLVIEWER);
        this->DestroyHtmlViewer=(pDestroyHtmlViewer)GetProcAddress(this->hModule,HTMLVIEWER_DESTROYHTMLVIEWER);
        this->Navigate=(pNavigate)GetProcAddress(this->hModule,HTMLVIEWER_NAVIGATE);
        this->WaitForPageCompleted=(pWaitForPageCompleted)GetProcAddress(this->hModule,HTMLVIEWER_WAIT_FOR_PAGE_COMPLETED);
        this->SetElementInnerHtml=(pSetElementInnerHtml)GetProcAddress(this->hModule,HTMLVIEWER_SET_ELEMENT_INNERHTML);
        this->SetElementSrc=(pSetElementSrc)GetProcAddress(this->hModule,HTMLVIEWER_SET_ELEMENT_SRC);
        this->AddHtmlContentToElement=(pAddHtmlContentToElement)GetProcAddress(this->hModule,HTMLVIEWER_ADD_HTML_CONTENT_TO_ELEMENT);
        this->AddHtmlContentToBody=(pAddHtmlContentToBody)GetProcAddress(this->hModule,HTMLVIEWER_ADD_HTML_CONTENT_TO_BODY);
        this->LoadEmptyPageAndSetBodyContent=(pLoadEmptyPageAndSetBodyContent)GetProcAddress(this->hModule,HTMLVIEWER_LOAD_EMPTY_PAGE_AND_SET_BODY_CONTENT);
        this->Save=(pSave)GetProcAddress(this->hModule,HTMLVIEWER_SAVE);
        this->SaveAs=(pSaveAs)GetProcAddress(this->hModule,HTMLVIEWER_SAVEAS);
        this->ExecScript=(pExecScript)GetProcAddress(this->hModule,HTMLVIEWER_EXECSCRIPT);
        this->ExecScriptEx=(pExecScriptEx)GetProcAddress(this->hModule,HTMLVIEWER_EXECSCRIPTEX);
        this->ExecScriptEx2=(pExecScriptEx2)GetProcAddress(this->hModule,HTMLVIEWER_EXECSCRIPTEX2);
        this->SetElementsEventsCallBack=(pSetElementsEventsCallBack)GetProcAddress(this->hModule,HTMLVIEWER_SET_ELEMENTS_EVENTS_CALLBACK);
        this->SetElementsEventsCallBackEx=(pSetElementsEventsCallBackEx)GetProcAddress(this->hModule,HTMLVIEWER_SET_ELEMENTS_EVENTS_CALLBACKEX);
        this->EnableContextMenu=(pEnableContextMenu)GetProcAddress(this->hModule,HTMLVIEWER_ENABLE_CONTEXT_MENU);
        this->EnableSelection=(pEnableSelection)GetProcAddress(this->hModule,HTMLVIEWER_ENABLE_SELECTION);
        this->GetHTMLElement=(pGetHTMLElement)GetProcAddress(this->hModule,HTMLVIEWER_GET_HTML_ELEMENT);
        this->GetHTMLDocument=(pGetHTMLDocument)GetProcAddress(this->hModule,HTMLVIEWER_GET_HTML_DOCUMENT);
        this->TranslateAccelerator=(pTranslateAccelerator)GetProcAddress(this->hModule,HTMLVIEWER_TRANSLATE_ACCELERATOR);
        this->GetWebBrowser=(pGetWebBrowser)GetProcAddress(this->hModule,HTMLVIEWER_GET_WEB_BROWSER);

        // should be set here to allow call to Unload
        this->Loaded=TRUE;

        if (
                (this->CreateHtmlViewerFromHwnd==NULL)
            ||  (this->CreateHtmlViewer==NULL)
            ||  (this->DestroyHtmlViewer==NULL)
            ||  (this->Navigate==NULL)
            ||  (this->WaitForPageCompleted==NULL)
            ||  (this->SetElementInnerHtml==NULL)
            ||  (this->SetElementSrc==NULL)
            ||  (this->AddHtmlContentToElement==NULL)
            ||  (this->AddHtmlContentToBody==NULL)
            ||  (this->LoadEmptyPageAndSetBodyContent==NULL)
            ||  (this->Save==NULL)
            ||  (this->SaveAs==NULL)
            ||  (this->ExecScript==NULL)
            ||  (this->ExecScriptEx==NULL)
            ||  (this->ExecScriptEx2==NULL)
            ||  (this->SetElementsEventsCallBack==NULL)
            ||  (this->SetElementsEventsCallBackEx==NULL)
            ||  (this->EnableContextMenu==NULL)
            ||  (this->EnableSelection==NULL)
            ||  (this->GetHTMLElement==NULL)
            ||  (this->GetHTMLDocument==NULL)
            ||  (this->TranslateAccelerator==NULL)
            ||  (this->GetWebBrowser==NULL)
            )
        {
            this->Unload();
            return FALSE;
        }

        return TRUE;
    }
    BOOL Unload()
    {
        if (!this->Loaded)
            return TRUE;

        return FreeLibrary(this->hModule);
    }
};