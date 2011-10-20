/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		driver.c
 *
 * Abstract:
 *
 *		This module implements all the device driver "plumbing" (DriverEntry, etc).
 *
 * Author:
 *
 *		Eugene Tsyrklevich 9-Feb-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include <devioctl.h>

#include "driver.h"
//#include "eugene.h"
#include "file.h"
#include "registry.h"
#include "sysinfo.h"
#include "policy.h"
#include "process.h"
#include "learn.h"
#include "event.h"
#include "semaphore.h"
#include "dirobj.h"
#include "symlink.h"
#include "mutant.h"
#include "port.h"
#include "timer.h"
#include "token.h"
#include "job.h"
#include "driverobj.h"
#include "network.h"
#include "section.h"
#include "atom.h"
#include "time.h"
#include "vdm.h"
#include "procname.h"
#include "userland.h"
#include "media.h"
#include "boprot.h"
#include "debug.h"
#include "i386.h"
#include "misc.h"
#include "log.h"


LONG	SysenterEip = 0;


__declspec(naked)
VOID
SysenterHandler()
{
	_asm	and ecx, 0x000000FF
	_asm	sub	esp, ecx
	_asm	jmp	[SysenterEip]
}


VOID
blah()
{
	LONG	c, s;

#define SYSENTER_CS_MSR			0x174
#define SYSENTER_ESP_MSR		0x175
#define SYSENTER_EIP_MSR		0x176

	_asm
	{
		mov	ecx, SYSENTER_CS_MSR
		rdmsr

		mov	c, eax


		mov	ecx, SYSENTER_ESP_MSR
		rdmsr

		mov	s, eax


		mov	ecx, SYSENTER_EIP_MSR
		rdmsr

		mov SysenterEip, eax

		mov eax, SysenterHandler
	
		wrmsr
	}

	KdPrint(("old eip=%x:%x (%x), new eip=%x\n", c, SysenterEip, s, SysenterHandler));
}



/*
 * DriverEntry()
 *
 * Description:
 *		Driver entry point.
 *
 * Parameters:
 *		pDriverObject - pointer to an initialized driver object that represents our driver
 *		pRegistryPath - name of the service key in the registry
 *
 * Returns:
 *		STATUS_SUCCESS to indicate success or an error code to indicate an error.
 */

/* macro shortcut for bailing out of DriverEntry in case of an error */

#define	ABORT_DriverEntry(msg)												\
		{																	\
			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_CRITICAL, (msg));		\
			if (irql != PASSIVE_LEVEL) KeLowerIrql(irql);					\
			DriverUnload(pDriverObject);									\
			return status;													\
		}
/*
PVOID Find_Kernel32_Base();
NTSTATUS
NTAPI
ZwCreateSymbolicLinkObject(
	OUT PHANDLE SymbolicLinkHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN PUNICODE_STRING TargetName
	);
		HANDLE				h;
*/
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	UNICODE_STRING		usDeviceName, usSymLinkName;
	PDEVICE_OBJECT		pDeviceObject;
	PDEVICE_EXTENSION	pDeviceExtension;
	NTSTATUS			status;
	KIRQL				irql = KeGetCurrentIrql();
	int					i;

/*
	{
		OBJECT_ATTRIBUTES	ObjectAttributes;
		UNICODE_STRING		dest, target;
		NTSTATUS			status;

		RtlInitUnicodeString(&dest, L"\\??\\MyRegistryMachine");
		RtlInitUnicodeString(&target, L"\\Registry\\Machine");

		InitializeObjectAttributes(&ObjectAttributes, &dest, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

		status = ZwCreateSymbolicLinkObject(&h, SYMBOLIC_LINK_ALL_ACCESS, &ObjectAttributes, &target);
		if (! NT_SUCCESS(status))
		{
			KdPrint(("failed, status %x\n", status));
		}
		else
		{
			KdPrint(("link ok\n"));
		}

//		return STATUS_UNSUCCESSFUL;
	}
*/
	//XXX add pRegistryPath to deny registry access rule?!

	__try
	{
		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverEntry: Entered (%x %S)\n", pDriverObject, pRegistryPath->Buffer));


//		blah();
//		KdPrint(("after blah\n"));


		/*
		 * Verify we are running on x86 & everything is in order
		 */

		if (!InitI386())
		{
			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_CRITICAL, ("InitI386 failed. Aborting.\n"));
			return STATUS_UNSUCCESSFUL;
		}


		/*
		 * Initialize all the driver object related data and create a device representing our device
		 */

		// set to NULL to disable unload by admins
