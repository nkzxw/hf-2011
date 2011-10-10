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
// Object: manages the about dialog
//-----------------------------------------------------------------------------

#include "about.h"

HWND hWndAbout;
HFONT hFontAboutTitle;
HFONT hFontAboutLink;
CSysLink* pSysLinkAbout=NULL;

//-----------------------------------------------------------------------------
// Name: ShowAboutDialog
// Object: display the about dialog if not already displayed
// Parameters :
//     in  : HINSTANCE hInstance : instance containing dialog resource
//           HWND hWndDialog : parent window dialog handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void ShowAboutDialog(HINSTANCE hInstance,HWND hWndDialog)
{
    if (!hWndAbout)
    {
        TCHAR NameAndVersion[MAX_PATH];
        TCHAR AppFileName[MAX_PATH];
        TCHAR PrettyVersion[MAX_PATH];

        CStdFileOperations::GetAppName(AppFileName,MAX_PATH);
        CVersion VersionInfo;
        if (VersionInfo.Read(AppFileName))
        {
            CVersion::GetPrettyVersion(VersionInfo.ProductVersion,3,PrettyVersion,MAX_PATH);
            _sntprintf(NameAndVersion,MAX_PATH,_T("%s v%s"),
                VersionInfo.ProductName,
                PrettyVersion
                );
        }
        else
            *NameAndVersion=0;

        DialogBoxParam(hInstance, (LPCTSTR)IDD_DIALOGABOUT, hWndDialog, (DLGPROC)AboutWndProc,(LPARAM)NameAndVersion);
        hWndAbout=NULL;
    }
}

//-----------------------------------------------------------------------------
// Name: AboutWndProc
// Object: about dialog window proc
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
LRESULT CALLBACK AboutWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        hWndAbout=hWnd;
        // load dlg icons
        CDialogHelper::SetIcon(hWndAbout,IDI_ICON_ABOUT);

        // set Font for Title
        hFontAboutTitle=CreateFont(ABOUT_TITLE_FONT_HEIGHT,0,0,0,FW_BOLD,0,0,0,
            ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,
            _T("MS Shell Dlg"));
        SendMessage(GetDlgItem(hWnd,IDC_STATIC_ABOUT_TITLE),WM_SETFONT,(WPARAM)hFontAboutTitle,0);

        // set title (name and version)
        SetDlgItemText(hWndAbout,IDC_STATIC_ABOUT_TITLE,(TCHAR*)lParam);

        // show syslink
        pSysLinkAbout=new CSysLink(hWnd,ID_ABOUT_SYSLINK,
            ABOUT_SYSLINK_TEXT,
            ABOUT_SYSLINK_XPOS,
            ABOUT_SYSLINK_YPOS,
            ABOUT_SYSLINK_WIDTH,
            ABOUT_SYSLINK_HEIGHT);

        // set Font for syslink
        hFontAboutLink=CreateFont(ABOUT_LINK_FONT_HEIGHT,0,0,0,FW_NORMAL,0,0,0,
            ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,
            _T("MS Shell Dlg"));
        SendMessage(pSysLinkAbout->GetControlHandle(),WM_SETFONT,(WPARAM)hFontAboutLink,0);

        CDialogHelper::SetTransparency(hWnd,ABOUT_OPACITY);
        break;
    case WM_CLOSE:
        AboutDlgExit();
        break;
    case WM_LBUTTONDOWN:
        break;
    case WM_LBUTTONUP:
    case WM_KILLFOCUS:
        break;
    case WM_MOUSEMOVE:
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            AboutDlgExit();
            break;
        }
        break;
    case WM_NOTIFY:
        {
            WCHAR IDName[48];
            if (pSysLinkAbout)
            {
                if (pSysLinkAbout->Notify(lParam,(LPWSTR)&IDName))
                {
                    if (wcscmp(IDName,L"IdHomePage")==0)
                    {
                        pSysLinkAbout->OpenLink(ABOUT_SYSLINK_PROJECT_LINK);
                    }
                    else if (wcscmp(IDName,L"IdUserDoc")==0)
                    {
                        pSysLinkAbout->OpenLink(ABOUT_SYSLINK_PROJECT_USER_DOC_LINK);
                    }
                    else // IdMailTo
                    {
                        pSysLinkAbout->OpenLink(ABOUT_SYSLINK_EMAIL_LINK);
                    }
                }
            }
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: AboutDlgExit
// Object: close the about dialog
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void AboutDlgExit()
{
    if (hWndAbout)
    {
        CDialogHelper::Fading(hWndAbout,ABOUT_OPACITY,0,ABOUT_FADING_STEP);
        DeleteFont(hFontAboutTitle);
        DeleteFont(hFontAboutLink);
        EndDialog(hWndAbout,0);
        hWndAbout=NULL;
    }
    if (pSysLinkAbout)
    {
        delete pSysLinkAbout;
        pSysLinkAbout=NULL;
    }
}