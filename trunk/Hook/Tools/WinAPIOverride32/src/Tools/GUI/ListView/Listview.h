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
// Object: class helper for listview control
//-----------------------------------------------------------------------------

#pragma once

// required lib : comctl32.lib
#pragma comment (lib,"comctl32")
// require manifest to make SetView work

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <malloc.h>
#include "..\menu\popupmenu.h"
#include "..\..\clipboard\clipboard.h"
#include "..\..\File\TextFile.h"

#define LIST_VIEW_TIMEOUT 5000
#define LIST_VIEW_COLUMN_DEFAULT_WIDTH 100
#define CLISTVIEW_DEFAULT_CUSTOM_DRAW_COLOR RGB(247,247,247) // (240,240,240)
#define CLISTVIEW_DEFAULT_HEAP_SIZE 4096
class CListview
{
public:
    typedef struct _COLUMN_INFO
    {
        TCHAR   pcName[MAX_PATH]; // column text
        int     iWidth;     // width of column in pixels
        int     iTextAlign; // LVCFMT_XX
    }COLUMN_INFO,*PCOLUMN_INFO;

    enum SortingType
    {
        SortingTypeString,
        SortingTypeNumber
    };

    CPopUpMenu* pPopUpMenu;

    typedef void (*tagSelectItemCallback)(int ItemIndex,int SubItemIndex,LPVOID UserParam);
    typedef void (*tagUnselectItemCallback)(int ItemIndex,int SubItemIndex,LPVOID UserParam);
    typedef void (*tagPopUpMenuItemClickCallback)(UINT MenuID,LPVOID UserParam);
    typedef int (*tagSortingCompareCallback)(TCHAR* String1,TCHAR* String2,CListview::SortingType DataSortingType,BOOL Ascending,LPVOID UserParam);

    CListview(HWND hWndListView);
    ~CListview();

    // Sets columns headers and some styles
    BOOL InitListViewColumns(int NbColumns,PCOLUMN_INFO pColumnInfo);

    // clear listview
    void Clear();

    // add item and sub items
    int AddItem(TCHAR* pcText);
    int AddItem(TCHAR* pcText,BOOL ScrollToItem);
    int AddItem(TCHAR* pcText,LPVOID UserData);
    int AddItem(TCHAR* pcText,LPVOID UserData,BOOL ScrollToItem);
    int AddItemAndSubItems(int NbItem,TCHAR** ppcText);
    int AddItemAndSubItems(int NbItem,TCHAR** ppcText,BOOL ScrollToItem);
    int AddItemAndSubItems(int NbItem,TCHAR** ppcText,BOOL ScrollToItem,LPVOID UserData);
    int AddItemAndSubItems(int NbItem,TCHAR** ppcText,int ItemIndex,BOOL ScrollToItem);
    int AddItemAndSubItems(int NbItem,TCHAR** ppcText,int ItemIndex,BOOL ScrollToItem,LPVOID UserData);
    BOOL RemoveItem(int ItemIndex);

    DWORD SetView(DWORD iView);
    void SetStyle(BOOL bFullRowSelect,BOOL bGridLines,BOOL bSendClickNotification,BOOL bCheckBoxes);
    void SetDefaultCustomDrawColor(COLORREF ColorRef);
    void EnableDefaultCustomDraw(BOOL bEnable);
    void EnablePopUpMenu(BOOL bEnable);
    void EnableColumnSorting(BOOL bEnable);
    void EnableColumnReOrdering(BOOL bEnable);

    int GetSelectedIndex();
    int GetSelectedCount();
    BOOL IsItemSelected(int ItemIndex);
    BOOL IsItemSelected(int ItemIndex,BOOL bSelectedIsNotChecked);
    void SetSelectedIndex(int ItemIndex);
    void SetSelectedIndex(int ItemIndex,BOOL bSetFocus);
    void SetSelectedState(int ItemIndex,BOOL bSelected);
    void SetSelectedState(int ItemIndex,BOOL bSelected,BOOL bSetFocus);
    void SelectAll();
    void UnselectAll();
    void SetAllItemsSelectedState(BOOL bSelected);
    void UncheckSelected();
    void CheckSelected();
    void InvertSelection();

    int GetItemCount();
    int GetColumnCount();
    HWND FORCEINLINE GetControlHandle(){return this->hWndListView;}
    HWND FORCEINLINE GetHeader(){return ListView_GetHeader(this->hWndListView);}