//		pDriverObject->DriverUnload = NULL;
		pDriverObject->DriverUnload = DriverUnload;

//XXX need to intercept DriverObject->FastIoDispatch = &VTrcFSFastIoDispatchTable;

		for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
			//XXX don't intercept IRP_MJ_POWER & IRP_MJ_PNP
			pDriverObject->MajorFunction[i] = DriverDeviceControl;

		pDriverObject->MajorFunction[ IRP_MJ_CREATE ] = DriverCreate;
		pDriverObject->MajorFunction[ IRP_MJ_CLEANUP ] = DriverCleanup;
		pDriverObject->MajorFunction[ IRP_MJ_CLOSE ] = DriverClose;
		pDriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] = DriverDeviceControl;
/*
		pDriverObject->MajorFunction[ IRP_MJ_READ ] = DriverRead;
		pDriverObject->MajorFunction[ IRP_MJ_WRITE ] = DriverWrite;
 */
		RtlInitUnicodeString(&usDeviceName, DEVICE_NAME);

		status = IoCreateDevice(pDriverObject, sizeof(DEVICE_EXTENSION),
								&usDeviceName, FILE_DEVICE_UNKNOWN,
								FILE_DEVICE_SECURE_OPEN, FALSE,//FALSE (Exclusive - Reserved for system use. Drivers set this parameter to FALSE.)
//								0, TRUE,
								&pDeviceObject);

		if (!NT_SUCCESS(status))
		{
			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverEntry: IoCreateDevice failed with status %x\n", status));

			return status;
		}


		pDeviceObject->Flags |= DO_BUFFERED_IO;

		pDeviceExtension = (PDEVICE_EXTENSION) pDeviceObject->DeviceExtension;
		pDeviceExtension->pDeviceObject = pDeviceObject;


		RtlInitUnicodeString(&pDeviceExtension->usSymLink, DEVICE_SYMLINK_NAME);

		status = IoCreateSymbolicLink(&pDeviceExtension->usSymLink, &usDeviceName);
		if (!NT_SUCCESS(status))
		{
			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverEntry: IoCreateSymbolicLink failed with status %x\n", status));

			IoDeleteDevice(pDeviceObject);

			return status;
		}


		/*
		 * Now, mediate all the necessary calls
		 */

#if HOOK_NETWORK
		status = InstallNetworkHooks(pDriverObject);
		if (! NT_SUCCESS(status))
			ABORT_DriverEntry("InstallNetworkHooks() failed");
#endif


		/* all consequitive calls that fail will cause the following error to be returned */

		status = STATUS_DRIVER_INTERNAL_ERROR;

		if (!InitSyscallsHooks())
			ABORT_DriverEntry("InitSyscallsHooks() failed\n");


		/*
		 * raise irql to DPC level to avoid any spurious system calls taking place before we
		 * manage to initialize appropriate hooked function pointers
		 */

		irql = KeRaiseIrqlToDpcLevel();


		if (!InstallSyscallsHooks())
			ABORT_DriverEntry("InstallSyscallsHooks() failed\n");


#if HOOK_FILE
		if (!InitFileHooks())
			ABORT_DriverEntry("InitFileHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitFileHooks\n"));
#endif

#if HOOK_REGISTRY
		if (!InitRegistryHooks())
			ABORT_DriverEntry("InitRegistryHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitRegistryHooks\n"));
#endif

#if HOOK_SECTION
		if (!InitSectionHooks())
			ABORT_DriverEntry("InitSectionHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitSectionHooks\n"));
#endif

#if HOOK_SYSINFO
		if (!InitSysInfoHooks())
			ABORT_DriverEntry("InitSysInfoHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitSysInfoHooks\n"));
#endif

#if HOOK_EVENT
		if (!InitEventHooks())
			ABORT_DriverEntry("InitEventHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitEventHooks\n"));
