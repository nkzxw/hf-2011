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

#include <Windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <Tlhelp32.h>
#include "resource.h"


#include "../Tools/gui/Dialog/DialogHelper.h"
#include "../tools/GUI/ListView/ListView.h"
#include "../tools/GUI/ToolBar/Toolbar.h"
#include "../tools/Privilege/privilege.h"
#include "../tools/Process/ProcessHelper/ProcessHelper.h"
#include "../Tools/Process/Memory/ProcessMemory.h"
#include "../Tools/File/StdFileOperations.h"

#include "HeapWalk.h"
#include "HexDisplay.h"
#include "filters.h"
#include "Search.h"
#include "defines.h"
#include "Options.h"

#define MAIN_DIALOG_MIN_WIDTH 520
#define MAIN_DIALOG_MIN_HEIGHT 200
#define TIMEOUT_CANCEL_WALKING_THREAD 5000 // max time in ms to wait for the cancel event to be taken into account
#define MENU_SHOW_HEX_DATA _T("Show Hex Data")
#define NB_COLUMNS 6
CListview::COLUMN_INFO pColumnInfo[NB_COLUMNS]={
                                                {_T("Address"),80,LVCFMT_CENTER},
                                                {_T("Block Size"),80,LVCFMT_CENTER},
                                                {_T("Flags"),50,LVCFMT_CENTER},
                                                {_T("Lock Count"),80,LVCFMT_CENTER},
                                                {_T("Data"),300,LVCFMT_LEFT},
                                                {_T("Ascii Data"),300,LVCFMT_LEFT}
                                                };


HINSTANCE mhInstance;
HWND mhWndDialog=NULL;
HWND hWndComboProcesses=NULL;
HWND hWndComboHeapList=NULL;
HWND hWndHeapContentGroup=NULL;
DWORD HeapListNumber=0;
CHeapWalk* pHeapWalk=NULL;
CListview* pListView=NULL;
CToolbar* pToolbar=NULL;
COptions* pOptions=NULL;
CProcessMemory* pProcessMemory=NULL;
DWORD CurrentProcessID=0;
HANDLE hEvtCancel=NULL;
HANDLE hWalkingThread=NULL;
BOOL ParseDoneOnce=FALSE;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Init();
void Close();
void CheckSize(RECT* pWinRect);
void Resize();

void ClearComboHeapList();
void ClearListviewHeapEntries();
BOOL CallBackHeapList(HEAPLIST* pHeapList,PVOID pUserParam);
BOOL CallBackHeapEntry(HEAPENTRY* pHeapEntry,PVOID pUserParam);
void RefreshProcessList();
BOOL CheckIfProcessIsAlive(DWORD dwProcessId);
DWORD GetProcessId();
void OnSelectedProcessChange();
void StartStop();
void Start();
DWORD WINAPI WalkHeap(LPVOID lpParameter);
void Cancel();
DWORD WINAPI CancelWalk(LPVOID lpParameter);
void CallBackPopUpMenuItem(UINT MenuID,LPVOID UserParam);
void ApplyFiltersToHeapContent();
BOOL CheckFilters(HEAPENTRY* pHeapEntry);
BOOL IsAHeapBeingParsed();
void SetToolbarMode(BOOL bIsWalking);
BOOL CheckAndReportDeadProcess();

//-----------------------------------------------------------------------------
// Name: WinMain
// Object: Entry point of app
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow
                   )
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);
    UNREFERENCED_PARAMETER(lpCmdLine);

    mhInstance=hInstance;

    // enable Xp style
    InitCommonControls();

    // try to get debug privileges
    CPrivilege Privilege(FALSE);
    Privilege.SetPrivilege(SE_DEBUG_NAME,TRUE);


    hEvtCancel=CreateEvent(NULL,FALSE,FALSE,NULL);

    TCHAR ConfigFileName[MAX_PATH];
    CStdFileOperations::GetAppName(ConfigFileName,MAX_PATH);
    CStdFileOperations::ChangeFileExt(ConfigFileName,_T("ini"));
    pOptions=new COptions(ConfigFileName);
    pOptions->Load();

    ///////////////////////////////////////////////////////////////////////////
    // show main window
    //////////////////////////////////////////////////////////////////////////
    DialogBox(hInstance, (LPCTSTR)IDD_DIALOG_HEAP_WALKER, NULL, (DLGPROC)WndProc);

    pOptions->Save();
    delete pOptions;
    CloseHandle(hEvtCancel);
}

