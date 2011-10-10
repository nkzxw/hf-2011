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
// Object: manages the compare dialog
//-----------------------------------------------------------------------------


#pragma once


#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "resource.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../Tools/GUI/Listview/Listview.h"
#include "../Tools/LinkList/LinkListSimple.h"
#include "../tools/Process/APIOverride/ApiOverride.h"
#include "../tools/Process/APIOverride/InterProcessCommunication.h"

#define CCOMPARELOGS_PROPERTIES_BACKGROUND_COLOR RGB(247,247,247)
#define CCOMPARELOGS_DIFFERENT_VALUES_COLOR RGB(255,174,174)

class CCompareLogs
{
private:
    LOG_ENTRY* pLog1;
    LOG_ENTRY* pLog2;
    CListview* pListview;

    HWND hWndDialog;
    void Close();
    void Init();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT ProcessListViewCustomDraw (LPARAM lParam);
    BOOL* pPropertyDontMatch;
    DWORD pPropertyDontMatchSize;
    void Compare();
    void CompareParameterField(int Index,TCHAR* str1,TCHAR* str2,CLinkListSimple* pPropertyDontMatchList);
    void CompareHexValue(int Index,TCHAR* PropertyName,DWORD Value1,DWORD Value2,CLinkListSimple* pPropertyDontMatchList);
    void CompareHexValue(int Index,TCHAR* PropertyName,DWORD Value1,DWORD Value2,BOOL bValue1Significant,BOOL bValue2Significant,CLinkListSimple* pPropertyDontMatchList);
    void CompareHexValue(int Index,TCHAR* PropertyName,PBYTE Value1,PBYTE Value2,CLinkListSimple* pPropertyDontMatchList);
    void CompareHexValue(int Index,TCHAR* PropertyName,PBYTE Value1,PBYTE Value2,BOOL bValue1Significant,BOOL bValue2Significant,CLinkListSimple* pPropertyDontMatchList);
    void Resize();
public:
    CCompareLogs(void);
    ~CCompareLogs(void);
    static void ShowDialog(HINSTANCE hInstance,HWND hWndDialog,LOG_ENTRY* pLog1,LOG_ENTRY* pLog2);
};
