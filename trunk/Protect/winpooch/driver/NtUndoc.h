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

#ifndef _UNDOCUMENTED_H
#define _UNDOCUMENTED_H

#include <ntddk.h>
#include <ddk/ntapi.h>

#ifndef NTUNDOC
#define NTUNDOC extern
#endif

#define FileNameInformation	9
#define FileRenameInformation	10
#define FileLinkInformation	11

#define KeyNameInformation 3


typedef struct {
  BOOL		ReplaceIfExists ;
  HANDLE	RootDirectory ;
  ULONG		FileNameLength ;
  WCHAR		FileName[1] ;
} FILE_LINK_RENAME_INFORMATION ;

typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    BYTE Reserved1[52];
    PVOID Reserved2[3];
    HANDLE UniqueProcessId;
    PVOID Reserved3;
    ULONG HandleCount;
    BYTE Reserved4[4];
    PVOID Reserved5[11];
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved6[6];
} SYSTEM_PROCESS_INFORMATION;

#define OBJ_KERNEL_HANDLE 0x0200


typedef struct _NTUNDOC_NAMESPACE
{
  /**************** NtCreateProcess ****************/
  NTSTATUS DDKAPI (*NtCreateProcess) 
    (PHANDLE, ACCESS_MASK,
     POBJECT_ATTRIBUTES,
     HANDLE, BOOLEAN, HANDLE, 
     HANDLE, HANDLE);
  
  /**************** NtCreateProcessEx ****************/
  NTSTATUS DDKAPI (*NtCreateProcessEx) 
    (PHANDLE, ACCESS_MASK,
     POBJECT_ATTRIBUTES,
     HANDLE, BOOLEAN, HANDLE, 
     HANDLE, HANDLE, HANDLE);
  
  /**************** NtCreateSection ****************/
  NTSTATUS DDKAPI (*NtCreateSection) 
    (OUT PHANDLE  SectionHandle,
     IN ACCESS_MASK  DesiredAccess,
     IN POBJECT_ATTRIBUTES  ObjectAttributes OPTIONAL,
     IN PLARGE_INTEGER  MaximumSize OPTIONAL,
     IN ULONG  SectionPageProtection,
     IN ULONG  AllocationAttributes,
     IN HANDLE  FileHandle OPTIONAL) ;

  /**************** NtCreateUserProcess ****************/
  NTSTATUS DDKAPI (*NtCreateUserProcess)
    (PHANDLE, PVOID, PVOID, PVOID, 
     PVOID, PVOID, PVOID, PVOID, 
     PVOID, PVOID, PVOID, PVOID) ; 
  
  /**************** NtTerminateProcess ****************/
  NTSTATUS DDKAPI (*NtTerminateProcess)
    (IN HANDLE ProcessHandle OPTIONAL, 
     IN NTSTATUS ExitStatus) ; 
  
  /**************** NtQueryInformationFile ****************/
  NTSTATUS DDKAPI (*NtQueryInformationFile)
    (HANDLE, PIO_STATUS_BLOCK,
     PVOID, ULONG, ULONG) ;
  
  /**************** NtQueryKey ****************/
  NTSTATUS DDKAPI (*NtQueryKey)
    (HANDLE,KEY_INFORMATION_CLASS,
     PVOID,ULONG,PULONG) ;

  /**************** ZwQueryValueKey ****************/
  NTSTATUS DDKAPI (*NtQueryValueKey)
    (HANDLE, PUNICODE_STRING,
     KEY_VALUE_INFORMATION_CLASS,
     PVOID,ULONG,PULONG) ;

  /**************** NtSetInformationFile ****************/
  NTSTATUS DDKAPI (*NtSetInformationFile) 
    (HANDLE, PIO_STATUS_BLOCK,
     PVOID, ULONG, ULONG) ;
  
  /**************** NtSetValueKey ****************/
  NTSTATUS DDKAPI (*NtSetValueKey) 
    (HANDLE, PUNICODE_STRING, ULONG,
     ULONG, PVOID, ULONG) ;
  
  /**************** ObpFreeObject ****************/
  NTSTATUS DDKFASTAPI (*ObpFreeObject)
    (PVOID  Object) ;

  /**************** PspTerminateProcess ****************/
  NTSTATUS DDKAPI (*PspTerminateProcess)
    (IN PEPROCESS Eprocess, 
     IN NTSTATUS ExitStatus) ;

  /**************** swprintf ****************/
  INT DDKFASTAPI (*swprintf)(LPWSTR,LPCWSTR,...) ;  
  
  /**************** ZwOpenProcess ****************/
  NTSTATUS DDKAPI (*ZwOpenProcess) 
    (OUT PHANDLE             ProcessHandle,
     IN ACCESS_MASK          DesiredAccess,
     IN POBJECT_ATTRIBUTES   ObjectAttributes,
     IN PCLIENT_ID           ClientId OPTIONAL) ;
  
  NTSTATUS DDKAPI (*ZwProtectVirtualMemory)
    (IN HANDLE  ProcessHandle,
     IN OUT PVOID  *BaseAddress,
     IN OUT PULONG  ProtectSize,
     IN ULONG  NewProtect,
     OUT PULONG  OldProtect);
  
  /**************** ZwReadVirtualMemory ****************/
  NTSTATUS DDKAPI (*ZwReadVirtualMemory)
    (HANDLE, PVOID,
     PVOID, ULONG, PULONG) ;

} NTUNDOC_NAMESPACE ;


#ifndef _NTUNDOC_C

extern NTUNDOC_NAMESPACE ntundoc ;

#endif


/*
 * No more needed functions
 * Keeping them just in case
 *

typedef struct {
  ULONG  NameLength;
  WCHAR  Name[1] ;  
} KEY_NAME_INFORMATION ;


NTUNDOC NTSTATUS DDKAPI (*NtDeleteFile)(IN POBJECT_ATTRIBUTES) ;

NTUNDOC DDKAPI NTSTATUS (*NtQueryInformationProcess) (HANDLE, ULONG,
						      PVOID, ULONG, PULONG) ; 

NTUNDOC
NTSTATUS
DDKAPI 
(*NtUndoc_ObQueryNameString)
  (IN PVOID Object,
   OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
   IN ULONG Length,
   OUT PULONG ReturnLength);   

NTUNDOC
NTSTATUS 
DDKFASTAPI
(*NtUndoc_ObfDereferenceObject)
  (PVOID  Object) ; 

NTUNDOC
NTSTATUS 
DDKFASTAPI
(*NtUndoc_ObFastDereferenceObject)
  (PVOID  Object) ; 


*/

NTSTATUS NtUndoc_Init() ;

#endif
