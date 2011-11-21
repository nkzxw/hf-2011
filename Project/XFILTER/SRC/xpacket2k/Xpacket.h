
// XPACKET.h - include file for VxD XPACKET

#ifndef XPACKET_H
#define XPACKET_H

typedef struct tagIOCTLParams
{
	DWORD		dioc_IOCtlCode;
	PVOID		dioc_InBuf;
	DWORD		dioc_cbInBuf;
	PVOID		dioc_OutBuf;
	DWORD		dioc_cbOutBuf;
} IOCTLPARAMS, *PIOCTLPARAMS;

NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT		DriverObject,
	IN	PUNICODE_STRING		RegistryPath
	);

VOID 
PacketUnload(
	IN PDRIVER_OBJECT		DriverObject
	);

NTSTATUS
XPacketIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
	);

DWORD
OnW32Deviceiocontrol(
	PIOCTLPARAMS pVtoolsD
	);

NTSTATUS
XPacketOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
XPacketClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
XPacketCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
XPacketWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
XPacketRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#endif //XPACKET_H
