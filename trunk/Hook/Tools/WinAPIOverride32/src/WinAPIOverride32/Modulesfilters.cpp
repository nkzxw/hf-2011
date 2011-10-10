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
// Object: manages the modules filters dialog
//-----------------------------------------------------------------------------

#include "modulesfilters.h"

extern CLinkListSimple* pApiOverrideList;
extern COptions* pOptions;//options object

CModulesFilters::CModulesFilters()
{
    this->pListviewSelectedModules=NULL;
}
CModulesFilters::~CModulesFilters()
{

}


//-----------------------------------------------------------------------------
// Name: ShowFilterDialog
// Object: show the filter dialog box
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
INT_PTR CModulesFilters::ShowFilterDialog(HINSTANCE hInstance,HWND hWndDialog)
{
    // show dialog
    return DialogBoxParam(hInstance,(LPCTSTR)IDD_DIALOG_MODULES_FILTERS,hWndDialog,(DLGPROC)CModulesFilters::WndProc,(LPARAM)this);
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: vars init. Called at WM_INITDIALOG
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CModulesFilters::Init()
{
    this->hwndFilterDialogListBoxHookedProcesses=GetDlgItem(this->hWndFiltersDialog,IDC_LISTBOX_HOOKED_PROCESSES);
    this->pListviewSelectedModules=new CListview(GetDlgItem(this->hWndFiltersDialog,IDC_LISTVIEW_LOGGED_MODULES));
    this->pListviewSelectedModules->SetStyle(TRUE,FALSE,FALSE,TRUE);
    this->pListviewSelectedModules->AddColumn(_T("Module Name"),270,LVCFMT_LEFT);
    this->pListviewSelectedModules->EnableDefaultCustomDraw(FALSE);
    
    // retrieve hooked process list
    TCHAR pszProcName[2*MAX_PATH];
    TCHAR psz[2*MAX_PATH];
    CApiOverride* pApiOverride;
    CLinkListItem* pItem;

    // loop through pApiOverrideList
    pApiOverrideList->Lock(TRUE);
    for (pItem=pApiOverrideList->Head;pItem;pItem=pItem->NextItem)
    {
        pApiOverride=(CApiOverride*)pItem->ItemData;
        // get name and pid
        pApiOverride->GetProcessName(pszProcName,MAX_PATH);
        _stprintf(psz,_T("%s (0x%.8x)"),pszProcName,pApiOverride->GetProcessID());
        // add to listbox
        CListbox::AddStringWithHScrollUpdate(this->hwndFilterDialogListBoxHookedProcesses,psz);
    }
    pApiOverrideList->Unlock();

    // render layout
    this->OnSize();
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CModulesFilters::Close()
{
    delete this->pListviewSelectedModules;
    this->pListviewSelectedModules=NULL;
    EndDialog(this->hWndFiltersDialog,FILTERS_DLG_RES_CANCEL);
}

//-----------------------------------------------------------------------------
// Name: SelectProcess
// Object: Reply to a Select Process button click
//          find pid of selected process, list its modules and select hooked modules
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CModulesFilters::SelectProcess()
{
    TCHAR psz[2*MAX_PATH];
    TCHAR* pPID;
    DWORD dwPID;
    int iSscanfNbRes;
    // find selected process
    LRESULT ItemIndex = SendMessage(this->hwndFilterDialogListBoxHookedProcesses,(UINT) LB_GETCURSEL,0,0);
    if (ItemIndex==LB_ERR)
        return;

    *psz=0;
    SendMessage(this->hwndFilterDialogListBoxHookedProcesses,(UINT)LB_GETTEXT,(WPARAM) ItemIndex,(LPARAM)psz);
    pPID=_tcsrchr(psz,'(');
    dwPID=0;
    // extract pid from selected listbox item text
    iSscanfNbRes=_stscanf(pPID,_T("(0x%x)"),&dwPID);
    if ((dwPID==0)||(iSscanfNbRes!=1))
        return;

    // find associated CApiOverride object (loop inside pApiOverrideList for matching PID)
    this->CurrentpApiOverride=NULL;
    CLinkListItem* pItem;
    pApiOverrideList->Lock(TRUE);
    for(pItem=pApiOverrideList->Head;pItem;pItem=pItem->NextItem)
    {
        // check process ID
        if (dwPID==((CApiOverride*)(pItem->ItemData))->GetProcessID())
        {
            // we found it
            this->CurrentpApiOverride=(CApiOverride*)pItem->ItemData;
            break;
        }
    }
    pApiOverrideList->Unlock();

    if (!this->CurrentpApiOverride)
    {
        // process xxx is no more hooked
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Process 0x") _T("%X") _T(" no more hooked"),dwPID);
        MessageBox(this->hWndFiltersDialog,pszMsg,_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);

        // remove item from list
        SendMessage(this->hwndFilterDialogListBoxHookedProcesses,LB_DELETESTRING,ItemIndex,0);

        return;
    }

    this->GetProcessModules();
}

//-----------------------------------------------------------------------------
// Name: GetProcessModules
// Object: list modules of dwPID and add them to listbox
// Parameters :
//     in  : DWORD dwPID : Process Id from which to retrieve modules
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CModulesFilters::GetProcessModules()
{
    // clear modules list
    this->pListviewSelectedModules->Clear();

    MODULEENTRY32 me32 = {0}; 
    TCHAR* psz;
    HANDLE hModuleSnap =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,this->CurrentpApiOverride->GetProcessID());

    if (hModuleSnap == INVALID_HANDLE_VALUE) 
        return; 
    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32); 
 
    // Walk the module list of the process
    if (!Module32First(hModuleSnap, &me32))
        return;
    do 
    { 
        psz=me32.szExePath;
        // add name to the module list
        this->pListviewSelectedModules->AddItem(psz);
    } 
    while (Module32Next(hModuleSnap, &me32)); 
 
    // clean up the snapshot object. 
    CloseHandle (hModuleSnap); 

    ////////////////////////
    // select hooked modules
    ////////////////////////

    // select all modules
    this->pListviewSelectedModules->SelectAll();

    TCHAR* pszNotLoggedModuleName=NULL;

    TCHAR** pNotLoggedArray=NULL;
    DWORD dwNbNotLoggedModules=0;
    if (!this->CurrentpApiOverride->GetNotLoggedModuleList(&pNotLoggedArray,&dwNbNotLoggedModules))
        return;

    int ItemIndex;
    // for each not logged module, unselect it
    for (DWORD dwCnt=0;dwCnt<dwNbNotLoggedModules;dwCnt++)
    {
        // get not logged module full path
        pszNotLoggedModuleName=(TCHAR*)&((PBYTE)pNotLoggedArray)[dwCnt*MAX_PATH*sizeof(TCHAR)];

        // find item index in list
        
        ItemIndex = this->pListviewSelectedModules->Find(pszNotLoggedModuleName);
        if (ItemIndex==-1)
            continue;

        // unselect it
        this->pListviewSelectedModules->SetSelectedState(ItemIndex,FALSE);
    }
    if (pNotLoggedArray)
        delete[] pNotLoggedArray;

    this->pListviewSelectedModules->ReSort();
}

