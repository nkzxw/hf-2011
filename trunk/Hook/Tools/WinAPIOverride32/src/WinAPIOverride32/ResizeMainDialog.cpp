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
// Object: manages the main dialog resize operations
//-----------------------------------------------------------------------------

#include "ResizeMainDialog.h"

extern HWND mhWndDialog;
extern CSplitter* pSplitterLoadMonitoringAndFaking;
extern CSplitter* pSplitterDetails;
extern CSplitter* pSplitterConfig;


//-----------------------------------------------------------------------------
// Name: SplitterLoadMonitoringAndFakingMove
// Object: callback called when the horizontal splitter (for loading and monitoring options)
//         position changes. It can be called when the window is resized
// Parameters :
//     in  : int LeftOrTopSplitterPos : top of the splitter rect in main window coordinates
//           int RightOrBottomSplitterPos : bottom of the splitter rect in main window coordinates
//           PVOID UserParam : not used
//     out :
//     return : 
//-----------------------------------------------------------------------------
void SplitterLoadMonitoringAndFakingMove(int LeftOrTopSplitterPos,int RightOrBottomSplitterPos,PVOID UserParam)
{

    UNREFERENCED_PARAMETER(UserParam);
    RECT Rect;
    RECT PreviousRect;
    HWND hItem;

    RECT RectStaticMonitoring;
    RECT RectStaticBrowseMonitoring;
    RECT RectStaticEditMonitoring;
    RECT RectStaticLoadMonitoring;

    RECT RectStaticFaking;
    RECT RectStaticEditFaking;
    RECT RectStaticLoadFaking;

    hItem=GetDlgItem(mhWndDialog,IDC_STATIC_MODULES_FILTERS);
    GetClientWindowRect(hItem,&PreviousRect);

    GetClientWindowRect(mhWndDialog,&Rect);
    DWORD WindowInternalWidth=Rect.right-Rect.left;
    int yMove;

    // IDC_STATIC_MONITORING
    hItem=GetDlgItem(mhWndDialog,IDC_STATIC_MONITORING);
    GetClientWindowRect(hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,Rect.left,PreviousRect.bottom+SPACE_BETWEEN_CONTROLS,
        (WindowInternalWidth-SPACE_BETWEEN_CONTROLS)/2-Rect.left-SPACE_BETWEEN_CONTROLS,
        LeftOrTopSplitterPos-(PreviousRect.bottom+SPACE_BETWEEN_CONTROLS),
        SWP_NOACTIVATE|SWP_NOZORDER);
    GetClientWindowRect(hItem,&RectStaticMonitoring);
    yMove=RectStaticMonitoring.top-Rect.top;
    CDialogHelper::Redraw(hItem);


    // assume it is visible
    if (LeftOrTopSplitterPos<PreviousRect.bottom+SPACE_BETWEEN_CONTROLS+10)
    {
        // hide static monitoring and faking
        ShowWindow(hItem,FALSE);
        hItem=GetDlgItem(mhWndDialog,IDC_STATIC_FAKING);
        ShowWindow(hItem,FALSE);
    }
    else
    {
        // assume static monitoring and faking are shown
        ShowWindow(hItem,TRUE);
        hItem=GetDlgItem(mhWndDialog,IDC_STATIC_FAKING);
        ShowWindow(hItem,TRUE);
    }


    // IDC_BUTTON_BROWSE_MONITORING_FILE
    hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_BROWSE_MONITORING_FILE);
    GetClientWindowRect(hItem,&Rect);
    // use IDC_STATIC_MONITORING rect
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectStaticMonitoring.right-SPACE_BETWEEN_CONTROLS-(Rect.right-Rect.left),Rect.top+yMove,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);

    GetClientWindowRect(hItem,&RectStaticBrowseMonitoring);
    CDialogHelper::Redraw(hItem);


    // assume it is visible
    if(LeftOrTopSplitterPos<RectStaticBrowseMonitoring.bottom+STATIC_GROUPBOX_BORDER_WIDTH)
    {
        // hide browse monitoring and faking, edit monitoring and faking
        ShowWindow(hItem,FALSE);
        hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_BROWSE_FAKING_FILE);
        ShowWindow(hItem,FALSE);
        hItem=GetDlgItem(mhWndDialog,IDC_EDIT_MONITORING_FILE);
        ShowWindow(hItem,FALSE);
        hItem=GetDlgItem(mhWndDialog,IDC_EDIT_FAKING_FILE);
        ShowWindow(hItem,FALSE);
    }
    else
    {
        // assume browse monitoring and faking, edit monitoring and faking are shown
        ShowWindow(hItem,TRUE);
        hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_BROWSE_FAKING_FILE);
        ShowWindow(hItem,TRUE);
        hItem=GetDlgItem(mhWndDialog,IDC_EDIT_MONITORING_FILE);
        ShowWindow(hItem,TRUE);
        hItem=GetDlgItem(mhWndDialog,IDC_EDIT_FAKING_FILE);
        ShowWindow(hItem,TRUE);
    }

    // IDC_EDIT_MONITORING_FILE
    hItem=GetDlgItem(mhWndDialog,IDC_EDIT_MONITORING_FILE);
    GetClientWindowRect(hItem,&Rect);
    // use IDC_STATIC_MONITORING && IDC_BUTTON_BROWSE_MONITORING_FILE rect
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectStaticMonitoring.left+SPACE_BETWEEN_CONTROLS,Rect.top+yMove,
        RectStaticMonitoring.right-RectStaticMonitoring.left-3*SPACE_BETWEEN_CONTROLS-(RectStaticBrowseMonitoring.right-RectStaticBrowseMonitoring.left),Rect.bottom-Rect.top,
        SWP_NOACTIVATE|SWP_NOZORDER);

    GetClientWindowRect(hItem,&RectStaticEditMonitoring);
    CDialogHelper::Redraw(hItem);

    // IDC_BUTTON_MONITORING_RELOAD
    hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_MONITORING_RELOAD);
    GetClientWindowRect(hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectStaticMonitoring.right-SPACE_BETWEEN_CONTROLS-(Rect.right-Rect.left),
        RectStaticBrowseMonitoring.bottom+SPACE_BETWEEN_CONTROLS,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    GetClientWindowRect(hItem,&PreviousRect);
    CDialogHelper::Redraw(hItem);

    // assume it is visible
    if(LeftOrTopSplitterPos<PreviousRect.bottom+STATIC_GROUPBOX_BORDER_WIDTH)
    {
        // hide load monitoring and faking
        ShowWindow(hItem,FALSE);
        hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_MONITORING_WIZARD);
        ShowWindow(hItem,FALSE);
        hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_FAKING_RELOAD);
        ShowWindow(hItem,FALSE);
    }
    else
    {
        // assume load monitoring and faking are shown
        ShowWindow(hItem,TRUE);
        hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_MONITORING_WIZARD);
        ShowWindow(hItem,TRUE);
        hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_FAKING_RELOAD);
        ShowWindow(hItem,TRUE);
    }

    // IDC_BUTTON_MONITORING_WIZARD
    hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_MONITORING_WIZARD);
    GetClientWindowRect(hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        PreviousRect.left-SPACE_BETWEEN_CONTROLS-(Rect.right-Rect.left),
        PreviousRect.top,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    GetClientWindowRect(hItem,&PreviousRect);
    CDialogHelper::Redraw(hItem);


    // IDC_BUTTON_LOAD_MONITORING_FILE
    hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_LOAD_MONITORING_FILE);
    GetClientWindowRect(hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectStaticMonitoring.right-SPACE_BETWEEN_CONTROLS-(Rect.right-Rect.left),
        PreviousRect.bottom+SPACE_BETWEEN_CONTROLS,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    GetClientWindowRect(hItem,&RectStaticLoadMonitoring);
    CDialogHelper::Redraw(hItem);


    // assume it is visible
    if(LeftOrTopSplitterPos<RectStaticLoadMonitoring.bottom+STATIC_GROUPBOX_BORDER_WIDTH)
    {
        // hide load monitoring and faking
        ShowWindow(hItem,FALSE);
        hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_LOAD_FAKING_FILE);
        ShowWindow(hItem,FALSE);
    }
    else
    {
        // assume load monitoring and faking are shown
        ShowWindow(hItem,TRUE);
        hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_LOAD_FAKING_FILE);
        ShowWindow(hItem,TRUE);
    }


    // IDC_BUTTON_UNLOAD_MONITORING_FILE
    hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_UNLOAD_MONITORING_FILE);
    GetClientWindowRect(hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectStaticLoadMonitoring.left,RectStaticLoadMonitoring.bottom+SPACE_BETWEEN_CONTROLS,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    CDialogHelper::Redraw(hItem);

    // assume it is visible
    if(LeftOrTopSplitterPos<RectStaticLoadMonitoring.bottom+SPACE_BETWEEN_CONTROLS  // y pos
                            +Rect.bottom-Rect.top // height
                            + STATIC_GROUPBOX_BORDER_WIDTH
                            )
    {
        // hide unload monitoring and faking
        ShowWindow(hItem,FALSE);
        hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_UNLOAD_FAKING_FILE);
        ShowWindow(hItem,FALSE);
    }
    else
    {
        // assume browse monitoring and faking, edit monitoring and faking are shown
        ShowWindow(hItem,TRUE);
        hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_UNLOAD_FAKING_FILE);
        ShowWindow(hItem,TRUE);
    }

    // IDC_LIST_MONITORING_FILES
    hItem=GetDlgItem(mhWndDialog,IDC_LIST_MONITORING_FILES);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectStaticMonitoring.left+SPACE_BETWEEN_CONTROLS,
        RectStaticBrowseMonitoring.bottom+SPACE_BETWEEN_CONTROLS,
        RectStaticMonitoring.right-RectStaticMonitoring.left-3*SPACE_BETWEEN_CONTROLS-(RectStaticLoadMonitoring.right-RectStaticLoadMonitoring.left),
        RectStaticMonitoring.bottom-SPACE_BETWEEN_CONTROLS-(RectStaticBrowseMonitoring.bottom+SPACE_BETWEEN_CONTROLS),
        SWP_NOACTIVATE|SWP_NOZORDER);
    CDialogHelper::Redraw(hItem);

    // assume it is visible
    if(LeftOrTopSplitterPos<RectStaticMonitoring.bottom-SPACE_BETWEEN_CONTROLS)
    {
        // hide unload monitoring and faking
        ShowWindow(hItem,FALSE);
        hItem=GetDlgItem(mhWndDialog,IDC_LIST_FAKING_FILES);
        ShowWindow(hItem,FALSE);
    }
    else
    {
        // assume browse monitoring and faking, edit monitoring and faking are shown
        ShowWindow(hItem,TRUE);
        hItem=GetDlgItem(mhWndDialog,IDC_LIST_FAKING_FILES);
        ShowWindow(hItem,TRUE);
    }


    // IDC_STATIC_FAKING
    hItem=GetDlgItem(mhWndDialog,IDC_STATIC_FAKING);
    // use IDC_STATIC_MONITORING rect
    SetWindowPos(hItem,HWND_NOTOPMOST,RectStaticMonitoring.right+SPACE_BETWEEN_CONTROLS,RectStaticMonitoring.top,
        RectStaticMonitoring.right-RectStaticMonitoring.left,RectStaticMonitoring.bottom-RectStaticMonitoring.top,
        SWP_NOACTIVATE|SWP_NOZORDER);
    GetClientWindowRect(hItem,&RectStaticFaking);
    CDialogHelper::Redraw(hItem);

    // IDC_EDIT_FAKING_FILE
    hItem=GetDlgItem(mhWndDialog,IDC_EDIT_FAKING_FILE);
    // use IDC_STATIC_FAKING rect
    SetWindowPos(hItem,HWND_NOTOPMOST,RectStaticFaking.left+SPACE_BETWEEN_CONTROLS,RectStaticEditMonitoring.top,
        RectStaticEditMonitoring.right-RectStaticEditMonitoring.left,RectStaticEditMonitoring.bottom-RectStaticEditMonitoring.top,
        SWP_NOACTIVATE|SWP_NOZORDER);
    GetClientWindowRect(hItem,&RectStaticEditFaking);
    CDialogHelper::Redraw(hItem);

    // IDC_BUTTON_BROWSE_FAKING_FILE
    hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_BROWSE_FAKING_FILE);
    // use IDC_EDIT_FAKING_FILE rect
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectStaticEditFaking.right+SPACE_BETWEEN_CONTROLS,RectStaticEditFaking.top,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    GetClientWindowRect(hItem,&PreviousRect);
    CDialogHelper::Redraw(hItem);

    // IDC_BUTTON_FAKING_RELOAD
    hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_FAKING_RELOAD);
    GetClientWindowRect(hItem,&Rect);
    // use IDC_EDIT_FAKING_FILE rect
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectStaticFaking.right-SPACE_BETWEEN_CONTROLS-(Rect.right-Rect.left),
        PreviousRect.bottom+SPACE_BETWEEN_CONTROLS,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    GetClientWindowRect(hItem,&PreviousRect);
    CDialogHelper::Redraw(hItem);

    // IDC_BUTTON_LOAD_FAKING_FILE
    hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_LOAD_FAKING_FILE);
    GetClientWindowRect(hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectStaticFaking.right-SPACE_BETWEEN_CONTROLS-(Rect.right-Rect.left),
        PreviousRect.bottom+SPACE_BETWEEN_CONTROLS,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    GetClientWindowRect(hItem,&RectStaticLoadFaking);
    CDialogHelper::Redraw(hItem);

    // IDC_BUTTON_UNLOAD_FAKING_FILE
    hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_UNLOAD_FAKING_FILE);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectStaticLoadFaking.left,
        RectStaticLoadFaking.bottom+SPACE_BETWEEN_CONTROLS,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    CDialogHelper::Redraw(hItem);

    // IDC_LIST_FAKING_FILES
    hItem=GetDlgItem(mhWndDialog,IDC_LIST_FAKING_FILES);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RectStaticFaking.left+SPACE_BETWEEN_CONTROLS,
        RectStaticBrowseMonitoring.bottom+SPACE_BETWEEN_CONTROLS,
        RectStaticFaking.right-RectStaticFaking.left-3*SPACE_BETWEEN_CONTROLS-(RectStaticLoadFaking.right-RectStaticLoadFaking.left),
        RectStaticMonitoring.bottom-SPACE_BETWEEN_CONTROLS-(RectStaticBrowseMonitoring.bottom+SPACE_BETWEEN_CONTROLS),
        SWP_NOACTIVATE|SWP_NOZORDER);
    CDialogHelper::Redraw(hItem);

    // put IDC_LISTLOGMONITORING in bottom of last control
    GetClientWindowRect(mhWndDialog,&PreviousRect);

    hItem=GetDlgItem(mhWndDialog,IDC_LISTLOGMONITORING);
    GetClientWindowRect(hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        Rect.left,RightOrBottomSplitterPos,
        Rect.right-Rect.left, // width is changed by pSplitterDetails->Redraw
        PreviousRect.bottom-RightOrBottomSplitterPos-3*SPACE_BETWEEN_CONTROLS,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW);
    // don't redraw yet

    // force redraw of splitter details
    if (pSplitterDetails)
    {
        pSplitterDetails->TopMinFreeSpace=RightOrBottomSplitterPos-PreviousRect.top;
        pSplitterDetails->BottomMinFreeSpace=3*SPACE_BETWEEN_CONTROLS;
        pSplitterDetails->Redraw();
    }


    //////////////////////////////////////////////////
    // check if modules filters parts should be shown
    // or if splitter hide them
    //////////////////////////////////////////////////
    hItem=GetDlgItem(mhWndDialog,IDC_STATIC_MODULES_FILTERS);
    GetClientWindowRect(hItem,&Rect);
    if (LeftOrTopSplitterPos<Rect.bottom)
    {
        // hide
        CDialogHelper::ShowGroup(hItem,FALSE);
        CDialogHelper::RedrawGroup(hItem);
    }
    else
    {
        // show
        CDialogHelper::ShowGroup(hItem,TRUE);
        CDialogHelper::RedrawGroup(hItem);
    }
}

