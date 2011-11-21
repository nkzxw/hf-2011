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
//		��ڡ�����DriverEntry��DeviceIoControl�Ĵ�������
//
//

#include "xprecomp.h"
#pragma hdrstop
 
#pragma NDIS_INIT_FUNCTION(DriverEntry)

//
// ��ں���������������ɻ�������ʼ����HOOK���豸�Ĵ���
// 
NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT		DriverObject,
	IN	PUNICODE_STRING		RegistryPath
)
{
	dprintf(("DriverEntry Loading...\n"));

	//
	// ���롢��ʼ�����������
	//
	if(!InitBuffer())
		return NDIS_STATUS_FAILURE;

	//
	// Hook Ndis Function
	//
	if(GetNdisModuleAddress() && m_NdisBaseAddress != NULL)
	{
		if(HookFunction(m_NdisBaseAddress, "NdisSend", XF_NdisSend
			, (ULONG*)&m_pNdisSend) == NULL
			)
			dprintf(("Hook NdisSend Failure\n"));
		else
			dprintf(("Hook NdisSend Success\n"));

		if(HookFunction(m_NdisBaseAddress, "NdisRegisterProtocol", XF_NdisRegisterProtocol
			, (ULONG*)&m_pNdisRegisterProtocol) == NULL
			)
			dprintf(("Hook NdisRegisterProtocol Failure\n"));
		else
			dprintf(("Hook NdisRegisterProtocol Success\n"));

		//
		// 2002/08/21 add
		//
		if(HookFunction(m_NdisBaseAddress, "NdisDeregisterProtocol", XF_NdisDeregisterProtocol
			, (ULONG*)&m_pNdisDeregisterProtocol) == NULL
			)
			dprintf(("Hook NdisDeregisterProtocol Failure\n"));
		else
			dprintf(("Hook NdisDeregisterProtocol Success\n"));

		if(HookFunction(m_NdisBaseAddress, "NdisOpenAdapter", XF_NdisOpenAdapter
			, (ULONG*)&m_pNdisOpenAdapter) == NULL
			)
			dprintf(("Hook NdisOpenAdapter Failure\n"));
		else
			dprintf(("Hook NdisOpenAdapter Success\n"));
	}

    DriverObject->MajorFunction[IRP_MJ_CREATE]			= XPacketOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]			= XPacketClose;
    DriverObject->MajorFunction[IRP_MJ_READ]			= XPacketRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE]			= XPacketWrite;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]			= XPacketCleanup;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = XPacketIoControl;

	DriverObject->DriverUnload = PacketUnload;

	//
	// ����ʵ��DeviceIoControl���豸
	//
	XFilter_Create(DriverObject, RegistryPath);

	return(0);
}

//
// ж����������
//
VOID 
PacketUnload(
	IN PDRIVER_OBJECT		DriverObject
)
{
    PDEVICE_OBJECT     DeviceObject;
    PDEVICE_OBJECT     OldDeviceObject;
		
 	dprintf(("DriverEntry unLoading...\n"));

	//
	// �ͷ��ڴ�
	//
	FreeMemory();
	FreeNameList();
	FreeBufferShareMemory();

	//
	// UnHook Ndis Function
	//
	if(m_pNdisSend != NULL)
	{
		if(UnHookFunction(m_NdisBaseAddress, "NdisSend", m_pNdisSend) == NULL)
			dprintf(("UnHook NdisSend Failure\n"));
		else
			dprintf(("UnHook NdisSend Success\n"));
		m_pNdisSend = NULL;
	}

	if(m_pNdisRegisterProtocol != NULL)
	{
		if(UnHookFunction(m_NdisBaseAddress, "NdisOpenAdapter", m_pNdisOpenAdapter) == NULL)
			dprintf(("UnHook NdisOpenAdapter Failure\n"));
		else
			dprintf(("UnHook NdisOpenAdapter Success\n"));
		m_pNdisOpenAdapter = NULL;
	}

	if(m_pNdisRegisterProtocol != NULL)
	{
		if(UnHookFunction(m_NdisBaseAddress, "NdisRegisterProtocol", m_pNdisRegisterProtocol) == NULL)
			dprintf(("UnHook NdisRegisterProtocol Failure\n"));
		else
			dprintf(("UnHook NdisRegisterProtocol Success\n"));
		m_pNdisRegisterProtocol = NULL;
	}

	DeviceObject = DriverObject->DeviceObject;
    while (DeviceObject != NULL) 
	{
        OldDeviceObject=DeviceObject;
        DeviceObject=DeviceObject->NextDevice;
		XFilter_Delete(OldDeviceObject);
        //IoDeleteDevice(OldDeviceObject);
    }
}

