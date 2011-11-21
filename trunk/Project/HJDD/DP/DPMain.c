#include <ntifs.h>
#include <windef.h>
#include <mountmgr.h>
#include <mountdev.h>
#include <ntddvol.h>
#include <ntstrsafe.h>
#include "DPBitmap.h"
#include "DPMain.h"

//�������жϳ��Ǹ��豸����Ҫ������ʱ�򣬻Ὣ���ָ��ָ�������Ҫ�����豸��DevExt
PDP_FILTER_DEV_EXTENSION gProtectDevExt = NULL;

VOID
DPReinitializationRoutine( 
	IN	PDRIVER_OBJECT	DriverObject, 
	IN	PVOID			Context, 
	IN	ULONG			Count 
	)
{
	//����ֵ
	NTSTATUS ntStatus;
	//D�̵Ļ����ļ���
	WCHAR				SparseFilename[] = L"\\??\\E:\\temp.dat";
	UNICODE_STRING		SparseFilenameUni;
	//�����ļ�ʱ��io����״ֵ̬
	IO_STATUS_BLOCK					ios = { 0 };
	//�����ļ�ʱ�Ķ������Ա���
	OBJECT_ATTRIBUTES				ObjAttr = { 0 };
	//�����ļ���С��ʱ��ʹ�õ��ļ���β������
	FILE_END_OF_FILE_INFORMATION    FileEndInfo = { 0 };

	//�����ǽ�Ҫ������ת�����ļ�
	//��ʼ��Ҫ�򿪵��ļ���
	RtlInitUnicodeString(&SparseFilenameUni,SparseFilename);
	//��ʼ���ļ�����Ӧ�Ķ�������������Ҫ�����ʼ��Ϊ�ں˶��󣬲��Ҵ�Сд������
	InitializeObjectAttributes(
		&ObjAttr, 
		&SparseFilenameUni,
		OBJ_KERNEL_HANDLE|OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);
	//�����ļ���������Ҫע����ǣ�Ҫ����FILE_NO_INTERMEDIATE_BUFFERINGѡ������ļ�ϵͳ�ٻ�������ļ�
	ntStatus = ZwCreateFile(
		&gProtectDevExt->TempFile,
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
		goto ERROUT;
	}
	//��������ļ�Ϊϡ���ļ�
	ntStatus = ZwFsControlFile(
		gProtectDevExt->TempFile,
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
		goto ERROUT;
	}
	//��������ļ��Ĵ�СΪ"D"�̵Ĵ�С��������10m�ı����ռ�
	FileEndInfo.EndOfFile.QuadPart = gProtectDevExt->TotalSizeInByte.QuadPart + 10*1024*1024;
	ntStatus = ZwSetInformationFile(
		gProtectDevExt->TempFile,
		&ios,
		&FileEndInfo,
		sizeof(FILE_END_OF_FILE_INFORMATION),
		FileEndOfFileInformation
		);
	if (!NT_SUCCESS(ntStatus))
	{
		goto ERROUT;
	}
	//����ɹ���ʼ���ͽ������ı�����־����Ϊ�ڱ���״̬
	gProtectDevExt->Protect = TRUE;
	return;
ERROUT:
	KdPrint(("error create temp file!\n"));
	return;
}
  
NTSTATUS
DriverEntry(
    IN	PDRIVER_OBJECT	DriverObject,
    IN	PUNICODE_STRING	RegistryPath
    )
{
	int i;

	//KdBreakPoint();

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		//��ʼ������������еķַ�������Ĭ��ֵ�ǳ�ʼ��ΪDPDispatchAny
		DriverObject->MajorFunction[i] = DPDispatchAny;
	}
    
	//���潫���������ע�ķַ��������¸�ֵΪ�����Լ��Ĵ�����
    DriverObject->MajorFunction[IRP_MJ_POWER] = DPDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_PNP] = DPDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DPDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_READ] = DPDispatchReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = DPDispatchReadWrite;

	//�����������AddDevice������ʼ��ΪDpAddDevice����
    DriverObject->DriverExtension->AddDevice = DPAddDevice;
	//�����������unload������ʼ��ΪDpUnload����
    DriverObject->DriverUnload = DPUnload;
    
	//ע��һ��boot���������ص�������ص������������е�boot���������������֮����ȥִ��
	IoRegisterBootDriverReinitialization(
		DriverObject,
		DPReinitializationRoutine,
		NULL
		);

	//��Ϊһ������������������ζ�Ҫ���سɹ�
    return STATUS_SUCCESS;
}

NTSTATUS
DPCompleteRequest(
	IN	PIRP			Irp,
	IN	NTSTATUS		Status,
	IN	CCHAR			Priority
	)
{	
	//��IRP��io״̬��ֵΪ����Ĳ���
	Irp->IoStatus.Status = Status;
	//����IoCompleteRequest��������Irp
	IoCompleteRequest(Irp, Priority);
	return STATUS_SUCCESS;
}

