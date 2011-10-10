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

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "resource.h"
#include "HtmlViewer.h"
#include "../../../../LinkList/LinkListSimple.h"
#include "../../../../String/AnsiUnicodeConvert.h"


BOOL Initialize();
BOOL Destroy();

HINSTANCE DllhInstance;
CLinkListSimple* pHtmlViewerObjectsList=NULL;
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, PVOID pvReserved)
{
    UNREFERENCED_PARAMETER(pvReserved);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DllhInstance=hInstDLL;

        // avoid DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications
        DisableThreadLibraryCalls(hInstDLL);

        return Initialize();
    case DLL_PROCESS_DETACH:
        return Destroy();
    }

    return TRUE;
}

BOOL Initialize()
{
    // allocate link list to store all HtmlViewers
    pHtmlViewerObjectsList=new CLinkListSimple();

    if (pHtmlViewerObjectsList)
        return TRUE;
    else
        return FALSE;
}
BOOL Destroy()
{
    // delete all HtmlViewer objects
    if (pHtmlViewerObjectsList)
    {
        pHtmlViewerObjectsList->Lock();

        CLinkListItem* pItem;
        for (pItem=pHtmlViewerObjectsList->Head;pItem;pItem=pItem->NextItem)
        {
            delete ((CHtmlViewer*)pItem->ItemData);
        }
        pHtmlViewerObjectsList->RemoveAllItems(TRUE);
        pHtmlViewerObjectsList->Unlock();
        delete pHtmlViewerObjectsList;
        pHtmlViewerObjectsList=NULL;
    }

    return TRUE;
}

CHtmlViewer* GetHtmlViewerObjectFromHandle(HANDLE hHtmlViewer)
{
    if (pHtmlViewerObjectsList==NULL)
        return FALSE;

    pHtmlViewerObjectsList->Lock();

    CLinkListItem* pItem;
    for (pItem=pHtmlViewerObjectsList->Head;pItem;pItem=pItem->NextItem)
    {
        if (hHtmlViewer==((HANDLE)pItem->ItemData))
        {
            pHtmlViewerObjectsList->Unlock();
            return (CHtmlViewer*)hHtmlViewer;
        }
    }
    pHtmlViewerObjectsList->Unlock();
    MessageBox(NULL,_T("Bad HtmlViewer handle"),_T("Error"),MB_ICONERROR|MB_OK);
    return NULL;
}

//-----------------------------------------------------------------------------
// Name: CreateHtmlViewer
// Object: Apply HtmlViewer to an existing control
//          HANDLE hHtmlViewer=CreateHtmlViewer(HWND hWnd)
// Parameters :
//     in  : 
//     out : 
//     return : HtmlViewer HANDLE on success, NULL on error
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) HANDLE __stdcall CreateHtmlViewerFromHwnd(HWND hWnd)
{
    if (pHtmlViewerObjectsList==NULL)
        return NULL;

    CHtmlViewer* pHtmlViewer=new CHtmlViewer(DllhInstance,hWnd);
    if (pHtmlViewer==NULL)
        return NULL;

    if(pHtmlViewerObjectsList->AddItem(pHtmlViewer)==NULL)
    {
        delete pHtmlViewer;
        return NULL;
    }
    return (HANDLE)pHtmlViewer;
}

//-----------------------------------------------------------------------------
// Name: CreateHtmlViewer
// Object: Create an HtmlViewer control
//          HANDLE hHtmlViewer=CreateHtmlViewer(HWND hWndParent,DWORD X, DWORD Y, DWORD Width, DWORD Height, OUT HWND* HtmlViewerhWnd)
// Parameters :
//     in  : 
//     out : 
//     return : HtmlViewer HANDLE on success, NULL on error
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) HANDLE __stdcall CreateHtmlViewer(HWND hWndParent,DWORD X, DWORD Y, DWORD Width, DWORD Height, OUT HWND* pHtmlViewerhWnd)
{
    if (pHtmlViewerObjectsList==NULL)
        return NULL;

    CHtmlViewer* pHtmlViewer=new CHtmlViewer(DllhInstance,hWndParent,X,Y,Width,Height,pHtmlViewerhWnd);
    if (pHtmlViewer==NULL)
        return NULL;

    if(pHtmlViewerObjectsList->AddItem(pHtmlViewer)==NULL)
    {
        delete pHtmlViewer;
        return NULL;
    }
    return (HANDLE)pHtmlViewer;
}

