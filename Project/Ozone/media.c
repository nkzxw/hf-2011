/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		media.c
 *
 * Abstract:
 *
 *		This module deals with removable media drives. Removable media drives can be made read-only
 *		or disabled altogether.
 *
 *		Addition of removable drives is detected by monitoring the creation new drive symbolic links.
 *		Drive removal is detected by receiving Plug-and-Play notification. 
 *
 *		Note: Plug-and-Play notifications are not used to detect addition of removable drives since
 *				drive letters are not yet assigned when notifications arrive.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 02-Jun-2004
 */


#include <NTDDK.h>

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) const GUID name	\
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#include <ntddstor.h>
#include <wdmguid.h>
#include "media.h"
#include "pathproc.h"
#include "policy.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InitRemovableMediaHooks)
#pragma alloc_text (PAGE, RemoveRemovableMediaHooks)
#endif


//XXX investigate IoRegisterFsRegistrationChange
//reference: www.pulsetrainsw.com/articlesitem.asp?id=3

PVOID	DiskNotificationEntry = NULL, CdromNotificationEntry = NULL;

/* removable media flags defined in drive.h (READONLY, etc) */
UCHAR	MediaRemovableFlags = 0;


typedef struct _WORK_CONTEXT
{
	PIO_WORKITEM		Item;
	UNICODE_STRING		SymbolicLinkName;

} WORK_CONTEXT, *PWORK_CONTEXT;



/*
 * AddDrive()
 *
 * Description:
 *		Create policy access rules for removable drives as they are added at runtime.
 *
 * Parameters:
 *		pusDriveName - .
 *		DriveLetter - Letter of the drive being added.
 *
 * Returns:
 *		Nothing.
 */

VOID
AddDrive(PUNICODE_STRING pusDriveName, CHAR DriveLetter)
{
	PDEVICE_OBJECT			pDeviceObject;
	PFILE_OBJECT			fo;
	NTSTATUS				rc;


	rc = IoGetDeviceObjectPointer(pusDriveName, FILE_READ_ATTRIBUTES, &fo, &pDeviceObject);
	if (! NT_SUCCESS(rc))
	{
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_VERBOSE, ("AddDrive: IoGetDeviceObjectPointer failed with status %x\n", rc));
		return;
	}


	LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("AddDrive(%S): %d\n", pusDriveName->Buffer, pDeviceObject->Characteristics));


	if (pDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
	{
		CHAR			rule[MAX_PATH];


		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("AddDrive: removable drive! %c: %S\n", DriveLetter, pusDriveName->Buffer));


		/* Create a new global policy rule */

		if (IS_REMOVABLE_MEDIA_DISABLED())
		{
			sprintf(rule, "name match \"%c:\\*\" then %s", DriveLetter, "deny");

			PolicyParseObjectRule(&gSecPolicy, RULE_FILE, "all", rule);

			/* no need to process other rules, this one denies everything already */
			return;
		}

		if (IS_REMOVABLE_MEDIA_READONLY() /*&& !(pDeviceObject->Characteristics & FILE_READ_ONLY_DEVICE)*/)
		{
			sprintf(rule, "name match \"%c:\\*\" then %s", DriveLetter, "deny");

			PolicyParseObjectRule(&gSecPolicy, RULE_FILE, "write", rule);
		}

		if (IS_REMOVABLE_MEDIA_NOEXECUTE())
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
	PFILE_OBJECT			fo;
	NTSTATUS				rc;


	rc = IoGetDeviceObjectPointer(pusDriveName, FILE_READ_ATTRIBUTES, &fo, &pDeviceObject);
	if (! NT_SUCCESS(rc))
	{
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("RemoveDrive: IoGetDeviceObjectPointer failed with status %x\n", rc));
		return;
	}


	if (pDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("RemoveDrive: removable drive! %S\n", pusDriveName->Buffer));


//	rc = RtlVolumeDeviceToDosName(pDeviceObject, pusDriveName);
//	if (NT_SUCCESS(rc))
//		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("RemoveDrive: IoVolumeDeviceToDosName returned %S\n", pusDriveName->Buffer));
}



