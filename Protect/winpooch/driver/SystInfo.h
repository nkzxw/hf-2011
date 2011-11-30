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

#ifndef _SYSTINFO_H
#define _SYSTINFO_H

#include <ntddk.h>

enum 
  {
    UNKNOWN_WINDOWS_VERSION,
    WINDOWS_2000_SP4_X86_RETAIL,
    WINDOWS_XP_SP1_X86_RETAIL,
    WINDOWS_XP_SP2_X86_RETAIL,
    WINDOWS_SERVER_2003_X86_RETAIL,
    WINDOWS_SERVER_2003_SP1_X86_RETAIL,
  } ;

WORD	SystInfo_GetWindowsVersion () ;

NTSTATUS SystInfo_GetSystemRoot (PUNICODE_STRING pusPath) ;

NTSTATUS SystInfo_GetModuleBase (LPCSTR szModule, PVOID * ppBase, ULONG * pnSize) ;

#endif
