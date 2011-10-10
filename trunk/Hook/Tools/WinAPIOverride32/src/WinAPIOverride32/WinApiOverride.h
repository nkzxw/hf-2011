#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include "options.h"
#include "../tools/GUI/ListView/ListView.h"
#include "../Tools/Gui/Menu/PopUpMenu.h"
#include "../tools/Process/APIOverride/ApiOverride.h"


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK FiltersWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CallBackToolBarDropDownMenu(CPopUpMenu* PopUpMenu,UINT MenuId,PVOID UserParam);
void CallBackProcessCreation(HANDLE hParentId,HANDLE hProcessId);
void CallBackSplitterConfigCollapsedStateChange(BOOL NewCollapsedState,PVOID UserParam);
void BrowseApplicationPath();
void BrowseMonitoringFile();
void BrowseFakingFile();
void BrowseNotLoggedModuleList();
BOOL StartStop();
BOOL StartStop(BOOL Start);
void ThreadedStartStop();
DWORD WINAPI StartStopThreadStartRoutine(LPVOID lpParameter);
DWORD WINAPI Stop(LPVOID lpParameter);
void StartStopMonitoring();
BOOL StartStopMonitoring(BOOL Start);
void StartStopFaking();
BOOL StartStopFaking(BOOL Start);
void LoadMonitoringFile();
BOOL LoadMonitoringFile(TCHAR* pszFile);
void UnloadMonitoringFile();
DWORD WINAPI UnloadMonitoringFileThread(LPVOID lpParameter);
void LoadOverridingFile();
BOOL LoadOverridingFile(TCHAR* pszFile);
void UnloadOverridingFile();
DWORD WINAPI UnloadOverridingFileThread(LPVOID lpParameter);
void STDMETHODCALLTYPE CallBackBeforeAppResume(DWORD dwProcessID,PVOID pUserParam);
void STDMETHODCALLTYPE CallBackUnexpectedUnload(DWORD dwProcessID,PVOID pUserParam);
void STDMETHODCALLTYPE CallBackOverridingDllQuery(TCHAR* PluginName,PVOID MessageId,PBYTE pMsg,SIZE_T MsgSize,PVOID pUserParam);
void EnableMonitoringFakingInterface(BOOL bEnable);
void ClearLogs(BOOL bWaitEndOfClearing);
void LoadLogs();
BOOL LoadLogs(TCHAR* LogName,BOOL bWaitEndOfLoading);
void SaveLogs();
BOOL SaveLogs(TCHAR* LogName,BOOL bWaitEndOfSaving);
void RemoveSelectedLogs();
void RemoveAllLogsEntries();
void About();
void Init();
BOOL ParseCommandLine();
void FreeMemoryAllocatedByCommandLineParsing();
DWORD WINAPI Exit(LPVOID lpParam);
void OnMouseDown(LPARAM lParam);
void OnMouseUp(LPARAM lParam);
void OnMouseMove(LPARAM lParam);
DWORD WINAPI GetRemoteWindowInfos(PVOID lParam);
void SetApplicationsFilters();
void SetModulesFilters();
void LogOnlyBaseModule();
BOOL LogOnlyBaseModule(BOOL OnlyBaseModule);
void LogUseModuleList();
void SetMonitoringFiltersState();
void SetFakingFiltersState();
void CallAnalysis();
void ShowOptions();
void Dump();
LRESULT ProcessCustomDraw (LPARAM lParam);
LRESULT ProcessCustomDrawListViewDetails (LPARAM lParam);
void ReportMessage(TCHAR* pszMsg, tagMsgTypes MsgType);
void ReportMessage(CApiOverride* pApiOverride,TCHAR* pszMsg, tagMsgTypes MsgType);
void ReportMessage(CApiOverride* pApiOverride,TCHAR* pszMsg, tagMsgTypes MsgType,FILETIME Time);
void STDMETHODCALLTYPE CallbackMonitoringLog(LOG_ENTRY* pLog,PVOID pUserParam);
void STDMETHODCALLTYPE CallBackReportMessage(tagReportMessageType ReportMessageType,TCHAR* pszReportMessage,FILETIME FileTime,LPVOID UserParam);
void ShowMSDNOnlineHelp(CListview* pListview);
void GoogleFuncName(CListview* pListview);
void CopyFuncName(CListview* pListview);
void ShowLastErrorMessage(CListview* pListview);
void GetUnrebasedVirtualAddress(CListview* pListview);
void ListviewPopUpMenuItemClickCallback(UINT MenuID,LPVOID UserParam);
void ListviewDetailsPopUpMenuItemClickCallback(UINT MenuID,LPVOID UserParam);
void CallBackLogListviewItemSelection(int ItemIndex,int SubItemIndex,LPVOID UserParam);
void MonitoringWizard();
DWORD WINAPI MonitoringWizard(LPVOID lpParameter);
void CompareLogs();
void EditModulesFiltersList();
void Help();
void MakeDonation();
void MonitoringFileGenerator();
void ReloadMonitoring();
DWORD WINAPI ReloadMonitoring(LPVOID lpParameter);
void ReloadOverriding();
DWORD WINAPI ReloadOverriding(LPVOID lpParameter);
void UpdateModulesFiltersList();
void SaveMonitoringListContent();
void SaveOverridingListContent();
void FindNextFailure();
void FindPreviousFailure();
DWORD WINAPI DetailListviewUpdater(LPVOID lpParameter);
void UpdateDetailListView(int ItemIndex);
LRESULT CALLBACK KeyboardProc (int code, WPARAM wParam, LPARAM lParam);
void ListviewKeyDown(WORD vKey);
void ShowReturnErrorMessage(CListview* pListview);
void SetWaitCursor(BOOL bSet);
BOOL ApplyGeneralOptions(CApiOverride* pApiOverride,COptions* Options);
BOOL SetBeforeStartOptions(CApiOverride* pAPIOverride);
BOOL SetBeforeStartOptions(CApiOverride* pApiOverride,COptions* Options);
BOOL SetAfterStartOptions(CApiOverride* pApiOverride);
BOOL SetAfterStartOptions(CApiOverride* pApiOverride,COptions* Options);
BOOL GetOptionsFromGUI();
BOOL SetGUIFromOptions();
void LoadCommandLineMonitoringAndFakingFiles();
COptions::tagStartWay GetStartWay();
void OnDropFile(HWND hWnd, HDROP hDrop);
void StartStopCOMHooking();
BOOL StartStopCOMHooking(BOOL Start);
void StartStopNETHooking();
BOOL StartStopNETHooking(BOOL Start);
DWORD WINAPI RemoveAllLogsEntriesThread(LPVOID lpParam);
DWORD WINAPI SaveLogsThread(LPVOID lpParam);
CApiOverride* GetApiOverrideObject();
void ListViewSpecializedSavingFunction(HANDLE SavingHandle,int ItemIndex,int SubItemIndex,LPVOID UserParam);
void CheckForUpdate();
BOOL CheckForUpdate(BOOL* pbNewVersionIsAvailable,WCHAR** ppDownloadLink);
BOOL CheckForUpdate(BOOL bDisplayDialog,BOOL* pbNewVersionIsAvailable);
void ReportBug();
void SupportedParametersReportError(IN TCHAR* const ErrorMessage,IN PBYTE const UserParam);
BOOL StopApiOverride(CApiOverride* pApiOverride);
COptions* GetOptions();
BOOL DestroyApiOverride(CApiOverride* pApiOverride);
BOOL UpdateDetailListView(LOG_LIST_ENTRY* pLogEntry);
BOOL LoadCurrentMonitoringFiles(CApiOverride* pApiOverride);
BOOL LoadCurrentOverridingFiles(CApiOverride* pApiOverride);
void ReportHookedProcess(CApiOverride* pApiOverride,DWORD ProcessId,BOOL bSuccess);
void ReportUnhookedProcess(CApiOverride* pApiOverride,BOOL bSuccess);
BOOL UnloadMonitoringFileAndRemoveFromGui(TCHAR* FileName);
BOOL UnloadOverridingFileAndRemoveFromGui(TCHAR* FileName);