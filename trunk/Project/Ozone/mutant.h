/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		mutant.h
 *
 * Abstract:
 *
 *		This module defines various types used by mutant (mutex) hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 25-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __MUTANT_H__
#define __MUTANT_H__


#include <NTDDK.h>
#include "policy.h"
#include "pathproc.h"
#include "hookproc.h"
#include "accessmask.h"
#include "learn.h"
#include "log.h"


/*
 * ZwCreateMutant creates or opens a mutant object. [NAR]
 */

typedef NTSTATUS (*fpZwCreateMutant) (
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOLEAN InitialOwner
	);

NTSTATUS
NTAPI
HookedNtCreateMutant(
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOLEAN InitialOwner
	);


/*
 * ZwOpenMutant opens a mutant object. [NAR]
 */

typedef NTSTATUS (*fpZwOpenMutant) (
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);

NTSTATUS
NTAPI
HookedNtOpenMutant(
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	);


BOOLEAN InitMutantHooks();


#endif	/* __MUTANT_H__ */
