/*
 * Copyright (c) 2004 Security Architects Corporation. All rights reserved.
 *
 * Module Name:
 *
 *		network.c
 *
 * Abstract:
 *
 *		This module defines various routines used for hooking the Transport Driver Interface (TDI) network routines.
 *
 * Author:
 *
 *		Eugene Tsyrklevich 12-Mar-2004
 *
 * Revision History:
 *
 *		None.
 */


#include <NTDDK.h>
#include <tdikrnl.h>
#include <ctype.h>
#include "network.h"
#include "hookproc.h"
#include "userland.h"
#include "learn.h"
#include "policy.h"
#include "log.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, InstallNetworkHooks)
#pragma alloc_text (PAGE, RemoveNetworkHooks)
#endif


//XXX fast io is not handled. TdiDispatchFastDeviceControl


PDEVICE_OBJECT	pTcpDevice = NULL, pTcpDeviceOriginal = NULL;
PDEVICE_OBJECT	pUdpDevice = NULL, pUdpDeviceOriginal = NULL;
PDEVICE_OBJECT	pIpDevice = NULL, pIpDeviceOriginal = NULL;

#if DBG
int	HookedTDIRunning = 0;
#endif


/*
 * TdiStub() XXX remove
 *
 * Description:
 *		.
 *
 * Parameters:
 *		pIrp - IRP (I/O Request Packet) request.
 *		pIrpStack - .
 *		pCompletion - .
 *
 * Returns:
 *		STATUS_SUCCESS.
 */

NTSTATUS
TdiStub(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrpStack, OUT PTDI_CALLBACK pCompletion, IN ULONG DeviceType)
{
//	LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("TdiStub(%x %x %x)\n", pIrp, pIrpStack, pCompletion));
	return STATUS_SUCCESS;
}



NTSTATUS
TdiSetEventHandler(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrpStack, OUT PTDI_CALLBACK pCompletion, IN ULONG DeviceType)
{
	PTDI_REQUEST_KERNEL_SET_EVENT	r = (PTDI_REQUEST_KERNEL_SET_EVENT) &pIrpStack->Parameters;


	if (r->EventType != TDI_EVENT_CONNECT)
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("%d TdiSetEventHandler: %x %x %x\n", CURRENT_PROCESS_PID, r->EventType, r->EventHandler, r->EventContext));
		return STATUS_SUCCESS;
	}


	if (r->EventHandler == NULL)
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("%d TdiSetEventHandler: TDI_EVENT_CONNECT deregistration %x %x %x\n", CURRENT_PROCESS_PID, r->EventHandler, r->EventContext, pIrpStack->FileObject));
		return STATUS_SUCCESS;
	}


	LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("%d TdiSetEventHandler: TDI_EVENT_CONNECT %x %x %x\n", CURRENT_PROCESS_PID, r->EventHandler, r->EventContext, pIrpStack->FileObject));


	return STATUS_SUCCESS;
}



