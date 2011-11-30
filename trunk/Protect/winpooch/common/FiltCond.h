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


#ifndef _CONDITION_H
#define _CONDITION_H

#include "Types.h"
#include "FiltReason.h"
#include "FiltParam.h"

// maximum number of parameters
#define MAX_PARAMS	4


typedef struct
{
  FILTREASON	nReason ;
  UINT		nParams ;
  FILTPARAM	aParams[MAX_PARAMS] ; 
} FILTCOND ;

typedef FILTCOND*	PFILTCOND ;
typedef const FILTCOND*	PCFILTCOND ;

// Condition format string :
// s = string
// n = uint
// w = wildcards 
// p = path
// * = any

BOOL	FiltCond_SetF	(PFILTCOND	pCondition, 
			 FILTREASON	nReason, 
			 LPCTSTR	szFormat, ...) ;

BOOL	FiltCond_SetFV	(PFILTCOND	pCondition, 
			 FILTREASON	nReason,
			 LPCTSTR	szFormat, 
			 va_list	vargs) ;

BOOL	FiltCond_Clear	(PFILTCOND	pCondition) ;

BOOL	FiltCond_Dup	(PFILTCOND	pCondDst,
			 PCFILTCOND	pCondSrc) ;

BOOL	FiltCond_Check	(PCFILTCOND	pCondToMatch, 
			 PCFILTCOND	pCondToTest) ;

UINT	FiltCond_Serialize	(PCFILTCOND	pCondition, 
				 LPVOID		pBuffer, 
				 UINT		nMaxSize) ;

UINT	FiltCond_Unserialize	(PFILTCOND	pCondition, 
				 LPCVOID	pBuffer, 
				 UINT		nMaxSize) ;


#endif
