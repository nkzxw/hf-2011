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
// Object: manages the Remote Call dialog
//-----------------------------------------------------------------------------


#pragma once


#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "resource.h"
#include "StructsAndDefines.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../Tools/LinkList/LinkList.h"
#include "../Tools/LinkList/LinkListSimple.h"
#include "../tools/Process/APIOverride/ApiOverride.h"
#include "../tools/GUI/ListView/Listview.h"
#include "../Tools/String/TrimString.h"
#include "../Tools/String/StringConverter.h"
#include "../Tools/String/StrToHex.h"
#include "remotecallresult.h"
#include "parseparametersforremotecall.h"

class CRemoteCall
{
private:
    CListview* pListview;
    HWND hWndDialog;
    HWND hWndComboCallingConvention;
    HINSTANCE hInstance;

    void Call();
    BOOL GetValue(int ControlID,DWORD* pValue);
    void UpdateHookedProcessList();
    void OnCallingConventionChange();

    void Close();
    void Init();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static DWORD WINAPI ModelessDialogThread(PVOID lParam);

public:
    CRemoteCall(void);
    ~CRemoteCall(void);
    static void ShowDialog(HINSTANCE hInstance,HWND hWndDialog);
};
