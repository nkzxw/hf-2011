#include "stdafx.h"


#ifdef ExAllocatePool
#undef ExAllocatePool
#endif

#define ExAllocatePool(x,y) ExAllocatePoolWithTag(x,y,'ISCS')

#define CONTROLLER_DEV_NAME_W L"\\Device\\vScsi"

#define CONTROLLER_SYM_NAME_W L"\\DosDevices\\vScsi"

#define IOCTL_MOUNT_DISK CTL_CODE(FILE_DEVICE_VIRTUAL_DISK,1,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_UNMOUNT_DISK CTL_CODE(FILE_DEVICE_VIRTUAL_DISK,2,METHOD_BUFFERED,FILE_ANY_ACCESS)

typedef struct _MOUNT_DISK_PARAM
{
	HANDLE hDataSource;
	LONGLONG lldiskSizeInBytes;
	ULONG    ulTracksPerCylinder;
	ULONG    ulMaxTransferLenInBytes;
	BOOLEAN  bRemoveable;
	BOOLEAN  bCanUnmount;
}MOUNT_DISK_PARAM,*PMOUNT_DISK_PARAM;

#pragma pack(push,1)
typedef struct _PORTABLE_CDB16 
{
	UCHAR OperationCode;
	UCHAR Reserved1:3;
	UCHAR ForceUnitAccess:1;
	UCHAR DisablePageOut:1;
	UCHAR Protection:3;
	UCHAR LogicalBlock[8];
	UCHAR TransferLength[4];
	UCHAR Reserved2;
	UCHAR Control;
}CDB16, *PCDB16;
#pragma pack(pop)

GUID GUID_BUS_TYPE_INTERNAL;



typedef enum {NotStarted=0L, Started=1L, StopPending=2L, Stopped=3L, RemovePending=4L, SurpriseRemovePending=5L, Deleted=6L} STATE, *PSTATE;

typedef struct _VSCSI_DEVICE_EXTENSION
{
	BOOLEAN bIsBus;
	LONG  eNowState;
	LONG  eOldState;
	PDEVICE_OBJECT pSelfDevObj;
	struct _VSCSI_DEVICE_EXTENSION *pNext;
	union 
	{
		struct 
		{
			PDEVICE_OBJECT pLowerDevObj;
			PDEVICE_OBJECT pPhyDevObj;
			LONG           lDiskCount;
		}Bus;
		struct 
		{
			BOOLEAN bCanUnmount;
			BOOLEAN bNeedDelete;
			ULONG ulDiskNumber;
			ULONG  ulSpecialFileCount;
			BOOLEAN bRemoveable;
			ULONG   ulMaxTransferLenInBytes;
			ULONG   ulCylinders;
			ULONG   ulTracksPerCylinder;//255
			ULONG   ulSectorsPerTrack;//63
			ULONG   ulBytesPerSector;
			LONGLONG llTotalSectorCount;
			HANDLE  hDataSource;

			HANDLE hReadWriteThread;
			KEVENT RWEvent;
			KEVENT ExitEvent;
			LIST_ENTRY RWList;
			KSPIN_LOCK RWLock;
			PDEVICE_OBJECT pBusDevObj;
		}Disk;
	};

} VSCSI_DEVICE_EXTENSION, *PVSCSI_DEVICE_EXTENSION;

void __stdcall vScsiUnload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS __stdcall vScsiCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS __stdcall vScsiAddDevice(IN PDRIVER_OBJECT  DriverObject, IN PDEVICE_OBJECT  PhysicalDeviceObject);

NTSTATUS __stdcall vScsiPnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS __stdcall vScsiPower(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS __stdcall vScsiSCSI(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS __stdcall vScsiSystemDevControl(PDEVICE_OBJECT pDevObj,PIRP pIrp);

NTSTATUS __stdcall vScsiDevCtl(PDEVICE_OBJECT pDevObj,PIRP pIrp);

PVOID MountDisk(PMOUNT_DISK_PARAM pMountParam,PVSCSI_DEVICE_EXTENSION pBusExt,PDRIVER_OBJECT pDrvObj);




#ifdef __cplusplus
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath);
#endif

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath)
{
	unsigned i=0;

	DbgPrint("Hello from vScsi!\n");

	for (;i<IRP_MJ_MAXIMUM_FUNCTION;i++)
	{
		DriverObject->MajorFunction[i]=NULL;
	}
	
	DriverObject->MajorFunction[IRP_MJ_CREATE] = vScsiCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = vScsiCreateClose;
	DriverObject->MajorFunction[IRP_MJ_PNP] = vScsiPnP;
	DriverObject->MajorFunction[IRP_MJ_POWER]=vScsiPower;
	DriverObject->MajorFunction[IRP_MJ_SCSI]=vScsiSCSI;
	DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]=vScsiSystemDevControl;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]=vScsiDevCtl;

	DriverObject->DriverUnload = vScsiUnload;
	DriverObject->DriverStartIo = NULL;
	DriverObject->DriverExtension->AddDevice = vScsiAddDevice;

	return STATUS_SUCCESS;
}

void __stdcall vScsiUnload(IN PDRIVER_OBJECT DriverObject)
{
	DbgPrint("Goodbye from vScsi!\n");
}

NTSTATUS __stdcall vScsiPower(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	PVSCSI_DEVICE_EXTENSION pExt=(PVSCSI_DEVICE_EXTENSION)pDevObj->DeviceExtension;

	PIO_STACK_LOCATION      pIoStack=IoGetCurrentIrpStackLocation(pIrp);

	NTSTATUS NtStatus=STATUS_SUCCESS;

	if (pExt->eNowState==Deleted)
	{

		if (pIoStack->MinorFunction==IRP_MJ_POWER)
		{
			PoStartNextPowerIrp(pIrp);
		}

		pIrp->IoStatus.Status=STATUS_NO_SUCH_DEVICE;

		NtStatus=STATUS_NO_SUCH_DEVICE;

		IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	}else
	{
		if (pExt->bIsBus) 
		{
			PoStartNextPowerIrp(pIrp);

			IoSkipCurrentIrpStackLocation(pIrp);

			NtStatus = PoCallDriver(pExt->Bus.pLowerDevObj, pIrp);

		} else 
		{
			PoStartNextPowerIrp(pIrp);

			pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

			IoCompleteRequest(pIrp, IO_NO_INCREMENT);

			NtStatus = STATUS_NOT_SUPPORTED;
		}
	}

	return NtStatus;
}


typedef struct _READ_WRITE_PARAM
{
	BOOLEAN bIsRead;
	LONGLONG   llStartSector;
	ULONG   ulSectorCount;
	PUCHAR  pDataBuf;
	PIRP    pIrp;
	LIST_ENTRY ListEntry;
}READ_WRITE_PARAM,*PREAD_WRITE_PARAM;

