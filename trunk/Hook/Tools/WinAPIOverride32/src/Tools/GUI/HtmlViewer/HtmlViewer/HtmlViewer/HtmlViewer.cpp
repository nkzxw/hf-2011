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

#include "HtmlViewer.h"

void CHtmlViewer::DoEvents()
{
    MSG Message;
    if (::PeekMessage(&Message,NULL,0,0,PM_REMOVE))
    {
        ::TranslateMessage(&Message);
        ::DispatchMessage(&Message);
    }
}

void CHtmlViewer::Initialize(HMODULE hModule)
{
    HRESULT hResult;
    this->EmptyPageLoaded=FALSE;
    this->pIWebBrowser=NULL;

    this->pIOleInPlaceActiveObject=NULL;
    this->hModule=hModule;
    this->pDocumentElementEvents=NULL;
    this->pBrowserEvents=NULL;
    this->AtlAxAttachControl=NULL;
    this->HelperScriptSuccessFullyWritten=FALSE;

    this->ElementEventsCallBack=NULL;
    this->ElementEventsCallBackUserParam=NULL;
    this->ElementEventsCallBackEx=NULL;
    this->ElementEventsCallBackExUserParam=NULL;
    this->bEnableContextMenu=FALSE;
    this->bEnableSelection=FALSE;
    this->hEvtNavigateError=CreateEvent(0,TRUE,FALSE,0);// manual reset event initial state not signaled
    this->hEvtDocumentFullyCompleted=CreateEvent(0,TRUE,FALSE,0);// manual reset event initial state not signaled

    HMODULE hModuleATL=GetModuleHandle(_T("atl.dll"));
    if (!hModuleATL)
    {
        hModuleATL = LoadLibrary(_T("atl.dll")); // don't try to free library, else calling process can crash
        if (hModuleATL==NULL)
        {
            MessageBox(NULL,_T("Error: Can't load atl.dll"),
                _T("Error"),MB_ICONERROR|MB_OK);
            return;
        }
    }

    this->AtlAxAttachControl = (pfAtlAxAttachControl) GetProcAddress(hModuleATL, "AtlAxAttachControl");

    // Create WebBrowser instance and get IWebBrowser2 interface
    OleInitialize(0);
    hResult=CoCreateInstance(CLSID_WebBrowser,0,CLSCTX_ALL,IID_IWebBrowser2,(void**)&this->pIWebBrowser);
    if (FAILED(hResult) || (this->pIWebBrowser==NULL))
    {
        MessageBox(NULL,_T("Error accessing IWebBrowser2 COM object, HtmlViewer can't be launched"),
                    _T("Error"),MB_ICONERROR|MB_OK);
        return;
    }

    hResult=this->pIWebBrowser->QueryInterface (IID_IOleInPlaceActiveObject, (void**)&this->pIOleInPlaceActiveObject);
    if (FAILED(hResult) || (this->pIOleInPlaceActiveObject==NULL))
    {
        MessageBox(NULL,_T("Error accessing IOleInPlaceActiveObject for web WebBrowser object"),
            _T("Error"),MB_ICONERROR|MB_OK);
        return;
    }
    this->pBrowserEvents=new CBrowserEvents(this->pIWebBrowser);
    this->pBrowserEvents->SetCallBack(CHtmlViewer::WebBrowserEventCallBackStatic,this);
    this->pBrowserEvents->ConnectEvents();
}

CHtmlViewer::CHtmlViewer(HMODULE hModule,HWND hWnd)
{
    this->Initialize(hModule);
    if (!this->pIWebBrowser)
        return;
    this->hWnd=hWnd;

    // Attach WebBrowser to container:
    this->AtlAxAttachControl(this->pIWebBrowser,hWnd,0);
}

CHtmlViewer::CHtmlViewer(HMODULE hModule,HWND hWndParent,DWORD X, DWORD Y, DWORD Width, DWORD Height, OUT HWND* pHtmlViewerhWnd)
{
    this->Initialize(hModule);
    if (!this->pIWebBrowser)
        return;

    HWND hActiveXContainer;
    // Create ActiveX container:
    hActiveXContainer=CreateWindowEx(WS_EX_CLIENTEDGE,_T("STATIC"),_T(""),WS_CHILD | WS_VISIBLE,
                                    X,
                                    Y,
                                    Width,
                                    Height,
                                    hWndParent,
                                    0,hModule,0);

    this->hWnd=hActiveXContainer;

    // Attach WebBrowser on our container:
    this->AtlAxAttachControl(pIWebBrowser,hActiveXContainer,0);

    // fill pHtmlViewerhWnd output parameter
    if (!IsBadWritePtr(pHtmlViewerhWnd,sizeof(HWND)))
        *pHtmlViewerhWnd=hActiveXContainer;
}

CHtmlViewer::~CHtmlViewer(void)
{
    if (this->pDocumentElementEvents)
        delete this->pDocumentElementEvents;

    if (this->pBrowserEvents)
        delete this->pBrowserEvents;

    // force WaitForPageCompleted to return
    SetEvent(this->hEvtNavigateError);
    CloseHandle(this->hEvtNavigateError);
    CloseHandle(this->hEvtDocumentFullyCompleted);

    if (this->pIOleInPlaceActiveObject)
        this->pIOleInPlaceActiveObject->Release();
    if (this->pIWebBrowser)
        this->pIWebBrowser->Release();
}

IWebBrowser2* CHtmlViewer::GetWebBrowser()
{
    return this->pIWebBrowser;
}

