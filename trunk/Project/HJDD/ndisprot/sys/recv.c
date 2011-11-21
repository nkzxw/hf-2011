/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    recv.c

Abstract:

    NDIS protocol entry points and utility routines to handle receiving
    data.

Environment:

    Kernel mode only.

Revision History:

--*/

#include "precomp.h"

#define __FILENUMBER 'VCER'



// �ַ�����֮һ�����������
NTSTATUS
NdisProtRead(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp
    )
{
    PIO_STACK_LOCATION      pIrpSp;
    NTSTATUS                NtStatus;
    PNDISPROT_OPEN_CONTEXT   pOpenContext;

    UNREFERENCED_PARAMETER(pDeviceObject);
    
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pOpenContext = pIrpSp->FileObject->FsContext;

    do
    {
        // ���������ĵĿɿ���
        if (pOpenContext == NULL)
        {
            DEBUGP(DL_FATAL, ("Read: NULL FsContext on FileObject %p\n",
                        pIrpSp->FileObject));
            NtStatus = STATUS_INVALID_HANDLE;
            break;
        }
            
        NPROT_STRUCT_ASSERT(pOpenContext, oc);

        // Read��Write����ʹ�õ�ֱ��IO����������Ӧ��ʹ��MdlAddress
        // �����ݻ��塣������ǣ����طǷ���������
        if (pIrp->MdlAddress == NULL)
        {
            DEBUGP(DL_FATAL, ("Read: NULL MDL address on IRP %p\n", pIrp));
            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }

        // �õ�����������ַ��
        if (MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority) == NULL)
        {
            DEBUGP(DL_FATAL, ("Read: MmGetSystemAddr failed for IRP %p, MDL %p\n",
                    pIrp, pIrp->MdlAddress));
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock);

        if (!NPROT_TEST_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_ACTIVE))
        {
            NPROT_RELEASE_LOCK(&pOpenContext->Lock);
            NtStatus = STATUS_INVALID_HANDLE;
            break;
        }

        // �����������봦���������Ѵ����������ü������Ӿ�1.
        // ��δ�����������Ŀ����1.
        NPROT_INSERT_TAIL_LIST(&pOpenContext->PendedReads, &pIrp->Tail.Overlay.ListEntry);
        NPROT_REF_OPEN(pOpenContext);  // pended read IRP
        pOpenContext->PendedReadCount++;

        // ���IRPδ������IRP����һ��ȡ��������ʹ֮��ÿ�ȡ����
        pIrp->Tail.Overlay.DriverContext[0] = (PVOID)pOpenContext;
        IoMarkIrpPending(pIrp);
        IoSetCancelRoutine(pIrp, NdisProtCancelRead);

        NPROT_RELEASE_LOCK(&pOpenContext->Lock);

        NtStatus = STATUS_PENDING;

        // ����һ��������������������δ���Ķ�����
        ndisprotServiceReads(pOpenContext);

    }
    while (FALSE);

    // ������ֻ����STATUS_PENDING.������ǣ���˵������
    // �����󷵻ء�
    if (NtStatus != STATUS_PENDING)
    {
        NPROT_ASSERT(NtStatus != STATUS_SUCCESS);
        pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = NtStatus;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

    return (NtStatus);
}


VOID
NdisProtCancelRead(
    IN PDEVICE_OBJECT               pDeviceObject,
    IN PIRP                         pIrp
    )
/*++

Routine Description:

    Cancel a pending read IRP. We unlink the IRP from the open context
    queue and complete it.

Arguments:

    pDeviceObject - pointer to our device object
    pIrp - IRP to be cancelled

Return Value:

    None

--*/
{
    PNDISPROT_OPEN_CONTEXT       pOpenContext;
    PLIST_ENTRY                 pIrpEntry;
    BOOLEAN                     Found;

    UNREFERENCED_PARAMETER(pDeviceObject);
    
    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    Found = FALSE;

    pOpenContext = (PNDISPROT_OPEN_CONTEXT) pIrp->Tail.Overlay.DriverContext[0];
    NPROT_STRUCT_ASSERT(pOpenContext, oc);

    NPROT_ACQUIRE_LOCK(&pOpenContext->Lock);

    //
    //  Locate the IRP in the pended read queue and remove it if found.
    //
    for (pIrpEntry = pOpenContext->PendedReads.Flink;
         pIrpEntry != &pOpenContext->PendedReads;
         pIrpEntry = pIrpEntry->Flink)
    {
        if (pIrp == CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry))
        {
            NPROT_REMOVE_ENTRY_LIST(&pIrp->Tail.Overlay.ListEntry);
            pOpenContext->PendedReadCount--;
            Found = TRUE;
            break;
        }
    }

    NPROT_RELEASE_LOCK(&pOpenContext->Lock);

    if (Found)
    {
        DEBUGP(DL_INFO, ("CancelRead: Open %p, IRP %p\n", pOpenContext, pIrp));
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        pIrp->IoStatus.Information = 0;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);

        NPROT_DEREF_OPEN(pOpenContext); // Cancel removed pended Read
    }
}
        


