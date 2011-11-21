#include "precomp.h"
#include "ntstrsafe.h"
PDEVICE_OBJECT gDeviceObject = NULL;

// ���������Լ�����IRP_MJ_CREATE�ķ���
NTSTATUS 
myCreate(
		 IN PDEVICE_OBJECT DeviceObject, 
		 IN PIRP Irp
		 )
{
	// ����������ǵ��豸��ֱ�ӷ��سɹ�
	if(DeviceObject == gDeviceObject)
		return STATUS_SUCCESS;

	// ��Ҫ����ϵͳ����
	return systemCreate(DeviceObject, Irp);
}


// ���������Լ�����IRP_MJ_WRITE�ķ���
NTSTATUS 
myWrite(
		IN PDEVICE_OBJECT DeviceObject, 
		IN PIRP Irp
		)
{
	// �ж��ǲ������ǵ��豸
	if(DeviceObject == gDeviceObject){
		// 
		NTSTATUS status = STATUS_SUCCESS;

		//
		// ����������Ҫ��IRP�е����ݽ�����Ӧ�ı��桢����
		//

		// ֱ�ӷ���
		return status;
	}

	// ��Ҫ����ϵͳ����
	return systemWrite(DeviceObject, Irp);
}


// ���������Լ�����IRP_MJ_READ�ķ���
NTSTATUS 
myRead(
	   IN PDEVICE_OBJECT DeviceObject, 
	   IN PIRP Irp
	   )
{
	// �ж��ǲ������ǵ��豸
	if(DeviceObject == gDeviceObject){
		// 
		NTSTATUS status = STATUS_SUCCESS;

		//
		// ����������Ҫ��һЩ������䵽IRP�е����ݻ�������
		//

		// ֱ�ӷ���
		return status;
	}

	// ��Ҫ����ϵͳ����
	return systemRead(DeviceObject, Irp);
}


// ���������Լ�����IRP_MJ_DEVICE_CONTROL�ķ���
NTSTATUS 
myDeviceControl(
				IN PDEVICE_OBJECT DeviceObject, 
				IN PIRP Irp
				)
{
	// �ж��ǲ������ǵ��豸
	if(DeviceObject == gDeviceObject){
		// 
		NTSTATUS status = STATUS_SUCCESS;

		//
		// ����������Ҫ��ȡDevice IO Control�ţ�������
		//

		// ֱ�ӷ���
		return status;
	}

	// ��Ҫ����ϵͳ����
	return systemDeviceControl(DeviceObject, Irp);
}

// 
// ʵ�������Լ���AddDevice����
//
NTSTATUS myAddDevice(
					 IN PDRIVER_OBJECT  DriverObject,
					 IN PDEVICE_OBJECT  PhysicalDeviceObject 
					 )
{
	if(gDeviceObject != NULL)
	{
		// �������洴�������Լ����豸���󣬻�����������Ҫ����Դ��
		// Ϊ�����ֲ�ͬʵ�������豸����������ɣ���MyNdisDevice��+HardwareID��
		UNICODE_STRING nameString; 
		WCHAR wcsName[256];
		UNICODE_STRING preName = RTL_CONSTANT_STRING(L"\\Device\\MyNdisDevice");

		// ����ȡ���豸��HDID��
		ULONG nameLength = 0;
		WCHAR wcsHardwareID[256]; //�㹻����
		NTSTATUS status = IoGetDeviceProperty (PhysicalDeviceObject,
			DevicePropertyHardwareID,
			256,
			wcsHardwareID,
			&nameLength);
		if(status != STATUS_SUCCESS){
			KdPrint(("Failed to get hardware ID %x\n", status));
			return status;
		}

		// ���湹���豸��������֣���������Ĺ��򣺡�MyNdisDevice��+ HardwareID��
		RtlInitEmptyUnicodeString( &nameString, wcsName, 256*2);
		RtlCopyUnicodeString( &nameString, &preName);
		//RtlUnicodeStringPrintf(&nameString, L"%wZ_%d_", &preName, 0);
		RtlAppendUnicodeToString( &nameString, wcsHardwareID);

		status = IoCreateDevice(DriverObject,
			0,
			&nameString,
			FILE_DEVICE_UNKNOWN,
			FILE_DEVICE_SECURE_OPEN,
			FALSE,
			&gDeviceObject); 

		// �������ʧ���ˣ�������Ȩ���ú�����ʧ�ܷ���
		// ���������ǵ���������Ҳ��ʧ����
		if(status != STATUS_SUCCESS){
			KdPrint(("Failed to create device %ws\n", nameString));
			return status;
		}
	}

	//ExAllocatePoolWithTag(); //������Դ������

	// 
	// �����Լ���������ȷ�Ĳ���
	//

	// ���ڵ��ñ����Ndis���е�AddDeviceʵ��
	// ǧ��Ҫ���ǣ�����ͻ����ش���
	return systemAddDevice(DriverObject, PhysicalDeviceObject);
}
