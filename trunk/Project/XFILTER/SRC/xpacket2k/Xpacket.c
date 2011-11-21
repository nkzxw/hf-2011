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
//		入口。包括DriverEntry和DeviceIoControl的处理函数。
//
//

#include "xprecomp.h"
#pragma hdrstop
 
#pragma NDIS_INIT_FUNCTION(DriverEntry)

//
// 入口函数，这里用来完成缓冲区初始化、HOOK和设备的创建
// 
NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT		DriverObject,
	IN	PUNICODE_STRING		RegistryPath
)
{
	dprintf(("DriverEntry Loading...\n"));

	//
	// 申请、初始化封包缓冲区
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
	// 创建实现DeviceIoControl的设备
	//
	XFilter_Create(DriverObject, RegistryPath);

	return(0);
}

//
// 卸载驱动程序
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
	// 释放内存
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
// DeviceIoControl处理函数，这里将Irp转化为PIOCTLPARAMS方式处理
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
// DeviceIoControl处理函数
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
		// 申请控管规则的内存空间
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
		// 释放控管规则的内存空间
		//
		dprintf(("IOCTL_XPACKET_FREE_ACL_BUFFER SystemAddress:0x%08X\n"
			, m_SystemVirtualAddress
			));
		FreeMemory();
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	case IOCTL_XPACKET_GET_ACL_BUFFER:
		//
		// 得到控管规则缓冲区的地址
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
		// 增加SPI端口
		//
		dprintf(("IOCTL_XPACKET_ADD_SPI_PORT Port: %u\n", *(USHORT*)pVtoolsD->dioc_InBuf));
		AddPort(*(USHORT*)pVtoolsD->dioc_InBuf);
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	case IOCTL_XPACKET_GET_BUFFER_POINT:
		//
		// 得到封包缓冲区的相关地址
		//
		dprintf(("IOCTL_XPACKET_GET_BUFFER_POINT\n"));
		GetPacketPoint((PPACKET_BUFFER_POINT)pOutBuffer);
		pVtoolsD->dioc_cbOutBuf = sizeof(PACKET_BUFFER_POINT);
		break;
	case IOCTL_XPACKET_GET_DIRECTION_POINT:
		//
		// 得到当前连线缓冲区的相关地址
		//
		dprintf(("IOCTL_XPACKET_GET_DIRECTION_POINT\n"));
		GetDirectionPoint((PDIRECTION_POINT)pOutBuffer);
		pVtoolsD->dioc_cbOutBuf = sizeof(DIRECTION_POINT);
		break;
	case IOCTL_XPACKET_ADD_NETBIOS_NAME:
		//
		// 增加一条网上邻居名字记录
		//
		dprintf(("IOCTL_XPACKET_ADD_NETBIOS_NAME\n"));
		AddName((char*)pVtoolsD->dioc_InBuf + 4, *(DWORD*)pVtoolsD->dioc_InBuf);
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	case IOCTL_XPACKET_DELETE_SPI_PORT:
		//
		// 删除一条SPI端口
		//
		dprintf(("IOCTL_XPACKET_DELETE_SPI_PORT Port: %u\n", *(USHORT*)pVtoolsD->dioc_InBuf));
		DeletePort(*(USHORT*)pVtoolsD->dioc_InBuf);
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	case IOCTL_XPACKET_SET_FILTER_MODE:
		//
		// 设置过滤模式
		//
		dprintf(("IOCTL_XPACKET_SET_FILTER_MODE FilterMode: %u\n", *(DWORD*)pVtoolsD->dioc_InBuf));
		SetFilterMode(*(DWORD*)pVtoolsD->dioc_InBuf);
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	case IOCTL_XPACKET_SET_XFILTER_HANDLE:
		//
		// 保存XFILTER.EXE的句柄
		//
		dprintf(("IOCTL_XPACKET_SET_XFILTER_HANDLE Xfilter Handle: %u\n", VWIN32_GetCurrentProcessHandle()));
		hXfilter = VWIN32_GetCurrentProcessHandle();
		pVtoolsD->dioc_cbOutBuf = 0;
		break;
	case IOCTL_XPACKET_GET_NETBIOS_NAME:
		//
		// 得到网上邻居名称列表的第一条记录指针
		//
		dprintf(("IOCTL_XPACKET_GET_NETBIOS_NAME NetBios Name Address: %u\n", GetNameList()));
		*pOutBuffer = (DWORD)GetNameList();
		pVtoolsD->dioc_cbOutBuf = sizeof(DWORD);
		break;
	case IOCTL_XPACKET_GET_NAME_FROM_IP:
		//
		// 根据IP得到网上邻居的名字
		//
		GetNameFromIp(*(DWORD*)pVtoolsD->dioc_InBuf, (char*)pOutBuffer);
		pVtoolsD->dioc_cbOutBuf = strlen((char*)pOutBuffer) + 1;
		dprintf(("IOCTL_XPACKET_GET_NAME_FROM_IP, %s\n", (char*)pOutBuffer));
		break;
	case IOCTL_XPACKET_GET_IP_FROM_NAME:
		//
		// 根据网上邻居的名字得到IP
		//
		dprintf(("IOCTL_XPACKET_GET_IP_FORM_NAME, %s\n", (char*)pVtoolsD->dioc_InBuf));
		*pOutBuffer = GetIpFromName((char*)pVtoolsD->dioc_InBuf);
		pVtoolsD->dioc_cbOutBuf = sizeof(DWORD);
		break;
	case IOCTL_XPACKET_GET_NETBIOS_NAME_LIST:
		//
		// 得到网上邻居名字的所有记录数据，复制到pVtoolsD->dioc_cbOutBuf缓冲区
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
			// 如果XFILTER.EXE退出，设置过滤模式为停止过滤
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
