/////////////////////////////////////////////////////
// filter.c�ļ�

// ����ļ����������������й�����صĴ���  ���˲���


#include "precomp.h"
#pragma hdrstop

#include "iocommon.h"


#include "protoinfo.h"

// ���˹����б�
typedef struct _PassthruFilterList
{
	PassthruFilter filter;
	struct _PassthruFilterList *pNext;

} PassthruFilterList, *PPassthruFilterList;

// ADAPT�ṹ��FilterReserved����
typedef struct _ADAPT_FILTER_RSVD
{
	BOOLEAN     bFilterInitDone;
	
	//  Per-Adapter������س�Ա
	PassthruStatistics Statistics;		// ��¼����״̬���紫���˶��ٷ���������˶��ٵȵ�
	
	PPassthruFilterList pFilterList;	// ָ������б�
	
}ADAPT_FILTER_RSVD, *PADAPT_FILTER_RSVD;
C_ASSERT(sizeof(ADAPT_FILTER_RSVD) <= sizeof(((PADAPT)0)->FilterReserved));


// OPEN_CONTEXT�ṹ��FilterReserved����.
typedef struct _OPEN_CONTEXT_FILTER_RSVD
{
	BOOLEAN     bFilterInitDone;

	// �����Per-Open-Handle������س�Ա

}OPEN_FILTER_RSVD, *POPEN_FILTER_RSVD;

C_ASSERT(sizeof(OPEN_FILTER_RSVD) <= sizeof(((POPEN_CONTEXT)0)->FilterReserved));



VOID FltOnInitAdapter(PADAPT pAdapt)
{
	PADAPT_FILTER_RSVD   pFilterContext;
	
	//
	// ��ʼ��ADAPT�ṹ�е�FilterReserved��
	//
	pFilterContext = (PADAPT_FILTER_RSVD )&pAdapt->FilterReserved;
}


VOID FltOnDeinitAdapter(PADAPT pAdapt)
{
	PADAPT_FILTER_RSVD   pFilterContext;
	
	//
	// ����ʼ��ADAPT�ṹ�е�FilterReserved��
	//
	pFilterContext = (PADAPT_FILTER_RSVD)&pAdapt->FilterReserved;
	
	ClearFilterList(pFilterContext);
}


/////////////////////////////////////////////////////

// �������������б������һ�����˹���
NTSTATUS AddFilterToAdapter(PADAPT_FILTER_RSVD pFilterContext, PPassthruFilter pFilter)
{
	PPassthruFilterList pNew;
	// Ϊ�µĹ��˹��������ڴ�ռ�
	if(NdisAllocateMemoryWithTag(&pNew, sizeof(PassthruFilterList), TAG) != NDIS_STATUS_SUCCESS)
		return STATUS_INSUFFICIENT_RESOURCES;

	// �������ڴ�
	NdisMoveMemory(&pNew->filter, pFilter, sizeof(PassthruFilter));
	
	// ���ӵ������б���
	pNew->pNext = pFilterContext->pFilterList;
	pFilterContext->pFilterList = pNew;
	
	return STATUS_SUCCESS;
}

// ɾ�������������б��еĹ���
void ClearFilterList(PADAPT_FILTER_RSVD pFilterContext)
{
	PPassthruFilterList pList = pFilterContext->pFilterList;
	PPassthruFilterList pNext;
	// �ͷŹ����б�ռ�õ��ڴ�
	while(pList != NULL)
	{
		pNext = pList->pNext;
		
		NdisFreeMemory(pList, 0, 0); 
		pList = pNext;
	}
	pFilterContext->pFilterList = NULL;
}





// ����Щ����ʶ���IOCTL��PassThru����Ҫ��DevIoControl���̵��ô�����

