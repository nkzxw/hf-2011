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

#ifndef _PROCINFO_H
#define _PROCINFO_H

#include <ntddk.h>

typedef UINT_PTR PROCADDR ;

typedef ULONG	PROCID ;

NTSTATUS ProcInfo_GetImagePath (HANDLE hProcess, PUNICODE_STRING pusPath) ;

NTSTATUS ProcInfo_GetSystemRoot (HANDLE hProcess, PUNICODE_STRING pusPath) ;

NTSTATUS ProcInfo_GetSystem32Root (HANDLE hProcess, PUNICODE_STRING pusPath) ;

NTSTATUS ProcInfo_GetCurDirDosPath (HANDLE hProcess, PUNICODE_STRING pusPath) ;

NTSTATUS ProcInfo_GetProcessId (IN HANDLE hProcess, OUT PROCID * pId) ;

NTSTATUS ProcInfo_GetAddress (IN HANDLE hProcess, OUT PROCADDR *) ;

#define ProcInfo_GetCurrentProcessAddress()	\
  ((PROCADDR)IoGetCurrentProcess())

#define ProcInfo_GetCurrentProcessId()	\
  ((PROCID)PsGetCurrentProcessId())

//NTSTATUS ProcInfo_KillProcessFromPid (ULONG nProcessId) ;

#endif