    BOOL SetColumnName(int ColumnIndex,TCHAR* Name);
    BOOL SetColumn(int ColumnIndex,TCHAR* Name,int iWidth,int iTextAlign);
    int AddColumn(TCHAR* Name,int iWidth,int iTextAlign,int ColumnIndex);
    int AddColumn(TCHAR* Name,int iWidth,int iTextAlign);
    BOOL RemoveColumn(int ColumnIndex);
    BOOL RemoveAllColumns();
    BOOL IsColumnVisible(int ColumnIndex);
    void HideColumn(int ColumnIndex);
    void ShowColumn(int ColumnIndex);
    void ShowColumn(int ColumnIndex,int Size);
    BOOL FORCEINLINE SetColumnWidth(int ColumnIndex,int Size){return ListView_SetColumnWidth(this->hWndListView, ColumnIndex,Size);}
    int  FORCEINLINE GetColumnWidth(int ColumnIndex){return ListView_GetColumnWidth(this->hWndListView, ColumnIndex);}
    
    void ShowColumnHeaders(BOOL bShow);

    void SetTransparentBackGround();

    void GetItemText(int ItemIndex,TCHAR* pcText,int pcTextMaxSize);
    void GetItemText(int ItemIndex,int SubItemIndex,TCHAR* pcText,int pcTextMaxSize);
    DWORD GetItemTextLen(int ItemIndex,int SubItemIndex);
    DWORD GetItemTextLenMax(BOOL bOnlySelected);
    DWORD GetItemTextLenMaxForColumn(DWORD ColumnIndex, BOOL bOnlySelected);
    void SetItemText(int ItemIndex,TCHAR* pcText);
    void SetItemText(int ItemIndex,int SubItemIndex,TCHAR* pcText);

    void ScrollTo(int ItemIndex);

    BOOL OnNotify(WPARAM wParam, LPARAM lParam);
    void SetSelectItemCallback(tagSelectItemCallback SelectItemCallback,LPVOID UserParam);
    void SetUnselectItemCallback(tagUnselectItemCallback UnselectItemCallback,LPVOID UserParam);
    void SetSelectionCallbackState(BOOL bEnable);
    void SetPopUpMenuItemClickCallback(tagPopUpMenuItemClickCallback PopUpMenuItemClickCallback,LPVOID UserParam);
    void ShowClearPopupMenu(BOOL bVisible);

    BOOL SetWatermarkImage(HBITMAP hBmp);// can be used for stretched background too

    int Find(TCHAR* Text);
    int Find(TCHAR* Text,BOOL bPartial);
    int Find(TCHAR* Text,int StartIndex,BOOL bPartial);

    void Sort();
    void Sort(int ColumnIndex);
    void Sort(int ColumnIndex,BOOL bAscending);
    void ReSort();
    void SetSortingType(CListview::SortingType type);
    void SetSortingCompareCallback(tagSortingCompareCallback SortingCompareCallback,LPVOID UserParam);

    BOOL SetItemUserData(int ItemIndex,LPVOID UserData);
    BOOL GetItemUserData(int ItemIndex,LPVOID* pUserData);

    

    enum tagSavingType
    {
        SavingTypeXML,
        SavingTypeHTML,
        SavingTypeCSV,
        SavingTypeTXT
    };
    typedef void (*pfSpecializedSavingFunction)(HANDLE SavingHandle,int ItemIndex,int SubItemIndex,LPVOID UserParam);
    void SaveContentForSpecializedSavingFunction(HANDLE SavingHandle,TCHAR* pszContent);
    // fSpecializedSavingFunction calls SaveContentForSpecializedSavingFunction like
    //void fSpecializedSavingFunction(HANDLE SavingHandle,int ItemIndex,int SubItemIndex,LPVOID UserParam)
    //{
    //    TCHAR* MyContent=GetMyContent();
    //    SaveContentForSpecializedSavingFunction(SavingHandle,MyContent);
    //}


    BOOL Save();
    BOOL Save(BOOL bOnlySelected);
    BOOL Save(BOOL bOnlySelected,BOOL bOnlyVisibleColumns);
    BOOL Save(BOOL bOnlySelected,BOOL bOnlyVisibleColumns,TCHAR* DefaultExtension);
    BOOL Save(BOOL bOnlySelected,BOOL bOnlyVisibleColumns,TCHAR* DefaultExtension,pfSpecializedSavingFunction SpecializedSavingFunction,LPVOID UserParam);
    BOOL Save(TCHAR* pszFileName);
    BOOL Save(TCHAR* pszFileName,BOOL bOnlySelected);
    BOOL Save(TCHAR* pszFileName,BOOL bOnlySelected,BOOL bOnlyVisibleColumns);
    BOOL Save(TCHAR* pszFileName,BOOL bOnlySelected,BOOL bOnlyVisibleColumns,pfSpecializedSavingFunction SpecializedSavingFunction,LPVOID UserParam);

