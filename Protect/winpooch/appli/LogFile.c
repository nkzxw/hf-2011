/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2005  Benoit Blanchon                      */
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
#include "LogFile.h"

// standards headers
#include <shlobj.h>
#include <stdio.h>
#include <tchar.h>

// project's headers
#include "Config.h"
#include "ProjectInfo.h"


/******************************************************************/
/* Internal function : Thread                                     */
/******************************************************************/

VOID _LogFile_Append (LPCTSTR szText) ;

VOID _LogFile_Rotate () ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static HANDLE		g_hLogFile ;
static CRITICAL_SECTION g_critsec ;
static TCHAR		g_szPath[MAX_PATH] ;
static UINT		g_nMaxSize ;


/******************************************************************/
/* Exported function : Init                                       */
/******************************************************************/

BOOL LogFile_Init (LPCTSTR szFile) 
{
  BOOL bAlreadyExists ;
  TCHAR *p ;

  // read configuration
  LogFile_ReloadConfig () ;

  // set the file path 
  GetModuleFileName (NULL, g_szPath, MAX_PATH) ;
  p = _tcsrchr (g_szPath, TEXT('\\')) ;
  _tcscpy (p+1, szFile) ;

  g_hLogFile = CreateFile (g_szPath, GENERIC_WRITE,
			 FILE_SHARE_READ, NULL, 
			 OPEN_ALWAYS, 
			 FILE_FLAG_WRITE_THROUGH|FILE_ATTRIBUTE_NORMAL,
			 NULL) ;
  
  if( g_hLogFile == INVALID_HANDLE_VALUE ) 
    {
      // if it failed to create the file in the executable's directory
      // then try in Application Data
      
      if( FAILED(SHGetFolderPath (NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, g_szPath)) )
	return FALSE ;
      _tcscat (g_szPath, TEXT("\\")) ;
      _tcscat (g_szPath, TEXT(APPLICATION_NAME)) ;
      CreateDirectory (g_szPath, NULL) ;
      _tcscat (g_szPath, TEXT("\\")) ;  
      _tcscat (g_szPath, szFile) ;

      g_hLogFile = CreateFile (g_szPath, GENERIC_WRITE,
			     FILE_SHARE_READ, NULL, 
			     OPEN_ALWAYS, 
			     FILE_FLAG_WRITE_THROUGH|FILE_ATTRIBUTE_NORMAL,
			     NULL) ;
      
      if( g_hLogFile == INVALID_HANDLE_VALUE ) 	
	return FALSE ;
    }

  bAlreadyExists = GetLastError() == ERROR_ALREADY_EXISTS ;

  SetFilePointer (g_hLogFile, 0, NULL, FILE_END) ;

  if( bAlreadyExists )
    _LogFile_Append (TEXT("\r\n")) ;
  
  InitializeCriticalSection(&g_critsec) ;

  return TRUE ;
}

/******************************************************************/
/* Exported function : Uninit                                     */
/******************************************************************/

VOID LogFile_Uninit ()
{
  CloseHandle (g_hLogFile) ; 
  DeleteCriticalSection (&g_critsec) ;
}


/******************************************************************/
/* Internal function : Append                                     */
/******************************************************************/

VOID _LogFile_Append (LPCTSTR szText) 
{
  DWORD dwBytesWritten ;

  if( g_nMaxSize!=0 && 
      lstrlen(szText)>5 &&
      GetFileSize(g_hLogFile,NULL)>=g_nMaxSize )
    _LogFile_Rotate () ;

  WriteFile (g_hLogFile,                // handle
	     szText,                  // buffer
	     lstrlen(szText)*sizeof(TCHAR), // size
	     &dwBytesWritten,          // bytes written
	     NULL) ;                  // overlapped
}


/******************************************************************/
/* Exported function : Print                                      */
/******************************************************************/

VOID LogFile_Print (LPCTSTR szText) 
{
  static DWORD dwTickPrevious ; 
  DWORD dwTickCurrent ;
  TCHAR szBuffer[32] ;
  DWORD dwSize ;

  EnterCriticalSection (&g_critsec) ;
   
  dwTickCurrent = GetTickCount() ;
  
  if( dwTickCurrent-dwTickPrevious > 2000 )
    {

      // get date string  
      GetDateFormat (LOCALE_USER_DEFAULT, // locale
		     DATE_LONGDATE,      // flags
		     NULL,                // date
		     NULL,                // format
		     szBuffer,            // string
		     sizeof(szBuffer)/sizeof(TCHAR)) ;  // buffer size
      
      // append date
      _LogFile_Append (szBuffer) ;
      
      // append separator
      _LogFile_Append (TEXT(" - ")) ;  
      
      // get time string
      GetTimeFormat (LOCALE_USER_DEFAULT, // locale
		     TIME_NOTIMEMARKER|   // flags
		     TIME_FORCE24HOURFORMAT,
		     NULL,                // time
		     NULL,                // format
		     szBuffer,            // string
		     sizeof(szBuffer)/sizeof(TCHAR)) ;  // buffer size
      
      // append time
      _LogFile_Append (szBuffer) ;
      
      // append separator
      _LogFile_Append (TEXT(" - ")) ; 
      
      // get the user name
      dwSize = sizeof(szBuffer)/sizeof(TCHAR) ;
      GetUserName (szBuffer, &dwSize) ;
      
      // append user name
      _LogFile_Append (szBuffer) ;
      
      // append newline and indent
      _LogFile_Append (TEXT("\r\n")) ;

    }

  // append indent
  _LogFile_Append (TEXT(" - ")) ;
      
  // append text
  _LogFile_Append (szText) ;

  // append newline
  _LogFile_Append (TEXT("\r\n")) ;

  dwTickPrevious = dwTickCurrent ;

  LeaveCriticalSection (&g_critsec) ;
}


/******************************************************************/
/* Exported function : Printf                                     */
/******************************************************************/

VOID LogFile_Printf (LPCTSTR szFormat, ...)
{
  TCHAR szMessage[1024] ;
  va_list args ;
  
  va_start (args, szFormat) ;
  
  _vsntprintf (szMessage, 1024, szFormat, args) ;
  
  va_end (args) ;

  LogFile_Print (szMessage) ;
}


/******************************************************************/
/* Exported function : GetPath                                    */
/******************************************************************/

LPCTSTR LogFile_GetPath ()
{
  return g_szPath ;
}


/******************************************************************/
/* Exported function : ReloadConfig                               */
/******************************************************************/

BOOL LogFile_ReloadConfig ()
{
  UINT	n = Config_GetInteger (CFGINT_MAX_LOG_SIZE) ;

  if( n!=0 && n<1024 ) n = 1024 ;

  Config_SetInteger (CFGINT_MAX_LOG_SIZE, n) ;

  g_nMaxSize = n ;

  return TRUE ;
}


/******************************************************************/
/* Internal function : Rotate                                     */
/******************************************************************/

VOID _LogFile_Rotate ()
{
  TCHAR szOldPath[MAX_PATH] ;

  CloseHandle (g_hLogFile) ;

  _tcscpy (szOldPath, g_szPath) ;
  _tcscat (szOldPath, TEXT(".old")) ;

  DeleteFile (szOldPath) ;
  MoveFile (g_szPath, szOldPath) ;

  g_hLogFile = CreateFile (g_szPath, GENERIC_WRITE,
			   FILE_SHARE_READ, NULL, 
			   OPEN_ALWAYS, 
			   FILE_FLAG_WRITE_THROUGH|FILE_ATTRIBUTE_NORMAL,
			   NULL) ;
}
