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
#pragma comment (lib,"comctl32")
#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "resource.h"
LRESULT CALLBACK TestWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    InitCommonControls();
    DialogBox(hInstance, (LPCTSTR)IDD_DIALOG1, NULL, (DLGPROC)TestWndProc);

    return 0;
}

int _cdecl StupidAdd(int a,int b) // _cdecl is here only to show API override works with _stdcall or _cdecl
{
    TCHAR szMsg[MAX_PATH];
    _stprintf(szMsg,_T("Adding %d and %d"),a,b);
    MessageBox(NULL,szMsg,_T("Info"),MB_OK);

    return a+b;
}


LRESULT CALLBACK TestWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    TCHAR szMsg[MAX_PATH];
    int val=0;
    switch (uMsg)
    {
    case WM_CLOSE:
         EndDialog(hWnd, LOWORD(wParam));
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDMSGBOX:
            MessageBox(NULL,_T("Text"),_T("Test"),MB_OK);
            break;
        case IDEXEINTERNALCALL:
            val=StupidAdd(5,7);
            _stprintf(szMsg,_T("Result of %d + %d is %d"),5,7,val);
            MessageBox(NULL,szMsg,_T("Add"),MB_OK);
            break;
        }
        break;
    }
    return FALSE;
}