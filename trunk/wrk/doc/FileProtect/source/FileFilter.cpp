#ifdef __cplusplus
extern "C"
{
#endif

#include "FileFilter.h"
#include "FastIoFlt.h"

#include <ntstrsafe.h>
#pragma comment (lib, "ntstrsafe.lib")

#ifdef __cplusplus
};
#endif


PDEVICE_OBJECT g_FileFltDev;
PDRIVER_OBJECT g_FileFltDrv;
FAST_MUTEX g_FltAttachLock;

#define GET_DEVICE_TYPE_NAME( _type ) \
	((((_type) > 0) && ((_type) < (sizeof(DeviceTypeNames) / sizeof(PCHAR)))) ? \
	DeviceTypeNames[ (_type) ] : \
	"[Unknown]")

//
//  Known device type names
//

static const PCHAR DeviceTypeNames[] = {
	"",
	"BEEP",
	"CD_ROM",
	"CD_ROM_FILE_SYSTEM",
	"CONTROLLER",
	"DATALINK",
	"DFS",
	"DISK",
	"DISK_FILE_SYSTEM",
	"FILE_SYSTEM",
	"INPORT_PORT",
	"KEYBOARD",
	"MAILSLOT",
	"MIDI_IN",
	"MIDI_OUT",
	"MOUSE",
	"MULTI_UNC_PROVIDER",
	"NAMED_PIPE",
	"NETWORK",
	"NETWORK_BROWSER",
	"NETWORK_FILE_SYSTEM",
	"NULL",
	"PARALLEL_PORT",
	"PHYSICAL_NETCARD",
	"PRINTER",
	"SCANNER",
	"SERIAL_MOUSE_PORT",
	"SERIAL_PORT",
	"SCREEN",
	"SOUND",
	"STREAMS",
	"TAPE",
	"TAPE_FILE_SYSTEM",
	"TRANSPORT",
	"UNKNOWN",
	"VIDEO",
	"VIRTUAL_DISK",
	"WAVE_IN",
	"WAVE_OUT",
	"8042_PORT",
	"NETWORK_REDIRECTOR",
	"BATTERY",
	"BUS_EXTENDER",
	"MODEM",
	"VDM",
	"MASS_STORAGE",
	"SMB",
	"KS",
	"CHANGER",
	"SMARTCARD",
	"ACPI",
	"DVD",
	"FULLSCREEN_VIDEO",
	"DFS_FILE_SYSTEM",
	"DFS_VOLUME",
	"SERENUM",
	"TERMSRV",
	"KSEC"
};



VOID FileFltNotification(PDEVICE_OBJECT pDevObj, BOOLEAN FsActive)
{
	WCHAR nameBuf[MAX_DEVICE_NAME];
	UNICODE_STRING devName;
	UNICODE_STRING fsRecName;

	RtlInitUnicodeString(&fsRecName, L"\\FileSystem\\Fs_Rec");
	RtlInitEmptyUnicodeString(&devName, nameBuf, MAX_DEVICE_NAME*sizeof(WCHAR));

	FZGetDevName(pDevObj, &devName);

	KdPrint(("FsActive %d,Device Type %s,Device Name %ws, Driver Name %ws, \n",FsActive, GET_DEVICE_TYPE_NAME(pDevObj->DeviceType),\
		devName.Buffer, pDevObj->DriverObject->DriverName.Buffer));


	if (RtlCompareUnicodeString(&devName, &fsRecName, TRUE) == 0)
	{
		return;
	}

	if (FsActive)
	{
		//ò���ڴ�û��attach������豸����
		FZEnumerateAttach(pDevObj);
	}
	else
	{
		//IoDetachDevice(pDevObj);
		FZEnumerateDetach(pDevObj);
	}
}

