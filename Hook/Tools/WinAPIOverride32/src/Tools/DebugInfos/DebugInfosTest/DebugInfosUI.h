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
// Object: manages the DebugInfos dialog
//-----------------------------------------------------------------------------


#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif
#include <windows.h>

#include "resource.h"
#include "about.h"
#include "../DebugInfosClasses/DebugInfos.h"
#include "UserTypesGenerator.h"

#include "../../../Tools/GUI/Dialog/DialogHelper.h"
#include "../../../tools/GUI/Treeview/Treeview.h"
#include "../../../tools/GUI/ToolBar/Toolbar.h"
#include "../../../tools/String/StringConverter.h"
#include "../../../tools/String/WildCharCompare.h"
#include "../../../tools/String/StringReplace.h"
#include "../../../tools/File/TextFile.h"
#include "../../../tools/pe/PE.h"
#include "../../../tools/Process/APIOverride/injected_dll/defines.h" // only for OPCODE_REPLACEMENT_SIZE to do a checking on function length
#include "../../../Tools/Disasm/Disasm.h"
#include "../../../Tools/GUI/HtmlViewer/HtmlViewerWindow.h"


#define APPLICATION_NAME             _T("Debug Infos Viewer")
#define MONITORING_FILES_PATH        _T("monitoring files\\") // same as winapioverride
#define USER_TYPES_FILES_PATH        _T("UserTypes\\") // same as winapioverride
#define EXE_INTERNAL_RVA             _T("EXE_INTERNAL_RVA@") // according to winapioverride
#define DLL_INTERNAL                 _T("DLL_INTERNAL@")// according to winapioverride
#define SPLITTER                     _T("|")

#define CDebugInfosUI_DIALOG_MIN_WIDTH 400
#define CDebugInfosUI_DIALOG_MIN_HEIGHT 400



class CDebugInfosUI
{
private:
    BOOL NetWarningReported;
    HWND hWndDialog;
    INT_PTR DialogResult;
    HINSTANCE hInstance;
    HANDLE hParentHandle;
    CTreeview* pTreeview;
    CPopUpMenu* pMenuExport;
    UINT MenuExportExpandedOnlyId;
    UINT MenuExportSelectedOnlyId;
    CToolbar* pToolbar;
    CDebugInfos* pDebugInfos;// debug infos of current type
    HANDLE hMonitoringFile;
    TCHAR* pszFileToParseAtStartup;
    TCHAR UserTypePath[MAX_PATH];
    enum tagFileToMonitorType
    {
        FileToMonitorType_EXE,
        FileToMonitorType_DLL
    };
    tagFileToMonitorType FileToMonitorType;

    enum tagQueryCheckState
    {
        QueryCheckState_CHECK_ALL,
        QueryCheckState_CHECK_PROJECT_SPECIFIC,
        QueryCheckState_CHECK_UNCHECK_ALL
    };
    tagQueryCheckState QueryCheckState;

    CPE* pPE;
    CTypesGeneratedManager TypesGenerated;

    typedef struct tagDiasmCallBackParam
    {
        CLinkListTemplate<CDebugInfosSourcesInfos>* pFunctionSourcesInfos;
        CHtmlViewerWindow* pHtmlViewerWindow;
        ULONGLONG ImageBase;
        CDisasm::tagDecodeType DecodeType;
    }DIASM_CALLBACK_PARAM,*PDIASM_CALLBACK_PARAM;

    typedef struct tagThreadedDiasmCallBackParam
    {
        DIASM_CALLBACK_PARAM* pDiasmCallBackParam;
        ULONGLONG InstructionAddress;
        TCHAR* InstructionHex;
        TCHAR* Mnemonic;
        TCHAR* Operands;
    }THREADED_DIASM_CALLBACK_PARAM,*PTHREADED_DIASM_CALLBACK_PARAM;

    int IconObjIndex;
    int IconFunctionIndex;
    int IconParamIndex;
    int IconLocalIndex;
    int IconThunkIndex;