VOID
ndisprotServiceReads(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext
    )
/*++

Routine Description:

    Utility routine to copy received data into user buffers and
    complete READ IRPs.

Arguments:

    pOpenContext - pointer to open context

Return Value:

    None

--*/
{
    PIRP                pIrp;
    PLIST_ENTRY         pIrpEntry;
    PNDIS_PACKET        pRcvPacket;
    PLIST_ENTRY         pRcvPacketEntry;
    PUCHAR              pSrc, pDst;
    ULONG               BytesRemaining; // at pDst
    PNDIS_BUFFER        pNdisBuffer;
    ULONG               BytesAvailable;
    BOOLEAN             FoundPendingIrp;

    DEBUGP(DL_VERY_LOUD, ("ServiceReads: open %p/%x\n",
            pOpenContext, pOpenContext->Flags));

    NPROT_REF_OPEN(pOpenContext);  // temp ref - service reads

    NPROT_ACQUIRE_LOCK(&pOpenContext->Lock);

    // ֻҪ��������кͽ��հ�����ͬʱ��Ϊ�գ��������...
    while (!NPROT_IS_LIST_EMPTY(&pOpenContext->PendedReads) &&
           !NPROT_IS_LIST_EMPTY(&pOpenContext->RecvPktQueue))
    {
        FoundPendingIrp = FALSE;
  
        // ��õ�һ��δ��������
        pIrpEntry = pOpenContext->PendedReads.Flink;
        while (pIrpEntry != &pOpenContext->PendedReads)
        {
            // ������ڵ�õ�IRP
            pIrp = CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry);

            // �����������Ƿ����ڱ�ȡ����
            if (IoSetCancelRoutine(pIrp, NULL))
            {
                // �����IRP���С�
                NPROT_REMOVE_ENTRY_LIST(pIrpEntry);
                FoundPendingIrp = TRUE;
                break;
            }
            else
            {
                // �������ȥ�������������IRP���ɡ�ʹ����һ����
                DEBUGP(DL_INFO, ("ServiceReads: open %p, skipping cancelled IRP %p\n",
                        pOpenContext, pIrp));
                pIrpEntry = pIrpEntry->Flink;
            }
        }
        // ���û��IRP,ֱ������������
        if (FoundPendingIrp == FALSE)
        {
            break;
        }

        // �õ���һ��������ɵģ��������С�
        pRcvPacketEntry = pOpenContext->RecvPktQueue.Flink;
        NPROT_REMOVE_ENTRY_LIST(pRcvPacketEntry);
        pOpenContext->RecvPktCount --;
        NPROT_RELEASE_LOCK(&pOpenContext->Lock);
        NPROT_DEREF_OPEN(pOpenContext);

        // �ӽڵ��ð���
        pRcvPacket = NPROT_LIST_ENTRY_TO_RCV_PKT(pRcvPacketEntry);

        //
        //  Copy as much data as possible from the receive packet to
        //  the IRP MDL.
        //
        // �õ�IRP����������ַ��Ȼ����������������ݡ�
        pDst = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);
        NPROT_ASSERT(pDst != NULL);  // since it was already mapped
        BytesRemaining = MmGetMdlByteCount(pIrp->MdlAddress);
        pNdisBuffer = NDIS_PACKET_FIRST_NDIS_BUFFER(pRcvPacket);

        // ��ע�⣬ÿ��PNDIS_BUFFER����һ��PMDL��ͬʱPNDIS_BUFFER
        // ������������NdisGetNextBuffer���Դ�һ���õ���������һ����
        // ��������ʵ�����Ǳ�����һ������������������ġ�
        while (BytesRemaining && (pNdisBuffer != NULL))
        {
#ifndef WIN9X
            NdisQueryBufferSafe(pNdisBuffer, &pSrc, &BytesAvailable, NormalPagePriority);

            if (pSrc == NULL) 
            {
                DEBUGP(DL_FATAL,
                    ("ServiceReads: Open %p, QueryBuffer failed for buffer %p\n",
                            pOpenContext, pNdisBuffer));
                break;
            }
#else
            NdisQueryBuffer(pNdisBuffer, &pSrc, &BytesAvailable);
#endif

            // ��������Լ����������ͼ���������
            if (BytesAvailable)
            {
                ULONG       BytesToCopy = MIN(BytesAvailable, BytesRemaining);
                NPROT_COPY_MEM(pDst, pSrc, BytesToCopy);
                BytesRemaining -= BytesToCopy;
                pDst += BytesToCopy;
            }

            NdisGetNextBuffer(pNdisBuffer, &pNdisBuffer);
        }

        // ����������֮�󣬽���IRP���ɡ�
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        pIrp->IoStatus.Information = MmGetMdlByteCount(pIrp->MdlAddress) - BytesRemaining;

        DEBUGP(DL_INFO, ("ServiceReads: Open %p, IRP %p completed with %d bytes\n",
            pOpenContext, pIrp, pIrp->IoStatus.Information));

        IoCompleteRequest(pIrp, IO_NO_INCREMENT);

        // �����������������Ǵӽ��հ��������ģ���ô���Ǵ�
        // �������������õġ���������õģ�����NdisReturnPackets
        // �黹�����������������ͷš�
        if (NdisGetPoolFromPacket(pRcvPacket) != pOpenContext->RecvPacketPool)
        {
            NdisReturnPackets(&pRcvPacket, 1);
        }
        else
        {
            // ����Ļ��Լ��ͷš�
            ndisprotFreeReceivePacket(pOpenContext, pRcvPacket);
        }

        NPROT_DEREF_OPEN(pOpenContext);    // took out pended Read

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock);
        pOpenContext->PendedReadCount--;

    }

    NPROT_RELEASE_LOCK(&pOpenContext->Lock);

    NPROT_DEREF_OPEN(pOpenContext);    // temp ref - service reads
}




