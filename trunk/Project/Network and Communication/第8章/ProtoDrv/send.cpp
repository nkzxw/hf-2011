///////////////////////////////////////////////
// send.cpp�ļ�
// NDISЭ����ڣ����������ݵ�����


extern "C"
{
	#include <ndis.h>
	#include <ntddk.h>
}

#include "nuiouser.h"
#include "ndisprot.h"

NTSTATUS DispatchWrite(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status;

	// ȡ��������������OPEN_INSTANCE�ṹ��ָ��
	OPEN_INSTANCE *pOpen = (OPEN_INSTANCE *)pDevObj->DeviceExtension;
	// ����IO���ü���
	IoIncrement(pOpen);
	do
	{
		if(!pOpen->bBound)
		{
			status = STATUS_DEVICE_NOT_READY;
			break;
		}

		// �ӷ����������һ�����
		PNDIS_PACKET pPacket;
		NdisAllocatePacket((NDIS_STATUS*)&status, &pPacket, pOpen->hPacketPool);
		if(status != NDIS_STATUS_SUCCESS)	// ������������ˣ�
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		RESERVED(pPacket)->pIrp = pIrp; // ����IRPָ�룬����������л�Ҫʹ��

		// ����д�����������
		NdisChainBufferAtFront(pPacket, pIrp->MdlAddress);

		// ע�⣬��Ȼ�����Ѿ���ʶ��IRPδ�������Ǳ��뷵��STATUS_PENDING��������
		// ����ǡ��ͬ����������IRP
		IoMarkIrpPending(pIrp);

		// ���ͷ�����²�NIC�豸
		NdisSend((NDIS_STATUS*)&status, pOpen->hAdapter, pPacket);
		if(status != NDIS_STATUS_PENDING)
		{
			ProtocolSendComplete(pOpen, pPacket, status);
		}
		return STATUS_PENDING;		
	}while(FALSE);

	if(status != STATUS_SUCCESS)
	{
		IoDecrement(pOpen);
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = status;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	}
	return status;
}


VOID
ProtocolSendComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_PACKET  pPacket,
    IN NDIS_STATUS   Status
    )
{
	OPEN_INSTANCE *pOpen = (OPEN_INSTANCE *)ProtocolBindingContext;
	PIRP pIrp = RESERVED(pPacket)->pIrp;
	PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

	// �ͷŷ��
	NdisFreePacket(pPacket);
	// ���IRP����
	if(Status == NDIS_STATUS_SUCCESS)
	{
		pIrp->IoStatus.Information = pIrpSp->Parameters.Write.Length;
		pIrp->IoStatus.Status = STATUS_SUCCESS;

		DbgPrint(" ProtoDrv: Send data success \n");
	}
	else
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	}
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	IoDecrement(pOpen);
}






