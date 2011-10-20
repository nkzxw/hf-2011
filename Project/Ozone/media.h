/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		media.c
 *
 * Abstract:
 *
 *		This module deals with removable media drives.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 02-Jun-2004
 */


#ifndef __MEDIA_H__
#define __MEDIA_H__


#define	MEDIA_REMOVABLE_PERMIT			0
#define	MEDIA_REMOVABLE_DISABLE			1
#define	MEDIA_REMOVABLE_READONLY		2
#define	MEDIA_REMOVABLE_NOEXECUTE		4

#define	IS_REMOVABLE_MEDIA_READONLY()	((MediaRemovableFlags & MEDIA_REMOVABLE_READONLY) == MEDIA_REMOVABLE_READONLY)
#define	IS_REMOVABLE_MEDIA_DISABLED()	((MediaRemovableFlags & MEDIA_REMOVABLE_DISABLE) == MEDIA_REMOVABLE_DISABLE)
#define	IS_REMOVABLE_MEDIA_NOEXECUTE()	((MediaRemovableFlags & MEDIA_REMOVABLE_NOEXECUTE) == MEDIA_REMOVABLE_NOEXECUTE)


extern UCHAR	MediaRemovableFlags;


BOOLEAN InitRemovableMediaHooks(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pDeviceObject);
VOID	RemoveRemovableMediaHooks();
VOID	MonitorDriveLinks(const PCHAR Link);


#endif	/* __MEDIA_H__ */
