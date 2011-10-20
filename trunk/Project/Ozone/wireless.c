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
 *		Eugene Tsyrklevich 12-Oct-2004
 */


#include <NTDDK.h>

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) const GUID name	\
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#include <ntddstor.h>
#include <wdmguid.h>
#include "wireless.h"
#include "pathproc.h"
#include "policy.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitWirelessHooks)
#pragma alloc_text (PAGE, RemoveWirelessHooks)
#endif


PVOID	WirelessNotificationEntry = NULL;

/* removable wireless flags defined in drive.h (READONLY, etc) */
UCHAR	WirelessRemovableFlags = 0;


typedef struct _WORK_CONTEXT
{
	PIO_WORKITEM		Item;
	UNICODE_STRING		SymbolicLinkName;

} WORK_CONTEXT, *PWORK_CONTEXT;



/*
 * AddDrive()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		pusDriveName - .
 *		DriveLetter - .
 *
 * Returns:
 *		Nothing.
 */

VOID
AddDrive(PUNICODE_STRING pusDriveName, CHAR DriveLetter)
{
	PDEVICE_OBJECT			pDeviceObject;
	STORAGE_HOTPLUG_INFO	HotplugInfo;


	pDeviceObject = GetDriveHotplugInformation(pusDriveName, &HotplugInfo);
	if (pDeviceObject == NULL)
		return;


	LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("AddDrive: %c:\\ drive: %d %d %d %d %d\n", DriveLetter, HotplugInfo.Size, HotplugInfo.WirelessRemovable, HotplugInfo.WirelessHotplug, HotplugInfo.DeviceHotplug, HotplugInfo.WriteCacheEnableOverride));


	//XXX remove
	if (HotplugInfo.WirelessRemovable == FALSE && HotplugInfo.WirelessHotplug == TRUE)
	{
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("AddDrive: hotpluggable but not removable drive! %c: %S\n", DriveLetter, pusDriveName->Buffer));
	}

	if (HotplugInfo.WirelessRemovable)
	{
		CHAR			rule[MAX_PATH];


		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("AddDrive: removable drive! %c: %S\n", DriveLetter, pusDriveName->Buffer));


		/* Create a new global policy rule */

		if (IS_REMOVABLE_WIRELESS_DISABLED())
		{
			sprintf(rule, "name match \"%c:\\*\" then %s", DriveLetter, "deny");

			PolicyParseObjectRule(&gSecPolicy, RULE_FILE, "all", rule);

			/* no need to process other rules, this one denies everything already */
			return;
		}

		if (IS_REMOVABLE_WIRELESS_READONLY())
		{
			sprintf(rule, "name match \"%c:\\*\" then %s", DriveLetter, "deny");

			PolicyParseObjectRule(&gSecPolicy, RULE_FILE, "write", rule);
		}

		if (IS_REMOVABLE_WIRELESS_NOEXECUTE())
		{
			sprintf(rule, "name match \"%c:\\*\" then %s", DriveLetter, "deny");

			PolicyParseObjectRule(&gSecPolicy, RULE_FILE, "execute", rule);
		}
	}


	return;
}



/*
 * RemoveDrive()
 *
 * Description:
 *		.
 *
 * Parameters:
 *		pusDriveName - .
 *
 * Returns:
 *		Nothing.
 */

VOID
RemoveDrive(PUNICODE_STRING pusDriveName)
{
	PDEVICE_OBJECT			pDeviceObject;
	STORAGE_HOTPLUG_INFO	HotplugInfo;
	NTSTATUS				rc;


	pDeviceObject = GetDriveHotplugInformation(pusDriveName, &HotplugInfo);
	if (pDeviceObject == NULL)
		return;

//	KdPrint(("success %d %d %d %d %d\n", info.Size, info.WirelessRemovable, info.WirelessHotplug, info.DeviceHotplug, info.WriteCacheEnableOverride));

	if (HotplugInfo.WirelessRemovable)
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("RemoveDrive: removable drive! %S\n", pusDriveName->Buffer));


	rc = RtlVolumeDeviceToDosName(pDeviceObject, pusDriveName);
	if (NT_SUCCESS(rc))
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("RemoveDrive: IoVolumeDeviceToDosName returned %S\n", pusDriveName->Buffer));

	return;
}



/*
 * PnpWorker()
 *
 * Description:
 *		A work item routine that runs at PASSIVE_LEVEL irql. The routine is scheduled by PnpCallback
 *		which is not allowed to block.
 *
 *		PnpWorker calls RemoveDrive() with a drive name setup by PnpCallback.
 *
 * Parameters:
 *		pDeviceObject - .
 *		Context - .
 *
 * Returns:
 *		Nothing.
 */

