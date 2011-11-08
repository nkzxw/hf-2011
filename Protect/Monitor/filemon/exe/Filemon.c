/******************************************************************************
*
* FileMon - File System Monitor for Windows NT/9x
*		
* Copyright (c) 1996-2000 Mark Russinovich and Bryce Cogswell
*
* See readme.txt for terms and conditions.
*
* PROGRAM: Filemon.c
*
* PURPOSE: Communicates with the Filemon driver to display 
*		   file system activity.
*
******************************************************************************/
#include <windows.h>    // includes basic windows functionality
#include <windowsx.h>
#include <tchar.h>
#include <commctrl.h>   // includes the common control header
#include <stdio.h>
#include <string.h>
#include <winioctl.h>
#include "resource.h"
#include "ioctlcmd.h"
#include "filemon.h"


HRESULT (CALLBACK *pDllGetVersionProc)( PDLLVERSIONINFO_ pdvi );

// Handle to device driver
static HANDLE		SysHandle = INVALID_HANDLE_VALUE;

// Drive name strings
TCHAR DrvNames[][32] = {
	_T("UNKNOWN"),
	_T("FIXED"),
	_T("REMOTE"),
	_T("RAM"),
	_T("CD"),
	_T("REMOVEABLE"),
};	

// drives that are hooked
DWORD				CurDriveSet;

// The variable that holds the position settings
POSITION_SETTINGS	PositionInfo;

// button definitions

// for installations that support flat style
TBBUTTON tbButtons[] = {
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 0, IDM_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 8, 0, 0, TBSTYLE_BUTTON, 0L, 0},
	{ 2, IDM_CAPTURE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 4, IDM_AUTOSCROLL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 6, IDM_CLEAR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 9, IDM_TIME, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0 },	
	{ 8, 0, 0, TBSTYLE_BUTTON, 0L, 0},
	{ 5, IDM_FILTER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 12, IDM_HISTORY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 8, 0, 0, TBSTYLE_BUTTON, 0L, 0},
	{ 7, IDM_FIND, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 11, IDM_JUMP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 8, 0, 0, TBSTYLE_BUTTON, 0L, 0},
};
#define NUMBUTTONS		15

// for older installations
TBBUTTON tbButtonsOld[] = {
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 0, IDM_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 2, IDM_CAPTURE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 4, IDM_AUTOSCROLL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 6, IDM_CLEAR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},	
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 9, IDM_TIME, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0 },	
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 5, IDM_FILTER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 12, IDM_HISTORY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 7, IDM_FIND, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 11, IDM_JUMP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
};

#define NUMBUTTONSOLD	14


// Buffer into which driver can copy statistics
char				Stats[ LOGBUFSIZE ];
// Current fraction of buffer filled
DWORD				StatsLen;

// Search string
TCHAR				FindString[256];
FINDREPLACE			FindTextInfo;
DWORD				FindFlags = FR_DOWN;
BOOLEAN				PrevMatch;
TCHAR				PrevMatchString[256];				

// Application instance handle
HINSTANCE			hInst;

// Are we running on NT or 9x?
BOOLEAN				IsNT;

// Misc globals
HWND				hWndMain;
HWND				hWndFind = NULL;
UINT				findMessageID;
HWND				hWndList;
WNDPROC 			ListViewWinMain;
HWND				hBalloon = NULL;
BOOLEAN				Capture = TRUE;
BOOLEAN				Autoscroll = TRUE;
BOOLEAN				Deleting = TRUE;
BOOLEAN				OnTop = FALSE;
BOOLEAN				ShowToolbar = TRUE;
BOOLEAN				HookPipes = FALSE;
BOOLEAN				HookSlots = FALSE;

// Highlight colors
DWORD				HighlightFg;
DWORD				HighlightBg;

// listview size limiting
DWORD				MaxLines = 0;
DWORD				LastRow = 0;

// is time absolute or duration?
BOOLEAN				TimeIsDuration;
BOOLEAN				ShowMs = FALSE;

// Filter strings
TCHAR				FilterString[MAXFILTERLEN];
TCHAR				ExcludeString[MAXFILTERLEN];
TCHAR				HighlightString[MAXFILTERLEN];

// Recent filters
char				RecentInFilters[NUMRECENTFILTERS][MAXFILTERLEN];
char				RecentExFilters[NUMRECENTFILTERS][MAXFILTERLEN];
char				RecentHiFilters[NUMRECENTFILTERS][MAXFILTERLEN];

// Filter-related
FILTER				FilterDefinition;

// For info saving
TCHAR				szFileName[MAX_PATH];
BOOLEAN				FileChosen = FALSE;

// font
HFONT				hFont;
LOGFONT				LogFont;

// General buffer for storing temporary strings
static TCHAR		msgbuf[MAX_PATH];

// General cursor manipulation
HCURSOR 			hSaveCursor;
HCURSOR 			hHourGlass;

// performance counter frequency
LARGE_INTEGER		PerfFrequency;


/******************************************************************************
*
*	FUNCTION:	Abort:
*
*	PURPOSE:	Handles emergency exit conditions.
*
*****************************************************************************/
DWORD Abort( HWND hWnd, TCHAR * Msg, DWORD Error )
{
	LPVOID	lpMsgBuf;
	TCHAR	errmsg[MAX_PATH];

	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					NULL, Error, 
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &lpMsgBuf, 0, NULL );
	if( IsNT ) UnloadDeviceDriver( SYS_NAME );
	_stprintf(errmsg, _T("%s: %s"), Msg, lpMsgBuf );
	if( (Error == ERROR_INVALID_HANDLE || Error == ERROR_ACCESS_DENIED ||
		 Error == ERROR_FILE_NOT_FOUND) && IsNT ) 
		_stprintf(errmsg, _T("%s\nMake sure that you are an administrator, and ")
			_T("that Filemon is not already running."), errmsg  );
	MessageBox( hWnd, errmsg, _T("Filemon"), MB_OK|MB_ICONERROR );
	PostQuitMessage( 1 );
	LocalFree( lpMsgBuf );
	return (DWORD) -1;
}


/******************************************************************************
*
*	FUNCTION:	ExplorerJump
*
*	PURPOSE:	Opens Explorer and navigates the desired file/folder
*
*****************************************************************************/
void ExplorerJump( HWND hWnd )
{
	int		currentItem;
	char	path[MAX_PATH], msg[MAX_PATH*2];
	char	*lastslash = NULL;
	char	*ptr;

	// See if we can get a Registry path out of the listview
	// find the item with the focus
	currentItem = ListView_GetNextItem( hWndList, -1, LVNI_SELECTED );

	if( currentItem == -1 ) {

		MessageBox( hWnd, "No item selected.", APPNAME, MB_OK|MB_ICONWARNING );
		return;
	}
	memset( path, 0, MAX_PATH );
	ListView_GetItemText( hWndList, currentItem, 4, path, MAX_PATH );

	// If the file is a handle reference, tell the user we're sorry
	if( path[0] == '0' ) {

		MessageBox( hWnd, "The full name of the selected directory or file is not available.",
			APPNAME, MB_OK|MB_ICONWARNING );
		return;
	}

	// Always explore the parent folder, if there is one
	ptr = path;
	while( *ptr ) {
		if( *ptr == '\\' ) lastslash = ptr;
		ptr++;
	}
	if( lastslash ) *lastslash = 0;

	if( ShellExecute( hWnd, "explore", path, NULL, NULL, SW_SHOWNORMAL ) < (HINSTANCE) 32 ) {

		sprintf( msg, "Explorer could not open %s.", path );
		MessageBox( hWnd, msg, APPNAME, MB_OK|MB_ICONWARNING );
		return;
	}
}


/******************************************************************************
*
*	FUNCTION:	BalloonDialog
*
*	PURPOSE:	Dialog function for home-brewed balloon help.
*
******************************************************************************/
LRESULT APIENTRY BalloonDialog( HWND hDlg, UINT message, UINT wParam, LPARAM lParam )
{
	static ITEM_CLICK	ctx;
	static RECT			rect;
	static HFONT		hfont;
	LPCREATESTRUCT		lpcs;
	HDC					hdc;
	POINTS				pts;
	POINT				pt;
	DWORD				newclicktime;
	static POINT		lastclickpt = {0,0};
	static DWORD		lastclicktime = 0;	

	switch (message) {
		case WM_CREATE:

			lpcs = (void *)lParam;
			ctx = *(PITEM_CLICK) lpcs->lpCreateParams;
			hdc = GetDC( hDlg );

			// is the app the focus?
			if( !GetFocus()) return -1;

			// Compute size of required rectangle
			rect.left	= 0;
			rect.top	= 1;
			rect.right	= lpcs->cx;
			rect.bottom	= lpcs->cy;
			SelectObject( hdc, hFont );
			DrawText( hdc, ctx.itemText, -1, &rect, 
						DT_NOCLIP|DT_LEFT|DT_NOPREFIX|DT_CALCRECT );

			// if the bounding rectangle of the subitem is big enough to display
			// the text then don't pop the balloon
			if( ctx.itemPosition.right > rect.right + 3 ) {

				return -1;
			}

			// Move and resize window
			if( ctx.itemPosition.left - 5 + rect.right + 10 >
				 GetSystemMetrics(SM_CXFULLSCREEN) ) {

				 ctx.itemPosition.left = GetSystemMetrics(SM_CXFULLSCREEN) -
							(rect.right+10);
			}
			MoveWindow( hDlg, 
						ctx.itemPosition.left-1, ctx.itemPosition.top, 
						rect.right + 6, 
						rect.bottom + 1,
						TRUE );

			// Adjust rectangle so text is centered
			rect.left	+= 2;
			rect.right	+= 2;
			rect.top	-= 1; 
			rect.bottom	+= 0;

			// make it so this window doesn't get the focus
			ShowWindow( hDlg, SW_SHOWNOACTIVATE );
			break;

		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:

			pts = MAKEPOINTS( lParam );
			pt.x = (LONG) pts.x;
			pt.y = (LONG) pts.y;
			ClientToScreen( hDlg, &pt );

			// pass this through to the listview
			if( ScreenToClient( hWndList, &pt )) {

				if( message == WM_LBUTTONDOWN ) {

					// see if its a double click
					newclicktime = GetTickCount();
					if( pt.x == lastclickpt.x && pt.y == lastclickpt.y && 
						newclicktime - lastclicktime < 300 ) {

						message = WM_LBUTTONDBLCLK;
					}
					lastclicktime = newclicktime;
					lastclickpt = pt;
				}

				PostMessage( hWndList, message, wParam, (SHORT) pt.y<<16 | (SHORT) pt.x );
			}
			break;

		case WM_PAINT:
			hdc = GetDC( hDlg );

			// Set colors
			SetTextColor( hdc, 0x00000000 );
			SetBkMode( hdc, TRANSPARENT );
			SelectObject( hdc, hFont );
			DrawText( hdc, ctx.itemText, -1, &rect, 
						DT_NOCLIP|DT_LEFT|DT_NOPREFIX|DT_WORDBREAK );
			break;

		case WM_DESTROY:
			hBalloon = NULL;
			break;

		case WM_CLOSE:	
			DestroyWindow( hDlg );
			break;
	}

    return DefWindowProc( hDlg, message, wParam, lParam );
}


/******************************************************************************
*
*	FUNCTION:	CopySelection
*
*	PURPOSE:	Copies the currently selected line in the output to the clip
*				board.
*
*****************************************************************************/
void CopySelection( HWND hWnd )
{
    LPTSTR  lptstrCopy; 
    HGLOBAL hglbCopy; 
	size_t	size = 0, newSize;
	int		currentItem, iColumn;
	TCHAR	curText[MAXITEMLENGTH];
	TCHAR	selectedText[NUMCOLUMNS * MAXITEMLENGTH];	

	// Get the currently selected item and construct
	// the message to go to the clipboard
	currentItem = ListView_GetNextItem( hWndList, -1, LVNI_SELECTED );
	if( currentItem == -1 ) {

		return;
	}
	selectedText[0] = 0;
	for( iColumn = 1; iColumn < NUMCOLUMNS; iColumn++ ) {

		curText[0] = 0;
		ListView_GetItemText( hWndList, currentItem, iColumn, 
					curText, MAXITEMLENGTH );
		strcat( selectedText, curText );
		strcat( selectedText, "\t");
	}
	strcat( selectedText, "\r\n");

	// Empty the clipboard
    if (!OpenClipboard( hWnd )) return; 
    EmptyClipboard(); 

	size = strlen( selectedText )+1;
    hglbCopy = GlobalAlloc( GMEM_DDESHARE|GMEM_MOVEABLE, size ); 
    lptstrCopy = GlobalLock(hglbCopy); 
    strcpy(lptstrCopy, selectedText );
    GlobalUnlock(hglbCopy); 

	while( (currentItem = ListView_GetNextItem( hWndList, currentItem, 
				LVNI_SELECTED )) != -1) {
		selectedText[0] = 0;
		for( iColumn = 1; iColumn < NUMCOLUMNS; iColumn++ ) {

			curText[0] = 0;
			ListView_GetItemText( hWndList, currentItem, iColumn, 
						curText, MAXITEMLENGTH );
			strcat( selectedText, curText );
			strcat( selectedText, "\t");
		}
		strcat( selectedText, "\r\n");

		newSize = size + strlen( selectedText );
		hglbCopy = GlobalReAlloc( hglbCopy, newSize, 0 );
		lptstrCopy = GlobalLock(hglbCopy); 
		strcpy( &lptstrCopy[size-1], selectedText );
		GlobalUnlock(hglbCopy); 

		size = newSize;
	}

	// Place it in the clipboard
	SetClipboardData(CF_TEXT, hglbCopy); 
    CloseClipboard(); 	
}