NTSTATUS FltDevIoControl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	// ����ʧ��
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	
	// ȡ�ô�IRP��pIrp����I/O��ջָ��
	PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	
	// ȡ��I/O���ƴ���
	ULONG uIoControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	// ȡ��I/O������ָ������ĳ���
	PVOID pIoBuffer = pIrp->AssociatedIrp.SystemBuffer;
	ULONG uInSize = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG uOutSize = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	
	ULONG uTransLen = 0;
	
	PADAPT              pAdapt = NULL;
    PADAPT_FILTER_RSVD  pFilterContext = NULL;
    POPEN_CONTEXT       pOpenContext = pIrpStack->FileObject->FsContext;
	
	if(pOpenContext == NULL || (pAdapt = pOpenContext->pAdapt) == NULL)
	{
		status = STATUS_INVALID_HANDLE;
		goto CompleteTheIRP;
	}
	
	pFilterContext = (PADAPT_FILTER_RSVD)&pAdapt->FilterReserved;

	
	//
	// Fail IOCTL If Unbind Is In Progress   Fail IOCTL If Adapter Is Powering Down
	//
	NdisAcquireSpinLock(&pAdapt->Lock);
	
	if( pAdapt->UnbindingInProcess || pAdapt->StandingBy == TRUE)
	{
		NdisReleaseSpinLock(&pAdapt->Lock);
		
		status = STATUS_INVALID_DEVICE_STATE;
		goto CompleteTheIRP;
	}
	
	// ���ı�����ʱ,Ҫӵ��SpinLock
	
	// ���,����IO���ƴ���
	switch(uIoControlCode)
	{
	case IOCTL_PTUSERIO_QUERY_STATISTICS:		// ��ȡ����״̬
		{
			uTransLen = sizeof(PassthruStatistics);
			if(uOutSize < uTransLen)
			{
				status =  STATUS_BUFFER_TOO_SMALL;
				break;
			}
			
			NdisMoveMemory(pIoBuffer, &pFilterContext->Statistics, uTransLen);
			status = STATUS_SUCCESS;
		}
		break;
	case IOCTL_PTUSERIO_RESET_STATISTICS:		// ��������״̬
		{
			NdisZeroMemory(&pFilterContext->Statistics, sizeof(PassthruStatistics));
			status = STATUS_SUCCESS;
		}
		break;
	case IOCTL_PTUSERIO_ADD_FILTER:				// ���һ�����˹���
		{
			if(uInSize >= sizeof(PassthruFilter))
			{
				DBGPRINT((" ���һ�����˹���"));
				status = AddFilterToAdapter(pFilterContext, (PPassthruFilter)pIoBuffer);
			}
			else
			{
				status = STATUS_INVALID_DEVICE_REQUEST;
			}
		}
		break;
	case IOCTL_PTUSERIO_CLEAR_FILTER:			// ������˹���
		{
			DBGPRINT((" ������˹���"));
			ClearFilterList(pFilterContext);
			status = STATUS_SUCCESS;
		}
		break;
	}
	
	NdisReleaseSpinLock(&pAdapt->Lock);
	
CompleteTheIRP:
	
	if(status == STATUS_SUCCESS)
		pIrp->IoStatus.Information = uTransLen;
	else
		pIrp->IoStatus.Information = 0;
	
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return status;
}