//-----------------------------------------------------------------------------
// Name: TranslateAccelerator
// Object: allow ctr+c, ctr + xxx to work under current web browser presentation
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
//                    if (pHtmlViewer->TranslateAccelerator(&Msg))
//                        continue;
//                }
//                TranslateMessage(&Msg); 
//                DispatchMessage(&Msg); 
//            }
//        }
// Parameters :
//     in  : MSG* pMsg : pointer to the message being processing
//     out : 
//     return : TRUE if message has been processed by TranslateAccelerator and you shouldn't apply your own translation
//              FALSE if message hasn't been processed and you should apply your own translation 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::TranslateAccelerator(MSG* pMsg)
{
    if (!this->pIOleInPlaceActiveObject)
        return FALSE;

    return SUCCEEDED(this->pIOleInPlaceActiveObject->TranslateAccelerator(pMsg));
}

//-----------------------------------------------------------------------------
// Name: LoadEmptyPageAndSetBodyContent
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::LoadEmptyPageAndSetBodyContent(TCHAR* Content)
{
    if (this->pIWebBrowser==NULL)
        return FALSE;

    TCHAR ModuleName[MAX_PATH];
    GetModuleFileName(this->hModule, ModuleName, MAX_PATH);
    TCHAR Url[2*MAX_PATH];
    _stprintf(Url,_T("res://%s/%d"), ModuleName, IDR_HTML_EMPTY_PAGE);

    if(!this->Navigate(Url))
        return FALSE;

    this->EmptyPageLoaded=TRUE;

    if(!this->WaitForPageCompleted(NULL))
        return FALSE;

    return this->AddHtmlContentToBody(Content);


}

