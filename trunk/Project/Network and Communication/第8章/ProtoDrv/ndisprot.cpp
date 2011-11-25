///////////////////////////////////////////////////
// ndisprot.cpp�ļ�
// NT��ڵ㣬��ǲ����

#define NDIS50 1  // ˵��Ҫʹ��NDIS 5.0

extern "C"
{
	#include <ndis.h>
	#include <ntddk.h>
	#include <stdio.h>
}
#include "nuiouser.h"
#include "ndisprot.h"
#pragma comment(lib, "ndis")

GLOBAL g_data;


// ��ʼ��Э������
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString)
{
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT pDeviceObj = NULL;
	NDIS_STRING protoName = NDIS_STRING_CONST("Packet");
	// ���û�ʹ�õķ�����������
	UNICODE_STRING ustrSymbolicLink;
	BOOLEAN bSymbolicLink = FALSE;

	DbgPrint(" ProtoDrv: DriverEntry...  \n");

	// ������������ָ�롣���g_data��GLOBAL���͵�ȫ�ֱ���
	g_data.pDriverObj = pDriverObj;

	do
	{
			// Ϊ����������һ�������豸�����û�����������豸����IOCTL���룬
			// �Ա��ȡ�󶨵���������Ϣ
		UNICODE_STRING ustrDevName;
		RtlInitUnicodeString(&ustrDevName, DEVICE_NAME);
		status = IoCreateDevice(pDriverObj, 
			0,
			&ustrDevName, 
			FILE_DEVICE_UNKNOWN,
			0,
			FALSE,
			&pDeviceObj);
		if(!NT_SUCCESS(status))
		{
			DbgPrint(" ProtoDrv: CreateDevice failed \n");
			break;
		}
		// Ϊ������豸������������
		RtlInitUnicodeString(&ustrSymbolicLink, LINK_NAME);
		status = IoCreateSymbolicLink(&ustrSymbolicLink, &ustrDevName);  
		if(!NT_SUCCESS(status))
		{
			DbgPrint(" ProtoDrv: CreateSymbolicLink failed \n");
			break;
		}
		bSymbolicLink = TRUE;
		// ����Ϊ������I/O��ʽ
		pDeviceObj->Flags |= DO_BUFFERED_IO;

			// ��ʼ��ȫ�ֱ���
		g_data.pControlDevice = pDeviceObj;
		InitializeListHead(&g_data.AdapterList);
		KeInitializeSpinLock(&g_data.GlobalLock);

			// ��ʼ��Э�������ṹ
		NDIS_PROTOCOL_CHARACTERISTICS protocolChar;
		NdisZeroMemory(&protocolChar, sizeof(protocolChar));
		protocolChar.Ndis40Chars.Ndis30Chars.MajorNdisVersion = 5;
		protocolChar.Ndis40Chars.Ndis30Chars.MinorNdisVersion = 0;

		protocolChar.Ndis40Chars.Ndis30Chars.Name = protoName;

		protocolChar.Ndis40Chars.BindAdapterHandler = ProtocolBindAdapter;
		protocolChar.Ndis40Chars.UnbindAdapterHandler = ProtocolUnbindAdapter;
		
		protocolChar.Ndis40Chars.Ndis30Chars.OpenAdapterCompleteHandler  = ProtocolOpenAdapterComplete;
		protocolChar.Ndis40Chars.Ndis30Chars.CloseAdapterCompleteHandler = ProtocolCloseAdapterComplete;

		protocolChar.Ndis40Chars.Ndis30Chars.ReceiveHandler              = ProtocolReceive;
//		protocolChar.Ndis40Chars.ReceivePacketHandler					= ProtocolReceivePacket;
		protocolChar.Ndis40Chars.Ndis30Chars.TransferDataCompleteHandler = ProtocolTransferDataComplete;

		protocolChar.Ndis40Chars.Ndis30Chars.SendCompleteHandler         = ProtocolSendComplete;

		
		protocolChar.Ndis40Chars.Ndis30Chars.ResetCompleteHandler        = ProtocolResetComplete;
		protocolChar.Ndis40Chars.Ndis30Chars.RequestCompleteHandler      = ProtocolRequestComplete;
		
		protocolChar.Ndis40Chars.Ndis30Chars.ReceiveCompleteHandler      = ProtocolReceiveComplete;
		
		protocolChar.Ndis40Chars.Ndis30Chars.StatusHandler               = ProtocolStatus;
		protocolChar.Ndis40Chars.Ndis30Chars.StatusCompleteHandler       = ProtocolStatusComplete;
		protocolChar.Ndis40Chars.PnPEventHandler						= ProtocolPNPHandler; 
		
			// ע��ΪЭ������
		NdisRegisterProtocol((PNDIS_STATUS)&status, 
			&g_data.hNdisProtocol, &protocolChar, sizeof(protocolChar));
		if(status != NDIS_STATUS_SUCCESS)
		{
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		DbgPrint(" ProtoDrv: NdisRegisterProtocol success \n");

			// ���ڣ���������Ҫ�������ǲ����
		pDriverObj->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
		pDriverObj->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
		pDriverObj->MajorFunction[IRP_MJ_READ]  = DispatchRead;
		pDriverObj->MajorFunction[IRP_MJ_WRITE]  = DispatchWrite;
		pDriverObj->MajorFunction[IRP_MJ_CLEANUP]  = DispatchCleanup;

		pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;
		pDriverObj->DriverUnload = DriverUnload;	
		status = STATUS_SUCCESS;
	}while(FALSE);
	
	if(!NT_SUCCESS(status))		// ������
	{
		if(pDeviceObj != NULL)
		{
			// ɾ���豸����
			IoDeleteDevice(pDeviceObj); 
			g_data.pControlDevice = NULL;
		}
		if(bSymbolicLink)
		{
			// ɾ����������
			IoDeleteSymbolicLink(&ustrSymbolicLink);
		}
	}
	return status;
}


// ж��
void DriverUnload(PDRIVER_OBJECT pDriverObj)
{	
	// ɾ�������豸����Ͷ�Ӧ�ķ�������
	UNICODE_STRING ustrLink;
	RtlInitUnicodeString(&ustrLink, LINK_NAME);
	IoDeleteSymbolicLink(&ustrLink);
	if(g_data.pControlDevice != NULL)
		IoDeleteDevice(g_data.pControlDevice);

	// ������а�
	NDIS_STATUS status;
	while(pDriverObj->DeviceObject != NULL) // ������˿����豸����֮�⣬����ȫ��NIC�豸����
	{
		ProtocolUnbindAdapter(&status, pDriverObj->DeviceObject->DeviceExtension, NULL);
	}	
	// ȡ��Э��������ע��
	NdisDeregisterProtocol(&status, g_data.hNdisProtocol);
}


// ����IRP_MJ_CREATE��IRP_MJ_CLOSE���ܴ���
NTSTATUS DispatchClose(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint(" ProtoDrv: DispatchClose \n");
	NTSTATUS status = STATUS_SUCCESS;
	
	if(pDevObj == g_data.pControlDevice)
	{
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return status;
	}
	
	POPEN_INSTANCE pOpen = (POPEN_INSTANCE)pDevObj->DeviceExtension;

	IoIncrement(pOpen);
	
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	
	IoDecrement(pOpen);


	return status;
}
// ����IRP_MJ_CREATE��IRP_MJ_CLOSE���ܴ���
NTSTATUS DispatchCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint(" ProtoDrv: DispatchCreate \n");
	NTSTATUS status = STATUS_SUCCESS;
	
	if(pDevObj == g_data.pControlDevice)
	{
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return status;
	}
	
	POPEN_INSTANCE pOpen = (POPEN_INSTANCE)pDevObj->DeviceExtension;


	IoIncrement(pOpen);

	if(!pOpen->bBound)
	{
		status = STATUS_DEVICE_NOT_READY;
	}
	
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	
	IoDecrement(pOpen);
	return status;
}


