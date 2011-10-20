/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		atom.h
 *
 * Abstract:
 *
 *		This module defines various types used by atom object hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 06-Apr-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __ATOM_H__
#define __ATOM_H__


#include <NTDDK.h>
#include "policy.h"
#include "pathproc.h"
#include "hookproc.h"
#include "accessmask.h"
#include "learn.h"
#include "log.h"


/*
 * ZwAddAtom adds an atom to the global atom table. [NAR]
 */

typedef NTSTATUS (*fpZwAddAtom) (
	IN PWSTR String,
	IN ULONG StringLength,
	OUT PUSHORT Atom
	);

NTSTATUS
NTAPI
HookedNtAddAtom(
	IN PWSTR String,
	IN ULONG StringLength,
	OUT PUSHORT Atom
	);



/*
 * ZwFindAtom searches for an atom in the global atom table. [NAR]
 */


typedef NTSTATUS (*fpZwFindAtom) (
	IN PWSTR String,
	IN ULONG StringLength,
	OUT PUSHORT Atom
	);

NTSTATUS
NTAPI
HookedNtFindAtom(
	IN PWSTR String,
	IN ULONG StringLength,
	OUT PUSHORT Atom
	);


BOOLEAN InitAtomHooks();


#endif	/* __ATOM_H__ */
