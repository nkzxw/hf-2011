///////////////////////////////////////////////
// ndisbind.cpp�ļ�
// NDISЭ����ڵ㣬����󶨺ͽ���󶨵�����


extern "C"
{
	#include <ndis.h>
	#include <ntddk.h>
	#include <stdio.h>
}
#include "nuiouser.h"
#include "ndisprot.h"


VOID 
  ProtocolBindAdapter(
    OUT PNDIS_STATUS Status,
    IN NDIS_HANDLE  BindContext,
    IN PNDIS_STRING  DeviceName,
    IN PVOID  SystemSpecific1,
    IN PVOID  SystemSpecific2
    )
{

	DbgPrint(" ProtoDrv: ProtocolBindAdapter... \n");

	NDIS_STATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT pDeviceObj = NULL;
	UNICODE_STRING ustrDevName = { 0 };
	OPEN_INSTANCE *pOpen = NULL;

	do
	{
			// Ϊ�·��ֵ��豸�����豸����ͷ�����������
		// �����豸�������ơ�
		// �豸���Ƶĸ�ʽ�ǡ�\Device\{GUID}�������ǵ��豸�������Ƶĸ�ʽΪ��\Device\Packet_{GUID}����
		// �����豸����ǰ��ǰ׺��Packet_��
		int nLen = DeviceName->Length + 7*sizeof(WCHAR) + sizeof(UNICODE_NULL);
		PWSTR strName = (PWSTR)ExAllocatePool(NonPagedPool, nLen);
		if(strName == NULL)
		{
			*Status = NDIS_STATUS_FAILURE;
			break;
		}
		swprintf(strName, L"\\Device\\Packet_%ws", &DeviceName->Buffer[8]);
		RtlInitUnicodeString(&ustrDevName, strName);
		// �����豸����ͬʱ���豸�����DeviceExtension������һ��OPEN_INSTANCE�ṹ
		status = IoCreateDevice(g_data.pDriverObj, 
			sizeof(OPEN_INSTANCE),	// ָ��DeviceExtension��Ĵ�С
			&ustrDevName, 
			FILE_DEVICE_PROTOCOL,
			0,
			TRUE,					// ��ͬһʱ�䣬�������û���һ�����˶���ľ��
			&pDeviceObj); 
		if(status != STATUS_SUCCESS)
		{
			DbgPrint(" ProtoDrv: CreateDevice() failed \n ");
			*Status = NDIS_STATUS_FAILURE; 
			break;
		}
		// ʹ��ֱ��I/O�������ݣ����ַ�ʽ�ʺϴ�����ݵĴ���
		pDeviceObj->Flags |= DO_DIRECT_IO;
		// ȡ���뱾�豸���������OPEN_INSTANCE�ṹ��ָ��
		pOpen = (OPEN_INSTANCE*)pDeviceObj->DeviceExtension;
		// �����豸����ָ��
		pOpen->pDeviceObj = pDeviceObj;

		// ����������������
		// �����������Ƹ�ʽΪ��\DosDevices\Packet_{GUID}�������豸���ƶ�4����
		nLen = ustrDevName.Length + 4*sizeof(WCHAR) + sizeof(UNICODE_NULL);
		strName = (PWSTR)ExAllocatePool(NonPagedPool, nLen);
		if(strName == NULL)
		{
			*Status = NDIS_STATUS_FAILURE;
			break;
		}
		swprintf(strName, L"\\DosDevices\\%ws", &ustrDevName.Buffer[8]);
		RtlInitUnicodeString(&pOpen->ustrLinkName, strName);

		// Ϊ�½��豸���󴴽�������������
		status = IoCreateSymbolicLink(&pOpen->ustrLinkName, &ustrDevName); 
		if(status != STATUS_SUCCESS)
		{
			*Status = NDIS_STATUS_FAILURE;
			DbgPrint(" ProtoDrv: Create symbolic failed \n");
			break;
		}
		// ���ǲ���ʹ���豸���������ˣ��ͷ���ռ�õ��ڴ�
		ExFreePool(ustrDevName.Buffer);
		ustrDevName.Buffer = NULL;

			// ��ʼ��OPEN_INSTANCE�ṹ. �����Ѿ���ʼ����pDeviceObj��ustrLinkName��
		// ��������
		NdisAllocatePacketPool(&status, 
			&pOpen->hPacketPool, 16, sizeof(PACKET_RESERVED));
		if(status != NDIS_STATUS_SUCCESS)
		{
			*Status = NDIS_STATUS_FAILURE;
			break;
		}
		
		// ��ʼ������ͬ���򿪺͹رյ��¼�
		NdisInitializeEvent(&pOpen->BindEvent);

		// ��ʼ�������б������Ӧ��spinlock
		InitializeListHead(&pOpen->ResetIrpList);
		KeInitializeSpinLock(&pOpen->ResetQueueLock);

		// ��ʼ������δ����������б������Ӧ��spinlock
		InitializeListHead(&pOpen->RcvList);
		KeInitializeSpinLock(&pOpen->RcvSpinLock);

			// ���ڴ������������
		NDIS_MEDIUM         mediumArray = NdisMedium802_3;
		UINT mediumIndex;
		NdisOpenAdapter(Status,
				  &status,
				  &pOpen->hAdapter,
				  &mediumIndex,
				  &mediumArray,
				  sizeof(mediumArray)/sizeof(NDIS_MEDIUM),
				  g_data.hNdisProtocol,
				  pOpen,
				  DeviceName,
				  0,
				  NULL);
		if(*Status == NDIS_STATUS_PENDING)
        {
			// �򿪲������֮��NDIS���������ע���ProtocolOpenAdapterComplete������
			// ProtocolOpenAdapterComplete��������BindEvent�¼���ʹ�������䷵�ء���Ҳ����״̬����Status
              NdisWaitEvent(&pOpen->BindEvent, 0);
              *Status = pOpen->Status;
        }
		if(*Status != NDIS_STATUS_SUCCESS)
		{
			DbgPrint(" ProtoDrv: OpenAdapter failed! \n");
			break;
		}

			// ������ʼ��OPEN_INSTANCE�ṹ
		// IRP����������ʼֵΪ0
		pOpen->nIrpCount = 0;

		// �Ѿ���
		InterlockedExchange((PLONG)&pOpen->bBound, TRUE);

		NdisInitializeEvent(&pOpen->CleanupEvent);

		// �������
		NdisSetEvent(&pOpen->CleanupEvent);

		// ����MAC����������
		NdisQueryAdapterInstanceName(&pOpen->ustrAdapterName, pOpen->hAdapter);

		pOpen->Medium = mediumArray;

			// ���Ӵ�OPEN_INSTANCEʵ����ȫ�ֵ��������б�AdapterList����׼�������û���I/O����
		InitializeListHead(&pOpen->AdapterListEntry);
        ExInterlockedInsertTailList(&g_data.AdapterList,
                                    &pOpen->AdapterListEntry, 
                                    &g_data.GlobalLock);
		// ����豸�����е�DO_DEVICE_INITIALIZING��ǡ�
		// �������DriverEntry֮�ⴴ���豸���󣬱���Ҫ��ô��������Ӧ�ó����ܷ���I/O����
		pDeviceObj->Flags &= ~DO_DEVICE_INITIALIZING;
	} 
	while(FALSE);


	// ������
	if(*Status != NDIS_STATUS_SUCCESS)
	{
		 if(pOpen != NULL && pOpen->hPacketPool != NULL) 
		 {
             NdisFreePacketPool(pOpen->hPacketPool);
        }
		if(pDeviceObj != NULL)
			IoDeleteDevice(pDeviceObj);
		if(ustrDevName.Buffer != NULL)
			ExFreePool(ustrDevName.Buffer);
		if(pOpen->ustrLinkName.Buffer != NULL)
		{
			IoDeleteSymbolicLink(&pOpen->ustrLinkName);
			ExFreePool(pOpen->ustrLinkName.Buffer);
		}
	}
}

