//////////////////////////////////////////////////
// DriverDemo.cpp�ļ�

//---------------------------DriverDemo.cpp�ļ�--------------------------------------//
extern "C"
{
	#include <ntddk.h>
}

// �Զ��庯��������
NTSTATUS DispatchCreateClose(PDEVICE_OBJECT pDevObj, PIRP pIrp);
void DriverUnload(PDRIVER_OBJECT pDriverObj);

// �����ڲ����ƺͷ�����������
#define DEVICE_NAME L"\\Device\\devDriverDemo"
#define LINK_NAME L"\\??\\slDriverDemo"

// �����������ʱ����DriverEntry����
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString)
{
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint(" DriverDemo: DriverEntry... \n");

	// ��ʼ��������ǲ����
	pDriverObj->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
	pDriverObj->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
	pDriverObj->DriverUnload = DriverUnload;

		// ��������ʼ���豸����
	// �豸����
	UNICODE_STRING ustrDevName;
	RtlInitUnicodeString(&ustrDevName, DEVICE_NAME);
	// �����豸����
	PDEVICE_OBJECT pDevObj;
	status = IoCreateDevice(pDriverObj, 
				0,
				&ustrDevName, 
				FILE_DEVICE_UNKNOWN,
				0,
				FALSE,
				&pDevObj);
	if(!NT_SUCCESS(status))
	{
		return status;
	}

		// ����������������
	// ������������
	UNICODE_STRING ustrLinkName;
	RtlInitUnicodeString(&ustrLinkName, LINK_NAME);
	// ��������
	status = IoCreateSymbolicLink(&ustrLinkName, &ustrDevName);  
	if(!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);  
		return status;
	}
	return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT pDriverObj)
{	
	DbgPrint(" DriverDemo: DriverUnload... \n");

	// ɾ��������������
	UNICODE_STRING strLink;
	RtlInitUnicodeString(&strLink, LINK_NAME);
	IoDeleteSymbolicLink(&strLink);
	// ɾ���豸����
	IoDeleteDevice(pDriverObj->DeviceObject);
}

// ����IRP_MJ_CREATE��IRP_MJ_CLOSE���ܴ���
NTSTATUS DispatchCreateClose(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint(" DriverDemo: DispatchCreateClose... \n");

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	// ��ɴ�����
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