// I/O������ǲ����
NTSTATUS DispatchIoctl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
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

	if(uIoControlCode == IOCTL_ENUM_ADAPTERS)
	{
		ULONG nDataLen = 0;
		if(pDevObj != g_data.pControlDevice)
			status = STATUS_INVALID_DEVICE_REQUEST;
		else
		{
			status = GetAdapterList(pIoBuffer, uOutSize, &nDataLen);
			if(status != STATUS_SUCCESS)
				DbgPrint("GetAdapterList error ");
		}
		pIrp->IoStatus.Information = nDataLen;
		pIrp->IoStatus.Status = status;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return status;
	}

	OPEN_INSTANCE *pOpen = (OPEN_INSTANCE *)pDevObj->DeviceExtension;
	if(pOpen == NULL || !pOpen->bBound)
	{
		pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_UNSUCCESSFUL;
	}

	IoIncrement(pOpen);

	IoMarkIrpPending(pIrp);

	if(uIoControlCode == IOCTL_PROTOCOL_RESET) 
	{
		// �����IRP������IRP�б�
       ExInterlockedInsertTailList(
                &pOpen->ResetIrpList,
                &pIrp->Tail.Overlay.ListEntry,
                &pOpen->ResetQueueLock);

	   // ������������
        NdisReset(
            &status,
            pOpen->hAdapter
            );
        if(status != NDIS_STATUS_PENDING) 
		{
            ProtocolResetComplete(
                pOpen,
                status);
        }
    }

	// ��ȡ��������OID��Ϣ
	else if(uIoControlCode == IOCTL_PROTOCOL_SET_OID 
				|| uIoControlCode == IOCTL_PROTOCOL_QUERY_OID) // ���������һ���Զ����PROTOCOL_OID_DATA�ṹ
	{
		PPROTOCOL_OID_DATA pOidData = (PPROTOCOL_OID_DATA)pIoBuffer;
		// ����һ��INTERNAL_REQUEST�ṹ
		PINTERNAL_REQUEST pInterRequest = 
			(PINTERNAL_REQUEST)ExAllocatePool(NonPagedPool, sizeof(INTERNAL_REQUEST));
	   if(pInterRequest == NULL)
        {
            pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
            IoDecrement(pOpen);
            return STATUS_PENDING;
        }         
        pInterRequest->pIrp = pIrp;

		if(uOutSize == uInSize && uOutSize >= sizeof(PROTOCOL_OID_DATA) &&
					uOutSize >= sizeof(PROTOCOL_OID_DATA) - 1 + pOidData->Length)	// ���������ã�
		{
			// ��ʼ��NDIS_REQUEST�ṹ
			if(uIoControlCode == IOCTL_PROTOCOL_SET_OID)
			{
				pInterRequest->Request.RequestType = NdisRequestSetInformation;
				pInterRequest->Request.DATA.SET_INFORMATION.Oid = pOidData->Oid;
				pInterRequest->Request.DATA.SET_INFORMATION.InformationBuffer = pOidData->Data;
				pInterRequest->Request.DATA.SET_INFORMATION.InformationBufferLength = pOidData->Length;
			}
			else
			{
				pInterRequest->Request.RequestType = NdisRequestQueryInformation;
				pInterRequest->Request.DATA.QUERY_INFORMATION.Oid = pOidData->Oid;
				pInterRequest->Request.DATA.QUERY_INFORMATION.InformationBuffer = pOidData->Data;
				pInterRequest->Request.DATA.QUERY_INFORMATION.InformationBufferLength = pOidData->Length;
			}

			// �ύ�������
			NdisRequest(&status, pOpen->hAdapter, &pInterRequest->Request);
		}
		else
		{
			status = NDIS_STATUS_FAILURE;
            pInterRequest->Request.DATA.SET_INFORMATION.BytesRead = 0;
            pInterRequest->Request.DATA.QUERY_INFORMATION.BytesWritten = 0;
		}
		
		if(status != NDIS_STATUS_PENDING)
		{
			ProtocolRequestComplete(pOpen, &pInterRequest->Request, status);
		}
	}

	return STATUS_PENDING;
}

