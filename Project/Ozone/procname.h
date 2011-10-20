/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		procname.h
 *
 * Abstract:
 *
 *		This module defines various types used by process id to process name conversion routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 23-Feb-2004
 *
 * Revision History:
 *
 *		07-Apr-2004 ET - Copied from process.h
 */

#ifndef __PROCNAME_H__
#define __PROCNAME_H__


#include "userland.h"


typedef struct	_IMAGE_PID_ENTRY
{
	struct _IMAGE_PID_ENTRY		*next;
	ULONG						ProcessId;
	ULONG						ParentId;
	BOOLEAN						FirstThread;				// Was more than one thread already created?
															// (some actions need to take place only in the main thread)
	UCHAR						WaitingForUserRequestId;	// contains the sequence id of the reply we are waiting for
	KEVENT						UserlandRequestDoneEvent;
	PUSERLAND_REPLY_HEADER		UserlandReply;
	SECURITY_POLICY				SecPolicy;
	WCHAR						ImageName[1];

} IMAGE_PID_ENTRY, *PIMAGE_PID_ENTRY;


/*
 * 1. The following number should be prime.
 * 2. It should also be slightly larger than the "average" number of processes of any given machine to
 *    minimize the number of hash table collisions (we want O(1) access) and at the same time not
 *    eating up too much memory (gImagePidHtbl[]).
 */
#define	IMAGE_PID_HASHTABLE_SIZE	67

extern IMAGE_PID_ENTRY	gImagePidHtbl[IMAGE_PID_HASHTABLE_SIZE];

extern USHORT			ProcessNameOffset, ThreadServiceTableOffset;
extern BOOLEAN			BootingUp;


BOOLEAN				InitProcessNameEntries();
VOID				RemoveProcessNameEntries();
PIMAGE_PID_ENTRY	FindImagePidEntry(ULONG ProcessId, ULONG ParentId);
BOOLEAN				ProcessInsertImagePidEntry(ULONG ProcessId, PIMAGE_PID_ENTRY NewProcess);
PIMAGE_PID_ENTRY	CreateNewProcessEntry(ULONG ProcessId, ULONG ParentId, PUNICODE_STRING ProcessName, BOOLEAN NewProcess);
//PIMAGE_PID_ENTRY	CreateAndLoadNewProcessEntry(ULONG ProcessId, PUNICODE_STRING ProcessName, BOOLEAN NewProcess);
VOID				EnumerateExistingProcesses();
PWCHAR				GetCurrentProcessName();


#endif	/* __PROCNAME_H__ */