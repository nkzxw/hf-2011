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
// Object: class helper for tree view control
//-----------------------------------------------------------------------------
#pragma once

#include "../../File/TextFile.h"
#include "../../File/StdFileOperations.h"
#include "../../String/WildCharCompare.h"
#include "../../Clipboard/Clipboard.h"
#include "../Menu/PopUpMenu.h"
#include "../../String/AnsiUnicodeConvert.h"

#include <windowsx.h>
#include <commctrl.h>

#define CTreeview_REPLACED_CHAR_ARRAY_SIZE 3
#define CTreeview_REPLACED_CHAR_MAX_INCREASE 5
#define CTreeview_TXT_FILE_SPLITTER _T("\t")
class CTreeview
{
public:

    typedef void (*tagPopUpMenuItemClickCallback)(UINT MenuID,LPVOID UserParam);
    typedef void (*tagCheckedStateChangedCallback)(HTREEITEM hItem,PVOID UserParam);
    typedef void (*tagSelectedItemChangedCallback)(LPNMTREEVIEW pChangeInfos,PVOID UserParam);

    enum tagCheckState
    {
        CHECK_STATE_CHECKED=2,
        CHECK_STATE_UNCHECKED=1,
        CHECK_STATE_NO_CHECK_BOX_IMAGE=0
    };

    CPopUpMenu* pPopUpMenu;

    BOOL Clear();
    BOOL Remove(HTREEITEM hItem);
    BOOL Expand(HTREEITEM hItem,BOOL FullBranch);
    BOOL Expand(HTREEITEM hItem);
    BOOL Expand();
    BOOL Collapse(HTREEITEM hItem,BOOL FullBranch);
    BOOL Collapse(HTREEITEM hItem);
    BOOL Collapse();

    HTREEITEM GetRoot();
    HTREEITEM GetParent(HTREEITEM hItem);
    HTREEITEM GetChild(HTREEITEM hItem);
    HTREEITEM GetPrevious(HTREEITEM hItem);
    HTREEITEM GetNext(HTREEITEM hItem);
    HTREEITEM Insert(HTREEITEM hParent,HTREEITEM hInsertAfter,TCHAR* Text,LPARAM UserParam);
    HTREEITEM Insert(HTREEITEM hParent,HTREEITEM hInsertAfter,TCHAR* Text);
    HTREEITEM Insert(HTREEITEM hParent,TCHAR* Text,LPARAM UserParam);
    HTREEITEM Insert(HTREEITEM hParent,TCHAR* Text);
    HWND GetControlHandle();
    CTreeview(HWND hWndTreeView);
    CTreeview(HWND hWndTreeView,BOOL HasCheckBoxes);
    ~CTreeview(void);

    void EnablePopUpMenu(BOOL bEnable);
    BOOL Save(BOOL bExpandedOnly,BOOL bSelectedOnly);
    BOOL Save(BOOL bExpandedOnly,BOOL bSelectedOnly,TCHAR* DefaultExtension);
    BOOL Save(TCHAR* pszFileName,BOOL bExpandedOnly,BOOL bSelectedOnly);
    DWORD GetItemTextLen(HTREEITEM hTreeItem);
    BOOL GetItemText(HTREEITEM hTreeItem,TCHAR* pszText,DWORD TextMaxSize);
    DWORD GetItemDepth(HTREEITEM hItem);
    HTREEITEM GetSelectedItem();
    BOOL SetSelectedItem(HTREEITEM hItem);
    BOOL GetItemUserData(HTREEITEM hTreeItem,OUT LPARAM* pUserData);
    BOOL SetBoldState(HTREEITEM hItem,BOOL bBold);
    BOOL IsChecked(HTREEITEM hItem);
    BOOL GetCheckedState(HTREEITEM hItem,tagCheckState* pCheckState);
    BOOL SetCheckedState(HTREEITEM hItem,tagCheckState CheckState);
    void SetPopUpMenuItemClickCallback(tagPopUpMenuItemClickCallback PopUpMenuItemClickCallback,LPVOID UserParam);
    void SetCheckedStateChangedCallback(tagCheckedStateChangedCallback CheckedStateChangedCallBack,LPVOID UserParam);
    void SetSelectedItemChangedCallback(tagSelectedItemChangedCallback SelectedItemChangedCallback,LPVOID UserParam);
    
    BOOL OnNotify(WPARAM wParam, LPARAM lParam);
    BOOL SetItemUserData(HTREEITEM hItem,LPVOID UserData);
    BOOL GetItemUserData(HTREEITEM hItem,LPVOID* pUserData);

    typedef BOOL (*pfParseCallBack)(HTREEITEM hItem,LPVOID UserParam); // return TRUE to continue parsing, FALSE to stop it
    BOOL Parse(pfParseCallBack ParseCallBack,LPVOID UserParam);
    BOOL Parse(HTREEITEM hItem,pfParseCallBack ParseCallBack,LPVOID UserParam);
    BOOL Parse(HTREEITEM hItem,DWORD MaxDepthFromItem,pfParseCallBack ParseCallBack,LPVOID UserParam);
    BOOL Parse(HTREEITEM hItem,DWORD MaxDepthFromItem,pfParseCallBack ParseCallBack,LPVOID UserParam,BOOL ReverseOrder);
    BOOL ParseSpecifiedDepth(DWORD SpecifiedDepthFromRoot,pfParseCallBack ParseCallBack,LPVOID UserParam);
    BOOL ParseSpecifiedDepth(HTREEITEM hItem,DWORD SpecifiedDepthFromRoot,pfParseCallBack ParseCallBack,LPVOID UserParam);
    BOOL ParseSpecifiedDepth(HTREEITEM hItem,DWORD SpecifiedDepthFromRoot,pfParseCallBack ParseCallBack,LPVOID UserParam,BOOL ReverseOrder);