NDIS_STATUS
NdisProtReceive(
    IN NDIS_HANDLE                              ProtocolBindingContext,
    IN NDIS_HANDLE                              MacReceiveContext,
    __in_bcount(HeaderBufferSize) IN PVOID      pHeaderBuffer,
    IN UINT                                     HeaderBufferSize,
    __in_bcount(LookaheadBufferSize) IN PVOID   pLookaheadBuffer,
    IN UINT                                     LookaheadBufferSize,
    IN UINT                                     PacketSize
    )
/*++

Routine Description:

    Our protocol receive handler called by NDIS, typically if we have
    a miniport below that doesn't indicate packets.

    We make a local packet/buffer copy of this data, queue it up, and
    kick off the read service routine.

Arguments:

    ProtocolBindingContext - pointer to open context
    MacReceiveContext - for use in NdisTransferData
    pHeaderBuffer - pointer to data header
    HeaderBufferSize - size of the above
    pLookaheadBuffer - pointer to buffer containing lookahead data
    LookaheadBufferSize - size of the above
    PacketSize - size of the entire packet, minus header size.

Return Value:

    NDIS_STATUS_NOT_ACCEPTED - if this packet is uninteresting
    NDIS_STATUS_SUCCESS - if we processed this successfully

--*/
{
    PNDISPROT_OPEN_CONTEXT   pOpenContext;
    NDIS_STATUS             Status;
    PNDIS_PACKET            pRcvPacket;
    PUCHAR                  pRcvData;
    UINT                    BytesTransferred;
    PNDIS_BUFFER            pOriginalNdisBuffer, pPartialNdisBuffer;

    // ��ð󶨾����
    pOpenContext = (PNDISPROT_OPEN_CONTEXT)ProtocolBindingContext;
    NPROT_STRUCT_ASSERT(pOpenContext, oc);
    pRcvPacket = NULL;
    pRcvData = NULL;
    Status = NDIS_STATUS_SUCCESS;

    do
    {
        // ���ͷ���Ȳ�����̫����ͷ�ĳ��ȣ��򲻽����������
        // ��Э��ֻ������̫������
        if (HeaderBufferSize != sizeof(NDISPROT_ETH_HEADER))
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        // ����ȽϱȽ���֡��ѵ�ͷ�����Ǹ���?
        if ((PacketSize + HeaderBufferSize) < PacketSize)
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }
       
        // ����һ�������������������ͻ������������Լ��ڴ�
        // �ռ䣬һ���Է���á�
        pRcvPacket = ndisprotAllocateReceivePacket(
                        pOpenContext,
                        PacketSize + HeaderBufferSize,
                        &pRcvData
                        );

        // �������ʧ���ˣ��Ͳ��ٽ���������ˡ�
        if ((pRcvPacket == NULL) || (pRcvData == NULL))
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        // �ڴ濽�����ȿ�����̫����ͷ��
        NdisMoveMappedMemory(pRcvData, pHeaderBuffer, HeaderBufferSize);

        // ���ǰ�������Ƿ�����������������ݡ�
        if (PacketSize == LookaheadBufferSize)
        {
            // ���ǰ�����Ѿ��������������ݰ�����ô����NdisCopyLookaheadData
            // �͵õ��������İ���Ȼ�����ndisprotQueueReceivePacket�������
            // ������м��ɡ�
            NdisCopyLookaheadData(pRcvData+HeaderBufferSize,
                                  pLookaheadBuffer,
                                  LookaheadBufferSize,
                                  pOpenContext->MacOptions);
            ndisprotQueueReceivePacket(pOpenContext, pRcvPacket);
        }
        else
        {
            // ����Ļ�����Ҫ����һ���µĻ�������������ע���������
            // ���Ŷ�Ӧ���ǴӰ���������ʼ֮��HeaderBufferSize���ֽ�֮
            // �󴦿�ʼ�Ŀռ䣨pRcvData + HeaderBufferSize����
            NdisAllocateBuffer(
                &Status,
                &pPartialNdisBuffer,
                pOpenContext->RecvBufferPool,
                pRcvData + HeaderBufferSize,
                PacketSize);
            
            if (Status == NDIS_STATUS_SUCCESS)
            {
                // ����ɹ��ˣ��ͰѰ���ԭ�еĻ��������ʹԭ���Ļ�������
                // ���������������
                NdisUnchainBufferAtFront(pRcvPacket, &pOriginalNdisBuffer);
                // ���ڰ�ԭ���İ������������ڰ��������У������Ա����ã�
                NPROT_RCV_PKT_TO_ORIGINAL_BUFFER(pRcvPacket) = pOriginalNdisBuffer;

                // Ȼ����µĻ������������ӵ����ϡ�
                NdisChainBufferAtBack(pRcvPacket, pPartialNdisBuffer);

                DEBUGP(DL_LOUD, ("Receive: setting up for TransferData:"
                        " Pkt %p, OriginalBuf %p, PartialBuf %p\n",
                        pRcvPacket, pOriginalNdisBuffer, pPartialNdisBuffer));

                // Ȼ�����NdisTransferData���������ݰ�ʣ��Ĳ��֡����
                // �������֮��Э�������е�NdisProtTransferDataComplete
                // �ᱻ���á�
                NdisTransferData(
                    &Status,
                    pOpenContext->BindingHandle,
                    MacReceiveContext,
                    0,  // ByteOffset
                    PacketSize,
                    pRcvPacket,
                    &BytesTransferred);
            }
            else
            {
                // ���ʧ���ˣ��Ͳ������NdisTransferData���������ǻ���
                // Ҫ��NdisProtTransferDataComplete�������Ĵ�������
                // �Լ���дBytesTransferred��
                BytesTransferred = 0;
            }
    
            if (Status != NDIS_STATUS_PENDING)
            {
                // ���ǰ���ʧ���ˣ������Լ�����NdisProtTransferDataComplete��
                NdisProtTransferDataComplete(
                    (NDIS_HANDLE)pOpenContext,
                    pRcvPacket,
                    Status,
                    BytesTransferred);
            }
        }
    }
    while (FALSE);

    DEBUGP(DL_LOUD, ("Receive: Open %p, Pkt %p, Size %d\n",
            pOpenContext, pRcvPacket, PacketSize));

    return Status;
}



