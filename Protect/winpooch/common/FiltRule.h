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

#ifndef _RULE_H
#define _RULE_H

#include "FiltCond.h"

// reactions
#define RULE_ACCEPT		0
#define RULE_FEIGN		1
#define RULE_REJECT		2
#define RULE_KILLPROCESS	3

// verbosity
#define RULE_SILENT		0
#define RULE_LOG		1
#define RULE_ALERT		2

// options
#define RULE_ASK		1
#define RULE_SCAN		2


typedef struct FILTRULE {
  UINT		nReaction ;
  UINT		nVerbosity ;
  UINT		nOptions ;
  FILTCOND	condition ;
  struct FILTRULE * pNext ; //< for filter internal use
} FILTRULE ;

typedef	FILTRULE*	PFILTRULE ;
typedef const FILTRULE*	PCFILTRULE ;

VOID	FiltRule_Clear		(PFILTRULE pRule) ;

UINT	FiltRule_Serialize	(PCFILTRULE pRule, PVOID pBuf, UINT nBufSize) ;

UINT	FiltRule_Unserialize	(PFILTRULE pRule, PCVOID pBuf, UINT nBufSize) ;


#endif
