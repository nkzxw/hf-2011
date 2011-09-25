/*++
Copyright (C) HF. 2011-2012

Abstract:
	Disk Filter	
Author:
	yhf (hongfu830202@163.com)
CreateTime:
	2011-9-25
--*/

#include <ntifs.h>
#include <windef.h>
#include <mountmgr.h>
#include <mountdev.h>
#include <ntddvol.h>
#include <ntstrsafe.h>
#include "Bitmap.h"
#include "DiskFilter.h"

static size_t ProNameOffset=0;
ULONG Proflag = 0;

PDF_FILTER_DEV_EXTENSION g_ProtectDevExt = NULL;
PDEVICE_OBJECT g_ControlDevice;

VOID
HFReinitializationRoutine( 
		IN	PDRIVER_OBJECT	DriverObject, 
		IN	PVOID	Context, 
		IN	ULONG	Count 
		)
{
		NTSTATUS ntStatus;
		
		WCHAR	SparseFilename[] = L"\\??\\C:\\temp.dat";
		UNICODE_STRING SparseFilenameUni;		
		IO_STATUS_BLOCK	ios = { 0 };
		OBJECT_ATTRIBUTES	ObjAttr = { 0 };
		FILE_END_OF_FILE_INFORMATION  FileEndInfo = { 0 };
		
		RtlInitUnicodeString(&SparseFilenameUni,SparseFilename);

		InitializeObjectAttributes(&ObjAttr, 
															&SparseFilenameUni,
															OBJ_KERNEL_HANDLE|OBJ_CASE_INSENSITIVE,
															NULL,
															NULL);

		ntStatus = ZwCreateFile(&g_ProtectDevExt->TempFile,
														GENERIC_READ | GENERIC_WRITE,
														&ObjAttr,
														&ios,
														NULL,
														FILE_ATTRIBUTE_NORMAL,
														0,
														FILE_OVERWRITE_IF,
														FILE_NON_DIRECTORY_FILE |
														FILE_RANDOM_ACCESS |
														FILE_SYNCHRONOUS_IO_NONALERT |
														FILE_NO_INTERMEDIATE_BUFFERING,
														NULL,
														0);
		if(!NT_SUCCESS(ntStatus))
		{
				KdPrint (("[DiskFilter]: ZwCreateFile Error.\n"));
				goto ERROUT;
		}

		ntStatus = ZwFsControlFile(g_ProtectDevExt->TempFile,
															NULL,
															NULL,
															NULL,
															&ios,
															FSCTL_SET_SPARSE,
															NULL,
															0,
															NULL,
															0);
		if(!NT_SUCCESS(ntStatus))
		{
				KdPrint (("[DiskFilter]: ZwFsControlFile Error.\n"));
				goto ERROUT;
		}
		
		FileEndInfo.EndOfFile.QuadPart = g_ProtectDevExt->TotalSizeInByte.QuadPart + 10*1024*1024;
		ntStatus = ZwSetInformationFile(g_ProtectDevExt->TempFile,
																		&ios,
																		&FileEndInfo,
																		sizeof(FILE_END_OF_FILE_INFORMATION),
																		FileEndOfFileInformation);
		if (!NT_SUCCESS(ntStatus))
		{
				KdPrint (("[DiskFilter]: ZwSetInformationFile Error.\n"));
				goto ERROUT;
		}
		
		g_ProtectDevExt->Protect = TRUE;
		return;

ERROUT:
		KdPrint(("[DiskFilter]: error create temp file!\n"));
		return;
}


ULONG HFCurrentProName(
		IN PUNICODE_STRING name
		)
{
		PEPROCESS CurPro;
		ULONG i;
		ULONG len;
		ANSI_STRING AnsiName;
		if(ProNameOffset=0)  return 0;
		
		CurPro = PsGetCurrentProcess();
		RtlInitAnsiString(&AnsiName,((PCHAR)CurPro + 0x174));
		len = RtlAnsiStringToUnicodeSize(&AnsiName);
		if(len>name->MaximumLength)
		{
				return RtlAnsiStringToUnicodeSize(&AnsiName);
		}
		RtlAnsiStringToUnicodeString(name,&AnsiName,FALSE);
		return len;
}


