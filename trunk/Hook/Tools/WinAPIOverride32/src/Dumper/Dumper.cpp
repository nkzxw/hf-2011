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
#pragma once
#include "dumper.h"

//////////////////////// globals //////////////////////// 
HINSTANCE mhInstance;
HWND mhWndDialog;
HWND mhContextRegistershDlg;
CListview* mpCListviewProcesses=NULL;
CListview* mpCListviewModules=NULL;
CListview* mpCListviewThreads=NULL;
CLinkListSimple* mpLinkListSubDialogs;
BOOL UserMode=TRUE;
BOOL IsClosing=FALSE;
BOOL InitializationFinished=FALSE;
BOOL bRefreshing = FALSE;

DWORD mpProcessesMenuIdDump=0;
DWORD mpProcessesMenuIdSuspend=0;
DWORD mpProcessesMenuIdResume=0;
DWORD mpProcessesMenuIdTerminate=0;
DWORD mpProcessesMenuIdProcessPriority=0;
CPopUpMenu* mpPopUpMenuProcessPriority=NULL;
DWORD mpProcessesMenuIdProcessPriorityAboveNormal=0;
DWORD mpProcessesMenuIdProcessPriorityBelowNormal=0;
DWORD mpProcessesMenuIdProcessPriorityHigh=0;
DWORD mpProcessesMenuIdProcessPriorityIdle=0;
DWORD mpProcessesMenuIdProcessPriorityNormal=0;
DWORD mpProcessesMenuIdProcessPriorityRealTime=0;

DWORD mpModulesMenuIdDump=0;
DWORD mpModulesMenuIdCheckIntegrity=0;
DWORD mpModulesMenuIdEject=0;


CPopUpMenu* mpPopUpMenuIntegrityChecking=NULL;
DWORD mpPopUpMenuIntegrityCheckingModule=0;
DWORD mpPopUpMenuIntegrityCheckingProcess=0;
DWORD mpPopUpMenuIntegrityCheckingCheckNotExecutableSections=0;
DWORD mpPopUpMenuIntegrityCheckingCheckWritableSections=0;
DWORD mpPopUpMenuIntegrityCheckingShowRebasing=0;

DWORD mpThreadMenuIdSuspend=0;
DWORD mpThreadMenuIdResume=0;
DWORD mpThreadMenuIdTerminate=0;
DWORD mpThreadMenuIdContext=0;
DWORD mpThreadMenuIdCallStack=0;
DWORD mpThreadMenuIdSuspendedCount=0;
DWORD mpThreadMenuIdThreadPriorityTimeCritical=0;
DWORD mpThreadMenuIdThreadPriorityHighest=0;
DWORD mpThreadMenuIdThreadPriorityAboveNormal=0;
DWORD mpThreadMenuIdThreadPriorityNormal=0;
DWORD mpThreadMenuIdThreadPriorityBelowNormal=0;
DWORD mpThreadMenuIdThreadPriorityLowest=0;
DWORD mpThreadMenuIdThreadPriorityIdle=0;
DWORD mpThreadMenuIdThreadPriority=0;
CPopUpMenu* mpPopUpMenuThreadPriority=NULL;

CKernelProcessesInfo* pKernelProcesseInfo=NULL;
CToolbar* mpProcessesToolbar=NULL;
CRebar* mpProcessesRebar=NULL;
CToolbar* mpThreadsToolbar=NULL;
CSplitter* mpProcessesModulesSplitter=NULL;
CSplitter* mpModulesThreadsSplitter=NULL;
CPopUpMenu* mpPopUpMenuDumpToExe=NULL;
DWORD mpDumpToExeMenuIdRemoveNotRAW=0;
DWORD mpDumpToExeMenuIdModifyPE=0;
TCHAR ApplicationName[MAX_PATH];

HMODULE hModInjLib=NULL;
InjectLib pInjectLibrary=NULL;
EjectLib  pEjectLibrary=NULL;

#define STR_PROCESS_TYPE_64 _T("64")
#define STR_PROCESS_TYPE_32 _T("32")

// listviews columns header

enum ColumnProcesses
{
	ColumnProcessesName=0,
	ColumnProcessType,
	ColumnProcessesId,
	ColumnProcessesNbThreads,
	ColumnProcessesParentId,
	ColumnProcessesPriorityClass,
	ColumnProcessesPriorityCreationTime,

	NB_PROCESSES_COLUMNS
};
CListview::COLUMN_INFO mpColumnInfoProcesses[NB_PROCESSES_COLUMNS]={
                                                                        {_T("Name"),280,LVCFMT_LEFT},
																		{_T("Type"),40,LVCFMT_LEFT},
                                                                        {_T("ID"),160,LVCFMT_LEFT},
                                                                        {_T("Nb Threads"),90,LVCFMT_CENTER},
                                                                        {_T("Parent ID"),160,LVCFMT_LEFT},
                                                                        {_T("Priority Class"),90,LVCFMT_CENTER},
                                                                        {_T("CreationTime"),150,LVCFMT_CENTER}
                                                                    };



enum ColumnModules
{
	ColumnModulesName=0,
	ColumnModulesAddress,
	ColumnModulesEntryPoint,
	ColumnModulesLoadCount,
	ColumnModulesTlsIndex,
	ColumnModulesFlags,
	ColumnModulesTimeDateStamp,

	NB_MODULES_COLUMNS
};
#define ModulesLoadCountStatic _T("Static")
CListview::COLUMN_INFO mpColumnInfoModules[NB_MODULES_COLUMNS]={
                                                                    {_T("Name"),400,LVCFMT_LEFT},
                                                                    {_T("Address"),150,LVCFMT_CENTER},
                                                                    {_T("Entry Point"),80,LVCFMT_CENTER},
                                                                    {_T("Load Count"),70,LVCFMT_CENTER},
                                                                    {_T("Tls Index"),70,LVCFMT_CENTER},
                                                                    {_T("Flags"),80,LVCFMT_CENTER},
                                                                    {_T("Date Stamp"),80,LVCFMT_CENTER}
                                                                };


enum ColumnThreads
{
	ColumnThreadsId=0,
	ColumnThreadsUsage,
	ColumnThreadsPriority,
	ColumnThreadsCreationTime,
	ColumnThreadsUserTime,
	ColumnThreadsKernelTime,

	NB_THREADS_COLUMNS
};
CListview::COLUMN_INFO mpColumnInfoThreads[NB_THREADS_COLUMNS]={
                                                                    {_T("Thread ID"),150,LVCFMT_LEFT},
                                                                    {_T("Usage"),100,LVCFMT_CENTER},
                                                                    {_T("Priority"),100,LVCFMT_CENTER},
                                                                    {_T("CreationTime"),150,LVCFMT_CENTER},
                                                                    {_T("UserTime"),150,LVCFMT_CENTER},
                                                                    {_T("KernelTime"),150,LVCFMT_CENTER}
                                                                };



enum ColumnProcessesKernel
{
	ColumnProcessesKernelName=0,
	ColumnProcessesKernelBaseAddress,
	ColumnProcessesKernelSize,
	ColumnProcessesKernelIndex,
	ColumnProcessesKernelLoadCount,
	ColumnProcessesKernelRank,
	ColumnProcessesKernelFlags,
	ColumnProcessesKernelPath,

	NB_PROCESSES_COLUMNS_KERNELMODE
};
CListview::COLUMN_INFO mpColumnInfoProcessesKernelMode[NB_PROCESSES_COLUMNS_KERNELMODE]=
                                                    {
                                                        {_T("Name"),140,LVCFMT_LEFT},
                                                        {_T("Base Address"),100,LVCFMT_CENTER},
                                                        {_T("Size"),100,LVCFMT_LEFT},
                                                        {_T("Index"),70,LVCFMT_LEFT},
                                                        {_T("Load Count"),70,LVCFMT_CENTER},
                                                        {_T("Rank"),70,LVCFMT_CENTER},
                                                        {_T("Flags"),70,LVCFMT_CENTER},
                                                        {_T("Path"),260,LVCFMT_LEFT}
                                                    };


int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    HWND hwnd=NULL;
    mhInstance=hInstance;


    // get app name
    CStdFileOperations::GetAppName(ApplicationName,MAX_PATH);

    // to enable Xp style
    InitCommonControls();

    // gives the debug privileges
    CPrivilege Privilege(FALSE);
    Privilege.SetPrivilege(SE_DEBUG_NAME,TRUE);

    pKernelProcesseInfo=new CKernelProcessesInfo();

    // show main window
    DialogBox(hInstance, (LPCTSTR)IDD_DIALOG_DUMPER, hwnd, (DLGPROC)DumperWndProc);

    delete pKernelProcesseInfo;

    if (hModInjLib)
        FreeLibrary(hModInjLib);

	return 0;
}

DWORD WINAPI ShowWindowThreadProcessId(LPVOID lpParameter)
{
    CWindowThreadProcessIdUserInterface* pWindowThreadProcessIdUserInterface = new CWindowThreadProcessIdUserInterface();
    pWindowThreadProcessIdUserInterface->Show(mhInstance,0,IDD_DIALOG_WINDOW_THREAD_PROCESS_ID,IDI_ICON_WINDOW_THREAD_PROCESS_ID);
    delete pWindowThreadProcessIdUserInterface;
    return 0;
}

//-----------------------------------------------------------------------------
// Name: OnInit
// Object: Initialize Dumper dialog
// Parameters :
//     in : HWND hDlg
// Return : 
//-----------------------------------------------------------------------------
void OnInit(HWND hDlg)
{
    LONG_PTR Style;
    RECT Rect;
    RECT RectWindow;
    HWND hItem;


    mhWndDialog=hDlg;
    // load icon
    CDialogHelper::SetIcon(hDlg,IDI_ICON_APP);


    ////////////////////
    // Process List View
    ////////////////////
    mpCListviewProcesses=new CListview(GetDlgItem(hDlg, IDC_LIST_PROCESSES));
    // full row select
    mpCListviewProcesses->SetStyle(TRUE,FALSE,FALSE,FALSE);
    // load listview column names
    mpCListviewProcesses->InitListViewColumns(NB_PROCESSES_COLUMNS,mpColumnInfoProcesses);
    // set callback
    mpCListviewProcesses->SetSelectItemCallback(ProcessSelectionCallBack,NULL);
    mpCListviewProcesses->SetPopUpMenuItemClickCallback(ProcessesPopUpCallBack,NULL);
    mpProcessesMenuIdDump=mpCListviewProcesses->pPopUpMenu->Add(_T("Dump"),(UINT)0);
    mpCListviewProcesses->pPopUpMenu->AddSeparator(1);

    mpPopUpMenuProcessPriority=new CPopUpMenu(mpCListviewProcesses->pPopUpMenu);

    mpProcessesMenuIdProcessPriorityRealTime=mpPopUpMenuProcessPriority->Add(_T("Realtime"));
    mpProcessesMenuIdProcessPriorityHigh=mpPopUpMenuProcessPriority->Add(_T("High"));
    mpProcessesMenuIdProcessPriorityAboveNormal=mpPopUpMenuProcessPriority->Add(_T("Above Normal"));
    mpProcessesMenuIdProcessPriorityNormal=mpPopUpMenuProcessPriority->Add(_T("Normal"));
    mpProcessesMenuIdProcessPriorityBelowNormal=mpPopUpMenuProcessPriority->Add(_T("Below Normal"));
    mpProcessesMenuIdProcessPriorityIdle=mpPopUpMenuProcessPriority->Add(_T("Idle"));

    mpProcessesMenuIdProcessPriority=mpCListviewProcesses->pPopUpMenu->AddSubMenu(_T("Priority"),mpPopUpMenuProcessPriority,2);
    mpCListviewProcesses->pPopUpMenu->AddSeparator(3);

    mpProcessesMenuIdSuspend=mpCListviewProcesses->pPopUpMenu->Add(_T("Suspend"),4);
    mpProcessesMenuIdResume=mpCListviewProcesses->pPopUpMenu->Add(_T("Resume"),5);
    mpProcessesMenuIdTerminate=mpCListviewProcesses->pPopUpMenu->Add(_T("Terminate"),6);

    mpCListviewProcesses->pPopUpMenu->AddSeparator(7);


    ////////////////////
    // Module List View
    ////////////////////
    mpCListviewModules=new CListview(GetDlgItem(hDlg, IDC_LIST_MODULES));
    // full row select
    mpCListviewModules->SetStyle(TRUE,FALSE,FALSE,FALSE);
    // load listview column names
    mpCListviewModules->InitListViewColumns(NB_MODULES_COLUMNS,mpColumnInfoModules);
    // set callback
    mpCListviewModules->SetPopUpMenuItemClickCallback(ModulesPopUpCallBack,NULL);
    mpModulesMenuIdDump=mpCListviewModules->pPopUpMenu->Add(_T("Dump"),(UINT)0);
    mpModulesMenuIdCheckIntegrity=mpCListviewModules->pPopUpMenu->Add(_T("Check Integrity"),1);
    mpModulesMenuIdEject=mpCListviewModules->pPopUpMenu->Add(_T("Eject"),2);
    mpCListviewModules->pPopUpMenu->AddSeparator(3);


    ////////////////////
    // Threads List View
    ////////////////////

    mpCListviewThreads=new CListview(GetDlgItem(hDlg, IDC_LIST_THREADS));
    mpCListviewThreads->SetStyle(TRUE,FALSE,FALSE,FALSE);
    // load listview column names
    mpCListviewThreads->InitListViewColumns(NB_THREADS_COLUMNS,mpColumnInfoThreads);
    // set callback
    mpCListviewThreads->SetPopUpMenuItemClickCallback(ThreadsPopUpCallBack,NULL);
    mpThreadMenuIdCallStack=mpCListviewThreads->pPopUpMenu->Add(_T("Call Stack"),(UINT)0);
    mpThreadMenuIdContext=mpCListviewThreads->pPopUpMenu->Add(_T("Context"),1);
    mpCListviewThreads->pPopUpMenu->AddSeparator(2);

    mpPopUpMenuThreadPriority=new CPopUpMenu(mpCListviewThreads->pPopUpMenu);
    mpThreadMenuIdThreadPriorityTimeCritical=mpPopUpMenuThreadPriority->Add(_T("Time Critical"));
    mpThreadMenuIdThreadPriorityHighest=mpPopUpMenuThreadPriority->Add(_T("Highest"));
    mpThreadMenuIdThreadPriorityAboveNormal=mpPopUpMenuThreadPriority->Add(_T("Above Normal"));
    mpThreadMenuIdThreadPriorityNormal=mpPopUpMenuThreadPriority->Add(_T("Normal"));
    mpThreadMenuIdThreadPriorityBelowNormal=mpPopUpMenuThreadPriority->Add(_T("Below Normal"));
    mpThreadMenuIdThreadPriorityLowest=mpPopUpMenuThreadPriority->Add(_T("Lowest"));
    mpThreadMenuIdThreadPriorityIdle=mpPopUpMenuThreadPriority->Add(_T("Idle"));

    mpThreadMenuIdThreadPriority=mpCListviewThreads->pPopUpMenu->AddSubMenu(_T("Priority"),mpPopUpMenuThreadPriority,3);

    mpCListviewThreads->pPopUpMenu->AddSeparator(4);


    mpThreadMenuIdSuspend=mpCListviewThreads->pPopUpMenu->Add(_T("Suspend"),5);
    mpThreadMenuIdResume=mpCListviewThreads->pPopUpMenu->Add(_T("Resume"),6);
    mpThreadMenuIdSuspendedCount=mpCListviewThreads->pPopUpMenu->Add(_T("Suspended Count"),7);
    mpCListviewThreads->pPopUpMenu->AddSeparator(8);
    mpThreadMenuIdTerminate=mpCListviewThreads->pPopUpMenu->Add(_T("Terminate"),9);
    mpCListviewThreads->pPopUpMenu->AddSeparator(10);

    // set number sorting type
    mpCListviewThreads->SetSortingType(CListview::SortingTypeNumber);

    RECT RectGroup;
    // threads group
    hItem=GetDlgItem(mhWndDialog,IDC_STATIC_GROUP_PROCESS_THREADS);
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&RectGroup);

    // threads list view 
    hItem=mpCListviewThreads->GetControlHandle();
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
                 RectGroup.left+SPACE_BETWEEN_CONTROLS,
                 Rect.top,
                 0,
                 0,
                 SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);


    hItem=GetDlgItem(mhWndDialog,IDC_STATIC_GROUP_PROCESS_MODULES);
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&RectGroup);
    hItem=mpCListviewModules->GetControlHandle();
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
                 RectGroup.left+SPACE_BETWEEN_CONTROLS,
                 Rect.top,
                 0,
                 0,
                 SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);



    // ProcessesToolbar
    mpProcessesToolbar=new CToolbar(mhInstance,mhWndDialog,TRUE,TRUE,24,24);
    mpProcessesToolbar->EnableDivider(FALSE);
    mpProcessesToolbar->SetDropDownMenuCallBack(ProcessesDropDownMenuCallBack,NULL);
    