VOID
ProtocolResetComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status
    )

{
	OPEN_INSTANCE *pOpen;
    pOpen = (OPEN_INSTANCE*)ProtocolBindingContext;

	// ȡ��IRPָ��
	PLIST_ENTRY pListEntry = ExInterlockedRemoveHeadList(
                       &pOpen->ResetIrpList,
                       &pOpen->ResetQueueLock
                       );
    PIRP pIrp = CONTAINING_RECORD(pListEntry,IRP,Tail.Overlay.ListEntry);

	// ��ɴ�IRP
    if(Status == NDIS_STATUS_SUCCESS) 
	{
        pIrp->IoStatus.Status = STATUS_SUCCESS;
    } 
	else 
	{
        pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    }
    pIrp->IoStatus.Information = 0;    
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    IoDecrement(pOpen);
}


VOID
ProtocolRequestComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS   Status
    )
{
	POPEN_INSTANCE pOpen = (POPEN_INSTANCE)ProtocolBindingContext;
	PINTERNAL_REQUEST pInterRequest = CONTAINING_RECORD(NdisRequest, INTERNAL_REQUEST, Request);
	PIRP pIrp = pInterRequest->pIrp;

	if(Status == NDIS_STATUS_SUCCESS)
	{
		PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
		UINT nIoControlCode = pIrpSp->Parameters.DeviceIoControl.IoControlCode;
		PPROTOCOL_OID_DATA pOidData = (PPROTOCOL_OID_DATA)pIrp->AssociatedIrp.SystemBuffer;

		//  ����С���ص��û�������
        if(nIoControlCode == IOCTL_PROTOCOL_SET_OID) 
		{
            pOidData->Length = pInterRequest->Request.DATA.SET_INFORMATION.BytesRead;
        } 
		else if(nIoControlCode == IOCTL_PROTOCOL_QUERY_OID) 
		{
			pOidData->Length = pInterRequest->Request.DATA.QUERY_INFORMATION.BytesWritten;
		}	

		// ���÷��ظ�I/O����������Ϣ
		pIrp->IoStatus.Information = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
		pIrp->IoStatus.Status = STATUS_SUCCESS;
	}
	else
	{
		pIrp->IoStatus.Information = 0;
		pIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	}
	ExFreePool(pInterRequest);
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	IoDecrement(pOpen);
}

