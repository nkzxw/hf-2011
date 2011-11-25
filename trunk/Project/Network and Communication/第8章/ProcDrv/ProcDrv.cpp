//////////////////////////////////////////////////
// ProcDrv.cpp�ļ�


extern "C"
{
	#include <ntddk.h>
}
#include <devioctl.h>
#include "ProcDrv.h"

// �Զ��庯��������
NTSTATUS DispatchCreateClose(PDEVICE_OBJECT pDevObj, PIRP pIrp);
void DriverUnload(PDRIVER_OBJECT pDriverObj);
NTSTATUS DispatchIoctl(PDEVICE_OBJECT pDevObj, PIRP pIrp);
VOID ProcessCallback(IN HANDLE  hParentId, IN HANDLE  hProcessId, IN BOOLEAN bCreate);

// �����ڲ����ơ������������ơ��¼���������
#define DEVICE_NAME		L"\\Device\\devNTProcDrv"
#define LINK_NAME		L"\\DosDevices\\slNTProcDrv"
#define EVENT_NAME		L"\\BaseNamedObjects\\NTProcDrvProcessEvent"


typedef struct _DEVICE_EXTENSION  // �豸�����˽�д洢
{
    HANDLE  hProcessHandle;	// �¼�������
    PKEVENT ProcessEvent;	// �û����ں�ͨ�ŵ��¼�����ָ��

    HANDLE  hPParentId;		// �ڻص������б��������Ϣ�����û���������ʱ�����ݹ�ȥ
    HANDLE  hPProcessId;
    BOOLEAN bPCreate;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

PDEVICE_OBJECT g_pDeviceObject;


// �����������ʱ����DriverEntry����
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString)
{
	NTSTATUS status = STATUS_SUCCESS;

	// ��ʼ��������ǲ����
	pDriverObj->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
	pDriverObj->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
	pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;
	pDriverObj->DriverUnload = DriverUnload;

		// ��������ʼ���豸����
	// �豸����
	UNICODE_STRING ustrDevName;
	RtlInitUnicodeString(&ustrDevName, DEVICE_NAME);
	// �����豸����
	PDEVICE_OBJECT pDevObj;
	status = IoCreateDevice(pDriverObj, 
				sizeof(DEVICE_EXTENSION), // Ϊ�豸��չ�ṹ����ռ�
				&ustrDevName, 
				FILE_DEVICE_UNKNOWN,
				0,
				FALSE,
				&pDevObj);
	if(!NT_SUCCESS(status))
	{
		return status;
	}
	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

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

	// ���浽�豸�����ָ�룬�����ڽ��̻ص������л�Ҫʹ��
	g_pDeviceObject = pDevObj;

	// Ϊ���û�ģʽ�����ܹ����ӣ������¼�����
	UNICODE_STRING  uszProcessEventString;
    RtlInitUnicodeString(&uszProcessEventString, EVENT_NAME);
    pDevExt->ProcessEvent = IoCreateNotificationEvent(&uszProcessEventString, &pDevExt->hProcessHandle);

    // ������Ϊ������״̬
    KeClearEvent(pDevExt->ProcessEvent);
    // ���ûص�����
    status = PsSetCreateProcessNotifyRoutine(ProcessCallback, FALSE);
	
	return status;
}


void DriverUnload(PDRIVER_OBJECT pDriverObj)
{	
	// �Ƴ����̻ص�����
    PsSetCreateProcessNotifyRoutine(ProcessCallback, TRUE);

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
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	// ��ɴ�����
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


// I/O������ǲ����
NTSTATUS DispatchIoctl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint(" ProcDrv: DispatchIoctl... \n");

	// ����ʧ��
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

	// ȡ�ô�IRP��pIrp����I/O��ջָ��
	PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

	// ȡ���豸��չ�ṹָ��
	PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;

	// ȡ��I/O���ƴ���
	ULONG uIoControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	// ȡ��I/O������ָ������ĳ���
	PCALLBACK_INFO pCallbackInfo = (PCALLBACK_INFO)pIrp->AssociatedIrp.SystemBuffer;
	ULONG uInSize = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG uOutSize = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch(uIoControlCode)
	{
	case IOCTL_NTPROCDRV_GET_PROCINFO:  // ���û����򷵻����¼������Ľ��̵���Ϣ
		{
			if(uOutSize >= sizeof(CALLBACK_INFO))
			{
				pCallbackInfo->hParentId  = pDevExt->hPParentId;
                pCallbackInfo->hProcessId = pDevExt->hPProcessId;
                pCallbackInfo->bCreate    = pDevExt->bPCreate;

                status = STATUS_SUCCESS;
			}
		}
		break;
	}

	if(status == STATUS_SUCCESS)
		pIrp->IoStatus.Information = uOutSize;
	else
		pIrp->IoStatus.Information = 0;


	// �������
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return status;
}

// ���̻ص�����
VOID ProcessCallback(IN HANDLE  hParentId, IN HANDLE  hProcessId, IN BOOLEAN bCreate)
{
	// �õ��豸��չ�ṹ��ָ��
    PDEVICE_EXTENSION pDevExt =  (PDEVICE_EXTENSION)g_pDeviceObject->DeviceExtension;

	// ���ŵ�ǰֵ���豸��չ�ṹ
	// �û�ģʽӦ�ó���ʹ��DeviceIoControl���ð���ȡ��
    pDevExt->hPParentId  = hParentId;
    pDevExt->hPProcessId = hProcessId;
    pDevExt->bPCreate    = bCreate;

	// ��������¼����Ա��κ����ڼ������û�����֪�������鷢���ˡ�
	// �û�ģʽ�µ�Ӧ�ó���������KM�¼�����������Ҫ�����ﴥ����
    KeSetEvent(pDevExt->ProcessEvent, 0, FALSE);
    KeClearEvent(pDevExt->ProcessEvent);
}