VOID
PnpWorker(IN PDEVICE_OBJECT pDeviceObject, IN PVOID Context)
{
	PWORK_CONTEXT	WorkContext = (PWORK_CONTEXT) Context;


	if (WorkContext == NULL)
	{
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("PnpWorker: WorkContext = NULL\n"));
		return;
	}

	LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("PnpWorker: %S\n", WorkContext->SymbolicLinkName.Buffer));


	RemoveDrive(&WorkContext->SymbolicLinkName);


	IoFreeWorkItem(WorkContext->Item);

	ExFreePoolWithTag(WorkContext, _POOL_TAG);
}



/*
 * PnpCallback()
 *
 * Description:
 *		Plug-and-Play callback. Gets called when a p-n-p drive/cdrom interface is modified
 *		(i.e. when a removable drive is added or removed from the system).
 *
 * Parameters:
 *		NotificationStructure - DEVICE_INTERFACE_CHANGE_NOTIFICATION indicating which interface changed.
 *		Context - the driver's device context.
 *
 * Returns:
 *		STATUS_SUCCESS.
 */

NTSTATUS
PnpCallback(IN PVOID NotificationStructure, IN PVOID Context)
{
	PDEVICE_INTERFACE_CHANGE_NOTIFICATION	Notify = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION) NotificationStructure;
	PDEVICE_OBJECT							pDeviceObject = (PDEVICE_OBJECT) Context;
	PIO_WORKITEM							WorkItem;
	PWORK_CONTEXT							WorkContext;
 

	if (IsEqualGUID((LPGUID) &Notify->Event, (LPGUID) &GUID_DEVICE_INTERFACE_REMOVAL))
	{
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("GUID_DEVICE_INTERFACE_REMOVAL %S\n", Notify->SymbolicLinkName->Buffer));
/*
    }


	if (IsEqualGUID((LPGUID) &Notify->Event, (LPGUID) &GUID_DEVICE_INTERFACE_ARRIVAL) ||
		IsEqualGUID((LPGUID) &Notify->Event, (LPGUID) &GUID_DEVICE_INTERFACE_REMOVAL))
	{
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("GUID_DEVICE_INTERFACE_ARRIVAL %x %S\n", Notify->SymbolicLinkName, Notify->SymbolicLinkName->Buffer));
*/

		/*
		 * Schedule a work item to process this request. Cannot block in this callback function.
		 */

		WorkItem = IoAllocateWorkItem(pDeviceObject);

		WorkContext = (PWORK_CONTEXT) ExAllocatePoolWithTag(PagedPool,
															sizeof(WORK_CONTEXT) + Notify->SymbolicLinkName->Length,
															_POOL_TAG);
		if (!WorkContext)
		{
			IoFreeWorkItem(WorkItem);
			return(STATUS_SUCCESS);
		}

		WorkContext->SymbolicLinkName.Buffer = (PWSTR) ((PCHAR) &WorkContext->SymbolicLinkName + sizeof(UNICODE_STRING));
		WorkContext->SymbolicLinkName.MaximumLength = Notify->SymbolicLinkName->Length;
		WorkContext->SymbolicLinkName.Length = 0;

		WorkContext->Item = WorkItem;
		RtlCopyUnicodeString(&WorkContext->SymbolicLinkName, Notify->SymbolicLinkName);

		IoQueueWorkItem(WorkItem, PnpWorker, DelayedWorkQueue, WorkContext);
    }


	return STATUS_SUCCESS;
}



/*
 * InitWirelessHooks()
 *
 * Description:
 *		Process any existing wireless cards and register Plug-and-Play notifications for future drive additions/removals.
 *
 *		NOTE: Called once during driver initialization (DriverEntry()).
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		TRUE to indicate success, FALSE if failed.
 */

BOOLEAN
InitWirelessHooks(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pDeviceObject)
{
	NTSTATUS				rc;


	rc = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
										/*0,*/PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
										(LPGUID) &GUID_DEVINTERFACE_DISK,
										pDriverObject,
										PnpCallback,
										pDeviceObject,
										&WirelessNotificationEntry);

	if (! NT_SUCCESS(rc))
	{
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("InitWirelessHooks: IoRegisterPlugPlayNotification failed with status %x\n", rc));
        return FALSE;
	}


	return TRUE;
}



/*
 * RemoveWirelessHooks()
 *
 * Description:
 *		Unregister Plug-and-Play notifications.
 *
 * Parameters:
 *		None.
 *
 * Returns:
 *		Nothing.
 */

VOID
RemoveWirelessHooks()
{
	if (WirelessNotificationEntry)
		if (! NT_SUCCESS(IoUnregisterPlugPlayNotification(WirelessNotificationEntry)))
			LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("RemoveWirelessHooks: IoUnregisterPlugPlayNotification failed\n"));
}
