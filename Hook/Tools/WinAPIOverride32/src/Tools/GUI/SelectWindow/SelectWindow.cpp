/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
based from WinSpy by Robert Kuster

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
// Object: allows to graphically select a window with mouse
//         The GetHandleInfo method will fill public info fields like 
//           ParentWindowHandle, WindowControlID,WindowStyle,WindowRect,WindowProc,WindowProcessID ...
//-----------------------------------------------------------------------------

#include "selectwindow.h"

CSelectWindow::CSelectWindow(void)
{
    this->bSelectOnlyDialog=FALSE;
    this->bOwnerWindowSelectable=FALSE;
    this->bInitializeDone=FALSE;
    this->bMinimizeOwnerWindowWhenSelect=FALSE;
    this->bCapture = FALSE;
    this->hWndOld=NULL;
    
    this->ParentWindowHandle=0;
    _tcscpy(this->ParentWindowText,_T("Not Available"));
    _tcscpy(this->ParentWindowClassName,_T("Not Available"));

    this->WindowHandle=0;
    this->WindowControlID=0;
    this->WindowStyle=0;
    memset(&this->WindowRect,0,sizeof(RECT));
    this->WindowExStyle=0;
    this->WindowProc=0;
    this->WindowHinst=0;
    this->WindowUserData=0;
    this->WindowProcessID=0;
    this->WindowThreadID=0;
    _tcscpy(this->WindowClassName,_T("Not Available"));
    this->WindowDlgProc=0;
    this->WindowDlgMsgResult=0;
    this->WindowDlgUser=0;
}


CSelectWindow::~CSelectWindow(void)
{
    if(this->bInitializeDone)
        this->FreeInitializedData();
}

//-----------------------------------------------------------------------------
// Name: FreeInitializedData
// Object: free object allocated in Initialize func
// Parameters :
// Return : 
//-----------------------------------------------------------------------------
void CSelectWindow::FreeInitializedData()
{
    DeleteObject(this->hBmpCross);
    DeleteObject(this->hBmpBlank);
}

//-----------------------------------------------------------------------------
// Name: Initialize
// Object: Initialize object
// Parameters :
//     in : HINSTANCE hInst : instance of app using SelectWindow object
//          HWND hDlg : Dialog handle of Dialog using SelectWindow object
//          DWORD ID_PICTURE_BOX : ID of picture box use for selecting
//          DWORD ID_BMP_BLANK : ID of Bitmap drawn in picture box during Window Selection
//          DWORD ID_BMP_CROSS : ID of Bitmap drawn in picture box when user id not using Window Selection
//          DWORD ID_CURSOR_TARGET : ID of cursor used during Window Selection
// Return : 
//-----------------------------------------------------------------------------
void CSelectWindow::Initialize(HINSTANCE hInst,HWND hDlg,
        DWORD ID_PICTURE_BOX,
        DWORD ID_BMP_BLANK,DWORD ID_BMP_CROSS,DWORD ID_CURSOR_TARGET)
{
    if(this->bInitializeDone)
        this->FreeInitializedData();

    this->hInst=hInst;
    this->hDlg=hDlg;
    this->hPictureBox=GetDlgItem(this->hDlg, ID_PICTURE_BOX);
    this->hBmpCross  =LoadBitmap(this->hInst, MAKEINTRESOURCE(ID_BMP_CROSS));
    this->hBmpBlank  =LoadBitmap(this->hInst, MAKEINTRESOURCE(ID_BMP_BLANK));
    this->hCurCross  =LoadCursor(this->hInst, MAKEINTRESOURCE(ID_CURSOR_TARGET));
    this->hCurNormal =LoadCursor(NULL, IDC_ARROW);
    this->bInitializeDone=TRUE;
}

//-----------------------------------------------------------------------------
// Name: MouseDown
// Object: should be call on a WM_LBUTTONDOWN message
// Parameters : 
//     in : LPARAM lParam : lParam of WndProc associated with WM_LBUTTONDOWN message
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSelectWindow::MouseDown(LPARAM lParam)
{
    POINT pt;
    pt.x = MAKEPOINTS(lParam).x;
    pt.y = MAKEPOINTS(lParam).y; 
    ClientToScreen (hDlg, &pt);

    return this->MouseDown(pt);
}