/*
    // add WS_GROUP style to toolbar
    Style=GetWindowLongPtr(mpProcessesToolbar->GetControlHandle(),GWL_STYLE);
    Style|=WS_GROUP;
    // apply new style
    SetWindowLongPtr(mpProcessesToolbar->GetControlHandle(),GWL_STYLE,Style);
*/
    mpProcessesToolbar->AddButton(IDC_BUTTON_REFRESH,IDI_ICON_REFRESH,_T("Refresh Processes List"));
    mpProcessesToolbar->AddSeparator();

    mpProcessesToolbar->AddButton(IDC_BUTTON_DUMP,_T("Dump"),IDI_ICON_DUMP,_T("Dump Selected Module"));
    mpProcessesToolbar->AddButton(IDC_BUTTON_RAW_DUMP,_T("Raw Dump"),IDI_ICON_DUMP,_T("Raw Dump"));
    mpPopUpMenuDumpToExe=new CPopUpMenu();
    mpDumpToExeMenuIdRemoveNotRAW=mpPopUpMenuDumpToExe->Add(_T("By Removing not RAW Parts"));
    mpDumpToExeMenuIdModifyPE=mpPopUpMenuDumpToExe->Add(_T("By Modifying PE"));
    mpProcessesToolbar->AddDropDownButton(IDC_BUTTON_DUMP_TO_EXE,
                                        _T("Dump To Exe"),
                                        IDI_ICON_DUMP_TO_EXE,
                                        _T("Convert Dump to an Executable Binary"),
                                        mpPopUpMenuDumpToExe,
                                        FALSE);
    mpProcessesToolbar->AddSeparator();

    mpProcessesToolbar->AddButton(IDC_BUTTON_TERMINATE_PROCESS,IDI_ICON_KILL,IDI_ICON_KILL_DISABLED,_T("Terminate Process"));
    mpProcessesToolbar->AddSeparator();

    mpProcessesToolbar->AddButton(IDC_BUTTON_SUSPEND_PROCESS,IDI_ICON_SUSPEND,IDI_ICON_SUSPEND_DISABLED,_T("Suspend Process Once"));
    mpProcessesToolbar->AddButton(IDC_BUTTON_RESUME_PROCESS,IDI_ICON_RESUME,IDI_ICON_RESUME_DISABLED,_T("Resume Process Once"));
    mpProcessesToolbar->AddSeparator();

    mpProcessesToolbar->AddDropDownButton(IDC_BUTTON_PRIORITY,
                                        _T("Process Priority"),
                                        IDI_ICON_PRIORITY,
                                        _T("Change Process Priotity"),
                                        mpPopUpMenuProcessPriority,
                                        TRUE);
    mpProcessesToolbar->AddSeparator();

    mpProcessesToolbar->AddButton(IDC_BUTTON_INJLIB,IDI_ICON_INJLIB,IDI_ICON_INJLIB,_T("Inject Library Inside Selected Process"));
    mpProcessesToolbar->AddButton(IDC_BUTTON_EJECTLIB,IDI_ICON_EJECTLIB,IDI_ICON_EJECTLIB,_T("Eject Selected Module From Selected Process"));

    mpProcessesToolbar->AddSeparator();
    mpProcessesToolbar->AddButton(IDC_BUTTON_WINDOW_THREAD_PROCESS_ID,IDI_ICON_WINDOW_THREAD_PROCESS_ID,IDI_ICON_WINDOW_THREAD_PROCESS_ID,_T("Get Window Thread And Process Id"));

    mpProcessesToolbar->AddSeparator();

    mpPopUpMenuIntegrityChecking=new CPopUpMenu();
    mpPopUpMenuIntegrityCheckingProcess=mpPopUpMenuIntegrityChecking->Add(_T("Check Integrity of Selected Process"));
    mpPopUpMenuIntegrityCheckingModule=mpPopUpMenuIntegrityChecking->Add(_T("Check Integrity of Selected Module"));
    mpPopUpMenuIntegrityChecking->AddSeparator();
    mpPopUpMenuIntegrityCheckingCheckNotExecutableSections=mpPopUpMenuIntegrityChecking->Add(_T("Check Not Executable Sections"));
    mpPopUpMenuIntegrityCheckingCheckWritableSections=mpPopUpMenuIntegrityChecking->Add(_T("Check Writable Sections"));
    mpPopUpMenuIntegrityCheckingShowRebasing=mpPopUpMenuIntegrityChecking->Add(_T("Show Rebasing"));

    mpProcessesToolbar->AddDropDownButton(IDC_BUTTON_INTEGRITY_CHECKING,
                                        _T("Integrity"),
                                        IDI_ICON_INTEGRITY_CHECKING,
                                        _T("Process / Module Integrity Checking"),
                                        mpPopUpMenuIntegrityChecking,
                                        FALSE);
    mpProcessesToolbar->AddSeparator();

    mpProcessesToolbar->AddButton(IDC_BUTTON_MEMORY,_T("Memory"),IDI_ICON_MEMORY,_T("Edit Processes Memory"));
    mpProcessesToolbar->AddSeparator();

    mpProcessesToolbar->AddButton(IDC_BUTTON_USER_KERNEL_MODE,_T("Kernel"),IDI_ICON_KERNEL,_T("Switch to Kernel Mode"));

    mpProcessesRebar=new CRebar(mhInstance,mhWndDialog);
    mpProcessesRebar->AddToolBarBand(mpProcessesToolbar->GetControlHandle(),NULL,FALSE,FALSE,TRUE);

    // add WS_GROUP style to toolbar
    Style=GetWindowLongPtr(mpProcessesRebar->GetControlHandle(),GWL_STYLE);
    Style|=WS_GROUP;
    // apply new style
    SetWindowLongPtr(mpProcessesRebar->GetControlHandle(),GWL_STYLE,Style);


    // ThreadsToolbar
    mpThreadsToolbar=new CToolbar(mhInstance,mhWndDialog,TRUE,TRUE,24,24);
	mpThreadsToolbar->SetPosition(CToolbar::ToolbarPosition_USER);
    mpThreadsToolbar->EnableDivider(FALSE);
    mpThreadsToolbar->EnableParentAlign(FALSE);
    mpThreadsToolbar->SetDropDownMenuCallBack(ThreadsDropDownMenuCallBack,NULL);

    // add WS_GROUP style to toolbar
    Style=GetWindowLongPtr(mpThreadsToolbar->GetControlHandle(),GWL_STYLE);
    Style|=WS_GROUP;
    // apply new style
    SetWindowLongPtr(mpThreadsToolbar->GetControlHandle(),GWL_STYLE,Style);


    mpThreadsToolbar->AddButton(IDC_BUTTON_CALLSTACK,_T("Call Stack"),IDI_ICON_CALLSTACK,_T("Display Call Stack"));
    mpThreadsToolbar->AddButton(IDC_BUTTON_CONTEXT,_T("Context"),IDI_ICON_CONTEXT,_T("Display Registers"));
    mpThreadsToolbar->AddSeparator();

    mpThreadsToolbar->AddButton(IDC_BUTTON_TERMINATE_THREAD,IDI_ICON_KILL,_T("Terminate Thread"));
    mpThreadsToolbar->AddSeparator();

    mpThreadsToolbar->AddButton(IDC_BUTTON_SUSPEND_THREAD,IDI_ICON_SUSPEND,_T("Suspend Thread Once"));
    mpThreadsToolbar->AddButton(IDC_BUTTON_RESUME_THREAD,IDI_ICON_RESUME,_T("Resume Thread Once"));
    mpThreadsToolbar->AddButton(IDC_BUTTON_THREAD_SUSPENDED_COUNT,IDI_ICON_SUSPENDED_COUNT,_T("Suspended Count"));
    mpThreadsToolbar->AddSeparator();

    mpThreadsToolbar->AddDropDownButton(IDC_BUTTON_PRIORITY,
                                        _T("Thread Priority"),
                                        IDI_ICON_PRIORITY,
                                        _T("Change Thread Priotity"),
                                        mpPopUpMenuThreadPriority,
                                        TRUE);

    CDialogHelper::GetClientWindowRect(mhWndDialog,mhWndDialog,&RectWindow);
    // ModulesThreadsSplitter
    mpModulesThreadsSplitter=new CSplitter(mhInstance,mhWndDialog,TRUE,FALSE,FALSE,FALSE,ORIGINAL_PERCENT_THREADS_GROUP,IDI_ICON_DOWN,IDI_ICON_DOWN_HOT,IDI_ICON_DOWN_DOWN,IDI_ICON_UP,IDI_ICON_UP_HOT,IDI_ICON_UP_DOWN,16,16);
    mpModulesThreadsSplitter->Show();
    mpModulesThreadsSplitter->GetRect(&Rect);
    mpModulesThreadsSplitter->TopMinFreeSpace=Rect.top;
    mpModulesThreadsSplitter->BottomMinFreeSpace=-RectWindow.left;// small border of window
    mpModulesThreadsSplitter->SetCollapsedStateChangeCallBack(OnModulesThreadsSplitterCollapsedStateChange,NULL);
    mpModulesThreadsSplitter->SetMoveCallBack(OnModulesThreadsSplitterMove,NULL);

    // ProcessesModulesSplitter
    mpProcessesModulesSplitter=new CSplitter(mhInstance,mhWndDialog,TRUE,FALSE,FALSE,FALSE,ORIGINAL_PERCENT_MODULES_GROUP,IDI_ICON_DOWN,IDI_ICON_DOWN_HOT,IDI_ICON_DOWN_DOWN,IDI_ICON_UP,IDI_ICON_UP_HOT,IDI_ICON_UP_DOWN,16,16);
    mpProcessesModulesSplitter->BottomMinFreeSpace=RectWindow.bottom-Rect.top;
    CDialogHelper::GetClientWindowRect(mhWndDialog,mpProcessesToolbar->GetControlHandle(),&Rect);
    mpProcessesModulesSplitter->TopMinFreeSpace=Rect.bottom-RectWindow.top;
    mpProcessesModulesSplitter->Show();
    mpProcessesModulesSplitter->SetCollapsedStateChangeCallBack(OnProcessesModulesSplitterCollapsedStateChange,NULL);
    mpProcessesModulesSplitter->SetMoveCallBack(OnProcessesModulesSplitterMove,NULL);

    mpLinkListSubDialogs=new CLinkListSimple();

    // size main dialog
    Resize();

    // load processes
    Refresh();

    InitializationFinished=TRUE;
}

//-----------------------------------------------------------------------------
// Name: OnClose
// Object: Close Dumper dialog
// Parameters :
//     in : HWND hDlg
// Return : 
//-----------------------------------------------------------------------------
void OnClose()
{
    // hide current window
    ShowWindow(mhWndDialog,FALSE);

    // close all sub dialogs before destroying memory
    CloseSubDialogs();

    // free memory
    delete mpModulesThreadsSplitter;
    delete mpProcessesModulesSplitter;
    delete mpProcessesToolbar;
    delete mpThreadsToolbar;
    delete mpPopUpMenuDumpToExe;

    delete mpProcessesRebar;
    mpProcessesRebar=NULL;
    delete mpPopUpMenuThreadPriority;
    mpPopUpMenuThreadPriority=NULL;
    delete mpPopUpMenuProcessPriority;
    mpPopUpMenuProcessPriority=NULL;
    delete mpPopUpMenuIntegrityChecking;
    mpPopUpMenuIntegrityChecking=NULL;
    delete mpCListviewProcesses;
    mpCListviewProcesses=NULL;
    delete mpCListviewModules;
    mpCListviewModules=NULL;
    delete mpCListviewThreads;
    mpCListviewThreads=NULL;
    delete mpLinkListSubDialogs;


    EndDialog(mhWndDialog, 0);
}