NTSTATUS FZGetDevName(PDEVICE_OBJECT pDevObj, PUNICODE_STRING pDevName)
{
	/*
	POBJECT_HEADER pObjHeader;
	POBJECT_HEADER_NAME_INFO pObjHeaderNameInfo;

	pObjHeader = OBJECT_TO_OBJECT_HEADER(pDevObj);

	if (pObjHeader)
	{
		pObjHeaderNameInfo = OBJECT_HEADER_TO_NAME_INFO(pObjHeader);

		if (pObjHeaderNameInfo && pObjHeaderNameInfo->Name.Buffer)
		{
			//wcscpy(pDevName, pObjHeaderNameInfo->Name.Buffer);
			RtlCopyUnicodeString(pDevName, &pObjHeaderNameInfo->Name);

			KdPrint(("Dev Name %ws",pObjHeaderNameInfo->Name.Buffer));

			return STATUS_SUCCESS;
		}
	}

	return STATUS_UNSUCCESSFUL;*/
	if (pDevObj == NULL || pDevName==NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	NTSTATUS ntStatus;
	CHAR nibuf[512];        //buffer that receives NAME information and name
	POBJECT_NAME_INFORMATION nameInfo = (POBJECT_NAME_INFORMATION)nibuf;
	ULONG retLength;

	ntStatus = ObQueryNameString( pDevObj, nameInfo, sizeof(nibuf), &retLength);

	pDevName->Length = 0;
	if (NT_SUCCESS( ntStatus )) {

		RtlCopyUnicodeString( pDevName, &nameInfo->Name );
	}

	return ntStatus;
}

#ifdef __cplusplus
extern "C"
{
#endif

NTSTATUS FileFilterInit(IN PDRIVER_OBJECT pDrvObj)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	g_FileFltDrv = pDrvObj;

	UNICODE_STRING usFltName;
	RtlInitUnicodeString(&usFltName, L"\\FileSystem\\Filters\\FZFileFlt");

	ntStatus = IoCreateDevice(pDrvObj, 0, &usFltName,
						FILE_DEVICE_DISK_FILE_SYSTEM,  //��Ա��ش��̲����� ���������� ���� CD_ROM
						FILE_DEVICE_SECURE_OPEN,
						FALSE, &g_FileFltDev);  


	if (ntStatus == STATUS_OBJECT_PATH_NOT_FOUND)
	{
		RtlInitUnicodeString(&usFltName, L"\\FileSystem\\FZFileFlt");

		ntStatus = IoCreateDevice(pDrvObj, 0, &usFltName,
			FILE_DEVICE_DISK_FILE_SYSTEM,  //��Ա��ش��̲����� ���������� ���� CD_ROM
			FILE_DEVICE_SECURE_OPEN,
			FALSE, &g_FileFltDev);
	}

	if (ntStatus != STATUS_SUCCESS)
	{
		return ntStatus;
	}

	ExInitializeFastMutex(&g_FltAttachLock);

	ntStatus = IoRegisterFsRegistrationChange(pDrvObj, FileFltNotification);

	//����û��Attach�������豸�� ��û��ֱ��Attach RawDisk RawCdrom
	for (int i= 0; i<= IRP_MJ_MAXIMUM_FUNCTION -1; i++)
	{
		pDrvObj->MajorFunction[i] = FZFileDefaultFilter;
	}
			

	//ע�� �����Ϊ��ʵ�� �ļ��б���
	pDrvObj->MajorFunction[IRP_MJ_CREATE] = FZFltCreate;   //�����ļ�
	pDrvObj->MajorFunction[IRP_MJ_SET_INFORMATION] = FZFltSetInfo; //ɾ�� �������ļ�
	pDrvObj->MajorFunction[IRP_MJ_WRITE] = FZFltWrite;	//��д�ļ�

    //ע�⿼��FastIo�����
	FAST_IO_DISPATCH* pFastIoDispatch = (FAST_IO_DISPATCH*)ExAllocatePoolWithTag(NonPagedPool, 
									sizeof(FAST_IO_DISPATCH), FileFltTag);


	RtlZeroMemory(pFastIoDispatch, sizeof(FAST_IO_DISPATCH));

	pFastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);

	pFastIoDispatch->FastIoCheckIfPossible = FZFastIoCheckPossible;
	pFastIoDispatch->FastIoQueryBasicInfo = FZFastIoQueryBasicInfo;
	pFastIoDispatch->FastIoQueryStandardInfo = FZFastIoQueryStandardInfo;
	pFastIoDispatch->FastIoRead = FZFastIoRead;
	pFastIoDispatch->FastIoWrite = FZFastIoWrite;
	pFastIoDispatch->FastIoReadCompressed = FZFastIoReadCompressed;
	pFastIoDispatch->FastIoWriteCompressed = FZFastIoWriteCompressed;
	pFastIoDispatch->FastIoQueryOpen = FZFastIoQueryOpen;
	pFastIoDispatch->FastIoQueryNetworkOpenInfo = FZFastIoQueryNetworkOpenInfo;
	pFastIoDispatch->FastIoLock = FZFastIoLock;
	pFastIoDispatch->FastIoUnlockSingle = FZFastIoUnlockSingle;
	pFastIoDispatch->FastIoUnlockAll = FZFastIoUnlockAll;
	pFastIoDispatch->FastIoUnlockAllByKey = FZFastIoUnlockAllByKey;
	pFastIoDispatch->FastIoDeviceControl = FZFastIoDeviceControl;
	pFastIoDispatch->FastIoDetachDevice = FZFastIoDetachDevice;
	pFastIoDispatch->MdlRead = FZFastIoMdlRead;
	pFastIoDispatch->MdlReadComplete = FZFastIoMdlReadComplete;
	pFastIoDispatch->MdlWriteComplete = FZFastIoMdlWriteComplete;
	pFastIoDispatch->MdlReadCompleteCompressed = FZFastIoMdlReadCompleteCompressed;
	pFastIoDispatch->MdlWriteCompleteCompressed = FZFastIoMdlWriteCompleteCompressed;
	pFastIoDispatch->PrepareMdlWrite = FZFastIoPrepareMdlWrite;

	pDrvObj->FastIoDispatch = pFastIoDispatch;

	ClearFlag(g_FileFltDev->Flags, DO_DEVICE_INITIALIZING);

	return STATUS_SUCCESS;
}
#ifdef __cplusplus
};
#endif





