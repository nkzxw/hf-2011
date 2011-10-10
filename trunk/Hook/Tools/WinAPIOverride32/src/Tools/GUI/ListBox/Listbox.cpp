/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Based from from T-Matsuo code

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
// Object: class helper for listbox control
//-----------------------------------------------------------------------------

#include "listbox.h"

//-----------------------------------------------------------------------------
// Name: UpdateListBoxHScroll
// Object: Check all listbox item size and adjust the horizontal scroll
//         can be use after item deletion or addition
//         THE LISTBOX MUST HAVE BE DEFINED WITH HORIZONTAL SCROLL
// Parameters :
//     in  : HWND hWndListBox : list box handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListbox::UpdateListBoxHScroll(HWND hWndListBox)
{
    LONG lHScrollWidth;
    SIZE Size;
    LONG lMaxRequieredSize;
    LONG lItemNumber;
    HDC hDc;
    int iSave;
    HFONT hFont;
    TCHAR* psz;
    HGDIOBJ oldObj=NULL;

    // get current Scroll width
    lHScrollWidth=(LONG)SendMessage(hWndListBox,LB_GETHORIZONTALEXTENT,0,0);

    // get DC associated with ListBox
    hDc=GetDC(hWndListBox);
    iSave=SaveDC(hDc);
    // get font
    hFont=(HFONT)SendMessage(hWndListBox,WM_GETFONT,0,0);
    if(hFont != NULL)
    {
        oldObj=SelectObject(hDc,hFont);
    }

    // get number of item in the listbox
    lItemNumber=(LONG)SendMessage(hWndListBox,LB_GETCOUNT,0,0);

    // find biggest required size
    LRESULT MaxItemSize = 1024;
    LRESULT CurrentItemSize;
    psz = new TCHAR[MaxItemSize];
    lMaxRequieredSize=0;
    for(int cnt=0;cnt<lItemNumber;cnt++)
    {
        CurrentItemSize = SendMessage(hWndListBox,LB_GETTEXTLEN,cnt,0);
        if (CurrentItemSize<=0)
            continue;
        if (CurrentItemSize>MaxItemSize)
        {
            MaxItemSize = CurrentItemSize*2;
            delete[] psz;
            psz = new TCHAR[MaxItemSize];
        }
        
        // get text of item cnt
        if (SendMessage(hWndListBox,LB_GETTEXT,cnt,(LPARAM)psz)<=0)
            continue;
         
        // get string size for current control/font
        GetTextExtentPoint32(hDc,psz,(int)_tcslen(psz),&Size);
        if (Size.cx>lMaxRequieredSize)
            lMaxRequieredSize=Size.cx;
    }
    delete[] psz;


    // add size of vertical Scroll Bar
    lMaxRequieredSize+=GetSystemMetrics(SM_CXVSCROLL);
    // if required size is greater than the existing one
    if (lMaxRequieredSize<lHScrollWidth)
        // update size
        SendMessage(hWndListBox,LB_SETHORIZONTALEXTENT,lMaxRequieredSize,0);

    // restore and release DC
    if (oldObj)
        SelectObject(hDc,oldObj);
    RestoreDC(hDc,iSave);
    ReleaseDC(hWndListBox,hDc);
}

//-----------------------------------------------------------------------------
// Name: AddStringWithHScrollUpdate
// Object: Add string to list box, and increase the horizontal scroll if the 
//         new added string require it
//         THE LISTBOX MUST HAVE BE DEFINED WITH HORIZONTAL SCROLL
// Parameters :
//     in  : HWND hWndListBox : list box handle
//           TCHAR* psz : new string to add
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListbox::AddStringWithHScrollUpdate(HWND hWndListBox,TCHAR* psz)
{
    LONG lHScrollWidth;
    SIZE Size;
    HDC hDc;
    int iSave;
    HFONT hFont;
    HGDIOBJ oldObj=NULL;

    // add string
    SendMessage(hWndListBox,LB_ADDSTRING,0,(LPARAM)psz);

    // get current Scroll width
    lHScrollWidth=(LONG)SendMessage(hWndListBox,LB_GETHORIZONTALEXTENT,0,0);

    // get DC associated with ListBox
    hDc=GetDC(hWndListBox);
    iSave=SaveDC(hDc);
    // get font
    hFont=(HFONT)SendMessage(hWndListBox,WM_GETFONT,0,0);
    if(hFont != NULL)
    {
        oldObj=SelectObject(hDc,hFont);
    }
    // get string size for current control/font
    GetTextExtentPoint32(hDc,psz,(int)_tcslen(psz),&Size);

    // add size of vertical Scroll Bar
    Size.cx+=GetSystemMetrics(SM_CXVSCROLL);
    // if required size is greater than the existing one
    if (Size.cx>lHScrollWidth)
        // update size
        SendMessage(hWndListBox,LB_SETHORIZONTALEXTENT,Size.cx,0);
    // restore and release DC
    if (oldObj)
        SelectObject(hDc,oldObj);
    RestoreDC(hDc,iSave);
    ReleaseDC(hWndListBox,hDc);
}