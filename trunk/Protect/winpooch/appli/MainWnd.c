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

#define TRACE_LEVEL	2 /* = warnings */


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "MainWnd.h"

// standard headers
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>

// project's headers
#include "AboutWnd.h"
#include "Assert.h"
#include "Config.h"
#include "ConfigWnd.h"
#include "EventLog.h"
#include "FilterTools.h"
#include "FilterWnd.h"
#include "HistoryWnd.h"
#include "Language.h"
#include "LogFile.h"
#include "PicMenuCtl.h"
#include "ProcList.h"
#include "ProcListWnd.h"
#include "ProjectInfo.h"
#include "Resources.h"
#include "ScanCacheWnd.h"
#include "SpyServer.h"
#include "SplashWnd.h"
#include "Trace.h"
#include "TrayIcon.h"
#include "UpdWatcher.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define WC_MAINWND	TEXT("MainWnd")
#define WC_PICMENU	TEXT("PicMenu")

#define TAB_FILTERS	0
#define TAB_HISTORY	1
#define TAB_PROCESSES	2
#define TAB_SCANCACHE	3
#define TAB_CONFIG	4
#define TAB_ABOUT	5
#define _TAB_COUNT	6

#define CX_MENU		150
#define CX_SPACE	10
#define CY_SPACE	10


static LPCTSTR g_szNewVersionAvailable = TEXT("A new version of Winpooch is available.") ;

static LPCTSTR g_szDownloadVersionS = TEXT("Winpooch version %s is available.\n"
					   "Do you want to download it ?") ;

static LPCTSTR g_szFirstCloseWarning = TEXT("This is the first time you close Winpooch window.\n"
					    "Please note that Winpooch will still be running.\n"
					    "To shutdown Winpooch, right-click on system tray "
					    "icon and click \"Shutdown\"") ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

LRESULT CALLBACK _MainWnd_WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) ;


/******************************************************************/
/* Exported function : RegisterClass                              */
/******************************************************************/

BOOL MainWnd_RegisterClass (HINSTANCE hInstance) 
{
  WNDCLASS	wndclass ;

  wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
  wndclass.lpfnWndProc   = _MainWnd_WndProc ;
  wndclass.cbClsExtra    = 0 ;
  wndclass.cbWndExtra    = 0 ;
  wndclass.hInstance     = hInstance ;
  wndclass.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_APP)) ;
  wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
  wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1) ;
  wndclass.lpszMenuName  = NULL ;
  wndclass.lpszClassName = WC_MAINWND ;

  return RegisterClass (&wndclass) ;
}


/******************************************************************/
/* Exported function : CreateWindow                               */
/******************************************************************/

HWND MainWnd_CreateWindow (HINSTANCE hInstance)
{  
  return CreateWindow (WC_MAINWND, 
		       TEXT(APPLICATION_NAME), 
		       WS_OVERLAPPEDWINDOW,
		       CW_USEDEFAULT, CW_USEDEFAULT, 
		       CW_USEDEFAULT, CW_USEDEFAULT,
		       NULL, NULL, hInstance, NULL) ;
}



/******************************************************************/
/* Exported function : WndProc                                    */
/******************************************************************/

