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
// Object: manages the monitoring file builder UI
//-----------------------------------------------------------------------------

#pragma once

#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "resource.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../Tools/GUI/ToolTip/ToolTip.h"
#include "../Tools/File/StdFileOperations.h"
#include "../Tools/Dll/DllStub.h"
#include "monitoringfilebuilder.h"
#include "WarningsReportUI.h"
#include "../Tools/DebugInfos/DebugInfosClasses/HasDebugInfos/HasDebugInfos.h"

#define CMONITORINGFILEBUILDERUI_INITIAL_LOG_SIZE 2048
#define DEBUG_INFOS_VIEWER_APPLICATION_NAME _T("DebugInfosViewer.exe")
class CMonitoringFileBuilderUI
{
private:
    HWND hWndDialog;
    HINSTANCE hInstance;

    HICON hIconCancel;
    HICON hIconWarningsAndErrors;
    CToolTip* pToolTipCancel;
    CToolTip* pToolWarningsAndErrors;
    BOOL bFullGeneration;
    BOOL bCheckOnly;
    TCHAR* pszLogContent;
    DWORD LogContentMaxSize;
    TCHAR* pszLogWarningsAndErrors;
    DWORD LogWarningsAndErrorsMaxSize;
    DWORD NbErrors;
    DWORD NbWarnings;
    CMonitoringFileBuilder* pMonitoringFileBuilder;
    HWND hProgressBar;
    void AddLogContent(TCHAR* pszMessage);
    void AddWarningAndErrorsContent(TCHAR* pszMessage);
    static void UserMessageInformationCallBackStatic(TCHAR* Message,tagUserMessagesTypes MessageType,LPVOID UserParam);
    void UserMessageInformationCallBack(TCHAR* Message,tagUserMessagesTypes MessageType);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void Close();
    void Init(HWND hwnd);
    void OnDropFile(HWND hWnd, HDROP hDrop);
    void SetInputFile(TCHAR* InputFile);

    void BrowseInputFile();
    void BrowseOutputFile();
    void BrowseOldMonitoringFile();
    void SetUIState(BOOL bStarted);
    void Generate(BOOL FullGeneration);
    static DWORD WINAPI GenerateThreadProc(LPVOID lpParameter);
    void CancelGenerate();
    void ClearLogs();
    BOOL IsErrorCheckingAllowed(BOOL ShowMsgIfNotAllowed);
    BOOL IsUpdateAllowed(BOOL ShowMsgIfNotAllowed);
    void UpdateCMonitoringFileBuilderOptionsFromUI(CMonitoringFileBuilder* pMonitoringFileBuilder);
    BOOL bUpdate;
    static void CallBackPercentCompleted(BYTE Percent,LPVOID UserParam);
    void CallBackPercentCompleted(BYTE Percent);
    enum UPDATE_WAY{UPDATE_FULL,UPDATE_FIRSTBYTES,UPDATE_REMOVEFIRSTBYTES};
    void EnableImportExportTableSpecificPanels(BOOL bEnable);
    
public:
    static void Show(HINSTANCE Instance,HWND hWndParent);
    CMonitoringFileBuilderUI();
    ~CMonitoringFileBuilderUI();
};