NTSTATUS
GetAdapterList(
    IN  PVOID              Buffer,		// ������
    IN  ULONG              Length,		// ��������С
    IN  OUT PULONG         DataLength	// ����ʵ����Ҫ�ĳ���
    )
{
	KIRQL oldIrql;
	KeAcquireSpinLock(&g_data.GlobalLock, &oldIrql);

	OPEN_INSTANCE *pOpen ;

		// �����б���������Ļ�������С
	ULONG nRequiredLength = 0;
	ULONG nAdapters = 0;

	PLIST_ENTRY pThisEntry;
	PLIST_ENTRY pHeader = &g_data.AdapterList;
	for(pThisEntry = pHeader->Flink ; pThisEntry != pHeader; pThisEntry = pThisEntry->Flink)
	{
		pOpen = CONTAINING_RECORD(pThisEntry, OPEN_INSTANCE, AdapterListEntry);
		nRequiredLength += pOpen->ustrAdapterName.Length + sizeof(UNICODE_NULL);
		nRequiredLength += pOpen->ustrLinkName.Length + sizeof(UNICODE_NULL);
		nAdapters++;
	}

	// ���ǽ�Ҫ������ĸ�ʽ�������ݣ�
	// nAdapters + һ�����߶����"AdapterName\0" + "SymbolicLink\0"�� + UNICODE_NULL
	// ���ԣ�����Ҫ������nAapters��UNICODE_NULL�Ĵ�С
	nRequiredLength += sizeof(nAdapters) + sizeof(UNICODE_NULL); 

	*DataLength = nRequiredLength;
	if(nRequiredLength > Length) 
	{
        KeReleaseSpinLock(&g_data.GlobalLock, oldIrql);
        return STATUS_BUFFER_TOO_SMALL;
    }

		// ��仺����
	// ����������������
	*(PULONG)Buffer = nAdapters;
	Buffer = (PCHAR)Buffer + sizeof(ULONG);

	// Ȼ�����������ͷ�����������
	for(pThisEntry = pHeader->Flink; 
			pThisEntry != pHeader;
			pThisEntry = pThisEntry->Flink)
    {
        pOpen = CONTAINING_RECORD(pThisEntry, OPEN_INSTANCE, AdapterListEntry);
        
        RtlCopyMemory(Buffer, pOpen->ustrAdapterName.Buffer,
                            pOpen->ustrAdapterName.Length + sizeof(WCHAR));

		Buffer = (PCHAR)Buffer + pOpen->ustrAdapterName.Length + sizeof(WCHAR);

        
        RtlCopyMemory(Buffer, pOpen->ustrLinkName.Buffer,
                            pOpen->ustrLinkName.Length + sizeof(WCHAR));

		Buffer = (PCHAR)Buffer + pOpen->ustrLinkName.Length + sizeof(WCHAR);                       
    }
	
	// ���Ľ�����־
    *(PWCHAR)Buffer = UNICODE_NULL;
    
    KeReleaseSpinLock(&g_data.GlobalLock, oldIrql);

	return STATUS_SUCCESS;
}



