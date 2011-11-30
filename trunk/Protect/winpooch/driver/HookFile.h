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

#ifndef _HOOK_FILE_H
#define _HOOK_FILE_H

#include <ntddk.h>

NTSTATUS WINAPI Hook_IoCreateFile(OUT PHANDLE FileHandle,
				  IN ACCESS_MASK DesiredAccess,
				  IN POBJECT_ATTRIBUTES ObjectAttributes,
				  OUT PIO_STATUS_BLOCK IoStatusBlock,
				  IN PLARGE_INTEGER AllocationSize OPTIONAL,
				  IN ULONG FileAttributes,
				  IN ULONG ShareAccess,
				  IN ULONG Disposition,
				  IN ULONG CreateOptions,
				  IN PVOID EaBuffer OPTIONAL,
				  IN ULONG EaLength,
				  IN CREATE_FILE_TYPE CreateFileType,
				  IN PVOID ExtraCreateParameters OPTIONAL,
				  IN ULONG Options) ;

NTSTATUS WINAPI Hook_NtDeleteFile (POBJECT_ATTRIBUTES) ;

NTSTATUS WINAPI Hook_NtSetInformationFile (HANDLE, PIO_STATUS_BLOCK,
					   PVOID, ULONG, ULONG) ;

NTSTATUS WINAPI Hook_NtDeviceIoControlFile (HANDLE FileHandle,
					    HANDLE Event,
					    PIO_APC_ROUTINE ApcRoutine,
					    PVOID ApcContext,
					    PIO_STATUS_BLOCK IoStatusBlock,
					    ULONG IoControlCode,
					    PVOID InputBuffer,
					    ULONG InputBufferLength,
					    PVOID OutputBuffer,
					    ULONG OutputBufferLength) ;

NTSTATUS WINAPI Hook_NtWriteFile (IN HANDLE FileHandle,
				  IN HANDLE  Event  OPTIONAL,
				  IN PIO_APC_ROUTINE  ApcRoutine  OPTIONAL,
				  IN PVOID  ApcContext  OPTIONAL,
				  OUT PIO_STATUS_BLOCK  IoStatusBlock,
				  IN PVOID  Buffer,
				  IN ULONG  Length,
				  IN PLARGE_INTEGER  ByteOffset  OPTIONAL,
				  IN PULONG  Key  OPTIONAL) ;



#endif
