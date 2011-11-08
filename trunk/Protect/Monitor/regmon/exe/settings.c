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
#include "settings.h"
#include "filter.h"
#include "listview.h"

// The variable that holds the position settings
POSITION_SETTINGS	PositionInfo;

BOOLEAN				OnTop;
BOOLEAN				ClockTime;
BOOLEAN				ShowToolbar = TRUE;
BOOLEAN				ShowMs = FALSE;

/******************************************************************************
*
*	FUNCTION:	GetPositionSettings
*
*	PURPOSE:	Reads the Registry to get the last-set window position.
*
******************************************************************************/
VOID GetPositionSettings()
{
	HKEY		hKey;
	DWORD		ParamSize, newPosSize;
	LOGFONT		lf;
	int			i;
	POSITION_SETTINGS newPositionInfo;

	char		*nextString;
	char		recentExList[(MAXFILTERLEN+1) * NUMRECENTFILTERS + 1];
	char		recentInList[(MAXFILTERLEN+1) * NUMRECENTFILTERS + 1];
	char		recentHiList[(MAXFILTERLEN+1) * NUMRECENTFILTERS + 1];

	// Delete old settings
	RegDeleteKey( HKEY_CURRENT_USER, "Software\\Systems Internals\\Regmon" );

	memset( &newPositionInfo ,   0, sizeof( newPositionInfo ));

	// Default font
	GetObject( GetStockObject(SYSTEM_FONT), sizeof lf, &lf ); 
	lf.lfWeight = FW_NORMAL;
	lf.lfHeight = 8;
	lf.lfWidth  = 0;
	strcpy( lf.lfFaceName, TEXT("MS Sans Serif") );
	PositionInfo.font = lf;

	// Fist, set the default settings
	PositionInfo.top		= 100;
	PositionInfo.left		= 100;
	PositionInfo.width		= 600;
	PositionInfo.height		= 300;
	PositionInfo.maximized	= FALSE;
	PositionInfo.highlightfg = 0x00FFFFFF;
	PositionInfo.highlightbg = 0x000000FF;
	PositionInfo.logerror	= TRUE;
	PositionInfo.logsuccess	= TRUE;
	PositionInfo.logwrites	= TRUE;
	PositionInfo.logreads	= TRUE;
	PositionInfo.logaux		= TRUE;
	PositionInfo.ontop		= FALSE;

	// set the default listview widths
	PositionInfo.column[0] = 35;
	PositionInfo.column[1] = 90;
	PositionInfo.column[2] = 70;
	PositionInfo.column[3] = 70;
	PositionInfo.column[4] = 200;
	PositionInfo.column[5] = 50;
	PositionInfo.column[6] = 70;

	// Clear the recent arrays
	recentInList[0] = '*';
	recentInList[1] = 0;
	recentInList[2] = 0;
	recentExList[0] = 0;
	recentHiList[0] = 0;
	memset( RecentExFilters,   0, sizeof( RecentExFilters ));
	memset( RecentInFilters,   0, sizeof( RecentInFilters ));
	memset( RecentHiFilters,   0, sizeof( RecentHiFilters ));

	// set the default history depth (infinite)
	PositionInfo.historydepth = 0;

	// get the params and ignore errors
	RegCreateKey(HKEY_CURRENT_USER, REGMON_SETTINGS_KEY, &hKey );
	newPosSize = sizeof( PositionInfo );
	newPositionInfo.posversion = 0;
	RegQueryValueEx( hKey,REGMON_SETTINGS_VALUE, NULL, NULL, (LPBYTE) &newPositionInfo,
				&newPosSize );
	ParamSize = sizeof( recentInList );
	RegQueryValueEx( hKey,REGMON_RECENT_INFILTER_VALUE, NULL, NULL, (LPBYTE) &recentInList,
				&ParamSize );
	ParamSize = sizeof( recentExList );
	RegQueryValueEx( hKey,REGMON_RECENT_EXFILTER_VALUE, NULL, NULL, (LPBYTE) &recentExList,
				&ParamSize );
	ParamSize = sizeof( recentHiList );
	RegQueryValueEx( hKey,REGMON_RECENT_HIFILTER_VALUE, NULL, NULL, (LPBYTE) &recentHiList,
				&ParamSize );
	CloseHandle( hKey );

	// See if we have new position info
	if( newPositionInfo.posversion == POSITION_VERSION &&
		newPosSize == sizeof(PositionInfo)) {
		
		PositionInfo = newPositionInfo;
	}

	// extract the history depth
	MaxLines	= PositionInfo.historydepth;
	OnTop		= PositionInfo.ontop;
	ClockTime	= PositionInfo.clocktime;
	ShowMs		= PositionInfo.showms;

	// get font
	LogFont = PositionInfo.font;
 	hFont = CreateFontIndirect( &LogFont ); 

	// set highlight colors
	HighlightFg = PositionInfo.highlightfg;
	HighlightBg = PositionInfo.highlightbg;

	// get misc device filter
	FilterDefinition.logsuccess = PositionInfo.logsuccess;
	FilterDefinition.logerror	= PositionInfo.logerror;
	FilterDefinition.logreads	= PositionInfo.logreads;
	FilterDefinition.logwrites	= PositionInfo.logwrites;
	FilterDefinition.logaux		= PositionInfo.logaux;

	// Get the recent filter lists
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

	// get the position of the main window
	PositionInfo.posversion = POSITION_VERSION;
	GetWindowRect( hWnd, &rc );
	if( !IsIconic( hWnd ) && !IsZoomed( hWnd )) {

		PositionInfo.left = rc.left;
		PositionInfo.top = rc.top;
		PositionInfo.width = rc.right - rc.left;
		PositionInfo.height = rc.bottom - rc.top;
	} 
	PositionInfo.maximized = IsZoomed( hWnd );
	PositionInfo.ontop = OnTop;

	// get the widths of the listview columns
	for( i = 0; i < NUMCOLUMNS; i++ ) {
		PositionInfo.column[i] = ListView_GetColumnWidth( hWndList, i );
	}

	// get the history depth
	PositionInfo.historydepth = MaxLines;

	// save time format
	PositionInfo.clocktime = ClockTime;
	PositionInfo.showms = ShowMs;	

	// save font
	PositionInfo.font = LogFont;

	// get misc device filters
	PositionInfo.logsuccess = FilterDefinition.logsuccess;
	PositionInfo.logerror   = FilterDefinition.logerror;
	PositionInfo.logreads   = FilterDefinition.logreads;
	PositionInfo.logwrites  = FilterDefinition.logwrites;
	PositionInfo.logaux		= FilterDefinition.logaux;

	// save highlight colors
	PositionInfo.highlightfg = HighlightFg;
	PositionInfo.highlightbg = HighlightBg;

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
	RegOpenKey(HKEY_CURRENT_USER, REGMON_SETTINGS_KEY,	&hKey );
	RegSetValueEx( hKey, REGMON_SETTINGS_VALUE, 0, REG_BINARY, (LPBYTE) &PositionInfo,
			sizeof( PositionInfo ) );
	RegSetValueEx( hKey, REGMON_RECENT_INFILTER_VALUE, 0, REG_BINARY, (LPBYTE) &recentInList,
			(DWORD) (nextInString - recentInList) + 1 );
	RegSetValueEx( hKey, REGMON_RECENT_EXFILTER_VALUE, 0, REG_BINARY, (LPBYTE) &recentExList,
			(DWORD) (nextExString - recentExList) + 1 );
	RegSetValueEx( hKey, REGMON_RECENT_HIFILTER_VALUE, 0, REG_BINARY, (LPBYTE) &recentHiList,
			(DWORD) (nextHiString - recentHiList) + 1 );
	CloseHandle( hKey );
}	