    BOOL Load();
    BOOL Load(TCHAR* pszFileName);

    BOOL CopyToClipBoard();
    BOOL CopyToClipBoard(BOOL bOnlySelected);
    BOOL CopyToClipBoard(BOOL bOnlySelected,BOOL bOnlyVisibleColumns);

    enum tagImageLists
    {
        ImageListNormal = LVSIL_NORMAL, // Image list with large icons.
        ImageListSmall = LVSIL_SMALL, // Image list with small icons.
        ImageListState = LVSIL_STATE, // Image list with state images.
        // ImageListGroupHeader = LVSIL_GROUPHEADER, // Image list for group header.
        ImageListColumns
    };
    BOOL SetColumnIconIndex(int ColumnIndex,int IconIndexInColumnIconList);
    BOOL SetItemIconIndex(int ItemIndex,int IconIndex);
    int AddIcon(tagImageLists ImageList,HMODULE hModule,int IdIcon);
    int AddIcon(tagImageLists ImageList,HICON hIcon);
    BOOL RemoveIcon(tagImageLists ImageList,int IconIndex);
    BOOL RemoveAllIcons(tagImageLists ImageList);
private:
    class CListviewItemParam // small class to provide user an associated value like Item.lparam
    {
    public:
        PVOID UserParam;// allow the user to have a PVOID LPARAM associated with item
                        // all other fields are for private use 
        DWORD ItemKey;
        CListviewItemParam(DWORD ItemKey,LPVOID UserParam);
        ~CListviewItemParam();
    };

    HIMAGELIST hImgListNormal;
    HIMAGELIST hImgListSmall;
    HIMAGELIST hImgListState;
    // HIMAGELIST hImgListGroupHeader;
    HIMAGELIST hImgListColumns;

    UINT MenuIdCopySelected;
    UINT MenuIdCopyAll;
    UINT MenuIdSaveSelected;
    UINT MenuIdSaveAll;
    UINT MenuIdClear;

    HANDLE HeapListViewItemParams;
    
    HWND hWndListView;// handle to listview
    HWND hWndParent;// handle to parent window
    // TCHAR replacement table for xml like loading saving
    TCHAR** ppszReplacedChar;
    // lock var
    HANDLE hevtUnlocked;
    // sorting vars
    BOOL bSortAscending;
    int iLastSortedColumn;
    DWORD ItemKey;

    BOOL PopUpMenuEnabled;
    BOOL ColumnSortingEnabled;
    BOOL DefaultCustomDrawEnabled;
    COLORREF DefaultCustomDrawColor;
    LRESULT ProcessDefaultCustomDraw (LPARAM lParam);
   
    tagSelectItemCallback SelectItemCallback;
    tagUnselectItemCallback UnselectItemCallback;
    BOOL bSelectionCallBackEnable;
    tagPopUpMenuItemClickCallback PopUpMenuItemClickCallback;
    LPVOID PopUpMenuItemClickCallbackUserParam;
    LPVOID UnselectItemCallbackUserParam;
    LPVOID SelectItemCallbackUserParam;
    tagSortingCompareCallback SortingCompareCallback;
    LPVOID SortingCompareCallbackUserParam;

    TCHAR** pSortArray;

    void FreeCListviewItemParam(BOOL NeedToReallocMemory);
    static int CALLBACK Compare(LPARAM lParam1, LPARAM lParam2,LPARAM lParamSort);
    void StrReplace(TCHAR* pszInputString,TCHAR* pszOutputString,TCHAR* pszOldStr,TCHAR* pszNewStr);
    void Convert(TCHAR* pszInputString,TCHAR* pszOutputString,BOOL bToXML);
    SortingType stSortingType;

    static void SpecializedSavingFunctionStatic(HANDLE SavingHandle,int ItemIndex,int SubItemIndex,LPVOID UserParam);
    void SpecializedSavingFunction(HANDLE SavingHandle,int ItemIndex,int SubItemIndex);

    BOOL IsCheckedListView();
    int GetCheckedCount();
    
    typedef struct tagSavingHandle
    {
        tagSavingType SavingType;
        HANDLE hFile;
    }SAVING_HANDLE,*PSAVING_HANDLE;
    
};