VOID ReadWriteThread(PVSCSI_DEVICE_EXTENSION pCtx)
{
	PVOID aWaitObj[]={&pCtx->Disk.ExitEvent,&pCtx->Disk.RWEvent};

	NTSTATUS  NtStatus=STATUS_SUCCESS;

	PLIST_ENTRY pListEntry=NULL;

	PREAD_WRITE_PARAM pRwParam=NULL;

	while(1)
	{
		NtStatus=KeWaitForMultipleObjects(2,aWaitObj,WaitAny,Executive,KernelMode,FALSE,NULL,NULL);

		if (NtStatus==0)
		{
			KeResetEvent(&pCtx->Disk.ExitEvent);

			while((pListEntry=ExInterlockedRemoveHeadList(&pCtx->Disk.RWList,&pCtx->Disk.RWLock)))
			{
				pRwParam=CONTAINING_RECORD(pListEntry,READ_WRITE_PARAM,ListEntry);

				pRwParam->pIrp->IoStatus.Information=0;

				pRwParam->pIrp->IoStatus.Status=STATUS_NO_SUCH_DEVICE;
				
				IoCompleteRequest(pRwParam->pIrp,IO_NO_INCREMENT);

				ExFreePoolWithTag(pRwParam,'ISCS');
			}

			break;
		}else
		{
			while((pListEntry=ExInterlockedRemoveHeadList(&pCtx->Disk.RWList,&pCtx->Disk.RWLock)))
			{
				
				LARGE_INTEGER liBytesOffset={0};

				ULONG         ulBufLen=0;

				pRwParam=CONTAINING_RECORD(pListEntry,READ_WRITE_PARAM,ListEntry);

				pRwParam->pIrp->IoStatus.Information=0;

				liBytesOffset.QuadPart=pRwParam->llStartSector*pCtx->Disk.ulBytesPerSector;

				ulBufLen=pRwParam->ulSectorCount*pCtx->Disk.ulBytesPerSector;

				pRwParam->pIrp->IoStatus.Status=STATUS_SUCCESS;

				pRwParam->pIrp->IoStatus.Information=0;

				if (pRwParam->bIsRead)
				{
					NtStatus=ZwReadFile(pCtx->Disk.hDataSource,
						NULL,
						NULL,
						NULL,
						&pRwParam->pIrp->IoStatus,
						pRwParam->pDataBuf,
						ulBufLen,
						&liBytesOffset,
						NULL);
				}else
				{
					NtStatus=ZwWriteFile(pCtx->Disk.hDataSource,
						NULL,
						NULL,
						NULL,
						&pRwParam->pIrp->IoStatus,
						pRwParam->pDataBuf,
						ulBufLen,
						&liBytesOffset,
						NULL);
				}

				IoCompleteRequest(pRwParam->pIrp,IO_NO_INCREMENT);

				ExFreePoolWithTag(pRwParam,'ISCS');
			}
		}
	}

	PsTerminateSystemThread(STATUS_SUCCESS);

	return;
}



NTSTATUS __stdcall vScsiSCSI(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	NTSTATUS NtStatus=STATUS_SUCCESS;
	PSCSI_REQUEST_BLOCK pSrb=NULL;
	PCDB pCdb=NULL;
	LONGLONG llStartSector=0;
	ULONG ulSectorCount=0, ulTemp=0;
	LONGLONG llLargeTemp=0;
	PMODE_PARAMETER_HEADER pModeParameterHeader=NULL;
	PIO_STACK_LOCATION pIoStack=IoGetCurrentIrpStackLocation(pIrp);
	PVSCSI_DEVICE_EXTENSION pDevExt=(PVSCSI_DEVICE_EXTENSION)pDevObj->DeviceExtension;

	if (pDevExt->bIsBus)
	{
		pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

		IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		return STATUS_NOT_SUPPORTED;
	}

	pSrb = pIoStack->Parameters.Scsi.Srb;

	pCdb = (PCDB)pSrb->Cdb;

	pSrb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
	pSrb->ScsiStatus = SCSISTAT_GOOD;
	pIrp->IoStatus.Information = 0;
	NtStatus = STATUS_SUCCESS;
	if (pSrb->Lun == 0) 
	{
		switch (pSrb->Function) 
		{
		case SRB_FUNCTION_EXECUTE_SCSI:
			{
				switch (pCdb->AsByte[0]) 
				{
				case SCSIOP_TEST_UNIT_READY:
					{
						pSrb->SrbStatus = SRB_STATUS_SUCCESS;
						break;
					}
				case SCSIOP_READ:
				case SCSIOP_READ16:
				case SCSIOP_WRITE:
				case SCSIOP_WRITE16:
					{
						PUCHAR pDataBuf=NULL;

						PREAD_WRITE_PARAM pParam=NULL;

						if (pCdb->AsByte[0] == SCSIOP_READ16 || pCdb->AsByte[0] == SCSIOP_WRITE16) 
						{
							REVERSE_BYTES_QUAD(&llStartSector, &(((PCDB16)pCdb)->LogicalBlock[0]));

							REVERSE_BYTES(&ulSectorCount, &(((PCDB16)pCdb)->TransferLength[0]));

						}else 
						{
							llStartSector = (pCdb->CDB10.LogicalBlockByte0 << 24) + (pCdb->CDB10.LogicalBlockByte1 << 16) + 
								(pCdb->CDB10.LogicalBlockByte2 << 8) + pCdb->CDB10.LogicalBlockByte3;
							
							ulSectorCount = (pCdb->CDB10.TransferBlocksMsb << 8) + pCdb->CDB10.TransferBlocksLsb;
						}
						if (llStartSector >= pDevExt->Disk.llTotalSectorCount) 
						{
							ulSectorCount = 0;
						}
						if ((llStartSector + ulSectorCount > pDevExt->Disk.llTotalSectorCount) && ulSectorCount != 0) 
						{
							ulSectorCount = (ULONG)(pDevExt->Disk.llTotalSectorCount - llStartSector);
						}
						if (ulSectorCount * pDevExt->Disk.ulBytesPerSector > pSrb->DataTransferLength) 
						{
							ulSectorCount = pSrb->DataTransferLength / pDevExt->Disk.ulBytesPerSector;
						}
						
						pSrb->DataTransferLength = ulSectorCount * pDevExt->Disk.ulBytesPerSector;

						pSrb->SrbStatus = SRB_STATUS_SUCCESS;

						if (ulSectorCount == 0) 
						{
							pIrp->IoStatus.Information = 0;

							break;
						}

						pDataBuf =(PUCHAR)pSrb->DataBuffer - 
								  (PUCHAR)MmGetMdlVirtualAddress(pIrp->MdlAddress)+
								  (PUCHAR)MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, HighPagePriority);

						if (!pDataBuf) 
						{
							NtStatus = STATUS_INSUFFICIENT_RESOURCES;

							pIrp->IoStatus.Information = 0;
							break;
						}

						pParam=(PREAD_WRITE_PARAM)ExAllocatePoolWithTag(NonPagedPool,sizeof(READ_WRITE_PARAM),'ISCS');

						if (!pParam)
						{
							NtStatus=STATUS_INSUFFICIENT_RESOURCES;

							break;
						}else
						{
							if (pCdb->AsByte[0] == SCSIOP_READ || pCdb->AsByte[0] == SCSIOP_READ16)
							{
								pParam->bIsRead=TRUE;
							}else
							{
								pParam->bIsRead=FALSE;
							}

							pParam->pIrp=pIrp;

							pParam->pDataBuf=pDataBuf;

							pParam->ulSectorCount=ulSectorCount;

							pParam->llStartSector=llStartSector;

							pIrp->IoStatus.Status=STATUS_PENDING;

							IoMarkIrpPending(pIrp);

							ExInterlockedInsertTailList(&pDevExt->Disk.RWList,&pParam->ListEntry,&pDevExt->Disk.RWLock);

							KeSetEvent(&pDevExt->Disk.RWEvent,IO_NO_INCREMENT,FALSE);

							NtStatus=STATUS_PENDING;

							return NtStatus;
						}
						break;
					}
				case SCSIOP_VERIFY:
				case SCSIOP_VERIFY16:
					{
						if (pCdb->AsByte[0] == SCSIOP_VERIFY16) 
						{
							REVERSE_BYTES_QUAD(&llStartSector, &(((PCDB16)pCdb)->LogicalBlock[0]));

							REVERSE_BYTES(&ulSectorCount, &(((PCDB16)pCdb)->TransferLength[0]));
						}else 
						{
							llStartSector = (pCdb->CDB10.LogicalBlockByte0 << 24) + 
								            (pCdb->CDB10.LogicalBlockByte1 << 16) + 
											(pCdb->CDB10.LogicalBlockByte2 << 8) + 
											pCdb->CDB10.LogicalBlockByte3;

							ulSectorCount = (pCdb->CDB10.TransferBlocksMsb << 8) + 
								             pCdb->CDB10.TransferBlocksLsb;
						}
						pSrb->SrbStatus = SRB_STATUS_SUCCESS;
						break;
					}
				case SCSIOP_READ_CAPACITY:
					{
						ulTemp = pDevExt->Disk.ulBytesPerSector;

						REVERSE_BYTES(&(((PREAD_CAPACITY_DATA)pSrb->DataBuffer)->BytesPerBlock), &ulTemp);

						if ((pDevExt->Disk.llTotalSectorCount - 1) > 0xffffffff) 
						{
							((PREAD_CAPACITY_DATA)pSrb->DataBuffer)->LogicalBlockAddress = -1;

						} else 
						{
							ulTemp = (ULONG)(pDevExt->Disk.llTotalSectorCount - 1);

							REVERSE_BYTES(&(((PREAD_CAPACITY_DATA)pSrb->DataBuffer)->LogicalBlockAddress), &ulTemp);
						}
						pIrp->IoStatus.Information = sizeof(READ_CAPACITY_DATA);

						pSrb->SrbStatus = SRB_STATUS_SUCCESS;

						NtStatus = STATUS_SUCCESS;

						break;
					}
				case SCSIOP_READ_CAPACITY16:
					{
						ulTemp = pDevExt->Disk.ulBytesPerSector;

						REVERSE_BYTES(&(((PREAD_CAPACITY_DATA_EX)pSrb->DataBuffer)->BytesPerBlock), &ulTemp);

						llLargeTemp = pDevExt->Disk.llTotalSectorCount - 1;

						REVERSE_BYTES_QUAD(&(((PREAD_CAPACITY_DATA_EX)pSrb->DataBuffer)->LogicalBlockAddress.QuadPart), &llLargeTemp);

						pIrp->IoStatus.Information = sizeof(READ_CAPACITY_DATA_EX);

						pSrb->SrbStatus = SRB_STATUS_SUCCESS;

						NtStatus = STATUS_SUCCESS;

						break;
					}
				case SCSIOP_MODE_SENSE:
					{
						if (pSrb->DataTransferLength < sizeof(MODE_PARAMETER_HEADER)) 
						{
							pSrb->SrbStatus = SRB_STATUS_DATA_OVERRUN;

							break;
						}
						pModeParameterHeader = (PMODE_PARAMETER_HEADER)pSrb->DataBuffer;

						RtlZeroMemory(pModeParameterHeader, pSrb->DataTransferLength);

						pModeParameterHeader->ModeDataLength = sizeof(MODE_PARAMETER_HEADER);

						pModeParameterHeader->MediumType = pDevExt->Disk.bRemoveable?RemovableMedia:FixedMedia;

						pModeParameterHeader->BlockDescriptorLength = 0;

						pSrb->DataTransferLength = sizeof(MODE_PARAMETER_HEADER);

						pIrp->IoStatus.Information = sizeof(MODE_PARAMETER_HEADER);

						pSrb->SrbStatus = SRB_STATUS_SUCCESS;

						break;
					}
				case SCSIOP_MEDIUM_REMOVAL:
					{
						pIrp->IoStatus.Information = 0;

						pSrb->SrbStatus = SRB_STATUS_SUCCESS;

						NtStatus = STATUS_SUCCESS;

						break;
					}
				default:
					{
						DbgPrint("!!Invalid SCSIOP (%02x)!!\n", pCdb->AsByte[0]);

						pSrb->SrbStatus = SRB_STATUS_ERROR;

						NtStatus = STATUS_NOT_IMPLEMENTED;
					}
				}
				break;
			}
			
		case SRB_FUNCTION_IO_CONTROL:
			{
				pSrb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
				break;
			}
		case SRB_FUNCTION_CLAIM_DEVICE:
			{
				pSrb->DataBuffer = pDevObj;
				break;
			}
		case SRB_FUNCTION_RELEASE_DEVICE:
			{
				ObDereferenceObject(pDevObj);
				break;
			}
		case SRB_FUNCTION_SHUTDOWN:
		case SRB_FUNCTION_FLUSH:
			{
				pSrb->SrbStatus = SRB_STATUS_SUCCESS;
				break;
			}
		default:
			{
				NtStatus = STATUS_NOT_IMPLEMENTED;
			}
		}
	}
	pIrp->IoStatus.Status = NtStatus;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return NtStatus;
}

