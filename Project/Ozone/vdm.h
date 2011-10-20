/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		vdm.h
 *
 * Abstract:
 *
 *		This module implements various VDM (Virtual Dos Machine) hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 06-Apr-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __VDM_H__
#define __VDM_H__



/*
 * ZwSetLdtEntries sets Local Descriptor Table (LDT) entries for a Virtual DOS Machine (VDM). [NAR]
 */

typedef NTSTATUS (*fpZwSetLdtEntries) (
    IN ULONG Selector0,
    IN ULONG Entry0Low,
    IN ULONG Entry0Hi,
    IN ULONG Selector1,
    IN ULONG Entry1Low,
    IN ULONG Entry1Hi
	);

NTSTATUS
NTAPI
HookedNtSetLdtEntries(
    IN ULONG Selector0,
    IN ULONG Entry0Low,
    IN ULONG Entry0Hi,
    IN ULONG Selector1,
    IN ULONG Entry1Low,
    IN ULONG Entry1Hi
	);


/*
 * ZwVdmControl performs a control operation on a VDM. [NAR]
 */

typedef NTSTATUS (*fpZwVdmControl) (
	IN ULONG ControlCode,
	IN PVOID ControlData
	);

NTSTATUS
NTAPI
HookedNtVdmControl(
	IN ULONG ControlCode,
	IN PVOID ControlData
	);


BOOLEAN InitVdmHooks();


#endif	/* __VDM_H__ */
