/*

  HookPortBypass.c
  
  Author: Adly
  Last Updated: 2008-11-16
	
*/

#include "comhdr.h"

//
// A structure representing the instance information associated with
// a particular device
//

typedef struct _DEVICE_EXTENSION
{
	ULONG  StateVariable;
	
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//
// Function Declare
//
extern "C"
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT		DriverObject,
	IN PUNICODE_STRING		RegistryPath
);

NTSTATUS
HookportbypassDispatchCreate(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
);

NTSTATUS
HookportbypassDispatchClose(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
);

NTSTATUS
HookportbypassDispatchDeviceControl(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
);

VOID
HookportbypassUnload(
	IN PDRIVER_OBJECT		DriverObject
);

// 
// #ifdef ALLOC_PRAGMA
// #pragma alloc_text(INIT, DriverEntry)
// #pragma alloc_text(PAGE, HookportbypassDispatchCreate)
// #pragma alloc_text(PAGE, HookportbypassDispatchClose)
// #pragma alloc_text(PAGE, HookportbypassDispatchDeviceControl)
// #pragma alloc_text(PAGE, HookportbypassUnload)
// #endif // ALLOC_PRAGMA


extern "C"
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT		DriverObject,
	IN PUNICODE_STRING		RegistryPath
)
{
	NTSTATUS			Status = STATUS_SUCCESS;    
	UNICODE_STRING		ntDeviceName;
	UNICODE_STRING		dosDeviceName;
	PDEVICE_EXTENSION	deviceExtension;
	PDEVICE_OBJECT		deviceObject = NULL;
	
	

	kprintf( "DriverEntry: %wZ\n", RegistryPath);
	
	GetProcessNameOffset();

	RtlInitUnicodeString(&ntDeviceName, HOOKPORTBYPASS_DEVICE_NAME_W);
	
	
	Status = IoCreateDevice(
		DriverObject,
		sizeof(DEVICE_EXTENSION),		// DeviceExtensionSize
		&ntDeviceName,					// DeviceName
		FILE_DEVICE_HOOKPORTBYPASS,	// DeviceType
		0,								// DeviceCharacteristics
		TRUE,							// Exclusive
		&deviceObject					// [OUT]
		);
	
	if(!NT_SUCCESS(Status))
	{
		KdPrint(("[HookPortBypass] IoCreateDevice Error Code = 0x%X\n", Status));
		
		return Status;
	}
	deviceObject->Flags |= DO_BUFFERED_IO;
	deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
	
	//
	// Set up synchronization objects, state info,, etc.
	//
	
	//
	// Create a symbolic link that Win32 apps can specify to gain access
	// to this driver/device
	//
	
	RtlInitUnicodeString(&dosDeviceName, HOOKPORTBYPASS_DOS_DEVICE_NAME_W);
	
	Status = IoCreateSymbolicLink(&dosDeviceName, &ntDeviceName);
	
	if(!NT_SUCCESS(Status))
	{
		KdPrint(("[HookPortBypass] IoCreateSymbolicLink Error Code = 0x%X\n", Status));
		
		//
		// Delete Object
		//

		IoDeleteDevice(deviceObject);
		
		return Status;
	}
	
	//
	// Create dispatch points for device control, create, close.
	//
	
	DriverObject->MajorFunction[IRP_MJ_CREATE]			= HookportbypassDispatchCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]			= HookportbypassDispatchClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]	= HookportbypassDispatchDeviceControl;
	DriverObject->DriverUnload							= HookportbypassUnload;
	
//	InitXOXOgetThread();
	return Status;
}

NTSTATUS
HookportbypassDispatchCreate(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	
	Irp->IoStatus.Information = 0;
	
	KdPrint(("[HookPortBypass] IRP_MJ_CREATE\n"));
	
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return Status;
}

NTSTATUS
HookportbypassDispatchClose(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
)
{
	NTSTATUS Status = STATUS_SUCCESS;
	
	Irp->IoStatus.Information = 0;
	
	KdPrint(("[HookPortBypass] IRP_MJ_CLOSE\n"));
	
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return Status;
}

NTSTATUS
HookportbypassDispatchDeviceControl(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
)
{
	NTSTATUS			Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION	irpStack;
	PDEVICE_EXTENSION	deviceExtension;
	PVOID				ioBuf, outputBuffer;
	ULONG				inBufLength, outBufLength;
	ULONG				ioControlCode;
	
	irpStack = IoGetCurrentIrpStackLocation(Irp);
	deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
	Irp->IoStatus.Information = 0;
	
	//
	// Get the pointer to the input/output buffer and it's length
	//
	
	ioBuf = Irp->AssociatedIrp.SystemBuffer;
	inBufLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
	outBufLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
	ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
	// Irp->UserBuffer;		// If METHOD_NEITHER, This is Output Buffer
	
	switch (ioControlCode)
	{
		case IOCTL_HOOKPORTBYPASS_HOOKPORT:
		{
			RtlZeroMemory(g_StrongOdSyS,sizeof(g_StrongOdSyS));
			strcpy(g_StrongOdSyS, (char*)ioBuf);
			Status	=	InitHookPort();
			PsSetLoadImageNotifyRoutine(LoadImageNotify);
			*(PULONG)ioBuf	=	g_ProxyModuleBase;
			Irp->IoStatus.Information	=	sizeof(g_ProxyModuleBase);
			kprintf("IOCTL_HOOKPORTBYPASS_HOOKPORT ,outputbuff %x\r\n", *(PULONG)ioBuf);
			break;
		}
		//Ð¶ÔØHOOK
		case IOCTL_HOOKPORTBYPASS_UNHOOKPORT:
		{
			PsRemoveLoadImageNotifyRoutine(LoadImageNotify);
			kprintf("IOCTL_HOOKPORTBYPASS_UNHOOKPORT");
			UnHookPort();

			Status	=	STATUS_SUCCESS;
			break;
		}
	case IOCTL_HOOKPORTBYPASS_HOOKINT1:
		{
			//
			
			break;
		}

	case IOCTL_HOOKPORTBYPASS_TEST1:
		{
			//
			// Sample IO Control
			//
			KdPrint(("[HookPortBypass] TEST1 IOCTL: 0x%X", ioControlCode));
			
			break;
		}
		
	default:
		{
			Status = STATUS_INVALID_PARAMETER;
			
			KdPrint(("[HookPortBypass] Unknown IOCTL: 0x%X (%04X,%04X)",
				ioControlCode, DEVICE_TYPE_FROM_CTL_CODE(ioControlCode),
				IoGetFunctionCodeFromCtlCode(ioControlCode)));
			
			break;
		}
	}
	
	
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	
	return Status;
}

VOID
HookportbypassUnload(
	IN PDRIVER_OBJECT		DriverObject
)
{
	UNICODE_STRING dosDeviceName;
//	UnhookXOXOthread();
	//
	// Free any resources
	//
	
	//
	// Delete the symbolic link
	//
	
	RtlInitUnicodeString(&dosDeviceName, HOOKPORTBYPASS_DOS_DEVICE_NAME_W);
	
	IoDeleteSymbolicLink(&dosDeviceName);
	
	//
	// Delete the device object
	//
	
	IoDeleteDevice(DriverObject->DeviceObject);
	kprintf("UnLoad");
	KdPrint(("[HookPortBypass] Unloaded"));
}