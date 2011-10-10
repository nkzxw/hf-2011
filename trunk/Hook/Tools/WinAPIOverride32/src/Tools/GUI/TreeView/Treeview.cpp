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
#include "treeview.h"

CTreeview::CTreeview(HWND hWndTreeView,BOOL HasCheckBoxes)
{
    this->CommonConstructor();
    this->hWndControl=hWndTreeView;
    if (HasCheckBoxes)
    {
        LONG_PTR Value;
        Value=GetWindowLongPtr(hWndTreeView,GWL_STYLE);
        SetWindowLongPtr(hWndTreeView,GWL_STYLE,Value | TVS_CHECKBOXES);
    }
}

CTreeview::CTreeview(HWND hWndTreeView)
{
    this->CommonConstructor();
    this->hWndControl=hWndTreeView;
}
void CTreeview::CommonConstructor()
{
    this->hWndControl=0;
    this->hImgListNormal=NULL;
    *this->pszFileName=0;
    this->SavingType=SAVING_TYPE_TXT;
    this->hFile=INVALID_HANDLE_VALUE;
    this->CheckedStateChangedCallBack=NULL;
    this->CheckedStateChangedCallBackUserParam=NULL;
    this->PopUpMenuItemClickCallback=NULL;
    this->PopUpMenuItemClickCallbackUserParam=NULL;
    this->SelectedItemChangedCallback=NULL;
    this->SelectedItemChangedCallbackUserParam=NULL;

    // pop up menu
    this->PopUpMenuEnabled=TRUE;
    this->pPopUpMenu=new CPopUpMenu();
    this->MenuIdCopySelected=this->pPopUpMenu->Add(_T("Copy Selected"));
    this->pPopUpMenu->AddSeparator();
    this->MenuIdSaveSelected=this->pPopUpMenu->Add(_T("Export Selected"));
    // this->MenuIdSaveSelectedExpanded=this->pPopUpMenu->Add(_T("Export Selected (Expanded Only)"));
    this->MenuIdSaveExpanded=this->pPopUpMenu->Add(_T("Export Expanded"));
    this->MenuIdSaveAll=this->pPopUpMenu->Add(_T("Export All"));
}
CTreeview::~CTreeview(void)
{
    this->CheckedStateChangedCallBack=NULL;
    this->PopUpMenuItemClickCallback=NULL;
    if (this->pPopUpMenu)
        delete this->pPopUpMenu;
    if (this->hImgListNormal)
        ImageList_Destroy(this->hImgListNormal);
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
void CTreeview::SetPopUpMenuItemClickCallback(tagPopUpMenuItemClickCallback PopUpMenuItemClickCallback,LPVOID UserParam)
{
    this->PopUpMenuItemClickCallback=PopUpMenuItemClickCallback;
    this->PopUpMenuItemClickCallbackUserParam=UserParam;
}

//-----------------------------------------------------------------------------
// Name: GetControlHandle
// Object: return treeview handle
// Parameters :
//     in  : 
//     out : 
//     return : treeview handle
//-----------------------------------------------------------------------------
HWND CTreeview::GetControlHandle()
{
    return this->hWndControl;
}

void CTreeview::EnablePopUpMenu(BOOL bEnable)
{
    this->PopUpMenuEnabled=bEnable;
}

//-----------------------------------------------------------------------------
// Name: Clear
// Object: remove all nodes
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Clear()
{
    return TreeView_DeleteAllItems(this->hWndControl);
}

//-----------------------------------------------------------------------------
// Name: Remove
// Object: remove specified node
// Parameters :
//     in  : HTREEITEM hItem : node to remove
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Remove(HTREEITEM hItem)
{
    return TreeView_DeleteItem(this->hWndControl,hItem);
}

//-----------------------------------------------------------------------------
// Name: GetParent
// Object: return parent node handle
// Parameters :
//     in  : HTREEITEM hItem : current node
//     out : 
//     return : parent node handle
//-----------------------------------------------------------------------------
HTREEITEM CTreeview::GetParent(HTREEITEM hItem)
{
    return TreeView_GetParent(this->hWndControl,hItem);
}

//-----------------------------------------------------------------------------
// Name: GetParent
// Object: return previous sibling node handle
// Parameters :
//     in  : HTREEITEM hItem : current node
//     out : 
//     return : previous sibling node handle
//-----------------------------------------------------------------------------
HTREEITEM CTreeview::GetPrevious(HTREEITEM hItem)
{
    return TreeView_GetPrevSibling(this->hWndControl,hItem);
}

//-----------------------------------------------------------------------------
// Name: GetParent
// Object: return next sibling node handle
// Parameters :
//     in  : HTREEITEM hItem : current node
//     out : 
//     return : next sibling node handle
//-----------------------------------------------------------------------------
HTREEITEM CTreeview::GetNext(HTREEITEM hItem)
{
    return TreeView_GetNextSibling(this->hWndControl,hItem);
}

//-----------------------------------------------------------------------------
// Name: GetChild
// Object: return first child node handle
// Parameters :
//     in  : HTREEITEM hItem : current node
//     out : 
//     return : first child node handle
//-----------------------------------------------------------------------------
HTREEITEM CTreeview::GetChild(HTREEITEM hItem)
{
    return TreeView_GetChild(this->hWndControl,hItem);
}

//-----------------------------------------------------------------------------
// Name: SetItemIconIndex
// Object: use icon index in icon list for specified item
// Parameters :
//     in  : HTREEITEM node handle
//           int IconIndex : unselected and selected icon index
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::SetItemIconIndex(HTREEITEM hItem,int IconIndex)
{
    return this->SetItemIconIndex(hItem,IconIndex,IconIndex);
}
//-----------------------------------------------------------------------------
// Name: SetItemIconIndex
// Object: use icon index in icon list for specified item
// Parameters :
//     in  : HTREEITEM node handle
//           int IconIndex : unselected icon index
//           int SelectedIconIndex : selected icon index
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::SetItemIconIndex(HTREEITEM hItem,int IconIndex,int SelectedIconIndex)
{
    TVITEM Item={0};
    Item.mask=TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    Item.hItem=hItem;
    Item.iImage=IconIndex;
    Item.iSelectedImage=SelectedIconIndex;

    return TreeView_SetItem(this->hWndControl,&Item);
}

//-----------------------------------------------------------------------------
// Name: AddIcon
// Object: add an icon to the image list
// Parameters :
//     in  : 
//     out : 
//     return : index of icon in image list
//-----------------------------------------------------------------------------
int CTreeview::AddIcon(HMODULE hModule,int IdIcon)
{
    HICON hicon=(HICON)LoadImage(hModule, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,0,0,LR_SHARED);
    if (!hicon)
        return -1;

    return this->AddIcon(hicon);
}
//-----------------------------------------------------------------------------
// Name: AddIcon
// Object: add an icon to the image list
// Parameters :
//     in  : 
//     out : 
//     return : index of icon in image list
//-----------------------------------------------------------------------------
int CTreeview::AddIcon(HICON hIcon)
{
    if (!this->hImgListNormal)
    {
        // create it
        this->hImgListNormal=ImageList_Create(16, 16, ILC_MASK|ILC_COLOR32, 20, 5);
        if (!this->hImgListNormal)
            return -1;
        // associate it to corresponding list
        TreeView_SetImageList(this->hWndControl,this->hImgListNormal,TVSIL_NORMAL);
    }
    // add icon to list
    return ImageList_AddIcon(this->hImgListNormal, hIcon);
}
//-----------------------------------------------------------------------------
// Name: RemoveIcon
// Object: remove an icon from the image list
// Parameters :
//     in  : index of icon in image list
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CTreeview::RemoveIcon(int IconIndex)
{
    return ImageList_Remove(this->hImgListNormal,IconIndex);
}
//-----------------------------------------------------------------------------
// Name: RemoveAllIcons
// Object: remove all icons from the image list
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CTreeview::RemoveAllIcons()
{
    return this->RemoveIcon(-1);
}

//-----------------------------------------------------------------------------
// Name: SetCheckedStateChangedCallback
// Object: add call back from items checked changed state
// Parameters :
//     in  : tagCheckedStateChangedCallback CheckedStateChangedCallBack : call back
//     out : LPVOID UserParam : user param
//     return : TRUE on success
//-----------------------------------------------------------------------------
void CTreeview::SetCheckedStateChangedCallback(tagCheckedStateChangedCallback CheckedStateChangedCallBack,LPVOID UserParam)
{
    this->CheckedStateChangedCallBack=CheckedStateChangedCallBack;
    this->CheckedStateChangedCallBackUserParam=UserParam;
}

//-----------------------------------------------------------------------------
// Name: SetSelectedItemChangedCallback
// Object: add call back from items checked changed state
// Parameters :
//     in  : tagCheckedStateChangedCallback CheckedStateChangedCallBack : call back
//     out : LPVOID UserParam : user param
//     return : TRUE on success
//-----------------------------------------------------------------------------
void CTreeview::SetSelectedItemChangedCallback(tagSelectedItemChangedCallback SelectedItemChangedCallback,LPVOID UserParam)
{
    this->SelectedItemChangedCallback=SelectedItemChangedCallback;
    this->SelectedItemChangedCallbackUserParam=UserParam;
}

//-----------------------------------------------------------------------------
// Name: SetCheckedState
// Object: set item checked state
// Parameters :
//     in  : HTREEITEM hItem : item handle
//     out : tagCheckState CheckState : checked state
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::SetCheckedState(HTREEITEM hItem,tagCheckState CheckState)
{
    TVITEM tvItem;
    tvItem.mask = TVIF_HANDLE | TVIF_STATE;
    tvItem.hItem = hItem;
    tvItem.stateMask = TVIS_STATEIMAGEMASK;
    // 0 to remove check img
    // 1 unchecked
    // 2 checked
    tvItem.state = INDEXTOSTATEIMAGEMASK(CheckState);
    return TreeView_SetItem(this->hWndControl, &tvItem);
}

//-----------------------------------------------------------------------------
// Name: GetCheckedState
// Object: get item checked state
// Parameters :
//     in  : HTREEITEM hItem : item handle
//     out : tagCheckState* pCheckState : checked state
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::GetCheckedState(HTREEITEM hItem,tagCheckState* pCheckState)
{
    TVITEM tvItem;
    *pCheckState=CHECK_STATE_UNCHECKED;

    // Prepare to receive the desired information.
    tvItem.mask = TVIF_HANDLE | TVIF_STATE;
    tvItem.hItem = hItem;
    tvItem.stateMask = TVIS_STATEIMAGEMASK;

    // Request the information.
    if (!TreeView_GetItem(this->hWndControl, &tvItem))
        return FALSE;

    *pCheckState=(tagCheckState)(tvItem.state >> 12);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: IsChecked
// Object: check if an item is checked
// Parameters :
//     in  : HTREEITEM hItem : item handle
//     out : 
//     return : TRUE if item is checked
//-----------------------------------------------------------------------------
BOOL CTreeview::IsChecked(HTREEITEM hItem)
{
    tagCheckState CheckState;
    if (!this->GetCheckedState(hItem,&CheckState))
        return FALSE;
    return (CheckState==CHECK_STATE_CHECKED);
}

//-----------------------------------------------------------------------------
// Name: OnNotify
// Object: a WM_NOTIFY message helper for tree view
//         Must be called in the WndProc when receiving a WM_NOTIFY message 
//         EXAMPLE :
//              LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//              switch (uMsg)
//              {
//                  case WM_NOTIFY:
//                      if (pTreeview)
//                      {
//                          if (pTreeview->OnNotify(wParam,lParam))
//                              return TRUE;
//                      }
//                      break;
//              }
// Parameters :
//     in  : WPARAM wParam : WndProc wParam
//           LPARAM lParam : WndProc lParam
//     out : 
//     return : TRUE if message have been internally proceed
//-----------------------------------------------------------------------------
BOOL CTreeview::OnNotify(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    LPNMHDR lpnmh = (LPNMHDR) lParam;
    if (lpnmh->hwndFrom != this->hWndControl)
        return FALSE;


    ///////////////////////////////////
    // find item on witch event occurs
    ///////////////////////////////////
    TVHITTESTINFO ht = {0};
    DWORD dwpos = GetMessagePos();

    // include <windowsx.h> and <windows.h> header files
    ht.pt.x = GET_X_LPARAM(dwpos);
    ht.pt.y = GET_Y_LPARAM(dwpos);
    MapWindowPoints(HWND_DESKTOP, lpnmh->hwndFrom, &ht.pt, 1);

    TreeView_HitTest(lpnmh->hwndFrom, &ht);
    ///////////////////////////////////
    // end of find item on witch event occurs
    ///////////////////////////////////


    switch(lpnmh->code)
    {
    // use TVN_SELCHANGEDA && TVN_SELCHANGEDW instead of TVN_SELCHANGED 
    // because even your application is built using on convention m$ is happy to load it's one convention
    // which can be the contrary of current application build specification
    case TVN_SELCHANGEDA:
        {
            LPNMTREEVIEWA pnmtv = (LPNMTREEVIEWA) lParam;
            if (this->SelectedItemChangedCallback)
            {
                if (!IsBadCodePtr((FARPROC)this->SelectedItemChangedCallback))
                {
#if (defined(UNICODE)||defined(_UNICODE))
                    NMTREEVIEWW nmtvU = *(NMTREEVIEWW*)pnmtv;
                    if (pnmtv->itemNew.mask & TVIF_TEXT)
                        CAnsiUnicodeConvert::AnsiToUnicode(pnmtv->itemNew.pszText,&nmtvU.itemNew.pszText);
                    if (pnmtv->itemOld.mask & TVIF_TEXT)
                        CAnsiUnicodeConvert::AnsiToUnicode(pnmtv->itemOld.pszText,&nmtvU.itemOld.pszText);
                    this->SelectedItemChangedCallback(&nmtvU,this->SelectedItemChangedCallbackUserParam);
                    if (pnmtv->itemNew.mask & TVIF_TEXT)
                    {
                        if (nmtvU.itemNew.pszText)
                            free(nmtvU.itemNew.pszText);
                    }
                    if (pnmtv->itemOld.mask & TVIF_TEXT)
                    {
                        if (nmtvU.itemOld.pszText)
                            free(nmtvU.itemOld.pszText);
                    }
#else
                    this->SelectedItemChangedCallback(pnmtv,this->SelectedItemChangedCallbackUserParam);
#endif
                }
            }
        }
        break;
    case TVN_SELCHANGEDW:
        {
            LPNMTREEVIEWW pnmtv = (LPNMTREEVIEWW) lParam;
            if (this->SelectedItemChangedCallback)
            {
                if (!IsBadCodePtr((FARPROC)this->SelectedItemChangedCallback))
                {
#if (defined(UNICODE)||defined(_UNICODE))
                    this->SelectedItemChangedCallback(pnmtv,this->SelectedItemChangedCallbackUserParam);
#else
                    NMTREEVIEWA nmtvA = *(NMTREEVIEWA*)pnmtv;
                    if (pnmtv->itemNew.mask & TVIF_TEXT)
                        CAnsiUnicodeConvert::UnicodeToAnsi(pnmtv->itemNew.pszText,&nmtvA.itemNew.pszText);
                    if (pnmtv->itemOld.mask & TVIF_TEXT)
                        CAnsiUnicodeConvert::UnicodeToAnsi(pnmtv->itemOld.pszText,&nmtvA.itemOld.pszText);
                    this->SelectedItemChangedCallback(&nmtvA,this->SelectedItemChangedCallbackUserParam);
                    if (pnmtv->itemNew.mask & TVIF_TEXT)
                    {
                        if (nmtvA.itemNew.pszText)
                            free(nmtvA.itemNew.pszText);
                    }
                    if (pnmtv->itemOld.mask & TVIF_TEXT)
                    {
                        if (nmtvA.itemOld.pszText)
                            free(nmtvA.itemOld.pszText);
                    }
#endif
                }
            }
        }
        break;
    case NM_CLICK:
        // if cursor on state icon
        if (ht.flags & TVHT_ONITEMSTATEICON)
        {
            if (this->CheckedStateChangedCallBack)
            {
                if (!IsBadCodePtr((FARPROC)this->CheckedStateChangedCallBack))
                {
                    this->CheckedStateChangedCallBack(ht.hItem,this->CheckedStateChangedCallBackUserParam);
                }
            }
        }
        break;
    case NM_RCLICK:
        // if cursor on state item
        if (ht.flags & TVHT_ONITEM)
        {
            if (this->PopUpMenuEnabled)
            {
                // select item
                this->SetSelectedItem(ht.hItem);

                LPNMITEMACTIVATE pnmitem;
                RECT Rect;
                UINT uiRetPopUpMenuItemID;
                pnmitem = (LPNMITEMACTIVATE) lParam;
                // get listview position
                GetWindowRect(this->hWndControl,&Rect);
                // show popupmenu
                uiRetPopUpMenuItemID=this->pPopUpMenu->Show(Rect.left+ht.pt.x,
                                                            Rect.top+ht.pt.y,
                                                            this->hWndControl);
                if (uiRetPopUpMenuItemID==0)// no menu selected
                {
                    return TRUE;
                }
                else if (uiRetPopUpMenuItemID==this->MenuIdCopySelected)
                {
                    DWORD Size=this->GetItemTextLen(ht.hItem);
                    TCHAR* psz=(TCHAR*)_alloca(Size*sizeof(TCHAR));
                    this->GetItemText(ht.hItem,psz,Size);
                    CClipboard::CopyToClipboard(this->hWndControl,psz);
                    return TRUE;
                }

                else if (uiRetPopUpMenuItemID==this->MenuIdSaveSelected)
                {
                    this->Save(FALSE,TRUE);
                    return TRUE;
                }
                else if (uiRetPopUpMenuItemID==this->MenuIdSaveExpanded)
                {
                    this->Save(TRUE,FALSE);
                    return TRUE;
                }
                else if (uiRetPopUpMenuItemID==this->MenuIdSaveAll)
                {
                    this->Save(FALSE,FALSE);
                    return TRUE;
                }
                // menu is not processed internally
                if (!IsBadCodePtr((FARPROC)this->PopUpMenuItemClickCallback))
                    this->PopUpMenuItemClickCallback(uiRetPopUpMenuItemID,this->PopUpMenuItemClickCallbackUserParam);
                return TRUE;
            }
        }
        break;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Name: SetItemUserData
// Object: set item user data
// Parameters :
//     in  : HTREEITEM hItem : item handle
//           LPVOID UserData
//     out : 
//     return : TRUE if item is check state has changed
//-----------------------------------------------------------------------------
BOOL CTreeview::SetItemUserData(HTREEITEM hItem,LPVOID UserData)
{
    TV_ITEM Item={0};
    Item.mask=LVIF_PARAM;
    Item.hItem=hItem;
    // try to get our own object associated with item
    if (!TreeView_GetItem(this->hWndControl,&Item))
        return FALSE;

    Item.lParam=(LPARAM)UserData;
    return TreeView_SetItem(this->hWndControl,&Item);
}
//-----------------------------------------------------------------------------
// Name: GetItemUserData
// Object: Get user data associated with item
// Parameters :
//     in  : HTREEITEM hItem : handle of item
//           LPVOID* pUserData : pointer to value to retrieve
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CTreeview::GetItemUserData(HTREEITEM hItem,LPVOID* pUserData)
{
    // check pointer
    if (IsBadWritePtr(pUserData,sizeof(LPVOID)))
        return FALSE;

    TV_ITEM Item={0};
    Item.mask=LVIF_PARAM;
    Item.hItem=hItem;
    // try to get our own object associated with item
    if (!TreeView_GetItem(this->hWndControl,&Item))
        return FALSE;

    *pUserData=(LPVOID)Item.lParam;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetBoldState
// Object: set bold state
// Parameters :
//     in  : HTREEITEM hItem : item handle
//           BOOL bBold : TRUE to set bold font, FALSE to remove bold font
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::SetBoldState(HTREEITEM hItem,BOOL bBold)
{
    TVITEM tvItem;
    tvItem.mask = TVIF_HANDLE | TVIF_STATE;
    tvItem.hItem = hItem;
    tvItem.stateMask = TVIS_BOLD;
    tvItem.state = (bBold ? TVIS_BOLD:0);
    return TreeView_SetItem(this->hWndControl, &tvItem);
}

//-----------------------------------------------------------------------------
// Name: GetRoot
// Object: return root node handle
// Parameters :
//     in  : 
//     out : 
//     return : root node handle
//-----------------------------------------------------------------------------
HTREEITEM CTreeview::GetRoot()
{
    return TreeView_GetRoot(this->hWndControl);
}

//-----------------------------------------------------------------------------
// Name: Expand
// Object: Expand specified node
// Parameters :
//     in  : HTREEITEM hItem : handle of item to expand
//           BOOL FullBranch : TRUE to expand all subitems
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Expand(HTREEITEM hItem,BOOL FullBranch)
{
    // expand current item
    BOOL bRet=TreeView_Expand(this->hWndControl,hItem,TVE_EXPAND);
    if (!FullBranch)
        return bRet;

    HTREEITEM hChild;
    hChild=TreeView_GetChild(this->hWndControl,hItem);
    while (hChild)
    {
        this->Expand(hChild,TRUE);
        hChild=TreeView_GetNextSibling(this->hWndControl,hChild);
    }

    return bRet;
}
//-----------------------------------------------------------------------------
// Name: Expand
// Object: Expand specified item
// Parameters :
//     in  : HTREEITEM hItem : handle of item to expand
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Expand(HTREEITEM hItem)
{
    return this->Expand(hItem,FALSE);
}
//-----------------------------------------------------------------------------
// Name: Expand
// Object: fully Expand the treeview
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Expand()
{
    return this->Expand(this->GetRoot(),TRUE);
}

//-----------------------------------------------------------------------------
// Name: Collapse
// Object: collapse specified all nodes of treeview
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Collapse()
{
    return this->Collapse(this->GetRoot(),TRUE);
}

//-----------------------------------------------------------------------------
// Name: Collapse
// Object: collapse specified node
// Parameters :
//     in  : HTREEITEM hItem : handle of item to collapse
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Collapse(HTREEITEM hItem)
{
    return this->Collapse(hItem,FALSE);
}

//-----------------------------------------------------------------------------
// Name: Collapse
// Object: collapse specified node
// Parameters :
//     in  : HTREEITEM hItem : handle of item to collapse
//           BOOL FullBranch : TRUE to collapse all subitems
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Collapse(HTREEITEM hItem,BOOL FullBranch)
{
    // expand current item
    BOOL bRet=TreeView_Expand(this->hWndControl,hItem,TVE_COLLAPSE);
    if (!FullBranch)
        return bRet;

    HTREEITEM hChild;
    hChild=TreeView_GetChild(this->hWndControl,hItem);
    while (hChild)
    {
        this->Collapse(hChild,TRUE);
        hChild=TreeView_GetNextSibling(this->hWndControl,hChild);
    }

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: Insert
// Object: insert an item into treeview
// Parameters :
//     in  : HTREEITEM hParent : parent node. If TVI_ROOT value or NULL, the item is inserted at the root of the tree-view
//           TCHAR* Text : node text
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
HTREEITEM CTreeview::Insert(HTREEITEM hParent,TCHAR* Text)
{
    return this->Insert(hParent,Text,NULL);
}

//-----------------------------------------------------------------------------
// Name: Insert
// Object: insert an item into treeview
// Parameters :
//     in  : HTREEITEM hParent : parent node. If TVI_ROOT value or NULL, the item is inserted at the root of the tree-view
//           TCHAR* Text : node text
//           LPARAM UserParam : user param
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
HTREEITEM CTreeview::Insert(HTREEITEM hParent,TCHAR* Text,LPARAM UserParam)
{
    return this->Insert(hParent,TVI_LAST,Text,UserParam);
}

//-----------------------------------------------------------------------------
// Name: Insert
// Object: insert an item into treeview at specified position
// Parameters :
//     in  : HTREEITEM hParent : parent node. If TVI_ROOT value or NULL, the item is inserted at the root of the tree-view
//           HTREEITEM hInsertAfter : Handle to the item after which the new item is to be inserted, or one of the following values:
//                                    TVI_FIRST :Inserts the item at the beginning of the list.
//                                    TVI_LAST : Inserts the item at the end of the list.
//                                    TVI_ROOT : Add the item as a root item.
//                                    TVI_SORT : Inserts the item into the list in alphabetical order.
//           TCHAR* Text : node text
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
HTREEITEM CTreeview::Insert(HTREEITEM hParent,HTREEITEM hInsertAfter,TCHAR* Text)
{
    return this->Insert(hParent,hInsertAfter,Text,NULL);
}
//-----------------------------------------------------------------------------
// Name: Insert
// Object: insert an item into treeview at specified position
// Parameters :
//     in  : HTREEITEM hParent : parent node. If TVI_ROOT value or NULL, the item is inserted at the root of the tree-view
//           HTREEITEM hInsertAfter : Handle to the item after which the new item is to be inserted, or one of the following values:
//                                    TVI_FIRST :Inserts the item at the beginning of the list.
//                                    TVI_LAST : Inserts the item at the end of the list.
//                                    TVI_ROOT : Add the item as a root item.
//                                    TVI_SORT : Inserts the item into the list in alphabetical order.
//           TCHAR* Text : node text
//           LPARAM UserParam : user param
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
HTREEITEM CTreeview::Insert(HTREEITEM hParent,HTREEITEM hInsertAfter,TCHAR* Text,LPARAM UserParam)
{
    TV_INSERTSTRUCT InsertStruct={0};
    InsertStruct.hParent=hParent;
    InsertStruct.hInsertAfter=hInsertAfter;
    InsertStruct.itemex.mask=TVIF_TEXT|TVIF_PARAM;
    InsertStruct.itemex.pszText=Text;
    InsertStruct.itemex.lParam=UserParam;
    return TreeView_InsertItem(this->hWndControl,&InsertStruct);
}

//-----------------------------------------------------------------------------
// Name: SetItemText
// Object: set item text
// Parameters :
//     in  : 
//           HTREEITEM hItem : tree item
//           TCHAR* Text : Text
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::SetItemText(HTREEITEM hItem,TCHAR* Text)
{
    TV_ITEM tvi={0};
    tvi.hItem = hItem;
    tvi.mask = TVIF_TEXT;
    tvi.pszText = Text;
    return TreeView_SetItem(this->hWndControl,&tvi);
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: Parse all tree
// Parameters :
//     in  : 
//           pfParseCallBack ParseCallBack : user callback
//           LPVOID UserParam : user param provided to callback
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Parse(pfParseCallBack ParseCallBack,LPVOID UserParam)
{
    return this->Parse(this->GetRoot(),ParseCallBack,UserParam);
}
//-----------------------------------------------------------------------------
// Name: Parse
// Object: Parse tree from specified item
// Parameters :
//     in  : HTREEITEM hItem : handle of item from which we want to parse
//           pfParseCallBack ParseCallBack : user callback
//           LPVOID UserParam : user param provided to callback
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Parse(HTREEITEM hItem,pfParseCallBack ParseCallBack,LPVOID UserParam)
{
    return this->Parse(hItem,INFINITE,ParseCallBack,UserParam);
}
//-----------------------------------------------------------------------------
// Name: Parse
// Object: Parse tree from specified item checking depth
// Parameters :
//     in  : HTREEITEM hItem : handle of item from which we want to parse
//           DWORD MaxDepthFromItem : max depth to parse from specified item
//           pfParseCallBack ParseCallBack : user callback
//           LPVOID UserParam : user param provided to callback
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Parse(HTREEITEM hItem,DWORD MaxDepthFromItem,pfParseCallBack ParseCallBack,LPVOID UserParam)
{
    return this->Parse(hItem,MaxDepthFromItem,ParseCallBack,UserParam,FALSE);
}
//-----------------------------------------------------------------------------
// Name: Parse
// Object: Parse tree from specified item
// Parameters :
//     in  : HTREEITEM hItem : handle of item from which we want to parse
//           pfParseCallBack ParseCallBack : user callback
//           LPVOID UserParam : user param provided to callback
//           BOOL ReverseOrder : TRUE to parse in reverse order
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Parse(HTREEITEM hItem,DWORD MaxDepthFromItem,pfParseCallBack ParseCallBack,LPVOID UserParam,BOOL ReverseOrder)
{
    return this->Parse(hItem,MaxDepthFromItem,(DWORD)-1,ParseCallBack,UserParam,ReverseOrder);
}

//-----------------------------------------------------------------------------
// Name: ParseSpecifiedDepth
// Object: Parse tree from root and call callback only for items with specified depth
// Parameters :
//     in  : 
//           pfParseCallBack ParseCallBack : user callback
//           LPVOID UserParam : user param provided to callback
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::ParseSpecifiedDepth(DWORD SpecifiedDepthFromRoot,pfParseCallBack ParseCallBack,LPVOID UserParam)
{
    return this->ParseSpecifiedDepth(this->GetRoot(),SpecifiedDepthFromRoot,ParseCallBack,UserParam);
}
//-----------------------------------------------------------------------------
// Name: ParseSpecifiedDepth
// Object: Parse tree from specified item and call callback only for items with specified depth
// Parameters :
//     in  : HTREEITEM hItem : handle of item from which we want to parse
//           pfParseCallBack ParseCallBack : user callback
//           DWORD SpecifiedDepthFromRoot : 
//           LPVOID UserParam : user param provided to callback
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::ParseSpecifiedDepth(HTREEITEM hItem,DWORD SpecifiedDepthFromRoot,pfParseCallBack ParseCallBack,LPVOID UserParam)
{
    return this->ParseSpecifiedDepth(hItem,SpecifiedDepthFromRoot,ParseCallBack,UserParam,FALSE);
}
//-----------------------------------------------------------------------------
// Name: ParseSpecifiedDepth
// Object: Parse tree from specified item and call callback only for items with specified depth
// Parameters :
//     in  : HTREEITEM hItem : handle of item from which we want to parse
//           pfParseCallBack ParseCallBack : user callback
//           DWORD SpecifiedDepthFromRoot : 
//           LPVOID UserParam : user param provided to callback
//           BOOL ReverseOrder : TRUE if reverse order
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::ParseSpecifiedDepth(HTREEITEM hItem,DWORD SpecifiedDepthFromRoot,pfParseCallBack ParseCallBack,LPVOID UserParam,BOOL ReverseOrder)
{
    return this->Parse(hItem,INFINITE,SpecifiedDepthFromRoot,ParseCallBack,UserParam,ReverseOrder);
}

//-----------------------------------------------------------------------------
// Name: Parse
// Object: Parse tree from specified item checking depth
// Parameters :
//     in  : HTREEITEM hItem : handle of item from which we want to parse
//           DWORD MaxDepthFromItem : max depth to parse from specified item
//           DWORD SpecifiedDepthFromRoot : find only item at a specified depth (-1 if all depth allowed)
//           pfParseCallBack ParseCallBack : user callback
//           LPVOID UserParam : user param provided to callback
//           BOOL ReverseOrder : TRUE to parse treeview backward
//     out : 
//     return : TRUE on success, FALSE if parsing has been queried to be stopped
//-----------------------------------------------------------------------------
BOOL CTreeview::Parse(HTREEITEM hItem,DWORD MaxDepthFromItem,DWORD SpecifiedDepthFromRoot,pfParseCallBack ParseCallBack,LPVOID UserParam,BOOL ReverseOrder)
{
    BOOL bCallCallback;
    DWORD CurrentItemDepth;
    HTREEITEM hChild;
    HTREEITEM hNextChild;
    DWORD NextMaxDepthFromItem;
    if (MaxDepthFromItem!=INFINITE)
        NextMaxDepthFromItem=MaxDepthFromItem-1;
    else
        NextMaxDepthFromItem=MaxDepthFromItem;

    CurrentItemDepth=this->GetItemDepth(hItem);

    // call callback if no filter on item depth or if filter is ok
    bCallCallback=((SpecifiedDepthFromRoot==(DWORD)-1) || (CurrentItemDepth==SpecifiedDepthFromRoot));

    while(hItem)
    {
        // call call back
        if (bCallCallback)
        {
            if (!ParseCallBack(hItem,UserParam))
                return FALSE;
        }

        if ( (MaxDepthFromItem>0)
             && (CurrentItemDepth<SpecifiedDepthFromRoot)
           )
        {
            // for each child of current item

            if (ReverseOrder)
            {
                // find last child
                hChild=TreeView_GetChild(this->hWndControl,hItem);
                hNextChild=hChild;
                while (hNextChild)
                {
                    hChild=hNextChild;
                    hNextChild=TreeView_GetNextSibling(this->hWndControl,hNextChild);
                }
            }
            else
                // get first child
                hChild=TreeView_GetChild(this->hWndControl,hItem);

            if (hChild)
            {
                if (!this->Parse(hChild,NextMaxDepthFromItem,SpecifiedDepthFromRoot,ParseCallBack,UserParam,ReverseOrder))
                    return FALSE;
            }
        }

        // get next item
        if (ReverseOrder)
            hItem=TreeView_GetPrevSibling(this->hWndControl,hItem);
        else
            hItem=TreeView_GetNextSibling(this->hWndControl,hItem);
    }
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: Find
// Object: find first matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//           HTREEITEM hFromItem : first item checked
//           DWORD MaxDepthFromItem : max depth, INFINITE if no depth checking
//           DWORD SpecifiedDepthFromRoot : return only item at specified depth (-1 if no specified depth)
//           BOOL ReverseOrder : TRUE for backward search
//           BOOL AllowParentSearch : allow to search in parent node and sub nodes
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::Find(TCHAR* Str,HTREEITEM hFromItem,DWORD MaxDepthFromItem,DWORD SpecifiedDepthFromRoot,BOOL ReverseOrder,BOOL AllowParentSearch,HTREEITEM* phNextMatchingItem)
{
    *phNextMatchingItem=NULL;

    if (!hFromItem)
        return FALSE;

    FIND_STRUCT FindInfo;
    FindInfo.pTreeView=this;
    FindInfo.Str=Str;
    FindInfo.ItemFound=FALSE;
    FindInfo.hMatchingItem=NULL;
    
    this->Parse(hFromItem,MaxDepthFromItem,SpecifiedDepthFromRoot,CTreeview::FindCallBackStatic,&FindInfo,ReverseOrder);
    *phNextMatchingItem=FindInfo.hMatchingItem;
    if (FindInfo.ItemFound)
        return TRUE;

    if (!AllowParentSearch)
        return FALSE;
    
    // parse next / prev sibling
    HTREEITEM hNextItem;
    HTREEITEM hItem;
    HTREEITEM hParentItem;

    hItem=hFromItem;

    for(;;)
    {
        // get next / prev items
        if (ReverseOrder)
            hNextItem=TreeView_GetPrevSibling(this->hWndControl,hItem);
        else
            hNextItem=TreeView_GetNextSibling(this->hWndControl,hItem);

        while (hNextItem)
        {
            this->Parse(hNextItem,MaxDepthFromItem,SpecifiedDepthFromRoot,CTreeview::FindCallBackStatic,&FindInfo,ReverseOrder);
            *phNextMatchingItem=FindInfo.hMatchingItem;
            if (FindInfo.ItemFound)
                return TRUE;

            hItem=hNextItem;

            if (ReverseOrder)
                hNextItem=TreeView_GetPrevSibling(this->hWndControl,hItem);
            else
                hNextItem=TreeView_GetNextSibling(this->hWndControl,hItem);
        }

        // no more item --> get parent of last valid item
        hParentItem=this->GetParent(hItem);

        // if no parent
        if (!hParentItem)
            return FALSE;

        // update hitem
        hItem=hParentItem;
    }
}

//-----------------------------------------------------------------------------
// Name: FindCallBackStatic
// Object: static parse callback for find method
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CTreeview::FindCallBackStatic(HTREEITEM hItem,LPVOID UserParam)
{
    FIND_STRUCT* pFindStruct=(FIND_STRUCT*)UserParam;
    // reenter object model
    return pFindStruct->pTreeView->FindCallBack(hItem,pFindStruct);
}
//-----------------------------------------------------------------------------
// Name: FindCallBackStatic
// Object: parse callback for find method
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CTreeview::FindCallBack(HTREEITEM hItem,FIND_STRUCT* pFindStruct)
{
    // get item text length
    DWORD Size=this->GetItemTextLen(hItem)+1;// +1 for \0

    // allocate memory on stack
    TCHAR* psz=(TCHAR*)_alloca((Size)*sizeof(TCHAR));

    // get item text
    this->GetItemText(hItem,psz,Size);

    // if item match
    if (CWildCharCompare::WildICmp(pFindStruct->Str,psz))
    {
        // fill informations
        pFindStruct->ItemFound=TRUE;
        pFindStruct->hMatchingItem=hItem;

        // stop parsing
        return FALSE;
    }
    else
        // continue parsing
        return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Find
// Object: find first matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::Find(TCHAR* Str,HTREEITEM* phFirstMatchingItem)
{
    return this->Find(Str,INFINITE,phFirstMatchingItem);
}

//-----------------------------------------------------------------------------
// Name: Find
// Object: find first matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//           DWORD MaxDepthFromRoot : max depth from root
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::Find(TCHAR* Str,DWORD MaxDepthFromRoot,HTREEITEM* phFirstMatchingItem)
{
    HTREEITEM hItem=this->GetRoot();
    return this->Find(Str,hItem,MaxDepthFromRoot,(DWORD)-1,FALSE,FALSE,phFirstMatchingItem);
}

//-----------------------------------------------------------------------------
// Name: FindAtSpecifiedDepth
// Object: find first matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//           DWORD DepthFromRoot : specified depth from root
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::FindAtSpecifiedDepth(TCHAR* Str,DWORD DepthFromRoot,HTREEITEM* phFirstMatchingItem)
{
    HTREEITEM hItem=this->GetRoot();
    return this->Find(Str,hItem,INFINITE,DepthFromRoot,FALSE,FALSE,phFirstMatchingItem);
}
//-----------------------------------------------------------------------------
// Name: FindNextAtSpecifiedDepth
// Object: find next matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//           HTREEITEM hItem : search will begin with following item of hItem
//           DWORD SpecifiedDepthFromRoot : return only item at specified depth (-1 if no specified depth)
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::FindNextAtSpecifiedDepth(TCHAR* Str,HTREEITEM hItem,DWORD DepthFromRoot,HTREEITEM* phNextMatchingItem)
{
    return this->FindNextAtSpecifiedDepth(Str,hItem,DepthFromRoot,TRUE,phNextMatchingItem);
}
//-----------------------------------------------------------------------------
// Name: FindNextAtSpecifiedDepth
// Object: find next matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//           HTREEITEM hItem : search will begin with following item of hItem
//           DWORD SpecifiedDepthFromRoot : return only item at specified depth (-1 if no specified depth)
//           BOOL AllowParentSearch : allow search in parent (and parent sub nodes) of hItem
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::FindNextAtSpecifiedDepth(TCHAR* Str,HTREEITEM hItem,DWORD DepthFromRoot,BOOL AllowParentSearch,HTREEITEM* phNextMatchingItem)
{
    return this->FindNext(Str,hItem,INFINITE,DepthFromRoot,AllowParentSearch,phNextMatchingItem);
}
//-----------------------------------------------------------------------------
// Name: FindPreviousAtSpecifiedDepth
// Object: find previous matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//           HTREEITEM hItem : search will begin with previous item of hItem
//           DWORD SpecifiedDepthFromRoot : return only item at specified depth (-1 if no specified depth)
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::FindPreviousAtSpecifiedDepth(TCHAR* Str,HTREEITEM hItem,DWORD DepthFromRoot,HTREEITEM* phPreviousMatchingItem)
{
    return this->FindPreviousAtSpecifiedDepth(Str,hItem,DepthFromRoot,TRUE,phPreviousMatchingItem);
}
//-----------------------------------------------------------------------------
// Name: FindPreviousAtSpecifiedDepth
// Object: find previous matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//           HTREEITEM hItem : search will begin with previous item of hItem
//           DWORD SpecifiedDepthFromRoot : return only item at specified depth (-1 if no specified depth)
//           BOOL AllowParentSearch : allow search in parent of hItem
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::FindPreviousAtSpecifiedDepth(TCHAR* Str,HTREEITEM hItem,DWORD DepthFromRoot,BOOL AllowParentSearch,HTREEITEM* phPreviousMatchingItem)
{
    return this->FindPrevious(Str,hItem,INFINITE,DepthFromRoot,AllowParentSearch,phPreviousMatchingItem);
}

//-----------------------------------------------------------------------------
// Name: FindNext
// Object: find next matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//           HTREEITEM hItem : search will begin with following item of hItem
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::FindNext(TCHAR* Str,HTREEITEM hItem,HTREEITEM* phNextMatchingItem)
{
    return this->FindNext(Str,hItem,INFINITE,TRUE,phNextMatchingItem);
}

//-----------------------------------------------------------------------------
// Name: FindNext
// Object: find next matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//           HTREEITEM hItem : search will begin with following item of hItem
//           DWORD MaxDepthFromItem : max depth from item
//           BOOL AllowParentSearch : allow search in parent of hItem
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::FindNext(TCHAR* Str,HTREEITEM hItem,DWORD MaxDepthFromItem,BOOL AllowParentSearch,HTREEITEM* phNextMatchingItem)
{
    return this->FindNext(Str,hItem,MaxDepthFromItem,(DWORD)-1,AllowParentSearch,phNextMatchingItem);
}
//-----------------------------------------------------------------------------
// Name: FindNext
// Object: find next matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//           HTREEITEM hItem : search will begin with following item of hItem
//           DWORD MaxDepthFromItem : max depth from item
//           DWORD SpecifiedDepthFromRoot : return only item at specified depth (-1 if no specified depth)
//           BOOL AllowParentSearch : allow search in parent of hItem
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::FindNext(TCHAR* Str,HTREEITEM hItem,DWORD MaxDepthFromItem,DWORD SpecifiedDepthFromRoot,BOOL AllowParentSearch,HTREEITEM* phNextMatchingItem)
{
    HTREEITEM hNextItem;
    HTREEITEM hParentItem;
    hNextItem=0;
    // if depth is non zero, try to get first child
    if(MaxDepthFromItem>0)
        hNextItem=TreeView_GetChild(this->hWndControl,hItem);
    // if depth is zero or item has no child, try to get next item
    if (!hNextItem)
        hNextItem=TreeView_GetNextSibling(this->hWndControl,hItem);
    if (hNextItem)
        return this->Find(Str,hNextItem,MaxDepthFromItem,SpecifiedDepthFromRoot,FALSE,AllowParentSearch,phNextMatchingItem);

    // item has no child nor next item

    // if parent search is allowed
    if (!AllowParentSearch)
        return FALSE;

    // get first next valid item
    for(;;) 
    {
        // no more item --> get parent of last valid item
        hParentItem=this->GetParent(hItem);
        MaxDepthFromItem++;

        if (!hParentItem)
            return FALSE;

        hNextItem=TreeView_GetNextSibling(this->hWndControl,hParentItem);
        if (hNextItem)
            break;

        hItem=hParentItem;
    }

    return this->Find(Str,hNextItem,MaxDepthFromItem,SpecifiedDepthFromRoot,FALSE,AllowParentSearch,phNextMatchingItem);
}

//-----------------------------------------------------------------------------
// Name: FindPrevious
// Object: find previous matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//           HTREEITEM hItem : search will begin with previous item of hItem
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::FindPrevious(TCHAR* Str,HTREEITEM hItem,HTREEITEM* phPreviousMatchingItem)
{
    return this->FindPrevious(Str,hItem,INFINITE,TRUE,phPreviousMatchingItem);
}

//-----------------------------------------------------------------------------
// Name: FindPrevious
// Object: find previous matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//           HTREEITEM hItem : search will begin with previous item of hItem
//           DWORD MaxDepthFromItem : max depth from item
//           BOOL AllowParentSearch : allow search in parent of hItem
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::FindPrevious(TCHAR* Str,HTREEITEM hItem,DWORD MaxDepthFromItem,BOOL AllowParentSearch,HTREEITEM* phPreviousMatchingItem)
{
    return this->FindPrevious(Str,hItem,MaxDepthFromItem,(DWORD)-1,AllowParentSearch,phPreviousMatchingItem);
}
//-----------------------------------------------------------------------------
// Name: FindPrevious
// Object: find previous matching item in the treeview
// Parameters :
//     in  : TCHAR* Str : string to find can contain jokers * and ?
//           HTREEITEM hItem : search will begin with previous item of hItem
//           DWORD MaxDepthFromItem : max depth from item
//           DWORD SpecifiedDepthFromRoot : return only item at specified depth (-1 if no specified depth)
//           BOOL AllowParentSearch : allow search in parent of hItem
//     out : HTREEITEM* phFirstMatchingItem : first matching item
//     return : TRUE if an item has been found
//-----------------------------------------------------------------------------
BOOL CTreeview::FindPrevious(TCHAR* Str,HTREEITEM hItem,DWORD MaxDepthFromItem,DWORD SpecifiedDepthFromRoot,BOOL AllowParentSearch,HTREEITEM* phPreviousMatchingItem)
{
    HTREEITEM hNextItem;
    HTREEITEM hParentItem;
    hNextItem=TreeView_GetPrevSibling(this->hWndControl,hItem);
    if (hNextItem)
        return this->Find(Str,hNextItem,MaxDepthFromItem,SpecifiedDepthFromRoot,TRUE,AllowParentSearch,phPreviousMatchingItem);

    if (!AllowParentSearch)
        return FALSE;

    if (!AllowParentSearch)
        return FALSE;

    // get first next valid item
    for(;;) 
    {
        // no more item --> get parent of last valid item
        hParentItem=this->GetParent(hItem);
        MaxDepthFromItem++;

        if (!hParentItem)
            return FALSE;

        hNextItem=TreeView_GetPrevSibling(this->hWndControl,hParentItem);
        if (hNextItem)
            break;

        hItem=hParentItem;
    }

    return this->Find(Str,hNextItem,MaxDepthFromItem,SpecifiedDepthFromRoot,TRUE,AllowParentSearch,phPreviousMatchingItem);
}

//-----------------------------------------------------------------------------
// Name: ShowError
// Object: show message box with error message 
// Parameters :
//     in  : TCHAR* psz : error message
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CTreeview::ShowError(TCHAR* psz)
{
     MessageBox(this->hWndControl,psz,_T("Error"),MB_OK|MB_ICONERROR|MB_TOPMOST);
}

//-----------------------------------------------------------------------------
// Name: OpenFile
// Object: open file, check control handle and find file type
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::OpenFile(TCHAR* pszFileName)
{
    TCHAR psz[2*MAX_PATH];

    // NULL pointer
    if (!pszFileName)
        return FALSE;

    // Empty File name
    if (!*pszFileName)
        return FALSE;

    /////////////////////////
    // check extension
    /////////////////////////
    if (CStdFileOperations::DoesExtensionMatch(pszFileName,_T("xml")))
        this->SavingType=CTreeview::SAVING_TYPE_XML;
    else if (CStdFileOperations::DoesExtensionMatch(pszFileName,_T("html")))
        this->SavingType=CTreeview::SAVING_TYPE_HTML;
    else
        this->SavingType=CTreeview::SAVING_TYPE_TXT;

    /////////////////////////
    // open file 
    /////////////////////////

    if (!CTextFile::CreateTextFile(pszFileName,&this->hFile))
    {
        _tcscpy(psz,_T("Can't create file "));
        _tcscat(psz,pszFileName);
        this->ShowError(psz);
        return FALSE;
    }

    _tcscpy(this->pszFileName,pszFileName);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CloseFile
// Object: close open file and show operation success message if bSuccess.
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::CloseFile(BOOL bSuccess)
{
    /////////////////////////
    // Close file
    /////////////////////////
    CloseHandle(this->hFile);

    /////////////////////////
    // Show success message if necessary
    /////////////////////////
    if (bSuccess)
    {
        if (MessageBox(NULL,_T("Save Successfully Completed\r\n\r\nDo you want to view results now ?"),_T("Information"),MB_YESNO|MB_ICONINFORMATION|MB_TOPMOST)==IDYES)
        {
            if (((int)ShellExecute(NULL,_T("open"),this->pszFileName,NULL,NULL,SW_SHOWNORMAL))<=32)
                this->ShowError(_T("Error opening file"));
        }
    }
    else
        DeleteFile(this->pszFileName);
    /////////////////////////
    // return result
    /////////////////////////
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetTreeViewItemDepth
// Object: get depth of specified TreeView item
// Parameters :
//     in  : HTREEITEM hTreeItem
//     out : 
//     return : item depth
//-----------------------------------------------------------------------------
DWORD CTreeview::GetItemDepth(HTREEITEM hTreeItem)
{
    HTREEITEM hParentTreeItem;
    DWORD ItemDepth=0;
    hParentTreeItem=TreeView_GetParent(this->hWndControl,hTreeItem);
    while(hParentTreeItem)
    {
        ItemDepth++;
        hParentTreeItem=TreeView_GetParent(this->hWndControl,hParentTreeItem);
    }
    return ItemDepth;
}

//-----------------------------------------------------------------------------
// Name: GetSelectedItem
// Object: get selected item
// Parameters :
//     in  : 
//     out : 
//     return : selected HTREEITEM, or NULL if no item selected
//-----------------------------------------------------------------------------
HTREEITEM CTreeview::GetSelectedItem()
{
    return TreeView_GetSelection(this->hWndControl);
}

//-----------------------------------------------------------------------------
// Name: SetSelectedItem
// Object: set selected item
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::SetSelectedItem(HTREEITEM hItem)
{
    return TreeView_SelectItem(this->hWndControl,hItem);
}

//-----------------------------------------------------------------------------
// Name: GetItemText
// Object: get item text
// Parameters :
//     in  :   HTREEITEM hTreeItem : item tree handle
//     out : LPARAM* pUserData : user data associated to item
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::GetItemUserData(HTREEITEM hTreeItem,OUT LPARAM* pUserData)
{
    *pUserData=NULL;
    TV_ITEM TreeViewItem={0};
    TreeViewItem.mask=TVIF_PARAM;
    TreeViewItem.hItem=hTreeItem;
    
    if(!TreeView_GetItem(this->hWndControl,&TreeViewItem))
        return FALSE;

    *pUserData=TreeViewItem.lParam;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetItemText
// Object: get item text
// Parameters :
//     in  :   HTREEITEM hTreeItem : item tree handle
//             DWORD TextMaxSize : max pszText length in TCHAR
//     inout : TCHAR* pszText : text
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::GetItemText(HTREEITEM hTreeItem,TCHAR* pszText,DWORD TextMaxSize)
{
    TV_ITEM TreeViewItem={0};
    TreeViewItem.mask=TVIF_TEXT;
    TreeViewItem.hItem=hTreeItem;
    TreeViewItem.cchTextMax=TextMaxSize;
    TreeViewItem.pszText=pszText;
    return TreeView_GetItem(this->hWndControl,&TreeViewItem);
}

//-----------------------------------------------------------------------------
// Name: GetItemTextLen
// Object: get item text len in TCHAR including \0
// Parameters :
//     in  : HTREEITEM hTreeItem : item tree handle
//     out : 
//     return : size of text in TCHAR including \0
//-----------------------------------------------------------------------------
DWORD CTreeview::GetItemTextLen(HTREEITEM hTreeItem)
{
    TCHAR* psz;
    DWORD pszSize=MAX_PATH*2;
    DWORD CopySize=pszSize;

    TV_ITEM TreeViewItem={0};
    TreeViewItem.mask=TVIF_TEXT;
    TreeViewItem.hItem=hTreeItem;

    // do a loop increasing size
    // as soon as size of text copied to lvi.pszText is smaller than buffer size
    // we got the real size
    while (CopySize+1>=pszSize)
    {
        pszSize*=2;

        psz=new TCHAR[pszSize];
        TreeViewItem.cchTextMax=pszSize;
        TreeViewItem.pszText=psz;
        if (!TreeView_GetItem(this->hWndControl,&TreeViewItem))
            return CopySize;

        CopySize=(DWORD)_tcslen(psz);

        delete[] psz;
    }

    return CopySize+1;
}

//-----------------------------------------------------------------------------
// Name: WriteTreeViewItemHeader
// Object: write header for specified item
// Parameters :
//     in  : DWORD ItemDepth : depth of item
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::WriteTreeViewItemHeader(DWORD ItemDepth)
{
    // item header
    if(this->SavingType==CTreeview::SAVING_TYPE_XML)
        CTextFile::WriteText(this->hFile, _T("<ITEM>"));
    else if (this->SavingType==CTreeview::SAVING_TYPE_HTML)
    {
        CTextFile::WriteText(this->hFile, _T("<TABLE>"));
        CTextFile::WriteText(this->hFile, _T("<TR>"));
        CTextFile::WriteText(this->hFile, _T("<TD>"));
    }
    else
    {
        for (DWORD cnt=0;cnt<ItemDepth;cnt++)
            CTextFile::WriteText(this->hFile, CTreeview_TXT_FILE_SPLITTER);
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: WriteTreeViewItemFooter
// Object: write footer for specified item
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::WriteTreeViewItemFooter()
{
    // item footer
    if(this->SavingType==CTreeview::SAVING_TYPE_XML)
        CTextFile::WriteText(this->hFile, _T("</ITEM>"));
    else if (this->SavingType==CTreeview::SAVING_TYPE_HTML)
    {
        CTextFile::WriteText(this->hFile, _T("</TD>"));
        CTextFile::WriteText(this->hFile, _T("</TR>"));
        CTextFile::WriteText(this->hFile, _T("</TABLE>"));
    }
    // splitting chars for next item
    CTextFile::WriteText(this->hFile, _T("\r\n"));

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: WriteTreeViewItemContent
// Object: write tree view item content
// Parameters :
//     in  : HTREEITEM hTreeItem : handle to tree view item
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::WriteTreeViewItemContent(HTREEITEM hTreeItem)
{
    TCHAR* psz;
    TCHAR* pszXMLLike;
    psz=NULL;
    TV_ITEM TreeViewItem={0};
    DWORD size=this->GetItemTextLen(hTreeItem);

    psz=new TCHAR[size];
    TreeViewItem.mask=TVIF_TEXT;
    TreeViewItem.cchTextMax=size;
    TreeViewItem.pszText=psz;
    TreeViewItem.hItem=hTreeItem;
    if (!TreeView_GetItem(this->hWndControl,&TreeViewItem))
        return FALSE;


    pszXMLLike=new TCHAR[_tcslen(psz)*CTreeview_REPLACED_CHAR_MAX_INCREASE+1];

    ///////////////////////////////////////////////////////////////////
    // write item text
    ///////////////////////////////////////////////////////////////////
    if(this->SavingType==CTreeview::SAVING_TYPE_XML)
    {
        CTextFile::WriteText(this->hFile, _T("<NAME>"));
        this->XMLConvert(psz,pszXMLLike,TRUE);
        CTextFile::WriteText(this->hFile, pszXMLLike);
        CTextFile::WriteText(this->hFile, _T("</NAME>"));
    }
    else if (this->SavingType==CTreeview::SAVING_TYPE_HTML)
    {
        this->XMLConvert(psz,pszXMLLike,TRUE);
        CTextFile::WriteText(this->hFile, pszXMLLike);
    }
    else
    {
        CTextFile::WriteText(this->hFile, psz);
    }

    delete[] pszXMLLike;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SaveTreeViewContent
// Object: Save content of selected TreeView control
// Parameters :
//     in  : TCHAR* pszFileName : output file name
//           BOOL bExpandedOnly : TRUE to save only expanded items
//           BOOL bSelectedOnly : TRUE to save only the selected item
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Save(TCHAR* pszFileName,BOOL bExpandedOnly,BOOL bSelectedOnly)
{
    HTREEITEM hTreeItem;
    BOOL bRet=TRUE;

    // open output file
    if (!this->OpenFile(pszFileName))
    {
        return FALSE;
    }

    // write tree view content header
    if(this->SavingType==CTreeview::SAVING_TYPE_XML)
        CTextFile::WriteText(this->hFile, _T("<TREEVIEW>"));
    else if (this->SavingType==CTreeview::SAVING_TYPE_HTML)
    {
        CTextFile::WriteText(this->hFile, _T("<HTML><BODY>"));
        CTextFile::WriteText(this->hFile, _T("<TABLE BORDER=\"1\"><TR><TD>"));
    }

    if (bSelectedOnly)
    {
        // get selected item
        hTreeItem=TreeView_GetSelection(this->hWndControl);

        // write item header
        this->WriteTreeViewItemHeader(0);

        // if item retrieval was successful
        if (hTreeItem)
        {
//            // write item content
//            this->WriteTreeViewItemContent(hTreeItem);
            // save all item data and it's sub items
            this->SaveTreeViewItemContent(hTreeItem,0,bExpandedOnly);
        }
        else
            bRet=FALSE;

        // write item footer
        this->WriteTreeViewItemFooter();
    }
    else
    {
        // get first item of tree view
        hTreeItem=TreeView_GetRoot(this->hWndControl);
        if(!hTreeItem)
            bRet=FALSE;

        // for each parent item
        while (hTreeItem)
        {
            // save all item data and it's sub items
            this->SaveTreeViewItemContent(hTreeItem,0,bExpandedOnly);

            // get next item having the same parent
            hTreeItem=TreeView_GetNextSibling(this->hWndControl,hTreeItem);
        }
    }

    // write tree view content footer
    if(this->SavingType==CTreeview::SAVING_TYPE_XML)
        CTextFile::WriteText(this->hFile, _T("</TREEVIEW>"));
    else if (this->SavingType==CTreeview::SAVING_TYPE_HTML)
        CTextFile::WriteText(this->hFile, _T("</TD></TR></TABLE></BODY></HTML>"));

    // close file
    if (!this->CloseFile(TRUE))
        return FALSE;

    return bRet;
}
//-----------------------------------------------------------------------------
// Name: SaveTreeViewContent
// Object: Save content of selected TreeView control
// Parameters :
//     in  : BOOL bExpandedOnly : TRUE to save only expanded items
//           BOOL bSelectedOnly : TRUE to save only the selected item
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Save(BOOL bExpandedOnly,BOOL bSelectedOnly)
{
    return this->Save(bExpandedOnly,bSelectedOnly,_T(""));
}
//-----------------------------------------------------------------------------
// Name: SaveTreeViewContent
// Object: Save content of selected TreeView control
// Parameters :
//     in  : BOOL bExpandedOnly : TRUE to save only expanded items
//           BOOL bSelectedOnly : TRUE to save only the selected item
//           TCHAR* DefaultExtension : default file extension
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::Save(BOOL bExpandedOnly,BOOL bSelectedOnly,TCHAR* DefaultExtension)
{

    TCHAR pszFile[MAX_PATH]=_T("");

    // open dialog
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=NULL;
    ofn.hInstance=NULL;
    ofn.lpstrFilter=_T("xml\0*.xml\0txt\0*.txt\0html\0*.html\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt=DefaultExtension;
    ofn.lpstrFile=pszFile;
    ofn.nMaxFile=MAX_PATH;

    if (_tcsicmp(DefaultExtension,_T("xml"))==0)
        ofn.nFilterIndex=1;
    else if (_tcsicmp(DefaultExtension,_T("txt"))==0)
        ofn.nFilterIndex=2;
    else // if (_tcsicmp(DefaultExtension,_T("html"))==0)
        ofn.nFilterIndex=3;

    if (!GetSaveFileName(&ofn))
        return TRUE;

    return this->Save(ofn.lpstrFile,bExpandedOnly,bSelectedOnly);

}

//-----------------------------------------------------------------------------
// Name: SaveTreeViewItemContent
// Object: Save content of specified hTreeItem and it's children (according to bExpandedOnly)
// Parameters :
//     in  : HTREEITEM hTreeItem : handle to current tree view item
//           DWORD ItemDepth : current item depth
//           BOOL bExpandedOnly : TRUE to get only expanded subitem content
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTreeview::SaveTreeViewItemContent(HTREEITEM hTreeItem,DWORD ItemDepth,BOOL bExpandedOnly)
{
    if (!hTreeItem)
        return FALSE;

    BOOL bGetSubItems=TRUE; // flag to known if we have to parse children

    // if we get only expanded content
    if (bExpandedOnly)
    {
        // check expanded state
        DWORD ItemState=TreeView_GetItemState(this->hWndControl,hTreeItem,TVIF_STATE);
        bGetSubItems=(ItemState&TVIS_EXPANDED);
    }

    // item header
    this->WriteTreeViewItemHeader(ItemDepth);

    // item content
    this->WriteTreeViewItemContent(hTreeItem);

    // item children
    if (bGetSubItems)// if we have to get them
    {
        // get first child
        hTreeItem=TreeView_GetChild(this->hWndControl,hTreeItem);
        if (hTreeItem)
        {
            // write children header
            if (this->SavingType==CTreeview::SAVING_TYPE_HTML)
                CTextFile::WriteText(this->hFile, _T("</TD><TD><TABLE BORDER=\"1\">"));
            else if (this->SavingType==CTreeview::SAVING_TYPE_TXT)
                CTextFile::WriteText(this->hFile, _T("\r\n"));

            // for each child
            while(hTreeItem)
            {
                // write child header
                if (this->SavingType==CTreeview::SAVING_TYPE_HTML)
                    CTextFile::WriteText(this->hFile, _T("<TR><TD>"));

                // write child content
                this->SaveTreeViewItemContent(hTreeItem,ItemDepth+1,bExpandedOnly);

                // write child footer
                if (this->SavingType==CTreeview::SAVING_TYPE_HTML)
                    CTextFile::WriteText(this->hFile, _T("</TD></TR>"));

                // get next child
                hTreeItem=TreeView_GetNextSibling(this->hWndControl,hTreeItem);
            }

            // write children footer
            if (this->SavingType==CTreeview::SAVING_TYPE_HTML)
                CTextFile::WriteText(this->hFile,_T("</TABLE>"));
        }
    }

    // item footer
    this->WriteTreeViewItemFooter();

    return TRUE;
}

TCHAR* XMLLikeConvertpppszReplacedChar[CTreeview_REPLACED_CHAR_ARRAY_SIZE][2]={{_T("&"),_T("&amp;")},{_T("<"),_T("&lt;")},{_T(">"),_T("&gt;")}};
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
void CTreeview::XMLConvert(TCHAR* pszInputString,TCHAR* pszOutputString,BOOL bToXML)
{
    if ((!pszInputString)||(!pszOutputString))
        return;

    TCHAR* pszLocalInput=new TCHAR [_tcslen(pszInputString)*CTreeview_REPLACED_CHAR_MAX_INCREASE+1];
    _tcscpy(pszLocalInput,pszInputString);
    int IndexOriginalString;
    int IndexReplaceString;

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
    // replace all strings defined in ppszReplacedChar
    for (int cnt=0;cnt<CTreeview_REPLACED_CHAR_ARRAY_SIZE;cnt++)
    {
        this->StrReplace(
            pszLocalInput,
            pszOutputString,
            XMLLikeConvertpppszReplacedChar[cnt][IndexOriginalString],
            XMLLikeConvertpppszReplacedChar[cnt][IndexReplaceString]
            );
            _tcscpy(pszLocalInput,pszOutputString);
    }

    delete[] pszLocalInput;

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
void CTreeview::StrReplace(TCHAR* pszInputString,TCHAR* pszOutputString,TCHAR* pszOldStr,TCHAR* pszNewStr)
{
    TCHAR* pszPos;
    TCHAR* pszOldPos;
    SIZE_T SearchedItemSize;

    if ((!pszInputString)||(!pszOutputString)||(!pszOldStr)||(!pszNewStr))
        return;

    *pszOutputString=0;

    pszOldPos=pszInputString;
    // get searched item size
    SearchedItemSize=_tcslen(pszOldStr);
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