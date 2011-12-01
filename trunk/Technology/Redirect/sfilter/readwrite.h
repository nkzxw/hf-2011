#ifndef _JETUS_RW_H_
#define _JETUS_RW_H_

NTSTATUS
JeTusRead (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
JeTusWrite (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#endif