BOOLEAN FZCheckFileName(PDEVICE_OBJECT pDevObj, WCHAR* fileName)
{
	if (pDevObj->DriverObject != g_FileFltDrv || pDevObj->DeviceExtension == NULL)
	{
		return FALSE;
	}

	FILE_FILTER_EXT* pFileFlt = (FILE_FILTER_EXT*)pDevObj->DeviceExtension;

	WCHAR wFullPath[MAX_DEVICE_NAME];
	//lstrcpyW(wFullPath, pFileFlt->StorageDevName);
	RtlStringCbCopyW(wFullPath, MAX_DEVICE_NAME*sizeof(WCHAR),pFileFlt->StorageDevName);
	//������StorageDevName�������SymbolicName?����
	//(wFullPath, fileName);

	if(fileName != NULL)
		RtlStringCbCatW(wFullPath, MAX_DEVICE_NAME*sizeof(WCHAR), fileName);

	//
	if (wcsstr(wFullPath, wProtectPath))
	{
		return TRUE;
	}
	else
		return FALSE;
}

BOOLEAN FZCheckProcessList()
{
	return FALSE;
}

NTSTATUS FZFltCreate(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	PIO_STACK_LOCATION pIoCurStack = IoGetCurrentIrpStackLocation(pIrp);
	PFILE_OBJECT pFileObj = pIoCurStack->FileObject;
	ULONG CreateOptions = ((pIoCurStack->Parameters.Create.Options>>24)&0x000000ff);

	KdPrint(("File Name %ws\n", pFileObj->FileName.Buffer));

	//������FsRegistrationʱû�д���unnamed���豸�󶨵�ԭFileSystem��CDO��
	//����IRP_MJ_FILE_SYSTEM_CONTROL�޷���� ��ζ���޷��󶨵��������ļ�ϵͳ������FILE_SYSTEM_OBJECT
	if (pDevObj == g_FileFltDev)
	{
		pIrp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
		pIrp->IoStatus.Information = 0;

		IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		return STATUS_INVALID_DEVICE_REQUEST;
	}
	
	//���������Ҫ�������ļ��� �Ҳ����� ������������Ľ���
	//�鿴��Create ���� Open Open���� ��Create������
	if (CreateOptions == FILE_CREATE || CreateOptions == FILE_OPEN_IF ||
		CreateOptions == FILE_OVERWRITE || CreateOptions == FILE_OVERWRITE_IF)
	{
		if (FZCheckFileName(pDevObj, pFileObj->FileName.Buffer)&& !FZCheckProcessList())
		{
			pIrp->IoStatus.Status = STATUS_ACCESS_DENIED;
			pIrp->IoStatus.Information = 0;

			IoCompleteRequest(pIrp, IO_NO_INCREMENT);

			return STATUS_ACCESS_DENIED;
		}
	}

	//���� ��������
	IoSkipCurrentIrpStackLocation(pIrp);

	PDEVICE_OBJECT pNextDevObj = ((FILE_FILTER_EXT*)pDevObj->DeviceExtension)->AttachToDevice;

	return IoCallDriver(pNextDevObj, pIrp);
}

