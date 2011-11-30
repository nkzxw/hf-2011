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
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "Trace.h"

// standard headers
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>

// project's headers
#include "ProjectInfo.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define HEAD_LENGTH		0
//#define HEAD_LENGTH		24
#define MAX_PROC_NAME_LEN	8


/******************************************************************/
/* Exported function : DbgPrint                                   */
/******************************************************************/

VOID DbgPrint (LPCTSTR szFormat, ...)
{
  TCHAR		szBuffer[1024] ;  
  va_list	va ;
  DWORD		dwLastError ;

  // WORK-AROUND :
  // OutputDebugString sometimes change the value of last error
  // so we need to save and restore this value
  dwLastError = GetLastError () ;

#if HEAD_LENGTH > 0

  static TCHAR szHead[HEAD_LENGTH] ;

  if( ! szHead[0] ) 
    {
      int i ;
      GetModuleBaseName (GetCurrentProcess(), NULL, szBuffer, MAX_PROC_NAME_LEN) ;
      szBuffer[MAX_PROC_NAME_LEN] = 0 ;
      _tcslwr (szBuffer) ;
      if( _tcsrchr(szBuffer,TEXT('.')) ) _tcsrchr(szBuffer,TEXT('.'))[0] = 0 ;
      wsprintf (szHead, TEXT(APPLICATION_NAME "%d %s"), 
		APPLICATION_BUILD,		
		szBuffer) ;
      for( i=_tcslen(szHead) ; i<HEAD_LENGTH-1 ; i++ )
	szHead[i] = TEXT(' ') ;
      szHead[i] = 0 ;
    }

  _tcscpy (szBuffer, szHead) ;

  wsprintf (szBuffer+_tcslen(szBuffer), TEXT("p=%4d t=%4d   "), 
	    GetCurrentProcessId(), GetCurrentThreadId()) ;

  va_start (va, szFormat) ;
  wvsprintf (szBuffer+_tcslen(szBuffer), szFormat, va) ;
  va_end (va) ;

#else
  
  va_start (va, szFormat) ;
  wvsprintf (szBuffer, szFormat, va) ;
  va_end (va) ;
  
#endif
  
  OutputDebugString (szBuffer) ;

  // WORK-AROUND :
  // OutputDebugString sometimes change the value of last error
  // so we need to save and restore this value
  SetLastError (dwLastError) ;
}
