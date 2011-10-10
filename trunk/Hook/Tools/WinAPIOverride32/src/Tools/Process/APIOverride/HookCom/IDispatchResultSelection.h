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
// Object: manages the Com IDispatchResultSelection dialog
//-----------------------------------------------------------------------------
#pragma once

class CIDispatchResultSelection;
#include "HookedClass.h"

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "resource.h"
#include "../../../GUI/Dialog/DialogHelper.h"
#include "../../../LinkList/LinkListSimple.h"
#include "../../../GUI/ListView/Listview.h"
#include "GetIDispatchWinApiOverrideFunctionsRepresentation.h"

class CIDispatchResultSelection
{
private:
    HWND hWndDialog;
    CLinkListItem* pItemDialog;
    CHookedClass* pHookedClass;
    CListview*    pListview;
    static BOOL IDispatchFunctionsRepresentation(TCHAR* pszFuncDesc,CMethodInfo* pMethodInfo,LPVOID UserParam);

    void OnOKClick();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void Close();
    void Init(HWND hwnd);
    static DWORD WINAPI ModelessDialogThread(PVOID lParam);
public:
    static void Show(CHookedClass* pHookedClass);
    CIDispatchResultSelection(CHookedClass* pHookedClass);
    ~CIDispatchResultSelection(void);
};