//-----------------------------------------------------------------------------
// Name: SelectModules
// Object: Reply to the Apply button click
//          Apply module logging filters : selected modules that will be logged, 
//          and unselected modules that won't be logged any more until another configuration
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CModulesFilters::SelectModules()
{
    // assume a process has been selected
    LRESULT ItemIndex = SendMessage(this->hwndFilterDialogListBoxHookedProcesses,(UINT) LB_GETCURSEL,0,0);
    if (ItemIndex==LB_ERR)
        return;

    LONG ulCnt;
    TCHAR psz[2*MAX_PATH];
    int NbItems=this->pListviewSelectedModules->GetItemCount();

    BOOL bModuleShouldBeLogged;

    // use set filtering way to FILTERING_WAY_ONLY_SPECIFIED_MODULES
    this->CurrentpApiOverride->SetModuleFilteringWay(FILTERING_WAY_ONLY_SPECIFIED_MODULES);

    // for each item of SelectedModules listbox
    for (ulCnt=0;ulCnt<NbItems;ulCnt++)
    {

        bModuleShouldBeLogged = this->pListviewSelectedModules->IsItemSelected(ulCnt);

        // get text
        this->pListviewSelectedModules->GetItemText(ulCnt,psz,2*MAX_PATH);

        if (bModuleShouldBeLogged)
            // if selected signal to log module
            this->CurrentpApiOverride->SetModuleLogState(psz,TRUE);
        else
            // else signal to not log module
            this->CurrentpApiOverride->SetModuleLogState(psz,FALSE);
    }

    // update view
    this->GetProcessModules();

    // show information message
    MessageBox(this->hWndFiltersDialog,_T("Changes applied"),_T("Information"),MB_ICONINFORMATION|MB_OK|MB_TOPMOST);
    
}

