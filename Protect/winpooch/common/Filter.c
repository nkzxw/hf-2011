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
#include "Filter.h"

// project's headers
#include "Assert.h"
#include "FiltReason.h"
#include "FiltRule.h"
#include "strlcpy.h"
#include "Trace.h"
#include "Wildcards.h"
#include "Malloc.h"

// standard headers
#include <tchar.h>


/******************************************************************/
/* Internal constants                                             */
/******************************************************************/

// ... empty ...


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct
{
  WCHAR		szProgram[MAX_PATH] ;
  FILTRULE	*aRules[_FILTREASON_COUNT] ;
  BOOL		bHookEnabled ;
  BOOL		bProtected ;
} FILTER ;

typedef struct {
  UINT		nSize ;
  UINT		nProgPathSize ;
  UINT		nRules ;
  BOOL		bHookEnabled ;
  BOOL		bProtected ;
} SERIALHEADER ;

typedef struct {
  SERIALHEADER	header ;
  BYTE		data[1] ;
} SERIALBUFFER ;

typedef SERIALBUFFER*		PSERIALBUFFER ;
typedef const SERIALBUFFER*	PCSERIALBUFFER ;


/******************************************************************/
/* Internal macros                                                */
/******************************************************************/

#ifndef PAGED_CODE
#define PAGED_CODE()	((void)0)
#endif


