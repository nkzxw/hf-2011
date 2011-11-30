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


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "SpyServer.h"

// Windows headers
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <winioctl.h>

// project's headers
#include "AskDlg.h"
#include "Assert.h"
#include "Config.h"
#include "DrvInterface.h"
#include "EventLog.h"
#include "Filter.h"
#include "FilterDefault.h"
#include "FilterFile.h"
#include "FilterSet.h"
#include "Language.h"
#include "Link.h"
#include "Malloc.h"
#include "ProcList.h"
#include "ProcListWnd.h"
#include "ProjectInfo.h"
#include "Reason.h"
#include "Resources.h"
#include "FiltRule.h"
#include "RuleDlg.h"
#include "Scanner.h"
#include "Sounds.h"
#include "Strlcpy.h"
#include "TrayIcon.h"
#include "Trace.h"
#include "VirusDlg.h"


/******************************************************************/
/* Internal macros                                                */
/******************************************************************/

#define arraysize(A) (sizeof(A)/sizeof((A)[0]))


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

TCHAR szFilterFilename[] = TEXT("Current.wpf") ;

TCHAR g_szServiceName[] = TEXT("Winpooch") ;

TCHAR g_szDriverFileName[] = TEXT("Winpooch.sys") ;

TCHAR szNoFiltersAlert[] = 
  TEXT("No filter file has been found, default filters will be used.\n"
       "You can ignore this alert if you are running Winpooch for the first time.") ;

TCHAR szFiltersErrorAlert[] =
  TEXT("Error reading file %s :\n%s\nDefault filters will be used.") ;

TCHAR szOldFiltersCleared[] =
  TEXT("Filters from a previous version have been found but can't be used.\n"
       "The file has been backed up but filters have been cleared.") ;


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef LPTHREAD_START_ROUTINE THREADPROC ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static HINSTANCE	g_hInstance = NULL ;
static HWND		g_hwndMain = NULL ;
static HFILTERSET	g_hFilterSet = NULL ;
static HANDLE		g_hFilterMutex = NULL ;
static HANDLE		g_hDriver = NULL ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

// Notification function called by spy DLL
DWORD _SpySrv_RequestFromDriver (LPVOID, DWORD) ;

// initialize spy dll
BOOL _SpySrv_InitDriver () ;

// destroy filters
BOOL _SpySrv_UninitFilters () ;

// uninitialize spy dll
VOID _SpySrv_UninitDriver () ;

BOOL _SpySrv_RefreshProcList () ;



