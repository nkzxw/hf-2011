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
// Object: class helper for dialog window
//-----------------------------------------------------------------------------

#include "DialogHelper.h"

//-----------------------------------------------------------------------------
// Name: SetIcon
// Object: set dialog big and small icon
// Parameters :
//     in  : HWND hWnd : window handle
//           int IdIconBig : id of big icon
//           int IdIconSmall : id of small icon
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDialogHelper::SetIcon(HWND hWnd,int IdIconBig,int IdIconSmall)
{
    HINSTANCE hInstance=(HINSTANCE)(GetWindowLongPtr(hWnd, GWLP_HINSTANCE));
    CDialogHelper::SetIcon(hInstance,hWnd,IdIconBig,IdIconSmall);
}

//-----------------------------------------------------------------------------
// Name: SetIcon
// Object: set dialog big and small icon
// Parameters :
//     in  : HINSTANCE hInstance : application instance or dll hmodule
//           HWND hWnd : window handle
//           int IdIconBig : id of big icon
//           int IdIconSmall : id of small icon
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDialogHelper::SetIcon(HINSTANCE hInstance,HWND hWnd,int IdIconBig,int IdIconSmall)
{
    SendMessage(hWnd, WM_SETICON, ICON_BIG,
        (LPARAM)LoadImage(hInstance, MAKEINTRESOURCE(IdIconBig),IMAGE_ICON,0,0,LR_SHARED)
        );
    SendMessage(hWnd, WM_SETICON, ICON_SMALL,
        (LPARAM)LoadImage(hInstance, MAKEINTRESOURCE(IdIconSmall),IMAGE_ICON,0,0,LR_SHARED)
        );
}
//-----------------------------------------------------------------------------
// Name: SetIcon
// Object: set dialog big and small icon
// Parameters :
//     in  : HWND hWnd : window handle
//           HICON hIcon : handle of big and small icon
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDialogHelper::SetIcon(HWND hWnd,HICON hIcon)
{
    CDialogHelper::SetIcon(hWnd,hIcon,hIcon);
}
//-----------------------------------------------------------------------------
// Name: SetIcon
// Object: set dialog big and small icon
// Parameters :
//     in  : HWND hWnd : window handle
//           HICON hIconBig : handle of big icon
//           HICON hIconSmall : handle of small icon
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDialogHelper::SetIcon(HWND hWnd,HICON hIconBig,HICON hIconSmall)
{
    SendMessage(hWnd, WM_SETICON, ICON_BIG,(LPARAM)hIconBig );
    SendMessage(hWnd, WM_SETICON, ICON_SMALL,(LPARAM)hIconSmall);
}

//-----------------------------------------------------------------------------
// Name: SetIcon
// Object: set dialog big and small icon
// Parameters :
//     in  : HWND hWnd : window handle
//           int IdIcon : id of big and small icon
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDialogHelper::SetIcon(HWND hWnd,int IdIcon)
{
    CDialogHelper::SetIcon(hWnd,IdIcon,IdIcon);
}

//-----------------------------------------------------------------------------
// Name: SetIcon
// Object: set dialog big and small icon
// Parameters :
//     in  : HINSTANCE hInstance : application instance or dll hmodule
//           HWND hWnd : window handle
//           int IdIcon : id of big and small icon
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CDialogHelper::SetIcon(HINSTANCE hInstance,HWND hWnd,int IdIcon)
{
    CDialogHelper::SetIcon(hInstance,hWnd,IdIcon,IdIcon);
}

