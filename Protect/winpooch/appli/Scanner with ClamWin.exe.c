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

//#define TRACE_LEVEL		2

#define AUTO_DETECT_SCANNER	1


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "Scanner.h"

// standard headers
#include <shlwapi.h>
#include <ctype.h>
#include <stdio.h>
#include <tchar.h>

// library's headers
#include <clamav.h>

// project's headers
#include "Config.h"
#include "FreshClam.h"
#include "Strlcpy.h"
#include "SpyServer.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

// how many bytes in a megabyte ?
#define MEGA			(1<<20)

// antivirus names (used for configuration)
static LPCTSTR szNoneName	= TEXT("None") ;
static LPCTSTR szClamWinName	= TEXT("ClamWin") ;
static LPCTSTR szKavWsName	= TEXT("KavWs") ;
static LPCTSTR szBitDefName	= TEXT("BitDefender") ;
static LPCTSTR szLibClamavName	= TEXT("Libclamav") ;

// constants for ClamWin
static LPCTSTR szClamWinKey	= TEXT("Software\\ClamWin") ;
static LPCTSTR szClamWinPathValue = TEXT("Path") ;

// constants for Kav Ws
static LPCTSTR szKavWsKey	= TEXT("Software\\KasperskyLab\\InstalledProducts\\Kaspersky Anti-Virus for Workstation") ;
static LPCTSTR szKavWsFolderValue = TEXT("Folder") ;
static LPCTSTR szKavWsExe	= TEXT("KAVShell.exe") ;

// constants for BitDefender
static LPCTSTR szBitDefKey	= TEXT("SOFTWARE\\Softwin") ;
static LPCTSTR szBitDefFolderValue = TEXT("BitDefender Scan Server") ;
static LPCTSTR szBitDefExe	= TEXT("bdc.exe") ;


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  TCHAR		szClamScanExe[MAX_PATH] ;
  TCHAR		szClamWinExe[MAX_PATH] ;
} CLAMWINCONF ;

typedef struct {
  TCHAR		szScanner[MAX_PATH] ;
} KAVWSCONF ;

typedef struct {
  TCHAR		szScanner[MAX_PATH] ;
  TCHAR		szFolder[MAX_PATH] ;
} BITDEFCONF ;

typedef struct {
  unsigned int		options ;
  struct cl_limits	limits ;
  struct cl_engine	*engine ;
  TCHAR			szScanner[MAX_PATH] ;
} LIBCLAMAVCONF ;

typedef struct {
  UINT		nScanner ;
  CLAMWINCONF	clamwinconf ;
  KAVWSCONF	kavwsconf ;
  BITDEFCONF	bitdefconf ;
  LIBCLAMAVCONF	libclamavconf;
} INTERNALDATA ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static INTERNALDATA	g_data ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

BOOL _Scanner_ClamWin_Configure (CLAMWINCONF*) ;

UINT _Scanner_ClamWin_ScanFile (CLAMWINCONF*, LPCTSTR, LPTSTR szOutput, UINT nOutputMax, DWORD nPriorityClass) ;

BOOL _Scanner_KavWs_Configure (KAVWSCONF*) ;

UINT _Scanner_KavWs_ScanFile (KAVWSCONF*, LPCTSTR, LPTSTR szOutput, UINT nOutputMax, DWORD nPriorityClass) ;

BOOL _Scanner_BitDef_Configure (BITDEFCONF*) ;

UINT _Scanner_BitDef_ScanFile (BITDEFCONF*, LPCTSTR, LPTSTR szOutput, UINT nOutputMax, DWORD nPriorityClass) ;

BOOL _Scanner_LibClamav_Configure (LIBCLAMAVCONF*) ;

UINT _Scanner_LibClamav_ScanFile (LIBCLAMAVCONF*, LPCTSTR, LPTSTR szOutput, UINT nOutputMax) ;

BOOL _Scanner_LibClamav_LoadDatabase (LIBCLAMAVCONF*) ;

VOID _Scanner_LibClamav_DatabaseUpdated () ;



/******************************************************************/
/* Exported function : Init                                       */
/******************************************************************/

