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

#ifndef _FILTERTOOLS_H
#define _FILTERTOOLS_H

#ifdef __NTDDK__
#error This is an user-mode only module.
#endif

#include "FiltCond.h"
#include "FiltReason.h"
#include "FiltRule.h"

LPCTSTR		FiltReason_GetName		(FILTREASON nReason) ;

FILTREASON	FiltReason_GetId		(LPCTSTR szReason) ;

UINT		FiltReason_GetParamCount	(FILTREASON nReason) ;

LPCTSTR		FiltReason_GetParamName		(FILTREASON nReason, UINT nParam) ;

UINT		FiltReason_GetOptionMask	(FILTREASON nReason) ;


UINT		FiltCond_GetParamAsString	(PCFILTCOND	pCond, 
						 UINT		nParamNum, 
						 LPTSTR		szBuffer, 
						 UINT		nBufSize) ;

UINT		FiltCond_GetParamCount		(PCFILTCOND	pCondition) ;

LPCTSTR		FiltCond_GetParamString		(PCFILTCOND	pCondition,
					 UINT		nParamNum) ;

UINT	FiltCond_GetParamUint		(PCFILTCOND	pCondition, 
					 UINT		nParamNum) ;

UINT	FiltCond_GetParamType		(PCFILTCOND	pCondition, 
					 UINT		pParamNum) ;

UINT	FiltCond_GetReasonAsString	(PCFILTCOND	pCondition, 
					 LPTSTR		szBuffer, 
					 UINT		nBufSize) ;

UINT	FiltCond_ToString		(PCFILTCOND	pCondition, 
					 LPTSTR		szBuffer, 
					 UINT		nBufSize) ;

UINT	FiltRule_GetReactionString	(PCFILTRULE	pRule, 
					 LPTSTR		szBuffer,
					 UINT		nBufSize) ;

UINT	FiltRule_GetVerbosityString	(PCFILTRULE	pRule,
					 LPTSTR		szBuffer,
					 UINT		nBufSize) ;

UINT	FiltRule_GetOptionsString	(PCFILTRULE	pRule,
					 LPTSTR		szBuffer,
					 UINT		nBufSize) ;

#endif
