/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_RTL_H
#define _WEHNTRUST_DRIVER_RTL_H

#ifndef ALLOC_TAG
#define ALLOC_TAG 'rtle'
#endif

NTSTATUS RtleCopyUnicodeString(
		OUT PUNICODE_STRING Destination,
		IN PUNICODE_STRING Source);
NTSTATUS RtleFreeUnicodeString(
		IN PUNICODE_STRING UnicodeString);
PUCHAR RtleSearchMemory(
		IN PUCHAR Haystack,
		IN ULONG HaystackLength,
		IN PUCHAR Needle,
		IN ULONG NeedleLength);

ULONG RtleUnicodeStringToInteger(
		IN PUNICODE_STRING UnicodeString,
		IN ULONG Base);

PWSTR RtleFindUnicodeStringInUnicodeString(
		IN PUNICODE_STRING Haystack,
		IN PUNICODE_STRING Needle);
PWSTR RtleFindStringInUnicodeString(
		IN PUNICODE_STRING Haystack,
		IN PWSTR Needle);

NTSTATUS RtleGetFileModificationTime(
		IN PFILE_OBJECT FileObject,
		OUT PLARGE_INTEGER ModificationTime);
BOOLEAN RtleDoesFileExist(
		IN PUNICODE_STRING FilePath);

NTSTATUS RtleGetFilePath(
		IN PFILE_OBJECT FileObject,
		OUT POBJECT_NAME_INFORMATION *FilePath);
VOID RtleFreeFilePath(
		IN POBJECT_NAME_INFORMATION FilePath);

BOOLEAN RtleCompareNtPathToPhysicalPath(
		IN PUNICODE_STRING NtFilePath,
		IN PUNICODE_STRING PhysicalFilePath,
		IN BOOLEAN IsDirectory);

VOID RtleTruncateUnicodeTerminator(
		IN PUNICODE_STRING UnicodeString);

NTSTATUS RtleFlushDirectory(
		IN PUNICODE_STRING DirectoryPath,
		IN BOOLEAN IgnoreInUse);


//
// Extended shortcuts routines
//

NTSTATUS RtleGetThreadObject(
		IN HANDLE ThreadHandle,
		IN ACCESS_MASK DesiredAccess,
		OUT PTHREAD_OBJECT *ThreadObject);

NTSTATUS RtleGetProcessObject(
		IN HANDLE ProcessHandle,
		IN ACCESS_MASK DesiredAccess,
		OUT PPROCESS_OBJECT *ProcessObject);

#endif
