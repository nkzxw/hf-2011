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

#include "stats.h"

extern CLinkList* pLogList;
extern CListview* pListview;
extern void RemoveSelectedLogs();

CLogsStatsUI* g_Displayed_pStats=NULL; 

CLogsStatsUI::CLogsStatsUI(HINSTANCE hInstance)
{
    this->hInstance = hInstance;
}
CLogsStatsUI::~CLogsStatsUI()
{
}

//-----------------------------------------------------------------------------
// Name: ModelessDialogThread
// Object: allow to act like a dialog box in modeless mode
// Parameters :
//     in  : HINSTANCE hInstance : instance containing dialog resource
//           HWND hWndDialog : parent window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CLogsStatsUI::ModelessDialogThread(PVOID lParam)
{
    CLogsStatsUI* pLogsStatsUI = (CLogsStatsUI*)lParam;
    ::DialogBoxParam (pLogsStatsUI->hInstance,(LPCTSTR)IDD_DIALOG_STATS,NULL,(DLGPROC)CLogsStatsUI::WndProc,(LPARAM)pLogsStatsUI);
    g_Displayed_pStats=NULL;
    delete pLogsStatsUI;
    return 0;
}
//-----------------------------------------------------------------------------
// Name: Show
// Object: display a stats dialog
// Parameters :
//     in  : HINSTANCE hInstance : application instance
//           HWND hWndDialog : main window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CLogsStatsUI::Show(HINSTANCE hInstance,HWND hWndDialog)
{
    // if an existing instance is already existing
    if (g_Displayed_pStats)
    {
        if (::IsIconic(g_Displayed_pStats->hWndDialog))
            ::ShowWindow(g_Displayed_pStats->hWndDialog,SW_RESTORE);
        else
            ::SetWindowPos(g_Displayed_pStats->hWndDialog,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);

        return;
    }

    // create a thread, else a deadlock can occur if a function having a lock on pLogList
    // sends message to main dialog wnd proc (because we are currently in main dialog wnd proc)

    // show dialog
    // don't use hWndDialog to allow to put winapioverride to an upper Z-order

    CLogsStatsUI* pStats=new CLogsStatsUI(hInstance);
    g_Displayed_pStats = pStats;
    pStats->hWndParentDialog = hWndDialog;
    pStats->Search.hWndDialog = hWndDialog;
    pStats->Search.bCheckApiName = TRUE;
    pStats->Search.bCheckModuleName = TRUE;

    // create thread instead of using CreateDialogParam to don't have to handle keyboard event like TAB
    CloseHandle(CreateThread(NULL,0,CLogsStatsUI::ModelessDialogThread,pStats,0,NULL));
}

void CLogsStatsUI::PopUpMenuItemClickCallbackStatic(UINT MenuID,LPVOID UserParam)
{
    ((CLogsStatsUI*) UserParam)->PopUpMenuItemClickCallback(MenuID);
}

