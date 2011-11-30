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

#ifndef _EVENTLOGGER_H
#define _EVENTLOGGER_H

#include <windows.h>

#include "FiltCond.h"


typedef struct {
  DWORD		dwProcessId ;
  TCHAR		szExeName[32] ;
  TCHAR		szPath[MAX_PATH] ;
  WORD		nReaction ;
  WORD		nVerbosity ;
  SYSTEMTIME	time ;
  FILTCOND	condition ;
} EVENTSTRUCT ;


BOOL	EventLog_Init () ;

VOID	EventLog_Uninit () ;

VOID	EventLog_ReloadConfig () ;

DWORD	EventLog_Add (DWORD dwProcessId, LPCTSTR szProcess,
		      int nReaction, int nVerbosity, 
		      PCFILTCOND pCondition) ;

DWORD	EventLog_GetBeginId () ;

DWORD	EventLog_GetEndId () ;

BOOL	EventLog_Clear () ;

EVENTSTRUCT *	EventLog_MapEvent (DWORD) ;

VOID	EventLog_UnmapEvent (DWORD) ;

#endif