NTSTATUS
HFReadDriverParameters (
		IN PUNICODE_STRING RegistryPath
		)
{
		NTSTATUS ntStatus;
		OBJECT_ATTRIBUTES attributes;
		HANDLE hRegister;
		ULONG ulLength;
		UNICODE_STRING valueName;
		
		UCHAR buffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( LONG )];

    if (0 == Proflag) {
        InitializeObjectAttributes(&attributes,
																	RegistryPath,
																	OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
																	NULL,
																	NULL);
		
				ntStatus = ZwOpenKey(&hRegister,
													KEY_READ,
													&attributes);
		
        if (!NT_SUCCESS( ntStatus )) {
        		KdPrint (("[DiskFilter]: ZwOpenKey Error.\n"));		
            return ntStatus;
        }

        RtlInitUnicodeString( &valueName, L"DebugFlags" );
		
        ntStatus = ZwQueryValueKey(hRegister,
																&valueName,
																KeyValuePartialInformation,
																buffer,
																sizeof(buffer),
																&ulLength );
																
        if (NT_SUCCESS(ntStatus)) {
            Proflag = *((PLONG) & (((PKEY_VALUE_PARTIAL_INFORMATION) buffer)->Data));
        }

        ZwClose(hRegister);
    }
    
    return ntStatus;
}
VOID HFSetDriverParameters(
		ULONG flag
		)
{
		UNICODE_STRING RegistryPath;
		OBJECT_ATTRIBUTES attributes;
		HANDLE driverRegKey;
		NTSTATUS status;
		ULONG resultLength;
		UNICODE_STRING valueName;
    
		RtlInitUnicodeString( &RegistryPath , MY_REG_KEY_NAME ) ;
		
		InitializeObjectAttributes(&attributes,
															&RegistryPath,
															OBJ_CASE_INSENSITIVE,
															NULL,
															NULL );
		
		status = ZwOpenKey(&driverRegKey,
											KEY_ALL_ACCESS,
											&attributes );
		
		if (!NT_SUCCESS( status )) {
				return;
		}
			
		RtlInitUnicodeString( &valueName, L"DebugFlags" );
		status =  ZwSetValueKey(driverRegKey,
														&valueName,
														0,
														REG_DWORD,
														&flag,
														sizeof(flag)
														);
		ZwClose(driverRegKey);
}

NTSTATUS
HFCompleteRequest(
		IN	PIRP	Irp,
		IN	NTSTATUS	Status,
		IN	CCHAR Priority
		)
{	
		Irp->IoStatus.Status = Status;
		
		IoCompleteRequest(Irp, Priority);
		return STATUS_SUCCESS;
}

NTSTATUS
HFSendToNextDriver(
		IN	PDEVICE_OBJECT TgtDevObj,
		IN	PIRP	Irp
		)
{	
		IoSkipCurrentIrpStackLocation(Irp);
		return IoCallDriver(TgtDevObj, Irp);
}


