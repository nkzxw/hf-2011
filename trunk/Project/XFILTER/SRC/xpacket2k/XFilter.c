//-----------------------------------------------------------
/*
	���̣�		�Ѷ����˷���ǽ
	��ַ��		http://www.xfilt.com
	�����ʼ���	xstudio@xfilt.com
	��Ȩ���� (c) 2002 ���޻�(�Ѷ���ȫʵ����)

	��Ȩ����:
	---------------------------------------------------
		�����Գ���������Ȩ���ı�����δ����Ȩ������ʹ��
	���޸ı����ȫ���򲿷�Դ���롣�����Ը��ơ����û�ɢ
	���˳���򲿷ֳ�������������κ�ԽȨ��Ϊ�����⵽��
	���⳥�����µĴ�������������������̷�����׷�ߡ�
	
		��ͨ���Ϸ�;�������Դ������(�����ڱ���)��Ĭ��
	��Ȩ�����Ķ������롢���ԡ������ҽ����ڵ��Ե���Ҫ��
	�����޸ı����룬���޸ĺ�Ĵ���Ҳ����ֱ��ʹ�á�δ��
	��Ȩ������������Ʒ��ȫ���򲿷ִ�������������Ʒ��
	������ת�����ˣ����������κη�ʽ���ƻ򴫲���������
	�����κη�ʽ����ҵ��Ϊ��	

    ---------------------------------------------------	
*/
//-----------------------------------------------------------
// ��飺
//		����һ��Device�������������������Ӧ�ó���Ŀ��ơ�
//		��Ҫ��������DeviceIoControl
//		
//
//

#include "xprecomp.h"
#pragma hdrstop

#ifdef USE_XFILTER 

//
// ����һ���ɼ����豸
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
// ɾ��һ���豸
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