#endif

#if HOOK_SEMAPHORE
		if (!InitSemaphoreHooks())
			ABORT_DriverEntry("InitSemaphoreHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitSemaphoreHooks\n"));
#endif

#if HOOK_JOB
		if (!InitJobHooks())
			ABORT_DriverEntry("InitJobHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitJobHooks\n"));
#endif

#if HOOK_MUTANT
		if (!InitMutantHooks())
			ABORT_DriverEntry("InitMutantHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitMutantHooks\n"));
#endif

#if HOOK_DIROBJ
		if (!InitDirobjHooks())
			ABORT_DriverEntry("InitDirobjHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitDirobjHooks\n"));
#endif

#if HOOK_PORT
		if (!InitPortHooks())
			ABORT_DriverEntry("InitPortHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitPortHooks\n"));
#endif

#if HOOK_SYMLINK
		if (!InitSymlinkHooks())
			ABORT_DriverEntry("InitSymlinkHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitSymlinkHooks\n"));
#endif

#if HOOK_TIMER
		if (!InitTimerHooks())
			ABORT_DriverEntry("InitTimerHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitTimerHooks\n"));
#endif

#if HOOK_TOKEN
		if (!InitTokenHooks())
			ABORT_DriverEntry("InitTokenHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitTokenHooks\n"));
#endif

#if HOOK_TIME
		if (!InitTimeHooks())
			ABORT_DriverEntry("InitTimeHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitTimeHooks\n"));
#endif

#if HOOK_DRIVEROBJ
		if (!InitDriverObjectHooks())
			ABORT_DriverEntry("InitDriverObjectHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitDriverObjectHooks\n"));
#endif

#if HOOK_ATOM
		if (!InitAtomHooks())
			ABORT_DriverEntry("InitAtomHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitAtomHooks\n"));
#endif

#if HOOK_VDM
		if (!InitVdmHooks())
			ABORT_DriverEntry("InitVdmHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitVdmHooks\n"));
#endif


#if HOOK_DEBUG
		if (!InitDebugHooks())
			ABORT_DriverEntry("InitDebugHooks() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitDebugHooks\n"));
#endif


		KeLowerIrql(irql);


		/*
		 * The order of the following calls is important:
		 *
		 * InitProcessEntries() initializes OzoneInstallPath
		 * InitPolicy() initiailizes policy related variables
		 * InitProcessNameEntries() then uses policy vars & OzoneInstallPath to load policies
		 */

#if HOOK_PROCESS
		if (!InitProcessEntries())
			ABORT_DriverEntry("InitProcessEntries() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitProcessEntries\n"));
#endif


		if (!InitProcessNameEntries())
			ABORT_DriverEntry("InitProcessNameEntries() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitProcessNameEntries\n"));


		if (!InitPolicy())
			ABORT_DriverEntry("InitPolicy() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitPolicy\n"));


		EnumerateExistingProcesses();

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past EnumerateExistingProcesses\n"));



		if (LearningMode == TRUE)
		{
			if (!InitLearningMode())
				ABORT_DriverEntry("InitLearningMode() failed\n");

			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitLearningMode\n"));
		}


#if HOOK_MEDIA
		if (!InitRemovableMediaHooks(pDriverObject, pDeviceObject))
			ABORT_DriverEntry("InitRemovableMedia() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitRemovableMediaHooks\n"));
#endif


#if HOOK_BOPROT
		if (!InitBufferOverflowProtection())
			ABORT_DriverEntry("InitBufferOverflowProtection() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitBufferOverflowProtection\n"));
#endif


		if (!InitLog())
			ABORT_DriverEntry("InitLog() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitLog\n"));


		if (!InitUserland())
			ABORT_DriverEntry("InitUserland() failed\n");

		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverEntry: Past InitUserland\n"));

	} // __try

	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		NTSTATUS status = GetExceptionCode();
		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_CRITICAL, ("DriverEntry: caught an exception. status = 0x%x\n", status));

		return STATUS_DRIVER_INTERNAL_ERROR;
	}


	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverEntry: Done\n"));


	return STATUS_SUCCESS;
}



