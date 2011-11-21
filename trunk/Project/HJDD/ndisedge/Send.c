/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

        Send.C

Abstract:

    This module contains miniport functions for handling Send
    packets and other helper routines called by the miniport function.

--*/

#include "ndiswdm.h"


VOID
MPSendPackets(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets)
/*++

Routine Description:

    Send Packet Array handler. Called by NDIS whenever a protocol
    bound to our miniport sends one or more packets.

    The input packet descriptor pointers have been ordered according
    to the order in which the packets should be sent over the network
    by the protocol driver that set up the packet array. The NDIS
    library preserves the protocol-determined ordering when it submits
    each packet array to MiniportSendPackets

    As a deserialized driver, we are responsible for holding incoming send
    packets in our internal queue until they can be transmitted over the
    network and for preserving the protocol-determined ordering of packet
    descriptors incoming to its MiniportSendPackets function.
    A deserialized miniport driver must complete each incoming send packet
    with NdisMSendComplete, and it cannot call NdisMSendResourcesAvailable.

    Runs at IRQL <= DISPATCH_LEVEL

Arguments:

    MiniportAdapterContext    Pointer to our adapter context
    PacketArray               Set of packets to send
    NumberOfPackets           Length of above array

Return Value:

    None

--*/
{
    PMP_ADAPTER       Adapter;
    UINT              PacketCount;
    PNDIS_PACKET      Packet = NULL;
    NDIS_STATUS       Status = NDIS_STATUS_FAILURE;
    PTCB              pTCB = NULL;
    NTSTATUS          ntStatus;

    DEBUGP(MP_TRACE, ("---> MPSendPackets\n"));

    Adapter = (PMP_ADAPTER)MiniportAdapterContext;

    for(PacketCount=0; PacketCount < NumberOfPackets; PacketCount++)
    {
        //
        // Check for a zero pointer
        //

        Packet = PacketArray[PacketCount];

        if (!Packet){
            ASSERTMSG("Packet pointer is NULL\n", FALSE);
            continue;
        }

        DEBUGP(MP_LOUD, ("Adapter %p, Packet %p\n", Adapter, Packet));

        Adapter->nPacketsArrived++;

        if (MP_IS_READY(Adapter) && MP_TEST_FLAG(Adapter, fMP_POST_WRITES))
        {
            //
            // Get a free TCB block
            //
            pTCB = (PTCB) NdisInterlockedRemoveHeadList(
                         &Adapter->SendFreeList,
                         &Adapter->SendLock);
            if (pTCB == NULL)
            {
                DEBUGP(MP_LOUD, ("Can't allocate a TCB......!\n"));

                Status = NDIS_STATUS_PENDING;

                //
                // Not able to get TCB block for this send. So queue
                // it for later transmission and return NDIS_STATUS_PENDING .
                //
                NdisInterlockedInsertTailList(
                    &Adapter->SendWaitList,
                    (PLIST_ENTRY)&Packet->MiniportReserved[0],
                    &Adapter->SendLock);
            }
            else
            {
                NdisInterlockedIncrement(&Adapter->nBusySend);
                ASSERT(Adapter->nBusySend <= NIC_MAX_BUSY_SENDS);

                //
                // Copy the content of the packet to a TCB block
                // buffer. We need to do this because the target driver
                // is not capable of handling chained buffers.
                //
                if (NICCopyPacket(Adapter, pTCB, Packet))
                {
                    Adapter->nWritesPosted++;
                    //
                    // Post a asynchronouse write request.
                    //
                    ntStatus = NICPostWriteRequest(Adapter, pTCB);

                    if ( !NT_SUCCESS ( ntStatus ) ) {
                        DEBUGP (MP_ERROR, ( "NICPostWriteRequest failed %x\n", ntStatus ));
                    }
                    NT_STATUS_TO_NDIS_STATUS(ntStatus, &Status);

                }else {
                    DEBUGP(MP_ERROR, ("NICCopyPacket returned error\n"));
                    Status = NDIS_STATUS_FAILURE;
                }
            }
        }

        if (!NT_SUCCESS(Status)) // yes, NT_SUCCESS macro can be used on NDIS_STATUS
        {

            DEBUGP(MP_LOUD, ("Calling NdisMSendComplete %x \n", Status));

            NDIS_SET_PACKET_STATUS(Packet, Status);

            NdisMSendComplete(
                Adapter->AdapterHandle,
                Packet,
                Status);

            if (pTCB && (NdisInterlockedDecrement(&pTCB->Ref) == 0))
            {
                NICFreeSendTCB(Adapter, pTCB);
            }
        }


    }

    DEBUGP(MP_TRACE, ("<--- MPSendPackets\n"));

    return;
}