//-----------------------------------------------------------------------------
// Name: DestroyHtmlViewer
// Object: Destroy an HtmlViewer control
//          HANDLE hHtmlViewer=CreateHtmlViewer(HWND hWndParent,DWORD X, DWORD Y, DWORD Width, DWORD Height, OUT HWND* HtmlViewerhWnd)
// Parameters :
//     in  : 
//     out : 
//     return : HtmlViewer HANDLE on success, NULL on error
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall DestroyHtmlViewer(HANDLE hHtmlViewer)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    pHtmlViewerObjectsList->Lock();
    delete pHtmlViewer;
    pHtmlViewerObjectsList->RemoveItemFromItemData(pHtmlViewer,TRUE);
    pHtmlViewerObjectsList->Unlock();

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Navigate
// Object: Navigate to an url or local resource
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL Navigate(HANDLE hHtmlViewer,TCHAR* Url)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->Navigate(Url);
}

//-----------------------------------------------------------------------------
// Name: NavigateA
// Object: Navigate to an url or local resource
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall NavigateA(HANDLE hHtmlViewer,CHAR* Url)
{
#if (defined(UNICODE)||defined(_UNICODE))
    WCHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::AnsiToUnicode(Url,&psz);
    bRet=Navigate(hHtmlViewer,psz);
    free(psz);
    return bRet;
#else
    return Navigate(hHtmlViewer,Url);
#endif
}
//-----------------------------------------------------------------------------
// Name: NavigateW
// Object: Navigate to an url or local resource
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall NavigateW(HANDLE hHtmlViewer,WCHAR* Url)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return Navigate(hHtmlViewer,Url);
#else
    CHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::UnicodeToAnsi(Url,&psz);
    bRet=Navigate(hHtmlViewer,psz);
    free(psz);
    return bRet;
#endif
}

//-----------------------------------------------------------------------------
// Name: WaitForPageCompleted
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall WaitForPageCompleted(HANDLE hHtmlViewer,HANDLE hEventCancel)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->WaitForPageCompleted(hEventCancel);
}

//-----------------------------------------------------------------------------
// Name: SetElementInnerHtml
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL SetElementInnerHtml(HANDLE hHtmlViewer,TCHAR* Id,TCHAR* Html)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->SetElementInnerHtml(Id,Html);
}

//-----------------------------------------------------------------------------
// Name: SetElementInnerHtml
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall SetElementInnerHtmlA(HANDLE hHtmlViewer,CHAR* Id,CHAR* Html)
{
#if (defined(UNICODE)||defined(_UNICODE))
    WCHAR* psz;
    WCHAR* psz2;
    BOOL bRet;
    CAnsiUnicodeConvert::AnsiToUnicode(Id,&psz);
    CAnsiUnicodeConvert::AnsiToUnicode(Html,&psz2);
    bRet=SetElementInnerHtml(hHtmlViewer,psz,psz2);
    free(psz);
    free(psz2);
    return bRet;
#else
    return SetElementInnerHtml(hHtmlViewer,Id,Html);
#endif
}

//-----------------------------------------------------------------------------
// Name: SetElementInnerHtml
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall SetElementInnerHtmlW(HANDLE hHtmlViewer,WCHAR* Id,WCHAR* Html)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return SetElementInnerHtml(hHtmlViewer,Id,Html);
#else
    CHAR* psz;
    CHAR* psz2;
    BOOL bRet;
    CAnsiUnicodeConvert::UnicodeToAnsi(Id,&psz);
    CAnsiUnicodeConvert::UnicodeToAnsi(Html,&psz2);
    bRet=SetElementInnerHtml(hHtmlViewer,psz,psz2);
    free(psz);
    free(psz2);
    return bRet;
#endif
}

//-----------------------------------------------------------------------------
// Name: SetElementSrc
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL SetElementSrc(HANDLE hHtmlViewer,TCHAR* Id,TCHAR* Src)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->SetElementSrc(Id,Src);
}

//-----------------------------------------------------------------------------
// Name: SetElementSrc
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall SetElementSrcA(HANDLE hHtmlViewer,CHAR* Id,CHAR* Src)
{
#if (defined(UNICODE)||defined(_UNICODE))
    WCHAR* psz;
    WCHAR* psz2;
    BOOL bRet;
    CAnsiUnicodeConvert::AnsiToUnicode(Id,&psz);
    CAnsiUnicodeConvert::AnsiToUnicode(Src,&psz2);
    bRet=SetElementSrc(hHtmlViewer,psz,psz2);
    free(psz);
    free(psz2);
    return bRet;
#else
    return SetElementSrc(hHtmlViewer,Id,Src);
#endif
}