NTSTATUS __stdcall vScsiCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

UNICODE_STRING  g_uniDevName={0},g_uniSymName={0};

NTSTATUS __stdcall vScsiAddDevice(IN PDRIVER_OBJECT  pDrvObj, IN PDEVICE_OBJECT  PhysicalDeviceObject)
{
	PDEVICE_OBJECT pBusDev = NULL;

	PVSCSI_DEVICE_EXTENSION pBusExt = NULL,pDiskExt=NULL;

	NTSTATUS NtStatus=STATUS_SUCCESS;
#if DBG

	MOUNT_DISK_PARAM MountParam={0};

	UNICODE_STRING   uniFileName={0};

	OBJECT_ATTRIBUTES Oa={0};

	IO_STATUS_BLOCK   IoStatus={0};

	FILE_END_OF_FILE_INFORMATION FileEndInfo={0};

#endif

	do 
	{
		RtlInitUnicodeString(&g_uniDevName,CONTROLLER_DEV_NAME_W);

		NtStatus = IoCreateDevice(pDrvObj,
			sizeof(VSCSI_DEVICE_EXTENSION),
			&g_uniDevName,
			FILE_DEVICE_CONTROLLER,
			FILE_DEVICE_SECURE_OPEN,
			FALSE,
			&pBusDev);

		if (!NT_SUCCESS(NtStatus))
		{
			break;
		}

		RtlInitUnicodeString(&g_uniSymName,CONTROLLER_SYM_NAME_W);

		NtStatus=IoCreateSymbolicLink(&g_uniSymName,&g_uniDevName);

		if (!NT_SUCCESS(NtStatus))
		{
			IoDeleteSymbolicLink(&g_uniSymName);

			NtStatus=IoCreateSymbolicLink(&g_uniSymName,&g_uniDevName);
			
			if (NtStatus)
			{
				IoDeleteDevice(pBusDev);

				pBusExt=NULL;

				break;
			}
		}

		pBusDev->Flags |=(DO_DIRECT_IO | DO_POWER_INRUSH);

		pBusExt = (PVSCSI_DEVICE_EXTENSION)pBusDev->DeviceExtension;

		pBusExt->bIsBus=TRUE;

		pBusExt->eNowState=pBusExt->eOldState=NotStarted;

		pBusExt->Bus.pPhyDevObj = PhysicalDeviceObject;

		pBusExt->pNext=NULL;

		pBusExt->pSelfDevObj=pBusDev;



		pBusExt->Bus.lDiskCount=0;

		pBusExt->Bus.pLowerDevObj = IoAttachDeviceToDeviceStack(pBusDev, PhysicalDeviceObject);

		if (!pBusExt->Bus.pLowerDevObj)
		{
			IoDeleteDevice(pBusDev);

			IoDeleteSymbolicLink(&g_uniSymName);

			NtStatus=STATUS_UNSUCCESSFUL;

			pBusExt=NULL;

			break;
		}

		pBusDev->Flags &= ~DO_DEVICE_INITIALIZING;

#if DBG

		RtlInitUnicodeString(&uniFileName,L"\\??\\C:\\Disk.Img");

		InitializeObjectAttributes(&Oa,&uniFileName,OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,NULL,NULL);

		NtStatus=ZwCreateFile(&MountParam.hDataSource,
			     GENERIC_READ | GENERIC_WRITE,
				 &Oa,
			     &IoStatus,
				 NULL,
				 0,
				 0,
				 FILE_OPEN_IF,
				 FILE_NON_DIRECTORY_FILE | 
				 FILE_RANDOM_ACCESS |
				 FILE_WRITE_THROUGH |
				 FILE_SYNCHRONOUS_IO_NONALERT,
				 NULL,
				 0);

		if (!NT_SUCCESS(NtStatus))
		{
			KdPrint(("Create file Error:0x%X",NtStatus));

			IoDeleteDevice(pBusDev);

			IoDeleteSymbolicLink(&g_uniSymName);
			
			NtStatus=STATUS_SUCCESS;
			
			break;
		}

		FileEndInfo.EndOfFile.QuadPart=1024*1024*100;

		NtStatus=ZwSetInformationFile(MountParam.hDataSource,&IoStatus,&FileEndInfo,sizeof(FILE_END_OF_FILE_INFORMATION),FileEndOfFileInformation);

		if (!NT_SUCCESS(NtStatus))
		{
			KdPrint(("Create file Error:0x%X",NtStatus));

			IoDeleteDevice(pBusDev);

			IoDeleteSymbolicLink(&g_uniSymName);
			
			ZwClose(MountParam.hDataSource);

			NtStatus=STATUS_SUCCESS;

		    break;
		}

		MountParam.bRemoveable=FALSE;

		MountParam.bCanUnmount=FALSE;

		MountParam.lldiskSizeInBytes=FileEndInfo.EndOfFile.QuadPart;

		MountParam.ulMaxTransferLenInBytes=(ULONG)-1;

		MountParam.ulTracksPerCylinder=255;

		pDiskExt=(PVSCSI_DEVICE_EXTENSION)MountDisk(&MountParam,pBusExt,pBusDev->DriverObject);

		if (!pDiskExt)
		{
			KdPrint(("Mount Disk Error!\n"));
		}else
		{
			pDiskExt->Disk.pBusDevObj=pBusDev;
			
			KdPrint(("Mount Disk Ok!\n"));
		}
#endif

	} while (FALSE);

	return NtStatus;
}


