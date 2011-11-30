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

#include <windows.h>

#define PS_UNKNOWN_STATE		0
#define PS_HOOK_DISABLED		1
#define PS_HOOKED_SINCE_BIRTH		2
#define PS_HOOKED_WHILE_RUNNING		3

typedef UINT_PTR PROCADDR ;

typedef struct {
  PROCADDR	nProcessAddress ;
  DWORD		nProcessId ;
  DWORD		nState ;
  TCHAR		szName[32] ;
  TCHAR		szPath[MAX_PATH] ;
} PROCSTRUCT ;

typedef BOOL(*ENUMPROCCALLBACK)(void*,PROCSTRUCT*) ;


BOOL	ProcList_Init () ;

VOID	ProcList_Uninit () ;

VOID	ProcList_Lock () ;

VOID	ProcList_Unlock () ;

VOID	ProcList_Clear () ;

BOOL   	ProcList_Add (const PROCSTRUCT * ) ;

VOID	ProcList_Remove (PROCADDR nProcessAddress) ;

VOID	ProcList_Enum (ENUMPROCCALLBACK, void*) ;

PROCSTRUCT * ProcList_Get (PROCADDR nProcessAddress) ;

DWORD	ProcList_GetState (PROCADDR nProcessAddress) ;

VOID	ProcList_SetState (PROCADDR nProcessAddress, DWORD nState) ;

#endif
