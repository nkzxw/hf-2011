#include <ntddk.h>
#include <ntstatus.h>
#include "..\\IOCTL.h"

extern PSSDT    KeServiceDescriptorTable;
	
VOID WPOFF ()
{
		__asm
		{
			cli		;//关中断
			mov eax, cr0
			and eax, ~0x10000
			mov cr0, eax
		}	
}

VOID WPON ()
{
		__asm
		{
			mov eax, cr0
			or eax, 0x10000
			mov cr0, eax
			sti		
		}
}

NTSTATUS SSDTDeviceIoCtl( PDEVICE_OBJECT pDeviceObject, PIRP Irp )
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION IrpStack;
	PVOID InputBuffer;
	PVOID OutputBuffer;
	ULONG InputBufferLength;
	ULONG OutputBufferLength;
	ULONG IoControlCode;
		
	Irp->IoStatus.Status = ntStatus;
	Irp->IoStatus.Information = 0;
	
	IrpStack = IoGetCurrentIrpStackLocation( Irp );
	
	InputBuffer = IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
	InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	
	OutputBuffer = Irp->UserBuffer;
	OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	
	IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
	
	switch( IoControlCode )
	{
	case IOCTL_GETSSDT:	//得到SSDT
		__try
		{
			ProbeForWrite( OutputBuffer, sizeof( SSDT ), sizeof( ULONG ) );
			RtlCopyMemory( OutputBuffer, KeServiceDescriptorTable, sizeof( SSDT ) );
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			ntStatus = GetExceptionCode();
			break;
		}
		break;
	case IOCTL_SETSSDT: //设置 SSDT
		__try
		{
			ProbeForRead( InputBuffer, sizeof( SSDT ), sizeof( ULONG ) );
			
			WPOFF ();
			RtlCopyMemory( KeServiceDescriptorTable, InputBuffer, sizeof( SSDT ) );
			WPON ();
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			ntStatus = GetExceptionCode();
			break;
		}		
		break;
	case IOCTL_GETHOOK:	//查询SSDT指定地址
		__try
		{
			ProbeForRead( InputBuffer, sizeof( ULONG ), sizeof( ULONG ) );
			ProbeForWrite( OutputBuffer, sizeof( ULONG ), sizeof( ULONG ) );
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			ntStatus = GetExceptionCode();
			break;
		}
		
		if( KeServiceDescriptorTable->ulNumberOfServices <= *(PULONG)InputBuffer )
		{
			ntStatus = STATUS_INVALID_PARAMETER;
			break;
		}
		*((PULONG)OutputBuffer) = *( (PULONG)(KeServiceDescriptorTable->pvSSDTBase) + *(PULONG)InputBuffer );
		break;
	//*************************************************
	case IOCTL_SETHOOK:	//设置SSDT指定地址
		__try
		{
			ProbeForRead( InputBuffer, sizeof( ULONG ), sizeof( ULONG ) );
			ProbeForRead( OutputBuffer, sizeof( ULONG ), sizeof( ULONG ) );
		}
		__except( EXCEPTION_EXECUTE_HANDLER )
		{
			ntStatus = GetExceptionCode();
			break;
		}
		//测试传入的参数是否正确
		if( KeServiceDescriptorTable->ulNumberOfServices <= *(PULONG)InputBuffer )
		{
			ntStatus = STATUS_INVALID_PARAMETER;
			break;
		}
		
		WPOFF ();
		 *( (PULONG)(KeServiceDescriptorTable->pvSSDTBase) + *(PULONG)InputBuffer ) = *((PULONG)OutputBuffer);
		WPON ();
		
		break;
	default:
		ntStatus = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	IoCompleteRequest( Irp, IO_NO_INCREMENT );	
	return ntStatus;
}

void SSDTUnload( PDRIVER_OBJECT pDriverObject )
{
	UNICODE_STRING	usDosDeviceName;
	
	RtlInitUnicodeString( &usDosDeviceName, DEVICE_NAME );
	IoDeleteSymbolicLink( &usDosDeviceName );
	IoDeleteDevice( pDriverObject->DeviceObject );
}

NTSTATUS SSDTCreate( IN PDEVICE_OBJECT pDeviceObject, IN PIRP Irp )
{
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(	
	PDRIVER_OBJECT pDriverObject,
	PUNICODE_STRING pRegistryPath )
{
	PDEVICE_OBJECT pdo = NULL;
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING usDriverName;
	UNICODE_STRING usDosDeviceName;
	
	RtlInitUnicodeString( &usDriverName, DRIVER_NAME );
	RtlInitUnicodeString( &usDosDeviceName, DEVICE_NAME );
	
	status = IoCreateDevice( pDriverObject, 0, &usDriverName, \
		FILE_DRIVER_SSDT, FILE_DEVICE_SECURE_OPEN, \
		FALSE, &pdo );
	
	if(NTSTATUS (status))
	{
		pDriverObject->MajorFunction[IRP_MJ_CREATE] = SSDTCreate;
		pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] \
			= SSDTDeviceIoCtl;
		pDriverObject->DriverUnload = SSDTUnload;
		
		IoCreateSymbolicLink( &usDosDeviceName, &usDriverName );
	}
	
	return status;
}