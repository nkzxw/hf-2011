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

#define	TRACE_LEVEL	2


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "Link.h"

// windows's headers
#include <windows.h>
#include <winioctl.h>

// project's headers
#include "Assert.h"
#include "FiltCond.h"
#include "DrvInterface.h"
#include "Trace.h"

#define SLOT_COUNT		3
#define LINK_BUFFER_SIZE	4096

typedef struct {
  int		iSlot ;
} REQUEST_HEADER ;

typedef struct {
  int		iSlot ;
} RESPONSE_HEADER ;

typedef struct {
  BYTE		pBuffer[LINK_BUFFER_SIZE] ;
  UINT		nRequestSize ;
} SLOT ;

typedef struct {
  HANDLE	hThread ;
  HANDLE	hDriver ;
  HANDLE	hStopEvent ;
  LINKCALLBACK	pfnCallback ;
  SLOT		aSlots[SLOT_COUNT] ;
} INTERNAL_DATA ;

static INTERNAL_DATA g_data ;


DWORD WINAPI _Link_RequestThreadProc (LPVOID pParam) ;



DWORD WINAPI Link_ThreadProc (LPVOID lpParameter)
{
  BYTE		pBuffer[LINK_BUFFER_SIZE] ;
  DWORD		nRequestSize ;
  BOOL		bSuccess ;
  HANDLE	aEvents[2] ;
  OVERLAPPED	ov ;

  TRACE ;

  ov.hEvent = CreateEvent (NULL, TRUE, FALSE, NULL) ;

  // list of events
  aEvents[0] = ov.hEvent ;
  aEvents[1] = g_data.hStopEvent ;

  while(1)
    {
      TRACE_INFO (TEXT("Waiting request from driver...\n")) ;
      
      bSuccess = DeviceIoControl (g_data.hDriver, 
				  IOCTL_LINK_DRV2APP,
				  NULL, 0, 
				  pBuffer,
				  LINK_BUFFER_SIZE,
				  &nRequestSize,
				  &ov) ;      
      
      if( ! bSuccess )
	{
	  if( GetLastError()==ERROR_IO_PENDING )
	    {
	      DWORD nWaitResult = WaitForMultipleObjects (2, aEvents, FALSE, INFINITE) ;
	      
	      if( nWaitResult != WAIT_OBJECT_0 )
		{
		  TRACE_INFO (TEXT("Exiting from app-link loop\n")) ;
		  break ;
		}
	      
	      GetOverlappedResult (g_data.hDriver, &ov, &nRequestSize, FALSE) ;
	    }
	  else
	    {
	      TRACE_ERROR (TEXT("DeviceIoControl failed (0x%08X)\n"), GetLastError()) ;
	      break ;
	    }
	}
      
      if( nRequestSize >= sizeof(REQUEST_HEADER) )
	{
	  UINT		iSlot ;

	  iSlot = ((REQUEST_HEADER*)pBuffer)->iSlot ;

	  if( iSlot < SLOT_COUNT )
	    {	  
	      HANDLE	hThread ;
	      
	      TRACE_INFO (TEXT("Slot%u : Request received\n"), iSlot) ;
	  
	      memcpy (g_data.aSlots[iSlot].pBuffer, pBuffer, LINK_BUFFER_SIZE) ;
	      g_data.aSlots[iSlot].nRequestSize = nRequestSize ;
	  
	      hThread = CreateThread (NULL, 0, _Link_RequestThreadProc, (LPVOID)iSlot, 0, NULL) ;
	      CloseHandle (hThread) ;
	    }
	  else
	    {
	      TRACE_ERROR (TEXT("Slot number (%u) is invalid\n"), iSlot) ;
	    }
	}
      else
	{
	  TRACE_ERROR (TEXT("Request size (%u) is invalid\n"), nRequestSize) ;
	}
    }
      
  TRACE_INFO (TEXT("Cancelling pending IO\n")) ;
  CancelIo (g_data.hDriver) ;
  TRACE_INFO (TEXT("Cancel completed\n")) ;
      
  CloseHandle (ov.hEvent) ;
  
  return 0 ;
}