//��������ɾ��
NTSTATUS FZFltSetInfo(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PIO_STACK_LOCATION pIoCurStack = IoGetCurrentIrpStackLocation(pIrp);
	PFILE_OBJECT pFileObj = pIoCurStack->FileObject;

	if (pIoCurStack->Parameters.SetFile.FileInformationClass == FileRenameInformation)
	{
		//����Ƿ��ǶԱ����ļ������ļ���Rename
		if (FZCheckFileName(pDevObj, pFileObj->FileName.Buffer)&& !FZCheckProcessList())
		{
			//ֱ�Ӿܾ�
			pIrp->IoStatus.Status = STATUS_ACCESS_DENIED;
			pIrp->IoStatus.Information = 0;
			
			IoCompleteRequest(pIrp, IO_NO_INCREMENT);
			return STATUS_ACCESS_DENIED;
		}
	}

	if (pIoCurStack->Parameters.SetFile.FileInformationClass == FileDispositionInformation &&
		   pIoCurStack->Parameters.SetFile.DeleteHandle == pFileObj)
	{
		if (FZCheckFileName(pDevObj, pFileObj->FileName.Buffer)&& !FZCheckProcessList())
		{
			//ֱ�Ӿܾ�
			pIrp->IoStatus.Status = STATUS_ACCESS_DENIED;
			pIrp->IoStatus.Information = 0;

			IoCompleteRequest(pIrp, IO_NO_INCREMENT);
			return STATUS_ACCESS_DENIED;
		}
	}

	//���� ���´���
	IoSkipCurrentIrpStackLocation(pIrp);

	PDEVICE_OBJECT pNextDevObj = ((FILE_FILTER_EXT*)pDevObj->DeviceExtension)->AttachToDevice;

	return IoCallDriver(pNextDevObj, pIrp);
}

NTSTATUS FZFltWrite(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PIO_STACK_LOCATION pIoCurStack = IoGetCurrentIrpStackLocation(pIrp);
	PFILE_OBJECT pFileObj = pIoCurStack->FileObject;

	if (FZCheckFileName(pDevObj, pFileObj->FileName.Buffer) && !FZCheckProcessList())
	{
		//ֱ�Ӿܾ�
		pIrp->IoStatus.Status = STATUS_ACCESS_DENIED;
		pIrp->IoStatus.Information = 0;

		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_ACCESS_DENIED;
	}

	IoSkipCurrentIrpStackLocation(pIrp);

	PDEVICE_OBJECT pNextDevObj = ((FILE_FILTER_EXT*)pDevObj->DeviceExtension)->AttachToDevice;

	return IoCallDriver(pNextDevObj, pIrp);
}


//��FilterĬ�ϵ�Dispatch����
NTSTATUS FZFileDefaultFilter(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp )
{
	IoSkipCurrentIrpStackLocation(pIrp);

	return IoCallDriver(((FILE_FILTER_EXT*)pDevObj->DeviceExtension)->AttachToDevice, pIrp);
}

//Ϊ��Attach����Ӧ��FileSystemObj�ϣ�  ���ж��Attach
//ע���FileSystemObj��Mountʱ�� �ļ�ϵͳ��������
NTSTATUS FZAttachFileSystemObj(PDEVICE_OBJECT pFltDevObj, PDEVICE_OBJECT pFileSystemObj)
{
	FILE_FILTER_EXT* pDevExt = (FILE_FILTER_EXT*)pFltDevObj->DeviceExtension;

	if (FlagOn(pFileSystemObj->Flags, DO_BUFFERED_IO))
	{
		SetFlag(pFltDevObj->Flags, DO_BUFFERED_IO);
	}

	if (FlagOn(pFileSystemObj->Flags, DO_DIRECT_IO))
	{
		SetFlag(pFltDevObj->Flags, DO_DIRECT_IO);
	}

	//
	for (int i=0; i<= 7; i++)
	{
		LARGE_INTEGER li;

		pDevExt->AttachToDevice = IoAttachDeviceToDeviceStack(pFltDevObj, pFileSystemObj);

		if (pDevExt->AttachToDevice != NULL)
		{
			ClearFlag(pFltDevObj->Flags, DO_DEVICE_INITIALIZING);

			return STATUS_SUCCESS;
		}

		//�ӳ�1/2�� ���а�
		li.QuadPart = 500 * (-10*1000);
		KeDelayExecutionThread(KernelMode, FALSE, &li);
	}

	return STATUS_UNSUCCESSFUL;
}

//���pDevObj���ڵ��豸ջ���Ƿ��е�ǰDriver������DeviceObject
BOOLEAN FZIsAttachedDevice(IN PDEVICE_OBJECT pDevObj)
{
	//����豸ջ����Device
	PDEVICE_OBJECT pTopDevObj = IoGetAttachedDeviceReference(pDevObj);

	PDEVICE_OBJECT pCurDevObj = pTopDevObj;
	PDEVICE_OBJECT pLowerDevObj = NULL;

	WCHAR devNameBuf[MAX_DEVICE_NAME];
	UNICODE_STRING devName;

	RtlInitEmptyUnicodeString(&devName, devNameBuf, MAX_DEVICE_NAME);

	//�Ӹ�Device��ʼ���±��� �鿴�Ƿ��ǵ�ǰ��Driver������
	while (pCurDevObj != NULL)
	{
		FZGetDevName(pCurDevObj, &devName);

		KdPrint(("device Name %ws\n", devName.Buffer));

		if (pCurDevObj->DriverObject == g_FileFltDrv)
		{
			ObDereferenceObject(pCurDevObj);
			return TRUE;
		}
		
		pLowerDevObj = IoGetLowerDeviceObject(pCurDevObj);

		ObDereferenceObject(pCurDevObj);
		pCurDevObj = pLowerDevObj;
	}

	return FALSE;
}

