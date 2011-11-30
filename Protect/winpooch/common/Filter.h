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

#ifndef _FILTER_H
#define _FILTER_H

#include "Types.h"
#include "FiltRule.h"


typedef void* HFILTER ;

typedef VOID (*ENUMRULESCALLBACK)(LPVOID pContext, FILTRULE * pRule) ;


HFILTER	Filter_Create (LPCWSTR szProgram) ;

VOID	Filter_Destroy (HFILTER) ;

VOID	Filter_Clear (HFILTER) ;

LPCWSTR	Filter_GetProgram (HFILTER) ;

VOID	Filter_SetProgram (HFILTER, LPCWSTR) ;

BOOL	Filter_GetProtected (HFILTER) ;

VOID	Filter_SetProtected (HFILTER hFilter, BOOL bProtected) ; 

BOOL	Filter_ResetRules (HFILTER hFilter, UINT nReason) ;

BOOL	Filter_AddRule (HFILTER hFilter, FILTRULE * pRule) ;

BOOL	Filter_AddRules (HFILTER hFilter, FILTRULE * pNewRules) ;

BOOL	Filter_AddNewRule (HFILTER hFilter,
			   DWORD nReaction, DWORD nVerbosity, DWORD nOptions, 
			   FILTREASON nReason, LPCTSTR szFormat, ...) ;

BOOL	Filter_DeleteRule (HFILTER hFilter, FILTRULE *pRule) ;

BOOL	Filter_Concat (HFILTER hFilter, HFILTER hFilterToAdd) ;

BOOL	Filter_IsHookEnabled (HFILTER hFilter) ;

VOID	Filter_EnableHook (HFILTER hFilter, BOOL) ;

BOOL	Filter_Match (HFILTER hFilter, LPCWSTR szPath) ;

BOOL	Filter_Test (HFILTER hFilter, PFILTCOND pCond, 
		     DWORD *pwReaction, DWORD *pwVerbosity, DWORD * pwOptions) ;

BOOL	Filter_EnumRules (HFILTER hFilter, ENUMRULESCALLBACK, LPVOID pContext) ;

BOOL	Filter_MoveRuleUp (HFILTER hFilter, FILTRULE * pRule) ;

BOOL	Filter_MoveRuleDown (HFILTER hFilter, FILTRULE * pRule) ;

BOOL	Filter_CanMoveUpRule (HFILTER hFilter, FILTRULE * pRule) ;

BOOL	Filter_CanMoveDownRule (HFILTER hFilter, FILTRULE * pRule) ;

UINT	Filter_Serialize	(HFILTER hFilter, PVOID pBuf, UINT nBufSize) ;

UINT	Filter_Unserialize	(HFILTER hFilter, PCVOID pBuf, UINT nBufSize) ;


#endif
