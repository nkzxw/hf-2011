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

#include "filters.h"

CFilters::CFilters(COptions* pOptions)
{
    this->hWndFiltersDialog=NULL;

    this->pdwFiltersCurrentPorcessID=NULL;
    this->pdwFiltersParentPorcessID=NULL;
    this->pszFiltersInclusion=NULL;
    this->pszFiltersExclusion=NULL;
    this->pdwFiltersNewPorcessIDToWatch=NULL;
    this->pdwFiltersPorcessIDToRelease=NULL;

    this->pdwFiltersCurrentPorcessIDSize=0;
    this->pdwFiltersParentPorcessIDSize=0;
    this->pszFiltersInclusionSize=0;
    this->pszFiltersExclusionSize=0;
    this->pdwFiltersNewPorcessIDToWatchSize=0;
    this->pdwFiltersPorcessIDToReleaseSize=0;

    this->pListView=NULL;

    this->pOptions=pOptions;
    // apply filters options according to options
    this->SetExclusionFilters();
    this->SetInclusionFilters();
    this->SetParentProcessIdFilters();
}
CFilters::~CFilters()
{
    this->FreeMemory();
}

//-----------------------------------------------------------------------------
// Name: FreeMemory
// Object: Free memory allocated by the class
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFilters::FreeMemory()
{
    // free pdwFiltersCurrentPorcessID
    if (this->pdwFiltersCurrentPorcessID)
    {
        delete[] this->pdwFiltersCurrentPorcessID;
        this->pdwFiltersCurrentPorcessID=NULL;
    }

    // free pdwFiltersParentPorcessID
    if (this->pdwFiltersParentPorcessID)
    {
        delete[] this->pdwFiltersParentPorcessID;
        this->pdwFiltersParentPorcessID=NULL;
    }

    // free pdwFiltersNewPorcessIDToWatch
    if (this->pdwFiltersNewPorcessIDToWatch)
    {
        delete[] this->pdwFiltersNewPorcessIDToWatch;
        this->pdwFiltersNewPorcessIDToWatch=NULL;
    }

    // free pdwFiltersPorcessIDToRelease
    if (this->pdwFiltersPorcessIDToRelease)
    {
        delete[] this->pdwFiltersPorcessIDToRelease;
        this->pdwFiltersPorcessIDToRelease=NULL;
    }

    // free pszFiltersInclusion and its content
    if (this->pszFiltersInclusion)
    {
        CMultipleElementsParsing::ParseStringArrayFree(this->pszFiltersInclusion,pszFiltersInclusionSize);
        this->pszFiltersInclusion=NULL;
    }

    // free pszFiltersExclusion and its content
    if (this->pszFiltersExclusion)
    {
        CMultipleElementsParsing::ParseStringArrayFree(this->pszFiltersExclusion,pszFiltersExclusionSize);
        this->pszFiltersExclusion=NULL;
    }

    this->pdwFiltersCurrentPorcessIDSize=0;
    this->pdwFiltersParentPorcessIDSize=0;
    this->pszFiltersInclusionSize=0;
    this->pszFiltersExclusionSize=0;
}

BOOL CFilters::SetParentProcessIdFilters()
{
    // free pdwFiltersParentPorcessID
    if (this->pdwFiltersParentPorcessID)
    {
        delete[] this->pdwFiltersParentPorcessID;
        this->pdwFiltersParentPorcessID=NULL;
    }
    this->pdwFiltersParentPorcessID=CMultipleElementsParsing::ParseDWORD(this->pOptions->AllProcessesParentPID,&this->pdwFiltersParentPorcessIDSize);

    return TRUE;
}

