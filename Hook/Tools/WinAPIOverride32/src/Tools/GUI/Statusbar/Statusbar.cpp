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
// Object: class helper for StatusBar control
//-----------------------------------------------------------------------------
#include "Statusbar.h"

//-----------------------------------------------------------------------------
// Name: CStatusbar
// Object: create control
// Parameters :
//     in  : HINSTANCE hInstance : module instance
//           HWND hwndParent : parent window handle
//     out : 
//     return : 
//-----------------------------------------------------------------------------
CStatusbar::CStatusbar(HINSTANCE hInstance,HWND hwndParent)
{
    // Ensure that the common control DLL is loaded.
    InitCommonControls();

    this->hInstance=hInstance;
    this->ClickCallBack=0;
    this->ClickCallBackUserParam=0;
    this->DoubleClickCallBack=0;
    this->DoubleClickCallBackUserParam=0;
    this->RightClickCallBack=0;
    this->RightClickCallBackUserParam=0;
    this->DoubleRightClickCallBack=0;
    this->DoubleRightClickCallBackUserParam=0;

    this->NbParts=0;

    // Create the status bar.
    this->hwndStatus = CreateWindowEx(
                                        0,                       // no extended styles
                                        STATUSCLASSNAME,         // name of status bar class
                                        (LPCTSTR) NULL,          // no text when first created
                                        SBARS_SIZEGRIP |         // includes a sizing grip
                                        SBARS_TOOLTIPS |         // allow tooltips
                                        WS_VISIBLE |             // show window
                                        WS_CHILD,                // creates a child window
                                        0, 0, 0, 0,              // ignores size and position
                                        hwndParent,              // handle to parent window
                                        (HMENU) 0,               // child window identifier
                                        hInstance,               // handle to application instance
                                        NULL);                   // no window creation data
}

CStatusbar::~CStatusbar(void)
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
HWND CStatusbar::GetControlHandle()
{
    return this->hwndStatus;
}
//-----------------------------------------------------------------------------
// Name: OnSize
// Object: must be called on parent WM_SIZE notification
// Parameters :
//     in  : WPARAM wParam, LPARAM lParam : parent WndProc WPARAM and LPARAM
//     out : 
//     return : result of WM_SIZE message sent to control
//-----------------------------------------------------------------------------
BOOL CStatusbar::OnSize(WPARAM wParam, LPARAM lParam)
{
    return (BOOL)SendMessage(this->hwndStatus,WM_SIZE,wParam,lParam);
}
//-----------------------------------------------------------------------------
// Name: OnNotify
// Object: must be called on parent WM_NOTIFY notification
// Parameters :
//     in  : WPARAM wParam, LPARAM lParam : parent WndProc WPARAM and LPARAM
//     out : 
//     return : TRUE if message has been processed
//-----------------------------------------------------------------------------
BOOL CStatusbar::OnNotify(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    NMHDR* pnm = (NMHDR*)lParam;
    if (pnm->hwndFrom!=this->hwndStatus)
        return FALSE;

    switch (pnm->code)
    {
    case NM_CLICK:
        {
            LPNMMOUSE pnmMouse = (LPNMMOUSE) lParam;
            if (!IsBadCodePtr((FARPROC)this->ClickCallBack))
                this->ClickCallBack((int)pnmMouse->dwItemSpec,&pnmMouse->pt,this->ClickCallBackUserParam);
        }
        break;
    case NM_DBLCLK:
        {
            LPNMMOUSE pnmMouse = (LPNMMOUSE) lParam;
            if (!IsBadCodePtr((FARPROC)this->DoubleClickCallBack))
                this->DoubleClickCallBack((int)pnmMouse->dwItemSpec,&pnmMouse->pt,this->DoubleClickCallBackUserParam);
        }
        break;
    case NM_RCLICK:
        {
            LPNMMOUSE pnmMouse = (LPNMMOUSE) lParam;
            if (!IsBadCodePtr((FARPROC)this->RightClickCallBack))
                this->RightClickCallBack((int)pnmMouse->dwItemSpec,&pnmMouse->pt,this->RightClickCallBackUserParam);
        }
        break;
    case NM_RDBLCLK:
        {
            LPNMMOUSE pnmMouse = (LPNMMOUSE) lParam; 
            if (!IsBadCodePtr((FARPROC)this->DoubleRightClickCallBack))
                this->DoubleRightClickCallBack((int)pnmMouse->dwItemSpec,&pnmMouse->pt,this->DoubleRightClickCallBackUserParam);
        }
        break;
    //case SBN_SIMPLEMODECHANGE:
    //    break;
    }

    return TRUE;
}

void CStatusbar::SetClickCallBack(pfMouseClickCallBack CallBack,PVOID UserParam)
{
    this->ClickCallBack=CallBack;
    this->ClickCallBackUserParam=UserParam;
}
void CStatusbar::SetRightClickCallBack(pfMouseClickCallBack CallBack,PVOID UserParam)
{
    this->RightClickCallBack=CallBack;
    this->RightClickCallBackUserParam=UserParam;
}
void CStatusbar::SetDoubleClickCallBack(pfMouseClickCallBack CallBack,PVOID UserParam)
{
    this->DoubleClickCallBack=CallBack;
    this->DoubleClickCallBackUserParam=UserParam;
}
void CStatusbar::SetDoubleRightClickCallBack(pfMouseClickCallBack CallBack,PVOID UserParam)
{
    this->DoubleRightClickCallBack=CallBack;
    this->DoubleRightClickCallBackUserParam=UserParam;
}