void IoIncrement(OPEN_INSTANCE *pOpen)
{
	if(InterlockedIncrement((PLONG)&pOpen->nIrpCount) == 1)
		NdisResetEvent(&pOpen->CleanupEvent);
}

void IoDecrement(OPEN_INSTANCE *pOpen)
{
	if(InterlockedDecrement((PLONG)&pOpen->nIrpCount) == 0)
		NdisSetEvent(&pOpen->CleanupEvent);
}




NTSTATUS
DispatchCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    POPEN_INSTANCE      pOpen = (POPEN_INSTANCE)DeviceObject->DeviceExtension;
    NTSTATUS            status = STATUS_SUCCESS;


    if(DeviceObject == g_data.pControlDevice) 
	{
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    IoIncrement(pOpen);
 
    CancelReadIrp(DeviceObject);

    IoDecrement(pOpen);

    NdisWaitEvent(&pOpen->CleanupEvent, 0);

    Irp->IoStatus.Information = 0;    
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return status;

}


VOID
ProtocolStatus(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN NDIS_STATUS   Status,
    IN PVOID         StatusBuffer,
    IN UINT          StatusBufferSize
    )
{
}

VOID
ProtocolStatusComplete(
    IN NDIS_HANDLE  ProtocolBindingContext
    )
{
}



NDIS_STATUS
ProtocolPNPHandler(
    IN    NDIS_HANDLE        ProtocolBindingContext,
    IN    PNET_PNP_EVENT     NetPnPEvent
    )
{
    NDIS_STATUS                 Status  = NDIS_STATUS_SUCCESS;
    PNET_DEVICE_POWER_STATE     powerState;
    
    powerState = (PNET_DEVICE_POWER_STATE)NetPnPEvent->Buffer;
    switch(NetPnPEvent->NetEvent)
    {
         case  NetEventSetPower :
            switch (*powerState) 
			{
            
                case NetDeviceStateD0:
                    Status = NDIS_STATUS_SUCCESS;
                    break;

                default:
                    //
                    // We can't suspend, so we ask NDIS to Unbind us by
                    // returning this status:
                    //
                    Status = NDIS_STATUS_NOT_SUPPORTED;
                    break;
            }
    }
    return Status;
}




