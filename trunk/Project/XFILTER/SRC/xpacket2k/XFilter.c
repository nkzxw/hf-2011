//-----------------------------------------------------------
/*
	工程：		费尔个人防火墙
	网址：		http://www.xfilt.com
	电子邮件：	xstudio@xfilt.com
	版权所有 (c) 2002 朱艳辉(费尔安全实验室)

	版权声明:
	---------------------------------------------------
		本电脑程序受著作权法的保护。未经授权，不能使用
	和修改本软件全部或部分源代码。凡擅自复制、盗用或散
	布此程序或部分程序或者有其它任何越权行为，将遭到民
	事赔偿及刑事的处罚，并将依法以最高刑罚进行追诉。
	
		凡通过合法途径购买此源程序者(仅限于本人)，默认
	授权允许阅读、编译、调试。调试且仅限于调试的需要才
	可以修改本代码，且修改后的代码也不可直接使用。未经
	授权，不允许将本产品的全部或部分代码用于其它产品，
	不允许转阅他人，不允许以任何方式复制或传播，不允许
	用于任何方式的商业行为。	

    ---------------------------------------------------	
*/
//-----------------------------------------------------------
// 简介：
//		创建一个Device对象，这个对象用来接受应用程序的控制。
//		主要用来处理DeviceIoControl
//		
//
//

#include "xprecomp.h"
#pragma hdrstop

#ifdef USE_XFILTER 

//
// 创建一个可见的设备
//
NTSTATUS
XFilter_Create(
	IN	PDRIVER_OBJECT		DriverObject,
	IN	PUNICODE_STRING		RegistryPath
)
{
	NTSTATUS					status = STATUS_SUCCESS;
	PXPACKET_DEVICE_EXTENSION	pDeviceExtension	 = NULL;
	PDEVICE_OBJECT				pFilterDeviceObject = NULL;
	UNICODE_STRING				usTempName;
	UNICODE_STRING				SymbolicLinkName;

	RtlInitUnicodeString(&usTempName, XPACKET_XFILTER_DEVICE_NAME);

	status = IoCreateDevice(
               IN	DriverObject,
               IN	sizeof(XPACKET_DEVICE_EXTENSION),
               IN	&usTempName,
               IN	FILE_DEVICE_XPACKET,
               IN	0,
               IN	FALSE,                 
               OUT	&pFilterDeviceObject
               );
	if(status != STATUS_SUCCESS)
	{
		dprintf(("XFilter_Create: Couldn't create the XFilter Device Object(0x%X)\n", status));
		return( status );
	}

	RtlInitUnicodeString(&SymbolicLinkName, XPACKET_XFILTER_DOS_DEVICE_NAME);

	status = IoCreateSymbolicLink(
				&SymbolicLinkName,
				&usTempName);

	if (!NT_SUCCESS(status)) 
	{
		dprintf(("IpFilter_Attach: Couldn't create the Ip Filter Dos Device Object(0x%X)\n", status));
		IoDeleteDevice(pFilterDeviceObject);
		pFilterDeviceObject = NULL;
		return( status );
	}

	pDeviceExtension = (PXPACKET_DEVICE_EXTENSION)pFilterDeviceObject->DeviceExtension;

	NdisZeroMemory(pDeviceExtension, sizeof(XPACKET_DEVICE_EXTENSION));
	pDeviceExtension->ulNodeType = XPACKET_NOTE_TYPE_X_FILTER_DEVICE;
	pDeviceExtension->ulNodeSize = sizeof(XPACKET_DEVICE_EXTENSION);

	dprintf(("XFilter_Create: Success Create XFilter Device Object(0x%X)\n", status));
	return status;
}

//
// 删除一个设备
//
VOID
XFilter_Delete(
	IN	PDEVICE_OBJECT		pDeviceObject
)
{
	PXPACKET_DEVICE_EXTENSION pDeviceExtension;
	UNICODE_STRING SymbolicLinkName;

	pDeviceExtension = (PXPACKET_DEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	RtlInitUnicodeString(&SymbolicLinkName, XPACKET_XFILTER_DOS_DEVICE_NAME);
	IoDeleteSymbolicLink( &SymbolicLinkName );

	pDeviceExtension->ulNodeType = 0;
	pDeviceExtension->ulNodeSize = 0;

	IoDeleteDevice( pDeviceObject );

	dprintf(("XFilter_Delete Success\n"));
}

#endif // USE_XFILTER

#pragma comment( exestr, "B9D3B8FD2A7A686B6E7667742B")
