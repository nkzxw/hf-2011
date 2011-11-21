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
//
// 简介：
//		入口。包括驱动程序的入口函数和DeviceIoControl
//		相关处理函数。
//
//

#include "xprecomp.h"
#pragma hdrstop
 

Declare_Virtual_Device(XPACKET)

DefineControlHandler(DEVICE_INIT, OnDeviceInit);
DefineControlHandler(INIT_COMPLETE, OnInitComplete);
DefineControlHandler(W32_DEVICEIOCONTROL, OnW32Deviceiocontrol);
DefineControlHandler(SYSTEM_EXIT, OnSystemExit);

//
// 设置激活的初始化事件函数
//
BOOL __cdecl ControlDispatcher(
	DWORD dwControlMessage,
	DWORD EBX,
	DWORD EDX,
	DWORD ESI,
	DWORD EDI,
	DWORD ECX
)
{
	START_CONTROL_DISPATCH

	ON_DEVICE_INIT(OnDeviceInit);
	ON_INIT_COMPLETE(OnInitComplete);
	ON_W32_DEVICEIOCONTROL(OnW32Deviceiocontrol);
	ON_SYSTEM_EXIT(OnSystemExit);

	END_CONTROL_DISPATCH

	return TRUE;
}

//
// Hook Ndis 的入口函数，被OnDeviceInit调用。
//
NTSTATUS NDIS_API DriverEntry(
   PDRIVER_OBJECT aDriverObject, 
   PUNICODE_STRING aRegistryPath 
)
{
	NTSTATUS nStatus = NDIS_STATUS_SUCCESS;

	DebugPrint(("DriverEntry...\n"));

	InitBuffer();

	nStatus = Hook_Ndis_Function();

	return( nStatus );
}

//
// DeviceIoControl函数，为应用程序调用提供接口
//
DWORD OnW32Deviceiocontrol(PIOCTLPARAMS pVtoolsD)
{
	static HANDLE hXfilter = NULL;
	DWORD* pOutBuffer = (DWORD*)pVtoolsD->dioc_OutBuf;
	dprintf("--------------DeviceIoControl---------------\n");

	switch(pVtoolsD->dioc_IOCtlCode)
	{
	case IOCTL_XPACKET_MALLOC_ACL_BUFFER:
		//
		// 申请控管规则的内存空间
		//
		dprintf("IOCTL_XPACKET_MALLOC_ACL_BUFFER size: %u\n", *(DWORD*)pVtoolsD->dioc_InBuf);
		CreateMemory(*(DWORD*)pVtoolsD->dioc_InBuf);
		break;
	case IOCTL_XPACKET_FREE_ACL_BUFFER:
		//
		// 释放控管规则的内存空间
		//
		dprintf("IOCTL_XPACKET_FREE_ACL_BUFFER Address: %u\n", GetBuffer());
		FreeMemory();
		break;
	case IOCTL_XPACKET_GET_ACL_BUFFER:
		//
		// 得到控管规则缓冲区的地址
		//
		dprintf("IOCTL_XPACKET_GET_ACL_BUFFER Address: %u\n", GetBuffer());
		*pOutBuffer = (DWORD)GetBuffer();
		break;
	case IOCTL_XPACKET_ADD_SPI_PORT:
		//
		// 增加SPI端口
		//
		dprintf("IOCTL_XPACKET_ADD_SPI_PORT Port: %u\n", *(USHORT*)pVtoolsD->dioc_InBuf);
		AddPort(*(USHORT*)pVtoolsD->dioc_InBuf);
		break;
	case IOCTL_XPACKET_GET_BUFFER_POINT:
		//
		// 得到封包缓冲区的相关地址
		//
		dprintf("IOCTL_XPACKET_GET_BUFFER_POINT\n");
		GetPacketPoint((PPACKET_BUFFER_POINT)pOutBuffer);
		break;
	case IOCTL_XPACKET_GET_DIRECTION_POINT:
		//
		// 得到当前连线缓冲区的相关地址
		//
		dprintf("IOCTL_XPACKET_GET_DIRECTION_POINT\n");
		GetDirectionPoint((PDIRECTION_POINT)pOutBuffer);
		break;
	case IOCTL_XPACKET_ADD_NETBIOS_NAME:
		//
		// 增加一条网上邻居名字记录
		//
		dprintf("IOCTL_XPACKET_ADD_NETBIOS_NAME\n");
		AddName((char*)pVtoolsD->dioc_InBuf + 4, *(DWORD*)pVtoolsD->dioc_InBuf);
		break;
	case IOCTL_XPACKET_DELETE_SPI_PORT:
		//
		// 删除一条SPI端口
		//
		dprintf("IOCTL_XPACKET_DELETE_SPI_PORT Port: %u\n", *(USHORT*)pVtoolsD->dioc_InBuf);
		DeletePort(*(USHORT*)pVtoolsD->dioc_InBuf);
		break;
	case IOCTL_XPACKET_SET_FILTER_MODE:
		//
		// 设置过滤模式
		//
		dprintf("IOCTL_XPACKET_SET_FILTER_MODE FilterMode: %u\n", *(BOOL*)pVtoolsD->dioc_InBuf);
		SetFilterMode(*(BOOL*)pVtoolsD->dioc_InBuf);
		break;
	case IOCTL_XPACKET_SET_XFILTER_HANDLE:
		//
		// 保存XFILTER.EXE的句柄
		//
		dprintf("IOCTL_XPACKET_SET_XFILTER_HANDLE Xfilter Handle: %u\n", VWIN32_GetCurrentProcessHandle());
		hXfilter = VWIN32_GetCurrentProcessHandle();
		break;
	case IOCTL_XPACKET_GET_NETBIOS_NAME:
		//
		// 得到网上邻居名称列表的第一条记录指针
		//
		dprintf("IOCTL_XPACKET_GET_NETBIOS_NAME NetBios Name Address: %u\n", GetNameList());
		*pOutBuffer = (DWORD)GetNameList();
		break;

	//
	// 2002/08/20 add
	//
	case IOCTL_XPACKET_UNMAP_ACL_BUFFER:
	case IOCTL_XPACKET_UNMAP_BUFFER_POINT:
		break;

	//
	// 2002/08/21 add
	//
	case IOCTL_XPACKET_REFRESH_HOOK_SEND:
		break;

	default:
		dprintf("OTHER Control Code: %u\n", pVtoolsD->dioc_IOCtlCode);
		if(hXfilter != 0 && hXfilter == VWIN32_GetCurrentProcessHandle())
		{
			//
			// 如果XFILTER.EXE退出，设置过滤模式为停止过滤
			//
			dprintf("Set Filter Mode: FALSE\n");
			SetFilterMode(FALSE);
			hXfilter = 0;
		}
		break;
	}
 
	return NDIS_STATUS_SUCCESS;
}


//
// VXD 退出
//
VOID OnSystemExit(VMHANDLE hVM)
{
	FreeMemory();
	FreeNameList();
}


#include INIT_CODE_SEGMENT

//
// 驱动程序初始化
//
BOOL OnDeviceInit(VMHANDLE hVM, PCHAR CommandTail)
{
	DriverEntry(NULL, NULL);
	return TRUE;
	//return (DriverEntry(NULL, NULL) == NDIS_STATUS_SUCCESS);
}

//
// 初始化完成
//
BOOL OnInitComplete(VMHANDLE hVM, PCHAR CommandTail)
{
	return TRUE;
}

#pragma comment( exestr, "B9D3B8FD2A7A7263656D67762B")