NTSTATUS
TdiConnect(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrpStack, OUT PTDI_CALLBACK pCompletion, IN ULONG DeviceType)
{
	/*
	 * IrpSp->Parameters 
	 *
	 * Pointer to a TDI_REQUEST_KERNEL_CONNECT structure, equivalent to the TDI_REQUEST_KERNEL structure.
	 */

	PTDI_REQUEST_KERNEL_CONNECT		ConnectInfo = (PTDI_REQUEST_KERNEL_CONNECT) &pIrpStack->Parameters;
	PTRANSPORT_ADDRESS				pTransportAddress;
	PTA_ADDRESS						pAddress;
	PTDI_ADDRESS_IP					ip;
	CHAR							NETWORKNAME[MAX_PATH];
	PCHAR							FunctionName = "TdiConnect";


	HOOK_ROUTINE_ENTER();


	if (! MmIsAddressValid(ConnectInfo) ||
		! MmIsAddressValid(ConnectInfo->RequestConnectionInformation) ||
		! MmIsAddressValid(ConnectInfo->RequestConnectionInformation->RemoteAddress))
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("TdiConnect: MmIsAddressValid failed\n"));
		HOOK_ROUTINE_EXIT(STATUS_SUCCESS);
	}


	pTransportAddress = (PTRANSPORT_ADDRESS) ConnectInfo->RequestConnectionInformation->RemoteAddress;

	pAddress = (PTA_ADDRESS) pTransportAddress->Address;

	/* verify that the specified address is a single IP address */
	if (pTransportAddress->TAAddressCount != 1 ||
		pAddress->AddressType != TDI_ADDRESS_TYPE_IP ||
		pAddress->AddressLength != TDI_ADDRESS_LENGTH_IP)        
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("%d TdiConnect: Invalid address detected\n", CURRENT_PROCESS_PID));
		HOOK_ROUTINE_EXIT(STATUS_SUCCESS);
	}

	ip = (PTDI_ADDRESS_IP) &pAddress->Address;


	LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("%d TdiConnect(%x %x %x). %d %x:%u (%s)\n", (ULONG) PsGetCurrentProcessId(), pIrp, pIrpStack, pCompletion, pTransportAddress->TAAddressCount, ntohl(ip->in_addr), ntohs(ip->sin_port), inet_ntoa2(ip->in_addr)));


	inet_ntoa(ip->in_addr, NETWORKNAME);

	if (LearningMode == FALSE)
	{
		POLICY_CHECK_OPTYPE_NAME(NETWORK, DeviceType == NET_DEVICE_TYPE_TCP ? OP_TCPCONNECT : OP_UDPCONNECT);
	}
	else
	{
		// learning mode
		AddRule(RULE_NETWORK, NETWORKNAME, DeviceType == NET_DEVICE_TYPE_TCP ? OP_TCPCONNECT : OP_UDPCONNECT);
	}


	HOOK_ROUTINE_EXIT(STATUS_SUCCESS);
}



NTSTATUS
TdiListen(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrpStack, OUT PTDI_CALLBACK pCompletion, IN ULONG DeviceType)
{
	/*
	 * IrpSp->Parameters 
	 *
	 * Pointer to a TDI_REQUEST_KERNEL_LISTEN structure, equivalent to the TDI_REQUEST_KERNEL structure.
	 */

	PTDI_REQUEST_KERNEL_LISTEN		ListenInfo = (PTDI_REQUEST_KERNEL_LISTEN) &pIrpStack->Parameters;
	PTRANSPORT_ADDRESS				pTransportAddress;
	PTA_ADDRESS						pAddress;
	PTDI_ADDRESS_IP					ip;


	if (! MmIsAddressValid(ListenInfo) ||
		! MmIsAddressValid(ListenInfo->RequestConnectionInformation) ||
		! MmIsAddressValid(ListenInfo->RequestConnectionInformation->RemoteAddress))
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("TdiListen: MmIsAddressValid failed\n"));
		HOOK_ROUTINE_EXIT(STATUS_SUCCESS);
	}

	pTransportAddress = (PTRANSPORT_ADDRESS) ListenInfo->RequestConnectionInformation->RemoteAddress;

	pAddress = (PTA_ADDRESS) pTransportAddress->Address;

	/* verify that the specified address is a single IP address */
	if (pTransportAddress->TAAddressCount != 1 ||
		pAddress->AddressType != TDI_ADDRESS_TYPE_IP ||
		pAddress->AddressLength != TDI_ADDRESS_LENGTH_IP)        
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("%d TdiListen: Invalid address detected\n", CURRENT_PROCESS_PID));
		HOOK_ROUTINE_EXIT(STATUS_SUCCESS);
	}

	ip = (PTDI_ADDRESS_IP) &pAddress->Address;


	LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("TdiListen(%x %x %x). %d %x:%u (%s)\n", pIrp, pIrpStack, pCompletion, pTransportAddress->TAAddressCount, ntohl(ip->in_addr), ntohs(ip->sin_port), inet_ntoa2(ip->in_addr)));


	return STATUS_SUCCESS;
}



