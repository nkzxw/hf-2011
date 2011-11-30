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
#include "FiltCond.h"

// standards headers
#include <windows.h>
#include <shlwapi.h>
#include <tchar.h>

// project's headers
#include "Assert.h"
#include "FiltReason.h"
#include "Strlcpy.h"
#include "Trace.h"
#include "Wildcards.h"


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  UINT		nSize ;
  UINT		nReason ;
  UINT		nParams ;
} SERIALHEADER ;

typedef struct {
  SERIALHEADER	header ;
  BYTE		data[1] ;
} SERIALBUFFER ;

typedef SERIALBUFFER*		PSERIALBUFFER ;
typedef const SERIALBUFFER*	PCSERIALBUFFER ;


/******************************************************************/
/* Internal functions                                             */
/******************************************************************/

#ifdef __NTDDK__

int wcsicmp (const wchar_t*, const wchar_t*) ;

#endif


/******************************************************************/
/* Code sections pragmas                                          */
/******************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, FiltCond_SetF)
#pragma alloc_text (PAGE, FiltCond_SetFV)
#pragma alloc_text (PAGE, FiltCond_Dup)
#pragma alloc_text (PAGE, FiltCond_Check)
#pragma alloc_text (PAGE, FiltCond_Serialize)
#pragma alloc_text (PAGE, FiltCond_Unserialize)
#pragma alloc_text (PAGE, wcsicmp)
#endif



/******************************************************************/
/* Internal function                                              */
/******************************************************************/

#ifdef __NTDDK__

int wcsicmp (const wchar_t* szString1, const wchar_t* szString2) 
{
  UNICODE_STRING ustrString1, ustrString2 ;

  RtlInitUnicodeString (&ustrString1, szString1) ;
  RtlInitUnicodeString (&ustrString2, szString2) ;
  
  return RtlCompareUnicodeString (&ustrString1, &ustrString2, TRUE) ;
}

