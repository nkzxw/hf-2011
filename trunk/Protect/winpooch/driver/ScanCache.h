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

#ifndef _SCAN_CACHE_H
#define _SCAN_CACHE_H

#include <ntddk.h>

#include "ScanResult.h"

typedef ULONG SCANCACHEID ;

typedef BOOL (*SCANCACHEENUMCALLBACK)(VOID * pUserPtr, SCANCACHEID nIdentifier, LPCWSTR wszFilePath, SCANRESULT nResult, LARGE_INTEGER*pliScanTime) ;

NTSTATUS ScanCache_Init () ;

NTSTATUS ScanCache_Uninit () ;

NTSTATUS ScanCache_Lock () ;

NTSTATUS ScanCache_Unlock () ;

BOOL	 ScanCache_IsLocked () ;

NTSTATUS ScanCache_GetCacheInfo (UINT* pnMaxLength, ULONG* pnFirstIdentifier, ULONG* pnLastIdentifier) ;

NTSTATUS ScanCache_GetFileId (LPCWSTR szFile, SCANCACHEID * pnId) ;

NTSTATUS ScanCache_SetStatus (SCANCACHEID nId, SCANRESULT nResult, LARGE_INTEGER * pliScanTime) ;

NTSTATUS ScanCache_GetStatus (SCANCACHEID nId, SCANRESULT*, LARGE_INTEGER *) ;

NTSTATUS ScanCache_GetFilePath (SCANCACHEID nId, LPWSTR szFilePath) ;

NTSTATUS ScanCache_WaitChange (SCANCACHEID nId) ;

NTSTATUS ScanCache_EnumChangesSince (SCANCACHEENUMCALLBACK, VOID*, LARGE_INTEGER * pliSinceTime) ;

#endif
