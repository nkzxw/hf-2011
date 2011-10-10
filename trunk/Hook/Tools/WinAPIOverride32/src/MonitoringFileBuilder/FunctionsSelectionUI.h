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
// Object: manages the functions selection UI
//-----------------------------------------------------------------------------

#pragma once

#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include <Dbghelp.h>
#pragma comment (lib,"Dbghelp")

#include "resource.h"
#include "../Tools/pe/pe.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../Tools/GUI/ListView/ListView.h"
#include "../Tools/string/ansiunicodeconvert.h"
#include "../Tools/String/WildCharCompare.h"
#include "../Tools/Process/APIOverride/HookNet/_NetMonitoringFileGenerator/NetMonitoringFileGenerator.h"

#define FUNCTION_SELECTION_UI_FUNCTION_HELPER_TIME_TO_SHOW 600 // time in ms

class CFunctionsSelectionUI
{
private:
    typedef struct tagImportExportFunctions
    {
        BOOL bOrdinal;
        DWORD Ordinal;
        TCHAR Name[3*MAX_PATH];
        TCHAR UndecoratedName[2*MAX_PATH];
        BOOL bSelected;
    }IMPORT_EXPORT_FUNCTIONS,*PIMPORT_EXPORT_FUNCTIONS;

    typedef struct tagLibrary
    {
        TCHAR Name[MAX_PATH];
        DWORD NbFunctions;
        IMPORT_EXPORT_FUNCTIONS* pFunctions;
    }LIBRARY,*PLIBRARY;

    HWND hWndDialog;
    HINSTANCE hInstance;

    CNetMonitoringFileGenerator* pNetMonitoringFileGenerator;
    CPE* pPE;
    BOOL bExport;
    CListview* pListViewLibraries;
    CListview* pListViewFunctions;
    CFunctionsSelectionUI::LIBRARY* pLibraries;
    int pLibrariesSize;
    int pLibrariesCurrentIndex;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void Close(INT_PTR DlgRes);
    void Init(HWND hwnd);

    void CommonConstructor();
    void ApplyChanges();
    void CheckAllImportedLibs();
    void UncheckAllImportedLibs();
    void CheckAll();
    void UncheckAll();
    void CheckSelected();
    void UncheckSelected();
    static void SelectItemCallbackStatic(int ItemIndex,int SubItemIndex,LPVOID UserParam);
    void SelectItemCallback(int ItemIndex,int SubItemIndex);
    static void UnselectItemCallbackStatic(int ItemIndex,int SubItemIndex,LPVOID UserParam);
    void UnselectItemCallback(int ItemIndex,int SubItemIndex);
    void ShowFunctionsOfLibrary(int IndexOfpLibraries);
    BOOL IsLibSelected(TCHAR* LibName);
    BOOL IsFuncSelected(TCHAR* FuncName,DWORD Ordinal);
    BOOL IsListviewNameSelected(CListview* pListview,TCHAR* Name,int SubItemIndex);

    HANDLE evtComboTextChange;
    HANDLE evtClose;
    HANDLE FinderHelperThread;
    BOOL bClosed;
    HWND hWndComboSearch;
    void SearchFunction();
    void SelectSearchedFunctionInListView();
    static DWORD WINAPI FunctionFinderHelper(LPVOID lpParameter);
    void FunctionFinderHelper();
public:
    INT_PTR Show(HINSTANCE Instance,HWND hWndParent);
    CFunctionsSelectionUI(CNetMonitoringFileGenerator* pNetMonitoringFileGenerator);
    CFunctionsSelectionUI(CPE* pPE,BOOL bExport);
    ~CFunctionsSelectionUI();
};
