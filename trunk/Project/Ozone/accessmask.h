/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		accessmask.h
 *
 * Abstract:
 *
 *		This module implements various ACCESS_MASK decoding routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 18-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __ACCESSMASK_H__
#define __ACCESSMASK_H__


#include <NTDDK.h>
#include "policy.h"
#include "ntproto.h"
#include "log.h"


// IBS = Is Bit Set?

#define	IS_BIT_SET(da, mask) (((da) & (mask)) == (mask))


UCHAR Get_FILE_OperationType(ACCESS_MASK DesiredAccess);
UCHAR Get_NAMEDPIPE_OperationType(ACCESS_MASK DesiredAccess);
UCHAR Get_MAILSLOT_OperationType(ACCESS_MASK DesiredAccess);
UCHAR Get_REGISTRY_OperationType(ACCESS_MASK DesiredAccess);
UCHAR Get_EVENT_OperationType(ACCESS_MASK DesiredAccess);
UCHAR Get_SEMAPHORE_OperationType(ACCESS_MASK DesiredAccess);
UCHAR Get_SECTION_OperationType(ACCESS_MASK DesiredAccess);
UCHAR Get_JOB_OperationType(ACCESS_MASK DesiredAccess);
UCHAR Get_MUTANT_OperationType(ACCESS_MASK DesiredAccess);
UCHAR Get_SYMLINK_OperationType(ACCESS_MASK DesiredAccess);
UCHAR Get_TIMER_OperationType(ACCESS_MASK DesiredAccess);
UCHAR Get_PORT_OperationType(ACCESS_MASK DesiredAccess);
UCHAR Get_DIROBJ_OperationType(ACCESS_MASK DesiredAccess);

void DecodeFileOperationType(ACCESS_MASK DesiredAccess);



#endif	/* __ACCESSMASK_H__ */