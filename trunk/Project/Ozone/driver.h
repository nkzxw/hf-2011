/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		driver.h
 *
 * Abstract:
 *
 *		This module defines various types used by the driver "plumbing" code.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 9-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#ifndef __DRIVER_H__
#define __DRIVER_H__



typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT	pDeviceObject;
	UNICODE_STRING	usSymLink;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


#define	DEVICE_NAME			L"\\Device\\Ozone"
#define DEVICE_SYMLINK_NAME	L"\\??\\Ozone"


#define	IOCTL_REGISTER_AGENT_SERVICE			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IOCTL_GET_ALERT							CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IOCTL_GET_USERLAND_REQUEST				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IOCTL_SEND_USERLAND_SID_RESOLVE_REPLY	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IOCTL_SEND_USERLAND_ASK_USER_REPLY		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IOCTL_START_CREATE_POLICY				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define	IOCTL_STOP_CREATE_POLICY				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)


// Build 23
#define	DRIVER_VERSION		0x00000023


extern BOOLEAN	ActiveUserAgent;


VOID		DriverUnload(IN PDRIVER_OBJECT	pDriverObject);
NTSTATUS	DriverCreate(IN PDEVICE_OBJECT	pDeviceObject, IN PIRP	pIrp);
NTSTATUS	DriverCleanup(IN PDEVICE_OBJECT	pDeviceObject, IN PIRP	pIrp);
NTSTATUS	DriverClose	(IN PDEVICE_OBJECT	pDeviceObject, IN PIRP	pIrp);
NTSTATUS	DriverRead	(IN PDEVICE_OBJECT	pDeviceObject, IN PIRP	pIrp);
NTSTATUS	DriverWrite	(IN PDEVICE_OBJECT	pDeviceObject, IN PIRP	pIrp);
NTSTATUS	DriverDeviceControl(IN PDEVICE_OBJECT	pDeviceObject, IN PIRP	pIrp);


/*
 HRSRC   hRsrc;
 HGLOBAL hDriverResource;
 DWORD   dwDriverSize;
 LPVOID  lpvDriver;
 HFILE   hfTempFile;
 •
 •
 •

 hRsrc = FindResource(hInst,MAKEINTRESOURCE(MSJDATNT),"BINRES");

 hDriverResource = LoadResource(hInst, hRsrc);
 dwDriverSize = SizeofResource(hInst, hRsrc);
 lpvDriver = LockResource(hDriverResource);

 hfTempFile = _lcreat("msj.tmp",0);
 _hwrite(hfTempFile, lpvDriver, dwDriverSize);
 _lclose(hfTempFile);


 http://www.microsoft.com/MSJ/0398/DRIVER.aspx

 */


#endif	/* __DRIVER_H__ */