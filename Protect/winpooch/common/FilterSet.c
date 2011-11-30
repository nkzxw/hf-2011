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
#include "FilterSet.h"

// standard headers
#include <windows.h>
#include <shlobj.h>
#include <tchar.h>

// project's headers
#include "Assert.h"
#include "Malloc.h"
#include "Trace.h"
#include "Wildcards.h"


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct
{
  int		nFilters ;
  int		nFiltersMax ;
  HFILTER	*pFilters ;
} FILTERSET ;

typedef struct {
  UINT		nSize ;
  UINT		nFilters ;
} SERIALHEADER ;

typedef struct {
  SERIALHEADER	header ;
  BYTE		data[1] ;
} SERIALBUFFER ;

typedef SERIALBUFFER*		PSERIALBUFFER ;
typedef const SERIALBUFFER*	PCSERIALBUFFER ;


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

HFILTERSET FilterSet_Create (int nFiltersMax) 
{
  FILTERSET	*p ;
  HFILTER	hDefFilter ;

  p = MALLOC (sizeof(FILTERSET)) ;

  if( ! p ) 
    {
      TRACE_ERROR (TEXT("Failed to allocate structure FILTERSET (%u bytes)\n"), sizeof(FILTERSET)) ;
      return NULL ;
    }

  p->nFiltersMax = nFiltersMax ;
  p->nFilters = 0 ;
  
  p->pFilters = MALLOC (nFiltersMax*sizeof(HFILTER)) ;

  if( ! p->pFilters ) 
    {
      TRACE_ERROR (TEXT("Failed to allocate HFILTER array (%u bytes)\n"), nFiltersMax*sizeof(HFILTER)) ;
      FREE (p) ;
      return NULL ;
    } 

  // clear buffer
  memset (p->pFilters, 0, nFiltersMax*sizeof(HFILTER)) ;

  // create the first filter : default 
  hDefFilter = Filter_Create(L"*") ;

  if( !hDefFilter )
    {
      FREE (p->pFilters) ;
      FREE (p) ;
      return NULL ;
    }

  Filter_SetProtected (hDefFilter, TRUE) ;
  FilterSet_AddFilter (p, hDefFilter) ;
  
  return p ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID FilterSet_Destroy (HFILTERSET h) 
{
  FILTERSET * p = h ;

  if( ! p ) return ;

  while( p->nFilters-- ) 
    {  
      Filter_Destroy (p->pFilters[p->nFilters]) ;
    }

  FREE (p->pFilters) ;
  FREE (p) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL FilterSet_AddFilter (HFILTERSET hFilterSet, HFILTER hNewFilter) 
{
  FILTERSET * p = hFilterSet ;
  HFILTER	hExistingFilter ;

  // verify params
  ASSERT (hFilterSet!=NULL) ;
  ASSERT (hNewFilter!=NULL) ;

  TRACE_INFO (TEXT("New = %s\n"), Filter_GetProgram(hNewFilter)) ;
  
  hExistingFilter = FilterSet_GetFilterStrict 
    (hFilterSet, Filter_GetProgram(hNewFilter)) ;
  
  if( hExistingFilter ) 
    {
      TRACE_INFO (TEXT("Found existing\n")) ;

      Filter_Concat (hExistingFilter, hNewFilter) ;
    }
  else
    {
      TRACE_INFO (TEXT("No existing found\n")) ;

      if( p->nFilters >= p->nFiltersMax ) return FALSE ;
      
      p->pFilters[p->nFilters++] = hNewFilter ;
    }

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

HFILTER FilterSet_GetFilter (HFILTERSET hFilterSet, LPCWSTR szProcess) 
{
  int i ;
  FILTERSET * p = hFilterSet ;
  
  // verify params
  ASSERT (hFilterSet!=NULL) ;
  ASSERT (szProcess!=NULL) ;

  for( i=p->nFilters-1 ; i>=0 ; i-- )
    {
      LPCWSTR szFiltProg = Filter_GetProgram(p->pFilters[i]) ;
      ASSERT (szFiltProg!=NULL) ;

      if( Wildcards_CompleteStringCmp(szFiltProg,szProcess) )
	return p->pFilters[i] ;
    }

  return NULL ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

HFILTER FilterSet_GetFilterStrict (HFILTERSET hFilterSet, LPCWSTR szProcess) 
{
  int i ;
  FILTERSET * p = hFilterSet ;
  
  // verify params
  ASSERT (hFilterSet!=NULL) ;
  ASSERT (szProcess!=NULL) ;

  for( i=p->nFilters-1 ; i>=0 ; i-- )
    if( ! wcsicmp(Filter_GetProgram(p->pFilters[i]),szProcess) )
      return p->pFilters[i] ;

  return NULL ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT FilterSet_GetFilterCount (HFILTERSET hFilterSet) 
{
  FILTERSET * p = hFilterSet ;
  
  // verify params
  ASSERT (hFilterSet!=NULL) ; 
  
  return p->nFilters ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

HFILTER FilterSet_GetFilterByNum (HFILTERSET hFilterSet, UINT i) 
{
  FILTERSET * p = hFilterSet ;
  
  // verify params
  ASSERT (hFilterSet!=NULL) ;

  if( i>=p->nFilters ) return NULL ;

  return p->pFilters[i] ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

HFILTER FilterSet_GetDefaultFilter (HFILTERSET hFilterSet)
{
  FILTERSET * p = hFilterSet ;

  // verify params
  ASSERT  (hFilterSet!=NULL) ;
  
  return p->pFilters[0] ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID FilterSet_RemoveByNum (HFILTERSET hFilterSet, UINT iFilter) 
{
  FILTERSET * p = hFilterSet ;
  HFILTER hFilter ;
  
  // verify params
  ASSERT (hFilterSet!=NULL) ;
  ASSERT (iFilter<p->nFilters) ;

  hFilter = p->pFilters[iFilter] ;
  p->pFilters[iFilter] = p->pFilters[--p->nFilters] ;
  p->pFilters[p->nFilters] = NULL ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID FilterSet_Remove (HFILTERSET hFilterSet, HFILTER hFilter) 
{
  FILTERSET * p = hFilterSet ;
  int i ;
  
  // verify params
  ASSERT (hFilterSet!=NULL) ;
  
  for( i=0 ; i<p->nFilters ; i++ )
    if( p->pFilters[i]==hFilter )
      break ;

  if( i<p->nFilters ) FilterSet_RemoveByNum (hFilterSet, i) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL FilterSet_EnumFilters (HFILTERSET hFilterSet, ENUMFILTERSCALLBACK pfnCb, LPVOID pContext) 
{
  FILTERSET * p = hFilterSet ;
  int i ;
  
  // verify params
  ASSERT (hFilterSet!=NULL) ;
  ASSERT (pfnCb!=NULL) ;

  for( i=0 ; i<p->nFilters ; i++ )
    if(! pfnCb (pContext, p->pFilters[i]) )
      break ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT FilterSet_Serialize (HFILTERSET hFilterSet, PVOID pBuffer, UINT nMaxSize)
{
  PSERIALBUFFER	pSerial = pBuffer ;
  FILTERSET	*pFilterSet = hFilterSet ;
  UINT		iFilter ;
  UINT		nTotalSize ;
  BYTE		*pWritePtr ;

  TRACE ;
  
  ASSERT (hFilterSet!=NULL) ;
  ASSERT (pBuffer!=NULL) ;
  ASSERT (nMaxSize>0) ;

  nTotalSize = sizeof(SERIALHEADER) ;
  pWritePtr = pSerial->data ;

  if( nTotalSize>nMaxSize ) {
    TRACE_ERROR (TEXT("Buffer too small to store header.\n")) ;
    return 0 ;
  } 

  for( iFilter=0 ; iFilter<pFilterSet->nFilters ; iFilter++ )
    {
      HFILTER	hCurFilter ; 
      UINT	nFilterSize ;

      hCurFilter = pFilterSet->pFilters[iFilter] ;
      
      nFilterSize = Filter_Serialize (hCurFilter, pWritePtr, nMaxSize-nTotalSize) ;

      if( ! nFilterSize ) {
	TRACE_ERROR (TEXT("Filter_Serialize failed\n")) ;
	return 0 ;
      }
      
      nTotalSize += nFilterSize ;
      pWritePtr += nFilterSize ;
      ASSERT (nTotalSize<=nMaxSize) ;
    }

  // fill header
  pSerial->header.nSize		= nTotalSize ;
  pSerial->header.nFilters	= pFilterSet->nFilters ;

  TRACE_INFO (TEXT("Set size = %u\n"), nTotalSize) ;
  
  return nTotalSize ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT FilterSet_Unserialize (HFILTERSET hFilterSet, PCVOID pBuffer, UINT nMaxSize)
{
  PCSERIALBUFFER pSerial = pBuffer ;
  FILTERSET	*pFilterSet = hFilterSet ;
  const BYTE	*pReadPtr ;
  int		nTotalSize ;
  int		nRemainSize ;
  UINT		iFilter ;
    
  ASSERT (hFilterSet!=NULL) ;
  ASSERT (pBuffer!=NULL) ;
  ASSERT (nMaxSize>0) ;
  
  // is buffer big enough ?
  if( nMaxSize < sizeof(SERIALHEADER) ) {
    TRACE_ERROR (TEXT("Buffer smaller than header.\n")) ;
    return 0 ;
  } 

  TRACE_INFO (TEXT("Set size = %u\n"), pSerial->header.nSize) ;

  // read header 
  nTotalSize	= pSerial->header.nSize ;
  nRemainSize	= nTotalSize - sizeof(SERIALHEADER) ;
  pFilterSet->nFilters = pSerial->header.nFilters ;
  pReadPtr	= pSerial->data ;

  // ---------------- for each filter (begin) ----------------
  for( iFilter=0 ; iFilter<pFilterSet->nFilters ; iFilter++ )
    {
      HFILTER	hCurFilter ;
      UINT	nFilterSize ;

      if( pFilterSet->pFilters[iFilter] )
	{
	  hCurFilter = pFilterSet->pFilters[iFilter] ;
	  Filter_Clear (hCurFilter) ;
	}
      else
	hCurFilter = Filter_Create (NULL) ;
      
      nFilterSize = Filter_Unserialize (hCurFilter, pReadPtr, nRemainSize) ;

      if( ! nFilterSize )
	{
	  TRACE_ERROR (TEXT("Filter_Unserialize failed\n")) ;
	  pFilterSet->nFilters = iFilter ;
	  return 0 ;
	}

      pFilterSet->pFilters[iFilter] = hCurFilter ;

      pReadPtr += nFilterSize ;
      nRemainSize -= nFilterSize ;
      ASSERT (nRemainSize>=0) ;
    }
  // ---------------- for each filter (end) ----------------

  if( nRemainSize!=0 )
    TRACE_WARNING (TEXT("Extra bytes ignored (size=%d)\n"), nRemainSize) ;
   
  return nTotalSize ;
}