NTSTATUS
HFIrpCompletionRoutine(
		IN	PDEVICE_OBJECT	DeviceObject,
		IN	PIRP Irp,
		IN	PVOID	Context
		)
{
		PKEVENT Event = (PKEVENT) Context;
		
		UNREFERENCED_PARAMETER(DeviceObject);
		UNREFERENCED_PARAMETER(Irp);
		
		KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
		
		return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
HFForwardIrpSync(
		IN PDEVICE_OBJECT TgtDevObj,
		IN PIRP Irp
		)
{
		KEVENT event;
		NTSTATUS status;
		KeInitializeEvent(&event, NotificationEvent, FALSE);
		IoCopyCurrentIrpStackLocationToNext(Irp);
		IoSetCompletionRoutine(Irp, 
													HFIrpCompletionRoutine,
													&event, 
													TRUE, 
													TRUE, 
													TRUE);
		
		status = IoCallDriver(TgtDevObj, Irp);
		if (status == STATUS_PENDING)
		{
				KeWaitForSingleObject(&event, 
															Executive, 
															KernelMode, 
															FALSE, 
													    NULL);
				status = Irp->IoStatus.Status;
		}
		return status;
}

VOID
HFReadWriteThread (
		IN PVOID Context
		)
{
		NTSTATUS	ntStatus = STATUS_SUCCESS;
		PDF_FILTER_DEV_EXTENSION	DevExt = (PDF_FILTER_DEV_EXTENSION)Context;
		PLIST_ENTRY	ReqEntry = NULL;
		PIRP Irp = NULL;
		PIO_STACK_LOCATION	Irpsp = NULL;
		PBYTE	sysBuf = NULL;
		ULONG	length = 0;
		LARGE_INTEGER		offset = { 0 };
		PBYTE	fileBuf = NULL;
		PBYTE	devBuf = NULL;
		IO_STATUS_BLOCK		ios;

		KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);
		for (;;)
		{	
				KeWaitForSingleObject(&DevExt->ReqEvent,
															Executive,
															KernelMode,
															FALSE,
															NULL);
															
				if (DevExt->ThreadTermFlag)
				{
						PsTerminateSystemThread(STATUS_SUCCESS);
						return;
				}
		
				ReqEntry = ExInterlockedRemoveHeadList(&DevExt->ReqList,&DevExt->ReqLock);
				while (ReqEntry)
				{
						Irp = CONTAINING_RECORD(ReqEntry, IRP, Tail.Overlay.ListEntry);
						Irpsp = IoGetCurrentIrpStackLocation(Irp);
						if (NULL == Irp->MdlAddress){
								sysBuf = (PBYTE)Irp->UserBuffer;
						}
						else{
								sysBuf = (PBYTE)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
						}
			
						if (IRP_MJ_READ == Irpsp->MajorFunction)
						{
								offset = Irpsp->Parameters.Read.ByteOffset;
								length = Irpsp->Parameters.Read.Length;
						}
						else if (IRP_MJ_WRITE == Irpsp->MajorFunction){
								offset = Irpsp->Parameters.Write.ByteOffset;
								length = Irpsp->Parameters.Write.Length;
						}
						else{
								offset.QuadPart = 0;
								length = 0;
						}
			
						if (NULL == sysBuf || 0 == length)
						{
								goto ERRNEXT;
						}
						
						if (IRP_MJ_READ == Irpsp->MajorFunction)
						{
								long tstResult = BitmapTest(DevExt->Bitmap, offset, length);
								switch (tstResult)
								{
										case BITMAP_RANGE_CLEAR: 
												goto ERRNEXT;
										case BITMAP_RANGE_SET: 		
												if (NULL == (fileBuf = (PBYTE)ExAllocatePoolWithTag(NonPagedPool, length, 'xypD')))
												{
														ntStatus = STATUS_INSUFFICIENT_RESOURCES;
														Irp->IoStatus.Information = 0;
														goto ERRERR;
												}
												RtlZeroMemory(fileBuf,length);
												ntStatus = ZwReadFile(DevExt->TempFile,
																							NULL,
																							NULL,
																							NULL,
																							&ios,
																							fileBuf,
																							length,
																							&offset,
																							NULL);
												if (NT_SUCCESS(ntStatus))
												{
														Irp->IoStatus.Information = length;
														RtlCopyMemory(sysBuf,fileBuf,Irp->IoStatus.Information);
														goto ERRCMPLT;
												}
												else{
														ntStatus = STATUS_INSUFFICIENT_RESOURCES;
														Irp->IoStatus.Information = 0;
														goto ERRERR;
												}
												break;
										case BITMAP_RANGE_BLEND:
												if (NULL == (fileBuf = (PBYTE)ExAllocatePoolWithTag(NonPagedPool, length, 'xypD')))
												{
														ntStatus = STATUS_INSUFFICIENT_RESOURCES;
														Irp->IoStatus.Information = 0;
														goto ERRERR;
												}
												RtlZeroMemory(fileBuf,length);
												if (NULL == (devBuf = (PBYTE)ExAllocatePoolWithTag(NonPagedPool, length, 'xypD')))
												{
														ntStatus = STATUS_INSUFFICIENT_RESOURCES;
														Irp->IoStatus.Information = 0;
														goto ERRERR;
												}
												RtlZeroMemory(devBuf,length);
												ntStatus = ZwReadFile(DevExt->TempFile,
																							NULL,
																							NULL,
																							NULL,
																							&ios,
																							fileBuf,
																							length,
																							&offset,
																							NULL);
												if (!NT_SUCCESS(ntStatus))
												{
														ntStatus = STATUS_INSUFFICIENT_RESOURCES;
														Irp->IoStatus.Information = 0;
														goto ERRERR;
												}
												ntStatus = HFForwardIrpSync(DevExt->LowerDevObj,Irp);
												if (!NT_SUCCESS(ntStatus))
												{
														ntStatus = STATUS_INSUFFICIENT_RESOURCES;
														Irp->IoStatus.Information = 0;
														goto ERRERR;
												}
												memcpy(devBuf, sysBuf, Irp->IoStatus.Information);
								
												ntStatus = BitmapGet(DevExt->Bitmap,
																						offset,
																						length,
																						devBuf,
																						fileBuf);
												if (!NT_SUCCESS(ntStatus))
												{
														ntStatus = STATUS_INSUFFICIENT_RESOURCES;
														Irp->IoStatus.Information = 0;
														goto ERRERR;
												}											
												memcpy(sysBuf, devBuf, Irp->IoStatus.Information);
												goto ERRCMPLT;
										default:
												ntStatus = STATUS_INSUFFICIENT_RESOURCES;
												goto ERRERR;
									}
							}
							else
							{
									ntStatus = ZwWriteFile(DevExt->TempFile,
																				NULL,
																				NULL,
																				NULL,
																				&ios,
																				sysBuf,
																				length,
																				&offset,
																				NULL);
									if(!NT_SUCCESS(ntStatus))
									{
											ntStatus = STATUS_INSUFFICIENT_RESOURCES;
											goto ERRERR;
									}
									else
									{
											if (NT_SUCCESS(ntStatus = BitmapSet(DevExt->Bitmap, offset, length)))
											{
													goto ERRCMPLT;
											}
											else{
													ntStatus = STATUS_INSUFFICIENT_RESOURCES;
													goto ERRERR;
											}
									}
						}
ERRERR:
			if (NULL != fileBuf)
			{
				ExFreePool(fileBuf);
				fileBuf = NULL;
			}
			if (NULL != devBuf)
			{
				ExFreePool(devBuf);
				devBuf = NULL;
			}
			HFCompleteRequest(
				Irp,
				ntStatus,
				IO_NO_INCREMENT
				);
			continue;
ERRNEXT:
			if (NULL != fileBuf)
			{
				ExFreePool(fileBuf);
				fileBuf = NULL;
			}
			if (NULL != devBuf)
			{
				ExFreePool(devBuf);
				devBuf = NULL;
			}	
			HFSendToNextDriver(
				DevExt->LowerDevObj,
				Irp);
			continue;
ERRCMPLT:
			if (NULL != fileBuf)
			{
				ExFreePool(fileBuf);
				fileBuf = NULL;
			}
			if (NULL != devBuf)
			{
				ExFreePool(devBuf);
				devBuf = NULL;
			}
			HFCompleteRequest(
				Irp,
				STATUS_SUCCESS,
				IO_DISK_INCREMENT
				);
			continue;
			
		}
	}

}