//
// DeviceIoControl�����������ｫIrpת��ΪPIOCTLPARAMS��ʽ����
//
NTSTATUS
XPacketIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
	PIO_STACK_LOCATION IrpStack;
	IOCTLPARAMS	 IoControl;

	dprintf(("<==XPacketIoControl...\n"));

	IrpStack = IoGetCurrentIrpStackLocation(Irp);

    IoMarkIrpPending(Irp);
    Irp->IoStatus.Status = STATUS_PENDING;

	IoControl.dioc_IOCtlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
	IoControl.dioc_cbInBuf = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	IoControl.dioc_cbOutBuf = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	IoControl.dioc_InBuf = Irp->AssociatedIrp.SystemBuffer;
	//IoControl.dioc_OutBuf = Irp->UserBuffer;
	IoControl.dioc_OutBuf = Irp->AssociatedIrp.SystemBuffer;

	OnW32Deviceiocontrol(&IoControl);

	Irp->IoStatus.Information = IoControl.dioc_cbOutBuf;
    Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

//
// 2002/05/24 move to here
//
HANDLE hXfilter = NULL;

//
// DeviceIoControl������
//
DWORD OnW32Deviceiocontrol(PIOCTLPARAMS pVtoolsD)
{
	PVOID pVoid;
	DWORD* pOutBuffer = (DWORD*)pVtoolsD->dioc_OutBuf;
	dprintf(("--------------DeviceIoControl---------------\n"));

	switch(pVtoolsD->dioc_IOCtlCode)
	{
	case IOCTL_XPACKET_MALLOC_ACL_BUFFER:
		//
		// ����عܹ�����ڴ�ռ�
		//
		pVoid = CreateMemory(*(DWORD*)pVtoolsD->dioc_InBuf);
		dprintf(("IOCTL_XPACKET_MALLOC_ACL_BUFFER size: %u, UserAddress:0x%08X, SystemAddress: 0x%08X\n"
			, *(DWORD*)pVtoolsD->dioc_InBuf
			, pVoid
			, m_SystemVirtualAddress
			));
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	case IOCTL_XPACKET_FREE_ACL_BUFFER:
		//
		// �ͷſعܹ�����ڴ�ռ�
		//
		dprintf(("IOCTL_XPACKET_FREE_ACL_BUFFER SystemAddress:0x%08X\n"
			, m_SystemVirtualAddress
			));
		FreeMemory();
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	case IOCTL_XPACKET_GET_ACL_BUFFER:
		//
		// �õ��عܹ��򻺳����ĵ�ַ
		//
		*pOutBuffer = (DWORD)GetBuffer(*(DWORD*)pVtoolsD->dioc_InBuf);
		dprintf(("IOCTL_XPACKET_FREE_ACL_BUFFER Address:0x%08X, SystemAddress:0x%08X\n"
			, *pOutBuffer
			, m_SystemVirtualAddress
			));
		pVtoolsD->dioc_cbOutBuf = sizeof(DWORD);
		break;
	case IOCTL_XPACKET_ADD_SPI_PORT:
		//
		// ����SPI�˿�
		//
		dprintf(("IOCTL_XPACKET_ADD_SPI_PORT Port: %u\n", *(USHORT*)pVtoolsD->dioc_InBuf));
		AddPort(*(USHORT*)pVtoolsD->dioc_InBuf);
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	case IOCTL_XPACKET_GET_BUFFER_POINT:
		//
		// �õ��������������ص�ַ
		//
		dprintf(("IOCTL_XPACKET_GET_BUFFER_POINT\n"));
		GetPacketPoint((PPACKET_BUFFER_POINT)pOutBuffer);
		pVtoolsD->dioc_cbOutBuf = sizeof(PACKET_BUFFER_POINT);
		break;
	case IOCTL_XPACKET_GET_DIRECTION_POINT:
		//
		// �õ���ǰ���߻���������ص�ַ
		//
		dprintf(("IOCTL_XPACKET_GET_DIRECTION_POINT\n"));
		GetDirectionPoint((PDIRECTION_POINT)pOutBuffer);
		pVtoolsD->dioc_cbOutBuf = sizeof(DIRECTION_POINT);
		break;
	case IOCTL_XPACKET_ADD_NETBIOS_NAME:
		//
		// ����һ�������ھ����ּ�¼
		//
		dprintf(("IOCTL_XPACKET_ADD_NETBIOS_NAME\n"));
		AddName((char*)pVtoolsD->dioc_InBuf + 4, *(DWORD*)pVtoolsD->dioc_InBuf);
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	case IOCTL_XPACKET_DELETE_SPI_PORT:
		//
		// ɾ��һ��SPI�˿�
		//
		dprintf(("IOCTL_XPACKET_DELETE_SPI_PORT Port: %u\n", *(USHORT*)pVtoolsD->dioc_InBuf));
		DeletePort(*(USHORT*)pVtoolsD->dioc_InBuf);
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	case IOCTL_XPACKET_SET_FILTER_MODE:
		//
		// ���ù���ģʽ
		//
		dprintf(("IOCTL_XPACKET_SET_FILTER_MODE FilterMode: %u\n", *(DWORD*)pVtoolsD->dioc_InBuf));
		SetFilterMode(*(DWORD*)pVtoolsD->dioc_InBuf);
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	case IOCTL_XPACKET_SET_XFILTER_HANDLE:
		//
		// ����XFILTER.EXE�ľ��
		//
		dprintf(("IOCTL_XPACKET_SET_XFILTER_HANDLE Xfilter Handle: %u\n", VWIN32_GetCurrentProcessHandle()));
		hXfilter = VWIN32_GetCurrentProcessHandle();
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	case IOCTL_XPACKET_GET_NETBIOS_NAME:
		//
		// �õ������ھ������б�ĵ�һ����¼ָ��
		//
		dprintf(("IOCTL_XPACKET_GET_NETBIOS_NAME NetBios Name Address: %u\n", GetNameList()));
		*pOutBuffer = (DWORD)GetNameList();
		pVtoolsD->dioc_cbOutBuf = sizeof(DWORD);
		break;
	case IOCTL_XPACKET_GET_NAME_FROM_IP:
		//
		// ����IP�õ������ھӵ�����
		//
		GetNameFromIp(*(DWORD*)pVtoolsD->dioc_InBuf, (char*)pOutBuffer);
		pVtoolsD->dioc_cbOutBuf = strlen((char*)pOutBuffer) + 1;
		dprintf(("IOCTL_XPACKET_GET_NAME_FROM_IP, %s\n", (char*)pOutBuffer));
		break;
	case IOCTL_XPACKET_GET_IP_FROM_NAME:
		//
		// ���������ھӵ����ֵõ�IP
		//
		dprintf(("IOCTL_XPACKET_GET_IP_FORM_NAME, %s\n", (char*)pVtoolsD->dioc_InBuf));
		*pOutBuffer = GetIpFromName((char*)pVtoolsD->dioc_InBuf);
		pVtoolsD->dioc_cbOutBuf = sizeof(DWORD);
		break;
	case IOCTL_XPACKET_GET_NETBIOS_NAME_LIST:
		//
		// �õ������ھ����ֵ����м�¼���ݣ����Ƶ�pVtoolsD->dioc_cbOutBuf������
		//
		dprintf(("IOCTL_XPACKET_GET_NETBIOS_NAME_LIST\n"));
		pVtoolsD->dioc_cbOutBuf = GetNameListEx((char*)pOutBuffer, pVtoolsD->dioc_cbOutBuf);
		break;

	//
	// 2002/08/20 add
	//
	case IOCTL_XPACKET_UNMAP_ACL_BUFFER:
		dprintf(("IOCTL_XPACKET_UNMAP_ACL_BUFFER(0x%08X)\n", *(DWORD*)pVtoolsD->dioc_InBuf));
		FreeUserMemory((void*)*(DWORD*)pVtoolsD->dioc_InBuf);
		break;
	case IOCTL_XPACKET_UNMAP_BUFFER_POINT:
		dprintf(("IOCTL_XPACKET_UNMAP_BUFFER_POINT(0x%08X)\n", *(DWORD*)pVtoolsD->dioc_InBuf));
		UnmapMemory(*(DWORD*)pVtoolsD->dioc_InBuf);
		break;

	//
	// 2002/08/21 add
	//
	case IOCTL_XPACKET_REFRESH_HOOK_SEND:
		HookLocalSend();
		break;

	default:
		dprintf(("OTHER Control Code: %u\n", pVtoolsD->dioc_IOCtlCode));
		if(hXfilter != 0 && hXfilter == VWIN32_GetCurrentProcessHandle())
		{
			//
			// ���XFILTER.EXE�˳������ù���ģʽΪֹͣ����
			//
			dprintf(("Set Filter Mode: FALSE\n"));
			hXfilter = 0;
		}
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	}

	return NDIS_STATUS_SUCCESS;
}

//
// IRP_MJ_CREATE
//
NTSTATUS
XPacketOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
	//
	// 2002/08/19
	//
    Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

//
// IRP_MJ_CLOSE
//
NTSTATUS
XPacketClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
	//
	// 2002/05/24 add
	//
	if(hXfilter == VWIN32_GetCurrentProcessHandle())
	{
		SetFilterMode(FALSE);
	}

	//
	// 2002/08/19
	//
    Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

//
// IRP_MJ_CLEANUP
//
NTSTATUS
XPacketCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
	//
	// 2002/08/19
	//
    Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

//
// IRP_MJ_READ
//
NTSTATUS
XPacketRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
	//
	// 2002/08/19
	//
    Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

//
// IRP_MJ_WRITE
//
NTSTATUS
XPacketWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
)
{
	//
	// 2002/08/19
	//
    Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

#pragma comment( exestr, "B9D3B8FD2A7A7263656D67762B")
