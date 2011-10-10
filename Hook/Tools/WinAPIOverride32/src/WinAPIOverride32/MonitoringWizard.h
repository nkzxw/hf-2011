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
// Object: manages the monitoring wizard dialog
//-----------------------------------------------------------------------------


#pragma once


#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "resource.h"
#include "../Tools/GUI/Dialog/DialogHelper.h"
#include "../Tools/GUI/Dialog/DialogSimple.h"
#include "../tools/GUI/Listview/Listview.h"
#include "../Tools/LinkList/LinkList.h"
#include "../Tools/File/TextFile.h"
#include "../Tools/File/StdFileOperations.h"
#include "../Tools/String/TrimString.h"
#include "../Tools/String/WildCharCompare.h"
#include "../Tools/Process/APIOverride/injected_dll/defines.h"
#include "../Tools/Com/GUIDStringConvert.h"

#define MONITORING_WIZARD_MONITORING_FILES_PATH _T("monitoring files\\")
#define MONITORING_WIZARD_MONITORING_COM_FILES_PATH _T("monitoring files\\COM\\")
#define MONITORING_WIZARD_MONITORING_FILES_DEFAULT_VALUES_PATH _T("monitoring files\\default\\")
#define MONITORING_WIZARD_MONITORING_COM_FILES_DEFAULT_VALUES_PATH _T("monitoring files\\default\\COM\\")
#define MONITORING_WIZARD_MONITORING_COM_FILES_INTREFACE_NAME _T("@InterfaceName=")

#define CMonitoringWizard_DIALOG_MIN_HEIGHT 400
#define CMonitoringWizard_DIALOG_MIN_WIDTH  900
#define SPACE_BETWEEN_CONTROLS 5

// ShowFilterDialog results
#define MONITORING_WIZARD_DLG_RES_FAILED 0
#define MONITORING_WIZARD_DLG_RES_OK     1    // user clicks OK
#define MONITORING_WIZARD_DLG_RES_CANCEL 2    // user clicks Cancel

#define MONITORING_WIZARD_DialogNewMonitoringFileName_EDIT_FILENAME 1000
#define MONITORING_WIZARD_DialogNewMonitoringFileName_STATIC_ENTER_FILENAME 1001

class CMonitoringWizard
{
public:
    enum tagMonitoringWizardType
    {
        MonitoringWizardType_API,
        MonitoringWizardType_COM
    };
private:
    UINT IdMenuListviewFileCreateMonitoringFile;
    UINT IdMenuListviewFileRenameMonitoringFile;
    UINT IdMenuListviewFileDeleteMonitoringFile;
    UINT IdMenuListviewFileEditMonitoringFile;
    typedef struct tagLogFileLine
    {
        DWORD dwLineNumber;
        TCHAR* pszContent;
        BOOL bStateEnabled;
    }LOGFILELINE,*PLOGFILELINE;

    CLinkList* pFileLines;      // link list containing functions descriptions and comments in the same order than the file
    CListview* pListviewFiles;  // listview containing monitoring files names
    CListview* pListviewFunctions;// listview containing functions description
    TCHAR pszMonitoringFilesPath[MAX_PATH];
    HWND hWndDialog;
    HWND hWndComboSearch;
    DWORD dwLastSelectedFileIndex;
    INT_PTR DialogResult;

    BOOL bClosed;
    BOOL bManualChangesDone;

    void Close();
    void Init();
    void OnSizing(RECT* pRect);
    void OnSize();

    void CommitChanges();
    void OkClick();
    void FreepFileLines();
    
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static void CallbackListViewFunctionSelectionStatic(int ItemIndex,int SubItemIndex,LPVOID UserParam);
    void CallbackListViewFunctionSelection(int ItemIndex);
    static void CallbackListViewFileSelectionStatic(int ItemIndex,int SubItemIndex,LPVOID UserParam);
    void CallbackListViewFileSelection(int ItemIndex);
    static BOOL CallBackReadLine(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber,LPVOID UserParam);
    void CallBackMonitoringFileReadLine(TCHAR* FileName,TCHAR* Line,DWORD dwLineNumber);
    static int CallbackSortingCompareCallback(TCHAR* String1,TCHAR* String2,CListview::SortingType DataSortingType,BOOL Ascending,LPVOID UserParam);
    int CallbackSortingCompareCallback(TCHAR* String1,TCHAR* String2,CListview::SortingType DataSortingType,BOOL Ascending);
    static void CallbackPopUpMenuItemClickStatic(UINT MenuID,LPVOID UserParam);
    void CallbackPopUpMenuItemClick(UINT MenuID);
     
    TCHAR* GetFunctionName(TCHAR* FunctionDefinition);
    void SearchFunction();
    void SelectSearchedFunctionInListView();
    BOOL CheckFunctionDescription(TCHAR* pszDescription);
    void SetFunctionDescription(TCHAR* pszDescription);
    void AddFunctionDescription(LOGFILELINE* pLog);
    void AddFunctionDescription();
    void ModifySelectedFunctionDescription();
    void RemoveSelectedFunctionDescription();
    void RestoreDefaultValues();
    BOOL GetAssociatedDefaultFile(OUT TCHAR* DefaultFile);
    void UpdateFunctionsList(TCHAR* FileFullPath);
    void FullEditAssociatedFile();
    LOGFILELINE* GetLogInfoFromSelectedFunction();
    void GetSelectedFilePath(TCHAR* pszFullPath);
    tagMonitoringWizardType MonitoringWizardType;
    void SetComDisplayFileName(TCHAR* FileNameWithoutExt,int ListViewItemIndex);

    TCHAR DialogNewMonitoringFileName_FileName[MAX_PATH];
    CDialogSimple::COMMANDS_CALLBACK_ARRAY_ITEM* pDialogNewMonitoringFileName_ArrayCommands;
    DWORD pDialogNewMonitoringFileName_ArrayCommandsSize;
    static void DialogNewMonitoringFileName_OnButtonCancelClick (CDialogSimple* pDialogSimple,PVOID UserParam);
    static void DialogNewMonitoringFileName_OnButtonOkClick (CDialogSimple* pDialogSimple,PVOID UserParam);
    static void DialogNewMonitoringFileName_OnCreate(CDialogSimple* pDialogSimple,PVOID UserParam);
    BOOL GetNewMonitoringFileName(TCHAR* OldFileName);

    COMBOBOXINFO ComboSearchInfos;
public:
    CLinkList* pFileToLoadList;// list of monitoring files that must be loaded
    CMonitoringWizard(tagMonitoringWizardType);
    ~CMonitoringWizard(void);
    INT_PTR ShowDialog(HINSTANCE hInstance,HWND hWndDialog);
};
