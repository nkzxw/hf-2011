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

//#define TRACE_LEVEL 2	// warning level


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "FreshClam.h"

// standard headers
#include <shlwapi.h>

// project's headers
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define TIME_BEFORE_START	3 // minutes

#define TIME_INTERVAL		3*60 // minutes

#define MAX_EXECUTE_TIME	5 // minutes


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  BOOL		bStarted ;
  HANDLE	hThread ;
  HANDLE	hStopEvent ;
  FRESHCLAMCALLBACK pfnCallback ;
  PVOID		pCbContext ;
} INTERNAL_DATA ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static INTERNAL_DATA g_data ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

DWORD WINAPI _FreshClam_ThreadProc (LPVOID) ;



/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL FreshClam_Start (FRESHCLAMCALLBACK pfnCallback, PVOID pCbContext) 
{
  TRACE ;

  g_data.pfnCallback = pfnCallback ;
  g_data.pCbContext = pCbContext ;

  g_data.hStopEvent = CreateEvent (NULL, TRUE, FALSE, NULL) ;
  
  g_data.hThread = CreateThread (NULL, 0, _FreshClam_ThreadProc, NULL, CREATE_SUSPENDED, NULL) ;

  SetThreadPriority (g_data.hThread, THREAD_PRIORITY_IDLE) ;

  g_data.bStarted = TRUE ;

  ResumeThread (g_data.hThread) ;  
  
  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL FreshClam_Stop () 
{
  TRACE ;

  if( g_data.bStarted )
    {
      g_data.bStarted = FALSE ;
      
      SetEvent (g_data.hStopEvent) ;
      
      TRACE_INFO (TEXT("Waiting for thread to stop\n")) ;
      WaitForSingleObject (g_data.hThread, INFINITE) ;
      
      CloseHandle (g_data.hStopEvent) ;
      CloseHandle (g_data.hThread) ;
      
      g_data.hThread = NULL ;
      g_data.hStopEvent = NULL ; 
    }

  return TRUE ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

DWORD WINAPI _FreshClam_ThreadProc (LPVOID pData) 
{
  TCHAR szDirectory[MAX_PATH] ;
  TCHAR szCmdLine[] = TEXT("freshclam --log=freshclam.log") ;
  UINT	nPriorityClass = IDLE_PRIORITY_CLASS ;
  DWORD	nWaitResult ;

  TRACE_INFO (TEXT("Freshclam thread started\n")) ;

  // get directory
  GetModuleFileName (NULL, szDirectory, MAX_PATH) ;
  PathRemoveFileSpec (szDirectory) ;

  // wait 5 minutes before start...
  nWaitResult = WaitForSingleObject (g_data.hStopEvent, TIME_BEFORE_START*60*1000) ;     
  
  while( nWaitResult!=WAIT_OBJECT_0 && g_data.bStarted )
    {
      STARTUPINFO         si = { sizeof(STARTUPINFO) } ;
      PROCESS_INFORMATION pi = { 0 } ;

      HANDLE	aEvents[2] ;

      si.dwFlags = STARTF_USESHOWWINDOW ;
      si.wShowWindow = SW_HIDE ;

      TRACE_INFO (TEXT("Starting freshclam.exe\n")) ;

      if( ! CreateProcess (NULL, szCmdLine, NULL, NULL, TRUE, 
			   nPriorityClass, 
			   NULL, szDirectory, &si, &pi) ) 
	{
	  TRACE_ERROR (TEXT("Failed to launch freshclam.exe (error=%d)\n"), GetLastError()) ;
	}

      aEvents[0] = pi.hProcess ;
      aEvents[1] = g_data.hStopEvent ;
  
      // wait for process end
      nWaitResult = WaitForMultipleObjects (2, aEvents, FALSE, MAX_EXECUTE_TIME*60*1000) ;

      // execution completed ? => read error code
      if( nWaitResult==WAIT_OBJECT_0 )
	{
	  DWORD dwExitCode = -1 ;
	  GetExitCodeProcess (pi.hProcess, &dwExitCode) ;

	  if( dwExitCode==0 )
	    {
	      TRACE_INFO (TEXT("Clamav database updated successfully\n")) ;
	      if( g_data.pfnCallback ) g_data.pfnCallback(g_data.pCbContext) ;
	    }
	  else if( dwExitCode==1 )
	    {
	      TRACE_INFO (TEXT("Database up-to-date\n")) ;
	    }
	  else
	    {
	      TRACE_ERROR (TEXT("freshclam.exe execution failed (exitcode=%u), see freshclam.log\n"), 
			   dwExitCode) ;
	    }
	}

      // time-out or stop request ? => need to kill process
      if( nWaitResult!=WAIT_OBJECT_0 )
	{
	  TRACE_ERROR (TEXT("freshclam.exe execution time-out (%d minutes)\n"), MAX_EXECUTE_TIME) ;
	  TerminateProcess (pi.hProcess, 1) ;
	}
      
      // process handles are no more needed
      CloseHandle (pi.hProcess) ;
      CloseHandle (pi.hThread) ;

      // wait for 4 hours...
      nWaitResult = WaitForSingleObject (g_data.hStopEvent, TIME_INTERVAL*60*1000) ;
    }
  
  TRACE_INFO (TEXT("Freshclam thread stopped\n")) ;

  return 0 ;
}
