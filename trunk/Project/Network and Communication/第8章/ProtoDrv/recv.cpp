/////////////////////////////////////////
// recv.cpp�ļ�
// NDISЭ����ڵ㣬����������ݵ�����

extern "C"
{
	#include <ndis.h>
	#include <ntddk.h>
}

#include "nuiouser.h"
#include "ndisprot.h"



NTSTATUS DispatchRead(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	OPEN_INSTANCE *pOpen = (OPEN_INSTANCE *)pDevObj->DeviceExtension;
	IoIncrement(pOpen);
	do
	{
		if(!pOpen->bBound)
		{
			status = STATUS_DEVICE_NOT_READY;
			break;
		}
		PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(pIrp);
		if(irpSp->Parameters.Read.Length < ETHERNET_HEADER_LENGTH)
		{
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		
			// ����������������ʼ��
		PNDIS_PACKET pPacket;
		NdisAllocatePacket((PNDIS_STATUS)&status, &pPacket, pOpen->hPacketPool);
		if(status != NDIS_STATUS_SUCCESS)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		RESERVED(pPacket)->pIrp = pIrp;
		RESERVED(pPacket)->pMdl = NULL;
		
			// ��ʶ��ǰIRP����δ��������I/O����ȡ������
		IoMarkIrpPending(pIrp);
		IoSetCancelRoutine(pIrp, ReadCancelRoutine);
		
			// ��ӵ�����������б���
		ExInterlockedInsertTailList(&pOpen->RcvList, 
			&RESERVED(pPacket)->ListElement, &pOpen->RcvSpinLock);
		
		return STATUS_PENDING;
		
	}while(FALSE);
	
	if(status != STATUS_SUCCESS)
	{
		IoDecrement(pOpen);
		pIrp->IoStatus.Status = status;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	}
	return status;
}

VOID
ReadCancelRoutine (
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
{
	POPEN_INSTANCE pOpen = (POPEN_INSTANCE)pDeviceObject->DeviceExtension;	
	NDIS_PACKET *pPacket = NULL;

	KIRQL oldIrql = pIrp->CancelIrql;

	KeAcquireSpinLockAtDpcLevel(&pOpen->RcvSpinLock);

	IoReleaseCancelSpinLock(KeGetCurrentIrql());

	// ��������������б����Ҷ�Ӧ�ķ��ָ�롣˫���б�ı�ͷû��ʹ�ã�����Ϊ��ʼ�ͽ������
	PLIST_ENTRY pThisEntry = NULL;
	PACKET_RESERVED *pReserved;
	PLIST_ENTRY pHead = &pOpen->RcvList;
	for(pThisEntry = pHead->Flink; pThisEntry != pHead;  pThisEntry = pThisEntry->Flink)
	{
			pReserved = CONTAINING_RECORD(pThisEntry, PACKET_RESERVED, ListElement);
			if(pReserved->pIrp == pIrp)
			{
				// ���б����Ƴ���δ��Irp�ķ��������
				RemoveEntryList(pThisEntry);
				pPacket = CONTAINING_RECORD(pReserved, NDIS_PACKET, ProtocolReserved);
				break;
			}
	}

	KeReleaseSpinLock(&pOpen->RcvSpinLock, oldIrql);

	if(pPacket != NULL)
	{
		// �ͷŴ˷��������ռ�õ��ڴ�
		NdisFreePacket(pPacket);
		// ��ɴ�δ����IRP����
		pIrp->IoStatus.Status = STATUS_CANCELLED;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		IoDecrement(pOpen);
	}
}


void CancelReadIrp(PDEVICE_OBJECT pDeviceObj)
{
	OPEN_INSTANCE *pOpen = (OPEN_INSTANCE *)pDeviceObj->DeviceExtension;
	PLIST_ENTRY thisEntry;
	PACKET_RESERVED *reserved;
	PNDIS_PACKET myPacket;
	PIRP pPendingIrp;
	// �Ƴ�����δ���Ľ���IRP�����ͷŶ�Ӧ�ķ��������
	while(thisEntry = ExInterlockedRemoveHeadList(&pOpen->RcvList, &pOpen->RcvSpinLock))
	{
		reserved = CONTAINING_RECORD(thisEntry, PACKET_RESERVED, ListElement);
		myPacket = CONTAINING_RECORD(reserved, NDIS_PACKET, ProtocolReserved);
		pPendingIrp = RESERVED(myPacket)->pIrp;

		NdisFreePacket(myPacket);

		IoSetCancelRoutine(pPendingIrp, NULL);

		pPendingIrp->IoStatus.Information = 0;
		pPendingIrp->IoStatus.Status = STATUS_CANCELLED;

		IoCompleteRequest(pPendingIrp, IO_NO_INCREMENT);
		// ��С���������ϵ�IO���ü���
		IoDecrement(pOpen);
	}
}


NDIS_STATUS
ProtocolReceive(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID       HeaderBuffer,
    IN UINT        HeaderBufferSize,
    IN PVOID       LookAheadBuffer,
    IN UINT        LookaheadBufferSize,
    IN UINT        PacketSize
    )
{
	OPEN_INSTANCE *pOpen = (OPEN_INSTANCE *)ProtocolBindingContext;

	if(HeaderBufferSize > ETHERNET_HEADER_LENGTH)
	{
		return NDIS_STATUS_SUCCESS;
	}

		// �ӷ���������б���ȡ��һ��������
	PLIST_ENTRY pListEntry = ExInterlockedRemoveHeadList(&pOpen->RcvList, &pOpen->RcvSpinLock);
	if(pListEntry == NULL)	// û��δ���Ķ�����
	{
		return NDIS_STATUS_NOT_ACCEPTED;
	}

	PACKET_RESERVED *pReserved = CONTAINING_RECORD(pListEntry, PACKET_RESERVED, ListElement);
	NDIS_PACKET *pPacket = CONTAINING_RECORD(pReserved, NDIS_PACKET, ProtocolReserved);
	PIRP pIrp = RESERVED(pPacket)->pIrp;
	PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

	IoSetCancelRoutine(pIrp, NULL);


		// ������̫ͷ��ʵ�ʵĶ�������
	NdisMoveMappedMemory(MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority), 
							HeaderBuffer, HeaderBufferSize);


		// ������̫ͷ��������ݵ���������

	// �����������²��ֵĳ���
	UINT nBufferLen = pIrpSp->Parameters.Read.Length - HeaderBufferSize; // ETHERNET_HEADER_LENGTH;
	// ����ʵ��Ҫ������ֽ�
	UINT nSizeToTransfer = nBufferLen < LookaheadBufferSize ? nBufferLen : LookaheadBufferSize;
	
	// ����һ��MDL��ӳ�������������̫ͷ�Ժ�Ĳ���
	NDIS_STATUS status;
	PMDL pMdl = IoAllocateMdl(MmGetMdlVirtualAddress(pIrp->MdlAddress), 
									MmGetMdlByteCount(pIrp->MdlAddress), FALSE, FALSE, NULL);
	if(pMdl == NULL) 
	{
        status = NDIS_STATUS_RESOURCES;
        goto ERROR;
	}
	
	// ������MDLʹ��ָ�򻺳�������̫ͷ����Ĳ���
    IoBuildPartialMdl(
        pIrp->MdlAddress,
        pMdl,
        ((PUCHAR)MmGetMdlVirtualAddress(pIrp->MdlAddress)) + ETHERNET_HEADER_LENGTH,
        0
        );

	// �����MDL�е���һ������
    pMdl->Next=NULL;
	// �������ָ�룬�Ա������ݴ�����Ϻ��ͷŴ�MDL
    RESERVED(pPacket)->pMdl = pMdl;

	// �������ǵĲ���MDL�����
    NdisChainBufferAtFront(pPacket,pMdl);

    //  ����Mac������������
	UINT nBytesTransfered;
    NdisTransferData(
        &status,
        pOpen->hAdapter,
        MacReceiveContext,
        0,
        nSizeToTransfer,
        pPacket,
        &nBytesTransfered);
    if(status == NDIS_STATUS_PENDING) 
	{
        return NDIS_STATUS_SUCCESS;
    }

ERROR:
	// ���û��δ�������ھ͵����������
    ProtocolTransferDataComplete(
                                pOpen,
                                pPacket,
                                status,
                                nBytesTransfered);
    return NDIS_STATUS_SUCCESS;
}

VOID
ProtocolTransferDataComplete (
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_PACKET  pPacket,
    IN NDIS_STATUS   Status,
    IN UINT          BytesTransfered
    )
{
	OPEN_INSTANCE *pOpen = (OPEN_INSTANCE *)ProtocolBindingContext;
	PMDL pMdl = RESERVED(pPacket)->pMdl;
	PIRP pIrp = RESERVED(pPacket)->pIrp;

		// �����Դ
	if(pMdl != NULL)
		IoFreeMdl(pMdl);
	NdisFreePacket(pPacket);

		// ��ɴ�δ����IRP
	if(Status == NDIS_STATUS_SUCCESS)
	{
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		pIrp->IoStatus.Information = BytesTransfered + ETHERNET_HEADER_LENGTH;
	}
	else
	{
		pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
		pIrp->IoStatus.Information = 0;
	}
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	IoDecrement(pOpen);
}


VOID
ProtocolReceiveComplete(
    IN NDIS_HANDLE  ProtocolBindingContext
    )
{
}








////////////////////////////////////
/*

int ProtocolReceivePacket(NDIS_HANDLE ProtocolBindingContext, PNDIS_PACKET Packet)
{
		DbgPrint("ProtocolReceivePacket \n");
	UINT nBufferLen;
	OPEN_INSTANCE *pOpen = (OPEN_INSTANCE *)ProtocolBindingContext;

	// ȡ��һ��δ���Ķ�����
	PLIST_ENTRY pListEntry = ExInterlockedRemoveHeadList(&pOpen->RcvList, &pOpen->RcvSpinLock);
	if(pListEntry == NULL)
	{
		return 0;
	}

	PACKET_RESERVED *pReserved = CONTAINING_RECORD(pListEntry, PACKET_RESERVED, ListElement);
	NDIS_PACKET *pPacket = CONTAINING_RECORD(pReserved, NDIS_PACKET, ProtocolReserved);

	PIRP pIrp = RESERVED(pPacket)->pIrp;
	PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

	IoSetCancelRoutine(pIrp, NULL);

	NTSTATUS status = STATUS_SUCCESS;

	// ����Ҫ���õ�NdisCopyFromPacketToPacket������������ڴ��Ƿ���ã�Ϊ�˰�ȫ������Ҫ���ȼ��
	{
		PNDIS_BUFFER firstBuffer, nextBuffer;
		UINT totalLength;
		PVOID virtualAddress;
		// �õ����ӵ�����еĵ�һ��������
		NdisQueryPacket(pPacket, NULL, NULL, &firstBuffer, &totalLength);
		while(firstBuffer != NULL)
		{
			// ��ȡ�������ĵ�ַ
			NdisQueryBufferSafe(firstBuffer, &virtualAddress, &totalLength, NormalPagePriority);
			if(virtualAddress == NULL)
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				goto CleanExit;
			}
			NdisGetNextBuffer(firstBuffer, &nextBuffer);
			firstBuffer = nextBuffer;
		}
	}

	// ���Ӷ���������MDL�����������
	NdisChainBufferAtFront(pPacket, pIrp->MdlAddress);
	// ��������С
	nBufferLen = pIrpSp->Parameters.Read.Length;

	UINT nTransfered;
	NdisCopyFromPacketToPacket(pPacket, 0, nBufferLen,Packet,0,&nTransfered);

	// �������
CleanExit:
	pIrp->IoStatus.Information = nTransfered;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	IoDecrement(pOpen);
	return 0;
}
*/