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

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <commctrl.h>
#include <exdisp.h> // WebBrowser stuff
#include <docobj.h> // OLECMDID_SAVEAS OLECMDID_SAVE
#include <Mshtml.h>
#include <malloc.h>

#pragma comment(lib,"comctl32.lib")


#include "../../../../String/AnsiUnicodeConvert.h"
#include "../../../../File/TextFile.h"
#include "../../../../BinResources/BinResource.h"
#include "BrowserEvents.h"
#include "ElementEvents.h"

#include "resource.h"
#define CHtmlViewer_FILES_DIR_EXT           _T("_files\\")
#define CHtmlViewer_RES_PROTOCOL            _T("\"res://")
#define CHtmlViewer_FILE_PROTOCOL           _T("\"file://")
#define CHtmlViewer_DONT_SAVE_BEGIN         _T("<!--DONT_SAVE_BEGIN-->")
#define CHtmlViewer_DONT_SAVE_END           _T("<!--DONT_SAVE_END-->")
#define CHtmlViewer_DOCUMENT_HEADER         _T("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\"><html xmlns=\"http://www.w3.org/1999/xhtml\">")
#define CHtmlViewer_DOCUMENT_FOOTER         _T("</html>")

typedef HRESULT (WINAPI *pfAtlAxAttachControl)(IUnknown*, HWND,IUnknown**);

class CHtmlViewer
{
private:
    HMODULE hModule;
    HWND hWnd;
    BOOL EmptyPageLoaded;
    IWebBrowser2* pIWebBrowser;
    pfAtlAxAttachControl AtlAxAttachControl;
    IOleInPlaceActiveObject* pIOleInPlaceActiveObject;
    CElementEvents* pDocumentElementEvents;
    CBrowserEvents* pBrowserEvents;
    BOOL HelperScriptSuccessFullyWritten;
    pfElementEventsCallBack ElementEventsCallBack;
    LPVOID ElementEventsCallBackUserParam;
    pfElementEventsCallBackEx ElementEventsCallBackEx;
    LPVOID ElementEventsCallBackExUserParam;
    BOOL bEnableContextMenu;
    BOOL bEnableSelection;
    HANDLE hEvtNavigateError;
    HANDLE hEvtDocumentFullyCompleted;
    static DWORD WINAPI DocumentStateWatcherStatic(LPVOID lParam);
    void DocumentStateWatcher();

    void Initialize(HMODULE hModule);
    void DoEvents();
    BOOL ModifyLinkAndExtractResource(TCHAR* SavingFileName,TCHAR* ResLink,OUT TCHAR* StandardLink);
    static HRESULT STDMETHODCALLTYPE WebBrowserEventCallBackStatic(DISPID dispidMember,
                                                                    REFIID riid,
                                                                    LCID lcid,
                                                                    WORD wFlags,
                                                                    DISPPARAMS* pdispparams,
                                                                    VARIANT* pvarResult,
                                                                    EXCEPINFO* pexcepinfo,
                                                                    UINT* puArgErr,
                                                                    PVOID UserParam);
    HRESULT STDMETHODCALLTYPE WebBrowserEventCallBack(DISPID dispidMember,
                                                        REFIID riid,
                                                        LCID lcid,
                                                        WORD wFlags,
                                                        DISPPARAMS* pdispparams,
                                                        VARIANT* pvarResult,
                                                        EXCEPINFO* pexcepinfo,
                                                        UINT* puArgErr);
    void OnDocumentCompleted();
    BOOL AddHtmlContentToElement(IHTMLElement* pElem,TCHAR* Content);
    BOOL IsPageCompleted();
public:
    CHtmlViewer(HMODULE hModule,HWND hWnd);
    CHtmlViewer(HMODULE hModule,HWND hWndParent,DWORD X, DWORD Y, DWORD Width, DWORD Height, OUT HWND* pHtmlViewerhWnd);
    ~CHtmlViewer(void);

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
    BOOL TranslateAccelerator(MSG* pMsg);
    IHTMLElement* GetHTMLElement(TCHAR* Id);
    IDispatch* GetHTMLDocument();
    IWebBrowser2* GetWebBrowser();
};
