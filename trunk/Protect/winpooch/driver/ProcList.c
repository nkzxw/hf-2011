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

#define ONLY_DEFAULT_FILTER	0
#define	TRACE_LEVEL		2 // warning level


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "ProcList.h"

// ddk's header
#include <ddk/ntifs.h>

// project's headers
#include "DrvFilter.h"
#include "DrvStatus.h"
#include "FileInfo.h"
#include "FilterSet.h"
#include "Malloc.h"
#include "NtUndoc.h"
#include "ProcInfo.h"
#include "Strlcpy.h"
#include "SystInfo.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define LOCK_TIMEOUT		30 /*secondes*/


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct NODE {
  PROCSTRUCT	*pData ; 
  struct NODE	*pPrev ;
  struct NODE	*pNext ;
} NODE ;

typedef struct {
  BOOL		bInitialized ;
  KMUTEX	mutex ;	
  WCHAR		szScannerExePath[MAX_PATH] ;
  struct NODE	*pFirst ;
  struct NODE	*pLast ;
} INTERNALDATA ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static INTERNALDATA	g_data ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

NTSTATUS _ProcList_GetProcessPath (LPWSTR wszPath, HANDLE hProcess) ;

PROCSTRUCT* _ProcList_DeleteNode (NODE * pNode) ;