void CloseSubDialogs()
{
    IsClosing=TRUE;
    CLinkListItem* pItem;
    mpLinkListSubDialogs->Lock();
    for (pItem=mpLinkListSubDialogs->Head;pItem;pItem=pItem->NextItem)
    {
        SendMessage((HWND)pItem->ItemData,WM_CLOSE,0,0);
    }
    mpLinkListSubDialogs->Unlock();
    // as sendMessage is blocking until code is executed, we can now free memory
}
void RegisterSubDialog(HWND hDlg)
{
    mpLinkListSubDialogs->AddItem(hDlg);
}
void UnregisterSubDialog(HWND hDlg)
{
    if (IsClosing)
        // mpLinkListSubDialogs is locked --> nothing to do
        // trying yo remove item from list can only crash CloseSubDialogs loop
        return;
    mpLinkListSubDialogs->RemoveItemFromItemData(hDlg);
}


//-----------------------------------------------------------------------------
// Name: DumperWndProc
// Object: Main window WndProc
// Parameters :
//     in : HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam
// Return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK DumperWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
            OnInit(hDlg);
			break;
        case WM_SIZING:
            CheckSize((RECT*)lParam);
            break;
        case WM_SIZE:
            if (mpProcessesRebar)
                mpProcessesRebar->OnSize(wParam,lParam);
            Resize();
            break;
        case WM_CLOSE:
            OnClose();
			break;
		case WM_COMMAND:
            switch(LOWORD(wParam))
            {
            case IDC_BUTTON_REFRESH:
                Refresh();
                break;
            case IDC_BUTTON_TERMINATE_PROCESS:
                ProcessesPopUpCallBack(mpProcessesMenuIdTerminate,NULL);
                break;
            case IDC_BUTTON_SUSPEND_PROCESS:
                ProcessesPopUpCallBack(mpProcessesMenuIdSuspend,NULL);
                break;
            case IDC_BUTTON_RESUME_PROCESS:
                ProcessesPopUpCallBack(mpProcessesMenuIdResume,NULL);
                break;
            case IDC_BUTTON_DUMP:
                Dump();
                break;
            case IDC_BUTTON_DUMP_TO_EXE:
                // remove not RAW by default
                CDumpToExe::RemoveUnusedMemory();
                break;
            case IDC_BUTTON_RAW_DUMP:
                RawDump(mhInstance,mhWndDialog);
                break;
            case IDC_BUTTON_MEMORY:
                if (UserMode)
                    CMemoryUserInterface::Show(mhInstance,mhWndDialog);
                else
                    CKernelMemoryUserInterface::Show(mhInstance,mhWndDialog);
                break;
            case IDC_BUTTON_CALLSTACK:
                ThreadsPopUpCallBack(mpThreadMenuIdCallStack,NULL);
                break;
            case IDC_BUTTON_CONTEXT:
                ThreadsPopUpCallBack(mpThreadMenuIdContext,NULL);
                break;
            case IDC_BUTTON_TERMINATE_THREAD:
                ThreadsPopUpCallBack(mpThreadMenuIdTerminate,NULL);
                break;
            case IDC_BUTTON_SUSPEND_THREAD:
                ThreadsPopUpCallBack(mpThreadMenuIdSuspend,NULL);
                break;
            case IDC_BUTTON_RESUME_THREAD:
                ThreadsPopUpCallBack(mpThreadMenuIdResume,NULL);
                break;
            case IDC_BUTTON_THREAD_SUSPENDED_COUNT:
                ThreadsPopUpCallBack(mpThreadMenuIdSuspendedCount,NULL);
                break;
            case IDC_BUTTON_USER_KERNEL_MODE:
                SwitchMode();
                break;
            case IDC_BUTTON_INJLIB:
                InjectLibrary();
                break;
            case IDC_BUTTON_EJECTLIB:
                EjectLibrary();
                break;
            case IDC_BUTTON_WINDOW_THREAD_PROCESS_ID:
                {
                    ::CloseHandle(::CreateThread(NULL,0,ShowWindowThreadProcessId,0,0,0));
                }
                break;
            case IDC_BUTTON_INTEGRITY_CHECKING:
                ProcessIntegrityChecking();
                break;
            }
            break;
        case WM_NOTIFY:
            if (mpProcessesRebar)
            {
                if (mpProcessesRebar->OnNotify(wParam,lParam))
                    break;
            }
            if (mpCListviewProcesses)
            {
                LPNMLISTVIEW pnm = (LPNMLISTVIEW)lParam;
                // set sorting type depending header click
                if (pnm->hdr.hwndFrom==ListView_GetHeader(mpCListviewProcesses->GetControlHandle()))
                {
                    if (pnm->hdr.code==HDN_ITEMCLICK) 
                    {
                        // on header click
                        if (UserMode)
                        {
                            if ((pnm->iItem==ColumnProcessesNbThreads)
                                ||(pnm->iItem==ColumnProcessesPriorityClass)
								||(pnm->iItem==ColumnProcessType))
                                mpCListviewProcesses->SetSortingType(CListview::SortingTypeNumber);
                            else
                                mpCListviewProcesses->SetSortingType(CListview::SortingTypeString);
                        }
                        else
                        {
                            if ((ColumnProcessesKernelSize<pnm->iItem)&&(pnm->iItem<7))
                                mpCListviewProcesses->SetSortingType(CListview::SortingTypeNumber);
                            else
                                mpCListviewProcesses->SetSortingType(CListview::SortingTypeString);
                        }
                    }
                }

                if (mpCListviewProcesses->OnNotify(wParam,lParam))
                    break;
            }
            if (mpCListviewModules)
            {
                LPNMLISTVIEW pnm = (LPNMLISTVIEW)lParam;
                // set sorting type depending header click
                if (pnm->hdr.hwndFrom==ListView_GetHeader(mpCListviewModules->GetControlHandle()))
                {
                    if (pnm->hdr.code==HDN_ITEMCLICK) 
                    {
                        // on header click
                        switch (pnm->iItem)
                        {
                        default:
                            mpCListviewModules->SetSortingType(CListview::SortingTypeString);
                            break;
                        case ColumnModulesLoadCount:
                            mpCListviewModules->SetSortingType(CListview::SortingTypeNumber);
                            break;
                        }
                    }
                }
                if (mpCListviewModules->OnNotify(wParam,lParam))
                    break;
            }
            if (mpCListviewThreads)
            {
                LPNMLISTVIEW pnm = (LPNMLISTVIEW)lParam;
                // set sorting type depending header click
                if (pnm->hdr.hwndFrom==ListView_GetHeader(mpCListviewThreads->GetControlHandle()))
                {
                    if (pnm->hdr.code==HDN_ITEMCLICK) 
                    {
                        // on header click
                        if (UserMode)
                        {
                            switch (pnm->iItem)
                            {
                                case ColumnThreadsId:
                                case ColumnThreadsCreationTime:
                                case ColumnThreadsUserTime:
                                case ColumnThreadsKernelTime:
                                    mpCListviewThreads->SetSortingType(CListview::SortingTypeString);
                                    break;
                                default:
                                    mpCListviewThreads->SetSortingType(CListview::SortingTypeNumber);
                                    break;
                            }
                        }
                    }
                }

                if (mpCListviewThreads->OnNotify(wParam,lParam))
                    break;
            }
            if (mpProcessesToolbar)
            {
                if (mpProcessesToolbar->OnNotify(wParam,lParam))
                    break;
            }

            if (mpThreadsToolbar)
            {
                if (mpThreadsToolbar->OnNotify(wParam,lParam))
                    break;
            }
            break;
        case WM_SHOW_INTEGRITY_SUCCESSFULLY_CHECKED_MSG:
            MessageBox(mhWndDialog,_T("Integrity has been successfully checked"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
            break;
        default:
            return FALSE;
	}
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Refresh
// Object: refresh processes and modules list in listviews
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
void Refresh()
{
    bRefreshing = TRUE;
    if (UserMode)
    {
        DWORD dwSelectedProcessId = GetSelectedProcessId();
        RefreshProcesses();
        SetSelectedProcessId(dwSelectedProcessId);
    }
    else
        RefreshKernelModules();
    bRefreshing = FALSE;
}

//-----------------------------------------------------------------------------
// Name: RefreshKernelModules
// Object: refresh kernel module list
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
void RefreshKernelModules()
{
    TCHAR* FileExt;
    HICON hIconSmall;
    int IconIndex;
    int DefaultAppIcoIndex;
    int DefaultDllIcoIndex;
    int DefaultSysIcoIndex;

    mpCListviewProcesses->Clear();
    mpCListviewModules->Clear();
    mpCListviewThreads->Clear();
    if (!pKernelProcesseInfo->Update())
    {
        MessageBox(mhWndDialog,_T("Error retrieving kernel modules informations"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    // remove previous icon from listview if any
    mpCListviewProcesses->RemoveAllIcons(CListview::ImageListSmall);

    // set default app icon (as all item must have an icon, and they take the icon at index 0 by default)
    DefaultAppIcoIndex=mpCListviewProcesses->AddIcon(CListview::ImageListSmall,mhInstance,IDI_ICON_DEFAULT_APP);
    DefaultDllIcoIndex=mpCListviewProcesses->AddIcon(CListview::ImageListSmall,mhInstance,IDI_ICON_DLL);
    DefaultSysIcoIndex=mpCListviewProcesses->AddIcon(CListview::ImageListSmall,mhInstance,IDI_ICON_SYS);

    TCHAR sz[2*MAX_PATH];
    TCHAR szPath[2*MAX_PATH];
    TCHAR* Path;
    char* pszName;

    for (DWORD Cnt=0;Cnt<pKernelProcesseInfo->pSystemModuleInformation->dwModules;Cnt++)
    {
        // name
        pszName=(char*)(pKernelProcesseInfo->pSystemModuleInformation->pModulesInfo[Cnt].pbPath
                        +pKernelProcesseInfo->pSystemModuleInformation->pModulesInfo[Cnt].wNameOffset);
#if (defined(UNICODE)||defined(_UNICODE))
        MultiByteToWideChar(CP_ACP, 0, pszName, -1,sz, 2*MAX_PATH);
#else
        strcpy(sz,pszName);
#endif
        mpCListviewProcesses->SetItemText(Cnt,ColumnProcessesKernelName,sz);

        // base address
        _stprintf(sz,_T("0x%p"),pKernelProcesseInfo->pSystemModuleInformation->pModulesInfo[Cnt].pBase);
        mpCListviewProcesses->SetItemText(Cnt,ColumnProcessesKernelBaseAddress,sz);

        // size
        _stprintf(sz,_T("0x%.8X"),pKernelProcesseInfo->pSystemModuleInformation->pModulesInfo[Cnt].dwSize);
        mpCListviewProcesses->SetItemText(Cnt,ColumnProcessesKernelSize,sz);

        // Index
        _stprintf(sz,_T("%u"),pKernelProcesseInfo->pSystemModuleInformation->pModulesInfo[Cnt].wIndex);
        mpCListviewProcesses->SetItemText(Cnt,ColumnProcessesKernelIndex,sz);

        // load count
        _stprintf(sz,_T("%u"),pKernelProcesseInfo->pSystemModuleInformation->pModulesInfo[Cnt].wLoadCount);
        mpCListviewProcesses->SetItemText(Cnt,ColumnProcessesKernelLoadCount,sz);

        // rank
        _stprintf(sz,_T("%u"),pKernelProcesseInfo->pSystemModuleInformation->pModulesInfo[Cnt].wRank);
        mpCListviewProcesses->SetItemText(Cnt,ColumnProcessesKernelRank,sz);

        // flags
        _stprintf(sz,_T("%u"),pKernelProcesseInfo->pSystemModuleInformation->pModulesInfo[Cnt].dwFlags);
        mpCListviewProcesses->SetItemText(Cnt,ColumnProcessesKernelFlags,sz);

        // path
        pszName=(char*)(pKernelProcesseInfo->pSystemModuleInformation->pModulesInfo[Cnt].pbPath);
#if (defined(UNICODE)||defined(_UNICODE))
        MultiByteToWideChar(CP_ACP, 0, pszName, -1,sz, 2*MAX_PATH);
#else
        strcpy(sz,pszName);
#endif

        Path=sz;
        // check for \??\ flags
        if (_tcsnicmp(Path,_T("\\??\\"),4)==0)
            Path=&Path[4];

        // check for \SystemRoot\ flag
        else if(_tcsnicmp(Path,_T("\\SystemRoot\\"),12)==0)
        {
            SHGetFolderPath(NULL,CSIDL_WINDOWS,NULL,SHGFP_TYPE_CURRENT,szPath);
            _tcscat(szPath,&Path[11]);
            Path=szPath;
        }
        else if (_tcsnicmp(Path,_T("\\Windows\\"),9)==0)
        {
            SHGetFolderPath(NULL,CSIDL_WINDOWS,NULL,SHGFP_TYPE_CURRENT,szPath);
            _tcscat(szPath,&Path[8]);
            Path=szPath;
        }

        mpCListviewProcesses->SetItemText(Cnt,ColumnProcessesKernelPath,Path);

        if (((int)::ExtractIconEx(Path, 0, NULL, &hIconSmall, 1))>0)
        {
            IconIndex=mpCListviewProcesses->AddIcon(CListview::ImageListSmall,hIconSmall);
            if (IconIndex!=-1)
                mpCListviewProcesses->SetItemIconIndex(Cnt,IconIndex);
            ::DestroyIcon(hIconSmall);
        }
        else
        {
            FileExt=CStdFileOperations::GetFileExt(Path);
            if (FileExt)
            {
                if (_tcsicmp(FileExt,_T("dll"))==0)
                {
                    mpCListviewProcesses->SetItemIconIndex(Cnt,DefaultDllIcoIndex);
                }
                else if (_tcsicmp(FileExt,_T("sys"))==0)
                {
                    mpCListviewProcesses->SetItemIconIndex(Cnt,DefaultSysIcoIndex);
                }
                // not required until DefaultAppIcoIndex==0
                /*
                else
                {

                mpCListviewProcesses->SetItemIconIndex(Cnt,DefaultAppIcoIndex);
                }
                */
            }
        }
    }

    // resort listview by the last sorting order
    mpCListviewProcesses->ReSort();
}

//-----------------------------------------------------------------------------
// Name: FileTimeToString
// Object: refresh threads listview depending the selected process in process listview
// Parameters :
//          in : FILETIME* pFileTime : file time to display
//               BOOL bDate : TRUE if date, FALSE for duration
//               DWORD StringMaxSize : String max size in TCHAR including \0
//          inout : TCHAR* String : string containing time. 
// Return : 
//-----------------------------------------------------------------------------
void FileTimeToString(FILETIME* pFileTime,BOOL bDate,TCHAR* String,DWORD StringMaxSize)
{
    if (!String)
        return;
    *String=0;

    SYSTEMTIME SysTime;
    SYSTEMTIME RefSysTime;
    FILETIME RefFileTime;
    RefFileTime.dwHighDateTime=0;
    RefFileTime.dwLowDateTime=0;
    ::FileTimeToLocalFileTime(&RefFileTime,&RefFileTime);
    ::FileTimeToSystemTime(&RefFileTime,&RefSysTime);

    // convert UTC time to local filetime
    ::FileTimeToLocalFileTime(pFileTime,pFileTime);

    if (::FileTimeToSystemTime(pFileTime,&SysTime))
    {
        if (bDate)
        {
            _sntprintf(String,StringMaxSize,_T("%04u/%02u/%02u %02u:%02u:%02u:%03u"),SysTime.wYear,SysTime.wMonth,SysTime.wDay,SysTime.wHour,SysTime.wMinute,SysTime.wSecond,SysTime.wMilliseconds);
        }
        else
        {
            // duration are expressed from reference of midnight on January 1, 1601 at Greenwich, England too
            SysTime.wYear-=RefSysTime.wYear;
            SysTime.wMonth-=RefSysTime.wMonth;
            SysTime.wDay-=RefSysTime.wDay;
            SysTime.wHour-=RefSysTime.wHour;

            if (SysTime.wYear)
            {
                _sntprintf(String,StringMaxSize,_T("%04u Years %02u Months %2u Days %02u:%02u:%02u:%03u"),SysTime.wYear,SysTime.wMonth,SysTime.wDay,SysTime.wHour,SysTime.wMinute,SysTime.wSecond,SysTime.wMilliseconds);
            }
            else if (SysTime.wMonth)
            {
                _sntprintf(String,StringMaxSize,_T("%02u Months %02u Days %02u:%02u:%02u:%03u"),SysTime.wMonth,SysTime.wDay,SysTime.wHour,SysTime.wMinute,SysTime.wSecond,SysTime.wMilliseconds);
            }
            else if (SysTime.wDay)
            {
                _sntprintf(String,StringMaxSize,_T("%02u Days %02u:%02u:%02u:%03u"),SysTime.wDay,SysTime.wHour,SysTime.wMinute,SysTime.wSecond,SysTime.wMilliseconds);
            }
            else
            {
                _sntprintf(String,StringMaxSize,_T("%02u:%02u:%02u:%03u"),SysTime.wHour,SysTime.wMinute,SysTime.wSecond,SysTime.wMilliseconds);
            }
        }
    }    
}

//-----------------------------------------------------------------------------
// Name: RefreshProcesses
// Object: refresh processes and modules list in listviews
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
void RefreshProcesses()
{
    TCHAR szPath[MAX_PATH];
    TCHAR* ProcessFullPath;
    TCHAR** ppc;
    DWORD dwProcessIndex;
    // fill processes listview
    mpCListviewProcesses->Clear();

    ppc=new TCHAR*[NB_PROCESSES_COLUMNS];
    for (dwProcessIndex=0;dwProcessIndex<NB_PROCESSES_COLUMNS;dwProcessIndex++)
    {
        ppc[dwProcessIndex]=new TCHAR[MAX_PATH];
        *ppc[dwProcessIndex]=0;
    }
    
    PROCESSENTRY32 pe32 = {0};
    HANDLE hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (hSnap == INVALID_HANDLE_VALUE) 
    {
        CAPIError::ShowLastError();
        return; 
    }

    // Fill the size of the structure before using it. 
    pe32.dwSize = sizeof(PROCESSENTRY32); 
 
    HICON hIconSmall;
    int ItemIndex;
    int IconIndex;
    HANDLE hSnapModule;
    MODULEENTRY32 me32 = {0}; 
    TCHAR* FileExt;
    int DefaultAppIcoIndex;
    int DefaultDllIcoIndex;
    int DefaultSysIcoIndex;

    HANDLE hProcess;
    FILETIME CreationTime;
    FILETIME ExitTime;
    FILETIME KernelTime;
    FILETIME UserTime;

    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32); 

    // remove previous icon from listview if any
    mpCListviewProcesses->RemoveAllIcons(CListview::ImageListSmall);

    // set default app icon (as all item must have an icon, and they take the icon at index 0 by default)
    DefaultAppIcoIndex=mpCListviewProcesses->AddIcon(CListview::ImageListSmall,mhInstance,IDI_ICON_DEFAULT_APP);
    DefaultDllIcoIndex=mpCListviewProcesses->AddIcon(CListview::ImageListSmall,mhInstance,IDI_ICON_DLL);
    DefaultSysIcoIndex=mpCListviewProcesses->AddIcon(CListview::ImageListSmall,mhInstance,IDI_ICON_SYS);

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

        _tcscpy(ppc[ColumnProcessesName],pe32.szExeFile);

		BOOL b32BitProcess;
		if (CProcessHelper::Is32bitsProcess(pe32.th32ProcessID,&b32BitProcess))
		{
			if (b32BitProcess)
				_tcscpy(ppc[ColumnProcessType],STR_PROCESS_TYPE_32);
			else
				_tcscpy(ppc[ColumnProcessType],STR_PROCESS_TYPE_64);
		}
		else
			*ppc[ColumnProcessType]=0;

        _stprintf(ppc[ColumnProcessesId],_T("0x%.8X (%u)"),pe32.th32ProcessID,pe32.th32ProcessID);
        _stprintf(ppc[ColumnProcessesNbThreads],_T("%u"),pe32.cntThreads);
        _stprintf(ppc[ColumnProcessesParentId],_T("0x%.8X (%u)"),pe32.th32ParentProcessID,pe32.th32ParentProcessID);
        _stprintf(ppc[ColumnProcessesPriorityClass],_T("%u"),pe32.pcPriClassBase);

        hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,pe32.th32ProcessID);
        if (hProcess)
        {
            if (::GetProcessTimes(hProcess,&CreationTime,&ExitTime,&KernelTime,&UserTime))
            {
                FileTimeToString(&CreationTime,TRUE,ppc[ColumnProcessesPriorityCreationTime],256);
            }

            ::CloseHandle(hProcess);
        }
        else
        {
            *ppc[ColumnProcessesPriorityCreationTime]=0;
        }
 
        ItemIndex=mpCListviewProcesses->AddItemAndSubItems(NB_PROCESSES_COLUMNS,ppc);

        ProcessFullPath=pe32.szExeFile;

        // try to get full process path
        hSnapModule =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,pe32.th32ProcessID);
        if (hSnapModule != INVALID_HANDLE_VALUE) 
        {
            if (Module32First(hSnapModule, &me32))
            {
                // check for \??\ flags
                if (_tcsnicmp(me32.szExePath,_T("\\??\\"),4)==0)
                    ProcessFullPath=&me32.szExePath[4];

                // check for \SystemRoot\ flag
                else if(_tcsnicmp(me32.szExePath,_T("\\SystemRoot\\"),12)==0)
                {
                    SHGetFolderPath(NULL,CSIDL_WINDOWS,NULL,SHGFP_TYPE_CURRENT,szPath);
                    _tcscat(szPath,&me32.szExePath[11]);
                    ProcessFullPath=szPath;
                }
                else
                    ProcessFullPath=me32.szExePath;
            }
            CloseHandle(hSnapModule);
        }
 
        if (((int)::ExtractIconEx(ProcessFullPath, 0, NULL, &hIconSmall, 1))>0)
        {
            IconIndex=mpCListviewProcesses->AddIcon(CListview::ImageListSmall,hIconSmall);
            if (IconIndex!=-1)
                mpCListviewProcesses->SetItemIconIndex(ItemIndex,IconIndex);
            ::DestroyIcon(hIconSmall);
        }
        else
        {
            FileExt=CStdFileOperations::GetFileExt(ProcessFullPath);
            if (FileExt)
            {
                if (_tcsicmp(FileExt,_T("dll"))==0)
                {
                    mpCListviewProcesses->SetItemIconIndex(ItemIndex,DefaultDllIcoIndex);
                }
                else if (_tcsicmp(FileExt,_T("sys"))==0)
                {
                    mpCListviewProcesses->SetItemIconIndex(ItemIndex,DefaultSysIcoIndex);
                }
                // not required until DefaultAppIcoIndex==0
                /*
                else
                {

                mpCListviewProcesses->SetItemIconIndex(Cnt,DefaultAppIcoIndex);
                }
                */
            }
        }
    } 
    while (Process32Next(hSnap, &pe32)); 
 
    // clean up the snapshot object. 
    CloseHandle (hSnap); 

    for (dwProcessIndex=0;dwProcessIndex<NB_PROCESSES_COLUMNS;dwProcessIndex++)
        delete ppc[dwProcessIndex];
    delete[] ppc;

    // resort listview by the last sorting order
    mpCListviewProcesses->ReSort();

    mpCListviewProcesses->SetSelectedIndex(0);
    RefreshModules(0);
    RefreshThreads(0);
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
        RefreshProcesses();

        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetSelectedModule
// Object: get id of first selected module in module listview
// Parameters :
// Return : index of selected item or 0
//-----------------------------------------------------------------------------
DWORD GetSelectedModuleIndex()
{
    int ItemIndex=mpCListviewModules->GetSelectedIndex();
    if (ItemIndex<0)
        ItemIndex=0;

    return ItemIndex;
}

//-----------------------------------------------------------------------------
// Name: GetModuleBaseAddress
// Object: get base address of selected module
// Parameters :
// Return : base address
//-----------------------------------------------------------------------------
PBYTE GetModuleBaseAddress(int ModuleIndex)
{
    PBYTE pRet=0;
    TCHAR psz[MAX_PATH];
    mpCListviewModules->GetItemText(ModuleIndex,ColumnModulesAddress,psz,MAX_PATH);
    _stscanf(psz,_T("0x%X"),&pRet);
    return pRet;
}

//-----------------------------------------------------------------------------
// Name: GetSelectedProcessId
// Object: get id of first selected process in process listview
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
DWORD GetSelectedProcessId()
{
    int ItemIndex=mpCListviewProcesses->GetSelectedIndex();
    if (ItemIndex<0)
        ItemIndex=0;

    return GetProcessIdFromIndex(ItemIndex);
}

//-----------------------------------------------------------------------------
// Name: SetSelectedProcessId
// Object: select process id in list in process listview
// Parameters :
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL SetSelectedProcessId(DWORD ProcessId)
{
    if (!CheckIfProcessIsAlive(ProcessId))
        return 0;

    DWORD dwProcessNumber=0;
    TCHAR psz[MAX_PATH];

    mpCListviewProcesses->UnselectAll();
    SIZE_T NbItems = mpCListviewProcesses->GetItemCount();
    for (SIZE_T Cnt=0;Cnt<NbItems;Cnt++)
    {
        mpCListviewProcesses->GetItemText(Cnt,ColumnProcessesId,psz,MAX_PATH);
        _stscanf(psz,_T("0x%X"),&dwProcessNumber);
        if (dwProcessNumber==ProcessId)
        {
            mpCListviewProcesses->SetSelectedIndex(Cnt,TRUE);
            RefreshModules(Cnt);
            return TRUE;
        }
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: GetProcessIdFromIndex
// Object: get id of process at index ProcessListViewItemIndex in process listview
// Parameters :
//              int ProcessListViewItemIndex : index in process listview
// Return : 
//-----------------------------------------------------------------------------
DWORD GetProcessIdFromIndex(int ProcessListViewItemIndex)
{
    DWORD dwProcessNumber=0;
    TCHAR psz[MAX_PATH];
    mpCListviewProcesses->GetItemText(ProcessListViewItemIndex,ColumnProcessesId,psz,MAX_PATH);
    _stscanf(psz,_T("0x%X"),&dwProcessNumber);

    if (!CheckIfProcessIsAlive(dwProcessNumber))
        return 0;
    return dwProcessNumber;
}

//-----------------------------------------------------------------------------
// Name: RefreshModules
// Object: refresh modules listview depending the selected process in process listview
// Parameters :
//          in : int ItemIndex : index of selected item in process listview
// Return : 
//-----------------------------------------------------------------------------
void RefreshModules(int ItemIndex)
{
    TCHAR** ppc;
    TCHAR pszPath[MAX_PATH];
    DWORD dwModuleIndex;
    DWORD dwProcessNumber=0;

    dwProcessNumber=GetProcessIdFromIndex(ItemIndex);
    if (dwProcessNumber==0)
        return;

    ppc=new TCHAR*[NB_MODULES_COLUMNS];
    for (dwModuleIndex=0;dwModuleIndex<NB_MODULES_COLUMNS;dwModuleIndex++)
    {
        ppc[dwModuleIndex]=new TCHAR[256];
        *ppc[dwModuleIndex]=0;
    }

    mpCListviewModules->Clear();

    CModulesInfos ModulesInfos;

#ifndef _WIN64
    TCHAR x86CrossCompatibilityPath[MAX_PATH];
    CStdFileOperations::GetAppPath(x86CrossCompatibilityPath,MAX_PATH);
    _tcscat(x86CrossCompatibilityPath, _T("x86CrossCompatibility.exe") );

    ModulesInfos.SetCrossCompatibilityBinaryPath(x86CrossCompatibilityPath);
#endif

    ModulesInfos.ParseProcess(dwProcessNumber);


#ifndef _WIN64
    BOOL bIs32Process;
    CProcessHelper::Is32bitsProcess(dwProcessNumber,&bIs32Process);

    if (bIs32Process)
    {
#endif
    MODULEENTRY32 me32 = {0}; 
    HANDLE hSnap;

    hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,dwProcessNumber);
    if (hSnap == INVALID_HANDLE_VALUE) 
    {
        if (InitializationFinished) // avoid to display error during loading (when running with limited accounts)
            CAPIError::ShowLastError();
        return; 
    }
    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32); 
 
    // Walk the module list of the process
    if (!Module32First(hSnap, &me32))
    {
        CloseHandle(hSnap);
        CAPIError::ShowLastError();
        return;
    }
    do 
    { 
        //////////////////////////////////
        // removes some szExePath patterns
        //////////////////////////////////

        // check for \??\ flags
        if (_tcsnicmp(me32.szExePath,_T("\\??\\"),4)==0)
            _tcscpy(ppc[ColumnModulesName],&me32.szExePath[4]);

        // check for \SystemRoot\ flag
        else if(_tcsnicmp(me32.szExePath,_T("\\SystemRoot\\"),12)==0)
        {
            SHGetFolderPath(NULL,CSIDL_WINDOWS,NULL,SHGFP_TYPE_CURRENT,pszPath);

            _tcscpy(ppc[ColumnModulesName],pszPath);
            _tcscat(ppc[ColumnModulesName],&me32.szExePath[11]);
        }

        else
            _tcscpy(ppc[ColumnModulesName],me32.szExePath);
        _stprintf(ppc[ColumnModulesAddress],_T("0x%p-0x%p"),me32.modBaseAddr,me32.modBaseAddr+me32.modBaseSize);

        SIZE_T LoadCount=0;
        SIZE_T Flags=0;
        SIZE_T TlsIndex=0;
        SIZE_T TimeDateStamp=0;
        SIZE_T EntryPoint=0;
        BOOL bRet = ModulesInfos.GetModuleInfos((HMODULE)me32.modBaseAddr,&EntryPoint,&LoadCount,&Flags,&TlsIndex,&TimeDateStamp);

        // find entry point of module
        if (bRet)
        {
            _stprintf(ppc[ColumnModulesEntryPoint],_T("0x%p"),/* (PBYTE)pModuleInfos->BaseAddress + */ EntryPoint); // pModuleInfos->EntryPoint is in VA not RVA, so no needs to add BaseAddress
            
            if (LoadCount == 0xFFFF)
                _tcscpy(ppc[ColumnModulesLoadCount],ModulesLoadCountStatic);
            else
                _stprintf(ppc[ColumnModulesLoadCount],_T("%u"),LoadCount);
            _stprintf(ppc[ColumnModulesFlags],_T("0x%.8X"),Flags);
            _stprintf(ppc[ColumnModulesTlsIndex],_T("%u"),TlsIndex);
            _stprintf(ppc[ColumnModulesTimeDateStamp],_T("0x%.8X"),TimeDateStamp);
        }
        else
        {
            CPE pe(ppc[ColumnModulesName]);
            if (pe.Parse())
            {
                SIZE_T EntryPoint = pe.NTHeader.OptionalHeader.AddressOfEntryPoint;
                if (EntryPoint)
                    EntryPoint+=(SIZE_T)(me32.modBaseAddr);
                _stprintf(ppc[ColumnModulesEntryPoint],_T("0x%p"),(PBYTE)(EntryPoint));
                _stprintf(ppc[ColumnModulesTimeDateStamp],_T("0x%.8X"),pe.NTHeader.FileHeader.TimeDateStamp);
            }
            else
            {
                _tcscpy(ppc[ColumnModulesEntryPoint],_T("Error retrieving Entry Point"));
                ppc[ColumnModulesTimeDateStamp][0]=0;
            }

            ppc[ColumnModulesLoadCount][0]=0;
            ppc[ColumnModulesFlags][0]=0;
            ppc[ColumnModulesTlsIndex][0]=0;
        }

        mpCListviewModules->AddItemAndSubItems(NB_MODULES_COLUMNS,ppc);
    } 
    while (Module32Next(hSnap, &me32)); 
 
    // clean up the snapshot object. 
    CloseHandle (hSnap); 
#ifndef _WIN64
    }
    else // current process is 32 bit and targeted process is 64 bit
    {
    
    x86CrossCompatibility::MODULEENTRY3264 me32;

    x86CrossCompatibility::Cx86CrossCompatibility x86CC(x86CrossCompatibilityPath);
    HANDLE64 hSnap64 =  x86CC.CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,dwProcessNumber);
    if (hSnap64 == (HANDLE64)-1) 
        return; // no last error available

    // Fill the size of the structure before using it. 
    me32.dwSize = sizeof(x86CrossCompatibility::MODULEENTRY3264); 
 
    typedef BOOLEAN (WINAPI *pfWow64EnableWow64FsRedirection)(__in  BOOLEAN Wow64FsEnableRedirection);
    pfWow64EnableWow64FsRedirection pWow64EnableWow64FsRedirection;
    pWow64EnableWow64FsRedirection = (pfWow64EnableWow64FsRedirection) ::GetProcAddress( ::GetModuleHandle( _T("Kernel32.dll")) ,"Wow64EnableWow64FsRedirection" );

    if (pWow64EnableWow64FsRedirection)
        pWow64EnableWow64FsRedirection(FALSE);

    // Walk the module list of the process
    if (!x86CC.Module32First(hSnap64, &me32))
    {
        x86CC.CloseHandle(hSnap64);
        return;
    }
    do 
    { 
        //////////////////////////////////
        // removes some szExePath patterns
        //////////////////////////////////

        // check for \??\ flags
        if (_tcsnicmp(me32.szExePath,_T("\\??\\"),4)==0)
            _tcscpy(ppc[ColumnModulesName],&me32.szExePath[4]);

        // check for \SystemRoot\ flag
        else if(_tcsnicmp(me32.szExePath,_T("\\SystemRoot\\"),12)==0)
        {
            ::SHGetFolderPath(NULL,CSIDL_WINDOWS,NULL,SHGFP_TYPE_CURRENT,pszPath);
            _tcscpy(ppc[ColumnModulesName],pszPath);
            _tcscat(ppc[ColumnModulesName],&me32.szExePath[11]);
        }

        else
            _tcscpy(ppc[ColumnModulesName],me32.szExePath);
        _stprintf(ppc[ColumnModulesAddress],_T("0x%016I64X-0x%016I64X"),me32.modBaseAddr,me32.modBaseAddr+me32.modBaseSize);

        SIZE_T LoadCount=0;
        SIZE_T Flags=0;
        SIZE_T TlsIndex=0;
        SIZE_T TimeDateStamp=0;
        UINT64 EntryPoint=0;
        BOOL bRet = ModulesInfos.GetModuleInfosT<UINT64>(me32.modBaseAddr,&EntryPoint,&LoadCount,&Flags,&TlsIndex,&TimeDateStamp);

        // find entry point of module
        if (bRet)
        {
            _stprintf(ppc[ColumnModulesEntryPoint],_T("0x%016I64X"),/* (PBYTE)pModuleInfos->BaseAddress + */ EntryPoint); // pModuleInfos->EntryPoint is in VA not RVA, so no needs to add BaseAddress
            
            if (LoadCount == 0xFFFF)
                _tcscpy(ppc[ColumnModulesLoadCount],ModulesLoadCountStatic);
            else
                _stprintf(ppc[ColumnModulesLoadCount],_T("%u"),LoadCount);
            _stprintf(ppc[ColumnModulesFlags],_T("0x%.8X"),Flags);
            _stprintf(ppc[ColumnModulesTlsIndex],_T("%u"),TlsIndex);
            _stprintf(ppc[ColumnModulesTimeDateStamp],_T("0x%.8X"),TimeDateStamp);
        }
        else
        {
            CPE pe(ppc[ColumnModulesName]);
            if (pe.Parse())
            {
                __int64 EntryPoint = pe.NTHeader.OptionalHeader.AddressOfEntryPoint;
                if (EntryPoint)
                    EntryPoint+=(__int64)(me32.modBaseAddr);
                _stprintf(ppc[ColumnModulesEntryPoint],_T("0x%016I64X"),EntryPoint);
                _stprintf(ppc[ColumnModulesTimeDateStamp],_T("0x%.8X"),pe.NTHeader.FileHeader.TimeDateStamp);
            }
            else
            {
                _tcscpy(ppc[ColumnModulesEntryPoint],_T("Error retrieving Entry Point"));
                ppc[ColumnModulesTimeDateStamp][0]=0;
            }

            ppc[ColumnModulesLoadCount][0]=0;
            ppc[ColumnModulesFlags][0]=0;
            ppc[ColumnModulesTlsIndex][0]=0;
        }

        mpCListviewModules->AddItemAndSubItems(NB_MODULES_COLUMNS,ppc);
    }while (x86CC.Module32Next(hSnap64, &me32)); 

    if (pWow64EnableWow64FsRedirection)
        pWow64EnableWow64FsRedirection(TRUE);

    // clean up the snapshot object. 
    x86CC.CloseHandle(hSnap64); 
    }
#endif

    
    for (dwModuleIndex=0;dwModuleIndex<NB_MODULES_COLUMNS;dwModuleIndex++)
        delete ppc[dwModuleIndex];
    delete[] ppc;

    // resort listview by the last sorting order
    mpCListviewModules->ReSort();

    // auto select first item
    if (mpCListviewModules->GetItemCount()>0)
        mpCListviewModules->SetSelectedIndex(0);
}


