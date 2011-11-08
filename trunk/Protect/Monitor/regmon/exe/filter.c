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
#include "driver.h"

// Highlight colors
DWORD				HighlightFg;
DWORD				HighlightBg;

// Filter strings
TCHAR				FilterString[MAXFILTERLEN];
TCHAR				ExcludeString[MAXFILTERLEN];
TCHAR				HighlightString[MAXFILTERLEN];

// Filter-related
FILTER				FilterDefinition;

// Recent filters
char				RecentInFilters[NUMRECENTFILTERS][MAXFILTERLEN];
char				RecentExFilters[NUMRECENTFILTERS][MAXFILTERLEN];
char				RecentHiFilters[NUMRECENTFILTERS][MAXFILTERLEN];

// listview size limiting
DWORD				MaxLines = 0;
DWORD				LastRow = 0;



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
	char		*filterPtr;
	char		curFilterBuf[MAXFILTERLEN];
	char		curMatchTest[MAXFILTERLEN];
	char		*curFilter, *endFilter;

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
		CheckDlgButton( hDlg, IDC_SUCCESS, 
			FilterDefinition.logsuccess ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton( hDlg, IDC_ERROR, 
			FilterDefinition.logerror ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton( hDlg, IDC_LOGAUX, 
			FilterDefinition.logaux ? BST_CHECKED : BST_UNCHECKED );
		return TRUE;

	case WM_COMMAND:              
		strcpy( oldHighlight, HighlightString );
		if ( LOWORD( wParam ) == IDOK )	 {

			// make sure that max lines is legal
			GetDlgItemTextA( hDlg, IDC_FILTERSTRING, newFilter, MAXFILTERLEN );
			GetDlgItemTextA( hDlg, IDC_EXFILTERSTRING, newExFilter, MAXFILTERLEN );
			GetDlgItemTextA( hDlg, IDC_HIFILTERSTRING, newHiFilter, MAXFILTERLEN );
			if( !newFilter[0] )	  strcpy( newFilter, " " );
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
			FilterDefinition.logsuccess = 
					(IsDlgButtonChecked( hDlg, IDC_SUCCESS ) == BST_CHECKED);
			FilterDefinition.logerror = 
					(IsDlgButtonChecked( hDlg, IDC_ERROR ) == BST_CHECKED);
			FilterDefinition.logaux = 
					(IsDlgButtonChecked( hDlg, IDC_LOGAUX ) == BST_CHECKED);
			
			EndDialog( hDlg, TRUE );

			// Apply filters 
			FilterDefinition.excludefilter[0] = 0;
			FilterDefinition.includefilter[0] = 0;
			if( strcmp( ExcludeString, " " ) )
				strcpy( FilterDefinition.excludefilter, ExcludeString );
			if( strcmp( FilterString, " " ) )
				strcpy( FilterDefinition.includefilter, FilterString );
			if ( ! DeviceIoControl(	SysHandle, IOCTL_REGMON_SETFILTER,
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
			CheckDlgButton( hDlg, IDC_SUCCESS, BST_CHECKED );
			CheckDlgButton( hDlg, IDC_ERROR, BST_CHECKED );
			CheckDlgButton( hDlg, IDC_LOGAUX, BST_CHECKED );

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
