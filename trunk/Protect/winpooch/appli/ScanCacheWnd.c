/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2006  Benoit Blanchon                      */
/*                                                                */
/*  This program is free software; you can redistribute it        */
/*  and/or modify it under the terms of the GNU General Public    */
/*  License as published by the Free Software Foundation; either  */
/*  version 2 of the License, or (at your option) any later       */
/*  version.                                                      */
/*                                                                */
/*  This program is distributed in the hope that it will be       */
/*  useful, but WITHOUT ANY WARRANTY; without even the implied    */
/*  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       */
/*  PURPOSE.  See the GNU General Public License for more         */
/*  details.                                                      */
/*                                                                */
/*  You should have received a copy of the GNU General Public     */
/*  License along with this program; if not, write to the Free    */
/*  Software Foundation, Inc.,                                    */
/*  675 Mass Ave, Cambridge, MA 02139, USA.                       */
/*                                                                */
/******************************************************************/


/******************************************************************/
/* Build configuration                                            */
/******************************************************************/

#define TRACE_LEVEL	2	// warning level

#define SHOW_IDENTIFIER	1


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "ScanCacheWnd.h"

// standard headers
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>

// project's headers
#include "Assert.h"
#include "Language.h"
#include "Resources.h"	// for IDC_XXX
#include "Scanner.h"
#include "SpyServer.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#if SHOW_IDENTIFIER

#define COL_IDENTIFIER	0
#define COL_SCANRESULT	1
#define COL_REACTION	2
#define COL_SCANTIME	3
#define COL_FILEPATH	4

#else

#define COL_SCANRESULT	0
#define COL_REACTION	1
#define COL_SCANTIME	2
#define COL_FILEPATH	3

#endif

#define WC_SCANCACHEWND TEXT("ScanCacheWnd")

// default english strings
LPCTSTR g_szIdentifier		= TEXT("Identifier") ;
LPCTSTR g_szScanResult		= TEXT("Scan result") ;
LPCTSTR	g_szFilePath		= TEXT("File path") ;
LPCTSTR g_szScanTime		= TEXT("Scan time") ;
LPCTSTR g_szReaction		= TEXT("Reaction") ;
LPCTSTR g_szNoScannerWarning	= TEXT("This window is empty because no antivirus has been configured.\n"
				       "You can select an antivirus in configuration window.") ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

LRESULT CALLBACK _ScanCacheWnd_WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) ;

BOOL _ScanCacheWnd_UpdateItem (HWND, UINT, ULONG, LPCTSTR, SCANRESULT nScanResult, LARGE_INTEGER*) ;

BOOL _ScanCacheWnd_SyncCallback (VOID * pUserPtr, ULONG nIdentifier, LPCWSTR wszFilePath, SCANRESULT nResult, LARGE_INTEGER *pliScanTime) ;


/******************************************************************/
/* Exported function : RegisterClass                              */
/******************************************************************/

BOOL ScanCacheWnd_RegisterClass (HINSTANCE hInstance) 
{
  WNDCLASS wndclass ;

  wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
  wndclass.lpfnWndProc   = _ScanCacheWnd_WndProc ;
  wndclass.cbClsExtra    = 0 ;
  wndclass.cbWndExtra    = 0 ;
  wndclass.hInstance     = hInstance ;
  wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION) ;
  wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
  wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1) ;
  wndclass.lpszMenuName  = NULL ;
  wndclass.lpszClassName = WC_SCANCACHEWND ;

  return 0!=RegisterClass (&wndclass) ;
}


/******************************************************************/
/* Exported function : CreateWindow                               */
/******************************************************************/

HWND ScanCacheWnd_CreateWindow (HINSTANCE hInstance, HWND hwndParent)
{  
  return CreateWindow (WC_SCANCACHEWND, NULL,
		       WS_CHILD,
		       CW_USEDEFAULT, CW_USEDEFAULT,
		       CW_USEDEFAULT, CW_USEDEFAULT, 
		       hwndParent, NULL, hInstance, NULL) ;
}