/******************************************************************************
*
*	FUNCTION:	CCHookProc
*
*	PURPOSE:	We use a hook procedure to force the stupid color
*				selection dialog to do what we want, including preview
*				the highlight text.
*
*****************************************************************************/
UINT CALLBACK CCHookProc( HWND hDlg, 
					  UINT uiMsg, WPARAM wParam,  
					  LPARAM lParam )
{
	static HWND	 sample;
	static DWORD  newFg, newBg;
	static UINT colorOkString, setRgbString;

	switch( uiMsg ) {
	case WM_INITDIALOG:
		sample = GetDlgItem( hDlg, IDC_SAMPLE );
		newFg = HighlightFg;
		newBg = HighlightBg;
		colorOkString = RegisterWindowMessage( COLOROKSTRING );
		setRgbString  = RegisterWindowMessage( SETRGBSTRING ); 
		CheckRadioButton( hDlg, IDC_RADIOFG, IDC_RADIOBG, IDC_RADIOFG );
		SendMessage(hDlg, setRgbString, 0, newFg); 
		SetFocus( GetDlgItem( hDlg, IDC_DONE ));
		break;
	case WM_CTLCOLORSTATIC:
		if( (HWND) lParam == sample ) {
			SetBkColor(GET_WM_CTLCOLOR_HDC(wParam, lParam, msg), 
                    newBg); 
			SetTextColor(GET_WM_CTLCOLOR_HDC(wParam, lParam, msg), 
                    newFg); 
            return (BOOL)GetStockObject(WHITE_BRUSH); 		
		}
		break;
	case WM_COMMAND:
		if( wParam == IDC_DONE ) {
			HighlightFg = newFg;
			HighlightBg = newBg;
			PostMessage( hDlg, WM_COMMAND, IDABORT, 0 );
			return FALSE;
		}
		break;
	default:
		if( uiMsg == colorOkString ) {
			if( !IsDlgButtonChecked( hDlg, IDC_RADIOBG )) {
				newFg = ((LPCHOOSECOLOR) lParam)->rgbResult;
				InvalidateRect( sample, NULL, TRUE );
				SendMessage(hDlg, setRgbString, 0, newBg); 
				return TRUE;
			} else {
				newBg = ((LPCHOOSECOLOR) lParam)->rgbResult;
				InvalidateRect( sample, NULL, TRUE );
				SendMessage(hDlg, setRgbString, 0, newFg); 
				return TRUE;
			}
		}
		break;
	}
	return 0;
}


/******************************************************************************
*
*	FUNCTION:	SelectHighlightColors
*
*	PURPOSE:	Let's the user pick the highlight foreground and background
*				colors.
*
*****************************************************************************/
VOID SelectHighlightColors( HWND hWnd )
{
	DWORD			dwColor;
	DWORD			dwCustClrs [16];
	BOOL			fSetColor = FALSE;
	int				i;
	CHOOSECOLOR		chsclr;

	for (i = 0; i < 15; i++)
		dwCustClrs [i] = RGB (255, 255, 255);
	dwColor = RGB (0, 0, 0);
	chsclr.lStructSize = sizeof (CHOOSECOLOR);
	chsclr.hwndOwner = hWnd;
	chsclr.hInstance = (HANDLE) hInst;
	chsclr.rgbResult = dwColor;
	chsclr.lpCustColors = (LPDWORD)dwCustClrs;
	chsclr.lCustData = 0L;
	chsclr.rgbResult = HighlightFg;
	chsclr.lpTemplateName = "CHOOSECOLORFG";
	chsclr.lpfnHook = (LPCCHOOKPROC)(FARPROC)CCHookProc;
	chsclr.Flags = CC_RGBINIT|CC_PREVENTFULLOPEN|
					CC_ENABLEHOOK|CC_ENABLETEMPLATE;
	ChooseColor (&chsclr);
	// Redraw to apply
	InvalidateRect( hWndList, NULL, TRUE );
} 


/******************************************************************************
*
*	FUNCTION:	FindInListview:
*
*	PURPOSE:	Searches for a string in the listview. Note: its okay if
*				items are being added to the list view or the list view
*				is cleared while this search is in progress - the effect
*				is harmless.
*
*****************************************************************************/
BOOLEAN FindInListview(HWND hWnd, LPFINDREPLACE FindInfo )
{
	int		currentItem, clearItem;
	DWORD	i;
	int		subitem, numItems;
	TCHAR	fieldtext[MAXITEMLENGTH];
	BOOLEAN match = FALSE;
	TCHAR	errmsg[MAX_PATH];
	BOOLEAN	goUp;

	// get the search direction
	goUp = ((FindInfo->Flags & FR_DOWN) == FR_DOWN);

	// initialize stuff
	if( !(numItems = ListView_GetItemCount( hWndList ))) {

		MessageBox( hWnd, TEXT("No items to search"), TEXT(APPNAME), 
			MB_OK|MB_ICONWARNING );
		if( hWndFind ) SetForegroundWindow( hWndFind );
		return FALSE;
	}

	// find the item with the focus
	currentItem = ListView_GetNextItem( hWndList, -1, LVNI_SELECTED );

	// if no current item, start at the top or the bottom
	if( currentItem == -1 ) {
		if( goUp )
			currentItem = 0;
		else {
			if( PrevMatch ) {
				sprintf(errmsg, TEXT("Cannot find string \"%s\""), FindInfo->lpstrFindWhat );
				MessageBox( hWnd, errmsg, TEXT(APPNAME), MB_OK|MB_ICONWARNING );
				if( hWndFind ) SetForegroundWindow( hWndFind );
				else SetFocus( hWndList );
				return FALSE;
			}
			currentItem = numItems;
		}
	}

	// if we're continuing a search, start with the next item
	if( PrevMatch && !strcmp( FindString, PrevMatchString ) ) {
		if( goUp ) currentItem++;
		else currentItem--;

		if( (!goUp && currentItem < 0) ||
			(goUp && currentItem >= numItems )) {

			sprintf(errmsg, TEXT("Cannot find string \"%s\""), FindInfo->lpstrFindWhat );
			MessageBox( hWnd, errmsg, TEXT(APPNAME), MB_OK|MB_ICONWARNING );
			if( hWndFind ) SetForegroundWindow( hWndFind );
			else SetFocus( hWndList );
			return FALSE;
		}
	}

	// loop through each item looking for the string
	while( 1 ) {

		// get the item text
		for( subitem = 0; subitem < NUMCOLUMNS; subitem++ ) {
			fieldtext[0] = 0;
			ListView_GetItemText( hWndList, currentItem, subitem, fieldtext, 256 );

			// make sure enought string for a match
			if( strlen( fieldtext ) < strlen( FindInfo->lpstrFindWhat ))
				continue;

			// do a scan all the way through for the substring
			if( FindInfo->Flags & FR_WHOLEWORD ) {

				i = 0;
				while( fieldtext[i] ) {
					while( fieldtext[i] && fieldtext[i] != ' ' ) i++;
					if( FindInfo->Flags & FR_MATCHCASE ) 
						match = !strcmp( fieldtext, FindInfo->lpstrFindWhat );
					else
						match = !stricmp( fieldtext, FindInfo->lpstrFindWhat );
					if( match) break;
					i++;
				}	
			} else {
				for( i = 0; i < strlen( fieldtext ) - strlen(FindInfo->lpstrFindWhat)+1; i++ ) {
					if( FindInfo->Flags & FR_MATCHCASE ) 
						match = !strncmp( &fieldtext[i], FindInfo->lpstrFindWhat, 
											strlen(FindInfo->lpstrFindWhat) );
					else
						match = !strnicmp( &fieldtext[i], FindInfo->lpstrFindWhat,
											strlen(FindInfo->lpstrFindWhat) );
					if( match ) break;
				}		
			}

			if( match ) {

				strcpy( PrevMatchString, FindInfo->lpstrFindWhat );
				PrevMatch = TRUE;
				// Clear all previously-selected items
				while( (clearItem = ListView_GetNextItem( hWndList, -1, LVNI_SELECTED )) != -1 ) {
					ListView_SetItemState( hWndList, clearItem, 0, LVIS_SELECTED|LVIS_FOCUSED );
				}
				ListView_SetItemState( hWndList, currentItem, 
							LVIS_SELECTED|LVIS_FOCUSED,
							LVIS_SELECTED|LVIS_FOCUSED );
				ListView_EnsureVisible( hWndList, currentItem, FALSE ); 
				SetFocus( hWndList );
				return TRUE;
			}
		}
		currentItem = currentItem + (goUp ? 1:-1);
		if( currentItem <= 0 || currentItem == numItems+1 ) {
			// end of the road
			break;
		}
	}
	sprintf(errmsg, TEXT("Cannot find string \"%s\""), FindInfo->lpstrFindWhat );
	MessageBox( hWnd, errmsg, TEXT(APPNAME), MB_OK|MB_ICONWARNING );
	if( hWndFind ) SetForegroundWindow( hWndFind );
	else SetFocus( hWndList );
	return FALSE;
}


/******************************************************************************
*
*	FUNCTION:	PopFindDialog:
*
*	PURPOSE:	Calls the find message dialog box.
*
*****************************************************************************/
void PopFindDialog(HWND hWnd)
{
	_tcscpy( FindString, PrevMatchString );
    FindTextInfo.lStructSize = sizeof( FindTextInfo );
    FindTextInfo.hwndOwner = hWnd;
    FindTextInfo.hInstance = hInst;
    FindTextInfo.lpstrFindWhat = FindString;
    FindTextInfo.lpstrReplaceWith = NULL;
    FindTextInfo.wFindWhatLen = sizeof(FindString);
    FindTextInfo.wReplaceWithLen = 0;
    FindTextInfo.lCustData = 0;
    FindTextInfo.Flags =  FindFlags;
    FindTextInfo.lpfnHook = (LPFRHOOKPROC)(FARPROC)NULL;
    FindTextInfo.lpTemplateName = NULL;

    if ((hWndFind = FindText(&FindTextInfo)) == NULL)
		MessageBox( hWnd, _T("Unable to create Find dialog"), APPNAME, MB_OK|MB_ICONERROR );      
}

/****************************************************************************
*
*	FUNCTION: MatchOkay
*
*	PURPOSE: Only thing left after compare is more mask. This routine makes
*	sure that its a valid wild card ending so that its really a match.
*
****************************************************************************/
BOOLEAN MatchOkay( PCHAR Pattern )
{
    // If pattern isn't empty, it must be a wildcard
    if( *Pattern && *Pattern != '*' ) {
 
       return FALSE;
    }

    // Matched
    return TRUE;
}


/****************************************************************************
*
*	FUNCTION: MatchWithPattern
*
*	PURPOSE: Performs nifty wildcard comparison.
*
****************************************************************************/
BOOLEAN MatchWithPattern( PCHAR Pattern, PCHAR Name )
{
	char matchchar;

    // End of pattern?
    if( !*Pattern ) {
        return FALSE;
    }

    // If we hit a wild card, do recursion
    if( *Pattern == '*' ) {

        Pattern++;
        while( *Name && *Pattern ) {

			matchchar = *Name;
			if( matchchar >= 'a' && 
				matchchar <= 'z' ) {

				matchchar -= 'a' - 'A';
			}

            // See if this substring matches
		    if( *Pattern == matchchar ) {

  		        if( MatchWithPattern( Pattern+1, Name+1 )) {

                    return TRUE;
                }
            }

            // Try the next substring
            Name++;
        }

        // See if match condition was met
        return MatchOkay( Pattern );
    } 

    // Do straight compare until we hit a wild card
    while( *Name && *Pattern != '*' ) {

		matchchar = *Name;
		if( matchchar >= 'a' && 
			matchchar <= 'z' ) {

			matchchar -= 'a' - 'A';
		}

        if( *Pattern == matchchar ) {
            Pattern++;
            Name++;

        } else {

            return FALSE;
		}
    }

    // If not done, recurse
    if( *Name ) {

        return MatchWithPattern( Pattern, Name );
    }

    // Make sure its a match
    return MatchOkay( Pattern );
}


/****************************************************************************
*
*	FUNCTION: MatchWithHighlightPattern
*
*	PURPOSE: Converts strings to upper-case before calling core 
*	comparison routine.
*
****************************************************************************/
BOOLEAN MatchWithHighlightPattern( PCHAR String )
{
	char	   *filterPtr;
	char	   curFilterBuf[MAXFILTERLEN];
	char		curMatchTest[MAXFILTERLEN];
	char	   *curFilter, *endFilter;

	// Is there a highlight filter?
	if( !HighlightString[0] ||
		(HighlightString[0] == ' ' && !HighlightString[1] )) return FALSE;

	// see if its in an highlight
	filterPtr = HighlightString;
	curFilter = curFilterBuf;
	while( 1 ) {

		endFilter = strchr( filterPtr, ';' );
		if( !endFilter )
			curFilter = filterPtr;
		else {
			strncpy( curFilter, filterPtr, (int) (endFilter - filterPtr ) );
			curFilter[ (int) (endFilter - filterPtr ) ] = 0;
		}

		// Now do the comparison
		sprintf( curMatchTest, "%s%s%s",
			*curFilter == '*' ? "" : "*",
			curFilter,
			curFilter[ strlen(curFilter)-1] == '*' ? "" : "*" );
		if( MatchWithPattern( curMatchTest, String ) ) {

			return TRUE;
		}

		if( endFilter ) filterPtr = endFilter+1;
		else break;
	}
	return FALSE;	
}