//-----------------------------------------------------------------------------
// Name: GetClientWindowRect
// Object: translate lpRect from Screen coordinates to Client Coordinates
//          (used for resizing controls)
// Parameters :
//     in  : HWND hDlg : dialog handle
//           HWND hItem : item handle
//     out : LPRECT lpRect : Rect in dialog coordinates
//     return : 
//-----------------------------------------------------------------------------
BOOL CDialogHelper::GetClientWindowRect(HWND hDlg,HWND hItem,LPRECT lpRect)
{
    if (!GetWindowRect(hItem,lpRect))
        return FALSE;
    if(!CDialogHelper::ScreenToClient(hDlg,lpRect))
        return FALSE;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetDialogInternalRect
// Object: get dialog internal rect (usable space)
// Parameters :
//     in  : HWND hDlg : dialog handle
//     out : LPRECT lpRect : Rect in dialog coordinates
//     return : 
//-----------------------------------------------------------------------------
BOOL CDialogHelper::GetDialogInternalRect(HWND hDlg,LPRECT lpRect)
{
    return ::GetClientRect(hDlg,lpRect);
}

//-----------------------------------------------------------------------------
// Name: ScreenToClient
// Object: translate lpRect from Screen coordinates to Client Coordinates
// Parameters :
//     in  : HWND hDlg : dialog handle
//     in out : LPRECT lpRect : in  : Rect in screen coordinates
//                              out : Rect in dialog coordinates
//     return : 
//-----------------------------------------------------------------------------
BOOL CDialogHelper::ScreenToClient (HWND hDlg, LPRECT pRect)
{
    /*
    POINT p;
    p.x=lpRect->left;
    p.y=lpRect->top;
    ScreenToClient(hDlg,&p);
    lpRect->left=p.x;
    lpRect->top=p.y;
    p.x=lpRect->right;
    p.y=lpRect->bottom;
    ScreenToClient(hDlg,&p);
    lpRect->right=p.x;
    lpRect->bottom=p.y;
    */
    if (!::ScreenToClient (hDlg, (LPPOINT)pRect))
        return FALSE;
    if(!::ScreenToClient (hDlg, ((LPPOINT)pRect)+1))
        return FALSE;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ClientToScreen
// Object: translate lpRect from Screen coordinates to Client Coordinates
// Parameters :
//     in  : HWND hDlg : dialog handle
//     in out : LPRECT lpRect : in  : Rect in dialog coordinates
//                              out : Rect in screen coordinates
//     return : 
//-----------------------------------------------------------------------------
BOOL CDialogHelper::ClientToScreen (HWND hDlg, LPRECT pRect)
{
    if(!::ClientToScreen (hDlg, (LPPOINT)pRect))
        return FALSE;
    if(!::ClientToScreen (hDlg, ((LPPOINT)pRect)+1))
        return FALSE;
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: Redraw
// Object: redraw an item
// Parameters :
//     in  : HWND hItem : handle of item to be redraw
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CDialogHelper::Redraw(HWND hItem)
{
    return ::RedrawWindow( hItem,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
}

//-----------------------------------------------------------------------------
// Name: SetTransparency
// Object: set dialog transparency
// Parameters :
//     in  : HWND hWnd : window handle
//           BYTE Opacity : 0 for transparent window, 100 for an opaque window
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CDialogHelper::SetTransparency(HWND hWnd,BYTE Opacity)
{
    LONG_PTR WindowLong;
    WindowLong=::GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    if (Opacity<100)
    {
        if ((WindowLong&WS_EX_LAYERED)==0)
            // Set WS_EX_LAYERED on this window 
            ::SetWindowLongPtr(hWnd,GWL_EXSTYLE, WindowLong | WS_EX_LAYERED);

        // Make this window Opacity% alpha
        ::SetLayeredWindowAttributes(hWnd, 0, (255 * Opacity) / 100, LWA_ALPHA);
        // Note that the third parameter of SetLayeredWindowAttributes
        // is a value that ranges from 0 to 255, with 0 making the window
        // completely transparent and 255 making it completely opaque.
        // This parameter mimics the more versatile BLENDFUNCTION of the AlphaBlend function.

        // dont't use CDialogHelper::Redraw(hWnd) as it's provide a flashing effect for fading
        RedrawWindow( hWnd,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW );
    }
    else
    {
        // To make this window completely opaque again, 
        // remove the WS_EX_LAYERED bit by calling SetWindowLongPtr and 
        // then ask the window to repaint. Removing the bit is desired 
        // to let the system know that it can free up some memory associated 
        // with layering and redirection. The code might look like this:
        // Remove WS_EX_LAYERED from this window styles
        if ((WindowLong&WS_EX_LAYERED)!=0)
            ::SetWindowLongPtr(hWnd,GWL_EXSTYLE,WindowLong & ~WS_EX_LAYERED);
        CDialogHelper::Redraw(hWnd);
    }
}

//-----------------------------------------------------------------------------
// Name: SetTransparentColor
// Object: set dialog transparent color
// Parameters :
//     in  : HWND hWnd : window handle
//           BYTE Opacity : 0 for transparent window, 100 for an opaque window
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CDialogHelper::SetTransparentColor(HWND hWnd,COLORREF crKey)
{
    LONG_PTR WindowLong;
    WindowLong=::GetWindowLongPtr(hWnd, GWL_EXSTYLE);

    if ((WindowLong&WS_EX_LAYERED)==0)
        // Set WS_EX_LAYERED on this window 
        ::SetWindowLongPtr(hWnd,GWL_EXSTYLE, WindowLong | WS_EX_LAYERED);

    // set transparent color
    ::SetLayeredWindowAttributes(hWnd, crKey, 0, LWA_COLORKEY);

    CDialogHelper::Redraw(hWnd);
}

//-----------------------------------------------------------------------------
// Name: Fading
// Object: Make a fading for specified window
// Parameters :
//     in  : HWND hWnd : window handle
//           BYTE BeginOpacity : begin opacity for fading (0 for transparent window, 100 for an opaque window)
//           BYTE EndOpacity : end opacity for fading (0 for transparent window, 100 for an opaque window)
//           BYTE Step : step size between 2 drawn transparency
//                       (number of step will be abs(EndOpacity-BeginOpacity)/Step)
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CDialogHelper::Fading(HWND hWnd,BYTE BeginOpacity,BYTE EndOpacity,BYTE Step)
{
    BYTE NbStep;
    BYTE CurrentOpacity;
    int StepDelta=Step;
    // check opacity parameters
    BeginOpacity=BeginOpacity<100?BeginOpacity:100;
    EndOpacity=EndOpacity<100?EndOpacity:100;
    // check if a fading is needed
    if (BeginOpacity==EndOpacity)
    {
        CDialogHelper::SetTransparency(hWnd,BeginOpacity);
        return;
    }
    // compute required step
    if (BeginOpacity<EndOpacity)
    {
        NbStep=(BYTE)((EndOpacity-BeginOpacity)/StepDelta);
    }
    else
    {
        NbStep=(BYTE)((BeginOpacity-EndOpacity)/StepDelta);
        StepDelta=-StepDelta;
    }
    CurrentOpacity=BeginOpacity;
    for (BYTE cnt=0;cnt<NbStep;cnt++)
    {
        CDialogHelper::SetTransparency(hWnd,CurrentOpacity);
        CurrentOpacity=(BYTE)(CurrentOpacity+StepDelta);
    }
    CDialogHelper::SetTransparency(hWnd,EndOpacity);
}


//-----------------------------------------------------------------------------
// Name: ParseGroupForAction
// Object: call specified callback for each item of the group
// Parameters :
//     in  : HWND hWndGroup : group window handle
//           CDialogHelper::pfCallBackGroupAction CallBackFunction : callback called for each item
//           PVOID UserParam : parameter provided to callback
//     out : 
//     return : AND between all result of callback calls
//-----------------------------------------------------------------------------
BOOL CDialogHelper::ParseGroupForAction(HWND hWndGroup,CDialogHelper::pfCallBackGroupAction CallBackFunction,PVOID UserParam)
{
    BOOL bRet=TRUE;
    BOOL bFunctionRet=TRUE;
    if (!hWndGroup)
        return FALSE;
    HWND hWndCtrl;
    hWndCtrl=hWndGroup;
    // for each item of the group
    for(;;)
    {
        // call callback with item handle and UserParam
        bFunctionRet = CallBackFunction(hWndCtrl,UserParam);
        bRet=bRet && bFunctionRet;

        // get next window handle
        hWndCtrl = GetWindow( hWndCtrl, GW_HWNDNEXT );
        // if no next window or next window has WS_GROUP style (window outside of the current group)
        if ( (hWndCtrl==NULL) || (GetWindowLong(hWndCtrl,GWL_STYLE)&WS_GROUP))
            break;

    } 
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: CallBackRedrawGroup
// Object: called to redraw item of a group. Call CDialogHelper::Redraw
// Parameters :
//     in  : HWND Item : window handle
//     out : 
//     return : TRUE
//-----------------------------------------------------------------------------
BOOL CDialogHelper::CallBackRedrawGroup(HWND Item,PVOID UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);
    CDialogHelper::Redraw(Item);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RedrawGroup
// Object: Call CDialogHelper::Redraw for each item of a group
// Parameters :
//     in  : HWND hWndGroup : group window handle
//     out : 
//     return : TRUE
//-----------------------------------------------------------------------------
BOOL CDialogHelper::RedrawGroup(HWND hWndGroup)
{
    if(!hWndGroup)
        return FALSE;
    return CDialogHelper::ParseGroupForAction(hWndGroup,CDialogHelper::CallBackRedrawGroup,NULL);
}

//-----------------------------------------------------------------------------
// Name: CallBackShowGroup
// Object: called to show item of a group. Call ShowWindow API
// Parameters :
//     in  : HWND Item : window handle
//           PVOID UserParam : int nCmdShow of ShowWindow API
//     out : 
//     return : TRUE
//-----------------------------------------------------------------------------
BOOL CDialogHelper::CallBackShowGroup(HWND Item,PVOID UserParam)
{
    ShowWindow(Item,(int)UserParam);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ShowGroup
// Object: Call ShowWindow API for each item of a group
// Parameters :
//     in  : HWND hWndGroup : group window handle
//           int CmdShow : int nCmdShow parameter of ShowWindow API
//     out : 
//     return : TRUE
//-----------------------------------------------------------------------------
BOOL CDialogHelper::ShowGroup(HWND hWndGroup,int CmdShow)
{
    if(!hWndGroup)
        return FALSE;
    return CDialogHelper::ParseGroupForAction(hWndGroup,CDialogHelper::CallBackShowGroup,(PVOID)CmdShow);
}

//-----------------------------------------------------------------------------
// Name: CallBackShowGroup
// Object: called to enable item of a group. Call ShowWindow API
// Parameters :
//     in  : HWND Item : window handle
//           PVOID UserParam : BOOL bEnable parameter of EnableWindow API
//     out : 
//     return : TRUE
//-----------------------------------------------------------------------------
BOOL CDialogHelper::CallBackEnableGroup(HWND Item,PVOID UserParam)
{
    EnableWindow(Item,(BOOL)UserParam);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: EnableGroup
// Object: Call EnableWindow API for each item of a group
// Parameters :
//     in  : HWND hWndGroup : group window handle
//           BOOL bEnable : BOOL bEnable parameter of EnableWindow API
//     out : 
//     return : TRUE
//-----------------------------------------------------------------------------
BOOL CDialogHelper::EnableGroup(HWND hWndGroup,BOOL bEnable)
{
    if(!hWndGroup)
        return FALSE;
    return CDialogHelper::ParseGroupForAction(hWndGroup,CDialogHelper::CallBackEnableGroup,(PVOID)bEnable);
}

//-----------------------------------------------------------------------------
// Name: CallBackMoveGroup
// Object: called to move items of a group. Call SetWindowPos API
// Parameters :
//     in  : HWND Item : window handle
//           PVOID UserParam : pointer to a MOVE_GROUP_STRUCT containing 
//                              informations to set window position
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CDialogHelper::CallBackMoveGroup(HWND Item,PVOID UserParam)
{
    CDialogHelper::MOVE_GROUP_STRUCT* pMoveStruct;
    RECT Rect;
    // get struct pointer
    pMoveStruct=(MOVE_GROUP_STRUCT*)UserParam;

    // get old item position
    if(!CDialogHelper::GetClientWindowRect(pMoveStruct->hClient,Item,&Rect))
        return FALSE;
    // set new position applying displacement
    return SetWindowPos(Item,NULL,
        Rect.left+pMoveStruct->Point.x,Rect.top+pMoveStruct->Point.y,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
}

//-----------------------------------------------------------------------------
// Name: CallBackMoveGroup
// Object: called to move items of a group. Call SetWindowPos API
// Parameters :
//     in  : HWND hClient : handle client window from which point refer (generally handle to dialog)
//           HWND hWndGroup : handle to group window
//           POINT* pPoint : new group position in client coordinates
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CDialogHelper::MoveGroupTo(HWND hClient,HWND hWndGroup,POINT* pPoint)
{
    RECT Rect;
    CDialogHelper::MOVE_GROUP_STRUCT MoveStruct;
    BOOL bRet;

    // get window group Rect
    if(!CDialogHelper::GetClientWindowRect(hClient,hWndGroup,&Rect))
        return FALSE;

    // fill MOVE_GROUP_STRUCT
    MoveStruct.hClient=hClient;
    // compute displacement values
    MoveStruct.Point.x=pPoint->x-Rect.left;
    MoveStruct.Point.y=pPoint->y-Rect.top;

    // apply displacement for all items of the group
    bRet=CDialogHelper::ParseGroupForAction(hWndGroup,CDialogHelper::CallBackMoveGroup,&MoveStruct);

    // redraw group
    CDialogHelper::Redraw(hWndGroup);

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: SetWindowPosUnderItem
// Object: put item under WindowReferenceItem with space between control of SpaceBetweenControls
// Parameters :
//     in  : HWND hClient : handle client window from which point refer (generally handle to dialog)
//           HWND hWnd : handle to window to change position
//           HWND hWindowReferenceItem : handle of reference window
//           DWORD SpaceBetweenControls : space between controls
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CDialogHelper::SetWindowPosUnderItem(HWND hClient,HWND hWnd,HWND hWindowReferenceItem,DWORD SpaceBetweenControls)
{
    RECT Rect;
    if(!CDialogHelper::GetClientWindowRect(hClient,hWindowReferenceItem,&Rect))
        return FALSE;

    // set new position applying displacement
    return SetWindowPos(hWnd,NULL,
        Rect.left,Rect.bottom+SpaceBetweenControls,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
}
//-----------------------------------------------------------------------------
// Name: SetWindowPosAtRightOfItem
// Object: put item at right of WindowReferenceItem with space between control of SpaceBetweenControls
// Parameters :
//     in  : HWND hClient : handle client window from which point refer (generally handle to dialog)
//           HWND hWnd : handle to window to change position
//           HWND hWindowReferenceItem : handle of reference window
//           DWORD SpaceBetweenControls : space between controls
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CDialogHelper::SetWindowPosAtRightOfItem(HWND hClient,HWND hWnd,HWND hWindowReferenceItem,DWORD SpaceBetweenControls)
{
    RECT Rect;
    if(!CDialogHelper::GetClientWindowRect(hClient,hWindowReferenceItem,&Rect))
        return FALSE;

    // set new position applying displacement
    return SetWindowPos(hWnd,NULL,
        Rect.right+SpaceBetweenControls,Rect.top,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
}



//-----------------------------------------------------------------------------
// Name: SetGroupPosUnderWindow
// Object: put group under WindowReferenceItem with space between control of SpaceBetweenControls
// Parameters :
//     in  : HWND hClient : handle client window from which point refer (generally handle to dialog)
//           HWND hWnd : handle to group to change position
//           HWND hWindowReferenceItem : handle of reference window
//           DWORD SpaceBetweenControls : space between controls
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CDialogHelper::SetGroupPosUnderWindow(HWND hClient,HWND hWnd,HWND hWindowReferenceItem,DWORD SpaceBetweenControls)
{
    RECT Rect;
    POINT Point;
    if(!CDialogHelper::GetClientWindowRect(hClient,hWindowReferenceItem,&Rect))
        return FALSE;
    Point.x=Rect.left;
    Point.y=Rect.bottom+SpaceBetweenControls;
    return CDialogHelper::MoveGroupTo(hClient,hWnd,&Point);
}
//-----------------------------------------------------------------------------
// Name: SetGroupPosAtRightOfWindow
// Object: put item at right of WindowReferenceItem with space between control of SpaceBetweenControls
// Parameters :
//     in  : HWND hClient : handle client window from which point refer (generally handle to dialog)
//           HWND hWnd : handle to group to change position
//           HWND hWindowReferenceItem : handle of reference window
//           DWORD SpaceBetweenControls : space between controls
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CDialogHelper::SetGroupPosAtRightOfWindow(HWND hClient,HWND hWnd,HWND hWindowReferenceItem,DWORD SpaceBetweenControls)
{
    RECT Rect;
    POINT Point;
    if(!CDialogHelper::GetClientWindowRect(hClient,hWindowReferenceItem,&Rect))
        return FALSE;
    Point.x=Rect.right+SpaceBetweenControls;
    Point.y=Rect.top;
    return CDialogHelper::MoveGroupTo(hClient,hWnd,&Point);
}


//-----------------------------------------------------------------------------
// Name: CreateButton
// Object: create a button
// Parameters :
//     in  : HWND hWndParent : parent window handle
//           int x : x position relative to parent 
//           int y : y position relative to parent
//           int Width : button width
//           int Height : button height
//           int Identifier : button ID
//           LPCTSTR ButtonText : Button text
//     out : 
//     return : button HWND 
//-----------------------------------------------------------------------------
HWND CDialogHelper::CreateButton(HWND hWndParent,
                  int x,
                  int y,
                  int Width,
                  int Height,
                  int Identifier,
                  LPCTSTR ButtonText
                  )
{
    return ::CreateWindowEx( 
                            0,
                            _T("BUTTON"),   // predefined class 
                            ButtonText,       // button text 
                            WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_CENTER | BS_DEFPUSHBUTTON,  // styles 
                            // Size and position values are given explicitly, because 
                            // the CW_USEDEFAULT constant gives zero values for buttons. 
                            x,         // starting x position 
                            y,         // starting y position 
                            Width,        // button width 
                            Height,        // button height 
                            hWndParent,       // parent window 
                            (HMENU)Identifier,       // No menu 
                            (HINSTANCE) ::GetWindowLongPtr(hWndParent, GWLP_HINSTANCE), 
                            NULL);      // pointer not needed 
}

//-----------------------------------------------------------------------------
// Name: CreateStatic
// Object: create a static
// Parameters :
//     in  : HWND hWndParent : parent window handle
//           int x : x position relative to parent 
//           int y : y position relative to parent
//           int Width : static width
//           int Height : static height
//           int Identifier : static ID
//           LPCTSTR StaticText : static text
//     out : 
//     return : static HWND 
//-----------------------------------------------------------------------------
HWND CDialogHelper::CreateStatic(HWND hWndParent,
                  int x,
                  int y,
                  int Width,
                  int Height,
                  int Identifier,
                  LPCTSTR StaticText
                  )
{
    return ::CreateWindowEx( 
                            0,
                            _T("STATIC"),   // predefined class 
                            StaticText,       // button text 
                            WS_VISIBLE | WS_CHILD |SS_LEFT,  // styles 
                            // Size and position values are given explicitly, because 
                            // the CW_USEDEFAULT constant gives zero values for buttons. 
                            x,         // starting x position 
                            y,         // starting y position 
                            Width,        // button width 
                            Height,        // button height 
                            hWndParent,       // parent window 
                            (HMENU)Identifier,       // No menu 
                            (HINSTANCE) ::GetWindowLongPtr(hWndParent, GWLP_HINSTANCE), 
                            NULL);      // pointer not needed 
}

//-----------------------------------------------------------------------------
// Name: CreateStaticCentered
// Object: create a static with centered align
// Parameters :
//     in  : HWND hWndParent : parent window handle
//           int x : x position relative to parent 
//           int y : y position relative to parent
//           int Width : static width
//           int Height : static height
//           int Identifier : static ID
//           LPCTSTR StaticText : static text
//     out : 
//     return : static HWND 
//-----------------------------------------------------------------------------
HWND CDialogHelper::CreateStaticCentered(HWND hWndParent,
                                 int x,
                                 int y,
                                 int Width,
                                 int Height,
                                 int Identifier,
                                 LPCTSTR StaticText
                                 )
{
    return ::CreateWindowEx( 
                            0,
                            _T("STATIC"),   // predefined class 
                            StaticText,       // button text 
                            WS_VISIBLE | WS_CHILD | SS_CENTER,  // styles 
                            // Size and position values are given explicitly, because 
                            // the CW_USEDEFAULT constant gives zero values for buttons. 
                            x,         // starting x position 
                            y,         // starting y position 
                            Width,        // button width 
                            Height,        // button height 
                            hWndParent,       // parent window 
                            (HMENU)Identifier,       // No menu 
                            (HINSTANCE) ::GetWindowLongPtr(hWndParent, GWLP_HINSTANCE), 
                            NULL);      // pointer not needed 
}

//-----------------------------------------------------------------------------
// Name: CreateEdit
// Object: create an edit
// Parameters :
//     in  : HWND hWndParent : parent window handle
//           int x : x position relative to parent 
//           int y : y position relative to parent
//           int Width : edit width
//           int Height : edit height
//           int Identifier : edit ID
//           LPCTSTR EditText : edit text
//     out : 
//     return : static HWND 
//-----------------------------------------------------------------------------
HWND CDialogHelper::CreateEdit(HWND hWndParent,
                int x,
                int y,
                int Width,
                int Height,
                int Identifier,
                LPCTSTR EditText
                )
{
    return ::CreateWindowEx( 
                            WS_EX_CLIENTEDGE,
                            _T("EDIT"),   // predefined class 
                            EditText,       // button text 
                            WS_VISIBLE | WS_CHILD |WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL,  // styles 
                            // Size and position values are given explicitly, because 
                            // the CW_USEDEFAULT constant gives zero values for buttons. 
                            x,         // starting x position 
                            y,         // starting y position 
                            Width,        // button width 
                            Height,        // button height 
                            hWndParent,       // parent window 
                            (HMENU)Identifier,       // No menu 
                            (HINSTANCE) ::GetWindowLongPtr(hWndParent, GWLP_HINSTANCE), 
                            NULL);      // pointer not needed 
}

//-----------------------------------------------------------------------------
// Name: CreateEditMultiLines
// Object: create a multi lines edit
// Parameters :
//     in  : HWND hWndParent : parent window handle
//           int x : x position relative to parent 
//           int y : y position relative to parent
//           int Width : edit width
//           int Height : edit height
//           int Identifier : edit ID
//           LPCTSTR EditText : edit text
//     out : 
//     return : static HWND 
//-----------------------------------------------------------------------------
HWND CDialogHelper::CreateEditMultiLines(HWND hWndParent,
                               int x,
                               int y,
                               int Width,
                               int Height,
                               int Identifier,
                               LPCTSTR EditText
                               )
{
    return ::CreateWindowEx( 
                            WS_EX_CLIENTEDGE,
                            _T("EDIT"),   // predefined class 
                            EditText,       // button text 
                            WS_VISIBLE | WS_CHILD |WS_TABSTOP | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_WANTRETURN,  // styles 
                            // Size and position values are given explicitly, because 
                            // the CW_USEDEFAULT constant gives zero values for buttons. 
                            x,         // starting x position 
                            y,         // starting y position 
                            Width,        // button width 
                            Height,        // button height 
                            hWndParent,       // parent window 
                            (HMENU)Identifier,       // No menu 
                            (HINSTANCE) ::GetWindowLongPtr(hWndParent, GWLP_HINSTANCE), 
                            NULL);      // pointer not needed 
}

//-----------------------------------------------------------------------------
// Name: GetInstance
// Object: get instance associated to hwnd
// Parameters :
//     in  : HWND hWnd : window handle
//     out : 
//     return : associated instance 
//-----------------------------------------------------------------------------
HINSTANCE CDialogHelper::GetInstance(HWND hWnd)
{
    return (HINSTANCE)::GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
}

//-----------------------------------------------------------------------------
// Name: IsButtonChecked
// Object: get checked state of a button
// Parameters :
//     in  : HWND hWndDialog : dialog handle
//           int ButtonID : resource id
//     out : 
//     return : TRUE if button is checked
//-----------------------------------------------------------------------------
BOOL CDialogHelper::IsButtonChecked(HWND hWndDialog,int ButtonID)
{
    return (::IsDlgButtonChecked(hWndDialog,ButtonID)!=BST_UNCHECKED);
}

//-----------------------------------------------------------------------------
// Name: IsButtonChecked
// Object: get checked state of a button
// Parameters :
//     in  : HWND hWnd : button handle
//     out : 
//     return : TRUE if button is checked
//-----------------------------------------------------------------------------
BOOL CDialogHelper::IsButtonChecked(HWND hWnd)
{
    return (::SendMessage(hWnd,BM_GETCHECK,0,0)!=BST_UNCHECKED);
}

//-----------------------------------------------------------------------------
// Name: SetButtonCheckState
// Object: set checked state of a button
// Parameters :
//     in  : HWND hWndDialog : dialog handle
//           int ButtonID : resource id
//           BOOL bChecked : TRUE to check button, FALSE to uncheck it
//     out : 
//     return : TRUE if button is checked
//-----------------------------------------------------------------------------
void CDialogHelper::SetButtonCheckState(HWND hWndDialog,int ButtonID,BOOL bChecked)
{
    return CDialogHelper::SetButtonCheckState(::GetDlgItem(hWndDialog,ButtonID),bChecked);
}

//-----------------------------------------------------------------------------
// Name: SetButtonCheckState
// Object: set checked state of a button
// Parameters :
//     in  : HWND hWnd : button handle
//           BOOL bChecked : TRUE to check button, FALSE to uncheck it
//     out : 
//     return : TRUE if button is checked
//-----------------------------------------------------------------------------
void CDialogHelper::SetButtonCheckState(HWND hWnd,BOOL bChecked)
{
    WPARAM CheckState;
    CheckState = bChecked ? BST_CHECKED : BST_UNCHECKED;
    ::SendMessage(hWnd,BM_SETCHECK,CheckState,0);
}