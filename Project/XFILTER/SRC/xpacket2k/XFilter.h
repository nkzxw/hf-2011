/*！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
	file:			IpFilter.h
	project:		xfilter personal firewall 2.0
	create date:	2002-01-28
	Comments:		ip packet filter driver, Attach function
	author:			tony zhu
	email:			xstudio@xfilt.com or xstudio@371.net
	url:			http://www.xfilt.com
	warning:		...
	copyright (c) 2002-2003 xstudio.di All Right Reserved.
*///！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！

#ifdef USE_XFILTER 

#ifndef XFILTER_H
#define XFILTER_H

#define	XPACKET_NOTE_TYPE_X_FILTER_DEVICE		0x374aef90

typedef struct _XPACKET_DEVICE_EXTENSION
{
	ULONG			ulNodeType;
	ULONG			ulNodeSize;
	ULONG			ulDeviceExtensionFlags;
	PDEVICE_OBJECT	pFilterDeviceObject;
	PDEVICE_OBJECT	pTargetDeviceObject;		
	PFILE_OBJECT    pTargetFileObject;		
	PDEVICE_OBJECT  pLowerDeviceObject;		
} XPACKET_DEVICE_EXTENSION, *PXPACKET_DEVICE_EXTENSION;


#define XPACKET_XFILTER_DEVICE_NAME				L"\\Device\\XPacket"
#define XPACKET_XFILTER_DOS_DEVICE_NAME			L"\\DosDevices\\XPacket"

NTSTATUS
XFilter_Create(
	IN	PDRIVER_OBJECT		DriverObject,
	IN	PUNICODE_STRING		RegistryPath
);

VOID
XFilter_Delete(
	IN	PDEVICE_OBJECT		pDeviceObject
);


#endif // XFILTER_H

#endif // USE_XFILTER