/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _SpySrv_Log (PROCADDR nProcessAddr, PCFILTCOND pCond,
		  DWORD nReaction, BOOL bAlert)
{
  UINT		nProcessId ;
  DWORD		dwEventId ;
  TCHAR		szProcess[MAX_PATH] = TEXT("???") ;
  
  TRACE ; 

  // get process name
  {
    PROCSTRUCT * pProc ;

    ProcList_Lock () ;
    pProc = ProcList_Get (nProcessAddr) ;

    nProcessId = pProc->nProcessId ;

    if( pProc ) {
      wcslcpy (szProcess, pProc->szPath, MAX_PATH) ;
    } else {
      wsprintf (szProcess, TEXT("Process %d"), nProcessId) ;
      TRACE_WARNING (TEXT("Process %d not in list (addr=0x08X)\n"), 
		     nProcessId, nProcessAddr) ;
    }

    ProcList_Unlock () ;
  }
    
  dwEventId = EventLog_Add (nProcessId, szProcess, nReaction,
			    bAlert ? RULE_ALERT : RULE_LOG,
			    pCond) ;
  
  if( bAlert ) 
    {
      Sounds_Play (SOUND_ALERT) ;
      PostMessage (g_hwndMain, WM_SPYNOTIFY, SN_ALERT, dwEventId) ;
    }
  
  PostMessage (g_hwndMain, WM_SPYNOTIFY, SN_EVENTLOGGED, dwEventId) ;

  return TRUE ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

DWORD _SpySrv_Ask (PROCADDR nProcessAddress, UINT nDefReaction, PFILTCOND pCond)
{
  TCHAR		szProcess[MAX_PATH] = TEXT("???") ;
  UINT		nProcessId ;
  int		nResult ;  
  
  TRACE ; 
  
  // get process name
  {
    PROCSTRUCT * pProc ;
    
    ProcList_Lock () ;
    pProc = ProcList_Get (nProcessAddress) ;

    nProcessId = pProc->nProcessId ;
    
    if( pProc ) {
      wcslcpy (szProcess, pProc->szPath, MAX_PATH) ;
    } else {
      wsprintf (szProcess, TEXT("Process %d"), nProcessId) ;
      TRACE_WARNING (TEXT("Process %d not in list (addr=0x%08X\n"), 
		     nProcessId, nProcessAddress) ;
    }
    
    ProcList_Unlock () ;
  }
  
  Sounds_Play (SOUND_ASK) ;
  
  TRACE_INFO (TEXT("  /----ASK----\\\n")) ;

 askdlg:

  nResult = AskDlg_DialogBox (g_hInstance, g_hwndMain, szProcess, nProcessId, nDefReaction, pCond) ;

  if( nResult==ASKDLG_CREATEFILTER )
    {
      FILTRULE	* pRule ;
	  
      // alloc new rule
      pRule = (FILTRULE*) calloc (1, sizeof(FILTRULE)) ;

      // fill params as they appear in the ask dialog
      pRule->nReaction	= nDefReaction ;
      pRule->nVerbosity	= RULE_LOG ;
      pRule->nOptions	= 0 ;
      FiltCond_Dup (&pRule->condition, pCond) ;
      
      // show rule dialog
      if( IDOK!=RuleDlg_DialogBox (g_hInstance, g_hwndMain, szProcess, pRule, FALSE) )
	{
	  free (pRule) ;
	  goto askdlg ;
	}

      // verify that this rule matches the current event
      if( ! FiltCond_Check(&pRule->condition,pCond) )
	{
	  MessageBox (g_hwndMain, 
		      STR_DEF(_RULE_DOESNT_MATCH,
			      TEXT("The rule you defined doesn't match current event")), 
		      TEXT(APPLICATION_NAME), MB_ICONERROR) ;
	  free (pRule) ;
	  goto askdlg ;	      
	}

      SpySrv_AddRuleForProgram (pRule, szProcess) ;
      
      nResult = pRule->nReaction ;
    }

  if( nResult==ASKDLG_UNHOOK )
    {
      SpySrv_IgnoreProcess (nProcessAddress, TRUE) ;    
      nResult = RULE_ACCEPT ;
    }

  TRACE_INFO (TEXT("  \\----ASK-%d--/\n"), nResult) ;

  return nResult ;
}

/******************************************************************/
/*                                                                */
/* Return true if file open is allowed                            */
/******************************************************************/

SCANRESULT SpySrv_ScanFile (LPCTSTR szFilePath, BOOL bBackground)
{
  TCHAR	szOutput[1024] ;
  UINT	nResult ;  
  VIRUSDLGPARAMS dlgparams = {
    szFilePath,
    szOutput
  } ;

  TRACE_INFO (TEXT("Will scan file %ls\n"), szFilePath) ;
  
  if( ! Scanner_IsConfigured() )
    return SCAN_NOT_SCANNED ;
  
  nResult = Scanner_ScanFile (szFilePath,szOutput,1024,bBackground) ; 

  TRACE_INFO (TEXT("Scan result = %d\n"), nResult) ;
  
  if( nResult != SCAN_NO_VIRUS )
    {
      UINT nReaction ;

      Sounds_Play (SOUND_VIRUS) ;
      
      nReaction = DialogBoxParam (g_hInstance, MAKEINTRESOURCE(DLG_VIRUS), g_hwndMain, 
				  VirusDlg_DlgProc, (LPARAM)&dlgparams) ;     

      if( nReaction == RULE_ACCEPT )
	nResult++ ;	
    } 
    
  return nResult ;
}


/******************************************************************/
/* Internal function : SpyDllNotify                               */
/******************************************************************/

DWORD _SpySrv_RequestFromDriver (LPVOID pBuffer, DWORD nSize) 
{
  SDNMHDR	*p = pBuffer ;
  VOID		*pSerial ;
  UINT		nSerialSize ;
  DWORD		nResponseSize = 0 ;

  TRACE ; 

  ASSERT (pBuffer) ;
  ASSERT (nSize>0) ;
  ASSERT (nSize>=sizeof(SDNMHDR)) ;
  
  TRACE_INFO (TEXT(" /----REQ-%d----\\ (size=%d\n"), p->dwCode, nSize) ;

  switch( p->dwCode )
    {
    case SDN_ASK:
      {
	DWORD		nReaction ;
	FILTCOND	cond ;

	pSerial = ((SDNASK*)p)->data ;
	nSerialSize = nSize - sizeof(SDNASK) ;
	
	ASSERT (nSerialSize>0) ;

	if( ! FiltCond_Unserialize (&cond, pSerial, nSerialSize) )
	  {
	    TRACE_ERROR (TEXT("FiltCond_Unserialize failed\n")) ;
	    nReaction = RULE_ACCEPT ;	    
	  }
	else
	  {   
	    nReaction = _SpySrv_Ask (((SDNASK*)p)->nProcessAddress, 
				     ((SDNASK*)p)->nDefReaction, 
				     &cond) ;
	  }
	
	*((DWORD*)pBuffer) = nReaction ;
	nResponseSize = sizeof(DWORD) ;
      }
      break ;

    case SDN_LOG:
    case SDN_ALERT:
      {
	DWORD		nReaction ;
	FILTCOND	cond ;

	nReaction = ((SDNLOG*)p)->dwReaction ;

	pSerial = ((SDNLOG*)p)->data ;
	nSerialSize = nSize - sizeof(SDNLOG) ;

	if( ! FiltCond_Unserialize (&cond, pSerial, nSerialSize) )
	  {
	    TRACE_ERROR (TEXT("FiltCond_Unserialize failed\n")) ;
	    nReaction = RULE_ACCEPT ;	    
	  }
	else
	  {
	    _SpySrv_Log (((SDNLOG*)p)->nProcessAddress, &cond, nReaction, p->dwCode==SDN_ALERT) ;
	  }
      }
      break ;

    case SDN_SCANFILE:
      {
	DWORD		nScanResult ;

	nScanResult = SpySrv_ScanFile (((SDNSCANFILE*)pBuffer)->wszFilePath, FALSE) ;

	*((DWORD*)pBuffer) = nScanResult ;
	nResponseSize = sizeof(DWORD) ;
      }
      break ;

    case SDN_PROCESSCREATED:
      {
	SDNPROCESSCREATED * pSdnpc = pBuffer ;
	PROCSTRUCT	proc ;
	
	proc.nProcessAddress	= pSdnpc->nProcessAddress ;
	proc.nProcessId		= pSdnpc->nProcessId ;
	proc.nState		= PS_HOOKED_SINCE_BIRTH ;
	wcslcpy (proc.szName, PathFindFileName(pSdnpc->wszFilePath), 32) ;
	wcslcpy (proc.szPath, pSdnpc->wszFilePath, MAX_PATH) ;
	
	ProcList_Lock () ;
	ProcList_Add (&proc) ;
	ProcList_Unlock () ;
	  
	PostMessage (g_hwndMain, WM_SPYNOTIFY, SN_PROCESSCREATED, pSdnpc->nProcessAddress) ;
      }
      break ;

    case SDN_PIDCHANGED:
      {
  	SDNPIDCHANGED	*pSdnpc = pBuffer ;
  	PROCSTRUCT	*pProc ;

	ProcList_Lock () ;	
	pProc = ProcList_Get (pSdnpc->nProcessAddress) ;
	if( pProc ) 
	  {
	    TRACE_ALWAYS (TEXT("PID changed %d -> %d\n"), pProc->nProcessId, pSdnpc->nNewProcessId) ; 
	    pProc->nProcessId = pSdnpc->nNewProcessId ;  
	  }
	ProcList_Unlock () ;

	// This notification has been disabled because it caused a dead-lock.
	// PostMessage (g_hwndMain, WM_SPYNOTIFY, SN_PIDCHANGED, pSdnpc->nProcessAddress) ;
      }
      break ;

    case SDN_PROCESSTERMINATED:
      {
	SDNPROCESSTERMINATED * pSdnpt = pBuffer ;

	TRACE_INFO (TEXT("Process terminated 0x%08X\n"),pSdnpt->nProcessAddress) ; 
	
	ProcList_Lock () ;
	ProcList_Remove (pSdnpt->nProcessAddress) ;
	ProcList_Unlock () ;
	  
	PostMessage (g_hwndMain, WM_SPYNOTIFY, SN_PROCESSTERMINATED, pSdnpt->nProcessAddress) ;
      }
      break ;

    default:

      TRACE_WARNING (TEXT("Driver request not handled (code=%d)\n"),  p->dwCode) ;
    }

  TRACE_INFO (TEXT(" \\----ANS------/\n")) ;

  return nResponseSize ;
}


/******************************************************************/
/* Exported function : GetFilterSet                               */
/******************************************************************/

HFILTERSET SpySrv_GetFilterSet () 
{
  TRACE ; 

  return g_hFilterSet ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL SpySrv_SendFilterSetToDriver ()
{
  DWORD		nMaxSize ;
  DWORD		nSize ;
  PVOID		pSerial ;
  BOOL		bSuccess ;
  DWORD		nWaitResult ;
  OVERLAPPED	ov ;

  ASSERT (g_hFilterSet!=NULL) ;

  nMaxSize = 1024*1024 ;
  pSerial = malloc (nMaxSize) ;

  nSize = FilterSet_Serialize (g_hFilterSet, pSerial, nMaxSize) ;

  if( ! nSize )
    {
      TRACE_ERROR(TEXT("FilterSet_Serialize failed\n")) ; 
      free (pSerial) ;
      return FALSE ;
    }  

  ov.hEvent = CreateEvent (NULL, TRUE, FALSE, NULL) ;

  bSuccess = DeviceIoControl (g_hDriver, 
			      IOCTL_SET_FILTERSET, 
			      pSerial, nSize,
			      NULL, 0, NULL, &ov) ;

  if( !bSuccess && GetLastError()==ERROR_IO_PENDING )
    {
      TRACE_WARNING (TEXT("IOCTL_SET_FILTERSET is running asynchronously\n")) ;

      nWaitResult = WaitForSingleObject (ov.hEvent, 10*1000) ;

      bSuccess = nWaitResult==WAIT_OBJECT_0 ;
    }

  CloseHandle (ov.hEvent) ;
  
  free (pSerial) ;

  if( ! bSuccess )
    TRACE_ERROR (TEXT("Failed to send filter set to driver\n")) ;
  
  return bSuccess ;
}

BOOL SpySrv_SetScannerExePath (LPCWSTR szScannerExe) 
{
  BOOL		bSuccess ;
  DWORD		nWaitResult ;
  OVERLAPPED	ov ;
  UINT		nSize ;

  ov.hEvent = CreateEvent (NULL, TRUE, FALSE, NULL) ;

  nSize = szScannerExe!=NULL ? (_tcslen(szScannerExe)+1)*sizeof(TCHAR) : 0 ;

  bSuccess = DeviceIoControl (g_hDriver, 
			      IOCTL_SET_SCANNER_PATH, 
			      (VOID*)szScannerExe, nSize,
			      NULL, 0, NULL, &ov) ;

  if( !bSuccess && GetLastError()==ERROR_IO_PENDING )
    {
      TRACE_WARNING (TEXT("IOCTL_SET_SCANNER_PATH is running asynchronously\n")) ;

      nWaitResult = WaitForSingleObject (ov.hEvent, 10*1000) ;

      bSuccess = nWaitResult==WAIT_OBJECT_0 ;
    }

  CloseHandle (ov.hEvent) ;
  
  if( ! bSuccess )
    TRACE_ERROR (TEXT("Failed to send scanner path to driver\n")) ;
  
  return bSuccess ;
}


/******************************************************************/
/* Internal function : InitDriver                                 */
/******************************************************************/

BOOL _SpySrv_InitDriver ()
{
  TRACE ;

  // open a handle on driver
  g_hDriver = CreateFile (TEXT("\\\\.\\WINPOOCH"),
			  GENERIC_READ|GENERIC_WRITE, 0,
			  NULL, OPEN_EXISTING, 
			  FILE_FLAG_OVERLAPPED, NULL) ;
  
  // ok ?
  if( g_hDriver==INVALID_HANDLE_VALUE || g_hDriver==NULL )
    {
      TRACE_ERROR(TEXT("CreateFile failed (0x%08X)\n"), GetLastError()) ;
      return FALSE ;
    }
 
  {
    LPCTSTR * pszFilters ;
    UINT nFilters ;
    
    pszFilters = Config_GetStringArray (CFGSAR_SCAN_PATTERNS, &nFilters) ;

    SpySrv_SetScanFilters (pszFilters, nFilters) ;
  }

  SpySrv_SetScannerExePath (Scanner_GetScannerExe()) ;

  return TRUE ;
}



/******************************************************************/
/* Internal function : UninitDriver                               */
/******************************************************************/

VOID _SpySrv_UninitDriver ()
{  
  TRACE ; 
  
  CloseHandle (g_hDriver) ;
}



/******************************************************************/
/* Exported function : Init                                       */
/******************************************************************/

BOOL SpySrv_Init (HWND hwndMain) 
{
  TRACE ; 

  g_hwndMain = hwndMain ;
  g_hInstance = (HINSTANCE) GetWindowLong (hwndMain, GWL_HINSTANCE) ;

  g_hFilterMutex = CreateMutex (NULL, FALSE, NULL) ;
 
  g_hFilterSet = FilterSet_Create (64) ;
  FilterSet_InitDefaultFilter (g_hFilterSet) ;

  // initialize 
  if( ! _SpySrv_InitDriver(g_hwndMain) )
    {
      TRACE_ERROR(TEXT("InitDriver failed (0x%08X)\n"), GetLastError()) ;
      CloseHandle (g_hFilterMutex) ;
      return FALSE ;
    }

  return TRUE ;
} 


/******************************************************************/
/* Exported function : Uninit                                     */
/******************************************************************/

VOID	SpySrv_Uninit () 
{  
  TRACE ; 

  _SpySrv_UninitDriver () ;

  _SpySrv_UninitFilters () ;

  CloseHandle (g_hFilterMutex) ;
}


/******************************************************************/
/* Exported function : Start                                      */
/******************************************************************/

BOOL SpySrv_Start (HWND hwndMain) 
{  
  BOOL	bSuccess ;

  TRACE ; 

  // send filters to driver
  bSuccess = SpySrv_SendFilterSetToDriver () ;

  if( ! bSuccess )
    {
      TRACE_ERROR(TEXT("SendFiltersToDriver failed\n")) ;    
      CloseHandle (g_hDriver) ;
      return FALSE ;
    }

  // initialize app-driver link
  bSuccess = Link_Init (g_hDriver, _SpySrv_RequestFromDriver) ;

  // ok ?
  if( ! bSuccess )
    {
      TRACE_ERROR(TEXT("Link_Init failed (0x%08X)\n"), GetLastError()) ;
      CloseHandle (g_hDriver) ;
      return FALSE ;
    }

  _SpySrv_RefreshProcList () ;
  
  return TRUE ;
}


/******************************************************************/
/* Exported function : Stop                                       */
/******************************************************************/

VOID SpySrv_Stop () 
{  
  TRACE ; 

  Link_Uninit () ;
}


/******************************************************************/
/* Internal function : UninitFilters                              */
/******************************************************************/

BOOL _SpySrv_UninitFilters ()
{  
  TRACE ; 

  FilterSet_Destroy (g_hFilterSet) ; 

  return TRUE ;
}



/******************************************************************/
/* Internal function : LockFilter                                       */
/******************************************************************/

VOID SpySrv_LockFilterSet ()
{
  DWORD dwResult ;

  //  TRACE ;

  dwResult = WaitForSingleObject (g_hFilterMutex, 10000) ;

  if( dwResult!=WAIT_OBJECT_0 )
    TRACE_ERROR (TEXT("WaitForSingleObject failed (res=%u, error=%u)\n"),
		  dwResult, GetLastError()) ;
}


/******************************************************************/
/* Internal function : UnlockFilter                                     */
/******************************************************************/

VOID SpySrv_UnlockFilterSet ()
{
  //TRACE ;

  if( ! ReleaseMutex (g_hFilterMutex) )
    TRACE_ERROR (TEXT("ReleaseMutex failed (error=%u)\n"), GetLastError()) ;  
}


/******************************************************************/
/* Internal function : GetAbsolutePath                            */
/******************************************************************/

void _SpySrv_GetAbsolutePath (LPTSTR szPath, LPCTSTR szFile)
{
  TCHAR *p ;

  GetModuleFileName (NULL, szPath, MAX_PATH) ;
  
  p = _tcsrchr (szPath, TEXT('\\')) ;  
  ASSERT (p!=NULL) ;
  _tcscpy (p+1, szFile) ;
}


/******************************************************************/
/* Exported function : ReadFilterFile                             */
/******************************************************************/  

VOID	SpySrv_ReadFilterFile ()
{
  HFILTERSET	hNewFilterSet ;
  TCHAR		szPath[MAX_PATH] ;
  TCHAR		szBuffer[1024] ;
  DWORD		dwFormatVersion ;
  DWORD		dwAppVersion ;

  _SpySrv_GetAbsolutePath (szPath, szFilterFilename) ;

  if( 0xFFFFFFFF==GetFileAttributes(szPath) )
    {
      TrayIcon_Alert (STR_DEF(_NO_FILTERS,szNoFiltersAlert)) ;
      return ;
    }

  FilterFile_GetFileVersion(szPath,&dwFormatVersion,&dwAppVersion) ;
  
  if( dwFormatVersion<4 || dwAppVersion<0x00600 )
    {
      wsprintf (szBuffer, TEXT("%s.bak"), szPath) ;
      MoveFile (szPath, szBuffer) ;
      TrayIcon_Alert (STR_DEF(_OLD_FILTERS_CLEARED,szOldFiltersCleared)) ;
      return ;
    }
 
  hNewFilterSet = FilterFile_Read (szPath) ;
  
  if( ! hNewFilterSet ) 
    {
      wsprintf (szBuffer, 
		STR_DEF(_ERROR_IN_FILTERS,szFiltersErrorAlert),
		szFilterFilename, FilterFile_GetErrorString()) ;
      MessageBox (NULL, szBuffer, TEXT(APPLICATION_NAME), MB_ICONERROR|MB_SETFOREGROUND) ;
      
      return ;
    } 
  
  SpySrv_LockFilterSet () ;
  
  FilterSet_Destroy (g_hFilterSet) ;
  g_hFilterSet = hNewFilterSet ;
  
  SpySrv_UnlockFilterSet () ; 
}


/******************************************************************/
/* Exported function : WriteFilterFile                            */
/******************************************************************/

VOID	SpySrv_WriteFilterFile ()
{
  TCHAR		szPath[MAX_PATH] ;

  SpySrv_LockFilterSet () ;

  _SpySrv_GetAbsolutePath (szPath, szFilterFilename) ;
 
  FilterFile_Write (szPath, g_hFilterSet) ;
  
  SpySrv_UnlockFilterSet () ; 
}


/******************************************************************/
/* Exported function :                                            */
/******************************************************************/

VOID SpySrv_SetFilterSet (HFILTERSET hNewFilterSet) 
{
  SpySrv_LockFilterSet () ;

  FilterSet_Destroy (g_hFilterSet) ;
  g_hFilterSet = hNewFilterSet ;

  SpySrv_UnlockFilterSet () ; 

  PostMessage (g_hwndMain, WM_SPYNOTIFY, SN_FILTERCHANGED, 0) ;
}


/******************************************************************/
/* Exported function :                                            */
/******************************************************************/

BOOL	SpySrv_AddRuleForProgram (FILTRULE* pRule, LPCTSTR szPath) 
{
  HFILTER	hFilter ;

  // verify params
  ASSERT (pRule!=NULL) ;
  ASSERT (szPath!=NULL) ;

  SpySrv_LockFilterSet () ;

  hFilter = FilterSet_GetFilterStrict (g_hFilterSet, szPath) ;

  if( ! hFilter ) {
    hFilter = Filter_Create (szPath) ;
    FilterSet_AddFilter (g_hFilterSet, hFilter) ;
  }

  Filter_AddRule (hFilter, pRule) ;

  SpySrv_SendFilterSetToDriver () ;
  SpySrv_UnlockFilterSet () ;

  PostMessage (g_hwndMain, WM_SPYNOTIFY, SN_FILTERCHANGED, 0) ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL SpySrv_KillProcess (PROCADDR nProcessAddress, BOOL bKernelModeKill) 
{
  BOOL		bSuccess ;

  if( ! bKernelModeKill )
    {
      PROCSTRUCT	*pProc ;
      HANDLE		hProcess ;
      UINT		nProcessId ;

      ProcList_Lock () ;
      pProc = ProcList_Get (nProcessAddress) ;
      if( pProc ) nProcessId = pProc->nProcessId ;
      ProcList_Unlock () ;
      
      if( pProc==NULL )
	{
	  TRACE_WARNING (TEXT("Tryed to kill an unknown process\n")) ;
	}
      else if( nProcessId==GetCurrentProcessId() )
	{
	  TRACE_WARNING (TEXT("Refused to kill my process\n")) ;
	  bSuccess = FALSE ;
	}
      else
	{
	  hProcess = OpenProcess (PROCESS_TERMINATE, FALSE, nProcessId) ;
	  
	  if( ! hProcess ) 
	    {
	      TRACE_WARNING (TEXT("OpenProcess failed (error=%u)\n"), GetLastError()) ;
	      return FALSE ;
	    }
	  
	  bSuccess = TerminateProcess (hProcess, 0) ;  
	  CloseHandle (hProcess) ;
	  
	  if( ! bSuccess )
	    TRACE_ERROR (TEXT("TerminateProcess failed (error=%u)\n"), GetLastError()) ;
	}
    }
  else
    { 
      DWORD	nBytesReturned ;
    
      bSuccess = DeviceIoControl (g_hDriver, IOCTL_KILL_PROCESS, 
				  &nProcessAddress, sizeof(nProcessAddress), 
				  NULL, 0, &nBytesReturned, NULL) ;
      
      if( ! bSuccess )
	TRACE_ERROR (TEXT("DeviceIoControl failed (error=%u)\n"), GetLastError()) ;     
    }
  
  return bSuccess ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL SpySrv_IgnoreProcess (PROCADDR nProcessAddress, BOOL bIgnore) 
{
  BOOL		bSuccess ;
  DWORD		nBytesReturned ;
  SDCIGNOREPROC	params ;
  PROCSTRUCT	*pProc ;
  
  params.nProcessAddress = nProcessAddress ;
  params.bIgnore = bIgnore ;

  bSuccess = DeviceIoControl (g_hDriver, IOCTL_IGNORE_PROCESS, 
			      &params, sizeof(params), 
			      NULL, 0, &nBytesReturned, NULL) ;
  
  if( ! bSuccess )
    {
      TRACE_ERROR (TEXT("DeviceIoControl failed (error=%u)\n"), GetLastError()) ;     
      return FALSE ;
    }
  
  ProcList_Lock () ;

  pProc = ProcList_Get (nProcessAddress) ;

  if( pProc )
    pProc->nState = bIgnore ? PS_HOOK_DISABLED : PS_HOOKED_WHILE_RUNNING ;

  ProcList_Unlock () ;

  PostMessage (g_hwndMain, WM_SPYNOTIFY, SN_PROCESSCHANGED, nProcessAddress) ;

  return TRUE ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _SpySrv_RefreshProcList () 
{
  PVOID		pBuffer ;
  DWORD		nBufferSize ;
  DWORD		nBytesReturned=0 ;
  BOOL		bSuccess ;
  PROCESSLISTENTRY*	pEntry ;
  
  nBufferSize = 1024*1024 ;
  pBuffer = malloc (nBufferSize) ;

  bSuccess = DeviceIoControl (g_hDriver, IOCTL_GET_PROCESSLIST, 
			      NULL, 0,
			      pBuffer, nBufferSize,
			      &nBytesReturned, NULL) ;

  if( ! bSuccess ||  nBytesReturned==0 )
    {
      TRACE_ERROR (TEXT("Failed to get process list (error=%u)\n"),
		   GetLastError()) ;
      free (pBuffer) ;
      return FALSE ;
    }

  
  ProcList_Lock () ;

  ProcList_Clear () ;
  
  pEntry = pBuffer ;

  while( 1 )
    {
      PROCSTRUCT proc ;

      TRACE_INFO (TEXT("%d : %ls\n"), pEntry->nProcessId, pEntry->wszFilePath) ;

      proc.nProcessAddress	= pEntry->nProcessAddress ;
      proc.nProcessId		= pEntry->nProcessId ;
      proc.nState		= PS_HOOKED_WHILE_RUNNING ;
      wcslcpy (proc.szName, PathFindFileName(pEntry->wszFilePath), 32) ;
      wcslcpy (proc.szPath, pEntry->wszFilePath, MAX_PATH) ;

      ProcList_Add (&proc) ;

      if( pEntry->nNextEntry == 0 ) break ;
      
      pEntry = (PROCESSLISTENTRY*)( (BYTE*)pEntry + pEntry->nNextEntry ) ;
    }
  
  ProcList_Unlock () ;
    
  free (pBuffer) ;
  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL SpySrv_InstallDriver (BOOL bPermanently)
{
  SC_HANDLE	hManager ;
  SC_HANDLE	hService ;
  BOOL		bSuccess ; 
  TCHAR		szDriverPath[MAX_PATH] ;

  _SpySrv_GetAbsolutePath (szDriverPath, g_szDriverFileName) ;

  if( !PathFileExists(szDriverPath) )
    {
      TRACE_ERROR (TEXT("File not found : %s\n"), szDriverPath) ;
      SetLastError (ERROR_FILE_NOT_FOUND) ;
      return FALSE ;
    }
  
  TRACE_INFO (TEXT("Driver path = %s\n"), szDriverPath) ;

  hManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS) ;

  if( hManager==NULL )
    {
      TRACE_ERROR (TEXT("OpenSCManager failed (error=%u)\n"), GetLastError()) ;
      return FALSE ;
    }

 create:

  hService = CreateService (hManager,
			    g_szServiceName,
			    TEXT("Winpooch kernel spy"),
			    SERVICE_START|DELETE,
			    SERVICE_KERNEL_DRIVER,
			    bPermanently?
			    SERVICE_SYSTEM_START:SERVICE_DEMAND_START,
			    SERVICE_ERROR_NORMAL,
			    szDriverPath, 
			    NULL, NULL,
			    NULL, NULL, NULL) ;
  
  if( hService==NULL )
    {
      DWORD dwLastError = GetLastError() ;

      if( dwLastError == ERROR_SERVICE_EXISTS )
	{
	  hService = OpenService (hManager, g_szServiceName, SERVICE_STOP|DELETE) ;
	  
	  if( hService!=NULL )
	    {
	      SERVICE_STATUS	srvstatus ;
	      ControlService (hService, SERVICE_CONTROL_STOP, &srvstatus) ;
	      bSuccess = DeleteService(hService) ;
	      CloseServiceHandle (hService) ;

	      if( bSuccess )
		{
		  TRACE_WARNING (TEXT("Service deleted. Try to re-create it\n")) ;
		  goto create ;
		}
	    }
	}

      TRACE_ERROR (TEXT("CreateService failed (error=%u)\n"), dwLastError) ;
      CloseServiceHandle (hManager) ;
      SetLastError (dwLastError) ;
      return FALSE ;
    }
  
  bSuccess = StartService (hService, 0, NULL) ;
  
  if( ! bSuccess ) 
    {
      DWORD dwLastError = GetLastError() ;
      TRACE_ERROR (TEXT("StartService failed (error=%u)\n"), dwLastError) ;
      DeleteService(hService) ;
      CloseServiceHandle (hService) ;
      CloseServiceHandle (hManager) ;
      SetLastError (dwLastError) ;
      return FALSE ;     
    }

  CloseServiceHandle (hService) ;
  CloseServiceHandle (hManager) ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL SpySrv_UninstallDriver ()
{
  SC_HANDLE	hManager ;
  SC_HANDLE	hService ;
  BOOL		bSuccess ; 
  SERVICE_STATUS	srvstatus ;
  
  hManager = OpenSCManager (NULL, NULL, 0) ;

  if( hManager==NULL )
    {
      TRACE_ERROR (TEXT("OpenSCManager failed (error=%u)\n"), GetLastError()) ;
      return FALSE ;
    }

  hService = OpenService (hManager, g_szServiceName, SERVICE_STOP|DELETE) ;

  if( hService==NULL )
    {
      TRACE_ERROR (TEXT("OpenService failed (error=%u)\n"), GetLastError()) ; 
      return FALSE ;
    }

  bSuccess = ControlService (hService, SERVICE_CONTROL_STOP, &srvstatus) ;

  if( ! bSuccess ) 
    {
      TRACE_WARNING (TEXT("ControlService failed (error=%u)\n"), GetLastError()) ;
    }
  
  bSuccess = DeleteService(hService) ;
  
  if( ! bSuccess ) 
    {
      TRACE_ERROR (TEXT("DeleteService failed (error=%u)\n"), GetLastError()) ;
      return FALSE ;     
    }

  CloseServiceHandle (hService) ;
  CloseServiceHandle (hManager) ;

  return TRUE ;
}



BOOL SpySrv_SetScanFilters (LPCWSTR * pszFilters, UINT nFilters) 
{
  BYTE	*pBuffer ;
  UINT	nBufferSize ;
  BOOL	bSuccess ;
  DWORD	nWaitResult ;
  OVERLAPPED	ov ;
    
  if( nFilters > 0 )
    {
      UINT i, n ;

      TRACE_INFO (TEXT("Calculating buffer size...\n")) ;
      
      n = 0 ;
      for( i=0 ; i<nFilters ; i++ )
	n += (wcslen(pszFilters[i])+1)*sizeof(WCHAR) ;

      TRACE_INFO (TEXT("Buffer size is %u\n"), n) ;
      
      nBufferSize = n ;
      pBuffer = malloc (n) ;
      
      n = 0 ;
      for( i=0 ; i<nFilters ; i++ )
	{	  
	  wcscpy ((LPWSTR)&pBuffer[n], pszFilters[i]) ;
	  n += (wcslen(pszFilters[i])+1)*sizeof(TCHAR) ;
	}     
    }
  else
    {
      pBuffer = NULL ;
      nBufferSize = 0 ;
    }

  ov.hEvent = CreateEvent (NULL, TRUE, FALSE, NULL) ;

  bSuccess = DeviceIoControl (g_hDriver, 
			      IOCTL_SET_SCAN_FILTERS, 
			      pBuffer, nBufferSize,
			      NULL, 0, NULL, &ov) ;

  if( !bSuccess && GetLastError()==ERROR_IO_PENDING )
    {
      TRACE_WARNING (TEXT("IOCTL_SET_SCAN_FILTERS is running asynchronously\n")) ;

      nWaitResult = WaitForSingleObject (ov.hEvent, 10*1000) ;

      bSuccess = nWaitResult==WAIT_OBJECT_0 ;
    }

  CloseHandle (ov.hEvent) ;

  free (pBuffer) ;
  
  if( ! bSuccess )
    TRACE_ERROR (TEXT("Failed to send scanner path to driver\n")) ;

  return bSuccess ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL  SpySrv_SyncScanCache (VOID * pUserPtr, SYNCCACHECALLBACK pfnCallback, LARGE_INTEGER * pliLastSyncTime,
			    ULONG * pnFirstIdentifier, ULONG * pnLastIdentifier)
{
  PVOID			pBuffer ;
  DWORD			nBufferSize ;
  DWORD			nBytesReturned=0 ;
  DWORD			nRemainingSize ;
  BOOL			bSuccess ;
  SCANCACHEENTRY*	pEntry ;
  SDCSYNCCACHE		synccache ;

  TRACE_INFO (TEXT("SYNCHRONIZE\n")) ;

  if( pnFirstIdentifier ) *pnFirstIdentifier = 0 ;
  if( pnLastIdentifier ) *pnLastIdentifier = 0 ;
  
  synccache.liLastSyncTime = *pliLastSyncTime ;

  nBufferSize = 1024*1024 ;
  pBuffer = MALLOC (nBufferSize) ;

  bSuccess = DeviceIoControl (g_hDriver, IOCTL_SYNC_CACHE,
			      &synccache, sizeof(synccache),
			      pBuffer, nBufferSize,
			      &nBytesReturned, NULL) ;

  if( ! bSuccess ||  nBytesReturned==0 )
    {
      TRACE_ERROR (TEXT("Failed to get process list (error=%u)\n"),
		   GetLastError()) ;
      FREE (pBuffer) ;
      return FALSE ;
    }

  if( pnFirstIdentifier )
    *pnFirstIdentifier = ((SCANCACHEHEADER*)pBuffer)->nFirstIdentifier ;

  if( pnLastIdentifier )
    *pnLastIdentifier = ((SCANCACHEHEADER*)pBuffer)->nLastIdentifier ;

  pEntry = (SCANCACHEENTRY*)( (BYTE*)pBuffer + sizeof(SCANCACHEHEADER) ) ;
  nRemainingSize = nBytesReturned - sizeof(SCANCACHEHEADER) ;

  while( nRemainingSize > sizeof(SCANCACHEENTRY) )
    {
      TRACE_INFO (TEXT("%d : %ls\n"), pEntry->nIdentifier, pEntry->wszFilePath) ;

      if( ! pfnCallback (pUserPtr, pEntry->nIdentifier, pEntry->wszFilePath, pEntry->nScanResult, &pEntry->liScanTime) )
	break ;
      
      if( pEntry->nNextEntry == 0 ) break ;
      
      nRemainingSize -= pEntry->nNextEntry ;
      pEntry = (SCANCACHEENTRY*)( (BYTE*)pEntry + pEntry->nNextEntry ) ;
    }

  FREE (pBuffer) ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL SpySrv_AddFileToCache (LPCWSTR szFilePath, SCANRESULT nResult, LARGE_INTEGER * pliScanTime) 
{
  SDCADDFILETOCACHE*	pAddfile ;
  UINT			nSize ;
  DWORD			nBytesReturned ;
  BOOL			bSuccess ;

  nSize = sizeof(SDCADDFILETOCACHE)+sizeof(WCHAR)*(wcslen(szFilePath)+1) ;
  
  pAddfile = MALLOC (nSize) ;
  
  pAddfile->nScanResult = nResult ;
  pAddfile->liScanTime = *pliScanTime ;
  wcscpy (pAddfile->wszFilePath, szFilePath) ;

  bSuccess = DeviceIoControl (g_hDriver, IOCTL_ADD_FILE_TO_CACHE,
			      pAddfile, nSize,
			      NULL, 0,
			      &nBytesReturned, NULL) ;

  FREE (pAddfile) ;

  if( ! bSuccess )
    {
      TRACE_ERROR (TEXT("Failed to add file to antivirus cache (error=%u)\n"),
		   GetLastError()) ;
      return FALSE ;
    }

  return TRUE ;
}
