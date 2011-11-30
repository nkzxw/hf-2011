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
#include "UpdWatcher.h"

// standard headers
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <wininet.h>

// project's headers
#include "Assert.h"
#include "Config.h"
#include "ProjectInfo.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define MAX_MESSAGE_LEN		256

static const DWORD	dwCheckIntervalIfFound = 4/*hours*/*60*60*1000 ;
static const DWORD	dwCheckIntervalIfNotFound = 10/*minutes*/*60*1000 ;

static const TCHAR	szServerName[] = TEXT("www.winpooch.com") ;
static const int	nServerPort = 80 ;
static const TCHAR	szUsername[] = TEXT("") ;  // but not NULL
static const TCHAR	szPassword[] = TEXT("") ;  // but not NULL
static const TCHAR	szObjectName[] = TEXT("version.php?current=" APPLICATION_VERSION_STRING) ;



/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

// real structure behind HUPDWATCHER
typedef struct {
  HWND		hwnd ;
  HANDLE	hThread ;
  HANDLE	hStopEvent ;
  TCHAR		szNewVersion[32] ;
  TCHAR		szDownloadPage[MAX_PATH] ;
} UPDWATCHER ;

typedef enum {
  CHECK_UNDEFINED,
  CHECK_DISABLED,
  CHECK_NO_CONNECTION,
  CHECK_OPEN_SESSION_FAILED,
  CHECK_OPEN_CONNECT_FAILED,
  CHECK_OPEN_REQUEST_FAILED,
  CHECK_SEND_REQUEST_FAILED,
  CHECK_READ_FILE_FAILED,
  CHECK_LOOK_FOR_VERSION_FAILED,
  CHECK_SAME_VERSION,
  CHECK_DIFFERENT_VERSION
} CHECK_RESULT ;


/******************************************************************/
/* Internal function : Check                                      */
/******************************************************************/

CHECK_RESULT _UpdWatcher_Check (UPDWATCHER * p)
{
  BOOL bSuccess ;
  CHECK_RESULT nResult = CHECK_UNDEFINED ;

  if( Config_GetInteger(CFGINT_CHECK_FOR_UPDATES) )
    {
      DWORD dwState = INTERNET_CONNECTION_OFFLINE ;
      
      // check if a connection is available
      bSuccess = InternetGetConnectedState (&dwState, 0) ;

      // connection available ?
      if( bSuccess ) 
	{
	  // open session
	  HINTERNET hSession = InternetOpen (TEXT(APPLICATION_NAME), 0, NULL, NULL, 0) ;
	  
	  // session opened ?
	  if( hSession )
	    {
	      // open connection
	      HINTERNET hConnect = InternetConnect (hSession, szServerName, nServerPort,
						    szUsername, szPassword, 
						    INTERNET_SERVICE_HTTP, 0,0) ;
	      
	      // connexion opened ?
	      if( hConnect )
		{
		  // open request
		  HINTERNET hRequest = HttpOpenRequest (hConnect, TEXT("GET"), szObjectName,
							HTTP_VERSION, NULL, NULL, 
							INTERNET_FLAG_RELOAD,0) ;
		  
		  // request opened ?
		  if( hRequest )
		    {  
		      // send request
		      bSuccess = HttpSendRequest (hRequest, NULL, 0, 0, 0) ;
		      
		      // request sent ?
		      if( bSuccess )
			{
			  char	szContent[256] ;
			  DWORD dwContentMax = 256 ;
			  DWORD dwBytesRead ;
			  
			  // read file
			  bSuccess = InternetReadFile (hRequest, szContent,
						       dwContentMax, 
						       &dwBytesRead);
			  
			  // failed to read file ?
			  if( bSuccess )
			    {			      
			      char * szVersion ;
			      
			      szContent[dwBytesRead] = 0 ;	      
			      
			      // look for version string
			      szVersion = strstr (szContent, "<!-- Version: ") ;
			      
			      // failed ?
			      if( szVersion )
				{
				  int nHigh, nMed, nLow ;
				  int nNetVersion, nMyVersion ;

				  szVersion += 14 ;

				  // read net's version
				  sscanf (szVersion, "%d.%d.%d", 
					  &nHigh, &nMed, &nLow) ;
				  nNetVersion = (nHigh*256 + nMed)*256 + nLow ;
				  TRACE_INFO (TEXT("Net version = %d.%d.%d\n"), nHigh, nMed, nLow) ;

				  // save net version
				  wsprintf (p->szNewVersion, TEXT("%d.%d.%d"), nHigh, nMed, nLow) ;
				  
				  // read my version
				  sscanf (APPLICATION_VERSION_STRING, "%d.%d.%d", 
					  &nHigh, &nMed, &nLow) ;			  
				  nMyVersion = (nHigh*256 + nMed)*256 + nLow ;
				  TRACE_INFO  (TEXT("Local version = %d.%d.%d\n"), nHigh, nMed, nLow) ;
				  			
				  // compare versions
				  bSuccess = nNetVersion > nMyVersion ;	      
		      				  
				  nResult = bSuccess ? CHECK_DIFFERENT_VERSION : CHECK_SAME_VERSION ;

				  if( bSuccess )
				    {
				      char *szStartPage, *szEndPage ;
				      
				      // look for page address
				      szStartPage = strstr (szContent, "<!-- Page: ") ;
				      
				      // failed ?
				      if( szStartPage )
					{
					  szStartPage += 11 ;
					  
					  szEndPage = strstr (szStartPage, " -->") ;
					  
					  if( szEndPage )
					    {
					      int nLen = min (szEndPage-szStartPage, MAX_PATH) ;
					      
					      nLen = MultiByteToWideChar (CP_ACP, 0,
									  szStartPage, nLen,
									  p->szDownloadPage, MAX_PATH) ;
					      p->szDownloadPage[nLen] = 0 ;

					      TRACE_INFO (TEXT("Download page = %s\n"), p->szDownloadPage) ;
					    }
					}
				    }
				}
			      else nResult = CHECK_LOOK_FOR_VERSION_FAILED ;
			      
			    }
			  else nResult = CHECK_READ_FILE_FAILED ;
			}
		      else nResult = CHECK_SEND_REQUEST_FAILED ;
		      
		      // close request
		      InternetCloseHandle (hRequest) ;
		    }
		  else nResult = CHECK_OPEN_REQUEST_FAILED ;
		  
		  // close connection
		  InternetCloseHandle (hConnect) ;
		}
	      else nResult = CHECK_OPEN_CONNECT_FAILED ;
	      
	      // close session
	      InternetCloseHandle (hSession) ;
	    }
	  else nResult = CHECK_OPEN_SESSION_FAILED ;
	}
      else nResult = CHECK_NO_CONNECTION ;
    }
  else nResult = CHECK_DISABLED ;
  
  return nResult ;
}


