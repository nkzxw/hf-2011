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
#include "listview.h"
#include "settings.h"
#include "filter.h"


// font
HFONT				hFont;
LOGFONT				LogFont;

// Windows
HWND				hBalloon = NULL;
HWND				hWndFind = NULL;

// For info saving
TCHAR				szFileName[MAX_PATH];
BOOLEAN				FileChosen = FALSE;


// Search string
TCHAR				FindString[MAXITEMLENGTH];
FINDREPLACE			FindTextInfo;
DWORD				FindFlags = FR_DOWN;
BOOLEAN				PrevMatch;
TCHAR				PrevMatchString[MAXITEMLENGTH];	

// Listview
WNDPROC 			ListViewWinMain;


// General cursor manipulation
HCURSOR 			hSaveCursor;
HCURSOR 			hHourGlass;

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
	TCHAR	errmsg[256];
	BOOLEAN	goUp;

	// get the search direction
	goUp = ((FindInfo->Flags & FR_DOWN) == FR_DOWN);

	// initialize stuff
	if( !(numItems = ListView_GetItemCount( hWndList ))) {

		MessageBox( hWnd, TEXT("No items to search"), TEXT(APPNAME), 
			MB_OK|MB_ICONWARNING );
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
			return FALSE;
		}
	}

	// loop through each item looking for the string
	while( 1 ) {

		// get the item text
		for( subitem = 0; subitem < NUMCOLUMNS; subitem++ ) {
			fieldtext[0] = 0;
			ListView_GetItemText( hWndList, currentItem, subitem, fieldtext, MAXITEMLENGTH );

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
	strcpy( FindString, PrevMatchString );
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
		MessageBox( hWnd, TEXT("Unable to create Find dialog"), TEXT(APPNAME), MB_OK|MB_ICONERROR );      
}



/******************************************************************************
*
*	FUNCTION:	DeleteSelection
*
*	PURPOSE:	The user wants to delete the lines from the listview.
*
*****************************************************************************/
void DeleteSelection( HWND hWnd )
{
	int		currentItem;
	
	while( (currentItem = ListView_GetNextItem( hWndList, -1, LVNI_SELECTED )) != -1 ) {

		ListView_DeleteItem( hWndList, currentItem );
	} 	

	currentItem = ListView_GetNextItem( hWndList, -1, LVNI_FOCUSED );
	if( currentItem != -1 ) {

		ListView_SetItemState( hWndList, currentItem, LVIS_SELECTED, LVIS_SELECTED );
	}
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
	char			szFile[MAX_PATH] = "", fieldtext[MAXITEMLENGTH];
	char			output[MAXITEMLENGTH*2];
	FILE			*hFile;
	int				numitems;
	int				row, subitem;

	if( SaveAs || !FileChosen ) {
		strcpy( szFile, APPNAME );
		SaveFileName.lStructSize       = sizeof (SaveFileName);
		SaveFileName.hwndOwner         = hWnd;
		SaveFileName.hInstance         = hInst;
		SaveFileName.lpstrFilter       = APPNAME" Data (*.LOG)\0*.LOG\0All (*.*)\0*.*\0";
		SaveFileName.lpstrCustomFilter = (LPTSTR)NULL;
		SaveFileName.nMaxCustFilter    = 0L;
		SaveFileName.nFilterIndex      = 1L;
		SaveFileName.lpstrFile         = szFile;
		SaveFileName.nMaxFile          = MAX_PATH;
		SaveFileName.lpstrFileTitle    = NULL;
		SaveFileName.nMaxFileTitle     = 0;
		SaveFileName.lpstrInitialDir   = NULL;
		SaveFileName.lpstrTitle        = "Save "APPNAME" Output to File...";
		SaveFileName.nFileOffset       = 0;
		SaveFileName.nFileExtension    = 0;
		SaveFileName.lpstrDefExt       = "*.log";
		SaveFileName.lpfnHook		   = NULL;
 		SaveFileName.Flags = OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

		if( !GetSaveFileName( &SaveFileName )) 
			return;
	} else 
		// open previous szFile
		strcpy( szFile, szFileName );

	// open the file
	hFile = fopen( szFile, "w" );
	if( !hFile ) {
		MessageBox(	NULL, "Create File Failed.",
				"Save Error", MB_OK|MB_ICONSTOP );
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
			ListView_GetItemText( ListBox, row, subitem, fieldtext, MAXITEMLENGTH );
			strcat( output, fieldtext );
			strcat( output, "\t" );
		}
		fprintf( hFile, "%s\n", output );
	}
	fclose( hFile );
	strcpy( szFileName, szFile );
	FileChosen = TRUE;
	SetCursor( hSaveCursor );
	ReleaseCapture(); 
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
			hitPoint.y > bottomrightPoint.y ) {

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
	hWndList = CreateWindowEx(  0, WC_LISTVIEW, TEXT(""), 
								WS_EX_CLIENTEDGE | WS_VISIBLE | WS_CHILD | LVS_NOSORTHEADER |
								LVS_REPORT | LVS_OWNERDRAWFIXED | WS_TABSTOP | WS_BORDER, 
								0, 0, 0, 0, 
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

