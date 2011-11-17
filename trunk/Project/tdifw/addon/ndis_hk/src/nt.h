// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// $Id: nt.h,v 1.1.1.1 2002/08/08 12:49:37 dev Exp $

#ifndef _nt_h_
#define _nt_h_

/*
 * some prototypes for Native API
 */

#define SystemModuleInformation     11

typedef struct _SYSTEM_MODULE_INFORMATION {
	ULONG	Reserved[2];
	PVOID	Base;
	ULONG	Size;
	ULONG	Flags;
	USHORT	Index;
	USHORT	Unknown;
	USHORT	LoadCount;
	USHORT	ModuleNameOffset;
	CHAR	ImageName[255];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

NTSYSAPI
NTSTATUS
ZwQuerySystemInformation(
	IN ULONG SystemInformationClass,
	IN PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength);

NTSTATUS
NTAPI
ZwWaitForSingleObject(
	IN HANDLE hObject,
	IN BOOLEAN bAlertable,
	IN PLARGE_INTEGER Timeout
);

#endif