//-----------------------------------------------------------------------------
// Name: RefreshThreads
// Object: refresh threads listview depending the selected process in process listview
// Parameters :
//          in : int ItemIndex : index of selected item in process listview
// Return : 
//-----------------------------------------------------------------------------
void RefreshThreads(int ItemIndex)
{
    TCHAR** ppc;
    DWORD dwModuleIndex;
    THREADENTRY32 te32 = {0}; 
    HANDLE hSnap;
    DWORD dwProcessNumber=0;

    HANDLE hThread;
    FILETIME CreationTime;
    FILETIME ExitTime;
    FILETIME KernelTime;
    FILETIME UserTime;


    dwProcessNumber=GetProcessIdFromIndex(ItemIndex);
    if (!dwProcessNumber)
        return;

    ppc=new TCHAR*[NB_THREADS_COLUMNS];
    for (dwModuleIndex=0;dwModuleIndex<NB_THREADS_COLUMNS;dwModuleIndex++)
    {
        ppc[dwModuleIndex]=new TCHAR[256];
        *ppc[dwModuleIndex]=0;
    }

    mpCListviewThreads->Clear();

    hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if (hSnap == INVALID_HANDLE_VALUE) 
    {
        CAPIError::ShowLastError();
        return; 
    }
    // Fill the size of the structure before using it. 
    te32.dwSize = sizeof(THREADENTRY32); 
 
    // Walk the thread list of the system
    if (!Thread32First(hSnap, &te32))
    {
        CloseHandle(hSnap);
        CAPIError::ShowLastError();
        return;
    }
    do 
    { 
        if (dwProcessNumber!=te32.th32OwnerProcessID)
            continue;
        _stprintf(ppc[ColumnThreadsId],_T("0x%.8X (%u)"),te32.th32ThreadID,te32.th32ThreadID);
        _stprintf(ppc[ColumnThreadsUsage],_T("%u"),te32.cntUsage);
        _stprintf(ppc[ColumnThreadsPriority],_T("%d+%d"),te32.tpBasePri,te32.tpDeltaPri);
        
        hThread = ::OpenThread(THREAD_QUERY_INFORMATION,FALSE,te32.th32ThreadID);
        if (hThread)
        {
            if (::GetThreadTimes(hThread,&CreationTime,&ExitTime,&KernelTime,&UserTime))
            {
                FileTimeToString(&CreationTime,TRUE,ppc[ColumnThreadsCreationTime],256);
                FileTimeToString(&KernelTime,FALSE,ppc[ColumnThreadsKernelTime],256);
                FileTimeToString(&UserTime,FALSE,ppc[ColumnThreadsUserTime],256);
            }

            ::CloseHandle(hThread);
        }
        else
        {
            *ppc[ColumnThreadsCreationTime]=0;
            *ppc[ColumnThreadsKernelTime]=0;
            *ppc[ColumnThreadsUserTime]=0;
        }


        mpCListviewThreads->AddItemAndSubItems(NB_THREADS_COLUMNS,ppc);
    } 
    while (Thread32Next(hSnap, &te32)); 
 
    // clean up the snapshot object. 
    CloseHandle (hSnap); 
    
    for (dwModuleIndex=0;dwModuleIndex<NB_THREADS_COLUMNS;dwModuleIndex++)
        delete ppc[dwModuleIndex];
    delete[] ppc;

    // resort listview by the last sorting order
    mpCListviewThreads->ReSort();

    // auto select first item
    if (mpCListviewThreads->GetItemCount()>0)
        mpCListviewThreads->SetSelectedIndex(0);
}

