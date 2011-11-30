/******************************************************************/
/*                                                                */
/*  Winpooch : Windows Watchdog                                   */
/*  Copyright (C) 2004-2005  Benoit Blanchon                      */
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

#ifndef _FILTPARAM_H
#define _FILTPARAM_H

#include "Types.h"

#define FILTPARAM_ANY		0
#define FILTPARAM_UINT		1
#define FILTPARAM_STRING	2
#define FILTPARAM_WILDCARDS	3
#define FILTPARAM_PATH		4

typedef struct
{
  UINT	nType ;

  union 
  {
    LPWSTR	szValue ;
    UINT	nValue ;		
  } ;
    
} FILTPARAM ;

typedef FILTPARAM*		PFILTPARAM ;
typedef const FILTPARAM*	PCFILTPARAM ;


BOOL FiltParam_SetAsAny		(PFILTPARAM pParam) ;

BOOL FiltParam_SetAsUint	(PFILTPARAM pParam, UINT nValue) ;

BOOL FiltParam_SetAsString	(PFILTPARAM pParam, UINT nType, LPCWSTR szValue) ;

BOOL FiltParam_Clear		(PFILTPARAM pParam) ;

BOOL FiltParam_Dup		(PFILTPARAM pDst, PCFILTPARAM pSrc) ;

UINT FiltParam_Serialize	(PCFILTPARAM pParam, PVOID pBuf, UINT nBufSize) ;

UINT FiltParam_Unserialize	(PFILTPARAM pParam, PCVOID pBuf, UINT nBufSize) ;

#endif
