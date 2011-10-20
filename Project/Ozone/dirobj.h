/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		dirobj.h
 *
 * Abstract:
 *
 *		This module defines various types used by object directory hooking routines.
 *		These are not file system directories (see file.c) but rather containers
 *		for other objects.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 03-Sep-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __DIROBJ_H__
#define __DIROBJ_H__


#include <NTDDK.h>
#include "policy.h"
#include "pathproc.h"
#include "hookproc.h"
#include "accessmask.h"
#include "learn.h"
#include "log.h"


/*
 * ZwCreateDirectoryObject creates or opens an object directory. [NAR]
 */

typedef NTSTATUS (*fpZwCreateDirectoryObject) (
	OUT PHANDLE DirectoryHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
NTAPI
HookedNtCreateDirectoryObject(
	OUT PHANDLE DirectoryHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);


/*
 * ZwOpenDirectoryObject opens an object directory. [NAR]
 */

typedef NTSTATUS (*fpZwOpenDirectoryObject) (
	OUT PHANDLE DirectoryHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
NTAPI
HookedNtOpenDirectoryObject(
	OUT PHANDLE DirectoryHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);


BOOLEAN InitDirobjHooks();


#endif	/* __DIROBJ_H__ */