LRESULT CALLBACK _MainWnd_WndProc (HWND hwnd, UINT message, 
				   WPARAM wParam, LPARAM lParam)
{  
  static HINSTANCE	g_hInstance ;
  static UINT		g_uTaskbarCreatedMsg ;
  static HWND		g_aTabs[_TAB_COUNT] ;
  static int		g_iSelectedTab ;
  static HWND		g_hwndMenu ;
  static BOOL		g_bNewVersionAvailable ;
  static HUPDWATCHER	g_hUpdWatcher ;

  EVENTSTRUCT		*pEvent ;
  TCHAR			szBuffer[1024] ;
  int			i, n ;
  RECT			rect ;

  switch (message)
    {
    case WM_CREATE:
      
      g_hInstance = ((CREATESTRUCT*)lParam)->hInstance ;

      g_hUpdWatcher = UpdWatcher_New (hwnd) ;
                 
      // register "TaskbarCreated" message
      g_uTaskbarCreatedMsg = RegisterWindowMessage (TEXT("TaskbarCreated")) ;    
      	
      return 0 ; // case WM_CREATE:

    case WM_ENDSESSION:
      
      TRACE_INFO (TEXT("System shutdown\n")) ;
      
      SplashWnd_Show () ;
      
      SpySrv_Stop () ;  
      SpySrv_WriteFilterFile () ;
      Config_Uninit () ;
      LogFile_Uninit () ;

      SplashWnd_Hide (FALSE) ;

      return 0 ;

    case WM_DESTROY: 

      UpdWatcher_Delete (g_hUpdWatcher) ;
      
      // stop application
      PostQuitMessage (0) ;

      return 0 ; // case WM_DESTROY:   
      
    case WM_LANGUAGECHANGED:

      // update menu
      PicMenuCtl_SetItemText (g_hwndMenu, TAB_FILTERS,	STR(_FILTERS)) ; 
      PicMenuCtl_SetItemText (g_hwndMenu, TAB_HISTORY,	STR(_HISTORY)) ; 
      PicMenuCtl_SetItemText (g_hwndMenu, TAB_PROCESSES, STR(_PROCESSES)) ; 
      PicMenuCtl_SetItemText (g_hwndMenu, TAB_SCANCACHE, STR(_TRUSTED_FILES)) ; 
      PicMenuCtl_SetItemText (g_hwndMenu, TAB_CONFIG,	STR(_CONFIGURATION)) ; 
      PicMenuCtl_SetItemText (g_hwndMenu, TAB_ABOUT,	STR(_ABOUT)) ; 

      // spread message
      for( i=0 ; i<_TAB_COUNT ; i++ ) 
	SendMessage (g_aTabs[i], message, wParam, lParam) ;

      // also update tray icon
      TrayIcon_LanguageChanged () ;

      return 0 ; // case WM_LANGUAGECHANGED:

    case WM_UPDATE_FOUND:
      {    
	g_bNewVersionAvailable = TRUE ;

	TrayIcon_Alert (STR_DEF(_NEW_VERSION_AVAILABLE,g_szNewVersionAvailable)) ;
      }
      return 0 ;

    case WM_SHOWWINDOW:

      if( wParam ) // window is being shown
	{
	  // create menu window
	  g_hwndMenu = PicMenuCtl_CreateWindow (g_hInstance, hwnd, WS_VISIBLE) ;

	  PicMenuCtl_AddItem (g_hwndMenu, TAB_FILTERS, 
			      STR_DEF (_FILTERS, TEXT("Filters")),
			      MAKEINTRESOURCE(IDB_FILTERS)) ;
	  
	  PicMenuCtl_AddItem (g_hwndMenu, TAB_HISTORY,
			      STR_DEF (_HISTORY,TEXT("History")),
			      MAKEINTRESOURCE(IDB_HISTORY)) ;

	  PicMenuCtl_AddItem (g_hwndMenu, TAB_PROCESSES, 
			      STR_DEF (_PROCESSES,TEXT("Processes")),
			      MAKEINTRESOURCE(IDB_PROCESSES)) ;

	  PicMenuCtl_AddItem (g_hwndMenu, TAB_SCANCACHE,
			      STR_DEF (_SCANNER_CACHE,TEXT("Scanner cache")),
			      MAKEINTRESOURCE(IDB_SCANCACHE)) ;

	  PicMenuCtl_AddItem (g_hwndMenu, TAB_CONFIG,
			      STR_DEF (_CONFIGURATION,TEXT("Configuration")),
			      MAKEINTRESOURCE(IDB_CONFIGURATION)) ;

	  PicMenuCtl_AddItem (g_hwndMenu, TAB_ABOUT,
			      STR_DEF (_ABOUT,TEXT("About")),
			      MAKEINTRESOURCE(IDB_ABOUT)) ;

	  PicMenuCtl_SelectItem (g_hwndMenu, TAB_FILTERS) ;
			      

	  g_aTabs[TAB_FILTERS] = FilterWnd_CreateWindow (g_hInstance, hwnd) ;
	  ASSERT (g_aTabs[TAB_FILTERS]!=NULL) ;

	  g_aTabs[TAB_HISTORY] = HistoryWnd_CreateWindow (g_hInstance, hwnd) ;
	  ASSERT (g_aTabs[TAB_HISTORY]!=NULL) ;

	  g_aTabs[TAB_PROCESSES] = ProcListWnd_CreateWindow (g_hInstance, hwnd) ; 
	  ASSERT (g_aTabs[TAB_PROCESSES]!=NULL) ;

	  g_aTabs[TAB_SCANCACHE] = ScanCacheWnd_CreateWindow (g_hInstance, hwnd) ; 
	  ASSERT (g_aTabs[TAB_SCANCACHE]!=NULL) ;

	  g_aTabs[TAB_CONFIG] = ConfigWnd_CreateWindow (g_hInstance, hwnd) ; 
	  ASSERT (g_aTabs[TAB_CONFIG]!=NULL) ;

	  g_aTabs[TAB_ABOUT] = AboutWnd_CreateWindow (g_hInstance, hwnd) ; 
	  ASSERT (g_aTabs[TAB_ABOUT]!=NULL) ;

	  g_iSelectedTab = 0 ;
	  ShowWindow (g_aTabs[g_iSelectedTab], SW_SHOW) ;

	  // send a WM_SIZE message
	  // to resize tabs
	  GetClientRect (hwnd, &rect) ;
	  SendMessage (hwnd, WM_SIZE, 0, 
		       MAKELPARAM(rect.right-rect.left,
				  rect.bottom-rect.top)) ;

	  if( g_bNewVersionAvailable )
	    {
	      wsprintf (szBuffer, STR_DEF(_DOWNLOAD_VERSION_S, g_szDownloadVersionS),
			UpdWatcher_GetNewVersion(g_hUpdWatcher)) ;
	      
	      if( IDYES==MessageBox(hwnd,szBuffer,TEXT(APPLICATION_NAME),
				    MB_ICONQUESTION|MB_YESNO) )
		ShellExecute (hwnd, NULL, UpdWatcher_GetDownloadPage(g_hUpdWatcher), NULL, NULL, SW_SHOW) ;
	      g_bNewVersionAvailable = FALSE ;
	    }
	}
      else // window is being hidden
	{
	  for( i=0 ; i<_TAB_COUNT ; i++ ) {
	    DestroyWindow (g_aTabs[i]) ;
	    g_aTabs[i] = NULL ;
	  }

	  DestroyWindow (g_hwndMenu) ;
	}

      return 0 ; // case WM_SHOWWINDOW:

    case WM_CLOSE:

      if( 0==Config_GetInteger(CFGINT_SKIP_FIRST_CLOSE_WARNING) )
	{
	  Config_SetInteger(CFGINT_SKIP_FIRST_CLOSE_WARNING, 1) ;
	  MessageBox (hwnd, 
		      STR_DEF(_FIRST_CLOSE_WARNING,g_szFirstCloseWarning), 
		      TEXT(APPLICATION_NAME), MB_ICONWARNING) ;
	}

      ShowWindow (hwnd, SW_HIDE) ;

      return 0 ; // case WM_CLOSE:

    case WM_SIZE:

      MoveWindow (g_hwndMenu, 
		  CX_SPACE, CY_SPACE, 
		  CX_MENU, HIWORD(lParam)-2*CY_SPACE, TRUE) ;

      // move tab windows
      for( i=0 ; i<_TAB_COUNT ; i++ )
	MoveWindow (g_aTabs[i], 
		    CX_MENU+2*CX_SPACE, CY_SPACE, 
		    LOWORD(lParam)-CX_MENU-3*CX_SPACE, HIWORD(lParam)-2*CY_SPACE, TRUE) ; 
  
      return 0 ; // case WM_SIZE:
      
      
    case WM_SPYNOTIFY:
      {
	switch( wParam )
	  {
	  case SN_ALERT:

	    pEvent = EventLog_MapEvent (lParam) ;

	    if( ! pEvent ) {
	      TRACE_ERROR (TEXT("EventLog_MapEvent failed\n")) ;
	      break ;
	    }

	    if( Language_IsLoaded() )
	      {
		n = wsprintf (szBuffer, 
			      pEvent->nReaction==RULE_REJECT ? STR(_REJECTED_FROM_U_S) : 
			      pEvent->nReaction==RULE_FEIGN ? STR(_FEIGNED_FROM_U_S) : 
			      STR(_ACCEPTED_FROM_U_S),
			      pEvent->dwProcessId, pEvent->szExeName) ;
		szBuffer[n++] = TEXT('\n') ;
		szBuffer[n] = 0 ;
	      }
	    else
	      {
		n = wsprintf (szBuffer, TEXT("Winpooch has just %s the following function from process %u (%s) :\n"),
			      pEvent->nReaction==RULE_REJECT ? TEXT("rejected") : 
			      pEvent->nReaction==RULE_FEIGN ? TEXT("feigned") : 
			      TEXT("accepted"),
			      pEvent->dwProcessId, pEvent->szExeName) ;	
	      }

	    FiltCond_ToString (&pEvent->condition, szBuffer+n, 1024-n) ;

	    EventLog_UnmapEvent (lParam) ;

	    TrayIcon_Alert (szBuffer) ;

	    break ;

	  case SN_EVENTLOGGED:    
	    SendMessage (g_aTabs[TAB_HISTORY], WM_UPDATEHISTORY, 0, 0) ;
	    break ;    
	    
	  case SN_FILTERCHANGED:
	    SendMessage (g_aTabs[TAB_FILTERS], WM_UPDATEFILTERS, 0, 0) ;
	    break ;

	  case SN_PROCESSCREATED:
	  case SN_PIDCHANGED:
	  case SN_PROCESSTERMINATED:
	  case SN_PROCESSCHANGED:
	    SendMessage (g_aTabs[TAB_PROCESSES], message, wParam, lParam) ;
	    break ;
	  }
      }
      return 0 ;


    case WM_COMMAND:

      if( HIWORD(wParam)==0 )  // menu command
	{
	  if( LOWORD(wParam) < _TAB_COUNT )
	    {
	      ShowWindow (g_aTabs[g_iSelectedTab], SW_HIDE) ;
	      g_iSelectedTab = LOWORD(wParam) ;
	      ShowWindow (g_aTabs[g_iSelectedTab], SW_SHOW) ;
	      return 0 ;
	    }

	  switch( LOWORD(wParam) )
	    {     
	    case IDM_OPEN:
	      ShowWindow (hwnd, SW_RESTORE) ;   
	      SetForegroundWindow (hwnd) ; 
	      TrayIcon_SetState (TIS_NORMAL) ;
	      break ;

	    case IDM_SHUTDOWN:
	      DestroyWindow (hwnd) ;
	      break ;
	    }
	  return 0 ;
	} // menu command

      break ; // case WM_COMMAND:


    case WM_MOUSEWHEEL:

      return SendMessage (g_hwndMenu, message, wParam, lParam) ;


    case WM_TRAYICON:
      
      switch( lParam ) 
	{
	case WM_RBUTTONDOWN:
	  TrayIcon_TrackPopupMenu () ;
	  break ;

	case WM_LBUTTONDOWN:

	  if( IsIconic(hwnd) )
	    {
	      ShowWindow (hwnd, SW_RESTORE) ;
	      SetForegroundWindow (hwnd) ;
	    }
	  else if( IsWindowVisible(hwnd) ) 
	    {
	      ShowWindow (hwnd, SW_HIDE) ;
	    } 
	  else 
	    {	    
	      ShowWindow (hwnd, SW_SHOW) ;
	      SetForegroundWindow (hwnd) ;
	    }
	  TrayIcon_SetState (TIS_NORMAL) ;
	  break ;	 
	} 

      return 0 ; // case WM_TRAYICON:

    default:

      if( message == g_uTaskbarCreatedMsg )
	TrayIcon_RestoreIcon () ; 
    }

  return DefWindowProc (hwnd, message, wParam, lParam) ;
}