NTSTATUS
HFAddDevice(
    IN	PDRIVER_OBJECT	DriverObject,
    IN	PDEVICE_OBJECT	PhysicalDeviceObject
    )
{
		NTSTATUS ntStatus = STATUS_SUCCESS;
		PDF_FILTER_DEV_EXTENSION	DevExt = NULL;
		PDEVICE_OBJECT				LowerDevObj = NULL;
		PDEVICE_OBJECT				FltDevObj = NULL;
		HANDLE						ThreadHandle = NULL;

		ntStatus = IoCreateDevice(DriverObject,
															sizeof(DF_FILTER_DEV_EXTENSION),
															NULL,
															FILE_DEVICE_DISK,
															FILE_DEVICE_SECURE_OPEN,
															FALSE,
															&FltDevObj);
		if (!NT_SUCCESS(ntStatus)){ 
				KdPrint (("[DiskFilter]: IoCreateDevice Error.\n"));
				goto ERROUT;
		}

		DevExt = FltDevObj->DeviceExtension;
		RtlZeroMemory(DevExt,sizeof(DF_FILTER_DEV_EXTENSION));

		LowerDevObj = IoAttachDeviceToDeviceStack(FltDevObj, PhysicalDeviceObject);
		if (NULL == LowerDevObj)
		{
				ntStatus = STATUS_NO_SUCH_DEVICE;
				KdPrint (("[DiskFilter]: IoAttachDeviceToDeviceStack Error.\n"));
				goto ERROUT;
		}

		KeInitializeEvent(&DevExt->PagingPathCountEvent,
											NotificationEvent, 
											TRUE);

		FltDevObj->Flags = LowerDevObj->Flags;
		FltDevObj->Flags |= DO_POWER_PAGABLE;
		FltDevObj->Flags &= ~DO_DEVICE_INITIALIZING;
		
		DevExt->FltDevObj = FltDevObj;
		DevExt->PhyDevObj = PhysicalDeviceObject;
		DevExt->LowerDevObj = LowerDevObj;

		InitializeListHead(&DevExt->ReqList);
		KeInitializeSpinLock(&DevExt->ReqLock);
		KeInitializeEvent(&DevExt->ReqEvent,
											SynchronizationEvent,
											FALSE);

		DevExt->ThreadTermFlag = FALSE;
		ntStatus = PsCreateSystemThread(&ThreadHandle,
																		(ACCESS_MASK)0L,
																		NULL,
																		NULL,
																		NULL,
																		HFReadWriteThread,
																		DevExt);
		if (!NT_SUCCESS(ntStatus)){
				KdPrint (("[DiskFilter]:PsCreateSystemThread Error.\n"));
				goto ERROUT;
		}

		ntStatus = ObReferenceObjectByHandle(ThreadHandle,
																				THREAD_ALL_ACCESS,
																				NULL,
																				KernelMode,
																				&DevExt->ThreadHandle,
																				NULL);
		if (!NT_SUCCESS(ntStatus))
		{
				DevExt->ThreadTermFlag = TRUE;
				KeSetEvent(&DevExt->ReqEvent,
									(KPRIORITY)0,
									FALSE);
				goto ERROUT;
		}

ERROUT:
		if (!NT_SUCCESS(ntStatus))
		{	
				if (NULL != LowerDevObj)
				{
						IoDetachDevice(LowerDevObj);
						DevExt->LowerDevObj = NULL;
				}			
				if (NULL != FltDevObj)
				{
						IoDeleteDevice(FltDevObj);
						DevExt->FltDevObj = NULL;
				}
		}
		
		if (NULL != ThreadHandle){
				ZwClose(ThreadHandle);
		}
		
		return ntStatus;
}