//-----------------------------------------------------------------------------
// Name: SplitterDetailsMove
// Object: callback called when the vertical splitter (for details)
//         position changes. It can be called when the window is resized
// Parameters :
//     in  : int LeftOrTopSplitterPos : left of the splitter rect in main window coordinates
//           int RightOrBottomSplitterPos : right of the splitter rect in main window coordinates
//           PVOID UserParam : not used
//     out :
//     return : 
//-----------------------------------------------------------------------------
void SplitterDetailsMove(int LeftOrTopSplitterPos,int RightOrBottomSplitterPos,PVOID UserParam)
{
    UNREFERENCED_PARAMETER(UserParam);
    RECT Rect;
    RECT PreviousRect;
    RECT RectMonitoringLog;
    HWND hItem=GetDlgItem(mhWndDialog,IDC_LISTLOGMONITORING);
    GetClientWindowRect(hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        0,0,
        LeftOrTopSplitterPos-Rect.left, 
        Rect.bottom-Rect.top,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    CDialogHelper::Redraw(hItem);
    GetClientWindowRect(hItem,&RectMonitoringLog);

    hItem=GetDlgItem(mhWndDialog,IDC_STATIC_MODULES_FILTERS);
    GetClientWindowRect(hItem,&Rect);
    LONG MaxRight=Rect.right;

    // detail type display
    hItem=GetDlgItem(mhWndDialog,IDC_LISTDETAILSTYPESDISPLAY);
    GetClientWindowRect(hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RightOrBottomSplitterPos,RectMonitoringLog.bottom-(Rect.bottom-Rect.top),
        MaxRight-RightOrBottomSplitterPos,
        Rect.bottom-Rect.top,
        SWP_NOACTIVATE|SWP_NOZORDER);
    GetClientWindowRect(hItem,&PreviousRect);
    CDialogHelper::Redraw(hItem);

    // details
    hItem=GetDlgItem(mhWndDialog,IDC_LISTDETAILS);
    GetClientWindowRect(hItem,&Rect);
    SetWindowPos(hItem,HWND_NOTOPMOST,
        RightOrBottomSplitterPos,RectMonitoringLog.top,
        MaxRight-RightOrBottomSplitterPos,
        PreviousRect.top-RectMonitoringLog.top,
        SWP_NOACTIVATE|SWP_NOZORDER);
    CDialogHelper::Redraw(hItem);

}

//-----------------------------------------------------------------------------
// Name: GetClientWindowRect
// Object: translate lpRect from Screen coordinates to Client Coordinates
//          (used for resizing controls)
// Parameters :
//     in  :
//     out :
//     return : 
//-----------------------------------------------------------------------------
void GetClientWindowRect(HWND hItem,LPRECT lpRect)
{
    GetWindowRect(hItem,lpRect);
    POINT p;
    p.x=lpRect->left;
    p.y=lpRect->top;
    ScreenToClient(mhWndDialog,&p);
    lpRect->left=p.x;
    lpRect->top=p.y;
    p.x=lpRect->right;
    p.y=lpRect->bottom;
    ScreenToClient(mhWndDialog,&p);
    lpRect->right=p.x;
    lpRect->bottom=p.y;
}

//-----------------------------------------------------------------------------
// Name: CheckSize
// Object: called on WM_SIZING. Assume main dialog has a min with and hight
// Parameters :
//     in  : 
//     out :
//     In Out : RECT* pWinRect : window rect
//     return : 
//-----------------------------------------------------------------------------
void CheckSize(RECT* pWinRect)
{
    // check min width and min height
    if ((pWinRect->right-pWinRect->left)<MAIN_DIALOG_MIN_WIDTH)
        pWinRect->right=pWinRect->left+MAIN_DIALOG_MIN_WIDTH;
    if ((pWinRect->bottom-pWinRect->top)<MAIN_DIALOG_MIN_HEIGHT)
        pWinRect->bottom=pWinRect->top+MAIN_DIALOG_MIN_HEIGHT;
}


//-----------------------------------------------------------------------------
// Name: Resize
// Object: called on WM_SIZE. Resize all components
// Parameters :
//     in  : BOOL UserInterfaceInStartedMode : TRUE is main dialog is in started mode
//     out :
//     return : 
//-----------------------------------------------------------------------------
void Resize(BOOL UserInterfaceInStartedMode)
{
    HWND hItem;
    RECT Rect;

    RECT PreviousRect;
    RECT PreviousUpperRect;

    RECT RectWindow;

    // resize/change position of controls from up to down

    GetClientWindowRect(mhWndDialog,&RectWindow);

    DWORD WindowInternalWidth=(RectWindow.right-RectWindow.left);

    hItem=GetDlgItem(mhWndDialog,IDC_STATICPIDSELECT);
    GetClientWindowRect(hItem,&Rect);

    if (UserInterfaceInStartedMode)
    {
        PreviousUpperRect.bottom=Rect.top;
    }
    /////////////////////////////////
    // Change members shown in stopped mode
    /////////////////////////////////
    else
    {
        // IDC_STATICPIDSELECT
        hItem=GetDlgItem(mhWndDialog,IDC_STATICPIDSELECT);
        GetClientWindowRect(hItem,&Rect);
        SetWindowPos(hItem,HWND_NOTOPMOST,
            0,0,
            WindowInternalWidth-2*(Rect.left+SPACE_BETWEEN_CONTROLS),
            Rect.bottom-Rect.top,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
        GetClientWindowRect(hItem,&PreviousUpperRect);
        GetClientWindowRect(hItem,&PreviousRect);
        CDialogHelper::Redraw(hItem);

        // IDC_EDIT_CMD_LINE
        hItem=GetDlgItem(mhWndDialog,IDC_EDIT_CMD_LINE);
        GetClientWindowRect(hItem,&Rect);
        SetWindowPos(hItem,HWND_NOTOPMOST,
            PreviousRect.right-SPACE_BETWEEN_CONTROLS-(PreviousRect.right-PreviousRect.left)/4,Rect.top,
            (PreviousRect.right-PreviousRect.left)/4,
            Rect.bottom-Rect.top,
            SWP_NOACTIVATE|SWP_NOZORDER);
        GetClientWindowRect(hItem,&PreviousRect);

        // IDC_STATIC_COMMAND_LINE
        hItem=GetDlgItem(mhWndDialog,IDC_STATIC_COMMAND_LINE);
        GetClientWindowRect(hItem,&Rect);
        SetWindowPos(hItem,HWND_NOTOPMOST,
            PreviousRect.left-SPACE_BETWEEN_CONTROLS-(Rect.right-Rect.left),Rect.top,
            0,
            0,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
        GetClientWindowRect(hItem,&PreviousRect);
        CDialogHelper::Redraw(hItem);

        // IDC_BUTTON_BROWSE_APP_PATH
        hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_BROWSE_APP_PATH);
        GetClientWindowRect(hItem,&Rect);
        SetWindowPos(hItem,HWND_NOTOPMOST,
            PreviousRect.left-SPACE_BETWEEN_CONTROLS-(Rect.right-Rect.left),Rect.top,
            0,
            0,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
        GetClientWindowRect(hItem,&PreviousRect);
        CDialogHelper::Redraw(hItem);

        // IDC_EDIT_APP_PATH
        hItem=GetDlgItem(mhWndDialog,IDC_EDIT_APP_PATH);
        GetClientWindowRect(hItem,&Rect);
        SetWindowPos(hItem,HWND_NOTOPMOST,0,0,
            PreviousRect.left-SPACE_BETWEEN_CONTROLS-Rect.left,Rect.bottom-Rect.top,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
        CDialogHelper::Redraw(hItem);


    }


    ///////////////////////////////
    //  modules filters panel
    ///////////////////////////////

    // IDC_STATIC_MODULES_FILTERS
    hItem=GetDlgItem(mhWndDialog,IDC_STATIC_MODULES_FILTERS);
    GetClientWindowRect(hItem,&Rect);
    // move group
    POINT Point;
    Point.x=Rect.left;
    Point.y=PreviousUpperRect.bottom+SPACE_BETWEEN_CONTROLS;
    CDialogHelper::MoveGroupTo(mhWndDialog,hItem,&Point);

    // resize items that require it, and set position for non standard move
    SetWindowPos(hItem,HWND_NOTOPMOST,
        Rect.left,
        PreviousUpperRect.bottom+SPACE_BETWEEN_CONTROLS,
        WindowInternalWidth-2*(Rect.left+SPACE_BETWEEN_CONTROLS),
        Rect.bottom-Rect.top,
        SWP_NOACTIVATE|SWP_NOZORDER);
    GetClientWindowRect(hItem,&PreviousRect);
    GetClientWindowRect(hItem,&PreviousUpperRect);
    CDialogHelper::Redraw(hItem);

    // IDC_BUTTON_UPDATE_MODULE_FILTERS_LIST
    hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_UPDATE_MODULE_FILTERS_LIST);
    GetClientWindowRect(hItem,&Rect);
    // use IDC_STATIC_MODULES_FILTERS rect
    SetWindowPos(hItem,HWND_NOTOPMOST,
        PreviousRect.right-(Rect.right-Rect.left)-2*SPACE_BETWEEN_CONTROLS,Rect.top,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    GetClientWindowRect(hItem,&PreviousRect);
    CDialogHelper::Redraw(hItem);

    // IDC_BUTTON_EDIT_MODULE_FILTERS_LIST
    hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_EDIT_MODULE_FILTERS_LIST);
    GetClientWindowRect(hItem,&Rect);
    // use IDC_STATIC_MODULES_FILTERS rect
    SetWindowPos(hItem,HWND_NOTOPMOST,
        PreviousRect.left-(Rect.right-Rect.left)-SPACE_BETWEEN_CONTROLS,Rect.top,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    GetClientWindowRect(hItem,&PreviousRect);
    CDialogHelper::Redraw(hItem);


    // IDC_BUTTON_BROWSE_NOT_LOGGED_MODULE_LIST
    hItem=GetDlgItem(mhWndDialog,IDC_BUTTON_BROWSE_NOT_LOGGED_MODULE_LIST);
    GetClientWindowRect(hItem,&Rect);
    // use IDC_STATIC_MODULES_FILTERS rect
    SetWindowPos(hItem,HWND_NOTOPMOST,
        PreviousRect.left-(Rect.right-Rect.left)-SPACE_BETWEEN_CONTROLS,Rect.top,
        0,0,
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    GetClientWindowRect(hItem,&PreviousRect);
    CDialogHelper::Redraw(hItem);

    // IDC_EDIT_FILTER_MODULE_LIST
    hItem=GetDlgItem(mhWndDialog,IDC_EDIT_FILTER_MODULE_LIST);
    GetClientWindowRect(hItem,&Rect);
    // use IDC_STATIC_MODULES_FILTERS rect
    SetWindowPos(hItem,HWND_NOTOPMOST,
        Rect.left,Rect.top,
        PreviousRect.left-SPACE_BETWEEN_CONTROLS-Rect.left,
        Rect.bottom-Rect.top,
        SWP_NOACTIVATE|SWP_NOZORDER);
    CDialogHelper::Redraw(hItem);




    /////////////////////////////////
    // Change members shown in started mode
    /////////////////////////////////
    if (UserInterfaceInStartedMode)
    {
        // if UserInterfaceInStartedMode IDC_LISTLOGMONITORING is moved 
        // by pSplitterLoadMonitoringAndFaking->Redraw();
        if (pSplitterLoadMonitoringAndFaking)
            pSplitterLoadMonitoringAndFaking->Redraw();
    }

    else 
    {
        if (pSplitterConfig)
        {
            GetClientWindowRect(GetDlgItem(mhWndDialog,IDC_STATIC_MODULES_FILTERS),&Rect);
            pSplitterConfig->BottomMinFreeSpace=RectWindow.bottom-Rect.bottom-pSplitterConfig->GetThickness();

            pSplitterConfig->Redraw();
    
            pSplitterConfig->GetRect(&PreviousUpperRect);
        }
        // put IDC_LISTLOGMONITORING in bottom of last control
        
        hItem=GetDlgItem(mhWndDialog,IDC_LISTLOGMONITORING);
        GetClientWindowRect(hItem,&Rect);
        SetWindowPos(hItem,HWND_NOTOPMOST,Rect.left,PreviousUpperRect.bottom+SPACE_BETWEEN_CONTROLS,
            Rect.right-Rect.left,// let width be set by pSplitterDetails->Redraw
            RectWindow.bottom-PreviousUpperRect.bottom-4*SPACE_BETWEEN_CONTROLS,
            SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW);
        // dont redraw yet
        if (pSplitterDetails)
        {
            hItem=GetDlgItem(mhWndDialog,IDC_LISTLOGMONITORING);
            GetClientWindowRect(hItem,&Rect);
            pSplitterDetails->TopMinFreeSpace=Rect.top-RectWindow.top;
            pSplitterDetails->BottomMinFreeSpace=RectWindow.bottom-Rect.bottom;
            pSplitterDetails->Redraw();
        }
    }
    CDialogHelper::Redraw(mhWndDialog);
}