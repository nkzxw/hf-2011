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

#ifndef _HOOK_SYS_H
#define _HOOK_SYS_H

#include <ntddk.h>


NTSTATUS DDKAPI Hook_NtCreateProcess (PHANDLE, ACCESS_MASK,
				      POBJECT_ATTRIBUTES,
				      HANDLE, BOOLEAN, HANDLE, 
				      HANDLE, HANDLE);

NTSTATUS DDKAPI Hook_NtCreateProcessEx (PHANDLE, ACCESS_MASK,
					POBJECT_ATTRIBUTES,
					HANDLE, BOOLEAN, HANDLE, 
					HANDLE, HANDLE, HANDLE);

NTSTATUS DDKAPI Hook_NtCreateSection (PHANDLE, ACCESS_MASK,
				      POBJECT_ATTRIBUTES,
				      PLARGE_INTEGER,
				      ULONG, ULONG, HANDLE) ;

NTSTATUS DDKAPI Hook_PspTerminateProcess (IN PEPROCESS	Eprocess, 
					  IN NTSTATUS	ExitStatus) ;

NTSTATUS DDKAPI Hook_NtTerminateProcess (IN HANDLE ProcessHandle OPTIONAL, 
					 IN NTSTATUS ExitStatus) ;
/*
NTSTATUS DDKFASTAPI Hook_ObfDereferenceObject (PVOID  Object) ;

NTSTATUS DDKFASTAPI Hook_ObFastDereferenceObject (PVOID  Object) ;
*/
NTSTATUS DDKFASTAPI Hook_ObpFreeObject_Win2K (PVOID  Object) ;

#define PARAM_IN_EAX __attribute__((regparm (1)))

NTSTATUS PARAM_IN_EAX Hook_ObpFreeObject_Vista (PVOID  Object) ;

NTSTATUS Hook_NtCreateUserProcess (PHANDLE, PVOID, PVOID, PVOID, 
				   PVOID, PVOID, PVOID, PVOID, 
				   PVOID, PVOID, PVOID, PVOID) ;

#endif