void CLogsStatsUI::PopUpMenuItemClickCallback(UINT MenuID)
{
    UINT FirstSelectedItem = this->pListview->GetSelectedIndex();

    if (MenuID == this->MenuIdCopyFunctionName)
    {
        TCHAR Name[MAX_PATH];
        this->pListview->GetItemText(FirstSelectedItem,0,Name,MAX_PATH);// function name is in column 0
        CClipboard::CopyToClipboard(this->hWndDialog,Name);
    }
    else if (MenuID == this->MenuIdCopyDllName)
    {
        TCHAR Name[MAX_PATH];
        this->pListview->GetItemText(FirstSelectedItem,1,Name,MAX_PATH);// dll name is in column 1
        CClipboard::CopyToClipboard(this->hWndDialog,Name);
    }
    // for the following we use as possible CSearch and main.cpp functions
    else if (MenuID == this->MenuIdRemoveAllTheseLogs)
    {
        this->Search.UnselectAll();

        SIZE_T SelectedCount =this->pListview->GetSelectedCount();
        SIZE_T ItemCount =this->pListview->GetItemCount();
        SIZE_T Cnt;
        SIZE_T CntSelected;
        for (Cnt=0,CntSelected=0;(CntSelected<SelectedCount) && (Cnt<ItemCount);Cnt++)
        {
            if (!this->pListview->IsItemSelected(Cnt))
                continue;
            // remove all logs having this function AND dll name
            this->pListview->GetItemText(Cnt,0,this->Search.ApiName,MAX_PATH);// function name is in column 0
            this->pListview->GetItemText(Cnt,1,this->Search.ModuleName,MAX_PATH);// dll name is in column 1

            // select items matching function and dll name
            this->Search.SelectAll();

            // remove function stats from stats listview
            this->pListview->RemoveItem(Cnt);
            Cnt--;// update Cnt to avoid infinite loop

            CntSelected++;// only for selected items
        }

        // all items should be selected
        RemoveSelectedLogs();// use main.cpp RemoveSelectedLogs function
    }
    else if (MenuID == this->MenuIdFindFirst)
    {
        this->pListview->GetItemText(FirstSelectedItem,0,this->Search.ApiName,MAX_PATH);// function name is in column 0
        this->pListview->GetItemText(FirstSelectedItem,1,this->Search.ModuleName,MAX_PATH);// dll name is in column 1
        this->Search.Find();
    }
    else if (MenuID == this->MenuIdFindNext)
    {
        this->pListview->GetItemText(FirstSelectedItem,0,this->Search.ApiName,MAX_PATH);// function name is in column 0
        this->pListview->GetItemText(FirstSelectedItem,1,this->Search.ModuleName,MAX_PATH);// dll name is in column 1
        this->Search.FindNext(::pListview->GetSelectedIndex()+1);// use main.cpp listview
    }
    else if (MenuID == this->MenuIdFindPrevious)
    {
        this->pListview->GetItemText(FirstSelectedItem,0,this->Search.ApiName,MAX_PATH);// function name is in column 0
        this->pListview->GetItemText(FirstSelectedItem,1,this->Search.ModuleName,MAX_PATH);// dll name is in column 1
        this->Search.FindPrevious(::pListview->GetSelectedIndex()-1);// use main.cpp listview
    }
    else if (MenuID == this->MenuIdHighLight)
    {
        this->Search.UnselectAll();

        SIZE_T SelectedCount =this->pListview->GetSelectedCount(); 
        SIZE_T ItemCount =this->pListview->GetItemCount();
        SIZE_T Cnt;
        SIZE_T CntSelected;
        for (Cnt=0,CntSelected=0;(CntSelected<SelectedCount) && (Cnt<ItemCount);Cnt++)
        {
            if (!this->pListview->IsItemSelected(Cnt))
                continue;
            // remove all logs having this function AND dll name
            this->pListview->GetItemText(Cnt,0,this->Search.ApiName,MAX_PATH);// function name is in column 0
            this->pListview->GetItemText(Cnt,1,this->Search.ModuleName,MAX_PATH);// dll name is in column 1

            // select items matching function and dll name
            this->Search.SelectAll();

            CntSelected++;// only for selected items
        }
    }

}

//-----------------------------------------------------------------------------
// Name: Init
// Object: init stats dialog
// Parameters :
//     in  : HWND hWndDialog : stat dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CLogsStatsUI::Init(HWND hwnd)
{
    this->hWndDialog=hwnd;
    this->pListview=new CListview(GetDlgItem(hwnd,IDC_LIST_STATS));
    this->pListview->SetStyle(TRUE,FALSE,FALSE,FALSE);
    this->pListview->AddColumn(_T("Function Name"),150,LVCFMT_LEFT);
    this->pListview->AddColumn(_T("Module Name"),100,LVCFMT_LEFT);
    this->pListview->AddColumn(_T("Call Count"),100,LVCFMT_CENTER);
    this->pListview->AddColumn(_T("Average Duration (us)"),150,LVCFMT_CENTER);

    this->pListview->SetPopUpMenuItemClickCallback(CLogsStatsUI::PopUpMenuItemClickCallbackStatic,this);
    UINT MenuIndex = 0;
    this->MenuIdCopyFunctionName=this->pListview->pPopUpMenu->Add(_T("Copy Function Name"),IDI_ICON_COPY_SMALL,this->hInstance,MenuIndex++);
    this->MenuIdCopyDllName=this->pListview->pPopUpMenu->Add(_T("Copy Dll Name"),IDI_ICON_CLIPBOARD_SMALL,this->hInstance,MenuIndex++);
    this->MenuIdRemoveAllTheseLogs=this->pListview->pPopUpMenu->Add(_T("Remove All These Logs"),IDI_ICON_REMOVE_SMALL,this->hInstance,MenuIndex++);
    this->MenuIdHighLight=this->pListview->pPopUpMenu->Add(_T("Highlight Logs"),IDI_ICON_HIGHLIGHT,this->hInstance,MenuIndex++);
    this->MenuIdFindFirst=this->pListview->pPopUpMenu->Add(_T("Find First"),IDI_ICON_FIND,this->hInstance,MenuIndex++);
    this->MenuIdFindNext=this->pListview->pPopUpMenu->Add(_T("Find Next"),IDI_ICON_FIND_NEXT,this->hInstance,MenuIndex++);
    this->MenuIdFindPrevious=this->pListview->pPopUpMenu->Add(_T("Find Previous"),IDI_ICON_FIND_PREVIOUS,this->hInstance,MenuIndex++);
    this->pListview->pPopUpMenu->AddSeparator(MenuIndex++);

    // render layout
    this->OnSize();

    this->MakeStats();

}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close stats dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CLogsStatsUI::Close()
{
    delete this->pListview;
    this->pListview=NULL;
    DestroyWindow(this->hWndDialog);
}