NTSTATUS
NICPostWriteRequest(
    PMP_ADAPTER Adapter,
    PTCB    pTCB
    )
/*++

Routine Description:

    This routine posts a write Request to the target device to send
    the network packet out to the device.

Arguments:

    Adapter    - pointer to the MP_ADAPTER structure
    PTCB    -  Pointer to the TCB block that contains the Request and data.


Return Value:

    NT status code

--*/
{
    PIRP            irp = NULL;
    PIO_STACK_LOCATION  nextStack;
    NTSTATUS    status = STATUS_SUCCESS;

    DEBUGP(MP_TRACE, ("--> NICPostWriteRequest\n"));

    // ׼��һ��д����ʵ�����������Ĳ�������OutputBuffer
    // ����InputBuffer������ԭʼ�Ĵ���ע�;�д����OuputBuffer��
    status = WdfIoTargetFormatRequestForWrite(
                    Adapter->IoTarget,
                    pTCB->Request,
                    NULL, //OutputBuffer
                    NULL, // OutputBufferOffsets
                    NULL); // StartingOffset

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // ��WdfRequest���IRP.
    irp = WdfRequestWdmGetIrp(pTCB->Request);

    // �����һ��ջ�ռ䡣Ȼ����д���뻺�塣
    nextStack = IoGetNextIrpStackLocation( irp );
    irp->MdlAddress = pTCB->Mdl;
    nextStack->Parameters.Write.Length = pTCB->ulSize;

    // ����һ����ɻص�������
    WdfRequestSetCompletionRoutine(pTCB->Request,
                   NICWriteRequestCompletion,
                   pTCB);

    // �����������ǲ����ķ��������״̬����Ϊ����
    // �������֮�����Ǵ���ɺ����л���������״̬��
    if (WdfRequestSend(pTCB->Request, Adapter->IoTarget, WDF_NO_SEND_OPTIONS) == FALSE) {
        status = WdfRequestGetStatus(pTCB->Request);
    }

    DEBUGP(MP_TRACE, ("<-- NICPostWriteRequest\n"));

    return status;
}

VOID
NICWriteRequestCompletion(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
    )
