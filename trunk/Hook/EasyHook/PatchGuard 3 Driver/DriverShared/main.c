/*
Released under MIT License

Copyright (c) 2008 by Christoph Husse, SecurityRevolutions e.K.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial 
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Visit http://www.codeplex.com/easyhook for more information.
*/
#include "EasyHookDriver.h"

#ifndef _M_X64
	#error "This driver part is intended for 64-bit builds only."
#endif



typedef struct _DEVICE_EXTENSION
{
    ULONG  StateVariable;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT InDriverObject,
	IN PUNICODE_STRING InRegistryPath);

NTSTATUS EasyHookDispatchCreate(
	IN PDEVICE_OBJECT InDeviceObject,
	IN PIRP	InIrp);

NTSTATUS EasyHookDispatchClose(
	IN PDEVICE_OBJECT InDeviceObject,
	IN PIRP InIrp);

NTSTATUS EasyHookDispatchDeviceControl(
	IN PDEVICE_OBJECT InDeviceObject,
	IN PIRP InIrp);

VOID EasyHookUnload(IN PDRIVER_OBJECT DriverObject);

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, EasyHookDispatchCreate)
#pragma alloc_text(PAGE, EasyHookDispatchClose)
#pragma alloc_text(PAGE, EasyHookDispatchDeviceControl)
#pragma alloc_text(PAGE, EasyHookUnload)

#endif

UCHAR*					KernelImageStart = NULL;
UCHAR*					KernelImageEnd = NULL;

/************************************************************************************
************************************** KeContainsSymbol()
*************************************************************************************

Description:

	Checks whether the given symbol is within the confines of
	the kernel image.
*/
BOOLEAN KeContainsSymbol(void* InSymbol) 
{ 
	return ((((UCHAR*)InSymbol) >= KernelImageStart) && (((UCHAR*)InSymbol) <= KernelImageEnd)); 
}


/************************************************************************************
************************************** ExtractKernelImageBoundaries()
*************************************************************************************

Description:

	Tries to find the kernel base address in the system module
	information. 

	After this, we will examine the end of the kernel image,
	by walking through the memory, until we encounter an invalid
	address...
*/
void ExtractKernelImageBoundaries()
{
	UCHAR*		InfoBuffer = NULL;
	ULONG		InfoSize = 0;
	UCHAR*		KernelPtr;
	ULONG		i;

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	
	KernelImageStart = 0xFFFFF80000000000;
	KernelImageEnd = 0xFFFFF81000000000;

	ZwQuerySystemInformation(
			11 /*SystemModuleInformation*/,
			0,
			0,
			&InfoSize);

	if((InfoBuffer = (UCHAR*)ExAllocatePool(PagedPool, InfoSize)) != NULL)
	__try
	{

		if(!NT_SUCCESS(ZwQuerySystemInformation(
				11 /*SystemModuleInformation*/,
				InfoBuffer,
				InfoSize,
				&InfoSize)))
			return;

		// extract base address
		for(i = 0; i < 100; i++)
		{
			KernelPtr = *((UCHAR**)(InfoBuffer + i));

			if(MmIsAddressValid(KernelPtr))
			{
				/*
					Try to walk through the image... 
				*/
				KernelImageStart = KernelPtr;

				do { KernelPtr += PAGE_SIZE; } while(MmIsAddressValid(KernelPtr));

				KernelImageEnd = KernelPtr;

				// check for some kernel methods...
				if(!KeContainsSymbol(KeCancelTimer) || !KeContainsSymbol(KeSetTimerEx))
				{
					KernelImageStart = 0xFFFFF80000000000;
					KernelImageEnd = 0xFFFFF81000000000;

					continue;
				}

				return;
			}
		}
	}
	__finally
	{
		ExFreePool(InfoBuffer);
	}
}

