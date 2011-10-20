/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		network.h
 *
 * Abstract:
 *
 *		This module defines various types used by the Transport Driver Interface (TDI) network hooking routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 12-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __NETWORK_H__
#define __NETWORK_H__



#define	NET_DEVICE_TYPE_TCP	1
#define	NET_DEVICE_TYPE_UDP	2
#define	NET_DEVICE_TYPE_IP	3


typedef struct _TDI_CALLBACK
{
	PIO_COMPLETION_ROUTINE	Routine;
	PVOID					Context;

} TDI_CALLBACK, *PTDI_CALLBACK;


typedef int (*TDI_IOCTL_PFUNC) (IN PIRP pIrp, IN PIO_STACK_LOCATION pIrpStack, OUT PTDI_CALLBACK pCompletion, IN ULONG DeviceType);

typedef struct _TDI_IOCTL
{
	UCHAR					MinorFunction;
	PCHAR					Description;
	TDI_IOCTL_PFUNC			pfRoutine;

} TDI_IOCTL, PTDI_IOCTL;


BOOLEAN	 TDIDispatch(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, NTSTATUS *status);
NTSTATUS TDICreate(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp, IN PIO_STACK_LOCATION pIrpStack, OUT PTDI_CALLBACK pCompletion);
NTSTATUS InstallNetworkHooks(PDRIVER_OBJECT pDriverObject);
void	 RemoveNetworkHooks(PDRIVER_OBJECT pDriverObject);


#endif	/* __NETWORK_H__ */
