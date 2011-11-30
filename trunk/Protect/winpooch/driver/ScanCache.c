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

#define	TRACE_LEVEL	2 // warning level

#define CACHE_LENGTH	2048


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "ScanCache.h"

// ddk's header
#include <ddk/ntapi.h>

// project's headers
#include "DrvStatus.h"
#include "Malloc.h"
#include "Strlcpy.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define LOCK_TIMEOUT		30 /*secondes*/


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  ULONG		nIdentifier ;
  WCHAR		szFile[MAX_PATH] ;
  SCANRESULT	nResult ;
  LARGE_INTEGER liScanTime ;
} SCANCACHEENTRY ;

typedef struct {
  BOOL			bInitialized ;
  KMUTEX		mutex ;
  KEVENT		event ;
  SCANCACHEENTRY	*pEntries ;
  UINT			nEntries ;
  UINT			nEntriesMax ;
  UINT			iNextEntry ;
  ULONG			nNextIdentifier ;
} INTERNALDATA ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static INTERNALDATA	g_data ;



/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ScanCache_Init ()
{
  UINT	nSize ;

  TRACE ;

  ASSERT (!g_data.bInitialized) ;

  g_data.nEntriesMax = CACHE_LENGTH ;
  g_data.nEntries = 0 ;
  g_data.iNextEntry = 0 ;
  g_data.nNextIdentifier = 0 ;

  nSize = g_data.nEntriesMax*sizeof(SCANCACHEENTRY) ;

  g_data.pEntries = MALLOC(nSize) ;

  if( g_data.pEntries == NULL )
    {
      TRACE_ERROR (TEXT("Failed to allocate scan cache buffer (%n bytes)\n"), nSize) ;
      return STATUS_INSUFFICIENT_RESOURCES ;
    }

  KeInitializeMutex (&g_data.mutex, 0) ;
  KeInitializeEvent (&g_data.event, NotificationEvent, FALSE) ;

  g_data.bInitialized = TRUE ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ScanCache_Uninit ()
{
  TRACE ;

  ASSERT (g_data.bInitialized) ;

  ScanCache_Lock () ;

  KeSetEvent (&g_data.event, 0, FALSE) ;

  FREE (g_data.pEntries) ;
  g_data.bInitialized = FALSE ;
  KeReleaseMutex (&g_data.mutex, FALSE) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL ScanCache_IsLocked ()
{
  return 0==KeReadStateMutex(&g_data.mutex) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ScanCache_Lock ()
{
  NTSTATUS	nStatus ;
  LARGE_INTEGER	liTimeOut ;

  ASSERT (g_data.bInitialized) ;

  liTimeOut.QuadPart = - 5000 * 10000 ;

  nStatus = KeWaitForMutexObject (&g_data.mutex,
				  Executive,
				  KernelMode,
				  FALSE,
				  &liTimeOut) ;

  if( nStatus==STATUS_TIMEOUT )
    {
      TRACE_WARNING (TEXT("Waiting for ScanCache mutex for more than 5 seconds, will fail in %d secondes.\n"), LOCK_TIMEOUT) ;

      liTimeOut.QuadPart = - LOCK_TIMEOUT * 1000 * 10000 ;

      nStatus = KeWaitForMutexObject (&g_data.mutex,
				      Executive,
				      KernelMode,
				      FALSE,
				      &liTimeOut) ;
    }

  if( nStatus!=STATUS_SUCCESS )
    {
      DrvStatus_Trace() ;
      TRACE_BREAK (TEXT("KeWaitForMutexObject failed (status=0x%08X)\n"), nStatus) ;
    }

  return nStatus ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ScanCache_Unlock ()
{
  ASSERT (g_data.bInitialized) ;
  ASSERT (ScanCache_IsLocked()) ;

  KeReleaseMutex (&g_data.mutex, FALSE) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS _ScanCache_AddFile (LPCWSTR szFile, SCANCACHEID * pnId)
{
  SCANCACHEENTRY	*pEntry ;

  TRACE_INFO (TEXT("Adding file %ls (index = %d)\n"), szFile, g_data.iNextEntry) ;

  ASSERT (g_data.bInitialized) ;
  ASSERT (ScanCache_IsLocked()) ;

  pEntry = &g_data.pEntries[g_data.iNextEntry] ;

  if( g_data.nEntries < g_data.nEntriesMax ) 
    g_data.nEntries++ ;
    
  g_data.iNextEntry++ ;
  if( g_data.iNextEntry >= g_data.nEntriesMax )
    g_data.iNextEntry = 0 ;
    
  wcslcpy (pEntry->szFile, szFile, MAX_PATH) ;
  pEntry->nResult = SCAN_NOT_SCANNED ;
  pEntry->liScanTime.QuadPart = 0 ;
  pEntry->nIdentifier = g_data.nNextIdentifier++ ;

  *pnId = pEntry->nIdentifier ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ScanCache_GetFileId (LPCWSTR szFile, SCANCACHEID * pnId)
{
  NTSTATUS		nStatus = STATUS_SUCCESS ;
  UINT			i ;
  BOOL			bFound ;

  TRACE_INFO (TEXT("Looking for \"%ls\" into %u files\n"), szFile, g_data.nEntries) ;

  ASSERT (g_data.bInitialized) ;
  ASSERT (ScanCache_IsLocked()) ;
  ASSERT (szFile!=NULL) ;
  ASSERT (pnId!=NULL) ;

  bFound = FALSE ;

  for( i=0 ; !bFound && i<g_data.nEntries ; i++ )
    {
      SCANCACHEENTRY	*pEntry ;

      pEntry = &g_data.pEntries[i] ;

      if( ! wcsicmp(szFile,pEntry->szFile) )
	{
	  bFound = TRUE ;
	  *pnId = pEntry->nIdentifier ;
	}
    }

  if( ! bFound )
    nStatus = _ScanCache_AddFile (szFile, pnId) ;

  TRACE_INFO (TEXT("%u => %ls\n"), *pnId, szFile) ;

  return nStatus ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

SCANCACHEENTRY *  _ScanCache_GetEntryFromId (SCANCACHEID nIdentifier)
{
  INT			iIndex ;

  ASSERT (g_data.bInitialized) ;
  ASSERT (ScanCache_IsLocked()) ;

  if( g_data.nEntries > 0 )
    iIndex = nIdentifier - g_data.pEntries[0].nIdentifier ;
  else
    iIndex = 0 ;

  if( iIndex<0 || iIndex>=g_data.nEntries )
    iIndex = g_data.pEntries[g_data.nEntries-1].nIdentifier - nIdentifier ;
      
  if( iIndex<0 || iIndex>=g_data.nEntries )
    {
      TRACE_ERROR (TEXT("Identifier %u is not available in cache (index=%d, count=%u)\n"), 
		   nIdentifier, iIndex, g_data.nEntries) ;
      return NULL ;
    }

  return &g_data.pEntries[iIndex] ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ScanCache_GetStatus (SCANCACHEID nIdentifier, SCANRESULT* pnResult, LARGE_INTEGER * pliScanTime)
{
  SCANCACHEENTRY	*pEntry ;

  ASSERT (g_data.bInitialized) ;
  ASSERT (ScanCache_IsLocked()) ;

  pEntry = _ScanCache_GetEntryFromId (nIdentifier) ;

  if( ! pEntry ) return STATUS_NOT_FOUND ;

  if( pnResult ) *pnResult = pEntry->nResult ;
  if( pliScanTime ) *pliScanTime = pEntry->liScanTime ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ScanCache_SetStatus (SCANCACHEID nIdentifier, SCANRESULT nResult, LARGE_INTEGER * pliScanTime)
{
  SCANCACHEENTRY	*pEntry ;

  pEntry = _ScanCache_GetEntryFromId (nIdentifier) ;

  if( ! pEntry ) return STATUS_NOT_FOUND ;

  pEntry->nResult = nResult ;
  if( pliScanTime ) pEntry->liScanTime = *pliScanTime ;

  KeSetEvent (&g_data.event, 0, FALSE) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ScanCache_WaitChange (SCANCACHEID nId) 
{
  NTSTATUS nStatus ;
  
  ASSERT (g_data.bInitialized) ;
  ASSERT (ScanCache_IsLocked()) ;

  KeClearEvent (&g_data.event) ;

  ScanCache_Unlock () ;

  nStatus = KeWaitForSingleObject (&g_data.event, Executive, KernelMode, TRUE, NULL) ;

  if( ! g_data.bInitialized )
    return STATUS_CANCELLED ;
  
  ScanCache_Lock () ;
  
  return nStatus ;
}

/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ScanCache_GetCacheInfo (UINT* pnMaxLength, ULONG* pnFirstIdentifier, ULONG* pnLastIdentifier) 
{
  ASSERT (g_data.bInitialized) ;
  ASSERT (ScanCache_IsLocked()) ;

  if( pnMaxLength ) *pnMaxLength = g_data.nEntriesMax ;

  if( pnFirstIdentifier ) 
    {
      if( g_data.nEntries < g_data.nEntriesMax )
	*pnFirstIdentifier = 0 ;
      else
	*pnFirstIdentifier = g_data.pEntries[g_data.iNextEntry].nIdentifier ;

      TRACE_INFO (TEXT("First identifier = %lu\n"), *pnFirstIdentifier) ;
    }

  if( pnLastIdentifier ) 
    {
      if( g_data.nEntries == 0 )
	*pnLastIdentifier = 0 ;
      else
	*pnLastIdentifier = g_data.nNextIdentifier - 1 ; 

      TRACE_INFO (TEXT("Last identifier = %lu\n"), *pnLastIdentifier) ;
    }

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ScanCache_EnumChangesSince (SCANCACHEENUMCALLBACK pfnCallback, VOID* pUserPtr, LARGE_INTEGER * pliSinceTime) 
{
  INT i ;

  ASSERT (g_data.bInitialized) ;
  ASSERT (ScanCache_IsLocked()) ;

  TRACE_INFO (TEXT("Last sync = %lu\n"), pliSinceTime->QuadPart) ;

  for( i=0 ; i<g_data.nEntries ; i++ )
    if( g_data.pEntries[i].liScanTime.QuadPart >= pliSinceTime->QuadPart )
      if( ! pfnCallback (pUserPtr, g_data.pEntries[i].nIdentifier, g_data.pEntries[i].szFile, g_data.pEntries[i].nResult, &g_data.pEntries[i].liScanTime) )
	return STATUS_UNSUCCESSFUL ;
  
  return STATUS_SUCCESS ;
}