/*
 * DriverUnload()
 *
 * Description:
 *		Clean up and unload the driver.
 *
 *		NOTE: Since this driver mediates system calls and other devices, it is not safe to
 *		unload the driver since there might remain outstanding references to our driver code and data
 *		segments once the driver is unloaded.
 *
 *		NOTE2: In release builds, unload functionality should be disabled for security reasons.
 *
 * Parameters:
 *		pDriverObject - pointer to a driver object that represents this driver.
 *
 * Returns:
 *		Nothing.
 */

VOID
DriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT		pDeviceObject, pNextDeviceObject;
	PDEVICE_EXTENSION	pDeviceExtension;
	LARGE_INTEGER		delay;


#if DBG
	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverUnload: irql = %d %d\n", KeGetCurrentIrql(), HookedTDIRunning));
#endif


	if (SysenterEip)
	_asm
	{
		mov	ecx, SYSENTER_EIP_MSR
		mov eax, SysenterEip
		xor edx, edx	
		wrmsr
	}

#if HOOK_BOPROT
	ShutdownBufferOverflowProtection();
	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverUnload: Past ShutdownBufferOverflowProtection\n"));
#endif


	RemoveRemovableMediaHooks();
	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverUnload: Past RemoveRemovableMediaHooks\n"));

	RemoveNetworkHooks(pDriverObject);
	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverUnload: Past RemoveNetworkHooks\n"));

	RemoveSyscallsHooks();
	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverUnload: Past RemoveSyscallsHooks\n"));

	RemoveProcessNameEntries();
	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverUnload: Past RemoveProcessNameEntries\n"));


	if (LearningMode)
		ShutdownLearningMode();
	else
		PolicyRemove();

	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverUnload: Past LearningMode\n"));


	ShutdownLog();
	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverUnload: Past ShutdownLog\n"));


	ShutdownUserland();
	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverUnload: Past UserlandShutdown\n"));


	pDeviceObject = pNextDeviceObject = pDriverObject->DeviceObject;

	while (pNextDeviceObject != NULL)
	{
		pNextDeviceObject = pDeviceObject->NextDevice;
		pDeviceExtension = (PDEVICE_EXTENSION) pDeviceObject->DeviceExtension;

		if (pDeviceExtension)
		{
			NTSTATUS	status;

			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverUnload: IoDeleteSymbolicLink(%S)\n", pDeviceExtension->usSymLink.Buffer));

			status = IoDeleteSymbolicLink(&pDeviceExtension->usSymLink);
			if (! NT_SUCCESS(status))
				LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverUnload: IoDeleteSymbolicLink failed: %x\n", status));
		}
		else
			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverUnload: pDeviceExtension = NULL\n"));

		IoDeleteDevice(pDeviceObject);
		pDeviceObject = pNextDeviceObject;
	}


	/* wait for 1 second for all timers/callbacks to complete */
	delay.QuadPart = SECONDS(1);

	KeDelayExecutionThread(KernelMode, FALSE, &delay);


#if DBG
	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverUnload: HookedRoutineRunning = %d\n", HookedRoutineRunning));
#endif


	return;
}



/*
 * DriverDeviceControl()
 *
 * Description:
 *		Dispatch routine. Process network (TDI) and our driver requests.
 *
 * Parameters:
 *		pDeviceObject - pointer to a device object that a request is being sent to.
 *		pIrp - IRP (I/O Request Packet) request.
 *
 * Returns:
 *		Nothing.
 */

#define	COMPLETE_REQUEST(irp, status)				\
		pIrp->IoStatus.Status = (status);			\
		IoCompleteRequest((irp), IO_NO_INCREMENT);	\
		return((status));

