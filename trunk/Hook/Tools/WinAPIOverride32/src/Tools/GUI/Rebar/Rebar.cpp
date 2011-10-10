#include "rebar.h"

CRebar::CRebar(HINSTANCE hInstance,HWND hwndParent)
{
    INITCOMMONCONTROLSEX icex;
    this->BandId = 0;

    //assume common controls are loaded
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC   = ICC_COOL_CLASSES|ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // create rebar control
    this->hwndRebar = CreateWindowEx(WS_EX_TOOLWINDOW,
                                    REBARCLASSNAME,
                                    NULL,
                                    WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | 
                                    WS_CLIPSIBLINGS | RBS_VARHEIGHT | 
                                    CCS_NODIVIDER,
                                    0,0,0,0,
                                    hwndParent,
                                    NULL,
                                    hInstance,
                                    NULL);

    if(!this->hwndRebar)
        return;
}

CRebar::~CRebar(void)
{
}

//-----------------------------------------------------------------------------
// Name: GetControlHandle
// Object: get control window handle
// Parameters :
//     in  : 
//     out : 
//     return : control HWND
//-----------------------------------------------------------------------------
HWND CRebar::GetControlHandle()
{
    return this->hwndRebar;
}

//-----------------------------------------------------------------------------
// Name: GetHeight
// Object: Retrieves the height, in pixels, of the rebar control
// Parameters :
//     in  : 
//     out : 
//     return : control HWND
//-----------------------------------------------------------------------------
UINT CRebar::GetHeight()
{
    return (UINT) SendMessage(this->hwndRebar,(UINT) RB_GETBARHEIGHT, 0, 0);  
}

//-----------------------------------------------------------------------------
// Name: AddToolBarBand
// Object: add a band containing toolbar
// Parameters :
//     in  : HWND hwndToolBar : toolbar handle
//     out : 
//     return : band index
//-----------------------------------------------------------------------------
UINT CRebar::AddToolBarBand(HWND hwndToolBar)
{
    return this->AddToolBarBand(hwndToolBar,NULL);
}

//-----------------------------------------------------------------------------
// Name: AddToolBarBand
// Object: add a band containing toolbar
// Parameters :
//     in  : HWND hwndToolBar : toolbar handle
//           TCHAR* Name : band name
//     out : 
//     return : band index
//-----------------------------------------------------------------------------
UINT CRebar::AddToolBarBand(HWND hwndToolBar,TCHAR* Name)
{
    return this->AddToolBarBand(hwndToolBar,Name,FALSE,FALSE,FALSE);
}