BOOL CFilters::SetInclusionFilters()
{
    // free pszFiltersInclusion and its content
    if (this->pszFiltersInclusion)
    {
        CMultipleElementsParsing::ParseStringArrayFree(this->pszFiltersInclusion,pszFiltersInclusionSize);
        this->pszFiltersInclusion=NULL;
    }

    this->pszFiltersInclusion=CMultipleElementsParsing::ParseString(this->pOptions->AllProcessesInclusion,&this->pszFiltersInclusionSize);

    return TRUE;
}
BOOL CFilters::SetExclusionFilters()
{
    // free pszFiltersExclusion and its content
    if (this->pszFiltersExclusion)
    {
        CMultipleElementsParsing::ParseStringArrayFree(this->pszFiltersExclusion,pszFiltersExclusionSize);
        this->pszFiltersExclusion=NULL;
    }

    this->pszFiltersExclusion=CMultipleElementsParsing::ParseString(this->pOptions->AllProcessesExclusion,&this->pszFiltersExclusionSize);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ReloadOptions
// Object: reload previously saved options
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFilters::ReloadOptions()
{
    // fill inclusion filter field
    SetDlgItemText(this->hWndFiltersDialog,IDC_EDIT_INCLUSION_FILTERS,this->pOptions->AllProcessesInclusion);

    // fill exclusion filter field
    SetDlgItemText(this->hWndFiltersDialog,IDC_EDIT_EXCLUSION_FILTERS,this->pOptions->AllProcessesExclusion);

    // fill parent filter field
    SetDlgItemText(this->hWndFiltersDialog,IDC_EDIT_PARENT_PID_FILTERS,this->pOptions->AllProcessesParentPID);

    // select previous defined process
    this->ReSelectProcess();
}


//-----------------------------------------------------------------------------
// Name: ReSelectProcess
// Object: Reselect process number saved in options in listbox.
//         Only process still running are reselected
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFilters::ReSelectProcess()
{
    if (this->pdwFiltersCurrentPorcessIDSize==0)
        return;
    DWORD dwNbItems;
    DWORD dwCnt;
    DWORD dwCnt2;
    DWORD dw;
    TCHAR ItemText[MAX_PATH];

    // remove no more alive processes from this->pdwFiltersCurrentPorcessID
    for (dwCnt=0;dwCnt<this->pdwFiltersCurrentPorcessIDSize;dwCnt++)
    {
        // if process is no more alive
        if (!CProcessHelper::IsAlive(this->pdwFiltersCurrentPorcessID[dwCnt]))
        {
            // if last element
            if (dwCnt==(this->pdwFiltersCurrentPorcessIDSize-1))
            {
                // just decrease array size and break
                this->pdwFiltersCurrentPorcessIDSize--;
                break;
            }
            // else
            // switch with last element
            this->pdwFiltersCurrentPorcessID[dwCnt]=this->pdwFiltersCurrentPorcessID[this->pdwFiltersCurrentPorcessIDSize-1];
            // decrease number of elements
            this->pdwFiltersCurrentPorcessIDSize--;
            // decrease counter to check the new element at pos dwCnt
            dwCnt--;
        }
    }


    // get number of items in listbox
    dwNbItems=this->pListView->GetItemCount();
    if (dwNbItems>0)
    {
        // for each item in listbox
        for (dwCnt=0;dwCnt<dwNbItems;dwCnt++)
        {
            // retrieve text 
            this->pListView->GetItemText(dwCnt,1,ItemText,MAX_PATH);

            dw=_ttoi(ItemText);
            if (dw==0)
                continue;

            // search Process ID in current process array
            for(dwCnt2=0;dwCnt2<this->pdwFiltersCurrentPorcessIDSize;dwCnt2++)
            {
                // if we found it
                if (dw==this->pdwFiltersCurrentPorcessID[dwCnt2])
                    // select item in listbox
                    this->pListView->SetSelectedState(dwCnt,TRUE);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Name: StoreOptions
// Object: Store options for a next filter dialog display.
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFilters::StoreOptions()
{
    // save inclusion filter field data
    GetDlgItemText(this->hWndFiltersDialog,IDC_EDIT_INCLUSION_FILTERS,this->pOptions->AllProcessesInclusion,MAX_PATH);

    // save exclusion filter field data
    GetDlgItemText(this->hWndFiltersDialog,IDC_EDIT_EXCLUSION_FILTERS,this->pOptions->AllProcessesExclusion,MAX_PATH);

    // save parent ID filter field data
    GetDlgItemText(this->hWndFiltersDialog,IDC_EDIT_PARENT_PID_FILTERS,this->pOptions->AllProcessesParentPID,MAX_PATH);
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
INT_PTR CFilters::ShowFilterDialog(HINSTANCE hInstance,HWND hWndDialog)
{
    // show dialog
    return DialogBoxParam(hInstance,(LPCTSTR)IDD_DIALOGAPPLICATIONFILTERS,hWndDialog,(DLGPROC)CFilters::FiltersWndProc,(LPARAM)this);
}

//-----------------------------------------------------------------------------
// Name: SaveFiltersSettings
// Object: Save and apply settings
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFilters::SaveFiltersSettings()
{
    DWORD dw;
    DWORD dwNbSelectedItems;
    DWORD dwNbItems;
    DWORD dwCnt;
    DWORD dwCnt2;
    DWORD dwOldProcIDSize;
    BOOL  bFound;
    TCHAR ItemText[MAX_PATH];
    DWORD* pdwOldProcID=NULL;

    // temporary store old process array
    dwOldProcIDSize=this->pdwFiltersCurrentPorcessIDSize;
    if (dwOldProcIDSize>0)
    {
        pdwOldProcID=new DWORD[dwOldProcIDSize];
        for (dwCnt=0;dwCnt<dwOldProcIDSize;dwCnt++)
            pdwOldProcID[dwCnt]=this->pdwFiltersCurrentPorcessID[dwCnt];
    }

    // free memory
    this->FreeMemory();

    // store data fields, to restore them at the next display of filter window 
    this->StoreOptions();

    ///////////////////////
    // retrieve current selected processes
    ///////////////////////

    // get number of selected items
    dwNbSelectedItems=this->pListView->GetSelectedCount();

    // get number of items in the listbox
    dwNbItems=this->pListView->GetItemCount();
    if ((dwNbSelectedItems>0)&&(dwNbItems>0))
    {
        this->pdwFiltersCurrentPorcessID=new DWORD[dwNbSelectedItems];

        // for each listbox item
        for (dwCnt=0;dwCnt<dwNbItems;dwCnt++)
        {
            if (!this->pListView->IsItemSelected(dwCnt))
                continue;
            
            // retrieve text 
            this->pListView->GetItemText(dwCnt,1,ItemText,MAX_PATH);
            {
                dw=_ttoi(ItemText);
                if (dw==0)
                    continue;

                // store Process ID in pdwFiltersCurrentPorcessID array
                this->pdwFiltersCurrentPorcessID[this->pdwFiltersCurrentPorcessIDSize]=dw;
                this->pdwFiltersCurrentPorcessIDSize++;
            }
        }
    }

    //////////////////////////////////////
    // get parent ID filters
    //////////////////////////////////////
    if (GetDlgItemText(this->hWndFiltersDialog,IDC_EDIT_PARENT_PID_FILTERS,ItemText,MAX_PATH))
        this->pdwFiltersParentPorcessID=CMultipleElementsParsing::ParseDWORD(ItemText,&this->pdwFiltersParentPorcessIDSize);

    //////////////////////////////////////
    // get inclusion filters
    //////////////////////////////////////
    if (GetDlgItemText(this->hWndFiltersDialog,IDC_EDIT_INCLUSION_FILTERS,ItemText,MAX_PATH))
        this->pszFiltersInclusion=CMultipleElementsParsing::ParseString(ItemText,&this->pszFiltersInclusionSize);

    //////////////////////////////////////
    // get exclusion filters
    //////////////////////////////////////
    if (GetDlgItemText(this->hWndFiltersDialog,IDC_EDIT_EXCLUSION_FILTERS,ItemText,MAX_PATH))
        this->pszFiltersExclusion=CMultipleElementsParsing::ParseString(ItemText,&this->pszFiltersExclusionSize);

    CLinkListSimple* pLinkList=new CLinkListSimple();
    /////////////////////////////////////////////////////
    // get new process to watch (process ID present in this->pdwFiltersCurrentPorcessID 
    // but not in pdwOldProcID)
    /////////////////////////////////////////////////////
    // for each process in this->pdwFiltersCurrentPorcessID
    for (dwCnt=0;dwCnt<this->pdwFiltersCurrentPorcessIDSize;dwCnt++)
    {
        // search process number in old process array
        bFound=FALSE;
        for (dwCnt2=0;dwCnt2<dwOldProcIDSize;dwCnt2++)
        {
            // if item is found, process is already monitored
            if (this->pdwFiltersCurrentPorcessID[dwCnt]==pdwOldProcID[dwCnt2])
            {
                bFound=TRUE;
                break;
            }
        }
        // if not found the item have to be monitored
        if (!bFound)
            pLinkList->AddItem((PVOID)this->pdwFiltersCurrentPorcessID[dwCnt]);
    }
    // update pdwFiltersNewPorcessIDToWatch and pdwFiltersNewPorcessIDToWatchSize
    this->pdwFiltersNewPorcessIDToWatch=(DWORD*)pLinkList->ToArray(&this->pdwFiltersNewPorcessIDToWatchSize);


    // remove items of the linked list (avoid to destroy and create a new one)
    pLinkList->RemoveAllItems();

    /////////////////////////////////////////////////////
    // get process to detach (process ID present in pdwOldProcID 
    // but not in this->pdwFiltersCurrentPorcessID and still alive)
    /////////////////////////////////////////////////////
    // for each process in pdwOldProcID
    for (dwCnt=0;dwCnt<dwOldProcIDSize;dwCnt++)
    {
        // search process number in pdwFiltersCurrentPorcessID array
        bFound=FALSE;
        for (dwCnt2=0;dwCnt2<this->pdwFiltersCurrentPorcessIDSize;dwCnt2++)
        {
            // if process ID is found we should still monitor process
            if (pdwOldProcID[dwCnt]==this->pdwFiltersCurrentPorcessID[dwCnt2])
            {
                bFound=TRUE;
                break;
            }
        }
        // if not found the item have to be released
        if (!bFound)
            pLinkList->AddItem((PVOID)pdwOldProcID[dwCnt]);
    }
    // update pdwFiltersNewPorcessIDToWatch and pdwFiltersNewPorcessIDToWatchSize
    this->pdwFiltersPorcessIDToRelease=(DWORD*)pLinkList->ToArray(&this->pdwFiltersPorcessIDToReleaseSize);

    // free memory
    delete pLinkList;

    if (dwOldProcIDSize>0)
        delete[] pdwOldProcID;
}

//-----------------------------------------------------------------------------
// Name: DoesProcessNameMatchFilters
// Object: Check if process name match given filters
// Parameters :
//     in  : TCHAR* ProcessName : name of process
//     out :
//     return : TRUE if ProcessName match filters, else returns FALSE
//-----------------------------------------------------------------------------
BOOL CFilters::DoesProcessNameMatchFilters(TCHAR* ProcessName)
{
    DWORD dwCnt;
    TCHAR* ShortProcessName;

    // assume we don't get full path else user has to write "*MyApp.exe" as filter instead of "MyApp.exe", which is disturbing
    ShortProcessName = CStdFileOperations::GetFileName(ProcessName);
    if (!ShortProcessName)
        ShortProcessName = ProcessName;

    BOOL bMatchInclude;
    if (this->pszFiltersInclusionSize!=0)
    {
        bMatchInclude=FALSE;
        // check include filters
        for (dwCnt=0;dwCnt<this->pszFiltersInclusionSize;dwCnt++)
        {
            // check if name doesn't match
            if (CWildCharCompare::WildICmp(this->pszFiltersInclusion[dwCnt],ShortProcessName))
                bMatchInclude=TRUE;
        }
        // if no one include filter match return
        if (!bMatchInclude)
            return FALSE;
    }

    // check exclude filters
    for (dwCnt=0;dwCnt<this->pszFiltersExclusionSize;dwCnt++)
    {
        // check if name match
        if (CWildCharCompare::WildICmp(this->pszFiltersExclusion[dwCnt],ShortProcessName))
            return FALSE;
    }

    // file names match inclusion and exclusion filters
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: DoesParentIDMatchFilters
// Object: Check if process parent ID match given filters
// Parameters :
//     in  : DWORD dwParentID : parent process ID
//     out :
//     return : TRUE if parent ID match filters, else returns FALSE
//-----------------------------------------------------------------------------
BOOL CFilters::DoesParentIDMatchFilters(DWORD dwParentID)
{
    // if no filters set
    if (this->pdwFiltersParentPorcessIDSize==0)
        return TRUE;

    DWORD dwCnt;
    // loop throw pdwFiltersParentPorcessID array
    for(dwCnt=0;dwCnt<this->pdwFiltersParentPorcessIDSize;dwCnt++)
    {
        // if item found
        if (this->pdwFiltersParentPorcessID[dwCnt]==dwParentID)
            return TRUE;
    }
    // item not found
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: RefreshProcessesList
// Object: refresh the list of current running processes
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFilters::RefreshProcessesList()
{
    TCHAR* pszProcess;
    TCHAR ProcId[20];

    // clear listview
    this->pListView->Clear();

    PROCESSENTRY32 pe32 = {0};
    HANDLE hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (hSnap == INVALID_HANDLE_VALUE) 
    {
        CAPIError::ShowLastError();
        return; 
    }
    // Fill the size of the structure before using it. 
    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
    // Walk the process list of the system
    if (!Process32First(hSnap, &pe32))
    {
        CAPIError::ShowLastError();
        CloseHandle(hSnap);
        return;
    }
    do 
    {
        // don't show system processes
        if ((pe32.th32ProcessID==FILTERS_INACTIVE_PROCESSES_PID)
            ||(pe32.th32ProcessID==FILTERS_SYSTEM_PID))
            continue;
        
        // get only file name
        pszProcess=_tcsrchr(pe32.szExeFile,'\\');
        if (pszProcess)
            pszProcess++;
        else
            pszProcess=pe32.szExeFile;

        // get pid
        _itot(pe32.th32ProcessID,ProcId,10);

        // add process to the list view
        TCHAR* ppc[2]={pszProcess,ProcId};
        this->pListView->AddItemAndSubItems(2,ppc);

    } 
    while (Process32Next(hSnap, &pe32)); 
 
    // clean up the snapshot object. 
    CloseHandle (hSnap); 

}

//-----------------------------------------------------------------------------
// Name: Close
// Object: close Filters dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CFilters::Close(tagDlgRes DlgRes)
{
    if (this->pListView)
    {
        delete this->pListView;
        this->pListView=NULL;
    }
    EndDialog(this->hWndFiltersDialog,(INT_PTR) DlgRes);
}

//-----------------------------------------------------------------------------
// Name: FiltersWndProc
// Object: dialog callback of the Filters dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CFilters::FiltersWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            CFilters* pFilters=(CFilters*)lParam;
            pFilters->hWndFiltersDialog=hWnd;

            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pFilters);
            // load dlg icons
            CDialogHelper::SetIcon(hWnd,IDI_ICON_PROCESSES_FILTERS);

            // init listview
            pFilters->pListView=new CListview(GetDlgItem(hWnd,IDC_LIST_RUNNING_APP_FILTER));
            pFilters->pListView->SetStyle(TRUE,FALSE,FALSE,TRUE);
            pFilters->pListView->AddColumn(_T("Name"),160,LVCFMT_LEFT);
            pFilters->pListView->AddColumn(_T("PID"),80,LVCFMT_CENTER);

            // update process list
            pFilters->RefreshProcessesList();

            // reload old options if any
            pFilters->ReloadOptions();

        }

        break;
    case WM_CLOSE:
        {
            CFilters* pFilters=((CFilters*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pFilters)
                break;
            pFilters->Close(CFilters::DLG_RES_CANCEL);
        }
        break;
    case WM_COMMAND:
        {
            CFilters* pFilters=((CFilters*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pFilters)
                break;

            switch (LOWORD(wParam))
            {
            case IDOK:
                // save filters settings
                pFilters->SaveFiltersSettings();
                // exit
                pFilters->Close(CFilters::DLG_RES_OK);
                break;
            case IDCANCEL:
                pFilters->Close(CFilters::DLG_RES_CANCEL);
                break;
            case IDC_BUTTON_REFRESH_PROCESSES_LIST:
                // update process list
                pFilters->RefreshProcessesList();

                // select process again
                pFilters->ReSelectProcess();
                break;
            case IDC_BUTTON_FILTERS_SELECT_ALL:
                pFilters->pListView->SelectAll();

            break;
            case IDC_BUTTON_FILTERS_UNSELECT_ALL:
                pFilters->pListView->UnselectAll();
                break;
            }
        }
        break;

    case WM_NOTIFY:
        {
            CFilters* pFilters=((CFilters*)GetWindowLongPtr(hWnd,GWLP_USERDATA));
            if (!pFilters)
                break;
            if (pFilters->pListView)
            {
                LPNMLISTVIEW pnm;
                pnm = (LPNMLISTVIEW)lParam;

                // set sorting type
                if (pnm->hdr.hwndFrom==ListView_GetHeader(pFilters->pListView->GetControlHandle()))
                {
                    if (pnm->hdr.code==HDN_ITEMCLICK) 
                    {
                        // on header click
                        if (pnm->iItem==0)
                            pFilters->pListView->SetSortingType(CListview::SortingTypeString);
                        else
                            pFilters->pListView->SetSortingType(CListview::SortingTypeNumber);
                    }
                }
                
                if (pFilters->pListView->OnNotify(wParam, lParam))
                    return TRUE;
            }
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}