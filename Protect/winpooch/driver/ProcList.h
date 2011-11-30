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

#ifndef _PROCLIST_H
#define _PROCLIST_H

#include <ntddk.h>

#include "Filter.h"
#include "ProcInfo.h"

#define MAX_FILTERS		4

#define PROCESS_IGNORE_ALL	1
#define PROCESS_NO_NOTIFICATION	2

typedef struct 
{
  PROCADDR	nProcessAddress ;
  ULONG		nProcessId ;
  WCHAR		wszPath[MAX_PATH] ;
  HFILTER	aFilters[MAX_FILTERS] ;
  ULONG		nFilters ;
  ULONG		nFlags ;
} PROCSTRUCT ;


VOID		ProcList_Init () ;

VOID		ProcList_Uninit () ;

NTSTATUS	ProcList_SetScannerExePath (LPCWSTR szScannerExe) ;

LPCWSTR		ProcList_GetScannerExePath () ;

PROCSTRUCT*	ProcList_New (IN PROCADDR	nProcessAddress, 
			      IN PROCID		nProcessId, 
			      IN LPCWSTR	wszFilePath) ;

VOID		ProcList_Delete (PROCSTRUCT*) ;

NTSTATUS	ProcList_Lock () ;

NTSTATUS	ProcList_Unlock () ;

BOOL		ProcList_IsLocked () ;

VOID		ProcList_Clear () ;

NTSTATUS	ProcList_Populate () ;

NTSTATUS	ProcList_Add (PROCSTRUCT*) ;

PROCSTRUCT*	ProcList_Remove (PROCADDR) ;

PROCSTRUCT*	ProcList_Get (PROCADDR) ;

NTSTATUS	ProcList_RefreshFilterLists () ;

typedef BOOL	(*ENUMPROCCALLBACK)(PVOID,PROCADDR,ULONG,LPCWSTR) ;

NTSTATUS	ProcList_Enum (ENUMPROCCALLBACK, PVOID) ;

#endif