NTSTATUS
DPSendToNextDriver(
	IN	PDEVICE_OBJECT	TgtDevObj,
	IN	PIRP			Irp
	)
{	
	//������ǰ��irp stack
	IoSkipCurrentIrpStackLocation(Irp);
	//����Ŀ���豸���������irp
	return IoCallDriver(TgtDevObj, Irp);
}


NTSTATUS
DPIrpCompletionRoutine(
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PIRP			Irp,
	IN	PVOID			Context
	)
{
	//������������Ĳ���ת��Ϊһ���ں��¼�����
	PKEVENT Event = (PKEVENT) Context;
	//���Ե�����Ĳ���Ҫ�Ĳ���
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	//����������󣬻��ѵȴ����Ľ���
	KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
	//����
	return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
DPForwardIrpSync(
	IN PDEVICE_OBJECT TgtDevObj,
	IN PIRP Irp
	)
{
	//�����ȴ����¼�
	KEVENT event;
	//����ֵ
	NTSTATUS status;
	//��ʼ���ȴ��¼�
	KeInitializeEvent(&event, NotificationEvent, FALSE);
	//����һ��irp stack
	IoCopyCurrentIrpStackLocationToNext(Irp);
	//������ɺ��������ҽ��ȴ��¼�����Ϊ�����ʼ�����¼��������ɺ��������ã�����¼����ᱻ���ã�ͬʱҲ�ɴ˻�֪���irp���������
	IoSetCompletionRoutine(
		Irp, 
		DPIrpCompletionRoutine,
		&event, 
		TRUE, 
		TRUE, 
		TRUE);
	//����Ŀ���豸ȥ�������irp
	status = IoCallDriver(TgtDevObj, Irp);
	//������÷��ص���STATUS_PENDING��˵��Ŀ���豸�ڴ������irp��ʱ����Ҫ�����ʱ�䣬���ǾͿ�ʼ�ȴ���ֱ�����������Ϊֹ
	if (status == STATUS_PENDING)
	{
		KeWaitForSingleObject(
			&event, 
			Executive, 
			KernelMode, 
			FALSE, 
			NULL);
		//�ȵ���֮��ֵ״̬����
		status = Irp->IoStatus.Status;
	}
	//����״̬����
	return status;
}

VOID
DPReadWriteThread (
	IN PVOID Context
	)
{
	//NTSTATUS���͵ĺ�������ֵ
	NTSTATUS					ntStatus = STATUS_SUCCESS;
	//����ָ������豸���豸��չ��ָ��
	PDP_FILTER_DEV_EXTENSION	DevExt = (PDP_FILTER_DEV_EXTENSION)Context;
	//������е����
	PLIST_ENTRY			ReqEntry = NULL;
	//irpָ��
	PIRP				Irp = NULL;
	//irp stackָ��
	PIO_STACK_LOCATION	Irpsp = NULL;
	//irp�а��������ݵ�ַ
	PBYTE				sysBuf = NULL;
	//irp�е����ݳ���
	ULONG				length = 0;
	//irpҪ�����ƫ����
	LARGE_INTEGER		offset = { 0 };
	//�ļ�����ָ��
	PBYTE				fileBuf = NULL;
	//�豸����ָ��
	PBYTE				devBuf = NULL;
	//io����״̬
	IO_STATUS_BLOCK		ios;

	//��������̵߳����ȼ�
	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);
	//�������̵߳�ʵ�ֲ��֣����ѭ�������˳�
	for (;;)
	{	
		//�ȵȴ��������ͬ���¼������������û��irp��Ҫ�������ǵ��߳̾͵ȴ�������ó�cpuʱ��������߳�
		KeWaitForSingleObject(
			&DevExt->ReqEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
		//��������߳̽�����־����ô�����߳��ڲ��Լ������Լ�
		if (DevExt->ThreadTermFlag)
		{
			//�����̵߳�Ψһ�˳��ص�
			PsTerminateSystemThread(STATUS_SUCCESS);
			return;
		}
		//��������е��ײ��ó�һ��������׼����������ʹ�������������ƣ����Բ����г�ͻ
		while (ReqEntry = ExInterlockedRemoveHeadList(
			&DevExt->ReqList,
			&DevExt->ReqLock
			))
		{
			//�Ӷ��е�������ҵ�ʵ�ʵ�irp�ĵ�ַ
			Irp = CONTAINING_RECORD(ReqEntry, IRP, Tail.Overlay.ListEntry);
			//ȡ��irp stack
			Irpsp = IoGetCurrentIrpStackLocation(Irp);
			//��ȡ���irp���а����Ļ����ַ�������ַ��������mdl��Ҳ���ܾ���ֱ�ӵĻ��壬��ȡ�������ǵ�ǰ�豸��io��ʽ��buffer����direct��ʽ
			if (NULL == Irp->MdlAddress)
				sysBuf = (PBYTE)Irp->UserBuffer;
			else
				sysBuf = (PBYTE)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

			if (IRP_MJ_READ == Irpsp->MajorFunction)
			{
				//����Ƕ���irp����������irp stack��ȡ����Ӧ�Ĳ�����Ϊoffset��length
				offset = Irpsp->Parameters.Read.ByteOffset;
				length = Irpsp->Parameters.Read.Length;
			}
			else if (IRP_MJ_WRITE == Irpsp->MajorFunction)
			{
				//�����д��irp����������irp stack��ȡ����Ӧ�Ĳ�����Ϊoffset��length
				offset = Irpsp->Parameters.Write.ByteOffset;
				length = Irpsp->Parameters.Write.Length;
			}
			else
			{
				//����֮�⣬offset��length����0
				offset.QuadPart = 0;
				length = 0;
			}
			if (NULL == sysBuf || 0 == length)
			{
				//�����������irpû��ϵͳ������߻���ĳ�����0����ô���Ǿ�û�б�Ҫ�������irp��ֱ���·����²��豸������
				goto ERRNEXT;
			}
			//������ת���Ĺ�����
			if (IRP_MJ_READ == Irpsp->MajorFunction)
			{
				//�����Ƕ��Ĵ���
				//���ȸ���bitmap���ж���ζ�������ȡ�ķ�Χ��ȫ��Ϊת���ռ䣬����ȫ��Ϊδת���ռ䣬���߼����֮
				long tstResult = DPBitmapTest(DevExt->Bitmap, offset, length);
				switch (tstResult)
				{
				case BITMAP_RANGE_CLEAR: 
					//��˵����ζ�ȡ�Ĳ���ȫ���Ƕ�ȡδת���Ŀռ䣬Ҳ���������Ĵ����ϵ����ݣ�����ֱ�ӷ����²��豸ȥ����
					goto ERRNEXT;
				case BITMAP_RANGE_SET: 
					//��˵����ζ�ȡ�Ĳ���ȫ���Ƕ�ȡ�Ѿ�ת���Ŀռ䣬Ҳ���ǻ����ļ��ϵ����ݣ����Ǵ��ļ��ж�ȡ������Ȼ��ֱ��������irp
					//����һ�������������ӻ����ļ��ж�ȡ
					if (NULL == (fileBuf = (PBYTE)ExAllocatePoolWithTag(NonPagedPool, length, 'xypD')))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					RtlZeroMemory(fileBuf,length);
					ntStatus = ZwReadFile(
						DevExt->TempFile,
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
					else
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					break;

				case BITMAP_RANGE_BLEND:
					//��˵����ζ�ȡ�Ĳ����ǻ�ϵģ�����Ҳ��Ҫ���²��豸�ж�����ͬʱ���ļ��ж�����Ȼ���ϲ�����
					//����һ�������������ӻ����ļ��ж�ȡ
					if (NULL == (fileBuf = (PBYTE)ExAllocatePoolWithTag(NonPagedPool, length, 'xypD')))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					RtlZeroMemory(fileBuf,length);
					//����һ���������������²��豸�ж�ȡ
					if (NULL == (devBuf = (PBYTE)ExAllocatePoolWithTag(NonPagedPool, length, 'xypD')))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					RtlZeroMemory(devBuf,length);
					ntStatus = ZwReadFile(
						DevExt->TempFile,
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
					//�����irp�����²��豸ȥ��ȡ��Ҫ���豸�϶�ȡ����Ϣ
					ntStatus = DPForwardIrpSync(DevExt->LowerDevObj,Irp);
					if (!NT_SUCCESS(ntStatus))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					//�����²��豸��ȡ�������ݴ洢��devBuf��
					memcpy(devBuf, sysBuf, Irp->IoStatus.Information);
					//�Ѵ��ļ���ȡ�������ݺʹ��豸��ȡ�������ݸ�����Ӧ��bitmapֵ�����кϲ����ϲ��Ľ������devBuf��
					ntStatus = DPBitmapGet(
						DevExt->Bitmap,
						offset,
						length,
						devBuf,
						fileBuf
						);
					if (!NT_SUCCESS(ntStatus))
					{
						ntStatus = STATUS_INSUFFICIENT_RESOURCES;
						Irp->IoStatus.Information = 0;
						goto ERRERR;
					}
					//�Ѻϲ�������ݴ���ϵͳ���岢���irp
					memcpy(sysBuf, devBuf, Irp->IoStatus.Information);
					goto ERRCMPLT;
				default:
					ntStatus = STATUS_INSUFFICIENT_RESOURCES;
					goto ERRERR;
				}
			}
			else
			{
				//������д�Ĺ���
				//����д������ֱ��д�����ļ���������д�������ݣ��������ν��ת��������ת��֮����Ҫ��bitmap������Ӧ�ı��
				ntStatus = ZwWriteFile(
					DevExt->TempFile,
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
					if (NT_SUCCESS(ntStatus = DPBitmapSet(DevExt->Bitmap, offset, length)))
					{
						goto ERRCMPLT;
					}
					else
					{
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
			DPCompleteRequest(
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
			DPSendToNextDriver(
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
			DPCompleteRequest(
				Irp,
				STATUS_SUCCESS,
				IO_DISK_INCREMENT
				);
			continue;
			
		}
	}

}



NTSTATUS
DPAddDevice(
    IN	PDRIVER_OBJECT	DriverObject,
    IN	PDEVICE_OBJECT	PhysicalDeviceObject
    )
{
	//NTSTATUS���͵ĺ�������ֵ
	NTSTATUS					ntStatus = STATUS_SUCCESS;
    //����ָ������豸���豸��չ��ָ��
	PDP_FILTER_DEV_EXTENSION	DevExt = NULL;
	//�����豸���²��豸��ָ�����
	PDEVICE_OBJECT				LowerDevObj = NULL;
	//�����豸���豸ָ���ָ�����
	PDEVICE_OBJECT				FltDevObj = NULL;
	//�����豸�Ĵ����̵߳��߳̾��
	HANDLE						ThreadHandle = NULL;

	//����һ�������豸������豸��FILE_DEVICE_DISK���͵��豸���Ҿ���DP_FILTER_DEV_EXTENSION���͵��豸��չ
	ntStatus = IoCreateDevice(
		DriverObject,
		sizeof(DP_FILTER_DEV_EXTENSION),
		NULL,
		FILE_DEVICE_DISK,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&FltDevObj);
	if (!NT_SUCCESS(ntStatus)) 
		goto ERROUT;
	//��DevExtָ������豸���豸��չָ��
	DevExt = FltDevObj->DeviceExtension;
	//��չ����豸���豸��չ
	RtlZeroMemory(DevExt,sizeof(DP_FILTER_DEV_EXTENSION));

	//���ոս����Ĺ����豸���ӵ�������豸�������豸��
	LowerDevObj = IoAttachDeviceToDeviceStack(
		FltDevObj, 
		PhysicalDeviceObject);
	if (NULL == LowerDevObj)
	{
		ntStatus = STATUS_NO_SUCH_DEVICE;
		goto ERROUT;
	}

	//��ʼ��������豸�ķ�ҳ·�������ļ����¼�
	KeInitializeEvent(
		&DevExt->PagingPathCountEvent,
		NotificationEvent, 
		TRUE);

	//�Թ����豸���豸���Խ��г�ʼ���������豸���豸����Ӧ�ú������²��豸��ͬ
	FltDevObj->Flags = LowerDevObj->Flags;
	//�������豸���豸���Լ��ϵ�Դ�ɷ�ҳ������
	FltDevObj->Flags |= DO_POWER_PAGABLE;
	//�Թ����豸�����豸��ʼ��
	FltDevObj->Flags &= ~DO_DEVICE_INITIALIZING;

	//�������豸��Ӧ���豸��չ�е���Ӧ�������г�ʼ��
	//���豸�Ĺ����豸����
	DevExt->FltDevObj = FltDevObj;
	//���豸�������豸����
	DevExt->PhyDevObj = PhysicalDeviceObject;
	//���豸���²��豸����
	DevExt->LowerDevObj = LowerDevObj;

	//��ʼ�����������������
	InitializeListHead(&DevExt->ReqList);
	//��ʼ����������е���
	KeInitializeSpinLock(&DevExt->ReqLock);
	//��ʼ����������е�ͬ���¼�
	KeInitializeEvent(
		&DevExt->ReqEvent,
		SynchronizationEvent,
		FALSE
		);

	//��ʼ����ֹ�����̱߳�־
	DevExt->ThreadTermFlag = FALSE;
	//����������������������Ĵ����̣߳��̺߳����Ĳ��������豸��չ
	ntStatus = PsCreateSystemThread(
		&ThreadHandle,
		(ACCESS_MASK)0L,
		NULL,
		NULL,
		NULL,
		DPReadWriteThread,
		DevExt
		);
	if (!NT_SUCCESS(ntStatus))
		goto ERROUT;

	//��ȡ�����̵߳Ķ���
	ntStatus = ObReferenceObjectByHandle(
		ThreadHandle,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		&DevExt->ThreadHandle,
		NULL
		);
	if (!NT_SUCCESS(ntStatus))
	{
		DevExt->ThreadTermFlag = TRUE;
		KeSetEvent(
			&DevExt->ReqEvent,
			(KPRIORITY)0,
			FALSE
			);
		goto ERROUT;
	}

ERROUT:
	if (!NT_SUCCESS(ntStatus))
	{	
		//��������в��ɹ��ĵط���������Ҫ������ܴ��ڵĸ���
		if (NULL != LowerDevObj)
		{
			IoDetachDevice(LowerDevObj);
			DevExt->LowerDevObj = NULL;
		}
		//Ȼ��ɾ�����ܽ����Ĺ����豸
		if (NULL != FltDevObj)
		{
			IoDeleteDevice(FltDevObj);
			DevExt->FltDevObj = NULL;
		}
	}
	//�ر��߳̾�������ǽ�󲻻��õ��������ж��̵߳����ö�ͨ���̶߳�����������
	if (NULL != ThreadHandle)
		ZwClose(ThreadHandle);
	//����״ֵ̬
    return ntStatus;
}

VOID
DPUnload(
	IN	PDRIVER_OBJECT	DriverObject
	)
{
	//����������Ṥ����ϵͳ�ػ����������ǲ���������ж�ص�ʱ�����κ�����������Ϊ֮��ϵͳ���Ͼ͹ر���
	UNREFERENCED_PARAMETER(DriverObject);
	return;
}

NTSTATUS
DPDispatchAny(
    IN	PDEVICE_OBJECT	DeviceObject,
    IN	PIRP			Irp
    )
{
	//NTSTATUS���͵ĺ�������ֵ
	NTSTATUS					ntStatus = STATUS_SUCCESS;
	//����ָ������豸���豸��չ��ָ��
	PDP_FILTER_DEV_EXTENSION	DevExt = DeviceObject->DeviceExtension;
    //�������ǲ�����Ȥ��irp���󣬱�׼�Ĵ���ʽֱ���·����²��豸ȥ����
	return DPSendToNextDriver(
		DevExt->LowerDevObj,
		Irp);
}

NTSTATUS
DPDispatchPower(
    IN	PDEVICE_OBJECT	DeviceObject,
    IN	PIRP			Irp
    )
{
	//����ָ������豸���豸��չ��ָ��
	PDP_FILTER_DEV_EXTENSION	DevExt = DeviceObject->DeviceExtension;
#if (NTDDI_VERSION < NTDDI_VISTA)
	//�����vista��ǰ�İ汾��windows����Ҫʹ����������²��豸ת���ĺ���
	PoStartNextPowerIrp(Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver(DevExt->LowerDevObj, Irp);
#else
	//�����vistaϵͳ������ʹ�ú�һ���·�irpһ���ķ������·�
	return DPSendToNextDriver(
		DevExt->LowerDevObj,
		Irp);
#endif  
}

NTSTATUS	
DPDispatchPnp(
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PIRP			Irp
	)
{
	//����ָ������豸���豸��չ��ָ��
	PDP_FILTER_DEV_EXTENSION	DevExt = DeviceObject->DeviceExtension;
	//����ֵ
	NTSTATUS ntStatus = STATUS_SUCCESS;
	//����ָ��irp stack��ָ��
	PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation(Irp);

	switch(irpsp->MinorFunction) 
	{
	case IRP_MN_REMOVE_DEVICE:
		//�����PnP manager���������Ƴ��豸��irp������������
		{
			//������Ҫ��һЩ������
			if (DevExt->ThreadTermFlag != TRUE && NULL != DevExt->ThreadHandle)
			{
				//����̻߳������еĻ���Ҫֹͣ��������ͨ�������߳�ֹͣ���еı�־���ҷ����¼���Ϣ�����߳��Լ���ֹ����
				DevExt->ThreadTermFlag = TRUE;
				KeSetEvent(
					&DevExt->ReqEvent,
					(KPRIORITY) 0,
					FALSE
					);
				//�ȴ��߳̽���
				KeWaitForSingleObject(
					DevExt->ThreadHandle,
					Executive,
					KernelMode,
					FALSE,
					NULL
					);
				//��������̶߳���
				ObDereferenceObject(DevExt->ThreadHandle);
			}
	 		if (NULL != DevExt->Bitmap)
	 		{
				//�������λͼ�����ͷ�
	 			DPBitmapFree(DevExt->Bitmap);
	 		}
			if (NULL != DevExt->LowerDevObj)
			{
				//����������²��豸������ȥ���ҽ�
				IoDetachDevice(DevExt->LowerDevObj);
			}
	 		if (NULL != DevExt->FltDevObj)
	 		{
				//������ڹ����豸����Ҫɾ����
	 			IoDeleteDevice(DevExt->FltDevObj);
	 		}
			break;
		}
	//�����PnP ����������ѯ���豸�ܷ�֧�������ļ���irp����Ϊ��Ĺ������������Ǳ��봦��
	case IRP_MN_DEVICE_USAGE_NOTIFICATION:
		{
			BOOLEAN setPagable;
			//�����ѯ���Ƿ�֧�������ļ���dump�ļ�����ֱ���·����²��豸ȥ����
			if (irpsp->Parameters.UsageNotification.Type != DeviceUsageTypePaging) 
			{
				ntStatus = DPSendToNextDriver(
					DevExt->LowerDevObj,
					Irp);
				return ntStatus; 
			}
			//�����һ�·�ҳ�����¼�
			ntStatus = KeWaitForSingleObject(
				&DevExt->PagingPathCountEvent,
				Executive, 
				KernelMode,
				FALSE, 
				NULL);

			//setPagable��ʼ��Ϊ�٣���û�����ù�DO_POWER_PAGABLE����˼
			setPagable = FALSE;
			if (!irpsp->Parameters.UsageNotification.InPath &&
				DevExt->PagingPathCount == 1 ) 
			{
				//�����PnP manager֪ͨ���ǽ�Ҫɾȥ��ҳ�ļ���������Ŀǰֻʣ�����һ����ҳ�ļ���ʱ����������
				if (DeviceObject->Flags & DO_POWER_INRUSH)
				{} 
				else 
				{
					//������˵��û�з�ҳ�ļ�������豸���ˣ���Ҫ����DO_POWER_PAGABLE��һλ��
					DeviceObject->Flags |= DO_POWER_PAGABLE;
					setPagable = TRUE;
				}
			}
			//������϶��ǹ��ڷ�ҳ�ļ����Ƿ�ɽ�����ѯ��������ɾ����֪ͨ�����ǽ����²��豸ȥ����������Ҫ��ͬ���ķ�ʽ���²��豸��Ҳ����˵Ҫ�ȴ��²��豸�ķ���
			ntStatus = DPForwardIrpSync(DevExt->LowerDevObj,Irp);

			if (NT_SUCCESS(ntStatus)) 
			{
				//��������²��豸������ɹ��ˣ�˵���²��豸֧�������������ִ�е�����
				//�ڳɹ����������������ı������Լ��ļ���ֵ���������ܼ�¼������������豸�ϵ����ж��ٸ���ҳ�ļ�
				IoAdjustPagingPathCount(
					&DevExt->PagingPathCount,
					irpsp->Parameters.UsageNotification.InPath);
				if (irpsp->Parameters.UsageNotification.InPath) 
				{
					if (DevExt->PagingPathCount == 1) 
					{
						//������������һ��������ҳ�ļ��Ĳ�ѯ���󣬲����²��豸֧��������󣬶������ǵ�һ��������豸�ϵķ�ҳ�ļ�����ô������Ҫ���DO_POWER_PAGABLEλ
						DeviceObject->Flags &= ~DO_POWER_PAGABLE;
					}
				}
			}
			else 
			{
				//������˵�����²��豸������ʧ���ˣ��²��豸��֧�����������ʱ��������Ҫ��֮ǰ�����Ĳ�����ԭ
				if (setPagable == TRUE) 
				{
					//����setPagable������ֵ���ж�����֮ǰ�Ƿ�������DO_POWER_PAGABLE�����ã�����еĻ�������������
					DeviceObject->Flags &= ~DO_POWER_PAGABLE;
					setPagable = FALSE;
				}
			}
			//���÷�ҳ�����¼�
			KeSetEvent(
				&DevExt->PagingPathCountEvent,
				IO_NO_INCREMENT, 
				FALSE
				);
			//���������ǾͿ���������irp������
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			return ntStatus;
		}		
	default:
		break;
	}
	return DPSendToNextDriver(
		DevExt->LowerDevObj,
		Irp);

}

NTSTATUS DPQueryVolumeInformationCompletionRoutine(
	IN	PDEVICE_OBJECT		pDeviceObject,
	IN	PIRP				pIrp,
	IN	PVOID				Context
	)
{
	KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS DPQueryVolumeInformation(
	PDEVICE_OBJECT			DevObj,
	LARGE_INTEGER *			TotalSize,
	DWORD *					ClusterSize,
	DWORD *					SectorSize
	)
{
#define _FileSystemNameLength	64
//����FAT16�ļ�ϵͳǩ����ƫ����
#define FAT16_SIG_OFFSET	54
//����FAT32�ļ�ϵͳǩ����ƫ����
#define FAT32_SIG_OFFSET	82
//����NTFS�ļ�ϵͳǩ����ƫ����
#define NTFS_SIG_OFFSET		3
	//����FAT16�ļ�ϵͳ�ı�־
	const UCHAR FAT16FLG[4] = {'F','A','T','1'};
	//����FAT32�ļ�ϵͳ�ı�־
	const UCHAR FAT32FLG[4] = {'F','A','T','3'};
	//����NTFS�ļ�ϵͳ�ı�־
	const UCHAR NTFSFLG[4] = {'N','T','F','S'};
	//����ֵ
	NTSTATUS ntStatus = STATUS_SUCCESS;
	//������ȡ��DBR���������ݻ�����
	BYTE DBR[512] = { 0 };
	//DBR������512��bytes��С
	ULONG DBRLength = 512;
	//����������ָ�룬ͳһָ���ȡ��DBR���ݣ�����������ָ������ͷֱ����FAT16��FAT32��NTFS�����ļ�ϵͳ��DBR���ݽṹ
	PDP_NTFS_BOOT_SECTOR pNtfsBootSector = (PDP_NTFS_BOOT_SECTOR)DBR;
	PDP_FAT32_BOOT_SECTOR pFat32BootSector = (PDP_FAT32_BOOT_SECTOR)DBR;
	PDP_FAT16_BOOT_SECTOR pFat16BootSector = (PDP_FAT16_BOOT_SECTOR)DBR;
	//��ȡ��ƫ����������DBR��˵�Ǿ����ʼλ�ã�����ƫ����Ϊ0
	LARGE_INTEGER readOffset = { 0 };
	//��ȡʱ��io����״̬
	IO_STATUS_BLOCK ios;
 	//Ϊ��ͬ����ȡ�����õ�ͬ���¼�
 	KEVENT Event;
 	//Ϊ��ͬ����ȡ����Ҫ������irpָ��
 	PIRP   pIrp	= NULL;

	//�����������ȴ�ָ���ľ��豸�϶�ȡƫ����Ϊ0��һ��������Ҳ����������DBR������׼�����Է���
 	//��Ϊ����Ҫͬ����ȡ�������ȳ�ʼ��һ��Ϊ��ͬ����ȡ���õ��¼�
 	KeInitializeEvent(&Event, NotificationEvent, FALSE);
 	//����һ��irp�����������豸����ȡ��Ϣ
 	pIrp = IoBuildAsynchronousFsdRequest(
 		IRP_MJ_READ,
 		DevObj,
 		DBR,
 		DBRLength,
 		&readOffset,
 		&ios
 		);
 	if (NULL == pIrp)
 	{
 		goto ERROUT;
 	}
 	//������ɺ��������ҽ�ͬ���¼���Ϊ��ɺ����Ĳ�������
 	IoSetCompletionRoutine(
 		pIrp,
 		DPQueryVolumeInformationCompletionRoutine,
 		&Event,
 		TRUE,
 		TRUE,
 		TRUE
 		);
 	//����Ŀ���豸ȥ�������irp
 	ntStatus = IoCallDriver(DevObj, pIrp);
 	if(ntStatus = STATUS_PENDING)
 	{
 		//����²��豸һʱ����������irp�������Ǿ͵�
 		ntStatus = KeWaitForSingleObject(
 			&Event,
 			Executive,
 			KernelMode,
 			FALSE,
 			NULL
 			);
 		//������ֵ����Ϊ���io������״̬
 		ntStatus = pIrp->IoStatus.Status;
 		if (!NT_SUCCESS(ntStatus))
 		{
 			goto ERROUT;
 		}
 	}
	
	if (*(DWORD*)NTFSFLG == *(DWORD*)&DBR[NTFS_SIG_OFFSET])
	{
		//ͨ���Ƚϱ�־�����������һ��ntfs�ļ�ϵͳ�ľ��������ntfs���DBR�������Ը�����Ҫ��ȡ��ֵ���и�ֵ����
		*SectorSize = (DWORD)(pNtfsBootSector->BytesPerSector);
		*ClusterSize = (*SectorSize) * (DWORD)(pNtfsBootSector->SectorsPerCluster);    
		TotalSize->QuadPart = (LONGLONG)(*SectorSize) * (LONGLONG)pNtfsBootSector->TotalSectors;
	}
	else if (*(DWORD*)FAT32FLG == *(DWORD*)&DBR[FAT32_SIG_OFFSET])
	{
		//ͨ���Ƚϱ�־�����������һ��ntfs�ļ�ϵͳ�ľ��������ntfs���DBR�������Ը�����Ҫ��ȡ��ֵ���и�ֵ����
		*SectorSize = (DWORD)(pFat32BootSector->BytesPerSector);
		*ClusterSize = (*SectorSize) * (DWORD)(pFat32BootSector->SectorsPerCluster);    
		TotalSize->QuadPart = (LONGLONG)(*SectorSize) * 
			(LONGLONG)(pFat32BootSector->LargeSectors + pFat32BootSector->Sectors);
	}
	else if (*(DWORD*)FAT16FLG == *(DWORD*)&DBR[FAT16_SIG_OFFSET])
	{
		//ͨ���Ƚϱ�־�����������һ��ntfs�ļ�ϵͳ�ľ��������ntfs���DBR�������Ը�����Ҫ��ȡ��ֵ���и�ֵ����
		*SectorSize = (DWORD)(pFat16BootSector->BytesPerSector);
		*ClusterSize = (*SectorSize) * (DWORD)(pFat16BootSector->SectorsPerCluster);    
		TotalSize->QuadPart = (LONGLONG)(*SectorSize) * 
			(LONGLONG)(pFat16BootSector->LargeSectors + pFat16BootSector->Sectors);
	}
	else
	{
		//�ߵ���������������κ��ļ�ϵͳ�����ǲ���windows��ʶ���ļ�ϵͳ������ͳһ���ش�
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
DPVolumeOnLineCompleteRoutine(
	IN PDEVICE_OBJECT  DeviceObject,
	IN PIRP  Irp,
	IN PVOLUME_ONLINE_CONTEXT  Context
	)
{
	//����ֵ
	NTSTATUS ntStatus = STATUS_SUCCESS;
	//������豸��dos���֣�Ҳ����C��D��
	UNICODE_STRING		DosName = { 0 };

	//������Context�ǲ�����Ϊ�յģ�Ϊ�վ��ǳ�����
	ASSERT(Context!=NULL);
	//������������Լ���VolumeOnline����
	//��ȡ������dos����
	ntStatus = IoVolumeDeviceToDosName(Context->DevExt->PhyDevObj, &DosName);
	if (!NT_SUCCESS(ntStatus))
		goto ERROUT;
	//��dos���ֱ�ɴ�д��ʽ
	Context->DevExt->VolumeLetter = DosName.Buffer[0];
	if (Context->DevExt->VolumeLetter > L'Z')
		Context->DevExt->VolumeLetter -= (L'a' - L'A');
	//����ֻ������D����
	if (Context->DevExt->VolumeLetter == L'D')
	{
		//��ȡ�����Ļ�����Ϣ
		ntStatus = DPQueryVolumeInformation(
			Context->DevExt->PhyDevObj,
			&(Context->DevExt->TotalSizeInByte),
			&(Context->DevExt->ClusterSizeInByte),
			&(Context->DevExt->SectorSizeInByte));
		if (!NT_SUCCESS(ntStatus))
		{
			goto ERROUT;
		}
		//����������Ӧ��λͼ
		ntStatus = DPBitmapInit(
			&Context->DevExt->Bitmap,
			Context->DevExt->SectorSizeInByte,
			8,
			25600,
			(DWORD)(Context->DevExt->TotalSizeInByte.QuadPart / 
			(LONGLONG)(25600 * 8 * Context->DevExt->SectorSizeInByte)) + 1);
		if (!NT_SUCCESS(ntStatus))
			goto ERROUT;
		//��ȫ������ֵ��˵�������ҵ���Ҫ�������Ǹ��豸��
		gProtectDevExt = Context->DevExt;
	}
	
ERROUT:
	if (!NT_SUCCESS(ntStatus))
	{
		if (NULL != Context->DevExt->Bitmap)
		{
			DPBitmapFree(Context->DevExt->Bitmap);
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
	//���õȴ�ͬ���¼����������������ǵȴ���DeviceIoControl������̼�������
	KeSetEvent(
		Context->Event,
		0,
		FALSE);
	return STATUS_SUCCESS;
}

NTSTATUS
DPDispatchDeviceControl(
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PIRP			Irp
	)
{
	//����ָ������豸���豸��չ��ָ��
	PDP_FILTER_DEV_EXTENSION	DevExt = DeviceObject->DeviceExtension;
	//����ֵ
	NTSTATUS ntStatus = STATUS_SUCCESS;
	//����ָ��irp stack��ָ��
	PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation(Irp);
	//����ͬ��IOCTL_VOLUME_ONLINE������¼�
	KEVENT					Event;
	//��������IOCTL_VOLUME_ONLINE����ɺ�����������
	VOLUME_ONLINE_CONTEXT	context;

	switch (irpsp->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_VOLUME_ONLINE:
		{
			//����Ǿ��豸��IOCTL_VOLUME_ONLINE������뵽����
			//���Ǵ����Լ��������irp���������ȳ�ʼ��һ���¼�����������������ɺ���������ͬ���ź�
			KeInitializeEvent(&Event, NotificationEvent, FALSE);
			//������������ɺ�����ʼ������
			context.DevExt = DevExt;
			context.Event = &Event;
			//����copyһ��irp stack
			IoCopyCurrentIrpStackLocationToNext(Irp);
			//������ɺ���
			IoSetCompletionRoutine(
				Irp, 
				DPVolumeOnLineCompleteRoutine, 
				&context,
				TRUE,
				TRUE,
				TRUE);
			//�����²��豸���������irp
			ntStatus = IoCallDriver(DevExt->LowerDevObj, Irp);
			//�ȴ��²��豸����������irp
			KeWaitForSingleObject(
				&Event,
				Executive,
				KernelMode,
				FALSE,
				NULL);
			//����
			return ntStatus;
		}
	default:
		//��������DeviceIoControl������һ�ɵ����²��豸ȥ����
		break;
	}
	return DPSendToNextDriver(DevExt->LowerDevObj,Irp);		
}

NTSTATUS
DPDispatchReadWrite(
    IN	PDEVICE_OBJECT	DeviceObject,
    IN	PIRP			Irp
    )
{	
	//����ָ������豸���豸��չ��ָ��
	PDP_FILTER_DEV_EXTENSION	DevExt = DeviceObject->DeviceExtension;
	//����ֵ
	NTSTATUS ntStatus = STATUS_SUCCESS;

	if (DevExt->Protect)
	{
		//������ڱ���״̬��
		//�������Ȱ����irp��Ϊpending״̬
		IoMarkIrpPending(Irp);
		//Ȼ�����irp�Ž���Ӧ�����������
		ExInterlockedInsertTailList(
			&DevExt->ReqList,
			&Irp->Tail.Overlay.ListEntry,
			&DevExt->ReqLock
			);
		//���ö��еĵȴ��¼���֪ͨ���ж����irp���д���
		KeSetEvent(
			&DevExt->ReqEvent, 
			(KPRIORITY)0, 
			FALSE);
		//����pending״̬�����irp���㴦������
		return STATUS_PENDING;
	}
	else
	{
		//������ڱ���״̬��ֱ�ӽ����²��豸���д���
		return DPSendToNextDriver(
			DevExt->LowerDevObj,
			Irp);
	}
}