//-----------------------------------------------------------------------------
// Name: Init
// Object: Initialize objects that requires the Dialog exists
//          and sets some dialog properties
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void Init()
{
    hWndHeapContentGroup=GetDlgItem(mhWndDialog,IDC_STATIC_HEAP_CONTENT_GROUP);
    hWndComboProcesses=GetDlgItem(mhWndDialog,IDC_COMBO_PROCESSES);

    // use ComboBoxEx for Heap list to associate data to combo item
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_USEREX_CLASSES;
    InitCommonControlsEx(&icex);
    hWndComboHeapList = CreateWindowEx(0, WC_COMBOBOXEX, NULL,
                            WS_BORDER | WS_VISIBLE | 
                            WS_CHILD | CBS_DROPDOWNLIST|WS_VSCROLL,
                            60,    // Horizontal position of Combobox
                            14,    // Vertical position of Combobox
                            140,   // Sets the width of Combobox
                            400,   // Sets the height of Combobox
                            GetDlgItem(mhWndDialog,IDC_STATIC_SELECT_HEAP_GROUP),
                            NULL,
                            mhInstance,
                            NULL);

    CDialogHelper::SetIcon(mhWndDialog,IDI_ICON_APP);

    pListView=new CListview(GetDlgItem(mhWndDialog,IDC_LIST_HEAP_CONTENT));

    pListView->InitListViewColumns(NB_COLUMNS,pColumnInfo);
    pListView->SetStyle(TRUE,FALSE,FALSE,FALSE);

    pListView->SetSortingType(CListview::SortingTypeNumber);
    pListView->SetPopUpMenuItemClickCallback(CallBackPopUpMenuItem,NULL);

    pListView->pPopUpMenu->AddSeparator();
    pListView->pPopUpMenu->Add(MENU_SHOW_HEX_DATA);

    pToolbar=new CToolbar(mhInstance,mhWndDialog,TRUE,TRUE,24,24);
    pToolbar->AddButton(IDC_BUTTON_START_STOP,_T("Start"),IDI_ICON_START,IDI_ICON_START,IDI_ICON_START);
    pToolbar->AddSeparator();
    pToolbar->AddButton(IDC_BUTTON_SAVE,_T("Save"),IDI_ICON_SAVE);
    pToolbar->AddSeparator();
    pToolbar->AddButton(IDC_BUTTON_FILTER,_T("Filters"),IDI_ICON_FILTER,_T("Set Filters For Heap Parsing"));
    pToolbar->AddButton(IDC_BUTTON_SEARCH,_T("Search"),IDI_ICON_SEARCH,_T("Search Data Into Current Heap"));
    pToolbar->AddSeparator();
    pToolbar->AddButton(IDC_BUTTON_REFRESH_PROCESSES,_T("Refresh"),IDI_ICON_REFRESH,_T("Refresh Process List"));

    RefreshProcessList();
}

//-----------------------------------------------------------------------------
// Name: Close
// Object: Close 
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void Close()
{
    // hide main window to avoid user interaction during closing
    ShowWindow(mhWndDialog,FALSE);

    // stop parsing current heap
    Cancel();
    // free memory associated to list view
    ClearComboHeapList();
    // free memory associated with combo heap list
    ClearListviewHeapEntries();

    // delete pListview
    delete pListView;
    pListView=NULL;

    // delete Toolbar
    delete pToolbar;
    pToolbar=NULL;

    // end dialog
    EndDialog(mhWndDialog,0);
}



