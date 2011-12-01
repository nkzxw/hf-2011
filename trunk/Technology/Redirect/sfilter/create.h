#ifndef _JETUS_CREATE_H_
#define _JETUS_CREATE_H_

NTSTATUS
JeTusCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#endif