/******************************************************************/
/* Code sections pragmas                                          */
/******************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Filter_Create)
#pragma alloc_text (PAGE, Filter_Clear)
#pragma alloc_text (PAGE, Filter_Destroy)
#pragma alloc_text (PAGE, Filter_GetProgram)
#pragma alloc_text (PAGE, Filter_SetProgram)
#pragma alloc_text (PAGE, Filter_GetProtected)
#pragma alloc_text (PAGE, Filter_SetProtected)
#pragma alloc_text (PAGE, Filter_ResetRules)
#pragma alloc_text (PAGE, Filter_AddRule)
#pragma alloc_text (PAGE, Filter_AddRules)
#pragma alloc_text (PAGE, Filter_AddNewRule)
#pragma alloc_text (PAGE, Filter_DeleteRule)
#pragma alloc_text (PAGE, Filter_Concat)
#pragma alloc_text (PAGE, Filter_IsHookEnabled)
#pragma alloc_text (PAGE, Filter_EnableHook)
#pragma alloc_text (PAGE, Filter_Test)
#pragma alloc_text (PAGE, Filter_EnumRules)
#pragma alloc_text (PAGE, Filter_MoveRuleUp)
#pragma alloc_text (PAGE, Filter_MoveRuleDown)
#pragma alloc_text (PAGE, Filter_CanMoveUpRule)
#pragma alloc_text (PAGE, Filter_CanMoveDownRule)
#pragma alloc_text (PAGE, Filter_Serialize)
#pragma alloc_text (PAGE, Filter_Unserialize)
#endif


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

HFILTER Filter_Create (LPCWSTR szProgram) 
{
  FILTER * p ;

  TRACE ;
  
  // assert paged memory is accessible
  PAGED_CODE() ;

  // alloc structrure
  p = MALLOC (sizeof(FILTER)) ;

  if( p==NULL )
    {
      TRACE_ERROR (TEXT("Failed to allocate FILTER structure (%u bytes)\n"), sizeof(FILTER)) ;
      return NULL ;
    }

  memset (p, 0, sizeof(FILTER)) ;

  if( szProgram )
    wcslcpy (p->szProgram, szProgram, MAX_PATH) ;
  else
    p->szProgram[0] = 0 ;
  p->bHookEnabled = TRUE ;
  p->bProtected = FALSE ;
		      
  return p ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID Filter_Destroy (HFILTER hFilter) 
{
  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  if( hFilter )
    {
      Filter_Clear (hFilter) ;
      
      FREE (hFilter) ;
    }
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID	Filter_Clear (HFILTER hFilter) 
{
  FILTER * p = hFilter ;
  UINT iReason ;
 
  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  ASSERT (hFilter!=NULL) ;
  
  for( iReason=1 ; iReason<_FILTREASON_COUNT ; iReason++ )
    Filter_ResetRules (p, iReason) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Filter_EnumRules (HFILTER hFilter, ENUMRULESCALLBACK pfnCallback, LPVOID pContext) 
{
  FILTER * p = hFilter ;
  UINT iReason ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;
  ASSERT (pfnCallback!=NULL) ;
  
  for( iReason=0 ; iReason<_FILTREASON_COUNT ; iReason++ )
    {
      FILTRULE * pRule = p->aRules[iReason] ;

      while( pRule )
	{
	  pfnCallback (pContext, pRule) ;
	  pRule = pRule->pNext ;
	}
    }

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

LPCWSTR Filter_GetProgram (HFILTER hFilter) 
{
  FILTER * p = hFilter ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  return p ? p->szProgram : NULL ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID Filter_SetProgram (HFILTER hFilter, LPCWSTR szProgram) 
{
  FILTER * p = hFilter ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;
  ASSERT (szProgram!=NULL) ;

  // set process name
  wcslcpy (p->szProgram, szProgram, MAX_PATH) ;
}

/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID Filter_SetProtected (HFILTER hFilter, BOOL bProtected) 
{
  FILTER * p = hFilter ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;

  // set "protected" flag
  p->bProtected = bProtected ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Filter_GetProtected (HFILTER hFilter) 
{
  FILTER * p = hFilter ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;

  // get "protected" flag
  return p->bProtected ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Filter_ResetRules (HFILTER hFilter, UINT nReason) 
{
  FILTER * p = hFilter ;
  FILTRULE * pRule ;

  TRACE_INFO ("nReason = %d\n", nReason) ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;
  ASSERT (p->aRules!=NULL) ;
  ASSERT (nReason<_FILTREASON_COUNT) ;

  while( (pRule=p->aRules[nReason])!=NULL )
    {
      ASSERT (pRule!=NULL) ;
      
      p->aRules[nReason] = pRule->pNext ;

      FiltRule_Clear (pRule) ;

      FREE (pRule) ; 
    }

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Filter_AddRules (HFILTER hFilter, FILTRULE * pNewRules) 
{
  FILTER	*p = hFilter ;
  FILTRULE	*pCurRule ;
  BOOL		bReasonValid ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;
  ASSERT (pNewRules!=NULL) ;

  bReasonValid = pNewRules->condition.nReason>=0 && pNewRules->condition.nReason<_FILTREASON_COUNT ;
  if( ! bReasonValid ) return FALSE ;
  
  pCurRule = p->aRules[pNewRules->condition.nReason] ;
  
  if( ! pCurRule ) {
    p->aRules[pNewRules->condition.nReason] = pNewRules ;
    return TRUE ;
  }
  
  while( pCurRule->pNext ) pCurRule = pCurRule->pNext ;

  ASSERT (pCurRule!=NULL) ;
  ASSERT (pCurRule->pNext==NULL) ;
  pCurRule->pNext = pNewRules ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Filter_AddRule (HFILTER hFilter, FILTRULE * pNewRule) 
{
  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;
  ASSERT (pNewRule!=NULL) ;
  
  pNewRule->pNext = NULL ;
  
  return Filter_AddRules (hFilter, pNewRule) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Filter_AddNewRule (HFILTER hFilter, 
			DWORD nReaction, DWORD nVerbosity, DWORD nOptions, 
			FILTREASON nReason, LPCTSTR szFormat, ...) 
{
  FILTRULE	*pNewRule ;
  va_list	va ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;

  pNewRule = MALLOC (sizeof(FILTRULE)) ;

  if( ! pNewRule ) 
    {
      TRACE_ERROR (TEXT("Failed to allocate FILTRULE structure (%u bytes)\n"), sizeof(FILTRULE)) ;
      return FALSE ;
    }

  pNewRule->nReaction	= nReaction ;
  pNewRule->nVerbosity	= nVerbosity ;
  pNewRule->nOptions	= nOptions ;
  pNewRule->pNext	= NULL ;

  va_start (va, szFormat) ;
  FiltCond_SetFV (&pNewRule->condition, nReason, szFormat, va) ;
  va_end (va) ;		  
  
  return Filter_AddRules (hFilter, pNewRule) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL	Filter_DeleteRule (HFILTER hFilter, FILTRULE *pRuleToDelete) 
{
  FILTER	*p = hFilter ;
  FILTRULE		*pRule ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;
  ASSERT (pRuleToDelete!=NULL) ;
 
  pRule = p->aRules[pRuleToDelete->condition.nReason] ;

  // is it the first rule ?
  if( pRule==pRuleToDelete )
    {
      p->aRules[pRuleToDelete->condition.nReason] = pRule->pNext ;
      FiltRule_Clear (pRuleToDelete) ;
      FREE (pRuleToDelete) ;
      return TRUE ;
    }

  // for each rule (begin)
  while( pRule )
    {
      // is it the next rule ?
      if( pRule->pNext == pRuleToDelete )
	{
	  pRule->pNext = pRule->pNext->pNext ;
	  FiltRule_Clear (pRuleToDelete) ;
	  FREE (pRuleToDelete) ;
	  return TRUE ;
	}

      // iterate list
      pRule = pRule->pNext ;
    }
  // for each rule (end)

  return FALSE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Filter_Concat (HFILTER hFilter, HFILTER hFilterToAdd) 
{
  FILTER	* p1 = (FILTER*) hFilter ;
  FILTER	* p2 = (FILTER*) hFilterToAdd ;
  int		i ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT  (hFilter!=NULL) ;
  ASSERT  (hFilterToAdd!=NULL) ;

  for( i=0 ; i<_FILTREASON_COUNT ; i++ )
    if( p2->aRules[i] ) 
      {
	TRACE ;
	Filter_AddRules (hFilter, p2->aRules[i]) ;
	ASSERT (p1->aRules[i]!=NULL) ;
	ASSERT (p1->aRules[i]==p2->aRules[i]) ;
	p2->aRules[i] = NULL ;
      }

  Filter_Destroy (hFilterToAdd) ;
   
  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Filter_Test (HFILTER hFilter, PFILTCOND pCond, 
		  DWORD *pwReaction, DWORD *pwVerbosity, DWORD * pwOptions) 
{
  FILTER * p = hFilter ;
  FILTRULE * pRule ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;
  ASSERT (pCond!=NULL) ;
  ASSERT (pCond->nReason<_FILTREASON_COUNT) ;
 
  pRule = p->aRules[pCond->nReason] ;

  while( pRule )
    {
      if( FiltCond_Check (&pRule->condition, pCond) )
	{
	  if( pwReaction )	*pwReaction	= pRule->nReaction ;
	  if( pwVerbosity )	*pwVerbosity	= pRule->nVerbosity ;
	  if( pwOptions )	*pwOptions	= pRule->nOptions ;
	  return TRUE ;
	}
      
      pRule = pRule->pNext ;
    }

  return FALSE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Filter_MoveRuleUp (HFILTER hFilter, FILTRULE *pRuleToMove) 
{
  FILTER	*p = hFilter ;
  FILTRULE		*pCurRule, *pPrevRule ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;
  ASSERT (pRuleToMove!=NULL) ;
 
  pPrevRule = NULL ;
  pCurRule = p->aRules[pRuleToMove->condition.nReason] ;

  // is it the first rule ?
  if( pCurRule==pRuleToMove ) return TRUE ;

  // for each rule (begin)
  while( pCurRule )
    {
      // is it the next rule ?
      if( pCurRule->pNext == pRuleToMove )
	{
	  pCurRule->pNext = pRuleToMove->pNext ;
	  pRuleToMove->pNext = pCurRule ;
	  
	  if( pPrevRule )
	    pPrevRule->pNext = pRuleToMove ;
	  else
	    p->aRules[pRuleToMove->condition.nReason] = pRuleToMove ;

	  return TRUE ;
	}

      // iterate list
      pPrevRule = pCurRule ;
      pCurRule = pCurRule->pNext ;      
    }
  // for each rule (end)

  return FALSE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Filter_CanMoveUpRule (HFILTER hFilter, FILTRULE *pRule) 
{
  FILTER	*p = hFilter ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;
  ASSERT (pRule!=NULL) ;
  ASSERT (pRule->condition.nReason<_FILTREASON_COUNT) ;
  
  return p->aRules[pRule->condition.nReason] != pRule ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Filter_MoveRuleDown (HFILTER hFilter, FILTRULE *pRuleToMove) 
{
  FILTER	*p = hFilter ;
  FILTRULE		*pCurRule, *pPrevRule ;
  DWORD		nReason ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;
  ASSERT (pRuleToMove!=NULL) ;

  // is it the last rule ?
  if( ! pRuleToMove->pNext ) return TRUE ;
 
  nReason = pRuleToMove->condition.nReason ;
  pCurRule = p->aRules[nReason] ;

  // is it the first rule ?
  if( pCurRule==pRuleToMove ) 
    {
      p->aRules[nReason] = pRuleToMove->pNext ;
      pRuleToMove->pNext = p->aRules[nReason]->pNext ;
      p->aRules[nReason]->pNext = pRuleToMove ;
	      
      return TRUE ;
    }

  // for each rule (begin)
  while( pCurRule )
    {
      // is it this rule ?
      if( pCurRule == pRuleToMove )
	{
	  pPrevRule->pNext = pRuleToMove->pNext ;
	  pRuleToMove->pNext = pRuleToMove->pNext->pNext ;
	  pPrevRule->pNext->pNext = pRuleToMove ;
	  
	  return TRUE ;
	}

      // iterate list
      pPrevRule = pCurRule ;
      pCurRule = pCurRule->pNext ;      
    }
  // for each rule (end)

  return FALSE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL Filter_CanMoveDownRule (HFILTER hFilter, FILTRULE *pRule) 
{
  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;
  ASSERT (pRule!=NULL) ;
  ASSERT (pRule->condition.nReason<_FILTREASON_COUNT) ;
  
  return pRule->pNext!=NULL ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL	Filter_IsHookEnabled (HFILTER hFilter) 
{
  FILTER	*p = hFilter ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;

  return p->bHookEnabled ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

VOID	Filter_EnableHook (HFILTER hFilter, BOOL bEnable) 
{
  FILTER	*p = hFilter ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  // verify params
  ASSERT (hFilter!=NULL) ;
  
  p->bHookEnabled = bEnable ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT Filter_Serialize (HFILTER hFilter, PVOID pBuffer, UINT nMaxSize)
{
  PSERIALBUFFER	pSerial = pBuffer ;
  FILTER	*pFilter = hFilter ;
  BYTE *	pWritePtr ;
  UINT		nProgPathSize ;
  UINT		nTotalSize ;
  UINT		nRules ;  
  UINT		iReason ;
  UINT		nRuleSize ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;
  
  ASSERT (hFilter!=NULL) ;
  ASSERT (pBuffer!=NULL) ;
  ASSERT (nMaxSize>0) ;

  nTotalSize = sizeof(SERIALHEADER) ;
  pWritePtr = pSerial->data ;
  
  if( nTotalSize>nMaxSize ) {
    TRACE_ERROR (TEXT("Buffer too small to store header.\n")) ;
    return 0 ;
  }

  nProgPathSize = ( wcslen(pFilter->szProgram) + 1 ) * sizeof(WCHAR) ;
  nTotalSize += nProgPathSize ;

  if( nTotalSize>nMaxSize ) {
    TRACE_ERROR (TEXT("Buffer too small to store program path.\n")) ;
    return 0 ;
  }

  memcpy (pWritePtr, pFilter->szProgram, nProgPathSize) ;
  pWritePtr += nProgPathSize ;

  nRules = 0 ;

  for( iReason=0 ; iReason<_FILTREASON_COUNT ; iReason++ )
    {
      PCFILTRULE pCurRule ;

      for( pCurRule=pFilter->aRules[iReason] ; pCurRule ; pCurRule=pCurRule->pNext )
	{
	  nRules++ ;

	  nRuleSize = FiltRule_Serialize (pCurRule, pWritePtr, nMaxSize-nTotalSize) ;

	  if( ! nRuleSize ) {
	    TRACE_ERROR (TEXT("FiltRule_Serialize failed\n")) ;
	    return 0 ;
	  }
	  
	  nTotalSize += nRuleSize ;
	  pWritePtr += nRuleSize ;

	  ASSERT (nTotalSize<=nMaxSize) ;
	}
    }

  // fill header
  pSerial->header.nSize		= nTotalSize ;
  pSerial->header.nProgPathSize	= nProgPathSize ;
  pSerial->header.nRules	= nRules ;
  pSerial->header.bHookEnabled	= pFilter->bHookEnabled ;
  pSerial->header.bProtected	= pFilter->bProtected ;

  TRACE_INFO (TEXT("Filter size = %u\n"), nTotalSize) ;

  ASSERT (nTotalSize>0) ;
  return nTotalSize ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT Filter_Unserialize (HFILTER hFilter, PCVOID pBuffer, UINT nMaxSize)
{
  PCSERIALBUFFER pSerial = pBuffer ;
  FILTER	*pFilter = hFilter ;
  const BYTE	*pReadPtr ;
  UINT		nTotalSize ;
  INT		nRemainSize ;
  INT		nRemainRules ;

  TRACE ;

  // assert paged memory is accessible
  PAGED_CODE() ;

  ASSERT (hFilter!=NULL) ;
  ASSERT (pBuffer!=NULL) ;
  ASSERT (nMaxSize>0) ;

  // is buffer big enough ?
  if( nMaxSize < sizeof(SERIALHEADER) ) {
    TRACE_ERROR (TEXT("Buffer smaller than header.\n")) ;
    return 0 ;
  }

  TRACE_INFO (TEXT("Filter size = %u\n"), pSerial->header.nSize) ;

  // is buffer big enough ?
  if( nMaxSize < pSerial->header.nSize ) {
    TRACE_ERROR (TEXT("Buffer is too small (size=%d,needed=%d)\n"),
		 nMaxSize, pSerial->header.nSize) ;
    return 0 ;
  }

  // is prog path size correct ?
  if( pSerial->header.nProgPathSize/2 >= MAX_PATH ) {
    TRACE_ERROR (TEXT("Program path too long\n")) ;
    return 0 ;
  }

  // read header 
  nTotalSize	= pSerial->header.nSize ;
  nRemainSize	= nTotalSize - sizeof(SERIALHEADER) ;
  nRemainRules	= pSerial->header.nRules ;
  pFilter->bHookEnabled	= pSerial->header.bHookEnabled ;
  pFilter->bProtected	= pSerial->header.bProtected ;
  pReadPtr	= pSerial->data ;

  // verify remaining size
  if( nRemainSize < pSerial->header.nProgPathSize ) {
    TRACE_ERROR (TEXT("Buffer too small to contain program path\n")) ;
    return 0 ;
  }

  // read prog path
  memcpy (pFilter->szProgram, pReadPtr, pSerial->header.nProgPathSize) ;
  pFilter->szProgram[MAX_PATH-1] = 0 ;
  pReadPtr += pSerial->header.nProgPathSize ;
  nRemainSize -= pSerial->header.nProgPathSize ;  

  // clear rules list
  //memset (pFilter->aRules, 0, sizeof(PFILTRULE)*_FILTREASON_COUNT) ;

  // ---------------- for each rule (begin) ----------------
  while( nRemainRules )
    {
      UINT	nRuleSize ;
      PFILTRULE pRule = MALLOC(sizeof(FILTRULE)) ;

      if( pRule == NULL )
	{
	  TRACE_ERROR (TEXT("Failed to allocate FILTRULE structure (%u bytes)\n"), sizeof(FILTRULE)) ;
	  return 0 ;
	}

      nRuleSize = FiltRule_Unserialize (pRule, pReadPtr, nRemainSize) ;
   
      if( ! nRuleSize ) {
	TRACE_ERROR (TEXT("FiltRule_Unserialize failed\n")) ;	
	return 0 ;
      }

      pReadPtr += nRuleSize ;
      nRemainSize -= nRuleSize ;

      Filter_AddRule (hFilter, pRule) ;

      nRemainRules-- ;

      ASSERT (nRemainSize>=0) ;
    }
  // ---------------- for each rule (end) ----------------

  if( nRemainSize!=0 )
    TRACE_WARNING (TEXT("Extra bytes ignored (size=%d)\n"), nRemainSize) ;

  if( pFilter->aRules[FILTREASON_UNDEFINED]!=NULL )
    TRACE_WARNING(TEXT("Filter has rules with undefined reason\n")) ;
  
  return nTotalSize ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL	Filter_Match (HFILTER hFilter, LPCWSTR szPath) 
{
  LPCWSTR szProgram ;

  // verify params
  ASSERT (hFilter!=NULL) ;
  ASSERT (szPath!=NULL) ;

  szProgram = Filter_GetProgram(hFilter) ;
  ASSERT (szProgram!=NULL) ;

  return Wildcards_CompleteStringCmp (szProgram, szPath) ;
}
