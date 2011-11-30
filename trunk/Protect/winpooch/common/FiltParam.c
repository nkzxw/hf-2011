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
#include "FiltParam.h"

// project's headers
#include "Assert.h"
#include "Malloc.h"
#include "Trace.h"


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  UINT		nSize ;
  UINT		nType ;
} SERIALHEADER ;

typedef struct {
  SERIALHEADER	header ;
  BYTE		data[1] ;
} SERIALBUFFER ;

typedef SERIALBUFFER*		PSERIALBUFFER ;
typedef const SERIALBUFFER*	PCSERIALBUFFER ;


/******************************************************************/
/* Code sections pragmas                                          */
/******************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, FiltParam_SetAsAny)
#pragma alloc_text (PAGE, FiltParam_SetAsUint)
#pragma alloc_text (PAGE, FiltParam_SetAsString)
#pragma alloc_text (PAGE, FiltParam_Clear)
#pragma alloc_text (PAGE, FiltParam_Dup)
#pragma alloc_text (PAGE, FiltParam_Serialize)
#pragma alloc_text (PAGE, FiltParam_Unserialize)
#endif


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL FiltParam_SetAsAny (PFILTPARAM pParam) 
{
  //TRACE ;

  ASSERT (pParam!=NULL) ;

  pParam->nType = FILTPARAM_ANY ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL FiltParam_SetAsUint (PFILTPARAM pParam, UINT nValue) 
{
  //TRACE ;

  ASSERT (pParam!=NULL) ;

  pParam->nType = FILTPARAM_UINT ;
  pParam->nValue = nValue ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL FiltParam_SetAsString (PFILTPARAM pParam, UINT nType, LPCWSTR szValue) 
{
  UINT	nSize ;

  //TRACE ;

  ASSERT (pParam!=NULL) ;


  if( szValue==NULL )
    szValue = L"(null)" ;

  nSize = ( wcslen (szValue) + 1 ) * sizeof(WCHAR) ;

  pParam->nType = nType ;
  pParam->szValue = MALLOC(nSize) ;

  // alloc failed ?
  if( ! pParam->szValue )
    { 
      TRACE_ERROR (TEXT("Memory allocation failed (%u bytes)\n"), nSize) ;
      return FALSE ;
    }

  memcpy (pParam->szValue, szValue, nSize) ;

  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL FiltParam_Clear (PFILTPARAM pParam) 
{
  TRACE ;

  ASSERT (pParam!=NULL) ;
  
  if( pParam->nType==FILTPARAM_STRING ||
      pParam->nType==FILTPARAM_WILDCARDS ||
      pParam->nType==FILTPARAM_PATH )
    FREE (pParam->szValue) ;
  
  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

BOOL FiltParam_Dup (PFILTPARAM pDst, PCFILTPARAM pSrc) 
{
  TRACE ;

  ASSERT (pDst!=NULL) ;
  ASSERT (pSrc!=NULL) ;    

  if( pSrc->nType==FILTPARAM_STRING ||
      pSrc->nType==FILTPARAM_WILDCARDS ||
      pSrc->nType==FILTPARAM_PATH )
    return FiltParam_SetAsString (pDst, pSrc->nType, pSrc->szValue) ;
  
  memcpy (pDst, pSrc, sizeof(FILTPARAM)) ;  
  return TRUE ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT FiltParam_Serialize (PCFILTPARAM pParam, PVOID pBuf, UINT nBufSize) 
{
  PSERIALBUFFER	pSerial = pBuf ;
  UINT	nValueSize ;
  UINT	nTotalSize ;

  TRACE ;

  ASSERT (pParam!=NULL) ;
  ASSERT (pBuf!=NULL) ;
  ASSERT (nBufSize>0) ;

  switch( pParam->nType )
    {
    case FILTPARAM_UINT:
      nValueSize = sizeof(UINT) ;
      break ;
    case FILTPARAM_STRING:
    case FILTPARAM_WILDCARDS:
    case FILTPARAM_PATH:
      nValueSize = ( wcslen (pParam->szValue) + 1 ) * sizeof(WCHAR) ;
      break ;
    default:
      nValueSize = 0 ;
    } ;

  nTotalSize = sizeof(SERIALHEADER) + nValueSize ;

  if( nTotalSize > nBufSize ) {
    TRACE_ERROR (TEXT("Buffer too small.\n")) ;
    return 0 ;
  }

  pSerial->header.nSize = nTotalSize ;
  pSerial->header.nType = pParam->nType ;
  
  switch( pParam->nType )
    {
    case FILTPARAM_UINT:
      memcpy (pSerial->data, &pParam->nValue, nValueSize) ;
      break ;
    case FILTPARAM_STRING:
    case FILTPARAM_WILDCARDS:
    case FILTPARAM_PATH:
      if( pParam->szValue[nValueSize/2-1]!=0 ) {
	TRACE_WARNING (TEXT("Null terminator is missing.\n")) ;
	pParam->szValue[nValueSize/2-1] = 0 ; 
	TRACE_WARNING (TEXT("--> %ls\n"), pParam->szValue) ;
      }
      memcpy (pSerial->data, pParam->szValue, nValueSize) ;
      break ;
    } ;

  TRACE_INFO (TEXT("Param size = %u\n"), nTotalSize) ;

  return nTotalSize ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT FiltParam_Unserialize (PFILTPARAM pParam, PCVOID pBuf, UINT nBufSize) 
{
  PCSERIALBUFFER pSerial = pBuf ;
  UINT	nValueSize ;
  UINT	nTotalSize ;

  TRACE ;

  ASSERT (pParam!=NULL) ;
  ASSERT (pBuf!=NULL) ;
  ASSERT (nBufSize>0) ;

  // is buffer big enough ?
  if( nBufSize < sizeof(SERIALHEADER) ) {
    TRACE_ERROR (TEXT("Buffer smaller than header.\n")) ;
    return 0 ;
  }

  TRACE_INFO (TEXT("Param size = %u\n"), pSerial->header.nSize) ;

  // read header 
  nTotalSize = pSerial->header.nSize ;
  nValueSize = nTotalSize - sizeof(SERIALHEADER) ;
  pParam->nType = pSerial->header.nType ;

  // is size valid ?
  switch( pParam->nType )
    {

    case FILTPARAM_ANY:

      if( nValueSize!=0 ) {
	TRACE_ERROR (TEXT("Invalid size for 'any'.\n")) ;
	return 0 ;
      }

      TRACE_INFO (TEXT("----------> *\n")) ;

      break ;    

    case FILTPARAM_UINT:

      if( nValueSize!=sizeof(UINT) ) {
	TRACE_ERROR (TEXT("Invalid size for 'uint'.\n")) ;
	return 0 ;
      }
      
      memcpy (&pParam->nValue, pSerial->data, nValueSize) ;

      TRACE_INFO (TEXT("----------> %u\n"), pParam->nValue) ;

      break ;

    case FILTPARAM_STRING:
    case FILTPARAM_WILDCARDS:
    case FILTPARAM_PATH:

      if( nValueSize < sizeof(WCHAR) ) {
	TRACE_ERROR (TEXT("Invalid size for string.\n")) ;
	return 0 ;
      }
      
      pParam->szValue = MALLOC(nValueSize) ;

      if( ! pParam->szValue )
	{ 
	  TRACE_ERROR (TEXT("Memory allocation failed (%u bytes)\n"), nValueSize) ;
	  return FALSE ;
	}

      memcpy (pParam->szValue, pSerial->data, nValueSize) ;
      
      if( pParam->szValue[nValueSize/2-1]!=0 ) {
	TRACE_WARNING (TEXT("Null terminator is missing.\n")) ;
	pParam->szValue[nValueSize/2-1] = 0 ;
	TRACE_WARNING (TEXT("--> %ls\n"), pParam->szValue) ;
      }

      TRACE_INFO (TEXT("----------> %ls\n"), pParam->szValue) ;

      break ;

    default:
      
      TRACE_ERROR (TEXT("Invalid param type (type=%u)\n"), pParam->nType) ;

      return 0 ;
    } ;

  return nTotalSize ;
}