NTSTATUS
DriverDeviceControl(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PIO_STACK_LOCATION	pIrpStack;
	ULONG				ControlCode;
	NTSTATUS			status;
	ULONG				InSize, OutSize;
	KIRQL				irql;


	if (pDeviceObject == NULL || pIrp == NULL)
	{
		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: NULL value %x %x\n", pDeviceObject, pIrp));

		COMPLETE_REQUEST(pIrp, STATUS_UNSUCCESSFUL);
	}


#if HOOK_NETWORK
	if (TDIDispatch(pDeviceObject, pIrp, &status) == TRUE)
	{
		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverDeviceControl(%x, %x): TDIDispatch\n", pDeviceObject, pIrp));
		return status;
	}
#endif


	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

	pIrp->IoStatus.Information = 0;


	ControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	InSize = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	OutSize = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (ControlCode)
	{
		/*
		 * When userland agent service starts up, it registers with the driver using IOCTL_REGISTER_AGENT_SERVICE.
		 * Expects back 1 ULONG - version of the driver
		 */

		// XXX save agent pid and version?
		case IOCTL_REGISTER_AGENT_SERVICE:
		{
			ULONG	DriverVersion = DRIVER_VERSION;


			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_REGISTER_AGENT_SERVICE ControlCode=%x InBufferSize=%x OutBufferSize=%x\n", ControlCode, InSize, OutSize));


			if (OutSize < sizeof(ULONG))
			{
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}


			RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, &DriverVersion, sizeof(ULONG));

			ActiveUserAgent = TRUE;


			pIrp->IoStatus.Information = sizeof(ULONG);

			status = STATUS_SUCCESS;


			break;
		}


		/*
		 * Userland agent service retrieves log alerts using IOCTL_GET_ALERT
		 */

		case IOCTL_GET_ALERT:
		{
			PSECURITY_ALERT		TmpAlert;


			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_GET_ALERT ControlCode=%x InBufferSize=%x OutBufferSize=%x\n", ControlCode, InSize, OutSize));


//			if (UserAgentRegistered == FALSE)
//				XXX;

			KeAcquireSpinLock(&gLogSpinLock, &irql);
			{
				if (LogList == NULL)
				{
					LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_GET_ALERT No More Alerts\n"));

					status = STATUS_NO_MORE_ENTRIES;

					KeReleaseSpinLock(&gLogSpinLock, irql);

					break;
				}

				/* don't count the size of the Next pointer */
				LogList->Size -= sizeof(struct _SECURITY_ALERT	*);

				if (OutSize < LogList->Size)
				{
					LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_GET_ALERT %d < %d\n", OutSize, LogList->Size));
					status = STATUS_INVALID_BUFFER_SIZE;
					KeReleaseSpinLock(&gLogSpinLock, irql);
					break;
				}

				/* copy the SECURITY_ALERT structure without including the Next pointer */
				RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, (PCHAR)LogList + sizeof(struct _SECURITY_ALERT	*), LogList->Size);

				pIrp->IoStatus.Information = LogList->Size;


				--NumberOfAlerts;

				TmpAlert = LogList;
				LogList = LogList->Next;

				ExFreePoolWithTag(TmpAlert, _POOL_TAG);
			}
			KeReleaseSpinLock(&gLogSpinLock, irql);


			status = STATUS_SUCCESS;

			break;
		}


		/*
		 * Userland agent service retrieves userland requests using IOCTL_GET_USERLAND_REQUEST
		 */

		case IOCTL_GET_USERLAND_REQUEST:
		{
			PUSERLAND_REQUEST_HEADER	TmpRequest;


			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_GET_USERLAND_REQUEST ControlCode=%x InBufferSize=%x OutBufferSize=%x\n", ControlCode, InSize, OutSize));


			KeAcquireSpinLock(&gUserlandRequestListSpinLock, &irql);
			{
				USHORT		UserlandRequestSize;


				if (UserlandRequestList == NULL)
				{
					LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_GET_USERLAND_REQUEST No More Process Requests\n"));

					status = STATUS_NO_MORE_ENTRIES;

					KeReleaseSpinLock(&gUserlandRequestListSpinLock, irql);

					break;
				}


				/* don't count the size of the Next pointer */
				UserlandRequestSize = UserlandRequestList->RequestSize - sizeof(struct _USERLAND_REQUEST *);

				if (OutSize < UserlandRequestSize)
				{
					LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_GET_USERLAND_REQUEST %d < %d\n", OutSize, UserlandRequestSize));

					KeReleaseSpinLock(&gUserlandRequestListSpinLock, irql);

					status = STATUS_INVALID_BUFFER_SIZE;

					break;
				}

				/* copy the PROCESS_REQUEST structure without including the Next pointer */
				RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, (PCHAR)UserlandRequestList + sizeof(struct _USERLAND_REQUEST *), UserlandRequestSize);

				pIrp->IoStatus.Information = UserlandRequestSize;


				TmpRequest = UserlandRequestList;
				UserlandRequestList = UserlandRequestList->Next;

				ExFreePoolWithTag(TmpRequest, _POOL_TAG);
			}
			KeReleaseSpinLock(&gUserlandRequestListSpinLock, irql);


			status = STATUS_SUCCESS;

			break;
		}


		/*
		 * Userland agent service returns userland replies using IOCTL_SEND_USERLAND_SID_RESOLVE_REPLY
		 */