VOID
NdisProtTransferDataComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNDIS_PACKET                 pNdisPacket,
    IN NDIS_STATUS                  TransferStatus,
    IN UINT                         BytesTransferred
    )
/*++

Routine Description:

    NDIS entry point called to signal completion of a call to
    NdisTransferData that had pended.

Arguments:

    ProtocolBindingContext - pointer to open context
    pNdisPacket - our receive packet into which data is transferred
    TransferStatus - status of the transfer
    BytesTransferred - bytes copied into the packet.

Return Value:

    None

--*/
{
    PNDISPROT_OPEN_CONTEXT   pOpenContext;
    PNDIS_BUFFER            pOriginalBuffer, pPartialBuffer;

    UNREFERENCED_PARAMETER(BytesTransferred);
    
    pOpenContext = (PNDISPROT_OPEN_CONTEXT)ProtocolBindingContext;
    NPROT_STRUCT_ASSERT(pOpenContext, oc);

    // �õ�������ľɵĻ�����������Ҫ�ǵ��ڴ���֮ǰ��Ϊ���ô���
    // ��������ȷ��д����̫����ͷ�����Ƿ�����һ���µĻ�������
    // ���滻�˾ɵĻ���������������Ҫ�ָ����ˡ�
    pOriginalBuffer = NPROT_RCV_PKT_TO_ORIGINAL_BUFFER(pNdisPacket);
    if (pOriginalBuffer != NULL)
    {

        // ��ǰ����滻ʱ�Ĳ���һ������Unchain��Ȼ���ٵ���Chain��
        // ����֮���Ѿ��ָ���ʹ�þɵİ���������
        NdisUnchainBufferAtFront(pNdisPacket, &pPartialBuffer);
        NdisChainBufferAtBack(pNdisPacket, pOriginalBuffer);

        DEBUGP(DL_LOUD, ("TransferComp: Pkt %p, OrigBuf %p, PartialBuf %p\n",
                pNdisPacket, pOriginalBuffer, pPartialBuffer));

        ASSERT(pPartialBuffer != NULL);
        
        // ��ô�Ǹ��µİ��������Ѿ�û���ˣ�����NdisFreeBuffer�ͷ�����
        if (pPartialBuffer != NULL)
        {
            NdisFreeBuffer(pPartialBuffer);
        }
    }

    if (TransferStatus == NDIS_STATUS_SUCCESS)
    {
        // ��������ǳɹ��ģ��������浽���ն����С�
        ndisprotQueueReceivePacket(pOpenContext, pNdisPacket);
    }
    else
    {
        // �������ʧ���ˣ�ֱ���ͷ��������
        ndisprotFreeReceivePacket(pOpenContext, pNdisPacket);
    }
}