NTSTATUS __stdcall IrpCompletion(
					  IN PDEVICE_OBJECT pDevObj,
					  IN PIRP pIrp,
					  IN PVOID pCtx
					  )
{
	PKEVENT pEvent = (PKEVENT) pCtx;

	UNREFERENCED_PARAMETER(pDevObj);
	UNREFERENCED_PARAMETER(pIrp);

	if (pEvent)
	{
		KeSetEvent(pEvent, IO_NO_INCREMENT, FALSE);
	}

	return(STATUS_MORE_PROCESSING_REQUIRED);
}

NTSTATUS __stdcall ForwardIrpSynchronous(
							  IN PDEVICE_OBJECT pDevObj,
							  IN PIRP pIrp
							  )
{
	PVSCSI_DEVICE_EXTENSION   pExt=NULL;

	KEVENT                    Event={0};

	NTSTATUS                  NtStatus=STATUS_SUCCESS;

	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	pExt = (PVSCSI_DEVICE_EXTENSION) pDevObj->DeviceExtension;

	IoCopyCurrentIrpStackLocationToNext(pIrp);

	IoSetCompletionRoutine(pIrp, IrpCompletion, &Event, TRUE, TRUE, TRUE);

	NtStatus = IoCallDriver(pDevObj, pIrp);

	if (NtStatus == STATUS_PENDING) 
	{
		KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

		NtStatus = pIrp->IoStatus.Status;
	}
	return NtStatus;
}

