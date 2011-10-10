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
// Object: class helper for Toolbar control
//-----------------------------------------------------------------------------

#include "Toolbar.h"

//-----------------------------------------------------------------------------
// Name: CToolbarButtonUserData
// Object: constructor for tool bar user data storage
// Parameters :
//     in  : TCHAR* ToolTip : tooltip text (null if no tooltip)
//           CPopUpMenu* PopUpMenu : popup menu for drop down button, null else
//     out :
//     return :
//-----------------------------------------------------------------------------
CToolbarButtonUserData::CToolbarButtonUserData(TCHAR* ToolTip,CPopUpMenu* PopUpMenu)
{
    // make a local copy of tooltip if pointer is valid
    if (IsBadReadPtr(ToolTip,sizeof(TCHAR)))
        this->ToolTip=NULL;
    else
        this->ToolTip=_tcsdup(ToolTip);
    // store popupmenu handle
    this->PopUpMenu=PopUpMenu;
}

//-----------------------------------------------------------------------------
// Name: ~CToolbarButtonUserData
// Object: destructor for tool bar user data storage
// Parameters :
//     in  :
//     out :
//     return : 
//-----------------------------------------------------------------------------
CToolbarButtonUserData::~CToolbarButtonUserData()
{
    // free allocated memory
    if (this->ToolTip)
        free(this->ToolTip);
    this->ToolTip=NULL;
}

//-----------------------------------------------------------------------------
// Name: SetToolTip
// Object: allow to change tooltip, freeing and allocating memory
// Parameters :
//     in  : TCHAR* ToolTip
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CToolbarButtonUserData::SetToolTip(TCHAR* ToolTip)
{
    // free allocated memory
    if (this->ToolTip)
        free(this->ToolTip);
    // make a local copy of tooltip if pointer is valid
    if (IsBadReadPtr(ToolTip,sizeof(TCHAR)))
        this->ToolTip=NULL;
    else
        this->ToolTip=_tcsdup(ToolTip);
}


//-----------------------------------------------------------------------------
// Name: CToolbar
// Object: constructor
// Parameters :
//     in  : 
//              HINSTANCE hInstance : resource instance
//              HWND hwndParent : parent window handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
CToolbar::CToolbar(HINSTANCE hInstance,HWND hwndParent)
{
    this->Constructor(hInstance,hwndParent,FALSE,FALSE,0,0);
}

//-----------------------------------------------------------------------------
// Name: CToolbar
// Object: constructor
// Parameters :
//     in  : 
//              HINSTANCE hInstance : resource instance
//              HWND hwndParent : parent window handle
//              BOOL bFlat : TRUE for flat style
//              BOOL bListStyle : TRUE for List style
//     out :
//     return : 
//-----------------------------------------------------------------------------
CToolbar::CToolbar(HINSTANCE hInstance,HWND hwndParent,BOOL bFlat,BOOL bListStyle)
{
    this->Constructor(hInstance,hwndParent,bFlat,bListStyle,0,0);
}

