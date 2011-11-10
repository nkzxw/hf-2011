/***********************************************************************
HelloDriver.cpp

Purpose:
	Practise write base driver program.

Author:
	yhf	(hongfu830202@163.com)
CreateTime:
	2011-5-19 22:45:00
***********************************************************************/

#include "HelloDriver.h"

NTSTATUS HelloDriverCreate (IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS HelloDriverClose (IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS HelloDriverWrite (IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp);
NTSTATUS HelloDriverRead (IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp);

NTSTATUS HelloCreateFirstDevice (IN PDRIVER_OBJECT pDriverObject);
NTSTATUS HelloCreateSecondDevice (IN PDRIVER_OBJECT pDriverObject);
NTSTATUS HelloCreateThirdDevice (IN PDRIVER_OBJECT pDriverObject);

VOID HelloDriverDump (IN PDRIVER_OBJECT pDriverObject);
VOID FileOperation (IN PDRIVER_OBJECT pDriverObject);
VOID LinkListOperation ();

#pragma PAGEDCODE
VOID HelloDriverUnload (IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pDeviceObject;
	pDeviceObject = pDriverObject->DeviceObject;

	PrintProcessName (L"HelloDriverUnload");

	while (pDeviceObject != NULL)
	{
		PDEVICE_EXTENSION pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
		UNICODE_STRING symbolicLinkName = pDeviceExtension->symbolLinkName;

		KdPrint (("Delete Device Name = %wZ\n", &(pDeviceExtension->deviceName)));
		KdPrint (("Delete Device symbolicLinkName = %wZ\n", &(pDeviceExtension->symbolLinkName)));
		
		pDeviceObject = pDeviceObject->NextDevice;

		IoDeleteSymbolicLink (&symbolicLinkName);
		IoDeleteDevice (pDeviceExtension->pDeviceObject);
	}
}

#pragma INITCODE
NTSTATUS DriverEntry (IN PDRIVER_OBJECT pDriverObject,
					  IN PUNICODE_STRING pRegistryPath)
{
	NTSTATUS  status;
	UNICODE_STRING deviceName;
	
	PrintProcessName (L"DriverEntry");

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDriverCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDriverClose;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloDriverWrite;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDriverRead;

	pDriverObject->DriverUnload = HelloDriverUnload;

	status = HelloCreateFirstDevice (pDriverObject);
	status = HelloCreateSecondDevice (pDriverObject);
	status = HelloCreateThirdDevice (pDriverObject);


	//
	// Dump Device Information.
	//
	HelloDriverDump (pDriverObject);

	//
	// File Operation
	//
	FileOperation (pDriverObject);

	//
	// Link List Operation.
	//
	LinkListOperation ();

	return status;
}

NTSTATUS HelloCreateFirstDevice (IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pDeviceObject;
	UNICODE_STRING deviceName;
	UNICODE_STRING symbolLinkName;
	PDEVICE_EXTENSION pDeviceExtension;

	NTSTATUS status = STATUS_SUCCESS;

	PrintProcessName (L"HelloCreateFirstDevice");

	RtlInitUnicodeString (&deviceName, HELLO_DRIVER_DEVICE_NAME1);
	status = IoCreateDevice (pDriverObject,
							 sizeof (DEVICE_EXTENSION),
							 &deviceName,
							 FILE_DEVICE_UNKNOWN,
							 0,
							 TRUE,
							 &pDeviceObject);
	if (!NT_SUCCESS (status))
	{
		KdPrint (("Create Device %wZ error.\n", &deviceName));
		return status;
	}

	pDeviceObject->Flags |=	DO_BUFFERED_IO;
	pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
	pDeviceExtension->pDeviceObject = pDeviceObject;
	pDeviceExtension->deviceName = deviceName;

	RtlInitUnicodeString (&symbolLinkName, HELLO_DRIVER_SYMBOL_NAME1);
	pDeviceExtension->symbolLinkName = 	symbolLinkName;

	status = IoCreateSymbolicLink (&symbolLinkName, &deviceName);
	if (!NT_SUCCESS (status))
	{
		KdPrint (("Create Device SymbolLink %wZ error.\n", &symbolLinkName));
		IoDeleteDevice (pDeviceObject);
	}

	return status; 

}

NTSTATUS HelloCreateSecondDevice (IN PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING deviceName;
	PDEVICE_OBJECT pDeviceObject;
	PDEVICE_EXTENSION pDeviceExtension;
	UNICODE_STRING symbolLinkName;
	NTSTATUS status = STATUS_SUCCESS;	   

	PrintProcessName (L"HelloCreateSecondDevice");

	RtlInitUnicodeString (&deviceName, HELLO_DRIVER_DEVICE_NAME2);
	status = IoCreateDevice (pDriverObject,
							 sizeof (DEVICE_EXTENSION),
							 &deviceName,
							 FILE_DEVICE_UNKNOWN,
						     0,
							 TRUE,
							 &pDeviceObject);
	if (!NT_SUCCESS (status))
	{
		KdPrint (("Create Device %wZ error.\n", &deviceName));
		return status;
	}

	pDeviceObject->Flags |=  DO_BUFFERED_IO;
	pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
    pDeviceExtension->pDeviceObject = pDeviceObject;
	pDeviceExtension->deviceName = deviceName;

	RtlInitUnicodeString (&symbolLinkName, HELLO_DRIVER_SYMBOL_NAME2);
	pDeviceExtension->symbolLinkName = symbolLinkName;
	
	status = IoCreateSymbolicLink (&symbolLinkName, &deviceName);
	if (!NT_SUCCESS (status))
	{
		KdPrint (("Create Device SymbolLink %wZ error.\n", &symbolLinkName));
		IoDeleteDevice (pDeviceObject);
	}
	return status;
}

NTSTATUS HelloCreateThirdDevice (IN PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING deviceName;
	PDEVICE_OBJECT pDeviceObject;
	PDEVICE_EXTENSION pDeviceExtension;
	UNICODE_STRING symbolLinkName;
	NTSTATUS status = STATUS_SUCCESS;	   

	PrintProcessName (L"HelloCreateThirdDevice");
	RtlInitUnicodeString (&deviceName, HELLO_DRIVER_DEVICE_NAME3);
	status = IoCreateDevice (pDriverObject,
							 sizeof (DEVICE_EXTENSION),
							 &deviceName,
							 FILE_DEVICE_UNKNOWN,
						     0,
							 TRUE,
							 &pDeviceObject);
	if (!NT_SUCCESS (status))
	{
		KdPrint (("Create Device %wZ error.\n", &deviceName));
		return status;
	}

	pDeviceObject->Flags |=  DO_BUFFERED_IO;
	pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
    pDeviceExtension->pDeviceObject = pDeviceObject;
	pDeviceExtension->deviceName = deviceName;

	RtlInitUnicodeString (&symbolLinkName, HELLO_DRIVER_SYMBOL_NAME3);
	pDeviceExtension->symbolLinkName = symbolLinkName;
	
	status = IoCreateSymbolicLink (&symbolLinkName, &deviceName);
	if (!NT_SUCCESS (status))
	{
		KdPrint (("Create Device SymbolLink %wZ error.\n", &symbolLinkName));
		IoDeleteDevice (pDeviceObject);
	}
	return status;
}

NTSTATUS HelloDriverCreate (IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;	
	
	PrintProcessName (L"HelloDriverCreate");

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest (pIrp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS HelloDriverClose (IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;

	PrintProcessName (L"HelloDriverClose");

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest (pIrp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS HelloDriverWrite (IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;

   	PrintProcessName (L"HelloDriverWrite");

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest (pIrp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS HelloDriverRead (IN PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;

	PrintProcessName (L"HelloDriverRead");

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest (pIrp, IO_NO_INCREMENT);

	return status;
}

VOID HelloDriverDump (IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pDeviceObject = pDriverObject->DeviceObject;
	int i = 1;

	PrintProcessName (L"HelloDriverDump");

	KdPrint(("----------------------------------------------\n"));
	KdPrint (("HelloDriverDump Enter:\n"));
	KdPrint (("Dump Begin.....\n"));
	KdPrint (("Driver Address = 0X%08X", pDriverObject));
	KdPrint (("Driver Name = %wZ\n", &(pDriverObject->DriverName)));
	KdPrint(("Driver HardwareDatabase:%wZ\n",pDriverObject->HardwareDatabase));
	KdPrint(("Driver first device:0X%08X\n",pDriverObject->DeviceObject));

	for (; pDeviceObject != NULL; pDeviceObject = pDeviceObject->NextDevice)
	{
		PDEVICE_EXTENSION pDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;
		KdPrint(("The %d device\n",i++));
		KdPrint (("Device Name: %wZ\n", &(pDeviceExtension->deviceName)));
		KdPrint(("Device AttachedDevice:0X%08X\n",pDeviceObject->AttachedDevice));
		KdPrint(("Device NextDevice:0X%08X\n",pDeviceObject->NextDevice));
		KdPrint(("Device StackSize:%d\n",pDeviceObject->StackSize));
		KdPrint(("Device's DriverObject:0X%08X\n",pDeviceObject->DriverObject));

	}
	KdPrint (("Dump End.....\n"));
	KdPrint(("----------------------------------------------\n"));
}


//
// File Operation.
//
VOID FileOperation (IN PDRIVER_OBJECT pDriverObject)
{
}


//
// Link list Operation.
// 
VOID LinkListOperation ()
{
	LIST_ENTRY LinkListHead;
	PMYDATASTRUCT pData;
	ULONG i = 0;

	InitializeListHead (&LinkListHead);	
	for (; i < 10; i++)
	{
		pData = (PMYDATASTRUCT)ExAllocatePool (PagedPool, sizeof(MYDATASTRUCT));
		pData->number = i;
		InsertHeadList (&LinkListHead, &pData->ListEntry);
	}

	while (!IsListEmpty (&LinkListHead))
	{
		//PLIST_ENTRY pEntry = RemoveTailList (&LinkListHead); //0.1.2.3.4.5.6.7.8.9  
		//pData = CONTAINING_RECORD(pEntry,
		//                          MYDATASTRUCT,
	    //                          ListEntry);
		PLIST_ENTRY pEntry = RemoveHeadList(&LinkListHead); //9.8.7.6.5.4.3.2.1.0
		pData = (PMYDATASTRUCT)pEntry;
		KdPrint (("number is %d\n",pData->number));
		ExFreePool (pData);
	}
}