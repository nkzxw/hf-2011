/*
Copyright (C) 2010 Jacquelin POTIER <jacquelin.potier@free.fr>
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

#pragma once

#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501
#endif

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#define CDialogBase_SHELL_DLL _T("shell32.dll")

class CDialogBase
{
private:
    HWND hWndDialog;
    int IconResourceId;
    HINSTANCE hInstance;
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static CDialogBase* GetAssociatedObject(HWND hWndDialog);
    BOOL bDragAcceptFileRequired; // if we have to enable drag accept file
    BOOL bDragAcceptFileStatusEnabled;// if drag accept file is already enabled
public:
    CDialogBase(void);
    virtual ~CDialogBase(void);

    FORCEINLINE virtual void OnInit(){}
    FORCEINLINE virtual void OnClose(){}
    FORCEINLINE virtual void OnCommand(WPARAM wParam,LPARAM lParam){UNREFERENCED_PARAMETER(wParam);UNREFERENCED_PARAMETER(lParam);}
    FORCEINLINE virtual void OnNotify(WPARAM wParam,LPARAM lParam){UNREFERENCED_PARAMETER(wParam);UNREFERENCED_PARAMETER(lParam);}
    FORCEINLINE virtual void OnDropFiles(WPARAM wParam,LPARAM lParam){UNREFERENCED_PARAMETER(wParam);UNREFERENCED_PARAMETER(lParam);}
    FORCEINLINE virtual void OnMouseDown(WPARAM wParam,LPARAM lParam){UNREFERENCED_PARAMETER(wParam);UNREFERENCED_PARAMETER(lParam);}
    FORCEINLINE virtual void OnMouseUp(WPARAM wParam,LPARAM lParam){UNREFERENCED_PARAMETER(wParam);UNREFERENCED_PARAMETER(lParam);}
    FORCEINLINE virtual void OnMouseMove(WPARAM wParam,LPARAM lParam){UNREFERENCED_PARAMETER(wParam);UNREFERENCED_PARAMETER(lParam);}
    FORCEINLINE virtual LRESULT OnMessage(UINT uMsg,WPARAM wParam,LPARAM lParam){UNREFERENCED_PARAMETER(uMsg);UNREFERENCED_PARAMETER(wParam);UNREFERENCED_PARAMETER(lParam);return FALSE;}

    INT_PTR Show(HINSTANCE hInstance,HWND hWndParentDialog,int ResourceId,int IconResourceId);
    void Close(int Code = 0);

    FORCEINLINE HWND GetControlHandle(){return this->hWndDialog;}
    FORCEINLINE HINSTANCE GetInstance(){return this->hInstance;}
    FORCEINLINE HWND GetDlgItem(IN int nIDDlgItem){return ::GetDlgItem(this->hWndDialog,nIDDlgItem);}
    FORCEINLINE LRESULT SendDlgMessage(IN int nIDDlgItem,IN UINT message,IN WPARAM wParam,IN LPARAM lParam){return ::SendMessage(::GetDlgItem(this->hWndDialog,nIDDlgItem),message,wParam,lParam);}
    FORCEINLINE LRESULT PostDlgMessage(IN int nIDDlgItem,IN UINT message,IN WPARAM wParam,IN LPARAM lParam){return ::PostMessage(::GetDlgItem(this->hWndDialog,nIDDlgItem),message,wParam,lParam);}

    FORCEINLINE BOOL SetDlgItemInt(IN int nIDDlgItem,IN UINT uValue,IN BOOL bSigned){return ::SetDlgItemInt(this->hWndDialog, nIDDlgItem, uValue, bSigned);}
    FORCEINLINE BOOL SetDlgItemText(IN int nIDDlgItem,LPCTSTR lpszString){return ::SetWindowText(::GetDlgItem(this->hWndDialog,nIDDlgItem), lpszString);}
    // nMaxCount : Specifies the maximum number of TCHARs to be copied, including the terminating null character
    FORCEINLINE int GetDlgItemText(IN int nIDDlgItem,OUT LPTSTR lpString,IN int nMaxCount){return ::GetWindowText(::GetDlgItem(this->hWndDialog,nIDDlgItem), lpString,nMaxCount);}
    // The return value is the length of the text in TCHARs, not including the terminating null character
    FORCEINLINE int GetDlgItemTextLength(IN int nIDDlgItem){return ::GetWindowTextLength(::GetDlgItem(this->hWndDialog,nIDDlgItem));}

    FORCEINLINE BOOL IsDlgButtonChecked(IN int nIDDlgItem){return (::SendMessage(::GetDlgItem(this->hWndDialog,nIDDlgItem),BM_GETCHECK,0,0)!=BST_UNCHECKED);}
    FORCEINLINE LRESULT SetDlgButtonCheckState(IN int nIDDlgItem,IN BOOL bChecked){return ::SendMessage(::GetDlgItem(this->hWndDialog,nIDDlgItem),BM_SETCHECK,bChecked ? BST_CHECKED : BST_UNCHECKED,0);}
    FORCEINLINE BOOL EnableDlgItem(IN int nIDDlgItem,IN BOOL bEnable){return ::EnableWindow(::GetDlgItem(this->hWndDialog,nIDDlgItem),bEnable);}

    FORCEINLINE HICON LoadIcon(int IconId,int Width,int Height){return (HICON)::LoadImage(this->hInstance,MAKEINTRESOURCE(IconId),IMAGE_ICON,Width,Height,LR_DEFAULTCOLOR);}
    FORCEINLINE LRESULT SetStaticIcon(int StaticControlId,HICON hIcon){return ::SendMessage(::GetDlgItem(this->hWndDialog,StaticControlId),STM_SETICON,(WPARAM)hIcon,0);}
    
    FORCEINLINE BOOL SetCaption(TCHAR* DlgCaption){return ::SetWindowText(this->hWndDialog, DlgCaption);}
    
    FORCEINLINE BOOL Hide(){return ::ShowWindow(this->hWndDialog,SW_HIDE);}
    FORCEINLINE BOOL Hide(IN int nIDDlgItem){return ::ShowWindow(this->GetDlgItem(nIDDlgItem),SW_HIDE);}
    FORCEINLINE BOOL Hide(IN HWND hWnd){return ::ShowWindow(hWnd,SW_HIDE);}

    FORCEINLINE BOOL Show(){return ::ShowWindow(this->hWndDialog,SW_SHOWNORMAL);}
    FORCEINLINE BOOL Show(IN int nIDDlgItem){return ::ShowWindow(this->GetDlgItem(nIDDlgItem),SW_SHOWNORMAL);}
    FORCEINLINE BOOL Show(IN HWND hWnd){return ::ShowWindow(hWnd,SW_SHOWNORMAL);}


    void EnableDragAndDrop(BOOL bEnable);

    enum ReportMessageTypes
    {
        ReportMessageType_ERROR,
        ReportMessageType_WARNING,
        ReportMessageType_INFO
    };
    void ReportMessage(TCHAR* pszMsg, ReportMessageTypes MsgType);
    void ReportError(TCHAR* pszMsg);
};
