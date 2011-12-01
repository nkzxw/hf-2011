#ifndef _JETUS_SQ_H_
#define _JETUS_SQ_H_

NTSTATUS
JeTusSetInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
JeTusQueryInformation (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#endif