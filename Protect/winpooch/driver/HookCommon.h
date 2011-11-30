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

#ifndef _HOOK_COMMON_H
#define _HOOK_COMMON_H

#include <ntddk.h>

#include "ProcList.h"	// for type PROCADDR
#include "FiltRule.h"

NTSTATUS HookCommon_Init () ;

NTSTATUS HookCommon_Uninit () ;

NTSTATUS HookCommon_CatchCall (UINT* pnReaction, UINT* pnOptions,
			       UINT nReason, LPCTSTR szFormat, ...) ;

NTSTATUS HookCommon_ShouldScanFile (LPCWSTR szFilePath) ;

NTSTATUS HookCommon_ScanFile (OUT UINT * pnReaction, 
			      IN  LPCWSTR szFilePath,
			      IN  LARGE_INTEGER * pliFileTime) ;

NTSTATUS HookCommon_SendProcessCreatedNotification (PROCADDR, ULONG nPid, LPCWSTR wszFilePath) ;

NTSTATUS HookCommon_SendPidChangedNotification (PROCADDR, ULONG nPid) ;

NTSTATUS HookCommon_SendProcessTerminatedNotification (PROCADDR) ;

NTSTATUS HookCommon_SetScanFilters (LPVOID pInBuf, UINT nInBufSize) ;

#endif