/******************************************************************/
/* Internal function : Thread                                     */
/******************************************************************/

// internal functions
DWORD WINAPI _UpdWatcher_Thread (LPVOID pContext)
{
  UPDWATCHER * p = (UPDWATCHER*) pContext ;
  BOOL bGoOn = TRUE ;
  DWORD dwWaitTime ;

  while( bGoOn )
    {
      switch( _UpdWatcher_Check (p) )
	{
	case CHECK_DIFFERENT_VERSION:
	  PostMessage (p->hwnd, WM_UPDATE_FOUND, 0, (LPARAM)p) ;
	  bGoOn = FALSE ;
	case CHECK_SAME_VERSION:
	  dwWaitTime = dwCheckIntervalIfFound ;
	  break ;
	case CHECK_DISABLED:
	default:
	  dwWaitTime = dwCheckIntervalIfNotFound ;
	}

      if( ! bGoOn ) break ;
      
      if( WAIT_OBJECT_0==WaitForSingleObject (p->hStopEvent, dwWaitTime) )
	bGoOn = FALSE ;
    }
  
  return 0 ;
}


/******************************************************************/
/* Exported function : New                                        */
/******************************************************************/

HUPDWATCHER UpdWatcher_New (HWND hwnd)
{
  UPDWATCHER * p ;
  DWORD	dwThreadId ;
  
  p = (UPDWATCHER*) malloc (sizeof(UPDWATCHER)) ;

  p->hwnd = hwnd ;
  p->hStopEvent = CreateEvent (NULL, FALSE, FALSE, NULL) ;

  p->hThread = CreateThread (NULL, 0, _UpdWatcher_Thread, p, 0, &dwThreadId) ;

  wsprintf (p->szDownloadPage, TEXT("http://%s/"), szServerName) ;
  _tcscpy (p->szNewVersion, TEXT(APPLICATION_VERSION_STRING)) ;
  
  return p ;
}


/******************************************************************/
/* Exported function : Delete                                     */
/******************************************************************/

VOID UpdWatcher_Delete (HUPDWATCHER h)
{
  UPDWATCHER * p = (UPDWATCHER*)h ;

  if( p ) 
    {
      SetEvent (p->hStopEvent) ;

      WaitForSingleObject (p->hThread, 10000) ;

      CloseHandle (p->hThread) ;
      CloseHandle (p->hStopEvent) ;

      free (p) ;
    }
}


/******************************************************************/
/* Exported function :                                            */
/******************************************************************/

LPCTSTR UpdWatcher_GetNewVersion (HUPDWATCHER h)
{
  UPDWATCHER * p = (UPDWATCHER*)h ;
  
  ASSERT (h!=NULL) ;

  return p->szNewVersion ;
}


/******************************************************************/
/* Exported function :                                            */
/******************************************************************/

LPCTSTR UpdWatcher_GetDownloadPage (HUPDWATCHER h)
{
  UPDWATCHER * p = (UPDWATCHER*)h ;
  
  ASSERT (h!=NULL) ;

  return p->szDownloadPage ;
}