//-----------------------------------------------------------------------------
// Name: OnSizing
// Object: check dialog box size
// Parameters :
//     in out  : RECT* pRect : pointer to dialog rect
//     return : 
//-----------------------------------------------------------------------------
void CModulesFilters::OnSizing(RECT* pRect)
{
    // check min width and min height
    if ((pRect->right-pRect->left)<CCModulesFilters_DIALOG_MIN_WIDTH)
        pRect->right=pRect->left+CCModulesFilters_DIALOG_MIN_WIDTH;
    if ((pRect->bottom-pRect->top)<CCModulesFilters_DIALOG_MIN_HEIGHT)
        pRect->bottom=pRect->top+CCModulesFilters_DIALOG_MIN_HEIGHT;
}

//-----------------------------------------------------------------------------
// Name: OnSize
// Object: Resize controls
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CModulesFilters::OnSize()
{
    RECT Rect;
    RECT RectApply;
    RECT RectUncheck;
    RECT RectProcessesGroup;
    RECT RectModulesGroup;
    RECT RectWindow;
    HWND hItem;
    CDialogHelper::GetClientWindowRect(this->hWndFiltersDialog,this->hWndFiltersDialog,&RectWindow);


    ////////////////////////////////////
    // position ok close buttons
    ////////////////////////////////////

    // ok 
    hItem=::GetDlgItem(this->hWndFiltersDialog,IDOK);
    CDialogHelper::GetClientWindowRect(this->hWndFiltersDialog,hItem,&Rect);
    ::SetWindowPos(hItem,0,
                    (RectWindow.right - RectWindow.left - 2*(Rect.right-Rect.left) - CModulesFilters_SPACE_BETWEEN_BUTTONS)/2,
                    RectWindow.bottom - (Rect.bottom-Rect.top) - CModulesFilters_SPACE_BETWEEN_CONTROLS,
                    0,
                    0,
                    SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);

    // get new ok pos
    CDialogHelper::GetClientWindowRect(this->hWndFiltersDialog,hItem,&RectApply);

    // close is right to ok
    // position close
    hItem=::GetDlgItem(this->hWndFiltersDialog,IDCANCEL);
    ::SetWindowPos(hItem,0,
                    RectApply.right + CModulesFilters_SPACE_BETWEEN_BUTTONS,
                    RectApply.top,
                    0,
                    0,
                    SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);


    ////////////////////////////////
    // hooked process group
    ////////////////////////////////
    hItem=::GetDlgItem(this->hWndFiltersDialog,IDC_STATIC_GROUP_MODULES_FILTERS_SELECT_HOOKED_PROCESS);
    CDialogHelper::GetClientWindowRect(this->hWndFiltersDialog,hItem,&RectProcessesGroup);
    ::SetWindowPos(hItem,0,
                    0,
                    0,
                    RectProcessesGroup.right - RectProcessesGroup.left,
                    RectApply.top - RectProcessesGroup.top - CModulesFilters_SPACE_BETWEEN_CONTROLS,
                    SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    
    CDialogHelper::GetClientWindowRect(this->hWndFiltersDialog,hItem,&RectProcessesGroup);
    CDialogHelper::GetClientWindowRect(this->hWndFiltersDialog,hwndFilterDialogListBoxHookedProcesses,&Rect);
    ::SetWindowPos(hwndFilterDialogListBoxHookedProcesses,0,
                    0,
                    0,
                    RectProcessesGroup.right - RectProcessesGroup.left- 2*(Rect.left - RectProcessesGroup.left),
                    RectProcessesGroup.bottom - Rect.top - CModulesFilters_SPACE_BETWEEN_CONTROLS,
                    SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);

    ////////////////////////////////
    // module group
    ////////////////////////////////
    hItem=::GetDlgItem(this->hWndFiltersDialog,IDC_STATIC_GROUP_MODULES_FILTERS_SELECT_MODULES);
    CDialogHelper::GetClientWindowRect(this->hWndFiltersDialog,hItem,&RectModulesGroup);
    ::SetWindowPos(hItem,0,
                    0,
                    0,
                    RectWindow.right - RectModulesGroup.left - CModulesFilters_SPACE_BETWEEN_BUTTONS,
                    RectApply.top - RectModulesGroup.top - CModulesFilters_SPACE_BETWEEN_CONTROLS,
                    SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::GetClientWindowRect(this->hWndFiltersDialog,hItem,&RectModulesGroup);


    hItem=::GetDlgItem(this->hWndFiltersDialog,IDC_BUTTON_MODULES_FILTERS_UNCHECK_ALL);
    CDialogHelper::GetClientWindowRect(this->hWndFiltersDialog,hItem,&Rect);
    ::SetWindowPos(hItem,0,
                    RectModulesGroup.right - (Rect.right-Rect.left) - CModulesFilters_SPACE_BETWEEN_CONTROLS,
                    RectModulesGroup.bottom - (Rect.bottom-Rect.top) - CModulesFilters_SPACE_BETWEEN_CONTROLS,
                    0,
                    0,
                    SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    

    CDialogHelper::GetClientWindowRect(this->hWndFiltersDialog,hItem,&RectUncheck);
    hItem=::GetDlgItem(this->hWndFiltersDialog,IDC_BUTTON_MODULES_FILTERS_CHECK_ALL);
    ::SetWindowPos(hItem,0,
                    RectUncheck.left - (RectUncheck.right-RectUncheck.left) - CModulesFilters_SPACE_BETWEEN_BUTTONS,
                    RectUncheck.top,
                    0,
                    0,
                    SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);

    hItem = this->pListviewSelectedModules->GetControlHandle();
    CDialogHelper::GetClientWindowRect(this->hWndFiltersDialog,hItem,&Rect);
    ::SetWindowPos(hItem,0,
                    0,
                    0,
                    RectModulesGroup.right - RectModulesGroup.left- 2*(Rect.left - RectModulesGroup.left),
                    RectUncheck.top - Rect.top - CModulesFilters_SPACE_BETWEEN_CONTROLS,
                    SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);

    // redraw dialog
    CDialogHelper::Redraw(this->hWndFiltersDialog);
}

//-----------------------------------------------------------------------------
// Name: ModulesFiltersWndProc
// Object: dialog callback of the module Filters dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CModulesFilters::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            CModulesFilters* pModulesFilters=(CModulesFilters*)lParam;
            pModulesFilters->hWndFiltersDialog=hWnd;

            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pModulesFilters);

            pModulesFilters->Init();
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_MODULES_FILTERS);
        }
        break;
    case WM_CLOSE:
        {
            CModulesFilters* pModulesFilters=((CModulesFilters*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pModulesFilters)
                break;
            pModulesFilters->Close();
        }
        break;
    case WM_COMMAND:
        {
            CModulesFilters* pModulesFilters=((CModulesFilters*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pModulesFilters)
                break;

            switch (LOWORD(wParam))
            {
            case IDOK:
                pModulesFilters->SelectModules();
                break;
            case IDCANCEL:
                pModulesFilters->Close();
                break;
            case IDC_BUTTON_MODULES_FILTERS_CHECK_ALL:
                pModulesFilters->pListviewSelectedModules->SelectAll();
                break;
            case IDC_BUTTON_MODULES_FILTERS_UNCHECK_ALL:
                pModulesFilters->pListviewSelectedModules->UnselectAll();
                break;
            }

            if ((HWND)lParam==pModulesFilters->hwndFilterDialogListBoxHookedProcesses)
            {
                switch (HIWORD(wParam))
                {
                    case LBN_SELCHANGE:
                            pModulesFilters->SelectProcess();
                        break;
                }
            }
        }
        break;
    case WM_NOTIFY:
        {
            CModulesFilters* pModulesFilters=((CModulesFilters*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pModulesFilters)
                break;
            if (pModulesFilters->pListviewSelectedModules)
            {
                if(pModulesFilters->pListviewSelectedModules->OnNotify(wParam, lParam))
                    return TRUE;
            }
        }
        break;

    case WM_SIZING:
        {
            CModulesFilters* pModulesFilters=((CModulesFilters*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pModulesFilters)
                break;
            pModulesFilters->OnSizing((RECT*)lParam);
        }
        break;
    case WM_SIZE:
        {
            CModulesFilters* pModulesFilters=((CModulesFilters*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pModulesFilters)
                break;
            pModulesFilters->OnSize();
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}