VOID
NdisProtReceiveComplete(
    IN NDIS_HANDLE                  ProtocolBindingContext
    )
/*++

Routine Description:

    Protocol entry point called by NDIS when the miniport
    has finished indicating up a batch of receives.

    We ignore this.

Arguments:

    ProtocolBindingContext - pointer to open context

Return Value:

    None

--*/
{
    PNDISPROT_OPEN_CONTEXT   pOpenContext;

    pOpenContext = (PNDISPROT_OPEN_CONTEXT)ProtocolBindingContext;
    NPROT_STRUCT_ASSERT(pOpenContext, oc);

    return;
}


INT
NdisProtReceivePacket(
    IN NDIS_HANDLE                  ProtocolBindingContext,
    IN PNDIS_PACKET                 pNdisPacket
    )
/*++

Routine Description:

    Protocol entry point called by NDIS if the driver below
    uses NDIS 4 style receive packet indications.

    If the miniport allows us to hold on to this packet, we
    use it as is, otherwise we make a copy.

Arguments:

    ProtocolBindingContext - pointer to open context
    pNdisPacket - the packet being indicated up.

Return Value:

    None

--*/
{
    PNDISPROT_OPEN_CONTEXT   pOpenContext;
    PNDIS_BUFFER            pNdisBuffer;
    UINT                    BufferLength;
    PNDISPROT_ETH_HEADER     pEthHeader;
    PNDIS_PACKET            pCopyPacket;
    PUCHAR                  pCopyBuf;
    UINT                    TotalPacketLength;
    UINT                    BytesCopied;
    INT                     RefCount = 0;
    NDIS_STATUS             Status;

    pOpenContext = (PNDISPROT_OPEN_CONTEXT)ProtocolBindingContext;
    NPROT_STRUCT_ASSERT(pOpenContext, oc);

#ifdef NDIS51
    NdisGetFirstBufferFromPacketSafe(
        pNdisPacket,
        &pNdisBuffer,
        &pEthHeader,
        &BufferLength,
        &TotalPacketLength,
        NormalPagePriority);

    if (pEthHeader == NULL)
    {
        //
        //  The system is low on resources. Set up to handle failure
        //  below.
        //
        BufferLength = 0;
    }
#else

    // �Ӱ��������еõ���һ��������������
    NdisGetFirstBufferFromPacket(
        pNdisPacket,
        &pNdisBuffer,
        &pEthHeader,
        &BufferLength,
        &TotalPacketLength);
#endif

    do
    {
        // ���������ĳ��ȱ���̫����ͷ��ҪС������֮��
        if (BufferLength < sizeof(NDISPROT_ETH_HEADER))
        {
            DEBUGP(DL_WARN,
                ("ReceivePacket: Open %p, runt pkt %p, first buffer length %d\n",
                    pOpenContext, pNdisPacket, BufferLength));

            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        DEBUGP(DL_LOUD, ("ReceivePacket: Open %p, interesting pkt %p\n",
                    pOpenContext, pNdisPacket));

        // ����������NDIS_STATUS_RESOURCES״̬������뿽��
        // ���������øð�����Ȼ�����ͱȽ�����ʱ�����Դ�ˡ�
        if ((NDIS_GET_PACKET_STATUS(pNdisPacket) == NDIS_STATUS_RESOURCES) ||
            pOpenContext->bRunningOnWin9x)
        {
            // �����Ƿ���һ���������������ݡ����߿��Բο�ǰ�潲��
            // ����������⡣
            pCopyPacket = ndisprotAllocateReceivePacket(
                            pOpenContext,
                            TotalPacketLength,
                            &pCopyBuf
                            );
            
            if (pCopyPacket == NULL)
            {
                DEBUGP(DL_FATAL, ("ReceivePacket: Open %p, failed to"
                    " alloc copy, %d bytes\n", pOpenContext, TotalPacketLength));
                break;
            }

            // ����NdisCopyFromPacketToPacket������������Ȼ�ڿ���֮
            // ǰ�����߱���ȷ��Ŀ����Ļ������������㹻�ġ�
            NdisCopyFromPacketToPacket(
                pCopyPacket,
                0,
                TotalPacketLength,
                pNdisPacket,
                0,
                &BytesCopied);
            
            NPROT_ASSERT(BytesCopied == TotalPacketLength);
            // ��ô���ڿ�ʼ�����µİ��ˡ�
            pNdisPacket = pCopyPacket;
        }
        else
        {
            // ����ֵ������ֵ��ʾ���������Ѿ�һ���������������
            // ����������ʱ�����ǾͿ��Ե���NdisReturnPackets
            // ��Ҫ���²������ͷ�������ˡ���������RefCount����
            // ����ֵ�����������0����ô�²���������Ϊ���ǲ���
            // ��Ҫ������ݰ���
            RefCount = 1;
        }
        // �����ݰ���������
        ndisprotQueueReceivePacket(pOpenContext, pNdisPacket);
    
    }
    while (FALSE);
    return (RefCount);
}


VOID
ndisprotQueueReceivePacket(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext,
    IN PNDIS_PACKET                 pRcvPacket
    )
/*++

Routine Description:

    Queue up a received packet on the open context structure.
    If the queue size goes beyond a water mark, discard a packet
    at the head of the queue.

    Finally, run the queue service routine.

Arguments:
    
    pOpenContext - pointer to open context
    pRcvPacket - the received packet

Return Value:

    None

--*/
{
    PLIST_ENTRY     pEnt;
    PLIST_ENTRY     pDiscardEnt;
    PNDIS_PACKET    pDiscardPkt;

    do
    {
        pEnt = NPROT_RCV_PKT_TO_LIST_ENTRY(pRcvPacket);
        NPROT_REF_OPEN(pOpenContext);    // queued rcv packet
        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock);

        // ������ڻ��״̬��������ȷ�ĵ�Դ״̬����ô�Ͱ������
        // ������ջ��������С�
        if (NPROT_TEST_FLAGS(pOpenContext->Flags, NUIOO_BIND_FLAGS, NUIOO_BIND_ACTIVE) &&
            (pOpenContext->PowerState == NetDeviceStateD0))
        {
            NPROT_INSERT_TAIL_LIST(&pOpenContext->RecvPktQueue, pEnt);
            pOpenContext->RecvPktCount++;
            DEBUGP(DL_VERY_LOUD, ("QueueReceivePacket: open %p,"
                    " queued pkt %p, queue size %d\n",
                    pOpenContext, pRcvPacket, pOpenContext->RecvPktCount));
        }
        else
        {

            // ����Ļ�����ֱ���ͷŵ���������ɡ�
            NPROT_RELEASE_LOCK(&pOpenContext->Lock);
            ndisprotFreeReceivePacket(pOpenContext, pRcvPacket);
            NPROT_DEREF_OPEN(pOpenContext);  // dropped rcv packet - bad state
            break;
        }

        // ������뻺�������̫���ˣ���Ҫɾ��һ����
        if (pOpenContext->RecvPktCount > MAX_RECV_QUEUE_SIZE)
        {
            // Ҫɾ���İ������ڵ�ָ��
            pDiscardEnt = pOpenContext->RecvPktQueue.Flink;
            NPROT_REMOVE_ENTRY_LIST(pDiscardEnt);
            // ���հ�������ȥ1
            pOpenContext->RecvPktCount --;
            // �����ͷ����ˡ�
            NPROT_RELEASE_LOCK(&pOpenContext->Lock);
            // �����ڵ�ת��Ϊ����ָ��
            pDiscardPkt = NPROT_LIST_ENTRY_TO_RCV_PKT(pDiscardEnt);
            // �Ѱ��ͷŵ���
            ndisprotFreeReceivePacket(pOpenContext, pDiscardPkt);
            // �������Ľ����á�������Ϊÿ���һ����Ҫ����һ�����ü�����
            NPROT_DEREF_OPEN(pOpenContext);  // dropped rcv packet - queue too long
            DEBUGP(DL_INFO, ("QueueReceivePacket: open %p queue"
                    " too long, discarded pkt %p\n",
                    pOpenContext, pDiscardPkt));
        }
        else
        {
            NPROT_RELEASE_LOCK(&pOpenContext->Lock);
        }

        // ������������������Ƿ���δ���Ķ���������У���ȡ��
        // ������������
        ndisprotServiceReads(pOpenContext);
    }
    while (FALSE);
}


