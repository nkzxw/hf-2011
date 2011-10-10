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
// Object: manages the processes filters dialog
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <Tlhelp32.h>
#include <stdio.h>

#include "resource.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../tools/GUI/ListView/ListView.h"
#include "../Tools/String/WildCharCompare.h"
#include "../Tools/String/MultipleElementsParsing.h"
#include "../tools/Process/ProcessHelper/ProcessHelper.h"
#include "../Tools/APIError/APIError.h"

#include "Options.h"


#define FILTERS_INACTIVE_PROCESSES_PID  0 // Inactive Processes PID not a hookable process
#define FILTERS_SYSTEM_PID              4 // System PID not a hookable process

class CFilters
{
public:
    enum tagDlgRes
    {
        // ShowFilterDialog results
        DLG_RES_FAILED=0,
        DLG_RES_OK,         // user clicks OK
        DLG_RES_CANCEL      // user clicks Cancel
    };
    DWORD* pdwFiltersCurrentPorcessID;
    DWORD pdwFiltersCurrentPorcessIDSize;

    DWORD* pdwFiltersNewPorcessIDToWatch;
    DWORD pdwFiltersNewPorcessIDToWatchSize;
    DWORD* pdwFiltersPorcessIDToRelease;
    DWORD pdwFiltersPorcessIDToReleaseSize;

    CFilters(COptions* pOptions);
    ~CFilters();
    INT_PTR ShowFilterDialog(HINSTANCE hInstance,HWND hWndDialog);
    BOOL DoesProcessNameMatchFilters(TCHAR* ProcessName);
    BOOL DoesParentIDMatchFilters(DWORD dwParentID);

private:
    COptions* pOptions;
    BOOL SetParentProcessIdFilters();
    BOOL SetInclusionFilters();
    BOOL SetExclusionFilters();

    HWND hWndFiltersDialog;

    DWORD* pdwFiltersParentPorcessID;
    DWORD pdwFiltersParentPorcessIDSize;
    TCHAR** pszFiltersInclusion;
    DWORD pszFiltersInclusionSize;
    TCHAR** pszFiltersExclusion;
    DWORD pszFiltersExclusionSize;

    CListview* pListView;

    void RefreshProcessesList();
    void SaveFiltersSettings();
    void FreeMemory();
    void ReloadOptions();
    void StoreOptions();
    void ReSelectProcess();
    void Close(tagDlgRes DlgRes);

    static LRESULT CALLBACK FiltersWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

