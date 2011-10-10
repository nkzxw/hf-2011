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
// Object: manages the COM components dialog
//-----------------------------------------------------------------------------


#pragma once


#include <windows.h>

#include "resource.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../tools/GUI/ListView/Listview.h"
#include "../tools/String/WildCharCompare.h"

#define CComComponents_DIALOG_MIN_WIDTH 300
#define CComComponents_DIALOG_MIN_HEIGHT 200

class CComComponents
{
private:
    HWND hWndDialog;
    
    void Init();
    void Close();
    void OnSize();
    void OnSizing(RECT* pRect);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    CListview* pListView;
    BOOL SearchForCLSID;
    BOOL SearchForName;
    BOOL SearchForProgID;
    TCHAR szSearchedString[MAX_PATH];
    HANDLE hParsingFinished;
    BOOL bStopParsing;
    HINSTANCE hInstance;
    HANDLE hParentHandle;
    BOOL bClosing;

    void Find();
    void FindPrevious(int StartItemIndex);
    void FindNext(int StartItemIndex);
    void NoItemFoundMessage();
    BOOL UpdateSearchFields();
    BOOL DoesItemMatch(int ItemIndex);
    static DWORD WINAPI FillCOMList(LPVOID lpParam);
    static DWORD WINAPI WaitingClose(LPVOID lpParam);
    static DWORD WINAPI ModelessDialogThread(PVOID lParam);
public:
    CComComponents(void);
    ~CComComponents(void);
    static void Show(HINSTANCE hInstance,HWND hWndDialog);
};