BOOL Scanner_Init ()
{
  LPCTSTR	szConfigScanner ;
  BOOL		bResult ;

  TRACE ;

  //
  // Which antivirus ?
  //

  // read configuration
  szConfigScanner = Config_GetString(CFGSTR_ANTIVIRUS) ;

  // -> NULL, autodetect
  if( ! szConfigScanner )
    {
#if AUTO_DETECT_SCANNER
      if( _Scanner_KavWs_Configure (&g_data.kavwsconf) )
	g_data.nScanner = SCANNER_BITDEFENDER ;
      else if( _Scanner_ClamWin_Configure (&g_data.clamwinconf) )
	g_data.nScanner = SCANNER_CLAMWIN ;
      else if( _Scanner_KavWs_Configure (&g_data.kavwsconf) )
	g_data.nScanner = SCANNER_KASPERSKY_WS ;
      else if( _Scanner_LibClamav_Configure (&g_data.libclamavconf) )
	g_data.nScanner = SCANNER_LIBCLAMAV ;
      else
	g_data.nScanner = SCANNER_NONE ;
      bResult = TRUE ;
#else
      bResult = FALSE ;
#endif
    }
  // -> ClamWin
  else if( !_tcsicmp(szClamWinName,szConfigScanner) )
    {
      bResult = _Scanner_ClamWin_Configure (&g_data.clamwinconf) ;
      g_data.nScanner = SCANNER_CLAMWIN ;
    }
  // -> Kaspersky WS ?
  else if( !_tcsicmp(szKavWsName,szConfigScanner) )
    {
      bResult = _Scanner_KavWs_Configure (&g_data.kavwsconf) ;
      g_data.nScanner = SCANNER_KASPERSKY_WS ;
    }
  // -> BitDefender ?
  else if( !_tcsicmp(szBitDefName,szConfigScanner) )
    {
      bResult = _Scanner_BitDef_Configure (&g_data.bitdefconf) ;
      g_data.nScanner = SCANNER_BITDEFENDER ;
    }
  // -> Libclamav ?
  else if( !_tcsicmp(szLibClamavName,szConfigScanner) )
    {
      bResult = _Scanner_LibClamav_Configure (&g_data.libclamavconf) ;
      g_data.nScanner = SCANNER_LIBCLAMAV ;
    }
  // -> None
  else if( !_tcsicmp(szNoneName,szConfigScanner) )
    {
      g_data.nScanner = SCANNER_NONE ;
      bResult = TRUE ;
    }
  // -> Other
  else bResult = FALSE ;

  // if failed, then set to "none"
  if( ! bResult ) g_data.nScanner = SCANNER_NONE ;

  return bResult ;
}


/******************************************************************/
/* Exported function : Uninit                                     */
/******************************************************************/

