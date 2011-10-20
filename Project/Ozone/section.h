/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		section.h
 *
 * Abstract:
 *
 *		This module defines various types used by section hooking related routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 29-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */

#ifndef __MEMORY_H__
#define __MEMORY_H__



/*
 * "Section objects are objects that can be mapped into the virtual address space of a process.
 * The Win32 API refers to section objects as file-mapping objects.
 *
 * ZwOpenSection opens a section object." [NAR]
 */

typedef NTSTATUS (*fpZwOpenSection) (
    OUT PHANDLE  SectionHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes
    );


NTSTATUS
NTAPI
HookedNtOpenSection(
    OUT PHANDLE  SectionHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes
	);


/*
 * ZwCreateSection creates a section object. [NAR]
 */

typedef NTSTATUS (*fpZwCreateSection) (
	OUT PHANDLE SectionHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PLARGE_INTEGER SectionSize OPTIONAL,
	IN ULONG Protect,
	IN ULONG Attributes,
	IN HANDLE FileHandle
	);

NTSTATUS
NTAPI
HookedNtCreateSection(
	OUT PHANDLE SectionHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PLARGE_INTEGER SectionSize OPTIONAL,
	IN ULONG Protect,
	IN ULONG Attributes,
	IN HANDLE FileHandle
	);


/*
 * ZwMapViewOfSection maps a view of a section to a range of virtual addresses. [NAR]
 */

typedef NTSTATUS (*fpZwMapViewOfSection) (
	IN HANDLE SectionHandle,
	IN HANDLE ProcessHandle,
	IN OUT PVOID *BaseAddress,
	IN ULONG ZeroBits,
	IN ULONG CommitSize,
	IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
	IN OUT PULONG ViewSize,
	IN SECTION_INHERIT InheritDisposition,
	IN ULONG AllocationType,
	IN ULONG Protect
	);

NTSTATUS
NTAPI
HookedNtMapViewOfSection(
	IN HANDLE SectionHandle,
	IN HANDLE ProcessHandle,
	IN OUT PVOID *BaseAddress,
	IN ULONG ZeroBits,
	IN ULONG CommitSize,
	IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
	IN OUT PULONG ViewSize,
	IN SECTION_INHERIT InheritDisposition,
	IN ULONG AllocationType,
	IN ULONG Protect
	);


BOOLEAN InitSectionHooks();


#endif	/* __MEMORY_H__ */