/*
 * MonitorDriveLinks()
 *
 * Description:
 *		This routine is called from HookedNtCreateSymbolicLinkObject whenever a new symbolic link is created.
 *		ProcessDriveLink is responsible for monitoring the creation of any new media drives. When a new removable
 *		media drive is added to a system, a new symbolic link "\GLOBAL??\X:" is created (where X is the
 *		assigned drive letter).
 *
 *		Note: We are not interested in creation of "fake" drives (i.e. subst c: z:\) since those will be
 *				correctly resolved by symbolic link name resolution code (policy enforcement). "Fake" drives
 *				are added in userland and come up as "\??\X:".
 *
 * Parameters:
 *		Link - created symbolic link name.
 *
 * Returns:
 *		Nothing.
 */

VOID
MonitorDriveLinks(const PCHAR Link)
{
	CHAR			DriveLetter;
	ANSI_STRING		AnsiString;
	UNICODE_STRING	UnicodeString;
	int				len = strlen(Link);


	/* match \GLOBAL??\Z: && \??\Z: drive references */
	if (len == 12 && (strncmp(Link, "\\GLOBAL??\\", 10) == 0) && isalpha(Link[10]) && Link[11] == ':')
	{
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("MonitorDriveLinks: matched \\global??\\. letter %c\n", Link[10]));

		DriveLetter = Link[10];
	}
	else if (len == 6 && (strncmp(Link, "\\??\\", 4) == 0) && isalpha(Link[4]) && Link[5] == ':')
	{
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("MonitorDriveLinks: matched \\??\\. letter %c\n", Link[4]));

		DriveLetter = Link[4];
	}
	else
	{
		return;
	}


	RtlInitAnsiString(&AnsiString, Link);

	if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE)))
	{
		AddDrive(&UnicodeString, DriveLetter);

		RtlFreeUnicodeString(&UnicodeString);
	}
	else
	{
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("MonitorDriveLinks: failed to convert %s\n", Link));
	}
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
 * InitRemovableMediaHooks()
 *
 * Description:
 *		Process any existing drives and register Plug-and-Play notifications for future drive additions/removals.
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
InitRemovableMediaHooks(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pDeviceObject)
{
	UNICODE_STRING			Name;
	WCHAR					Buffer[MAX_PATH] = L"\\??\\A:";
	NTSTATUS				rc;
	HANDLE					hLink;
	OBJECT_ATTRIBUTES		oa;
	char					DriveLetter;


	for (DriveLetter = 'A'; DriveLetter <= 'Z'; DriveLetter++)
	{
		Buffer[4] = DriveLetter;

		RtlInitUnicodeString(&Name, Buffer);

		AddDrive(&Name, DriveLetter);
	}


	rc = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
										0,//PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
										(LPGUID) &GUID_DEVINTERFACE_DISK,
										pDriverObject,
										PnpCallback,
										pDeviceObject,
										&DiskNotificationEntry);

	if (! NT_SUCCESS(rc))
	{
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("InitRemovableMediaHooks: IoRegisterPlugPlayNotification failed with status %x\n", rc));
        return FALSE;
	}


	rc = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
										0,//PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
										(LPGUID) &GUID_DEVINTERFACE_CDROM,
										pDriverObject,
										PnpCallback,
										pDeviceObject,
										&CdromNotificationEntry);

	if (! NT_SUCCESS(rc))
	{
		LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("InitRemovableMediaHooks: IoRegisterPlugPlayNotification2 failed with status %x\n", rc));
        return FALSE;
	}


	return TRUE;
}



/*
 * RemoveRemovableMediaHooks()
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
RemoveRemovableMediaHooks()
{
	if (DiskNotificationEntry)
		if (! NT_SUCCESS(IoUnregisterPlugPlayNotification(DiskNotificationEntry)))
			LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("RemoveRemovableMediaHooks: IoUnregisterPlugPlayNotification failed\n"));

	if (CdromNotificationEntry)
		if (! NT_SUCCESS(IoUnregisterPlugPlayNotification(CdromNotificationEntry)))
			LOG(LOG_SS_DRIVE, LOG_PRIORITY_DEBUG, ("RemoveRemovableMediaHooks: IoUnregisterPlugPlayNotification2 failed\n"));
}