PNDIS_PACKET
ndisprotAllocateReceivePacket(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext,
    IN UINT                         DataLength,
    OUT PUCHAR *                    ppDataBuffer
    )
/*++

Routine Description:

    Allocate resources to copy and queue a received packet.

Arguments:

    pOpenContext - pointer to open context for received packet
    DataLength - total length in bytes of the packet
    ppDataBuffer - place to return pointer to allocated buffer

Return Value:

    Pointer to NDIS packet if successful, else NULL.

--*/
{
    PNDIS_PACKET            pNdisPacket;
    PNDIS_BUFFER            pNdisBuffer;
    PUCHAR                  pDataBuffer;
    NDIS_STATUS             Status;

    pNdisPacket = NULL;
    pNdisBuffer = NULL;
    pDataBuffer = NULL;

    do
    {
        NPROT_ALLOC_MEM(pDataBuffer, DataLength);

        if (pDataBuffer == NULL)
        {
            DEBUGP(DL_FATAL, ("AllocRcvPkt: open %p, failed to alloc"
                " data buffer %d bytes\n", pOpenContext, DataLength));
            break;
        }

        //
        //  Make this an NDIS buffer.
        //
        NdisAllocateBuffer(
            &Status,
            &pNdisBuffer,
            pOpenContext->RecvBufferPool,
            pDataBuffer,
            DataLength);
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_FATAL, ("AllocateRcvPkt: open %p, failed to alloc"
                " NDIS buffer, %d bytes\n", pOpenContext, DataLength));
            break;
        }

        NdisAllocatePacket(&Status, &pNdisPacket, pOpenContext->RecvPacketPool);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(DL_FATAL, ("AllocateRcvPkt: open %p, failed to alloc"
                " NDIS packet, %d bytes\n", pOpenContext, DataLength));
            break;
        }

        NDIS_SET_PACKET_STATUS(pNdisPacket, 0);
        NPROT_RCV_PKT_TO_ORIGINAL_BUFFER(pNdisPacket) = NULL;

        NdisChainBufferAtFront(pNdisPacket, pNdisBuffer);

        *ppDataBuffer = pDataBuffer;

      
    }
    while (FALSE);

    if (pNdisPacket == NULL)
    {
        //
        //  Clean up
        //
        if (pNdisBuffer != NULL)
        {
            NdisFreeBuffer(pNdisBuffer);
        }

        if (pDataBuffer != NULL)
        {
            NPROT_FREE_MEM(pDataBuffer);
        }
    }

    return (pNdisPacket);
}



