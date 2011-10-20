/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		file.h
 *
 * Abstract:
 *
 *		This module defines various types used by file hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 19-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __FILE_H__
#define __FILE_H__


/*
 * ZwCreateFile creates or opens a file. [NAR]
 */

typedef NTSTATUS (*fpZwCreateFile) (
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    );

NTSTATUS
NTAPI
HookedNtCreateFile(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PLARGE_INTEGER AllocationSize OPTIONAL,
	IN ULONG FileAttributes,
	IN ULONG ShareAccess,
	IN ULONG CreateDisposition,
	IN ULONG CreateOptions,
	IN PVOID EaBuffer OPTIONAL,
	IN ULONG EaLength
	);


/*
 * ZwOpenFile opens a file. [NAR]
 */

typedef NTSTATUS (*fpZwOpenFile) (
    OUT PHANDLE  FileHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes,
    OUT PIO_STATUS_BLOCK  IoStatusBlock,
    IN ULONG  ShareAccess,
    IN ULONG  OpenOptions
    );

NTSTATUS
NTAPI
HookedNtOpenFile(
	OUT PHANDLE  FileHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_ATTRIBUTES  ObjectAttributes,
	OUT PIO_STATUS_BLOCK  IoStatusBlock,
	IN ULONG  ShareAccess,
	IN ULONG  OpenOptions
	);


/*
 * ZwDeleteFile deletes a file. [NAR]
 */

typedef NTSTATUS (*fpZwDeleteFile) (
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
NTAPI
HookedNtDeleteFile(
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);


/*
 * ZwQueryDirectoryFile retrieves information about the contents of a directory. [NAR]
 */

typedef NTSTATUS (*fpZwQueryDirectoryFile) (
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG FileInformationLength,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	IN BOOLEAN ReturnSingleEntry,
	IN PUNICODE_STRING FileName OPTIONAL,
	IN BOOLEAN RestartScan
	);

NTSTATUS
NTAPI
HookedNtQueryDirectoryFile(
	IN HANDLE FileHandle,
	IN HANDLE Event OPTIONAL,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
	IN PVOID ApcContext OPTIONAL,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG FileInformationLength,
	IN FILE_INFORMATION_CLASS FileInformationClass,
	IN BOOLEAN ReturnSingleEntry,
	IN PUNICODE_STRING FileName OPTIONAL,
	IN BOOLEAN RestartScan
	);


/*
 * ZwQueryAttributesFile retrieves basic information about a file object. [NAR]
 */

typedef NTSTATUS (*fpZwQueryAttributesFile) (
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PFILE_BASIC_INFORMATION FileInformation
	);

NTSTATUS
NTAPI
HookedNtQueryAttributesFile(
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PFILE_BASIC_INFORMATION FileInformation
	);


/*
 * ZwQueryFullAttributesFile retrieves extended information about a file object. [NAR]
 */

typedef NTSTATUS (*fpZwQueryFullAttributesFile) (
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
	);

NTSTATUS
NTAPI
HookedNtQueryFullAttributesFile(
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
	);


/*
 * ZwSetInformationFile sets information affecting a file object. [NAR]
 */

typedef NTSTATUS (*fpZwSetInformationFile) (
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID FileInformation,
	IN ULONG FileInformationLength,
	IN FILE_INFORMATION_CLASS FileInformationClass
	);

NTSTATUS
NTAPI
HookedNtSetInformationFile(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN PVOID FileInformation,
	IN ULONG FileInformationLength,
	IN FILE_INFORMATION_CLASS FileInformationClass
	);



/*
 * ZwCreateNamedPipeFile creates a named pipe. [NAR]
 */

typedef NTSTATUS (*fpZwCreateNamedPipeFile) (
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG ShareAccess,
	IN ULONG CreateDisposition,
	IN ULONG CreateOptions,
/* The following 3 parameters listed in NAR are wrong
	IN BOOLEAN TypeMessage,
	IN BOOLEAN ReadmodeMessage,
	IN BOOLEAN Nonblocking,
*/
	IN ULONG TypeMessage,
	IN ULONG ReadmodeMessage,
	IN ULONG Nonblocking,
	IN ULONG MaxInstances,
	IN ULONG InBufferSize,
	IN ULONG OutBufferSize,
	IN PLARGE_INTEGER DefaultTimeout OPTIONAL
	);

NTSTATUS
NTAPI
HookedNtCreateNamedPipeFile(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG ShareAccess,
	IN ULONG CreateDisposition,
	IN ULONG CreateOptions,
	IN ULONG TypeMessage,
	IN ULONG ReadmodeMessage,
	IN ULONG Nonblocking,
	IN ULONG MaxInstances,
	IN ULONG InBufferSize,
	IN ULONG OutBufferSize,
	IN PLARGE_INTEGER DefaultTimeout OPTIONAL
	);



/*
 * ZwCreateMailslotFile creates a mailslot. [NAR]
 */

typedef NTSTATUS (*fpZwCreateMailslotFile) (
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CreateOptions,
	IN ULONG InBufferSize,
	IN ULONG MaxMessageSize,
	IN PLARGE_INTEGER ReadTimeout OPTIONAL
	);

NTSTATUS
NTAPI
HookedNtCreateMailslotFile(
	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CreateOptions,
	IN ULONG InBufferSize,
	IN ULONG MaxMessageSize,
	IN PLARGE_INTEGER ReadTimeout OPTIONAL
	);


BOOLEAN InitFileHooks();


#endif	/* __FILE_H__ */