/******************************************************************/
/* Internal function : WndProc                                    */
/******************************************************************/

LRESULT CALLBACK _ScanCacheWnd_WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static HWND		g_hwndList ;
  static HINSTANCE	g_hInstance ;
  static HWND		g_hwndWarning ;
  static LARGE_INTEGER	g_liLastSyncTime ;
  static ULONG		g_nFirstIdentifier ;

  int		nWidth ;
  int		nHeight ;
  LV_COLUMN	lvc ;

  switch (message)
    {
    case WM_CREATE:
      
      g_hInstance = ((CREATESTRUCT*)lParam)->hInstance ;

      g_hwndList = CreateWindowEx (WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
				   WS_CHILD|WS_VSCROLL|LVS_REPORT|
				   LVS_SINGLESEL|LVS_SHOWSELALWAYS|LVS_NOSORTHEADER,
				   0,0,0,0, hwnd, (HMENU)IDC_TRUSTEDFILES, g_hInstance, NULL) ;
      ListView_SetExtendedListViewStyle (g_hwndList, LVS_EX_FULLROWSELECT) ;

      g_hwndWarning = CreateWindowEx (WS_EX_CLIENTEDGE, WC_STATIC, NULL,
				      WS_CHILD|SS_LEFT, 0, 0, 0, 0,
				      hwnd, NULL, g_hInstance, NULL) ;

       // create columns
      ZeroMemory (&lvc, sizeof(lvc)) ;
      lvc.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM ;
      lvc.pszText = TEXT("") ;

#if SHOW_IDENTIFIER
      lvc.cx		= 50 ;
      lvc.iSubItem	= COL_IDENTIFIER ;
      lvc.pszText	= (TCHAR*)g_szIdentifier ;
      ListView_InsertColumn (g_hwndList, lvc.iSubItem, &lvc) ; 
#endif

      lvc.cx		= 90 ;
      lvc.iSubItem	= COL_SCANRESULT ;
      lvc.pszText	= (TCHAR*)g_szScanResult ;
      ListView_InsertColumn (g_hwndList, lvc.iSubItem, &lvc) ;

      lvc.cx		= 90 ;
      lvc.iSubItem	= COL_REACTION ;
      lvc.pszText	= (TCHAR*)g_szReaction ;
      ListView_InsertColumn (g_hwndList, lvc.iSubItem, &lvc) ;  

      lvc.cx		= 90 ;
      lvc.iSubItem	= COL_SCANTIME ;
      ListView_InsertColumn (g_hwndList, lvc.iSubItem, &lvc) ;       

      lvc.cx		= 400 ;
      lvc.iSubItem	= COL_FILEPATH ;
      ListView_InsertColumn (g_hwndList, lvc.iSubItem, &lvc) ;      
  
      SendMessage (hwnd, WM_LANGUAGECHANGED, 0, 0) ;

      g_liLastSyncTime.QuadPart = 0 ;
      SendMessage (hwnd, WM_TIMER, 0, 0) ;
      SetTimer (hwnd, 0, 5000, NULL) ;
	
      return 0 ; // case WM_CREATE:

    case WM_LANGUAGECHANGED:

      ZeroMemory (&lvc, sizeof(lvc)) ;
      lvc.mask = LVCF_TEXT ;

      lvc.pszText = (LPTSTR) STR_DEF (_PATH, g_szFilePath) ;
      ListView_SetColumn (g_hwndList, COL_FILEPATH, &lvc) ; 
      
      lvc.pszText = (LPTSTR) STR_DEF (_SCAN_TIME, g_szScanTime) ;
      ListView_SetColumn (g_hwndList, COL_SCANTIME, &lvc) ; 

      SetWindowText (g_hwndWarning, STR_DEF(_NO_SCANNER_CONFIGURED, g_szNoScannerWarning)) ;
     
      return 0 ; // case WM_LANGUAGECHANGED:
      
    case WM_DESTROY:

      KillTimer (hwnd, 0) ;

      return 0 ;

    case WM_SIZE:

      nWidth = LOWORD (lParam) ;
      nHeight = HIWORD (lParam) ;
      MoveWindow (g_hwndList, 0, 0, nWidth, nHeight, TRUE) ;
      MoveWindow (g_hwndWarning, 0, 0, nWidth, nHeight, TRUE) ;

      return 0 ;

    case WM_SHOWWINDOW:
      
      if( wParam ) // window is being shown
	{
	  if( Scanner_IsConfigured() )
	    {
	      ShowWindow (g_hwndList, SW_SHOW) ;
	      ShowWindow (g_hwndWarning, SW_HIDE) ;
	    }
	  else
	    {
	      ShowWindow (g_hwndWarning, SW_SHOW) ;
	      ShowWindow (g_hwndList, SW_HIDE) ;
	    }
	}
	  
      return 0 ; // case WM_UPDATESCANCACHE:


    case WM_TIMER:
      {
	ULONG	nFirstIdentifier, nLastIdentifier ;
	SYSTEMTIME st ;
	BOOL	bListWasEmpty ;
	GetLocalTime (&st) ;

	bListWasEmpty = ListView_GetItemCount(g_hwndList) == 0 ;

	SpySrv_SyncScanCache (g_hwndList, 
			      _ScanCacheWnd_SyncCallback, 
			      &g_liLastSyncTime,
			      &nFirstIdentifier,
			      &nLastIdentifier) ;

	SystemTimeToFileTime (&st, (FILETIME*)&g_liLastSyncTime) ;

	if( ! bListWasEmpty && g_nFirstIdentifier != nFirstIdentifier )
	  {
	    ULONG	nIdentifier ;

	    //
	    // remove obsolete items
	    //

	    TRACE_INFO (TEXT("Removing from %lu to %lu\n"), g_nFirstIdentifier, nFirstIdentifier) ;

	    for( nIdentifier=g_nFirstIdentifier ; nIdentifier<nFirstIdentifier ; nIdentifier++ )
	      {
		LVFINDINFO lvfi = {0} ;
		INT iCurrent ;

		lvfi.flags	= LVFI_PARAM ;
		lvfi.lParam	= nIdentifier ;  
		
		iCurrent = ListView_FindItem (g_hwndList, -1, &lvfi) ;
		
		if( iCurrent==-1 )
		  {
		    TRACE_WARNING (TEXT("Identifier %lu not found\n"), nIdentifier) ;
		    continue ;
		  }
		
		ListView_DeleteItem (g_hwndList, iCurrent) ;
	      }
	  }

	g_nFirstIdentifier = nFirstIdentifier ;
      }
      return 0 ;
    }

  return DefWindowProc (hwnd, message, wParam, lParam) ;
}


