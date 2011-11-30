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
#include "WatchedObjects.h"

// Windows' headers
#include <ntddk.h>

// project's headers
#include "Malloc.h"
#include "Trace.h"


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct NODE {
  PVOID		pObject ;
  ULONG		nType ;
  PVOID		pUserData ;
  ULONG		nUserDataSize ;
  struct NODE	*pPrev ;
  struct NODE	*pNext ;
} NODE ;

typedef struct {
  BOOL		bInitialized ;
  KMUTEX	mutex ;
  ULONG		nNodeCount ;
  struct NODE	*pFirst ;
  struct NODE	*pLast ;
} INTERNAL_DATA ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static INTERNAL_DATA g_data ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

BOOL	_WatchObjs_IsLocked () ;

NTSTATUS _WatchObjs_CleanList () ;


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WatchObjs_Init () 
{
  TRACE ;

  KeInitializeMutex (&g_data.mutex, 0) ;

  g_data.pFirst = NULL ;
  g_data.pLast = NULL ;
  g_data.nNodeCount = 0 ;

  g_data.bInitialized = TRUE ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WatchObjs_Uninit () 
{
  NODE	*pNode, *pNext ;
  ULONG	nNodeCount = 0 ;

  TRACE ;

  WatchObjs_Lock () ;

  if( g_data.pFirst ) ASSERT (g_data.pFirst->pPrev==NULL) ;
  if( g_data.pLast ) ASSERT (g_data.pLast->pNext==NULL) ;
  
  for( pNode=g_data.pFirst ; pNode ; pNode=pNext )
    {
      pNext = pNode->pNext ;

      if( pNext ) ASSERT(pNext->pPrev==pNode) ;

      FREE (pNode->pUserData) ;
      FREE (pNode) ;

      nNodeCount++ ;
    }

  WatchObjs_Unlock () ;

  if( nNodeCount!=g_data.nNodeCount )
    TRACE_WARNING (TEXT("Freed %u nodes, whereas node count was %u\n"), 
		   nNodeCount, g_data.nNodeCount) ;

  g_data.bInitialized = FALSE ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WatchObjs_Lock () 
{
  NTSTATUS	nStatus ;
  LARGE_INTEGER	liTimeOut ;

  // verify that this module is initialized
  ASSERT (g_data.bInitialized) ;

  liTimeOut.QuadPart = - 10000 * 10000 ;

  nStatus = KeWaitForMutexObject (&g_data.mutex, 
				  Executive, 
				  KernelMode,
				  FALSE,
				  &liTimeOut) ;
  
  if( nStatus != STATUS_SUCCESS )
    TRACE_ERROR (TEXT("KeWaitForMutexObject failed (0x%08X)\n"), nStatus) ;

  return nStatus ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WatchObjs_Unlock () 
{
  // TRACE ;

  // verify that this module is initialized
  ASSERT (g_data.bInitialized) ;

  ASSERT (_WatchObjs_IsLocked()) ;

  KeReleaseMutex (&g_data.mutex, FALSE) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

BOOL _WatchObjs_IsLocked () 
{
  ASSERT (g_data.bInitialized) ;

  return 0==KeReadStateMutex(&g_data.mutex) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WatchObjs_AddFromPointer (PVOID	pObject,
				   ULONG	nType,
				   PVOID	pUserData,
				   ULONG	nUserDataSize) 
{
  NODE	* pNode ;

  TRACE_INFO (TEXT("Object = 0x%08X\n"), pObject) ;

  ASSERT (_WatchObjs_IsLocked()) ;

  pNode = MALLOC (sizeof(NODE)) ;
  if( pNode==NULL ) 
    {
      TRACE_ERROR (TEXT("Failed to allocate structure NODE (%u bytes)\n"), sizeof(NODE)) ;
      return STATUS_INSUFFICIENT_RESOURCES ;
    }

  pNode->pObject	= pObject ;
  pNode->nType		= nType ;
  pNode->pUserData	= pUserData ;
  pNode->nUserDataSize	= nUserDataSize ;
  
  pNode->pPrev	= g_data.pLast ;
  pNode->pNext	= NULL ;

  if( g_data.pLast )
    g_data.pLast->pNext = pNode ;
  g_data.pLast = pNode ;

  if( ! g_data.pFirst )
    g_data.pFirst = pNode ;

  g_data.nNodeCount++ ;
/*
  if( (g_data.nNodeCount>=1000) && (g_data.nNodeCount%100)==0 )
    {
      TRACE_WARNING (TEXT("More than %u objects are currently being watched\n"), 
		     g_data.nNodeCount) ;
      _WatchObjs_CleanList () ;
    }
*/
  return STATUS_SUCCESS ;
}

/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WatchObjs_AddFromHandle (HANDLE	hObject,
				  ULONG		nType,
				  PVOID		pUserData,
				  ULONG		nUserDataSize) 
{
  PVOID		pObject=NULL ;
  NTSTATUS	nStatus ;

  ASSERT (_WatchObjs_IsLocked()) ;
  
  nStatus = ObReferenceObjectByHandle (hObject, GENERIC_ALL,
				       NULL, KernelMode, &pObject,
				       NULL) ;    

  if( nStatus!=STATUS_SUCCESS || pObject==NULL )
    {
      TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), 
		   nStatus) ;
      return nStatus ;
    }

  nStatus = WatchObjs_AddFromPointer (pObject, nType,
				      pUserData, nUserDataSize) ;

  ObDereferenceObject (pObject) ;

  return nStatus ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WatchObjs_RemFromPointer (PVOID pObject) 
{
  NODE	*pPrev, *pCur, *pNext ;

  //TRACE ;

  ASSERT (_WatchObjs_IsLocked()) ;

  for( pCur=g_data.pFirst ; pCur ; pCur=pNext )
    {
      pPrev = pCur->pPrev ;
      pNext = pCur->pNext ;

      if( pPrev==NULL ) ASSERT (g_data.pFirst==pCur) ;
      if( pNext==NULL ) ASSERT (g_data.pLast==pCur) ;
      if( g_data.pFirst==pCur ) ASSERT(pPrev==NULL) ;
      if( g_data.pLast==pCur ) ASSERT(pNext==NULL) ;

      if( pCur->pObject == pObject )
	{
	  ASSERT (pCur->nType!=0) ;

	  FREE (pCur->pUserData) ; // <- bug check 19 here ?
	  FREE (pCur) ;

	  if( pPrev!=NULL ) 
	    pPrev->pNext = pNext ;
	  else
	    g_data.pFirst = pNext ;

	  if( pNext!=NULL )
	    pNext->pPrev = pPrev ;
	  else
	    g_data.pLast = pPrev ;
	  
	  pCur = pPrev ;

	  TRACE_INFO (TEXT("Object = 0x%08X\n"), pObject) ;

	  g_data.nNodeCount-- ;
/*
	  if( (g_data.nNodeCount>=1000) && (g_data.nNodeCount%100)==0 )
	    {
	      TRACE_WARNING (TEXT("Less than %u objects are currently being watched\n"), 
			     g_data.nNodeCount) ;
	      _WatchObjs_CleanList () ;
	    }
*/
	}
    }

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WatchObjs_GetFromPointer (PVOID	pObject,
				   ULONG	nType,
				   PVOID	*ppUserData,
				   ULONG	*pnUserDataSize) 
{
  NODE * pCur ;

  TRACE ;

  ASSERT (_WatchObjs_IsLocked()) ;
  
  for( pCur=g_data.pFirst ; pCur ; pCur=pCur->pNext )
    {
      if( pCur->pObject==pObject && pCur->nType==nType )
	{
	  *ppUserData	= pCur->pUserData ;
	  *pnUserDataSize = pCur->nUserDataSize ;
	  
	  return STATUS_SUCCESS ;
	}
    }

  //TRACE_WARNING (TEXT("Object 0x%08X not found\n"), pObject) ;

  return STATUS_NOT_FOUND ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS WatchObjs_GetFromHandle (HANDLE	hObject,
				  ULONG		nType,
				  PVOID		*ppUserData,
				  ULONG		*pnUserDataSize) 
{
  PVOID		pObject=NULL ;
  NTSTATUS	nStatus ;
  
  ASSERT (g_data.bInitialized) ;
  ASSERT (_WatchObjs_IsLocked()) ;

  nStatus = ObReferenceObjectByHandle (hObject, GENERIC_ALL,
				       NULL, KernelMode, &pObject,
				       NULL) ;    

  if( nStatus!=STATUS_SUCCESS || pObject==NULL )
    {
      TRACE_ERROR (TEXT("ObReferenceObjectByHandle failed (status=0x%08X)\n"), 
		   nStatus) ;
      return nStatus ;
    }

  nStatus = WatchObjs_GetFromPointer (pObject, nType,
				      ppUserData, pnUserDataSize) ;

  ObDereferenceObject (pObject) ;

  return nStatus ;
}


/******************************************************************/
/* Internal function                                              */
/******************************************************************/

NTSTATUS _WatchObjs_CleanList () 
{  
  ULONG		nRemovedObjects = 0 ;
  NODE		*pPrev, *pCur, *pNext ;

  TRACE ;

  ASSERT (_WatchObjs_IsLocked()) ;

  for( pCur=g_data.pFirst ; pCur ; pCur=pNext )
    {
      ULONG	nPointerCount = *(ULONG*)((BYTE*)pCur->pObject-0x18) ;
      ULONG	nHandleCount = *(ULONG*)((BYTE*)pCur->pObject-0x14) ;

      pPrev = pCur->pPrev ;
      pNext = pCur->pNext ;

      if( nPointerCount==0 || nPointerCount>40000 || nHandleCount>40000 )
	{
	  nRemovedObjects++ ;

	  FREE (pCur->pUserData) ;
	  FREE (pCur) ;

	  if( pPrev!=NULL )
	    pPrev->pNext = pNext ;
	  else
	    g_data.pFirst = pNext ;

	  if( pNext!=NULL )
	    pNext->pPrev = pPrev ;
	  else
	    g_data.pLast = pPrev ;
	  
	  pCur = pPrev ;

	  g_data.nNodeCount-- ;
	}
    }

  TRACE_WARNING (TEXT("%u objects removed\n"), nRemovedObjects) ;

  return STATUS_SUCCESS ;
}



NTSTATUS WatchObjs_AddWotFile (IN PVOID			pObject,
			       IN LARGE_INTEGER		*pliFileTime,
			       IN UNICODE_STRING	*pusFilePath)
{
  WOTFILE	*pUserData ;
  ULONG		nUserDataSize ;
  NTSTATUS	nStatus ;

  ASSERT (_WatchObjs_IsLocked()) ;
  ASSERT (pusFilePath!=NULL) ;
  ASSERT (pliFileTime!=NULL) ;
  
  nUserDataSize = sizeof(WOTFILE) + pusFilePath->Length+2 ;
  pUserData = (WOTFILE*) MALLOC (nUserDataSize) ;

  if( pUserData == NULL ) 
    {
      TRACE_ERROR (TEXT("Failed to allocate structure WOTFILE (%u bytes)\n"), nUserDataSize) ;
      return STATUS_INSUFFICIENT_RESOURCES ;
    }

  pUserData->liFileTime = *pliFileTime ;
  memcpy (pUserData->wszFilePath, pusFilePath->Buffer, pusFilePath->Length) ;
  pUserData->wszFilePath[pusFilePath->Length/2] = 0 ;

  nStatus = WatchObjs_AddFromPointer (pObject, WOT_FILE, pUserData, nUserDataSize) ;

  if( FAILED(nStatus) )
    {
      FREE (pUserData) ;
      TRACE_ERROR (TEXT("WatchObjs_AddFromHandle failed (status=0x%08X)\n"), nStatus) ;
    }

  return nStatus ;
}
