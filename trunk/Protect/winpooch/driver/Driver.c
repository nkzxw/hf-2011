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

// ddk's header
#include <ddk\ntddk.h>

// project's headers
#include "BuildCount.h"
#include "DrvFilter.h"
#include "FiltCond.h"
#include "HookCommon.h"
#include "Hooks.h"
#include "Link.h"
#include "Malloc.h"
#include "NtUndoc.h"
#include "ProcList.h"
#include "ProjectInfo.h"
#include "ScanCache.h"
#include "Strlcpy.h"
#include "Trace.h"
#include "WatchedObjects.h"

// module's interface
#include "DrvInterface.h"


/******************************************************************/
/* Internal macros                                                */
/******************************************************************/

#define STATIC_UNICODE_STRING(symbol,value) \
  static UNICODE_STRING symbol = {sizeof(value)-sizeof(WCHAR),sizeof(value),value} ;


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct 
{
  PDRIVER_OBJECT  pDriverObject ;
  PDEVICE_OBJECT  pDeviceObject ;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION, **PPDEVICE_EXTENSION;

typedef struct 
{
  PVOID		pPrevBlock ;
  PVOID		pWritePos ;
  ULONG		nRemainBytes ;
} ENUMPROCCONTEXT ;

typedef struct 
{
  PVOID		pPrevBlock ;
  PVOID		pWritePos ;
  ULONG		nRemainBytes ;
} ENUMCACHECONTEXT ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

STATIC_UNICODE_STRING (usDeviceName, L"\\Device\\Winpooch");
STATIC_UNICODE_STRING (usSymbolicName, L"\\DosDevices\\WINPOOCH");

PDEVICE_OBJECT  gpDeviceObject  = NULL;


/******************************************************************/
/* Exported functions                                             */
/******************************************************************/

NTSTATUS STDCALL DriverEntry      (PDRIVER_OBJECT  pDriverObject,
				   PUNICODE_STRING pusRegistryPath) ;

NTSTATUS DDKAPI DispatchCreate (PDEVICE_OBJECT pDeviceObject, PIRP pIRP) ; 

NTSTATUS DDKAPI DispatchClose (PDEVICE_OBJECT pDeviceObject, PIRP pIRP) ;

NTSTATUS DDKAPI DispatchDeviceControl (PDEVICE_OBJECT pDeviceObject, PIRP pIRP) ;

VOID DDKAPI DriverUnload (PDRIVER_OBJECT pDriverObject);


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

NTSTATUS DriverCreateDevice (PDRIVER_OBJECT  pDriverObject) ;

BOOL _Driver_EnumProcCallback (PVOID pUserPtr, 
			       PROCADDR nProcessAddress,
			       ULONG nProcessId, 
			       LPCWSTR wszFilePath) ;

BOOL _Driver_EnumCacheCallback (VOID * pUserPtr, SCANCACHEID nIdentifier, LPCWSTR wszFilePath, SCANRESULT nResult, LARGE_INTEGER*pliScanTime) ;


/******************************************************************/
/* Code sections pragmas                                          */
/******************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#endif


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS DriverCreateDevice (PDRIVER_OBJECT  pDriverObject)
{
  PDEVICE_OBJECT	pDeviceObject = NULL ;
  NTSTATUS		nStatus ; 
  DEVICE_EXTENSION	*pDevExt ;
  
  nStatus = IoCreateDevice (pDriverObject, sizeof(DEVICE_EXTENSION),
			    &usDeviceName, FILE_DEVICE_UNKNOWN,
			    0, FALSE, &pDeviceObject) ;

  if( nStatus != STATUS_SUCCESS) {
    TRACE_ERROR (TEXT("IoCreateDevice failed (0x%08X)\n"), nStatus) ;
  }

  nStatus = IoCreateSymbolicLink (&usSymbolicName, &usDeviceName) ;
  
  if( nStatus != STATUS_SUCCESS) {
    TRACE_ERROR (TEXT("IoCreateSymbolicLink failed (0x%08X)\n"), nStatus) ;
    IoDeleteDevice (pDeviceObject) ;
  }

  gpDeviceObject  = pDeviceObject;
  pDevExt = pDeviceObject->DeviceExtension ;
  
  pDevExt->pDriverObject = pDriverObject ;
  pDevExt->pDeviceObject = pDeviceObject ;

  TRACE_INFO (TEXT("Device created successfully\n")) ;
  
  return nStatus ;  
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS DDKAPI DriverEntry (PDRIVER_OBJECT  pDriverObject,
			      PUNICODE_STRING pusRegistryPath)
{
  NTSTATUS	nStatus ;

  DbgPrint (TRACE_HEADER TEXT("Driver %s:%u loading\n"),
	    APPLICATION_VERSION_STRING, DRIVER_BUILD) ;

  Malloc_Init () ;

  nStatus = NtUndoc_Init() ;
  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("Can't load driver (unsuppoted Windows version)\n")) ;
      Malloc_Uninit () ;
      return nStatus ;
    }

  nStatus = WatchObjs_Init() ;
  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("WatchObjs_Init failed (status=0x%08X)\n"), nStatus) ;
      Malloc_Uninit () ;
      return nStatus ;
    }

  nStatus = ScanCache_Init () ;
  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("ScanCache_Init failed (status=0x%08X)\n"), nStatus) ;
      Malloc_Uninit () ;
      WatchObjs_Uninit () ;
      return nStatus ;
    }  

  ProcList_Init () ;
  HookCommon_Init () ;

  nStatus = Hooks_Init() ;
  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("Hooks_Init failed (0x%08X)\n"), nStatus) ;
      return nStatus ;
    }

  nStatus = DrvFilter_Init () ;
  if( nStatus != STATUS_SUCCESS )
    {
      TRACE_ERROR (TEXT("DrvFilter_Init failed (0x%08X)\n"), nStatus) ;
      Hooks_Uninit() ;
      return nStatus ;
    }

  nStatus = DriverCreateDevice (pDriverObject) ;
  
  if( nStatus == STATUS_SUCCESS)
    {
      pDriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate ;
      pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose ;
      pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl ;
      
      pDriverObject->DriverUnload  = DriverUnload ;      
    }

  nStatus = Hooks_InstallHooks () ;
  if( nStatus != STATUS_SUCCESS ) return nStatus ;
  DbgPrint (TRACE_HEADER TEXT("Hooks successfully installed.\n")) ;

  nStatus = ProcList_Lock () ;
  if( nStatus != STATUS_SUCCESS ) return nStatus ;

  ProcList_Populate () ;
  ProcList_Unlock () ;

  DbgPrint (TRACE_HEADER TEXT("Driver %s:%u loaded successfully\n"),
	    APPLICATION_VERSION_STRING, DRIVER_BUILD) ;

  return nStatus ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID DDKAPI DriverUnload (PDRIVER_OBJECT pDriverObject)
{
  TRACE ;

  Hooks_UninstallHooks () ;
  DbgPrint (TRACE_HEADER TEXT("Hooks successfully removed.\n")) ;
  //??? why here
  //??? this should be put after device deletion
  Hooks_Uninit() ;

  HookCommon_Uninit () ;

  IoDeleteSymbolicLink (&usSymbolicName) ;  
  IoDeleteDevice (gpDeviceObject) ;

  DrvFilter_Uninit () ;
  ProcList_Uninit () ;
  ScanCache_Uninit () ;
  WatchObjs_Uninit () ;
  
  Malloc_PrintStats() ;
  Malloc_Uninit () ;

  DbgPrint (TRACE_HEADER TEXT("Driver %s:%u unloaded successfully\n"),
	    APPLICATION_VERSION_STRING, DRIVER_BUILD) ;
  
  return;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS DDKAPI DispatchCreate (PDEVICE_OBJECT pDeviceObject, PIRP pIRP) 
{
  PROCSTRUCT * pProc ;
  NTSTATUS	nStatus ;

  TRACE ;

  Link_Init () ;

  nStatus = ProcList_Lock () ;
  if( nStatus != STATUS_SUCCESS ) return nStatus ;

  pProc = ProcList_Get (ProcInfo_GetCurrentProcessAddress()) ;
  if( pProc ) {
    pProc->nFlags |= PROCESS_IGNORE_ALL|PROCESS_NO_NOTIFICATION ;
    TRACE_INFO (TEXT("Application process is 0x%08X (pid=%u)\n"), 
		  ProcInfo_GetCurrentProcessAddress(),
		  ProcInfo_GetCurrentProcessId()) ;
  }
  else
    TRACE_ERROR (TEXT("Application process is not in list.\n")) ;
  ProcList_Unlock () ;

  // simply complete the request
  pIRP->IoStatus.Status = STATUS_SUCCESS ;
  pIRP->IoStatus.Information = 0 ;
  IoCompleteRequest (pIRP, IO_NO_INCREMENT) ;
	
  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS DDKAPI DispatchClose (PDEVICE_OBJECT pDeviceObject, PIRP pIRP) 
{
  TRACE ;

  Link_Uninit () ;

  // TO BE REMOVED !!!
  DrvFilter_LockMutex () ;
  DrvFilter_SetFilterSet (NULL) ;
  ProcList_Lock () ;
  ProcList_RefreshFilterLists () ;
  ProcList_Unlock () ;
  DrvFilter_UnlockMutex () ;

  // simply complete the request
  pIRP->IoStatus.Status = STATUS_SUCCESS ;
  pIRP->IoStatus.Information = 0 ;
  IoCompleteRequest (pIRP, IO_NO_INCREMENT) ;
	
  return STATUS_SUCCESS ;
}

/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS DDKAPI DispatchDeviceControl (PDEVICE_OBJECT pDeviceObject, PIRP pIrp) 
{
  NTSTATUS		nStatus = STATUS_NOT_IMPLEMENTED ;
  IO_STACK_LOCATION	*pStackLoc ;
  int			nCode ;
  UINT			nTransferedBytes = 0 ;

  TRACE ;

  pStackLoc = IoGetCurrentIrpStackLocation (pIrp) ;
  nCode = pStackLoc->Parameters.DeviceIoControl.IoControlCode ;

  TRACE_INFO (TEXT("IoControlCode = 0x%06X\n"), nCode) ;

  switch( nCode )
    {
    case IOCTL_LINK_DRV2APP:

      TRACE_INFO (TEXT("IOCTL_LINK_DRV2APP\n")) ;

      return Link_CatchIrpDrv2App (pDeviceObject, pIrp) ;

    case IOCTL_LINK_APP2DRV:

      TRACE_INFO (TEXT("IOCTL_LINK_APP2DRV\n")) ;

      return Link_CatchIrpApp2Drv (pDeviceObject, pIrp) ;

    case IOCTL_SET_FILTERSET:   
      { 
	INT		nInBufSize ;
	LPVOID		pInBuf ;

	TRACE_INFO (TEXT("IOCTL_SET_FILTERSET\n")) ;

	nInBufSize = pStackLoc->Parameters.DeviceIoControl.InputBufferLength ;
	pInBuf = pIrp->AssociatedIrp.SystemBuffer ;
       
	nStatus = DrvFilter_LockMutex () ;
	if( nStatus != STATUS_SUCCESS ) break ;

	nStatus = DrvFilter_SetSerializedFilterSet (pInBuf, nInBufSize) ;

	if( nStatus==STATUS_SUCCESS )
	  {
	    DbgPrint (TRACE_HEADER TEXT("Filters has been updated.\n")) ;

	    nStatus = ProcList_Lock () ;

	    if( nStatus==STATUS_SUCCESS )
	      {
		ProcList_RefreshFilterLists () ;
		ProcList_Unlock () ;
	      }
	  }

	DrvFilter_UnlockMutex () ;
      }
      break ;


    case IOCTL_GET_PROCESSLIST:
      {
	INT		nOutBufSize ;
	LPVOID		pOutBuf ;

	ENUMPROCCONTEXT	context ;

	TRACE_INFO (TEXT("IOCTL_GET_PROCESSLIST\n")) ;

       	nOutBufSize = pStackLoc->Parameters.DeviceIoControl.OutputBufferLength ;
	pOutBuf = pIrp->AssociatedIrp.SystemBuffer ;

	context.pPrevBlock = pOutBuf ;
	context.pWritePos = pOutBuf ;
	context.nRemainBytes = nOutBufSize ;
	
	nStatus = ProcList_Lock () ;	
	
	if( nStatus==STATUS_SUCCESS )
	  {	
	    nStatus = ProcList_Enum (_Driver_EnumProcCallback, &context) ;		       
	    ProcList_Unlock () ;
	  }

	if( nStatus==STATUS_SUCCESS )
	  {
	    nTransferedBytes = nOutBufSize - context.nRemainBytes ;	    
	    ((PROCESSLISTENTRY*)context.pPrevBlock)->nNextEntry = 0 ;
	  }
	else
	  {
	    TRACE_WARNING (TEXT("Buffer too small for process list\n")) ;
	    nTransferedBytes = 0 ;
	    nStatus = STATUS_BUFFER_TOO_SMALL ;
	  }
      }       
      break ;

    case IOCTL_SET_SCANNER_PATH:   
      { 
	INT		nInBufSize ;
	LPVOID		pInBuf ;

	TRACE_INFO (TEXT("IOCTL_SET_SCANNER_PATH\n")) ;

	nInBufSize = pStackLoc->Parameters.DeviceIoControl.InputBufferLength ;
	pInBuf = pIrp->AssociatedIrp.SystemBuffer ;

	if( ( pInBuf!=NULL && nInBufSize!=0 ) ||
	    ( pInBuf==NULL && nInBufSize==0 ) )
	  {
	    nStatus = ProcList_Lock() ;
	    if( nStatus == STATUS_SUCCESS )
	      {
		nStatus = ProcList_SetScannerExePath (pInBuf) ;
		ProcList_Unlock() ;
	      }
	  }
	else 
	  TRACE_ERROR (TEXT("Wrong arguments to IOCTL_SET_SCANNER_PATH\n")) ;
      }
      break ;

    case IOCTL_SET_SCAN_FILTERS:
      {
	INT		nInBufSize ;
	LPVOID		pInBuf ;

	TRACE_INFO (TEXT("IOCTL_SET_SCAN_FILTERS\n")) ;

	nInBufSize = pStackLoc->Parameters.DeviceIoControl.InputBufferLength ;
	pInBuf = pIrp->AssociatedIrp.SystemBuffer ;

	if( ( pInBuf!=NULL && nInBufSize!=0 ) ||
	    ( pInBuf==NULL && nInBufSize==0 ) )
	  {	    
	    nStatus = HookCommon_SetScanFilters (pInBuf, nInBufSize) ;
	  }
	else 
	  TRACE_ERROR (TEXT("Wrong arguments to IOCTL_SET_SCAN_FILTERS\n")) ;
      }
      break ;

    case IOCTL_KILL_PROCESS:
      {
	INT		nInBufSize ;
	LPVOID		pInBuf ;

	TRACE_INFO (TEXT("IOCTL_KILL_PROCESS\n")) ;
	
	nInBufSize = pStackLoc->Parameters.DeviceIoControl.InputBufferLength ;
	pInBuf = pIrp->AssociatedIrp.SystemBuffer ;

	if( pInBuf!=NULL && nInBufSize==sizeof(PROCADDR) )
	  {
	    PROCADDR	nProcessAddress = *(PROCADDR*)pInBuf ;
	    BOOL	bIsInList = FALSE ;

	    TRACE_ALWAYS (TEXT("IOCTL_KILL_PROCESS (ProcessAddess = 0x%08X\n"), nProcessAddress) ;

	    nStatus = ProcList_Lock () ;
	    
	    if( nStatus == STATUS_SUCCESS )
	      {
		PROCSTRUCT* p =	ProcList_Get (nProcessAddress) ;
		ProcList_Unlock () ;
		bIsInList = p != NULL ;
	      }

	    if( bIsInList )
	      {
		if( ntundoc.PspTerminateProcess )
		  nStatus = ntundoc.PspTerminateProcess ((PEPROCESS)nProcessAddress,
							 STATUS_SUCCESS) ;
		else
		  nStatus = STATUS_NOT_IMPLEMENTED ;

		if( nStatus!=STATUS_SUCCESS )
		  TRACE_ERROR (TEXT("PspTerminateProcess failed (status=0x%08X)\n"), nStatus) ;	    
	      }
	    else 
	      {
		nStatus = STATUS_INVALID_ADDRESS ;
		TRACE_ERROR (TEXT("Process 0x%08X is not in list\n"), nProcessAddress) ;
	      }	      
	  }
      }
      break ;

    case IOCTL_IGNORE_PROCESS:
      {
	INT		nInBufSize ;
	LPVOID		pInBuf ;

	TRACE_INFO (TEXT("IOCTL_IGNORE_PROCESS\n")) ;
	
	nInBufSize = pStackLoc->Parameters.DeviceIoControl.InputBufferLength ;
	pInBuf = pIrp->AssociatedIrp.SystemBuffer ;

	if( pInBuf!=NULL && nInBufSize==sizeof(SDCIGNOREPROC) )
	  {
	    SDCIGNOREPROC* pParam = pInBuf ;

	    nStatus = ProcList_Lock () ;

	    if( nStatus == STATUS_SUCCESS )
	      {
		PROCSTRUCT* p =	ProcList_Get (pParam->nProcessAddress) ;

		if( p!=NULL )
		  {
		    if( pParam->bIgnore )
		      p->nFlags |= PROCESS_IGNORE_ALL ; 
		    else
		      p->nFlags &= ~PROCESS_IGNORE_ALL ;
		  }
		else nStatus = STATUS_INVALID_ADDRESS ;

		ProcList_Unlock () ;
	      }
	  }
	else
	  {
	    TRACE_ERROR (TEXT("Wrong parameters to IOCTL_IGNORE_PROCESS\n")) ;
	  }
      }
      break ;

    case IOCTL_SYNC_CACHE:
      {
	INT		nOutBufSize ;
	LPVOID		pOutBuf ;
	INT		nInBufSize ;
	LPVOID		pInBuf ;
	LARGE_INTEGER	liLastSyncTime ;

	ENUMPROCCONTEXT	context ;

	TRACE_INFO (TEXT("IOCTL_SYNC_CACHE\n")) ;

	nInBufSize = pStackLoc->Parameters.DeviceIoControl.InputBufferLength ;
	pInBuf = pIrp->AssociatedIrp.SystemBuffer ;

       	nOutBufSize = pStackLoc->Parameters.DeviceIoControl.OutputBufferLength ;
	pOutBuf = pIrp->AssociatedIrp.SystemBuffer ;

	liLastSyncTime =  ((SDCSYNCCACHE*)pInBuf)->liLastSyncTime ;

	if( nInBufSize>=sizeof(SDCSYNCCACHE) && nOutBufSize>=sizeof(SCANCACHEHEADER) )
	  {
	    context.pPrevBlock = (BYTE*)pOutBuf + sizeof(SCANCACHEHEADER) ;
	    context.pWritePos = context.pPrevBlock ;
	    context.nRemainBytes = nOutBufSize - sizeof(SCANCACHEHEADER) ;
	    
	    nStatus = ScanCache_Lock () ;	
	    
	    if( nStatus==STATUS_SUCCESS )
	      {	
		ScanCache_GetCacheInfo (&((SCANCACHEHEADER*)pOutBuf)->nMaxCacheLength,
					&((SCANCACHEHEADER*)pOutBuf)->nFirstIdentifier,
					&((SCANCACHEHEADER*)pOutBuf)->nLastIdentifier) ;
		
		nStatus = ScanCache_EnumChangesSince (_Driver_EnumCacheCallback, 
						      &context, 
						      &liLastSyncTime) ;		       
		ScanCache_Unlock () ;
		
		if( nStatus==STATUS_SUCCESS )
		  {
		    nTransferedBytes = nOutBufSize - context.nRemainBytes ;	    
		    ((SCANCACHEENTRY*)context.pPrevBlock)->nNextEntry = 0 ;
		  }
		else
		  {
		    TRACE_WARNING (TEXT("Buffer too small for scan cache\n")) ;
		    nTransferedBytes = 0 ;
		    nStatus = STATUS_BUFFER_TOO_SMALL ;
		  }
	      }
	  }
	else
	  {
	    TRACE_WARNING (TEXT("Buffer too small for scan cache\n")) ;
	    nTransferedBytes = 0 ;
	    nStatus = STATUS_BUFFER_TOO_SMALL ;	    
	  }
      } 
      break ;

    case IOCTL_ADD_FILE_TO_CACHE:
      {
	INT		nInBufSize ;
	LPVOID		pInBuf ;

	TRACE_INFO (TEXT("IOCTL_ADD_FILE_TO_CACHE\n")) ;

	nInBufSize = pStackLoc->Parameters.DeviceIoControl.InputBufferLength ;
	pInBuf = pIrp->AssociatedIrp.SystemBuffer ;

	if( nInBufSize>=sizeof(SDCADDFILETOCACHE) )
	  {
	    SDCADDFILETOCACHE* pParams = (SDCADDFILETOCACHE*)pInBuf ;
	    WCHAR szFilePath[MAX_PATH] ;

	    wcslcpy (szFilePath, pParams->wszFilePath, min(MAX_PATH,nInBufSize-sizeof(SDCADDFILETOCACHE))) ;

	    nStatus = ScanCache_Lock () ;

	    if( nStatus == STATUS_SUCCESS )
	      {
		SCANCACHEID nIdentifier ;

		nStatus = ScanCache_GetFileId (szFilePath, &nIdentifier) ;

		if( nStatus == STATUS_SUCCESS )
		  {
		    TRACE_INFO (TEXT("Adding file \"%ls\" to cache\n"), szFilePath) ;

		    nStatus = ScanCache_SetStatus (nIdentifier, pParams->nScanResult, &pParams->liScanTime) ;
		  }

		ScanCache_Unlock () ;
	      }
	  }
	else
	  {
	    TRACE_WARNING (TEXT("Input buffer too small for IOCTL_ADD_FILE_TO_CACHE (size=%u)\n"), nInBufSize) ;
	    nTransferedBytes = 0 ;
	    nStatus = STATUS_BUFFER_TOO_SMALL ;
	  }		
      }
      break ;

    default:

      TRACE_WARNING (TEXT("IO control code 0x%06X not supported\n"), nCode) ;
      break ;
    }

  // simply complete the request
  pIrp->IoStatus.Status = nStatus ;
  pIrp->IoStatus.Information = nTransferedBytes ;
  IoCompleteRequest (pIrp, IO_NO_INCREMENT) ;
	
  return nStatus ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _Driver_EnumProcCallback (PVOID	pUserPtr, 
			       PROCADDR	nProcessAddress,
			       ULONG	nProcessId,
			       LPCWSTR	wszFilePath)
{
  ENUMPROCCONTEXT * pContext = pUserPtr ;
  ULONG	nBlockSize = sizeof(PROCESSLISTENTRY) + wcslen(wszFilePath)*2 + 2 ;

  TRACE_INFO (TEXT("%d : %ls\n"), nProcessId, wszFilePath) ;

  if( pContext->nRemainBytes < nBlockSize )
    {
      TRACE_WARNING (TEXT("Buffer too small (remain=%u, need=%u)\n"),
		     pContext->nRemainBytes, nBlockSize) ;
      return FALSE ;
    }

  ((PROCESSLISTENTRY*)pContext->pWritePos)->nNextEntry		= nBlockSize ;
  ((PROCESSLISTENTRY*)pContext->pWritePos)->nProcessAddress	= nProcessAddress ;
  ((PROCESSLISTENTRY*)pContext->pWritePos)->nProcessId		= nProcessId ;
  wcscpy (((PROCESSLISTENTRY*)pContext->pWritePos)->wszFilePath, wszFilePath) ;

  pContext->pPrevBlock = pContext->pWritePos ;
  pContext->nRemainBytes -= nBlockSize ;
  pContext->pWritePos = (BYTE*)pContext->pWritePos + nBlockSize ;

  return TRUE ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _Driver_EnumCacheCallback (VOID * pUserPtr, SCANCACHEID nIdentifier, LPCWSTR wszFilePath, SCANRESULT nScanResult, LARGE_INTEGER*pliScanTime) 
{
  ENUMCACHECONTEXT * pContext = pUserPtr ;
  ULONG	nBlockSize = sizeof(SCANCACHEENTRY) + wcslen(wszFilePath)*2 + 2 ;

  TRACE_INFO (TEXT("%lu : %ls\n"), nIdentifier, wszFilePath) ;

  if( pContext->nRemainBytes < nBlockSize )
    {
      TRACE_WARNING (TEXT("Buffer too small (remain=%u, need=%u)\n"),
		     pContext->nRemainBytes, nBlockSize) ;
      return FALSE ;
    }

  ((SCANCACHEENTRY*)pContext->pWritePos)->nNextEntry		= nBlockSize ;
  ((SCANCACHEENTRY*)pContext->pWritePos)->nIdentifier		= nIdentifier ;
  ((SCANCACHEENTRY*)pContext->pWritePos)->nScanResult		= nScanResult ;
  ((SCANCACHEENTRY*)pContext->pWritePos)->liScanTime		= *pliScanTime ;
  wcscpy (((SCANCACHEENTRY*)pContext->pWritePos)->wszFilePath, wszFilePath) ;

  pContext->pPrevBlock = pContext->pWritePos ;
  pContext->nRemainBytes -= nBlockSize ;
  pContext->pWritePos = (BYTE*)pContext->pWritePos + nBlockSize ;

  return TRUE ;
}