//-----------------------------------------------------------------------------
// Name: MouseDown
// Object: should be call on a WM_LBUTTONDOWN message
// Parameters : 
//     in : POINT pt : mouse point in Screen reference
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSelectWindow::MouseDown(POINT pt)
{
    if(!this->bInitializeDone)
        return FALSE;

    RECT rc;
    GetWindowRect(this->hPictureBox, &rc);

    // if mouse inside picturebox
    if( PtInRect(&rc, pt) )
    {
        // change cursor
        SetCursor( this->hCurCross );
        // change application image
        SendMessage (this->hPictureBox,STM_SETIMAGE,IMAGE_BITMAP,(LONG)this->hBmpBlank);

        // set old window handle to null
        this->hWndOld = NULL;

        // minimize window BEFORE calling SetCapture
        if (this->bMinimizeOwnerWindowWhenSelect)
            ShowWindow(this->hDlg,SW_MINIMIZE);

        SetCapture( this->hDlg );
        this->bCapture = TRUE;
        
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: MouseMove
// Object: should be call on a WM_LBUTTONUP or WM_KILLFOCUS message
// Parameters : 
// Return : TRUE if we were choosing a window
//-----------------------------------------------------------------------------
BOOL CSelectWindow::MouseUp()
{
    if(!this->bInitializeDone)
        return FALSE;

    if(!this->bCapture)
        return FALSE;
    
    if(this->hWndOld)
        HighlightWindow(this->hWndOld,FALSE);
    this->hWndOld = NULL;

    SetCursor( this->hCurNormal );
    SendMessage (this->hPictureBox,STM_SETIMAGE,IMAGE_BITMAP,(LONG)this->hBmpCross);
    ReleaseCapture();
    this->bCapture = FALSE;
    if (this->bMinimizeOwnerWindowWhenSelect)
        ShowWindow(this->hDlg,SW_RESTORE);
    
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: MouseMove
// Object: should be call on a WM_MOUSEMOVE message
// Parameters : 
//     in : LPARAM lParam : lParam of WndProc associated with WM_MOUSEMOVE message
//     out : BOOL* pbWindowChanged : TRUE if selected window has changed
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSelectWindow::MouseMove(LPARAM lParam,BOOL* pbWindowChanged)
{
    UNREFERENCED_PARAMETER(lParam);
    POINT pt;
    // allow to retrieve correct cursor position even if parent window is minimized
    if (!GetCursorPos(&pt))
        return FALSE;
    return this->MouseMove(pt,pbWindowChanged);
}
//-----------------------------------------------------------------------------
// Name: MouseMove
// Object: should be call on a WM_MOUSEMOVE message
// Parameters : 
//     in : POINT pt : mouse point in Screen reference
//     out : BOOL* pbWindowChanged : TRUE if selected window has changed
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSelectWindow::MouseMove(POINT pt,BOOL* pbWindowChanged)
{
    HWND hWnd;
    HWND hWndTmp;
    DWORD       PID=0;
    DWORD       TID;

    if (!pbWindowChanged)
        return FALSE;
    *pbWindowChanged=FALSE;

    if (!this->bCapture)
        return TRUE;

    if(!this->bInitializeDone)
        return FALSE;

    // get window handle from point
    if (this->bSelectOnlyDialog)
    {
        hWnd = WindowFromPoint (pt);
        hWndTmp=NULL;
        while(hWnd)
        {
            hWndTmp=hWnd;
            hWnd=GetParent(hWndTmp);
        }
        // get last not null handle
        hWnd=hWndTmp;
    }
    else
        hWnd = this->SmallestWindowFromPoint (pt);

    if (!hWnd)
        return FALSE;

    // get window thread and pid from handle
    TID = GetWindowThreadProcessId (hWnd, &PID);

    // check if we can select our window
    if (!this->bOwnerWindowSelectable)
    {
        // if selected window is our one
        if (GetCurrentProcessId () == PID)
            return TRUE;
    }
    
    // if mouse has not changed of window
    if (this->hWndOld == hWnd)
        return TRUE;    // prevent flickering

    *pbWindowChanged=TRUE;

    // if an old window was highlight remove highlight
    if(this->hWndOld) 
        HighlightWindow(this->hWndOld,FALSE);
    else
    {
        if (this->bMinimizeOwnerWindowWhenSelect)
        {
            // assume the first selected window is refreshed before highlighting it
            // else highlightement will be lost (done before the refresh)
            // and as we currently do a xor on window border regardless to flag passed
            // the window will be highlighted when you leave it... quite surprising for the user
            UpdateWindow(hWnd);
        }
    }

    // highlight new selected window
    HighlightWindow(hWnd,TRUE);
    // update old window handler
    this->hWndOld = hWnd;

    return this->GetHandleInfo(hWnd);
}
//-----------------------------------------------------------------------------
// Name: GetHandleInfo
// Object: fill CSelectWindow object information depending of the handle
// Parameters : 
//     in : HWND hWnd handle of the window
// Return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CSelectWindow::GetHandleInfo(HWND hWnd)
{
    DWORD       PID;
    DWORD       TID;

    // get window thread and pid from handle
    TID = GetWindowThreadProcessId (hWnd, &PID);

    ///////////////////////////
    // parent window
    ///////////////////////////
    this->ParentWindowHandle = GetParent (hWnd);
    if (this->ParentWindowHandle != NULL) 
    {
        // parent text
        GetWindowText (this->ParentWindowHandle, this->ParentWindowText, MAX_PATH);
        // parent class name
        GetClassName (this->ParentWindowHandle, this->ParentWindowClassName, MAX_PATH);
    }
    else
    {
        _tcscpy(this->ParentWindowText,_T("Not Available"));
        _tcscpy(this->ParentWindowClassName,_T("Not Available"));
    }

    ///////////////////////////
    // window under cursor
    ///////////////////////////

    // window handle
    this->WindowHandle=hWnd;

    // window extended style
    this->WindowExStyle=(DWORD)GetWindowLongPtr (this->WindowHandle, GWL_EXSTYLE);

    // window style
    this->WindowStyle=(DWORD)GetWindowLongPtr (this->WindowHandle, GWL_STYLE);

    // address of the window procedure 
    this->WindowProc=(DWORD)GetWindowLongPtr (this->WindowHandle, GWLP_WNDPROC);

    // handle to the application instance
    this->WindowHinst=(DWORD)GetWindowLongPtr (this->WindowHandle, GWLP_HINSTANCE);

    // user data associated with the window
    this->WindowUserData=(DWORD)GetWindowLongPtr (this->WindowHandle, GWLP_USERDATA);

    // control ID
    this->WindowControlID=(DWORD)GetWindowLongPtr(this->WindowHandle, GWLP_ID);

    // RECT
    GetWindowRect(this->WindowHandle, &this->WindowRect);
 
    // thread ID
    this->WindowThreadID=TID;
    // process ID
    this->WindowProcessID=PID;

    // class name
    GetClassName (this->WindowHandle, WindowClassName, MAX_PATH);

    // The following values are also available when the hWnd parameter identifies a dialog box.
    // address of the dialog box procedure
    this->WindowDlgProc=(DWORD)GetWindowLongPtr (this->WindowHandle, DWLP_DLGPROC);
    // value of a message processed in the dialog box procedure
    this->WindowDlgMsgResult=(DWORD)GetWindowLongPtr (this->WindowHandle, DWLP_MSGRESULT);
    // extra information private to the application
    this->WindowDlgUser=(DWORD)GetWindowLongPtr (this->WindowHandle, DWLP_USER);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: HighlightWindow (from MSDN Spy Sample)
// Object: highlight or unhightlight a window
// Parameters : 
//     in : HWND hwnd : target window handle
//          BOOL fDraw : TRUE to draw, FALSE to clear
// Return : TRUE on success
//-----------------------------------------------------------------------------
void CSelectWindow::HighlightWindow( HWND hwnd, BOOL fDraw )
{
    #define DINV                3
    HDC hdc;
    RECT rc;
    BOOL bBorderOn;
    bBorderOn = fDraw;

    if (hwnd == NULL || !IsWindow(hwnd))
        return;

    hdc = GetWindowDC(hwnd);
    GetWindowRect(hwnd, &rc);
    OffsetRect(&rc, -rc.left, -rc.top);

    if (!IsRectEmpty(&rc))
    {
        PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, DINV,  DSTINVERT);
        PatBlt(hdc, rc.left, rc.bottom - DINV, DINV,
            -(rc.bottom - rc.top - 2 * DINV), DSTINVERT);
        PatBlt(hdc, rc.right - DINV, rc.top + DINV, DINV,
            rc.bottom - rc.top - 2 * DINV, DSTINVERT);
        PatBlt(hdc, rc.right, rc.bottom - DINV, -(rc.right - rc.left),
            DINV, DSTINVERT);
    }

    ReleaseDC(hwnd, hdc);
}

//-----------------------------------------------------------------------------
// Name: SmallestWindowFromPoint (from PasswordSpy by Brian Friesen)
// Object: Find the smallest window still containing the point
//          WindowFromPoint returns the first window in the Z-order ->
//          if the control is surrounded by a Group Box or some other control,
//          WindowFromPoint returns the handle to the surrounding control instead
//          to the control.
// Parameters : 
//     in : HWND hwnd : target window handle
//          BOOL fDraw : TRUE to draw, FALSE to clear
// Return : TRUE on success
//-----------------------------------------------------------------------------
HWND CSelectWindow::SmallestWindowFromPoint( const POINT point )
{    
    RECT rect, rcTemp;
    HWND hParent, hWnd, hTemp;

    hWnd = WindowFromPoint( point );
    if( hWnd != NULL )
    {
        GetWindowRect( hWnd, &rect );
        hParent = GetParent( hWnd );

        // Has window a parent?
        if( hParent != NULL )
        {
            // Search down the Z-Order
            hTemp = hWnd;
            do{
                hTemp = GetWindow( hTemp, GW_HWNDNEXT );
                if (!hTemp)
                    break;
                // Search window contains the point, has the same parent, and is visible?
                GetWindowRect( hTemp, &rcTemp );
                if(PtInRect(&rcTemp, point) && (GetParent(hTemp) == hParent) && IsWindowVisible(hTemp))
                {
                    // Is it smaller?
                    if(((rcTemp.right - rcTemp.left) * (rcTemp.bottom - rcTemp.top)) < ((rect.right - rect.left) * (rect.bottom - rect.top)))
                    {
                        // Found new smaller window!
                        hWnd = hTemp;
                        GetWindowRect(hWnd, &rect);
                    }
                }
            }while( hTemp != NULL );
        }
    }

    return hWnd;
}