//-----------------------------------------------------------------------------
// Name: CToolbar
// Object: constructor
// Parameters :
//     in  : 
//              HINSTANCE hInstance : resource instance
//              HWND hwndParent : parent window handle
//              BOOL bFlat : TRUE for flat style
//              BOOL bListStyle : TRUE for List style
//              DWORD dwIconWidth : Icon width
//              DWORD dwIconHeight : Icon height
//     out :
//     return : 
//-----------------------------------------------------------------------------
CToolbar::CToolbar(HINSTANCE hInstance,HWND hwndParent,BOOL bFlat,BOOL bListStyle,DWORD dwIconWidth,DWORD dwIconHeight)
{
    this->Constructor(hInstance,hwndParent,bFlat,bListStyle,dwIconWidth,dwIconHeight);
}
//-----------------------------------------------------------------------------
// Name: Constructor
// Object: constructor
// Parameters :
//     in  : 
//              HINSTANCE hInstance : resource instance
//              HWND hwndParent : parent window handle
//              BOOL bFlat : TRUE for flat style
//              BOOL bListStyle : TRUE for List style
//              DWORD dwIconWidth : Icon width
//              DWORD dwIconHeight : Icon height
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CToolbar::Constructor(HINSTANCE hInstance,HWND hwndParent,BOOL bFlat,BOOL bListStyle,DWORD dwIconWidth,DWORD dwIconHeight)
{
    this->hInstance=hInstance;
    this->pDropDownMenuCallBack=NULL;
    this->DropDownMenuUserParam=NULL;

    // Ensure that the common control DLL is loaded. 
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);


    DWORD dwStyle=WS_CHILD | TBSTYLE_TOOLTIPS | WS_VISIBLE;
                    // TBSTYLE_TRANSPARENT
                    // | CCS_ADJUSTABLE // to customize toolbar only

    if (bListStyle)
        dwStyle|=TBSTYLE_LIST;// to add text to the right of image instead under
    if (bFlat)
        dwStyle|=TBSTYLE_FLAT;

    // create toolbar
    this->hwndTB = CreateWindowEx(
                                    0,
                                    TOOLBARCLASSNAME,
                                    (LPTSTR) NULL, 
                                    dwStyle, 
                                    0, // x
                                    0, // y
                                    0, // width
                                    0, // height
                                    hwndParent, 
                                    (HMENU) NULL,
                                    hInstance,
                                    NULL); 

    SendMessage(this->hwndTB, TB_SETEXTENDEDSTYLE,0,(LPARAM)TBSTYLE_EX_DRAWDDARROWS);

    // Send the TB_BUTTONSTRUCTSIZE message, which is required for 
    // backward compatibility. 
    SendMessage(this->hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0); 


    // set the icon size
    if (dwIconWidth && dwIconHeight)
    {
        SendMessage(
                    (HWND) this->hwndTB,
                    (UINT) TB_SETBITMAPSIZE,
                    (WPARAM) 0,//not used, must be zero 
                    (LPARAM) MAKELONG (dwIconWidth, dwIconHeight)// = (LPARAM) MAKELONG (dxBitmap, dyBitmap)
                    );
    }
    else
    {
        // "If an application does not explicitly set the bitmap size, 
        // the size defaults to 16 by 15 pixels." (MSDN)
        dwIconWidth=16;
        dwIconHeight=15;
    }

    // create image lists
    this->hImgList=ImageList_Create(dwIconWidth, dwIconHeight, ILC_MASK|ILC_COLOR32, CToolbar_DEFAULT_IMAGELIST_SIZE, CToolbar_IMAGELIST_GROW_SIZE);
    this->hImgListDisabled=ImageList_Create(dwIconWidth, dwIconHeight, ILC_MASK|ILC_COLOR32, CToolbar_DEFAULT_IMAGELIST_SIZE, CToolbar_IMAGELIST_GROW_SIZE);
    this->hImgListHot=ImageList_Create(dwIconWidth, dwIconHeight, ILC_MASK|ILC_COLOR32, CToolbar_DEFAULT_IMAGELIST_SIZE, CToolbar_IMAGELIST_GROW_SIZE);

    // associate image list to toolbar control
    SendMessage(this->hwndTB, TB_SETIMAGELIST, 0, (LPARAM)this->hImgList);
    SendMessage(this->hwndTB, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)this->hImgListDisabled);
    SendMessage(this->hwndTB, TB_SETHOTIMAGELIST, 0, (LPARAM)this->hImgListHot);

    // get handle to tooltip
    this->hwndToolTip=(HWND)SendMessage(this->hwndTB,TB_GETTOOLTIPS,0,0);  

    SendMessage(this->hwndTB, TB_AUTOSIZE, 0, 0); 

    SetProp(this->hwndTB,CToolbar_MARKER_PROP_NAME,(HANDLE)this);
}

//-----------------------------------------------------------------------------
// Name: ~CToolbar
// Object: MUST BE CALLED BEFORE CALLING EndDialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
CToolbar::~CToolbar(void)
{
    RemoveProp(this->hwndTB,CToolbar_MARKER_PROP_NAME);
    int NbButtons=this->GetButtonCount();
    // get button info
    TBBUTTON  Button={0};

    for (int cnt=0;cnt<NbButtons;cnt++)
    {
        // get button info
        if (SendMessage( this->hwndTB,(UINT) TB_GETBUTTON,(WPARAM) cnt,(LPARAM) &Button)==-1)
            continue;

        // free lparam memory
        if (Button.dwData)
            delete (CToolbarButtonUserData*)Button.dwData;
    }


    ImageList_Destroy(this->hImgList);
    ImageList_Destroy(this->hImgListDisabled);
    ImageList_Destroy(this->hImgListHot);
}


