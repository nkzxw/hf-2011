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
#include "BkgndScan.h"

// standard headers
#include <tchar.h>
#include <shlwapi.h>

// project's headers
#include "Config.h"
#include "ProcList.h"
#include "Scanner.h"
#include "SpyServer.h"
#include "Trace.h"
#include "Wildcards.h"


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  BOOL		bStarted ;
  HANDLE	hThread ;
  HANDLE	hStopEvent ;
  LPTSTR	*pszFilters ;
  UINT		nFilters ;
} INTERNAL_DATA ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static INTERNAL_DATA g_data ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

DWORD WINAPI _BkgndScan_ThreadProc (LPVOID) ;

VOID _BkgndScan_ScanFolder (LPCTSTR szFolder) ;

VOID _BkgndScan_ScanFile (LPCTSTR szFile) ;

BOOL _BkgndScan_CanScanFile (LPCTSTR szFile) ;

BOOL _BkgndScan_EnumProcessCallback (void* pContext, PROCSTRUCT* pProc) ;


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL BkgndScan_Start () 
{
  {
    LPCTSTR	*pszFilters ;
    UINT	i, nFilters ;
    
    pszFilters = Config_GetStringArray (CFGSAR_SCAN_PATTERNS, &nFilters) ;

    g_data.nFilters = nFilters ;

    if( nFilters > 0 )
      {
	g_data.pszFilters = malloc (nFilters*sizeof(LPCTSTR)) ;
		
	for( i=0 ; i<nFilters ; i++ )
	  g_data.pszFilters[i] = _tcsdup (pszFilters[i]) ;	  
      }
  }

  g_data.hStopEvent = CreateEvent (NULL, TRUE, FALSE, NULL) ;
  
  g_data.hThread = CreateThread (NULL, 0, _BkgndScan_ThreadProc, NULL, CREATE_SUSPENDED, NULL) ;

  SetThreadPriority (g_data.hThread, THREAD_PRIORITY_IDLE) ;

  g_data.bStarted = TRUE ;

  ResumeThread (g_data.hThread) ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL BkgndScan_Stop () 
{
  if( g_data.bStarted )
    {
      g_data.bStarted = FALSE ;
      
      SetEvent (g_data.hStopEvent) ;
      
      WaitForSingleObject (g_data.hThread, INFINITE) ;
      
      CloseHandle (g_data.hStopEvent) ;
      CloseHandle (g_data.hThread) ;
      
      g_data.hThread = NULL ;
      g_data.hStopEvent = NULL ; 
      
      {
	UINT i ;

	for( i=0 ; i<g_data.nFilters ; i++ )
	  free (g_data.pszFilters[i]) ;
	
	free (g_data.pszFilters) ;
	
	g_data.pszFilters = NULL ;
	g_data.nFilters = 0 ;    
      }
    }

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL BkgndScan_ReloadConfig () 
{
  BkgndScan_Stop () ;
  BkgndScan_Start () ;
  
  return TRUE ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

DWORD WINAPI _BkgndScan_ThreadProc (LPVOID pData) 
{
  UINT iFolder ;
  UINT nFolders ;
  LPTSTR *pszFolders ;

  if( ! Scanner_IsConfigured() )
    return 0 ;

  //
  // First scan running programs
  //
  ProcList_Enum (_BkgndScan_EnumProcessCallback, NULL) ;  
 
  //
  // Get folder list
  //
  {
    LPCTSTR *pszConfig ;
    UINT i ;

    pszConfig = Config_GetStringArray (CFGSAR_SCAN_FOLDERS, &nFolders) ;

    if( nFolders == 0 ) return 0 ;

    pszFolders = malloc (nFolders*sizeof(LPTSTR)) ;

    for( i=0 ; i<nFolders ; i++ )
      pszFolders[i] = _tcsdup (pszConfig[i]) ;
  }

  for( iFolder=0 ; iFolder<nFolders ; iFolder++ )
    { 
      // Is it time to stop ?
      if( WAIT_OBJECT_0==WaitForSingleObject(g_data.hStopEvent,0) )
	break ;

      _BkgndScan_ScanFolder (pszFolders[iFolder]) ;
    }

  //
  // Free folder list
  //
  {
    UINT i ;

    for( i=0 ; i<nFolders ; i++ )
      free(pszFolders[i]) ;

    free (pszFolders) ;
  }
  
  return 0 ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _BkgndScan_EnumProcessCallback (void* pContext, PROCSTRUCT* pProc) 
{
  // Is it time to stop ?
  if( WAIT_OBJECT_0==WaitForSingleObject(g_data.hStopEvent,0) )
    return FALSE ;

  if( pProc->szPath[0] && _BkgndScan_CanScanFile (pProc->szPath) )
    {
      _BkgndScan_ScanFile (pProc->szPath) ;
      Sleep (100) ;
    }

  return TRUE ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

VOID _BkgndScan_ScanFolder (LPCTSTR szFolder)
{
  TCHAR szFilePath[MAX_PATH] ;
  HANDLE hFind ;
  WIN32_FIND_DATA find ;   

  TRACE_INFO (TEXT("Start scanning folder %s\n"), szFolder) ;
  
  PathCombine (szFilePath, szFolder, TEXT("*")) ;
  
  hFind = FindFirstFile (szFilePath, &find) ;
  
  if( hFind != INVALID_HANDLE_VALUE )
    {
      
      do {
	
	PathCombine (szFilePath, szFolder, find.cFileName) ;
	
	// Is it time to stop ?
	if( WAIT_OBJECT_0==WaitForSingleObject(g_data.hStopEvent,0) )
	  break ;
	
	if( find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	  {
	    if( !_tcscmp(find.cFileName,TEXT(".")) || !_tcscmp(find.cFileName,TEXT("..")) )
	      TRACE_INFO (TEXT("Skipped directory %s\n"), find.cFileName) ;
	    else
	      _BkgndScan_ScanFolder (szFilePath) ;
	  }
	else
	  {
	    if( _BkgndScan_CanScanFile (szFilePath) )
	      {
		_BkgndScan_ScanFile (szFilePath) ;		    
		Sleep (2000) ;
	      }
	  }
	
      } while( FindNextFile(hFind,&find) ) ;
      
    }
  else
    {
      TRACE_INFO (TEXT("Can't read directory %s\n"), szFolder) ;
    }
  
  FindClose(hFind) ;

  TRACE_INFO (TEXT("Finished scanning folder %s\n"), szFolder) ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _BkgndScan_CanScanFile (LPCTSTR szFilePath)
{
  UINT i ;

  if( ! _tcsicmp(Scanner_GetScannerExe(),szFilePath) )
    return FALSE ;

  for( i=0 ; i<g_data.nFilters ; i++ )
    {
      LPCWSTR szFilter = g_data.pszFilters[i] ;
      
      if( Wildcards_CompleteStringCmp(szFilter,szFilePath) )
	return TRUE ;
    }

  return FALSE ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

VOID _BkgndScan_ScanFile (LPCTSTR szFilePath)
{
  TRACE_INFO (TEXT("Scan file %s\n"), szFilePath) ;

  UINT		nScanResult ;      
  SYSTEMTIME	stScanTime ;
  FILETIME	ftScanTime ;
  
  GetLocalTime (&stScanTime) ;
  SystemTimeToFileTime (&stScanTime, &ftScanTime) ;
  
  nScanResult = SpySrv_ScanFile (szFilePath, TRUE) ;

  SpySrv_AddFileToCache (szFilePath, nScanResult, (LARGE_INTEGER*)&ftScanTime) ;
}
