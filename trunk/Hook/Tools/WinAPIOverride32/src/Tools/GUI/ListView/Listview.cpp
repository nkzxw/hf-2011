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

#include "listview.h"

#pragma message (__FILE__ " Information : Require the ComCtl32.dll version 6 and requires a manifest that specifies that version 6 of the dynamic-link library (DLL) must be used to make SetView method work\r\n")

#define CListview_REPLACED_CHAR_ARRAY_SIZE 3
#define CListview_REPLACED_CHAR_MAX_INCREASE 5
TCHAR* pppszReplacedChar[CListview_REPLACED_CHAR_ARRAY_SIZE][2]={{_T("&"),_T("&amp;")},{_T("<"),_T("&lt;")},{_T(">"),_T("&gt;")}};

//-----------------------------------------------------------------------------
// Name: CListviewItemParam
// Object: CListviewItemParam Constructor.
// Parameters :
//     in  : DWORD ItemKey : item key requiered for sorting
//           LPVOID UserParam : user data
//     out :
//     return : 
//-----------------------------------------------------------------------------
CListview::CListviewItemParam::CListviewItemParam(DWORD ItemKey,LPVOID UserParam)
{
    this->ItemKey=ItemKey;
    this->UserParam=UserParam;
}
CListview::CListviewItemParam::~CListviewItemParam()
{
}

//-----------------------------------------------------------------------------
// Name: CListview
// Object: Constructor. Assume that the common control DLL is loaded.
// Parameters :
//     in  : HWND hWndListView : handle to listview
//     out :
//     return : 
//-----------------------------------------------------------------------------
CListview::CListview(HWND hWndListView)
{
    this->hWndListView=hWndListView;
    this->hWndParent=GetParent(this->hWndListView);

    // Ensure that the common control DLL is loaded.
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // set lock for multi threading access
    this->hevtUnlocked=CreateEvent(NULL,FALSE,TRUE,NULL);

    // set sorting vars
    this->bSortAscending=TRUE;
    this->iLastSortedColumn=-1;
    this->ItemKey=0;
    this->stSortingType=CListview::SortingTypeString;

    this->bSelectionCallBackEnable=TRUE;
    this->SelectItemCallback=NULL;
    this->UnselectItemCallback=NULL;
    this->PopUpMenuItemClickCallback=NULL;
    this->PopUpMenuItemClickCallbackUserParam=NULL;
    this->SortingCompareCallback=NULL;
    this->UnselectItemCallbackUserParam = NULL;
    this->SelectItemCallbackUserParam = NULL;
    this->SortingCompareCallbackUserParam = NULL;
    this->pSortArray = NULL;
    this->ppszReplacedChar = NULL;

    this->pPopUpMenu=new CPopUpMenu();

    this->MenuIdCopySelected=this->pPopUpMenu->Add(_T("Copy Selected"));
    this->MenuIdCopyAll=this->pPopUpMenu->Add(_T("Copy All"));
    this->MenuIdSaveSelected=this->pPopUpMenu->Add(_T("Save Selected"));
    this->MenuIdSaveAll=this->pPopUpMenu->Add(_T("Save All"));
    this->MenuIdClear=(UINT)(-1);

    this->PopUpMenuEnabled=TRUE;
    this->ColumnSortingEnabled=TRUE;
    this->DefaultCustomDrawEnabled=TRUE;
    this->DefaultCustomDrawColor=CLISTVIEW_DEFAULT_CUSTOM_DRAW_COLOR;

    this->HeapListViewItemParams=HeapCreate(0,CLISTVIEW_DEFAULT_HEAP_SIZE,0);

    this->hImgListNormal=NULL;
    this->hImgListSmall=NULL;
    this->hImgListState=NULL;
    this->hImgListColumns=NULL;

}
CListview::~CListview()
{
    this->PopUpMenuItemClickCallback=NULL;
    this->SelectItemCallback=NULL;
    this->UnselectItemCallback=NULL;

    this->FreeCListviewItemParam(FALSE);
    CloseHandle(this->hevtUnlocked);
    delete this->pPopUpMenu;

    if (this->hImgListNormal)
        ImageList_Destroy(this->hImgListNormal);
    if (this->hImgListSmall)
        ImageList_Destroy(this->hImgListSmall);
    if (this->hImgListState)
        ImageList_Destroy(this->hImgListState);
    if (this->hImgListColumns)
        ImageList_Destroy(this->hImgListColumns);
}

//-----------------------------------------------------------------------------
// Name: FreeCListviewItemParam
// Object: remove all CListviewItemParam associated with items
//         ListView MUST BE LOCKED WHEN CALLING THIS FUNCTION
// Parameters :
//     in  : BOOL NeedToReallocMemory : TRUE if items param are going to be used again
//                                      FALSE if no more items param are going to be used (should be FALSE only in destructor)
//     out :
//     return : HWND handle to the control
//-----------------------------------------------------------------------------
void CListview::FreeCListviewItemParam(BOOL NeedToReallocMemory)
{

    // slow way
    //LV_ITEM Item={0};
    //CListviewItemParam* pListviewItemParam;
    //Item.mask=LVIF_PARAM;

    //int NbItems=this->GetItemCount();
    //for (int ItemIndex=0;ItemIndex<NbItems;ItemIndex++)
    //{
    //    Item.iItem=ItemIndex;
    //    // try to get our own object associated with item
    //    if (!ListView_GetItem(this->hWndListView,&Item))
    //        return;
    //    // check if object is valid
    //    pListviewItemParam=(CListviewItemParam*)Item.lParam;
    //    if (IsBadReadPtr(pListviewItemParam,sizeof(CListviewItemParam)))
    //        return;

    //    delete pListviewItemParam;
    //}

    // high speed way (MSDN : you don't need to destroy allocated memory by calling HeapFree before calling HeapDestroy)
    HeapDestroy(this->HeapListViewItemParams);
    if (NeedToReallocMemory)
        this->HeapListViewItemParams=HeapCreate(0,CLISTVIEW_DEFAULT_HEAP_SIZE,0);
}
//-----------------------------------------------------------------------------
// Name: Clear
// Object: Clear the listview.
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::Clear()
{
    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);
    this->FreeCListviewItemParam(TRUE);
    ListView_DeleteAllItems(this->hWndListView);
    this->ItemKey=0;
    SetEvent(this->hevtUnlocked);
}

//-----------------------------------------------------------------------------
// Name: InitListViewColumns
// Object: Sets columns headers and some styles.
// Parameters :
//     in  : 
//     out :
//     return : TRUE if successful,FALSE on error
//-----------------------------------------------------------------------------
BOOL CListview::InitListViewColumns(int NbColumns,PCOLUMN_INFO pColumnInfo)
{ 
    int iCol; 

    // remove columns
    this->RemoveAllColumns();

    // Add the columns. 
    for (iCol = 0; iCol < NbColumns; iCol++) 
	{ 
        if (!this->SetColumn(iCol,pColumnInfo[iCol].pcName,pColumnInfo[iCol].iWidth,pColumnInfo[iCol].iTextAlign))
            return FALSE;
    } 
    return TRUE; 
}