VOID
HFUnload(
		IN	PDRIVER_OBJECT	DriverObject
		)
{	
		UNICODE_STRING usDevcieLinkName;
		
	  IoDeleteDevice (DriverObject->DeviceObject);
	  RtlInitUnicodeString (&usDevcieLinkName, DEVICE_LINK_NAME);
		IoDeleteSymbolicLink (&usDevcieLinkName);
		
		return;
}

NTSTATUS
HFDispatch(
    IN	PDEVICE_OBJECT	DeviceObject,
    IN	PIRP			Irp
    )
{
		NTSTATUS	ntStatus = STATUS_SUCCESS;
		PDF_FILTER_DEV_EXTENSION	DevExt = DeviceObject->DeviceExtension;	
		if (DeviceObject == g_ControlDevice) 
		{
				PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);
				if(irpStack->MajorFunction = IRP_MJ_CREATE || IRP_MJ_CLOSE)
				{	
						Irp->IoStatus.Status = ntStatus;
						Irp->IoStatus.Information = 0;
						IoCompleteRequest( Irp, IO_NO_INCREMENT );
						return ntStatus;
				}
		}
		
		ASSERT(DeviceObject != g_ControlDevice);
		
		return HFSendToNextDriver(DevExt->LowerDevObj,Irp);
}

NTSTATUS
HFPower(
    IN	PDEVICE_OBJECT	DeviceObject,
    IN	PIRP			Irp
    )
{
		PDF_FILTER_DEV_EXTENSION DevExt = DeviceObject->DeviceExtension;

		ASSERT(DeviceObject != g_ControlDevice);	
		
		return HFSendToNextDriver(DevExt->LowerDevObj,Irp);
}

NTSTATUS	
HFPnp(
		IN	PDEVICE_OBJECT	DeviceObject,
		IN	PIRP			Irp
		)
{
		PDF_FILTER_DEV_EXTENSION	DevExt = DeviceObject->DeviceExtension;
		NTSTATUS ntStatus = STATUS_SUCCESS;
		PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);
	
		if (DeviceObject == g_ControlDevice) 
		{			
				Irp->IoStatus.Status = ntStatus;
				Irp->IoStatus.Information = 0;
				IoCompleteRequest( Irp, IO_NO_INCREMENT);
				return ntStatus;			
		}
		
		switch(irpStack->MinorFunction) 
		{
			case IRP_MN_REMOVE_DEVICE:
			{
					if (DevExt->ThreadTermFlag != TRUE && 
							NULL != DevExt->ThreadHandle)
					{
							DevExt->ThreadTermFlag = TRUE;
							KeSetEvent(&DevExt->ReqEvent,
												(KPRIORITY) 0,
												FALSE);
												
							KeWaitForSingleObject(DevExt->ThreadHandle,
																		Executive,
																		KernelMode,
																		FALSE,
																		NULL);
																		
							ObDereferenceObject(DevExt->ThreadHandle);
					}
			 		if (NULL != DevExt->Bitmap)
			 		{
				 			BitmapFree(DevExt->Bitmap);
			 		}
					if (NULL != DevExt->LowerDevObj)
					{
							IoDetachDevice(DevExt->LowerDevObj);
					}
			 		if (NULL != DevExt->FltDevObj)
			 		{
			 				IoDeleteDevice(DevExt->FltDevObj);
			 		}
					break;
			}
			case IRP_MN_DEVICE_USAGE_NOTIFICATION:
			{
					BOOLEAN setPagable;
					if (irpStack->Parameters.UsageNotification.Type != DeviceUsageTypePaging) 
					{
							ntStatus = HFSendToNextDriver(DevExt->LowerDevObj,Irp);
							return ntStatus; 
					}
				
					ntStatus = KeWaitForSingleObject(&DevExt->PagingPathCountEvent,
																					Executive, 
																					KernelMode,
																					FALSE, 
																					NULL);			
				
					setPagable = FALSE;
					if (!irpStack->Parameters.UsageNotification.InPath &&
							DevExt->PagingPathCount == 1 ) 
					{
							if (DeviceObject->Flags & DO_POWER_INRUSH)
							{
									//TODO
							} 
							else {
									DeviceObject->Flags |= DO_POWER_PAGABLE;
									setPagable = TRUE;
							}
					}

					ntStatus = HFForwardIrpSync(DevExt->LowerDevObj,Irp);			
					if (NT_SUCCESS(ntStatus)) 
					{
							IoAdjustPagingPathCount(&DevExt->PagingPathCount,
																			irpStack->Parameters.UsageNotification.InPath);
							if (irpStack->Parameters.UsageNotification.InPath) 
							{
									if (DevExt->PagingPathCount == 1) 
									{
											DeviceObject->Flags &= ~DO_POWER_PAGABLE;
									}
							}
					}
					else {
							if (setPagable == TRUE) 
							{
									DeviceObject->Flags &= ~DO_POWER_PAGABLE;
									setPagable = FALSE;
							}
					}
					
					KeSetEvent(&DevExt->PagingPathCountEvent,
										IO_NO_INCREMENT, 
										FALSE);
					
					IoCompleteRequest(Irp, IO_NO_INCREMENT);
					return ntStatus;
			}		
			default:
				break;
		}
		
		return HFSendToNextDriver(DevExt->LowerDevObj,Irp);
}

