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
#include "about.h"
#include "filter.h"
#include "settings.h"


// Version information function
HRESULT (CALLBACK *pDllGetVersionProc)( PDLLVERSIONINFO_ pdvi );


// button definitions

// for installations that support flat style
TBBUTTON tbButtons[] = {
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 0, IDM_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 8, 0, 0, TBSTYLE_BUTTON, 0L, 0},
	{ 2, IDM_CAPTURE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 4, IDM_AUTOSCROLL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 6, IDM_CLEAR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 10, IDM_TIME, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 8, 0, 0, TBSTYLE_BUTTON, 0L, 0},
	{ 5, IDM_FILTER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 12, IDM_HISTORY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 8, 0, 0, TBSTYLE_BUTTON, 0L, 0},
	{ 7, IDM_FIND, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 9, IDM_JUMP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 8, 0, 0, TBSTYLE_BUTTON, 0L, 0},
};
#define NUMBUTTONS		13

// for older installations
TBBUTTON tbButtonsOld[] = {
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 0, IDM_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 2, IDM_CAPTURE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 4, IDM_AUTOSCROLL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 6, IDM_CLEAR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},	
	{ 10, IDM_TIME, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 5, IDM_FILTER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 12, IDM_HISTORY, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
	{ 7, IDM_FIND, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
	{ 9, IDM_JUMP, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
};						 
#define NUMBUTTONSOLD	12

// Buffer into which driver can copy statistics
char				Stats[ LOGBUFSIZE ];
// Current fraction of buffer filled
DWORD				StatsLen;


// Application instance handle
HINSTANCE			hInst;



// Windows NT or Windows 9x?
BOOLEAN				IsNT;

// Misc globals
HWND				hWndMain;
UINT				findMessageID;
HWND				hWndList;
HWND				hWndToolbar;

BOOLEAN				Capture = TRUE;
BOOLEAN				Autoscroll = TRUE;
BOOLEAN				BootLog = FALSE;
BOOLEAN				BootLogMenuUsed = FALSE;
BOOLEAN				Deleting = FALSE;



// General buffer for storing temporary strings
static TCHAR		msgbuf[ MAXITEMLENGTH ];


// performance counter frequency
LARGE_INTEGER		PerfFrequency;


/******************************************************************************
*
*	FUNCTION:	RegeditJump
*
*	PURPOSE:	Opens Regedit and navigates the desired key
*
*****************************************************************************/
void RegeditJump( HWND hWnd )
{
	int		currentItem;
	int		pos;
	char	* ch;
	HWND	regeditHwnd, regeditMainHwnd;
	char	compressPath[MAX_PATH];
	HKEY	hKey;
	char	*value = NULL;
	DWORD	status;
	char	RegPath[MAX_PATH];
	DEVMODE	devMode;

	// Get the color depth, which can effect the speed that Regedit
	// responds to keyboard commands
	devMode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &devMode );

	// See if we can get a Registry path out of the listview
	// find the item with the focus
	currentItem = ListView_GetNextItem( hWndList, -1, LVNI_SELECTED );

	if( currentItem == -1 ) {

		MessageBox( hWnd, "No item selected.", APPNAME, MB_OK|MB_ICONWARNING );
		return;
	}
	memset( compressPath, 0, MAX_PATH );
	ListView_GetItemText( hWndList, currentItem, 4, compressPath, MAX_PATH );

	// If the key is a handle reference, tell the user we're sorry
	if( compressPath[0] == '0' ) {

		MessageBox( hWnd, "The full name of the selected key or value is not available.",
			APPNAME, MB_OK|MB_ICONWARNING );
		return;
	}
	
	// We need to uncompress abbreviations
	if( !strncmp( compressPath, "HKLM", strlen("HKLM"))) {

		sprintf( RegPath, "\\HKEY_LOCAL_MACHINE%s", &compressPath[strlen("HKLM")] );
		status = RegOpenKey( HKEY_LOCAL_MACHINE, &compressPath[strlen("HKLM")+1], &hKey );

	} else if( !strncmp( compressPath, "HKCU", strlen("HKCU"))) {

		sprintf( RegPath, "\\HKEY_CURRENT_USER%s", &compressPath[strlen("HKCU")] );
		status = RegOpenKey( HKEY_CURRENT_USER, &compressPath[strlen("HKCU")+1], &hKey );

	} else if( !strncmp( compressPath, "HKCC", strlen("HKCC"))) {

		sprintf( RegPath, "\\HKEY_CURRENT_CONFIG%s", &compressPath[strlen("HKCC")] );
		status = RegOpenKey( HKEY_CURRENT_CONFIG, &compressPath[strlen("HKCC")+1], &hKey );

	} else if( !strncmp( compressPath, "HKCR", strlen("HKCR"))) {

		sprintf( RegPath, "\\HKEY_CLASSES_ROOT%s", &compressPath[strlen("HKCR")] );
		status = RegOpenKey( HKEY_CLASSES_ROOT, &compressPath[strlen("HKCR")+1], &hKey );

	} else if( !strncmp( compressPath, "HKU", strlen("HKU"))) {

		sprintf( RegPath, "\\HKEY_USERS%s", &compressPath[strlen("HKU")] );
		status = RegOpenKey( HKEY_USERS, &compressPath[strlen("HKU")+1], &hKey );

	} else {

		strcpy( RegPath, compressPath );
		status = FALSE;
	}

	// Is it a value or a key?
	if( status == ERROR_SUCCESS ) {
		
		RegCloseKey( hKey );
		strcat( RegPath, "\\" );

	} else {

		value = strrchr( RegPath, '\\')+1;
		*strrchr( RegPath, '\\' ) = 0;
	}

	// Open RegEdit
	regeditMainHwnd = FindWindow( "RegEdit_RegEdit", NULL );
	if ( regeditMainHwnd == NULL )  {
		SHELLEXECUTEINFO info;
		memset( &info, 0, sizeof info );
		info.cbSize = sizeof info;
		info.fMask	= SEE_MASK_NOCLOSEPROCESS; 
		info.lpVerb	= "open"; 
		info.lpFile	= "regedit.exe"; 
		info.nShow	= SW_SHOWNORMAL; 
		ShellExecuteEx( &info );
		WaitForInputIdle( info.hProcess, INFINITE );
		regeditMainHwnd = FindWindow( "RegEdit_RegEdit", NULL );
	} 

	if( regeditMainHwnd == NULL ) {

		MessageBox( hWnd, "Regmon was unable to launch Regedit.",
			APPNAME, MB_OK|MB_ICONERROR );
		return;
	}
    ShowWindow( regeditMainHwnd, SW_SHOW );
	SetForegroundWindow( regeditMainHwnd );

	// Get treeview
	regeditHwnd = FindWindowEx( regeditMainHwnd, NULL, "SysTreeView32", NULL );
	SetForegroundWindow( regeditHwnd );
	SetFocus( regeditHwnd );

	// Close it up
	for ( pos = 0; pos < 30; ++pos )  {
		UINT vk = VK_LEFT;
		SendMessage( regeditHwnd, WM_KEYDOWN, vk, 0 );
	}

	// wait for slow displays - 
	// Regedit takes a while to open keys with lots of subkeys
	// when running at high color depths 
	if( devMode.dmBitsPerPel > 8 ) Sleep(REGEDITSLOWWAIT); 

	// Open path
	for ( ch = RegPath; *ch; ++ch )  {
		if ( *ch == '\\' )  {
			UINT vk = VK_RIGHT;
			SendMessage( regeditHwnd, WM_KEYDOWN, vk, 0 );

			// wait for slow displays - 
			// Regedit takes a while to open keys with lots of subkeys
			// when running at high color depths 
			if( devMode.dmBitsPerPel > 8 ) Sleep(REGEDITSLOWWAIT); 

		} else {
			UINT vk = toupper(*ch);

			SendMessage( regeditHwnd, WM_CHAR, vk, 0 );
		}
	}

	// If its a value select the value
	if( value ) {
		UINT vk = VK_HOME;

		regeditHwnd = FindWindowEx( regeditMainHwnd, NULL, "SysListView32", NULL );
		SetForegroundWindow( regeditHwnd );
		SetFocus( regeditHwnd );
		Sleep(1000); // have to wait for Regedit to update listview
		SendMessage( regeditHwnd, WM_KEYDOWN, vk, 0 );

		for ( ch = value; *ch; ++ch )  {
			UINT vk = toupper(*ch);

			SendMessage( regeditHwnd, WM_CHAR, vk, 0 );
		}
	}

	SetForegroundWindow( regeditMainHwnd );
	SetFocus( regeditMainHwnd );
}


/******************************************************************************
*
*	FUNCTION:	Abort:
*
*	PURPOSE:	Handles emergency exit conditions.
*
*****************************************************************************/
LONG Abort( HWND hWnd, TCHAR * Msg, DWORD Error )
{
	LPVOID	lpMsgBuf;
	TCHAR	errmsg[MAX_PATH];
	DWORD	error = GetLastError();
 
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					NULL, Error, 
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &lpMsgBuf, 0, NULL );
	if( IsNT ) UnloadDeviceDriver( SYS_NAME );
	sprintf(errmsg, _T("%s: %s"), Msg, lpMsgBuf );
	if( (Error == ERROR_INVALID_HANDLE || Error == ERROR_ACCESS_DENIED ||
		 Error == ERROR_FILE_NOT_FOUND) && IsNT ) 
		wsprintf(errmsg, _T("%s\nMake sure that you are an administrator, that Regmon is")
			_T("not already running, and that Regmon is located on a local drive."), errmsg  );
	MessageBox( hWnd, errmsg, _T("Regmon"), MB_OK|MB_ICONERROR );
	PostQuitMessage( 1 );
	LocalFree( lpMsgBuf );
	return (DWORD) -1;
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
BOOL List_Append( HWND hWndList, DWORD seq, 
				 LONGLONG time, LONGLONG perftime,
				 char * line )
{
	LV_ITEM		lvI;	// list view item structure
	int			row;
	char		*items[NUMCOLUMNS];
	int			itemcnt = 0;
	float		elapsed;
	char		timeBuf[64], timeSub[64];
	int			msIndex = 0;
	int			i;
	char		*secondsPtr;
	FILETIME	localTime;
	SYSTEMTIME	systemTime;

	// Split line into columns, chopping off win32 pids if necessary
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
		if ( row == -1 ) {
			// No request associated with status.
			return FALSE;
		}
	}

	// Sequence number if a new item
	if ( *items[0] )  {
		sprintf( msgbuf, TEXT("%d"), seq );
		lvI.mask		= LVIF_TEXT | LVIF_PARAM;
		lvI.iItem		= row;
		lvI.iSubItem	= 0;
		lvI.pszText		= msgbuf;
		lvI.cchTextMax	= lstrlen( lvI.pszText ) + 1;
		lvI.lParam		= seq;
		row = ListView_InsertItem( hWndList, &lvI );
		if ( row == -1 )  {
			sprintf( msgbuf, TEXT("Error adding item %d to list view"), seq );
			MessageBox( hWndList, msgbuf, TEXT(APPNAME" Error"), MB_OK );
			return FALSE;
		}
        LastRow = row;
	}

	// format timestamp according to user preference
	if( ClockTime ) {

        if( IsNT ) {
            FileTimeToLocalFileTime( (PFILETIME) &time, &localTime );
            FileTimeToSystemTime( &localTime, &systemTime );
        } else {
            DosDateTimeToFileTime( (WORD) (time >> 48), (WORD) (time >> 32), &localTime );
            FileTimeToSystemTime( &localTime, &systemTime );
			systemTime.wSecond += ((WORD) time) / 1000;
			systemTime.wMilliseconds = ((WORD) time) % 1000;
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

	} else {

		// convert to elapsed microseconds since start of regmon or last
		// gui clear
        if( IsNT ) {
            elapsed = ((float) perftime)/(float)PerfFrequency.QuadPart;
            sprintf( msgbuf, "%10.8f", elapsed );
        } else {
			sprintf( msgbuf, "%10.8f", perftime * 0.8 / 1e6);
        }
	}
	ListView_SetItemText( hWndList, row, 1, msgbuf );

	// Fill the columns
	for( i = 0; i < 5; i++ ) {

		if ( itemcnt>i && *items[i] ) {

			ListView_SetItemText( hWndList, row, i+2, items[i] );
		}
	} 
	return TRUE;
}


/******************************************************************************
*
*	FUNCTION:	UpdateStatistics
*
*	PURPOSE:	Clear the statistics window and refill it with the current 
*				contents of the statistics buffer.  
*
******************************************************************************/
void UpdateStatistics( HWND hWnd, HWND hWndList, PCHAR Buffer, DWORD BufLen, BOOL Clear )
{
	PENTRY	ptr;
	BOOL	itemsAdded = FALSE;
    size_t  len;

	BOOLEAN isfirst = TRUE;
	BOOLEAN Frag = FALSE;
	static char lasttext[4096];

	// Just return if nothing to do
	if ( !Clear  &&  BufLen < sizeof(int)+2 )
		return;

	// Do different stuff on NT than Win9x
	if( IsNT ) {

		for ( ptr = (void *)Buffer; (char *)ptr < min(Buffer+BufLen,Buffer + LOGBUFSIZE); )  {

			len = strlen(ptr->text);
			len += 4; len &= 0xFFFFFFFC; // +1 for null-terminator +3 for 32bit alignment

			// truncate if necessary
			if( len > MAXITEMLENGTH - 1 ) ptr->text[MAXITEMLENGTH-1] = 0;
			isfirst = FALSE;

			itemsAdded |= List_Append( hWndList, ptr->seq, ptr->time.QuadPart, 
									   ptr->perftime.QuadPart, ptr->text );

			ptr = (void *)(ptr->text + len);
		}

	} else {

		for ( ptr = (void *)Buffer; (char *)ptr < min(Buffer+BufLen,Buffer + LOGBUFSIZE); )  {
			// Add to list
			len = strlen(ptr->text);

			// truncate if necessary
			if( len > MAXITEMLENGTH - 1 ) ptr->text[MAXITEMLENGTH-1] = 0;

			itemsAdded |= List_Append( hWndList, ptr->seq, ptr->time.QuadPart, 
									   ptr->perftime.QuadPart, ptr->text );
			ptr = (void *)(ptr->text + len + 1);
		}
	}

	// Limit number of lines saved
	if (MaxLines && itemsAdded ) {
		SendMessage(hWndList, WM_SETREDRAW, FALSE, 0);
		while ( LastRow >= MaxLines ) {
			ListView_DeleteItem ( hWndList, 0 );
		    LastRow--;
		}
		SendMessage(hWndList, WM_SETREDRAW, TRUE, 0);
    }

	// Scroll so newly added items are visible
	if ( Autoscroll && itemsAdded ) {
		if( hBalloon ) DestroyWindow( hBalloon );
		ListView_EnsureVisible( hWndList, ListView_GetItemCount(hWndList)-1, FALSE ); 
	}

	// Start with empty list
	if ( Clear ) {

		if( !IsNT ) {
			hSaveCursor = SetCursor(hHourGlass);
			SendMessage(hWndList, WM_SETREDRAW, FALSE, 0);
		} 

		ListView_DeleteAllItems( hWndList );
		LastRow = 0;

		if( !IsNT ) {
			SetCursor( hSaveCursor );
			SendMessage(hWndList, WM_SETREDRAW, TRUE, 0);
			InvalidateRect( hWndList, NULL, FALSE );
		}
	}
}




/******************************************************************************
*
*	FUNCTION:	GetDLLVersion
*
*	PURPOSE:	Gets the version number of the specified DLL.
*
******************************************************************************/
HRESULT GetDLLVersion( PCHAR DllName, LPDWORD pdwMajor, LPDWORD pdwMinor)
{
	HINSTANCE			hDll;
	HRESULT				hr = S_OK;
	DLLVERSIONINFO_		dvi;

	*pdwMajor = 0;
	*pdwMinor = 0;

	//Load the DLL.
	hDll = LoadLibrary(DllName);

	if( hDll ) {

	   pDllGetVersionProc = (PVOID)GetProcAddress(hDll, _T("DllGetVersion"));

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
	static TCHAR	Path[ MAX_PATH ];
	static BOOLEAN	isWnet;
	static TCHAR	group[] = "System Bus Extender";
	static TCHAR	driverPath[ MAX_PATH ];
	static TCHAR	szBuf[128];
	static TCHAR	logFile[ MAX_PATH ];
	DWORD			nb, versionNumber;
	DWORD			length, type, tag, driverStart;
	LPTOOLTIPTEXT	lpToolTipText;
	TCHAR			systemRoot[ MAX_PATH ];
	HKEY			hDriverKey;
	LPFINDREPLACE	findMessageInfo;
	DWORD			error, majorver, minorver;
	TCHAR			*File;
	WIN32_FIND_DATA findData;
	HANDLE			findHandle;
	DWORD			startTime;
	RECT			rc;
	POINT			hitPoint;
	LOGFONT			lf;
	CHOOSEFONT		chf;
	TEXTMETRIC		textMetrics;
	RECT			listRect;
	HDC				hDC;
	PAINTSTRUCT		Paint;
	MENUITEMINFO	bootMenuItem;

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
		GetDLLVersion( "comctl32.dll", &majorver, &minorver );
		if( majorver > 4 || (majorver == 4 && minorver >= 70) ) {
			hWndToolbar = CreateToolbarEx( 
				hWnd, TOOLBAR_FLAT | WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,  
				ID_TOOLBAR, NUMTOOLBUTTONS, hInst, IDB_TOOLBAR, (LPCTBBUTTON)&tbButtons,
				NUMBUTTONS, 16,16,16,15, sizeof(TBBUTTON)); 
		} else {
			hWndToolbar = CreateToolbarEx( 
				hWnd, WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,  
				ID_TOOLBAR, NUMTOOLBUTTONS, hInst, IDB_TOOLBAR, (LPCTBBUTTON)&tbButtonsOld,
				NUMBUTTONSOLD, 16,16,16,15, sizeof(TBBUTTON)); 
		}
		if (hWndToolbar == NULL )
			MessageBox (NULL, _T("Toolbar not created!"), NULL, MB_OK );

		// Create the ListBox within the main window
		hWndList = CreateList( hWnd );
		if ( hWndList == NULL )
			MessageBox( NULL, _T("List not created!"), NULL, MB_OK );

		// open the handle to the device
		if( IsNT ) {

			// See if this version implements the kernel Registry callback API
			isWnet = (BOOLEAN) ((DWORD)(HIWORD(GetVersion()) & ~0x8000) >= WNET_BUILD_NUMBER);

			// Add the log boot menu item
			bootMenuItem.cbSize = sizeof( bootMenuItem );
			bootMenuItem.fMask = MIIM_TYPE;
			bootMenuItem.fType = MFT_SEPARATOR;
			InsertMenuItem( GetSubMenu( GetMenu(hWnd), 2 ), 
							GetMenuItemCount( GetSubMenu( GetMenu(hWnd), 2 )),
							TRUE, &bootMenuItem );
			bootMenuItem.fMask = MIIM_TYPE|MIIM_ID;
			bootMenuItem.fType = MFT_STRING;
			bootMenuItem.wID = IDM_BOOTLOG;
			bootMenuItem.dwTypeData = (PCHAR) "Log &Boot";
			InsertMenuItem( GetSubMenu( GetMenu(hWnd), 2 ), 
							GetMenuItemCount( GetSubMenu( GetMenu(hWnd), 2 )),
							TRUE, &bootMenuItem );

			GetCurrentDirectory( sizeof Path, Path );
			sprintf( Path+lstrlen(Path), _T("\\%s"), isWnet ? SYS_FILE_WNET : SYS_FILE );

			findHandle = FindFirstFile( Path, &findData );
			if( findHandle == INVALID_HANDLE_VALUE ) {

				if( !SearchPath( NULL, isWnet ? SYS_FILE_WNET : SYS_FILE, NULL, sizeof(Path), Path, &File ) ) {

					sprintf( msgbuf, _T("%s was not found."), isWnet ? SYS_FILE_WNET : SYS_FILE );
					return Abort( hWnd, msgbuf, GetLastError() );
				}

			} else FindClose( findHandle );

			// read driver start type to see if boot-logging is enabled
			driverStart = SERVICE_DEMAND_START;
			if( RegOpenKey( HKEY_LOCAL_MACHINE, REGMON_DRIVER_KEY, &hDriverKey ) == 
				ERROR_SUCCESS ) {

				length = sizeof( driverStart );
				RegQueryValueEx( hDriverKey, "Start", NULL, &type,
					(PBYTE) &driverStart, &length);
				RegCloseKey( hDriverKey );
			} 
			BootLog = (driverStart != SERVICE_DEMAND_START);

			// check boot logging menu item if boot start
			CheckMenuItem( GetMenu(hWnd), IDM_BOOTLOG,
					MF_BYCOMMAND|(BootLog?MF_CHECKED:MF_UNCHECKED) ); 

			// copy the driver to <winnt>\system32\drivers so that we can do
			// boot-time monitoring with the flip of a bit
			// get the system root
			if( !GetEnvironmentVariable( "SYSTEMROOT", systemRoot, sizeof(systemRoot))) {

				strcpy( msgbuf, _T("Could not resolve SYSTEMROOT environment variable") );
				return Abort( hWnd, msgbuf, GetLastError() );
			}
			sprintf( logFile, _T("%s\\REGMON.LOG"), systemRoot );
			sprintf( driverPath, _T("%s\\system32\\drivers\\%s"), 
							systemRoot, isWnet ? SYS_FILE_WNET : SYS_FILE );
			if( !LoadDeviceDriver( SYS_NAME, driverPath, &SysHandle, &error )) {

				if( !CopyFile( Path, driverPath, FALSE )) {

					sprintf( msgbuf, _T("Unable to copy %s to %s\n\n")
						_T("Make sure that %s is in the current directory."), 
						SYS_NAME, driverPath, isWnet ? SYS_FILE_WNET : SYS_FILE );
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
			}

			// We can delete the driver and its Registry key now that its loaded
			if( RegCreateKey( HKEY_LOCAL_MACHINE, REGMON_DRIVER_KEY, &hDriverKey ) == 
				ERROR_SUCCESS ) {

				RegDeleteKey( hDriverKey, "ENUM" );
				RegDeleteKey( hDriverKey, "SECURITY");
				RegCloseKey( hDriverKey );
				RegDeleteKey( HKEY_LOCAL_MACHINE, REGMON_DRIVER_KEY );
				DeleteFile( driverPath );
			}

			// Correct driver version?
			if( ! DeviceIoControl(	SysHandle, IOCTL_REGMON_VERSION,
									NULL, 0, &versionNumber, sizeof(DWORD), &nb, NULL ) ||
					versionNumber != REGMON_VERSION ) {

				MessageBox( hWnd, _T("Regmon located a driver with the wrong version.\n")
					_T("\nIf you just installed a new version you must reboot before you are\n")
					_T("able to use it."), APPNAME, MB_ICONERROR);
				return -1;
			}

		} else {

			// Win9x
			SysHandle = CreateFile( VXD_FILE, 0, 0, NULL,
							0, FILE_FLAG_OVERLAPPED|
							FILE_FLAG_DELETE_ON_CLOSE,
							NULL );
			if ( SysHandle == INVALID_HANDLE_VALUE )  {
				wsprintf( msgbuf, "%s is not loaded properly.", VXD_NAME );
				Abort( hWnd, msgbuf, GetLastError() );
				return FALSE;
			}
		}

		// Set the filter
		FilterDefinition.excludefilter[0] = 0;
		FilterDefinition.includefilter[0] = 0;
		if( strcmp( ExcludeString, " " ) )
			strcpy( FilterDefinition.excludefilter, ExcludeString );
		if( strcmp( FilterString, " " ) )
			strcpy( FilterDefinition.includefilter, FilterString );

		// See if they want to keep any filters
		if( strcmp( FilterString, "*" ) ||
			(*ExcludeString && strcmp( ExcludeString, " "))) {

			DialogBox( hInst, TEXT("InitFilter"), hWnd, (DLGPROC) FilterProc );
		} else {

			if ( ! DeviceIoControl(	SysHandle, IOCTL_REGMON_SETFILTER,
									(PVOID) &FilterDefinition, sizeof(FilterDefinition), 
									NULL, 0, &nb, NULL ) )	{

				return Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
			}
		}

		// Have driver zero information
		if ( ! DeviceIoControl(	SysHandle, IOCTL_REGMON_ZEROSTATS,
								NULL, 0, NULL, 0, &nb, NULL ) ) {

			return Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
		}

		// Start up timer to periodically update screen
		SetTimer( hWnd,	1, 500/*ms*/, NULL );

		// Have driver turn on hooks
		if ( ! DeviceIoControl(	SysHandle, IOCTL_REGMON_HOOK,
								NULL, 0, NULL, 0, &nb, NULL ) )	{

			return Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
		}
		
		// Initialization done
		SetCursor( hSaveCursor );
		ReleaseCapture();
		return FALSE;

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

				// open the specified key in Regedit, if we can
				RegeditJump( hWnd );
				return TRUE;

			case LVN_ITEMCHANGED:

				if( ListView_GetNextItem( hWndList, -1, LVNI_SELECTED ) == -1 ) {

					EnableMenuItem( GetMenu( hWndMain ), IDM_DELETE, MF_BYCOMMAND|MF_GRAYED ); 
					EnableMenuItem( GetMenu( hWndMain ), IDM_COPY, MF_BYCOMMAND|MF_GRAYED );
				} else {

					EnableMenuItem( GetMenu( hWndMain ), IDM_DELETE, MF_BYCOMMAND|MF_ENABLED ); 
					EnableMenuItem( GetMenu( hWndMain ), IDM_COPY, MF_BYCOMMAND|MF_ENABLED );
				}
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
			if ( ! DeviceIoControl(	SysHandle, IOCTL_REGMON_ZEROSTATS,
									NULL, 0, NULL, 0, &nb, NULL ) )
			{
				Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
				return TRUE;
			}
			// Update statistics windows
			UpdateStatistics( hWnd, hWndList, NULL, 0, TRUE );
			return FALSE;

		case IDM_HELP:
			WinHelp(hWnd, _T("regmon.hlp"), HELP_CONTENTS, 0L);
			return 0;

		case IDM_HISTORY:
			DialogBox( hInst, TEXT("History"), hWnd, (DLGPROC) HistoryProc );
			return 0;

		case IDM_SHOWMS:
			// Toggle time format
			ShowMs = !ShowMs;
			CheckMenuItem( GetMenu(hWnd), IDM_SHOWMS,
							MF_BYCOMMAND|(ShowMs?MF_CHECKED:MF_UNCHECKED) );
			return FALSE;

		case IDM_TIME:
			// Toggle time format
			ClockTime = !ClockTime;
			CheckMenuItem( GetMenu(hWnd), IDM_TIME,
							MF_BYCOMMAND|(ClockTime?MF_CHECKED:MF_UNCHECKED) );
			SendMessage( hWndToolbar, TB_CHANGEBITMAP, IDM_TIME, (ClockTime?10:11) );
			InvalidateRect( hWndToolbar, NULL, TRUE );
			EnableMenuItem( GetMenu( hWnd ), IDM_SHOWMS, 
				MF_BYCOMMAND|(ClockTime?MF_ENABLED:MF_GRAYED));
			return FALSE;
		
		case IDM_FIND:
			// search the listview
			PrevMatch = FALSE;
			PopFindDialog( hWnd );
			return 0;

		case IDM_FINDPREV:
		case IDM_FINDAGAIN:

			if( LOWORD( wParam ) == IDM_FINDAGAIN ) {

				FindTextInfo.Flags |= FR_DOWN;

			} else {

				FindTextInfo.Flags &= ~FR_DOWN;
			}
			if( PrevMatch ) {
				hSaveCursor = SetCursor(hHourGlass);
				if (FindInListview( hWnd, &FindTextInfo ) ) {
					Autoscroll = FALSE;
					CheckMenuItem( GetMenu(hWnd), IDM_AUTOSCROLL,
									MF_BYCOMMAND|MF_UNCHECKED ); 
					SendMessage( hWndToolbar, TB_CHANGEBITMAP, IDM_AUTOSCROLL, 3 );
				}
				SetCursor( hSaveCursor );
			}
			return 0;

		case IDM_DELETE:

			DeleteSelection( hWnd );
			return 0;

		case IDM_CAPTURE:
			// Read statistics from driver
			Capture = !Capture;
			CheckMenuItem( GetMenu(hWnd), IDM_CAPTURE,
							MF_BYCOMMAND|(Capture?MF_CHECKED:MF_UNCHECKED) );

			// Have driver turn on hooks
			if ( ! DeviceIoControl(	SysHandle, Capture ? IOCTL_REGMON_HOOK : 
									IOCTL_REGMON_UNHOOK,
									NULL, 0, NULL, 0, &nb, NULL ) ) {

				Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
				return TRUE;
			}
			SendMessage( hWndToolbar, TB_CHANGEBITMAP, IDM_CAPTURE, (Capture?2:1) );
			InvalidateRect( hWndToolbar, NULL, TRUE );
			return FALSE;

		case IDM_AUTOSCROLL:
			Autoscroll = !Autoscroll;
			CheckMenuItem( GetMenu(hWnd), IDM_AUTOSCROLL,
							MF_BYCOMMAND|(Autoscroll?MF_CHECKED:MF_UNCHECKED) ); 
			SendMessage( hWndToolbar, TB_CHANGEBITMAP, IDM_AUTOSCROLL, (Autoscroll?4:3) );
			InvalidateRect( hWndToolbar, NULL, TRUE );					
			return FALSE;

		case IDM_BOOTLOG:
			BootLog = !BootLog;
			CheckMenuItem( GetMenu(hWnd), IDM_BOOTLOG,
							MF_BYCOMMAND|(BootLog?MF_CHECKED:MF_UNCHECKED) ); 
			if( BootLog ) {

				if( !CopyFile( Path, driverPath, FALSE )) {

					sprintf( msgbuf, _T("Unable to copy %s to %s\n\n")
						_T("Make sure that %s is in the current directory and that you have permission to\n")
						_T("the <Winnt>\\System32\\Drivers directory."),
						SYS_NAME, driverPath, isWnet ? SYS_FILE_WNET : SYS_FILE );
					MessageBox( hWnd, msgbuf, APPNAME, MB_OK|MB_ICONERROR);
					return FALSE;
				}
				SetFileAttributes( driverPath, FILE_ATTRIBUTE_NORMAL );
				sprintf( msgbuf, 
					_T("Regmon has been configured to log Registry activity to %s during the next boot"),
					logFile );
				MessageBox( hWnd, msgbuf, APPNAME, MB_OK|MB_ICONINFORMATION);
			} else {

				DeleteFile( driverPath );
			}
			BootLogMenuUsed = TRUE;
			return FALSE;

		case IDM_EXIT:
			// Close ourself
			SendMessage( hWnd, WM_CLOSE, 0, 0 );
			return FALSE;

		case IDM_FILTER:
			DialogBox( hInst, _T("Filter"), hWnd, (DLGPROC) FilterProc );
			return FALSE;

		case IDM_STAYTOP:
			OnTop = !OnTop;
			if( OnTop ) SetWindowPos( hWnd, HWND_TOPMOST, 0, 0, 0, 0, 
							SWP_NOMOVE|SWP_NOSIZE );
			else  SetWindowPos( hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, 
							SWP_NOMOVE|SWP_NOSIZE );
			CheckMenuItem( GetMenu(hWnd), IDM_STAYTOP,
							MF_BYCOMMAND|(OnTop?MF_CHECKED:MF_UNCHECKED) );
			return FALSE;

		case IDM_COPY:
			CopySelection( hWnd );
			return FALSE;
		
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
				
		case IDM_JUMP:

			// open the specified key in Regedit, if we can
			RegeditJump( hWnd );
			return FALSE;

		case IDM_ABOUT:
			// Show the names of the authors
			DialogBox( hInst, _T("AboutBox"), hWnd, (DLGPROC)AboutDlgProc );
			return FALSE;

		case IDM_SAVE:
			SaveFile( hWnd, hWndList, FALSE );
			return FALSE;

		case IDM_SAVEAS:
			SaveFile( hWnd, hWndList, TRUE );
			return FALSE;

		default:
			break;
		}
		break;

	case WM_TIMER:
		// Time to query the device driver for more data
		if ( Capture )  {

			// don't process for more than a second without pausing
			startTime = GetTickCount();
			for (;;)  {

				// Have driver fill Stats buffer with information
				if ( ! DeviceIoControl(	SysHandle, IOCTL_REGMON_GETSTATS,
										NULL, 0, &Stats, sizeof Stats,
										&StatsLen, NULL ) )
				{
					Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
					return TRUE;
				}
				if ( StatsLen == 0 )
					break;

				// Update statistics windows
				UpdateStatistics( hWnd, hWndList, Stats, StatsLen, FALSE );

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
		MoveWindow( hWndToolbar, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE );
        MoveWindow( hWndList, 0, TOOLBARHEIGHT, LOWORD(lParam), HIWORD(lParam)-TOOLBARHEIGHT, TRUE );
		if( hBalloon ) DestroyWindow( hBalloon );
		return 0;

	case WM_MOVE:
	case WM_MOUSEMOVE:
		if( hBalloon ) DestroyWindow( hBalloon );
		return 0;

	case WM_DRAWITEM:
		DrawListViewItem( (LPDRAWITEMSTRUCT) lParam );
		break;

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

	case WM_CLOSE:

		// Have driver unhook if necessary
		if( Capture ) {
			if ( ! DeviceIoControl(	SysHandle, IOCTL_REGMON_UNHOOK,
								NULL, 0, NULL, 0, &nb, NULL ) )
			{
				Abort( hWnd, _T("Couldn't access device driver"), GetLastError() );
				return 0;
			}
		}

		KillTimer( hWnd, 1 );
		CloseHandle( SysHandle );	
#if _DEBUG
		if ( IsNT &&  !UnloadDeviceDriver( SYS_NAME ) )  {
			wsprintf( msgbuf, _T("Error unloading \"%s\""), SYS_NAME );
			MessageBox( hWnd, msgbuf, _T("Regmon"), MB_OK );
		}
#endif
		// Make sure the user knows boot logging will take place
		if( IsNT ) {

			if( !BootLogMenuUsed && BootLog ) {

				sprintf( msgbuf, 
					_T("Regmon is configured to log Registry activity to %s during the next boot"),
					logFile );

				MessageBox( hWnd, msgbuf, _T("Regmon"), MB_ICONINFORMATION);
			}

			// if boot logging isn't enabled, delete the driver from 
			// the drivers directory
			if( BootLog ) {

				// boot logging on - configure the regmon service key.
				if( RegCreateKey( HKEY_LOCAL_MACHINE, REGMON_DRIVER_KEY, &hDriverKey ) == 
					ERROR_SUCCESS ) {

					// the driver is already in the winnt\system32\drivers directory
					driverStart = SERVICE_BOOT_START;
					RegDeleteValue( hDriverKey, _T("DeleteFlag" ));
					RegSetValueEx( hDriverKey, _T("Start"), 0, REG_DWORD, 
						(PBYTE) &driverStart, sizeof(driverStart));
					RegSetValueEx( hDriverKey, "Group", 0, REG_SZ, group, sizeof( group ));
					tag = 1;
					RegSetValueEx( hDriverKey, "Tag", 0, REG_DWORD, 
						(PBYTE) &tag, sizeof(tag));
					RegSetValueEx( hDriverKey, "Type", 0, REG_DWORD,
						(PBYTE) &tag, sizeof(tag));
					sprintf( Path, _T("System32\\Drivers\\%s"),
						isWnet ? SYS_FILE_WNET : SYS_FILE );	
					RegSetValueEx( hDriverKey, _T("ImagePath"), 0, REG_EXPAND_SZ,
						Path, (DWORD) strlen(Path));
					RegCloseKey( hDriverKey );

				} else {

					Abort( hWnd, _T("Regmon could not configure boot logging"), GetLastError());
				}
			}	
		}
		SavePositionSettings( hWnd );
		break;

	case WM_SETFOCUS:
		SetFocus( hWndList );
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return FALSE;

	case WM_PAINT:
		if( !IsNT && Deleting ) {
			hDC = BeginPaint( hWnd, &Paint );
			EndPaint( hWnd, &Paint );
			return TRUE;
		}
		break;

	default:
		// is it a find-string message?
		if (message == findMessageID ){ 

			// get a pointer to the find structure
			findMessageInfo = (LPFINDREPLACE)lParam;

			// If the FR_DIALOGTERM flag is set, invalidate the find window handle
			if( findMessageInfo->Flags & FR_DIALOGTERM) {

				FindFlags = FindTextInfo.Flags & (FR_DOWN|FR_MATCHCASE|FR_WHOLEWORD);
				return 0;
			}

			// if the FR_FINDNEXT flag is set, go do the search
			if( findMessageInfo->Flags & FR_FINDNEXT ) {

				hSaveCursor = SetCursor(hHourGlass);
				if( FindInListview( hWnd, findMessageInfo ) ) {
					Autoscroll = FALSE;
					CheckMenuItem( GetMenu(hWnd), IDM_AUTOSCROLL,
									MF_BYCOMMAND|MF_UNCHECKED ); 
					SendMessage( hWndToolbar, TB_CHANGEBITMAP, IDM_AUTOSCROLL, 3 );
				}
				ReleaseCapture(); 
				PostMessage( hWndFind, WM_COMMAND, IDABORT, 0 );
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
*    FUNCTION: InitApplication(HINSTANCE)
*
*    PURPOSE: Initializes window data and registers window class
*
****************************************************************************/
BOOL InitApplication( HINSTANCE hInstance, PTCHAR RegmonClassName )
{
	WNDCLASS  wc;
	
	// Fill in window class structure with parameters that describe the
	// main (statistics) window. 
	wc.style			= 0;                     
	wc.lpfnWndProc		= (WNDPROC)MainWndProc; 
	wc.cbClsExtra		= 0;              
	wc.cbWndExtra		= 0;              
	wc.hInstance		= hInstance;       
	wc.hIcon			= LoadIcon( hInstance, _T("ICON") );
	wc.hCursor			= LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground	= (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName		= _T("LISTMENU");
	sprintf( RegmonClassName, "%d-%d", rand(), rand());	
	wc.lpszClassName	= RegmonClassName;
	if ( ! RegisterClass( &wc ) )
		return FALSE;

	wc.lpszMenuName	  = NULL;
 	wc.lpfnWndProc    = (WNDPROC) BalloonDialog;
	wc.hbrBackground  = CreateSolidBrush( 0x00E0FFFF );
	wc.lpszClassName  = "BALLOON";
	RegisterClass( &wc );
	
	return TRUE;
}


/****************************************************************************
*
*    FUNCTION:  InitInstance(HINSTANCE, int)
*
*    PURPOSE:  Saves instance handle and creates main window
*
****************************************************************************/
HWND InitInstance( HINSTANCE hInstance, int nCmdShow, PTCHAR RegmonClassName )
{
	// get the window position settings from the registry
	GetPositionSettings();

	hInst = hInstance;
	hWndMain = CreateWindow( RegmonClassName, 
							_T("Registry Monitor - Sysinternals: www.sysinternals.com"), 
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
	UpdateWindow( hWndMain ); 

	// update clock time button
	SendMessage( hWndToolbar, TB_CHANGEBITMAP, IDM_TIME, (ClockTime?10:11) );
	InvalidateRect( hWndToolbar, NULL, TRUE );
    CheckMenuItem( GetMenu( hWndMain ), IDM_TIME,
                   MF_BYCOMMAND|(ClockTime?MF_CHECKED:MF_UNCHECKED));
    CheckMenuItem( GetMenu( hWndMain ), IDM_SHOWMS,
                   MF_BYCOMMAND|(ShowMs?MF_CHECKED:MF_UNCHECKED));
	EnableMenuItem( GetMenu( hWndMain ), IDM_SHOWMS, 
					MF_BYCOMMAND|(ClockTime?MF_ENABLED:MF_GRAYED));

	// maximize it if necessary
	if( PositionInfo.maximized ) {

		ShowWindow( hWndMain, SW_SHOWMAXIMIZED );
	}
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
    CHAR    regmonClassName[MAX_PATH]; 	        

	if ( ! InitApplication( hInstance, regmonClassName ) )
		return FALSE;   
	
	// get NT version
	IsNT = GetVersion() < 0x80000000;

	// initializations that apply to a specific instance 
	if ( (hWnd = InitInstance( hInstance, nCmdShow, regmonClassName )) == NULL )
		return FALSE;

	// load accelerators
	hAccel = LoadAccelerators( hInstance, _T("ACCELERATORS"));

	// register for the find window message
    findMessageID = RegisterWindowMessage( FINDMSGSTRING );

	// acquire and dispatch messages until a WM_QUIT message is received.
	while ( GetMessage( &msg, NULL, 0, 0 ) )  {

		if( hWndFind && IsDialogMessage( hWndFind, &msg )) {

			continue;
		}
		if( TranslateAccelerator( hWnd, hAccel, &msg )) {

			continue;
		}
		TranslateMessage( &msg );
		DispatchMessage( &msg ); 
	}
	return 0;										 
}
