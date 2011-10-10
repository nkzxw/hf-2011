/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

Based on Idea of CCSplitter by R.W.G. Hünen (rhunen@xs4all.nl) and Rob Pitt.

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
// Object: class for a Splitter control
//-----------------------------------------------------------------------------

#include "Splitter.h"


//-----------------------------------------------------------------------------
// Name: CSplitter
// Object: constructor
// Parameters :
//     in  : HINSTANCE hInstance : instance containing resources
//           HWND hParent : parent window dialog handle
//           BOOL bInitiallyVisible : TRUE if item is visible after creation
//           BOOL Vertical : TRUE for vertical splitter, FALSE for Horizontal
//           BOOL TopOrLeft : TRUE if vertical splitter must be on left or horizontal splitter must be on top
//                            FALSE if vertical splitter must be on right or horizontal splitter must be on bottom
//           int DefaultExpandedPercent : first expanded size in percent
//           DWORD ID_IconExpanded : resource ID of expanded icon
//           DWORD ID_IconExpandedHot : resource ID of expanded hot icon
//           DWORD ID_IconExpandedDown : resource ID of expanded icon down
//           DWORD ID_IconCollapsed : resource ID of collapsed icon
//           DWORD ID_IconCollapsedHot : resource ID of collapsed hot icon
//           DWORD ID_IconCollapsedDown : resource ID of collapsed icon down
//           int IconWidth : width of icons
//           int IconHeight : height of icons
//     out :
//     return : 
//-----------------------------------------------------------------------------
CSplitter::CSplitter(HINSTANCE hInstance,HWND hParent,BOOL bInitiallyVisible,
                     BOOL Vertical,BOOL TopOrLeft,BOOL StartCollapsed,int DefaultExpandedPercent,
                     DWORD ID_IconExpanded,DWORD ID_IconExpandedHot,DWORD ID_IconExpandedDown,
                     DWORD ID_IconCollapsed,DWORD ID_IconCollapsedHot,DWORD ID_IconCollapsedDown,
                     int IconWidth,int IconHeight)
{
    this->BackGroundBrush=GetSysColorBrush(COLOR_3DFACE);
    this->SplitterBrush=GetSysColorBrush (COLOR_3DSHADOW);


    this->bLeftButtonDown=FALSE;
    this->bCreated=FALSE;
    this->bCapture=FALSE;
    this->bMouseOverIcon=FALSE;
    this->hWndSplitterIcon=0;
    this->hWndSplitter=0;
    this->hWndSplitterBar=0;
    this->LeftMinFreeSpace=0;
    this->RightMinFreeSpace=0;
    this->TopMinFreeSpace=0;
    this->BottomMinFreeSpace=0;
    this->bCollapsed=StartCollapsed;
    this->AllowSizing=TRUE;

    this->UserParam=NULL;
    this->pMoveCallBack=NULL;
    this->CollapsedStateChangeUserParam=NULL;
    this->pCollapsedStateChangeCallBack=NULL;

    this->hInstance=hInstance;
    this->hWndParent=hParent;
    this->bVerticalSplitter=Vertical;
    this->bTopOrLeft=TopOrLeft;
    this->Percent=(float)DefaultExpandedPercent;
    // check percent value
    if (this->Percent>100)
        this->Percent=100;
    if (this->Percent<0)
        this->Percent=0;

    // load cursors
    this->hcurVert = LoadCursor (NULL, IDC_SIZENS);
    this->hcurHorz = LoadCursor (NULL, IDC_SIZEWE);
    
    // load icons
    this->hIconExpanded=(HICON)LoadImage(hInstance,MAKEINTRESOURCE(ID_IconExpanded),IMAGE_ICON,IconWidth,IconHeight,LR_DEFAULTCOLOR|LR_SHARED);
    this->hIconExpandedHot=(HICON)LoadImage(hInstance,MAKEINTRESOURCE(ID_IconExpandedHot),IMAGE_ICON,IconWidth,IconHeight,LR_DEFAULTCOLOR|LR_SHARED);
    this->hIconExpandedDown=(HICON)LoadImage(hInstance,MAKEINTRESOURCE(ID_IconExpandedDown),IMAGE_ICON,IconWidth,IconHeight,LR_DEFAULTCOLOR|LR_SHARED);
    this->hIconCollapsed=(HICON)LoadImage(hInstance,MAKEINTRESOURCE(ID_IconCollapsed),IMAGE_ICON,IconWidth,IconHeight,LR_DEFAULTCOLOR|LR_SHARED);
    this->hIconCollapsedHot=(HICON)LoadImage(hInstance,MAKEINTRESOURCE(ID_IconCollapsedHot),IMAGE_ICON,IconWidth,IconHeight,LR_DEFAULTCOLOR|LR_SHARED);
    this->hIconCollapsedDown=(HICON)LoadImage(hInstance,MAKEINTRESOURCE(ID_IconCollapsedDown),IMAGE_ICON,IconWidth,IconHeight,LR_DEFAULTCOLOR|LR_SHARED);

    this->IconWidth=IconWidth;
    this->IconHeight=IconHeight;

    // get splitter with
    if(this->bVerticalSplitter)
        this->Width=this->IconWidth;
    else
        this->Width=this->IconHeight;
    
    if (this->Width<CSPLITTER_SPLITTER_SIZE)
        this->Width=CSPLITTER_SPLITTER_SIZE;

    // assume control creation is done in WndProc thread (bug can occur else if creation is done in other threads)
    this->CreateControl();

    if (bInitiallyVisible)
        this->Show(TRUE);
}