#define	MAXIMUM_USERLAND_REPLY_SIZE	512

		case IOCTL_SEND_USERLAND_SID_RESOLVE_REPLY:
		{
			PSID_RESOLVE_REPLY		pSidResolveReply;
			PIMAGE_PID_ENTRY		ProcessEntry;


			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_SEND_USERLAND_SID_RESOLVE_REPLY ControlCode=%x InBufferSize=%x OutBufferSize=%x\n", ControlCode, InSize, OutSize));


			if (InSize > MAXIMUM_USERLAND_REPLY_SIZE)
			{
				LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_SEND_USERLAND_SID_RESOLVE_REPLY %d > %d\n", InSize, MAXIMUM_USERLAND_REPLY_SIZE));
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}

			pSidResolveReply = ExAllocatePoolWithTag(PagedPool, InSize, _POOL_TAG);
			if (pSidResolveReply == NULL)
			{
				LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_SEND_USERLAND_SID_RESOLVE_REPLY out of memory\n"));
				status = STATUS_UNSUCCESSFUL;
				break;
			}


			RtlCopyMemory(pSidResolveReply, pIrp->AssociatedIrp.SystemBuffer, InSize);

			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: Received sid resolve reply. insize=%d seq=%d, %S\n", InSize, pSidResolveReply->ReplyHeader.SeqId, pSidResolveReply->UserName));

			ProcessEntry = FindImagePidEntry(pSidResolveReply->ReplyHeader.ProcessId, 0);

			if (ProcessEntry)
			{
				if (ProcessEntry->WaitingForUserRequestId == 0)
				{
					LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: Process (pid=%d) is not expecting a user request!\n", pSidResolveReply->ReplyHeader.ProcessId));
					ExFreePoolWithTag(pSidResolveReply, _POOL_TAG);
					ProcessEntry->UserlandReply = NULL;
					break;
				}

				if (ProcessEntry->WaitingForUserRequestId != pSidResolveReply->ReplyHeader.SeqId)
				{
					LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: Process (pid=%d) is expecting to receive sequence id %d. Got %d\n", pSidResolveReply->ReplyHeader.ProcessId, ProcessEntry->WaitingForUserRequestId, pSidResolveReply->ReplyHeader.SeqId));
					ExFreePoolWithTag(pSidResolveReply, _POOL_TAG);
					ProcessEntry->UserlandReply = NULL;
					break;
				}


				/* deliver the reply */
				LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: Waking up process %d\n", pSidResolveReply->ReplyHeader.ProcessId));
				
				ProcessEntry->UserlandReply = (PUSERLAND_REPLY_HEADER) pSidResolveReply;
				
				KeSetEvent(&ProcessEntry->UserlandRequestDoneEvent, IO_NO_INCREMENT, FALSE);
			}
			else
				LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: cannot find process with pid=%d\n", pSidResolveReply->ReplyHeader.ProcessId));


			status = STATUS_SUCCESS;

			break;
		}


		/*
		 * Userland agent service returns "ask user" replies using IOCTL_SEND_USERLAND_ASK_USER_REPLY
		 */

		case IOCTL_SEND_USERLAND_ASK_USER_REPLY:
		{
			PASK_USER_REPLY			pAskUserReply;
			PIMAGE_PID_ENTRY		ProcessEntry;


			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_SEND_USERLAND_ASK_USER_REPLY ControlCode=%x InBufferSize=%x OutBufferSize=%x\n", ControlCode, InSize, OutSize));


			if (InSize != sizeof(ASK_USER_REPLY))
			{
				LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_SEND_USERLAND_ASK_USER_REPLY %d != %d\n", InSize, sizeof(ASK_USER_REPLY)));
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}

			pAskUserReply = ExAllocatePoolWithTag(PagedPool, sizeof(ASK_USER_REPLY), _POOL_TAG);
			if (pAskUserReply == NULL)
			{
				LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_SEND_USERLAND_ASK_USER_REPLY out of memory\n"));
				status = STATUS_UNSUCCESSFUL;
				break;
			}


			RtlCopyMemory(pAskUserReply, pIrp->AssociatedIrp.SystemBuffer, InSize);

			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: Received ask user reply. insize=%d, action=%d\n", InSize, pAskUserReply->Action));

			ProcessEntry = FindImagePidEntry(pAskUserReply->ReplyHeader.ProcessId, 0);

			if (ProcessEntry)
			{
				if (ProcessEntry->WaitingForUserRequestId == 0)
				{
					LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: Process (pid=%d) is not expecting a user request!\n", pAskUserReply->ReplyHeader.ProcessId));
					ExFreePoolWithTag(pAskUserReply, _POOL_TAG);
					ProcessEntry->UserlandReply = NULL;
					break;
				}

				if (ProcessEntry->WaitingForUserRequestId != pAskUserReply->ReplyHeader.SeqId)
				{
					LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: Process (pid=%d) is expecting to receive sequence id %d. Got %d\n", pAskUserReply->ReplyHeader.ProcessId, ProcessEntry->WaitingForUserRequestId, pAskUserReply->ReplyHeader.SeqId));
					ExFreePoolWithTag(pAskUserReply, _POOL_TAG);
					ProcessEntry->UserlandReply = NULL;
					break;
				}


				/* deliver the reply */
				LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: Waking up process %d\n", pAskUserReply->ReplyHeader.ProcessId));
				
				ProcessEntry->UserlandReply = (PUSERLAND_REPLY_HEADER) pAskUserReply;

				KeSetEvent(&ProcessEntry->UserlandRequestDoneEvent, IO_NO_INCREMENT, FALSE);
			}
			else
				LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("cannot find process with pid=%d\n", pAskUserReply->ReplyHeader.ProcessId));


			status = STATUS_SUCCESS;

			break;
		}


		/*
		 * train.exe puts the driver in learning/training mode using IOCTL_START_CREATE_POLICY
		 */

		case IOCTL_START_CREATE_POLICY:
		{
			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverDeviceControl: IOCTL_START_CREATE_POLICY ControlCode=%x InBufferSize=%x OutBufferSize=%x\n", ControlCode, InSize, OutSize));


			if ((InSize > MAX_PROCESS_NAME * sizeof(WCHAR)) || (InSize % 2))
			{
				LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_START_CREATE_POLICY Invalid Insize: %d\n", InSize));
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}

			status = STATUS_SUCCESS;

			if (LearningMode == TRUE)
			{
				LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_START_CREATE_POLICY Already in Learning Mode\n"));
				break;
			}

			RtlCopyMemory(ProcessToMonitor, pIrp->AssociatedIrp.SystemBuffer, InSize);
			ProcessToMonitor[(InSize / sizeof(WCHAR)) - 1] = 0;

			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_START_CREATE_POLICY Learning about '%S'\n", ProcessToMonitor));

			LearningMode = TRUE;

			InitLearningMode();

			break;
		}


		/*
		 * train.exe stops training/learning mode using IOCTL_STOP_CREATE_POLICY
		 */

		case IOCTL_STOP_CREATE_POLICY:
		{
			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverDeviceControl: IOCTL_STOP_CREATE_POLICY ControlCode=%x InBufferSize=%x OutBufferSize=%x\n", ControlCode, InSize, OutSize));


			if ((InSize > MAX_PROCESS_NAME * sizeof(WCHAR)) || (InSize % 2))
			{
				LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_STOP_CREATE_POLICY Invalid Insize: %d\n", InSize));
				status = STATUS_INVALID_BUFFER_SIZE;
				break;
			}

			status = STATUS_SUCCESS;

			if (LearningMode == FALSE)
			{
				LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_STOP_CREATE_POLICY Not in Learning Mode\n"));
				break;
			}

//			RtlCopyMemory(ProcessToMonitor, pIrp->AssociatedIrp.SystemBuffer, InSize);
//			ProcessToMonitor[(InSize / sizeof(WCHAR)) - 1] = 0;

//			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_STOP_CREATE_POLICY '%S'\n", ProcessToMonitor));
			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverDeviceControl: IOCTL_STOP_CREATE_POLICY\n"));

			ShutdownLearningMode();

			LearningMode = FALSE;

			break;
		}


		default:
			LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("%d DriverDeviceControl default %x %x %x %x\n", (ULONG) PsGetCurrentProcessId(), pIrpStack->MajorFunction, ControlCode, InSize, OutSize));
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}


	COMPLETE_REQUEST(pIrp, status);
}