/*++

Routine Description:

    Completion routine for the write request. This routine
    indicates to NDIS that send operation is complete and
    free the TCB if nobody has already done so. This routine
    also handles the case where another thread has canceled the
    write request.

Arguments:

    DeviceObject    -  not used. Should be NULL
    Request    -   Pointer to our Request
    Context - pointer to our adapter context structure

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - because this is an asynchronouse Request
    and we want to reuse it.

--*/
{
    PTCB              pTCB = (PTCB)Context;
    PMP_ADAPTER       Adapter = pTCB->Adapter;
    PLIST_ENTRY       pEntry = NULL;
    PNDIS_PACKET      Packet = NULL;
    NTSTATUS          status;
    ULONG             bytesWritten;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Request);

    DEBUGP(MP_TRACE, ("--> NICWriteRequestCompletion\n"));

    status = CompletionParams->IoStatus.Status;

    if (NT_SUCCESS(status)) {
        bytesWritten = (ULONG)CompletionParams->IoStatus.Information;
        DEBUGP (MP_VERY_LOUD, ("Wrote %d bytes\n", bytesWritten));
        Adapter->GoodTransmits++;
        Adapter->nBytesWritten += bytesWritten;

    } else {

        DEBUGP (MP_ERROR, ("Write request failed %x\n", status));
        Adapter->nWritesCompletedWithError++;
        //
        // Clear the flag to prevent any more writes from being
        // posted to the target device.
        //
        //MP_CLEAR_FLAG(Adapter, fMP_POST_WRITES);

    }

    if (NdisInterlockedDecrement(&pTCB->Ref) == 0)
    {
        DEBUGP(MP_VERY_LOUD, ("Calling NdisMSendComplete \n"));

        Packet = pTCB->OrgSendPacket;

        //
        // For performance, let us free the TCB block before completing send.
        //
        NICFreeSendTCB(Adapter, pTCB);

        NdisMSendComplete(
            Adapter->AdapterHandle,
            Packet,
            NDIS_STATUS_SUCCESS);

        //
        // Before we exit, since we have the control, let use see if there any
        // more packets waiting in the queue to be sent.
        //

        pEntry = (PLIST_ENTRY) NdisInterlockedRemoveHeadList(
                            &Adapter->SendWaitList,
                            &Adapter->SendLock);
        if (pEntry)
        {
            Packet = CONTAINING_RECORD(pEntry, NDIS_PACKET, MiniportReserved);
            //
            // TODO: This can cause recursion if the target driver completes
            // write requests synchronously. Use workitem to avoid recursion.
            //
            MPSendPackets(Adapter, &Packet, 1);
        }

    }


    return;
}


VOID
NICFreeSendTCB(
    IN PMP_ADAPTER Adapter,
    IN PTCB pTCB)
/*++

Routine Description:

    Adapter    - pointer to the adapter structure
    pTCB      - pointer to TCB block

Arguments:

    This routine reinitializes the TCB block and puts it back
    into the SendFreeList for reuse.


Return Value:

    VOID

--*/
{
    WDF_REQUEST_REUSE_PARAMS    params;
    PIRP                        irp = NULL;
    PIO_STACK_LOCATION          nextStack;
    NTSTATUS status;

    DEBUGP(MP_TRACE, ("--> NICFreeSendTCB %p\n", pTCB));

    pTCB->OrgSendPacket = NULL;
    pTCB->Buffer->Next = NULL;

    ASSERT(pTCB->Request);
    ASSERT(!pTCB->Ref);

    WDF_REQUEST_REUSE_PARAMS_INIT(&params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);

    status = WdfRequestReuse(pTCB->Request, &params);
    if (!NT_SUCCESS(status)){
        DEBUGP(MP_ERROR, ("WdfRequestReuse failed 0x%x\n", status));
        ASSERT(FALSE);
        return;
    }


    //
    // Set the MDL field to NULL so that we don't end up free the
    // MDL in our call to Free Request.
    //
    irp = WdfRequestWdmGetIrp(pTCB->Request);
    nextStack = IoGetNextIrpStackLocation( irp );
    irp->MdlAddress = NULL;

    //
    // Re adjust the length to the originl size
    //
    NdisAdjustBufferLength(pTCB->Buffer, NIC_BUFFER_SIZE);

    //
    // Insert the TCB back in the send free list
    //
    NdisAcquireSpinLock(&Adapter->SendLock);

    RemoveEntryList(&pTCB->List);

    InsertTailList(&Adapter->SendFreeList, &pTCB->List);

    NdisInterlockedDecrement(&Adapter->nBusySend);
    ASSERT(Adapter->nBusySend >= 0);

    NdisReleaseSpinLock(&Adapter->SendLock);


    DEBUGP(MP_TRACE, ("<-- NICFreeSendTCB\n"));

}



VOID
NICFreeQueuedSendPackets(
    PMP_ADAPTER Adapter
    )