/****************************************************************************
*
*	FUNCTION:	FilterProc
*
*	PURPOSE:	Processes messages for "Filter" dialog box
*
****************************************************************************/
BOOL APIENTRY FilterProc( HWND hDlg, UINT message, UINT wParam, LONG lParam )
{
	char			newFilter[MAXFILTERLEN];
	char			newExFilter[MAXFILTERLEN];
	char			newHiFilter[MAXFILTERLEN], oldHighlight[MAXFILTERLEN];
	int				i, j, nb;
	static HWND		hInFilter;
	static HWND		hExFilter;
	static HWND		hHiFilter;

	switch ( message )  {
	case WM_INITDIALOG:

		// initialize the controls to reflect the current filter
		// We use a ' ' as a placeholder in the filter strings to represent no filter ("")
		hInFilter = GetDlgItem( hDlg, IDC_FILTERSTRING );
		for( i = 0; i < NUMRECENTFILTERS; i++ ) {
			if( RecentInFilters[i][0] ) {
				SendMessage( hInFilter,	CB_ADDSTRING, 0, 
					(LPARAM ) (strcmp( RecentInFilters[i], " ") ? 
							RecentInFilters[i] : ""));
			}
		}
		hExFilter = GetDlgItem( hDlg, IDC_EXFILTERSTRING );
		for( i = 0; i < NUMRECENTFILTERS; i++ ) {
			if( RecentExFilters[i][0] ) {
				SendMessage( hExFilter, CB_ADDSTRING, 0, 
					(LPARAM ) (strcmp( RecentExFilters[i], " ") ? 
							RecentExFilters[i] : ""));
			}
		}
		hHiFilter = GetDlgItem( hDlg, IDC_HIFILTERSTRING );
		for( i = 0; i < NUMRECENTFILTERS; i++ ) {
			if( RecentHiFilters[i][0] ) {
				SendMessage( hHiFilter, CB_ADDSTRING, 0, 
					(LPARAM ) (strcmp( RecentHiFilters[i], " ") ? 
							RecentHiFilters[i] : ""));			
			}
		}
		SendMessage( hInFilter, CB_SETCURSEL, 0, 0);
		SendMessage( hExFilter, CB_SETCURSEL, 0, 0);
		SendMessage( hHiFilter, CB_SETCURSEL, 0, 0);

		// Set the check box stats
		CheckDlgButton( hDlg, IDC_READ, 
			FilterDefinition.logreads ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton( hDlg, IDC_WRITE, 
			FilterDefinition.logwrites ? BST_CHECKED : BST_UNCHECKED );
		return TRUE;

	case WM_COMMAND:              
		strcpy( oldHighlight, HighlightString );
		if ( LOWORD( wParam ) == IDOK )	 {

			// make sure that max lines is legal
			GetDlgItemTextA( hDlg, IDC_FILTERSTRING, newFilter, MAXFILTERLEN );
			GetDlgItemTextA( hDlg, IDC_EXFILTERSTRING, newExFilter, MAXFILTERLEN );
			GetDlgItemTextA( hDlg, IDC_HIFILTERSTRING, newHiFilter, MAXFILTERLEN );
			if( !newFilter[0] ) strcpy( newFilter, " " );
			if( !newExFilter[0] ) strcpy( newExFilter, " " );
			if( !newHiFilter[0] ) strcpy( newHiFilter, " " );

			strcpy( FilterString, newFilter );
			strupr( FilterString );
			for( i = 0; i < NUMRECENTFILTERS; i++ ) {
				if( !stricmp( RecentInFilters[i], newFilter )) {	
					i++;
					break;
				}
			}
			for( j = i-2; j != (DWORD) -1; j-- ) {
				strcpy( RecentInFilters[j+1], RecentInFilters[j] );
			}
			strcpy( RecentInFilters[0], newFilter );

			strcpy( ExcludeString, newExFilter );
			strupr( ExcludeString );
			for( i = 0; i < NUMRECENTFILTERS; i++ ) {
				if( !stricmp( RecentExFilters[i], newExFilter )) {	
					i++;
					break;
				}
			}
			for( j = i-2; j != (DWORD) -1; j-- ) {
				strcpy( RecentExFilters[j+1], RecentExFilters[j] );
			}
			strcpy( RecentExFilters[0], newExFilter );

			strcpy( HighlightString, newHiFilter );
			strupr( HighlightString );
			for( i = 0; i < NUMRECENTFILTERS; i++ ) {
				if( !stricmp( RecentHiFilters[i], newHiFilter )) {	
					i++;
					break;
				}
			}
			for( j = i-2; j != (DWORD) -1; j-- ) {
				strcpy( RecentHiFilters[j+1], RecentHiFilters[j] );
			}
			strcpy( RecentHiFilters[0], newHiFilter );

			if( stricmp( oldHighlight, HighlightString )) {
				InvalidateRgn( hWndList, NULL, TRUE );
			}

			// Get the button states
			FilterDefinition.logreads = 
					(IsDlgButtonChecked( hDlg, IDC_READ ) == BST_CHECKED);
			FilterDefinition.logwrites = 
					(IsDlgButtonChecked( hDlg, IDC_WRITE ) == BST_CHECKED);

			EndDialog( hDlg, TRUE );

			// Apply the new filter
			FilterDefinition.excludefilter[0] = 0;
			FilterDefinition.includefilter[0] = 0;
			if( strcmp( ExcludeString, " " ) )
				strcpy( FilterDefinition.excludefilter, ExcludeString );
			if( strcmp( FilterString, " " ) )
				strcpy( FilterDefinition.includefilter, FilterString );
			if ( ! DeviceIoControl(	SysHandle, IOCTL_FILEMON_SETFILTER,
									(PVOID) &FilterDefinition, sizeof(FilterDefinition), 
									NULL, 0, &nb, NULL ) )	{

				MessageBox( hDlg, TEXT("Couldn't access device driver"), APPNAME, MB_ICONERROR );
				return FALSE;
			}
			return TRUE;

		} else if( LOWORD( wParam ) == IDCANCEL ) {

			EndDialog( hDlg, TRUE );

		} else if( LOWORD( wParam ) == IDRESET ) {

			// initialize the controls to reflect the current filter
			SetDlgItemText( hDlg, IDC_FILTERSTRING, "*" );
			SetDlgItemText( hDlg, IDC_EXFILTERSTRING, "" );
			SetDlgItemText( hDlg, IDC_HIFILTERSTRING, "" );
			CheckDlgButton( hDlg, IDC_READ, BST_CHECKED );
			CheckDlgButton( hDlg, IDC_WRITE, BST_CHECKED );

			if( stricmp( oldHighlight, HighlightString )) {
				InvalidateRgn( hWndList, NULL, TRUE );
			}
		}
		break;

	case WM_CLOSE:
		EndDialog( hDlg, TRUE );
		return TRUE;
	}
	return FALSE;   
}

/******************************************************************************
*
*	FUNCTION:	GetPositionSettings
*
*	PURPOSE:	Reads the Registry to get the last-set window position.
*
******************************************************************************/
VOID GetPositionSettings()
{
	HKEY	hKey;
	DWORD	ParamSize, newPosSize, i;
	POSITION_SETTINGS	newPositionInfo;
	LOGFONT	lf;
	char	*nextString;
	char	recentExList[(MAXFILTERLEN+1) * NUMRECENTFILTERS + 1];
	char	recentInList[(MAXFILTERLEN+1) * NUMRECENTFILTERS + 1];
	char	recentHiList[(MAXFILTERLEN+1) * NUMRECENTFILTERS + 1];

	// Delete old settings
	RegDeleteKey( HKEY_CURRENT_USER, "Software\\Systems Internals\\Filemon" );

	// Default font
	GetObject( GetStockObject(SYSTEM_FONT), sizeof lf, &lf ); 
	lf.lfWeight = FW_NORMAL;
	lf.lfHeight = 8;
	lf.lfWidth  = 0;
	strcpy( lf.lfFaceName, TEXT("MS Sans Serif") );
	PositionInfo.font = lf;

	// Fist, set the default settings
	PositionInfo.top			= CW_USEDEFAULT;
	PositionInfo.left			= CW_USEDEFAULT;
	PositionInfo.width			= CW_USEDEFAULT;
	PositionInfo.height			= CW_USEDEFAULT;
	PositionInfo.maximized		= FALSE;
	PositionInfo.ontop			= FALSE;
	PositionInfo.hookpipes		= FALSE;
	PositionInfo.hookslots		= FALSE;
	PositionInfo.highlightfg	= 0x00FFFFFF;
	PositionInfo.highlightbg	= 0x000000FF;

	// set the default listview widths
	PositionInfo.column[0] = 35;  // seq 
	PositionInfo.column[1] = 90;  // time
	PositionInfo.column[2] = 90;  // process
	PositionInfo.column[3] = 130; // irp
	PositionInfo.column[4] = 200; // path
	PositionInfo.column[5] = 70;  // result
	PositionInfo.column[6] = 150; // other

	// intialize the hooked drives
	PositionInfo.curdriveset = (DWORD) -1;

	// duration is default
	PositionInfo.timeduration = FALSE;

	// initialize history depth
	PositionInfo.historydepth = 0;

	// initialize filter
	recentInList[0] = '*';
	recentInList[1] = 0;
	recentInList[2] = 0;
	recentExList[0] = 0;
	recentHiList[0] = 0;
	memset( RecentExFilters,   0, sizeof( RecentExFilters ));
	memset( RecentInFilters,   0, sizeof( RecentInFilters ));
	memset( RecentHiFilters,   0, sizeof( RecentHiFilters ));
	PositionInfo.logreads = TRUE;
	PositionInfo.logwrites = TRUE;

	// first, get the last-entered params from the registry
	RegCreateKey(HKEY_CURRENT_USER, FILEMON_SETTINGS_KEY, &hKey );

	// get the params and ignore errors
	newPosSize = sizeof( PositionInfo );
	newPositionInfo.posversion = 0;
	RegQueryValueEx( hKey,FILEMON_SETTINGS_VALUE, NULL, NULL, (LPBYTE) &newPositionInfo,
				&newPosSize );
	ParamSize = sizeof( recentInList );
	RegQueryValueEx( hKey,FILEMON_RECENT_INFILTER_VALUE, NULL, NULL, (LPBYTE) &recentInList,
				&ParamSize );
	ParamSize = sizeof( recentExList );
	RegQueryValueEx( hKey,FILEMON_RECENT_EXFILTER_VALUE, NULL, NULL, (LPBYTE) &recentExList,
				&ParamSize );
	ParamSize = sizeof( recentHiList );
	RegQueryValueEx( hKey,FILEMON_RECENT_HIFILTER_VALUE, NULL, NULL, (LPBYTE) &recentHiList,
				&ParamSize );
	RegCloseKey( hKey );

	// only use the registry settings if the version matches
	if( newPositionInfo.posversion == POSITION_VERSION ) PositionInfo = newPositionInfo;

	// extract global settings from the value returned from the Registry (or the default)
	CurDriveSet			= PositionInfo.curdriveset;
	MaxLines			= PositionInfo.historydepth;
	TimeIsDuration		= PositionInfo.timeduration;
	OnTop				= PositionInfo.ontop;
	HookPipes			= PositionInfo.hookpipes;
	HookSlots			= PositionInfo.hookslots;
	ShowMs				= PositionInfo.showms;

	// get misc device filter
	FilterDefinition.logreads	= PositionInfo.logreads;
	FilterDefinition.logwrites	= PositionInfo.logwrites;

	// Set up the recent filter arrays
	nextString = recentInList;
	i = 0;
	while( *nextString ) {
		strcpy( RecentInFilters[i++], nextString );
		nextString = &nextString[strlen(nextString)+1];
	}
	nextString = recentExList;
	i = 0;
	while( *nextString ) {
		strcpy( RecentExFilters[i++], nextString );
		nextString = &nextString[strlen(nextString)+1];
	}
	nextString = recentHiList;
	i = 0;
	while( *nextString ) {
		strcpy( RecentHiFilters[i++], nextString );
		nextString = &nextString[strlen(nextString)+1];
	}

	strcpy( FilterString, RecentInFilters[0] );
	strupr( FilterString );
	strcpy( ExcludeString, RecentExFilters[0] );
	strupr( ExcludeString );	
	strcpy( HighlightString, RecentHiFilters[0] );
	strupr( HighlightString );

    // Get font
	LogFont     = PositionInfo.font;
 	hFont       = CreateFontIndirect( &LogFont ); 

	// set highlight colors
	HighlightFg = PositionInfo.highlightfg;
	HighlightBg = PositionInfo.highlightbg;
}


/******************************************************************************
*
*	FUNCTION:	SavePositionSettings
*
*	PURPOSE:	Saves the current window settings to the Registry.
*
******************************************************************************/
VOID SavePositionSettings( HWND hWnd )
{
	RECT		rc;
	int			i;
	char		*nextInString, *nextExString, *nextHiString;
	char		recentExList[(MAXFILTERLEN+1) * NUMRECENTFILTERS + 1];
	char		recentInList[(MAXFILTERLEN+1) * NUMRECENTFILTERS + 1];
	char		recentHiList[(MAXFILTERLEN+1) * NUMRECENTFILTERS + 1];
	HKEY		hKey;

	// set version #
	PositionInfo.posversion = POSITION_VERSION;

	// get the position of the main window
	GetWindowRect( hWnd, &rc );
	if( !IsIconic( hWnd ) && !IsZoomed( hWnd )) {

		PositionInfo.left = rc.left;
		PositionInfo.top = rc.top;
		PositionInfo.width = rc.right - rc.left;
		PositionInfo.height = rc.bottom - rc.top;
	} 
	PositionInfo.showtoolbar = ShowToolbar;
	PositionInfo.maximized	= IsZoomed( hWnd );
	PositionInfo.ontop		= OnTop;
	PositionInfo.hookpipes	= HookPipes;
	PositionInfo.hookslots	= HookSlots;

	// get the history depth
	PositionInfo.historydepth = MaxLines;

	// get time format
	PositionInfo.timeduration = TimeIsDuration;
	PositionInfo.showms = ShowMs;	

	// get the widths of the listview columns
	for( i = 0; i < NUMCOLUMNS; i++ ) {
		PositionInfo.column[i] = ListView_GetColumnWidth( hWndList, i );
	}

	// save font
	PositionInfo.font = LogFont;

	// get misc device filters
	PositionInfo.logreads = FilterDefinition.logreads;
	PositionInfo.logwrites = FilterDefinition.logwrites;

	// save highlight colors
	PositionInfo.highlightfg = HighlightFg;
	PositionInfo.highlightbg = HighlightBg;

	// get the current drive set
	PositionInfo.curdriveset = CurDriveSet;

	// Save recent filters
	recentInList[0] = 0;
	nextInString = recentInList;
	for( i = 0; i < NUMRECENTFILTERS; i++ ) {
		if( !RecentInFilters[i][0] ) {
			break;
		}
		strcpy( nextInString, RecentInFilters[i] );
		nextInString = &nextInString[ strlen( nextInString ) + 1];
	}
	*nextInString = 0;

	recentExList[0] = 0;
	nextExString = recentExList;
	for( i = 0; i < NUMRECENTFILTERS; i++ ) {
		if( !RecentExFilters[i][0] ) {
			break;
		}
		strcpy( nextExString, RecentExFilters[i] );
		nextExString = &nextExString[ strlen( nextExString ) + 1];
	}	
	*nextExString = 0;

	recentHiList[0] = 0;
	nextHiString = recentHiList;
	for( i = 0; i < NUMRECENTFILTERS; i++ ) {
		if( !RecentHiFilters[i][0] ) {
			break;
		}
		strcpy( nextHiString, RecentHiFilters[i] );
		nextHiString = &nextHiString[ strlen( nextHiString ) + 1];
	}
	*nextHiString = 0;

	// save connection info to registry
	RegOpenKey(HKEY_CURRENT_USER, FILEMON_SETTINGS_KEY,	&hKey );
	RegSetValueEx( hKey, FILEMON_SETTINGS_VALUE, 0, REG_BINARY, (LPBYTE) &PositionInfo,
			sizeof( PositionInfo ) );
	RegSetValueEx( hKey, FILEMON_RECENT_INFILTER_VALUE, 0, REG_BINARY, (LPBYTE) &recentInList,
			(DWORD) (nextInString - recentInList) + 1 );
	RegSetValueEx( hKey, FILEMON_RECENT_EXFILTER_VALUE, 0, REG_BINARY, (LPBYTE) &recentExList,
            (DWORD) (nextExString - recentExList) + 1 );
	RegSetValueEx( hKey, FILEMON_RECENT_HIFILTER_VALUE, 0, REG_BINARY, (LPBYTE) &recentHiList,
            (DWORD) (nextHiString - recentHiList) + 1 );
	CloseHandle( hKey );
}


/******************************************************************************
*
*	FUNCTION:	HookDrives
*
*	PURPOSE:	Hook the currently selected drives, updating menu checks
*
******************************************************************************/
DWORD HookDrives( HMENU DriveMenu, DWORD MaxDriveSet, DWORD CurDriveSet ) 
{
	DWORD nb;
	DWORD drive;

	// Tell device driver which drives to monitor
	if ( ! DeviceIoControl(	SysHandle, IOCTL_FILEMON_SETDRIVES,
							&CurDriveSet, sizeof CurDriveSet,
							&CurDriveSet, sizeof CurDriveSet,
							&nb, NULL ) )
		return 0;

	// Update menu items
	for ( drive = 0; drive < 32; ++drive )
		if ( MaxDriveSet & (1<<drive) )  {
			if ( CurDriveSet & (1<<drive) )
				CheckMenuItem( DriveMenu, IDC_DRIVE+drive, MF_BYCOMMAND|MF_CHECKED );
			else
				CheckMenuItem( DriveMenu, IDC_DRIVE+drive, MF_BYCOMMAND|MF_UNCHECKED );
		}
	return CurDriveSet;
}


/******************************************************************************
*
*	FUNCTION:	Split
*
*	PURPOSE:	Split a delimited line into components
*
******************************************************************************/
int Split( char * line, char delimiter, char * items[] )
{
	int		cnt = 0;

	for (;;)  {
		// Add prefix to list of components		
		items[cnt++] = line;

		// Check for more components
		line = strchr( line, delimiter );
		if ( line == NULL )
			return cnt;

		// Terminate previous component and move to next
		*line++ = '\0';
	}		
}


/******************************************************************************
*
*	FUNCTION:	ListAppend
*
*	PURPOSE:	Add a new line to List window
*
******************************************************************************/
BOOL ListAppend( HWND hWndList, DWORD seq, 
				 LONGLONG perfTime, LONGLONG dateTime,
				 char * line )
{
	LV_ITEM		lvI;	// list view item structure
	int			row;
	char		*items[NUMCOLUMNS];
	char		timeBuf[64], timeSub[64];
	float		elapsed;
	int			itemcnt = 0;
	char		*secondsPtr;
	FILETIME	localTime;
	SYSTEMTIME	systemTime;
	int			msIndex;

	// Split line into columns
	itemcnt = Split( line, '\t', items );
	if ( itemcnt == 0 )
		return FALSE;

	// Determine row number for request
	if ( *items[0] )  {
		// Its a new request.  Put at end.
		row = 0x7FFFFFFF;
	} else {
		// Its a status.  Locate its associated request.
		lvI.mask = LVIF_PARAM;
		lvI.iSubItem = 0;
		for ( row = ListView_GetItemCount(hWndList) - 1; row >= 0; --row )  {
			lvI.iItem = row;
			if ( ListView_GetItem( hWndList, &lvI )  &&  (DWORD)lvI.lParam == seq )
				break;
		}
		if ( row == -1 )
			// No request associated with status.
			return FALSE;
	}

	// Sequence number if a new item
	if ( *items[0] )  {
		_stprintf( msgbuf, _T("%d"), seq );
		lvI.mask		= LVIF_TEXT | LVIF_PARAM;
		lvI.iItem		= row;
		lvI.iSubItem	= 0;
		lvI.pszText		= msgbuf;
		lvI.cchTextMax	= lstrlen( lvI.pszText ) + 1;
		lvI.lParam		= seq;
		row = ListView_InsertItem( hWndList, &lvI );
		if ( row == -1 )  {
			_stprintf( msgbuf, _T("Error adding item %d to list view"), seq );
			MessageBox( hWndList, msgbuf, APPNAME, MB_OK|MB_ICONERROR );
			return FALSE;
		}
        LastRow = row;
	}
	if( !TimeIsDuration ) {

		// no timestamp for completions
		if( dateTime ) {

			if( IsNT ) {
				FileTimeToLocalFileTime( (PFILETIME) &dateTime, &localTime );
				FileTimeToSystemTime( &localTime, &systemTime );
			} else {
				DosDateTimeToFileTime( (WORD) (dateTime >> 48), (WORD) (dateTime >> 32), &localTime );
				FileTimeToSystemTime( &localTime, &systemTime );
				systemTime.wSecond += ((WORD) dateTime) / 1000;
				systemTime.wMilliseconds = ((WORD) dateTime) % 1000;
			}
			GetTimeFormat( LOCALE_USER_DEFAULT, 0,
						   &systemTime, NULL, timeBuf, 64 );
			if( ShowMs ) {

				secondsPtr = strrchr( timeBuf, ':');
				msIndex = (DWORD) (secondsPtr - timeBuf);
				while( timeBuf[msIndex] && timeBuf[msIndex] != ' ') msIndex++;
				strcpy( timeSub, &timeBuf[msIndex] );
				timeBuf[ msIndex ] = 0;
				sprintf( msgbuf, "%s.%03d%s", timeBuf, systemTime.wMilliseconds, timeSub );
			} else {

				strcpy( msgbuf, timeBuf );
			}
			ListView_SetItemText( hWndList, row, 1, msgbuf );
		}

	} else {

		if( IsNT ) {
			elapsed = ((float) perfTime)/(float)PerfFrequency.QuadPart;
			sprintf( msgbuf, "%10.8f", elapsed );
		} else {
			sprintf( msgbuf, "%10.8f", (float) perfTime * 0.8 / 1e6);
		}
		ListView_SetItemText( hWndList, row, 1, msgbuf );
	}

	// Process name
	if ( itemcnt>0 && *items[0] ) {

		ListView_SetItemText( hWndList, row, 2, items[0] );
	}

	// Request type
	if ( itemcnt>1 && *items[1] )  {

		ListView_SetItemText( hWndList, row, 3, items[1] );
	}

	// Path
	if ( itemcnt>2 && *items[2] )  {

		ListView_SetItemText( hWndList, row, 4, items[2] );
	}

	// Result
	if ( itemcnt>4 && *items[4] )  {

		ListView_SetItemText( hWndList, row, 5, items[4] );
	}

	// Additional
	if ( itemcnt>3 && *items[3] )  {

		ListView_SetItemText( hWndList, row, 6, items[3] );
	}
	return TRUE;
}


/******************************************************************************
*
*	FUNCTION:	UpdateStatistics
*
*	PURPOSE:	Clear the statistics window and refill it with the current 
*				contents of the statistics buffer.  Does not refresh the 
*				buffer from the device driver.
*
******************************************************************************/
void UpdateStatistics( HWND hWnd, HWND hList, BOOL Clear )
{
	PENTRY	ptr;
	BOOLEAN itemsAdded = FALSE;
	int		totitems, i;

	// Just return if nothing to do
	if ( !Clear  &&  StatsLen < sizeof(int)+2 )
		return;

	// post hourglass icon
	if( !IsNT ) {
		
		hSaveCursor = SetCursor(hHourGlass);
		SendMessage(hList, WM_SETREDRAW, FALSE, 0);
	}

	// Start with empty list
	if( Clear ) {

		if( IsNT ) {

			ListView_DeleteAllItems( hList );
		} else {

			// Win9x listview clear (or delete) is *very* slow
			Deleting = TRUE;
			totitems = ListView_GetItemCount( hList );
			for(i = 0; i < totitems; i++) {
				ListView_DeleteItem( hList, 0 );
			}
			Deleting = FALSE;
		}
		LastRow = 0;
	}

	// Add all List items from Stats[] data
	for ( ptr = (void *)Stats; (char *)ptr < min(Stats+StatsLen,Stats + sizeof (Stats)); )  {
	 	// Add to list
		size_t len = strlen(ptr->text);
        
		itemsAdded |= ListAppend( hList, ptr->seq, ptr->perftime.QuadPart, 
								ptr->datetime.QuadPart, ptr->text );

		if( IsNT ) {
			
			len += 4; len &= 0xFFFFFFFC; // +1 for null-terminator +3 for 32bit alignment
			ptr = (void *)(ptr->text + len);
		} else 
			ptr = (void *)(ptr->text + len + 1);
	}

	// Empty the buffer
	StatsLen = 0;

	// only do stuff if we added stuff
	if( itemsAdded ) {

		// limit number of lines saved
		if (MaxLines) {
			SendMessage(hList, WM_SETREDRAW, FALSE, 0);
			while ( LastRow > MaxLines ) {
				ListView_DeleteItem ( hList, 0 );
				LastRow--;
			}
			SendMessage(hList, WM_SETREDRAW, TRUE, 0);
		}

		// Scroll so newly added items are visible
		if ( Autoscroll ) {
			if( hBalloon ) DestroyWindow( hBalloon );
			ListView_EnsureVisible( hList, ListView_GetItemCount(hList)-1, FALSE ); 
		}
	}

	if( !IsNT) {
		SendMessage(hList, WM_SETREDRAW, TRUE, 0);
		InvalidateRect( hList, NULL, FALSE );
		SetCursor( hSaveCursor );
	}
}

/****************************************************************************
*
*    FUNCTION: CalcStringEllipsis
*
*    PURPOSE:  Determines if an item will fit in a listview row, and if
*			   not, attaches the appropriate number of '.' to a truncated 
*			   version.
*
****************************************************************************/
BOOL WINAPI CalcStringEllipsis (HDC     hdc, 
                                LPTSTR  szString, 
                                int     cchMax, 
                                UINT    uColWidth) 
{ 
    static TCHAR szEllipsis3[] = TEXT("..."); 
    static TCHAR szEllipsis2[] = TEXT(".."); 
    static TCHAR szEllipsis1[] = TEXT("."); 
    SIZE		sizeString; 
    SIZE		sizeEllipsis3, sizeEllipsis2, sizeEllipsis1; 
    int			cbString; 
    LPTSTR		lpszTemp; 
     
    // Adjust the column width to take into account the edges 
    uColWidth -= 4; 

	// Allocate a string for us to work with.  This way we can mangle the 
    // string and still preserve the return value 
    lpszTemp = (LPTSTR)HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, cchMax); 
    if (!lpszTemp) return FALSE;
    lstrcpy (lpszTemp, szString); 
 
    // Get the width of the string in pixels 
    cbString = lstrlen(lpszTemp); 
    if (!GetTextExtentPoint32 (hdc, lpszTemp, cbString, &sizeString)) {
        HeapFree (GetProcessHeap(), 0, (LPVOID)lpszTemp); 
		return FALSE;
    } 
 
    // If the width of the string is greater than the column width shave 
    // the string and add the ellipsis 
    if ((ULONG)sizeString.cx > uColWidth) {
		
        if (!GetTextExtentPoint32 (hdc, 
                                   szEllipsis3, 
                                   lstrlen(szEllipsis3), 
                                   &sizeEllipsis3)) {
			HeapFree (GetProcessHeap(), 0, (LPVOID)lpszTemp); 
			return FALSE;
        } 
        if (!GetTextExtentPoint32 (hdc, 
                                   szEllipsis2, 
                                   lstrlen(szEllipsis2), 
                                   &sizeEllipsis2)) {
			HeapFree (GetProcessHeap(), 0, (LPVOID)lpszTemp); 
			return FALSE;
        } 
        if (!GetTextExtentPoint32 (hdc, 
                                   szEllipsis1, 
                                   lstrlen(szEllipsis1), 
                                   &sizeEllipsis1)) {
			HeapFree (GetProcessHeap(), 0, (LPVOID)lpszTemp); 
			return FALSE;
        } 
 
        while (cbString > 0) { 

			lpszTemp[--cbString] = 0; 
			if (!GetTextExtentPoint32 (hdc, lpszTemp, cbString, &sizeString)) {
 				HeapFree (GetProcessHeap(), 0, (LPVOID)lpszTemp); 
				return FALSE;
			} 
			if ((ULONG)(sizeString.cx + DOTOFFSET + sizeEllipsis3.cx) <= uColWidth) { 
				break;
			} 
        } 
		lpszTemp[0] = szString[0];
 		if((ULONG)(sizeString.cx + DOTOFFSET + sizeEllipsis3.cx) <= uColWidth) { 
			lstrcat (lpszTemp, szEllipsis3); 
		} else if((ULONG)(sizeString.cx + DOTOFFSET + sizeEllipsis2.cx) <= uColWidth) { 
			lstrcat (lpszTemp, szEllipsis2); 
		} else if((ULONG)(sizeString.cx + DOTOFFSET + sizeEllipsis1.cx) <= uColWidth) { 
			lstrcat (lpszTemp, szEllipsis1); 
		} else {
			lpszTemp[0] = szString[0];
		}
        lstrcpy (szString, lpszTemp); 
 		HeapFree (GetProcessHeap(), 0, (LPVOID)lpszTemp); 
		return TRUE;

    } else {

		HeapFree (GetProcessHeap(), 0, (LPVOID)lpszTemp); 
		return TRUE;
	}

	HeapFree (GetProcessHeap(), 0, (LPVOID)lpszTemp); 
	return FALSE;
} 


