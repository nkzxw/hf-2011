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

#define TRACE_LEVEL 2	// warning level


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// standard headers
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <stdio.h>
#include <tchar.h>

// libraries' headers
#define _WINDOWS_ // <-- workaround needed for freeimage 3.9.1
#include <FreeImage.h>

// project's headers
#include "AboutWnd.h"
#include "AskDlg.h"
#include "Assert.h"
#include "BkgndScan.h"
#include "FiltCond.h"
#include "Config.h"
#include "ConfigWnd.h"
#include "EventLog.h"
#include "Filter.h"
#include "FilterSet.h"
#include "FilterWnd.h"
#include "HistoryWnd.h"
#include "IncompReport.h"
#include "Language.h"
#include "LogFile.h"
#include "MainWnd.h"
#include "PicBtnCtl.h"
#include "PicMenuCtl.h"
#include "ProcList.h"
#include "ProcListWnd.h"
#include "ProjectInfo.h"
#include "Resources.h"
#include "RuleDlg.h"
#include "ScanCacheWnd.h"
#include "Scanner.h"
#include "Sounds.h"
#include "SplashWnd.h"
#include "SpyServer.h"
#include "Trace.h"
#include "TrayIcon.h"


typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);



#define LOG_FILE_NAME (APPLICATION_NAME TEXT(".log"))

TCHAR szProcessGuardAlert[] = 
  TEXT("Winpooch has detected that ProcessGuard is installed on your computer.\n"
       "\nIn order to make Winpooch works correctly, you have to remove "
       "protections over Winpooch's executable.\n"
       "Please set the following configuration :\n"
       "- Uncheck \"Protect this application from modification\"\n"
       "- Check \"Authorize this application to modify protected applications\"") ;

TCHAR szUninstProcessGuardNotify[] =
  TEXT("Winpooch has detected that ProcessGuard has been removed from your computer.\n"
       "\nYou decided to replace PG by Winpooch : great !") ;

TCHAR szRegDefendAlert[] = 
  TEXT("Winpooch has detected that RegDefend is installed on your computer.\n"
       "\nThere is no known incompatibility between RegDefend and Winpooch.\n"
       "But since they both make the same job, you may encounter problems.") ;

TCHAR szUninstRegDefendNotify[] =
  TEXT("Winpooch has detected that RegDefend has been removed from your computer.\n"
       "\nYou decided to replace RD by Winpooch : great !") ;

TCHAR szCantRunUnderWin64[] =
  TEXT("Winpooch can't work on Windows 64-bits.\n"
       "This will be added in future version.") ;

TCHAR szOldWindowsAlert[] =
  TEXT("This program requires Windows 2000 or higher.") ;

TCHAR szFailedLogInitAlert[] =
  TEXT("Failed to initialize log file.") ;     

TCHAR szAlreadyRunningAlert[] =
  TEXT("Winpooch is already running") ;

TCHAR szLanguageInitAlert[] =
  TEXT("Failed to initialize language module.\n"
       "Please verify that language packs are correctly installed.") ;

TCHAR g_szDriverInstalledTemporary[] =
  TEXT("Winpooch's driver was not present in kernel.\n"
       "It has been installed temporarily and will be removed when Winpooch closes.\n"
       "To prevent this, please install Winpooch with automatic setup, or run \"winpooch install\".") ;
     

BOOL IsDriverPresent (LPCTSTR szPath) 
{
  HANDLE	hDevice ;

  TRACE_INFO (TEXT("Looking for %s\n"), szPath) ;

  hDevice = CreateFile (szPath, 0, FILE_SHARE_WRITE|FILE_SHARE_READ,
			NULL, OPEN_EXISTING, 0, NULL) ;
  
  if( hDevice==INVALID_HANDLE_VALUE )
    {
      TRACE_INFO (TEXT("CreateFile failed (error=%d)\n"), GetLastError()) ;
      return ERROR_FILE_NOT_FOUND!=GetLastError() ;
    }

  CloseHandle (hDevice) ;
  
  TRACE_INFO (TEXT("Found %s\n"), szPath) ;
  
  return TRUE ;
}

