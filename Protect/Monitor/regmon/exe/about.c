/******************************************************************************
*
* Regmon - Registry Monitor for Windows 95/98/Me/NT/2K/XP/IA64 
*		
* Copyright (c) 1996-2002 Mark Russinovich and Bryce Cogswell
* See readme.txt for terms and conditions.
*
* Displays Registry activity in real-time.
*
******************************************************************************/
#include <windows.h>   
#include <windowsx.h>
#include <tchar.h>
#include <commctrl.h>  
#include <stdio.h>
#include <string.h>
#include <winioctl.h>
#include "resource.h"
#include "ioctlcmd.h"
#include "regmon.h"


/****************************************************************************
*
*	FUNCTION:	AboutDlgProc
*
*	PURPOSE:	Processes messages for "About" dialog box
*
****************************************************************************/
BOOL CALLBACK AboutDlgProc( HWND hDlg, UINT message, UINT wParam, LONG lParam ) 
{
	RECT	parentRc, childRc;
	static HWND		hLink;
	static BOOL		underline_link;
	static HFONT	hFontNormal = NULL;
	static HFONT	hFontUnderline = NULL;
	static HCURSOR	hHandCursor = NULL;
	static HCURSOR	hRegularCursor;
	LOGFONT			logfont;

	switch ( message )  {
	case WM_INITDIALOG:
		GetWindowRect( GetParent(hDlg), &parentRc );
		GetWindowRect( hDlg, &childRc );
		parentRc.left += 70;
		parentRc.top  += 60;
		MoveWindow( hDlg, parentRc.left, parentRc.top, childRc.right - childRc.left, childRc.bottom - childRc.top, TRUE );

		underline_link = TRUE;
		hLink = GetDlgItem( hDlg, IDC_LINK );

		// get link fonts
		hFontNormal = GetStockObject(DEFAULT_GUI_FONT);
		GetObject( hFontNormal, sizeof logfont, &logfont); 
		logfont.lfUnderline = TRUE;
		hFontUnderline = CreateFontIndirect( &logfont );

		// get hand
		hHandCursor = LoadCursor( hInst, TEXT("HAND") );
		hRegularCursor = LoadCursor( NULL, IDC_ARROW );
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if ( (HWND)lParam == hLink )  {
			HDC	hdc = (HDC)wParam;
			SetBkMode( hdc, TRANSPARENT );
			if ( GetSysColorBrush(26/*COLOR_HOTLIGHT*/) )
				SetTextColor( hdc, GetSysColor(26/*COLOR_HOTLIGHT*/) );
			else
				SetTextColor( hdc, RGB(0,0,255) );
			SelectObject( hdc, underline_link ? hFontUnderline : hFontNormal );
			return (LONG)GetSysColorBrush( COLOR_BTNFACE );
		}
		break;

	case WM_MOUSEMOVE: {
		POINT	pt = { LOWORD(lParam), HIWORD(lParam) };
		HWND	hChild = ChildWindowFromPoint( hDlg, pt );
		if ( underline_link == (hChild == hLink) )  {
			underline_link = !underline_link;
			InvalidateRect( hLink, NULL, FALSE );
		}
		if ( underline_link )
			SetCursor( hRegularCursor );
		else
			SetCursor( hHandCursor );
		break;
	}

	case WM_LBUTTONDOWN: {
		POINT		pt = { LOWORD(lParam), HIWORD(lParam) };
		HWND		hChild = ChildWindowFromPoint( hDlg, pt );
		if ( hChild == hLink )  {
			ShellExecute( hDlg, TEXT("open"), TEXT("http://www.sysinternals.com"), NULL, NULL, SW_SHOWNORMAL );
		} 
		break;
	}

	case WM_COMMAND:
		switch ( wParam ) {
		case IDOK:
		case IDCANCEL:
			EndDialog( hDlg, 0 );
			return TRUE;
		}
		break; 

	case WM_CLOSE:
		EndDialog( hDlg, 0 );
		return TRUE;

	default:
		break;
	}
    return FALSE;
}

