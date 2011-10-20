/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		symlink.h
 *
 * Abstract:
 *
 *		This module defines various types used by symbolic link object hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 25-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __SYMLINK_H__
#define __SYMLINK_H__


#include <NTDDK.h>
#include "policy.h"
#include "pathproc.h"
#include "hookproc.h"
#include "accessmask.h"
#include "learn.h"
#include "log.h"


/*
 * ZwCreateSymbolicLinkObject creates or opens a symbolic link object. [NAR]
 */

typedef NTSTATUS (*fpZwCreateSymbolicLinkObject) (
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PUNICODE_STRING TargetName
	);

NTSTATUS
NTAPI
HookedNtCreateSymbolicLinkObject(
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PUNICODE_STRING TargetName
	);


/*
 * ZwOpenSymbolicLinkObject opens a symbolic link object. [NAR]
 */

typedef NTSTATUS (*fpZwOpenSymbolicLinkObject) (
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
NTAPI
HookedNtOpenSymbolicLinkObject(
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);


BOOLEAN InitSymlinkHooks();


#endif	/* __SYMLINK_H__ */