//-----------------------------------------------------------------------------
// Name: WndProc
// Object: stats dialog window proc
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CLogsStatsUI::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // init dialog
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG)lParam);
        CDialogHelper::SetIcon(hWnd,IDI_ICON_STATS);
        ((CLogsStatsUI*)lParam)->Init(hWnd);
        break;
    case WM_CLOSE:
        {
            // close dialog
            CLogsStatsUI* pStats=(CLogsStatsUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (pStats)
                pStats->Close();
            break;
        }
        break;

    case WM_SIZING:
        {
            CLogsStatsUI* pStats=(CLogsStatsUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (!pStats)
                break;
            pStats->OnSizing((RECT*)lParam);
            break;
        }
    case WM_SIZE:
        {
            CLogsStatsUI* pStats=(CLogsStatsUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (!pStats)
                break;
            pStats->OnSize();
            break;
        }

    case WM_NOTIFY:
        {
            CLogsStatsUI* pStats=(CLogsStatsUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (!pStats)
                break;

            //we are only interested in NM_CUSTOMDRAW notifications for ListView
            if (pStats->pListview)
            {
                LPNMLISTVIEW pnm;
                pnm = (LPNMLISTVIEW)lParam;

                // depending header column click, change the sorting type
                if (pnm->hdr.hwndFrom==ListView_GetHeader(pStats->pListview->GetControlHandle()))
                {
                    if (pnm->hdr.code==HDN_ITEMCLICK) 
                    {
                        // on header click
                        if ((pnm->iItem==2)// call count
                            ||(pnm->iItem==3))// average duration
                            pStats->pListview->SetSortingType(CListview::SortingTypeNumber);
                        else
                            pStats->pListview->SetSortingType(CListview::SortingTypeString);
                    }
                }
                if (pStats->pListview->OnNotify(wParam,lParam))
                    return TRUE;
            }
        }
        break;
    case WM_DESTROY:
        {
            CLogsStatsUI* pStats=(CLogsStatsUI*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
            if (!pStats)
                break;
            SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)0);

            return FALSE;
        }
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: OnSizing
// Object: check dialog box size
// Parameters :
//     in out  : RECT* pRect : pointer to dialog rect
//     return : 
//-----------------------------------------------------------------------------
void CLogsStatsUI::OnSizing(RECT* pRect)
{
    // check min width and min height
    if ((pRect->right-pRect->left)<CLogsStatsUI_DIALOG_MIN_WIDTH)
        pRect->right=pRect->left+CLogsStatsUI_DIALOG_MIN_WIDTH;
    if ((pRect->bottom-pRect->top)<CLogsStatsUI_DIALOG_MIN_HEIGHT)
        pRect->bottom=pRect->top+CLogsStatsUI_DIALOG_MIN_HEIGHT;
}

//-----------------------------------------------------------------------------
// Name: OnSize
// Object: Resize controls
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CLogsStatsUI::OnSize()
{
    RECT Rect;
    RECT RectWindow;

    HWND hItem=GetDlgItem(this->hWndDialog,IDC_LIST_STATS);
    // get dialog rect
    CDialogHelper::GetDialogInternalRect(this->hWndDialog,&RectWindow);

    CDialogHelper::GetClientWindowRect(this->hWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        0,0,
        RectWindow.right-RectWindow.left-2*(Rect.left),
        RectWindow.bottom-RectWindow.top-2*(Rect.top),
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItem);
}

//-----------------------------------------------------------------------------
// Name: MakeStats
// Object: compute statistics and add them to listview
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CLogsStatsUI::MakeStats()
{
    CLinkListItem* pItemLogList;
    CLinkListItem* pItemStatList;
    CLinkList* pStatList;
    LOG_LIST_ENTRY* pLogEntry;
    STATSTRUCT* pStat;
    STATSTRUCT Stat;
    BOOL bFoundInStatList;
    TCHAR** ppc=new TCHAR*[4];
    TCHAR pszCallCount[12];
    TCHAR pszAverageDuration[12];
    DWORD AverageDuration;
    HCURSOR hOldCursor;

    // create a list of STATSTRUCT
    pStatList=new CLinkList(sizeof(STATSTRUCT));

    // store original cursor
    hOldCursor=GetCursor();

    // set cusor to wait cursor
    SetCursor(LoadCursor(NULL,IDC_WAIT));

    // for each item in pLogList
    pLogList->Lock(TRUE);
    for (pItemLogList=pLogList->Head;pItemLogList;pItemLogList=pItemLogList->NextItem)
    {
        pLogEntry=(LOG_LIST_ENTRY*)pItemLogList->ItemData;
        // if pLogEntry is an user message, go to next entry
        if (pLogEntry->Type!=ENTRY_LOG)
            continue;

        bFoundInStatList=FALSE;

        // a function is defined by it's api name and module name
        // so try to find an item in pStatList with same api name and module name
        pStatList->Lock();
        for (pItemStatList=pStatList->Head;pItemStatList;pItemStatList=pItemStatList->NextItem)
        {
            pStat=(STATSTRUCT*)pItemStatList->ItemData;
            // compare api and module name
            if (_tcsicmp(pStat->ApiName,pLogEntry->pLog->pszApiName)==0)
            {
                if(_tcsicmp(pStat->ModuleName,pLogEntry->pLog->pszModuleName)==0)
                {
                    // the func is already in stat list, so update it's stat values

                    // update count
                    pStat->Count++;

                    // update DurationSum (trying to avoid buffer overflow)
                    if (0xFFFFFFFF-pLogEntry->pLog->pHookInfos->dwCallDuration<=pStat->DurationSum)
                        pStat->DurationSum=0xFFFFFFFF;
                    else
                        pStat->DurationSum+=pLogEntry->pLog->pHookInfos->dwCallDuration;

                    // we have found item
                    bFoundInStatList=TRUE;
                    break;
                }
            }
        }
        pStatList->Unlock();

        // if item was not in state list, add it to stat list
        if (!bFoundInStatList)
        {
            _tcscpy(Stat.ApiName,pLogEntry->pLog->pszApiName);
            _tcscpy(Stat.ModuleName,pLogEntry->pLog->pszModuleName);
            Stat.Count=1;
            Stat.DurationSum=pLogEntry->pLog->pHookInfos->dwCallDuration;

            pStatList->AddItem(&Stat);
        }
    }
    pLogList->Unlock();

    // for each item of pStatList
    pStatList->Lock();
    for (pItemStatList=pStatList->Head;pItemStatList;pItemStatList=pItemStatList->NextItem)
    {
        // add item to list view
        pStat=(STATSTRUCT*)pItemStatList->ItemData;

        // api name
        ppc[0]=pStat->ApiName;

        // module name
        ppc[1]=pStat->ModuleName;

        // call count
        _stprintf(pszCallCount,_T("%u"),pStat->Count);
        ppc[2]=pszCallCount;

        // average duration
        AverageDuration=pStat->DurationSum/pStat->Count;
        if (pStat->DurationSum==0xFFFFFFFF)
            _stprintf(pszAverageDuration,_T(">%u"),AverageDuration);
        else
            _stprintf(pszAverageDuration,_T("%u"),AverageDuration);
    
        ppc[3]=pszAverageDuration;

        this->pListview->AddItemAndSubItems(4,ppc,FALSE);
    }
    pStatList->Unlock();

    // free allocated memory
    delete[] ppc;

    delete pStatList;

    // restore original cursor
    SetCursor(hOldCursor);
}