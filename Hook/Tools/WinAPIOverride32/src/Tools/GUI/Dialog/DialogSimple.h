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


 
//// Example of use
//
//#define IDC_BUTTON_1 1234
//#define IDC_STATIC_1 1235
//#define IDC_EDIT_1 1236
//
//void OnButton1Click (CDialogSimple* pDialogSimple,PVOID UserParam)
//{
//    MessageBox(pDialogSimple->GetControlHandle(),_T("Button 1 Click"),_T("Test"),MB_ICONINFORMATION);
//}
//CDialogSimple::COMMANDS_CALLBACK_ARRAY_ITEM g_ArrayCommands[]=
//{
//    {IDC_BUTTON_1,OnButton1Click}
//};
//
//void OnCreate(CDialogSimple* pDialogSimple,PVOID UserParam)
//{
//    CDialogHelper::CreateButton(pDialogSimple->GetControlHandle(),5,5,50,25,IDC_BUTTON_1,_T("button"));
//    CDialogHelper::CreateStatic(pDialogSimple->GetControlHandle(),75,0,25,20,IDC_STATIC_1,_T("static"));
//    CDialogHelper::CreateEdit(pDialogSimple->GetControlHandle(),100,0,100,20,IDC_EDIT_1,_T("edit"));
//    pDialogSimple->SetCommandsCallBacks(g_ArrayCommands,sizeof(g_ArrayCommands)/sizeof(g_ArrayCommands[0]),NULL);
//}
//
//int WINAPI WinMain(HINSTANCE hInstance,
//                   HINSTANCE hPrevInstance,
//                   LPSTR lpCmdLine,
//                   int nCmdShow
//                   )
//{
//    // to enable XP visual style
//    InitCommonControls();
//
//    CDialogSimple DialogSimple(200,100,TRUE);
//    DialogSimple.SetCreateCallback(OnCreate,0);
//    DialogSimple.Show(hInstance,NULL,_T("Title"),hInstance,IDI_ICON1);
//}
#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#define CDialogSimpleClassName _T("CDialogSimpleClassName")

class CDialogSimple
{
public:
    typedef void (*pfCreateCallback) (CDialogSimple* pDialogSimple,PVOID UserParam);
    typedef void (*pfDestroyCallback) (CDialogSimple* pDialogSimple,PVOID UserParam);
    typedef void (*pfCommandCallback) (CDialogSimple* pDialogSimple,PVOID UserParam);
    typedef void (*pfNotifyCallback) (CDialogSimple* pDialogSimple,WPARAM wParam, LPARAM lParam,PVOID UserParam);

    typedef struct tagCommandsCallbackArrayItem
    {
        WORD CommandID;
        pfCommandCallback pCommandCallback;
    }COMMANDS_CALLBACK_ARRAY_ITEM,*PCOMMANDS_CALLBACK_ARRAY_ITEM;

    CDialogSimple(DWORD Width,DWORD Height,BOOL bModal);
    CDialogSimple(DWORD x,DWORD y,DWORD Width,DWORD Height,BOOL bModal);
    ~CDialogSimple(void);
    void Show(HINSTANCE hInstance,HWND hParent,TCHAR* pszWindowTitle);
    void Show(HINSTANCE hInstance,HWND hParent,TCHAR* pszWindowTitle,HMODULE hModule, DWORD IDIcon);
    void Show(HINSTANCE hInstance,HWND hParent,TCHAR* pszWindowTitle,HICON hIcon);
    BOOL Close();
    BOOL Close(INT_PTR DialogResult);
    HWND GetControlHandle();
    INT_PTR GetDialogResult();

    BOOL SetCreateCallback(pfCreateCallback pCreateCallback,PVOID UserParam);
    BOOL SetDestroyCallback(pfDestroyCallback pDestroyCallback,PVOID UserParam);
    BOOL SetCommandsCallBacks(COMMANDS_CALLBACK_ARRAY_ITEM* Array,DWORD NbItems,PVOID UserParam);
    BOOL SetNotifyCallback(pfNotifyCallback pNotifyCallback,PVOID UserParam);

private:
    
    HINSTANCE hInstance;
    HWND hParentHandle; // parent window handle
    HWND hWndDialog;    // current dialog handle

    TCHAR* pszWindowTitle;
    HICON hIcon;
    int x;
    int y;
    DWORD Width;
    DWORD Height;
    BOOL CenterWindow;
    BOOL bModal;
    HFONT hDefaultFont;
    INT_PTR DialogResult;


    pfNotifyCallback NotifyCallback;
    PVOID NotifyUserParam;
    pfCreateCallback CreateCallback;
    PVOID CreateUserParam;
    pfDestroyCallback DestroyCallback;
    PVOID DestroyUserParam;
    COMMANDS_CALLBACK_ARRAY_ITEM* CommandCallbackArray;
    DWORD CommandCallbackArraySize;
    PVOID CommandUserParam;


    void OnCreate();
    void OnDestroy();

    static DWORD WINAPI ModelessDialogThread(PVOID lParam);
    static CDialogSimple* GetAssociatedObject(HWND hWndDialog);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void CommonConstructor();
};
