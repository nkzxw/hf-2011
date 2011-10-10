#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <stdio.h>
#include <Tlhelp32.h>
#include <shlobj.h>
#include "resource.h"

#include "RawDump.h"
#include "MemoryUserInterface.h"
#include "KernelMemoryUserInterface.h"
#include "ThreadCallStack.h"
#include "KernelProcessesInfo.h"
#include "ModuleIntegrityChecking.h"
#include "MemoryIntegrityReport.h"
#include "windowthreadprocessiduserinterface.h"

#include "../Tools/Gui/ListView/Listview.h"
#include "../Tools/Gui/Menu/PopUpMenu.h"
#include "../Tools/APIError/APIError.h"
#include "../Tools/Process/Memory/ProcessMemory.h"
#include "../Tools/Process/Dump/DumpToExe.h"
#include "../Tools/Process/Dump/Dump.h"
#include "../Tools/Process/ProcessAndThreadID/ProcessAndThreadID.h"
#include "../Tools/Process/ModulesInfos/ModulesInfos.h"
#include "../Tools/Process/ProcessHelper/ProcessHelper.h"
#include "../Tools/Process/x86CrossCompatibility/x86CrossCompatibility.h"
#include "../Tools/Process/InjLib/InjLib.h"
#include "../Tools/Gui/Dialog/DialogHelper.h"
#include "../Tools/Privilege/Privilege.h"
#include "../Tools/string/ansiunicodeconvert.h"
#include "../Tools/Thread/ThreadContext.h"
#include "../Tools/gui/ToolBar/Toolbar.h"
#include "../Tools/gui/Splitter/Splitter.h"
#include "../Tools/File/StdFileOperations.h"
#include "../Tools/gui/HtmlViewer/HtmlViewerWindow.h"
#include "../Tools/string/StringConverter.h"
#include "../Tools/GUI/Rebar/Rebar.h"



//////////////////////// defines //////////////////////// 
#define APP_MIN_WIDTH 500
#define APP_MIN_HEIGHT 590
#define SPACE_BETWEEN_CONTROLS 5
#define MIN_PROCESSES_GROUP_HEIGHT 50
#define MIN_MODULES_GROUP_HEIGHT 20
#define MIN_THREADS_GROUP_HEIGHT 60
#define ORIGINAL_PERCENT_THREADS_GROUP 40
#define ORIGINAL_PERCENT_MODULES_GROUP 40
#define WM_SHOW_INTEGRITY_SUCCESSFULLY_CHECKED_MSG WM_USER+1
#define CHECK_INTEGRITY_MinMatchingSizeAfterReplacement 10

//////////////////////// functions //////////////////////// 
LRESULT CALLBACK DumperWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ContextRegistersWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void ProcessSelectionCallBack(int ItemIndex, int SubitemIndex,LPVOID UserParam);
void ProcessesPopUpCallBack(UINT MenuID,LPVOID UserParam);
void ModulesPopUpCallBack(UINT MenuID,LPVOID UserParam);
void ThreadsPopUpCallBack(UINT MenuID,LPVOID UserParam);
void ProcessesDropDownMenuCallBack(CPopUpMenu* PopUpMenu,UINT MenuId,PVOID UserParam);
void ThreadsDropDownMenuCallBack(CPopUpMenu* PopUpMenu,UINT MenuId,PVOID UserParam);
void RefreshKernelModules();
void RefreshProcesses();
void RefreshModules(int ItemIndex);
void RefreshThreads(int ItemIndex);
void Refresh();
BOOL CheckIfProcessIsAlive(DWORD dwProcessId);
BOOL IsThreadAlive(DWORD dwThreadId);
void Dump();
void Resize();
void CheckSize(RECT* pWinRect);
void SwitchMode();

void InjectLibrary();
void EjectLibrary();
void ProcessIntegrityChecking();
void ModuleIntegrityChecking(int ModuleIndex);
CModuleIntegrityChecking* ModuleIntegrityCheck(int ItemIndex);

DWORD GetProcessIdFromIndex(int ProcessListViewItemIndex);
DWORD GetSelectedProcessId();
BOOL SetSelectedProcessId(DWORD ProcessId);
PBYTE GetModuleBaseAddress(int ModuleIndex);
DWORD GetSelectedModuleIndex();

void OnModulesThreadsSplitterMove(int LeftOrTopSplitterPos,int RightOrBottomSplitterPos,PVOID UserParam);
void OnModulesThreadsSplitterCollapsedStateChange(BOOL NewCollapsedState,PVOID UserParam);
void OnProcessesModulesSplitterMove(int LeftOrTopSplitterPos,int RightOrBottomSplitterPos,PVOID UserParam);
void OnProcessesModulesSplitterCollapsedStateChange(BOOL NewCollapsedState,PVOID UserParam);

void CloseSubDialogs();
void RegisterSubDialog(HWND hDlg);
void UnregisterSubDialog(HWND hDlg);