//-----------------------------------------------------------------------------
// Name: SetPosition
// Object: set position of tool bar
// Parameters :
//     in  : tagCToolbarPosition Position
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CToolbar::SetPosition(tagCToolbarPosition Position)
{
    BOOL bRet;
    LONG_PTR Style;
    Style=GetWindowLongPtr(this->hwndTB,GWL_STYLE);
    // remove all position style
    Style&=~(CCS_TOP | CCS_LEFT | CCS_BOTTOM | CCS_RIGHT);
    // Add new position style
    Style|=(LONG_PTR)Position;

    // apply new style
    bRet=SetWindowLongPtr(this->hwndTB,GWL_STYLE,Style);

    // force item to be redrawn
    RedrawWindow( this->hwndTB,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: EnableDivider
// Object: show/hide the two-pixel highlight at the top of the control
// Parameters :
//     in  : BOOL bEnable : TRUE to show divider
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CToolbar::EnableDivider(BOOL bEnable)
{
    BOOL bRet;
    LONG_PTR Style;
    Style=GetWindowLongPtr(this->hwndTB,GWL_STYLE);
    if (bEnable)
        Style&=~CCS_NODIVIDER;
    else
        Style|=CCS_NODIVIDER;

    // apply new style
    bRet=SetWindowLongPtr(this->hwndTB,GWL_STYLE,Style);

    // force item to be redrawn
    RedrawWindow( this->hwndTB,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: SetDirection
// Object: display control vertically or horizontally
// Parameters :
//     in  : BOOL bHorizontal : FALSE to display control vertically
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CToolbar::SetDirection(BOOL bHorizontal)
{
    BOOL bRet;
    LONG_PTR Style;
    Style=GetWindowLongPtr(this->hwndTB,GWL_STYLE);
    if (bHorizontal)
        Style&=~CCS_VERT;
    else
        Style|=CCS_VERT;

    // apply new style
    bRet=SetWindowLongPtr(this->hwndTB,GWL_STYLE,Style);

    // force item to be redrawn
    RedrawWindow( this->hwndTB,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: EnableParentAlign
// Object: make the control automatically moving to the top or bottom of the parent window, or let it
//         keep its position within the parent window despite changes to the size of the parent
// Parameters :
//     in  : BOOL bEnable : TRUE for automatically moving to the top or bottom of the parent window,
//                          FALSE to keep position
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CToolbar::EnableParentAlign(BOOL bEnable)
{
    BOOL bRet;
    LONG_PTR Style;
    Style=GetWindowLongPtr(this->hwndTB,GWL_STYLE);
    if (bEnable)
        Style&=~CCS_NOPARENTALIGN ;
    else
        Style|=CCS_NOPARENTALIGN ;

    // apply new style
    bRet=SetWindowLongPtr(this->hwndTB,GWL_STYLE,Style);

    // force item to be redrawn
    RedrawWindow( this->hwndTB,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: Autosize
// Object: auto resize toolbar WARNING must be called after parent has been updated (redrawn inside WM_SIZE message processing)
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CToolbar::Autosize()
{
    SendMessage(this->hwndTB, TB_AUTOSIZE, 0, 0);
    // force item to be redrawn
    RedrawWindow( this->hwndTB,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);

}

//-----------------------------------------------------------------------------
// Name: GetControlHandle
// Object: get control toolbar hwnd
// Parameters :
//     in  : 
//     out :
//     return : control hwnd
//-----------------------------------------------------------------------------
HWND CToolbar::GetControlHandle()
{
    return this->hwndTB;
}

//-----------------------------------------------------------------------------
// Name: GetButtonCount
// Object: get number of button in toolbar
// Parameters :
//     in  : 
//     out :
//     return : number of buttons
//-----------------------------------------------------------------------------
int CToolbar::GetButtonCount()
{
    return (int)SendMessage(this->hwndTB,(UINT) TB_BUTTONCOUNT, 0,0); 
}

//-----------------------------------------------------------------------------
// Name: EnableButton
// Object: set button to enable or disable
// Parameters :
//     in  : int ButtonID : button index in toolbar
//           BOOL bEnable : TRUE to put button in an enabled state, FALSE else
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
void CToolbar::EnableButton(int ButtonID,BOOL bEnable)
{
    SendMessage(
                (HWND) this->hwndTB,         
                (UINT) TB_ENABLEBUTTON,
                (WPARAM) ButtonID,          // = (WPARAM) (int) idButton
                (LPARAM) MAKELONG(bEnable,0)// = (LPARAM) MAKELONG (fEnable, 0)
                ); 
}

//-----------------------------------------------------------------------------
// Name: RemoveButton
// Object: remove button from toolbar
// Parameters :
//     in  : int ButtonID : button id in toolbar
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::RemoveButton(int ButtonID)
{
    // get button info
    TBBUTTONINFO  ButtonInfo={0};
    // retrieve button param
    ButtonInfo.dwMask=TBIF_LPARAM;
    ButtonInfo.cbSize=sizeof(TBBUTTONINFO);

    // get button info
    if (SendMessage( this->hwndTB,(UINT) TB_GETBUTTONINFO,(WPARAM) ButtonID,(LPARAM) &ButtonInfo)==-1)
        return FALSE;

    // free lparam memory
    if (ButtonInfo.lParam)
        free((void*)ButtonInfo.lParam);

    // remove button
    return (BOOL)SendMessage((HWND)this->hwndTB,(UINT) TB_DELETEBUTTON,(WPARAM) ButtonID,0);
}


//-----------------------------------------------------------------------------
// Name: GetButtonIndex
// Object: retrieve button index from button id
// Parameters :
//     in  : int ButtonID: button id that will be send by WM_COMMAND message in DialogProc
//     out :
//     return : button index if found, -1 on error
//-----------------------------------------------------------------------------
int CToolbar::GetButtonIndex(int ButtonID)
{
    TBBUTTONINFO  ButtonInfo={0};
    ButtonInfo.cbSize=sizeof(TBBUTTONINFO);

    return (int)SendMessage( this->hwndTB,
            (UINT) TB_GETBUTTONINFO,
            (WPARAM) ButtonID,//Command identifier of the button
            (LPARAM) &ButtonInfo);
}
//-----------------------------------------------------------------------------
// Name: GetButtonId
// Object: retrieve button id from button index
// Parameters :
//     in  : int ButtonIndex: 0 based index button in toolbar
//     out :
//     return : button index if found, -1 on error
//-----------------------------------------------------------------------------
int CToolbar::GetButtonId(int ButtonIndex)
{
    TBBUTTON  Button={0};
    if (SendMessage( this->hwndTB,(UINT) TB_GETBUTTON,(WPARAM) ButtonIndex,(LPARAM) &Button)==-1)
        return -1;
    return Button.idCommand;
}


//-----------------------------------------------------------------------------
// Name: AddSeparator
// Object: add separator to toolbar
// Parameters :
//     in  :
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CToolbar::AddSeparator()
{
    TBBUTTON Button={0};
    // add button to toolbar
    Button.iBitmap = I_IMAGENONE; 
    Button.idCommand = 0;
    Button.fsState = TBSTATE_ENABLED;
    Button.fsStyle = BTNS_SEP; 
    Button.dwData = 0; 
    Button.iString = 0; 

    SendMessage(hwndTB, TB_ADDBUTTONS, (WPARAM) 1, (LPARAM) (LPTBBUTTON) &Button);
}
//-----------------------------------------------------------------------------
// Name: AddButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddButton(int ButtonID,TCHAR* ButtonText)
{
    return this->AddButton(ButtonID,ButtonText,(TCHAR*)NULL);
}
//-----------------------------------------------------------------------------
// Name: AddButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          TCHAR* ToolTip : button tooltip 
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddButton(int ButtonID,TCHAR* ButtonText,TCHAR* ToolTip)
{
    TBBUTTON Button={0};

    // add button to toolbar
    Button.iBitmap = I_IMAGENONE; 
    Button.idCommand = ButtonID; 
    Button.fsState = TBSTATE_ENABLED; 
    Button.fsStyle = BTNS_SHOWTEXT|BTNS_AUTOSIZE; 
    Button.iString = (INT_PTR)ButtonText; 
    // store tooltip
    Button.dwData = (DWORD_PTR)new CToolbarButtonUserData(ToolTip,0);

    if (!SendMessage(hwndTB, TB_ADDBUTTONS, (WPARAM) 1, (LPARAM) (LPTBBUTTON) &Button))
        return FALSE;

    // resize toolbar if required
    SendMessage(this->hwndTB, TB_AUTOSIZE, 0, 0); 

   return TRUE;
}

//-----------------------------------------------------------------------------
// Name: AddButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          int IdIcon : icon for enable and disable state
//          TCHAR* ToolTip : button tooltip 
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddButton(int ButtonID,TCHAR* ButtonText,int IdIcon,TCHAR* ToolTip)
{
    return this->AddButton(ButtonID,ButtonText,IdIcon,IdIcon,ToolTip);
}
//-----------------------------------------------------------------------------
// Name: AddButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          int IdIcon : icon for enable and disable state
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddButton(int ButtonID,TCHAR* ButtonText,int IdIcon)
{
    return this->AddButton(ButtonID,ButtonText,BTNS_SHOWTEXT|BTNS_AUTOSIZE ,TBSTATE_ENABLED,IdIcon,IdIcon,IdIcon,NULL);
}

//-----------------------------------------------------------------------------
// Name: AddCheckButton
// Object: add check button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          int IdIcon : icon for enable and disable state
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddCheckButton(int ButtonID,int IdIcon,TCHAR* ToolTip)
{
    return this->AddCheckButton(ButtonID,IdIcon,IdIcon,IdIcon,ToolTip);
}

//-----------------------------------------------------------------------------
// Name: AddCheckButton
// Object: add check button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          int IdIcon : icon for enable state
//          int IdIconDisabled : icon for disable state
//          int IdIconHot : icon hot
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddCheckButton(int ButtonID,int IdIcon,int IdIconDisabled,int IdIconHot,TCHAR* ToolTip)
{
    return this->AddButton(ButtonID,NULL,BTNS_CHECK|BTNS_AUTOSIZE ,TBSTATE_ENABLED,IdIcon,IdIconDisabled,IdIconHot,ToolTip);
}

//-----------------------------------------------------------------------------
// Name: AddButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          int IdIcon : icon for enable and disable state
//          TCHAR* ToolTip : button tooltip
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddButton(int ButtonID,int IdIcon,TCHAR* ToolTip)
{
    return this->AddButton(ButtonID,NULL,BTNS_BUTTON|BTNS_AUTOSIZE ,TBSTATE_ENABLED,IdIcon,IdIcon,IdIcon,ToolTip);
}
//-----------------------------------------------------------------------------
// Name: AddButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          int IdIconEnable : icon for enable state
//          int IdIconDisable : icon for disable state
//          TCHAR* ToolTip : button tooltip
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddButton(int ButtonID,int IdIconEnable,int IdIconDisable,TCHAR* ToolTip)
{
    return this->AddButton(ButtonID,NULL,BTNS_BUTTON|BTNS_AUTOSIZE ,TBSTATE_ENABLED,IdIconEnable,IdIconDisable,IdIconEnable,ToolTip);
}

//-----------------------------------------------------------------------------
// Name: AddButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          int IdIconEnable : icon for enable state
//          int IdIconDisable : icon for disable state
//          int IdIconHot : icon for hot state (only for flat toolbars)
//          TCHAR* ToolTip : button tooltip
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddButton(int ButtonID,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip)
{
    return this->AddButton(ButtonID,NULL,BTNS_BUTTON|BTNS_AUTOSIZE ,TBSTATE_ENABLED,IdIconEnable,IdIconDisable,IdIconHot,ToolTip);
}

//-----------------------------------------------------------------------------
// Name: AddButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          int IdIconEnable : icon for enable state
//          int IdIconDisable : icon for disable state
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable)
{
    return this->AddButton(ButtonID,ButtonText,BTNS_SHOWTEXT|BTNS_AUTOSIZE ,TBSTATE_ENABLED,IdIconEnable,IdIconDisable,IdIconEnable,NULL);
}
//-----------------------------------------------------------------------------
// Name: AddButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          int IdIconEnable : icon for enable state
//          int IdIconDisable : icon for disable state
//          TCHAR* ToolTip : button tooltip
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable,TCHAR* ToolTip)
{
    return this->AddButton(ButtonID,ButtonText,BTNS_SHOWTEXT|BTNS_AUTOSIZE ,TBSTATE_ENABLED,IdIconEnable,IdIconDisable,IdIconEnable,ToolTip);
}

//-----------------------------------------------------------------------------
// Name: AddButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          int IdIconEnable : icon for enable state
//          int IdIconDisable : icon for disable state
//          int IdIconHot : icon for hot state (only for flat toolbars)
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable,int IdIconHot)
{
    return this->AddButton(ButtonID,ButtonText,BTNS_SHOWTEXT|BTNS_AUTOSIZE ,TBSTATE_ENABLED,IdIconEnable,IdIconDisable,IdIconHot,NULL);
}
//-----------------------------------------------------------------------------
// Name: AddButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          int IdIconEnable : icon for enable state
//          int IdIconDisable : icon for disable state
//          int IdIconHot : icon for hot state (only for flat toolbars)
//          TCHAR* ToolTip : button tooltip
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip)
{
    return this->AddButton(ButtonID,ButtonText,BTNS_SHOWTEXT|BTNS_AUTOSIZE ,TBSTATE_ENABLED,IdIconEnable,IdIconDisable,IdIconHot,ToolTip);
}
//-----------------------------------------------------------------------------
// Name: AddButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          BYTE ButtonStyle : button style
//          BYTE ButtonState : button state
//          int IdIconEnable : icon for enable state
//          int IdIconDisable : icon for disablestate
//          int IdIconHot : icon for hot state (only for flat toolbars)
//          TCHAR* ToolTip : button tooltip.
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddButton(int ButtonID,TCHAR* ButtonText,BYTE ButtonStyle,BYTE ButtonState,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip)
{
    return this->AddButton(ButtonID,ButtonText,ButtonStyle,ButtonState,IdIconEnable,IdIconDisable,IdIconHot,ToolTip,NULL);
}

//-----------------------------------------------------------------------------
// Name: AddButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          BYTE ButtonStyle : button style
//          BYTE ButtonState : button state
//          int IdIconEnable : icon for enable state
//          int IdIconDisable : icon for disablestate
//          int IdIconHot : icon for hot state (only for flat toolbars)
//          TCHAR* ToolTip : button tooltip.
//          CPopUpMenu* PopUpMenu : Popup menu (only for drop down buttons)
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddButton(int ButtonID,TCHAR* ButtonText,BYTE ButtonStyle,BYTE ButtonState,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip,CPopUpMenu* PopUpMenu)
{
    TBBUTTON Button={0};
    int iIconEnable;
    int iIconDisable;
    int iIconHot;
    HICON hicon;

    // load enable icon
    hicon=(HICON)LoadImage(this->hInstance, MAKEINTRESOURCE(IdIconEnable),IMAGE_ICON,0,0,LR_SHARED);
    if (!hicon)
        return FALSE;
    // add it to enabled list
    iIconEnable=ImageList_AddIcon(this->hImgList, hicon);
    if (iIconEnable==-1)
        return FALSE;

    // load disable icon
    hicon=(HICON)LoadImage(this->hInstance, MAKEINTRESOURCE(IdIconDisable),IMAGE_ICON,0,0,LR_SHARED);
    if (!hicon)
    {
        // remove icon from enable list to keep same index in the lists
        ImageList_Remove(this->hImgList,iIconEnable);
        return FALSE;
    }
    // add it to disabled list
    iIconDisable=ImageList_AddIcon(this->hImgListDisabled, hicon); 
    if (iIconDisable==-1)
    {
        // remove icon from enable list to keep same index in the lists
        ImageList_Remove(this->hImgList,iIconEnable);
        return FALSE;
    }

    // load hot icon
    hicon=(HICON)LoadImage(this->hInstance, MAKEINTRESOURCE(IdIconHot),IMAGE_ICON,0,0,LR_SHARED);
    if (!hicon)
    {
        // remove icon from enable and disables list to keep same index in the lists
        ImageList_Remove(this->hImgList,iIconEnable);
        ImageList_Remove(this->hImgListDisabled,iIconDisable);
        return FALSE;
    }
    // add it to hot list
    iIconHot=ImageList_AddIcon(this->hImgListHot, hicon); 
    if (iIconHot==-1)
    {
        // remove icon from enable and disables list to keep same index in the lists
        ImageList_Remove(this->hImgList,iIconEnable);
        ImageList_Remove(this->hImgListDisabled,iIconDisable);
        return FALSE;
    }

    // icon index should be the same in all imagelists
    if ((iIconEnable!=iIconDisable)
        ||(iIconEnable!=iIconHot))
        return FALSE;


    // add button to toolbar
    Button.iBitmap = iIconEnable; 
    Button.idCommand = ButtonID; 
    Button.fsState = ButtonState; 
    Button.fsStyle = ButtonStyle;
    Button.iString = (INT_PTR)ButtonText;

    // store UserData
    Button.dwData = (DWORD_PTR)new CToolbarButtonUserData(ToolTip,PopUpMenu);

    if (!SendMessage(this->hwndTB, TB_ADDBUTTONS, (WPARAM) 1, (LPARAM) (LPTBBUTTON) &Button))
        return FALSE;

    // resize toolbar if required
    SendMessage(this->hwndTB, TB_AUTOSIZE, 0, 0); 

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: AddDropDownButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          BYTE ButtonState : button state
//          int IdIcon : icon id
//          TCHAR* ToolTip : button tooltip.
//          CPopUpMenu* PopUpMenu : Popup menu (only for drop down buttons)
//          BOOL bWholeDropDown : TRUE for full drop down (arrow not separeted)
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddDropDownButton(int ButtonID,int IdIcon,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,BOOL bWholeDropDown)
{
    return this->AddDropDownButton(ButtonID,NULL,IdIcon,ToolTip,PopUpMenu,bWholeDropDown);
}

//-----------------------------------------------------------------------------
// Name: AddDropDownButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          BYTE ButtonState : button state
//          int IdIcon : icon id
//          TCHAR* ToolTip : button tooltip.
//          CPopUpMenu* PopUpMenu : Popup menu (only for drop down buttons)
//          BOOL bWholeDropDown : TRUE for full drop down (arrow not separeted)
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddDropDownButton(int ButtonID,TCHAR* ButtonText,int IdIcon,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,BOOL bWholeDropDown)
{
    return this->AddDropDownButton(ButtonID,ButtonText,TBSTATE_ENABLED,IdIcon,IdIcon,IdIcon,ToolTip,PopUpMenu,bWholeDropDown);
}

//-----------------------------------------------------------------------------
// Name: AddDropDownButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          int IdIconEnable : icon for enable state
//          int IdIconDisable : icon for disablestate
//          int IdIconHot : icon for hot state (only for flat toolbars)
//          TCHAR* ToolTip : button tooltip.
//          CPopUpMenu* PopUpMenu : Popup menu (only for drop down buttons)
//          BOOL bWholeDropDown : TRUE for full drop down (arrow not separeted)
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddDropDownButton(int ButtonID,TCHAR* ButtonText,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,BOOL bWholeDropDown)
{
    return this->AddDropDownButton(ButtonID,ButtonText,TBSTATE_ENABLED,IdIconEnable,IdIconDisable,IdIconHot,ToolTip,PopUpMenu,bWholeDropDown);
}

//-----------------------------------------------------------------------------
// Name: AddDropDownButton
// Object: add button to toolbar
// Parameters :
//     in  : int ButtonID : button id that will be send by WM_COMMAND message in DialogProc
//          TCHAR* ButtonText : button visible text
//          BYTE ButtonState : button state
//          int IdIconEnable : icon for enable state
//          int IdIconDisable : icon for disablestate
//          int IdIconHot : icon for hot state (only for flat toolbars)
//          TCHAR* ToolTip : button tooltip.
//          CPopUpMenu* PopUpMenu : Popup menu (only for drop down buttons)
//          BOOL bWholeDropDown : TRUE for full drop down (arrow not separeted)
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::AddDropDownButton(int ButtonID,TCHAR* ButtonText,BYTE ButtonState,int IdIconEnable,int IdIconDisable,int IdIconHot,TCHAR* ToolTip,CPopUpMenu* PopUpMenu,BOOL bWholeDropDown)
{
    BYTE ButtonStyle;
    if (bWholeDropDown)
        ButtonStyle=BTNS_WHOLEDROPDOWN;
    else
        ButtonStyle=BTNS_DROPDOWN;

    ButtonStyle|=BTNS_AUTOSIZE;

    return this->AddButton(ButtonID,ButtonText,ButtonStyle,ButtonState,IdIconEnable,IdIconDisable,IdIconHot,ToolTip,PopUpMenu);
}

//-----------------------------------------------------------------------------
// Name: GetButtonDropDownMenu
// Object: get drop down menu associated to button
// Parameters :
//     in  : int ButtonID : button id
//     out :
//     return : CPopUpMenu object pointer on success, NULL if no pop up menu associated (or on error)
//-----------------------------------------------------------------------------
CPopUpMenu* CToolbar::GetButtonDropDownMenu(int ButtonID)
{
    TBBUTTONINFO  ButtonInfo={0};
    // retrieve button param
    ButtonInfo.dwMask=TBIF_LPARAM;
    ButtonInfo.cbSize=sizeof(TBBUTTONINFO);
    if (SendMessage( this->hwndTB,(UINT) TB_GETBUTTONINFO,(WPARAM) ButtonID,(LPARAM) &ButtonInfo)==-1)
        return NULL;

    if (IsBadReadPtr((PVOID)ButtonInfo.lParam,sizeof(CToolbarButtonUserData)))
        return NULL;

    return ((CToolbarButtonUserData*)ButtonInfo.lParam)->PopUpMenu;
}

//-----------------------------------------------------------------------------
// Name: ReplaceIcon
// Object: change icon of a button
// Parameters :
//     in  : CToolbar::ImageListType ImgListType : image liste CToolbar::ImageListTypeXXX
//           int ButtonID : button id
//           int IdNewIcon : resource icon id
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::ReplaceIcon(int ButtonID,CToolbar::ImageListType ImgListType,int IdNewIcon)
{
    HIMAGELIST hImgL;
    HICON hicon;

    // get image list
    if (ImgListType==CToolbar::ImageListTypeEnable)
        hImgL=this->hImgList;
    else if (ImgListType==CToolbar::ImageListTypeDisable)
        hImgL=this->hImgListDisabled;
    else if (ImgListType==CToolbar::ImageListTypeHot)
        hImgL=this->hImgListHot;
    else
        return FALSE;


    // retrieve image index
    TBBUTTONINFO  ButtonInfo={0};
    ButtonInfo.dwMask=TBIF_IMAGE;
    ButtonInfo.cbSize=sizeof(TBBUTTONINFO);
    if (SendMessage( this->hwndTB,(UINT) TB_GETBUTTONINFO,(WPARAM) ButtonID,(LPARAM) &ButtonInfo)==-1)
        return FALSE;


    // load icon
    hicon=(HICON)LoadImage(this->hInstance, MAKEINTRESOURCE(IdNewIcon),IMAGE_ICON,0,0,LR_SHARED);
    if (!hicon)
        return FALSE;

    // replace icon
    if(ImageList_ReplaceIcon(hImgL,ButtonInfo.iImage,hicon)==-1)
        return FALSE;


    // force item to be redrawn
    RedrawWindow( this->hwndTB,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);

    return TRUE;

}

//-----------------------------------------------------------------------------
// Name: ReplaceText
// Object: change text of a button
// Parameters :
//     in  : int ButtonID : button id
//           TCHAR* NewText : new text associated with icon. Put to NULL to remove text
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::ReplaceText(int ButtonID,TCHAR* NewText)
{
    TBBUTTONINFO  ButtonInfo={0};
    // retrieve button param
    ButtonInfo.dwMask=TBIF_TEXT;
    ButtonInfo.cbSize=sizeof(TBBUTTONINFO);
    ButtonInfo.pszText=NewText;
    
    if (SendMessage( this->hwndTB,(UINT) TB_SETBUTTONINFO,(WPARAM) ButtonID,(LPARAM) &ButtonInfo)==-1)
        return FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetButtonState
// Object: set state of a button
// Parameters :
//     in  : int ButtonID : button index in toolbar
//           BYTE State : button state (TBSTATE_CHECKED ,TBSTYLE_CHECK ,TBSTATE_ELLIPSES ,TBSTATE_ENABLED ,
//                        TBSTATE_HIDDEN ,TBSTATE_INDETERMINATE ,TBSTATE_MARKED ,TBSTATE_PRESSED ,TBSTATE_WRAP )
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::SetButtonState(int ButtonID,BYTE State)
{
    TBBUTTONINFO  ButtonInfo={0};
    // set button param
    ButtonInfo.dwMask=TBIF_STATE;
    ButtonInfo.fsState=State;
    ButtonInfo.cbSize=sizeof(TBBUTTONINFO);

    // set button info
    return (BOOL)SendMessage( this->hwndTB,(UINT) TB_SETBUTTONINFO,(WPARAM) ButtonID,(LPARAM) &ButtonInfo);
}

//-----------------------------------------------------------------------------
// Name: GetButtonState
// Object: get state of a button
// Parameters :
//     in  : int ButtonID : button id
//     out :
//     return : Button state (TBSTATE_CHECKED , TBSTATE_ELLIPSES ,TBSTATE_ENABLED ,
//                            TBSTATE_HIDDEN ,TBSTATE_INDETERMINATE ,TBSTATE_MARKED ,TBSTATE_PRESSED ,TBSTATE_WRAP )
//-----------------------------------------------------------------------------
BYTE CToolbar::GetButtonState(int ButtonID)
{
    TBBUTTONINFO  ButtonInfo={0};
    // retrieve button param
    ButtonInfo.dwMask=TBIF_STATE;
    ButtonInfo.cbSize=sizeof(TBBUTTONINFO);

    // get button info
    if (SendMessage( this->hwndTB,(UINT) TB_GETBUTTONINFO,(WPARAM) ButtonID,(LPARAM) &ButtonInfo)==-1)
        return 0;

    return ButtonInfo.fsState;
}
//-----------------------------------------------------------------------------
// Name: GetButtonStyle
// Object: get state of a button
// Parameters :
//     in  : int ButtonID : button id
//     out :
//     return : Button Style TBSTYLE_CHECK, TBSTYLE_DROPDOWN, ...
//-----------------------------------------------------------------------------
BYTE CToolbar::GetButtonStyle(int ButtonID)
{
    TBBUTTONINFO  ButtonInfo={0};
    // retrieve button param
    ButtonInfo.dwMask=TBIF_STYLE;
    ButtonInfo.cbSize=sizeof(TBBUTTONINFO);

    // get button info
    if (SendMessage( this->hwndTB,(UINT) TB_GETBUTTONINFO,(WPARAM) ButtonID,(LPARAM) &ButtonInfo)==-1)
        return 0;

    return ButtonInfo.fsStyle;
}

//-----------------------------------------------------------------------------
// Name: ReplaceText
// Object: change text of a button
// Parameters :
//     in  : int ButtonID : button id
//           TCHAR* NewToolTipText : new text associated with tooltip
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::ReplaceToolTipText(int ButtonID,TCHAR* NewToolTipText)
{
    TBBUTTONINFO  ButtonInfo={0};
    // retrieve button param
    ButtonInfo.dwMask=TBIF_LPARAM;
    ButtonInfo.cbSize=sizeof(TBBUTTONINFO);

    // get button info
    if (SendMessage( this->hwndTB,(UINT) TB_GETBUTTONINFO,(WPARAM) ButtonID,(LPARAM) &ButtonInfo)==-1)
        return FALSE;

    if (IsBadReadPtr((PVOID)ButtonInfo.lParam,sizeof(CToolbarButtonUserData)))
        return FALSE;

    // change tooltip
    ((CToolbarButtonUserData*)ButtonInfo.lParam)->SetToolTip(NewToolTipText);

    // set button info
    if (SendMessage( this->hwndTB,(UINT) TB_SETBUTTONINFO,(WPARAM) ButtonID,(LPARAM) &ButtonInfo)==-1)
        return FALSE;

    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: OnNotify
// Object: must called on WM_NOTIFY messages
// Parameters :
//     in  : WPARAM wParam, LPARAM lParam : the parameters given in dialogproc
//     out :
//     return : TRUE if message has been proceed
//-----------------------------------------------------------------------------
BOOL CToolbar::OnNotify(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    // check that message is for tooltip
    if ((this->hwndToolTip!=((LPNMHDR) lParam)->hwndFrom)
        &&(this->hwndTB!=((LPNMHDR) lParam)->hwndFrom))
        return FALSE;

    switch (((LPNMHDR) lParam)->code) 
	{ 
        case TTN_GETDISPINFO: 
            { 
                UINT_PTR idButton;
                LPTOOLTIPTEXT lpttt;
                TBBUTTONINFO  ButtonInfo={0};

                // retrieve tooltip
                lpttt = (LPTOOLTIPTEXT) lParam; 
                lpttt->hinst = this->hInstance; 

                // Specify the resource identifier of the descriptive 
                // text for the given button. 
                idButton = lpttt->hdr.idFrom; 

                // retrieve button param
                ButtonInfo.dwMask=TBIF_LPARAM;
                ButtonInfo.cbSize=sizeof(TBBUTTONINFO);
                if (SendMessage( this->hwndTB,(UINT) TB_GETBUTTONINFO,(WPARAM) idButton,(LPARAM) &ButtonInfo)==-1)
                    return TRUE;

                if (IsBadReadPtr((PVOID)ButtonInfo.lParam,sizeof(CToolbarButtonUserData)))
                    return TRUE;

                // set tooltip text
                lpttt->lpszText = ((CToolbarButtonUserData*)ButtonInfo.lParam)->ToolTip;

            } 
            break;

        case TBN_DROPDOWN:
                {
                    // Specify the resource identifier of the descriptive 
                    // text for the given button. 
                    UINT_PTR idButton = ((LPNMTOOLBAR) lParam)->iItem; 

                    // get associated pop up menu
                    CPopUpMenu* PopUpMenu;
                    PopUpMenu=this->GetButtonDropDownMenu(idButton);
                    if (!PopUpMenu)
                        return TRUE;

                    // get button rect 
                    RECT      rc;
                    SendMessage(this->hwndTB, TB_GETRECT,idButton, (LPARAM)&rc);
                    MapWindowPoints(((LPNMTOOLBAR)lParam)->hdr.hwndFrom,HWND_DESKTOP, (LPPOINT)&rc, 2); 
                    // show menu
                    
                    UINT MenuId;
                    MenuId=PopUpMenu->Show(rc.left, rc.bottom, this->hwndTB);
                    if (MenuId)// if a menu has been selected
                    {
                        if (!IsBadCodePtr((FARPROC)this->pDropDownMenuCallBack))
                            this->pDropDownMenuCallBack(PopUpMenu,MenuId,this->DropDownMenuUserParam);
                    }
                }
                break;
        default:
            return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: SetDropDownMenuCallBack
// Object: set drop down menu callback
// Parameters :
//     in  : tagDropDownMenuCallBack pDropDownMenuCallBack : pointer to callback
//           PVOID UserParam : user parameter that will be transmit to callback
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::SetDropDownMenuCallBack(tagDropDownMenuCallBack pDropDownMenuCallBack,PVOID UserParam)
{
    if (IsBadCodePtr((FARPROC)pDropDownMenuCallBack))
        return FALSE;

    this->pDropDownMenuCallBack=pDropDownMenuCallBack;
    this->DropDownMenuUserParam=UserParam;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetDropDownMenuCallBack
// Object: get drop down menu callback
// Parameters :
//     in  : tagDropDownMenuCallBack* ppDropDownMenuCallBack : pointer to callback
//           PVOID* pUserParam : user parameter that will be transmit to callback
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CToolbar::GetDropDownMenuCallBack(tagDropDownMenuCallBack* ppDropDownMenuCallBack,PVOID* pUserParam)
{
    *ppDropDownMenuCallBack=this->pDropDownMenuCallBack;
    *pUserParam=this->DropDownMenuUserParam;
    return TRUE;
}