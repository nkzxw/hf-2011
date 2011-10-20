/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		debug.h
 *
 * Abstract:
 *
 *		This module implements various debug hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 23-Apr-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __DEBUG_H__
#define __DEBUG_H__



/*
 * ZwDebugActiveProcess.
 */

typedef NTSTATUS (*fpZwDebugActiveProcess) (
	UINT32	Unknown1,
	UINT32	Unknown2
	);

NTSTATUS
NTAPI
HookedNtDebugActiveProcess(
	UINT32	Unknown1,
	UINT32	Unknown2
	);


BOOLEAN InitDebugHooks();


#endif	/* __DEBUG_H__ */