//-----------------------------------------------------------------------------
// Name: SetView
// Object: Set listview view style. works only for XP and upper OS
//          require use of commctls 6 or upper
// Parameters :
//     in  : DWORD iView : new view to apply
//     out :
//     return : TRUE if successful,FALSE on error
//-----------------------------------------------------------------------------
DWORD CListview::SetView(DWORD iView)
{
    return ListView_SetView(this->hWndListView,iView);
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add items at the end of listview
// Parameters :
//     in  : TCHAR* pcText : text string to add
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItem(TCHAR* pcText)
{
    return this->AddItemAndSubItems(1,&pcText);
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add items at the end of listview
// Parameters :
//     in  : TCHAR* pcText : text string to add
//           BOOL ScrollToItem : TRUE to scroll to added item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItem(TCHAR* pcText,BOOL ScrollToItem)
{
    return this->AddItem(pcText,NULL,ScrollToItem);
}
//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add items at the end of listview
// Parameters :
//     in  : TCHAR* pcText : text string to add
//           LPVOID UserData : User data to associate to item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItem(TCHAR* pcText,LPVOID UserData)
{
    return this->AddItem(pcText,UserData,FALSE);
}
//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add items at the end of listview
// Parameters :
//     in  : TCHAR* pcText : text string to add
//           LPVOID UserData : User data to associate to item
//           BOOL ScrollToItem : TRUE to scroll to added item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItem(TCHAR* pcText,LPVOID UserData,BOOL ScrollToItem)
{
    return this->AddItemAndSubItems(1,&pcText,this->GetItemCount(),ScrollToItem,UserData);
}

//-----------------------------------------------------------------------------
// Name: AddItemAndSubItems
// Object: Add items and sub items contains in array of string ppcText at the 
//          end of listview
// Parameters :
//     in  : int NbItem : number of string in ppcText
//           TCHAR** ppcText : array of Item/SubItems string 
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItemAndSubItems(int NbItem,TCHAR** ppcText)
{
    return this->AddItemAndSubItems(NbItem,ppcText,FALSE);
}

//-----------------------------------------------------------------------------
// Name: AddItemAndSubItems
// Object: Add items and sub items contains in array of string ppcText at the 
//          end of listview
// Parameters :
//     in  : int NbItem : number of string in ppcText
//           TCHAR** ppcText : array of Item/SubItems string 
//           BOOL ScrollToItem : TRUE to scroll to inserted item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItemAndSubItems(int NbItem,TCHAR** ppcText,BOOL ScrollToItem)
{
    return this->AddItemAndSubItems(NbItem,ppcText,ScrollToItem,(LPVOID)NULL);
}

//-----------------------------------------------------------------------------
// Name: AddItemAndSubItems
// Object: Add items and sub items contains in array of string ppcText at the 
//          end of listview
// Parameters :
//     in  : int NbItem : number of string in ppcText
//           TCHAR** ppcText : array of Item/SubItems string 
//           BOOL ScrollToItem : TRUE to scroll to inserted item
//           LPVOID UserData : User data to associate to item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItemAndSubItems(int NbItem,TCHAR** ppcText,BOOL ScrollToItem,LPVOID UserData)
{
    return this->AddItemAndSubItems(NbItem,ppcText,this->GetItemCount(),ScrollToItem,UserData);
}

//-----------------------------------------------------------------------------
// Name: AddItemAndSubItems
// Object: Add items and sub items contains in array of string ppcText at  
//          position specified by ItemIndex
// Parameters :
//     in  : int NbItem : number of string in ppcText
//           TCHAR** ppcText : array of Item/SubItems string 
//           int ItemIndex : position in listview to and the new row
//           BOOL ScrollToItem : TRUE to scroll to inserted item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItemAndSubItems(int NbItem,TCHAR** ppcText,int ItemIndex,BOOL ScrollToItem)
{
    return this->AddItemAndSubItems(NbItem,ppcText,ItemIndex,ScrollToItem,(LPVOID)NULL);
}

//-----------------------------------------------------------------------------
// Name: AddItemAndSubItems
// Object: Add items and sub items contains in array of string ppcText at  
//          position specified by ItemIndex
// Parameters :
//     in  : int NbItem : number of string in ppcText
//           TCHAR** ppcText : array of Item/SubItems string 
//           int ItemIndex : position in listview to and the new row
//           BOOL ScrollToItem : TRUE to scroll to inserted item
//           LPVOID UserData : User data to associate to item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItemAndSubItems(int NbItem,TCHAR** ppcText,int ItemIndex,BOOL ScrollToItem,LPVOID UserData)
{
    LVITEM lvI;
    CListviewItemParam* pListviewItemParam;
    int SubItemIndex;

    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);
    
    memset(&lvI,0,sizeof(LVITEM));
    lvI.mask = LVIF_TEXT|LVIF_PARAM; 
    lvI.iItem=ItemIndex;
	lvI.pszText =ppcText[0];
    // get a unique ID for parameter sorting
    pListviewItemParam=(CListviewItemParam*)HeapAlloc(this->HeapListViewItemParams,0,sizeof(CListviewItemParam));
    pListviewItemParam->ItemKey=this->ItemKey;
    pListviewItemParam->UserParam=UserData;
    lvI.lParam=(LPARAM)pListviewItemParam;
    this->ItemKey++;
    ItemIndex=ListView_InsertItem(this->hWndListView, &lvI);
    if (ItemIndex<0)
        return -1;
    for (SubItemIndex=1;SubItemIndex<NbItem;SubItemIndex++)
    {
        ListView_SetItemText(
                            this->hWndListView,
                            ItemIndex,
                            SubItemIndex,
                            ppcText[SubItemIndex]
                            );
    }

    if (ScrollToItem)
        this->ScrollTo(ItemIndex);

    SetEvent(this->hevtUnlocked);
    return ItemIndex;
}

//-----------------------------------------------------------------------------
// Name: ScrollTo
// Object: scroll to item index
// Parameters :
//     in  : int ItemIndex : index of item
//     out :
//     return :
//-----------------------------------------------------------------------------
void CListview::ScrollTo(int ItemIndex)
{
    ListView_EnsureVisible(this->hWndListView,ItemIndex,FALSE);
}

//-----------------------------------------------------------------------------
// Name: RemoveItem
// Object: remove an item from list
// Parameters :
//     in  : int ItemIndex : index of item to remove
//     out :
//     return : TRUE if successful,FALSE on error
//-----------------------------------------------------------------------------
BOOL CListview::RemoveItem(int ItemIndex)
{
    LV_ITEM Item={0};
    CListviewItemParam* pListviewItemParam;
    Item.mask=LVIF_PARAM;
    Item.iItem=ItemIndex;
    if (ListView_GetItem(this->hWndListView,&Item))
    {
        pListviewItemParam=(CListviewItemParam*)Item.lParam;
        if (pListviewItemParam)
        {
            // delete pListviewItemParam;
            HeapFree(this->HeapListViewItemParams,0,pListviewItemParam);
            pListviewItemParam=NULL;
        }
    }
    return ListView_DeleteItem(this->hWndListView,ItemIndex);
}

//-----------------------------------------------------------------------------
// Name: GetSelectedIndex
// Object: GetFirst selected item index 
//          ONLY FOR NON CHECKED LISTVIEW STYLE
// Parameters :
//     in  :
//     out :
//     return : first selected item index, or -1 if no selected item
//-----------------------------------------------------------------------------
int CListview::GetSelectedIndex()
{
    for (int cnt=0;cnt<this->GetItemCount();cnt++)
    {
        if (ListView_GetItemState(this->hWndListView,cnt,LVIS_SELECTED)==LVIS_SELECTED)
            return cnt;
    }
    return -1;
}

//-----------------------------------------------------------------------------
// Name: SetSelectedIndex
// Object: Select item at the specified index  
// Parameters :
//     in  : int ItemIndex : index of item to select
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetSelectedIndex(int ItemIndex)
{
    this->SetSelectedIndex(ItemIndex,FALSE);
}
//-----------------------------------------------------------------------------
// Name: SetSelectedIndex
// Object: Select item at the specified index  
// Parameters :
//     in  : int ItemIndex : index of item to select
//           BOOL ScrollToItem : TRUE to scroll to item and make item visible
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetSelectedIndex(int ItemIndex,BOOL ScrollToItem)
{
    ListView_SetItemState (this->hWndListView,
                          ItemIndex,
                          LVIS_FOCUSED | LVIS_SELECTED, // item state
                          0x000F);                      // mask 
    if (ScrollToItem)
        this->ScrollTo(ItemIndex);
}


//-----------------------------------------------------------------------------
// Name: IsItemSelected
// Object: Check if item at the specified index is selected.
// Parameters :
//     in  : int ItemIndex : index of item to check
//     out :
//     return : TRUE if item is checked for checked listview, TRUE if item is selected for 
//              non checked listview
//-----------------------------------------------------------------------------
BOOL CListview::IsItemSelected(int ItemIndex)
{
    if (this->IsCheckedListView())
        return ListView_GetCheckState(this->hWndListView,ItemIndex);
    // else

        return (ListView_GetItemState(
            this->hWndListView,
            ItemIndex,
            LVIS_SELECTED)==LVIS_SELECTED);
}

//-----------------------------------------------------------------------------
// Name: IsItemSelected
// Object: Check if item at the specified index is selected
// Parameters :
//     in  : int ItemIndex : index of item to check
//           BOOL bSelectedIsNotChecked 
//     out :
//     return : TRUE if item is selected
//-----------------------------------------------------------------------------
BOOL CListview::IsItemSelected(int ItemIndex,BOOL bSelectedIsNotChecked)
{
    if (!bSelectedIsNotChecked)
        return this->IsItemSelected(ItemIndex);
    // else
        return (ListView_GetItemState(
            this->hWndListView,
            ItemIndex,
            LVIS_SELECTED)==LVIS_SELECTED);
}

//-----------------------------------------------------------------------------
// Name: SetSelectedState
// Object: set item at the specified index to a selected state
// Parameters :
//     in  : int ItemIndex : index of item to check
//           BOOL bSelected : TRUE to select, FALSE to unselect
//     out :
//     return : TRUE if item is selected
//-----------------------------------------------------------------------------
void CListview::SetSelectedState(int ItemIndex,BOOL bSelected)
{
    this->SetSelectedState(ItemIndex,bSelected,TRUE);
}


//-----------------------------------------------------------------------------
// Name: SetSelectedState
// Object: set item at the specified index to a selected state
// Parameters :
//     in  : int ItemIndex : index of item to check
//           BOOL bSelected : TRUE to select, FALSE to unselect
//           BOOL bSetFocus : TRUE if focus must be set (must be at least one for selecting in non checked mode)
//     out :
//     return : TRUE if item is selected
//-----------------------------------------------------------------------------
void CListview::SetSelectedState(int ItemIndex,BOOL bSelected,BOOL bSetFocus)
{
    if (this->IsCheckedListView())
    {
        ListView_SetCheckState(this->hWndListView,ItemIndex,bSelected);
    }
    else
    {
        if (bSetFocus)
            SetFocus(this->hWndListView);

        UINT state;
        if (bSelected)
            state=LVIS_SELECTED;
        else
            state=0;
        ListView_SetItemState (this->hWndListView,
                          ItemIndex,
                          state,    // item state
                          0x000F);  // mask 

    }
}


//-----------------------------------------------------------------------------
// Name: SelectAll
// Object: Select all items
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::SelectAll()
{
    this->SetAllItemsSelectedState(TRUE);
}

//-----------------------------------------------------------------------------
// Name: UnselectAll
// Object: Unselect all items
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::UnselectAll()
{
    this->SetAllItemsSelectedState(FALSE);
}

//-----------------------------------------------------------------------------
// Name: SetAllItemState
// Object: set the state of all items to selected or unselected
// Parameters :
//     in  : BOOL bSelected : TRUE to select, FALSE to unselect
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetAllItemsSelectedState(BOOL bSelected)
{
    int ItemCount;

    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);

    // get number of items
    ItemCount=this->GetItemCount();

    // set focus to listview to avoid to do it each time an item is selected
    SetFocus(this->hWndListView);

    // for each item
    for (int cnt=0;cnt<ItemCount;cnt++)
        this->SetSelectedState(cnt,bSelected,FALSE);

    SetEvent(this->hevtUnlocked);
}

//-----------------------------------------------------------------------------
// Name: UncheckSelected
// Object: Uncheck all selected items. Available only for checked list view
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::UncheckSelected()
{
    if (!this->IsCheckedListView())
        return;

    int ItemCount;

    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);

    // get number of items
    ItemCount=this->GetItemCount();

    // set focus to listview to avoid to do it each time an item is selected
    SetFocus(this->hWndListView);

    // for each item
    for (int cnt=0;cnt<ItemCount;cnt++)
    {
        if (this->IsItemSelected(cnt,TRUE))
            this->SetSelectedState(cnt,FALSE,FALSE);
    }

    SetEvent(this->hevtUnlocked);
}
//-----------------------------------------------------------------------------
// Name: CheckSelected
// Object: check all selected items. Available only for checked list view
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::CheckSelected()
{
    if (!this->IsCheckedListView())
        return;

    int ItemCount;

    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);

    // get number of items
    ItemCount=this->GetItemCount();

    // set focus to listview to avoid to do it each time an item is selected
    SetFocus(this->hWndListView);

    // for each item
    for (int cnt=0;cnt<ItemCount;cnt++)
    {
        if (this->IsItemSelected(cnt,TRUE))
            this->SetSelectedState(cnt,TRUE,FALSE);
    }

    SetEvent(this->hevtUnlocked);
}

//-----------------------------------------------------------------------------
// Name: GetSelectedCount
// Object: return number of selected items
// Parameters :
//     in  : 
//     out :
//     return : number of selected items
//-----------------------------------------------------------------------------
int CListview::GetSelectedCount()
{
    if (this->IsCheckedListView())
        return this->GetCheckedCount();
    return ListView_GetSelectedCount(this->hWndListView);
}
//-----------------------------------------------------------------------------
// Name: GetCheckedCount
// Object: return number of selected items
//          WORK ONLY FOR LISTVIEW WITH CHECKBOXES STYLE
// Parameters :
//     in  : 
//     out :
//     return : number of selected items
//-----------------------------------------------------------------------------
int CListview::GetCheckedCount()
{
    int NbChecked=0;
    int ItemCount;

    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);

    // get number of items
    ItemCount=this->GetItemCount();

    // for each item
    for (int cnt=0;cnt<ItemCount;cnt++)
        // if item is checked
        if (ListView_GetCheckState(this->hWndListView,cnt))
            // increase NbChecked
            NbChecked++;

    SetEvent(this->hevtUnlocked);

    return NbChecked;
}

//-----------------------------------------------------------------------------
// Name: IsCheckedListView
// Object: allow to know if listview has checked state
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if checked listview, FALSE else
//-----------------------------------------------------------------------------
BOOL CListview::IsCheckedListView()
{
    return (ListView_GetExtendedListViewStyle(this->hWndListView)&LVS_EX_CHECKBOXES);
}