NTSTATUS
TdiAccept(IN PIRP pIrp, IN PIO_STACK_LOCATION pIrpStack, OUT PTDI_CALLBACK pCompletion, IN ULONG DeviceType)
{
	/*
	 * IrpSp->Parameters 
	 *
	 * Specifies a TDI_REQUEST_KERNEL_ACCEPT structure.
	 */

	PTDI_REQUEST_KERNEL_ACCEPT		AcceptInfo = (PTDI_REQUEST_KERNEL_ACCEPT) &pIrpStack->Parameters;
	PTRANSPORT_ADDRESS				pTransportAddress = (PTRANSPORT_ADDRESS) AcceptInfo->RequestConnectionInformation->RemoteAddress;
	PTA_ADDRESS						pAddress = (PTA_ADDRESS) pTransportAddress->Address;
	PTDI_ADDRESS_IP					ip = (PTDI_ADDRESS_IP) &pAddress->Address;


	LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("TdiAccept(%x %x %x). %d %x:%u (%s)\n", pIrp, pIrpStack, pCompletion, pTransportAddress->TAAddressCount, ntohl(ip->in_addr), ntohs(ip->sin_port), inet_ntoa2(ip->in_addr)));


	return STATUS_SUCCESS;
}


/*
NTSTATUS
GenericCompletion(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp, IN PVOID pContext)
{             
	if (pIrp->PendingReturned )
		IoMarkIrpPending(pIrp);
    
	return STATUS_SUCCESS;
}
*/

			   
TDI_IOCTL	TdiIoctl[] =
{
	{ TDI_ASSOCIATE_ADDRESS, "TDI_ASSOCIATE_ADDRESS", TdiStub },
	{ TDI_DISASSOCIATE_ADDRESS, "TDI_DISASSOCIATE_ADDRESS", TdiStub },
	{ TDI_CONNECT, "TDI_CONNECT", TdiConnect },
	{ TDI_LISTEN, "TDI_LISTEN", TdiListen },
	{ TDI_ACCEPT, "TDI_ACCEPT", TdiAccept },
	{ TDI_DISCONNECT, "TDI_DISCONNECT", TdiStub },
	{ TDI_SEND, "TDI_SEND", TdiStub },
	{ TDI_RECEIVE, "TDI_RECEIVE", TdiStub },
	{ TDI_SEND_DATAGRAM, "TDI_SEND_DATAGRAM", TdiStub },
	{ TDI_RECEIVE_DATAGRAM, "TDI_RECEIVE_DATAGRAM", TdiStub },
	{ TDI_SET_EVENT_HANDLER, "TDI_SET_EVENT_HANDLER", TdiSetEventHandler },
	{ TDI_QUERY_INFORMATION, "TDI_QUERY_INFORMATION", TdiStub },
	{ TDI_SET_INFORMATION, "TDI_SET_INFORMATION", TdiStub },
	{ TDI_ACTION, "TDI_ACTION", TdiStub },
	{ TDI_DIRECT_SEND, "TDI_DIRECT_SEND", TdiStub },
	{ TDI_DIRECT_SEND_DATAGRAM, "TDI_DIRECT_SEND_DATAGRAM", TdiStub },
};



//XXX this function can be called from HookedNtCreateFile (-> NtCreateFile -> IoCreateFile -> ObOpenObjectbyName -> ... -> TDI)
BOOLEAN
TDIDispatch(PDEVICE_OBJECT pDeviceObject, PIRP pIrp, NTSTATUS *status)
{
	PIO_STACK_LOCATION	pIrpStack;
	TDI_CALLBACK		Callback;
	ULONG				DeviceType = 0;	


	if (pDeviceObject == pTcpDevice)
	{
		DeviceType = NET_DEVICE_TYPE_TCP;
	}
	else if (pDeviceObject == pUdpDevice)
	{
		DeviceType = NET_DEVICE_TYPE_UDP;
	}
	else if (pDeviceObject == pIpDevice)
	{
		DeviceType = NET_DEVICE_TYPE_IP;
	}
	else
	{
		return FALSE;
	}


	HOOK_TDI_ENTER_NORC();


	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);


	memset(&Callback, 0, sizeof(Callback));