/*++

Routine Description:

    This routine is called by the Halt handler to fail all
    the queued up SendPackets because the device is either
    gone, being stopped for resource rebalance.

Arguments:

    Adapter    - pointer to the adapter structure

Return Value:

    VOID

--*/
{
    PLIST_ENTRY       pEntry = NULL;
    PNDIS_PACKET      Packet = NULL;

    DEBUGP(MP_TRACE, ("--> NICFreeQueuedSendPackets\n"));

    while (MP_TEST_FLAG(Adapter, fMP_SEND_SIDE_RESOURCE_ALLOCATED))
    {
        pEntry = (PLIST_ENTRY) NdisInterlockedRemoveHeadList(
                        &Adapter->SendWaitList,
                        &Adapter->SendLock);
        if (!pEntry)
        {
            break;
        }

        Packet = CONTAINING_RECORD(pEntry, NDIS_PACKET, MiniportReserved);
        NdisMSendComplete(
            Adapter->AdapterHandle,
            Packet,
            NDIS_STATUS_FAILURE);
    }

    DEBUGP(MP_TRACE, ("<-- NICFreeQueuedSendPackets\n"));

}

BOOLEAN
NICCopyPacket(
    PMP_ADAPTER Adapter,
    PTCB pTCB,
    PNDIS_PACKET Packet)