//-----------------------------------------------------------------------------
// Name: GetItemText
// Object: get item text
// Parameters :
//     in  : int ItemIndex : index of item
//           int pcTextMaxSize : max buffer size for pcText
//     out : TCHAR* pcText : item text
//     return : 
//-----------------------------------------------------------------------------
void CListview::GetItemText(int ItemIndex,TCHAR* pcText,int pcTextMaxSize)
{
    this->GetItemText(
                        ItemIndex,
                        0,
                        pcText,
                        pcTextMaxSize
                        );
}
//-----------------------------------------------------------------------------
// Name: GetItemText
// Object: get item text
// Parameters :
//     in  : int ItemIndex : item index
//           int SubItemIndex : sub item index
//           int pcTextMaxSize : max buffer size for pcText
//     out : TCHAR* pcText : subitem text
//     return : 
//-----------------------------------------------------------------------------
void CListview::GetItemText(int ItemIndex,int SubItemIndex,TCHAR* pcText,int pcTextMaxSize)
{
    *pcText=0;
    ListView_GetItemText(
    this->hWndListView,
    ItemIndex,
    SubItemIndex,
    pcText,
    pcTextMaxSize
    );
}
//-----------------------------------------------------------------------------
// Name: GetItemTextLen
// Object: get item text len in TCHAR including \0
// Parameters :
//     in  : int ItemIndex : item index
//           int SubItemIndex : sub item index
//     out : 
//     return : size of text in TCHAR including \0
//-----------------------------------------------------------------------------
DWORD CListview::GetItemTextLen(int ItemIndex,int SubItemIndex)
{
    LVITEM lvi;
    TCHAR* psz;
    DWORD pszSize=MAX_PATH*2;
    DWORD CopySize=pszSize;

    lvi.iSubItem=SubItemIndex;
    
    // do a loop increasing size
    // as soon as size of text copied to lvi.pszText is smaller than buffer size
    // we got the real size
    while ((CopySize==pszSize)||(CopySize==pszSize-1))
    {
        pszSize*=2;
        psz=new TCHAR[pszSize];
        lvi.cchTextMax=pszSize;
        lvi.pszText=psz;
        CopySize=(DWORD)SendMessage(this->hWndListView,LVM_GETITEMTEXT,ItemIndex,(LPARAM)&lvi);
        delete[] psz;
    }

    return CopySize+1;
}

//-----------------------------------------------------------------------------
// Name: GetItemTextLenMaxForColumn
// Object: get max text len of all items and subitems in the listview
// Parameters :
//     in  : DWORD ColumnIndex 
//           BOOL bOnlySelected : if we ignore unselected items
//     out : 
//     return : max size of text in TCHAR including \0 of all item of column in the list
//-----------------------------------------------------------------------------
DWORD CListview::GetItemTextLenMaxForColumn(DWORD ColumnIndex, BOOL bOnlySelected)
{
    int iNbRow=this->GetItemCount();
    int iNbColumn=this->GetColumnCount();

    if (iNbColumn<0)
        iNbColumn=1;

    // get max text len
    DWORD MaxTextLen=0;
    DWORD TextLen=0;
    // for each row
    for (int cnt=0;cnt<iNbRow;cnt++)
    {
        // if we save only selected items
        if (bOnlySelected)
        {
            // if item is not selected
            if (!this->IsItemSelected(cnt,TRUE))
                continue;
        }

        TextLen=this->GetItemTextLen(cnt,ColumnIndex);
        if (MaxTextLen<=TextLen)
            MaxTextLen=TextLen;
    }

    return MaxTextLen;
}

//-----------------------------------------------------------------------------
// Name: GetItemTextLenMax
// Object: get max text len of all items and subitems in the listview
// Parameters :
//     in  : BOOL bOnlySelected : if we ignore unselected items
//     out : 
//     return : max size of text in TCHAR including \0 of all item and subitems in the list
//-----------------------------------------------------------------------------
DWORD CListview::GetItemTextLenMax(BOOL bOnlySelected)
{
    int iNbRow=this->GetItemCount();
    int iNbColumn=this->GetColumnCount();

    if (iNbColumn<0)
        iNbColumn=1;

    // get max text len
    DWORD MaxTextLen=0;
    DWORD TextLen=0;
    // for each row
    for (int cnt=0;cnt<iNbRow;cnt++)
    {
        // if we save only selected items
        if (bOnlySelected)
        {
            // if item is not selected
            if (!this->IsItemSelected(cnt,TRUE))
                continue;
        }

        // for each column
        for (int cnt2=0;cnt2<iNbColumn;cnt2++)
        {
            TextLen=this->GetItemTextLen(cnt,cnt2);
            if (MaxTextLen<=TextLen)
                MaxTextLen=TextLen;
            
        }
    }

    return MaxTextLen;
}

//-----------------------------------------------------------------------------
// Name: SetColumnName
// Object: set column name
// Parameters :
//     in  : int ColumnIndex : column index
//           TCHAR* Name : column text
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CListview::SetColumnName(int ColumnIndex,TCHAR* Name)
{
    LVCOLUMN lvc; 
    if (this->GetColumnCount()<=ColumnIndex)
        return this->AddColumn(Name,100,LVCFMT_LEFT,ColumnIndex);

    lvc.mask = LVCF_TEXT;	  
    lvc.pszText = Name;

    return ListView_SetColumn(this->hWndListView, ColumnIndex,&lvc);
}
//-----------------------------------------------------------------------------
// Name: SetColumn
// Object: set column name
// Parameters :
//     in  : int ColumnIndex : column index
//           TCHAR* Name : column text
//           int iWidth : width of column in pixels
//           int iTextAlign : LVCFMT_XX
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CListview::SetColumn(int ColumnIndex,TCHAR* Name,int iWidth,int iTextAlign)
{
    LVCOLUMN lvc; 
    if (this->GetColumnCount()<=ColumnIndex)
        return this->AddColumn(Name,iWidth,iTextAlign,ColumnIndex);

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;	  
    lvc.pszText = Name;
    lvc.cx = iWidth;
    lvc.fmt =iTextAlign;
    
    return ListView_SetColumn(this->hWndListView, ColumnIndex,&lvc);
}

//-----------------------------------------------------------------------------
// Name: AddColumn
// Object: set column name
// Parameters :
//     in  : int ColumnIndex : column index
//           TCHAR* Name : column text
//           int iWidth : width of column in pixels
//           int iTextAlign : LVCFMT_XX
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CListview::AddColumn(TCHAR* Name,int iWidth,int iTextAlign,int ColumnIndex)
{
    LVCOLUMN lvc; 

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;	  

    lvc.pszText = Name;
    lvc.cx = iWidth;
    lvc.fmt =iTextAlign;

    return ListView_InsertColumn(this->hWndListView, ColumnIndex,&lvc);
}

//-----------------------------------------------------------------------------
// Name: AddColumn
// Object: set column name
// Parameters :
//     in  : TCHAR* Name : column text
//           int iWidth : width of column in pixels
//           int iTextAlign : LVCFMT_XX
//     out : 
//     return : ColumnIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddColumn(TCHAR* Name,int iWidth,int iTextAlign)
{
    int iCol=this->GetColumnCount();
    if (iCol==-1)
        iCol++;
    return this->AddColumn(Name,iWidth,iTextAlign,iCol);
}

//-----------------------------------------------------------------------------
// Name: RemoveColumn
// Object: Remove column. 
//          Notice : column 0 can't be removed, so only it's name and it's content will be free
// Parameters :
//     in  : int ColumnIndex : 0 based column index
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CListview::RemoveColumn(int ColumnIndex)
{
    if (ColumnIndex==0)
    {
        this->Clear();
        return this->SetColumnName(0,_T(""));
    }
    return ListView_DeleteColumn(this->hWndListView,ColumnIndex);
}

