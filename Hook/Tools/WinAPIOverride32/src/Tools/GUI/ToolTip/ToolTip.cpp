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
// Object: class helper for tooltip control
//-----------------------------------------------------------------------------

#include "tooltip.h"

//-----------------------------------------------------------------------------
// Name: CToolTip
// Object: Constructor.
// Parameters :
//     in  : HWND ControlHandle : target control of the tooltip
//           TCHAR* pszText : text shown in tooltip
//           BOOL BalloonTooltip : TRUE to set ballon style
//     out :
//     return : 
//-----------------------------------------------------------------------------
CToolTip::CToolTip(HWND ControlHandle,TCHAR* pszText,BOOL BalloonTooltip)
{
    // struct specifying control classes to register
    INITCOMMONCONTROLSEX iccex; 
    DWORD dwStyle;

    /* INITIALIZE COMMON CONTROLS */
    iccex.dwICC = ICC_WIN95_CLASSES;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitCommonControlsEx(&iccex);

    // set style
    dwStyle=WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP;
    if (BalloonTooltip)
        dwStyle|=TTS_BALLOON;

    /* CREATE A TOOLTIP WINDOW */
    this->hwndToolTip = CreateWindowEx(WS_EX_TOPMOST,
                                    TOOLTIPS_CLASS,
                                    NULL,
                                    dwStyle,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL
                                    );

    if (!this->hwndToolTip)
        return;

    // Do the standard ToolTip coding. 
    TOOLINFO ti={0};

    ti.cbSize = sizeof(ti);
    ti.uFlags = TTF_TRANSPARENT | TTF_CENTERTIP |TTF_SUBCLASS;
    ti.hwnd = ControlHandle;
    ti.uId = 0;
    ti.hinst = NULL;
    ti.lpszText = pszText;

    GetClientRect(ControlHandle, &ti.rect);
    SendMessage(this->hwndToolTip, TTM_ADDTOOL, 0, (LPARAM) &ti );

}

//-----------------------------------------------------------------------------
// Name: ~CToolTip
// Object: Destructor.
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
CToolTip::~CToolTip()
{
    if (this->hwndToolTip)
    {
        ::DestroyWindow(this->hwndToolTip);
        this->hwndToolTip = NULL;
    }
}

//-----------------------------------------------------------------------------
// Name: GetControlHandle
// Object: return handle of Tool tip
// Parameters :
//     in  : 
//     out :
//     return : handle of Tool tip
//-----------------------------------------------------------------------------
HWND CToolTip::GetControlHandle()
{
    return this->hwndToolTip;
}

//-----------------------------------------------------------------------------
// Name: SetTitle
// Object: Set Title and icon for the tooltip
// Parameters :
//     in  : 
//     out :
//     return : result of SendMessage
//-----------------------------------------------------------------------------
LRESULT CToolTip::SetTitle(TCHAR* Title,TOOLTIP_ICON Icon)
{
    return SendMessage((HWND) this->hwndToolTip,(UINT)TTM_SETTITLE, (WPARAM)Icon, (LPARAM)Title); 
}

//-----------------------------------------------------------------------------
// Name: SetTextColor
// Object: Set Text Color for the tooltip
// Parameters :
//     in  : COLORREF Color : new color
//     out :
//     return : result of SendMessage
//-----------------------------------------------------------------------------
LRESULT CToolTip::SetTextColor(COLORREF Color)
{
    return SendMessage(this->hwndToolTip,TTM_SETTIPTEXTCOLOR,(WPARAM)Color,0);
}
//-----------------------------------------------------------------------------
// Name: SetBackGroundColor
// Object: Set SetBackGroundColor for the tooltip
// Parameters :
//     in  : COLORREF Color : new color
//     out :
//     return : result of SendMessage
//-----------------------------------------------------------------------------
LRESULT CToolTip::SetBackGroundColor(COLORREF Color)
{
    return SendMessage(this->hwndToolTip,TTM_SETTIPBKCOLOR,(WPARAM)Color,0);
}