//-----------------------------------------------------------------------------
// Name: SetElementSrc
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall SetElementSrcW(HANDLE hHtmlViewer,WCHAR* Id,WCHAR* Src)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return SetElementSrc(hHtmlViewer,Id,Src);
#else
    CHAR* psz;
    CHAR* psz2;
    BOOL bRet;
    CAnsiUnicodeConvert::UnicodeToAnsi(Id,&psz);
    CAnsiUnicodeConvert::UnicodeToAnsi(Src,&psz2);
    bRet=SetElementSrc(hHtmlViewer,psz,psz2);
    free(psz);
    free(psz2);
    return bRet;
#endif
}


//-----------------------------------------------------------------------------
// Name: AddHtmlContentToElement
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL AddHtmlContentToElement(HANDLE hHtmlViewer,TCHAR* Id,TCHAR* Content)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->AddHtmlContentToElement(Id,Content);
}

//-----------------------------------------------------------------------------
// Name: AddHtmlContentToElement
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall AddHtmlContentToElementA(HANDLE hHtmlViewer,CHAR* Id,CHAR* Content)
{
#if (defined(UNICODE)||defined(_UNICODE))
    WCHAR* psz;
    WCHAR* psz2;
    BOOL bRet;
    CAnsiUnicodeConvert::AnsiToUnicode(Id,&psz);
    CAnsiUnicodeConvert::AnsiToUnicode(Content,&psz2);
    bRet=AddHtmlContentToElement(hHtmlViewer,psz,psz2);
    free(psz);
    free(psz2);
    return bRet;
#else
    return AddHtmlContentToElement(hHtmlViewer,Id,Content);
#endif
}

//-----------------------------------------------------------------------------
// Name: AddHtmlContentToElement
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall AddHtmlContentToElementW(HANDLE hHtmlViewer,WCHAR* Id,WCHAR* Content)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return AddHtmlContentToElement(hHtmlViewer,Id,Content);
#else
    CHAR* psz;
    CHAR* psz2;
    BOOL bRet;
    CAnsiUnicodeConvert::UnicodeToAnsi(Id,&psz);
    CAnsiUnicodeConvert::UnicodeToAnsi(Content,&psz2);
    bRet=AddHtmlContentToElement(hHtmlViewer,psz,psz2);
    free(psz);
    free(psz2);
    return bRet;
#endif
}

//-----------------------------------------------------------------------------
// Name: AddHtmlContentToElement
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL AddHtmlContentToBody(HANDLE hHtmlViewer,TCHAR* Content)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->AddHtmlContentToBody(Content);
}

//-----------------------------------------------------------------------------
// Name: AddHtmlContentToElement
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall AddHtmlContentToBodyA(HANDLE hHtmlViewer,CHAR* Content)
{
#if (defined(UNICODE)||defined(_UNICODE))
    WCHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::AnsiToUnicode(Content,&psz);
    bRet=AddHtmlContentToBody(hHtmlViewer,psz);
    free(psz);
    return bRet;
#else
    return AddHtmlContentToBody(hHtmlViewer,Content);
#endif
}

//-----------------------------------------------------------------------------
// Name: AddHtmlContentToElement
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall AddHtmlContentToBodyW(HANDLE hHtmlViewer,WCHAR* Content)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return AddHtmlContentToBody(hHtmlViewer,Content);
#else
    CHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::UnicodeToAnsi(Content,&psz);
    bRet=AddHtmlContentToBody(hHtmlViewer,psz);
    free(psz);
    return bRet;
#endif
}

//-----------------------------------------------------------------------------
// Name: LoadEmptyPageAndSetBodyContent
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL LoadEmptyPageAndSetBodyContent(HANDLE hHtmlViewer,TCHAR* Content)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->LoadEmptyPageAndSetBodyContent(Content);
}

//-----------------------------------------------------------------------------
// Name: LoadEmptyPageAndSetBodyContentA
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall LoadEmptyPageAndSetBodyContentA(HANDLE hHtmlViewer,CHAR* Content)
{
#if (defined(UNICODE)||defined(_UNICODE))
    WCHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::AnsiToUnicode(Content,&psz);
    bRet=LoadEmptyPageAndSetBodyContent(hHtmlViewer,psz);
    free(psz);
    return bRet;
#else
    return LoadEmptyPageAndSetBodyContent(hHtmlViewer,Content);
#endif
}