VOID Scanner_Uninit ()
{
  TRACE ;

  // update config
  switch( g_data.nScanner )
    {
    case SCANNER_CLAMWIN:
      Config_SetString (CFGSTR_ANTIVIRUS, szClamWinName) ;
      break ;
    case SCANNER_KASPERSKY_WS:
      Config_SetString (CFGSTR_ANTIVIRUS, szKavWsName) ;
      break ;
    case SCANNER_BITDEFENDER:
      Config_SetString (CFGSTR_ANTIVIRUS, szBitDefName) ;
      break ;
    case SCANNER_LIBCLAMAV:
      FreshClam_Stop () ;
      if( g_data.libclamavconf.engine )
	cl_free (g_data.libclamavconf.engine);
      Config_SetString (CFGSTR_ANTIVIRUS, szLibClamavName) ;
      break ;
    default:
      Config_SetString (CFGSTR_ANTIVIRUS, szNoneName) ;
    }
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Scanner_SetScanner (UINT nScanner)
{
  BOOL bResult = FALSE ;

  TRACE ;

  switch( nScanner )
    {
    case SCANNER_CLAMWIN:
      bResult = _Scanner_ClamWin_Configure (&g_data.clamwinconf)  ;
      break ;
    case SCANNER_KASPERSKY_WS:
      bResult = _Scanner_KavWs_Configure (&g_data.kavwsconf)  ;
      break ;
    case SCANNER_BITDEFENDER:
      bResult = _Scanner_BitDef_Configure (&g_data.bitdefconf)  ;
      break ;
    case SCANNER_LIBCLAMAV:
      bResult = _Scanner_LibClamav_Configure (&g_data.libclamavconf)  ;
      break ;
    case SCANNER_NONE:
      bResult = TRUE ;
      break ;
    }

  g_data.nScanner = bResult ? nScanner : SCANNER_NONE ; 

  // tell the spy server that he should not scan this file
  SpySrv_SetScannerExePath (Scanner_GetScannerExe()) ;

  return bResult ;
}

/******************************************************************/
/* Exported function : IsConfigured                               */
/******************************************************************/

BOOL Scanner_IsConfigured ()
{
  TRACE ;

  return g_data.nScanner!=SCANNER_NONE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT Scanner_GetScanner  ()
{
  TRACE ;

  return g_data.nScanner ;
}


/******************************************************************/
/* Exported function : IsScanner                                  */
/******************************************************************/

BOOL Scanner_IsScanner (LPCTSTR szPath)
{
  LPCTSTR szScanner = Scanner_GetScannerExe () ;

  return 0==_tcsicmp(szScanner,szPath) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

LPCTSTR Scanner_GetScannerExe ()
{
  LPCTSTR szScanner ;

  switch( g_data.nScanner )
    {
    case SCANNER_CLAMWIN:
      szScanner = g_data.clamwinconf.szClamScanExe ;
      break ;

    case SCANNER_KASPERSKY_WS:
      szScanner = g_data.kavwsconf.szScanner ;
      break ;

    case SCANNER_BITDEFENDER:
      szScanner = g_data.bitdefconf.szScanner ;
      break ;

    case SCANNER_LIBCLAMAV:
      szScanner = g_data.libclamavconf.szScanner ;
      break ;

    default:
      return NULL  ;
    }

  TRACE_INFO (TEXT("Scanner = %s\n"), szScanner) ;

  return szScanner ;
}


/******************************************************************/
/* Exported function : ScanFile                                   */
/******************************************************************/

UINT Scanner_ScanFile (LPCTSTR szPath, LPTSTR szOutput, UINT nOutputMax)
{
  TRACE_INFO ("File = %s\n", szPath) ;

  // avoid scanning the scanner
  if( Scanner_IsScanner(szPath) )
    return FALSE ;

  switch( g_data.nScanner )
    {
    case SCANNER_CLAMWIN:
      return _Scanner_ClamWin_ScanFile (&g_data.clamwinconf, szPath, szOutput, nOutputMax, HIGH_PRIORITY_CLASS) ;
      
    case SCANNER_KASPERSKY_WS:
      return _Scanner_KavWs_ScanFile (&g_data.kavwsconf, szPath, szOutput, nOutputMax, HIGH_PRIORITY_CLASS) ;
      
    case SCANNER_BITDEFENDER:
      return _Scanner_BitDef_ScanFile (&g_data.bitdefconf, szPath, szOutput, nOutputMax, HIGH_PRIORITY_CLASS) ;
      
    case SCANNER_LIBCLAMAV:
      return _Scanner_LibClamav_ScanFile (&g_data.libclamavconf, szPath, szOutput, nOutputMax) ;
 }

  return SCANNER_NONE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT Scanner_ScanFileBg (LPCTSTR szPath) 
{
  TRACE_INFO (TEXT("Path = %s\n"), szPath) ;

  // avoid scanning the scanner
  if( Scanner_IsScanner(szPath) )
    return SCAN_NO_VIRUS ;

  switch( g_data.nScanner )
    {
    case SCANNER_CLAMWIN:
      return _Scanner_ClamWin_ScanFile (&g_data.clamwinconf, szPath, NULL, 0, IDLE_PRIORITY_CLASS) ;
      
    case SCANNER_KASPERSKY_WS:
      return _Scanner_KavWs_ScanFile (&g_data.kavwsconf, szPath, NULL, 0, IDLE_PRIORITY_CLASS) ;

    case SCANNER_BITDEFENDER:
      return _Scanner_BitDef_ScanFile (&g_data.bitdefconf, szPath, NULL, 0, IDLE_PRIORITY_CLASS) ;

    case SCANNER_LIBCLAMAV:
      return _Scanner_LibClamav_ScanFile (&g_data.libclamavconf, szPath, NULL, 0) ;
    }

  return SCAN_FAILED ;
}



/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _Scanner_ClamWin_GetAppDir (LPTSTR szPath)
{
  HKEY	hkey ;
  LONG	nResult ;
  DWORD dwType, dwSize ;
  UINT	nLen ;
  TCHAR szValue[MAX_PATH] ;

  // open key on HKCU
  nResult = RegOpenKeyEx (HKEY_CURRENT_USER, szClamWinKey, 0, KEY_READ, &hkey) ;
  if( ERROR_SUCCESS!=nResult )
    {
      TRACE_INFO (TEXT("Failed to open key HKCU\\%s (error=%d)\n"),
		  szClamWinKey, nResult) ;

      // else try on HKLM
      nResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szClamWinKey, 0, KEY_READ, &hkey) ;
      if( ERROR_SUCCESS!=nResult )
	{
	  TRACE_INFO (TEXT("Failed to open key HKLM\\%s (error=%d)\n"),
		      szClamWinKey, nResult) ;
	  return FALSE ;
	}
    }

  // read install path
  dwSize = sizeof(szValue) ;
  nResult = RegQueryValueEx (hkey, szClamWinPathValue, NULL,
			     &dwType, (BYTE*)szValue, &dwSize);

  // ok ?
  if( ERROR_SUCCESS!=nResult || dwSize==0 ) {
      TRACE_ERROR (TEXT("Failed to read ClamWin path (error=%d)\n"),nResult) ;
      CloseHandle(hkey) ;
      return FALSE ;
    }

  // expand env strings
  nLen = ExpandEnvironmentStrings (szValue, szPath, MAX_PATH) ;
  if( ! nLen ) {
    TRACE_ERROR (TEXT("ExpandEnvironmentStrings failed (error=%d)\n"), GetLastError()) ;
    CloseHandle(hkey) ;
    return FALSE ;
  }

  CloseHandle(hkey) ;

  return TRUE ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _Scanner_ClamWin_Configure (CLAMWINCONF * pConf)
{
  TCHAR szDir[MAX_PATH] ;

  if( ! _Scanner_ClamWin_GetAppDir (szDir) ) 
    {
      TRACE_ERROR (TEXT("Failed to find ClamWin directory\n")) ;
      return FALSE ;
    }

  PathCombine (pConf->szClamScanExe, szDir, TEXT("clamscan.exe")) ;
  PathCombine (pConf->szClamWinExe, szDir, TEXT("clamwin.exe")) ;

  TRACE_INFO (TEXT("CLAMWIN =  %s\n"), pConf->szClamWinExe) ;
  TRACE_INFO (TEXT("CLAMSCAN =  %s\n"), pConf->szClamScanExe) ;

  if( 0xFFFFFFFF == GetFileAttributes(pConf->szClamWinExe) )
    {
      TRACE_ERROR (TEXT("File not found: %s\n"), pConf->szClamWinExe) ;
      return FALSE ;
    }

  if( 0xFFFFFFFF == GetFileAttributes(pConf->szClamScanExe) )
    {
      TRACE_ERROR (TEXT("File not found: %s\n"), pConf->szClamScanExe) ;
      return FALSE ;
    }

  return TRUE ;
}

/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _Scanner_Run (LPTSTR szCmdLine, LPTSTR szDirectory,
		   DWORD*pdwExitCode,
		   LPTSTR szOutput, UINT nOutputMax,
		   DWORD nPriorityClass) 
{
  SECURITY_ATTRIBUTES sa = {0};
  STARTUPINFO         si = {0};
  PROCESS_INFORMATION pi = {0};
  HANDLE              hPipeOutputRead  = NULL;
  HANDLE              hPipeOutputWrite = NULL;

  DWORD		dwBytesRead ;
  BOOL		bSuccess ;
  UINT		nOutputPos = 0 ;

  *pdwExitCode = (DWORD)-1 ;

  TRACE_INFO (TEXT("CmdLine = %s\n"), szCmdLine) ;

  sa.nLength		= sizeof(sa) ;
  sa.bInheritHandle	= TRUE ; 
  sa.lpSecurityDescriptor = NULL ;
  
  if( szOutput!=NULL )
    {
      if( ! CreatePipe (&hPipeOutputRead, &hPipeOutputWrite, &sa, 0) )
	TRACE_WARNING (TEXT("CreatePipe failed (error=%d)\n"), GetLastError()) ;
 
      si.cb		= sizeof(si);
      si.dwFlags	= STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
      si.wShowWindow	= SW_SHOW ; //SW_HIDE;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      si.hStdInput	= GetStdHandle (STD_INPUT_HANDLE) ;
      si.hStdOutput	= hPipeOutputWrite ;
      si.hStdError	= hPipeOutputWrite ;//GetStdHandle (STD_ERROR_HANDLE) ;
    }
  else
    {
      si.cb		= sizeof(si);
      si.dwFlags	= STARTF_USESHOWWINDOW ;
      si.wShowWindow	= SW_HIDE;
    }
  
  if( ! CreateProcess (NULL, szCmdLine, NULL, NULL, TRUE, nPriorityClass, 
		       NULL, szDirectory, &si, &pi) ) {
    TRACE_ERROR (TEXT("Failed to run scanner\n")) ;
    CloseHandle (hPipeOutputWrite);
    CloseHandle (hPipeOutputRead);
    return FALSE ;
  }
  
  CloseHandle (hPipeOutputWrite);
 
  // wait for process end
  WaitForSingleObject (pi.hProcess, 30000) ;

  GetExitCodeProcess (pi.hProcess, pdwExitCode) ;

  if( szOutput!=NULL )
    {
      while( nOutputPos<nOutputMax-1 )
	{
	  UINT i ;
	  char szBuffer[64] ;
	  
	  // try to read pipe
	  bSuccess = ReadFile (hPipeOutputRead, 
			       szBuffer, 
			       64,
			       &dwBytesRead, NULL) ;  
	  
	  // failed to read ?
	  if( !bSuccess || !dwBytesRead ) break ;
	  
	  for( i=0 ; i<dwBytesRead ; i++ )
	    {
	      switch( szBuffer[i] )
		{
		case '\r':
		  break ;
		case '\n':
		  szOutput[nOutputPos++] = TEXT('\r') ;
		  szOutput[nOutputPos++] = TEXT('\n') ;
		  break ;
		default:
		  szOutput[nOutputPos++] = szBuffer[i] ;
		}
	      
	      if( nOutputPos>=nOutputMax-1 ) break ;
	    }
	}
      
      szOutput[nOutputPos] = 0 ;
    }      
  
  TRACE_INFO (TEXT("Scan result = %u\n"), *pdwExitCode) ;
  
  CloseHandle (hPipeOutputRead);
  CloseHandle (pi.hThread) ;
  CloseHandle (pi.hProcess) ;

  return TRUE ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

UINT _Scanner_ClamWin_ScanFile (CLAMWINCONF * pConf, LPCTSTR szFile,
				LPTSTR szOutput, UINT nOutputMax,
				DWORD nPriorityClass) 
{
  TCHAR		szCmdLine[1024] ;
  DWORD		dwExitCode ;
  BOOL		bSuccess ;
  
  wsprintf (szCmdLine, TEXT("\"%s\" --mode=scanner --path=\"%s\" --close"), 
	    pConf->szClamWinExe, szFile) ;
  
  bSuccess = _Scanner_Run (szCmdLine, NULL,
			   &dwExitCode,
			   szOutput, nOutputMax,
			   nPriorityClass) ;

  if( ! bSuccess ) return SCAN_FAILED ;

  return
    dwExitCode==0 ? SCAN_NO_VIRUS :
    dwExitCode==1 ? SCAN_VIRUS_FOUND :
    SCAN_FAILED ;
}



/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _Scanner_KavWs_Configure (KAVWSCONF * pConf)
{
  HKEY	hkey ;
  LONG	nResult ;
  DWORD	dwSize ;
  DWORD	dwType ;
  TCHAR	szBuffer[MAX_PATH] ;
  BOOL	bFound ;

  nResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
			  szKavWsKey, 0,
			  KEY_QUERY_VALUE,
			  &hkey) ;
  if( nResult!=ERROR_SUCCESS ) {
    TRACE_INFO (TEXT("Registry key for KavWs not found\n")) ;
    return FALSE ;
  }

  dwSize = sizeof(szBuffer) ;

  nResult = RegQueryValueEx (hkey,
			     szKavWsFolderValue,
			     NULL,
			     &dwType,
			     (BYTE*)szBuffer,
			     &dwSize) ;
  RegCloseKey (hkey) ;

  if( nResult!=ERROR_SUCCESS ) {
    TRACE_INFO (TEXT("Failed to read folder value for KavWs\n")) ;
    return FALSE ;
  }

  PathCombine (pConf->szScanner, szBuffer, szKavWsExe) ;

  bFound = GetFileAttributes (pConf->szScanner) != 0xFFFFFFFF ;

  if( ! bFound )
    TRACE_WARNING (TEXT("KavWs scanner not found (path=%s)\n"), pConf->szScanner) ;

  return bFound ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

UINT _Scanner_KavWs_ScanFile (KAVWSCONF * pConf, LPCTSTR szFile,
			      LPTSTR szOutput, UINT nOutputMax,
			      DWORD nPriorityClass) 
{
  TCHAR		szCmdLine[1024] ;
  TCHAR		szTmpFile[MAX_PATH] ;
  TCHAR		szTmpDir[MAX_PATH] ;
  DWORD		dwExitCode ;
  BOOL		bSuccess ;
  FILE		*fp ;

  GetTempPath (MAX_PATH, szTmpDir) ;
  GetTempFileName (szTmpDir, PathFindFileName(szFile), 0, szTmpFile) ;
  
  wsprintf (szCmdLine, TEXT("\"%s\" scan \"%s\" /w:\"%s\""), 
	    pConf->szScanner, szFile, szTmpFile) ;
  
  bSuccess = _Scanner_Run (szCmdLine, NULL,
			   &dwExitCode,
			   szOutput, nOutputMax, 
			   nPriorityClass) ;

  if( ! bSuccess ) return SCAN_FAILED ;
  
  fp = _tfopen(szTmpFile, TEXT("rt")) ;
  
  if( fp!=NULL )
    {
      TCHAR szLine[128] ;
      szOutput[0] = 0 ;

      while( _fgetts(szLine,128,fp) )
	_tcscat (szOutput, szLine) ;

      fclose (fp) ;
    }

  return
    dwExitCode==0 ? SCAN_NO_VIRUS :
    dwExitCode==1 ? SCAN_VIRUS_FOUND :
    SCAN_FAILED ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _Scanner_BitDef_Configure (BITDEFCONF * pConf)
{
  HKEY	hkey ;
  LONG	nResult ;
  DWORD	dwSize ;
  DWORD	dwType ;
  BOOL	bFound ;

  nResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szBitDefKey, 0,
			  KEY_QUERY_VALUE, &hkey) ;
  if( nResult!=ERROR_SUCCESS ) {
    TRACE_INFO (TEXT("Registry key for BitDefender not found\n")) ;
    return FALSE ;
  }

  dwSize = sizeof(TCHAR)*MAX_PATH ;

  nResult = RegQueryValueEx (hkey,
			     szBitDefFolderValue,
			     NULL,
			     &dwType,
			     (BYTE*)pConf->szFolder,
			     &dwSize) ;
  RegCloseKey (hkey) ;

  if( nResult!=ERROR_SUCCESS ) {
    TRACE_INFO (TEXT("Failed to read folder value for BitDefender\n")) ;
    return FALSE ;
  }

  PathCombine (pConf->szScanner, pConf->szFolder, szBitDefExe) ;

  bFound = GetFileAttributes (pConf->szScanner) != 0xFFFFFFFF ;

  if( ! bFound )
    TRACE_WARNING (TEXT("BitDefender scanner not found (path=%s)\n"), pConf->szScanner) ;

  return bFound ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

UINT _Scanner_BitDef_ScanFile (BITDEFCONF * pConf, LPCTSTR szFile,
			       LPTSTR szOutput, UINT nOutputMax,
			       DWORD nPriorityClass) 
{
  TCHAR		szCmdLine[1024] ;
  DWORD		dwExitCode ;
  BOOL		bSuccess ;
  
  wsprintf (szCmdLine, TEXT("\"%s\" \"%s\" /files"), 
	    pConf->szScanner, szFile) ;
  
  bSuccess = _Scanner_Run (szCmdLine, pConf->szFolder,
			   &dwExitCode,
			   szOutput, nOutputMax,
			   nPriorityClass) ;

  if( ! bSuccess ) return SCAN_FAILED ;

  return
    dwExitCode==0 ? SCAN_NO_VIRUS :
    dwExitCode==1 ? SCAN_VIRUS_FOUND :
    SCAN_FAILED ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _Scanner_LibClamav_Configure (LIBCLAMAVCONF* conf)
{    
  TRACE_INFO (TEXT("Libclamav version : %hs\n"), cl_retver());
    
#if TRACE_LEVEL>=4
  cl_debug();
#endif

  conf->engine = NULL ;

  if( ! _Scanner_LibClamav_LoadDatabase (conf) )
    return FALSE ;

  conf->options = CL_SCAN_MAIL | CL_SCAN_OLE2 | CL_SCAN_HTML | CL_SCAN_PE | CL_SCAN_ALGO | CL_SCAN_BLOCKMAX;

  memset (&conf->limits, 0, sizeof(struct cl_limits));
  conf->limits.maxreclevel = 8;
  conf->limits.maxfiles = 1000;
  conf->limits.maxratio = 200;
  conf->limits.archivememlim = 0;
  conf->limits.maxfilesize = (unsigned long int)10*MEGA;

  // scanner path is winpooch path
  GetModuleFileName (NULL, conf->szScanner, MAX_PATH) ;  

  FreshClam_Start (_Scanner_LibClamav_DatabaseUpdated, conf) ;

  return TRUE;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _Scanner_LibClamav_LoadDatabase (LIBCLAMAVCONF* conf)
{
  struct cl_engine * pNewEngine ;
  struct cl_engine * pOldEngine ;

  int iResult ;
  
  UINT nSignCount = 0 ;
  CHAR szDbDirectory[MAX_PATH] ;

  GetModuleFileNameA (0, szDbDirectory, sizeof(szDbDirectory)-1);
  strrchr(szDbDirectory,'\\')[0] = 0 ;
    
  TRACE_INFO (TEXT("Database directory : %hs\n"), szDbDirectory);

  pNewEngine = NULL ;
  iResult = cl_loaddbdir (szDbDirectory, &pNewEngine, &nSignCount);
  if( iResult!=0 )
    {
      TRACE_ERROR (TEXT("cl_loaddbdir() failed (%d) %hs\n"), iResult, cl_strerror(iResult));
      return FALSE;
    }

  iResult = cl_build (pNewEngine) ;
  if( iResult!=0 )
    {
      TRACE_ERROR(TEXT("cl_build() failed (%d) %hs\n"),iResult,cl_strerror(iResult));
      cl_free(pNewEngine);
      return FALSE;
    }
  
  iResult = cl_retflevel();
  TRACE_INFO (TEXT("Libclamav loaded %d ClamAV malware signatures (CVD ver:%d).\n"), nSignCount, iResult);

  pOldEngine = conf->engine ;
  conf->engine = pNewEngine ;

  if( pOldEngine )
    {
      Sleep (10000) ;
      cl_free (pOldEngine) ;
    }

  return TRUE ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

UINT _Scanner_LibClamav_ScanFile (LIBCLAMAVCONF* conf, 
				  LPCTSTR wszFile,
				  LPTSTR szOutput, UINT nOutputMax)
{
  int iResultCode;
  unsigned long int scanned;
  LPCSTR szVirname ;
  char csFullPath[MAX_PATH] = {0} ;
      
  // convert filename to ASCII
  wcstombs (csFullPath, wszFile, MAX_PATH-1);
  TRACE_INFO(TEXT("File = %hs\n"), csFullPath);

  iResultCode = cl_scanfile (csFullPath, &szVirname, &scanned, 
			     conf->engine, &conf->limits, conf->options);

  TRACE_INFO(TEXT("Libclamav result = %d\n"), iResultCode);
  
  if( iResultCode == CL_VIRUS )
    {     
      TRACE_INFO (TEXT("%hs : %hs FOUND\n"), csFullPath, szVirname);
      wsprintf (szOutput, TEXT("%hs FOUND"), szVirname) ;
    }
  else if( iResultCode != CL_CLEAN )
    {
      TRACE_ERROR (TEXT("%hs : Error: %hs\n"), csFullPath, cl_strerror(iResultCode));
      wsprintf (szOutput, TEXT("Error: %hs\n"), cl_strerror(iResultCode)) ;    
    }
    
  return
    iResultCode==CL_CLEAN ? SCAN_NO_VIRUS :
    iResultCode==CL_VIRUS ? SCAN_VIRUS_FOUND :
    SCAN_FAILED ;

  return SCAN_NO_VIRUS ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

VOID _Scanner_LibClamav_DatabaseUpdated (LPVOID pContext) 
{
  _Scanner_LibClamav_LoadDatabase ((LIBCLAMAVCONF*)pContext) ;
}
