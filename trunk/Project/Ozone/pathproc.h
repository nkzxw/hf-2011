/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		pathproc.h
 *
 * Abstract:
 *
 *		This module definies various types used by pathname handling routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 19-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __PATHPROC_H__
#define __PATHPROC_H__


#include <NTDDK.h>
#include "log.h"


// some registry keys are actually longer than 260 chars!
// HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\DeviceClasses\{6994AD04-93EF-11D0-A3CC-00A0C9223196}\##?#Root#SYSTEM#0000#{6994ad04-93ef-11d0-a3cc-00a0c9223196}\#{2f412ab5-ed3a-4590-ab24-b0ce2aa77d3c}&{9B365890-165F-11D0-A195-0020AFD156E4}\Device Parameters
//#define	MAX_PATH		260

#define	MAX_PATH		300
#define chMAX_PATH		MAX_PATH
#define bMAX_PATH		(MAX_PATH * sizeof(WCHAR))


/* maximum number of links to follow */
#define	MAX_NUMBER_OF_LINKS		3


#define	DO_NOT_RESOLVE_LINKS	0
#define	RESOLVE_LINKS			1


BOOLEAN	GetPathFromOA(IN POBJECT_ATTRIBUTES ObjectAttributes, OUT PCHAR OutBuffer, IN USHORT OutBufferSize, IN BOOLEAN ResolveLinks);
BOOLEAN	GetPathFromOAW(IN POBJECT_ATTRIBUTES ObjectAttributes, OUT PCHAR OutBuffer, IN USHORT OutBufferSize, IN BOOLEAN ResolveLinks);
BOOLEAN ResolveFilename(IN PCHAR szFileName, OUT PCHAR szResult, IN USHORT szResultSize);
BOOLEAN	ResolveFilenameW(IN PUNICODE_STRING szFileName, OUT PCHAR szResult, IN USHORT szResultSize);
PWSTR	GetNameFromHandle(IN HANDLE ObjectHandle, OUT PWSTR OutBuffer, IN USHORT OutBufferSize);
BOOLEAN	FixupFilename(IN PCHAR szFileName, OUT PCHAR szResult, IN USHORT szResultSize);
BOOLEAN	VerifyExecutableName(IN PCHAR szFileName);
PCHAR	StripFileMacros(IN PCHAR Path, OUT PCHAR Buffer, IN USHORT BufferSize);
BOOLEAN	ConvertLongFileNameToShort(IN PCHAR LongFileName, OUT PCHAR ShortFileName, IN USHORT ShortFileNameSize);


NTSTATUS
ObQueryNameString(
    IN PVOID Object,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength
    );


#endif	/* __PATHPROC_H__ */