NTSTATUS
DriverCreate(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	NTSTATUS	status;


#if HOOK_NETWORK
	if (TDIDispatch(pDeviceObject, pIrp, &status) == TRUE)
	{
		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverCreate(%x, %x): TDIDispatch\n", pDeviceObject, pIrp));
		return status;
	}
#endif


	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverCreate(%x, %x)\n", pDeviceObject, pIrp));


	//XXX need to consider any possible lock out issues where a valid userland agent is disallowed access
	//can verify userland binary name as well
#if 0
	if (ActiveUserAgent == TRUE)
	{
		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("Userland agent already exists!\n"));

		pIrp->IoStatus.Status = STATUS_ACCESS_DENIED;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		return STATUS_ACCESS_DENIED;
	}

	ActiveUserAgent = TRUE;
#endif

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);


	return STATUS_SUCCESS;
}



NTSTATUS
DriverClose(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	NTSTATUS	status;


#if HOOK_NETWORK
	if (TDIDispatch(pDeviceObject, pIrp, &status) == TRUE)
	{
		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverClose(%x, %x): TDIDispatch\n", pDeviceObject, pIrp));
		return status;
	}
#endif


	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverClose(%x, %x)\n", pDeviceObject, pIrp));

#if 0
	if (ActiveUserAgent == FALSE)
	{
		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("Userland agent does not exist!\n"));
	}

	ActiveUserAgent = FALSE;