//-----------------------------------------------------------------------------
// Name: IsThreadAlive
// Object: check if thread is still alive
// Parameters :
//          in : DWORD dwThreadId : id of thread to check
// Return : 
//-----------------------------------------------------------------------------
BOOL IsThreadAlive(DWORD dwThreadId)
{

    THREADENTRY32 te32 = {0}; 
    HANDLE hSnap;
    BOOL bAlive=FALSE;

    hSnap =CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if (hSnap == INVALID_HANDLE_VALUE) 
    {
        CAPIError::ShowLastError();
        return FALSE; 
    }
    // Fill the size of the structure before using it. 
    te32.dwSize = sizeof(THREADENTRY32); 
 
    // Walk the thread list of the system
    if (!Thread32First(hSnap, &te32))
    {
        CloseHandle(hSnap);
        CAPIError::ShowLastError();
        return FALSE;
    }
    do 
    { 
        if (te32.th32ThreadID==dwThreadId)
        {
            bAlive=TRUE;
            break;
        }
    } 
    while (Thread32Next(hSnap, &te32)); 
 
    // clean up the snapshot object. 
    CloseHandle (hSnap); 

    return bAlive;
}

void SwitchMode()
{
    // switch modes
    UserMode=!UserMode;

    if (UserMode)
    {
        mpCListviewProcesses->InitListViewColumns(NB_PROCESSES_COLUMNS,mpColumnInfoProcesses);
        mpProcessesToolbar->ReplaceText(IDC_BUTTON_USER_KERNEL_MODE,_T("Kernel"));
        mpProcessesToolbar->ReplaceIcon(IDC_BUTTON_USER_KERNEL_MODE,CToolbar::ImageListTypeEnable,IDI_ICON_KERNEL);
        mpProcessesToolbar->ReplaceIcon(IDC_BUTTON_USER_KERNEL_MODE,CToolbar::ImageListTypeHot,IDI_ICON_KERNEL);
        mpProcessesToolbar->ReplaceText(IDC_BUTTON_DUMP_TO_EXE,_T("Dump to Exe"));
        mpProcessesModulesSplitter->Show();
        mpModulesThreadsSplitter->Show();
    }
    else
    {
        mpCListviewProcesses->InitListViewColumns(NB_PROCESSES_COLUMNS_KERNELMODE,mpColumnInfoProcessesKernelMode);
        mpProcessesToolbar->ReplaceText(IDC_BUTTON_USER_KERNEL_MODE,_T("User"));
        mpProcessesToolbar->ReplaceIcon(IDC_BUTTON_USER_KERNEL_MODE,CToolbar::ImageListTypeEnable,IDI_ICON_USER);
        mpProcessesToolbar->ReplaceIcon(IDC_BUTTON_USER_KERNEL_MODE,CToolbar::ImageListTypeHot,IDI_ICON_USER);
        mpProcessesToolbar->ReplaceText(IDC_BUTTON_DUMP_TO_EXE,_T("Dump to Sys"));
        mpProcessesModulesSplitter->Hide();
        mpModulesThreadsSplitter->Hide();
    }

    // hide some part
    ShowWindow(mpThreadsToolbar->GetControlHandle(),UserMode);
    CDialogHelper::ShowGroup(GetDlgItem(mhWndDialog,IDC_STATIC_GROUP_PROCESS_THREADS),UserMode);
    CDialogHelper::ShowGroup(GetDlgItem(mhWndDialog,IDC_STATIC_GROUP_PROCESS_MODULES),UserMode);

    // Enable/Disable toolbar items
    mpProcessesToolbar->EnableButton(IDC_BUTTON_PRIORITY,UserMode);
    mpProcessesToolbar->EnableButton(IDC_BUTTON_SUSPEND_PROCESS,UserMode);
    mpProcessesToolbar->EnableButton(IDC_BUTTON_RESUME_PROCESS,UserMode);
    mpProcessesToolbar->EnableButton(IDC_BUTTON_TERMINATE_PROCESS,UserMode);
    mpProcessesToolbar->EnableButton(IDC_BUTTON_INJLIB,UserMode);
    mpProcessesToolbar->EnableButton(IDC_BUTTON_EJECTLIB,UserMode);
    mpProcessesToolbar->EnableButton(IDC_BUTTON_INTEGRITY_CHECKING,UserMode);

    // enable/disable popup menu items
    mpCListviewProcesses->pPopUpMenu->SetEnabledState(mpProcessesMenuIdSuspend,UserMode);
    mpCListviewProcesses->pPopUpMenu->SetEnabledState(mpProcessesMenuIdResume,UserMode);
    mpCListviewProcesses->pPopUpMenu->SetEnabledState(mpProcessesMenuIdTerminate,UserMode);
    mpCListviewProcesses->pPopUpMenu->SetEnabledState(mpProcessesMenuIdProcessPriority,UserMode);
    mpCListviewModules->pPopUpMenu->SetEnabledState(mpModulesMenuIdDump,UserMode);
    mpCListviewModules->pPopUpMenu->SetEnabledState(mpModulesMenuIdCheckIntegrity,UserMode);
    mpCListviewModules->pPopUpMenu->SetEnabledState(mpModulesMenuIdEject,UserMode);
    mpCListviewThreads->pPopUpMenu->SetEnabledState(mpThreadMenuIdSuspend,UserMode);
    mpCListviewThreads->pPopUpMenu->SetEnabledState(mpThreadMenuIdResume,UserMode);
    mpCListviewThreads->pPopUpMenu->SetEnabledState(mpThreadMenuIdTerminate,UserMode);
    mpCListviewThreads->pPopUpMenu->SetEnabledState(mpThreadMenuIdContext,UserMode);
    mpCListviewThreads->pPopUpMenu->SetEnabledState(mpThreadMenuIdCallStack,UserMode);
    mpCListviewThreads->pPopUpMenu->SetEnabledState(mpThreadMenuIdSuspendedCount,UserMode);
    mpCListviewThreads->pPopUpMenu->SetEnabledState(mpThreadMenuIdThreadPriority,UserMode);


    // force new representation
    Resize();

    Refresh();
}