////////////////////////////////////////////////////
// ��ȡ����е�����
void FltReadPacketData(PNDIS_PACKET pPacket, 
					   PUCHAR lpBufferIn, ULONG nNumberToRead, PUINT lpNumberOfRead)
{	
	PUCHAR pBuf;
	ULONG nBufferSize;
	PNDIS_BUFFER pBufferDes = NULL;
	
	
	// ������
	if(pPacket == NULL || lpBufferIn == NULL || nNumberToRead == 0)
	{
		if(lpNumberOfRead != NULL)
		{
			*lpNumberOfRead = 0;
			return ;
		}
	}

	// ���÷�������
	*lpNumberOfRead = 0;
	
	
	// ��������еĻ�����������,�����ݸ��Ƶ��û�������
	pBufferDes = pPacket->Private.Head;
	while(pBufferDes != pPacket->Private.Tail && pBufferDes != NULL)
	{
		// ��ȡ�˻�����������Ļ�������Ϣ
		NdisQueryBufferSafe(pBufferDes, &pBuf, &nBufferSize, NormalPagePriority);
		if(pBuf == NULL)
			return;
		
		if(nNumberToRead > nBufferSize) // ��������������
		{
			NdisMoveMemory(lpBufferIn + *lpNumberOfRead, pBuf, nBufferSize);
			nNumberToRead -= nBufferSize;
			*lpNumberOfRead += nBufferSize;
		}
		else							// ������ʣ�µĲ���
		{
			NdisMoveMemory(lpBufferIn + *lpNumberOfRead, pBuf, nNumberToRead);
			*lpNumberOfRead += nNumberToRead;
			return;
		}
		// ��һ��������������
		pBufferDes = pBufferDes->Next;
	}
}

/////////////////////////////////////////////
// �����˹���
BOOLEAN FltCheckFilterRules(PPassthruFilterList pFilterList, PUCHAR pPacketData, ULONG nDataLen, BOOLEAN bIncludeETHdr)
{	
	int nLeavingLen = nDataLen;
	
	PETHeader pEtherHdr;
	PIPHeader pIpHdr;
	PTCPHeader pTcpHdr;
	PUDPHeader pUdpHdr;


	
	// �ӻ���������ȡ��IPͷ
	// ���������̫ͷ����Ҫ�ȼ����̫ͷ
	if(bIncludeETHdr)
	{
		if(nLeavingLen < sizeof(ETHeader))
		{
			return TRUE;
		}
		nLeavingLen -= sizeof(ETHeader);
		
		pEtherHdr = (PETHeader)pPacketData;
		
		if(pEtherHdr->type != 0x8) // �������IPЭ�飬�򲻴���
			return TRUE;
		
		pIpHdr = (PIPHeader)(pEtherHdr + 1);
	}
	else
	{
		pIpHdr = (PIPHeader)pPacketData;
	}
	
	// ��֤ʣ�����ݳ��ȣ���ֹ�����ں˷Ƿ�����
	if(nLeavingLen < sizeof(IPHeader))
		return TRUE;
	nLeavingLen -= sizeof(IPHeader);
	
	
	// ���汾��Ϣ�����ǽ�����IPv4
	if(((pIpHdr->iphVerLen >> 4) & 0x0f) == 6)
	{
		return TRUE;
	}
	
	if(pIpHdr->ipProtocol == 6 && nLeavingLen >= sizeof(TCPHeader))  // ��TCPЭ�飿
	{
		// ��ȡTCPͷ
		pTcpHdr = (PTCPHeader)(pIpHdr + 1);
		// ���ǽ��������Ѿ��������ӵ�TCP���
		if(!(pTcpHdr->flags & 0x02))
		{
			return TRUE;
		}
	}
	
	// ����˹���Ƚϣ�������ȡ���ж�
	while(pFilterList != NULL)
	{
		// �鿴���ʹ�õ�Э���Ƿ�͹��˹�����ͬ
		if(pFilterList->filter.protocol == 0 || pFilterList->filter.protocol == pIpHdr->ipProtocol)
		{
			// ���Э����ͬ���ٲ鿴ԴIP��ַ
			if(pFilterList->filter.sourceIP != 0 &&
				pFilterList->filter.sourceIP != (pFilterList->filter.sourceMask & pIpHdr->ipSource))
			{
				pFilterList = pFilterList->pNext;
				continue;
			}
			
			// �ٲ鿴Ŀ��IP��ַ
			if(pFilterList->filter.destinationIP != 0 &&
				pFilterList->filter.destinationIP != (pFilterList->filter.destinationMask & pIpHdr->ipDestination))
			{
				pFilterList = pFilterList->pNext;
				continue;
			}
			
			// �����TCP��������Ų鿴TCP�˿ں�
			if(pIpHdr->ipProtocol == 6)
			{
				if(nLeavingLen < 4)
				{
					return TRUE;
				}
				pTcpHdr = (PTCPHeader)(pIpHdr + 1);
				// ���Դ�˿ںź�Ŀ�Ķ˿ںŶ�������е�һ�������չ���ļ�¼����������
				if(pFilterList->filter.sourcePort == 0 || pFilterList->filter.sourcePort == pTcpHdr->sourcePort)
				{
					if(pFilterList->filter.destinationPort == 0 ||
						pFilterList->filter.destinationPort == pTcpHdr->destinationPort)
					{
						DBGPRINT((" ���չ�����һ��TCP��� \n "));
						return !pFilterList->filter.bDrop; 
					}
				}
				
			}
			// �����UDP��������Ų鿴UDP�˿ں�
			else if(pIpHdr->ipProtocol == 17)
			{
				if(nLeavingLen < 4)
				{
					return !pFilterList->filter.bDrop;
				}
				pUdpHdr = (PUDPHeader)(pIpHdr + 1);
				if(pFilterList->filter.sourcePort == 0 || 
					pFilterList->filter.sourcePort == pUdpHdr->sourcePort)
				{
					if(pFilterList->filter.destinationPort == 0 ||
							pFilterList->filter.destinationPort == pUdpHdr->destinationPort)
					{
						DBGPRINT((" ���չ�����һ��UDP��� \n "));
						return !pFilterList->filter.bDrop; 
					}
				}
			}
			else
			{
				// �����������������ֱ�Ӵ���
				return !pFilterList->filter.bDrop; 
			}
		}
		// �Ƚ���һ�����
		pFilterList = pFilterList->pNext;
	}

	
	// Ĭ������½������з��
	return TRUE;
}