DWORD WINAPI _Link_RequestThreadProc (LPVOID pParam)
{
  OVERLAPPED	ov ;
  UINT	iSlot = (UINT)pParam ;
  SLOT * pSlot = &g_data.aSlots[iSlot] ;
  BOOL bSuccess ;
  UINT nResponseSize ;
  
  ov.hEvent = CreateEvent (NULL, TRUE, FALSE, NULL) ;
  
  TRACE_INFO (TEXT("Slot%u : Handling request %u\n"), iSlot, ((DWORD*)pSlot->pBuffer)[1]) ;
  
  nResponseSize = g_data.pfnCallback (pSlot->pBuffer+sizeof(REQUEST_HEADER), 
				      pSlot->nRequestSize-sizeof(REQUEST_HEADER)) ;
  
  TRACE_INFO (TEXT("Slot%u : Request handled\n"), iSlot) ;

  TRACE_INFO (TEXT("Slot%u : Sending response (size=%d)\n"), iSlot, nResponseSize) ;
  
  nResponseSize += sizeof(RESPONSE_HEADER) ;
  
  bSuccess = DeviceIoControl (g_data.hDriver, 
			      IOCTL_LINK_APP2DRV,
			      pSlot->pBuffer, nResponseSize, 
			      NULL, 0, NULL,
			      &ov) ;      
  
  if( !bSuccess )
    {
      if( GetLastError()==ERROR_IO_PENDING )
	{
	  DWORD nWaitResult = WaitForSingleObject (ov.hEvent, INFINITE) ;
	  
	  if( nWaitResult != WAIT_OBJECT_0 )
	    {
	      TRACE_ERROR (TEXT("Slot%u : DeviceIoControl failed (error=%u)\n"), iSlot, GetLastError()) ;
	      return 1 ;
	    }
	}
      else
	{
	  TRACE_ERROR (TEXT("Slot%u : DeviceIoControl failed (error=%u)\n"), iSlot, GetLastError()) ;
	  return 1 ;
	}
    }  

  
  if( nResponseSize>sizeof(RESPONSE_HEADER) )
    TRACE_INFO (TEXT("Slot%u : Response sent\n"), iSlot) ;
  else
    TRACE_INFO (TEXT("Slot%u : Ack sent\n"), iSlot) ;

  CloseHandle (ov.hEvent) ;

  return 0 ;
}


BOOL Link_Init (HANDLE hDriver, LINKCALLBACK pfnCallback) 
{
  DWORD	dwTid ;

  TRACE ;

  ASSERT (hDriver!=NULL) ;
  ASSERT (hDriver!=INVALID_HANDLE_VALUE) ;
  ASSERT (pfnCallback!=NULL) ;

  g_data.hDriver = hDriver ;
  g_data.pfnCallback = pfnCallback ;

  g_data.hStopEvent = CreateEvent (NULL, TRUE, FALSE, NULL) ;

  g_data.hThread = CreateThread (NULL, 0, Link_ThreadProc, 
				 NULL, CREATE_SUSPENDED, &dwTid) ;

  SetThreadPriority (g_data.hThread, THREAD_PRIORITY_HIGHEST) ;

  ResumeThread (g_data.hThread) ;

  return TRUE ;
}

BOOL Link_Uninit () 
{
  SetEvent (g_data.hStopEvent) ;

  TRACE_INFO (TEXT("Waiting for app-link disconnection\n")) ;
  
  if( WAIT_OBJECT_0 != WaitForSingleObject (g_data.hThread, 10*1000) )
    {
      TRACE_ERROR (TEXT("Time-out while waiting for app-link disconnection\n")) ;
      TerminateThread (g_data.hThread, 0) ;
      TRACE_INFO (TEXT("Thread terminated\n")) ;
    }
  else
    TRACE_INFO (TEXT("App-link thread terminated gracefully\n")) ;
  
  CloseHandle (g_data.hThread) ;
  CloseHandle (g_data.hStopEvent) ;

  return TRUE ;
}
