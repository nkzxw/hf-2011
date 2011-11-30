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

#define	TRACE_LEVEL		2
#define ONLY_DEFAULT_FILTER	0


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "HookCommon.h"

// project's headers
#include "FileInfo.h"
#include "FiltCond.h"
#include "DrvInterface.h"
#include "DrvFilter.h"
#include "Link.h"
#include "Malloc.h"
#include "NtUndoc.h"
#include "ProcInfo.h"
#include "ProcList.h"
#include "ScanCache.h"
#include "strlcpy.h"
#include "Trace.h"
#include "Wildcards.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define DEFAULT_REACTION	RULE_ACCEPT
#define DEFAULT_VERBOSITY	RULE_SILENT

#define MAX_SERIAL_SIZE		1024


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  
  struct {
    KMUTEX	mutex ;
    UINT	nLength ;
    LPCWSTR	*pszArray ;
    BYTE	*pBuffer ;
    UINT	nBufferSize ;  
  } scanfilters ;

} INTERNAL_DATA ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static LPCWSTR g_szUnknownFile = L"!! Unknown file !!" ;

static INTERNAL_DATA g_data ;


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS HookCommon_Init () 
{
  KeInitializeMutex (&g_data.scanfilters.mutex, 0) ; 

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS HookCommon_Uninit () 
{
  FREE (g_data.scanfilters.pszArray) ;
  FREE (g_data.scanfilters.pBuffer) ;

  g_data.scanfilters.pszArray = NULL ;
  g_data.scanfilters.nLength = 0 ;
  g_data.scanfilters.pBuffer = NULL ;
  g_data.scanfilters.nBufferSize = 0 ;

  return STATUS_SUCCESS ; 
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS _HookCommon_Ask (PCFILTCOND pCond, DWORD * pnReaction)
{
  SDNASK	*pReq ;
  UINT		nSerialSize ;
  DWORD		nRequestSize, nResponseSize ;
  NTSTATUS	nStatus ;

  TRACE_INFO ("-- ASK --\n") ;

  pReq	= (SDNASK*) MALLOC (sizeof(SDNASK)+MAX_SERIAL_SIZE) ;

  if( pReq == NULL )
    {
      TRACE_ERROR (TEXT("Failed to allocate structure SDNASK (%u bytes)\n"), sizeof(SDNASK)+MAX_SERIAL_SIZE) ;
      return STATUS_INSUFFICIENT_RESOURCES ;
    }
    
  pReq->hdr.dwCode	= SDN_ASK ;
  pReq->nProcessAddress	= (PROCADDR)PsGetCurrentProcess() ;
  pReq->nDefReaction	= *pnReaction ;
      
  nSerialSize = FiltCond_Serialize (pCond, pReq->data, MAX_SERIAL_SIZE) ;
  
  if( ! nSerialSize ) {
    TRACE_ERROR (TEXT("FiltCond_Serialize failed\n")) ;
    FREE (pReq) ;
    return STATUS_UNSUCCESSFUL ;
  }
  
  nRequestSize = sizeof(SDNASK) + nSerialSize ;

  nStatus = Link_QueryServer (pReq, nRequestSize, 
			      pnReaction, &nResponseSize,
			      sizeof(DWORD)) ;
  
  if( nStatus==STATUS_PORT_DISCONNECTED )
    TRACE_INFO (TEXT("App-link disconnected\n")) ;  
  else if( nStatus!=STATUS_SUCCESS )
    TRACE_ERROR (TEXT("Failed to query server\n")) ;  
  else if( nResponseSize!=sizeof(DWORD) )
    TRACE_ERROR (TEXT("Invalid response size\n")) ;

  FREE (pReq) ;

  return nStatus ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS _HookCommon_Log (PCFILTCOND pCond, DWORD nReaction, BOOL bAlert)
{
  SDNLOG	*pReq ;  
  UINT		nSerialSize ;
  DWORD		nRequestSize ;
  NTSTATUS	nStatus ;

  TRACE_INFO ("-- LOG (addr=0x%08X, pid=%u) --\n", 
		ProcInfo_GetCurrentProcessAddress(),
		ProcInfo_GetCurrentProcessId()) ;

  pReq	= (SDNLOG*) MALLOC (sizeof(SDNLOG)+MAX_SERIAL_SIZE) ;

  if( pReq == NULL )
    {
      TRACE_ERROR (TEXT("Failed to allocate structure SDNLOG (%u bytes)\n"), sizeof(SDNLOG)+MAX_SERIAL_SIZE) ;
      return STATUS_INSUFFICIENT_RESOURCES ;
    }
    
  pReq->hdr.dwCode		= bAlert ? SDN_ALERT : SDN_LOG ;
  pReq->nProcessAddress		= (PROCADDR)PsGetCurrentProcess() ;
  pReq->dwReaction		= nReaction ;
      
  nSerialSize = FiltCond_Serialize (pCond, pReq->data, MAX_SERIAL_SIZE) ;
  
  if( ! nSerialSize ) {
    TRACE_ERROR (TEXT("FiltCond_Serialize failed\n")) ;
    FREE (pReq) ;
    return STATUS_UNSUCCESSFUL ;
  }
  
  nRequestSize = sizeof(SDNLOG) + nSerialSize ;
      
  nStatus = Link_QueryServer (pReq, nRequestSize, 
			      NULL, NULL, 0) ;
  
  if( nStatus==STATUS_PORT_DISCONNECTED )
    {
      TRACE_INFO (TEXT("App-link disconnected\n")) ;  
      return FALSE ;
    }

  FREE (pReq) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS HookCommon_ShouldScanFile (LPCWSTR szFilePath)
{
  UINT	i ;
  BOOL	bShouldScan = FALSE ;

  if( ProcList_GetScannerExePath()==NULL ||
      ProcList_GetScannerExePath()[0]==0 ||
      !wcsicmp(ProcList_GetScannerExePath(),szFilePath) )
    return FALSE ;

  KeWaitForMutexObject (&g_data.scanfilters.mutex, Executive,
			KernelMode, FALSE, NULL) ;

  for( i=0 ; i<g_data.scanfilters.nLength ; i++ )
    {
      LPCWSTR szFilter = g_data.scanfilters.pszArray[i] ;

      if( Wildcards_CompleteStringCmp(szFilter,szFilePath) )
	{
	  bShouldScan = TRUE ;
	  break ;
	}
    }

  KeReleaseMutex (&g_data.scanfilters.mutex, FALSE) ;

  return bShouldScan ;
}

/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS HookCommon_ScanFile (UINT * pnReaction,
			      LPCWSTR szFilePath, 
			      LARGE_INTEGER * pliFileTime)
{
  SDNSCANFILE	req ;
  DWORD		nResponseSize ;
  NTSTATUS	nStatus ;
  SCANCACHEID	nCacheId ;
  SCANRESULT	nScanResult ;
  LARGE_INTEGER	liScanTime ;

  ASSERT (szFilePath!=NULL) ;
  ASSERT (pliFileTime!=NULL) ;

  
  nStatus = ScanCache_Lock () ;

  if( nStatus != STATUS_SUCCESS ) {
    TRACE_ERROR (TEXT("ScanCache_Lock() failed (status = 0x%08X)\n"), nStatus) ;
    return nStatus ;
  }  

  //
  // 1. Get file identifier in scan cache
  //
  nStatus = ScanCache_GetFileId (szFilePath, &nCacheId) ;

  if( nStatus != STATUS_SUCCESS ) {
    TRACE_ERROR (TEXT("ScanCache_GetFileId() failed (status = 0x%08X)\n"), nStatus) ;
    ScanCache_Unlock () ;
    return nStatus ;
  }

  //
  // 2. Get scan cache status
  //
 getstatus:
  nStatus = ScanCache_GetStatus (nCacheId, &nScanResult, &liScanTime) ;

  if( nStatus != STATUS_SUCCESS ) {
    TRACE_ERROR (TEXT("ScanCache_GetStatus() failed (status = 0x%08X)\n"), nStatus) ;
    ScanCache_Unlock () ;
    return nStatus ;
  }
  
 checkstatus:
  switch( nScanResult )
    {
    case SCAN_NO_VIRUS:
    case SCAN_VIRUS_ACCEPTED:
    case SCAN_FAILED_ACCEPTED:
      *pnReaction = RULE_ACCEPT ;
      ScanCache_Unlock () ;
      return STATUS_SUCCESS ;

    case SCAN_VIRUS:
    case SCAN_FAILED:
      *pnReaction = RULE_REJECT ;
      ScanCache_Unlock () ;
      return STATUS_SUCCESS ;

    case SCAN_BEING_SCANNED:

      TRACE_WARNING (TEXT("%ls is already being scanned\n"), szFilePath) ;

      nStatus = ScanCache_WaitChange (nCacheId) ;
      
      if( nStatus != STATUS_SUCCESS ) {
	ScanCache_Unlock () ;
	TRACE_ERROR (TEXT("ScanCache_WaitChange() failed (status = 0x%08X)\n"), nStatus) ;
	return nStatus ;
      }  
      
      goto getstatus ;

    case SCAN_NOT_SCANNED:
     
      break ;
    }	

  if( ! Link_IsConnected() ) 
    {
      *pnReaction = DEFAULT_REACTION ;
      ScanCache_Unlock () ;
      return STATUS_SUCCESS ;
    }

  KeQuerySystemTime (&liScanTime) ;
  ExSystemTimeToLocalTime (&liScanTime, &liScanTime) ;
  
  ScanCache_SetStatus (nCacheId, SCAN_BEING_SCANNED, &liScanTime) ; 

  ScanCache_Unlock () ;   
  
  TRACE_INFO (TEXT("Scan file %ls\n"), szFilePath) ;

  if( pliFileTime->QuadPart == 0 )
    TRACE_WARNING (TEXT("File time is null.\n")) ;
    
  req.hdr.dwCode		= SDN_SCANFILE ;
  req.nProcessAddress		= (DWORD)PsGetCurrentProcess() ;
  wcslcpy (req.wszFilePath, szFilePath, MAX_PATH) ;
  req.liFileTime		= *pliFileTime ;
  
  nStatus = Link_QueryServer (&req, sizeof(SDNSCANFILE), 
			      &nScanResult, &nResponseSize,
			      sizeof(SCANRESULT)) ;

  if( nStatus!=STATUS_SUCCESS )
    TRACE_ERROR (TEXT("Link_QueryServer failed (status=0x%08X)\n"), nStatus) ;
  else
    TRACE_INFO (TEXT("Scan result %u\n"), nScanResult) ;

  KeQuerySystemTime (&liScanTime) ;
  ExSystemTimeToLocalTime (&liScanTime, &liScanTime) ;

  ScanCache_Lock () ;
  ScanCache_SetStatus (nCacheId, nScanResult, &liScanTime) ;

  goto checkstatus ;
}

/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS HookCommon_CatchCall (UINT* pnReaction, UINT* pnOptions,
			       UINT nReason, LPCTSTR szFormat, ...) 
{
  DWORD		nReaction, nVerbosity, nOptions ;
  BOOL		bResult ;
  va_list	va ;
  FILTCOND	cond ;

  PROCADDR	nProcessAddress = ProcInfo_GetCurrentProcessAddress() ;
  PROCID	nProcessId	= ProcInfo_GetCurrentProcessId() ;
  ULONG		nProcFlags	= 0 ;
    
  BOOL	bUnknownProcess = FALSE ;
  BOOL	bPidChanged = FALSE ;

  TRACE_INFO (TEXT("Process address = 0x%08X\n"), nProcessAddress) ;
  TRACE_INFO (TEXT("Process id = %d\n"), nProcessId) ;

  // set function result to default values
  // in case it returns prematurely
  *pnReaction = RULE_ACCEPT ;
  if( pnOptions!=NULL ) *pnOptions = 0 ;
  
  //
  // Build condition structure
  //
  va_start (va, szFormat) ;
  bResult = FiltCond_SetFV (&cond, nReason, szFormat, va) ;
  va_end (va) ;

  if( ! bResult ) 
    {
      TRACE_ERROR (TEXT("Failed to create condition\n")) ;
      return STATUS_UNSUCCESSFUL ;
    }

  TRACE_INFO (TEXT("Reason = %d\n"), cond.nReason) ;
  if( cond.nParams>0 && cond.aParams[0].nType==FILTPARAM_STRING )
    TRACE_INFO (TEXT("Param = %ls\n"), cond.aParams[0].szValue) ;
        
  bResult = FALSE ;

  //
  // Apply filtering 
  // ---------------
  // Results are stored in nReaction, nVerbosity and nOptions
  //
  {
    HFILTER	aFilters[MAX_FILTERS] ;
    ULONG	nFilters = 0 ;
    NTSTATUS	nStatus ;
    
    // Lock filters 
    nStatus = DrvFilter_LockMutex () ;

    if( nStatus != STATUS_SUCCESS ) 
      {
	FiltCond_Clear (&cond) ;
	return nStatus ;
      }

    //       
    // Get filters associated to this process
    // 
    {
      PROCSTRUCT	*pProc ;

      nStatus = ProcList_Lock () ;

      if( nStatus != STATUS_SUCCESS )
	{
	  FiltCond_Clear (&cond) ;
	  DrvFilter_UnlockMutex () ;
	  return nStatus ;
	}

      pProc = ProcList_Get (nProcessAddress) ;
      
      if( pProc != NULL )
	{
	  TRACE_INFO (TEXT("Process 0x%08X found in list.\n"), nProcessAddress) ;

	  bPidChanged =  pProc->nProcessId != nProcessId ;
	  pProc->nProcessId = nProcessId ;

	  nProcFlags = pProc->nFlags ;

	  TRACE_INFO (TEXT("Process 0x%08X flags = 0x%08X\n"), nProcessAddress, nProcFlags) ;

	  nFilters = pProc->nFilters ;

	  ASSERT (nFilters<=MAX_FILTERS) ;

	  TRACE_INFO (TEXT("Process 0x%08X filters = 0x%08X\n"), nProcessAddress, nFilters) ;

	  if( nFilters>0 )
	    memcpy (aFilters, pProc->aFilters, nFilters*sizeof(HFILTER)) ; 
	}
      else
	{
	  TRACE_INFO (TEXT("Process 0x%08X not found in list.\n"), nProcessAddress) ;

	  bUnknownProcess = TRUE ;
	}

      ProcList_Unlock () ;
    }

    //
    // If the process was not in list, we have to add it.
    //
    if( bUnknownProcess )
      {
	PROCSTRUCT	*pProc ;

	TRACE_WARNING(TEXT("A process 0x%08X (pid=%u) has been created but I didn't saw it.\n"),
		      nProcessAddress, nProcessId) ;

	// alloc a new process descriptor
	pProc = ProcList_New (nProcessAddress, nProcessId, g_szUnknownFile) ;

	nProcFlags = pProc->nFlags ;
	
	// get associated filters
	DrvFilter_GetFiltersForProgram (pProc->wszPath, pProc->aFilters, 
					&pProc->nFilters, MAX_FILTERS) ;

	// copy associated filters
	nFilters = pProc->nFilters ;

	ASSERT (nFilters<=MAX_FILTERS) ;

	if( nFilters>0 )
	  memcpy (aFilters, pProc->aFilters, nFilters*sizeof(HFILTER)) ; 
	
	// add process descriptor to process list
	nStatus = ProcList_Lock () ;

	if( nStatus == STATUS_SUCCESS )
	  {
	    ProcList_Add (pProc) ;
	    ProcList_Unlock () ;
	  }
      }

    //
    // If the process is flagged "ignore all", we ignore the event
    //
    if( nProcFlags & PROCESS_IGNORE_ALL )
      {
	TRACE_INFO (TEXT("Process 0x%08X (pid=%u) is flagged \"ignore all\"\n"), 
		    nProcessAddress, nProcessId) ;
	DrvFilter_UnlockMutex () ;
	FiltCond_Clear (&cond) ;
	return STATUS_SUCCESS ;
      }

    //
    // Test each filter one by one
    //
    {
      int	i ;     
      
      ASSERT (nFilters<=MAX_FILTERS) ;

      //TRACE_INFO (TEXT("Process 0x%08X (pid=%u) has %u filters\n"), 
      //nProcessAddress, nProcessId, nFilters) ;  

      for( i=0 ; i<nFilters && !bResult ; i++ )
	{
	  TRACE_INFO (TEXT("Testing filter %d/%d\n"), i, nFilters) ;

	  if( aFilters[i]==NULL )
	    TRACE_WARNING (TEXT("Filter %d of process %u is NULL\n"), i, PsGetCurrentProcessId()) ;
	  else
	    bResult = Filter_Test (aFilters[i],&cond,&nReaction,&nVerbosity,&nOptions) ;	
	}
     
      if( nFilters==0 )
	{
	  HFILTERSET	hFilterSet ;
	  HFILTER	hFilter ; 
	  
	  hFilterSet = DrvFilter_GetFilterSet() ;
	  
	  if( hFilterSet ) 
	    {	      
	      TRACE_WARNING (TEXT("Process 0x%08X (pid=%u) has an empty filter list\n"), 
			     nProcessAddress, nProcessId) ;  
	      
	      hFilter = FilterSet_GetDefaultFilter (hFilterSet) ;
	      bResult = hFilter ? Filter_Test (hFilter,&cond,&nReaction,&nVerbosity,&nOptions) : FALSE ; 
	    } 
	}

      // failed to apply filters, use defaults
      if( ! bResult )  
	{
	  nReaction  = DEFAULT_REACTION;
	  nVerbosity = DEFAULT_VERBOSITY ;
	  nOptions = 0 ;
	}
    }
  }

  DrvFilter_UnlockMutex () ;
  
  if( Link_IsConnected() && (nProcFlags&PROCESS_NO_NOTIFICATION)==0 )
    { 
      //
      // If PID of this process has changed, we notify the application
      // (we send notification only when DrvFilter mutex is released).
      //
      if( bPidChanged )
	{
	  TRACE_INFO (TEXT("Process 0x%08X changed its PID to %u\n"), 
		      nProcessAddress, nProcessId) ;
	  HookCommon_SendPidChangedNotification (nProcessAddress,
						 nProcessId) ;
	}
      
      //
      // If the process the process wasn't known, we notify the application
      // (we send notification only when DrvFilter mutex is released).
      //
      if( bUnknownProcess )
	{
	  HookCommon_SendProcessCreatedNotification (nProcessAddress, nProcessId, g_szUnknownFile) ;
	}
      
      if( nOptions & RULE_ASK )
      	_HookCommon_Ask (&cond, &nReaction) ;
      
      if( nVerbosity==RULE_LOG )
	_HookCommon_Log (&cond, nReaction, FALSE) ;    
      
      if( nVerbosity==RULE_ALERT )
	_HookCommon_Log (&cond, nReaction, TRUE) ;    
    }

  *pnReaction = nReaction ;
  if( pnOptions!=NULL ) *pnOptions = nOptions ;

  TRACE_INFO(TEXT("Finished\n")) ;

  if( nReaction == RULE_KILLPROCESS )
    {
      TRACE_WARNING (TEXT("Killing process\n")) ;
      ntundoc.NtTerminateProcess (INVALID_HANDLE_VALUE, 0) ;
    }

  FiltCond_Clear (&cond) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS HookCommon_SendProcessCreatedNotification (PROCADDR	nProcessAddress,
						    ULONG	nProcessId, 
						    LPCWSTR	wszFilePath)
{
  SDNPROCESSCREATED	*pReq ;
  DWORD			nRequestSize ;
  NTSTATUS		nStatus ;

  if( ! Link_IsConnected() ) return STATUS_SUCCESS ;

  nRequestSize = sizeof(SDNPROCESSCREATED) + wcslen(wszFilePath)*2 + 2 ;

  pReq	= (SDNPROCESSCREATED*) MALLOC (nRequestSize) ;
    
  if( pReq == NULL )
    {
      TRACE_ERROR (TEXT("Failed to allocate strcuture SDNPROCESSCREATED (%u bytes)\n"), nRequestSize) ;
      return STATUS_INSUFFICIENT_RESOURCES ;
    }

  pReq->hdr.dwCode		= SDN_PROCESSCREATED ;      
  pReq->nProcessAddress		= nProcessAddress ;
  pReq->nProcessId		= nProcessId ;

  memcpy (pReq->wszFilePath, wszFilePath, wcslen(wszFilePath)*2+2) ;
        
  nStatus = Link_QueryServer (pReq, nRequestSize, NULL, NULL, 0) ;
  
  if( nStatus!=STATUS_SUCCESS )
    TRACE_ERROR (TEXT("Link_QueryServer failed (status=0x%08X)\n"), nStatus) ;  

  FREE (pReq) ;

  return nStatus ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS HookCommon_SendPidChangedNotification (PROCADDR nProcessAddress, ULONG nPid)
{
  SDNPIDCHANGED	req ;
  NTSTATUS	nStatus ;

  if( ! Link_IsConnected() ) return STATUS_SUCCESS ;
    
  req.hdr.dwCode	= SDN_PIDCHANGED;
        
  req.nProcessAddress	= nProcessAddress ;
  req.nNewProcessId	= nPid ;
        
  nStatus = Link_QueryServer (&req, sizeof(req), NULL, NULL, 0) ;
  
  if( nStatus!=STATUS_SUCCESS )
    TRACE_ERROR (TEXT("Link_QueryServer failed (status=0x%08X)\n"), nStatus) ;  

  return nStatus ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS HookCommon_SendProcessTerminatedNotification (PROCADDR nProcessAddress)
{
  SDNPROCESSTERMINATED	req ;
  NTSTATUS		nStatus ;

  if( ! Link_IsConnected() ) return STATUS_SUCCESS ;
    
  req.hdr.dwCode		= SDN_PROCESSTERMINATED ;
      
  req.nProcessAddress		= nProcessAddress ;
        
  nStatus = Link_QueryServer (&req, sizeof(req), NULL, NULL, 0) ;
  
  if( nStatus!=STATUS_SUCCESS )
    TRACE_ERROR (TEXT("Link_QueryServer failed (status=0x%08X)\n"), nStatus) ;  

  return nStatus ;
}


NTSTATUS HookCommon_SetScanFilters (LPVOID pInBuf, UINT nInBufSize) 
{
  KeWaitForMutexObject (&g_data.scanfilters.mutex, Executive,
			KernelMode, FALSE, NULL) ;

  FREE (g_data.scanfilters.pszArray) ;
  FREE (g_data.scanfilters.pBuffer) ;

  g_data.scanfilters.nBufferSize = 0 ;
  g_data.scanfilters.pBuffer = NULL ;
  g_data.scanfilters.nLength = 0 ;
  g_data.scanfilters.pszArray = NULL ;
  
  if( nInBufSize > 0 )
    {
      LPWSTR szBuffer ;
      int j, n ;

      g_data.scanfilters.nBufferSize = nInBufSize ;
      g_data.scanfilters.pBuffer = MALLOC (nInBufSize) ;

      if( g_data.scanfilters.pBuffer == NULL )
	{
	  g_data.scanfilters.nBufferSize = 0 ;
	  KeReleaseMutex (&g_data.scanfilters.mutex, FALSE) ;
	  TRACE_ERROR (TEXT("Failed to allocate buffer (%u bytes)\n"), nInBufSize) ;
	  return STATUS_INSUFFICIENT_RESOURCES ;
	}
	  
      TRACE_INFO (TEXT("Reading array...\n")) ;

      memcpy (g_data.scanfilters.pBuffer, pInBuf, nInBufSize) ;
      
      TRACE_INFO (TEXT("Counting string in array...\n")) ;
      
      szBuffer = (LPWSTR)g_data.scanfilters.pBuffer ;
      n = 0 ;
      
      for( j=1 ; j<nInBufSize/sizeof(WCHAR) ; j++ )
	if( szBuffer[j]==0 && szBuffer[j-1]!=0 )  n++ ;		
      
      
      TRACE_INFO (TEXT("Found %d strings.\n"), n) ;
      
      g_data.scanfilters.nLength = n ;
      g_data.scanfilters.pszArray = MALLOC (n*sizeof(LPTSTR)) ;

      if( g_data.scanfilters.pszArray == NULL )
	{
	  FREE (g_data.scanfilters.pBuffer) ;
	  g_data.scanfilters.pBuffer = NULL ;
	  g_data.scanfilters.nBufferSize = 0 ;
	  g_data.scanfilters.nLength = 0 ;
	  KeReleaseMutex (&g_data.scanfilters.mutex, FALSE) ;
	  TRACE_ERROR (TEXT("Failed to allocate array (%u bytes)\n"), n*sizeof(LPTSTR)) ;
	  return STATUS_INSUFFICIENT_RESOURCES ;
	}
      
      n = 0 ;
      g_data.scanfilters.pszArray[n] = szBuffer ;
      TRACE_INFO (TEXT("String %d is \"%ls\"\n"), n, g_data.scanfilters.pszArray[n]) ;
      n++ ;
      
      for( j=1 ; j<nInBufSize/sizeof(WCHAR) ; j++ )
	{
	  if( szBuffer[j]!=0 && szBuffer[j-1]==0 )
	    {
	      g_data.scanfilters.pszArray[n] = &szBuffer[j] ;
	      TRACE_INFO (TEXT("String %d is \"%ls\"\n"), n, g_data.scanfilters.pszArray[n]) ;
	      n++ ;
	    }
	}
      
      TRACE_INFO (TEXT("Finished getting %d strings\n"), n) ;
    }

  KeReleaseMutex (&g_data.scanfilters.mutex, FALSE) ;

  return STATUS_SUCCESS ;
}