//	if (pIrpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER)
//	{
//		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER\n"));
//	}

	switch (pIrpStack->MajorFunction)
	{
		case IRP_MJ_CREATE:

			*status = TDICreate(pDeviceObject, pIrp, pIrpStack, &Callback);

			break;


		case IRP_MJ_DEVICE_CONTROL:

//			if (DeviceType == NET_DEVICE_TYPE_IP && pIrpStack->Parameters.DeviceIoControl.IoControlCode == 0x120000)
			if (DeviceType == NET_DEVICE_TYPE_IP)
			{
				LOG(LOG_SS_NETWORK, LOG_PRIORITY_VERBOSE, ("%d pIpDevice in use (%x %x %x)\n", (ULONG) PsGetCurrentProcessId(), pIrpStack->Parameters.DeviceIoControl.IoControlCode, pIrpStack->MajorFunction, pIrpStack->MinorFunction));
//				*status = STATUS_ACCESS_DENIED;
				break;
			}

			if (KeGetCurrentIrql() != PASSIVE_LEVEL || ! NT_SUCCESS(TdiMapUserRequest(pDeviceObject, pIrp, pIrpStack)))
			{
				LOG(LOG_SS_NETWORK, LOG_PRIORITY_VERBOSE, ("TdiMapUserRequest failed: %x (irql %d)\n", pIrpStack->Parameters.DeviceIoControl.IoControlCode, KeGetCurrentIrql()));
				break;
			}

			LOG(LOG_SS_NETWORK, LOG_PRIORITY_VERBOSE, ("IRP_MJ_DEVICE_CONTROL2 %x\n", pIrpStack->Parameters.DeviceIoControl.IoControlCode));

			/* FALLTHROUGH */


		case IRP_MJ_INTERNAL_DEVICE_CONTROL:
		{
			int		i;

			if (DeviceType == NET_DEVICE_TYPE_IP)
				LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("%d pIpDevice in use2\n", (ULONG) PsGetCurrentProcessId()));

			for (i = 0; i < sizeof(TdiIoctl) / sizeof(TdiIoctl[0]); i++)
			{
				if (TdiIoctl[i].MinorFunction == pIrpStack->MinorFunction)
				{
					if (TdiIoctl[i].pfRoutine == TdiStub)
						LOG(LOG_SS_NETWORK, LOG_PRIORITY_VERBOSE, ("%d IRP_MJ_INTERNAL_DEVICE_CONTROL %s\n", (ULONG) PsGetCurrentProcessId(), TdiIoctl[i].Description));

					*status = TdiIoctl[i].pfRoutine(pIrp, pIrpStack, &Callback, DeviceType);

					break;
				}
			}

			break;
		}

		case IRP_MJ_CLEANUP:
			LOG(LOG_SS_NETWORK, LOG_PRIORITY_VERBOSE, ("IRP_MJ_CLEANUP\n"));
			break;

		case IRP_MJ_CLOSE:
			LOG(LOG_SS_NETWORK, LOG_PRIORITY_VERBOSE, ("IRP_MJ_CLOSE\n"));
			break;

		default:
			LOG(LOG_SS_NETWORK, LOG_PRIORITY_VERBOSE, ("TDIDispatch: default switch case triggered\n"));
			break;
	}


	if (*status == STATUS_ACCESS_DENIED)
	{
		pIrp->IoStatus.Status = STATUS_ACCESS_DENIED;
		IoCompleteRequest (pIrp, IO_NO_INCREMENT);

		HOOK_TDI_EXIT(TRUE);
	}


	if (Callback.Routine)
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("TDI Callback.Routine\n"));

		//XXX IoCopyCurrentIrpStackLocationToNext()
		IoSetCompletionRoutine(pIrp, Callback.Routine, Callback.Context, TRUE, TRUE, TRUE);
	}
	else
	{
		// Set up a completion routine to handle the bubbling of the "pending" mark of an IRP
//		IoSetCompletionRoutine(pIrp, GenericCompletion, NULL, TRUE, TRUE, TRUE);

		IoSkipCurrentIrpStackLocation(pIrp);
	}


	if (DeviceType == NET_DEVICE_TYPE_TCP)
	{
		*status = IoCallDriver(pTcpDeviceOriginal, pIrp);
	}
	else if (DeviceType == NET_DEVICE_TYPE_UDP)
	{
		*status = IoCallDriver(pUdpDeviceOriginal, pIrp);
	}
	else if (DeviceType == NET_DEVICE_TYPE_IP)
	{
		*status = IoCallDriver(pIpDeviceOriginal, pIrp);
	}
	else
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("TDIDispatch: Unknown device type\n"));
	}


	HOOK_TDI_EXIT(TRUE);
}