NTSTATUS FZEnumerateDetach(IN PDEVICE_OBJECT pDevObj)
{
	return STATUS_SUCCESS;
}


//ö��ĳ��DeviceObject(File System�Ŀ����豸)������������FileSystemDeviceObject ��Attach��
NTSTATUS FZEnumerateAttach(IN PDEVICE_OBJECT pDevObj)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	ULONG ulDevNum = 0;
	WCHAR devNameBuf[MAX_DEVICE_NAME];

	UNICODE_STRING devName;
	RtlInitEmptyUnicodeString(&devName, devNameBuf, MAX_DEVICE_NAME);

	IoEnumerateDeviceObjectList(pDevObj->DriverObject, NULL, 0, &ulDevNum);

	PDEVICE_OBJECT * pDevList = (PDEVICE_OBJECT *)ExAllocatePoolWithTag(NonPagedPool, ulDevNum*sizeof(PDEVICE_OBJECT), FileFltTag);
	
	if (pDevList == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}


	ntStatus = IoEnumerateDeviceObjectList(pDevObj->DriverObject, pDevList, ulDevNum*sizeof(PDEVICE_OBJECT), &ulDevNum);

	if (ntStatus != STATUS_SUCCESS)
	{
		ExFreePoolWithTag(pDevList, FileFltTag);

		return ntStatus;
	}

	for (int i=0; i<=ulDevNum -1 ;i++)
	{
		//���ַ�ʽ:  1.�󶨵�Ntfs�����豸�ϣ�
		//��û�á�����Ϊ�����豸��һֱ���ڵģ�����IRP�Ƿ��͸���Ӧ��DeviceObject�� �����ǿ��Ƶ�DeviceObject 
		// �����豸��DriverEntry�д��� ������ʶ��Driver ������ ��DeviceExtension
		// �����豸������IoAttachDeviceʱ����һ����Ӧ��DeviceObject��
		//2. �󶨵�Ntfs��Ӧ��uname���豸�� �Ը��豸��ͨ���ļ�ϵͳ���������� ���ܻ��
		//3. �󶨵���Ӧ��Volume���豸�� ��ʹ�ǶԸ��豸���ƹ��ļ�ϵͳ������ Ҳ�ܻ��
		PDEVICE_OBJECT pStorageDev;

		FZGetDevName(pDevObj, &devName);
		KdPrint(("device name %ws\n", devName.Buffer));

		if (pDevList[i] == pDevObj || pDevList[i]->DeviceType != pDevObj->DeviceType ||
			FZIsAttachedDevice(pDevObj))
		{
			continue;			
		}

		ExAcquireFastMutex(&g_FltAttachLock);

		if (!FZIsAttachedDevice(pDevObj))
		{
			ntStatus = IoCreateDevice(g_FileFltDrv, sizeof(FILE_FILTER_EXT), 
								NULL, pDevList[i]->DeviceType, 0, FALSE, &pDevObj);			

			if (ntStatus == STATUS_SUCCESS)
			{
				FILE_FILTER_EXT * pFltDevExt=(FILE_FILTER_EXT *)pDevObj->DeviceExtension;

				ntStatus = IoGetDiskDeviceObject(pDevList[i], &pStorageDev);

				pFltDevExt->AttachToDevice = pDevList[i];
				pFltDevExt->StorageDevObj = pStorageDev;

				RtlInitEmptyUnicodeString(&pFltDevExt->AttachToDevName, pFltDevExt->StorageDevName, 
					sizeof(pFltDevExt->StorageDevName));

				if(NT_SUCCESS(ntStatus))
					FZGetDevName(pFltDevExt->StorageDevObj, &pFltDevExt->AttachToDevName);
				
				//Attach����Ӧ��FileSystemDeviceObject��
				FZAttachFileSystemObj(pDevObj, pDevList[i]);
			}
		}

		ExReleaseFastMutex(&g_FltAttachLock);
	}

	ExFreePoolWithTag(pDevList, FileFltTag);

	return STATUS_SUCCESS;
}