CSplitter::~CSplitter(void)
{
    DestroyIcon(this->hIconExpanded);
    DestroyIcon(this->hIconExpandedHot);
    DestroyIcon(this->hIconExpandedDown);
    DestroyIcon(this->hIconCollapsed);
    DestroyIcon(this->hIconCollapsedHot);
    DestroyIcon(this->hIconCollapsedDown);
}

//-----------------------------------------------------------------------------
// Name: CreateControl
// Object: create and show the control
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CSplitter::CreateControl()
{
    RECT IconRect;
    RECT SplitterRect;
    RECT SplitterBarRect;

    if (this->bCreated)
        return TRUE;

    if (!this->bTopOrLeft)
        this->Percent=100-this->Percent;

    if (this->bCollapsed)
    {
        // save percent position to restore it next
        this->LastExpendedPercentPosition=this->Percent;

        // set Percent
        if (this->bTopOrLeft)
            this->Percent=0;
        else
            this->Percent=100;
    }

    // get splitter rect
    this->GetSplitterRect(&SplitterRect);


    // unfortunately, as child control can't have transparent background, 
    // we have to give a background for splitter and icon

    // create a Splitter window containing a bar and an icon
    WNDCLASSEX	wc;
    if (!GetClassInfoEx (hInstance, CSPLITTER_SPLITTER_CLASSEX, &wc))
    {
        ZeroMemory (&wc, sizeof(wc));
        wc.cbSize	     = sizeof (wc);
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hCursor	     = LoadCursor (NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpfnWndProc   = CSplitter::WindowProc;
        wc.lpszClassName = CSPLITTER_SPLITTER_CLASSEX;
        RegisterClassEx (&wc);
    }
    this->hWndSplitter = CreateWindowEx (
        WS_EX_TOPMOST,
	    CSPLITTER_SPLITTER_CLASSEX, // Window class name
	    NULL,                       // Window name
        WS_CHILD|WS_CLIPCHILDREN|WS_GROUP,// Window style
        // Position & size
        SplitterRect.left,SplitterRect.top,
        SplitterRect.right-SplitterRect.left,SplitterRect.bottom-SplitterRect.top,
	    this->hWndParent,		// Parent window handle
        (HMENU)NULL,		// Application defined ID must be unique for all child windows with the same parent window
	    hInstance,			// Application instance
	    this);				// Parameter

    if (!this->hWndSplitter)
        return FALSE;

    // put CSplitter object in window parameter
    SetWindowLongPtr(this->hWndSplitter,GWLP_USERDATA,(LONG_PTR)this);

    // create the bar
    this->GetSplitterBarRect(&SplitterRect,&SplitterBarRect);

    this->hWndSplitterBar=CreateWindow(
        _T("STATIC"),
        NULL,
        WS_CHILD,
        // Position & size
        SplitterBarRect.left,SplitterBarRect.top,
        SplitterBarRect.right-SplitterBarRect.left,SplitterBarRect.bottom-SplitterBarRect.top,
        this->hWndSplitter,
        (HMENU)CSPLITTER_ID_STATIC_BAR,
        hInstance,
        NULL);

    if (!this->hWndSplitterBar)
    {
        this->Hide();
        return FALSE;
    }

    // create the icon
    this->GetIconRect(&SplitterRect,&IconRect);
    this->hWndSplitterIcon=CreateWindowEx(
        WS_EX_TOPMOST,
        _T("STATIC"),
        NULL,
        WS_CHILD
        |SS_CENTERIMAGE|SS_ICON, // don't use SS_NOTIFY as it hides WM_MOUSE_MOVE
        IconRect.left,IconRect.top,IconRect.right-IconRect.left,IconRect.bottom-IconRect.top,
        this->hWndSplitter,
        (HMENU)CSPLITTER_ID_STATIC_ICON,
        hInstance,
        NULL); 


    if (!this->hWndSplitterIcon)
    {
        this->Hide();
        return FALSE;
    }

    // set icon
    this->SetIconImg(IconType_NORMAL);

    // control is created
    this->bCreated=TRUE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: WindowProc
// Object: control window proc
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK CSplitter::WindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    CSplitter* pSplitter;
    switch(uMsg)
    {
	case WM_MOUSELEAVE:
        pSplitter=(CSplitter*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (pSplitter)
        {
            pSplitter->OnMouseLeave(wParam,lParam);
        }
		break;

	case WM_LBUTTONDOWN:
        pSplitter=(CSplitter*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (pSplitter)
            pSplitter->OnLButtonDown(wParam,lParam);
		break;

    case WM_LBUTTONUP:
    case WM_KILLFOCUS:
        pSplitter=(CSplitter*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (pSplitter)
            pSplitter->OnLButtonUp(wParam,lParam);
		break;

	case WM_MOUSEMOVE:
        pSplitter=(CSplitter*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (pSplitter)
            pSplitter->OnMouseMove(wParam,lParam);
		break;

   case WM_CTLCOLORSTATIC:
       pSplitter=(CSplitter*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
       if (!pSplitter)
           break;
        // SetTextColor((HDC)wParam,STATIC_TEXT_COLOR);
        // SetBkColor((HDC)wParam,STATIC_BACKGROUND_TEXT_COLOR);
       if ((HANDLE)(lParam)==pSplitter->hWndSplitterBar)
           return (LRESULT)pSplitter->SplitterBrush;
       else if ((HANDLE)(lParam)==pSplitter->hWndSplitterIcon)
           return (LRESULT)pSplitter->BackGroundBrush;
       break;
    case WM_ERASEBKGND: 
       pSplitter=(CSplitter*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
       if (!pSplitter)
           break;

        {
            HDC hdc = (HDC) wParam; 
            RECT rect;
            // Get window coordinates, and normalize.
            GetWindowRect(hwnd, &rect);
            rect.right = rect.right - rect.left;  // Get width.
            rect.bottom = rect.bottom - rect.top; // Get height.
            rect.left = rect.top = 0;
            FillRect(hdc, &rect, pSplitter->BackGroundBrush);
        }
        return TRUE;
    }

    return DefWindowProc (hwnd, uMsg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Name: OnMouseMove
// Object: called when mouse moves over the control (WM_MOUSEMOVE)
// Parameters :
//     in  : WM_MOUSEMOVE params
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    if (!this->bCapture)
    {
        // is mouse over icon
        if (this->IsMouseOverIcon())
        {
            // if button left is down
            if (wParam==MK_LBUTTON)
            {
                // if button left was not down previously
                if (!this->bLeftButtonDown)
                {
                    this->SetIconImg(IconType_DOWN);
                    this->bLeftButtonDown=TRUE;
                }
            }

            // if mouse was already on icon
            if (this->bMouseOverIcon)
                // do nothing
                return;

            // mouse enter the icon

            // change bMouseOverIcon flag state
            this->bMouseOverIcon=TRUE;

            if (!this->bLeftButtonDown)
                // change Icon
                this->SetIconImg(IconType_HOT);

            // we have to watch hWndSplitter Mouse leave event
            TRACKMOUSEEVENT TrackEvent={0};
            TrackEvent.cbSize=sizeof(TRACKMOUSEEVENT);
            TrackEvent.dwFlags=TME_LEAVE;
            TrackEvent.hwndTrack=this->hWndSplitter;
            TrackMouseEvent(&TrackEvent);

        }
        else // mouse is not over icon
        {
            // if mouse was over icon
            if (this->bMouseOverIcon)
            {
                // we have to change icon
                this->SetIconImg(IconType_NORMAL);
                // update bMouseOverIcon flag state
                this->bMouseOverIcon=FALSE;
                this->bLeftButtonDown=FALSE;
            }
        }

        // if mouse is over icon we don't have to check if it's on splitter
        if (this->bMouseOverIcon)
            // so go out
            return;

        // don't allow sizing if not allowed
        if (!this->AllowSizing)
            return;

        // don't allow resize if collapsed
        if (this->bCollapsed)
            return;

        // is mouse over splitter bar
        if (this->IsMouseOverSplitter())
        {
            if (this->bVerticalSplitter)
                SetCursor(this->hcurHorz);
            else
                SetCursor(this->hcurVert);
        }
        
        return;
    }

    // we are in capture mode
    RECT SplitterRect;
    POINT ptMouse;
    GetCursorPos(&ptMouse);

    if (this->bVerticalSplitter)
    {
        this->UpdatePercent(ptMouse.x);
        this->GetSplitterRect(&SplitterRect);
    }
    else
    {
        this->UpdatePercent(ptMouse.y);
        this->GetSplitterRect(&SplitterRect);
    }

    this->Draw(&SplitterRect);

}

//-----------------------------------------------------------------------------
// Name: OnLButtonDown
// Object: called on WM_LBUTTONDOWN
// Parameters :
//     in  : WM_LBUTTONDOWN params
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    // is mouse over icon
    if (this->bMouseOverIcon)
    {
        this->SetIconImg(IconType_DOWN);
        this->bLeftButtonDown=TRUE;
        return;
    }

    // don't allow sizing if not allowed
    if (!this->AllowSizing)
        return;

    // don't allow resize if collapsed
    if (this->bCollapsed)
        return;
    // is mouse over splitter
    if (this->IsMouseOverSplitter())
    {
        if (this->bVerticalSplitter)
            SetCursor(this->hcurHorz);
        else
            SetCursor(this->hcurVert);
        SetCapture( this->hWndSplitter );
        this->bCapture=TRUE;
    }
}

//-----------------------------------------------------------------------------
// Name: OnLButtonUp
// Object: called on WM_LBUTTONUP and WM_KILLFOCUS
// Parameters :
//     in  : WM_LBUTTONUP or WM_KILLFOCUS params (not used)
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    if (this->bCapture)
    {
        ReleaseCapture();
        this->bCapture=FALSE;
        // force parent to be repaint
        RedrawWindow( this->hWndParent,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
    }


    // if button was not down
    if (!this->bLeftButtonDown)
        // return
        return;

    if (!this->IsMouseOverIcon())
        return;

    // reset button down state
    this->bLeftButtonDown=FALSE;

    if (this->bCollapsed)
        this->Expand();
    else
        this->Collapse();

    // force parent to be repaint
    RedrawWindow( this->hWndParent,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
}

//-----------------------------------------------------------------------------
// Name: OnMouseLeave
// Object: called on WM_MOUSELEAVE
// Parameters :
//     in  : WM_MOUSELEAVE params (not used)
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    // mouse can't be over icon so update flag and restore normal icon
    this->bMouseOverIcon=FALSE;
    // reset button down state
    this->bLeftButtonDown=FALSE;
    this->SetIconImg(IconType_NORMAL);
}

//-----------------------------------------------------------------------------
// Name: SetMoveCallBack
// Object: set the splitter move callback
// Parameters :
//     in  : tagMoveCallBack MoveCallBack : splitter move callback
//           PVOID UserParam : user parameter
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::SetMoveCallBack(tagMoveCallBack MoveCallBack,PVOID UserParam)
{
    this->pMoveCallBack=MoveCallBack;
    this->UserParam=UserParam;
}

//-----------------------------------------------------------------------------
// Name: SetCollapsedStateChangeCallBack
// Object: set the splitter Collapsed State Change callback
// Parameters :
//     in  : tagCollapsedStateChange CollapsedStateChangeCallBack : Collapsed State Change callback
//           PVOID UserParam : user parameter
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::SetCollapsedStateChangeCallBack(tagCollapsedStateChange CollapsedStateChangeCallBack,PVOID UserParam)
{
    this->pCollapsedStateChangeCallBack=CollapsedStateChangeCallBack;
    this->CollapsedStateChangeUserParam=UserParam;
}

//-----------------------------------------------------------------------------
// Name: IsMouseOverIcon
// Object: check if mouse is over icon
// Parameters :
//     in  :
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CSplitter::IsMouseOverIcon()
{
    POINT pt;
    RECT Rect;
    GetCursorPos(&pt);
    GetWindowRect(this->hWndSplitterIcon,&Rect);

    return PtInRect(&Rect,pt);
}

//-----------------------------------------------------------------------------
// Name: IsMouseOverSplitter
// Object: check if mouse is over splitter
// Parameters :
//     in  :
//     out :
//     return : 
//-----------------------------------------------------------------------------
BOOL CSplitter::IsMouseOverSplitter()
{
    POINT pt;
    RECT Rect;
    GetCursorPos(&pt);
    GetWindowRect(this->hWndSplitterBar,&Rect);

    return PtInRect(&Rect,pt);
}

//-----------------------------------------------------------------------------
// Name: SetIconImg
// Object: set icon image depending on Type
// Parameters :
//     in  : IconType Type : IconType_NORMAL, IconType_HOT or IconType_DOWN
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::SetIconImg(IconType Type)
{
    switch (Type)
    {
    case IconType_NORMAL:
        if (this->bCollapsed)
            SendMessage(this->hWndSplitterIcon,STM_SETICON, (WPARAM)this->hIconCollapsed,0);
        else
            SendMessage(this->hWndSplitterIcon,STM_SETICON, (WPARAM)this->hIconExpanded,0);
        break;

    case IconType_HOT:
        if (this->bCollapsed)
            SendMessage(this->hWndSplitterIcon,STM_SETICON, (WPARAM)this->hIconCollapsedHot,0);
        else
            SendMessage(this->hWndSplitterIcon,STM_SETICON, (WPARAM)this->hIconExpandedHot,0);
        break;

    case IconType_DOWN:
        if (this->bCollapsed)
            SendMessage(this->hWndSplitterIcon,STM_SETICON, (WPARAM)this->hIconCollapsedDown,0);
        else
            SendMessage(this->hWndSplitterIcon,STM_SETICON, (WPARAM)this->hIconExpandedDown,0);
        break;
    }
}

//-----------------------------------------------------------------------------
// Name: GetIconRect
// Object: get icon rect from icon in this->hWndSplitter coordinates
// Parameters :
//     in  : RECT* pSplitterRect : splitter rect
//     out : RECT* pRect : icon rect
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::GetIconRect(RECT* pSplitterRect,RECT* pRect)
{
    if (this->bVerticalSplitter)
    {
        pRect->left=0;
        pRect->top=(pSplitterRect->bottom-pSplitterRect->top-this->IconHeight)/2;// parent height/2 - icon height/2
    }
    else
    {
        pRect->left=(pSplitterRect->right-pSplitterRect->left-this->IconWidth)/2;// parent width/2 - icon width/2
        pRect->top=0;
    }
    pRect->right=pRect->left+this->IconWidth;
    pRect->bottom=pRect->top+this->IconHeight;
}


//-----------------------------------------------------------------------------
// Name: GetSplitterBarRect
// Object: get splitter bar rect from bar in this->hWndSplitter coordinates
// Parameters :
//     in  : RECT* pSplitterRect : splitter rect
//     out : RECT* pRect : splitter bar rect
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::GetSplitterBarRect(RECT* pSplitterRect,RECT* pRect)
{
    if (this->bVerticalSplitter)
    {
        pRect->left=(this->Width-CSPLITTER_SPLITTER_SIZE)/2;
        pRect->top=0;
        pRect->right=pRect->left+CSPLITTER_SPLITTER_SIZE;
        pRect->bottom=pRect->top+pSplitterRect->bottom-pSplitterRect->top;
    }
    else
    {
        pRect->left=0;
        pRect->top=(this->Width-CSPLITTER_SPLITTER_SIZE)/2;
        pRect->right=pRect->left+pSplitterRect->right-pSplitterRect->left;
        pRect->bottom=pRect->top+CSPLITTER_SPLITTER_SIZE;
     }
}

//-----------------------------------------------------------------------------
// Name: GetSplitterRect
// Object: get splitter rect splitter in parent window coordinate (client coordinates)
// Parameters :
//     in  : 
//     out : RECT* pOutSplitterRect : splitter rect
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::GetSplitterRect(RECT* pOutSplitterRect)
{
    RECT ParentRect;
    int Position;
    POINT pt;
    GetWindowRect(this->hWndParent,&ParentRect);
    int TopOrLeft;
    int BottomOrRight;
    float fPosition;

    if (this->bVerticalSplitter)
    {
        // get position from percent
        TopOrLeft=ParentRect.left+this->LeftMinFreeSpace;
        BottomOrRight=ParentRect.right-this->RightMinFreeSpace;
        fPosition=((BottomOrRight-TopOrLeft)*this->Percent)/100+TopOrLeft;
        // round fPosition
        Position=(int)fPosition;
        if ((fPosition-Position)*10>5.0)
            Position++;

        // get left
        pOutSplitterRect->left=Position-this->Width/2;
        // if point is out of bounds
        if (pOutSplitterRect->left>BottomOrRight-this->Width)
            pOutSplitterRect->left=BottomOrRight-this->Width;
        if (pOutSplitterRect->left<TopOrLeft)
            pOutSplitterRect->left=TopOrLeft;

        // get right
        pOutSplitterRect->right=pOutSplitterRect->left+this->Width;

        // get top
        pOutSplitterRect->top=ParentRect.top+this->TopMinFreeSpace;
        // get bottom
        pOutSplitterRect->bottom=ParentRect.bottom-this->BottomMinFreeSpace;
        if (pOutSplitterRect->bottom-pOutSplitterRect->top<this->IconHeight)
            pOutSplitterRect->bottom=pOutSplitterRect->top+this->IconHeight;

    }
    else
    {
        // get top
        TopOrLeft=ParentRect.top+this->TopMinFreeSpace;
        BottomOrRight=ParentRect.bottom-this->BottomMinFreeSpace;
        fPosition=((BottomOrRight-TopOrLeft)*this->Percent)/100+TopOrLeft;
        // round fPosition
        Position=(int)fPosition;
        if ((fPosition-Position)*10>5.0)
            Position++;

        pOutSplitterRect->top=Position-this->Width/2;
        // if point is out of bounds
        if (pOutSplitterRect->top>BottomOrRight-this->Width)
            pOutSplitterRect->top=BottomOrRight-this->Width;
        if (pOutSplitterRect->top<TopOrLeft)
            pOutSplitterRect->top=TopOrLeft;

        // get bottom
        pOutSplitterRect->bottom=pOutSplitterRect->top+this->Width;

        // get left
        pOutSplitterRect->left=ParentRect.left+this->LeftMinFreeSpace;

        // get right
        pOutSplitterRect->right=ParentRect.right-this->RightMinFreeSpace;
        if (pOutSplitterRect->right-pOutSplitterRect->left<this->IconWidth)
            pOutSplitterRect->right=pOutSplitterRect->left+this->IconWidth;
    }


    // translate into client coordinates
    pt.x=pOutSplitterRect->left;
    pt.y=pOutSplitterRect->top;
    ScreenToClient(this->hWndParent,&pt);
    pOutSplitterRect->left=pt.x;
    pOutSplitterRect->top=pt.y;

    pt.x=pOutSplitterRect->right;
    pt.y=pOutSplitterRect->bottom;
    ScreenToClient(this->hWndParent,&pt);
    pOutSplitterRect->right=pt.x;
    pOutSplitterRect->bottom=pt.y;
}

//-----------------------------------------------------------------------------
// Name: GetRect
// Object: get splitter rect in parent window coordinate (client coordinates)
// Parameters :
//     in  : 
//     out : RECT* pRect : splitter rect
//     return : TRUE on Success
//-----------------------------------------------------------------------------
BOOL CSplitter::GetRect(RECT* pRect)
{
    if (!GetWindowRect(this->hWndSplitter,pRect))
        return FALSE;
    
    POINT p;
    p.x=pRect->left;
    p.y=pRect->top;
    ScreenToClient(this->hWndParent,&p);
    pRect->left=p.x;
    pRect->top=p.y;
    p.x=pRect->right;
    p.y=pRect->bottom;
    ScreenToClient(this->hWndParent,&p);
    pRect->right=p.x;
    pRect->bottom=p.y;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetThickness
// Object: get splitter Thickness (height for horizontal and width for vertical)
//     return : Thickness of the control
//-----------------------------------------------------------------------------
int CSplitter::GetThickness()
{
    return this->Width;
}

//-----------------------------------------------------------------------------
// Name: UpdatePercent
// Object: update percent from the new position in the parent window
// Parameters :
//     in  : int Position : position in the parent window (client coordinates)
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::UpdatePercent(int Position)
{
    RECT ParentRect;
    GetWindowRect(this->hWndParent,&ParentRect);

    if (this->bVerticalSplitter)
        this->Percent=(float)(((Position-(ParentRect.left+this->LeftMinFreeSpace))*100))
                        /((ParentRect.right-this->RightMinFreeSpace)-(ParentRect.left+this->LeftMinFreeSpace));

    else // horizontal splitter
        this->Percent=(float)(((Position-(ParentRect.top+this->TopMinFreeSpace))*100))
                        /((ParentRect.bottom-this->BottomMinFreeSpace)-(ParentRect.top+this->TopMinFreeSpace));

    // check percent bounds
    if (this->Percent>100)
        this->Percent=100;
    if (this->Percent<0)
        this->Percent=0;
}

//-----------------------------------------------------------------------------
// Name: Draw
// Object: draw splitter control
// Parameters :
//     in  : RECT* pSplitterRect : splitter rect (in client coordinates)
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::Draw(RECT* pSplitterRect)
{
    // if a splitter move callback is defined : call it
    if(!IsBadCodePtr((FARPROC)this->pMoveCallBack))
    {
        if (this->bVerticalSplitter)
            this->pMoveCallBack(pSplitterRect->left,pSplitterRect->right,this->UserParam);
        else
            this->pMoveCallBack(pSplitterRect->top,pSplitterRect->bottom,this->UserParam);
    }

    // move splitter background
    SetWindowPos(this->hWndSplitter,HWND_NOTOPMOST,
            pSplitterRect->left,pSplitterRect->top,pSplitterRect->right-pSplitterRect->left,pSplitterRect->bottom-pSplitterRect->top,
            SWP_NOACTIVATE|SWP_NOZORDER);

    RECT Rect;
    // get splitter bar position
    this->GetSplitterBarRect(pSplitterRect,&Rect);
    // move splitter bar
    SetWindowPos(this->hWndSplitterBar,HWND_NOTOPMOST,
                Rect.left,Rect.top,Rect.right-Rect.left,Rect.bottom-Rect.top,
                SWP_NOACTIVATE|SWP_NOZORDER);

    // get splitter icon position
    this->GetIconRect(pSplitterRect,&Rect);
    // move splitter icon
    SetWindowPos(this->hWndSplitterIcon,HWND_NOTOPMOST,
                Rect.left,Rect.top,Rect.right-Rect.left,Rect.bottom-Rect.top,
                SWP_NOACTIVATE|SWP_NOZORDER);

    // force item to be redrawn
    RedrawWindow( this->hWndSplitter,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
}

//-----------------------------------------------------------------------------
// Name: ReDraw
// Object: redraw splitter control
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::Redraw()
{
    // if control is not created
    if (!this->bCreated)
       return;

    RECT SplitterRect;
    // get splitter rect
    this->GetSplitterRect(&SplitterRect);
    // draw control
    this->Draw(&SplitterRect);
}
//-----------------------------------------------------------------------------
// Name: Collapse
// Object: collapse splitter
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::Collapse()
{
    if (this->bCollapsed)
        return;

    if (!this->bCreated)
        return;

    // set collapsed flag
    this->bCollapsed=TRUE;

    // save percent position to restore it next
    this->LastExpendedPercentPosition=this->Percent;

    // set Percent
    if (this->bTopOrLeft)
        this->Percent=0;
    else
        this->Percent=100;

    // change icon
    this->SetIconImg(IconType_NORMAL);

    this->Redraw();

    // call pCollapsedStateChangeCallBack only after redraw
    if(!IsBadCodePtr((FARPROC)this->pCollapsedStateChangeCallBack))
        this->pCollapsedStateChangeCallBack(TRUE,this->CollapsedStateChangeUserParam);
}
//-----------------------------------------------------------------------------
// Name: Expand
// Object: Expand splitter
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::Expand()
{
    if (!this->bCollapsed)
        return;

    if (!this->bCreated)
        return;

    // set collapsed flag
    this->bCollapsed=FALSE;

    // restore percent position
    this->Percent=this->LastExpendedPercentPosition;

    // change icon
    this->SetIconImg(IconType_NORMAL);

    this->Redraw();

    // call pCollapsedStateChangeCallBack only after redraw
    if(!IsBadCodePtr((FARPROC)this->pCollapsedStateChangeCallBack))
        this->pCollapsedStateChangeCallBack(FALSE,this->CollapsedStateChangeUserParam);
}

//-----------------------------------------------------------------------------
// Name: Show
// Object: show splitter control
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CSplitter::Show()
{
    // if control is not created
    if (!this->bCreated)
        return FALSE;

    this->Show(TRUE);

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: Show
// Object: show or hide splitter control
// Parameters :
//     in  : BOOL bShow : TRUE to show control, FALSE to hide it
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::Show(BOOL bShow)
{
    ShowWindow(this->hWndSplitter,bShow);
    ShowWindow(this->hWndSplitterBar,bShow);
    ShowWindow(this->hWndSplitterIcon,bShow);
}

//-----------------------------------------------------------------------------
// Name: Hide
// Object: Hide splitter control
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CSplitter::Hide()
{
    if (!this->bCreated)
        return TRUE;

    this->Show(FALSE);
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: IsCollapsed
// Object: return collapsed state
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CSplitter::IsCollapsed()
{
    return this->bCollapsed;
}

//-----------------------------------------------------------------------------
// Name: SetBackGroundBrush
// Object: set the background brush of splitter control
//         use them same color as parent window to make control like transparent
// Parameters :
//     in  : HBRUSH Brush : background brush
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::SetBackGroundBrush(HBRUSH Brush)
{
    this->BackGroundBrush=Brush;
}

//-----------------------------------------------------------------------------
// Name: SetSplitterBarBrush
// Object: set the splitter bar brush of the splitter control
// Parameters :
//     in  : HBRUSH Brush : splitter bar brush
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CSplitter::SetSplitterBarBrush(HBRUSH Brush)
{
    this->SplitterBrush=Brush;
}