NTSTATUS
TDICreateAddressCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp, IN PVOID Context)
{
	return STATUS_SUCCESS;
}



NTSTATUS
TDICreate(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp, IN PIO_STACK_LOCATION pIrpStack, OUT PTDI_CALLBACK pCompletion)
{
	FILE_FULL_EA_INFORMATION	*ea = (FILE_FULL_EA_INFORMATION *) pIrp->AssociatedIrp.SystemBuffer;


	HOOK_ROUTINE_ENTER();


	LOG(LOG_SS_NETWORK, LOG_PRIORITY_VERBOSE, ("TDICreate(%x %x %x)\n", pIrp, pIrpStack, ea));


	/*
	 * From DDK (TdiDispatchCreate):
	 *
	 * Irp->AssociatedIrp.SystemBuffer
	 *
	 * Pointer to a FILE_FULL_EA_INFORMATION-structured buffer if the file object represents an address or a
	 * connection endpoint to be opened. 
	 * For an address, the EaName member is set to the system-defined constant TdiTransportAddress and the EA value
	 * following the EaName array is of type TRANSPORT_ADDRESS, set up by the client to specify the address to be
	 * opened. For some transports, this value can be a symbolic netBIOS or DNS name to be translated by the transport. 
	 *
	 * For a connection endpoint, the EaName member is set to the system-defined constant TdiConnectionContext and
	 * the EA value following the EaName array is a client-supplied handle, opaque to the transport driver. The
	 * transport must save this handle and subsequently pass it back to the client's registered event handlers for
	 * this connection. 
	 *
	 * If the given file object represents a control channel, this member is NULL. 
	 */

	if (ea == NULL)
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_VERBOSE, ("TDICreate: Control channel\n"));
		HOOK_ROUTINE_EXIT(STATUS_SUCCESS);
	}


	if (! MmIsAddressValid(ea) || ea->EaName == NULL || ! MmIsAddressValid(ea->EaName))
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("TDICreate: MmIsAddressValid() failed\n"));
		HOOK_ROUTINE_EXIT(STATUS_SUCCESS);
	}


	if (ea->EaNameLength == TDI_CONNECTION_CONTEXT_LENGTH &&
				memcmp(ea->EaName, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH) == 0)
	{	
		CONNECTION_CONTEXT conn_ctx = *(CONNECTION_CONTEXT *) (ea->EaName + ea->EaNameLength + 1);

		if (conn_ctx)
			LOG(LOG_SS_NETWORK, LOG_PRIORITY_VERBOSE, ("TDI Connection object 0x%x %x\n", conn_ctx, * (PULONG) conn_ctx));

		HOOK_ROUTINE_EXIT(STATUS_SUCCESS);
	}


	// NOTE: for RawIp you can extract protocol number from irps->FileObject->FileName

	if (ea->EaNameLength == TDI_TRANSPORT_ADDRESS_LENGTH &&
		memcmp(ea->EaName, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH) == 0)
	{
		PTRANSPORT_ADDRESS	pTransportAddress;
		PTA_ADDRESS			pAddress;
		PIRP				QueryIrp;
		int					i;


		pTransportAddress = (PTRANSPORT_ADDRESS) (ea->EaName + ea->EaNameLength + 1);
		pAddress = pTransportAddress->Address;

		LOG(LOG_SS_NETWORK, LOG_PRIORITY_VERBOSE, ("TDICreate: TDI Address object. Num %d\n", pTransportAddress->TAAddressCount));


		for (i = 0; i < pTransportAddress->TAAddressCount; i++)
		{
			LOG(LOG_SS_NETWORK, LOG_PRIORITY_VERBOSE, ("TDICreate: TDI Address %d: %x %x\n", i, pAddress->AddressLength, pAddress->AddressType));


			if (pAddress->AddressType == TDI_ADDRESS_TYPE_IP)
			{
				PTDI_ADDRESS_IP		ip = (PTDI_ADDRESS_IP) &pAddress->Address;
				CHAR				NETWORKNAME[MAX_PATH];
				PCHAR				FunctionName = "TDICreate";


				if (ip->sin_port != 0)
				{
					LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("%d TDICreate: Bind IP %x:%u (%s)\n", (ULONG) PsGetCurrentProcessId(), ntohl(ip->in_addr), ntohs(ip->sin_port), inet_ntoa2(ip->in_addr)));

					itoa( ntohs(ip->sin_port), NETWORKNAME, 10 );
					//inet_ntoa(ip->in_addr, NETWORKNAME);

					if (LearningMode == FALSE)
					{
						POLICY_CHECK_OPTYPE_NAME(NETWORK, OP_BIND);
					}
					else
					{
						// learning mode
						AddRule(RULE_NETWORK, NETWORKNAME, OP_BIND);
					}
				}
				else
				{
					LOG(LOG_SS_NETWORK, LOG_PRIORITY_VERBOSE, ("%d TDICreate: IP & port are both zero\n", (ULONG) PsGetCurrentProcessId()));
				}
			}
			else
			{
				//XXX fail if only IP network addresses are allowed.
			}

			pAddress += 1;
		}

		//XXX reread WDM ch 5.3 "COmpleting I/O requests"