//-----------------------------------------------------------------------------
// Name: AddToolBarBand
// Object: add a band containing toolbar
// Parameters :
//     in  : HWND hwndToolBar : toolbar handle
//           TCHAR* Name : band name
//           BOOL NewLine : TRUE if band must be on a new line (shouldn't be set in the same time as FixedSize)
//           BOOL FixedSize : TRUE if band can't be resized (shouldn't be set in the same time as NewLine)
//           BOOL NoGripper : TRUE if band musn't have a gripper
//     out : 
//     return : band index
//-----------------------------------------------------------------------------
UINT CRebar::AddToolBarBand(HWND hwndToolBar,TCHAR* Name,BOOL NewLine,BOOL FixedSize,BOOL NoGripper)
{
    // Adjust toolbar style
    LONG_PTR Style;
    Style=GetWindowLongPtr(hwndToolBar,GWL_STYLE);
    Style|=CCS_NORESIZE | CCS_NOPARENTALIGN // must be set for toolbar attachment
            | CCS_NODIVIDER // remove 2 px horizontal line upper the control
            | TBSTYLE_TRANSPARENT // allow to use the rebar background
            | TBSTYLE_EX_HIDECLIPPEDBUTTONS // with RBBS_USECHEVRON
            ;
    SetWindowLongPtr(hwndToolBar,GWL_STYLE,Style);

    REBARBANDINFO rbBand={0};
    // Initialize structure members that both bands will share.
    rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
    rbBand.fMask  = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE |RBBIM_STYLE | RBBIM_ID;

    // to add chevron
    rbBand.fMask|= RBBIM_IDEALSIZE|RBBIM_STYLE;
    rbBand.fStyle|= RBBS_USECHEVRON;

    // fill fields
    rbBand.hwndChild  = hwndToolBar;
    if (Name)
    {
        rbBand.fMask|= RBBIM_TEXT;
        rbBand.lpText     = Name;
    }
    if (NewLine)
        rbBand.fStyle|=RBBS_BREAK;
    if (FixedSize)
        rbBand.fStyle|=RBBS_FIXEDSIZE;
    if (NoGripper)
        rbBand.fStyle|=RBBS_NOGRIPPER;

    // Get the height of the toolbar.
    DWORD BtnSize = (DWORD)SendMessage(hwndToolBar, TB_GETBUTTONSIZE, 0,0);
    RECT RectButton;

    // get toolbar button[0] rect
    SendMessage(hwndToolBar,TB_GETITEMRECT,0,(LPARAM)&RectButton);

    rbBand.cxMinChild = RectButton.right-RectButton.left; // assume at least first button is visible
    rbBand.cyMinChild = HIWORD(BtnSize);
    rbBand.cyChild    = HIWORD(BtnSize);
    rbBand.cyMaxChild = HIWORD(BtnSize);
    rbBand.cyIntegral = HIWORD(BtnSize);
    rbBand.wID  = this->BandId++;

    SIZE Size;
    SendMessage(hwndToolBar, TB_GETMAXSIZE, 0,(LPARAM)&Size);
    rbBand.cxIdeal= Size.cx;

    rbBand.cx     = rbBand.cxIdeal;

    // Add the band
    BOOL bRet=(BOOL)SendMessage(this->hwndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
    if (!bRet)
        return (UINT)-1;

    UINT BandIndex=(UINT)SendMessage(this->hwndRebar,RB_GETBANDCOUNT,0,0)-1;

    rbBand.fMask=RBBIM_HEADERSIZE|RBBIM_IDEALSIZE;
    SendMessage(this->hwndRebar,RB_GETBANDINFO,BandIndex,(LPARAM)&rbBand);
    rbBand.fMask=RBBIM_SIZE;

    rbBand.cx   = rbBand.cxIdeal+rbBand.cxHeader+10;

    SendMessage(this->hwndRebar,RB_SETBANDINFO,BandIndex,(LPARAM)&rbBand);

    return BandIndex;
}

//-----------------------------------------------------------------------------
// Name: AddBand
// Object: add a band containing item 
//         use this function for combo, edit, ...
// Parameters :
//     in  : HWND hwnd : item window handle
//           TCHAR* Name : band name
//     out : 
//     return : band index
//-----------------------------------------------------------------------------
UINT CRebar::AddBand(HWND hwnd)
{
    return this->AddBand(hwnd,NULL);
}

//-----------------------------------------------------------------------------
// Name: AddBand
// Object: add a band containing item 
//         use this function for combo, edit, ...
// Parameters :
//     in  : HWND hwnd : item window handle
//           TCHAR* Name : toolbar name
//     out : 
//     return : band index
//-----------------------------------------------------------------------------
UINT CRebar::AddBand(HWND hwnd,TCHAR* Name)
{
    return this->AddBand(hwnd,Name,FALSE,FALSE,FALSE);
}

//-----------------------------------------------------------------------------
// Name: AddBand
// Object: add a band containing item 
//         use this function for combo, edit, ...
// Parameters :
//     in  : HWND hwnd : item window handle
//           TCHAR* Name : toolbar name
//           BOOL NewLine : TRUE if band must be on a new line (shouldn't be set in the same time as FixedSize)
//           BOOL FixedSize : TRUE if band can't be resized (shouldn't be set in the same time as NewLine)
//           BOOL NoGripper : TRUE if band musn't have a gripper
//     out : 
//     return : band index
//-----------------------------------------------------------------------------
UINT CRebar::AddBand(HWND hwnd,TCHAR* Name,BOOL NewLine,BOOL FixedSize,BOOL NoGripper)
{
    REBARBANDINFO rbBand={0};
    // Initialize structure members that both bands will share.
    rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
    rbBand.fMask  = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID;

    rbBand.hwndChild = hwnd;

    if (Name)
    {
        rbBand.fMask|= RBBIM_TEXT;
        rbBand.lpText     = Name;
    }
    if (NewLine)
    {
        rbBand.fMask|=RBBIM_STYLE;
        rbBand.fStyle|=RBBS_BREAK;
    }
    if (FixedSize)
    {
        rbBand.fMask|=RBBIM_STYLE;
        rbBand.fStyle|=RBBS_FIXEDSIZE;
    }
    if (NoGripper)
    {
        rbBand.fMask|=RBBIM_STYLE;
        rbBand.fStyle|=RBBS_NOGRIPPER;
    }

    RECT Rect;
    GetWindowRect(hwnd,&Rect);
    rbBand.cxMinChild = Rect.right-Rect.left;
    rbBand.cyMinChild = Rect.bottom-Rect.top;
    rbBand.cyChild    = Rect.bottom-Rect.top;
    rbBand.cyMaxChild = Rect.bottom-Rect.top;
    rbBand.cyIntegral = Rect.bottom-Rect.top;
    rbBand.wID  = this->BandId++;

    BOOL bRet=(BOOL)SendMessage(this->hwndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
    if (!bRet)
        return (UINT)-1;

    UINT BandIndex=(UINT)SendMessage(this->hwndRebar,RB_GETBANDCOUNT,0,0)-1;
    return BandIndex;
}

//-----------------------------------------------------------------------------
// Name: GetBandCount
// Object: get number of band
// Parameters :
//     in  : 
//     out : 
//     return :  number of band
//-----------------------------------------------------------------------------
SIZE_T CRebar::GetBandCount()
{
    return ::SendMessage(this->hwndRebar,(UINT) RB_GETBANDCOUNT, 0, 0);  
}


//-----------------------------------------------------------------------------
// Name: OnSize
// Object: must be called on parent WM_SIZE notification
// Parameters :
//     in  : WPARAM wParam, LPARAM lParam : parent WndProc WPARAM and LPARAM
//     out : 
//     return : result of WM_SIZE message sent to control
//-----------------------------------------------------------------------------
BOOL CRebar::OnSize(WPARAM wParam, LPARAM lParam)
{
    return (BOOL)SendMessage(this->hwndRebar,WM_SIZE,wParam,lParam);
}

//-----------------------------------------------------------------------------
// Name: OnNotify
// Object: must be called on parent WM_NOTIFY notification
// Parameters :
//     in  : WPARAM wParam, LPARAM lParam : parent WndProc WPARAM and LPARAM
//     out : 
//     return : TRUE if message has been processed
//-----------------------------------------------------------------------------
BOOL CRebar::OnNotify(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    NMHDR* pnm = (NMHDR*)lParam;
    if (pnm->hwndFrom!=this->hwndRebar)
        return FALSE;

    switch (pnm->code)
    {

    case RBN_CHEVRONPUSHED:
        {
            NMREBARCHEVRON* lpnm = (NMREBARCHEVRON*) lParam;
            RECT RectToolBar;

            // get toolbar rect
            SendMessage (lpnm->hdr.hwndFrom,RB_GETRECT,lpnm->uBand,(LPARAM)&RectToolBar);

            int ButtonCount;
            REBARBANDINFO rbbi;
            rbbi.cbSize=sizeof(REBARBANDINFO);
            rbbi.fMask=RBBIM_CHILD;
            if (!SendMessage(lpnm->hdr.hwndFrom,(UINT) RB_GETBANDINFO,lpnm->uBand,(LPARAM)&rbbi))
                break;

            // get number of button in toolbar
            ButtonCount=(int)SendMessage(rbbi.hwndChild,TB_BUTTONCOUNT,0,0);

            // remove chevron size from toolbar
            RectToolBar.right-=lpnm->rc.right-lpnm->rc.left;

            RECT RectButton;
            RECT RectIntersect;
            CPopUpMenu PopUpMenu;
            int FirstHiddenItemIndex=0;
            TBBUTTON Button;

            // get toolbar image list
            HIMAGELIST ImageList=(HIMAGELIST)SendMessage(rbbi.hwndChild,TB_GETIMAGELIST,0,0);

            int NbIconToDestroy=0;
            HICON* ArrayHICONToDestroy=new HICON[ButtonCount];
            int NbItemHidden=0;

            // try to get toolbar object (if CToolbar class provided is used)
            CToolbar* pToolBar=(CToolbar*)GetProp(rbbi.hwndChild,CToolbar_MARKER_PROP_NAME);
            CHEVRON_MENU_INFOS ChevronMenuInfos;
            CPopUpMenu* pDropDownMenu;
            BOOL bItemHasDropDownMenu;
            memset(&ChevronMenuInfos,0,sizeof(CHEVRON_MENU_INFOS));
            ChevronMenuInfos.pOriginalMenu=NULL;
            ChevronMenuInfos.pSubMenuChevronInfosArray=new CHEVRON_MENU_INFOS[ButtonCount];
            memset(ChevronMenuInfos.pSubMenuChevronInfosArray,0,ButtonCount*sizeof(CHEVRON_MENU_INFOS));
            ChevronMenuInfos.CommandId=0;
            ChevronMenuInfos.MenuId=0;
            ChevronMenuInfos.pSubMenu=&PopUpMenu;


            // for each button of the toolbar
            for (int Cnt=0;Cnt<ButtonCount;Cnt++)
            {
                // get toolbar button[Cnt] rect
                SendMessage(rbbi.hwndChild,TB_GETITEMRECT,Cnt,(LPARAM)&RectButton);
                ::MapWindowPoints(rbbi.hwndChild, lpnm->hdr.hwndFrom, (LPPOINT)&RectButton, (sizeof(RECT)/sizeof(POINT)) );

                // check if rect intersection is equal to toolbar button intersection
                ::IntersectRect(&RectIntersect,&RectToolBar,&RectButton);
                if (::EqualRect(&RectIntersect,&RectButton))
                    // item is fully visible
                    continue;
                //else

                ////////////////////////////
                // item is not fully visible
                ////////////////////////////
                if (!FirstHiddenItemIndex)
                    FirstHiddenItemIndex=Cnt;

                // for debug purpose only
                //TCHAR sz[MAX_PATH];
                //_stprintf(sz,"not visible item %d\r\n",Cnt);
                //OutputDebugString(sz);

                // get button info from button index
                SendMessage(rbbi.hwndChild,TB_GETBUTTON,Cnt,(LPARAM)&Button);

                // if button is a separator
                if (Button.fsStyle & BTNS_SEP)
                {
                    ChevronMenuInfos.pSubMenuChevronInfosArray[NbItemHidden].CommandId=0;
                    ChevronMenuInfos.pSubMenuChevronInfosArray[NbItemHidden].MenuId=PopUpMenu.AddSeparator();

                    NbItemHidden++;
                    continue;
                }

                // get item icon if any
                int BmpIndex=(int)SendMessage(rbbi.hwndChild,TB_GETBITMAP,Button.idCommand,0);
                HICON hIcon=ImageList_GetIcon(ImageList,BmpIndex,0);
                // store hicon to destroy it later
                if (hIcon)
                    ArrayHICONToDestroy[NbIconToDestroy++]=hIcon;

                // security issue if text size is larger than sz size
                TCHAR* psz=0;
                int TextSize=0;
                BOOL ToolbarButtonEnabled=TRUE;

                // get button text
                TextSize=(int)SendMessage(rbbi.hwndChild,TB_GETBUTTONTEXT,Button.idCommand,NULL);
                if (TextSize>0)
                {
                    TextSize++;
                    psz=(TCHAR*)_alloca(TextSize*sizeof(TCHAR));
                    SendMessage(rbbi.hwndChild,TB_GETBUTTONTEXT,Button.idCommand,(LPARAM)psz);
                }
                // if no text
                else // get info from tooltip
                {
                    TOOLTIPTEXT ttt={0};
                    ttt.hdr.idFrom=Button.idCommand;
                    ttt.hdr.hwndFrom=rbbi.hwndChild;
                    ttt.hdr.code=TTN_GETDISPINFO;
                    ttt.uFlags=0;
                    SendMessage(rbbi.hwndChild,WM_NOTIFY,ttt.hdr.idFrom,(LPARAM)&ttt);
                    psz=ttt.lpszText;
                }

                bItemHasDropDownMenu=FALSE;

                // if toolbar has an associated CToolbar object
                if (pToolBar)
                {
                    ToolbarButtonEnabled=(pToolBar->GetButtonState(Button.idCommand) & TBSTATE_ENABLED);

                    // get dropdown menu
                    pDropDownMenu=pToolBar->GetButtonDropDownMenu(Button.idCommand);

                    // if toolbar button has a dropdown menu
                    if (pDropDownMenu)
                    {
                        bItemHasDropDownMenu=TRUE;
                        // add menu item and sub items to current menu
                        ChevronMenuInfos.pSubMenuChevronInfosArray[NbItemHidden].CommandId=0;
                        ChevronMenuInfos.pSubMenuChevronInfosArray[NbItemHidden].OriginalMenuId=0;
                        ChevronMenuInfos.pSubMenuChevronInfosArray[NbItemHidden].pOriginalMenu=pDropDownMenu;
                        this->ParseSubMenu(&PopUpMenu,pDropDownMenu,pDropDownMenu,&ChevronMenuInfos.pSubMenuChevronInfosArray[NbItemHidden]);
                        ChevronMenuInfos.pSubMenuChevronInfosArray[NbItemHidden].MenuId=PopUpMenu.AddSubMenu(psz,pDropDownMenu,hIcon);

                        if (!ToolbarButtonEnabled)
                        {
                            PopUpMenu.SetEnabledState(ChevronMenuInfos.pSubMenuChevronInfosArray[NbItemHidden].MenuId,FALSE);
                        }
                    }
                }
                if (!bItemHasDropDownMenu)
                {
                    // add to popupmenu
                    ChevronMenuInfos.pSubMenuChevronInfosArray[NbItemHidden].CommandId=Button.idCommand;
                    ChevronMenuInfos.pSubMenuChevronInfosArray[NbItemHidden].MenuId=PopUpMenu.Add(psz,hIcon);

                    if (!ToolbarButtonEnabled)
                    {
                        PopUpMenu.SetEnabledState(ChevronMenuInfos.pSubMenuChevronInfosArray[NbItemHidden].MenuId,FALSE);
                    }
                }

                NbItemHidden++;
            }
            // if there is hidden items
            if (NbItemHidden)
            {
                // show pop up menu
                UINT ClickedItemId=PopUpMenu.Show(lpnm->rc.left,lpnm->rc.bottom,lpnm->hdr.hwndFrom,TRUE,FALSE);
                if (ClickedItemId)
                {
                    UINT MenuOrCommandId;
                    BOOL IsMenuId;
                    if (this->GetChevronMenuInfos(ClickedItemId,&ChevronMenuInfos,&MenuOrCommandId,&pDropDownMenu,&IsMenuId))
                    {
                        if (IsMenuId)
                        {
                            CToolbar::tagDropDownMenuCallBack CallBack;
                            PVOID CallBackUserParam;
                            pToolBar->GetDropDownMenuCallBack(&CallBack,&CallBackUserParam);
                            if (!IsBadCodePtr((FARPROC)CallBack))
                                CallBack(pDropDownMenu,MenuOrCommandId,CallBackUserParam);
                        }
                        else
                        {
                            BYTE Style=pToolBar->GetButtonStyle(MenuOrCommandId);
                            // if Button has a checked style
                            if (Style & BTNS_CHECK)
                            {
                                // invert button state
                                BYTE State=pToolBar->GetButtonState(MenuOrCommandId);
                                State^=TBSTATE_CHECKED;
                                pToolBar->SetButtonState(MenuOrCommandId,State);
                            }

                            // send associated notification
                            PostMessage(pnm->hwndFrom,WM_COMMAND,MenuOrCommandId,(LPARAM)rbbi.hwndChild);
                        }
                    }

                }

                // restore original sub menus id infos
                this->RestoreSubMenuIds(&ChevronMenuInfos,TRUE);

                // free memory
                for (int Cnt=0;Cnt<NbIconToDestroy;Cnt++)
                    DestroyIcon(ArrayHICONToDestroy[Cnt]);
            }
            // free memory
            delete[] ArrayHICONToDestroy;
        }
        break;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ParseSubMenu
// Object: parse a menu to add it to chevron menu (warning recursive call)
// Parameters :
//     in  : CPopUpMenu* pRootMenu : the chevron menu
//           CPopUpMenu* pToolBarDropDownMenu : the popup menu of the toolbar
//           CPopUpMenu* pPopUpMenu : menu to parse
//     intout : CHEVRON_MENU_INFOS* pChevronMenuInfos : informations stored for chevron menu
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CRebar::ParseSubMenu(CPopUpMenu* pRootMenu,CPopUpMenu* pToolBarDropDownMenu,CPopUpMenu* pPopUpMenu,CHEVRON_MENU_INFOS* pChevronMenuInfos)
{
    if (!pRootMenu)
        return;
    if (!pPopUpMenu)
        return;
    if (!pChevronMenuInfos)
        return;

    // get sub menu item number
    int SubMenuItemsNumber=pPopUpMenu->GetItemCount();
    if (SubMenuItemsNumber<=0)
        return;

    pChevronMenuInfos->pSubMenu=pPopUpMenu;
    pChevronMenuInfos->pSubMenuChevronInfosArray=new CHEVRON_MENU_INFOS[SubMenuItemsNumber];
    memset(pChevronMenuInfos->pSubMenuChevronInfosArray,0,SubMenuItemsNumber*sizeof(CHEVRON_MENU_INFOS));
    
    // assume that root menu ids are greater than sub menus ids 
    // to avoid ReplaceMenuId to interfere with sub menu ids
    int MaxId;
    // find max sub menu id
    MaxId = pPopUpMenu->GetMaxMenuId();

    // increment root menu id until ids are >= than MaxId
    while (MaxId>(int)pRootMenu->GetNextMenuId())
    {        
    }

    // for each item of sub menu
    for (int CntMenu=0;CntMenu<SubMenuItemsNumber;CntMenu++)
    {
        pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu].pToolBarDropDownMenu = pToolBarDropDownMenu;
        // store menu infos
        pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu].pOriginalMenu=pPopUpMenu;
        // command id is unused
        pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu].CommandId=0;
        // get old id of sub menu
        pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu].OriginalMenuId=pPopUpMenu->GetID(CntMenu);
        // allocate a new id to menu
        pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu].MenuId=pRootMenu->GetNextMenuId();

        // replace old menu id by a new one
        pPopUpMenu->ReplaceMenuId(
                                  pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu].OriginalMenuId,
                                  pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu].MenuId
                                 );

        // get item sub menu
        pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu].pSubMenu=pPopUpMenu->GetSubMenu(pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu].MenuId);
        // if item has a sub menu
        if (pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu].pSubMenu)
        {
            // parse item sub menu
            this->ParseSubMenu(pRootMenu,
                                pToolBarDropDownMenu,
                                pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu].pSubMenu,
                                &pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu]
                                );
        }
    }
}