//-----------------------------------------------------------------------------
// Name: WndProc
// Object: Main dialog callback
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        mhWndDialog=hWnd;
        Init();
        break;
    case WM_CLOSE:
        Close();
        break;
    case WM_SIZING:
        CheckSize((RECT*)lParam);
        break;
    case WM_SIZE:
        Resize();
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON_REFRESH_PROCESSES:
            RefreshProcessList();
            break;
        case IDC_BUTTON_START_STOP:
            StartStop();
            break;
        case IDC_BUTTON_SAVE:

            pToolbar->EnableButton(IDC_BUTTON_START_STOP,FALSE);
            pToolbar->EnableButton(IDC_BUTTON_SAVE,FALSE);
            pToolbar->EnableButton(IDC_BUTTON_FILTER,FALSE);

            if(pListView)
                pListView->Save();

            pToolbar->EnableButton(IDC_BUTTON_START_STOP,TRUE);
            pToolbar->EnableButton(IDC_BUTTON_SAVE,TRUE);
            pToolbar->EnableButton(IDC_BUTTON_FILTER,TRUE);

            break;
        case IDC_BUTTON_FILTER:
            if (CFilters::Show(mhInstance,mhWndDialog,pOptions)==CFilters::DLG_RES_OK)
            {
                if ((pOptions->FilterApplyToCurrentEntries)&&ParseDoneOnce)
                    ApplyFiltersToHeapContent();
            }
            break;
        case IDC_BUTTON_SEARCH:
            CSearch::Show(mhInstance,mhWndDialog,pListView,pOptions);
            break;
        }
        switch(HIWORD(wParam))
        {
        case CBN_SELCHANGE:
            // process selection change
            if (lParam==(LONG_PTR)hWndComboProcesses)
            {
                OnSelectedProcessChange();
            }
            break;
        }
        break;
    case WM_NOTIFY:
        if (pListView)
        {
            LPNMLISTVIEW pnm;
            pnm = (LPNMLISTVIEW)lParam;

            if (pnm->hdr.hwndFrom==ListView_GetHeader(pListView->GetControlHandle()))
            {
                if (pnm->hdr.code==HDN_ITEMCLICK) 
                {
                    // on header click
                    if ((pnm->iItem==ColumnIndexBlockSize)
                        ||(pnm->iItem==ColumnIndexFlags)
                        ||(pnm->iItem==ColumnIndexLockCount)
                        )
                        pListView->SetSortingType(CListview::SortingTypeNumber);
                    else
                        pListView->SetSortingType(CListview::SortingTypeString);
                }
            }

            if (pListView->OnNotify(wParam,lParam))
                break;

        }
        if (pToolbar)
        {
            if (pToolbar->OnNotify(wParam,lParam))
                break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CheckSize
// Object: called on WM_SIZING. Assume main dialog has a min with and hight
// Parameters :
//     in  : 
//     out :
//     In Out : RECT* pWinRect : window rect
//     return : 
//-----------------------------------------------------------------------------
void CheckSize(RECT* pWinRect)
{
    // check min width and min height
    if ((pWinRect->right-pWinRect->left)<MAIN_DIALOG_MIN_WIDTH)
        pWinRect->right=pWinRect->left+MAIN_DIALOG_MIN_WIDTH;
    if ((pWinRect->bottom-pWinRect->top)<MAIN_DIALOG_MIN_HEIGHT)
        pWinRect->bottom=pWinRect->top+MAIN_DIALOG_MIN_HEIGHT;
}

//-----------------------------------------------------------------------------
// Name: Resize
// Object: called on WM_SIZE. Resize all components
// Parameters :
//     return : 
//-----------------------------------------------------------------------------
void Resize()
{
    RECT RectListView;
    RECT RectGroupContent;
    RECT RectDialog;

    // resize toolbar
    pToolbar->Autosize();

    // get dialog rect
    CDialogHelper::GetClientWindowRect(mhWndDialog,mhWndDialog,&RectDialog);

    // resize Heap content group
    CDialogHelper::GetClientWindowRect(mhWndDialog,hWndHeapContentGroup,&RectGroupContent);
    SetWindowPos(hWndHeapContentGroup,HWND_NOTOPMOST,0,0,
        RectDialog.right-RectDialog.left-2*(RectGroupContent.left-RectDialog.left),
        RectDialog.bottom-RectGroupContent.top-(RectGroupContent.left-RectDialog.left),
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOMOVE);
    CDialogHelper::GetClientWindowRect(mhWndDialog,hWndHeapContentGroup,&RectGroupContent);

    // resize ListView
    CDialogHelper::GetClientWindowRect(mhWndDialog,pListView->GetControlHandle(),&RectListView);
    SetWindowPos(pListView->GetControlHandle(),HWND_NOTOPMOST,0,0,
        RectGroupContent.right-RectGroupContent.left-2*(RectListView.left-RectGroupContent.left),
        RectGroupContent.bottom-RectGroupContent.top-(RectListView.top-RectGroupContent.top)-(RectListView.left-RectGroupContent.left),
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOMOVE);    

    CDialogHelper::Redraw(mhWndDialog);
}

//-----------------------------------------------------------------------------
// Name: RefreshProcessList
// Object: refresh processes list
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void RefreshProcessList()
{
    DWORD CurrentProcessID=GetCurrentProcessId();

    TCHAR psz[MAX_PATH];
    SendMessage((HWND) hWndComboProcesses,(UINT) CB_RESETCONTENT,0,0);

    // create a snapshot and list processes
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
        if ((pe32.th32ProcessID==0)
            ||(pe32.th32ProcessID==4))
            continue;

        // don't show current process ID
        if (pe32.th32ProcessID==CurrentProcessID)
            continue;

        // add ProcessName(ProcessId) to combo
        _stprintf(psz,_T("%s (%u)"),pe32.szExeFile,pe32.th32ProcessID);
        SendMessage((HWND) hWndComboProcesses,(UINT) CB_ADDSTRING,0,(LPARAM)psz);
    } 
    while (Process32Next(hSnap, &pe32)); 

    // clean up the snapshot object. 
    CloseHandle (hSnap); 

    // select first process in combo
    SendMessage((HWND)hWndComboProcesses,(UINT) CB_SETCURSEL,0,0);

    // show Heap list for first process
    OnSelectedProcessChange();
}