VOID
ndisprotFreeReceivePacket(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext,
    IN PNDIS_PACKET                 pNdisPacket
    )
/*++

Routine Description:

    Free up all resources associated with a received packet. If this
    is a local copy, free the packet to our receive pool, else return
    this to the miniport.

Arguments:
    
    pOpenContext - pointer to open context
    pNdisPacket - pointer to packet to be freed.

Return Value:

    None

--*/
{
    PNDIS_BUFFER        pNdisBuffer;
    UINT                TotalLength;
    UINT                BufferLength;
    PUCHAR              pCopyData;

    if (NdisGetPoolFromPacket(pNdisPacket) == pOpenContext->RecvPacketPool)
    {
        //
        //  This is a local copy.
        //
#ifdef NDIS51
        NdisGetFirstBufferFromPacketSafe(
            pNdisPacket,
            &pNdisBuffer,
            (PVOID *)&pCopyData,
            &BufferLength,
            &TotalLength,
            NormalPagePriority);
#else
        NdisGetFirstBufferFromPacket(
            pNdisPacket,
            &pNdisBuffer,
            (PVOID *)&pCopyData,
            &BufferLength,
            &TotalLength);
#endif

        NPROT_ASSERT(BufferLength == TotalLength);

        NPROT_ASSERT(pNdisBuffer != NULL);

        NPROT_ASSERT(pCopyData != NULL); // we would have allocated non-paged pool

        NdisFreePacket(pNdisPacket);

        NdisFreeBuffer(pNdisBuffer);

        NPROT_FREE_MEM(pCopyData);
    }
    else
    {
        NdisReturnPackets(&pNdisPacket, 1);
    }
}
        

