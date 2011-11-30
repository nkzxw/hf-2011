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
#include "FiltRule.h"

// standard headers
#include <stdio.h>
#include <tchar.h>

// project's headers
#include "Assert.h"
#include "Trace.h"


/******************************************************************/
/* Internal data types                                            */
/******************************************************************/

typedef struct {
  UINT		nSize ;
  UINT		nReaction ;
  UINT		nVerbosity ;
  UINT		nOptions ;
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

VOID FiltRule_Clear (PFILTRULE pRule) 
{
  //TRACE ;

  ASSERT (pRule!=NULL) ;

  FiltCond_Clear (&pRule->condition) ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT	FiltRule_Serialize	(PCFILTRULE	pRule, 
				 LPVOID		pBuffer, 
				 UINT		nMaxSize)
{
  PSERIALBUFFER	pSerial = pBuffer ;
  UINT	nTotalSize ;
  UINT	nCondSize ;

  TRACE ;
  
  ASSERT (pRule!=NULL) ;
  ASSERT (pBuffer!=NULL) ;
  ASSERT (nMaxSize>0) ;

  if( sizeof(SERIALHEADER)>nMaxSize ) {
    TRACE_ERROR (TEXT("Buffer too small to store header.\n")) ;
    return 0 ;
  }

  nCondSize = FiltCond_Serialize (&pRule->condition,
				  pSerial->data, 
				  nMaxSize-sizeof(SERIALHEADER)) ;

  if( ! nCondSize ) {
    TRACE_ERROR (TEXT("FiltCond_Serialize failed\n")) ;
    return 0 ;
  }
  
  nTotalSize = sizeof(SERIALHEADER) + nCondSize ;

  pSerial->header.nSize		= nTotalSize ;
  pSerial->header.nReaction	= pRule->nReaction ;
  pSerial->header.nVerbosity	= pRule->nVerbosity ;
  pSerial->header.nOptions	= pRule->nOptions ;

  TRACE_INFO (TEXT("Rule size = %u\n"), nTotalSize) ;

  ASSERT (nTotalSize>0) ;
  return nTotalSize ;
}


/******************************************************************/
/* Exported function                                              */
/******************************************************************/

UINT	FiltRule_Unserialize	(PFILTRULE	pRule, 
				 LPCVOID	pBuffer, 
				 UINT		nMaxSize)
{
  PCSERIALBUFFER pSerial = pBuffer ;
  UINT	nCondSize ;
  UINT	nTotalSize ;

  TRACE ;

  ASSERT (pRule!=NULL) ;
  ASSERT (pBuffer!=NULL) ;
  ASSERT (nMaxSize>0) ;

  // is buffer big enough ?
  if( nMaxSize <= sizeof(SERIALHEADER) ) {
    TRACE_ERROR (TEXT("Buffer smaller than header.\n")) ;
    return 0 ;
  }  

  TRACE_INFO (TEXT("Rule size = %u\n"), pSerial->header.nSize) ;

  // is buffer big enough ?
  if( nMaxSize < pSerial->header.nSize ) {
    TRACE_ERROR (TEXT("Buffer is too small (size=%d,needed=%d)\n"),
		 nMaxSize, pSerial->header.nSize) ;
    return 0 ;
  }

  // read header 
  nTotalSize		= pSerial->header.nSize ;
  pRule->nReaction	= pSerial->header.nReaction ;
  pRule->nVerbosity	= pSerial->header.nVerbosity ;
  pRule->nOptions	= pSerial->header.nOptions ;
  pRule->pNext		= NULL ;
  
  nCondSize = FiltCond_Unserialize (&pRule->condition,
				    pSerial->data, 
				    nTotalSize-sizeof(SERIALHEADER)) ;
  
  if( ! nCondSize ) {
    TRACE_ERROR (TEXT("FiltCond_Unserialize failed\n")) ;
    return 0 ;
  }

  if( nTotalSize!=nCondSize+sizeof(SERIALHEADER) )
    TRACE_WARNING (TEXT("Extra data ignored (size=%d)\n"), 
		   nTotalSize-nCondSize-sizeof(SERIALHEADER)) ;
  
  return nTotalSize ;
}
