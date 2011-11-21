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
//
// ��飺
//		��ڡ����������������ں�����DeviceIoControl
//		��ش�������
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
// ���ü���ĳ�ʼ���¼�����
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
// Hook Ndis ����ں�������OnDeviceInit���á�
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
// DeviceIoControl������ΪӦ�ó�������ṩ�ӿ�
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
		// ����عܹ�����ڴ�ռ�
		//
		dprintf("IOCTL_XPACKET_MALLOC_ACL_BUFFER size: %u\n", *(DWORD*)pVtoolsD->dioc_InBuf);
		CreateMemory(*(DWORD*)pVtoolsD->dioc_InBuf);
		break;
	case IOCTL_XPACKET_FREE_ACL_BUFFER:
		//
		// �ͷſعܹ�����ڴ�ռ�
		//
		dprintf("IOCTL_XPACKET_FREE_ACL_BUFFER Address: %u\n", GetBuffer());
		FreeMemory();
		break;
	case IOCTL_XPACKET_GET_ACL_BUFFER:
		//
		// �õ��عܹ��򻺳����ĵ�ַ
		//
		dprintf("IOCTL_XPACKET_GET_ACL_BUFFER Address: %u\n", GetBuffer());
		*pOutBuffer = (DWORD)GetBuffer();
		break;
	case IOCTL_XPACKET_ADD_SPI_PORT:
		//
		// ����SPI�˿�
		//
		dprintf("IOCTL_XPACKET_ADD_SPI_PORT Port: %u\n", *(USHORT*)pVtoolsD->dioc_InBuf);
		AddPort(*(USHORT*)pVtoolsD->dioc_InBuf);
		break;
	case IOCTL_XPACKET_GET_BUFFER_POINT:
		//
		// �õ��������������ص�ַ
		//
		dprintf("IOCTL_XPACKET_GET_BUFFER_POINT\n");
		GetPacketPoint((PPACKET_BUFFER_POINT)pOutBuffer);
		break;
	case IOCTL_XPACKET_GET_DIRECTION_POINT:
		//
		// �õ���ǰ���߻���������ص�ַ
		//
		dprintf("IOCTL_XPACKET_GET_DIRECTION_POINT\n");
		GetDirectionPoint((PDIRECTION_POINT)pOutBuffer);
		break;
	case IOCTL_XPACKET_ADD_NETBIOS_NAME:
		//
		// ����һ�������ھ����ּ�¼
		//
		dprintf("IOCTL_XPACKET_ADD_NETBIOS_NAME\n");
		AddName((char*)pVtoolsD->dioc_InBuf + 4, *(DWORD*)pVtoolsD->dioc_InBuf);
		break;
	case IOCTL_XPACKET_DELETE_SPI_PORT:
		//
		// ɾ��һ��SPI�˿�
		//
		dprintf("IOCTL_XPACKET_DELETE_SPI_PORT Port: %u\n", *(USHORT*)pVtoolsD->dioc_InBuf);
		DeletePort(*(USHORT*)pVtoolsD->dioc_InBuf);
		break;
	case IOCTL_XPACKET_SET_FILTER_MODE:
		//
		// ���ù���ģʽ
		//
		dprintf("IOCTL_XPACKET_SET_FILTER_MODE FilterMode: %u\n", *(BOOL*)pVtoolsD->dioc_InBuf);
		SetFilterMode(*(BOOL*)pVtoolsD->dioc_InBuf);
		break;
	case IOCTL_XPACKET_SET_XFILTER_HANDLE:
		//
		// ����XFILTER.EXE�ľ��
		//
		dprintf("IOCTL_XPACKET_SET_XFILTER_HANDLE Xfilter Handle: %u\n", VWIN32_GetCurrentProcessHandle());
		hXfilter = VWIN32_GetCurrentProcessHandle();
		break;
	case IOCTL_XPACKET_GET_NETBIOS_NAME:
		//
		// �õ������ھ������б�ĵ�һ����¼ָ��
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
			// ���XFILTER.EXE�˳������ù���ģʽΪֹͣ����
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
// VXD �˳�
//
VOID OnSystemExit(VMHANDLE hVM)
{
	FreeMemory();
	FreeNameList();
}


#include INIT_CODE_SEGMENT

//
// ���������ʼ��
//
BOOL OnDeviceInit(VMHANDLE hVM, PCHAR CommandTail)
{
	DriverEntry(NULL, NULL);
	return TRUE;
	//return (DriverEntry(NULL, NULL) == NDIS_STATUS_SUCCESS);
}

//
// ��ʼ�����
//
BOOL OnInitComplete(VMHANDLE hVM, PCHAR CommandTail)
{
	return TRUE;
}

#pragma comment( exestr, "B9D3B8FD2A7A7263656D67762B")