/*
		QueryIrp = TdiBuildInternalDeviceControlIrp(TDI_QUERY_INFORMATION, pDeviceObject,
													pIrpStack->FileObject, NULL, NULL);
		if (QueryIrp == NULL)
		{
			LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("TDICreate: QueryIrp is NULL\n"));
			return FALSE;
		}

		pCompletion->Routine = TDICreateAddressCompletion;
		pCompletion->Context = QueryIrp;
*/
	}


	HOOK_ROUTINE_EXIT(STATUS_SUCCESS);
}



/*
 * InstallNetworkHooks()
 *
 * Description:
 *		.
 *
 *		NOTE: Called once during driver initialization (DriverEntry()).
 *			  There is no need to cleanup in case a failure since RemoveNetworkHooks() will be called later.
 *
 * Parameters:
 *		pDriverObject - pointer to a driver object that represents this driver.
 *
 * Returns:
 *		STATUS_SUCCESS to indicate success or an NTSTATUS error code if failed.
 */

NTSTATUS
InstallNetworkHooks(PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING	Name;
	NTSTATUS		status;


	status = IoCreateDevice(pDriverObject, 0, NULL, FILE_DEVICE_UNKNOWN, 0, TRUE, &pTcpDevice);
	if (! NT_SUCCESS(status))
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("InstallNetworkHooks: IoCreateDevice(tcp) failed\n"));
		return status;
	}


	status = IoCreateDevice(pDriverObject, 0, NULL, FILE_DEVICE_UNKNOWN, 0, TRUE, &pUdpDevice);
	if (! NT_SUCCESS(status))
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("InstallNetworkHooks: IoCreateDevice(udp) failed\n"));
		return status;
	}


	status = IoCreateDevice(pDriverObject, 0, NULL, FILE_DEVICE_UNKNOWN, 0, TRUE, &pIpDevice);
	if (! NT_SUCCESS(status))
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("InstallNetworkHooks: IoCreateDevice(udp) failed\n"));
		return status;
	}


	RtlInitUnicodeString(&Name, L"\\Device\\Tcp");

	status = IoAttachDevice(pTcpDevice, &Name, &pTcpDeviceOriginal);
	if (! NT_SUCCESS(status))
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("InstallNetworkHooks: IoAttachDevice(\\Device\\Tcp) failed\n"));
		return status;
	}


	RtlInitUnicodeString(&Name, L"\\Device\\Udp");

	status = IoAttachDevice(pUdpDevice, &Name, &pUdpDeviceOriginal);
	if (! NT_SUCCESS(status))
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("InstallNetworkHooks: IoAttachDevice(\\Device\\Udp) failed\n"));
		return status;
	}


	RtlInitUnicodeString(&Name, L"\\Device\\Ip");

	status = IoAttachDevice(pIpDevice, &Name, &pIpDeviceOriginal);
	if (! NT_SUCCESS(status))
	{
		LOG(LOG_SS_NETWORK, LOG_PRIORITY_DEBUG, ("InstallNetworkHooks: IoAttachDevice(\\Device\\Ip) failed\n"));
		return status;
	}


	pTcpDevice->StackSize = pTcpDeviceOriginal->StackSize + 1;
	// XXX Flags &= ~DO_DEVICE_INITIALIZING;
	pTcpDevice->Flags |= pTcpDeviceOriginal->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE | DO_POWER_INRUSH) & ~DO_DEVICE_INITIALIZING;
	pTcpDevice->DeviceType = pTcpDeviceOriginal->DeviceType;
	pTcpDevice->Characteristics = pTcpDeviceOriginal->Characteristics;


	pUdpDevice->StackSize = pUdpDeviceOriginal->StackSize + 1;
	pUdpDevice->Flags |= pUdpDeviceOriginal->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE | DO_POWER_INRUSH) & ~DO_DEVICE_INITIALIZING;
	pUdpDevice->DeviceType = pUdpDeviceOriginal->DeviceType;
	pUdpDevice->Characteristics = pUdpDeviceOriginal->Characteristics;


	pIpDevice->StackSize = pIpDeviceOriginal->StackSize + 1;
	pIpDevice->Flags |= pIpDeviceOriginal->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE | DO_POWER_INRUSH) & ~DO_DEVICE_INITIALIZING;
	pIpDevice->DeviceType = pIpDeviceOriginal->DeviceType;
	pIpDevice->Characteristics = pIpDeviceOriginal->Characteristics;


	return STATUS_SUCCESS;
}



/*
 * RemoveNetworkHooks()
 *
 * Description:
 *		Detach from all network devices.
 *
 * Parameters:
 *		pDriverObject - pointer to a driver object that represents this driver.
 *
 * Returns:
 *		Nothing.
 */

void
RemoveNetworkHooks(PDRIVER_OBJECT pDriverObject)
{
//	int	i;

	//XXX is this necessary? we detach so we should not receive any network IRPs
//	if (pDriverObject && pTcpDevice && pTcpDeviceOriginal)
//		for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
//			pDriverObject->MajorFunction[i] = pTcpDeviceOriginal->DriverObject->MajorFunction[i];


	if (pTcpDeviceOriginal != NULL)
		IoDetachDevice(pTcpDeviceOriginal);

	if (pUdpDeviceOriginal != NULL)
		IoDetachDevice(pUdpDeviceOriginal);

	if (pIpDeviceOriginal != NULL)
		IoDetachDevice(pIpDeviceOriginal);


	if (pTcpDevice != NULL)
		IoDeleteDevice(pTcpDevice);

	if (pUdpDevice != NULL)
		IoDeleteDevice(pUdpDevice);

	if (pIpDevice != NULL)
		IoDeleteDevice(pIpDevice);
}