VOID
ndisprotCancelPendingReads(
    IN PNDISPROT_OPEN_CONTEXT        pOpenContext
    )
/*++

Routine Description:

    Cancel any pending read IRPs queued on the given open.

Arguments:

    pOpenContext - pointer to open context

Return Value:

    None

--*/
{
    PIRP                pIrp;
    PLIST_ENTRY         pIrpEntry;

    NPROT_REF_OPEN(pOpenContext);  // temp ref - cancel reads

    NPROT_ACQUIRE_LOCK(&pOpenContext->Lock);

    while (!NPROT_IS_LIST_EMPTY(&pOpenContext->PendedReads))
    {
        //
        //  Get the first pended Read IRP
        //
        pIrpEntry = pOpenContext->PendedReads.Flink;
        pIrp = CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry);

        //
        //  Check to see if it is being cancelled.
        //
        if (IoSetCancelRoutine(pIrp, NULL))
        {
            //
            //  It isn't being cancelled, and can't be cancelled henceforth.
            //
            NPROT_REMOVE_ENTRY_LIST(pIrpEntry);

            NPROT_RELEASE_LOCK(&pOpenContext->Lock);

            //
            //  Complete the IRP.
            //
            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            DEBUGP(DL_INFO, ("CancelPendingReads: Open %p, IRP %p cancelled\n",
                pOpenContext, pIrp));

            IoCompleteRequest(pIrp, IO_NO_INCREMENT);

            NPROT_DEREF_OPEN(pOpenContext);    // took out pended Read for cancelling

            NPROT_ACQUIRE_LOCK(&pOpenContext->Lock);
            pOpenContext->PendedReadCount--;
        }
        else
        {
            //
            //  It is being cancelled, let the cancel routine handle it.
            //
            NPROT_RELEASE_LOCK(&pOpenContext->Lock);

            //
            //  Give the cancel routine some breathing space, otherwise
            //  we might end up examining the same (cancelled) IRP over
            //  and over again.
            //
            NPROT_SLEEP(1);

            NPROT_ACQUIRE_LOCK(&pOpenContext->Lock);
        }
    }

    NPROT_RELEASE_LOCK(&pOpenContext->Lock);

    NPROT_DEREF_OPEN(pOpenContext);    // temp ref - cancel reads
}


VOID
ndisprotFlushReceiveQueue(
    IN PNDISPROT_OPEN_CONTEXT            pOpenContext
    )
/*++

Routine Description:

    Free any receive packets queued up on the specified open

Arguments:

    pOpenContext - pointer to open context

Return Value:

    None

--*/
{
    PLIST_ENTRY         pRcvPacketEntry;
    PNDIS_PACKET        pRcvPacket;

    NPROT_REF_OPEN(pOpenContext);  // temp ref - flushRcvQueue

    NPROT_ACQUIRE_LOCK(&pOpenContext->Lock);
    
    while (!NPROT_IS_LIST_EMPTY(&pOpenContext->RecvPktQueue))
    {
        //
        //  Get the first queued receive packet
        //
        pRcvPacketEntry = pOpenContext->RecvPktQueue.Flink;
        NPROT_REMOVE_ENTRY_LIST(pRcvPacketEntry);

        pOpenContext->RecvPktCount --;

        NPROT_RELEASE_LOCK(&pOpenContext->Lock);

        pRcvPacket = NPROT_LIST_ENTRY_TO_RCV_PKT(pRcvPacketEntry);

        DEBUGP(DL_LOUD, ("FlushReceiveQueue: open %p, pkt %p\n",
            pOpenContext, pRcvPacket));

        ndisprotFreeReceivePacket(pOpenContext, pRcvPacket);

        NPROT_DEREF_OPEN(pOpenContext);    // took out pended Read

        NPROT_ACQUIRE_LOCK(&pOpenContext->Lock);
    }

    NPROT_RELEASE_LOCK(&pOpenContext->Lock);

    NPROT_DEREF_OPEN(pOpenContext);    // temp ref - flushRcvQueue
}