//-----------------------------------------------------------------------------
// Name: RestoreSubMenuIds 
// Object: as ParseSubMenu changes drop down menu ids, they need to be restored after pop up menu use
//           (warning recursive call)
// Parameters :
//     in  : CPopUpMenu* pRootMenu : the chevron menu
//           CPopUpMenu* pPopUpMenu : menu to restore
//     intout : CHEVRON_MENU_INFOS* pChevronMenuInfos : informations stored for chevron menu
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CRebar::RestoreSubMenuIds(CHEVRON_MENU_INFOS* pChevronMenuInfos,BOOL bChevronMenu)
{
    // restore item menu id
    if ((pChevronMenuInfos->pOriginalMenu) && (pChevronMenuInfos->OriginalMenuId))
    {
        pChevronMenuInfos->pOriginalMenu->ReplaceMenuId(pChevronMenuInfos->MenuId,pChevronMenuInfos->OriginalMenuId);
    }

    // if item as sub items
    if (pChevronMenuInfos->pSubMenu && pChevronMenuInfos->pSubMenuChevronInfosArray)
    {
        int NbItems=pChevronMenuInfos->pSubMenu->GetItemCount();
        // for each sub item
        for (int CntMenu=0;CntMenu<NbItems;CntMenu++)
        {
            if (bChevronMenu)
            {
                // remove item from chevron menu to avoid menu to be destroyed by DestroyMenu called on chevron menu handle
                pChevronMenuInfos->pSubMenu->Remove(pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu].MenuId);
            }

            // parse sub item
            this->RestoreSubMenuIds(
                                    &pChevronMenuInfos->pSubMenuChevronInfosArray[CntMenu],
                                    FALSE
                                    );

        }
        // free pSubMenuChevronInfosArray
        delete pChevronMenuInfos->pSubMenuChevronInfosArray;
        pChevronMenuInfos->pSubMenuChevronInfosArray=0;
    }
}