    CFunctionInfos* pFunctionInfosFindInTreeByAddress;

    void Init();
    void Close();
    void OnSize();
    void OnSizing(RECT* pRect);
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void OnGenerate();
    void OnCancel();
    static void OnTreeViewCheckedStateChangedStatic(HTREEITEM hTreeItemCheckedChanged,PVOID UserParam);
    void OnTreeViewCheckedStateChanged(HTREEITEM hTreeItemCheckedChanged);
    void OnCheckProjectSpecific();
    void OnCheckAll();
    void OnUncheckAll();
    static BOOL CheckCallBackStatic(HTREEITEM hItem,LPVOID UserParam);
    void CheckCallBack(HTREEITEM hItem);

    void OnCollapseModules();
    void OnExpandModules();
    void OnAbout();
    void OnDonation();
    void OnDropFile(HWND hWnd, HDROP hDrop);
    static DWORD WINAPI OnShowFunctionDisasm(LPVOID UserParam);
    void ShowFunctionDisasm();
    static void __stdcall AsmHtmlElementEventsCallBack(DISPID dispidMember,WCHAR* ElementId,LPVOID UserParam);
    static BOOL DisasmInstructionCallBack(ULONGLONG InstructionAddress,DWORD InstructionsSize,TCHAR* InstructionHex,TCHAR* Mnemonic,TCHAR* Operands,PVOID UserParam);
    static DWORD ThreadedDisasmInstructionCallBack(PVOID UserParam);

    BOOL GenerateMonitoringFile();
    void GenerateUserTypesInfosForFunction(CFunctionInfos* pFunctionInfos);
    static BOOL CollapseModulesParseCallBackStatic(HTREEITEM hItem,LPVOID UserParam);
    static BOOL ExpandModulesParseCallBackStatic(HTREEITEM hItem,LPVOID UserParam);
    void CollapseExpandParseCallBack(HTREEITEM hItem,BOOL bCollapse);
    static BOOL GenerateMonitoringParseCallBackStatic(HTREEITEM hItem,LPVOID UserParam);
    void GenerateMonitoringParseCallBack(HTREEITEM hItem);
    static BOOL FindInTreeByAddressParseCallBackStatic(HTREEITEM hItem,LPVOID UserParam);
    BOOL FindInTreeByAddressParseCallBack(HTREEITEM hItem);
    void ReportError(TCHAR* Msg);
    void Find();
    void FindPrevious();
    void FindNext();
    void NoItemFoundMessage();
    void SetWaitCursor(BOOL bSet);
    void FreeMemory();
    
    BOOL DisplayLocation(HTREEITEM hTreeItemParent,CSymbolLocation* pSymbolLocation);
    void DisplayStaticLocation(HTREEITEM hTreeItemParent,DWORD SectionIndex,DWORD Offset,ULONGLONG RelativeVirtualAddress);
    void InsertBlocks(HTREEITEM hTreeItemParent,CLinkListSimple* pLinkListBlocks);
    void InsertLabels(HTREEITEM hTreeItemParent,CLinkListSimple* pLinkListLabels);
    void InsertThunks(HTREEITEM hTreeItemParent,CLinkListSimple* pLinkListThunks);
    void InsertDebugInfos();
    void VariantToString(VARIANT var,TCHAR* psz,DWORD pszMaxSize);

    static void ToolBarDropDownMenuCallBackStatic(CPopUpMenu* PopUpMenu,UINT MenuId,PVOID UserParam);
    void ToolBarDropDownMenuCallBack(CPopUpMenu* PopUpMenu,UINT MenuId);
    
    BOOL OpenFile();
    BOOL OpenFile(TCHAR* FileName);
    BOOL CloseFile();

    void GenerateUserDataType(IDiaSymbol* pSymbol);
public:
    CDebugInfosUI(void);
    ~CDebugInfosUI(void);
    static INT_PTR Show(HINSTANCE hInstance,HWND hWndDialog);
    static INT_PTR Show(HINSTANCE hInstance,HWND hWndDialog,TCHAR* pszFile);
};