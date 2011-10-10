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
// Object: class helper for Syslink control
//-----------------------------------------------------------------------------

#include "syslink.h"
#pragma message (__FILE__ " Information : The SysLink control is defined in the ComCtl32.dll version 6 and requires a manifest that specifies that version 6 of the dynamic-link library (DLL) should be used if it is available")

//-----------------------------------------------------------------------------
// Name: CSysLink
// Object: Ansi constructor
// Parameters : 
//        in :  HWND hWndParent : handle of parent control
//              int ControlID : control ID for the new SysLink control
//              LPCSTR Text : text of the control something like
//                             "For more information, see the <A ID=\"idLaunchHelp\">HelpCenter</A>,
//                              <A ID=\"idHelpMenu\">Help Menu</A><A HREF=\"http://www.help.com\">
//                              Website</A>"
//              int XPos : x pos of control in the Dialog window
//              int YPos : y pos of control in the Dialog window
//              int Width : width of the control
//              int Height : height of the control
// Return : 
//-----------------------------------------------------------------------------
CSysLink::CSysLink(HWND hWndParent,int ControlID,LPCSTR Text,int XPos,int YPos,int Width,int Height)
{
    LPWSTR wText;
    CAnsiUnicodeConvert::AnsiToUnicode(Text,&wText);
    this->Initialize(hWndParent,ControlID,wText,XPos,YPos,Width,Height);
    free(wText);
}

//-----------------------------------------------------------------------------
// Name: CSysLink
// Object: Unicode constructor
// Parameters : 
//        in :  HWND hWndParent : handle of parent control
//              int ControlID : control ID for the new SysLink control
//              LPCWSTR Text : text of the control something like
//                             L"For more information, see the <A ID=\"idLaunchHelp\">HelpCenter</A>,
//                              <A ID=\"idHelpMenu\">Help Menu</A><A HREF=\"http://www.help.com\">
//                              Website</A>"
//              int XPos : x pos of control in the Dialog window
//              int YPos : y pos of control in the Dialog window
//              int Width : width of the control
//              int Height : height of the control
// Return : 
//-----------------------------------------------------------------------------
CSysLink::CSysLink(HWND hWndParent,int ControlID,LPCWSTR Text,int XPos,int YPos,int Width,int Height)
{
    this->Initialize(hWndParent,ControlID,Text,XPos,YPos,Width,Height);
}

void CSysLink::Initialize(HWND hWndParent,int ControlID,LPCWSTR Text,int XPos,int YPos,int Width,int Height)
{
    // Ensure that the common control DLL is loaded.
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_LINK_CLASS;
    InitCommonControlsEx(&icex);

    // get CreateWindowExW func addr (Syslink work only with CreateWindowExW)
    // so make it compatible with non unicode and unicode version
    this->bLibUser32Loaded=FALSE;
    this->hModuleHandle=GetModuleHandle(_T("user32.dll"));
    if (!this->hModuleHandle)
    {
        this->hModuleHandle=LoadLibrary(_T("user32.dll"));
        this->bLibUser32Loaded=TRUE;
    }
    this->pCreateWindowExW=(ptrCreateWindowExW)GetProcAddress(this->hModuleHandle,"CreateWindowExW");

    // create SysLink
    this->hWindowHandle=this->pCreateWindowExW(
                       WS_EX_TOPMOST,
                       WC_LINK,
                       Text,
                       WM_CREATE|WS_VISIBLE|WS_CHILD,
                       XPos,
                       YPos,
                       Width,
                       Height,
                       hWndParent,
                       (HMENU) ControlID,
                       NULL,
                       NULL
                    );
    // change default white background to a Dialog color background
    SetClassLongPtr(this->hWindowHandle,GCLP_HBRBACKGROUND,(LONG_PTR)GetSysColorBrush(COLOR_3DFACE));

    this->bInitializeDone=TRUE;
}
//-----------------------------------------------------------------------------
// Name: ~CSysLink
// Object: destructor
// Parameters : 
// Return : 
//-----------------------------------------------------------------------------
CSysLink::~CSysLink(void)
{
    if (this->bInitializeDone)
    {
        // if we have loaded user32lib
        if (this->bLibUser32Loaded)
            FreeLibrary(this->hModuleHandle);
        // destroy created window
        DestroyWindow(this->hWindowHandle);
        this->bInitializeDone=FALSE;
    }
}

//-----------------------------------------------------------------------------
// Name: Notify
// Object: should be call on a WM_NOTIFY message
// Parameters : 
//     in : LPARAM lParam : lParam of WndProc associated with WM_NOTIFY message
//     out: BOOL* bClicked : TRUE if a link was clicked
//          LPWSTR IdName : Id name of clicked link (must be WCHAR[48] at least)
// Return : FALSE on error 
//                if message is not for this control
//                if message is not a clicked one
//          TRUE on success
//-----------------------------------------------------------------------------
BOOL CSysLink::Notify(LPARAM lParam,LPWSTR IdName)
{
    if (!this->bInitializeDone)
        return FALSE;

    // check out params
    if (IsBadWritePtr(IdName,sizeof(WCHAR)*48))
        return FALSE;

    // initialize out params
    *IdName=0;

    NMHDR* pNMHdr=(NMHDR*) lParam;

    // if message is not for our control
    if (pNMHdr->hwndFrom!=this->hWindowHandle)
        return FALSE;

    PNMLINK pNMLink = (PNMLINK) lParam;
    switch (pNMHdr->code)
    {
        case NM_RETURN:
	    case NM_CLICK:
        {
             // copy A HREF ID Name
            wcscpy(IdName,pNMLink->item.szID);
            return TRUE;
            break;
        }
    }
    return FALSE;
}
//-----------------------------------------------------------------------------
// Name: OpenLink
// Object: launch link
// Parameters : 
//     in : LPCTSTR URL : url to visit
// Return : FALSE on error, TRUE on success
//-----------------------------------------------------------------------------
BOOL CSysLink::OpenLink(LPCTSTR URL)
{
    if (((int)ShellExecute(NULL,_T("open"),URL,NULL,NULL,SW_SHOWNORMAL))<33)
        return FALSE;
    return TRUE;
}