/**************************************************************

Description:

	Initializes the driver and also loads the system specific PatchGuard
	information.
*/
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT		InDriverObject,
	IN PUNICODE_STRING		InRegistryPath)
{
	NTSTATUS				Status;    
    UNICODE_STRING			NtDeviceName;
	UNICODE_STRING			DosDeviceName;
    PDEVICE_EXTENSION		DeviceExtension;
	PDEVICE_OBJECT			DeviceObject = NULL;
	BOOLEAN					SymbolicLink = FALSE;

	/*
		Create device...
	*/
    RtlInitUnicodeString(&NtDeviceName, EASYHOOK_DEVICE_NAME);

    Status = IoCreateDevice(
		InDriverObject,
		sizeof(DEVICE_EXTENSION),		// DeviceExtensionSize
		&NtDeviceName,					// DeviceName
		FILE_DEVICE_EASYHOOK,			// DeviceType
		0,								// DeviceCharacteristics
		TRUE,							// Exclusive
		&DeviceObject					// [OUT]
		);

	if (!NT_SUCCESS(Status))
		goto ERROR_ABORT;

	DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

	/*
		Register for user-mode accessibility and set major functions...
	*/
    RtlInitUnicodeString(&DosDeviceName, EASYHOOK_DOS_DEVICE_NAME);

    if (!NT_SUCCESS(Status = IoCreateSymbolicLink(&DosDeviceName, &NtDeviceName)))
		goto ERROR_ABORT;

	SymbolicLink = TRUE;

    InDriverObject->MajorFunction[IRP_MJ_CREATE] = EasyHookDispatchCreate;
    InDriverObject->MajorFunction[IRP_MJ_CLOSE] = EasyHookDispatchClose;
    InDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = EasyHookDispatchDeviceControl;
    InDriverObject->DriverUnload = EasyHookUnload;

	ExtractKernelImageBoundaries();

#ifdef PATCHGUARD_v2
	if(!NT_SUCCESS(Status = PgInitialize()))
		goto ERROR_ABORT;
#endif

	return Status;

ERROR_ABORT:

	/*
		Rollback in case of errors...
	*/

	if (SymbolicLink)
		IoDeleteSymbolicLink(&DosDeviceName);

	if (DeviceObject != NULL)
		IoDeleteDevice(DeviceObject);

	return Status;
}

NTSTATUS EasyHookDispatchCreate(
	IN PDEVICE_OBJECT InDeviceObject,
	IN PIRP InIrp)
{
    InIrp->IoStatus.Information = 0;
    InIrp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(InIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS EasyHookDispatchClose(
	IN PDEVICE_OBJECT InDeviceObject,
	IN PIRP InIrp)
{
    InIrp->IoStatus.Information = 0;
    InIrp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(InIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/************************************************************
	
Description:

	Handles all device requests.

*/
NTSTATUS EasyHookDispatchDeviceControl(
	IN PDEVICE_OBJECT InDeviceObject,
	IN PIRP	InIrp)
{
	NTSTATUS			Status = STATUS_INVALID_PARAMETER;
    PIO_STACK_LOCATION	IOStack;
    PDEVICE_EXTENSION	DeviceExtension;
    PVOID				IOBuffer;
    ULONG				InputBufferLength, OutputBufferLength;
	ULONG				IOControlCode;

    IOStack = IoGetCurrentIrpStackLocation(InIrp);
    DeviceExtension = (PDEVICE_EXTENSION)InDeviceObject->DeviceExtension;

    InIrp->IoStatus.Information = 0;

    /*
		Get the pointer to the input/output buffer and it's length
    */
    IOBuffer			= InIrp->AssociatedIrp.SystemBuffer;
    InputBufferLength	= IOStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength	= IOStack->Parameters.DeviceIoControl.OutputBufferLength;
    IOControlCode		= IOStack->Parameters.DeviceIoControl.IoControlCode;

    switch (IOControlCode)
    {
	case IOCTL_PATCHGUARD_DUMP:
		{
#ifdef PATCHGUARD_v2
			Status = PgDumpTimerTable();
#else
			Status = PgDumpFingerprints();
#endif
		}break;

	case IOCTL_PATCHGUARD_PROBE:
		{
#ifdef PATCHGUARD_v2
			// in case of PatchGuard 3 a probe is automatically done by disabling it...
			PgInstallTestHook();
#endif
			Status = STATUS_SUCCESS;
		}break;

	case IOCTL_PATCHGUARD_DISABLE:
		{
			if(PgDisablePatchGuard(InDeviceObject))
				Status = STATUS_SUCCESS;
			else
				Status = STATUS_NOT_SUPPORTED;
		}break;

    default:
		Status = STATUS_INVALID_PARAMETER;

        break;
	}

	/*
		Complete IRP request...
	*/

    InIrp->IoStatus.Status = Status;

    IoCompleteRequest(InIrp, IO_NO_INCREMENT);

    return Status;
}

/***************************************************

Description:

	Release all resources and remove the driver object.
*/
VOID EasyHookUnload(IN PDRIVER_OBJECT InDriverObject)
{
    UNICODE_STRING DosDeviceName;

    /*
		Delete the symbolic link
    */
    RtlInitUnicodeString(&DosDeviceName, EASYHOOK_DOS_DEVICE_NAME);

    IoDeleteSymbolicLink(&DosDeviceName);

    /*
		Delete the device object
    */

    IoDeleteDevice(InDriverObject->DeviceObject);
}