NTSTATUS HFQueryVolumeInformationCompletionRoutine(
		IN	PDEVICE_OBJECT pDeviceObject,
		IN	PIRP	pIrp,
		IN	PVOID	 Context
		)
{
		KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
		return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS HFQueryVolumeInformation(
		PDEVICE_OBJECT	DevObj,
		LARGE_INTEGER *TotalSize,
		DWORD *ClusterSize,
		DWORD *SectorSize
		)
{
		const UCHAR FAT16FLG[4] = {'F','A','T','1'};
		const UCHAR FAT32FLG[4] = {'F','A','T','3'};
		const UCHAR NTFSFLG[4] = {'N','T','F','S'};

		NTSTATUS ntStatus = STATUS_SUCCESS;
		BYTE DBR[512] = { 0 };
		ULONG DBRLength = 512;

		PNTFS_BOOT_SECTOR pNtfsBootSector = (PNTFS_BOOT_SECTOR)DBR;
		PFAT32_BOOT_SECTOR pFat32BootSector = (PFAT32_BOOT_SECTOR)DBR;
		PFAT16_BOOT_SECTOR pFat16BootSector = (PFAT16_BOOT_SECTOR)DBR;

		LARGE_INTEGER readOffset = { 0 };
		IO_STATUS_BLOCK ios;
		KEVENT Event;
		PIRP pIrp	= NULL;

		KeInitializeEvent(&Event, NotificationEvent, FALSE);
		pIrp = IoBuildAsynchronousFsdRequest(IRP_MJ_READ,
																				DevObj,
																				DBR,
																				DBRLength,
																				&readOffset,
																				&ios);
	 	if (NULL == pIrp)
	 	{
	 			KdPrint (("[DiskFilter]: IoBuildAsynchronousFsdRequest Error.\n"));
		 		goto ERROUT;
	 	}

		IoSetCompletionRoutine(pIrp,
													HFQueryVolumeInformationCompletionRoutine,
													&Event,
													TRUE,
													TRUE,
													TRUE);
													
		ntStatus = IoCallDriver(DevObj, pIrp);
		if(ntStatus = STATUS_PENDING)
		{
				ntStatus = KeWaitForSingleObject(&Event,
																				Executive,
																				KernelMode,
																				FALSE,
																				NULL);
				ntStatus = pIrp->IoStatus.Status;
				if (!NT_SUCCESS(ntStatus))
				{
						KdPrint (("[DiskFilter]: KeWaitForSingleObject Error.\n"));
						goto ERROUT;
				}
		}
	
		if (*(DWORD*)NTFSFLG == *(DWORD*)&DBR[NTFS_SIG_OFFSET])
		{
				*SectorSize = (DWORD)(pNtfsBootSector->BytesPerSector);
				*ClusterSize = (*SectorSize) * (DWORD)(pNtfsBootSector->SectorsPerCluster);    
				TotalSize->QuadPart = (LONGLONG)(*SectorSize) * (LONGLONG)pNtfsBootSector->TotalSectors;
		}
		else if (*(DWORD*)FAT32FLG == *(DWORD*)&DBR[FAT32_SIG_OFFSET]){
				*SectorSize = (DWORD)(pFat32BootSector->BytesPerSector);
				*ClusterSize = (*SectorSize) * (DWORD)(pFat32BootSector->SectorsPerCluster);    
				TotalSize->QuadPart = (LONGLONG)(*SectorSize) * (LONGLONG)(pFat32BootSector->LargeSectors + pFat32BootSector->Sectors);
		}
		else if (*(DWORD*)FAT16FLG == *(DWORD*)&DBR[FAT16_SIG_OFFSET]){
				*SectorSize = (DWORD)(pFat16BootSector->BytesPerSector);
				*ClusterSize = (*SectorSize) * (DWORD)(pFat16BootSector->SectorsPerCluster);    
				TotalSize->QuadPart = (LONGLONG)(*SectorSize) * (LONGLONG)(pFat16BootSector->LargeSectors + pFat16BootSector->Sectors);
		}
		else{
				ntStatus = STATUS_UNSUCCESSFUL;
		}
		
ERROUT:
		if (NULL != pIrp)
		{
				IoFreeIrp(pIrp);
		}
		
		return ntStatus;
}

NTSTATUS
HFVolumeOnLineCompleteRoutine(
		IN PDEVICE_OBJECT  DeviceObject,
		IN PIRP  Irp,
		IN PVOLUME_ONLINE_CONTEXT  Context
		)
{
		NTSTATUS ntStatus = STATUS_SUCCESS;
		UNICODE_STRING	DosName = { 0 };
		
		ASSERT(NULL != Context);
		
		ntStatus = IoVolumeDeviceToDosName(Context->DevExt->PhyDevObj, &DosName);
		if (!NT_SUCCESS(ntStatus))
			goto ERROUT;
			
		Context->DevExt->VolumeLetter = DosName.Buffer[0];
		if (Context->DevExt->VolumeLetter > L'Z'){
				Context->DevExt->VolumeLetter -= (L'a' - L'A');
		}
	
		if (Context->DevExt->VolumeLetter == L'D')
		{
				ntStatus = HFQueryVolumeInformation(Context->DevExt->PhyDevObj,
																					&(Context->DevExt->TotalSizeInByte),
																					&(Context->DevExt->ClusterSizeInByte),
																					&(Context->DevExt->SectorSizeInByte));
				if (!NT_SUCCESS(ntStatus))
				{
						KdPrint (("[DiskFilter]: HFQueryVolumeInformation Error.\n"));
						goto ERROUT;
				}
				
				ntStatus = BitmapInit(&Context->DevExt->Bitmap,
															Context->DevExt->SectorSizeInByte,
															8,
															25600,
															(DWORD)(Context->DevExt->TotalSizeInByte.QuadPart / 
															(LONGLONG)(25600 * 8 * Context->DevExt->SectorSizeInByte)) + 1);
				if (!NT_SUCCESS(ntStatus)){
						KdPrint (("[DiskFilter]: BitmapInit Error.\n"));
						goto ERROUT;
				}
				
				g_ProtectDevExt = Context->DevExt;
		}
		
ERROUT:
		if (!NT_SUCCESS(ntStatus))
		{
				if (NULL != Context->DevExt->Bitmap)
				{
						BitmapFree(Context->DevExt->Bitmap);
				}

				if (NULL != Context->DevExt->TempFile)
				{
						ZwClose(Context->DevExt->TempFile);
				}
		}
		
		if (NULL != DosName.Buffer)
		{
				ExFreePool(DosName.Buffer);
		}
		
		KeSetEvent(Context->Event,
							0,
							FALSE);

		return ntStatus;
}

NTSTATUS
HFDeviceControl(
		IN	PDEVICE_OBJECT	DeviceObject,
		IN	PIRP			Irp
		)
{
		KEVENT	Event;
		VOLUME_ONLINE_CONTEXT	context;
		PDF_FILTER_DEV_EXTENSION	DevExt = DeviceObject->DeviceExtension;
		NTSTATUS ntStatus = STATUS_SUCCESS;
		PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);
		
		if (DeviceObject == g_ControlDevice)
		{	
				switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
				{
						UNICODE_STRING Registry;
						case IOCTL_CLOSE_PROTECT:
								HFSetDriverParameters(0x00000001);
																
								Irp->IoStatus.Status = ntStatus;
								Irp->IoStatus.Information = 0;
								IoCompleteRequest( Irp, IO_NO_INCREMENT );
								
								return ntStatus;
						case IOCTL_OPEN_PROTECT:
								HFSetDriverParameters(0x00000000);
								
								Irp->IoStatus.Status = ntStatus;
								Irp->IoStatus.Information = 0;
								IoCompleteRequest( Irp, IO_NO_INCREMENT );
								
								return ntStatus;
						default:
								Irp->IoStatus.Status = ntStatus;
								Irp->IoStatus.Information = 0;
								IoCompleteRequest( Irp, IO_NO_INCREMENT );
								
								return ntStatus;
				}
		}
		else {	
				switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
				{
						case IOCTL_VOLUME_ONLINE:
						{
								KeInitializeEvent(&Event, NotificationEvent, FALSE);
								context.DevExt = DevExt;
								context.Event = &Event;
								
								IoCopyCurrentIrpStackLocationToNext(Irp);
								IoSetCompletionRoutine(Irp, 
																			HFVolumeOnLineCompleteRoutine, 
																			&context,
																			TRUE,
																			TRUE,
																			TRUE);
																			
								ntStatus = IoCallDriver(DevExt->LowerDevObj, Irp);
								KeWaitForSingleObject(&Event,
																			Executive,
																			KernelMode,
																			FALSE,
																			NULL);

								return ntStatus;
						}
						default:
								break;
				}
		}
		return HFSendToNextDriver(DevExt->LowerDevObj,Irp);		
}