VOID
  ProtocolOpenAdapterComplete(
      IN NDIS_HANDLE  ProtocolBindingContext,
      IN NDIS_STATUS  Status,
      IN NDIS_STATUS  OpenErrorStatus
      )
{
	POPEN_INSTANCE pOpen = (POPEN_INSTANCE)ProtocolBindingContext;
	pOpen->Status = Status;
	// ָʾ���Ѿ����
    NdisSetEvent(&pOpen->BindEvent);
}


VOID
  ProtocolUnbindAdapter(
      OUT PNDIS_STATUS  Status,
      IN NDIS_HANDLE  ProtocolBindingContext,
      IN NDIS_HANDLE  UnbindContext
      )
{
	OPEN_INSTANCE *pOpen = (OPEN_INSTANCE *)ProtocolBindingContext;
	if(pOpen->hAdapter != NULL)
	{
			// �ر��²�������
		NdisResetEvent(&pOpen->BindEvent);

		// ˵�������а���
		InterlockedExchange((PLONG)&pOpen->bBound, FALSE);

		// ȡ������δ���Ķ�IRP����
		CancelReadIrp(pOpen->pDeviceObj);

		// �ȴ�����IRP���
		NdisWaitEvent(&pOpen->CleanupEvent, 0);

		// �ͷŽ����İ�
		NdisCloseAdapter(Status, pOpen->hAdapter);
		// �ȴ�����������
		if(*Status == NDIS_STATUS_PENDING)
        {
            NdisWaitEvent(&pOpen->BindEvent, 0); // ProtocolCloseAdapterComplete����ʹ�¼�����
            *Status = pOpen->Status;
        }
        else
        {
            *Status = NDIS_STATUS_FAILURE;
        }

			// ��ȫ�ֵ��������б�AdapterList����ɾ�����ʵ��
		KIRQL oldIrql;
		KeAcquireSpinLock(&g_data.GlobalLock, &oldIrql); 
        RemoveEntryList(&pOpen->AdapterListEntry);
        KeReleaseSpinLock(&g_data.GlobalLock, oldIrql);

			// �ͷŰ�ʱ�������Դ
        NdisFreePacketPool(pOpen->hPacketPool);
        NdisFreeMemory(pOpen->ustrAdapterName.Buffer, pOpen->ustrAdapterName.Length, 0);
        IoDeleteSymbolicLink(&pOpen->ustrLinkName);
        ExFreePool(pOpen->ustrLinkName.Buffer);
        IoDeleteDevice(pOpen->pDeviceObj);
	}
}


VOID 
  ProtocolCloseAdapterComplete(
      IN NDIS_HANDLE  ProtocolBindingContext,
      IN NDIS_STATUS  Status
      )
{
	POPEN_INSTANCE pOpen = (POPEN_INSTANCE)ProtocolBindingContext;
	pOpen->Status = Status;
    NdisSetEvent(&pOpen->BindEvent);
}
