int CStatusbar::AddPart()
{
    return this->AddPart(CStatusbar_DEFAULT_SIZE);
}
int CStatusbar::AddPart(TCHAR* PartText)
{
    return this->AddPart(PartText,CStatusbar_DEFAULT_SIZE);
}
int CStatusbar::AddPart(TCHAR* PartText,TCHAR* ToolTip)
{
    return this->AddPart(PartText,ToolTip,CStatusbar_DEFAULT_SIZE);
}
int CStatusbar::AddPart(TCHAR* PartText,HICON hIcon)
{
    return this->AddPart(PartText,hIcon,CStatusbar_DEFAULT_SIZE);
}
int CStatusbar::AddPart(TCHAR* PartText,HINSTANCE hInstance,int IdIcon)
{
    HICON hIcon=(HICON)LoadImage(hInstance, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,0,0,LR_SHARED);
    return this->AddPart(PartText,hIcon);
}
int CStatusbar::AddPart(HICON hIcon,TCHAR* ToolTip)
{
    return this->AddPart(hIcon,ToolTip,CStatusbar_DEFAULT_SIZE);
}
int CStatusbar::AddPart(HINSTANCE hInstance,int IdIcon,TCHAR* ToolTip)
{
    HICON hIcon=(HICON)LoadImage(hInstance, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,0,0,LR_SHARED);
    return this->AddPart(hIcon,ToolTip);
}

int CStatusbar::AddPart(int Size)
{
    this->PartsWidths[this->NbParts]=0;
    for(int Cnt=0;Cnt<this->NbParts;Cnt++)
    {
        this->PartsWidths[this->NbParts]+=this->PartsWidths[Cnt];
    }
    this->PartsWidths[this->NbParts]+=Size;
    this->NbParts++;
    if (!SendMessage(this->hwndStatus, SB_SETPARTS,(WPARAM)this->NbParts,(LPARAM)this->PartsWidths))
        return -1;
    return this->NbParts-1;
}
int CStatusbar::AddPart(TCHAR* PartText,int Size)
{
    int Index;
    Index=this->AddPart(Size);
    if (Index<0)
        return Index;

    this->SetText(this->NbParts-1,PartText);
    return Index;
}
int CStatusbar::AddPart(TCHAR* PartText,TCHAR* ToolTip,int Size)
{
    int Index;
    Index=this->AddPart(Size);
    if (Index<0)
        return Index;
    this->SetToolTip(this->NbParts-1,ToolTip);
    this->SetText(this->NbParts-1,PartText);
    return Index;
}
int CStatusbar::AddPart(TCHAR* PartText,HICON hIcon,int Size)
{
    int Index;
    Index=this->AddPart(Size);
    if (Index<0)
        return Index;
    this->SetIcon(this->NbParts-1,hIcon);
    this->SetText(this->NbParts-1,PartText);
    return Index;
}
int CStatusbar::AddPart(TCHAR* PartText,HINSTANCE hInstance,int IdIcon,int Size)
{
    HICON hIcon=(HICON)LoadImage(hInstance, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,0,0,LR_SHARED);
    return this->AddPart(PartText,hIcon,Size);
}
int CStatusbar::AddPart(HICON hIcon,TCHAR* ToolTip,int Size)
{
    int Index;
    Index=this->AddPart(Size);
    if (Index<0)
        return Index;
    this->SetToolTip(this->NbParts-1,ToolTip);
    this->SetIcon(this->NbParts-1,hIcon);
    return Index;
}

BOOL CStatusbar::SetIcon(int PartIndex,HICON hIcon)
{
    return (BOOL)SendMessage(this->hwndStatus,SB_SETICON,PartIndex,(LPARAM)hIcon);
}
BOOL CStatusbar::SetIcon(int PartIndex,HINSTANCE hInstance,int IdIcon)
{
    HICON hIcon=(HICON)LoadImage(hInstance, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,0,0,LR_SHARED);
    return this->SetIcon(PartIndex,hIcon);
}
BOOL CStatusbar::SetText(int PartIndex,TCHAR* PartText)
{
    return (BOOL)SendMessage(this->hwndStatus,SB_SETTEXT,PartIndex,(LPARAM)PartText);
}

// msdn : shown only if icon or text not fully displayed
BOOL CStatusbar::SetToolTip(int PartIndex,TCHAR* ToolTip)
{
    SendMessage(this->hwndStatus,SB_SETTIPTEXT,PartIndex,(LPARAM)ToolTip);
    // msdn : The SB_SETTIPTEXT return value is not used
    return TRUE;
}
BOOL CStatusbar::SetSize(int PartIndex,int Size)
{
    if (PartIndex>=this->NbParts)
        return FALSE;

    this->PartsWidths[PartIndex]=Size;
    return (BOOL)SendMessage(this->hwndStatus, SB_SETPARTS,(WPARAM)this->NbParts,(LPARAM)this->PartsWidths);
}

