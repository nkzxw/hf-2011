#ifndef _JETUS_CC_H_
#define _JETUS_CC_H_

NTSTATUS
JeTusCleanup (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
JeTusClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#endif