// �������ⷢ�͵����ݣ���MPSendPackets����MPSend��������
// �����MPSendPackets���þ�������IRQL <= DISPATCH_LEVEL����
// �����MPSend���ã���������IRQL == DISPATCH_LEVEL����
BOOLEAN FltFilterSendPacket(
				IN PADAPT          pAdapt,
				IN PNDIS_PACKET   pSendPacket,
				IN BOOLEAN         bDispatchLevel  // TRUE -> IRQL == DISPATCH_LEVEL
	)
{
	BOOLEAN bPass = TRUE;
	PADAPT_FILTER_RSVD pFilterContext = (PADAPT_FILTER_RSVD)&pAdapt->FilterReserved;
	UCHAR buffer[MAX_PACKET_HEADER_LEN];
	ULONG nReadBytes;
	
	// ��ʹ�ù�������ʱ��Ҫ��ȡ��ת��
	if(bDispatchLevel)
	{
		NdisDprAcquireSpinLock(&pAdapt->Lock);
	}
	else
	{
		NdisAcquireSpinLock(&pAdapt->Lock);
	}
	
	// ����ͳ������
	pFilterContext->Statistics.nMPSendPktsCt ++;
	
	// ���û�����ù��˹�����������з��
	if(pFilterContext->pFilterList == NULL)
		goto ExitTheFilter;
	
	////////////////////////////////////////////////////
	// ��ȡ����е����ݣ��������ȡ���ͷ����
	FltReadPacketData(pSendPacket, buffer, MAX_PACKET_HEADER_LEN, &nReadBytes);
	// �����˹��򣬿����Ƿ�����������ͨ��
	bPass = FltCheckFilterRules(pFilterContext->pFilterList, buffer, nReadBytes, TRUE);
	
	if(!bPass)
	{
		// �ܾ���һ�����
		pFilterContext->Statistics.nMPSendPktsDropped ++;
	}
	
ExitTheFilter:
	// ����֮��Ҫ�ͷ���ת��
	if(bDispatchLevel)
		NdisDprReleaseSpinLock(&pAdapt->Lock);
	else
		NdisReleaseSpinLock(&pAdapt->Lock);
	
	return bPass;
}