//-----------------------------------------------------------------------------
// Name: RemoveAllColumns
// Object: Remove all columns
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CListview::RemoveAllColumns()
{
    this->Clear();
    int NbColumns=this->GetColumnCount();
    this->SetColumnName(0,_T(""));
    for (int cnt=NbColumns-1;cnt>=1;cnt--)
        this->RemoveColumn(cnt);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: HideColumn
// Object: hide specified column
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::HideColumn(int ColumnIndex)
{
    ListView_SetColumnWidth(this->hWndListView, ColumnIndex,0);
}

//-----------------------------------------------------------------------------
// Name: ShowColumn
// Object: show specified column
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::ShowColumn(int ColumnIndex)
{
    this->ShowColumn(ColumnIndex,LIST_VIEW_COLUMN_DEFAULT_WIDTH);
}
//-----------------------------------------------------------------------------
// Name: ShowColumn
// Object: show specified column
// Parameters :
//     in  : int Size : size of visible column
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::ShowColumn(int ColumnIndex,int Size)
{
    ListView_SetColumnWidth(this->hWndListView, ColumnIndex,Size);
}

//-----------------------------------------------------------------------------
// Name: IsColumnVisible
// Object: check if specified column is visible
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if visible, FALSE else
//-----------------------------------------------------------------------------
BOOL CListview::IsColumnVisible(int ColumnIndex)
{
    return (ListView_GetColumnWidth(this->hWndListView,ColumnIndex)!=0);
}

//-----------------------------------------------------------------------------
// Name: SetItemText
// Object: add item if required and set item text
// Parameters :
//     in  : int ItemIndex : item index
//           TCHAR* pcText : item text
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetItemText(int ItemIndex,TCHAR* pcText)
{
    this->SetItemText(
    ItemIndex,
    0,
    pcText
    );
}
//-----------------------------------------------------------------------------
// Name: SetItemText
// Object: add item if required and set item text
// Parameters :
//     in  : int ItemIndex : item index
//           int SubItemIndex subitem index
//           TCHAR* pcText : subitem text
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetItemText(int ItemIndex,int SubItemIndex,TCHAR* pcText)
{
    LVITEM lvI;
    memset(&lvI,0,sizeof(LVITEM));
    lvI.iItem=ItemIndex;
    TCHAR* Array[1];

    // if item don't exist
    if (!ListView_GetItem(this->hWndListView,&lvI))
    {
        Array[0]=_T("");
        // create it
        this->AddItemAndSubItems(1,Array,ItemIndex,FALSE);
    }

    ListView_SetItemText(
    this->hWndListView,
    ItemIndex,
    SubItemIndex,
    pcText
    );
}

//-----------------------------------------------------------------------------
// Name: GetItemCount
// Object: get number of items
// Parameters :
//     in  : 
//     out : 
//     return : number of items
//-----------------------------------------------------------------------------
int CListview::GetItemCount()
{
    return ListView_GetItemCount(this->hWndListView);
}
//-----------------------------------------------------------------------------
// Name: GetColumnCount
// Object: get number of columns
// Parameters :
//     in  : 
//     out : 
//     return : number of columns
//-----------------------------------------------------------------------------
int CListview::GetColumnCount()
{
    return Header_GetItemCount(ListView_GetHeader(this->hWndListView));
}

//-----------------------------------------------------------------------------
// Name: SetStyle
// Object: set style helper
// Parameters :
//     in  : BOOL bFullRowSelect : true for full row select
//           BOOL bGridLines : true for gridLines
//           BOOL bSendClickNotification : true for sending click notifications
//           BOOL bCheckBoxes : TRUE for check box style listview
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetStyle(BOOL bFullRowSelect,BOOL bGridLines,BOOL bSendClickNotification,BOOL bCheckBoxes)
{   
    DWORD dwStyles=ListView_GetExtendedListViewStyle(this->hWndListView);
    if (bFullRowSelect)
        dwStyles|=LVS_EX_FULLROWSELECT;
    else
        dwStyles&=~LVS_EX_FULLROWSELECT;
    if (bGridLines)
        dwStyles|=LVS_EX_GRIDLINES;
    else
        dwStyles&=~LVS_EX_GRIDLINES;
    if (bSendClickNotification)
    {
        dwStyles|=LVS_EX_ONECLICKACTIVATE;
        dwStyles|=LVS_EX_TWOCLICKACTIVATE;
    }
    else
    {
        dwStyles&=~LVS_EX_ONECLICKACTIVATE;
        dwStyles&=~LVS_EX_TWOCLICKACTIVATE;
    }

    if (bCheckBoxes)
        dwStyles|=LVS_EX_CHECKBOXES;
    else
        dwStyles&=~LVS_EX_CHECKBOXES;

    ListView_SetExtendedListViewStyle(this->hWndListView,dwStyles);
}

//-----------------------------------------------------------------------------
// Name: ReSort
// Object: Do the same action as the last sort command
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::ReSort()
{
    // if never sorted
    if (this->iLastSortedColumn==-1)
        return;
    this->Sort(this->iLastSortedColumn,this->bSortAscending);
}

//-----------------------------------------------------------------------------
// Name: Sort
// Object: Sort content of the listview based on the first column.
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::Sort()
{
    this->Sort(0);
}
//-----------------------------------------------------------------------------
// Name: Sort
// Object: Sort content of the listview based on the column with index ColumnIndex.
//         first time a column is selected, sorting is ascending, and next 
//         column sorting direction is auto changed until sort is call for another column index
// Parameters :
//     in  : int ColumnIndex : 0 based column index
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::Sort(int ColumnIndex)
{
    int ColumnCount=this->GetColumnCount();
    // adjust column count in case of null index
    if (ColumnCount==-1)
        ColumnCount=0;
    else// modify ColumnCount for 0 base index
        ColumnCount--;
    if (ColumnIndex>ColumnCount)
        return;

    if (ColumnIndex==this->iLastSortedColumn)
        this->bSortAscending=!this->bSortAscending;
    else
        this->bSortAscending=TRUE;

    this->Sort(ColumnIndex,this->bSortAscending);

}
//-----------------------------------------------------------------------------
// Name: Sort
// Object: Sort content of the listview based on the column with index ColumnIndex.
//         You can call Sort with same column index without specifying the direction
//         if you just want to inverse order (it will be managed internally)
// Parameters :
//     in  : int ColumnIndex : 0 based column index
//           BOOL bAscending : TRUE for ascending sorting
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::Sort(int ColumnIndex,BOOL bAscending)
{
    HCURSOR hOldCursor=GetCursor();
    // update iLastSortedColumn before calling ListView_SortItems
    this->iLastSortedColumn=ColumnIndex;
    this->bSortAscending=bAscending;
    // sort
    WaitForSingleObject(hevtUnlocked,LIST_VIEW_TIMEOUT);
    SetCursor(LoadCursor(NULL,IDC_WAIT));

    // make a local copy of {lparam,string} as content of listview is unstable during sorting
    // make a new array of ItemKey size to store string at lparam position in the array
    this->pSortArray=new TCHAR*[this->ItemKey];
    memset (this->pSortArray,0,this->ItemKey*sizeof(TCHAR*));

    DWORD nbItem=this->GetItemCount();
    DWORD cnt;
    LV_ITEM Item={0};
    CListviewItemParam* pListviewItemParam;
    Item.mask=LVIF_PARAM|LVIF_TEXT;
    TCHAR psz[MAX_PATH];
    Item.iSubItem=this->iLastSortedColumn;
    Item.pszText=psz;
    Item.cchTextMax=MAX_PATH;
    // for each item of list
    for (cnt=0;cnt<nbItem;cnt++)
    {
        Item.iItem=cnt;
        if (ListView_GetItem(this->hWndListView,&Item))
        {
            pListviewItemParam=(CListviewItemParam*)Item.lParam;
            if (pListviewItemParam)
                this->pSortArray[pListviewItemParam->ItemKey]=_tcsdup(Item.pszText);
        }
    }

    ListView_SortItems(this->hWndListView,CListview::Compare,this);

    for (cnt=0;cnt<this->ItemKey;cnt++)
    {
        if (this->pSortArray[cnt])
            free(this->pSortArray[cnt]);
    }
    delete this->pSortArray;

    // show up/down arrow on headers
    HDITEM HeaderItem;
    HWND hHeader=ListView_GetHeader(this->hWndListView);
    HeaderItem.mask=HDI_FORMAT;
    for (cnt=0;cnt<(DWORD)this->GetColumnCount();cnt++)
    {
        Header_GetItem(hHeader,cnt,&HeaderItem);

        // if current modify index
        if (cnt==(DWORD)ColumnIndex)
        {
            // show up or down arrow
            if (bAscending)
            {
                HeaderItem.fmt|=HDF_SORTUP;
                HeaderItem.fmt&=~HDF_SORTDOWN;
            }
            else
            {
                HeaderItem.fmt|=HDF_SORTDOWN;
                HeaderItem.fmt&=~HDF_SORTUP;
            }
        }
        else
        {
            // remove any previous arrow
            HeaderItem.fmt&=~HDF_SORTUP;
            HeaderItem.fmt&=~HDF_SORTDOWN;
        }
        Header_SetItem(hHeader,cnt,&HeaderItem);
    }

    SetCursor(hOldCursor);
    SetEvent(this->hevtUnlocked);
}

//-----------------------------------------------------------------------------
// Name: Sorting compare
// Object: 
// Parameters :
//     in  : LPARAM lParam1 : Item lParam given when item was inserted
//           LPARAM lParam2 : Item lParam given when item was inserted
//           LPARAM lParamSort : CListview* pointer to CListview object being sort
//     out : 
//     return : a negative value if the first item should precede the second,
//              a positive value if the first item should follow the second, 
//              or zero if the two items are equivalent
//-----------------------------------------------------------------------------
int CALLBACK CListview::Compare(LPARAM lParam1, LPARAM lParam2,LPARAM lParamSort)
{
    CListview* pCListview=(CListview*)lParamSort;
    CListviewItemParam* Param1;
    CListviewItemParam* Param2;

    Param1=(CListviewItemParam*)lParam1;
    Param2=(CListviewItemParam*)lParam2;

    int iAscendingFlag;

    if (pCListview->bSortAscending)
        iAscendingFlag=1;
    else
        iAscendingFlag=-1;

    if (IsBadReadPtr(Param1,sizeof(CListviewItemParam*)))
        return (-iAscendingFlag);
    if (IsBadReadPtr(Param1,sizeof(CListviewItemParam*)))
        return (iAscendingFlag);

    if (IsBadReadPtr(pCListview->pSortArray[Param1->ItemKey],sizeof(TCHAR*)))
        return (-iAscendingFlag);
    if (IsBadReadPtr(pCListview->pSortArray[Param2->ItemKey],sizeof(TCHAR*)))
        return (iAscendingFlag);
    
    if (pCListview->SortingCompareCallback)
    {
        return pCListview->SortingCompareCallback(pCListview->pSortArray[Param1->ItemKey],
                                                    pCListview->pSortArray[Param2->ItemKey],
                                                    pCListview->stSortingType,
                                                    pCListview->bSortAscending,
                                                    pCListview->SortingCompareCallbackUserParam);
    }
    else
    {
        // compare strings
        switch (pCListview->stSortingType)
        {
            case CListview::SortingTypeNumber:
                {
                    int i1=_ttoi(pCListview->pSortArray[Param1->ItemKey]);
                    int i2=_ttoi(pCListview->pSortArray[Param2->ItemKey]);
                    return ((i1-i2)*iAscendingFlag);
                }
            default:
            case CListview::SortingTypeString:
                return (_tcsicmp(pCListview->pSortArray[Param1->ItemKey],pCListview->pSortArray[Param2->ItemKey])*iAscendingFlag);
        }
    }
}

//-----------------------------------------------------------------------------
// Name: SetSortingCompareCallback
// Object: Set the sorting callback for not standard item sorting 
// Parameters :
//     in  : tagSortingCompareCallback SortingCompareCallback : sorting callback must return :
//                                                              - negative value if first item is less than second item
//                                                              - positive value else
//           LPVOID UserParam : user param
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetSortingCompareCallback(tagSortingCompareCallback SortingCompareCallback,LPVOID UserParam)
{
    this->SortingCompareCallback=SortingCompareCallback;
    this->SortingCompareCallbackUserParam=UserParam;
}

//-----------------------------------------------------------------------------
// Name: SetSortingType
// Object: Set the sorting type 
// Parameters :
//     in  : CListview::SortingType type : new type of sorting
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetSortingType(CListview::SortingType type)
{
    this->stSortingType=type;
}

BOOL CListview::SetItemUserData(int ItemIndex,LPVOID UserData)
{
    LV_ITEM Item={0};
    CListviewItemParam* pListviewItemParam;
    Item.mask=LVIF_PARAM;
    Item.iItem=ItemIndex;
    // try to get our own object associated with item
    if (!ListView_GetItem(this->hWndListView,&Item))
        return FALSE;
    // check if object is valid
    pListviewItemParam=(CListviewItemParam*)Item.lParam;
    if (IsBadReadPtr(pListviewItemParam,sizeof(CListviewItemParam)))
        return FALSE;

    // set the user part of our object
    pListviewItemParam->UserParam=UserData;

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: GetItemUserData
// Object: Get user data associated with item
// Parameters :
//     in  : int ItemIndex : index of item
//           LPVOID* pUserData : pointer to value to retrieve
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CListview::GetItemUserData(int ItemIndex,LPVOID* pUserData)
{
    // check pointer
    if (IsBadWritePtr(pUserData,sizeof(LPVOID)))
        return FALSE;

    LV_ITEM Item={0};
    CListviewItemParam* pListviewItemParam;
    Item.mask=LVIF_PARAM;
    Item.iItem=ItemIndex;
    // try to get our own object associated with item
    if (!ListView_GetItem(this->hWndListView,&Item))
        return FALSE;
    // check if object is valid
    pListviewItemParam=(CListviewItemParam*)Item.lParam;
    if (IsBadReadPtr(pListviewItemParam,sizeof(CListviewItemParam)))
        return FALSE;

    // return the user part of our object to user
    *pUserData=pListviewItemParam->UserParam;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Find
// Object: search for item string in listview
// Parameters :
//     in  : Text : text to search for
//     out : 
//     return : index on success, -1 if not found
//-----------------------------------------------------------------------------
int CListview::Find(TCHAR* Text)
{
    return this->Find(Text,-1,FALSE);
}
//-----------------------------------------------------------------------------
// Name: Find
// Object: search for item string in listview
// Parameters :
//     in  : Text : text to search for
//           BOOL bPartial : TRUE for partial matching : Checks to see if the item text begins with Text
//     out : 
//     return : index on success, -1 if not found
//-----------------------------------------------------------------------------
int CListview::Find(TCHAR* Text,BOOL bPartial)

{
    return this->Find(Text,-1,bPartial);
}
//-----------------------------------------------------------------------------
// Name: Find
// Object: search for item string in listview
// Parameters :
//     in  : Text : text to search for
//           int StartIndex : Index of the item to begin the search
//           BOOL bPartial : TRUE for partial matching : Checks to see if the item text begins with Text
//     out : 
//     return : index on success, -1 if not found
//-----------------------------------------------------------------------------
int CListview::Find(TCHAR* Text,int StartIndex,BOOL bPartial)
{
    LVFINDINFO FindInfo={0};
    FindInfo.psz=Text;
    FindInfo.flags=LVFI_STRING;
    if (bPartial)
        FindInfo.flags|=LVFI_PARTIAL;
    return ListView_FindItem(this->hWndListView,StartIndex,&FindInfo);
}

//-----------------------------------------------------------------------------
// Name: ShowColumnHeaders
// Object: show or hides columns headers
// Parameters :
//     in  : BOOL bShow : True to show columns headers
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::ShowColumnHeaders(BOOL bShow)
{
    LONG_PTR Style=GetWindowLongPtr(this->hWndListView,GWL_STYLE);

    if (bShow)
        Style&=~LVS_NOCOLUMNHEADER;        
    else
        Style|=LVS_NOCOLUMNHEADER;

    SetWindowLongPtr(this->hWndListView,GWL_STYLE,Style);
    
}

//-----------------------------------------------------------------------------
// Name: SetTransparentBackGround
// Object: make background of listview to become transparent
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetTransparentBackGround()
{
    ListView_SetBkColor(this->hWndListView,CLR_NONE);
    ListView_SetTextBkColor(this->hWndListView,CLR_NONE);
    ::InvalidateRect(this->hWndListView,0,TRUE);
}

//-----------------------------------------------------------------------------
// Name: InvertSelection
// Object: change the selected state of all items
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::InvertSelection()
{
    int NbItems;
    BOOL bSelectCallback;

    WaitForSingleObject(hevtUnlocked,LIST_VIEW_TIMEOUT);

    // store current selection callback state
    bSelectCallback=this->bSelectionCallBackEnable;
    NbItems=this->GetItemCount();

    // for each item in list
    for (int ItemIndex=0;ItemIndex<NbItems;ItemIndex++)
    {
        // reverse it's state
        this->SetSelectedState(ItemIndex,!this->IsItemSelected(ItemIndex),FALSE);

        // disable callback after fisrt selection
        if (this->bSelectionCallBackEnable)
            this->bSelectionCallBackEnable=FALSE;
    }

    // restore selection callback state
    this->bSelectionCallBackEnable=bSelectCallback;

    SetEvent(this->hevtUnlocked);
}


void CListview::EnableDefaultCustomDraw(BOOL bEnable)
{
    this->DefaultCustomDrawEnabled=bEnable;
}
void CListview::EnablePopUpMenu(BOOL bEnable)
{
    this->PopUpMenuEnabled=bEnable;
}
void CListview::EnableColumnSorting(BOOL bEnable)
{
    this->ColumnSortingEnabled=bEnable;
}
void CListview::EnableColumnReOrdering(BOOL bEnable)
{
    DWORD Style=ListView_GetExtendedListViewStyle(this->hWndListView);
    if (bEnable)
        Style |=LVS_EX_HEADERDRAGDROP;
    else
        Style &=~LVS_EX_HEADERDRAGDROP;
    ListView_SetExtendedListViewStyle(this->hWndListView,Style);
}
void CListview::SetDefaultCustomDrawColor(COLORREF ColorRef)
{
    this->DefaultCustomDrawColor=ColorRef;
}

LRESULT CListview::ProcessDefaultCustomDraw (LPARAM lParam)
{
    LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

    switch(lplvcd->nmcd.dwDrawStage) 
    {
    case CDDS_PREPAINT : //Before the paint cycle begins
        //request notifications for individual listview items
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT: //Before an item is drawn
        // lplvcd->nmcd.dwItemSpec // item index
        // to request notification for subitems
        // return CDRF_NOTIFYSUBITEMDRAW;

        //customize item appearance
        if (lplvcd->nmcd.dwItemSpec%2==1)
            // set a background color
            lplvcd->clrTextBk = this->DefaultCustomDrawColor;

        //To set a custom font:
        //SelectObject(lplvcd->nmcd.hdc, <your custom HFONT>);
        // return CDRF_NEWFONT;

        return CDRF_DODEFAULT;

        // for subitem customization
        // case CDDS_SUBITEM | CDDS_ITEMPREPAINT: //Before a subitem is drawn
        // lplvcd->nmcd.dwItemSpec // item number
        // lplvcd->iSubItem // subitem number
        // // customize subitem appearance for column 0
        // lplvcd->clrText   = RGB(255,255,255);
        // lplvcd->clrTextBk = RGB(255,0,0);
        // //To set a custom font:
        // //SelectObject(lplvcd->nmcd.hdc, <your custom HFONT>);

        // return CDRF_NEWFONT;
    }
    return CDRF_DODEFAULT;
}
//-----------------------------------------------------------------------------
// Name: OnNotify
// Object: a WM_NOTIFY message helper for list view and it's associated columns
//         Must be called in the WndProc when receiving a WM_NOTIFY message 
//          if you want to reorder listview by clicking columns
//         EXAMPLE :
//         assuming mpListview is NULL before creation and after deletion, an example can be the following 
//         where mpListview is a CListview*
//              LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//              switch (uMsg)
//              {
//                  case WM_NOTIFY:
//                      if (mpListview)
//                          mpListview->OnNotify(wParam,lParam);
//                      break;
//              }
// Parameters :
//     in  : WPARAM wParam : WndProc wParam
//           LPARAM lParam : WndProc lParam
//     out : 
//     return : TRUE if message have been internally proceed
//-----------------------------------------------------------------------------
BOOL CListview::OnNotify(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    NMHDR* pnmh=((NMHDR*)lParam);

    // listview messages
    if (pnmh->hwndFrom==this->hWndListView)
    {
        switch (pnmh->code) 
        {
        case NM_RCLICK:
            {
                if (this->PopUpMenuEnabled)
                {
                    LPNMITEMACTIVATE pnmitem;
                    RECT Rect;
                    UINT uiRetPopUpMenuItemID;
                    pnmitem = (LPNMITEMACTIVATE) lParam;
                    // get listview position
                    GetWindowRect(this->hWndListView,&Rect);
                    // show popupmenu
                    uiRetPopUpMenuItemID=this->pPopUpMenu->Show(Rect.left+pnmitem->ptAction.x,
                                                                Rect.top+pnmitem->ptAction.y,
                                                                this->hWndListView);
                    if (uiRetPopUpMenuItemID==0)// no menu selected
                    {
                        return TRUE;
                    }
                    else if (uiRetPopUpMenuItemID==this->MenuIdCopySelected)
                    {
                        this->CopyToClipBoard(TRUE,TRUE);
                        return TRUE;
                    }
                    else if (uiRetPopUpMenuItemID==this->MenuIdCopyAll)
                    {
                        this->CopyToClipBoard(FALSE,TRUE);
                        return TRUE;
                    }
                    else if (uiRetPopUpMenuItemID==this->MenuIdSaveSelected)
                    {
                        this->Save(TRUE,TRUE);
                        return TRUE;
                    }
                    else if (uiRetPopUpMenuItemID==this->MenuIdSaveAll)
                    {
                        this->Save(FALSE,TRUE);
                        return TRUE;
                    }
                    else if (uiRetPopUpMenuItemID==this->MenuIdClear)
                    {
                        this->Clear();
                        return TRUE;
                    }
                    // menu is not processed internally
                    if (!IsBadCodePtr((FARPROC)this->PopUpMenuItemClickCallback))
                        this->PopUpMenuItemClickCallback(uiRetPopUpMenuItemID,this->PopUpMenuItemClickCallbackUserParam);
                    return TRUE;
                }
            }
            break;

        case LVN_ITEMCHANGED:
            {
                if (this->bSelectionCallBackEnable)
                {
                    LPNMLISTVIEW pnmv;
                    pnmv = (LPNMLISTVIEW) lParam;
                    // if item selection has changed
                    if (pnmv->iItem>=0)
                    {
                        // if new state is selected
                        if ((pnmv->uNewState & LVIS_SELECTED)&& (!(pnmv->uOldState & LVIS_SELECTED)))
                        {
                            if (!IsBadCodePtr((FARPROC)this->SelectItemCallback))
                                this->SelectItemCallback(pnmv->iItem,pnmv->iSubItem,this->SelectItemCallbackUserParam);
                            return TRUE;
                        }// if unselected
                        else if ((pnmv->uOldState & LVIS_SELECTED)&& (!(pnmv->uNewState & LVIS_SELECTED)))
                        {
                            if (!IsBadCodePtr((FARPROC)this->UnselectItemCallback))
                                this->UnselectItemCallback(pnmv->iItem,pnmv->iSubItem,this->UnselectItemCallbackUserParam);
                            return TRUE;
                        }
                    }
                }
            }
            break;
        case NM_CUSTOMDRAW:
            if (this->DefaultCustomDrawEnabled)
            {
                //must use SetWindowLongPtr to return value from dialog proc
                SetWindowLongPtr(this->hWndParent, DWLP_MSGRESULT, (LONG_PTR)this->ProcessDefaultCustomDraw(lParam));
                return TRUE;
            }
        }
    }
    // listview headers messages
    else if (pnmh->hwndFrom==ListView_GetHeader(this->hWndListView))
    {
        LPNMHEADER pnmheader;
        pnmheader = (LPNMHEADER) lParam;
        switch (pnmh->code) 
        {
        // on header click
        case HDN_ITEMCLICKA:
        case HDN_ITEMCLICKW:
            if (this->ColumnSortingEnabled)
            {
                // sort the clicked column ascending or descending, depending last sort
                this->Sort(pnmheader->iItem);
                return TRUE;
            }
            break;
        //case HDN_ENDDRAG:
        //    OutputDebugString(_T("HDN_ENDDRAG"));
        //    {
        //    LRESULT NbColumns = Header_GetItemCount(pnmh->hwndFrom);
        //    int* lpiArray = new int[NbColumns];
        //    Header_GetOrderArray(pnmh->hwndFrom,NbColumns,lpiArray);
        //    DebugBreak;
        //    }
        //    break;
        case HDN_BEGINTRACKA:
        case HDN_BEGINTRACKW:
            if (pnmheader->pitem->cxy ==0) // avoid tracking for hidden columns
            {
                SetWindowLongPtr(this->hWndParent, DWLP_MSGRESULT, (LONG_PTR)TRUE);// MUST set in case of dialog proc
                return TRUE;// item is hidden return TRUE to prevent tracking
            }
            break;
        case HDN_ENDTRACKA:
        case HDN_ENDTRACKW:
            if (pnmheader->pitem->cxy <5) 
                // avoid 0 value to allow tracking for user manually hidden column
                ::PostMessage(this->hWndListView,LVM_SETCOLUMNWIDTH,pnmheader->iItem,5);
            break;
        }
    }
    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: ShowClearPopupMenu
// Object: Show or hide the Clear popupmenu (by default Clear menu is hidden)
// Parameters :
//     in  : BOOL bVisible : TRUE to show it, FLASE to hide it
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::ShowClearPopupMenu(BOOL bVisible)
{
    // assume menu is removed before adding it
    this->pPopUpMenu->Remove(this->MenuIdClear);

    // if we should show menu
    if (bVisible)
        // show it
        this->MenuIdClear=this->pPopUpMenu->Add(_T("Clear"));
}

//-----------------------------------------------------------------------------
// Name: SetWatermarkImage
// Object: set a watermark / background image
//         Thanks to Stephan of TortoiseSVN team http://tortoisesvn.net/blog/3
// Parameters :
//     in  : HBITMAP hBmp
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::SetWatermarkImage(HBITMAP hBmp)
{
    LVBKIMAGE LvBkImage={0};
    LvBkImage.hbm=hBmp;
    LvBkImage.ulFlags= LVBKIF_TYPE_WATERMARK;
    return ListView_SetBkImage(this->hWndListView,&LvBkImage);
}

//-----------------------------------------------------------------------------
// Name: SetSelectItemCallback
// Object: set the callback called when an item is selected
// Parameters :
//     in  : tagSelectItemCallback SelectItemCallback : callback called when 
//           an item is selected
//           LPVOID UserParam : any user parameter transmitted to the callback
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetSelectItemCallback(tagSelectItemCallback SelectItemCallback,LPVOID UserParam)
{
    this->SelectItemCallback=SelectItemCallback;
    this->SelectItemCallbackUserParam=UserParam;
}
//-----------------------------------------------------------------------------
// Name: SetUnselectItemCallback
// Object: set the callback called when an item is unselected
// Parameters :
//     in  : tagUnselectItemCallback UnselectItemCallback : callback called when 
//           an item is unselected
//           LPVOID UserParam : any user parameter transmitted to the callback
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetUnselectItemCallback(tagUnselectItemCallback UnselectItemCallback,LPVOID UserParam)
{
    this->UnselectItemCallback=UnselectItemCallback;
    this->UnselectItemCallbackUserParam=UserParam;
}

//-----------------------------------------------------------------------------
// Name: SetSelectionCallbackState
// Object: Allow to disable selections callback for a while and then reenable them
// Parameters :
//     in  : BOOL bEnable : new state. 
//                          TRUE to activate SetUnselectItemCallback and SetSelectItemCallback
//                          FALSE to disable SetUnselectItemCallback and SetSelectItemCallback
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetSelectionCallbackState(BOOL bEnable)
{
    this->bSelectionCallBackEnable=bEnable;
}

//-----------------------------------------------------------------------------
// Name: SetPopUpMenuItemClickCallback
// Object: set a call back for menuItemClick event
// Parameters :
//     in  : tagPopUpMenuItemClickCallback PopUpMenuItemClickCallback : call back
//           LPVOID UserParam : any user parameter transmitted to the callback
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetPopUpMenuItemClickCallback(tagPopUpMenuItemClickCallback PopUpMenuItemClickCallback,LPVOID UserParam)
{
    this->PopUpMenuItemClickCallback=PopUpMenuItemClickCallback;
    this->PopUpMenuItemClickCallbackUserParam=UserParam;
}

//-----------------------------------------------------------------------------
// Name: SetColumnIconIndex
// Object: use icon index in column icon list for specified column
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::SetColumnIconIndex(int ColumnIndex,int IconIndexInColumnIconList)
{
    // get header handle
    HWND Header=ListView_GetHeader(this->hWndListView);
    if (!Header)
        return FALSE;

    HDITEM HeaderItem={0};
    HeaderItem.mask= HDI_FORMAT;
    // get current format
    Header_GetItem(Header,ColumnIndex,&HeaderItem);

    HeaderItem.mask=HDI_IMAGE | HDI_FORMAT;
    HeaderItem.iImage=IconIndexInColumnIconList;
    // add image format
    HeaderItem.fmt|=HDF_IMAGE; //add |HDF_BITMAP_ON_RIGHT to display on right
    

    return Header_SetItem(Header,ColumnIndex,&HeaderItem);
}
//-----------------------------------------------------------------------------
// Name: SetItemIconIndex
// Object: use icon index in icon list for specified item
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::SetItemIconIndex(int ItemIndex,int IconIndex)
{
    LVITEM Item={0};
    Item.mask=LVIF_IMAGE;
    Item.iItem=ItemIndex;
    Item.iImage=IconIndex;

    return ListView_SetItem(this->hWndListView,&Item);
}

//-----------------------------------------------------------------------------
// Name: AddIcon
// Object: remove an icon from the specified list
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::RemoveIcon(tagImageLists ImageList,int IconIndex)
{
    HIMAGELIST hImgList;
    switch(ImageList)
    {
    case ImageListNormal:
        hImgList=this->hImgListNormal;
        break;
    case ImageListSmall:
        hImgList=this->hImgListSmall;
        break;
    case ImageListState:
        hImgList=this->hImgListState;
        break;
    case ImageListColumns:
        hImgList=this->hImgListColumns;
        break;
    default:
        return FALSE;
    }
    if (!hImgList)
        return FALSE;

    return ImageList_Remove(hImgList,IconIndex);
}
//-----------------------------------------------------------------------------
// Name: RemoveAllIcons
// Object: remove all icons from the specified list
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::RemoveAllIcons(tagImageLists ImageList)
{
    return this->RemoveIcon(ImageList,-1);
}
//-----------------------------------------------------------------------------
// Name: AddIcon
// Object: add an icon to the specified list
// Parameters :
//     in  : 
//     out : 
//     return : index of icon in image list
//-----------------------------------------------------------------------------
int CListview::AddIcon(tagImageLists ImageList,HMODULE hModule,int IdIcon)
{
    HICON hicon=(HICON)LoadImage(hModule, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,0,0,LR_SHARED); 
    if (!hicon)
        return -1;

    return this->AddIcon(ImageList,hicon);
}
//-----------------------------------------------------------------------------
// Name: AddIcon
// Object: add an icon to the specified list
// Parameters :
//     in  : 
//     out : 
//     return : index of icon in image list
//-----------------------------------------------------------------------------
int CListview::AddIcon(tagImageLists ImageList,HICON hIcon)
{
    HIMAGELIST hImgList;
    switch(ImageList)
    {
    case ImageListNormal:
        // if list doesn't exists
        if (!this->hImgListNormal)
        {
            // create it
            this->hImgListNormal=ImageList_Create(32, 32, ILC_MASK|ILC_COLOR32, 20, 5);
            if (!this->hImgListNormal)
                return -1;
            // associate it to corresponding list
            ListView_SetImageList(this->hWndListView,this->hImgListNormal,LVSIL_NORMAL);
        }
        hImgList=this->hImgListNormal;
        break;
    case ImageListSmall:
        // if list doesn't exists
        if (!this->hImgListSmall)
        {
            // create it
            this->hImgListSmall=ImageList_Create(16, 16, ILC_MASK|ILC_COLOR32, 20, 5);
            if (!this->hImgListSmall)
                return -1;
            // associate it to corresponding list
            ListView_SetImageList(this->hWndListView,this->hImgListSmall,LVSIL_SMALL);
        }
        hImgList=this->hImgListSmall;
        break;
    case ImageListState:
        // if list doesn't exists
        if (!this->hImgListState)
        {
            // create it
            this->hImgListState=ImageList_Create(16, 16, ILC_MASK|ILC_COLOR32, 20, 5);
            if (!this->hImgListState)
                return -1;
            // associate it to corresponding list
            ListView_SetImageList(this->hWndListView,this->hImgListState,LVSIL_STATE);
        }
        hImgList=this->hImgListState;
        break;
    case ImageListColumns:
        // if list doesn't exists
        if (!this->hImgListColumns)
        {
            // get header handle
            HWND Header=ListView_GetHeader(this->hWndListView);
            if (!Header)
                return -1;
            // create it
            this->hImgListColumns=ImageList_Create(16, 16, ILC_MASK|ILC_COLOR32, 20, 5);
            if (!this->hImgListColumns)
                return -1;
            // associate it to corresponding list
            Header_SetImageList(Header,this->hImgListColumns);
        }
        hImgList=this->hImgListColumns;
        break;
    default:
        return -1;
    }
    
    // add icon to list
    return ImageList_AddIcon(hImgList, hIcon);
}

//-----------------------------------------------------------------------------
// Name: CopyToClipBoard
// Object: Copy all listview content to clipboard
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::CopyToClipBoard()
{
    return this->CopyToClipBoard(FALSE);
}
//-----------------------------------------------------------------------------
// Name: CopyToClipBoard
// Object: Copy listview content to clipboard
// Parameters :
//     in  : BOOL bOnlySelected : TRUE to copy only selected items
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::CopyToClipBoard(BOOL bOnlySelected)
{
    return this->CopyToClipBoard(bOnlySelected,FALSE);
}

//-----------------------------------------------------------------------------
// Name: CopyToClipBoard
// Object: Copy listview content to clipboard
// Parameters :
//     in  : BOOL bOnlySelected : TRUE to copy only selected items
//           BOOL bOnlyVisibleColumns : TRUE to copy only visible columns
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::CopyToClipBoard(BOOL bOnlySelected,BOOL bOnlyVisibleColumns)
{
    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);
    TCHAR* pszFullData;
    TCHAR* psz;
    TCHAR* pszItemText;
    BOOL  bOneOrLessItem;
    DWORD FullDataSize=MAX_PATH*2;
    DWORD FullDataUsedSize=0;
    DWORD ItemTextSize=MAX_PATH*2;
    LVITEM lvi;
    DWORD ItemTextLen;

    int iNbRow=this->GetItemCount();
    int iNbColumn=this->GetColumnCount();

    if (iNbColumn<0)
        iNbColumn=1;


    if (bOnlySelected)
        bOneOrLessItem=(this->GetSelectedCount()<=1);
    else
        bOneOrLessItem=(this->GetItemCount()<=1);

    pszFullData=new TCHAR[FullDataSize];
    pszItemText=new TCHAR[ItemTextSize];
    *pszFullData=0;
    // for each row
    for (int cnt=0;cnt<iNbRow;cnt++)
    {
        // if we save only selected items
        if (bOnlySelected)
        {
            // if item is not selected
            if (!this->IsItemSelected(cnt,TRUE))
                continue;
        }

        // for each column
        for (int cnt2=0;cnt2<iNbColumn;cnt2++)
        {
            if (bOnlyVisibleColumns)
            {
                if (!this->IsColumnVisible(cnt2))
                    continue;
            }

            lvi.cchTextMax=ItemTextSize;
            lvi.pszText=pszItemText;
            lvi.iSubItem=cnt2;
            ItemTextLen=(DWORD)SendMessage(this->hWndListView,LVM_GETITEMTEXT,cnt,(LPARAM)&lvi);

            // do a loop increasing size
            // as soon as size of text copied to lvi.pszText is smaller than buffer size
            // we got the real size
            while ((ItemTextLen==ItemTextSize)||(ItemTextLen==ItemTextSize-1))
            {
                delete[] pszItemText;
                ItemTextSize*=2;
                pszItemText=new TCHAR[ItemTextSize];
                if (!pszItemText)
                {
                    delete[] pszFullData;
                    SetEvent(this->hevtUnlocked);

                    return FALSE;
                }
                lvi.cchTextMax=ItemTextSize;
                lvi.pszText=pszItemText;
                ItemTextLen=(DWORD)SendMessage(this->hWndListView,LVM_GETITEMTEXT,cnt,(LPARAM)&lvi);
            }
            ItemTextLen++;

            FullDataUsedSize+=ItemTextLen+4;// +4 for \t \r\n \0
            if (FullDataUsedSize>=FullDataSize)
            {
                FullDataSize*=2;
                if (FullDataUsedSize>=FullDataSize)
                    FullDataSize=FullDataUsedSize*2;
                psz=pszFullData;
                pszFullData=new TCHAR[FullDataSize];
                if (!pszFullData)
                {
                    delete[] pszItemText;
                    delete[] psz;
                    SetEvent(this->hevtUnlocked);

                    return FALSE;
                }
                _tcscpy(pszFullData,psz);
                delete[] psz;
            }

            // add to pszFullData
            _tcscat(pszFullData,pszItemText);

            // if not last column
            if (cnt2!=(iNbColumn-1))
                _tcscat(pszFullData,_T("\t"));
        }
        if (!bOneOrLessItem)
            _tcscat(pszFullData,_T("\r\n"));
    }
    
    // copy data to clipboard
    CClipboard::CopyToClipboard(this->hWndListView,pszFullData);

    // free memory
    delete[] pszItemText;
    delete[] pszFullData;

    SetEvent(this->hevtUnlocked);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview. This function shows an interface to user
//         for choosing file
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save()
{
    return this->Save(FALSE);
}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview. This function shows an interface to user
//         for choosing file
// Parameters :
//     in  : BOOL bOnlySelected : TRUE to save only selected items
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(BOOL bOnlySelected)
{
    return this->Save(bOnlySelected,FALSE);
}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview. This function shows an interface to user
//         for choosing file
// Parameters :
//     in  : BOOL bOnlySelected : TRUE to save only selected items
//           BOOL bOnlyVisibleColumns : TRUE to save only visible columns
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(BOOL bOnlySelected,BOOL bOnlyVisibleColumns)
{
    return this->Save(bOnlySelected,bOnlyVisibleColumns,_T("xml"));
}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview. This function shows an interface to user
//         for choosing file
// Parameters :
//     in  : BOOL bOnlySelected : TRUE to save only selected items
//           BOOL bOnlyVisibleColumns : TRUE to save only visible columns
//           TCHAR* DefaultExtension : default extension (xml, html or txt)
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(BOOL bOnlySelected,BOOL bOnlyVisibleColumns,TCHAR* DefaultExtension)
{
    return this->Save(bOnlySelected,bOnlyVisibleColumns,DefaultExtension,CListview::SpecializedSavingFunctionStatic,this);
}
//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview. This function shows an interface to user
//         for choosing file
// Parameters :
//     in  : BOOL bOnlySelected : TRUE to save only selected items
//           BOOL bOnlyVisibleColumns : TRUE to save only visible columns
//           TCHAR* DefaultExtension : default extension (xml, html or txt)
//           pfSpecializedSavingFunction SpecializedSavingFunction : function specialized fo saving
//                 use it only for non standard saving (like saving data that are not in listview)
//           LPVOID UserParam : UserParam passed on each call of SpecializedSavingFunction
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(BOOL bOnlySelected,BOOL bOnlyVisibleColumns,TCHAR* DefaultExtension,pfSpecializedSavingFunction SpecializedSavingFunction,LPVOID UserParam)
{

    TCHAR pszFile[MAX_PATH]=_T("");

    // open dialog
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=NULL;
    ofn.hInstance=NULL;
    ofn.lpstrFilter=_T("xml\0*.xml\0txt\0*.txt\0html\0*.html\0csv\0*.csv\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt=DefaultExtension;
    ofn.lpstrFile=pszFile;
    ofn.nMaxFile=MAX_PATH;

    if (_tcsicmp(DefaultExtension,_T("xml"))==0)
        ofn.nFilterIndex=1;
    else if (_tcsicmp(DefaultExtension,_T("txt"))==0)
        ofn.nFilterIndex=2;
    else if (_tcsicmp(DefaultExtension,_T("html"))==0)
        ofn.nFilterIndex=3;
    else //if (_tcsicmp(DefaultExtension,_T("csv"))==0)
        ofn.nFilterIndex=4;

    if (!GetSaveFileName(&ofn))
        return TRUE;

    return this->Save(ofn.lpstrFile,bOnlySelected,bOnlyVisibleColumns,SpecializedSavingFunction,UserParam);

}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview in the specified file.
//         Save depends of extension name. Save will be done in xml for .xml files
//         else it will be done in text
//         Warning Only saves done in xml can be reloaded
// Parameters :
//     in  : TCHAR* pszFileName : output file (full pathname)
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(TCHAR* pszFileName)
{
    return this->Save(pszFileName,FALSE);
}
//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview in the specified file.
//         Save depends of extension name. Save will be done in xml for .xml files
//         else it will be done in text
//         Warning Only saves done in xml can be reloaded
// Parameters :
//     in  : TCHAR* pszFileName : output file (full pathname)
//           BOOL bOnlySelected : TRUE to save only selected items
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(TCHAR* pszFileName,BOOL bOnlySelected)
{
    return this->Save(pszFileName,bOnlySelected,FALSE);
}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview in the specified file.
//         Save depends of extension name. Can be ".xml", ".html", ".csv"
//         else it will be done in text
//         Warning Only saves done in xml can be reloaded
// Parameters :
//     in  : TCHAR* pszFileName : output file (full pathname)
//           BOOL bOnlySelected : TRUE to save only selected items
//           BOOL bOnlyVisibleColumns : TRUE to save only visible columns
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(TCHAR* pszFileName,BOOL bOnlySelected,BOOL bOnlyVisibleColumns)
{
    return this->Save(pszFileName,bOnlySelected,bOnlyVisibleColumns,CListview::SpecializedSavingFunctionStatic,this);
}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview in the specified file.
//         Save depends of extension name. Can be ".xml", ".html", ".csv"
//         else it will be done in text
//         Warning Only saves done in xml can be reloaded
// Parameters :
//     in  : TCHAR* pszFileName : output file (full pathname)
//           BOOL bOnlySelected : TRUE to save only selected items
//           BOOL bOnlyVisibleColumns : TRUE to save only visible columns
//           pfSpecializedSavingFunction SpecializedSavingFunction : function specialized fo saving
//                 use it only for non standard saving (like saving data that are not in listview)
//           LPVOID UserParam : UserParam passed on each call of SpecializedSavingFunction
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(TCHAR* pszFileName,BOOL bOnlySelected,BOOL bOnlyVisibleColumns,pfSpecializedSavingFunction SpecializedSavingFunction,LPVOID UserParam)
{
    int iNbRow=this->GetItemCount();
    int iNbColumn=this->GetColumnCount();
    DWORD NbBytesWritten;

    if (!pszFileName)
        return FALSE;

    if (!*pszFileName)
        return FALSE;

    if (iNbColumn<0)
        iNbColumn=1;

    SAVING_HANDLE SavingHandle;
    tagSavingType SavingType;
    // default saving type
    SavingType=SavingTypeTXT;

    // check HTML extension
    if (_tcslen(pszFileName)>5)
    {
        if (_tcsicmp(_T(".html"),&pszFileName[_tcslen(pszFileName)-5])==0)
            SavingType=SavingTypeHTML;
    }
    if (_tcslen(pszFileName)>4)
    {
        // check xml extension
        if (_tcsicmp(_T(".xml"),&pszFileName[_tcslen(pszFileName)-4])==0)
            SavingType=SavingTypeXML;
        // check csv extension
        else if (_tcsicmp(_T(".csv"),&pszFileName[_tcslen(pszFileName)-4])==0)
            SavingType=SavingTypeCSV;
    }

    HANDLE hFile;
    if (!CTextFile::CreateTextFile(pszFileName,&hFile))
        return FALSE;

    // fill SavingHandle struct content
    SavingHandle.hFile=hFile;
    SavingHandle.SavingType=SavingType;

    // begin of listview
    switch (SavingType)
    {
    case SavingTypeXML:
        CTextFile::WriteText(hFile,_T("<LISTVIEW>"));
        break;
    case SavingTypeHTML:
        CTextFile::WriteText(hFile,_T("<HTML><BODY><TABLE align=\"center\" border=\"1\">"));
        break;
    case SavingTypeCSV:
        // CSV doesn't support unicode
        // --> remove unicode file header if any by going to begin of file
        SetFilePointer(hFile,0,NULL,FILE_BEGIN);
        break;
    }
    // for each raw
    for (int cnt=0;cnt<iNbRow;cnt++)
    {
        // if we save only selected items
        if (bOnlySelected)
        {
            // if item is not selected
            if (!this->IsItemSelected(cnt,TRUE))
                continue;
        }

        // row start
        switch (SavingType)
        {
        case SavingTypeXML:
            CTextFile::WriteText(hFile,_T("<ROW>"));
            break;
        case SavingTypeHTML:
            CTextFile::WriteText(hFile,_T("<TR>"));
            break;
        }

        // for each column
        for (int cnt2=0;cnt2<iNbColumn;cnt2++)
        {
            if (bOnlyVisibleColumns)
            {
                if (!this->IsColumnVisible(cnt2))
                    continue;
            }
            // column start
            switch (SavingType)
            {
            case SavingTypeXML:
                CTextFile::WriteText(hFile,_T("<COLUMN>"));
                break;
            case SavingTypeHTML:
                CTextFile::WriteText(hFile,_T("<TD>"));
                break;
            case SavingTypeCSV:
                WriteFile(hFile,"\"",1,&NbBytesWritten,NULL);
                break;
            }

            // call saving func
            SpecializedSavingFunction(&SavingHandle,cnt,cnt2,UserParam);

            // column end
            switch (SavingType)
            {
            case SavingTypeXML:
                CTextFile::WriteText(hFile,_T("</COLUMN>"));
                break;
            case SavingTypeHTML:
                CTextFile::WriteText(hFile,_T("</TD>"));
                break;
            case SavingTypeCSV:
                WriteFile(hFile,"\";",2,&NbBytesWritten,NULL);
                break;
            case SavingTypeTXT:
                CTextFile::WriteText(hFile,_T("\t"));
                break;
            }
        }
        // end of row
        switch (SavingType)
        {
        case SavingTypeXML:
            CTextFile::WriteText(hFile,_T("</ROW>"));
            break;
        case SavingTypeHTML:
            CTextFile::WriteText(hFile,_T("</TR>"));
            break;
        case SavingTypeCSV:
            WriteFile(hFile,"\r\n",2,&NbBytesWritten,NULL);
            break;
        case SavingTypeTXT:
            CTextFile::WriteText(hFile,_T("\r\n"));
            break;
        }
    }
    // end of listview
    switch (SavingType)
    {
    case SavingTypeXML:
        CTextFile::WriteText(hFile,_T("</LISTVIEW>"));
        break;
    case SavingTypeHTML:
        CTextFile::WriteText(hFile,_T("</TABLE></BODY></HTML>"));
        break;
    }

    CloseHandle(hFile);

    if (MessageBox(NULL,_T("Save Successfully Completed\r\nDo you want to open saved document now ?"),_T("Information"),MB_YESNO|MB_ICONINFORMATION|MB_TOPMOST)==IDYES)
    {
        if (((int)ShellExecute(NULL,_T("open"),pszFileName,NULL,NULL,SW_SHOWNORMAL))<33)
            MessageBox(NULL,_T("Error opening associated viewer"),_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SpecializedSavingFunctionStatic
// Object: generic item saving function in case no user SpecializedSavingFunction was provided
// Parameters :
//     in  : HANDLE SavingHandle : handle provided by Save function
//           int ItemIndex : item index of item to be saved
//           int SubItemIndex : subitem index of item to be saved
//           LPVOID UserParam : user parameter (current listview object pointer)
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SpecializedSavingFunctionStatic(HANDLE SavingHandle,int ItemIndex,int SubItemIndex,LPVOID UserParam)
{
    // reenter object model
    ((CListview*)UserParam)->SpecializedSavingFunction(SavingHandle,ItemIndex,SubItemIndex);
}

//-----------------------------------------------------------------------------
// Name: SpecializedSavingFunction
// Object: generic item saving function in case no user SpecializedSavingFunction was provided
// Parameters :
//     in  : HANDLE SavingHandle : handle provided by Save function
//           int ItemIndex : item index of item to be saved
//           int SubItemIndex : subitem index of item to be saved
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SpecializedSavingFunction(HANDLE SavingHandle,int ItemIndex,int SubItemIndex)
{
    DWORD TextLen=this->GetItemTextLen(ItemIndex,SubItemIndex);
    TCHAR* psz=(TCHAR*)_alloca(TextLen*sizeof(TCHAR));
    this->GetItemText(ItemIndex,SubItemIndex,psz,TextLen);
    this->SaveContentForSpecializedSavingFunction(SavingHandle,psz);
}


//-----------------------------------------------------------------------------
// Name: SaveContentForSpecializedSavingFunction
// Object: used to write content of a SpecializedSavingFunction, convert pszContent
//         in xml, html, csv or txt according to user saving type choice
//         Notice : can be called more than once by the SpecializedSavingFunction
//         just use it as a WriteFile
// Parameters :
//     in  : HANDLE SavingHandle : handle provided by SpecializedSavingFunction
//           TCHAR* pszContent : content to write to file
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SaveContentForSpecializedSavingFunction(HANDLE SavingHandle,TCHAR* pszContent)
{
    if (IsBadReadPtr(pszContent,sizeof(TCHAR*)))
        return;

    SAVING_HANDLE* pSavingHandle=(SAVING_HANDLE*)SavingHandle;

    switch (pSavingHandle->SavingType)
    {
        // in case of XML and HTML saving, replace disturbing chars
    case SavingTypeXML:
    case SavingTypeHTML:
        {
            size_t TextLen=_tcslen(pszContent)+1;
            TCHAR* pszOut=(TCHAR*)_alloca(TextLen*sizeof(TCHAR)*CListview_REPLACED_CHAR_MAX_INCREASE);
            this->Convert(pszContent,pszOut,TRUE);
            CTextFile::WriteText(pSavingHandle->hFile,pszOut);
        }
        break;
    case SavingTypeCSV:
        {
            if (_tcslen(pszContent)>0)
            {
                char* pszAnsi=0;
                size_t nbChar=_tcslen(pszContent)+1;
                pszAnsi = (char*)_alloca(nbChar*2);
#if (defined(UNICODE)||defined(_UNICODE))
                WideCharToMultiByte(CP_ACP, 0, pszContent, nbChar, pszAnsi,nbChar, NULL, NULL);
#else
                _tcscpy(pszAnsi,pszContent);
#endif
                if (pszAnsi)
                {
                    DWORD NbBytesWritten;
                    char* pc;
                    // replace " by ""
                    pc=strtok(pszAnsi,"\"");
                    while(pc)
                    {
                        WriteFile(pSavingHandle->hFile,pc,(DWORD)strlen(pc),&NbBytesWritten,NULL);
                        pc=strtok(NULL,"\"");
                        if (pc)
                            WriteFile(pSavingHandle->hFile,"\"\"",2,&NbBytesWritten,NULL);
                    }
                }
            }
        }
        break;
    default:
        CTextFile::WriteText(pSavingHandle->hFile,pszContent);
        break;
    }
}

//-----------------------------------------------------------------------------
// Name: Load
// Object: load content of the file and put it in listview. This function shows 
//         an interface to user for choosing file
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Load()
{
    TCHAR pszFile[MAX_PATH]=_T("");

    // open dialog
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=NULL;
    ofn.hInstance=NULL;
    ofn.lpstrFilter=_T("xml\0*.xml\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt=_T("xml");
    ofn.lpstrFile=pszFile;
    ofn.nMaxFile=MAX_PATH;
    
    if (!GetOpenFileName(&ofn))
        return TRUE;

    return this->Load(ofn.lpstrFile);
}

//-----------------------------------------------------------------------------
// Name: Load
// Object: load content of the file and put it in listview
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Load(TCHAR* pszFileName)
{
    TCHAR pszMsg[MAX_PATH];
    TCHAR* psz=NULL;
    TCHAR* pszTagBegin;
    TCHAR* pszTagEnd;
    TCHAR* pszChildTagEnd;
    TCHAR* pszChildTagBegin;
    TCHAR* pszCurrentFilePos;
    TCHAR* pszOut;

    DWORD dwCurrentRow;
    DWORD dwCurrentColumn;
    BOOL bUnicodeFile=FALSE;
    TCHAR* pszFile;
    DWORD dwpszSize;
    dwpszSize=2048;
    

    if (!pszFileName)
        return FALSE;

    if (!*pszFileName)
        return FALSE;

    this->Clear();

    
    if (!CTextFile::Read(pszFileName,&pszFile,&bUnicodeFile))
        return FALSE;

    // search Listview tag
    pszCurrentFilePos=pszFile;
    pszTagBegin=_tcsstr(pszCurrentFilePos,_T("<LISTVIEW>"));
    if (!pszTagBegin)
    {
        _tcscpy(pszMsg,_T("Syntax error in file "));
        _tcscat(pszMsg,pszFileName);
        _tcscat(pszMsg,_T("\r\nNo LISTVIEW Tag found"));
        MessageBox(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }
    // go after Listview tag
    pszCurrentFilePos+=10;
    // search end of listview tag
    pszTagEnd=_tcsstr(pszCurrentFilePos,_T("</LISTVIEW>"));
    if (!pszTagEnd)
    {
        _tcscpy(pszMsg,_T("Syntax error in file "));
        _tcscat(pszMsg,pszFileName);
        _tcscat(pszMsg,_T("\r\nNo LISTVIEW end Tag found"));
        MessageBox(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
        return FALSE;
    }
    // ends string at begin of listviw tag end
    *pszTagEnd=0;

    // loop into row
    pszCurrentFilePos=_tcsstr(pszCurrentFilePos,_T("<ROW>"));
    dwCurrentRow=0;
    psz=new TCHAR[dwpszSize];
    while (pszCurrentFilePos>0)
    {
        // go after Row tag
        pszCurrentFilePos+=5;
        // search end of row tag
        pszTagEnd=_tcsstr(pszCurrentFilePos,_T("</ROW>"));
        if (!pszTagEnd)
        {
            _tcscpy(pszMsg,_T("Syntax error in file "));
            _tcscat(pszMsg,pszFileName);
            _tcscat(pszMsg,_T("\r\nNo ROW end Tag found"));
            MessageBox(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
            delete[] psz;
            return FALSE;
        }
        // search begin of column tag
        pszChildTagBegin=_tcsstr(pszCurrentFilePos,_T("<COLUMN>"));
        // loop into columns
        dwCurrentColumn=0;
        while ((pszTagEnd>pszChildTagBegin)&&(pszChildTagBegin))
        {
            // go after column tag
            pszCurrentFilePos+=8;
            // search end of column tag
            pszChildTagEnd=_tcsstr(pszCurrentFilePos,_T("</COLUMN>"));
            if(!pszChildTagEnd)
            {
                if (psz)
                    delete[] psz;

                _tcscpy(pszMsg,_T("Syntax error in file "));
                _tcscat(pszMsg,pszFileName);
                _tcscat(pszMsg,_T("\r\nNo COLUMN end Tag found"));
                MessageBox(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
                return FALSE;
            }
            if ((DWORD)(pszChildTagEnd-pszCurrentFilePos+1)>dwpszSize)
            {
                // if already allocated
                if (psz)
                    delete[] psz;

                // allocate new size
                dwpszSize=(DWORD)(pszChildTagEnd-pszCurrentFilePos+1);
                psz=new TCHAR[dwpszSize];
            }
            _tcsncpy(psz,pszCurrentFilePos,pszChildTagEnd-pszCurrentFilePos);
            psz[pszChildTagEnd-pszCurrentFilePos]=0;
            pszOut=new TCHAR[pszChildTagEnd-pszCurrentFilePos+1];
            // convert xml-like back to normal text
            this->Convert(psz,pszOut,FALSE);
            this->SetItemText(dwCurrentRow,dwCurrentColumn,pszOut);
            delete[] pszOut;

            // go after Column end tag 
            pszCurrentFilePos=pszChildTagEnd+9;
            // search begin of next column tag
            pszChildTagBegin=_tcsstr(pszCurrentFilePos,_T("<COLUMN>"));
            dwCurrentColumn++;
        } // end of column loop

        // go after row tag end
        pszCurrentFilePos=pszTagEnd+6;
        // search next row
        pszCurrentFilePos=_tcsstr(pszCurrentFilePos,_T("<ROW>"));
        dwCurrentRow++;
    } // end of row loop

    if (psz)
        delete[] psz;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Convert
// Object: convert string in xml like or xml like to string depending bToXML
//          Warning pszOutputString must be large enough no check is done
// Parameters :
//     in  : TCHAR* pszInputString : string to translate
//           BOOL bToXML : TRUE to convert to xml, FALSE to convert from XML
//     out : TCHAR* pszOutputString : result string
//     return : 
//-----------------------------------------------------------------------------
void CListview::Convert(TCHAR* pszInputString,TCHAR* pszOutputString,BOOL bToXML)
{
    if ((!pszInputString)||(!pszOutputString))
        return;

    // try to earn some time
    if (*pszInputString==0)
    {
        *pszOutputString=0;
        return;
    }

    int IndexOriginalString;
    int IndexReplaceString;

    TCHAR* Tmp=new TCHAR[_tcslen(pszInputString)*CListview_REPLACED_CHAR_MAX_INCREASE+1];

    if (bToXML)
    {
        IndexOriginalString=0;
        IndexReplaceString=1;
    }
    else
    {
        IndexOriginalString=1;
        IndexReplaceString=0;
    }

    _tcscpy(Tmp,pszInputString);

    // replace all strings defined in ppszReplacedChar
    for (int cnt=0;cnt<CListview_REPLACED_CHAR_ARRAY_SIZE;cnt++)
    {
        StrReplace(
            Tmp,
            pszOutputString,
            pppszReplacedChar[cnt][IndexOriginalString],
            pppszReplacedChar[cnt][IndexReplaceString]
            );
        _tcscpy(Tmp,pszOutputString);
    }

    delete[] Tmp;
}

//-----------------------------------------------------------------------------
// Name: StrReplace
// Object: replace pszOldStr by pszNewStr in pszInputString and put the result in 
//         pszOutputString.
//         Warning pszOutputString must be large enough no check is done
// Parameters :
//     in  : TCHAR* pszInputString : string to translate
//           TCHAR* pszOldStr : string to replace
//           TCHAR* pszNewStr : replacing string
//     out : TCHAR* pszOutputString : result string
//     return : 
//-----------------------------------------------------------------------------
void CListview::StrReplace(TCHAR* pszInputString,TCHAR* pszOutputString,TCHAR* pszOldStr,TCHAR* pszNewStr)
{
    TCHAR* pszPos;
    TCHAR* pszOldPos;
    int SearchedItemSize;

    if ((!pszInputString)||(!pszOutputString)||(!pszOldStr)||(!pszNewStr))
        return;

    *pszOutputString=0;

    pszOldPos=pszInputString;
    // get searched item size
    SearchedItemSize=(int)_tcslen(pszOldStr);
    // look for next string to replace
    pszPos=_tcsstr(pszInputString,pszOldStr);
    while(pszPos)
    {
        // copy unchanged data
        _tcsncat(pszOutputString,pszOldPos,pszPos-pszOldPos);
        // copy replace string
        _tcscat(pszOutputString,pszNewStr);
        // update old position
        pszOldPos=pszPos+SearchedItemSize;
        // look for next string to replace
        pszPos=_tcsstr(pszOldPos,pszOldStr);
    }
    // copy remaining data
    _tcscat(pszOutputString,pszOldPos);
}