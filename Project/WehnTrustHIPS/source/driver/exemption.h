/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#ifndef _WEHNTRUST_DRIVER_EXEMPTION_H
#define _WEHNTRUST_DRIVER_EXEMPTION_H

//
// Exemption keys
//
#define WEHNTRUST_EXEMPTION_KEY \
	WEHNTRUST_REGISTRY_CONFIG_PATH L"\\Exemptions"
#define WEHNTRUST_GLOBAL_SCOPE_EXEMPTION_KEY \
	WEHNTRUST_REGISTRY_CONFIG_PATH L"\\Exemptions\\Global"

#define EXEMPTION_PATH_CHECKSUM_SIZE (SHA1_HASH_SIZE * 2)

typedef struct _EXEMPTION
{
	LIST_ENTRY      Entry;
	UNICODE_STRING  SymbolicPath;
	EXEMPTION_TYPE  Type;
	EXEMPTION_SCOPE Scope;
	ULONG           Flags;
} EXEMPTION, *PEXEMPTION;

//
// Initialization
//
NTSTATUS InitializeExemptions();

//
// Exemption checking routines
//
BOOLEAN IsImageFileExempted(
		IN PPROCESS_OBJECT ProcessObject,
		IN PFILE_OBJECT FileObject);

NTSTATUS AddExemption(
		IN EXEMPTION_TYPE Type,
		IN EXEMPTION_SCOPE Scope,
		IN ULONG Flags,
		IN PUNICODE_STRING SymbolicPath,
		IN BOOLEAN AddToRegistry);
NTSTATUS RemoveExemption(
		IN EXEMPTION_TYPE Type,
		IN EXEMPTION_SCOPE Scope,
		IN ULONG Flags,
		IN PUNICODE_STRING SymbolicPath,
		IN BOOLEAN RemoveFromRegistry);
NTSTATUS FlushExemptions(
		IN EXEMPTION_TYPE Type,
		IN EXEMPTION_SCOPE Scope,
		IN BOOLEAN RemoveFromRegistry);

#endif
