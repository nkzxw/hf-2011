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

#ifndef _WATCHED_OBJECTS_H
#define _WATCHED_OBJECTS_H

#include <ntddk.h>

#define WOT_SECTION	1
#define WOT_FILE	2
#define WOT_SOCKET	3

typedef struct {
  LARGE_INTEGER		liFileTime ;
  WCHAR			wszFilePath[0] ;
} WOTFILE ;

typedef WOTFILE WOTSECTION ;


NTSTATUS WatchObjs_Init () ;

NTSTATUS WatchObjs_Uninit () ;

NTSTATUS WatchObjs_Lock () ;

NTSTATUS WatchObjs_Unlock () ;

NTSTATUS WatchObjs_AddFromPointer (PVOID	pObject,
				   ULONG	nType,
				   PVOID	pUserData,
				   ULONG	nUserDataSize) ;

__attribute__ ((deprecated))
NTSTATUS WatchObjs_AddFromHandle (HANDLE hObject,
				  ULONG	nType,
				  PVOID	pUserData,
				  ULONG	nUserDataSize) ;

NTSTATUS WatchObjs_RemFromPointer (PVOID pObject) ;

NTSTATUS WatchObjs_GetFromPointer (PVOID	pObject,
				   ULONG	nType,
				   PVOID	*ppUserData,
				   ULONG	*pnUserDataSize) ;

__attribute__ ((deprecated))
NTSTATUS WatchObjs_GetFromHandle (HANDLE	hObject,
				  ULONG		nType,
				  PVOID		*ppUserData,
				  ULONG		*pnUserDataSize) ;

NTSTATUS WatchObjs_AddWotFile (IN PVOID			pObject,
			       IN LARGE_INTEGER		*pliFileTime,
			       IN UNICODE_STRING	*pusFilePath) ;

#endif