//-----------------------------------------------------------------------------
// Name: AddHtmlContentToBody
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::AddHtmlContentToBody(TCHAR* Content)
{
    HRESULT hr;
    BOOL bRet;
    IDispatch* pHtmlDocDispatch = NULL;
    IHTMLDocument2* pHtmlDoc = NULL;
    IHTMLElement* pElem = NULL;

    if (this->pIWebBrowser==NULL)
        return FALSE;

    if (!this->IsPageCompleted())
        return FALSE;

    // get IDispatch document interface
    hr = pIWebBrowser->get_Document(&pHtmlDocDispatch);
    if (FAILED (hr) || (pHtmlDocDispatch == NULL))
        return FALSE;

    // get IHTMLDocument2 document interface
    hr = pHtmlDocDispatch->QueryInterface(IID_IHTMLDocument2,(void**)&pHtmlDoc);
    if (FAILED (hr) || (pHtmlDoc == NULL))
    {
        pHtmlDocDispatch->Release();
        return FALSE;
    }

    // get body
    hr = pHtmlDoc->get_body(&pElem);
    if (FAILED (hr) || (pElem == NULL))
    {
        pHtmlDoc->Release();
        pHtmlDocDispatch->Release();
        return FALSE;
    }

    bRet=this->AddHtmlContentToElement(pElem,Content);

    pElem->Release();
    pHtmlDoc->Release();
    pHtmlDocDispatch->Release();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: AddHtmlContentToElement
// Object: 
// Parameters :
//     in  : TCHAR* Content : must be full html content
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::AddHtmlContentToElement(IHTMLElement* pElem,TCHAR* Content)
{
    if (!pElem)
        return FALSE;

    if (!this->IsPageCompleted())
        return FALSE;

    HRESULT hr;
#if ((!defined(UNICODE))&& (!defined(_UNICODE)))
    WCHAR* psz;
#endif

    BSTR bsWhere=SysAllocString(L"beforeEnd");
    BSTR bstrContent;
#if (defined(UNICODE)||defined(_UNICODE))
    bstrContent=SysAllocString(Content);
#else
    CAnsiUnicodeConvert::AnsiToUnicode(Content,&psz);
    bstrContent=SysAllocString(psz);
    free(psz);    
#endif

    hr=pElem->insertAdjacentHTML(bsWhere,bstrContent);
    SysFreeString(bstrContent);
    SysFreeString(bsWhere);

    return SUCCEEDED(hr);
}


//-----------------------------------------------------------------------------
// Name: GetHTMLElement
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
IHTMLElement* CHtmlViewer::GetHTMLElement(TCHAR* Id)
{
    if (this->pIWebBrowser==NULL)
        return NULL;

    if (!this->IsPageCompleted())
        return NULL;

    HRESULT hr;
    IDispatch* pHtmlDocDispatch = NULL;
    IHTMLDocument3* pHtmlDoc = NULL;
    IHTMLElement* pElem = NULL;
    BSTR bstrElementId;

#if ((!defined(UNICODE))&& (!defined(_UNICODE)))
    WCHAR* psz;
#endif

    // get IDispatch document interface
    hr = pIWebBrowser->get_Document(&pHtmlDocDispatch);
    if (FAILED (hr) || (pHtmlDocDispatch == NULL))
        return NULL;

    // get IHTMLDocument3 document interface
    hr = pHtmlDocDispatch->QueryInterface(IID_IHTMLDocument3,(void**)&pHtmlDoc);
    if (FAILED (hr) || (pHtmlDoc == NULL))
    {
        pHtmlDocDispatch->Release();
        return NULL;
    }

    // get pointer to element from it's Id
#if (defined(UNICODE)||defined(_UNICODE))
    bstrElementId=SysAllocString(Id);
#else
    CAnsiUnicodeConvert::AnsiToUnicode(Id,&psz);
    bstrElementId=SysAllocString(psz);
    free(psz);    
#endif

    hr = pHtmlDoc->getElementById(bstrElementId,&pElem);
    SysFreeString(bstrElementId);

    pHtmlDoc->Release();
    pHtmlDocDispatch->Release();

    return pElem;
}

//-----------------------------------------------------------------------------
// Name: GetHTMLDocument
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
IDispatch* CHtmlViewer::GetHTMLDocument()
{
    if (this->pIWebBrowser==NULL)
        return NULL;

    if (!this->IsPageCompleted())
        return NULL;

    IDispatch* pHtmlDocDispatch = NULL;

    // get IDispatch document interface
    pIWebBrowser->get_Document(&pHtmlDocDispatch);

    return pHtmlDocDispatch;
}

//-----------------------------------------------------------------------------
// Name: AddHtmlContentToElement
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::AddHtmlContentToElement(TCHAR* Id,TCHAR* Content)
{
    if (this->pIWebBrowser==NULL)
        return FALSE;

    if (!this->IsPageCompleted())
        return FALSE;

    HRESULT hr;
    BOOL bRet;
    IDispatch* pHtmlDocDispatch = NULL;
    IHTMLDocument3* pHtmlDoc = NULL;
    IHTMLElement* pElem = NULL;
    BSTR bstrElementId;

#if ((!defined(UNICODE))&& (!defined(_UNICODE)))
    WCHAR* psz;
#endif

    // get IDispatch document interface
    hr = pIWebBrowser->get_Document(&pHtmlDocDispatch);
    if (FAILED (hr) || (pHtmlDocDispatch == NULL))
        return FALSE;

    // get IHTMLDocument3 document interface
    hr = pHtmlDocDispatch->QueryInterface(IID_IHTMLDocument3,(void**)&pHtmlDoc);
    if (FAILED (hr) || (pHtmlDoc == NULL))
    {
        pHtmlDocDispatch->Release();
        return FALSE;
    }

    // get pointer to element from it's Id
#if (defined(UNICODE)||defined(_UNICODE))
    bstrElementId=SysAllocString(Id);
#else
    CAnsiUnicodeConvert::AnsiToUnicode(Id,&psz);
    bstrElementId=SysAllocString(psz);
    free(psz);    
#endif

    hr = pHtmlDoc->getElementById(bstrElementId,&pElem);
    SysFreeString(bstrElementId);
    if (FAILED (hr) || (pElem == NULL))
    {
        pHtmlDoc->Release();
        pHtmlDocDispatch->Release();
        return FALSE;
    }

    bRet=this->AddHtmlContentToElement(pElem,Content);

    pElem->Release();
    pHtmlDoc->Release();
    pHtmlDocDispatch->Release();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: SetElementSrc
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::SetElementSrc(TCHAR* Id,TCHAR* Src)
{
    if (this->pIWebBrowser==NULL)
        return FALSE;

    if (!this->IsPageCompleted())
        return FALSE;

    HRESULT hr;
    BOOL bRet;
    IDispatch* pHtmlDocDispatch = NULL;
    IHTMLDocument3* pHtmlDoc = NULL;
    IHTMLElement* pElem = NULL;
    BSTR bstrElementId;
    BSTR bstrNewContent;

#if ((!defined(UNICODE))&& (!defined(_UNICODE)))
    WCHAR* psz;
#endif

    // get IDispatch document interface
    hr = pIWebBrowser->get_Document(&pHtmlDocDispatch);
    if (FAILED (hr) || (pHtmlDocDispatch == NULL))
        return FALSE;

    // get IHTMLDocument3 document interface
    hr = pHtmlDocDispatch->QueryInterface(IID_IHTMLDocument3,(void**)&pHtmlDoc);
    if (FAILED (hr) || (pHtmlDoc == NULL))
    {
        pHtmlDocDispatch->Release();
        return FALSE;
    }

    // get pointer to element from it's Id
#if (defined(UNICODE)||defined(_UNICODE))
    bstrElementId=SysAllocString(Id);
#else
    CAnsiUnicodeConvert::AnsiToUnicode(Id,&psz);
    bstrElementId=SysAllocString(psz);
    free(psz);    
#endif
    hr = pHtmlDoc->getElementById(bstrElementId,&pElem);
    SysFreeString(bstrElementId);
    if (FAILED (hr) || (pElem == NULL))
    {
        pHtmlDoc->Release();
        pHtmlDocDispatch->Release();
        return FALSE;
    }

    // set new src
#if (defined(UNICODE)||defined(_UNICODE))
    bstrNewContent=SysAllocString(Src);
#else
    CAnsiUnicodeConvert::AnsiToUnicode(Src,&psz);
    bstrNewContent=SysAllocString(psz);
    free(psz);    
#endif
    bRet=SUCCEEDED(pElem->put_innerHTML(bstrNewContent));
    SysFreeString(bstrNewContent);

    pElem->Release();
    pHtmlDoc->Release();
    pHtmlDocDispatch->Release();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: SetElementInnerHtml
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::SetElementInnerHtml(TCHAR* Id,TCHAR* Html)
{
    if (this->pIWebBrowser==NULL)
        return FALSE;

    if (!this->IsPageCompleted())
        return FALSE;

    HRESULT hr;
    BOOL bRet;
    IDispatch* pHtmlDocDispatch = NULL;
    IHTMLDocument3* pHtmlDoc = NULL;
    IHTMLElement* pElem = NULL;
    BSTR bstrElementId;
    BSTR bstrNewContent;

#if ((!defined(UNICODE))&& (!defined(_UNICODE)))
    WCHAR* psz;
#endif

    // get IDispatch document interface
    hr = pIWebBrowser->get_Document(&pHtmlDocDispatch);
    if (FAILED (hr) || (pHtmlDocDispatch == NULL))
        return FALSE;

    // get IHTMLDocument3 document interface
    hr = pHtmlDocDispatch->QueryInterface(IID_IHTMLDocument3,(void**)&pHtmlDoc);
    if (FAILED (hr) || (pHtmlDoc == NULL))
    {
        pHtmlDocDispatch->Release();
        return FALSE;
    }

    // get pointer to element from it's Id
#if (defined(UNICODE)||defined(_UNICODE))
    bstrElementId=SysAllocString(Id);
#else
    CAnsiUnicodeConvert::AnsiToUnicode(Id,&psz);
    bstrElementId=SysAllocString(psz);
    free(psz);    
#endif
    hr = pHtmlDoc->getElementById(bstrElementId,&pElem);
    SysFreeString(bstrElementId);
    if (FAILED (hr) || (pElem == NULL))
    {
        pHtmlDoc->Release();
        pHtmlDocDispatch->Release();
        return FALSE;
    }

    // set new content
#if (defined(UNICODE)||defined(_UNICODE))
    bstrNewContent=SysAllocString(Html);
#else
    CAnsiUnicodeConvert::AnsiToUnicode(Html,&psz);
    bstrNewContent=SysAllocString(psz);
    free(psz);    
#endif
    bRet=SUCCEEDED(pElem->put_innerHTML(bstrNewContent));
    SysFreeString(bstrNewContent);

    pElem->Release();
    pHtmlDoc->Release();
    pHtmlDocDispatch->Release();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: IsPageCompleted
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::IsPageCompleted()
{
    return (WaitForSingleObject(this->hEvtDocumentFullyCompleted,0)==WAIT_OBJECT_0);
}

//-----------------------------------------------------------------------------
// Name: WaitForPageCompleted
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::WaitForPageCompleted(HANDLE hEventCancel)
{
    if (this->pIWebBrowser==NULL)
        return FALSE;

    HANDLE ph[3]={this->hEvtDocumentFullyCompleted,this->hEvtNavigateError,hEventCancel};
    DWORD NbEventToWait;
    DWORD ResWait;
    if (hEventCancel)
        NbEventToWait=3;
    else
        NbEventToWait=2;

    for(;;) 
    {
        ResWait=WaitForMultipleObjects(NbEventToWait,ph,FALSE,0);
        switch(ResWait)
        {
        case WAIT_OBJECT_0: // hEvtDocumentFullyCompleted
        	return TRUE;

        case WAIT_OBJECT_0+1: // hEvtNavigateError || hEventCancel
        case WAIT_OBJECT_0+2:
            return FALSE;

        case WAIT_TIMEOUT: // no event occurs
            break; // continue to watch for events

        default:// error occurs in WaitForMultipleObjects
            return FALSE;
        }

        // do events
        this->DoEvents();
    }
}

//-----------------------------------------------------------------------------
// Name: Navigate
// Object: Navigate to an url or local resource
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::Navigate(TCHAR* Url)
{
    if (this->pIWebBrowser==NULL)
    {
        SetEvent(this->hEvtNavigateError);
        return FALSE;
    }

    if (this->pDocumentElementEvents)
    {
        delete this->pDocumentElementEvents;
        this->pDocumentElementEvents=NULL;
    }

    ResetEvent(this->hEvtNavigateError);
    ResetEvent(this->hEvtDocumentFullyCompleted);

    BSTR SrcUrl;
    BOOL bRet;
#if (defined(UNICODE)||defined(_UNICODE))
    SrcUrl=SysAllocString(Url);
#else
    WCHAR* psz;
    CAnsiUnicodeConvert::AnsiToUnicode(Url,&psz);
    SrcUrl=SysAllocString(psz);
    free(psz);    
#endif
    bRet=SUCCEEDED(this->pIWebBrowser->Navigate(SrcUrl, NULL, NULL, NULL, NULL));
    SysFreeString(SrcUrl);

    if (!bRet)
        SetEvent(this->hEvtNavigateError);
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: Save
// Object: save page
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::Save()
{
    if (this->pIWebBrowser==NULL)
        return FALSE;
    // this->pIWebBrowser->ExecWB(OLECMDID_SAVEAS,...) troubles : 
    // don't get updated content 
    // don't update res:// links)
    // return SUCCEEDED(this->pIWebBrowser->ExecWB(OLECMDID_SAVEAS,OLECMDEXECOPT_DODEFAULT,NULL,NULL));
    TCHAR* pszFile=new TCHAR[MAX_PATH];
    *pszFile=0;

    // open dialog
    OPENFILENAME* pofn=new OPENFILENAME;
    memset(pofn,0,sizeof (OPENFILENAME));
    pofn->lStructSize=sizeof (OPENFILENAME);
    pofn->hwndOwner=this->hWnd;
    pofn->hInstance=this->hModule;
    pofn->lpstrFilter=_T("html\0*.html\0All\0*.*\0");
    pofn->nFilterIndex = 1;
    pofn->Flags=OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT;
    pofn->lpstrDefExt=_T("xml");
    pofn->lpstrFile=pszFile;
    pofn->nMaxFile=MAX_PATH;

    if (!GetSaveFileName(pofn))
        return FALSE;

    return this->SaveAs(pszFile);
}



//-----------------------------------------------------------------------------
// Name: SaveAs
// Object: save page as
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
enum CHtmlViewerSave_FirstItem
{
    CHtmlViewerSave_FirstItem_Res,
    CHtmlViewerSave_FirstItem_File,
    CHtmlViewerSave_FirstItem_DontSave
};
BOOL CHtmlViewer::SaveAs(TCHAR* FileName)
{
    if (this->pIWebBrowser==NULL)
        return FALSE;

    BOOL bSuccess=FALSE;

    // this->pIWebBrowser->ExecWB(OLECMDID_SAVEAS,...) troubles : 
    // don't get updated content 
    // don't update res:// links)

    // this saving way troubles : don't save remote files only res and file protocols supported

    CHtmlViewerSave_FirstItem FirstItem;

    HRESULT hr;
    HANDLE hFile;
    IDispatch* pHtmlDocDispatch = NULL;
    IHTMLDocument3* pHtmlDoc = NULL;
    // Retrieve the document object.

    hr = pIWebBrowser->get_Document(&pHtmlDocDispatch);
    if (SUCCEEDED (hr) && (pHtmlDocDispatch != NULL))
    {
        hr = pHtmlDocDispatch->QueryInterface(IID_IHTMLDocument3,(void**)&pHtmlDoc);
        if (SUCCEEDED (hr) && (pHtmlDoc != NULL))
        {

            IHTMLElement* pElem = NULL;

            hr = pHtmlDoc->get_documentElement(&pElem);
            if (SUCCEEDED (hr) && (pElem != NULL))
            {

                BSTR bstrDocContent;
                pElem->get_innerHTML(&bstrDocContent);
                TCHAR* pDocContent;
                TCHAR* pszCurrent;
                TCHAR* pszResProtocol;
                TCHAR* pszResProtocolEnd;
                TCHAR* pszFileProtocol;
                TCHAR* pszFileProtocolEnd;
                TCHAR* pszDontSaveBegin;
                TCHAR* pszDontSaveEnd;
                TCHAR* pszMin;
                TCHAR* psz;
                TCHAR Link[MAX_PATH];
                TCHAR ChangedLink[MAX_PATH];
    #if (defined(UNICODE)||defined(_UNICODE))
                pDocContent=bstrDocContent;
    #else
                CAnsiUnicodeConvert::UnicodeToAnsi(bstrDocContent,&pDocContent);
    #endif
                if (CTextFile::CreateTextFile(FileName,&hFile))
                {
                    CTextFile::WriteText(hFile,CHtmlViewer_DOCUMENT_HEADER);

                    pszCurrent=pDocContent;

                    for (;;)
                    {
                        // find next dont save begin
                        pszDontSaveBegin=_tcsstr(pszCurrent,CHtmlViewer_DONT_SAVE_BEGIN);

                        // find next file:// protocol
                        pszFileProtocol=_tcsstr(pszCurrent,CHtmlViewer_FILE_PROTOCOL);

                        // find next res:// protocol (like "res://foo.exe/jpg/105")
                        pszResProtocol=_tcsstr(pszCurrent,CHtmlViewer_RES_PROTOCOL);


                        // if no more res::// protocol, file:// protocol or <!--DONT_SAVE_BEGIN-->
                        if ((pszResProtocol==NULL)
                            && (pszFileProtocol==NULL)
                            && (pszDontSaveBegin==NULL)
                            ) 
                        {
                            CTextFile::WriteText(hFile,pszCurrent);
                            break;
                        }

                        // at least one item is not null
                        // get first not null item
                        if (pszDontSaveBegin)
                            pszMin=pszDontSaveBegin;
                        else
                            pszMin=(TCHAR*)-1;
                        FirstItem=CHtmlViewerSave_FirstItem_DontSave;

                        if ((pszFileProtocol<pszMin)
                            && (pszFileProtocol!=0)
                            )
                        {
                            FirstItem=CHtmlViewerSave_FirstItem_File;
                            pszMin=pszFileProtocol;
                        }

                        if ((pszResProtocol<pszMin)
                            && (pszResProtocol!=0)
                            )
                        {
                            FirstItem=CHtmlViewerSave_FirstItem_Res;
                            pszMin=pszResProtocol;
                        }

                        // do treatment for first found type
                        // (use if instead case to allow breaking for(;;). Else we need a goto)
                        if (FirstItem==CHtmlViewerSave_FirstItem_Res)
                        {
                            // write data before res:// protocol
                            CTextFile::WriteText(hFile,pszCurrent,pszResProtocol-pszCurrent+1);

                            // look for the end of res:// protocol
                            pszResProtocolEnd=_tcschr(pszResProtocol+_tcslen(CHtmlViewer_RES_PROTOCOL),'"');
                            if (!pszResProtocolEnd)
                            {
                                // bad html format, output raw content
                                SetFilePointer(hFile,0,0,FILE_BEGIN);
                                CTextFile::WriteText(hFile,CHtmlViewer_DOCUMENT_HEADER);
                                CTextFile::WriteText(hFile,pDocContent);
                                break;
                            }

                            _tcsncpy(Link,pszResProtocol,pszResProtocolEnd-pszResProtocol+1);
                            Link[pszResProtocolEnd-pszResProtocol+1]=0;

                            // change link from resource one to extracted resource one
                            this->ModifyLinkAndExtractResource(FileName,Link,ChangedLink);
                            // write modified link
                            CTextFile::WriteText(hFile,ChangedLink);
                            // end link by adding '"'
                            CTextFile::WriteText(hFile,_T("\""));

                            // pointer after res protocol
                            pszCurrent=pszResProtocolEnd+1;
                        }
                        else if (FirstItem==CHtmlViewerSave_FirstItem_File)
                        {
                            // write data before file:// protocol
                            CTextFile::WriteText(hFile,pszCurrent,pszFileProtocol-pszCurrent+1);

                            // look for the end of file:// protocol
                            pszFileProtocolEnd=_tcschr(pszFileProtocol+_tcslen(CHtmlViewer_FILE_PROTOCOL),'"');
                            if (!pszFileProtocolEnd)
                            {
                                // bad html format, output raw content
                                SetFilePointer(hFile,0,0,FILE_BEGIN);
                                CTextFile::WriteText(hFile,CHtmlViewer_DOCUMENT_HEADER);
                                CTextFile::WriteText(hFile,pDocContent);
                                break;
                            }
                            pszFileProtocol+=_tcslen(CHtmlViewer_FILE_PROTOCOL);
                            _tcsncpy(Link,pszFileProtocol,pszFileProtocolEnd-pszFileProtocol);
                            Link[pszFileProtocolEnd-pszFileProtocol]=0;
                            

                            // get saving filename
                            _tcscpy(ChangedLink,FileName);
                            // remove file extension of saving file name
                            CStdFileOperations::RemoveFileExt(ChangedLink);
                            // add "_files\" to saving file to create a directory like SavingFileName_files
                            _tcscat(ChangedLink,CHtmlViewer_FILES_DIR_EXT);
                            // add the name of file
                            _tcscat(ChangedLink,CStdFileOperations::GetFileName(Link));
                            // create directory if necessary
                            CStdFileOperations::CreateDirectoryForFile(ChangedLink);
                            // copy file to directory
                            CopyFile(Link,ChangedLink,FALSE);
                           
                            // now make a relative link to file
                            _tcscpy(Link,ChangedLink);
                            psz=_tcsrchr(Link,'\\');
                            if (psz)
                            {
                                *psz='/';
                                psz=_tcsrchr(Link,'\\');
                                if (psz)
                                {
                                    _tcscpy(ChangedLink,psz+1);
                                }
                            }

                            // write modified link
                            CTextFile::WriteText(hFile,ChangedLink);
                            // end link by adding '"'
                            CTextFile::WriteText(hFile,_T("\""));

                            // pointer after file protocol
                            pszCurrent=pszFileProtocolEnd+1;
                        }
                        else if (FirstItem==CHtmlViewerSave_FirstItem_DontSave)
                        {
                            // write data before <!--DONT_SAVE_BEGIN-->
                            CTextFile::WriteText(hFile,pszCurrent,pszDontSaveBegin-pszCurrent);

                            pszDontSaveEnd=_tcsstr(pszDontSaveBegin+_tcslen(CHtmlViewer_DONT_SAVE_BEGIN),CHtmlViewer_DONT_SAVE_END);
                            if (!pszDontSaveEnd)
                            {
                                // bad html format, output raw content
                                SetFilePointer(hFile,0,0,FILE_BEGIN);
                                CTextFile::WriteText(hFile,CHtmlViewer_DOCUMENT_HEADER);
                                CTextFile::WriteText(hFile,pDocContent);
                                break;
                            }
                            // point after <!--DONT_SAVE_END-->
                            pszCurrent=pszDontSaveEnd+_tcslen(CHtmlViewer_DONT_SAVE_END);
                        }
                    }
                    CTextFile::WriteText(hFile,CHtmlViewer_DOCUMENT_FOOTER);
                    CloseHandle(hFile);

                    bSuccess=TRUE;
                }
    #if ((!defined(UNICODE))&& (!defined(_UNICODE)))
                // free allocated string for ansi
                free(pDocContent);
    #endif

                SysFreeString(bstrDocContent);

                pElem->Release();
            }
            pHtmlDoc->Release();
        }
        
        pHtmlDocDispatch->Release();
    }
    return bSuccess;
}

//-----------------------------------------------------------------------------
// Name: ModifyLinkAndExtractResource
// Object: Modify res Link ExtractResource
//         translate something like "res://foo.exe/jpg/105" to "SavingName_files/jpg/105"
//         and extract resource jpg/105 to matching directory
//         res protocol syntax res://sFile[/sType]/sID
// Parameters :
//     in  : TCHAR* SavingFileName
//           TCHAR* ResLink link like "res://foo.exe/jpg/105" WITH quotes
//     out : TCHAR* StandardLink
//     return : 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::ModifyLinkAndExtractResource(TCHAR* SavingFileName,TCHAR* ResLink,OUT TCHAR* StandardLink)
{
    // default value
    _tcscpy(StandardLink,ResLink);

    TCHAR* psz;
    TCHAR* pszRelativeResLink;
    TCHAR LocalResLink[MAX_PATH];
    TCHAR* pszRelativePath;
    TCHAR RelativePath[MAX_PATH];
    TCHAR* pszPath=(TCHAR*)_alloca((_tcslen(SavingFileName)+_tcslen(ResLink)+_tcslen(CHtmlViewer_FILES_DIR_EXT)+1)*sizeof(TCHAR));
    _tcscpy(pszPath,SavingFileName);

    // remove file extension
    if (!CStdFileOperations::RemoveFileExt(pszPath))
        return FALSE;

    // get relative path
    pszRelativePath=_tcsrchr(pszPath,'\\');
    if (!pszRelativePath)
        return FALSE;
    // point after '\'
    pszRelativePath++;

    // add "_files\"
    _tcscat(pszPath,CHtmlViewer_FILES_DIR_EXT);

    // get res relative path (extract jpg/105 from "res://foo.exe/jpg/105")
    _tcscpy(LocalResLink,ResLink);
    pszRelativeResLink=LocalResLink+_tcslen(CHtmlViewer_RES_PROTOCOL);
    pszRelativeResLink=_tcschr(pszRelativeResLink,'/');
    if (!pszRelativeResLink)
        return FALSE;
    // point after '/' (remove application name/path)
    pszRelativeResLink++;

    // remove last " of LocalResLink
    if (pszRelativeResLink[_tcslen(pszRelativeResLink)-1]=='"')
        pszRelativeResLink[_tcslen(pszRelativeResLink)-1]=0;

    // replace '/'' by '\\'
    for (psz=pszRelativeResLink;*psz!=0;psz++)
    {
        if (*psz=='/')
            *psz='\\';
    }
    _tcscat(pszPath,pszRelativeResLink);

    // create file from resource
    CBinResource::ExtractBinResource(ResLink,pszPath);

    // get html link
    _tcscpy(RelativePath,pszRelativePath);
    // replace '\' by '/' in RelativePath
    for (psz=RelativePath;*psz!=0;psz++)
    {
        if (*psz=='\\')
            *psz='/';
    }

    _tcscpy(StandardLink,RelativePath);

    return TRUE;
}

HRESULT STDMETHODCALLTYPE CHtmlViewer::WebBrowserEventCallBackStatic(DISPID dispidMember,
                                                               REFIID riid,
                                                               LCID lcid,
                                                               WORD wFlags,
                                                               DISPPARAMS* pdispparams,
                                                               VARIANT* pvarResult,
                                                               EXCEPINFO* pexcepinfo,
                                                               UINT* puArgErr,
                                                               PVOID UserParam)
{
    return ((CHtmlViewer*)UserParam)->WebBrowserEventCallBack(
                                                            dispidMember,
                                                            riid,
                                                            lcid,
                                                            wFlags,
                                                            pdispparams,
                                                            pvarResult,
                                                            pexcepinfo,
                                                            puArgErr);
}
HRESULT STDMETHODCALLTYPE CHtmlViewer::WebBrowserEventCallBack(DISPID dispidMember,
                                                  REFIID riid,
                                                  LCID lcid,
                                                  WORD wFlags,
                                                  DISPPARAMS* pdispparams,
                                                  VARIANT* pvarResult,
                                                  EXCEPINFO* pexcepinfo,
                                                  UINT* puArgErr)
{
    UNREFERENCED_PARAMETER(puArgErr);
    UNREFERENCED_PARAMETER(pexcepinfo);
    UNREFERENCED_PARAMETER(pvarResult);
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(wFlags);
    UNREFERENCED_PARAMETER(pdispparams);

    switch (dispidMember)
    {
    case DISPID_DOCUMENTCOMPLETE:
        {
        this->OnDocumentCompleted();
        // pdispparams->rgvarg[0] : url
        // pdispparams->rgvarg[1]

        LPDISPATCH pDisp=pdispparams->rgvarg[1].pdispVal;
        HRESULT hr;
        IUnknown* pUnkBrowser = NULL;
        IUnknown* pUnkDisp = NULL;

        hr = this->pIWebBrowser->QueryInterface( IID_IUnknown,  (void**)&pUnkBrowser );
        if ( SUCCEEDED(hr) )
        {
            hr = pDisp->QueryInterface( IID_IUnknown,  (void**)&pUnkDisp );
            if ( SUCCEEDED(hr) )
            {
                if ( pUnkBrowser == pUnkDisp )
                {   // This is the DocumentComplete event for the top frame - page is loaded!
                    SetEvent(this->hEvtDocumentFullyCompleted);
                }
                pUnkDisp->Release();
            }
            pUnkBrowser->Release();
        }
        }
        break;
    case DISPID_NAVIGATEERROR:
        SetEvent(this->hEvtNavigateError);
        break;
    }
    return S_OK;
}

void CHtmlViewer::OnDocumentCompleted()
{
    // disconnect / remove old documents events
    if (this->pDocumentElementEvents)
        delete this->pDocumentElementEvents;

    // attach new documents events
    HRESULT hr;
    IDispatch* pHtmlDocDispatch = NULL;
    IHTMLDocument3* pHtmlDoc = NULL;
    // Retrieve the document object.

    hr = pIWebBrowser->get_Document(&pHtmlDocDispatch);
    if (SUCCEEDED (hr) && (pHtmlDocDispatch != NULL))
    {
        hr = pHtmlDocDispatch->QueryInterface(IID_IHTMLDocument3,(void**)&pHtmlDoc);
        if (SUCCEEDED (hr) && (pHtmlDoc != NULL))
        {

            IHTMLElement* pElem = NULL;
            pHtmlDoc->get_documentElement(&pElem);

            this->pDocumentElementEvents=new CElementEvents(pElem);
            this->pDocumentElementEvents->ConnectEvents();

            this->pDocumentElementEvents->SetElementEventsCallBack(this->ElementEventsCallBack,this->ElementEventsCallBackUserParam);
            this->pDocumentElementEvents->SetElementEventsCallBackEx(this->ElementEventsCallBackEx,this->ElementEventsCallBackExUserParam);

            this->pDocumentElementEvents->EnableSelection(this->bEnableSelection);
            this->pDocumentElementEvents->EnableContextMenu(this->bEnableContextMenu);

            pElem->Release();
            pHtmlDoc->Release();
        }

        pHtmlDocDispatch->Release();
    }
}


BOOL CHtmlViewer::ExecScript(TCHAR* ScriptContent)
{
    VARIANT ScriptResult;
    return this->ExecScript(ScriptContent,&ScriptResult);
}


BOOL CHtmlViewer::ExecScript(TCHAR* ScriptContent,VARIANT* pScriptResult)
{
    BOOL bRet;
    EXCEPINFO ExcepInfo;

    bRet=this->ExecScript(ScriptContent,pScriptResult,&ExcepInfo);

    if (ExcepInfo.bstrSource)
        SysFreeString(ExcepInfo.bstrSource);
    if (ExcepInfo.bstrHelpFile)
        SysFreeString(ExcepInfo.bstrHelpFile);
    if (ExcepInfo.bstrDescription)
        SysFreeString(ExcepInfo.bstrDescription);

    return bRet;
}


// thanks to Eugene Khodakovsky (I've just modified his code to make more lazy script call)
BOOL CHtmlViewer::ExecScript(TCHAR* ScriptContent,VARIANT* pScriptResult,EXCEPINFO* pExcepInfo)
{
    HRESULT hr = S_OK;

    IDispatch* pHtmlDocDispatch = NULL;
    IHTMLDocument2* pHtmlDoc = NULL;
    DISPID dispid = NULL;
    BSTR bstrMember;
    IDispatch* spScript;

    if (IsBadWritePtr(pExcepInfo,sizeof(EXCEPINFO)))
        return FALSE;

    memset(pExcepInfo, 0, sizeof(EXCEPINFO));
    pScriptResult->vt=VT_EMPTY;

    // Retrieve the document object.

    hr = pIWebBrowser->get_Document(&pHtmlDocDispatch);
    if (SUCCEEDED (hr) && (pHtmlDocDispatch != NULL))
    {
        hr = pHtmlDocDispatch->QueryInterface(IID_IHTMLDocument2,(void**)&pHtmlDoc);
        if (SUCCEEDED (hr) && (pHtmlDoc != NULL))
        {
            //Getting IDispatch for Java Script objects
            hr = pHtmlDoc->get_Script(&spScript);

            if (SUCCEEDED (hr) && (spScript != NULL))
            {
                bstrMember=SysAllocString(L"eval");// THE trick is here
                if (bstrMember)
                {
                    hr = spScript->GetIDsOfNames(IID_NULL,&bstrMember,1,LOCALE_SYSTEM_DEFAULT,&dispid);

                    if (SUCCEEDED (hr))
                    {
                        //Putting parameters
                        DISPPARAMS dispparams;
                        memset(&dispparams, 0, sizeof (DISPPARAMS));
                        dispparams.cArgs      = 1;
                        dispparams.rgvarg     = new VARIANT[dispparams.cArgs];
                        dispparams.cNamedArgs = 0;

                        dispparams.rgvarg[0].vt = VT_BSTR;

                        WCHAR* wScriptContent;
#if (defined(UNICODE)||defined(_UNICODE))
                        wScriptContent=ScriptContent;
#else
                        CAnsiUnicodeConvert::AnsiToUnicode(ScriptContent,&wScriptContent);
#endif
                        dispparams.rgvarg[0].bstrVal = SysAllocString(wScriptContent);

#if ((!defined(UNICODE))&& (!defined(_UNICODE)))
                        free(wScriptContent);
#endif
                        /*
                        // code for those who want to do the same thing
                        // using an array of string "const CStringArray& paramArray" as parameter
                        // instead of eval tips
                        
                        const int arraySize = paramArray.GetSize();
                        //Putting parameters
                        DISPPARAMS dispparams;
                        memset(&dispparams, 0, sizeof dispparams);
                        dispparams.cArgs      = arraySize;
                        dispparams.rgvarg     = new VARIANT[dispparams.cArgs];
                        dispparams.cNamedArgs = 0;

                        for( int i = 0; i < arraySize; i++)
                        {
                            CComBSTR> bstr = paramArray.GetAt(arraySize - 1 - i);
                            // back reading
                            bstr.CopyTo(&dispparams.rgvarg[i].bstrVal);
                            dispparams.rgvarg[i].vt = VT_BSTR;
                        }
                        */

                        if (dispparams.rgvarg[0].bstrVal)
                        {
                            UINT nArgErr = (UINT)-1;// initialize to invalid arg
                            //Call JavaScript function
                            hr = spScript->Invoke(dispid,
                                                    IID_NULL,
                                                    0,
                                                    DISPATCH_METHOD,
                                                    &dispparams,
                                                    pScriptResult,
                                                    pExcepInfo,
                                                    &nArgErr);

                            SysFreeString(dispparams.rgvarg[0].bstrVal);
                        }

                        delete [] dispparams.rgvarg;
                    }

                    SysFreeString(bstrMember);
                }
                
            }
            pHtmlDoc->Release();
        }
        pHtmlDocDispatch->Release();
    }

    return SUCCEEDED (hr);
}

//-----------------------------------------------------------------------------
// Name: SetElementsEventsCallBack
// Object: set simple document events callback
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::SetElementsEventsCallBack(pfElementEventsCallBack ElementEventsCallBack,LPVOID UserParam)
{
    if (IsBadCodePtr((FARPROC)ElementEventsCallBack) && (ElementEventsCallBack!=0))
        return FALSE;

    this->ElementEventsCallBack=ElementEventsCallBack;
    this->ElementEventsCallBackUserParam=UserParam;

    if (this->pDocumentElementEvents)
        this->pDocumentElementEvents->SetElementEventsCallBack(this->ElementEventsCallBack,this->ElementEventsCallBackUserParam);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetElementsEventsCallBackEx
// Object: set extended document events callback
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CHtmlViewer::SetElementsEventsCallBackEx(pfElementEventsCallBackEx ElementEventsCallBackEx,LPVOID UserParam)
{
    if (IsBadCodePtr((FARPROC)ElementEventsCallBackEx) && (ElementEventsCallBackEx!=0))
        return FALSE;

    this->ElementEventsCallBackEx=ElementEventsCallBackEx;
    this->ElementEventsCallBackExUserParam=UserParam;

    if (this->pDocumentElementEvents)
        this->pDocumentElementEvents->SetElementEventsCallBackEx(this->ElementEventsCallBackEx,this->ElementEventsCallBackExUserParam);

    return TRUE;
}

BOOL CHtmlViewer::EnableContextMenu(BOOL Enable)
{
    this->bEnableContextMenu=Enable;

    if (this->pDocumentElementEvents)
        this->pDocumentElementEvents->EnableContextMenu(this->bEnableContextMenu);

    return TRUE;
}
BOOL CHtmlViewer::EnableSelection(BOOL Enable)
{
    this->bEnableSelection=Enable;

    if (this->pDocumentElementEvents)
        this->pDocumentElementEvents->EnableSelection(this->bEnableSelection);

    return TRUE;
}