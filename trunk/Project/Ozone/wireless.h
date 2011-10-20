/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		wireless.c
 *
 * Abstract:
 *
 *		This module deals with wireless cards.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 02-Jun-2004
 */


#ifndef __WIRELESS_H__
#define __WIRELESS_H__


#define	WIRELESS_REMOVABLE_PERMIT			0
#define	WIRELESS_REMOVABLE_DISABLE			1
#define	WIRELESS_REMOVABLE_READONLY		2
#define	WIRELESS_REMOVABLE_NOEXECUTE		4

#define	IS_REMOVABLE_WIRELESS_READONLY()	((WirelessRemovableFlags & WIRELESS_REMOVABLE_READONLY) == WIRELESS_REMOVABLE_READONLY)
#define	IS_REMOVABLE_WIRELESS_DISABLED()	((WirelessRemovableFlags & WIRELESS_REMOVABLE_DISABLE) == WIRELESS_REMOVABLE_DISABLE)
#define	IS_REMOVABLE_WIRELESS_NOEXECUTE()	((WirelessRemovableFlags & WIRELESS_REMOVABLE_NOEXECUTE) == WIRELESS_REMOVABLE_NOEXECUTE)


extern UCHAR	WirelessRemovableFlags;


BOOLEAN InitWirelessHooks(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pDeviceObject);
VOID	RemoveRemovableWirelessHooks();
VOID	MonitorDriveLinks(const PCHAR Link);


#endif	/* __WIRELESS_H__ */