#endif


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL FiltCond_SetFV (PFILTCOND	pCond, 
		     FILTREASON	nReason, 
		     LPCTSTR	szFormat, 
		     va_list	va) 
{
  UINT iParam ;

  //TRACE ;

  // verify params
  ASSERT (pCond!=NULL) ;
  ASSERT (_tcslen(szFormat)<MAX_PARAMS) ;

  pCond->nReason = nReason ;
  pCond->nParams = _tcslen (szFormat) ;

  // for each param (begin)
  for( iParam=0 ; iParam<pCond->nParams ; iParam++ )
    {
      FILTPARAM * pCurParam = &pCond->aParams[iParam] ;
      BOOL bSuccess ;

      switch( szFormat[iParam] )
	{
	case 'n':
	  bSuccess = FiltParam_SetAsUint (pCurParam, va_arg(va,UINT)) ;
	  break ;

	case 's':
	  bSuccess = FiltParam_SetAsString (pCurParam, FILTPARAM_STRING, va_arg(va,LPWSTR)) ;
	  break ;

	case 'w':
	  bSuccess = FiltParam_SetAsString (pCurParam, FILTPARAM_WILDCARDS, va_arg(va,LPWSTR)) ;
	  break ;

	case 'p':
	  bSuccess = FiltParam_SetAsString (pCurParam, FILTPARAM_PATH, va_arg(va,LPWSTR)) ;
	  break ;
	  
	case '*':
	  bSuccess = FiltParam_SetAsAny (pCurParam) ;
	  break ;

	default:
	  ASSERT (!"Invalid condition format") ;
	}

      if( ! bSuccess )
	{
	  UINT i ;

	  TRACE_INFO (TEXT("Failed to create param %d/%d (format=%c)\n"), 
		      iParam, pCond->nParams, szFormat[iParam]) ;

	  for( i=0 ; i<iParam ; i++ )
	    FiltParam_Clear (&pCond->aParams[iParam]) ;	  

	  return FALSE ;
	}
    }
  // for each param (end)

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL	FiltCond_Clear	(PFILTCOND	pCondition) 
{
  int i ;

  TRACE ;

  ASSERT (pCondition!=NULL) ;
  ASSERT (pCondition->nParams<=MAX_PARAMS) ;

  for( i=0 ; i<pCondition->nParams ; i++ )
    FiltParam_Clear (&pCondition->aParams[i]) ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL FiltCond_Dup (PFILTCOND	pCondDst,
		   PCFILTCOND	pCondSrc)
{
  int i ;

  TRACE ;

  ASSERT (pCondSrc!=NULL) ;
  ASSERT (pCondDst!=NULL) ;
  
  // copy
  memcpy (pCondDst, pCondSrc, sizeof(FILTCOND)) ;

  // copy each param
  for( i=0 ; i<pCondSrc->nParams ; i++ )
    {
      FiltParam_Dup (&pCondDst->aParams[i],
		     &pCondSrc->aParams[i]) ;
    }

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL	FiltCond_Check	(PCFILTCOND	pCondToMatch, 
			 PCFILTCOND	pCondToTest)
{
  UINT			iParam, nParams ;
  BOOL			bParamMatch ;

  TRACE ;

  // verify params
  ASSERT (pCondToMatch!=NULL) ;
  ASSERT (pCondToTest!=NULL) ;

  // check if reasons are equals
  if( pCondToMatch->nReason != pCondToTest->nReason )
    return FALSE ;

  // check if params count are equals
  if( pCondToMatch->nParams != pCondToTest->nParams )
    {
      TRACE_WARNING (TEXT("Wrong number of parameters (%d!=%d) while checking reason %d\n"), 
		     pCondToMatch->nParams, pCondToTest->nParams, pCondToMatch->nReason) ; 
      return FALSE ;
    }

  nParams = pCondToMatch->nParams ;

  // for each param (begin)
  for( iParam=0 ; iParam<nParams ; iParam++ )
    {
      PCFILTPARAM pParamToMatch, pParamToTest ;
      
      pParamToMatch = &pCondToMatch->aParams[iParam] ;
      pParamToTest = &pCondToTest->aParams[iParam] ;

      switch( pParamToMatch->nType )
	{
	case FILTPARAM_ANY:	  
	  bParamMatch = TRUE ;
	  break ;
	  
	case FILTPARAM_UINT:
	  if( pParamToTest->nType == pParamToMatch->nType )
	    bParamMatch = pParamToMatch->nValue == pParamToTest->nValue ;	    
	  else bParamMatch = FALSE ;
	  break ;
	  
	case FILTPARAM_STRING:
	  if( pParamToTest->nType == pParamToMatch->nType )
	    bParamMatch = !wcsicmp(pParamToMatch->szValue, pParamToTest->szValue) ;
	  else bParamMatch = FALSE ;
	  break ;

	case FILTPARAM_WILDCARDS:
	  switch( pParamToTest->nType )
	    {
	    case FILTPARAM_STRING:
	      ASSERT (pParamToMatch->szValue!=NULL) ;
	      ASSERT (pParamToTest->szValue!=NULL) ;
	      bParamMatch = Wildcards_CompleteStringCmp(pParamToMatch->szValue, 
							pParamToTest->szValue) ;
	      break ;
	    case FILTPARAM_WILDCARDS:
	    case FILTPARAM_PATH:
	      ASSERT (pParamToMatch->szValue!=NULL) ;
	      ASSERT (pParamToTest->szValue!=NULL) ;
	      bParamMatch = !wcsicmp(pParamToMatch->szValue, pParamToTest->szValue) ;
	      break ;
	    default:
	      bParamMatch = FALSE ;
	    }
	  break ;

	case FILTPARAM_PATH:
	  switch( pParamToTest->nType )
	    {
	    case FILTPARAM_STRING:
	      ASSERT (pParamToMatch->szValue!=NULL) ;
	      ASSERT (pParamToTest->szValue!=NULL) ;
	      bParamMatch = Wildcards_CompletePathCmp(pParamToMatch->szValue, 
						      pParamToTest->szValue) ;
	      break ;
	    case FILTPARAM_WILDCARDS:
	    case FILTPARAM_PATH:
	      ASSERT (pParamToMatch->szValue!=NULL) ;
	      ASSERT (pParamToTest->szValue!=NULL) ;
	      bParamMatch = !wcsicmp(pParamToMatch->szValue, 
				     pParamToTest->szValue) ;
	      break ;
	    default:
	      bParamMatch = FALSE ;
	    }
	  break ;

	default:
	  ASSERT (!"[Invalid param type]") ;
	}

      // stop as soon as one param doesn't match
      if( ! bParamMatch ) return FALSE ;
    }
  // for each param (end)

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT	FiltCond_Serialize	(PCFILTCOND	pCondition, 
				 LPVOID		pBuffer, 
				 UINT		nMaxSize)
{
  PSERIALBUFFER	pSerial = pBuffer ;
  UINT	iParam ;
  UINT	nTotalSize ;
  UINT	nParamSize ;  
  BYTE *pWritePtr ;

  TRACE ;

  ASSERT (pCondition!=NULL) ;
  ASSERT (pBuffer!=NULL) ;
  ASSERT (nMaxSize>0) ;

  nTotalSize = sizeof(SERIALHEADER) ;

  if( nTotalSize>nMaxSize ) {
    TRACE_ERROR (TEXT("Buffer too small to store header (bufsize=%u)\n"), nMaxSize) ;
    return 0 ;
  }


  pWritePtr = pSerial->data ;

  for( iParam=0 ; iParam<pCondition->nParams ; iParam++ )
    {
      nParamSize = FiltParam_Serialize (&pCondition->aParams[iParam],
					pWritePtr, 
					nMaxSize-nTotalSize) ;
      
      if( ! nParamSize ) {
	TRACE_ERROR (TEXT("FiltParam_Serialize failed\n")) ;
	return 0 ;
      }

      nTotalSize += nParamSize ;
      pWritePtr += nParamSize ;

      ASSERT (nTotalSize<=nMaxSize) ;
    } 
  
  pSerial->header.nSize		= nTotalSize ;
  pSerial->header.nReason	= pCondition->nReason ;
  pSerial->header.nParams	= pCondition->nParams ;

  TRACE_INFO (TEXT("Condition size = %u\n"), nTotalSize) ;

  ASSERT (nTotalSize>0) ;
  return nTotalSize ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT	FiltCond_Unserialize	(PFILTCOND	pCondition, 
				 LPCVOID       	pBuffer, 
				 UINT		nMaxSize)
{
  PCSERIALBUFFER pSerial = pBuffer ;
  const BYTE	*pReadPtr ;
  UINT	iParam ;
  UINT	nParamSize ;
  UINT	nTotalSize ;
  INT	nRemainBytes ;

  TRACE ;

  ASSERT (pCondition!=NULL) ;
  ASSERT (pBuffer!=NULL) ;
  ASSERT (nMaxSize>0) ;

  // is buffer big enough ?
  if( nMaxSize < sizeof(SERIALHEADER) ) {
    TRACE_ERROR (TEXT("Buffer smaller than header.\n")) ;
    return 0 ;
  }  

  TRACE_INFO (TEXT("Condition size = %u\n"), pSerial->header.nSize) ;

  // read header 
  nTotalSize = pSerial->header.nSize ;
  pCondition->nReason = pSerial->header.nReason ;
  pCondition->nParams = pSerial->header.nParams ;

  nRemainBytes = nTotalSize - sizeof(SERIALHEADER) ;
  pReadPtr = pSerial->data ;

  for( iParam=0 ; iParam<pCondition->nParams ; iParam++ )
    {
      nParamSize = FiltParam_Unserialize (&pCondition->aParams[iParam],
					  pReadPtr, nRemainBytes) ;
      
      nParamSize = *(UINT*) pReadPtr ;
      
      if( ! nParamSize ) {
	TRACE_ERROR (TEXT("FiltParam_Unserialize failed\n")) ;
	return 0 ;
      }

      nRemainBytes -= nParamSize ;
      pReadPtr += nParamSize ;

      ASSERT (nRemainBytes>=0) ;
    }

  if( nRemainBytes!=0 )
    TRACE_WARNING (TEXT("Extra bytes ignored (size=%d)\n"), nRemainBytes) ;

  return nTotalSize ;
}