NTSTATUS BusPnp(IN PDEVICE_OBJECT pBusDevObj, IN PIRP pIrp, IN PIO_STACK_LOCATION pIoStack, IN PVSCSI_DEVICE_EXTENSION pBusExt)
{
	NTSTATUS                NtStatus=STATUS_SUCCESS;

	KEVENT                  Event={0};

	PDEVICE_RELATIONS       pDevRelations=NULL;

	PDEVICE_OBJECT          pDiskDevObj=NULL;

	ULONG                   ulTmpIndex=0;

	PVSCSI_DEVICE_EXTENSION pTmpDiskExt=NULL,pPreDiskExt=NULL;

	switch (pIoStack->MinorFunction) 
	{
	case IRP_MN_START_DEVICE:
		{
			NtStatus=ForwardIrpSynchronous(pBusExt->Bus.pLowerDevObj, pIrp);

			if (NT_SUCCESS(NtStatus)) 
			{
				pBusExt->eOldState=InterlockedExchange(&pBusExt->eNowState,(LONG)Started);
			}

			IoCompleteRequest(pIrp, IO_NO_INCREMENT);

			return NtStatus;
		}
	case IRP_MN_REMOVE_DEVICE:
		{
			pBusExt->eOldState=(LONG)InterlockedExchange((PLONG)&pBusExt->eNowState,(LONG)Deleted);

			pTmpDiskExt=pBusExt->pNext;

			pBusExt->pNext=NULL;

			while(pTmpDiskExt)
			{
				if (pTmpDiskExt->bIsBus)
				{
					pTmpDiskExt=pTmpDiskExt->pNext;
					
					continue;
				}

				pDiskDevObj=pTmpDiskExt->pSelfDevObj;

				KeSetEvent(&pTmpDiskExt->Disk.ExitEvent,IO_NO_INCREMENT,FALSE);

				if (pTmpDiskExt->Disk.hReadWriteThread)
				{
					ZwClose(pTmpDiskExt->Disk.hReadWriteThread);
				}

				if (pTmpDiskExt->Disk.hDataSource && pTmpDiskExt->Disk.hDataSource!=((HANDLE)-1))
				{
					ZwClose(pTmpDiskExt->Disk.hDataSource);
				}

				pTmpDiskExt=pTmpDiskExt->pNext;

				if (pDiskDevObj)
				{
					IoDeleteDevice(pDiskDevObj);
				}
			}

			pIrp->IoStatus.Information = 0;

			pIrp->IoStatus.Status = STATUS_SUCCESS;

			IoSkipCurrentIrpStackLocation(pIrp);

			NtStatus=IoCallDriver(pBusExt->Bus.pLowerDevObj,pIrp);

			if (NT_SUCCESS(NtStatus) || NtStatus==STATUS_PENDING)
			{
				IoDetachDevice(pBusExt->Bus.pLowerDevObj);

				IoDeleteDevice(pBusDevObj);

				IoDeleteSymbolicLink(&g_uniSymName);
			}

			return NtStatus;
		}
	case IRP_MN_QUERY_DEVICE_RELATIONS:
		{
			if (pIoStack->Parameters.QueryDeviceRelations.Type != BusRelations || pIrp->IoStatus.Information) 
			{
				NtStatus=pIrp->IoStatus.Status;
				
				break;
			}

			pDevRelations = (PDEVICE_RELATIONS)ExAllocatePool(NonPagedPool, sizeof(DEVICE_RELATIONS) + (sizeof(PDEVICE_OBJECT) * pBusExt->Bus.lDiskCount));

			if (!pDevRelations) 
			{
				pIrp->IoStatus.Information = 0;

				NtStatus = STATUS_INSUFFICIENT_RESOURCES;

				break;
			}

			pDevRelations->Count = (ULONG)pBusExt->Bus.lDiskCount;

			pTmpDiskExt=pBusExt->pNext;

			while(pTmpDiskExt)
			{

				if (pTmpDiskExt->bIsBus)
				{
					pTmpDiskExt=pTmpDiskExt->pNext;

					continue;
				}

				pBusDevObj=pTmpDiskExt->pSelfDevObj;

				pDevRelations->Objects[ulTmpIndex]=pBusDevObj;

				ulTmpIndex++;

				pTmpDiskExt=pTmpDiskExt->pNext;

			}

			ObReferenceObject(pBusDevObj);

			pIrp->IoStatus.Information = (ULONG_PTR)pDevRelations;

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_QUERY_PNP_DEVICE_STATE:
		{
			pIrp->IoStatus.Information = 0;

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_QUERY_STOP_DEVICE:
		{
			pBusExt->eOldState=InterlockedExchange(&pBusExt->eNowState,StopPending);

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_CANCEL_STOP_DEVICE:
		{
			pBusExt->eOldState=InterlockedExchange(&pBusExt->eNowState,pBusExt->eOldState);

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_STOP_DEVICE:
		{
			pBusExt->eOldState=InterlockedExchange(&pBusExt->eNowState,Stopped);

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_QUERY_REMOVE_DEVICE:
		{
			pBusExt->eOldState=InterlockedExchange(&pBusExt->eNowState,RemovePending);

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_CANCEL_REMOVE_DEVICE:
		{
			pBusExt->eOldState=InterlockedExchange(&pBusExt->eNowState,pBusExt->eOldState);

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_SURPRISE_REMOVAL:
		{
			pBusExt->eOldState=InterlockedExchange(&pBusExt->eNowState,SurpriseRemovePending);

			NtStatus = STATUS_SUCCESS;

			break;
		}
	default:
		{

			NtStatus=pIrp->IoStatus.Status;

			break;
		}
	}

	pIrp->IoStatus.Status = NtStatus;

	IoSkipCurrentIrpStackLocation(pIrp);

	NtStatus = IoCallDriver(pBusExt->Bus.pLowerDevObj, pIrp);

	return NtStatus;
}


NTSTATUS  GetDeviceCapabilities(IN PDEVICE_OBJECT pDevObj, IN PDEVICE_CAPABILITIES pDevCapabilities) 
{
	IO_STATUS_BLOCK IoStatus={0};

	KEVENT Event={0};

	NTSTATUS NtStatus=STATUS_SUCCESS;

	PDEVICE_OBJECT pDevToSndIrp=NULL;

	PIO_STACK_LOCATION pIoStack=NULL;

	PIRP pIrpToSend=NULL;

	RtlZeroMemory(pDevCapabilities, sizeof(DEVICE_CAPABILITIES));

	pDevCapabilities->Size = sizeof(DEVICE_CAPABILITIES);

	pDevCapabilities->Version = 1;

	pDevCapabilities->Address = -1;

	pDevCapabilities->UINumber = -1;

	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	pDevToSndIrp = IoGetAttachedDeviceReference(pDevObj);

	pIrpToSend = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, pDevToSndIrp, NULL, 0, NULL, &Event, &IoStatus);

	if (pIrpToSend == NULL) 
	{
		NtStatus = STATUS_INSUFFICIENT_RESOURCES;

	} else 
	{
		pIrpToSend->IoStatus.Status = STATUS_NOT_SUPPORTED;

		pIoStack = IoGetNextIrpStackLocation(pIrpToSend);

		RtlZeroMemory(pIoStack, sizeof(IO_STACK_LOCATION));

		pIoStack->MajorFunction = IRP_MJ_PNP;

		pIoStack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;

		pIoStack->Parameters.DeviceCapabilities.Capabilities = pDevCapabilities;

		NtStatus = IoCallDriver(pDevToSndIrp, pIrpToSend);

		if (NtStatus == STATUS_PENDING) 
		{
			KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

			NtStatus = IoStatus.Status;
		}
	}

	ObDereferenceObject(pDevToSndIrp);

	return NtStatus;
}

NTSTATUS DiskPnp(IN PDEVICE_OBJECT pDiskDev, IN PIRP pIrp, IN PIO_STACK_LOCATION pIoStack, IN PVSCSI_DEVICE_EXTENSION pDiskExt) 
{
	NTSTATUS             NtStatus;
	PDEVICE_RELATIONS    pDevRelations=NULL;
	PPNP_BUS_INFORMATION pPnPBusInformation=NULL;
	PDEVICE_CAPABILITIES pDevCapabilities=NULL;
	DEVICE_CAPABILITIES  BusDevCapabilities={0};
	WCHAR                pwsBuf[512]={0};
	ULONG                ulStrLen=0;

	switch (pIoStack->MinorFunction) 
	{
	case IRP_MN_QUERY_ID:
		{
			switch (pIoStack->Parameters.QueryId.IdType) 
			{
			case BusQueryDeviceID:
				{
					ulStrLen = swprintf(pwsBuf, L"vScsi\\Disk%lu", pDiskExt->Disk.ulDiskNumber) + 1;

					pIrp->IoStatus.Information = (ULONG_PTR)ExAllocatePool(PagedPool, ulStrLen * sizeof(WCHAR));

					if (!pIrp->IoStatus.Information) 
					{
						NtStatus = STATUS_INSUFFICIENT_RESOURCES;

						break;
					}

					RtlCopyMemory((PWCHAR)pIrp->IoStatus.Information, pwsBuf, ulStrLen * sizeof(WCHAR));

					NtStatus = STATUS_SUCCESS;
				}

				break;

			case BusQueryInstanceID:
				{
					ulStrLen = swprintf(pwsBuf, L"vScsi%lu", pDiskExt->Disk.ulDiskNumber) + 1;

					pIrp->IoStatus.Information = (ULONG_PTR)ExAllocatePool(PagedPool, ulStrLen * sizeof(WCHAR));

					if (!pIrp->IoStatus.Information) 
					{
						NtStatus = STATUS_INSUFFICIENT_RESOURCES;

						break;
					}
					RtlCopyMemory((PWCHAR)pIrp->IoStatus.Information, pwsBuf, ulStrLen * sizeof(WCHAR));

					NtStatus = STATUS_SUCCESS;
				}

				break;
			case BusQueryHardwareIDs:
				{
					ulStrLen = swprintf(pwsBuf, L"vScsi\\Disk%lu", pDiskExt->Disk.ulDiskNumber) + 1;

					ulStrLen += swprintf(&pwsBuf[ulStrLen], L"GenDisk") + 4;

					if ((pIrp->IoStatus.Information = (ULONG_PTR)ExAllocatePool(PagedPool, ulStrLen * sizeof(WCHAR))) == 0)
					{
						NtStatus = STATUS_INSUFFICIENT_RESOURCES;

						break;
					}
					RtlCopyMemory((PWCHAR)pIrp->IoStatus.Information, pwsBuf, ulStrLen * sizeof(WCHAR));

					NtStatus = STATUS_SUCCESS;
				}

				break;
			case BusQueryCompatibleIDs:
				{
					ulStrLen = swprintf(pwsBuf, L"GenDisk") + 4;

					pIrp->IoStatus.Information = (ULONG_PTR)ExAllocatePool(PagedPool, ulStrLen * sizeof(WCHAR));

					if (!pIrp->IoStatus.Information) 
					{
						NtStatus = STATUS_INSUFFICIENT_RESOURCES;

						break;
					}

					RtlCopyMemory((PWCHAR)pIrp->IoStatus.Information, pwsBuf, ulStrLen * sizeof(WCHAR));

					NtStatus = STATUS_SUCCESS;
				}
				
				break;
			default:
				{
					pIrp->IoStatus.Information = 0;

					NtStatus = STATUS_NOT_SUPPORTED;
				}
			}

			break;
		}

	case IRP_MN_QUERY_DEVICE_TEXT:
		{
			switch (pIoStack->Parameters.QueryDeviceText.DeviceTextType ) 
			{
			case DeviceTextDescription:
				{
					ulStrLen = swprintf(pwsBuf, L"vScsi Disk") + 1;

					if ((pIrp->IoStatus.Information = (ULONG_PTR)ExAllocatePool(PagedPool, ulStrLen * sizeof(WCHAR))) == 0) 
					{
						NtStatus = STATUS_INSUFFICIENT_RESOURCES;
						break;
					}

					RtlCopyMemory((PWCHAR)pIrp->IoStatus.Information, pwsBuf, ulStrLen * sizeof(WCHAR));

					NtStatus = STATUS_SUCCESS;
				}
				break;
			case DeviceTextLocationInformation:
				{
					ulStrLen = swprintf(pwsBuf, L"vScsi Root Bus") + 1;

					pIrp->IoStatus.Information = (ULONG_PTR)ExAllocatePool(PagedPool, ulStrLen * sizeof(WCHAR));

					if (!pIrp->IoStatus.Information) 
					{
						NtStatus = STATUS_INSUFFICIENT_RESOURCES;

						break;
					}

					RtlCopyMemory((PWCHAR)pIrp->IoStatus.Information, pwsBuf, ulStrLen * sizeof(WCHAR));

					NtStatus = STATUS_SUCCESS;
				}
				break;
			default:
				{
					pIrp->IoStatus.Information = 0;

					pIrp->IoStatus.Status=0;

					NtStatus = STATUS_NOT_SUPPORTED;
				}
			}

			break;
		}

	case IRP_MN_QUERY_DEVICE_RELATIONS:
		{
			if (pIoStack->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation) 
			{
				NtStatus = pIrp->IoStatus.Status;

				break;
			}
			pDevRelations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS) + sizeof (PDEVICE_OBJECT));

			if (!pDevRelations) 
			{
				NtStatus = STATUS_INSUFFICIENT_RESOURCES;

				break;
			}
			pDevRelations->Objects[0] = pDiskDev;

			pDevRelations->Count = 1;

			ObReferenceObject(pDiskDev);

			pIrp->IoStatus.Information = (ULONG_PTR)pDevRelations;

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_QUERY_BUS_INFORMATION:
		{
			pPnPBusInformation = (PPNP_BUS_INFORMATION)ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));

			if (!pPnPBusInformation)
			{
				NtStatus = STATUS_INSUFFICIENT_RESOURCES;

				break;
			}
			pPnPBusInformation->BusTypeGuid = GUID_BUS_TYPE_INTERNAL;

			pPnPBusInformation->LegacyBusType = PNPBus;

			pPnPBusInformation->BusNumber = 0;

			pIrp->IoStatus.Information = (ULONG_PTR)pPnPBusInformation;

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_QUERY_CAPABILITIES:
		{
			pDevCapabilities = pIoStack->Parameters.DeviceCapabilities.Capabilities;

			if (pDevCapabilities->Version != 1 || pDevCapabilities->Size < sizeof(DEVICE_CAPABILITIES)) 
			{
				NtStatus = STATUS_UNSUCCESSFUL;

				break;
			}

			NtStatus = GetDeviceCapabilities(((PVSCSI_DEVICE_EXTENSION)pDiskExt->Disk.pBusDevObj->DeviceExtension)->Bus.pLowerDevObj, &BusDevCapabilities);

			if (!NT_SUCCESS(NtStatus)) 
			{
				break;
			}

			RtlCopyMemory(pDevCapabilities->DeviceState, BusDevCapabilities.DeviceState, (PowerSystemShutdown + 1) * sizeof(DEVICE_POWER_STATE));

			pDevCapabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;

			if (pDevCapabilities->DeviceState[PowerSystemSleeping1] != PowerDeviceD0)
			{
				pDevCapabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD1;
			}

			if (pDevCapabilities->DeviceState[PowerSystemSleeping2] != PowerDeviceD0)
			{
				pDevCapabilities->DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
			}

			pDevCapabilities->DeviceWake = PowerDeviceD1;

			pDevCapabilities->DeviceD1 = TRUE;

			pDevCapabilities->DeviceD2 = FALSE;

			pDevCapabilities->WakeFromD0 = FALSE;

			pDevCapabilities->WakeFromD1 = FALSE;

			pDevCapabilities->WakeFromD2 = FALSE;

			pDevCapabilities->WakeFromD3 = FALSE;

			pDevCapabilities->D1Latency = 0;

			pDevCapabilities->D2Latency = 0;

			pDevCapabilities->D3Latency = 0;

			pDevCapabilities->EjectSupported = FALSE;

			pDevCapabilities->HardwareDisabled = FALSE;

			pDevCapabilities->Removable = pDiskExt->Disk.bRemoveable;

			pDevCapabilities->SurpriseRemovalOK = FALSE;

			pDevCapabilities->UniqueID = FALSE;

			pDevCapabilities->SilentInstall = FALSE;

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_DEVICE_USAGE_NOTIFICATION:
		{
			if (pIoStack->Parameters.UsageNotification.InPath) 
			{
				pDiskExt->Disk.ulSpecialFileCount++;
			} else 
			{
				pDiskExt->Disk.ulSpecialFileCount--;
			}

			pIrp->IoStatus.Information = 0;

			NtStatus = STATUS_SUCCESS;
			break;
		}
	case IRP_MN_QUERY_PNP_DEVICE_STATE:
		{
			pIrp->IoStatus.Information = 0;

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_START_DEVICE:
		{
			pDiskExt->eOldState=InterlockedExchange(&pDiskExt->eNowState,Started);

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_QUERY_STOP_DEVICE:
		{
			pDiskExt->eOldState=InterlockedExchange(&pDiskExt->eNowState,StopPending);

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_CANCEL_STOP_DEVICE:
		{
			pDiskExt->eOldState=InterlockedExchange(&pDiskExt->eNowState,pDiskExt->eOldState);

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_STOP_DEVICE:
		{
			pDiskExt->eOldState=InterlockedExchange(&pDiskExt->eNowState,Stopped);

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_QUERY_REMOVE_DEVICE:
		{
			pDiskExt->eOldState=InterlockedExchange(&pDiskExt->eNowState,RemovePending);

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_REMOVE_DEVICE:
		{
			pDiskExt->eOldState=InterlockedExchange(&pDiskExt->eNowState,NotStarted);

			if (pDiskExt->Disk.bNeedDelete)
			{
				PVSCSI_DEVICE_EXTENSION pBusExt=(PVSCSI_DEVICE_EXTENSION)pDiskExt->Disk.pBusDevObj->DeviceExtension;

				InterlockedDecrement(&pBusExt->Bus.lDiskCount);

				if (pDiskExt==pBusExt)
				{
					pBusExt->pNext=pDiskExt->pNext;
				}else
				{
					PVSCSI_DEVICE_EXTENSION pTmpDiskExt=pBusExt->pNext,pPreTmpDiskExt=NULL;

					while(pTmpDiskExt)
					{
						if (pTmpDiskExt==pDiskExt)
						{
							break;
						}

						pPreTmpDiskExt=pTmpDiskExt;

						pTmpDiskExt=pTmpDiskExt->pNext;
					}

					if (pTmpDiskExt)
					{
						pPreTmpDiskExt->pNext=pDiskExt->pNext;
					}
				}

				KeSetEvent(&pDiskExt->Disk.ExitEvent,IO_NO_INCREMENT,FALSE);

				if (pDiskExt->Disk.hReadWriteThread)
				{
					ZwClose(pDiskExt->Disk.hReadWriteThread);
				}

				if (pDiskExt->Disk.hDataSource && pDiskExt->Disk.hDataSource!=(HANDLE)-1)
				{
					ZwClose(pDiskExt->Disk.hDataSource);

					pDiskExt->Disk.hDataSource=NULL;
				}

				IoDeleteDevice(pDiskDev);

				NtStatus = STATUS_NO_SUCH_DEVICE;

			} else 
			{
				NtStatus = STATUS_SUCCESS;
			}

			break;
		}
	case IRP_MN_CANCEL_REMOVE_DEVICE:
		{
			pDiskExt->eOldState=InterlockedExchange(&pDiskExt->eNowState,pDiskExt->eOldState);

			NtStatus = STATUS_SUCCESS;

			break;
		}
	case IRP_MN_SURPRISE_REMOVAL:
		{
			pDiskExt->eOldState=InterlockedExchange(&pDiskExt->eNowState,SurpriseRemovePending);

			NtStatus = STATUS_SUCCESS;

			break;
		}
	default:
		NtStatus = pIrp->IoStatus.Status;
	}

	pIrp->IoStatus.Status = NtStatus;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return NtStatus;
}


NTSTATUS __stdcall vScsiPnP(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	PIO_STACK_LOCATION pIoStack = IoGetCurrentIrpStackLocation(pIrp);

	PVSCSI_DEVICE_EXTENSION pExt = ((PVSCSI_DEVICE_EXTENSION)pDevObj->DeviceExtension);

	NTSTATUS NtStatus=STATUS_SUCCESS;

	ASSERT(pExt);

	do 
	{
		if (pExt->bIsBus)
		{
			NtStatus=BusPnp(pDevObj,pIrp,pIoStack,pExt);
		}else
		{
			NtStatus=DiskPnp(pDevObj,pIrp,pIoStack,pExt);
		}
	} while (FALSE);

	return NtStatus;
}

NTSTATUS __stdcall vScsiSystemDevControl(PDEVICE_OBJECT pDevObj,PIRP pIrp)
{
	PVSCSI_DEVICE_EXTENSION pExt=(PVSCSI_DEVICE_EXTENSION)pDevObj->DeviceExtension;

	NTSTATUS NtStatus=STATUS_SUCCESS;

	if (pExt->bIsBus)
	{
		IoSkipCurrentIrpStackLocation(pIrp);

		NtStatus=IoCallDriver(pExt->Bus.pLowerDevObj, pIrp);
	}else
	{
		pIrp->IoStatus.Status=NtStatus;

		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	}

	return NtStatus;
}

PVOID MountDisk(PMOUNT_DISK_PARAM pMountParam,PVSCSI_DEVICE_EXTENSION pBusExt,PDRIVER_OBJECT pDrvObj)
{
	PDEVICE_OBJECT    pDiskDevObj=NULL;

	PVSCSI_DEVICE_EXTENSION pDiskExt=NULL;

	OBJECT_ATTRIBUTES Oa={0};

	PVOID            pObj=NULL;

	NTSTATUS         NtStatus=STATUS_SUCCESS;

	do 
	{
		NtStatus=IoCreateDevice(pDrvObj,
			sizeof(VSCSI_DEVICE_EXTENSION),
			NULL,
			FILE_DEVICE_DISK,
			FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN,
			FALSE,
			&pDiskDevObj);

		if (!NT_SUCCESS(NtStatus))
		{
			break;
		}

		pDiskExt=(PVSCSI_DEVICE_EXTENSION)pDiskDevObj->DeviceExtension;

		RtlZeroMemory(pDiskExt, sizeof(VSCSI_DEVICE_EXTENSION));

		pDiskExt->bIsBus = FALSE;

		pDiskExt->eNowState=pBusExt->eOldState=NotStarted;

		pDiskExt->pSelfDevObj=pDiskDevObj;

		pDiskExt->Disk.bNeedDelete=FALSE;

		InitializeListHead(&pDiskExt->Disk.RWList);

		KeInitializeSpinLock(&pDiskExt->Disk.RWLock);

		KeInitializeEvent(&pDiskExt->Disk.RWEvent,SynchronizationEvent,FALSE);

		KeInitializeEvent(&pDiskExt->Disk.ExitEvent,NotificationEvent,FALSE);

		if (ExGetPreviousMode()==KernelMode)
		{
			pDiskExt->Disk.hDataSource=pMountParam->hDataSource;
		}else
		{
			NtStatus=ObReferenceObjectByHandle(pMountParam->hDataSource,GENERIC_READ | GENERIC_WRITE,NULL,KernelMode,&pObj,NULL);

			if (!pObj)
			{
				IoDeleteDevice(pDiskDevObj);

				pDiskExt=NULL;

				break;;
			}

			NtStatus=ObOpenObjectByPointer(pObj,OBJ_KERNEL_HANDLE,NULL,GENERIC_WRITE | GENERIC_WRITE,NULL,KernelMode,&pDiskExt->Disk.hDataSource);

			ObDereferenceObject(pObj);

			if (!pDiskExt->Disk.hDataSource)
			{
				IoDeleteDevice(pDiskDevObj);

				pDiskExt=NULL;

				break;
			}
		}

		pDiskExt->Disk.ulMaxTransferLenInBytes=pMountParam->ulMaxTransferLenInBytes-(pMountParam->ulMaxTransferLenInBytes % 512);

		pDiskExt->Disk.bRemoveable=pMountParam->bRemoveable;

		pDiskExt->Disk.ulBytesPerSector=512;

		pDiskExt->Disk.ulSectorsPerTrack=63;

		pDiskExt->Disk.ulTracksPerCylinder=pMountParam->ulTracksPerCylinder>0?pMountParam->ulTracksPerCylinder:255;

		pDiskExt->Disk.llTotalSectorCount=pMountParam->lldiskSizeInBytes/512;

		pDiskExt->Disk.ulCylinders=(ULONG)(pDiskExt->Disk.llTotalSectorCount/(pDiskExt->Disk.ulTracksPerCylinder*pDiskExt->Disk.ulSectorsPerTrack));

		pDiskExt->Disk.bCanUnmount=pMountParam->bCanUnmount;

		if (ExGetPreviousMode()==KernelMode)
		{
			NtStatus=PsCreateSystemThread(&pDiskExt->Disk.hReadWriteThread,THREAD_ALL_ACCESS,NULL,NULL,NULL,(PKSTART_ROUTINE)ReadWriteThread,pDiskExt);
		}else
		{
			InitializeObjectAttributes(&Oa,NULL,OBJ_KERNEL_HANDLE,NULL,NULL);

			NtStatus=PsCreateSystemThread(&pDiskExt->Disk.hReadWriteThread,THREAD_ALL_ACCESS,&Oa,NULL,NULL,(PKSTART_ROUTINE)ReadWriteThread,pDiskExt);
		}

		if (!pDiskExt->Disk.hReadWriteThread)
		{
			if (ExGetPreviousMode()!=KernelMode)
			{
				ZwClose(pDiskExt->Disk.hDataSource);
			}

			IoDeleteDevice(pDiskDevObj);

			pDiskExt=NULL;

			break;
		}

		pDiskDevObj->Flags |= (DO_DIRECT_IO | DO_POWER_INRUSH | DO_BUS_ENUMERATED_DEVICE);  

		pDiskExt->pNext=pBusExt->pNext;

		pBusExt->pNext=pDiskExt;

		InterlockedIncrement(&pBusExt->Bus.lDiskCount);

		InterlockedExchange((PLONG)&pDiskExt->Disk.ulDiskNumber,((pBusExt->Bus.lDiskCount-1)>=0?(pBusExt->Bus.lDiskCount-1):0));

		pDiskDevObj->Flags &= ~DO_DEVICE_INITIALIZING;

		IoInvalidateDeviceRelations(pBusExt->Bus.pPhyDevObj, BusRelations);
	} while (FALSE);

	return pDiskExt;

}


NTSTATUS __stdcall vScsiDevCtl(PDEVICE_OBJECT pDevObj,PIRP pIrp)
{
	PVSCSI_DEVICE_EXTENSION pExt=(PVSCSI_DEVICE_EXTENSION)pDevObj->DeviceExtension;

	PIO_STACK_LOCATION pIoStack=IoGetCurrentIrpStackLocation(pIrp);

	NTSTATUS NtStatus=STATUS_SUCCESS;

	if (pExt->bIsBus)
	{
		PVSCSI_DEVICE_EXTENSION pBusExt=pExt;

		switch (pIoStack->Parameters.DeviceIoControl.IoControlCode) 
		{
		case IOCTL_MOUNT_DISK:
			{
				PMOUNT_DISK_PARAM pMountParam=(PMOUNT_DISK_PARAM)pIrp->AssociatedIrp.SystemBuffer;

				PVSCSI_DEVICE_EXTENSION pDiskExt=NULL;

				if (!pMountParam || 
					pIoStack->Parameters.DeviceIoControl.InputBufferLength<sizeof(MOUNT_DISK_PARAM) ||
					pIoStack->Parameters.DeviceIoControl.OutputBufferLength<sizeof(PVOID))
				{
					NtStatus=STATUS_INVALID_PARAMETER;
					break;
				}
				
				pDiskExt=(PVSCSI_DEVICE_EXTENSION)MountDisk(pMountParam,pBusExt,pDevObj->DriverObject);

				if (!pDiskExt)
				{
					NtStatus=STATUS_UNSUCCESSFUL;

					pIrp->IoStatus.Information=0;
				}else
				{
					*((PVOID*)pIrp->AssociatedIrp.SystemBuffer)=(PVOID)pDiskExt;

					pIrp->IoStatus.Information=sizeof(PVOID);

					pDiskExt->Disk.pBusDevObj=pDevObj;
				}

				break;
			}
		case IOCTL_UNMOUNT_DISK:
			{
				PVSCSI_DEVICE_EXTENSION pDiskDevExt=(PVSCSI_DEVICE_EXTENSION)pIrp->AssociatedIrp.SystemBuffer;

				if (!pDiskDevExt || pIoStack->Parameters.DeviceIoControl.InputBufferLength<sizeof(PVOID))
				{
					NtStatus=STATUS_INVALID_PARAMETER;

					break;
				}

				if (pDiskDevExt->Disk.bCanUnmount)
				{
					NtStatus=STATUS_ACCESS_DENIED;

					break;
				}

				pDiskDevExt->Disk.bNeedDelete=TRUE;

				IoInvalidateDeviceRelations(pBusExt->Bus.pPhyDevObj,BusRelations);

				break;
			}
		default:
			{
				NtStatus=STATUS_NOT_SUPPORTED;

				break;
			}
		}

		pIrp->IoStatus.Status = NtStatus;

		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	}else
	{
		ULONG ulCopySize;
		PSTORAGE_PROPERTY_QUERY pStoragePropertyQuery;
		STORAGE_ADAPTER_DESCRIPTOR StorageAdapterDescriptor;
		STORAGE_DEVICE_DESCRIPTOR StorageDeviceDescriptor;
		DISK_GEOMETRY DiskGeometry;
		SCSI_ADDRESS ScsiAdress;
		PVSCSI_DEVICE_EXTENSION pDiskExt=pExt;

		switch (pIoStack->Parameters.DeviceIoControl.IoControlCode) 
		{
		case IOCTL_STORAGE_QUERY_PROPERTY:
			{
				NtStatus = STATUS_INVALID_PARAMETER;

				pStoragePropertyQuery = (PSTORAGE_PROPERTY_QUERY)pIrp->AssociatedIrp.SystemBuffer;

				if (pStoragePropertyQuery->PropertyId == StorageAdapterProperty && 
					pStoragePropertyQuery->QueryType == PropertyStandardQuery) 
				{
					ulCopySize = 
						(pIoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_ADAPTER_DESCRIPTOR)?
						pIoStack->Parameters.DeviceIoControl.OutputBufferLength:sizeof(STORAGE_ADAPTER_DESCRIPTOR));

					StorageAdapterDescriptor.Version = sizeof(STORAGE_ADAPTER_DESCRIPTOR);

					StorageAdapterDescriptor.Size = sizeof(STORAGE_ADAPTER_DESCRIPTOR);

					StorageAdapterDescriptor.MaximumTransferLength = pDiskExt->Disk.ulMaxTransferLenInBytes;

					StorageAdapterDescriptor.MaximumPhysicalPages = (ULONG)-1;

					StorageAdapterDescriptor.AlignmentMask = 0;

					StorageAdapterDescriptor.AdapterUsesPio = TRUE;

					StorageAdapterDescriptor.AdapterScansDown = FALSE;

					StorageAdapterDescriptor.CommandQueueing = FALSE;

					StorageAdapterDescriptor.AcceleratedTransfer = FALSE;

					StorageAdapterDescriptor.BusType = BusTypeiScsi;

					RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, &StorageAdapterDescriptor, ulCopySize);

					pIrp->IoStatus.Information = (ULONG_PTR)ulCopySize;

					NtStatus = STATUS_SUCCESS;
				}

				if (pStoragePropertyQuery->PropertyId == StorageDeviceProperty && 
					pStoragePropertyQuery->QueryType == PropertyStandardQuery) 
				{
					ulCopySize = 
						(pIoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_DEVICE_DESCRIPTOR)?
						pIoStack->Parameters.DeviceIoControl.OutputBufferLength:sizeof(STORAGE_DEVICE_DESCRIPTOR));

					StorageDeviceDescriptor.Version = sizeof(STORAGE_DEVICE_DESCRIPTOR);

					StorageDeviceDescriptor.Size = sizeof(STORAGE_DEVICE_DESCRIPTOR);

					StorageDeviceDescriptor.DeviceType = DIRECT_ACCESS_DEVICE;

					StorageDeviceDescriptor.DeviceTypeModifier = 0;

					StorageDeviceDescriptor.RemovableMedia = pDiskExt->Disk.bRemoveable;

					StorageDeviceDescriptor.CommandQueueing = FALSE;

					StorageDeviceDescriptor.VendorIdOffset = 0;

					StorageDeviceDescriptor.ProductIdOffset = 0;

					StorageDeviceDescriptor.ProductRevisionOffset = 0;

					StorageDeviceDescriptor.SerialNumberOffset = 0;

					StorageDeviceDescriptor.BusType = BusTypeScsi;

					StorageDeviceDescriptor.RawPropertiesLength = 0;

					RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, &StorageDeviceDescriptor, ulCopySize);

					pIrp->IoStatus.Information = (ULONG_PTR)ulCopySize;

					NtStatus = STATUS_SUCCESS;
				}

				break;
			}
		case IOCTL_DISK_GET_DRIVE_GEOMETRY:
			{
				ulCopySize = 
					(pIoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY)?
					pIoStack->Parameters.DeviceIoControl.OutputBufferLength:sizeof(DISK_GEOMETRY));

				if (pDiskExt->Disk.bRemoveable)
				{
					DiskGeometry.MediaType = RemovableMedia;
				}else
				{
					DiskGeometry.MediaType = FixedMedia;
				}

				DiskGeometry.Cylinders.QuadPart = pDiskExt->Disk.ulCylinders;

				DiskGeometry.TracksPerCylinder = pDiskExt->Disk.ulTracksPerCylinder;

				DiskGeometry.SectorsPerTrack = pDiskExt->Disk.ulSectorsPerTrack;

				DiskGeometry.BytesPerSector = pDiskExt->Disk.ulBytesPerSector;

				RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, &DiskGeometry, ulCopySize);

				pIrp->IoStatus.Information = (ULONG_PTR)ulCopySize;

				NtStatus = STATUS_SUCCESS;

				break;
			}
		case IOCTL_SCSI_GET_ADDRESS:
			{
				ulCopySize = 
					(pIoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SCSI_ADDRESS)?
					pIoStack->Parameters.DeviceIoControl.OutputBufferLength:sizeof(SCSI_ADDRESS));

				ScsiAdress.Length = sizeof(SCSI_ADDRESS);

				ScsiAdress.PortNumber = 0;

				ScsiAdress.PathId = 0;

				ScsiAdress.TargetId = (UCHAR)pDiskExt->Disk.ulDiskNumber;

				ScsiAdress.Lun = 0;

				RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer, &ScsiAdress, ulCopySize);

				pIrp->IoStatus.Information = (ULONG_PTR)ulCopySize;

				NtStatus = STATUS_SUCCESS;

				break;
			}
		default:
			{
				pIrp->IoStatus.Information = 0;

				NtStatus = STATUS_INVALID_PARAMETER;
			}
		}

		pIrp->IoStatus.Status = NtStatus;

		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	}

	return NtStatus;
}