//-----------------------------------------------------------------------------
// Name: CheckIfProcessIsAlive
// Object: check if process is alive
// Parameters :
//     in  : DWORD dwProcessId : Id of process to check
//     out :
//     return : TRUE if process is alive
//-----------------------------------------------------------------------------
BOOL CheckIfProcessIsAlive(DWORD dwProcessId)
{

    if (!CProcessHelper::IsAlive(dwProcessId))
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Process 0x%X doesn't exist anymore"),dwProcessId);
        MessageBox(mhWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        RefreshProcessList();

        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetProcessId
// Object: get selected process ID
// Parameters :
//     in  : 
//     out :
//     return : selected Process ID
//-----------------------------------------------------------------------------
DWORD GetProcessId()
{
    TCHAR psz[MAX_PATH];
    TCHAR* pc;

    int index = (int)SendMessage(hWndComboProcesses, CB_GETCURSEL, (WORD)0, 0L);
    if(SendMessage((HWND) hWndComboProcesses,(UINT) CB_GETLBTEXT,index,(LPARAM)psz)<=0)
        return 0;
    pc=_tcsrchr(psz,'(');
    if (!pc)
        return 0;
    pc++;

    return (DWORD)_ttol(pc);
}

//-----------------------------------------------------------------------------
// Name: CallBackPopUpMenuItem
// Object: Called each time a list view pop up menu item is click (for no CListview internal menu only)
// Parameters :
//     in  : UINT MenuID : Id of menu
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CallBackPopUpMenuItem(UINT MenuID,LPVOID UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);
    HEAP_CONTENT* pHeapContent;
    if (!pListView->GetItemUserData(pListView->GetSelectedIndex(),(LPVOID*)&pHeapContent))
        return;

    if (IsBadReadPtr(pHeapContent,sizeof(HEAP_CONTENT)))
        return;

    // unused until there's only one menu
    UNREFERENCED_PARAMETER(MenuID);
    /*
    TCHAR psz[MAX_PATH];
    this->pListView->pPopUpMenu->GetText(MenuID,psz,MAX_PATH);
    if (_tcscmp(psz,MENU_SHOW_HEX_DATA)==0)
    */
    CHexDisplay::Show(mhInstance,mhWndDialog,pHeapContent->pData,(DWORD)pHeapContent->HeapEntry.dwBlockSize);
}

//-----------------------------------------------------------------------------
// Name: ClearComboHeapList
// Object: Clear combo heap list and free it's associated memory
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void ClearComboHeapList()
{
    //  for each item, free lParam memory
    COMBOBOXEXITEM Item={0};
    Item.mask=CBEIF_LPARAM;
    for (int cnt=HeapListNumber-1;cnt>=0;cnt--)
    {
        Item.iItem=cnt;
        SendMessage(hWndComboHeapList,CBEM_GETITEM,0,(LPARAM) (PCOMBOBOXEXITEM) &Item);
        if (Item.lParam)
            delete ((HEAPLIST*)Item.lParam);

        SendMessage(hWndComboHeapList,CBEM_DELETEITEM,cnt,0);
    }

    // reset heap list counter
    HeapListNumber=0;
}

//-----------------------------------------------------------------------------
// Name: CallBackHeapList
// Object: Call back called for each heap list found
// Parameters :
//     in  : HEAPLIST* pHeapList : new heap list found
//     out :
//     return : FALSE to stop parsing 
//-----------------------------------------------------------------------------
BOOL CallBackHeapList(HEAPLIST* pHeapList,PVOID pUserParam)
{
    UNREFERENCED_PARAMETER(pUserParam);

    // store memory into local memory space
    HEAPLIST* pLocalHeapList=new HEAPLIST();
    memcpy(pLocalHeapList,pHeapList,sizeof(HEAPLIST));

    TCHAR psz[MAX_PATH];
    COMBOBOXEXITEM Item={0};
    Item.mask=CBEIF_TEXT|CBEIF_LPARAM;
    Item.iItem=-1;// add to the end of list
    Item.lParam=(LPARAM)pLocalHeapList;// add allocated object as lParam of combo
    Item.pszText=psz;
    Item.cchTextMax=MAX_PATH;

    // set combo item text
    _stprintf(psz,_T("HeapList %u"),HeapListNumber);
    if (HeapListNumber==0)
    {
        _tcscat(psz,_T(" (Default)"));
    }

    // add item to combo
    SendMessage(hWndComboHeapList,CBEM_INSERTITEM,0,(LPARAM) (PCOMBOBOXEXITEM) &Item);
    // increase counter
    HeapListNumber++;

    // select first heap
    SendMessage(hWndComboHeapList,CB_SETCURSEL,0,0);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CallBackHeapEntry
// Object: Call back called for each heap entry found
// Parameters :
//     in  : HEAPENTRY* pHeapEntry : new heap entry found
//     out :
//     return : FALSE to stop parsing
//-----------------------------------------------------------------------------
BOOL CallBackHeapEntry(HEAPENTRY* pHeapEntry,PVOID pUserParam)
{
    UNREFERENCED_PARAMETER(pUserParam);
    HEAP_CONTENT* pHeapContent;

    // if heap entry don't check current filters
    if (!CheckFilters(pHeapEntry))
        // don't add it
        return TRUE;
    
    // create object to store informations
    pHeapContent=new HEAP_CONTENT();

    // no more memory
    if (!pHeapContent)
    {
        // display error message
        MessageBox(mhWndDialog,_T("No more memory available"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        // stop parsing
        CloseHandle(CreateThread(NULL,0,CancelWalk,0,0,NULL));

        return FALSE;
    }
    
    // if there's no data in memory
    if (pHeapEntry->dwFlags==LF32_FREE)
        // we don't try to read memory
        pHeapContent->pData=0;
    else 
    {
        // allocate memory for storing data
        pHeapContent->pData=new BYTE[pHeapEntry->dwBlockSize];
        if (!pHeapContent->pData)
        {
            delete pHeapContent;

            // display error message
            MessageBox(mhWndDialog,_T("No more memory available"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            // stop parsing
            CloseHandle(CreateThread(NULL,0,CancelWalk,0,0,NULL));

            return FALSE;
        }

        // get heap entry data
        DWORD dwReadSize;
        if (!pProcessMemory->Read((LPCVOID)pHeapEntry->dwAddress,pHeapContent->pData,pHeapEntry->dwBlockSize,&dwReadSize))
        {
            delete pHeapContent->pData;
            pHeapContent->pData=0;
        }
    }

    // copy HeapEntry content
    memcpy(&pHeapContent->HeapEntry,pHeapEntry,sizeof(HEAPENTRY));


    //////////////////////////////////
    // ListView filling
    //////////////////////////////////

    TCHAR* ppc[NB_COLUMNS];
    TCHAR pcAddress[64];
    TCHAR pcBlockSize[64];
    TCHAR pcIndexFlag[64];
    TCHAR pcLockCount[64];
    ppc[ColumnIndexAddress]=pcAddress;
    ppc[ColumnIndexBlockSize]=pcBlockSize;
    ppc[ColumnIndexFlags]=pcIndexFlag;
    ppc[ColumnIndexLockCount]=pcLockCount;

    _stprintf(pcAddress,_T("0x%.8X"),pHeapEntry->dwAddress);
    _stprintf(pcBlockSize,_T("%u"),pHeapEntry->dwBlockSize);

    switch(pHeapEntry->dwFlags)
    //LF32_FIXED The memory block has a fixed (unmovable) location. 
    //LF32_FREE The memory block is not used. 
    //LF32_MOVEABLE The memory block location can be moved. 
    {
    case LF32_FIXED:
        _tcscpy(pcIndexFlag,HEAP_WALKER_MEMORY_FLAG_FIXED);
        break;
    case LF32_FREE:
        _tcscpy(pcIndexFlag,HEAP_WALKER_MEMORY_FLAG_FREE);
        break;
    case LF32_MOVEABLE:
        _tcscpy(pcIndexFlag,HEAP_WALKER_MEMORY_FLAG_MOVABLE);
        break;
    default:
        _stprintf(pcIndexFlag,_T("%u"),pHeapEntry->dwFlags);
    }

    _stprintf(pcLockCount,_T("%u"),pHeapEntry->dwLockCount);
   
    // show memory content (hex + ascii)
    if (pHeapContent->pData)
    {
        BYTE CurrentByte;
        BOOL MemoryAllocationError=FALSE;
        // allocate memory
        ppc[ColumnIndexData]=new TCHAR[(pHeapEntry->dwBlockSize*3)+1];
        ppc[ColumnIndexAsciiData]=new TCHAR[pHeapEntry->dwBlockSize+1];
        // check memory allocation
        if (ppc[ColumnIndexData]==NULL)
        {
            MemoryAllocationError=TRUE;
            if(ppc[ColumnIndexAsciiData])
            {
                delete ppc[ColumnIndexAsciiData];
                ppc[ColumnIndexAsciiData]=NULL;
            }
        }
        if (ppc[ColumnIndexAsciiData]==NULL)
        {
            MemoryAllocationError=TRUE;
            if(ppc[ColumnIndexData])
            {
                delete ppc[ColumnIndexData];
                ppc[ColumnIndexData]=NULL;
            }
        }
        if (MemoryAllocationError)
        {
            delete pHeapContent->pData;
            delete pHeapContent;

            // display error message
            MessageBox(mhWndDialog,_T("No more memory available"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            // stop parsing
            CloseHandle(CreateThread(NULL,0,CancelWalk,0,0,NULL));

            return FALSE;
        }

        ppc[ColumnIndexData][pHeapEntry->dwBlockSize*3]=0;
        ppc[ColumnIndexAsciiData][pHeapEntry->dwBlockSize]=0;

        // translate buffer to readable data
        for (DWORD cnt=0;cnt<pHeapContent->HeapEntry.dwBlockSize;cnt++)
        {
            CurrentByte=pHeapContent->pData[cnt];
            // print hex representation
            _stprintf(&(ppc[ColumnIndexData][3*cnt]),_T("%.2X "),CurrentByte);

            if ((CurrentByte>=0x20)&&(CurrentByte<=0x7E))
                ppc[ColumnIndexAsciiData][cnt]=CurrentByte;
            else
                ppc[ColumnIndexAsciiData][cnt]='.';
        }
        // add item to listview
        pListView->AddItemAndSubItems(NB_COLUMNS,ppc,FALSE,pHeapContent);

        // free memory
        delete ppc[ColumnIndexData];
        delete ppc[ColumnIndexAsciiData];
    }
    else
    {
        // add item to listview
        pListView->AddItemAndSubItems(NB_COLUMNS-2,ppc,FALSE,pHeapContent);
    }

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: CheckAndReportDeadProcess
// Object: check if a process is still alive or dead, if dead refresh process list
// Parameters :
//     in  : 
//     out :
//     return : TRUE if process is still alive
//-----------------------------------------------------------------------------
BOOL CheckAndReportDeadProcess()
{
    if (!CProcessHelper::IsAlive(CurrentProcessID))
    {
        MessageBox(mhWndDialog,_T("Process doesn't exists anymore"),_T("Error"),MB_ICONERROR|MB_OK|MB_TOPMOST);
        RefreshProcessList();
        return FALSE;
    }
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: OnSelectedProcessChange
// Object: Called when selected process changes
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void OnSelectedProcessChange()
{
    // clear heap list
    ClearComboHeapList();
    // get process Id of selected process
    CurrentProcessID=GetProcessId();
    // check if process is still alive
    if (!CheckAndReportDeadProcess())
        return;

    // walk heap list for selected process
    CHeapWalk::WalkHeapList(CurrentProcessID,CallBackHeapList,NULL);
}

//-----------------------------------------------------------------------------
// Name: CancelWalk
// Object: Cancel heap entries walk (for thread cancellation)
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI CancelWalk(LPVOID lpParameter)
{
    UNREFERENCED_PARAMETER(lpParameter);
    Cancel();
    return 0;
}

//-----------------------------------------------------------------------------
// Name: StartStop
// Object: Start or stop heap entries walking
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void StartStop()
{
    // if a heap is being parsed, 
    if (IsAHeapBeingParsed())
    {
        // stop parsing
        pToolbar->EnableButton(IDC_BUTTON_START_STOP,FALSE);
        CloseHandle(CreateThread(NULL,0,CancelWalk,0,0,NULL));
    }   
    else
    {
        // start parsing
        pToolbar->EnableButton(IDC_BUTTON_START_STOP,FALSE);
        Start();
    }
        
}

//-----------------------------------------------------------------------------
// Name: Cancel
// Object: Cancel heap entries walk (blocking)
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void Cancel()
{
    if (IsAHeapBeingParsed())
    {
        if (!hWalkingThread)
            return;

        // set cancel event
        SetEvent(hEvtCancel);
        // wait for end of walking thread
        WaitForSingleObject(hWalkingThread,TIMEOUT_CANCEL_WALKING_THREAD);

        // close walking thread
        CloseHandle(hWalkingThread);
        hWalkingThread=NULL;

        SetToolbarMode(FALSE);
    }
}

//-----------------------------------------------------------------------------
// Name: IsAHeapBeingParsed
// Object: allow to know if a heap is being parsed
// Parameters :
//     in  : 
//     out :
//     return : TRUE if a heap is being parsed
//-----------------------------------------------------------------------------
BOOL IsAHeapBeingParsed()
{
    return (hWalkingThread!=NULL);
}

//-----------------------------------------------------------------------------
// Name: Start
// Object: Start heap entries walking
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void Start()
{
    if (!CheckAndReportDeadProcess())
        return;

    // cancel previous walk if any
    Cancel();

    // reset cancel event
    ResetEvent(hEvtCancel);

    // walk heap in a new thread
    hWalkingThread=CreateThread(NULL,0,WalkHeap,NULL,0,NULL);

    SetToolbarMode(TRUE);
}
//-----------------------------------------------------------------------------
// Name: WalkHeap
// Object: Start heap entries walking
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
DWORD WINAPI WalkHeap(LPVOID lpParameter)
{
    UNREFERENCED_PARAMETER(lpParameter);

    ParseDoneOnce=TRUE;

    // clear list view and it's associated memory
    ClearListviewHeapEntries();
    int index = (int)SendMessage(hWndComboHeapList, CB_GETCURSEL, (WORD)0, 0L);

    HEAPLIST* pLocalHeapList;
    COMBOBOXEXITEM Item={0};
    Item.mask=CBEIF_LPARAM;
    Item.iItem=index;// add to the end of list

    // get heap list object
    SendMessage(hWndComboHeapList,CBEM_GETITEM,0,(LPARAM)&Item);
    pLocalHeapList=(HEAPLIST*)Item.lParam;
	if (!pLocalHeapList)
	{
		CloseHandle(hWalkingThread);
		hWalkingThread=NULL;
		MessageBox(mhWndDialog,_T("Error parsing heap"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
		// at the end of walk, restore toolbar buttons
		SetToolbarMode(FALSE);
		return (DWORD)-1;
	}

    // create process memory object to access process memory
    pProcessMemory=new CProcessMemory(CurrentProcessID,TRUE,FALSE);

    // start heap entries walking
    if (CHeapWalk::WalkHeapEntry(pLocalHeapList,hEvtCancel,CallBackHeapEntry,NULL))
    {
        CloseHandle(hWalkingThread);
        hWalkingThread=NULL;
        MessageBox(mhWndDialog,_T("Heap successfully parsed"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
    }

    // at the end of walk, restore toolbar buttons
    SetToolbarMode(FALSE);

    delete pProcessMemory;

    return 0;
}

//-----------------------------------------------------------------------------
// Name: SetToolbarMode
// Object: set toolbar mode (enable or disable some functionalities)
// Parameters :
//     in  : BOOL bIsWalking : TRUE if in walking mode
//     out :
//     return : 
//-----------------------------------------------------------------------------
void SetToolbarMode(BOOL bIsWalking)
{
    if (bIsWalking)
    {
        pToolbar->ReplaceIcon(IDC_BUTTON_START_STOP,CToolbar::ImageListTypeEnable,IDI_ICON_CANCEL);
        pToolbar->ReplaceIcon(IDC_BUTTON_START_STOP,CToolbar::ImageListTypeHot,IDI_ICON_CANCEL);
        pToolbar->ReplaceText(IDC_BUTTON_START_STOP,_T("Cancel"));
        pToolbar->EnableButton(IDC_BUTTON_START_STOP,TRUE);
        pToolbar->EnableButton(IDC_BUTTON_FILTER,FALSE);
        pToolbar->EnableButton(IDC_BUTTON_SAVE,FALSE);
    }
    else
    {
        pToolbar->ReplaceIcon(IDC_BUTTON_START_STOP,CToolbar::ImageListTypeEnable,IDI_ICON_START);
        pToolbar->ReplaceIcon(IDC_BUTTON_START_STOP,CToolbar::ImageListTypeHot,IDI_ICON_START);
        pToolbar->ReplaceText(IDC_BUTTON_START_STOP,_T("Start"));
        pToolbar->EnableButton(IDC_BUTTON_START_STOP,TRUE);
        pToolbar->EnableButton(IDC_BUTTON_FILTER,TRUE);
        pToolbar->EnableButton(IDC_BUTTON_SAVE,TRUE);
    }
}

//-----------------------------------------------------------------------------
// Name: ClearListviewHeapEntries
// Object: clear heap content list view and associated memory
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void ClearListviewHeapEntries()
{
    HEAP_CONTENT* pHeapContent;

    // for each item
    int NbItems=pListView->GetItemCount();
    for (int cnt=NbItems-1;cnt>=0;cnt--)
    {

        // delete memory associated to item
        if (pListView->GetItemUserData(cnt,(LPVOID*)&pHeapContent))
        {
            if (!IsBadReadPtr(pHeapContent,sizeof(HEAP_CONTENT)))
            {
                if (pHeapContent->pData)
                    delete pHeapContent->pData;
                delete pHeapContent;
            }
        }

        // remove item
        pListView->RemoveItem(cnt);
    }
}

//-----------------------------------------------------------------------------
// Name: ApplyFiltersToHeapContent
// Object: apply filters on current heap list
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void ApplyFiltersToHeapContent()
{
    // as filters can be less restrictive than current, 
    // and as we don't store in memory non matching entries
    // we have to do another walk
    Start();
}
//-----------------------------------------------------------------------------
// Name: CheckFilters
// Object: check filters for provided heap entry
// Parameters :
//     in  : HEAPENTRY* pHeapEntry : pointer of heap entry to check
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CheckFilters(HEAPENTRY* pHeapEntry)
{
    if (pOptions->FilterMinSize!=0)
    {
        if (pHeapEntry->dwBlockSize<pOptions->FilterMinSize)
            return FALSE;
    }
    if (pOptions->FilterMaxSize!=0)
    {
        if (pHeapEntry->dwBlockSize>pOptions->FilterMaxSize)
            return FALSE;
    }
    if (pOptions->FilterMinAddress!=0)
    {
        if (pHeapEntry->dwAddress<pOptions->FilterMinAddress)
            return FALSE;
    }
    if (pOptions->FilterMaxAddress!=0)
    {
        if (pHeapEntry->dwAddress>pOptions->FilterMaxAddress)
            return FALSE;
    }
    if (pOptions->FilterMemoryFlags!=COptions::FILTERMEMORYFLAGS_ALL)
    {
        switch(pOptions->FilterMemoryFlags)
        {
        case COptions::FILTERMEMORYFLAGS_FIXED:
            if (pHeapEntry->dwFlags!=LF32_FIXED)
                return FALSE;
            break;
        case COptions::FILTERMEMORYFLAGS_MOVABLE:
            if (pHeapEntry->dwFlags!=LF32_MOVEABLE)
                return FALSE;
            break;
        case COptions::FILTERMEMORYFLAGS_FREE:
            if (pHeapEntry->dwFlags!=LF32_FREE)
                return FALSE;
            break;
        }
    }
    return TRUE;
}