//-----------------------------------------------------------------------------
// Name: LoadEmptyPageAndSetBodyContentW
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall LoadEmptyPageAndSetBodyContentW(HANDLE hHtmlViewer,WCHAR* Content)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return LoadEmptyPageAndSetBodyContent(hHtmlViewer,Content);
#else
    CHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::UnicodeToAnsi(Content,&psz);
    bRet=LoadEmptyPageAndSetBodyContent(hHtmlViewer,psz);
    free(psz);
    return bRet;
#endif
}


//-----------------------------------------------------------------------------
// Name: Save
// Object: 
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall Save(HANDLE hHtmlViewer)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->Save();
}


//-----------------------------------------------------------------------------
// Name: SaveAs
// Object: Save as
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL SaveAs(HANDLE hHtmlViewer,TCHAR* FileName)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->SaveAs(FileName);
}

//-----------------------------------------------------------------------------
// Name: SaveAsA
// Object: Save as
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall SaveAsA(HANDLE hHtmlViewer,CHAR* FileName)
{
#if (defined(UNICODE)||defined(_UNICODE))
    WCHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::AnsiToUnicode(FileName,&psz);
    bRet=SaveAs(hHtmlViewer,psz);
    free(psz);
    return bRet;
#else
    return Navigate(hHtmlViewer,FileName);
#endif
}
//-----------------------------------------------------------------------------
// Name: SaveAsW
// Object: Save as
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall SaveAsW(HANDLE hHtmlViewer,WCHAR* FileName)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return SaveAs(hHtmlViewer,FileName);
#else
    CHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::UnicodeToAnsi(FileName,&psz);
    bRet=SaveAs(hHtmlViewer,psz);
    free(psz);
    return bRet;
#endif
}

//-----------------------------------------------------------------------------
// Name: ExecScriptEx
// Object: Exec Script
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL ExecScriptEx(HANDLE hHtmlViewer,TCHAR* ScriptContent,VARIANT* pScriptResult)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->ExecScript(ScriptContent,pScriptResult);
}

//-----------------------------------------------------------------------------
// Name: ExecScriptExA
// Object: Exec Script
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall ExecScriptExA(HANDLE hHtmlViewer,CHAR* ScriptContent,VARIANT* pScriptResult)
{
#if (defined(UNICODE)||defined(_UNICODE))
    WCHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::AnsiToUnicode(ScriptContent,&psz);
    bRet=ExecScriptEx(hHtmlViewer,psz,pScriptResult);
    free(psz);
    return bRet;
#else
    return ExecScriptEx(hHtmlViewer,ScriptContent,pScriptResult);
#endif
}
//-----------------------------------------------------------------------------
// Name: ExecScriptExW
// Object: Exec Script
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall ExecScriptExW(HANDLE hHtmlViewer,WCHAR* ScriptContent,VARIANT* pScriptResult)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return ExecScriptEx(hHtmlViewer,ScriptContent,pScriptResult);
#else
    CHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::UnicodeToAnsi(ScriptContent,&psz);
    bRet=ExecScriptEx(hHtmlViewer,psz,pScriptResult);
    free(psz);
    return bRet;
#endif
}


//-----------------------------------------------------------------------------
// Name: ExecScript
// Object: Exec Script
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL ExecScript(HANDLE hHtmlViewer,TCHAR* ScriptContent)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->ExecScript(ScriptContent);
}

//-----------------------------------------------------------------------------
// Name: ExecScriptA
// Object: Exec Script
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall ExecScriptA(HANDLE hHtmlViewer,CHAR* ScriptContent)
{
#if (defined(UNICODE)||defined(_UNICODE))
    WCHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::AnsiToUnicode(ScriptContent,&psz);
    bRet=ExecScript(hHtmlViewer,psz);
    free(psz);
    return bRet;
#else
    return ExecScript(hHtmlViewer,ScriptContent);
#endif
}
//-----------------------------------------------------------------------------
// Name: ExecScriptW
// Object: Exec Script
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall ExecScriptW(HANDLE hHtmlViewer,WCHAR* ScriptContent)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return ExecScript(hHtmlViewer,ScriptContent);
#else
    CHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::UnicodeToAnsi(ScriptContent,&psz);
    bRet=ExecScript(hHtmlViewer,psz);
    free(psz);
    return bRet;
#endif
}


//-----------------------------------------------------------------------------
// Name: ExecScriptEx2
// Object: Exec Script
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL ExecScriptEx2(HANDLE hHtmlViewer,TCHAR* ScriptContent,VARIANT* pScriptResult,EXCEPINFO* pExcepInfo)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->ExecScript(ScriptContent,pScriptResult,pExcepInfo);
}