    BOOL Find(TCHAR* Str,HTREEITEM* phFirstMatchingItem);
    BOOL Find(TCHAR* Str,DWORD MaxDepthFromRoot,HTREEITEM* phFirstMatchingItem);
    BOOL FindNext(TCHAR* Str,HTREEITEM hItem,HTREEITEM* phNextMatchingItem);
    BOOL FindNext(TCHAR* Str,HTREEITEM hItem,DWORD MaxDepthFromItem,BOOL AllowParentSearch,HTREEITEM* phNextMatchingItem);
    BOOL FindPrevious(TCHAR* Str,HTREEITEM hItem,HTREEITEM* phPreviousMatchingItem);
    BOOL FindPrevious(TCHAR* Str,HTREEITEM hItem,DWORD MaxDepthFromItem,BOOL AllowParentSearch,HTREEITEM* phPreviousMatchingItem);
    BOOL FindAtSpecifiedDepth(TCHAR* Str,DWORD DepthFromRoot,HTREEITEM* phFirstMatchingItem);
    BOOL FindNextAtSpecifiedDepth(TCHAR* Str,HTREEITEM hItem,DWORD DepthFromRoot,HTREEITEM* phNextMatchingItem);
    BOOL FindNextAtSpecifiedDepth(TCHAR* Str,HTREEITEM hItem,DWORD DepthFromRoot,BOOL AllowParentSearch,HTREEITEM* phNextMatchingItem);
    BOOL FindPreviousAtSpecifiedDepth(TCHAR* Str,HTREEITEM hItem,DWORD DepthFromRoot,HTREEITEM* phPreviousMatchingItem);
    BOOL FindPreviousAtSpecifiedDepth(TCHAR* Str,HTREEITEM hItem,DWORD DepthFromRoot,BOOL AllowParentSearch,HTREEITEM* phPreviousMatchingItem);


    BOOL SetItemText(HTREEITEM hItem,TCHAR* Text);
    BOOL SetItemIconIndex(HTREEITEM hItem,int IconIndex,int SelectedIconIndex);
    BOOL SetItemIconIndex(HTREEITEM hItem,int IconIndex);
    int AddIcon(HMODULE hModule,int IdIcon);
    int AddIcon(HICON hIcon);
    BOOL RemoveIcon(int IconIndex);
    BOOL RemoveAllIcons();

private:
    HWND hWndControl;
    HIMAGELIST hImgListNormal;

    enum tagSavingType
    {
        SAVING_TYPE_XML,
        SAVING_TYPE_HTML,
        SAVING_TYPE_TXT
    };
    tagSavingType SavingType;
    HANDLE hFile;

    tagCheckedStateChangedCallback CheckedStateChangedCallBack;
    LPVOID CheckedStateChangedCallBackUserParam;
    tagPopUpMenuItemClickCallback PopUpMenuItemClickCallback;
    LPVOID PopUpMenuItemClickCallbackUserParam;
    tagSelectedItemChangedCallback SelectedItemChangedCallback;
    LPVOID SelectedItemChangedCallbackUserParam;

    BOOL PopUpMenuEnabled;
    UINT MenuIdCopySelected;
    UINT MenuIdSaveExpanded;
    UINT MenuIdSaveSelected;
    UINT MenuIdSaveAll;

    void CommonConstructor();

    TCHAR pszFileName[MAX_PATH];
    DWORD GetTreeViewItemDepth(HTREEITEM hTreeItem);
    void ShowError(TCHAR* psz);
    BOOL WriteTreeViewItemHeader(DWORD ItemDepth);
    BOOL WriteTreeViewItemFooter();
    BOOL WriteTreeViewItemContent(HTREEITEM hTreeItem);
    BOOL SaveTreeViewItemContent(HTREEITEM hTreeItem,DWORD ItemDepth,BOOL bExpandedOnly);
    BOOL OpenFile(TCHAR* pszFileName);
    BOOL CloseFile(BOOL bSuccess);
    void StrReplace(TCHAR* pszInputString,TCHAR* pszOutputString,TCHAR* pszOldStr,TCHAR* pszNewStr);
    void XMLConvert(TCHAR* pszInputString,TCHAR* pszOutputString,BOOL bToXML);
    typedef struct tagFindStruct
    {
        CTreeview* pTreeView;
        TCHAR* Str;
        BOOL ItemFound;
        HTREEITEM hMatchingItem;
    }FIND_STRUCT,*PFIND_STRUCT;

    static BOOL FindCallBackStatic(HTREEITEM hItem,LPVOID UserParam);
    BOOL Parse(HTREEITEM hItem,DWORD MaxDepthFromItem,DWORD SpecifiedDepthFromRoot,pfParseCallBack ParseCallBack,LPVOID UserParam,BOOL ReverseOrder);
    BOOL FindCallBack(HTREEITEM hItem,FIND_STRUCT* pFindStruct);
    BOOL FindNext(TCHAR* Str,HTREEITEM hItem,DWORD MaxDepthFromItem,DWORD SpecifiedDepthFromRoot,BOOL AllowParentSearch,HTREEITEM* phNextMatchingItem);
    BOOL FindPrevious(TCHAR* Str,HTREEITEM hItem,DWORD MaxDepthFromItem,DWORD SpecifiedDepthFromRoot,BOOL AllowParentSearch,HTREEITEM* phPreviousMatchingItem);
    BOOL Find(TCHAR* Str,HTREEITEM hFromItem,DWORD MaxDepthFromItem,DWORD SpecifiedDepthFromRoot,BOOL ReverseOrder,BOOL AllowParentSearch,HTREEITEM* phNextMatchingItem);
};
