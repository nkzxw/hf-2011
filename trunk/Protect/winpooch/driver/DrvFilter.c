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
#include "DrvFilter.h"

// project's headers
#include "DrvStatus.h"
#include "Trace.h"


/******************************************************************/
/* Code sections pragmas                                          */
/******************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, DrvFilter_Init)
#pragma alloc_text (PAGE, DrvFilter_Uninit)
#pragma alloc_text (PAGE, DrvFilter_SetFilterSet)
#pragma alloc_text (PAGE, DrvFilter_GetFilterSet)
#pragma alloc_text (PAGE, DrvFilter_LockMutex)
#pragma alloc_text (PAGE, DrvFilter_UnlockMutex)
#pragma alloc_text (PAGE, DrvFilter_SetSerializedFilterSet)
#endif


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

#define LOCK_TIMEOUT		30 /*secondes*/


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  BOOL		bInitialized ;
  HFILTERSET	hFilterSet ;
  KMUTEX	mutex ;
} INTERNALDATA ;


/******************************************************************/
/* Internal data                                                  */
/******************************************************************/

static INTERNALDATA	g_data ;



/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS DrvFilter_Init () 
{
  TRACE ;

  ASSERT (!g_data.bInitialized) ;
  
  KeInitializeMutex (&g_data.mutex, 0) ;  

  g_data.hFilterSet = NULL ;
  g_data.bInitialized = TRUE ;
 
  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS DrvFilter_Uninit () 
{
  TRACE ;

  ASSERT (g_data.bInitialized) ;

  FilterSet_Destroy (g_data.hFilterSet) ;

  g_data.hFilterSet = NULL ;
  g_data.bInitialized = FALSE ;
    
  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

HFILTERSET DrvFilter_GetFilterSet () 
{
  ASSERT (g_data.bInitialized) ;
  ASSERT (DrvFilter_IsLocked()) ;
  
  return g_data.hFilterSet ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID DrvFilter_SetFilterSet (HFILTERSET hFilterSet) 
{
  TRACE ;

  // verify that this module is initialized
  ASSERT (g_data.bInitialized) ;
  ASSERT (DrvFilter_IsLocked()) ;
  
  FilterSet_Destroy (g_data.hFilterSet) ;
  
  g_data.hFilterSet = hFilterSet ;
}



/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS DrvFilter_SetSerializedFilterSet (PCVOID pBuffer, UINT nMaxSize) 
{
  HFILTERSET	hFilterSet ;
  UINT		nSize ;

  TRACE ;

  ASSERT (g_data.bInitialized) ;
  ASSERT (DrvFilter_IsLocked()) ;

  // verify params
  ASSERT (pBuffer!=NULL) ;
  ASSERT (nMaxSize!=0) ;

  hFilterSet = FilterSet_Create (64) ;
  if( ! hFilterSet )    
    return STATUS_NO_MEMORY ;
 
  nSize = FilterSet_Unserialize (hFilterSet, pBuffer, nMaxSize) ;

  if( nSize==0 ) {
    TRACE_ERROR (TEXT("FilterSet_Unserialize failed\n")) ;
    FilterSet_Destroy (hFilterSet) ;
    return STATUS_INVALID_PARAMETER ;
  }
  
  DrvFilter_SetFilterSet (hFilterSet) ;

  return STATUS_SUCCESS ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS DrvFilter_LockMutex () 
{
  NTSTATUS	nStatus ;
  LARGE_INTEGER	liTimeOut ;

  // verify that this module is initialized
  ASSERT (g_data.bInitialized) ;

  liTimeOut.QuadPart = - 5000 * 10000 ; // 5 sec

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
  
  if( nStatus != STATUS_SUCCESS )
    {      
      DrvStatus_Trace() ;
      TRACE_BREAK (TEXT("KeWaitForMutexObject failed (0x%08X)\n"), nStatus) ;
    }

  return nStatus ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID DrvFilter_UnlockMutex () 
{
  // verify that this module is initialized
  ASSERT (g_data.bInitialized) ;

  ASSERT (DrvFilter_IsLocked()) ;

  KeReleaseMutex (&g_data.mutex, FALSE) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL DrvFilter_IsLocked ()
{
  // verify that this module is initialized
  ASSERT (g_data.bInitialized) ;

  return 0==KeReadStateMutex(&g_data.mutex) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

NTSTATUS DrvFilter_GetFiltersForProgram (IN LPCWSTR	wszPath,
					 OUT HFILTER	*pFilters,
					 OUT ULONG	*pnFilters,
					 IN ULONG	nMaxFilters) 
{
  HFILTER	hFilter ;
  ULONG		nFilters ;
  int		i, n ;

  ASSERT (DrvFilter_IsLocked()) ;

  *pnFilters = nFilters = 0 ;

  if( ! g_data.hFilterSet ) return STATUS_SUCCESS ;

  n = FilterSet_GetFilterCount (g_data.hFilterSet) ;

  for( i=n ; i>0 ; i-- )
    {
      hFilter = FilterSet_GetFilterByNum (g_data.hFilterSet, i) ;
      if( hFilter && Filter_Match(hFilter,wszPath) )
	{
	  pFilters[nFilters++] = hFilter ;
	  if( nFilters>=nMaxFilters-1 )
	    break ;
	}
    }  

  // the last filter is always the default filter :
  pFilters[nFilters] = FilterSet_GetFilterByNum (g_data.hFilterSet, 0) ;
  if( pFilters[nFilters] ) nFilters++ ;

  *pnFilters = nFilters ;

  TRACE_INFO (TEXT("Configured %d filter(s) for %ls\n"), nFilters, wszPath) ;

  return STATUS_SUCCESS ;
}