/*++

Routine Description:

    This routine copies the packet data into the TCB data block and
    inserts the TCB in the SendBusyList.

Arguments:

    Adapter    - pointer to the MP_ADAPTER structure
    pTCB      - pointer to TCB block
    Packet    - packet to be transfered.


Return Value:

    TRUE if the data is copied to TCB buffer

--*/
{
    PNDIS_BUFFER   CurrentBuffer = NULL;
    PVOID          VirtualAddress = NULL;
    UINT           CurrentLength;
    UINT           BytesToCopy;
    UINT           BytesCopied = 0;
    UINT           PacketLength;
    PUCHAR         pDest = pTCB->pData;
    BOOLEAN        bResult = TRUE, bPadding = FALSE;
    PETH_HEADER    pEthHeader;
    UINT           DestBufferSize = NIC_BUFFER_SIZE;

    DEBUGP(MP_TRACE, ("--> NICCopyPacket\n"));

    //
    // Initialize TCB structure
    //
    // ���λ�ñ���Ҫ���͵�ԭʼ�İ���������
    pTCB->OrgSendPacket = Packet;
    // ���ü�����Ϊ1.
    pTCB->Ref = 1;
    // ��ʼ������ڵ㡣ʹ֮���һ������ͷ��
    NdisInitializeListHead(&pTCB->List);

    //
    // Query the packet to get length and pointer to the first buffer.
    //
    // ���Ҫ���͵����ݰ��ĳ��ȡ�
    NdisQueryPacket(Packet,
                    NULL,
                    NULL,
                    &CurrentBuffer,
                    &PacketLength);

    // ���ȱ����TCB��׼������󳤶�ҪС��
    ASSERT(PacketLength <= NIC_BUFFER_SIZE);

    // ���ֻ�ڲ�������������������������á����ǰ�̫����
    // ʱ��ض̣�ʹ֮��������޶ȵ�Ҫ��
    PacketLength = min(PacketLength, NIC_BUFFER_SIZE);
    if (PacketLength < ETH_MIN_PACKET_SIZE)
    {
        PacketLength = ETH_MIN_PACKET_SIZE;   // padding
        bPadding = TRUE;
    }

    // �Ƿ�֧����״MDL��
    if (Adapter->IsTargetSupportChainedMdls
                    && bPadding == FALSE) {
        //
        // If the lower driver supports chained MDLs, we will
        // assign the MDL to the request and send it down. However if
        // the packet is less than 60 bytes, we need to pad zero to
        // make it atleast 60bytes long. In that case we will just use
        // our preallocated flat buffer. If this driver is adapted for
        // a real device, the user should only preallocate 60 bytes TCB
        // Buffer instead of NIC_BUFFER_SIZE.
        //
        // ����²�����֧����״MDL������ֱ�Ӱ��Ǹ�MDLָ�����
        // ����Ȼ����ȥ��������Σ����������ĳ���С��60��
        // �ڣ����Ǳ���׷��0������60�ֽڡ��������������ֻʹ��
        // ����Ԥ�ȷ���������ռ䡣������������������һ����
        // ʵ�豸��ʹ���߱���ֻ����60�ֽڵ�TCB��������������
        // NIC_BUFFER_SIZE��
        pTCB->Mdl = (PMDL)CurrentBuffer;
        pTCB->ulSize = PacketLength;
        Adapter->nBytesArrived += PacketLength;

    } else {

        BytesToCopy = PacketLength;

        //
        // Copy the data from chained buffers to our TCB flat buffer.
        //
        while (CurrentBuffer)
        {
            NdisQueryBufferSafe(
                CurrentBuffer,
                &VirtualAddress,
                &CurrentLength,
                NormalPagePriority);

            if (!VirtualAddress) {
                bResult = FALSE;
                break;
            }

            CurrentLength = min(CurrentLength, DestBufferSize);

            if (CurrentLength)
            {
                // Copy the data.
                NdisMoveMemory(pDest, VirtualAddress, CurrentLength);
                BytesCopied += CurrentLength;
                DestBufferSize -= CurrentLength;
                pDest += CurrentLength;
            }

            NdisGetNextBuffer(
                CurrentBuffer,
                &CurrentBuffer);
        }


        if (bResult) {

            Adapter->nBytesArrived += BytesCopied;

            //
            // If the packet size is less than ETH_MIN_PACKET_SIZE. Pad the buffer
            // up to ETH_MIN_PACKET_SIZE with zero bytes for security reason.
            //
            if (BytesCopied < BytesToCopy)
            {
                NdisZeroMemory(pDest, BytesToCopy-BytesCopied);
                BytesCopied = BytesToCopy;
            }

            //
            // Adjust the buffer length to reflect the new length
            //
            NdisAdjustBufferLength(pTCB->Buffer, BytesCopied);
            pTCB->ulSize = BytesCopied;

            pTCB->Mdl = pTCB->Buffer;

            if (PacketLength >= sizeof(ETH_HEADER)) {

                pEthHeader = (PETH_HEADER)pTCB->pData;

                //memcpy(pEthHeader->SrcAddr, Adapter->PhyNicMacAddress,
                //                ETH_LENGTH_OF_ADDRESS);

                DEBUGP(MP_LOUD, ("Src Address = %02x-%02x-%02x-%02x-%02x-%02x",
                    pEthHeader->SrcAddr[0],
                    pEthHeader->SrcAddr[1],
                    pEthHeader->SrcAddr[2],
                    pEthHeader->SrcAddr[3],
                    pEthHeader->SrcAddr[4],
                    pEthHeader->SrcAddr[5]));

                DEBUGP(MP_LOUD, ("  Dest Address = %02x-%02x-%02x-%02x-%02x-%02x\n",
                    pEthHeader->DstAddr[0],
                    pEthHeader->DstAddr[1],
                    pEthHeader->DstAddr[2],
                    pEthHeader->DstAddr[3],
                    pEthHeader->DstAddr[4],
                    pEthHeader->DstAddr[5]));

            }else{
                ASSERTMSG("Packet length is less than ETH_HEADER\n", FALSE);
                bResult = FALSE;
            }
        }
    }

    if (bResult){

        NdisInterlockedInsertTailList(
                            &Adapter->SendBusyList,
                            &pTCB->List,
                            &Adapter->SendLock);

    }

    DEBUGP(MP_TRACE, ("<-- NICCopyPacket\n"));
    return bResult;
}