BOOL DetectProcessGuard ()
{
  return IsDriverPresent (TEXT("\\\\.\\procguard")) ;
}

BOOL DetectRegDefend ()
{
  return IsDriverPresent (TEXT("\\\\.\\regdefend")) ;
}

BOOL DetectWinpoochDriver ()
{
  return IsDriverPresent (TEXT("\\\\.\\winpooch")) ;
}

BOOL DetectWin64 () 
{
  BOOL bResult ;

  LPFN_ISWOW64PROCESS pfnIsWow64Process =
    (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")),"IsWow64Process");
 
  if( ! pfnIsWow64Process)
    return FALSE ;

  if (!pfnIsWow64Process(GetCurrentProcess(),&bResult))
    return FALSE ;

  return bResult ;
}


BOOL DetectW2kOrHigher()  
{
  OSVERSIONINFO	osInfo = { sizeof(OSVERSIONINFO) } ;
  
  GetVersionEx (&osInfo) ;
  
  return osInfo.dwMajorVersion >= 5 ;
}



int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)
{
  MSG		msg ;
  HWND		hwnd ;
  HANDLE	hMutex ;
  BOOL		bRemoveDriverAfterClose = FALSE ;

  //
  // Read command line
  //
  {
    LPWSTR	*argv ;
    int		argc ; 
    TCHAR	szBuffer[256] ;

    argv = CommandLineToArgvW (GetCommandLineW(), &argc) ;

    if( argc > 1 )
      {	
	if( ! _tcsicmp(argv[1],TEXT("install")) )
	  {
	    if( ! SpySrv_InstallDriver(TRUE) )
	      {
		wsprintf (szBuffer, TEXT("Driver installation failed (error=%d):\n"),
			  GetLastError()) ;

		if( GetLastError() == ERROR_GEN_FAILURE )
		  {
		    LPCTSTR szFilename = TEXT("Winpooch_Incompatibility.txt") ;
		    _tcscat (szBuffer, TEXT("This version of Windows is not supported. A report has been generated, please send it as specified in the report\n")) ;
		    IncompReport_Generate (szFilename) ;
		    ShellExecute (NULL, TEXT("open"), szFilename, NULL, NULL, SW_SHOWNORMAL) ;
		  }
		else
		  {
		    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
				  NULL,
				  GetLastError(),
				  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				  szBuffer+_tcslen(szBuffer),
				  256-_tcslen(szBuffer), NULL) ;		   
		  }

		if( _tcsicmp(argv[2],TEXT("silent")) )
		  MessageBox (NULL, szBuffer, TEXT(APPLICATION_NAME), MB_ICONERROR) ;

		return 1 ;
	      }
	  }
	else if( ! _tcsicmp(argv[1],TEXT("uninstall")) )
	  {
	    if( ! SpySrv_UninstallDriver() )
	      {
		wsprintf (szBuffer, TEXT("Driver uninstallation failed :\n")) ;
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
			      NULL,
			      GetLastError(),
			      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			      szBuffer+_tcslen(szBuffer),
			      256-_tcslen(szBuffer), NULL) ;

		if( _tcsicmp(argv[2],TEXT("silent")) )
		  MessageBox (NULL, szBuffer, TEXT(APPLICATION_NAME), MB_ICONERROR) ;

		return 1 ;
	      }
	  }
	else if( ! _tcsicmp(argv[1],TEXT("report")) )
	  {
	    LPCTSTR szFilename = TEXT("Winpooch_Incompatibility.txt") ;
	    IncompReport_Generate (szFilename) ;
	    ShellExecute (NULL, TEXT("open"), szFilename, NULL, NULL, SW_SHOWNORMAL) ;
	    return 1 ;
	  }
	else
	  {
	    wsprintf (szBuffer, TEXT("Unrecognize command line option \"%s\"\n"), argv[1]) ;
	    MessageBox (NULL, szBuffer, TEXT(APPLICATION_NAME), MB_ICONERROR) ;
	    return 1 ;
	  }

	return 0 ; 
      }
  }

  // init config module
  Config_Init () ;
  
  // init language module
  if( ! Language_Init () )
    MessageBox (NULL, szLanguageInitAlert,
		TEXT(APPLICATION_NAME), MB_ICONWARNING) ;

  // verify that we are not running on a 64-bits Windows
  if( DetectWin64() ) 
    {
      MessageBox (NULL, 
		  STR_DEF(_CANT_RUN_WITH_WIN64,szCantRunUnderWin64), 
		  TEXT(APPLICATION_NAME), 
		  MB_ICONERROR) ;
      return 0 ;
    }

  // verify that we are running on a w2k or higher
  if( ! DetectW2kOrHigher() ) {
    MessageBox (NULL, szOldWindowsAlert, 
		TEXT(APPLICATION_NAME), MB_ICONERROR) ;
    return 0 ;
  }

  // set mutex to avoid multiple instance
  hMutex = CreateMutex (NULL, TRUE, TEXT(APPLICATION_NAME)) ;

  // an instance is already running ?
  if( GetLastError()!=0 )
    {
      MessageBox (NULL, 
		  STR_DEF(_ALREADY_RUNNING,szAlreadyRunningAlert), 
		  TEXT(APPLICATION_NAME), MB_ICONINFORMATION) ;
      Config_Uninit ();
      return 0 ;
    }
  
  // alert user if PG is installed
  if( DetectProcessGuard() )
    {
      if( !Config_GetInteger(CFGINT_PROCGUARD_DETECTED) )
	{
	  MessageBox (NULL, szProcessGuardAlert, 
		      TEXT(APPLICATION_NAME), MB_ICONWARNING|MB_SETFOREGROUND) ;
	  Config_SetInteger(CFGINT_PROCGUARD_DETECTED,1) ;
	}
    }
  else
    {
      if( Config_GetInteger(CFGINT_PROCGUARD_DETECTED) )
	{
	  MessageBox (NULL, szUninstProcessGuardNotify, 
		      TEXT(APPLICATION_NAME), MB_ICONINFORMATION|MB_SETFOREGROUND) ;
	  Config_SetInteger(CFGINT_PROCGUARD_DETECTED,0) ;
	}
    }

  // aler user if RegDefend is installed
  if( DetectRegDefend() )
    {
      if( !Config_GetInteger(CFGINT_REGDEFEND_DETECTED) )
	{
	  MessageBox (NULL, szRegDefendAlert, 
		      TEXT(APPLICATION_NAME), MB_ICONWARNING|MB_SETFOREGROUND) ;
	  Config_SetInteger(CFGINT_REGDEFEND_DETECTED, 1) ;
	}
    }
  else
    {
      if( Config_GetInteger(CFGINT_REGDEFEND_DETECTED) )
	{
	  MessageBox (NULL, szUninstRegDefendNotify, 
		      TEXT(APPLICATION_NAME), MB_ICONINFORMATION|MB_SETFOREGROUND) ;
	  Config_SetInteger(CFGINT_REGDEFEND_DETECTED, 0) ;
	}
    }

  // initialize FreeImage library
  FreeImage_Initialise (FALSE) ; 

  // show splash screen
  SplashWnd_RegisterClass (hInstance) ;
  SplashWnd_Show () ;

  InitCommonControls () ;  

  AboutWnd_RegisterClass (hInstance) ;
  ConfigWnd_RegisterClass (hInstance) ;
  FilterWnd_RegisterClass (hInstance) ;
  HistoryWnd_RegisterClass (hInstance) ;
  MainWnd_RegisterClass	(hInstance) ;
  PicBtnCtl_RegisterClass (hInstance) ;
  PicMenuCtl_RegisterClass (hInstance) ;
  ProcListWnd_RegisterClass(hInstance) ;
  ScanCacheWnd_RegisterClass (hInstance) ;
  
  hwnd = MainWnd_CreateWindow (hInstance) ;
  Language_SetHwnd (hwnd) ;

  // initialize sounds
  Sounds_Init (hInstance) ;

  // create tray icon
  TrayIcon_Create (hInstance, hwnd) ;

  if( ! DetectWinpoochDriver() )
    {
      if( ! SpySrv_InstallDriver(FALSE) )
	{
	  TCHAR	szBuffer[256] ;
	  wsprintf (szBuffer, TEXT("Driver installation failed (error=%d):\n"),
		    GetLastError()) ;
	  if( GetLastError() == ERROR_GEN_FAILURE )
	    {
	      _tcscat (szBuffer, TEXT("This version of Windows is not supported\n")) ;
	    }
	  else
	    {
	      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
			    NULL,
			    GetLastError(),
			    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			    szBuffer+_tcslen(szBuffer),
			    256-_tcslen(szBuffer), NULL) ;
	    }
	  MessageBox (NULL, szBuffer, TEXT(APPLICATION_NAME), MB_ICONERROR) ;
	  return 1 ;
	}

      TrayIcon_Alert (g_szDriverInstalledTemporary) ;   
      bRemoveDriverAfterClose = TRUE ;
    }
  
  if( ! LogFile_Init (LOG_FILE_NAME) )
    TrayIcon_Alert (szFailedLogInitAlert) ;    

  ProcList_Init () ;
  EventLog_Init () ;
  AskDlg_Init () ;

  Scanner_Init () ;

  if( ! SpySrv_Init (hwnd) )
    {
      MessageBox (NULL, TEXT("FATA ERROR : Failed to initialize spy server !!!\n"),
		  TEXT(APPLICATION_NAME), MB_ICONERROR) ;
      return 0 ;
    }  

  SpySrv_ReadFilterFile () ; 
  SpySrv_Start () ;

  BkgndScan_Start () ;

  TrayIcon_EnableMenuItem (IDM_OPEN, TRUE) ;
  TrayIcon_EnableMenuItem (IDM_SHUTDOWN, TRUE) ;

  SetOnAssertFailed ((PROC)SpySrv_Stop) ;

  SplashWnd_Hide (FALSE) ;

  while( GetMessage (&msg,NULL,0,0) )
    {
      TranslateMessage (&msg) ;
      DispatchMessage (&msg) ;
    }

  SplashWnd_Show () ;

  TrayIcon_EnableMenuItem (IDM_OPEN, FALSE) ;
  TrayIcon_EnableMenuItem (IDM_SHUTDOWN, FALSE) ;
  
  SetOnAssertFailed (NULL) ;

  SpySrv_Stop () ; 
  SpySrv_WriteFilterFile () ;
  SpySrv_Uninit () ;

  if( bRemoveDriverAfterClose )
    {
      TRACE_ALWAYS (TEXT("REMOVING DRIVER\n")) ;
      if( ! SpySrv_UninstallDriver() )
	{
	  TCHAR	szBuffer[256] ;
	  wsprintf (szBuffer, TEXT("Driver uninstallation failed :\n")) ;
	  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			szBuffer+_tcslen(szBuffer),
			256-_tcslen(szBuffer), NULL) ;
	  MessageBox (NULL, szBuffer, TEXT(APPLICATION_NAME), MB_ICONERROR) ;
	  return 1 ;
	}
    }
  
  BkgndScan_Stop () ;
  Scanner_Uninit () ;
  AskDlg_Uninit() ;
  EventLog_Uninit () ;
  ProcList_Uninit () ;
  LogFile_Uninit () ;
  FreeImage_DeInitialise () ; 
  Language_Uninit () ;
  Config_Uninit () ;

  SplashWnd_Hide (TRUE) ;
  TrayIcon_Destroy () ;
  Sounds_Uninit () ;

  CloseHandle (hMutex) ;
 
  return msg.wParam ;
}