/******************************************************************/
/* Internal function : WndProc                                    */
/******************************************************************/

BOOL _ScanCacheWnd_UpdateItem (HWND hwndList, UINT nNumber, 
			       ULONG nIdentifier, LPCTSTR szFilePath, 
			       SCANRESULT nScanResult,LARGE_INTEGER *pliScanTime)
{
  TCHAR		szBuffer[32] ;
  LVITEM	lvi ;
  SYSTEMTIME	stScanTime ;
  TCHAR		szScanResult[32] ;
  TCHAR		szReaction[32] ;

  ZeroMemory (&lvi, sizeof(lvi)) ;	      
  lvi.mask = LVIF_TEXT ;

  lvi.iItem = nNumber ;

#if SHOW_IDENTIFIER

  wsprintf (szBuffer, TEXT("%lu"), nIdentifier) ;
  lvi.iSubItem	= COL_IDENTIFIER ;
  lvi.pszText	= szBuffer ;
  ListView_SetItem (hwndList, &lvi) ;

#endif

  switch( nScanResult )
    {
    case SCAN_NOT_SCANNED:
      _tcscpy (szScanResult, STR_DEF(_NOT_SCANNED,TEXT("Not scanned"))) ;
      break ;

    case SCAN_BEING_SCANNED:
      _tcscpy (szScanResult, STR_DEF(_BEING_SCANNED,TEXT("Being scanned"))) ;
      break ;

    case SCAN_NO_VIRUS:
      _tcscpy (szScanResult, STR_DEF(_NO_VIRUS,TEXT("No virus"))) ;
      break ;

    case SCAN_VIRUS:
    case SCAN_VIRUS_ACCEPTED:
      _tcscpy (szScanResult, STR_DEF(_VIRUS_FOUND,TEXT("Virus found"))) ;
      break ;

    case SCAN_FAILED:
    case SCAN_FAILED_ACCEPTED:
      _tcscpy (szScanResult, STR_DEF(_SCAN_FAILED,TEXT("Scan failed"))) ;
      break ; 

    default:
      _tcscpy (szScanResult, STR_DEF(_UNKNOWN,TEXT("Unknown"))) ;
      break ;
    }

  switch( nScanResult )
    {
    case SCAN_VIRUS:
    case SCAN_FAILED:
      _tcscpy (szReaction, STR_DEF(_REJECT,TEXT("Reject"))) ;
      break ;

    case SCAN_NO_VIRUS:
    case SCAN_VIRUS_ACCEPTED:
    case SCAN_FAILED_ACCEPTED:
      _tcscpy (szReaction, STR_DEF(_ACCEPT,TEXT("Accept"))) ;
      break ;
      
    default:
      _tcscpy (szReaction, TEXT("")) ;
      break ;
    }

  lvi.iSubItem	= COL_SCANRESULT ;
  lvi.pszText	= szScanResult ;
  ListView_SetItem (hwndList, &lvi) ;

  lvi.iSubItem	= COL_REACTION ;
  lvi.pszText	= szReaction ;
  ListView_SetItem (hwndList, &lvi) ;

  lvi.iSubItem	= COL_FILEPATH ;
  lvi.pszText	= (LPTSTR)szFilePath ;
  ListView_SetItem (hwndList, &lvi) ;

  FileTimeToSystemTime ((FILETIME*)pliScanTime, &stScanTime) ;

  wsprintf (szBuffer, TEXT("%02u:%02u:%02u"),
	    stScanTime.wHour, stScanTime.wMinute, stScanTime.wSecond) ;

  lvi.iSubItem	= COL_SCANTIME ;
  lvi.pszText	= szBuffer ;
  ListView_SetItem (hwndList, &lvi) ;

  return TRUE ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _ScanCacheWnd_SyncCallback (VOID * pUserPtr, ULONG nIdentifier, LPCWSTR wszFilePath, SCANRESULT nScanResult, LARGE_INTEGER *pliScanTime)
{
  HWND hwndList = (HWND)pUserPtr ;
  LVFINDINFO lvfi = {0} ;
  INT iCurrent ;

  lvfi.flags	= LVFI_PARAM ;
  lvfi.lParam	= nIdentifier ;  

  iCurrent = ListView_FindItem (hwndList, -1, &lvfi) ;
 
  if( iCurrent==-1 )
    {
      LVITEM lvi ;

      iCurrent = ListView_GetItemCount(hwndList) ;

      lvi.mask = LVIF_PARAM ;
      lvi.iItem = iCurrent ;
      lvi.iSubItem = 0 ;
      lvi.lParam = (LPARAM)nIdentifier ;
      ListView_InsertItem (hwndList, &lvi) ;    
    }

  _ScanCacheWnd_UpdateItem (hwndList, iCurrent, nIdentifier, wszFilePath, nScanResult, pliScanTime) ;

  return TRUE ;
}