/****************************************************************************
*
*    FUNCTION: DrawItemColumn
*
*    PURPOSE:  Draws text to the listview.
*
****************************************************************************/
void WINAPI DrawItemColumn (HDC hdc, LPTSTR szText, LPRECT prcClip) 
{ 
    TCHAR szString[MAXITEMLENGTH]; 

    // Check to see if the string fits in the clip rect.  If not, truncate 
    // the string and add "...". 
    lstrcpy(szString, szText); 
    CalcStringEllipsis (hdc, szString, sizeof( szString ), prcClip->right - prcClip->left); 
    ExtTextOut (hdc, 
                prcClip->left + 2, 
                prcClip->top + 1, 
                ETO_CLIPPED | ETO_OPAQUE, 
                prcClip, 
                szString, 
                lstrlen(szString), 
                NULL); 
} 


/****************************************************************************
*
*    FUNCTION: DrawListViewItem
*
*    PURPOSE:  Handles a request from Windows to draw one of the lines
*				in the listview window.
*
****************************************************************************/
void DrawListViewItem(LPDRAWITEMSTRUCT lpDrawItem)
{
	TCHAR 		colString[NUMCOLUMNS][MAXITEMLENGTH];
	BOOLEAN		highlight = FALSE;
    LV_ITEM		lvi;
    RECT		rcClip;
    int			iColumn;
	DWORD		width, leftOffset;
	UINT		uiFlags = ILD_TRANSPARENT;

    // Get the item image to be displayed
    lvi.mask = LVIF_IMAGE | LVIF_STATE;
    lvi.iItem = lpDrawItem->itemID;
    lvi.iSubItem = 0;
    ListView_GetItem(lpDrawItem->hwndItem, &lvi);

	// Get the column text and see if there is a highlight
	for( iColumn = 0; iColumn < NUMCOLUMNS; iColumn++ ) {
		colString[iColumn][0] = 0;
		ListView_GetItemText( hWndList, lpDrawItem->itemID,
							  iColumn, colString[iColumn], 
							  MAXITEMLENGTH);
		if( !highlight && iColumn != 0) {

			highlight = MatchWithHighlightPattern( colString[iColumn] );
		}
	}

    // Check to see if this item is selected
	if (lpDrawItem->itemState & ODS_SELECTED) {

        // Set the text background and foreground colors
		SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHT));

		// Also add the ILD_BLEND50 so the images come out selected
		uiFlags |= ILD_BLEND50;
    } else {
        // Set the text background and foreground colors to the standard window
        // colors
		if( highlight ) {
			SetTextColor(lpDrawItem->hDC, HighlightFg ); 
	        SetBkColor(lpDrawItem->hDC, HighlightBg );
		} else {
			SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOWTEXT));
			SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOW));
		}
    }

    // Set up the new clipping rect for the first column text and draw it
	leftOffset = 0;
	for( iColumn = 0; iColumn< NUMCOLUMNS; iColumn++ ) {

		width = ListView_GetColumnWidth( hWndList, iColumn );
		rcClip.left		= lpDrawItem->rcItem.left + leftOffset;
		rcClip.right	= lpDrawItem->rcItem.left + leftOffset + width;
		rcClip.top		= lpDrawItem->rcItem.top;
		rcClip.bottom	= lpDrawItem->rcItem.bottom;

		DrawItemColumn(lpDrawItem->hDC, colString[iColumn], &rcClip);
		leftOffset += width;
	}

    // If we changed the colors for the selected item, undo it
    if (lpDrawItem->itemState & ODS_SELECTED) {
        // Set the text background and foreground colors
        SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOW));
    }

    // If the item is focused, now draw a focus rect around the entire row
    if (lpDrawItem->itemState & ODS_FOCUS)
    {
        // Adjust the left edge to exclude the image
        rcClip = lpDrawItem->rcItem;

        // Draw the focus rect
        DrawFocusRect(lpDrawItem->hDC, &rcClip);
    }
}


