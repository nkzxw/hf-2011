#include "sfilter.h"

NTSTATUS
JeTusRWCompletion (
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
JeTusRead (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS status;

	KEVENT waitEvent;

	PSFILTER_DEVICE_EXTENSION DevExt;

	PDEVICE_OBJECT LowerDeviceObject;

	PIO_STACK_LOCATION irpsp;

	PFILE_OBJECT FileObject;

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

	KeInitializeEvent( &waitEvent, NotificationEvent, FALSE );

	IoCopyCurrentIrpStackLocationToNext( Irp );

	IoSetCompletionRoutine(
		Irp,
		JeTusRWCompletion,
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

NTSTATUS
JeTusWrite (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS status;

	KEVENT waitEvent;

	PSFILTER_DEVICE_EXTENSION DevExt;

	PDEVICE_OBJECT LowerDeviceObject;

	PIO_STACK_LOCATION irpsp;

	PFILE_OBJECT FileObject;

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

	KeInitializeEvent( &waitEvent, NotificationEvent, FALSE );

	IoCopyCurrentIrpStackLocationToNext( Irp );

	IoSetCompletionRoutine(
		Irp,
		JeTusRWCompletion,
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