NTSTATUS
HFReadWrite(
		IN	PDEVICE_OBJECT	DeviceObject,
		IN	PIRP			Irp
		)
{	
		PDF_FILTER_DEV_EXTENSION	DevExt = DeviceObject->DeviceExtension;
		NTSTATUS ntStatus = STATUS_SUCCESS;
		
		if (DeviceObject == g_ControlDevice) 
		{				
				Irp->IoStatus.Status = ntStatus;
				Irp->IoStatus.Information = 0;
				IoCompleteRequest( Irp, IO_NO_INCREMENT );
				
				return ntStatus;
		}
		else {
				WCHAR namebuf[32] = {0};
				UNICODE_STRING ProName = {0};
				ULONG length;
				
				RtlInitEmptyUnicodeString(&ProName, namebuf, 32*sizeof(WCHAR));
				length = HFCurrentProName(&ProName);

				KdPrint(("[DiskFilter]: Current Process is %wZ\n",&ProName));
				
				if (DevExt->Protect && !Proflag)
				{
						IoMarkIrpPending(Irp);
						
						ExInterlockedInsertTailList(&DevExt->ReqList,
																			&Irp->Tail.Overlay.ListEntry,
																			&DevExt->ReqLock);
						KeSetEvent(&DevExt->ReqEvent, 
											(KPRIORITY)0, 
											FALSE);
											
						return STATUS_PENDING;
				}
				else{
						return HFSendToNextDriver(DevExt->LowerDevObj,
																		Irp);
				}
		}
}


