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
// Object: manages the stats dialog
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "resource.h"
#include "structsanddefines.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../Tools/GUI/ListView/ListView.h"
#include "../Tools/LinkList/LinkList.h"
#include "../tools/Process/APIOverride/ApiOverride.h"
#include "../tools/Process/APIOverride/InterProcessCommunication.h"
#include "Search.h"

#define CLogsStatsUI_DIALOG_MIN_HEIGHT 50
#define CLogsStatsUI_DIALOG_MIN_WIDTH 100

class CLogsStatsUI
{
private:
    CSearch Search;
    typedef struct tagStatStruct
    {
        TCHAR ApiName[MAX_PATH];
        TCHAR ModuleName[MAX_PATH];
        DWORD Count;
        DWORD DurationSum;
    }STATSTRUCT,*PSTATSTRUCT;

    HINSTANCE hInstance;
    HWND hWndDialog;
    HWND hWndParentDialog;
    CListview* pListview;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void Close();
    void OnSize();
    void OnSizing(RECT* pRect);
    void Init(HWND hwnd);
    void MakeStats();
    static DWORD WINAPI ModelessDialogThread(PVOID lParam);
    static void PopUpMenuItemClickCallbackStatic(UINT MenuID,LPVOID UserParam);
    void PopUpMenuItemClickCallback(UINT MenuID);

    UINT MenuIdCopyFunctionName;
    UINT MenuIdCopyDllName;
    UINT MenuIdRemoveAllTheseLogs;
    UINT MenuIdFindFirst;
    UINT MenuIdFindNext;
    UINT MenuIdFindPrevious;
    UINT MenuIdHighLight;
    CLogsStatsUI(HINSTANCE hInstance);
    ~CLogsStatsUI();
public:
    static void Show(HINSTANCE Instance,HWND hWndParent);
};