#endif

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}



NTSTATUS
DriverCleanup(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	NTSTATUS	status;


#if HOOK_NETWORK
	if (TDIDispatch(pDeviceObject, pIrp, &status) == TRUE)
	{
		LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_VERBOSE, ("DriverCleanup(%x, %x): TDIDispatch\n", pDeviceObject, pIrp));
		return status;
	}
#endif


	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverCleanup(%x, %x)\n", pDeviceObject, pIrp));

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}



#if 0
NTSTATUS
DriverRead(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PDEVICE_EXTENSION	pDeviceExtension;
	PIO_STACK_LOCATION	pIrpStack;
	ULONG				size = 0;


	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("DriverRead()\n"));

	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

	pDeviceExtension = (PDEVICE_EXTENSION) pDeviceObject->DeviceExtension;
/*
	size = min(pDeviceExtension->BufferSize, pIrpStack->Parameters.Read.Length);

	RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, pDeviceExtension->Buffer, size);

	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("Wrote %d bytes: %s\n", size, pDeviceExtension->Buffer));

	pDeviceExtension->BufferSize = 0;
*/
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = size;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}



NTSTATUS
DriverWrite(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	PDEVICE_EXTENSION	pDeviceExtension;
	PIO_STACK_LOCATION	pIrpStack;
	ULONG				size = 0;


	LOG(LOG_SS_DRIVER_INTERNAL,LOG_PRIORITY_DEBUG,  ("DriverWrite()\n"));

	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

	pDeviceExtension = (PDEVICE_EXTENSION) pDeviceObject->DeviceExtension;
/*
	size = min(128, pIrpStack->Parameters.Write.Length);
	RtlCopyMemory(pDeviceExtension->Buffer, pIrp->AssociatedIrp.SystemBuffer, size);

	pDeviceExtension->BufferSize = size;

	LOG(LOG_SS_DRIVER_INTERNAL, LOG_PRIORITY_DEBUG, ("Read %d bytes: %s\n", size, pDeviceExtension->Buffer));
*/
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = size;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}
#endif
