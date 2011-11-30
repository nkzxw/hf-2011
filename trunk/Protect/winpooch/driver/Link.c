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

#define	TRACE_LEVEL			2
#define	SLOT_COUNT			3
#define ENABLE_TIMEOUT			1
#define MAX_RESPONSE_SIZE		16		


/******************************************************************/
/* Includes                                                       */
/******************************************************************/

// module's interface
#include "Link.h"

// project's headers
#include "DrvInterface.h"
#include "DrvStatus.h"
#include "Trace.h"


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define REQUEST_TIMEOUT		60 //seconds
#define RESPONSE_TIMEOUT	180 //seconds


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  int		iSlot ;
} REQUEST_HEADER ;

typedef struct {
  int		iSlot ;
} RESPONSE_HEADER ;

typedef struct {
  BOOL		bSlotInUse ;
  KEVENT	evResponseReceived ;
  BYTE		aResponseBuffer[MAX_RESPONSE_SIZE] ;
  UINT		nResponseSize ;
} SLOT ;

typedef struct {
  BOOL		bEnabled ;
  KEVENT	evAppWaiting ;
  KSEMAPHORE	semFreeSlots ;
  KMUTEX	mutReserveSlot ;
  PIRP		pPendingIrp ;
#if ENABLE_TIMEOUT	
  LARGE_INTEGER liRequestTimeOut ;
  LARGE_INTEGER liResponseTimeOut ;
#endif
  SLOT		aSlots[SLOT_COUNT] ;
} INTERNAL_DATA ;

static INTERNAL_DATA g_data ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