//-----------------------------------------------------------------------------
// Name: GetChevronMenuInfos 
// Object: Translate chevron menu id to command id or original dropdown menu id
//           (warning recursive call)
// Parameters : 
//     in  : UINT MenuId : chevron menu id
//           CHEVRON_MENU_INFOS* pMenuInfos : informations stored for menu
//     out : UINT* pMenuOrCommandId : command id if chevron item menu is for a toolbar button
//                                    or original menu id if chevron item menu is for dropdown item menu
//           CPopUpMenu** ppDropDownMenu : pointer to original dropdown menu object if chevron item menu is for dropdown item menu
//           BOOL* pIsMenuId : TRUE if chevron item menu is for dropdown item menu, FALSE if chevron item menu is for a toolbar button
//     return : TRUE if we found an item associated with MenuId 
//-----------------------------------------------------------------------------
BOOL CRebar::GetChevronMenuInfos(IN UINT MenuId,IN CHEVRON_MENU_INFOS* pMenuInfos,OUT UINT* pMenuOrCommandId,OUT CPopUpMenu** ppDropDownMenu,OUT BOOL* pIsMenuId)
{
    if (!pMenuInfos)
        return FALSE;

    // if chevron menu id match menu id
    if (pMenuInfos->MenuId==MenuId)
    {
        // if chevron item menu is for dropdown item menu
        if (pMenuInfos->OriginalMenuId)
        {
            *pIsMenuId=TRUE;
            *pMenuOrCommandId=pMenuInfos->OriginalMenuId;
            // don't use *ppDropDownMenu=pMenuInfos->pOriginalMenu; but pToolBarDropDownMenu instead to broadcast event
            *ppDropDownMenu=pMenuInfos->pToolBarDropDownMenu;
            
        }
        else // if chevron item menu is for a toolbar button
        {
            *pIsMenuId=FALSE;
            *pMenuOrCommandId=pMenuInfos->CommandId;
            *ppDropDownMenu=NULL;
        }
        return TRUE;
    }

    // if item has a sub menu
    if (pMenuInfos->pSubMenu)
    {
        // parse each item of the sub menu
        for (int CntMenu=0;CntMenu<pMenuInfos->pSubMenu->GetItemCount();CntMenu++)
        {
            // parse item sub menu
            if (this->GetChevronMenuInfos(MenuId,&pMenuInfos->pSubMenuChevronInfosArray[CntMenu],pMenuOrCommandId,ppDropDownMenu,pIsMenuId))
                return TRUE;
        }
    }

    return FALSE;
}