//-----------------------------------------------------------------------------
// Name: KernelDump
// Object: Dump selected kernel module (or module 0 if none is selected)
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
void KernelDump()
{
    TCHAR psz[MAX_PATH];
    DWORD Size;
    PVOID StartAddress;

    // get selected kernel module
    int iProcessIndex=mpCListviewProcesses->GetSelectedIndex();
    // if none is selected, select first
    if (iProcessIndex<0)
    {
        iProcessIndex=0;
        mpCListviewProcesses->SetSelectedIndex(0);
    }

    // get module base address
    mpCListviewProcesses->GetItemText(iProcessIndex,ColumnProcessesKernelBaseAddress,psz,MAX_PATH);
    _stscanf(psz,_T("0x%x"),&StartAddress);

    // get module size
    mpCListviewProcesses->GetItemText(iProcessIndex,ColumnProcessesKernelSize,psz,MAX_PATH);
    _stscanf(psz,_T("0x%x"),&Size);


    CKernelMemoryAccessInterface KMem;
    if (!KMem.StartDriver())
        return;

    if (!KMem.OpenDriver())
        return;

    // make dump
    BYTE* Buffer=new BYTE[Size];
    DWORD ReadSize=0;

    if (!KMem.ReadMemory(StartAddress,Size,Buffer,&ReadSize))
    {
        if (ReadSize==0)
        {
            MessageBox(mhWndDialog,_T("Error reading memory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            delete[] Buffer;
            return;
        }

        // else
        Size=ReadSize;
        _stprintf(psz,_T("Error reading memory.\r\nOnly first 0x%X bytes are readable"),ReadSize);
        MessageBox(mhWndDialog,psz,_T("Warning"),MB_OK|MB_ICONWARNING|MB_TOPMOST);
    }
    KMem.CloseDriver();

    /////////////////
    // save dump
    ////////////////

    OPENFILENAME ofn;
    TCHAR pszFileName[MAX_PATH];

    
    // save file dialog
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=mhWndDialog;
    ofn.hInstance=mhInstance;
    ofn.lpstrFilter=_T("dmp\0*.dmp\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt=_T("dmp");
    *pszFileName=0;
    ofn.lpstrFile=pszFileName;
    ofn.nMaxFile=MAX_PATH;
    
    if (!GetSaveFileName(&ofn))
    {
        delete[] Buffer;
        return;
    }

    // write dump
    HANDLE hFile = CreateFile(
                                ofn.lpstrFile,
                                GENERIC_READ|GENERIC_WRITE,
                                FILE_SHARE_READ|FILE_SHARE_WRITE, 
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        CAPIError::ShowLastError();
        delete[] Buffer;
        return;
    }

    if (!WriteFile(hFile,
                    Buffer,
                    Size,
                    &Size,
                    NULL
                    ))
    {
        CAPIError::ShowLastError();
        CloseHandle(hFile);
        delete[] Buffer;
        return;
    }

    CloseHandle(hFile);
    delete[] Buffer;

    // show message information
    MessageBox(NULL,_T("Dump successfully completed"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);

    return;
}
//-----------------------------------------------------------------------------
// Name: Dump
// Object: Dump selected module (or module 0 if none is selected)
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
void Dump()
{
    if (!UserMode)
        return KernelDump();

    DWORD dwProcessID=0;
    HMODULE hModule=0;
    int iProcessIndex=mpCListviewProcesses->GetSelectedIndex();

    int iModuleIndex=mpCListviewModules->GetSelectedIndex();

    if (iProcessIndex<0)
    {
        iProcessIndex=0;
        mpCListviewProcesses->SetSelectedIndex(0);
    }
    if (iModuleIndex<0)
    {
        iModuleIndex=0;
        mpCListviewModules->SetSelectedIndex(0);
    }

    dwProcessID=GetProcessIdFromIndex(iProcessIndex);
    hModule=(HMODULE)GetModuleBaseAddress(iModuleIndex);

    if ((dwProcessID==0)||(hModule==0))
        return;

    CDump::Dump(dwProcessID,hModule);// could be done with Toolhelp32ReadProcessMemory too
}


//-----------------------------------------------------------------------------
// Name: ProcessSelectionCallBack
// Object: called when an item is selected in the Process listview
// Parameters :
//     in : int ItemIndex : item index in the Process listview
//          int SubitemIndex : subitem index in the Process listview
//          LPVOID UserParam
// Return : 
//-----------------------------------------------------------------------------
void ProcessSelectionCallBack(int ItemIndex, int SubitemIndex,LPVOID UserParam)
{
    if (!UserMode)
        return;
    UNREFERENCED_PARAMETER(SubitemIndex);
    UNREFERENCED_PARAMETER(UserParam);
    if (!bRefreshing)
    {
        RefreshModules(ItemIndex);
        RefreshThreads(ItemIndex);
    }
}

//-----------------------------------------------------------------------------
// Name: ProcessesPopUpCallBack
// Object: called when an item of the popup menu of the Process listview
//          is clicked
// Parameters :
//     in : UINT MenuID : Id of the clicked menu
//          LPVOID UserParam
// Return : 
//-----------------------------------------------------------------------------
void ProcessesPopUpCallBack(UINT MenuID,LPVOID UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);

    // retrieve process id
    TCHAR psz[MAX_PATH];
    int ItemIndex=0;
    DWORD dwProcessId=0;
    ItemIndex=mpCListviewProcesses->GetSelectedIndex();
    if (ItemIndex==-1)
    {
        MessageBox(mhWndDialog,_T("No Process Selected"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }
    mpCListviewProcesses->GetItemText(ItemIndex,ColumnProcessesId,psz,MAX_PATH);
    if (_stscanf(psz,_T("0x%X"),&dwProcessId)!=1)
    {
#ifdef _DEBUG
        DebugBreak();
#endif
        return;
    }

    if (MenuID==mpProcessesMenuIdDump)
        Dump();
    else if (MenuID==mpProcessesMenuIdSuspend)
    {
        if (dwProcessId==GetCurrentProcessId())
        {
            MessageBox(mhWndDialog,_T("Can't suspend my process"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
            return;
        }
        if (CProcessHelper::SuspendProcess(dwProcessId))
            MessageBox(mhWndDialog,_T("Process successfully suspended"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
    }
    else if (MenuID==mpProcessesMenuIdResume)
    {
        if (CProcessHelper::ResumeProcess(dwProcessId))
            MessageBox(mhWndDialog,_T("Process successfully resumed"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
    }
    else if (MenuID==mpProcessesMenuIdTerminate)
    {
        HANDLE hProcess=OpenProcess(PROCESS_TERMINATE,FALSE,dwProcessId);
        if (!hProcess)
        {
            CAPIError::ShowLastError();
            return;
        }

        if (!TerminateProcess(hProcess,0))
            CAPIError::ShowLastError();
        CloseHandle(hProcess);
        Refresh();
    }

    else
    {
        HANDLE hProcess=OpenProcess(PROCESS_SET_INFORMATION ,FALSE,dwProcessId);
        if (!hProcess)
        {
            CAPIError::ShowLastError();
            return;
        }

        if (MenuID==mpProcessesMenuIdProcessPriorityAboveNormal)
        {
            if (!SetPriorityClass(hProcess,ABOVE_NORMAL_PRIORITY_CLASS))
                CAPIError::ShowLastError();
        }
        else if (MenuID==mpProcessesMenuIdProcessPriorityBelowNormal)
        {
            if (!SetPriorityClass(hProcess,BELOW_NORMAL_PRIORITY_CLASS))
                CAPIError::ShowLastError();
        }
        else if (MenuID==mpProcessesMenuIdProcessPriorityHigh)
        {
            if (!SetPriorityClass(hProcess,HIGH_PRIORITY_CLASS))
                CAPIError::ShowLastError();
        }
        else if (MenuID==mpProcessesMenuIdProcessPriorityIdle)
        {
            if (!SetPriorityClass(hProcess,IDLE_PRIORITY_CLASS))
                CAPIError::ShowLastError();
        }
        else if (MenuID==mpProcessesMenuIdProcessPriorityNormal)
        {
            if (!SetPriorityClass(hProcess,NORMAL_PRIORITY_CLASS))
                CAPIError::ShowLastError();
        }
        else if (MenuID==mpProcessesMenuIdProcessPriorityRealTime)
        {
            if (!SetPriorityClass(hProcess,REALTIME_PRIORITY_CLASS))
                CAPIError::ShowLastError();
        }
        CloseHandle(hProcess);
        Sleep(1);// allow system to do it's job
        Refresh();
        // not the same item
        ListView_EnsureVisible(mpCListviewProcesses->GetControlHandle(),ItemIndex,FALSE);
    }
}

//-----------------------------------------------------------------------------
// Name: ModulesPopUpCallBack
// Object: called when an item of the popup menu of the modules listview
//          is clicked
// Parameters :
//     in : UINT MenuID : Id of the clicked menu
//          LPVOID UserParam
// Return : 
//-----------------------------------------------------------------------------
void ModulesPopUpCallBack(UINT MenuID,LPVOID UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);

    if (MenuID==mpModulesMenuIdDump)
    {
        Dump();
    }
    else if (MenuID==mpModulesMenuIdCheckIntegrity)
    {
        ModuleIntegrityChecking(GetSelectedModuleIndex());
    }
    else if (MenuID==mpModulesMenuIdEject)
    {
        EjectLibrary();
    }
}

//-----------------------------------------------------------------------------
// Name: ThreadsPopUpCallBack
// Object: called when an item of the popup menu of the threads listview
//          is clicked
// Parameters :
//     in : UINT MenuID : Id of the clicked menu
//          LPVOID UserParam
// Return : 
//-----------------------------------------------------------------------------
void ThreadsPopUpCallBack(UINT MenuID,LPVOID UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);

    // retrieve thread id
    DWORD dwThreadId=0;
    TCHAR psz[MAX_PATH];
    int ItemIndex=mpCListviewThreads->GetSelectedIndex();
    if (ItemIndex<0)
    {
        MessageBox(mhWndDialog,_T("No Thread Selected"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return;
    }

    mpCListviewThreads->GetItemText(ItemIndex,ColumnThreadsId,psz,MAX_PATH);
    // get thread ID
    if (_stscanf(psz,_T("0x%X"),&dwThreadId)!=1)
        return;

    if (!IsThreadAlive(dwThreadId))
    {
        TCHAR pszMsg[MAX_PATH];
        _stprintf(pszMsg,_T("Thread 0x%X (%u) doesn't exist anymore"),dwThreadId,dwThreadId);
        MessageBox(mhWndDialog,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        RefreshProcesses();
        return;
    }

    // open thread
    HANDLE hThread=OpenThread( THREAD_ALL_ACCESS,
                              FALSE,
                              dwThreadId
                             );
    if (!hThread)
    {
        CAPIError::ShowLastError();
        return;
    }

    // switch PopUpMenuID
    if (MenuID==mpThreadMenuIdSuspend)
    {
        if (dwThreadId==GetCurrentThreadId())
        {
            MessageBox(NULL,_T("Can't suspend my thread"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
            // close thread handle
            CloseHandle(hThread);
            return;
        }
        if (SuspendThread(hThread)==(DWORD)-1)
            CAPIError::ShowLastError();
        else
            MessageBox(mhWndDialog,_T("Thread successfully suspended"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
    }
    else if (MenuID==mpThreadMenuIdResume)
    {
        if (ResumeThread(hThread)==(DWORD)-1)
            CAPIError::ShowLastError();
        else
            MessageBox(mhWndDialog,_T("Thread successfully resumed"),_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
    }
    else if (MenuID==mpThreadMenuIdTerminate)
    {
        if (!TerminateThread(hThread,0))
            CAPIError::ShowLastError();
        RefreshThreads(mpCListviewProcesses->GetSelectedIndex());
    }
	else if (MenuID==mpThreadMenuIdSuspendedCount)
	{
        if (dwThreadId==GetCurrentThreadId())
        {
            // if we are here our thread is not suspended :) --> SuspendedCount=0
            _stprintf(psz,_T("Thread 0x%X (%u) suspended count: %u"),dwThreadId,dwThreadId,0);
			MessageBox(mhWndDialog,psz,_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
            // close thread handle
            CloseHandle(hThread);
            return;
        }
		// if someone just knows another way of SuspendThread/ResumeThread
		// to get thread suspended count just tell me
		DWORD SuspendedCount;
        SuspendedCount=SuspendThread(hThread);
		if (SuspendedCount==(DWORD)-1)
            CAPIError::ShowLastError();
		if (ResumeThread(hThread)==(DWORD)-1)
            CAPIError::ShowLastError();
		else
		{
            if (SuspendedCount!=(DWORD)-1)
            {
			    _stprintf(psz,_T("Thread 0x%X (%u) suspended count: %u"),dwThreadId,dwThreadId,SuspendedCount);
			    MessageBox(mhWndDialog,psz,_T("Information"),MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
            }
		}
	}
    else if (MenuID==mpThreadMenuIdThreadPriorityTimeCritical)
    {
        if (!SetThreadPriority(hThread,THREAD_PRIORITY_TIME_CRITICAL))
            CAPIError::ShowLastError();
        RefreshThreads(mpCListviewProcesses->GetSelectedIndex());
    }
    else if (MenuID==mpThreadMenuIdThreadPriorityHighest)
    {
        if (!SetThreadPriority(hThread,THREAD_PRIORITY_HIGHEST))
            CAPIError::ShowLastError();
        RefreshThreads(mpCListviewProcesses->GetSelectedIndex());
    }
    else if (MenuID==mpThreadMenuIdThreadPriorityAboveNormal)
    {
        if (!SetThreadPriority(hThread,THREAD_PRIORITY_ABOVE_NORMAL))
            CAPIError::ShowLastError();
        RefreshThreads(mpCListviewProcesses->GetSelectedIndex());
    }
    else if (MenuID==mpThreadMenuIdThreadPriorityNormal)
    {
        if (!SetThreadPriority(hThread,THREAD_PRIORITY_NORMAL))
            CAPIError::ShowLastError();
        RefreshThreads(mpCListviewProcesses->GetSelectedIndex());
    }
    else if (MenuID==mpThreadMenuIdThreadPriorityBelowNormal)
    {
        if (!SetThreadPriority(hThread,THREAD_PRIORITY_BELOW_NORMAL))
            CAPIError::ShowLastError();
        RefreshThreads(mpCListviewProcesses->GetSelectedIndex());
    }
    else if (MenuID==mpThreadMenuIdThreadPriorityLowest)
    {
        if (!SetThreadPriority(hThread,THREAD_PRIORITY_LOWEST))
            CAPIError::ShowLastError();
        RefreshThreads(mpCListviewProcesses->GetSelectedIndex());
    }
    else if (MenuID==mpThreadMenuIdThreadPriorityIdle)
    {
        if (!SetThreadPriority(hThread,THREAD_PRIORITY_IDLE))
            CAPIError::ShowLastError();
        RefreshThreads(mpCListviewProcesses->GetSelectedIndex());
    }
    else if (MenuID==mpThreadMenuIdContext)
    {
        CONTEXT ct={0};
        CThreadContext ThreadContext;
        CProcessAndThreadID ProcessAndThreadID;
        DWORD SuspendedCount=0;

        if (ProcessAndThreadID.GetThreadId(hThread)==GetCurrentThreadId())
        {
// stupid stuff anyway
            __asm
            {
label:
                push eax
                mov eax,label
                mov [ct.Eip],eax
                pop eax
                mov [ct.Ebp],ebp
                mov [ct.Esp],esp
                mov [ct.Eax],eax
                mov [ct.Ebx],ebx
                mov [ct.Ecx],ecx
                mov [ct.Edx],edx
                mov [ct.Edi],edi
                mov [ct.Esi],esi
                pushfd
                pop [ct.EFlags]
            }

            DialogBoxParam(mhInstance, (LPCTSTR)IDD_DIALOG_THREAD_CONTEXT, mhWndDialog, (DLGPROC)ContextRegistersWndProc,(LPARAM)&ct);
        }
        else
        {
            SuspendedCount=SuspendThread(hThread);
            if (SuspendedCount==(DWORD)-1)
                CAPIError::ShowLastError();
            if (ResumeThread(hThread)==(DWORD)-1)
                CAPIError::ShowLastError();
        

            // get context
            if (ThreadContext.GetThreadContext(hThread,&ct))
            {
                // on success show window
                DialogBoxParam(mhInstance, (LPCTSTR)IDD_DIALOG_THREAD_CONTEXT, mhWndDialog, (DLGPROC)ContextRegistersWndProc,(LPARAM)&ct);

                // if thread was not suspended (else don't free memory)
                if (SuspendedCount==0)
                    ThreadContext.GetThreadContextFree();
            }
        }

    }
    else if (MenuID==mpThreadMenuIdCallStack)
    {
        CThreadCallStack ThreadCallStack(hThread);
        ThreadCallStack.Show(mhInstance,mhWndDialog);
    }
    // close thread handle
    CloseHandle(hThread);
}

//-----------------------------------------------------------------------------
// Name: ProcessesDropDownMenuCallBack
// Object: called when an item of the popup menu of the processes toolbar
//          is clicked
// Parameters :
//     in : CPopUpMenu* PopUpMenu : CPopUpMenu object pointer from which event comes
//          UINT MenuId : clicked menu Id
//          PVOID UserParam : user param provided when callback has been attached
// Return : 
//-----------------------------------------------------------------------------
void ProcessesDropDownMenuCallBack(CPopUpMenu* PopUpMenu,UINT MenuId,PVOID UserParam)
{
    if (PopUpMenu==mpPopUpMenuDumpToExe)
    {
        if (MenuId==mpDumpToExeMenuIdRemoveNotRAW)
        {
            CDumpToExe::RemoveUnusedMemory();
        }
        else if (MenuId==mpDumpToExeMenuIdModifyPE)
        {
            CDumpToExe::KeepAllAndModifyPe();
        }
    }
    else if (PopUpMenu==mpPopUpMenuProcessPriority)
    {
        ProcessesPopUpCallBack(MenuId,UserParam);
    }
    else if (PopUpMenu==mpPopUpMenuIntegrityChecking)
    {
        if (MenuId==mpPopUpMenuIntegrityCheckingModule)
        {
            ModuleIntegrityChecking(GetSelectedModuleIndex());
        }
        else if (MenuId==mpPopUpMenuIntegrityCheckingProcess)
        {
            ProcessIntegrityChecking();
        }
        else if (MenuId==mpPopUpMenuIntegrityCheckingCheckNotExecutableSections)
        {
            // switch checked state
            mpPopUpMenuIntegrityChecking->SetCheckedState(MenuId,!mpPopUpMenuIntegrityChecking->IsChecked(MenuId));
        }
        else if (MenuId==mpPopUpMenuIntegrityCheckingCheckWritableSections)
        {
            // switch checked state
            mpPopUpMenuIntegrityChecking->SetCheckedState(MenuId,!mpPopUpMenuIntegrityChecking->IsChecked(MenuId));
        }
        else if (MenuId==mpPopUpMenuIntegrityCheckingShowRebasing)
        {
            // switch checked state
            mpPopUpMenuIntegrityChecking->SetCheckedState(MenuId,!mpPopUpMenuIntegrityChecking->IsChecked(MenuId));
        }

    }
}
BOOL LoadInjLibrary()
{
    hModInjLib=LoadLibrary(_T("InjLib.dll"));
    if (!hModInjLib)
    {
        MessageBox(mhWndDialog,_T("Can't load InjLib.dll\r\nAssume dll is present in dumper directory"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }
    return TRUE;
}
void InjectLibrary()
{
    // load library if not already done
    if (hModInjLib==NULL)
    {
        if (!LoadInjLibrary())
            return;
    }
    // get function pointer if not already done
    if (pInjectLibrary==NULL)
    {
        pInjectLibrary=(InjectLib)GetProcAddress(hModInjLib,INJECTLIB_FUNC_NAME);
        if (!pInjectLibrary)
            return;
    }

    DWORD ProcessId=GetSelectedProcessId();
    if (!ProcessId)
        return;

    TCHAR pszDllPath[MAX_PATH];
    *pszDllPath=0;
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=mhWndDialog;
    ofn.hInstance=mhInstance;
    ofn.lpstrFilter=_T("dll\0*.dll\0All\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    ofn.lpstrFile=pszDllPath;
    ofn.nMaxFile=MAX_PATH;
    ofn.lpstrTitle=_T("Select dll to inject in selected process");

    // get file name
    if (GetOpenFileName(&ofn))
    {
        if (pInjectLibrary(ProcessId,pszDllPath))
            RefreshModules(mpCListviewProcesses->GetSelectedIndex());
    }
}
void EjectLibrary()
{
    // load library if not already done
    if (hModInjLib==NULL)
    {
        if (!LoadInjLibrary())
            return;
    }
    // get function pointer if not already done
    if (pEjectLibrary==NULL)
    {
        pEjectLibrary=(EjectLib)GetProcAddress(hModInjLib,EJECTLIB_FUNC_NAME);
        if (!pEjectLibrary)
            return;
    }


    TCHAR pszDllPath[MAX_PATH];
    TCHAR pszLoadCount[MAX_PATH];
    DWORD ProcessId=GetSelectedProcessId();
    if (!ProcessId)
        return;

    mpCListviewModules->GetItemText(mpCListviewModules->GetSelectedIndex(),ColumnModulesName,pszDllPath,MAX_PATH);
    mpCListviewModules->GetItemText(mpCListviewModules->GetSelectedIndex(),ColumnModulesLoadCount,pszLoadCount,MAX_PATH);
    if (*pszLoadCount)
    {
        if ( _tcscmp(pszLoadCount,ModulesLoadCountStatic) == 0 )
        {
            MessageBox(mhWndDialog,_T("Can't eject static linked library"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            return;
        }

        SIZE_T LoadCount;
        if (CStringConverter::StringToSIZE_T(pszLoadCount,&LoadCount))
        {
            for (SIZE_T Cnt = 0; Cnt<LoadCount; Cnt++)
                pEjectLibrary(ProcessId,pszDllPath);
            RefreshModules(mpCListviewProcesses->GetSelectedIndex());
        }
        else
        {
            if (pEjectLibrary(ProcessId,pszDllPath))
                RefreshModules(mpCListviewProcesses->GetSelectedIndex());
        }
    }
    else
    {
        if (pEjectLibrary(ProcessId,pszDllPath))
            RefreshModules(mpCListviewProcesses->GetSelectedIndex());
    }
}


void ProcessIntegrityChecking()
{
    CModuleIntegrityChecking* pModuleIntegrityChecking;
    CLinkListSimple* pList=new CLinkListSimple();

    // for each module
    for (int cnt=0;cnt<mpCListviewModules->GetItemCount();cnt++)
    {
        // check integrity
        pModuleIntegrityChecking=ModuleIntegrityCheck(cnt);
        if (pModuleIntegrityChecking)
            pList->AddItem(pModuleIntegrityChecking);
    }
    DisplayModuleIntegrityChecking(pList);
}

void ModuleIntegrityChecking(int ItemIndex)
{
    CLinkListSimple* pList=new CLinkListSimple();
    CModuleIntegrityChecking* pModuleIntegrityChecking;
    pModuleIntegrityChecking=ModuleIntegrityCheck(ItemIndex);
    if (!pModuleIntegrityChecking)
    {
        delete pList;
        return;
    }
    pList->AddItem(pModuleIntegrityChecking);
    DisplayModuleIntegrityChecking(pList);
}
CModuleIntegrityChecking* ModuleIntegrityCheck(int ItemIndex)
{
    TCHAR DllPath[MAX_PATH];

    CModuleIntegrityChecking* pModuleIntegrityChecking;
    pModuleIntegrityChecking=new CModuleIntegrityChecking();
    BOOL CheckOnlyCodeAndExecutableSections;
    BOOL DontCheckWritableSections;
    BOOL ShowRebasing;
    CheckOnlyCodeAndExecutableSections=!mpPopUpMenuIntegrityChecking->IsChecked(mpPopUpMenuIntegrityCheckingCheckNotExecutableSections);
    DontCheckWritableSections=!mpPopUpMenuIntegrityChecking->IsChecked(mpPopUpMenuIntegrityCheckingCheckWritableSections);
    ShowRebasing=mpPopUpMenuIntegrityChecking->IsChecked(mpPopUpMenuIntegrityCheckingShowRebasing);

    mpCListviewModules->GetItemText(ItemIndex,ColumnModulesName,DllPath,MAX_PATH);
    if (!pModuleIntegrityChecking->CheckIntegrity(
                                                    GetSelectedProcessId(),
                                                    DllPath,
                                                    CheckOnlyCodeAndExecutableSections,// BOOL CheckOnlyCodeAndExecutableSections
                                                    DontCheckWritableSections,         // BOOL DontCheckWritableSections
                                                    ShowRebasing,                      // BOOL ShowRebasing
                                                    CHECK_INTEGRITY_MinMatchingSizeAfterReplacement)   // DWORD MinMatchingSizeAfterReplacement 
        )                                                    
    {
        MessageBox(mhWndDialog,_T("Error checking module integrity"),_T("Error"),MB_OK|MB_TOPMOST|MB_ICONERROR);
        delete pModuleIntegrityChecking;
        return NULL;
    }
    return pModuleIntegrityChecking;
}



//-----------------------------------------------------------------------------
// Name: ThreadsDropDownMenuCallBack
// Object: called when an item of the popup menu of the threads toolbar
//          is clicked
// Parameters :
//     in : CPopUpMenu* PopUpMenu : CPopUpMenu object pointer from which event comes
//          UINT MenuId : clicked menu Id
//          PVOID UserParam : user param provided when callback has been attached
// Return : 
//-----------------------------------------------------------------------------
void ThreadsDropDownMenuCallBack(CPopUpMenu* PopUpMenu,UINT MenuId,PVOID UserParam)
{
    if (PopUpMenu==mpPopUpMenuThreadPriority)
    {
        ThreadsPopUpCallBack(MenuId,UserParam);
    }
}

//-----------------------------------------------------------------------------
// Name: ContextRegistersWndProc
// Object: thread Context registers window WndProc
// Parameters :
//     in : HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam
// Return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK ContextRegistersWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		case WM_INITDIALOG:
            {
            mhContextRegistershDlg=hDlg;

            // set fixed width font
            HFONT hFont=CreateFont (14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                    DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, NULL);
            if (hFont)
                SendMessage((HWND) GetDlgItem(hDlg,IDC_EDIT_CONTEXT_REGISTERS),(UINT) WM_SETFONT,(WPARAM)hFont,FALSE);

            CONTEXT* pCt=(CONTEXT*)lParam;
            TCHAR psz[MAX_PATH];

            _stprintf(psz,_T("EIP: 0x%.8X  EFL: 0x%.8X\r\n")
                          _T("EBP: 0x%.8X  ESP: 0x%.8X\r\n")
                          _T("EAX: 0x%.8X  EBX: 0x%.8X\r\n")
                          _T("ECX: 0x%.8X  EDX: 0x%.8X\r\n")
                          _T("EDI: 0x%.8X  ESI: 0x%.8X\r\n"),
                                 pCt->Eip,pCt->EFlags,
                                 pCt->Ebp,pCt->Esp,
                                 pCt->Eax,pCt->Ebx,
                                 pCt->Ecx,pCt->Edx,
                                 pCt->Edi,pCt->Esi);

            SetDlgItemText(hDlg,IDC_EDIT_CONTEXT_REGISTERS,psz);
			return FALSE;
            }
		case WM_COMMAND:
            switch(LOWORD(wParam))
            {
            case IDOK:
            case IDCANCEL:
                EndDialog(mhContextRegistershDlg,0);
				return TRUE;
            }
            break;
    }
    return FALSE;

}

void CheckSize(RECT* pWinRect)
{
    // check min widh and min height
    if ((pWinRect->right-pWinRect->left)<APP_MIN_WIDTH)
        pWinRect->right=pWinRect->left+APP_MIN_WIDTH;
    if ((pWinRect->bottom-pWinRect->top)<APP_MIN_HEIGHT)
        pWinRect->bottom=pWinRect->top+APP_MIN_HEIGHT;
}


//-----------------------------------------------------------------------------
// Name: Resize
// Object: resize main window (called on a WM_SIZE message)
// Parameters :
//      in :
// Return : 
//-----------------------------------------------------------------------------
void Resize()
{
    HWND hItem;
    RECT Rect;
    RECT RectWindow;

    RECT RectGroup;
    CDialogHelper::GetClientWindowRect(mhWndDialog,mhWndDialog,&RectWindow);


    if (!UserMode)
    {
        // in kernel mode, only process listview is shown

        // process listview
        hItem=mpCListviewProcesses->GetControlHandle();
        CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&Rect);
        SetWindowPos(hItem,HWND_NOTOPMOST,
                    0,
                    0,
                    (RectWindow.right-RectWindow.left)-2*Rect.left+2*RectWindow.left,
                    RectWindow.bottom-Rect.top-SPACE_BETWEEN_CONTROLS,
                    SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
        CDialogHelper::Redraw(hItem);

        return;
    }

    // process listview
    hItem=mpCListviewProcesses->GetControlHandle();
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
                0,
                0,
                (RectWindow.right-RectWindow.left)-2*Rect.left+2*RectWindow.left,
                Rect.bottom-Rect.top,
                SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItem);

    // thread group
    hItem=GetDlgItem(mhWndDialog,IDC_STATIC_GROUP_PROCESS_THREADS);
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&RectGroup);
    SetWindowPos(hItem,HWND_NOTOPMOST,
                 0,
                 0,
                 (RectWindow.right-RectWindow.left)-2*RectGroup.left+2*RectWindow.left,
                 RectGroup.bottom-RectGroup.top,
                 SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&RectGroup);
    CDialogHelper::Redraw(hItem);

	// thread toolbar
	// mpThreadsToolbar->Autosize();
	hItem=mpThreadsToolbar->GetControlHandle();
	CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&Rect);
	SetWindowPos(hItem,HWND_NOTOPMOST,
				0,0,
				(RectGroup.right-RectGroup.left)-2*SPACE_BETWEEN_CONTROLS,
				Rect.bottom-Rect.top,
				SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOREPOSITION);
	CDialogHelper::Redraw(hItem);

    // thread listview
    hItem=mpCListviewThreads->GetControlHandle();
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
                0,
                0,
                (RectGroup.right-RectGroup.left)-2*SPACE_BETWEEN_CONTROLS,
                Rect.bottom-Rect.top,
                SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItem);

    // modules group
    hItem=GetDlgItem(mhWndDialog,IDC_STATIC_GROUP_PROCESS_MODULES);
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&RectGroup);
    SetWindowPos(hItem,HWND_NOTOPMOST,
                 0,
                 0,
                 (RectWindow.right-RectWindow.left)-2*RectGroup.left+2*RectWindow.left,
                 RectGroup.bottom-RectGroup.top,
                 SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&RectGroup);
    CDialogHelper::Redraw(hItem);

    // modules listview
    hItem=mpCListviewModules->GetControlHandle();
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
                0,
                0,
                (RectGroup.right-RectGroup.left)-2*SPACE_BETWEEN_CONTROLS,
                Rect.bottom-Rect.top,
                SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItem);

    mpProcessesModulesSplitter->Redraw();
    mpModulesThreadsSplitter->Redraw();

    // autosize toolbar (not needed if using rebar)
    //if (mpProcessesToolbar)    
    //    mpProcessesToolbar->Autosize();
}

void OnModulesThreadsSplitterCollapsedStateChange(BOOL NewCollapsedState,PVOID UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);
    HWND HwndGroup=GetDlgItem(mhWndDialog,IDC_STATIC_GROUP_PROCESS_THREADS);
    CDialogHelper::ShowGroup(HwndGroup,!NewCollapsedState);

    RECT RectWindow;
    RECT RectModulesThreads;
    // get window rect
    CDialogHelper::GetClientWindowRect(mhWndDialog,mhWndDialog,&RectWindow);
    mpModulesThreadsSplitter->GetRect(&RectModulesThreads);
    mpProcessesModulesSplitter->BottomMinFreeSpace=RectWindow.bottom-RectModulesThreads.top-RectWindow.left;

    if (mpProcessesModulesSplitter->IsCollapsed())
        mpProcessesModulesSplitter->Redraw();
}
void OnProcessesModulesSplitterCollapsedStateChange(BOOL NewCollapsedState,PVOID UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);
    HWND HwndGroup=GetDlgItem(mhWndDialog,IDC_STATIC_GROUP_PROCESS_MODULES);
    CDialogHelper::ShowGroup(HwndGroup,!NewCollapsedState);


    RECT Rect;
    RECT RectWindow;
    CDialogHelper::GetClientWindowRect(mhWndDialog,mhWndDialog,&RectWindow);
    mpProcessesModulesSplitter->GetRect(&Rect);
    if (NewCollapsedState)// on collapse
    {
        mpModulesThreadsSplitter->TopMinFreeSpace=MIN_PROCESSES_GROUP_HEIGHT+Rect.bottom-Rect.top; 
    }
}

void OnProcessesModulesSplitterMove(int LeftOrTopSplitterPos,int RightOrBottomSplitterPos,PVOID UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);
    RECT RectWindow;
    CDialogHelper::GetClientWindowRect(mhWndDialog,mhWndDialog,&RectWindow);

    if (!mpProcessesModulesSplitter->IsCollapsed())
    {
        // update limit of other splitter
        mpModulesThreadsSplitter->TopMinFreeSpace=RightOrBottomSplitterPos-RectWindow.top;
    }

    // change position resize group and listview
    POINT p;
    RECT Rect;
    RECT RectModulesThreads;
    RECT RectProcessesModules;
    HWND hItem;
    HWND HwndGroup;
    
    mpModulesThreadsSplitter->GetRect(&RectModulesThreads);
    mpProcessesModulesSplitter->GetRect(&RectProcessesModules);

    HwndGroup=mpCListviewProcesses->GetControlHandle();
    CDialogHelper::GetClientWindowRect(mhWndDialog,HwndGroup,&Rect);
    if (LeftOrTopSplitterPos<MIN_PROCESSES_GROUP_HEIGHT)
        ShowWindow(HwndGroup,FALSE);
    else
        ShowWindow(HwndGroup,TRUE);

    HwndGroup=GetDlgItem(mhWndDialog,IDC_STATIC_GROUP_PROCESS_MODULES);
    if (RectModulesThreads.top-RightOrBottomSplitterPos<MIN_MODULES_GROUP_HEIGHT)
        CDialogHelper::ShowGroup(HwndGroup,FALSE);
    else
        CDialogHelper::ShowGroup(HwndGroup,TRUE);


    // resize module group
    CDialogHelper::GetClientWindowRect(mhWndDialog,HwndGroup,&Rect);
    p.x=Rect.left;
    p.y=RightOrBottomSplitterPos;
    CDialogHelper::MoveGroupTo(mhWndDialog,HwndGroup,&p);
    Rect.top+=RightOrBottomSplitterPos;
    Rect.bottom+=RightOrBottomSplitterPos;
    SetWindowPos(HwndGroup,HWND_NOTOPMOST,
                0,
                0,
                Rect.right-Rect.left,
                RectModulesThreads.top-RightOrBottomSplitterPos,
                SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(HwndGroup);
    
    // resize module listview
    hItem=mpCListviewModules->GetControlHandle();
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
                0,
                0,
                Rect.right-Rect.left,
                RectModulesThreads.top-Rect.top-SPACE_BETWEEN_CONTROLS,
                SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItem);

    // resize processes group
    hItem=mpCListviewProcesses->GetControlHandle();
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
                0,
                0,
                Rect.right-Rect.left,
                LeftOrTopSplitterPos-Rect.top,
                SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItem);
}
void OnModulesThreadsSplitterMove(int LeftOrTopSplitterPos,int RightOrBottomSplitterPos,PVOID UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);
    RECT RectWindow;
    POINT p;
    RECT Rect;
    HWND hItem;
    HWND HwndGroup;
    CDialogHelper::GetClientWindowRect(mhWndDialog,mhWndDialog,&RectWindow);

    // update limit of other splitter
    mpProcessesModulesSplitter->BottomMinFreeSpace=RectWindow.bottom-LeftOrTopSplitterPos;
    if (mpProcessesModulesSplitter->IsCollapsed())
    {
        mpProcessesModulesSplitter->Redraw();
    }

    mpProcessesModulesSplitter->GetRect(&Rect);
    HwndGroup=GetDlgItem(mhWndDialog,IDC_STATIC_GROUP_PROCESS_MODULES);
    if (LeftOrTopSplitterPos-Rect.bottom<MIN_MODULES_GROUP_HEIGHT)
        CDialogHelper::ShowGroup(HwndGroup,FALSE);
    else
        CDialogHelper::ShowGroup(HwndGroup,TRUE);


    HwndGroup=GetDlgItem(mhWndDialog,IDC_STATIC_GROUP_PROCESS_THREADS);
    if (RectWindow.bottom-RightOrBottomSplitterPos<MIN_THREADS_GROUP_HEIGHT)
    {
        CDialogHelper::ShowGroup(HwndGroup,FALSE);
        ShowWindow(mpThreadsToolbar->GetControlHandle(),FALSE);
    }
    else
    {
        CDialogHelper::ShowGroup(HwndGroup,TRUE);
        ShowWindow(mpThreadsToolbar->GetControlHandle(),TRUE);
    }

    // move thread group
    CDialogHelper::GetClientWindowRect(mhWndDialog,HwndGroup,&Rect);
    p.x=Rect.left;
    p.y=RightOrBottomSplitterPos;
    CDialogHelper::MoveGroupTo(mhWndDialog,HwndGroup,&p);
    Rect.top+=RightOrBottomSplitterPos;
    Rect.bottom+=RightOrBottomSplitterPos;

    // resize Threads group
    SetWindowPos(HwndGroup,HWND_NOTOPMOST,
                0,
                0,
                Rect.right-Rect.left,
                RectWindow.bottom-RightOrBottomSplitterPos+RectWindow.left-SPACE_BETWEEN_CONTROLS,
                SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(HwndGroup);
    

    // move Threads Toolbar
    CDialogHelper::GetClientWindowRect(mhWndDialog,HwndGroup,&Rect);
    SetWindowPos(mpThreadsToolbar->GetControlHandle(),HWND_NOTOPMOST,
                Rect.left+SPACE_BETWEEN_CONTROLS,  
                Rect.top+3*SPACE_BETWEEN_CONTROLS,
                0,
                0,
                SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    // CDialogHelper::Redraw(mpThreadsToolbar->GetControlHandle()); // flashing effect

    // resize thread listview
    hItem=mpCListviewThreads->GetControlHandle();
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
                0,
                0,
                Rect.right-Rect.left,
                RectWindow.bottom+RectWindow.left-2*SPACE_BETWEEN_CONTROLS-Rect.top,
                SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItem);


    // resize module group
    RECT RectModuleGroup;
    HwndGroup=GetDlgItem(mhWndDialog,IDC_STATIC_GROUP_PROCESS_MODULES);
    CDialogHelper::GetClientWindowRect(mhWndDialog,HwndGroup,&RectModuleGroup);
    SetWindowPos(HwndGroup,HWND_NOTOPMOST,
                0,
                0,
                RectModuleGroup.right-RectModuleGroup.left,
                LeftOrTopSplitterPos-RectModuleGroup.top,
                SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::GetClientWindowRect(mhWndDialog,HwndGroup,&RectModuleGroup);
    CDialogHelper::Redraw(HwndGroup);
    
    // resize module listview
    hItem=mpCListviewModules->GetControlHandle();
    CDialogHelper::GetClientWindowRect(mhWndDialog,hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
                0,
                0,
                Rect.right-Rect.left,
                RectModuleGroup.bottom-Rect.top-SPACE_BETWEEN_CONTROLS,
                SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItem);

}