// ���˽��յ�������,��PtReceivePacket�������ã�������DISPATCH_LEVEL IRQL����
BOOLEAN FltFilterReceivePacket(
				IN PADAPT         pAdapt,
				IN	PNDIS_PACKET   pReceivedPacket
				)
{
	BOOLEAN bPass = TRUE;
	PADAPT_FILTER_RSVD pFilterContext = (PADAPT_FILTER_RSVD)&pAdapt->FilterReserved;
	UCHAR buffer[MAX_PACKET_HEADER_LEN];
	ULONG nReadBytes;
	
	// ��ʹ�ù�������ʱ��Ҫ��ȡ��ת��
	NdisDprAcquireSpinLock(&pAdapt->Lock);
	
	
	// ����ͳ������
	pFilterContext->Statistics.nPTRcvPktCt ++;

	// ���û�����ù��˹�����������з��
	if(pFilterContext->pFilterList == NULL)
		goto ExitTheFilter;
	
	////////////////////////////////////////////////////
	// ��ȡ����е����ݣ��������ȡ���ͷ����
	FltReadPacketData(pReceivedPacket, buffer, MAX_PACKET_HEADER_LEN, &nReadBytes);
	if(nReadBytes != MAX_PACKET_HEADER_LEN)
	{
		DBGPRINT(("  FltFilterReceivePacket:  nReadBytes != MAX_PACKET_HEADER_LEN"));
	}
	// �����˹��򣬿����Ƿ�����������ͨ��
	bPass = FltCheckFilterRules(pFilterContext->pFilterList,buffer, nReadBytes, TRUE);
	if(!bPass)
	{
		// �ܾ���һ�����
		pFilterContext->Statistics.nPTRcvPktDropped ++;
	}

ExitTheFilter:
	// ����֮��Ҫ�ͷ���ת��
	NdisDprReleaseSpinLock(&pAdapt->Lock);
	
	return bPass;
}

// ���˽��յ�������,��PtReceivePacket�������ã�������DISPATCH_LEVEL IRQL����
BOOLEAN FltFilterReceive(
					IN PADAPT         pAdapt,
					IN NDIS_HANDLE    MacReceiveContext,
					IN PVOID          HeaderBuffer,
					IN UINT           HeaderBufferSize,
					IN PVOID          LookAheadBuffer,
					IN UINT           LookAheadBufferSize,
					IN UINT           PacketSize
					)
{
	BOOLEAN bPass = TRUE;
	PADAPT_FILTER_RSVD pFilterContext = (PADAPT_FILTER_RSVD)&pAdapt->FilterReserved;
	PETHeader pEtherHdr = (PETHeader)HeaderBuffer;
	
	// ��ʹ�ù�������ʱ��Ҫ��ȡ��ת��
	NdisDprAcquireSpinLock(&pAdapt->Lock);
	
	
	// ����ͳ������
	pFilterContext->Statistics.nPTRcvCt ++;
	// ���û�����ù��˹�����������з��
	if(pFilterContext->pFilterList == NULL)
		goto ExitTheFilter;
	
	// �������IPЭ�飬�����
	if(pEtherHdr->type != 0x8)  
		goto ExitTheFilter;
	
	// �����˹��򣬿����Ƿ�����������ͨ��
	bPass = FltCheckFilterRules(pFilterContext->pFilterList,LookAheadBuffer, LookAheadBufferSize, FALSE);
	if(!bPass)
	{
		// �ܾ���һ�����
		pFilterContext->Statistics.nPTRcvDropped ++;
	}
	
ExitTheFilter:
	// ����֮��Ҫ�ͷ���ת��
    NdisDprReleaseSpinLock(&pAdapt->Lock);

	return bPass;
}




