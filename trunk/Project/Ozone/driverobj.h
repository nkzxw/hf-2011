/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		driverobj.h
 *
 * Abstract:
 *
 *		This module defines various types used by driver object hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 06-Apr-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __DRIVEROBJ_H__
#define __DRIVEROBJ_H__


#include <NTDDK.h>
#include "policy.h"
#include "pathproc.h"
#include "hookproc.h"
#include "accessmask.h"
#include "learn.h"
#include "log.h"


/*
 * ZwLoadDriver loads a device driver. [NAR]
 */

typedef NTSTATUS (*fpZwLoadDriver) (
	IN PUNICODE_STRING DriverServiceName
	);

NTSTATUS
NTAPI
HookedNtLoadDriver(
	IN PUNICODE_STRING DriverServiceName
	);


/*
 * ZwUnloadDriver unloads a device driver. [NAR]
 */

typedef NTSTATUS (*fpZwUnloadDriver) (
	IN PUNICODE_STRING DriverServiceName
	);

NTSTATUS
NTAPI
HookedNtUnloadDriver(
	IN PUNICODE_STRING DriverServiceName
	);


BOOLEAN InitDriverObjectHooks();


#endif	/* __DRIVEROBJ_H__ */