PROCSTRUCT* _ProcList_NewPid (PROCID nPid) ;


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID ProcList_Init () 
{
  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  ASSERT (!g_data.bInitialized) ;

  KeInitializeMutex (&g_data.mutex, 0) ;

  g_data.pFirst = NULL ;
  g_data.pLast = NULL ;

  g_data.szScannerExePath[0] = 0 ;

  g_data.bInitialized = TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID ProcList_Uninit () 
{
  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  ASSERT (g_data.bInitialized) ;

  ProcList_Lock () ;
  ProcList_Clear () ;
  ProcList_Unlock () ;

  g_data.szScannerExePath[0] = 0 ;
  
  g_data.bInitialized = FALSE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ProcList_SetScannerExePath (LPCWSTR szScannerExe)
{
  ASSERT (ProcList_IsLocked()) ;
  
  if( szScannerExe!=NULL ) {
    wcslcpy (g_data.szScannerExePath, szScannerExe, MAX_PATH) ;
    TRACE_INFO (TEXT("Anti-virus has been changed to %ls\n"), szScannerExe) ;
  }
  else {
    g_data.szScannerExePath[0] = 0 ;  
    TRACE_INFO (TEXT("No anti-virus specified.\n")) ;
  }

  return STATUS_SUCCESS ;
}

/******************************************************************/
/* Exported function                                              */
/******************************************************************/

LPCWSTR ProcList_GetScannerExePath ()
{
  return g_data.szScannerExePath ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL ProcList_IsLocked () 
{
  return 0==KeReadStateMutex(&g_data.mutex) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ProcList_Lock () 
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
      TRACE_WARNING (TEXT("Waiting for ProcList mutex for more than 5 seconds, will fail in %d secondes.\n"), LOCK_TIMEOUT) ;

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

NTSTATUS ProcList_Unlock () 
{
  ASSERT (g_data.bInitialized) ;
  ASSERT (ProcList_IsLocked()) ;

  KeReleaseMutex (&g_data.mutex, FALSE) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID ProcList_Clear () 
{
  NODE * pCur ;
  NODE * pNext ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  ASSERT (ProcList_IsLocked()) ;
  ASSERT (g_data.bInitialized) ;

  for( pCur=g_data.pFirst ; pCur!=NULL ; pCur=pNext )
    {
      pNext = pCur->pNext ;
      FREE (pCur->pData) ;
      FREE (pCur) ;
    }

  g_data.pFirst = NULL ;
  g_data.pLast = NULL ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ProcList_Populate () 
{
  NTSTATUS	nStatus ;
  const SYSTEM_PROCESS_INFORMATION * pCurrent ;
  VOID		*pBuffer ;
  ULONG		nBufferSize ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  ASSERT (ProcList_IsLocked()) ;
  ASSERT (g_data.bInitialized) ;

  nBufferSize = 1000000 ;

  do {

    pBuffer = MALLOC (nBufferSize) ;

    if( pBuffer == NULL )
      {
	TRACE_ERROR (TEXT("Failed to allocate buffer for process list (%u bytes)\n"), nBufferSize) ;
	return STATUS_INSUFFICIENT_RESOURCES ;
      }

    nStatus = ZwQuerySystemInformation (SystemProcessInformation, 
					pBuffer, nBufferSize, NULL) ;
    
    if( nStatus!=STATUS_SUCCESS )
      {
	if( nStatus==STATUS_INFO_LENGTH_MISMATCH )
	  {
	    FREE (pBuffer) ;
	    nBufferSize *= 2 ;
	    continue ;
	  }
	else
	  {
	    TRACE_ERROR (TEXT("NtQuerySystemInformation failed (status=0x%08X)\n"), nStatus) ;
	    FREE (pBuffer) ;
	    return nStatus ;	    
	  }
      }     
    
  } while( nStatus!=STATUS_SUCCESS ) ;
  
  nStatus = ZwQuerySystemInformation (SystemProcessInformation,
				      pBuffer, nBufferSize,
				      &nBufferSize) ;

  if( nStatus != STATUS_SUCCESS ) {
    TRACE_ERROR (TEXT("NtQuerySystemInformation failed (status=0x%08X)\n"), nStatus) ;
    FREE (pBuffer) ;
    return nStatus ;
  }

  pCurrent = pBuffer ;
  while( pCurrent ) 
    {
      PROCSTRUCT * pProc ;

      TRACE_INFO (TEXT("Process %d (delta=%d)\n"), pCurrent->UniqueProcessId, pCurrent->NextEntryOffset) ;

      pProc = _ProcList_NewPid ((PROCID)pCurrent->UniqueProcessId) ;
      ProcList_Add (pProc) ;

      if( pCurrent->NextEntryOffset!=0 )
	pCurrent = (SYSTEM_PROCESS_INFORMATION*)((BYTE*)pCurrent + pCurrent->NextEntryOffset) ;
      else
	pCurrent = NULL ;
    }

  TRACE_INFO (TEXT("Buffer = 0x%08X\n"), pBuffer) ;
  //DbgBreakPoint () ;

  FREE (pBuffer) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

PROCSTRUCT* ProcList_New (IN PROCADDR	nProcessAddress, 
			  IN PROCID	nProcessId, 
			  IN LPCWSTR	wszFilePath)
{
  PROCSTRUCT	*pProc ;
  
  // assert paged memory is accessible
  PAGED_CODE() ;

  pProc = MALLOC (sizeof(PROCSTRUCT)) ;

  if( pProc == NULL )
    {
      TRACE_ERROR (TEXT("Failed to allocate strcuture PROCSTRUCT (%u bytes)\n"), sizeof(PROCSTRUCT)) ;
      return NULL ;
    }

  memset (pProc, 0, sizeof(PROCSTRUCT)) ;
  
  pProc->nProcessAddress	= nProcessAddress ;
  pProc->nProcessId		= nProcessId ;
  wcslcpy (pProc->wszPath, wszFilePath, MAX_PATH) ;
  
  // if the new file is the scanner, give it special flags
  if( ! wcsicmp(g_data.szScannerExePath,wszFilePath) )
    {
      TRACE_INFO (TEXT("Anti-virus scanner has been launched (addr=0x%08X, pid=%u)\n"),
		  nProcessAddress, nProcessId) ;
      pProc->nFlags |= PROCESS_IGNORE_ALL|PROCESS_NO_NOTIFICATION ;
    }
 
  return pProc ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID ProcList_Delete (PROCSTRUCT * pProc)
{
  FREE (pProc) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/
/*
NTSTATUS ProcList_AddNameHandle (LPCWSTR wszPath, HANDLE hProcess) 
{
  NTSTATUS	nStatus ;
  PROCSTRUCT	*pData ;
  
  // assert paged memory is accessible
  PAGED_CODE() ;

  ASSERT (ProcList_IsLocked()) ;

  //
  // Add node
  //
  
  pData = MALLOC (sizeof(PROCSTRUCT)) ;

  memset (pProc, 0, sizeof(PROCSTRUCT)) ;

  // save program path
  wcslcpy (pData->wszPath, wszPath, MAX_PATH) ;

  // save process id
  ProcInfo_GetProcessId (hProcess, &pData->nProcessId) ;

  // save EPROCESS address
 ProcInfo_GetAddress (hProcess  pData->nProcessAddress =) ;
   
  nStatus = _ProcList_AddNewNode (pData) ;    

  return nStatus ;
}
*/

/******************************************************************/
/* Internal function                                              */
/******************************************************************/

PROCSTRUCT* _ProcList_NewPid (PROCID nPid) 
{
  HANDLE	hProcess ;
  OBJECT_ATTRIBUTES oa ;
  CLIENT_ID	clid ;
  NTSTATUS	nStatus ;
  PROCSTRUCT	*pProc ;
  
  // assert paged memory is accessible
  PAGED_CODE() ;

  //
  // Open process handle
  //
  if( nPid!=0 )
    {
      InitializeObjectAttributes (&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL) ;
      
      clid.UniqueProcess = (HANDLE)nPid ;
      clid.UniqueThread = 0 ;
  
      nStatus = ntundoc.ZwOpenProcess (&hProcess,
				       PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,
				       &oa, &clid) ;
      
      if( nStatus != STATUS_SUCCESS )
	{
	  TRACE_ERROR (TEXT("ZwOpenProcess failed (status=0x%08X)\n"), nStatus) ;
	  hProcess = NULL ;
	}
    }
  else
    {
       hProcess = NULL ;
    }

  //
  // Add node
  //
  
  pProc = MALLOC (sizeof(PROCSTRUCT)) ;

  if( pProc == NULL )
    {
      TRACE_ERROR (TEXT("Failed to allocate strcuture PROCSTRUCT (%u bytes)\n"), sizeof(PROCSTRUCT)) ;
      if( hProcess ) ZwClose (hProcess) ;
      return NULL ;
    }

  memset (pProc, 0, sizeof(PROCSTRUCT)) ;

  // save EPROCESS address
  if( hProcess!=NULL )
    ProcInfo_GetAddress (hProcess, &pProc->nProcessAddress) ;
  
  // save process id
  pProc->nProcessId = nPid ;
  
  // save program path
  if( hProcess )
    nStatus = _ProcList_GetProcessPath (pProc->wszPath, hProcess) ;
  if( hProcess==NULL || nStatus!=STATUS_SUCCESS )
    ntundoc.swprintf (pProc->wszPath, L"Process %d", nPid) ;

  // if the new file is the scanner, give it special flags
  if( ! wcsicmp(g_data.szScannerExePath,pProc->wszPath) )
    pProc->nFlags |= PROCESS_IGNORE_ALL|PROCESS_NO_NOTIFICATION ;
   
  //
  // Close process handle
  //
  
  if( hProcess ) ZwClose (hProcess) ;

  return pProc ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/
/*
NTSTATUS ProcList_AddHandle (HANDLE hProcess)
{
  PROCSTRUCT	*pData ;

  TRACE ;

  ASSERT (ProcList_IsLocked()) ;
  
  pData = MALLOC (sizeof(PROCSTRUCT)) ;

  // save EPROCESS address
  pData->nProcessAddress = _ProcList_GetObjectAddress (hProcess) ;

  // save process id
  ProcInfo_GetUniqueProcessId (hProcess, &pData->nProcessId) ;
  TRACE_INFO (TEXT("PID = %lu\n"), pData->nProcessId) ;

  // save program path
  pData->wszPath[0] = 0 ;
  _ProcList_GetProcessPath (pData->wszPath, hProcess) ;
  
  return _ProcList_AddNewNode (pData) ;
}
*/

/******************************************************************/
/* Exported function                                              */
/******************************************************************/

PROCSTRUCT* ProcList_Remove (PROCADDR nProcessAddress) 
{
  NODE		*pCurNode ;
  NODE		*pNextNode ; 

  // assert paged memory is accessible
  PAGED_CODE() ;

  ASSERT (ProcList_IsLocked()) ;

  for( pCurNode=g_data.pFirst ; pCurNode!=NULL ; pCurNode=pNextNode )
     {
       pNextNode = pCurNode->pNext ;
       
       if( pCurNode->pData->nProcessAddress == nProcessAddress )
	 {	   
	   return _ProcList_DeleteNode (pCurNode) ;
	 }
     }

  TRACE_WARNING (TEXT("Process 0x%08X not found\n"), nProcessAddress) ;
  
  return NULL ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS ProcList_Add (PROCSTRUCT * pData)
{
  NODE * pNewNode = MALLOC(sizeof(NODE)) ;

  ASSERT (ProcList_IsLocked()) ;

  if( pNewNode == NULL )
    {
      TRACE_ERROR (TEXT("Failed to allocate structure NODE (%u bytes)\n"), sizeof(NODE)) ;
      return STATUS_INSUFFICIENT_RESOURCES ;
    }
  
  pNewNode->pData = pData ;
  
  pNewNode->pPrev = g_data.pLast ;
  pNewNode->pNext = NULL ;

  if( g_data.pFirst==NULL )
    g_data.pFirst = pNewNode ;

  if( g_data.pLast )
    g_data.pLast->pNext = pNewNode ;

  g_data.pLast = pNewNode ;
  
  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

PROCSTRUCT* _ProcList_DeleteNode (NODE * pNode)
{
  PROCSTRUCT * pProc ;
  NODE *pNextNode, *pPrevNode ; 

  ASSERT (pNode!=NULL) ;

  pPrevNode = pNode->pPrev ;
  pNextNode = pNode->pNext ; 

  pProc = pNode->pData ;

  FREE (pNode) ;
  
  if( pPrevNode!=NULL ) pPrevNode->pNext = pNextNode ;
  else g_data.pFirst = pNextNode ;

  if( pNextNode!=NULL ) pNextNode->pPrev = pPrevNode ;
  else g_data.pLast = pPrevNode ;

  return pProc ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS _ProcList_GetProcessPath (LPWSTR wszPath, HANDLE hProcess)
{
  NTSTATUS		nStatus ;
  UNICODE_STRING	usDosPath ;
  UNICODE_STRING	usNtPath ;
  WCHAR			wszBuffer[MAX_PATH] ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (wszPath!=NULL) ;
  
  usDosPath.Length = 0 ;
  usDosPath.MaximumLength = MAX_PATH*sizeof(WCHAR) ;
  usDosPath.Buffer = wszPath ;

  usNtPath.Length = 0 ;
  usNtPath.MaximumLength = MAX_PATH*sizeof(WCHAR) ;
  usNtPath.Buffer = wszBuffer ;

  nStatus = ProcInfo_GetImagePath (hProcess, &usNtPath) ;
  
  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_INFO (TEXT("ProcInfo_GetImagePath failed (status=0x%08X)\n"), nStatus) ;
      return nStatus ;
    }

  TRACE_INFO (TEXT("Process 0x%08X image (nt path) = %ls\n"), hProcess, usNtPath.Buffer) ;

  FileInfo_NtPathToDosPath (hProcess, NULL, &usNtPath, &usDosPath) ;

  TRACE_INFO (TEXT("Process 0x%08X image (dos path) = %ls\n"), hProcess, usDosPath.Buffer) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ProcList_Enum (ENUMPROCCALLBACK pfnCallBack, PVOID pUserPtr) 
{
  NODE	*pNode ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  ASSERT (pfnCallBack!=NULL) ;

  for( pNode=g_data.pFirst ; pNode ; pNode=pNode->pNext )
    if( ! pfnCallBack (pUserPtr, pNode->pData->nProcessAddress, pNode->pData->nProcessId, pNode->pData->wszPath) )
      return STATUS_UNSUCCESSFUL ;	

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS ProcList_RefreshFilterLists () 
{
  NODE	* pNode ;

  ASSERT (g_data.bInitialized) ;
  ASSERT (DrvFilter_IsLocked()) ;
  ASSERT (ProcList_IsLocked()) ;
  
  for( pNode=g_data.pFirst ; pNode!=NULL ; pNode=pNode->pNext )  
    {
      PROCSTRUCT * pProc = pNode->pData ;
      
      ASSERT (pProc!=NULL) ;

      DrvFilter_GetFiltersForProgram (pProc->wszPath, pProc->aFilters,
				      &pProc->nFilters, MAX_FILTERS) ;
      
      TRACE_INFO (TEXT("Program %ls has %d filter(s) assigned\n"), 
		  pProc->wszPath, pProc->nFilters) ;
    }

  return STATUS_SUCCESS ;
}

/*

NTSTATUS ProcList_GetFilterListOfCurrentProcess (HFILTER	*pFilters,
						 ULONG		*pnLength)
{
  PROCADDR	nProcessAddress ;
  NODE		*pNode ;

  ASSERT (ProcList_IsLocked()) ;

  nProcessAddress = (PROCADDR)IoGetCurrentProcess () ;

  for( pNode=g_data.pFirst ; pNode!=NULL ; pNode=pNode->pNext )
    {
      if( pNode->pData->nProcessAddress == nProcessAddress )	 
	{
	  *pnLength = min(*pnLength, pNode->pData->nFilters) ;
	  
	  if( *pnLength > 0 )
	    memcpy (pFilters, pNode->pData->aFilters, (*pnLength)*sizeof(HFILTER)) ;
	  
	  if( pNode->pData->nProcessId != (UINT)PsGetCurrentProcessId() )
	    {
	      TRACE_WARNING (TEXT("ID of process %d changed to %d\n"),
			     pNode->pData->nProcessId, PsGetCurrentProcessId()) ;
	      pNode->pData->nProcessId = (UINT)PsGetCurrentProcessId() ;
	    }
	  
	  return STATUS_SUCCESS ;
	}
    } 
  
  *pnLength = 0 ;

  return STATUS_NOT_FOUND ;
}
*/


PROCSTRUCT* ProcList_Get (PROCADDR nProcessAddress) 
{
  NODE		*pCurNode ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  ASSERT (ProcList_IsLocked()) ;
  
  for( pCurNode=g_data.pFirst ; pCurNode!=NULL ; pCurNode=pCurNode->pNext )
    if( pCurNode->pData->nProcessAddress == nProcessAddress )
      return pCurNode->pData ;

  TRACE_WARNING (TEXT("Process with address 0x%08X not found\n"), nProcessAddress) ;

  return NULL ;
}

/*
NTSTATUS ProcList_GetProcessId (PROCADDR nProcessAddress, ULONG * pnProcessID) 
{
  NODE		*pCurNode ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  ASSERT (ProcList_IsLocked()) ;
  
  for( pCurNode=g_data.pFirst ; pCurNode!=NULL ; pCurNode=pCurNode->pNext )
    if( pCurNode->pData->nProcessAddress == nProcessAddress )
      {
	*pnProcessID = pCurNode->pData->nProcessId ;
	return STATUS_SUCCESS ;
      }

  TRACE_WARNING (TEXT("Process with address 0x%08X nor found\n"), nProcessAddress) ;

  return STATUS_NOT_FOUND ;
}
*/
