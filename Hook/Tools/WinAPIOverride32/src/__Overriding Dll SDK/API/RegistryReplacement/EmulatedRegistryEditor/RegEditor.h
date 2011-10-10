/*
Copyright (C) 2010 Jacquelin POTIER <jacquelin.potier@free.fr>
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
#include <windowsx.h>

#include "../Common_Files/RegistryCommonFunctions.h"
#include "../Common_Files/EmulatedRegistry.h"
#include "resource.h"

// ! REQUIRE FULL WINAPIOVERRIDE SOURCES TO COMPILE !
// sources are available at http://jacquelin.potier.free.fr/winapioverride32/
#include "../../../../../Tools/gui/Dialog/DialogBase.h"
#include "../../../../../Tools/gui/ListView/Listview.h"
#include "../../../../../Tools/gui/TreeView/Treeview.h"
#include "../../../../../Tools/gui/StatusBar/Statusbar.h"
#include "../../../../../Tools/gui/ToolBar/Toolbar.h"
#include "../../../../../Tools/string/StrToHex.h"
#include "../../../../../Tools/string/AnsiUnicodeConvert.h"


class CRegEditorGUI:public CDialogBase
{
public:
    CRegEditorGUI();
    ~CRegEditorGUI();
protected:

    CToolbar* pToolbar;
    CListview* pListviewValues;
    CTreeview* pTreeViewKeys;
    CStatusbar* pStatusBar;
    EmulatedRegistry::CEmulatedRegistry* pRegistry;
    HWND hWndComboValueType;

    TCHAR CurrentRegistryFileName[MAX_PATH];
    BOOL bChangesSinceLastSaving;

    int TreeViewIconIndexRegistry;
    int TreeViewIconIndexEmulatedKey;
    int TreeViewIconIndexNotEmulatedKey;

    int ListViewIconIndexRegistryBinary;
    int ListViewIconIndexRegistryString;

    virtual void OnInit();
    virtual void OnClose();
    virtual void OnCommand(WPARAM wParam,LPARAM lParam);
    virtual void OnNotify(WPARAM wParam,LPARAM lParam);
    virtual void OnDropFiles(WPARAM wParam,LPARAM lParam);
    static void OnSelectedKeyChange(LPNMTREEVIEW pChangeInfos,PVOID UserParam);
    void OnSelectedKeyChange(LPNMTREEVIEW pChangeInfos);
    static void OnSelectedValueChange(int ItemIndex,int SubItemIndex,LPVOID UserParam);
    void OnSelectedValueChange(int ItemIndex,int SubItemIndex);
    static BOOL TreeSubKeyParsingToUpdateEmulatedState(HTREEITEM hItem,LPVOID UserParam);
    BOOL TreeSubKeyParsingToUpdateEmulatedState(HTREEITEM hItem);
    void OnModifyKey();
    void OnAddKey();
    void OnRemoveSelectedKey();
    void OnModifyValue();
    void OnAddValue();
    void OnRemoveSelectedValues();
    void OnLoad();
    void OnSave();
    void OnSaveAs();
    void OnCopyKeyPath();
    void CheckSaving();

    BOOL LoadRegistry(TCHAR* const FileName);
    BOOL SaveRegistry(TCHAR* const FileName);
    void EnableKeyEdition(BOOL bEnable);
    void EnableEditing(BOOL bEnable);
    BOOL QuerySavingFileName(TCHAR* FileName);
    void ShowKeyValues(EmulatedRegistry::CKeyReplace* pKey);
    void ShowKeyContent(HTREEITEM hTreeItem,EmulatedRegistry::CKeyReplace* pKey);
    void UpdateKeyIcon(HTREEITEM hTreeItemKey,EmulatedRegistry::CKeyReplace* pKey);
    EmulatedRegistry::CKeyReplace* GetSelectedKey();
    EmulatedRegistry::CKeyValue* GetSelectedValue();
    void UpdateKeyValue(int ListViewItemIndex,EmulatedRegistry::CKeyValue* pValue);
    void SetUIFromValueContent(EmulatedRegistry::CKeyValue* pValue);
    void SetValueContentFromUI(EmulatedRegistry::CKeyValue* pValue);
    TCHAR* TypeToString(DWORD Type);
    BOOL GetStringValue(EmulatedRegistry::CKeyValue* pValue,OUT TCHAR** pStr,OUT BOOL* pbAnsiString);
    LRESULT ProcessCustomDraw (LPARAM lParam);
};