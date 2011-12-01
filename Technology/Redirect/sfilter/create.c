#include "sfilter.h"

NTSTATUS
JeTusCreateCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PKEVENT event = Context;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
JeTusCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS	status;
	KEVENT		waitEvent;
	PSFILTER_DEVICE_EXTENSION DevExt;
	PDEVICE_OBJECT		LowerDeviceObject;
	PIO_STACK_LOCATION	irpsp;
	PFILE_OBJECT		FileObject;
	UNICODE_STRING		Temp = { 0 };

    PAGED_CODE();

    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return STATUS_SUCCESS;
    }

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

	DevExt = DeviceObject->DeviceExtension;

	LowerDeviceObject = DevExt->AttachedToDeviceObject;

	irpsp = IoGetCurrentIrpStackLocation( Irp );

	FileObject = irpsp->FileObject;

	if (FileObject) {

		RtlInitUnicodeString( &Temp, L"\\1.txt" );
		if (!RtlCompareUnicodeString( &Temp, &FileObject->FileName, TRUE )) {

			WCHAR * Buffer = NULL;
			Buffer = ExAllocatePoolWithTag( NonPagedPool, 0x400, 'APER' );
			if (!Buffer) {

				Irp->IoStatus.Information	= 0;
				Irp->IoStatus.Status		= STATUS_INSUFFICIENT_RESOURCES;
				IoCompleteRequest( Irp, IO_NO_INCREMENT );
				return STATUS_INSUFFICIENT_RESOURCES;
			}

			RtlZeroMemory( Buffer, 0x400 );
			RtlCopyMemory( Buffer, L"\\??\\e:\\test\\2.txt", 34 );
			ExFreePool( FileObject->FileName.Buffer );
			FileObject->FileName.Buffer = Buffer;
			FileObject->FileName.Length = 34;
			FileObject->FileName.MaximumLength = 0x400;

			Irp->IoStatus.Status		= STATUS_REPARSE;
			Irp->IoStatus.Information	= IO_REPARSE;
			IoCompleteRequest( Irp, IO_NO_INCREMENT );
			return STATUS_REPARSE;
		}
	}

	KeInitializeEvent( &waitEvent, NotificationEvent, FALSE );

	IoCopyCurrentIrpStackLocationToNext( Irp );

	IoSetCompletionRoutine(
		Irp,
		JeTusCreateCompletion,
		&waitEvent,
		TRUE,
		TRUE,
		TRUE );

	status = IoCallDriver( LowerDeviceObject, Irp );

	if (STATUS_PENDING == status) {

		NTSTATUS localStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
		ASSERT(STATUS_SUCCESS == localStatus);
	}

	ASSERT(KeReadStateEvent(&waitEvent) ||
			!NT_SUCCESS(Irp->IoStatus.Status));


	status = Irp->IoStatus.Status;

	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return status;
}