//-----------------------------------------------------------------------------
// Name: ExecScriptEx2A
// Object: Exec Script
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall ExecScriptEx2A(HANDLE hHtmlViewer,CHAR* ScriptContent,VARIANT* pScriptResult,EXCEPINFO* pExcepInfo)
{
#if (defined(UNICODE)||defined(_UNICODE))
    WCHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::AnsiToUnicode(ScriptContent,&psz);
    bRet=ExecScriptEx2(hHtmlViewer,psz,pScriptResult,pExcepInfo);
    free(psz);
    return bRet;
#else
    return ExecScriptEx2(hHtmlViewer,ScriptContent,pScriptResult,pExcepInfo);
#endif
}
//-----------------------------------------------------------------------------
// Name: ExecScriptEx2W
// Object: Exec Script
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall ExecScriptEx2W(HANDLE hHtmlViewer,WCHAR* ScriptContent,VARIANT* pScriptResult,EXCEPINFO* pExcepInfo)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return ExecScriptEx2(hHtmlViewer,ScriptContent,pScriptResult,pExcepInfo);
#else
    CHAR* psz;
    BOOL bRet;
    CAnsiUnicodeConvert::UnicodeToAnsi(ScriptContent,&psz);
    bRet=ExecScriptEx2(hHtmlViewer,psz,pScriptResult,pExcepInfo);
    free(psz);
    return bRet;
#endif
}


//-----------------------------------------------------------------------------
// Name: SetElementsEventsCallBack
// Object: set simple document events callback
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall SetElementsEventsCallBack(HANDLE hHtmlViewer,pfElementEventsCallBack ElementEventsCallBack,LPVOID UserParam)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->SetElementsEventsCallBack(ElementEventsCallBack,UserParam);
}
//-----------------------------------------------------------------------------
// Name: SetElementsEventsCallBackEx
// Object: set extended document events callback
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
extern "C" __declspec(dllexport) BOOL __stdcall SetElementsEventsCallBackEx(HANDLE hHtmlViewer,pfElementEventsCallBackEx ElementEventsCallBackEx,LPVOID UserParam)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->SetElementsEventsCallBackEx(ElementEventsCallBackEx,UserParam);
}

extern "C" __declspec(dllexport) BOOL __stdcall EnableContextMenu(HANDLE hHtmlViewer,BOOL Enable)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->EnableContextMenu(Enable);
}
extern "C" __declspec(dllexport) BOOL __stdcall EnableSelection(HANDLE hHtmlViewer,BOOL Enable)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->EnableSelection(Enable);
}

IHTMLElement* GetHTMLElement(HANDLE hHtmlViewer,TCHAR* Id)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->GetHTMLElement(Id);
}
extern "C" __declspec(dllexport) IHTMLElement* __stdcall GetHTMLElementA(HANDLE hHtmlViewer,CHAR* Id)
{
#if (defined(UNICODE)||defined(_UNICODE))
    WCHAR* psz;
    IHTMLElement* pElement;
    CAnsiUnicodeConvert::AnsiToUnicode(Id,&psz);
    pElement=GetHTMLElement(hHtmlViewer,psz);
    free(psz);
    return pElement;
#else
    return GetHTMLElement(hHtmlViewer,Id);
#endif
}
extern "C" __declspec(dllexport) IHTMLElement* __stdcall GetHTMLElementW(HANDLE hHtmlViewer,WCHAR* Id)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return GetHTMLElement(hHtmlViewer,Id);
#else
    CHAR* psz;
    IHTMLElement* pElement;
    CAnsiUnicodeConvert::UnicodeToAnsi(Id,&psz);
    pElement=GetHTMLElement(hHtmlViewer,psz);
    free(psz);
    return pElement;
#endif
}
extern "C" __declspec(dllexport) IDispatch* __stdcall GetHTMLDocument(HANDLE hHtmlViewer)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->GetHTMLDocument();
}

extern "C" __declspec(dllexport) BOOL __stdcall TranslateAcceleratorForWebBrowser(HANDLE hHtmlViewer,MSG* pMsg)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->TranslateAccelerator(pMsg);
}

extern "C" __declspec(dllexport) IWebBrowser2* __stdcall GetWebBrowser(HANDLE hHtmlViewer)
{
    CHtmlViewer* pHtmlViewer=GetHtmlViewerObjectFromHandle(hHtmlViewer);
    if (pHtmlViewer==NULL)
        return FALSE;

    return pHtmlViewer->GetWebBrowser();
}