NTSTATUS
DriverEntry(
    IN	PDRIVER_OBJECT	DriverObject,
    IN	PUNICODE_STRING	RegistryPath
    )
{
		int i;
		NTSTATUS ntStatus; 
		PDEVICE_OBJECT DeviceObject; 
		
		UNICODE_STRING usDeviceName;
		UNICODE_STRING usDeviceLinkName; 
		
		ntStatus = HFReadDriverParameters (RegistryPath);
		
		RtlInitUnicodeString (&usDeviceName, DEVICE_NAME); 
		
		ntStatus =	IoCreateDevice (DriverObject, 
																0, 
																&usDeviceName, 
																FILE_DEVICE_DISK,
																FILE_DEVICE_SECURE_OPEN,
																FALSE,
																&DeviceObject);
		if (!NT_SUCCESS (ntStatus))
		{
				KdPrint (("[DiskFilter]: IoCreateDevice Error.\n"));
				return ntStatus;
		}																
																 
		g_ControlDevice = DeviceObject;
		RtlInitUnicodeString (&usDeviceLinkName, DEVICE_LINK_NAME);
		ntStatus = IoCreateSymbolicLink (&usDeviceLinkName, &usDeviceName); 
		if(!NT_SUCCESS(ntStatus)) 
		{ 
				KdPrint (("[DiskFilter]: IoCreateSymbolicLink Error.\n"));
				IoDeleteDevice(DeviceObject); 
				return ntStatus; 
		} 
		
		for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
		{
				DriverObject->MajorFunction[i] = HFDispatch;
		}
		
		DriverObject->MajorFunction[IRP_MJ_POWER] = HFPower;
		DriverObject->MajorFunction[IRP_MJ_PNP] = HFPnp;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HFDeviceControl;
		DriverObject->MajorFunction[IRP_MJ_READ] = HFReadWrite;
		DriverObject->MajorFunction[IRP_MJ_WRITE] = HFReadWrite;
		
		DriverObject->DriverExtension->AddDevice = HFAddDevice;
		
		DriverObject->DriverUnload = HFUnload;
		
		IoRegisterBootDriverReinitialization(DriverObject,
																				HFReinitializationRoutine,
																				NULL
																				);
				
		return ntStatus;
}