VOID DDKAPI _Link_CancelRoutine (PDEVICE_OBJECT pDeviceObject, PIRP pIrp) ;


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Link_IsConnected () 
{
  return g_data.bEnabled ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS _Link_AcquireSlot (UINT * piSlot)
{
  UINT iSlot; 
  NTSTATUS nStatus ;

  //
  // Waiting for a free slot to be avail
  //
  {
    TRACE_INFO (TEXT("Waiting for a free slot...\n")) ;
    
    nStatus = KeWaitForSingleObject (&g_data.semFreeSlots,
				     Executive, KernelMode, 
				     TRUE, NULL) ;
    
    if( nStatus != STATUS_SUCCESS )
      {
	TRACE_BREAK (TEXT("Error while waiting for semaphore\n")) ;
	return nStatus ;
      }
    
    TRACE_INFO (TEXT("A free slot should be available\n")) ;
  }
  
  //
  // Reserve a slot number
  //
  {
    TRACE_INFO (TEXT("Looking for the free slot...\n")) ;

    KeWaitForMutexObject (&g_data.mutReserveSlot,
			  Executive, KernelMode, 
			  TRUE, NULL) ;
    
    // look for slot number
    for( iSlot=0 ; iSlot<SLOT_COUNT ; iSlot++ )
      if( ! g_data.aSlots[iSlot].bSlotInUse )
	break ;
    
    // no slot found -> error
    if( iSlot >= SLOT_COUNT ) 
      {
	TRACE_BREAK (TEXT("No free slot found")) ;
	KeReleaseMutex (&g_data.mutReserveSlot, FALSE) ;
	return STATUS_UNSUCCESSFUL ;
      }
    
    g_data.aSlots[iSlot].bSlotInUse = TRUE ;
    
    KeReleaseMutex (&g_data.mutReserveSlot, FALSE) ;
    
    TRACE_INFO (TEXT("Acquired slot %d\n"), iSlot) ;
  }

  *piSlot = iSlot ;
  
  return STATUS_SUCCESS ;
}
 

/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS _Link_ReleaseSlot (UINT iSlot)
{ 
  ASSERT (iSlot<=SLOT_COUNT) ;
  ASSERT (g_data.aSlots[iSlot].bSlotInUse) ;

  //
  // Release slot
  //
  {
    TRACE_INFO (TEXT("Releasing slot %u...\n"), iSlot) ;

    KeWaitForMutexObject (&g_data.mutReserveSlot,
			  Executive, KernelMode, 
			  TRUE, NULL) ;

    g_data.aSlots[iSlot].bSlotInUse = FALSE ;

    KeReleaseMutex (&g_data.mutReserveSlot, FALSE) ;

    TRACE_INFO (TEXT("Slot %u released\n"), iSlot) ;
  }

  //
  // Alert pending threads
  //
  KeReleaseSemaphore (&g_data.semFreeSlots, 0, 1, FALSE) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS _Link_QueryServer (UINT iSlot,
			    LPVOID pRequestData, DWORD nRequestSize,
			    LPVOID pResponseData, LPDWORD pnResponseSize,
			    DWORD nMaxResponseSize) 
{
  NTSTATUS		nStatus ;
  
  ASSERT (iSlot<=SLOT_COUNT) ;
  ASSERT (pRequestData!=NULL) ;
  ASSERT (nRequestSize>0) ;
  ASSERT (*(DWORD*)pRequestData) ;

  // application not connected => do nothing
  if( ! g_data.bEnabled ) 
    {
      if( pnResponseSize!=NULL ) *pnResponseSize = 0 ;
      return STATUS_PORT_DISCONNECTED ;
    }

  TRACE_INFO (TEXT("Slot%d: REQUEST addr=0x%08X, pid=%d, code=%u\n"), iSlot, 
	      IoGetCurrentProcess(), PsGetCurrentProcessId(), *(DWORD*)pRequestData) ;

  if( *(DWORD*)pRequestData == 0 )
    {
      TRACE_BREAK (TEXT("Request code is NULL\n")) ;
    }

  //
  // Wait for application to be ready to receive request
  //
  {
    TRACE_INFO (TEXT("Slot%d: Waiting for app to be ready...\n"), iSlot) ;
    
    // wait for app to be ready 
    nStatus = KeWaitForSingleObject (&g_data.evAppWaiting,
				     UserRequest, UserMode, 
				     TRUE,
#if ENABLE_TIMEOUT
				     &g_data.liRequestTimeOut) ;
#else
                                     NULL) ;
#endif
    
    if( nStatus!=STATUS_SUCCESS )
      {
	DrvStatus_Trace (); 
	TRACE_BREAK (TEXT("Time-out while waiting for app to be ready\n")) ;
	
	g_data.bEnabled = FALSE ;
	
	return STATUS_PORT_UNREACHABLE ;
      }

    TRACE_INFO (TEXT("Slot%d: Application is ready\n"), iSlot) ;
  }

  //
  // Send request
  //
  {
    PIRP			pIrp ;
    PIO_STACK_LOCATION		pStackLoc ;
    KIRQL			nIrql ;
    REQUEST_HEADER		header ;
    BYTE			*pWritePtr ;

    TRACE_INFO (TEXT("Slot%d: Acquiring pending IRP.\n"), iSlot) ;

    IoAcquireCancelSpinLock (&nIrql) ;
    
    pIrp = g_data.pPendingIrp ;
    g_data.pPendingIrp = NULL ;
    
    if( pIrp ) IoSetCancelRoutine (pIrp, NULL) ;
    
    IoReleaseCancelSpinLock(nIrql) ;
    
    // NULL means another thread called _Link_QueryServer at the same time
    // or that the IRP has been cancelled
    if( ! pIrp ) 
      {
	TRACE_WARNING (TEXT("Slot%d: Pending IRP has been cancelled.\n"), iSlot) ;
	g_data.bEnabled = FALSE ;
	return STATUS_PORT_DISCONNECTED ;
      }

    TRACE_INFO (TEXT("Slot%d: Pending IRP 0x%08X acquired.\n"), iSlot, pIrp) ;

    TRACE_INFO (TEXT("Slot%d: Sending request to app...\n"), iSlot) ;
    
    // write request data
    pStackLoc = IoGetCurrentIrpStackLocation (pIrp) ;
    TRACE_INFO (TEXT("Slot%d: Link buffer size = %u\n"), iSlot, pStackLoc->Parameters.DeviceIoControl.OutputBufferLength) ;
    TRACE_INFO (TEXT("Slot%d: Required size = %u\n"), iSlot, nRequestSize+sizeof(header)) ;
    ASSERT (pStackLoc->Parameters.DeviceIoControl.OutputBufferLength>=nRequestSize+sizeof(header)) ; 
    
    // get buffer address 
    pWritePtr = pIrp->AssociatedIrp.SystemBuffer ;

    // write header 
    header.iSlot = iSlot ;
    RtlCopyMemory (pWritePtr, &header, sizeof(header)) ;
    pWritePtr += sizeof(header) ;

    // write request data
    RtlCopyMemory (pWritePtr, pRequestData, nRequestSize) ;
       
    // complete the request
    pIrp->IoStatus.Status = STATUS_SUCCESS ;
    pIrp->IoStatus.Information = nRequestSize+sizeof(header) ;
    IoCompleteRequest (pIrp, IO_NO_INCREMENT) ;

    TRACE_INFO (TEXT("Slot%d: Request sent to app\n"), iSlot) ;
  }

  //
  // Wait response or Ack from application
  //
  {    
    if( nMaxResponseSize>0 && pResponseData!=NULL )
      TRACE_INFO (TEXT("Slot%d: Waiting for response...\n"), iSlot) ;
    else
      TRACE_INFO (TEXT("Slot%d: Waiting for ack...\n"), iSlot) ;
    
    // wait for response
#if ENABLE_TIMEOUT
    nStatus = KeWaitForSingleObject (&g_data.aSlots[iSlot].evResponseReceived,
				     UserRequest, UserMode, 
				     TRUE, 
				     &g_data.liResponseTimeOut) ;
#else
    nStatus = KeWaitForSingleObject (&g_data.aSlots[iSlot].evResponseReceived,
				     UserRequest, UserMode, 
				     TRUE, 
                                     NULL) ;
#endif
    
    if( nStatus!=STATUS_SUCCESS )
      {
	DrvStatus_Trace () ;
	TRACE_BREAK (TEXT("Slot%d: Time-out while waiting for app response\n"), iSlot) ;
	
	g_data.bEnabled = FALSE ;
	
	return STATUS_REQUEST_ABORTED ;
      }
    
    if( g_data.aSlots[iSlot].nResponseSize > nMaxResponseSize )
      {
	TRACE_WARNING (TEXT("Slot%u: Response size (%u bytes) bigger than expected (%u bytes)\n"), 
		       iSlot, g_data.aSlots[iSlot].nResponseSize, nMaxResponseSize) ;
	g_data.aSlots[iSlot].nResponseSize = nMaxResponseSize ;
      }

    // read response data
    if( pResponseData!=NULL )
      RtlCopyMemory (pResponseData, g_data.aSlots[iSlot].aResponseBuffer, g_data.aSlots[iSlot].nResponseSize) ;
    
    // set response size
    if( pnResponseSize ) 
      *pnResponseSize = g_data.aSlots[iSlot].nResponseSize;
      
    if( g_data.aSlots[iSlot].nResponseSize>0 )
      TRACE_INFO (TEXT("Slot%d: Response received (size=%u)\n"), iSlot, *pnResponseSize) ;
    else
      TRACE_INFO (TEXT("Slot%d: Ack received\n"), iSlot) ;
  }

  TRACE_INFO (TEXT("Slot%d: REQUEST FINISHED\n"), iSlot) ;
  
  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS Link_QueryServer (LPVOID pRequestData, DWORD nRequestSize,
			   LPVOID pResponseData, LPDWORD pnResponseSize,
			   DWORD nMaxResponseSize) 
{
  NTSTATUS	nStatus ;
  UINT		iSlot ;

  TRACE ;

  nStatus = _Link_AcquireSlot (&iSlot) ;

  if( nStatus == STATUS_SUCCESS )
    {
      nStatus = _Link_QueryServer (iSlot, pRequestData, nRequestSize,
				   pResponseData, pnResponseSize,
				   nMaxResponseSize) ;
      _Link_ReleaseSlot (iSlot) ;
    }

  return nStatus ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS Link_CatchIrpApp2Drv (PDEVICE_OBJECT pDeviceObject, PIRP pIrp) 
{
  NTSTATUS		nStatus = STATUS_SUCCESS ;
  PIO_STACK_LOCATION	pStackLoc ;

  TRACE_INFO (TEXT("Message received from application\n")) ;

  if( ! g_data.bEnabled )
    {
      TRACE_ERROR (TEXT("Rejecting ioctl from app because app-link not initialized\n")) ;

      nStatus = STATUS_PORT_CONNECTION_REFUSED ;

      pIrp->IoStatus.Status = nStatus ;
      pIrp->IoStatus.Information = 0 ;
      IoCompleteRequest (pIrp, IO_NO_INCREMENT) ;  
      
      return nStatus ;
    }
  
  pStackLoc = IoGetCurrentIrpStackLocation (pIrp) ;

  // is there a message from app ?
  if( pIrp->AssociatedIrp.SystemBuffer==NULL ||
      pStackLoc->Parameters.DeviceIoControl.InputBufferLength<sizeof(RESPONSE_HEADER) )
    {
      TRACE_ERROR (TEXT("Empty response from application\n")) ;
      nStatus = STATUS_UNSUCCESSFUL ;
      pIrp->IoStatus.Status = nStatus ;
      pIrp->IoStatus.Information = 0 ;
      IoCompleteRequest (pIrp, IO_NO_INCREMENT) ;  
      return nStatus ;  
    }

  {
    BYTE	*pReadPtr = pIrp->AssociatedIrp.SystemBuffer ;
    UINT	nSize = pStackLoc->Parameters.DeviceIoControl.InputBufferLength ;
    UINT	iSlot ;
    
    RESPONSE_HEADER * pHeader = (VOID*)pReadPtr ;
    
    pReadPtr += sizeof(RESPONSE_HEADER) ;
    nSize -= sizeof(RESPONSE_HEADER) ;
    
    iSlot = pHeader->iSlot ;
    
    TRACE_INFO (TEXT("Message belongs to slot %u\n"), iSlot) ;
    
    // check slot number
    if( iSlot>=SLOT_COUNT ) 
      {
	TRACE_ERROR (TEXT("Invalid slot number %u\n"), iSlot) ;
	nStatus = STATUS_UNSUCCESSFUL ;
	pIrp->IoStatus.Status = nStatus ;
	pIrp->IoStatus.Information = 0 ;
	IoCompleteRequest (pIrp, IO_NO_INCREMENT) ;  
	return nStatus ;   
      }
    
    if( ! g_data.aSlots[iSlot].bSlotInUse )
      {
	TRACE_ERROR (TEXT("Slot%u: neither response nor ask was expected\n"), iSlot) ;
	nStatus = STATUS_UNSUCCESSFUL ;
	pIrp->IoStatus.Status = nStatus ;
	pIrp->IoStatus.Information = 0 ;
	IoCompleteRequest (pIrp, IO_NO_INCREMENT) ;  
	return nStatus ;   
      }

    if( nSize > MAX_RESPONSE_SIZE )
      {
	TRACE_WARNING (TEXT("Slot%u: Response size (%u) bigger than MAX_RESPONSE_SIZE (%u)\n"), 
		       iSlot, nSize, MAX_RESPONSE_SIZE) ;
	nSize = MAX_RESPONSE_SIZE ;
      }
    
    // read response data
    RtlCopyMemory (g_data.aSlots[iSlot].aResponseBuffer, pReadPtr, nSize) ;
    g_data.aSlots[iSlot].nResponseSize = nSize ;
        
    // tell the other thread that the response has been received
    KeSetEvent (&g_data.aSlots[iSlot].evResponseReceived, 0, FALSE) ;
  }

  nStatus = STATUS_SUCCESS ;
  pIrp->IoStatus.Status = nStatus ;
  pIrp->IoStatus.Information = pStackLoc->Parameters.DeviceIoControl.InputBufferLength ;
  IoCompleteRequest (pIrp, IO_NO_INCREMENT) ;  
  
  return nStatus ;
}
  

/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS Link_CatchIrpDrv2App (PDEVICE_OBJECT pDeviceObject, PIRP pIrp) 
{
  NTSTATUS	nStatus = STATUS_SUCCESS ;
  KIRQL		nIrql ;
  PIO_STACK_LOCATION	pStackLoc ;

  pStackLoc = IoGetCurrentIrpStackLocation (pIrp) ;

  // app doesn't want to receive a message ?
  if( pStackLoc->Parameters.DeviceIoControl.OutputBufferLength==0 )
    {
      TRACE_WARNING (TEXT("No output buffer provided\n")) ;

      nStatus = STATUS_SUCCESS ;
      pIrp->IoStatus.Status = nStatus ;
      pIrp->IoStatus.Information = 0 ;
      IoCompleteRequest (pIrp, IO_NO_INCREMENT) ;  
      return nStatus ;   
    }

  TRACE_INFO (TEXT("Registering pending IRP 0x%08X\n"), pIrp) ;

  IoAcquireCancelSpinLock (&nIrql) ;

  if( pIrp->Cancel )
    {
      nStatus = STATUS_CANCELLED ;
    }
  else if( g_data.pPendingIrp )
    {
      nStatus = STATUS_DEVICE_BUSY ;
      TRACE_ERROR (TEXT("Rejecting an IRP because another one is pending\n")) ;
    }
  else
    {
      IoMarkIrpPending (pIrp) ;
      IoSetCancelRoutine (pIrp, _Link_CancelRoutine) ;

      g_data.pPendingIrp = pIrp ;
      
      nStatus = STATUS_PENDING ;
    } 

  IoReleaseCancelSpinLock(nIrql) ;

  if( nStatus==STATUS_PENDING )
    {
      TRACE_INFO (TEXT("IRP 0x%08X registered, notify other threads\n"), pIrp) ;
      KeSetEvent (&g_data.evAppWaiting, 0, FALSE) ;
    }
  else
    {
      TRACE_ERROR (TEXT("Failed to register pending IRP 0x%08X (status=0x%08X)\n"), pIrp, nStatus) ;
      pIrp->IoStatus.Status = nStatus;
      IoCompleteRequest (pIrp, IO_NO_INCREMENT) ; 
    }

  return nStatus ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS Link_Init () 
{
  UINT iSlot ;

  TRACE ;

#if ENABLE_TIMEOUT
  g_data.liRequestTimeOut.QuadPart = - REQUEST_TIMEOUT *1000*1000*10 ;
  g_data.liResponseTimeOut.QuadPart = - RESPONSE_TIMEOUT *1000*1000*10 ;
#endif

  KeInitializeMutex (&g_data.mutReserveSlot, 0) ;
  KeInitializeSemaphore (&g_data.semFreeSlots, SLOT_COUNT, SLOT_COUNT) ;
  KeInitializeEvent (&g_data.evAppWaiting, SynchronizationEvent, FALSE) ;

  for( iSlot=0 ; iSlot<SLOT_COUNT ; iSlot++ )
    {
      g_data.aSlots[iSlot].bSlotInUse = FALSE ;
      KeInitializeEvent (&g_data.aSlots[iSlot].evResponseReceived, SynchronizationEvent, FALSE) ;
    }

  g_data.pPendingIrp = NULL ;

  g_data.bEnabled = TRUE ;

  TRACE_INFO (TEXT("App-link connected\n")) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS Link_Uninit () 
{
  PIRP	pIrp ;
  KIRQL	nIrql ;

  TRACE ;

  g_data.bEnabled = FALSE ;

  TRACE_INFO (TEXT("App-link disconnected\n")) ;

  IoAcquireCancelSpinLock (&nIrql) ;

  pIrp = g_data.pPendingIrp ;
  g_data.pPendingIrp = NULL ;

  if( pIrp ) IoSetCancelRoutine (pIrp, NULL) ;

  IoReleaseCancelSpinLock(nIrql) ;

  // cancel pending IRP
  if( pIrp )
    {
      TRACE_INFO (TEXT("Cancelling pending IRP\n")) ;
      pIrp->IoStatus.Status = STATUS_CANCELLED ;
      IoCompleteRequest (pIrp, IO_NO_INCREMENT) ;
    }
  
  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

VOID DDKAPI _Link_CancelRoutine (PDEVICE_OBJECT pDeviceObject, PIRP pIrp) 
{
  TRACE_INFO (TEXT("Cancelling IRP 0x%08X\n"), pIrp) ;
     
  g_data.pPendingIrp = NULL ;

  IoReleaseCancelSpinLock (pIrp->CancelIrql);

  pIrp->IoStatus.Status = STATUS_CANCELLED ;
  IoCompleteRequest (pIrp, IO_NO_INCREMENT) ;

  KeSetEvent (&g_data.evAppWaiting, 0, FALSE) ;

  TRACE_INFO (TEXT("IRP 0x%08X cancelled\n"), pIrp) ;
}



/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL Link_IsRequesting () 
{
  return KeReadStateSemaphore(&g_data.semFreeSlots)!=SLOT_COUNT ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL Link_IsAppWaiting () 
{
  return 0!=KeReadStateEvent(&g_data.evAppWaiting) ;
}