/****************************************************************************
* 
*    FUNCTION: ListViewSubclass(HWND,UINT,WPARAM)
*
*    PURPOSE:  Subclasses the listview so that we can do tooltips
*
****************************************************************************/
LRESULT CALLBACK ListViewSubclass(HWND hWnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
{
	ITEM_CLICK		itemClick;
	LVHITTESTINFO	hitItem;
	static initTrack = FALSE;
	POINT           hitPoint, topleftPoint, bottomrightPoint;
	RECT			listRect;
	static POINTS  mousePosition;

	if( !initTrack ) {

		SetTimer( hWnd,	2, BALLOONDELAY, NULL );
		initTrack = TRUE;
	}

    switch (uMsg) {

	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_MOUSEMOVE:

		// delete any existing balloon
		if( hBalloon ) DestroyWindow( hBalloon );

		// save mouse position and reset the timer
		mousePosition = MAKEPOINTS( lParam );
		SetTimer( hWnd,	2, BALLOONDELAY, NULL );
		break;

	case WM_VSCROLL:
	case WM_HSCROLL:
	case WM_KEYDOWN:
		if( hBalloon ) DestroyWindow( hBalloon );

		if( uMsg == WM_KEYDOWN && 
			wParam == VK_ESCAPE &&
			hWndFind ) {

			DestroyWindow( hWndFind );
			hWndFind = NULL;
		}
		break;

	case WM_RBUTTONDOWN:
		mousePosition = MAKEPOINTS( lParam );
		SetTimer( hWnd,	2, BALLOONDELAY, NULL );
		// fall-through

	case WM_TIMER:

		// are we currently in the listview?
		GetCursorPos( &hitPoint );
		GetClientRect( hWnd, &listRect );
		topleftPoint.x = listRect.left;
		topleftPoint.y = listRect.top;
		ClientToScreen( hWnd, &topleftPoint );
		bottomrightPoint.x = listRect.right;
		bottomrightPoint.y = listRect.bottom;
		ClientToScreen( hWnd, &bottomrightPoint );
		if( hitPoint.x < topleftPoint.x ||
			hitPoint.x > bottomrightPoint.x ||
			hitPoint.y < topleftPoint.y ||
			hitPoint.y > bottomrightPoint.y ||
			(hWndFind && GetFocus() != hWndList) ) {

			// delete any existing balloon
			if( hBalloon ) DestroyWindow( hBalloon );
			break;
		}

		hitItem.pt.x = mousePosition.x;
		hitItem.pt.y =  mousePosition.y;
		if(	ListView_SubItemHitTest( hWndList, &hitItem ) != -1 ) {

			itemClick.itemText[0] = 0;
			ListView_GetItemText( hWndList, hitItem.iItem,
					hitItem.iSubItem, itemClick.itemText, 1024 );

			// delete any existing balloon
			if( hBalloon ) DestroyWindow( hBalloon );

			if( strlen( itemClick.itemText ) ) {

				if( hitItem.iSubItem ) {

					ListView_GetSubItemRect( hWndList, hitItem.iItem, hitItem.iSubItem,
							LVIR_BOUNDS, &itemClick.itemPosition);

					itemClick.itemPosition.bottom -= itemClick.itemPosition.top;
					itemClick.itemPosition.right  -= itemClick.itemPosition.left;

				} else {

					ListView_GetSubItemRect( hWndList, hitItem.iItem, 0,
							LVIR_BOUNDS, &itemClick.itemPosition);

					itemClick.itemPosition.bottom -= itemClick.itemPosition.top;
					itemClick.itemPosition.right  = ListView_GetColumnWidth( hWndList, 0 );
					itemClick.itemPosition.left   = 0;
				}

				hitPoint.y = itemClick.itemPosition.top;
				hitPoint.x = itemClick.itemPosition.left;

				ClientToScreen( hWnd, &hitPoint );

				itemClick.itemPosition.left = hitPoint.x;
				itemClick.itemPosition.top  = hitPoint.y;

				// pop-up a balloon (tool-tip like window)
				hBalloon = CreateWindowEx( 0, "BALLOON", 
								"balloon", 
								WS_POPUP|WS_BORDER,
								100, 100,
								200, 200,
								hWndMain, NULL, hInst, 
								&itemClick );
				if( hBalloon) SetFocus( hWnd );
			}
		}
		break;
    }

	// pass-through to real listview handler
    return CallWindowProc(ListViewWinMain, hWnd, uMsg, wParam, 
            lParam);
}


/****************************************************************************
* 
*    FUNCTION: CreateListView(HWND)
*
*    PURPOSE:  Creates the statistics list view window and initializes it
*
****************************************************************************/
HWND CreateList( HWND hWndParent )
{
	HWND		hWndList;    	  	// handle to list view window
	RECT		rc;         	  	// rectangle for setting size of window
	LV_COLUMN	lvC;				// list view column structure
	DWORD		j;
	static char process9xLabel[] = {"Process"};
	static struct {
		TCHAR *	Label;	// title of column
		DWORD	Width;
	} column[] = {
		{	"#"			},
		{	"Time"		},
		{	"Process"	},
		{	"Request"	},
		{	"Path"		},
		{	"Result"	},
		{	"Other"		},
	};

	// Ensure that the common control DLL is loaded.
	InitCommonControls();

	// Set the column widths according to the user-settings
	for( j = 0; j < NUMCOLUMNS; j++ ) {
		column[j].Width = PositionInfo.column[j];
	}

	// Get the size and position of the parent window.
	GetClientRect( hWndParent, &rc );

	// Create the list view window
	hWndList = CreateWindowEx(  WS_EX_OVERLAPPEDWINDOW, WC_LISTVIEW, TEXT(""), 
								WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT |
								LVS_OWNERDRAWFIXED,
								0, ShowToolbar ? TOOLBARHEIGHT : 0, 
								rc.right - rc.left, 
								rc.bottom - rc.top - (ShowToolbar ? TOOLBARHEIGHT : 0),
								hWndParent,	(HMENU)ID_LIST, hInst, NULL ); 
	if ( hWndList == NULL )
		return NULL;

	// Make it a nice fix-width font for easy reading
	SendMessage( hWndList, WM_SETFONT, (WPARAM) hFont, (LPARAM) 0 );

	// Initialize columns
	lvC.mask	= LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvC.fmt		= LVCFMT_LEFT;	// left-align column

	// Add the columns.
	for ( j = 0; j < sizeof column/sizeof column[0]; ++j )  {
		lvC.iSubItem	= j;
		lvC.cx			= column[j].Width;
		if( j == 2 && !IsNT ) 
			lvC.pszText		= process9xLabel;
		else 
			lvC.pszText		= column[j].Label;
		if ( ListView_InsertColumn( hWndList, j, &lvC ) == -1 )
			return NULL;
	}

	// Set full-row selection
	SendMessage( hWndList, LVM_SETEXTENDEDLISTVIEWSTYLE,
			LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT );

	// Sub-class
	ListViewWinMain = (WNDPROC) SetWindowLongPtr(hWndList, 
                                                 GWLP_WNDPROC,
                                                 (LONG_PTR) ListViewSubclass); 
	return hWndList;
}



/****************************************************************************
* 
*    FUNCTION: SaveFile()
*
*    PURPOSE:  Lets the user go select a file.
*
****************************************************************************/
void SaveFile( HWND hWnd, HWND ListBox, BOOLEAN SaveAs )
{
	OPENFILENAME	SaveFileName;
	TCHAR			szFile[MAX_PATH] = _T(""), fieldtext[MAXITEMLENGTH];
	TCHAR			output[MAXITEMLENGTH*NUMCOLUMNS];
	FILE			*hFile;
	int				numitems;
	int				row, subitem;

	if( SaveAs || !FileChosen ) {
		SaveFileName.lStructSize       = sizeof (SaveFileName);
		SaveFileName.hwndOwner         = hWnd;
		SaveFileName.hInstance         = hInst;
		SaveFileName.lpstrFilter       = _T("File Info (*.LOG)\0*.LOG\0All (*.*)\0*.*\0");
		SaveFileName.lpstrCustomFilter = (LPTSTR)NULL;
		SaveFileName.nMaxCustFilter    = 0L;
		SaveFileName.nFilterIndex      = 1L;
		SaveFileName.lpstrFile         = szFile;
		SaveFileName.nMaxFile          = 256;
		SaveFileName.lpstrFileTitle    = NULL;
		SaveFileName.nMaxFileTitle     = 0;
		SaveFileName.lpstrInitialDir   = NULL;
		SaveFileName.lpstrTitle        = _T("Save File Info...");
		SaveFileName.nFileOffset       = 0;
		SaveFileName.nFileExtension    = 0;
		SaveFileName.lpstrDefExt       = _T("*.log");
		SaveFileName.lpfnHook		   = NULL;
 		SaveFileName.Flags = OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

		if( !GetSaveFileName( &SaveFileName )) 
			return;
	} else 
		// open previous szFile
		_tcscpy( szFile, szFileName );

	// open the file
	hFile = _tfopen( szFile, _T("w") );
	if( !hFile ) {
		MessageBox(	NULL, _T("Create File Failed."),
				_T("Save Error"), MB_OK|MB_ICONSTOP );
		return;
	}

	// post hourglass icon
	SetCapture(hWnd);
	hSaveCursor = SetCursor(hHourGlass);

	numitems = ListView_GetItemCount(ListBox);
	for ( row = 0; row < numitems; row++ )  {
		output[0] = 0;
		for( subitem = 0; subitem < NUMCOLUMNS; subitem++ ) {
			fieldtext[0] = 0;
			ListView_GetItemText( ListBox, row, subitem, fieldtext, 256 );
			_tcscat( output, fieldtext );
			_tcscat( output, _T("\t") );
		}
		_ftprintf( hFile, _T("%s\n"), output );
	}
	fclose( hFile );
	_tcscpy( szFileName, szFile );
	FileChosen = TRUE;
	SetCursor( hSaveCursor );
	ReleaseCapture(); 
}


/****************************************************************************
*
*	FUNCTION:	HistoryProc
*
*	PURPOSE:	Processes messages for "Filter" dialog box
*
****************************************************************************/
BOOL APIENTRY HistoryProc( HWND hDlg, UINT message, UINT wParam, LONG lParam )
{
	DWORD			newMaxLines, numRows;
	char			history[64];

	switch ( message )  {
	case WM_INITDIALOG:

		// initialize the controls to reflect the current filter
		sprintf( history, "%d", MaxLines );
		SetDlgItemTextA( hDlg, IDC_HISTORY, history );
		SendMessage (GetDlgItem( hDlg, IDC_SPIN), UDM_SETRANGE, 0L, 
							MAKELONG (9999, 0));
		return TRUE;

	case WM_COMMAND:              
		if ( LOWORD( wParam ) == IDOK )	 {

			// make sure that max lines is legal
			GetDlgItemTextA( hDlg, IDC_HISTORY, history, 64 );
			if( !sscanf( history, "%d", &newMaxLines )) {

				MessageBox(	NULL, TEXT("Invalid History Depth."),
						TEXT("Filter Error"), MB_OK|MB_ICONWARNING );
				return TRUE;
			} 
			MaxLines = newMaxLines;

			EndDialog( hDlg, TRUE );
			if (MaxLines ) {
				numRows = ListView_GetItemCount( hWndList );
				SendMessage(hWndList, WM_SETREDRAW, FALSE, 0);
				while ( numRows >= MaxLines ) {
					ListView_DeleteItem ( hWndList, 0 );
					numRows--;
				}
				SendMessage(hWndList, WM_SETREDRAW, TRUE, 0);
			}
			return TRUE;

		} else if( LOWORD( wParam ) == IDCANCEL ) {

			EndDialog( hDlg, TRUE );
		} else if( LOWORD( wParam ) == IDRESET ) {

			// reset filter to default of none
			SetDlgItemTextA( hDlg, IDC_HISTORY, "0" );
		}
		break;
	case WM_CLOSE:
		EndDialog( hDlg, TRUE );
		return TRUE;
	}
	return FALSE;   
}


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


/******************************************************************************
*
*	FUNCTION:	GetDLLVersion
*
*	PURPOSE:	Gets the version number of the specified DLL.
*
******************************************************************************/
HRESULT GetDLLVersion( PTCHAR DllName, LPDWORD pdwMajor, LPDWORD pdwMinor)
{
	HINSTANCE			hDll;
	HRESULT				hr = S_OK;
	DLLVERSIONINFO_		dvi;

	*pdwMajor = 0;
	*pdwMinor = 0;

	//Load the DLL.
	hDll = LoadLibrary(DllName);

	if( hDll ) {

	   pDllGetVersionProc = (PVOID)GetProcAddress(hDll, "DllGetVersion");

	   if(pDllGetVersionProc) {
  
		  ZeroMemory(&dvi, sizeof(dvi));
		  dvi.cbSize = sizeof(dvi);

		  hr = (*pDllGetVersionProc)(&dvi);
  
		  if(SUCCEEDED(hr)) {

			 *pdwMajor = dvi.dwMajorVersion;
			 *pdwMinor = dvi.dwMinorVersion;
		  }
 	  } else {

		  // If GetProcAddress failed, the DLL is a version previous to the one 
		  // shipped with IE 3.x.
		  *pdwMajor = 4;
		  *pdwMinor = 0;
      }
   
	  FreeLibrary(hDll);
	  return hr;
	}

	return E_FAIL;
}


/****************************************************************************
*
*    FUNCTION: MainWndProc(HWND, unsigned, WORD, LONG)
*
*    PURPOSE:  Processes messages for the statistics window.
*
****************************************************************************/
LRESULT APIENTRY MainWndProc( HWND hWnd, UINT message, UINT wParam, LPARAM lParam) 
{
	static DWORD	MaxDriveSet = 0;
	static HMENU	DriveMenu;
    static HWND		hWndTT;
	static HWND		hWndToolbar;
	static POINTS   hoverPoints;
	DWORD			newDriveSet;
	LPTOOLTIPTEXT	lpToolTipText;
	LPFINDREPLACE	findMessageInfo;
#if _DEBUG
	ULONG			irpcount;
#endif
	POINT			hitPoint;
	BOOLEAN			hookDrive;
	LOGFONT			lf;
	CHOOSEFONT		chf;
	RECT			listRect;
	DWORD			nb, versionNumber;
	TEXTMETRIC		textMetrics;
	DWORD			drive, drivetype;
	static TCHAR	driverPath[ MAX_PATH ];
	TCHAR			Path[ MAX_PATH ];
	TCHAR			systemRoot[ MAX_PATH ];
	static TCHAR	szBuf[MAX_PATH];
	TCHAR 			name[ MAX_PATH ];
	TCHAR			*File;
	RECT			rc;  
	DWORD			majorver, minorver;
	WIN32_FIND_DATA findData;
	HANDLE			findHandle;
	DWORD			startTime, error;
	HDC				hDC;
	FILE_SYSTEM_TYPE fsType;
	PAINTSTRUCT		Paint;

	switch ( message ) {

		case WM_CREATE:


			// get hourglass icon ready
			hHourGlass = LoadCursor( NULL, IDC_WAIT );

			// post hourglass icon
			SetCapture(hWnd);
			hSaveCursor = SetCursor(hHourGlass);

			// determine performance counter frequency
			QueryPerformanceFrequency( &PerfFrequency );

			// Create the toolbar control - use modern style if available.
			GetDLLVersion( _T("comctl32.dll"), &majorver, &minorver );
			if( majorver > 4 || (majorver == 4 && minorver >= 70) ) {
				hWndToolbar = CreateToolbarEx( 
					hWnd, TOOLBAR_FLAT | WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS,  
					ID_TOOLBAR, NUMTOOLBARBUTTONS, hInst, IDB_TOOLBAR, (LPCTBBUTTON)&tbButtons,
					NUMBUTTONS, 16,16,16,15, sizeof(TBBUTTON)); 
			} else {
				hWndToolbar = CreateToolbarEx( 
					hWnd, WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS,  
					ID_TOOLBAR, NUMTOOLBARBUTTONS, hInst, IDB_TOOLBAR, (LPCTBBUTTON)&tbButtonsOld,
					NUMBUTTONSOLD, 16,16,16,15, sizeof(TBBUTTON)); 
			}
			if (hWndToolbar == NULL ) MessageBox (NULL, _T("Toolbar not created!"), NULL, MB_OK );

			// Create the ListBox within the main window
			hWndList = CreateList( hWnd );
			if ( hWndList == NULL )   MessageBox( NULL, _T("List not created!"), NULL, MB_OK );

		    // open the handle to the device
			if( IsNT ) {

				GetCurrentDirectory( sizeof Path, Path );
				_stprintf( Path+lstrlen(Path), _T("\\%s"), SYS_FILE );

				findHandle = FindFirstFile( Path, &findData );
				if( findHandle == INVALID_HANDLE_VALUE ) {

					if( !SearchPath( NULL, SYS_FILE, NULL, sizeof(Path), Path, &File ) ) {

						_stprintf( msgbuf, _T("%s was not found."), SYS_FILE );
						return Abort( hWnd, msgbuf, GetLastError() );
					}

				} else FindClose( findHandle );

				// copy the driver to <winnt>\system32\drivers so that we
				// can run off of a CD or network drive
				if( !GetEnvironmentVariable( "SYSTEMROOT", systemRoot, sizeof(systemRoot))) {

					strcpy( msgbuf, _T("Could not resolve SYSTEMROOT environment variable") );
					return Abort( hWnd, msgbuf, GetLastError() );
				}
				sprintf( driverPath, _T("%s\\system32\\drivers\\%s"), 
								systemRoot, SYS_FILE );
				if( !LoadDeviceDriver( SYS_NAME, driverPath, &SysHandle, &error ) )  {

					if( !CopyFile( Path, driverPath, FALSE )) {

						sprintf( msgbuf, _T("Unable to copy %s to %s\n\n")
							_T("Make sure that %s is in the current directory."), 
							SYS_NAME, driverPath, SYS_FILE );
						return Abort( hWnd, msgbuf, GetLastError() );
					}
					SetFileAttributes( driverPath, FILE_ATTRIBUTE_NORMAL );
					if( !LoadDeviceDriver( SYS_NAME, driverPath, &SysHandle, &error ) )  {

						UnloadDeviceDriver( SYS_NAME );
						if( !LoadDeviceDriver( SYS_NAME, driverPath, &SysHandle, &error ) )  {

							_stprintf( msgbuf, _T("Error loading %s (%s): %d"),
								SYS_NAME, Path, error );
							DeleteFile( driverPath );
							return Abort( hWnd, msgbuf, error );
						}
					}
					DeleteFile( driverPath );
				}

				// Correct driver version?
				if( !DeviceIoControl(	SysHandle, IOCTL_FILEMON_VERSION,
										NULL, 0, &versionNumber, sizeof(DWORD), &nb, NULL ) ||
						versionNumber != FILEMONVERSION ) {

					MessageBox( hWnd, _T("Filemon located a driver with the wrong version.\n")
						_T("If you just installed a new version you must reboot before you are\n")
						_T("able to use it."), _T("Filemon"), MB_ICONERROR);
					PostQuitMessage( 1 );
					return 0;
				}
			} else {

				// connect with VxD
				SysHandle = CreateFile( VXD_FILE, 0, 0, NULL,
								0, FILE_FLAG_OVERLAPPED|
								FILE_FLAG_DELETE_ON_CLOSE,
								NULL );
				if ( SysHandle == INVALID_HANDLE_VALUE )  {
					_stprintf( msgbuf, "%s is not loaded properly.", VXD_NAME );
					return Abort( hWnd, msgbuf, GetLastError() );
				}
			}

			// Have driver zero information
			if ( ! DeviceIoControl(	SysHandle, IOCTL_FILEMON_ZEROSTATS,
				NULL, 0, NULL, 0, &nb, NULL ) ) {

				Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
				return 0;
			}

			// Set up the filter
			FilterDefinition.excludefilter[0] = 0;
			FilterDefinition.includefilter[0] = 0;
			if( strcmp( ExcludeString, " " ) )
				strcpy( FilterDefinition.excludefilter, ExcludeString );
			if( strcmp( FilterString, " " ) )
				strcpy( FilterDefinition.includefilter, FilterString );

			// Give the user to change initial filter
			if( strcmp( FilterString, "*" ) ||
				(*ExcludeString && strcmp( ExcludeString, " "))) {

				DialogBox( hInst, TEXT("InitFilter"), hWnd, (DLGPROC) FilterProc );
						
			} else {

				// tell the driver the initial filter
				if ( ! DeviceIoControl(	SysHandle, IOCTL_FILEMON_SETFILTER,
										&FilterDefinition, sizeof(FILTER), NULL, 
										0, &nb, NULL ) ) {

					return Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
				}	
			}
			CheckMenuItem( GetMenu(hWnd), IDM_TIME,
							MF_BYCOMMAND|(TimeIsDuration?MF_CHECKED:MF_UNCHECKED) ); 
			CheckMenuItem( GetMenu( hWnd ), IDM_SHOWMS,
						   MF_BYCOMMAND|(ShowMs?MF_CHECKED:MF_UNCHECKED));
			EnableMenuItem( GetMenu( hWnd ), IDM_SHOWMS, 
							MF_BYCOMMAND|(!TimeIsDuration?MF_ENABLED:MF_GRAYED));

			// Tell driver to start filtering
			if ( ! DeviceIoControl(	SysHandle, IOCTL_FILEMON_STARTFILTER,
									NULL, 0, NULL, 0, &nb, NULL ) )	{

				return Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
			}	

			// Create a pop-up menu item with the drives
			if( IsNT ) {

				DriveMenu = CreateMenu();

				// Add the special file systems menu items
				InsertMenu( DriveMenu, 0xFFFFFFFF, MF_BYPOSITION|MF_STRING,
							IDC_DRIVE+PIPE_DRIVE, _T("Named Pipes") );
				InsertMenu( DriveMenu, 0xFFFFFFFF, MF_BYPOSITION|MF_STRING,
							IDC_DRIVE+SLOT_DRIVE, _T("Mail Slots") );
				InsertMenu( DriveMenu, 0xFFFFFFFF, MF_BYPOSITION|MF_SEPARATOR,
							0, NULL );

				// Get available drives we can monitor
				MaxDriveSet = GetLogicalDrives();
				if( PositionInfo.curdriveset != (DWORD) -1 )
					CurDriveSet = PositionInfo.curdriveset;
				else
					CurDriveSet = MaxDriveSet;
				for ( drive = 0; drive < 32; ++drive )  {
					if ( MaxDriveSet & (1 << drive) )  {
						_stprintf( name, _T("%c:\\"), 'A'+drive );
						switch ( GetDriveType( name ) )  {
							// We don't like these: remove them
							case 0:					// The drive type cannot be determined.
							case 1:					// The root directory does not exist.
								drivetype = DRVUNKNOWN;
								CurDriveSet &= ~(1 << drive);
								break;
							case DRIVE_REMOVABLE:	// The drive can be removed from the drive.
								drivetype = DRVREMOVE;
								CurDriveSet &= ~(1 << drive);
								break;
							case DRIVE_CDROM:		// The drive is a CD-ROM drive.
								drivetype = DRVCD;
								CurDriveSet &= ~(1 << drive);
								break;

							// We like these types
							case DRIVE_FIXED:		// The disk cannot be removed from the drive.
								drivetype = DRVFIXED;
								break;
							case DRIVE_REMOTE:		// The drive is a remote (network) drive.
								drivetype = DRVREMOTE;
								break;
							case DRIVE_RAMDISK:		// The drive is a RAM disk.
								drivetype = DRVRAM;
								break;
						}
						_stprintf( name, _T("Drive &%c: (%s)"), 'A'+drive, DrvNames[drivetype] );
						InsertMenu( DriveMenu, 0xFFFFFFFF, MF_BYPOSITION|MF_STRING,
									IDC_DRIVE+drive, name );
					}
				}

				// Add "all drives"/"no drives"
				InsertMenu( DriveMenu, 0xFFFFFFFF, MF_BYPOSITION|MF_SEPARATOR,
									0, NULL );
				InsertMenu( DriveMenu, 0xFFFFFFFF, MF_BYPOSITION|MF_STRING,
									IDC_DRIVE+ALLDRIVES, "All Drives" );
				InsertMenu( DriveMenu, 0xFFFFFFFF, MF_BYPOSITION|MF_STRING,
									IDC_DRIVE+NODRIVES, "No Drives" );

				// Insert into top-level menu
				InsertMenu( GetMenu( hWnd ), 3, MF_BYPOSITION|MF_POPUP|MF_STRING,
							(UINT)DriveMenu, _T("&Drives") );

				// Have driver hook the selected drives
				CurDriveSet = HookDrives( DriveMenu, MaxDriveSet, CurDriveSet );

				if( HookPipes ) {
					fsType = NPFS;
					if( !DeviceIoControl( SysHandle, HookPipes ? IOCTL_FILEMON_HOOKSPECIAL :
												IOCTL_FILEMON_UNHOOKSPECIAL,
										&fsType, sizeof(fsType), NULL, 0, &nb, NULL )) {
				
						MessageBox( hWnd, _T("Filemon could not attach to Named Pipe File System"), 
							APPNAME, MB_ICONWARNING|MB_OK );
						HookPipes = FALSE;
					}
				}
				if( HookSlots ) {
					fsType = MSFS;
					if( !DeviceIoControl( SysHandle, HookSlots ? IOCTL_FILEMON_HOOKSPECIAL :
												IOCTL_FILEMON_UNHOOKSPECIAL,
										&fsType, sizeof(fsType), NULL, 0, &nb, NULL )) {
				
						MessageBox( hWnd, _T("Filemon could not attach to Mail Slot File System"), 
							APPNAME, MB_ICONWARNING|MB_OK );
						HookSlots = FALSE;
					}
				}

				CheckMenuItem( DriveMenu, IDC_DRIVE+PIPE_DRIVE, MF_BYCOMMAND|
						(HookPipes ? MF_CHECKED:MF_UNCHECKED ));
				CheckMenuItem( DriveMenu, IDC_DRIVE+SLOT_DRIVE, MF_BYCOMMAND|
						(HookSlots ? MF_CHECKED:MF_UNCHECKED ));
			}

			// set the time type bitmap
			SendMessage( hWndToolbar, TB_CHANGEBITMAP, IDM_TIME, (TimeIsDuration?10:9) );

			// Start up timer to periodically update screen
			SetTimer( hWnd,	1, 500/*ms*/, NULL );

			// Initialization done
			SetCursor( hSaveCursor );
			ReleaseCapture();
			return 0;

		case WM_NOTIFY:

			// Make sure its intended for us
			if ( wParam == ID_LIST )  {
				NM_LISTVIEW	* pNm = (NM_LISTVIEW *)lParam;
				switch ( pNm->hdr.code )  {

			        case LVN_BEGINLABELEDIT:
						// Don't allow editing of information
						return TRUE;

					case NM_DBLCLK:
					case NM_RETURN:

						// open the specified file/folder in Explorer, if we can
						ExplorerJump( hWnd );
						return TRUE;

					case HDN_ENDTRACK:
						// Listview header sizes changed
						InvalidateRect( hWndList, NULL, TRUE );
						return FALSE;
				}
			} else {

				switch (((LPNMHDR) lParam)->code) 
				{
					case TTN_NEEDTEXT:    
						// Display the ToolTip text.
						lpToolTipText = (LPTOOLTIPTEXT)lParam;
    					LoadString (hInst, (UINT) lpToolTipText->hdr.idFrom, szBuf, sizeof(szBuf)/sizeof(CHAR));
				    	lpToolTipText->lpszText = szBuf;
						break;

					default:
						return FALSE;
				}
			}
			return FALSE;

		case WM_COMMAND:

			switch ( LOWORD( wParam ) )	 {

				// stats related commands to send to driver
				case IDM_CLEAR:
					// Have driver zero information
					if ( ! DeviceIoControl(	SysHandle, IOCTL_FILEMON_ZEROSTATS,
											NULL, 0, NULL, 0, &nb, NULL ) )
					{
						Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
						return 0;
					}
					// Update statistics windows
					UpdateStatistics( hWnd, hWndList, TRUE );
					return 0;

				case IDM_HELP:
					WinHelp(hWnd, _T("filemon.hlp"), HELP_CONTENTS, 0L);
					return 0;

				case IDM_CAPTURE:
					// Disable filtering
					Capture = !Capture;
					if ( ! DeviceIoControl(	SysHandle, 
								Capture ? IOCTL_FILEMON_STARTFILTER: IOCTL_FILEMON_STOPFILTER,
								NULL, 0, NULL, 0, &nb, NULL ) )	{

						Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
						return 0;
					}
					CheckMenuItem( GetMenu(hWnd), IDM_CAPTURE,
									MF_BYCOMMAND|(Capture?MF_CHECKED:MF_UNCHECKED) );
					SendMessage( hWndToolbar, TB_CHANGEBITMAP, IDM_CAPTURE, (Capture?2:1) );
					InvalidateRect( hWndToolbar, NULL, TRUE );
					return 0;

				case IDM_AUTOSCROLL:
					Autoscroll = !Autoscroll;
					CheckMenuItem( GetMenu(hWnd), IDM_AUTOSCROLL,
									MF_BYCOMMAND|(Autoscroll?MF_CHECKED:MF_UNCHECKED) ); 
					SendMessage( hWndToolbar, TB_CHANGEBITMAP, IDM_AUTOSCROLL, (Autoscroll?4:3) );
					InvalidateRect( hWndToolbar, NULL, TRUE );
					return 0;

				case IDM_TIME:
					TimeIsDuration = !TimeIsDuration;
					CheckMenuItem( GetMenu(hWnd), IDM_TIME,
									MF_BYCOMMAND|(TimeIsDuration?MF_CHECKED:MF_UNCHECKED) ); 
					SendMessage( hWndToolbar, TB_CHANGEBITMAP, IDM_TIME, (TimeIsDuration?10:9) );
					InvalidateRect( hWndToolbar, NULL, TRUE );
					EnableMenuItem( GetMenu( hWnd ), IDM_SHOWMS, 
						MF_BYCOMMAND|(!TimeIsDuration?MF_ENABLED:MF_GRAYED));
					return 0;

				case IDM_SHOWMS:
					// Toggle time format
					ShowMs = !ShowMs;
					CheckMenuItem( GetMenu(hWnd), IDM_SHOWMS,
									MF_BYCOMMAND|(ShowMs?MF_CHECKED:MF_UNCHECKED) );
					return FALSE;

				case IDM_COPY:
					CopySelection( hWnd );
					return FALSE;

				case IDM_JUMP:
					// open the folder in regedit if we can
					ExplorerJump( hWnd );
					return FALSE;

				case IDM_HISTORY:
					DialogBox( hInst, TEXT("History"), hWnd, (DLGPROC) HistoryProc );
					return 0;

				case IDM_FILTER:
					DialogBox( hInst, _T("Filter"), hWnd, (DLGPROC) FilterProc );
					return 0;

				case IDM_EXIT:
					// Close ourself
					SendMessage( hWnd, WM_CLOSE, 0, 0 );
					return 0;

				case IDM_STAYTOP:
					OnTop = !OnTop;
					if( OnTop ) SetWindowPos( hWnd, HWND_TOPMOST, 0, 0, 0, 0, 
									SWP_NOMOVE|SWP_NOSIZE );
					else  SetWindowPos( hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, 
									SWP_NOMOVE|SWP_NOSIZE );
					CheckMenuItem( GetMenu(hWnd), IDM_STAYTOP,
									MF_BYCOMMAND|(OnTop?MF_CHECKED:MF_UNCHECKED) ); 
					return 0;

				case IDM_FIND:
					// search the listview
					if( !hWndFind ) {
						PrevMatch = FALSE;
						PopFindDialog( hWnd );
					} else if( PrevMatch ) {

						// treat this like a find-next
						SetCapture(hWndFind);
						hSaveCursor = SetCursor(hHourGlass);
						EnableWindow( hWndFind, FALSE );
						if (FindInListview( hWnd, &FindTextInfo ) ) {
							Autoscroll = FALSE;
							CheckMenuItem( GetMenu(hWnd), IDM_AUTOSCROLL,
											MF_BYCOMMAND|MF_UNCHECKED ); 
							SendMessage( hWndToolbar, TB_CHANGEBITMAP, IDM_AUTOSCROLL, 3 );
						}
						EnableWindow( hWndFind, TRUE );
						SetCursor( hSaveCursor );
						ReleaseCapture(); 
					}
					return 0;

				case IDM_HIGHLIGHT:
					SelectHighlightColors( hWnd );
					break;

				case IDM_FONT:

					hDC = GetDC (hWndList );
					lf = LogFont;
					chf.hDC = CreateCompatibleDC (hDC);
					ReleaseDC (hWnd, hDC);
					chf.lStructSize = sizeof (CHOOSEFONT);
					chf.hwndOwner = hWndList;
					chf.lpLogFont = &lf;
					chf.Flags     = CF_SCREENFONTS|CF_ENABLETEMPLATE|
								CF_INITTOLOGFONTSTRUCT| CF_LIMITSIZE; 
					chf.rgbColors = RGB (0, 0, 0);
					chf.lCustData = 0;
					chf.hInstance = hInst;
					chf.lpszStyle = (LPTSTR)NULL;
					chf.nFontType = SCREEN_FONTTYPE;
					chf.nSizeMin  = 0;
					chf.nSizeMax  = 20;
					chf.lpfnHook  = (LPCFHOOKPROC)(FARPROC)NULL;
					chf.lpTemplateName = (LPTSTR)MAKEINTRESOURCE (FORMATDLGORD31);
					if( ChooseFont( &chf ) ) {
						LogFont = lf;

						// Update listview font
						DeleteObject( hFont );
 						hFont = CreateFontIndirect( &LogFont ); 
						SendMessage( hWndList, WM_SETFONT, 
							(WPARAM) hFont, 0);
						InvalidateRgn( hWndList, NULL, TRUE );

						// Trick the listview into updating
						GetWindowRect( hWndMain, &rc );
						SetWindowPos( hWndMain, NULL,
									rc.left, rc.top, 
									rc.right - rc.left -1 , 
									rc.bottom - rc.top,
									SWP_NOZORDER );
						SetWindowPos( hWndMain, NULL,
									rc.left, rc.top, 
									rc.right - rc.left, 
									rc.bottom - rc.top,
									SWP_NOZORDER );
					}					
					return 0;

				case IDM_ABOUT:
					// Show the names of the authors
					DialogBox( hInst, _T("AboutBox"), hWnd, (DLGPROC)AboutDlgProc );
					return 0;

				case IDM_SAVE:
					SaveFile( hWnd, hWndList, FALSE );
					return 0;

				case IDM_SAVEAS:
					SaveFile( hWnd, hWndList, TRUE );
					return 0;

				default:

					// selection in drive menu?
					if( IsNT ) {

						drive = LOWORD( wParam ) - IDC_DRIVE;
						if ( drive < 32 )  {
							if( drive == ALLDRIVES ) {						
								hookDrive = TRUE;
								newDriveSet = MaxDriveSet;
							} else if( drive == NODRIVES ) {
								hookDrive = FALSE;
								newDriveSet = 0;
							} else {
								// Toggle status
								hookDrive = !(CurDriveSet & (1 << drive));
								newDriveSet = CurDriveSet ^ (1 << drive);
							}
							// Have driver hook the selected drives
							CurDriveSet = HookDrives( DriveMenu, MaxDriveSet, newDriveSet );
							if( newDriveSet != CurDriveSet && hookDrive ) {

								for( drive = 0; drive < 26; drive++ ) {

									if( newDriveSet & (1 << drive ) && 
										!(CurDriveSet & (1 << drive))) {

										sprintf( szBuf, "Filemon could not attach to drive %C:",
											drive + L'A' );
										MessageBox( hWnd, szBuf, 
													APPNAME, MB_ICONWARNING|MB_OK );
									}
								}
							}
							return 0;

						} else if( drive == PIPE_DRIVE ) {

							HookPipes = !HookPipes;
							fsType = NPFS;
							if( !DeviceIoControl( SysHandle, HookPipes ? IOCTL_FILEMON_HOOKSPECIAL :
														IOCTL_FILEMON_UNHOOKSPECIAL,
												&fsType, sizeof(fsType), NULL, 0, &nb, NULL )) {
						
								_stprintf( msgbuf, _T("Filemon could not attach to Named Pipe File System"));
								MessageBox( hWnd, msgbuf, 
									APPNAME, MB_ICONWARNING|MB_OK );
								HookPipes = FALSE;
							}
							CheckMenuItem( DriveMenu, IDC_DRIVE+PIPE_DRIVE, MF_BYCOMMAND|
								(HookPipes ? MF_CHECKED:MF_UNCHECKED ));
							
						} else if( drive == SLOT_DRIVE ) {

							HookSlots = !HookSlots;
							fsType = MSFS;
							if( !DeviceIoControl( SysHandle, HookSlots ? IOCTL_FILEMON_HOOKSPECIAL :
														IOCTL_FILEMON_UNHOOKSPECIAL,
												&fsType, sizeof(fsType), NULL, 0, &nb, NULL )) {
						
								_stprintf( msgbuf, _T("Filemon could not attach to Mail Slot File System"));
								MessageBox( hWnd, msgbuf, 
									APPNAME, MB_ICONWARNING|MB_OK );
								HookSlots = FALSE;
							}
							CheckMenuItem( DriveMenu, IDC_DRIVE+SLOT_DRIVE, MF_BYCOMMAND|
								(HookSlots ? MF_CHECKED:MF_UNCHECKED ));
						} 
					}
				}
			break;

		case WM_TIMER:
			// Time to query the device driver for more data
			if ( Capture )  {

				// don't process for more than a second without pausing
				startTime = GetTickCount();
				for (;;)  {
					// Have driver fill Stats buffer with information
					if ( ! DeviceIoControl(	SysHandle, IOCTL_FILEMON_GETSTATS,
											NULL, 0, &Stats, sizeof Stats,
											&StatsLen, NULL ) )
					{
						Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
						return TRUE;
					}
					if ( StatsLen == 0 )
						break;
					// Update statistics windows
					UpdateStatistics( hWnd, hWndList, FALSE );

					if( GetTickCount() - startTime > 1000 ) break;
				}
			}

			// delete balloon if necessary
			if( hBalloon ) {
				GetCursorPos( &hitPoint );
				GetWindowRect( hWndList, &listRect );
				if( hitPoint.x < listRect.left ||
					hitPoint.x > listRect.right ||
					hitPoint.y < listRect.top ||
					hitPoint.y > listRect.bottom ) {
	
					DestroyWindow( hBalloon );
				}
			}
			return 0;

		case WM_SIZE:
			// Move or resize the List
            MoveWindow( hWndList, 0, TOOLBARHEIGHT, LOWORD(lParam), HIWORD(lParam)-TOOLBARHEIGHT, TRUE );
			MoveWindow( hWndToolbar, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE );
			if( hBalloon ) DestroyWindow( hBalloon );
            return 0;

		case WM_MEASUREITEM:
			// If its the listview that's being queried, return the height of the
			// font
			if( ((MEASUREITEMSTRUCT *) lParam)->CtlType == ODT_LISTVIEW ) {
				hDC = GetDC( hWndList );
				SelectObject( hDC, hFont );
				if( !GetTextMetrics( hDC, &textMetrics)) return FALSE;
				((MEASUREITEMSTRUCT *) lParam)->itemHeight = textMetrics.tmHeight + 1;
				ReleaseDC( hWndList, hDC );
			}
            break;

		case WM_DRAWITEM:
			DrawListViewItem( (LPDRAWITEMSTRUCT) lParam );
			break;

		case WM_ACTIVATE:
		case WM_SETFOCUS:
			SetFocus( hWndList );
			break;

		case WM_MOUSEMOVE:
		case WM_MOVE:
			if( hBalloon ) DestroyWindow( hBalloon );
			return 0;

		case WM_CLOSE:
			
#if _DEBUG
			if( IsNT ) {
				// see if the driver can unload
				if ( ! DeviceIoControl(	SysHandle, IOCTL_FILEMON_UNLOADQUERY,
										NULL, 0, NULL, 0,
										&irpcount, NULL ) ) {
					Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
					return 0;
				}
				if( irpcount ) {

					_stprintf( msgbuf, 	_T("The Filemon device driver cannot unload\n")
										_T("at this time due to oustanding requests.\n\n")
										_T("Do you wish to exit the GUI now?"));
					if( MessageBox( hWnd, msgbuf, APPNAME, MB_ICONSTOP|MB_YESNO ) == IDNO )
						return 0;
				} else {
					if ( ! UnloadDeviceDriver( SYS_NAME ) )  {
						_stprintf( msgbuf, _T("Error unloading \"%s\""), SYS_NAME );
						MessageBox( hWnd, msgbuf, APPNAME, MB_OK );
					}
				}
			} 
#endif
			CloseHandle( SysHandle );
			SavePositionSettings( hWnd );
            break;

		case WM_PAINT:
			if( !IsNT && Deleting ) {
				hDC = BeginPaint( hWnd, &Paint );
				EndPaint( hWnd, &Paint );
				return TRUE;
			}
            break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		default:

			// is it a find-string message?
			if (message == findMessageID ){ 

				// get a pointer to the find structure
				findMessageInfo = (LPFINDREPLACE)lParam;

				// If the FR_DIALOGTERM flag is set, invalidate the find window handle
				if( findMessageInfo->Flags & FR_DIALOGTERM) {
					hWndFind = NULL;
					PrevMatch = FALSE;
				    FindFlags = FindTextInfo.Flags & (FR_DOWN|FR_MATCHCASE|FR_WHOLEWORD);
					return 0;
				}

				// if the FR_FINDNEXT flag is set, go do the search
				if( findMessageInfo->Flags & FR_FINDNEXT ) {
					SetCapture(hWndFind);
					hSaveCursor = SetCursor(hHourGlass);
					EnableWindow( hWndFind, FALSE );
					if( FindInListview( hWnd, findMessageInfo ) ) {
						Autoscroll = FALSE;
						CheckMenuItem( GetMenu(hWnd), IDM_AUTOSCROLL,
										MF_BYCOMMAND|MF_UNCHECKED ); 
						SendMessage( hWndToolbar, TB_CHANGEBITMAP, IDM_AUTOSCROLL, 3 );
					}
					EnableWindow( hWndFind, TRUE );
					ReleaseCapture(); 
					return 0;
				}
				return 0;
			}
	}
    // Default behavior
    return DefWindowProc( hWnd, message, wParam, lParam );
}


/****************************************************************************
*
*    FUNCTION: InitApplication(HANDLE)
*
*    PURPOSE: Initializes window data and registers window class
*
****************************************************************************/
BOOL InitApplication( HINSTANCE hInstance )
{
	WNDCLASS  wc;
	
	// Fill in window class structure with parameters that describe the
	// main (statistics) window. 
	wc.style			= 0;                     
	wc.lpfnWndProc		= (WNDPROC)MainWndProc; 
	wc.cbClsExtra		= 0;              
	wc.cbWndExtra		= 0;              
	wc.hInstance		= hInstance;       
	wc.hIcon			= LoadIcon( hInstance, _T("APPICON") );
	wc.hCursor			= LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground	= (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName		= _T("LISTMENU");  
	wc.lpszClassName	= _T("FileMonClass");
	if ( ! RegisterClass( &wc ) )
		return FALSE;

	wc.lpszMenuName	  = NULL;
 	wc.lpfnWndProc    = (WNDPROC) BalloonDialog;
	wc.hbrBackground  = CreateSolidBrush( 0x00E0FFFF );
	wc.lpszClassName  = _T("BALLOON");
	RegisterClass( &wc );
	
	return TRUE;
}


/****************************************************************************
*
*    FUNCTION:  InitInstance(HANDLE, int)
*
*    PURPOSE:  Saves instance handle and creates main window
*
****************************************************************************/
HWND InitInstance( HINSTANCE hInstance, int nCmdShow )
{
	// get the window position settings from the registry
	GetPositionSettings();

	// create the main window
	hInst = hInstance;
	hWndMain = CreateWindow( _T("FileMonClass"), 
							_T("File Monitor - Sysinternals: www.sysinternals.com"), 
							WS_OVERLAPPEDWINDOW,
							PositionInfo.left, PositionInfo.top, 
							PositionInfo.width, PositionInfo.height,
							NULL, NULL, hInstance, NULL );

	// if window could not be created, return "failure" 
	if ( ! hWndMain ) {

		return NULL;
    }
	
	// make the window visible; update its client area; and return "success"
	ShowWindow( hWndMain, nCmdShow );

	// maximize it if necessary
	if( PositionInfo.maximized ) {

		ShowWindow( hWndMain, SW_SHOWMAXIMIZED );
	}

	UpdateWindow( hWndMain ); 
	if( OnTop ) {
		
		SetWindowPos( hWndMain, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE );
		CheckMenuItem( GetMenu(hWndMain), IDM_STAYTOP,
						MF_BYCOMMAND|(OnTop?MF_CHECKED:MF_UNCHECKED) );
	}
	return hWndMain;      
}


/****************************************************************************
*
*	FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)
*
*	PURPOSE:	calls initialization function, processes message loop
*
****************************************************************************/
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
						LPSTR lpCmdLine, int nCmdShow )
{
	MSG 	msg;      
	HWND	hWnd;
	HACCEL	hAccel;
 	DWORD	NTVersion;
        
	if ( ! InitApplication( hInstance ) )
		return FALSE;   
	
	// get NT version
	NTVersion = GetVersion();
	if( NTVersion >= 0x80000000 ) {

		IsNT = FALSE;
	} else {

		IsNT = TRUE;
	}

	// initializations that apply to a specific instance 
	if ( (hWnd = InitInstance( hInstance, nCmdShow )) == NULL )
		return FALSE;

	// load accelerators
	hAccel = LoadAccelerators( hInstance, _T("ACCELERATORS"));

	// register for the find window message
    findMessageID = RegisterWindowMessage( FINDMSGSTRING );

	// acquire and dispatch messages until a WM_QUIT message is received.
	while ( GetMessage( &msg, NULL, 0, 0 ) )  {
		if( !TranslateAccelerator( hWnd, hAccel, &msg ) &&
			(!hWndFind || !IsWindow(hWndFind) || !IsDialogMessage( hWndFind, &msg ))) {
			TranslateMessage( &msg );
			DispatchMessage( &msg ); 
		}
	}
	return 0;
}
