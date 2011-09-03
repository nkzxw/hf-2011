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
		//貌似在此没有attach其控制设备。。
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
						FILE_DEVICE_DISK_FILE_SYSTEM,  //针对本地磁盘操作的 而不是网络 或者 CD_ROM
						FILE_DEVICE_SECURE_OPEN,
						FALSE, &g_FileFltDev);  


	if (ntStatus == STATUS_OBJECT_PATH_NOT_FOUND)
	{
		RtlInitUnicodeString(&usFltName, L"\\FileSystem\\FZFileFlt");

		ntStatus = IoCreateDevice(pDrvObj, 0, &usFltName,
			FILE_DEVICE_DISK_FILE_SYSTEM,  //针对本地磁盘操作的 而不是网络 或者 CD_ROM
			FILE_DEVICE_SECURE_OPEN,
			FALSE, &g_FileFltDev);
	}

	if (ntStatus != STATUS_SUCCESS)
	{
		return ntStatus;
	}

	ExInitializeFastMutex(&g_FltAttachLock);

	ntStatus = IoRegisterFsRegistrationChange(pDrvObj, FileFltNotification);

	//这里没有Attach到控制设备上 故没有直接Attach RawDisk RawCdrom
	for (int i= 0; i<= IRP_MJ_MAXIMUM_FUNCTION -1; i++)
	{
		pDrvObj->MajorFunction[i] = FZFileDefaultFilter;
	}
			

	//注意 这个是为了实现 文件夹保护
	pDrvObj->MajorFunction[IRP_MJ_CREATE] = FZFltCreate;   //创建文件
	pDrvObj->MajorFunction[IRP_MJ_SET_INFORMATION] = FZFltSetInfo; //删除 重命名文件
	pDrvObj->MajorFunction[IRP_MJ_WRITE] = FZFltWrite;	//读写文件

    //注意考虑FastIo的情况
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
	//怎样从StorageDevName获得它的SymbolicName?不解
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

	//由于在FsRegistration时没有创建unnamed的设备绑定到原FileSystem的CDO上
	//导致IRP_MJ_FILE_SYSTEM_CONTROL无法获得 意味着无法绑定到新增的文件系统对象上FILE_SYSTEM_OBJECT
	if (pDevObj == g_FileFltDev)
	{
		pIrp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
		pIrp->IoStatus.Information = 0;

		IoCompleteRequest(pIrp, IO_NO_INCREMENT);

		return STATUS_INVALID_DEVICE_REQUEST;
	}
	
	//如果是我们要保护的文件夹 且操作者 并非我们允许的进程
	//查看是Create 还是 Open Open允许 但Create不允许
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

	//否则 正常传递
	IoSkipCurrentIrpStackLocation(pIrp);

	PDEVICE_OBJECT pNextDevObj = ((FILE_FILTER_EXT*)pDevObj->DeviceExtension)->AttachToDevice;

	return IoCallDriver(pNextDevObj, pIrp);
}

//重命名或删除
NTSTATUS FZFltSetInfo(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PIO_STACK_LOCATION pIoCurStack = IoGetCurrentIrpStackLocation(pIrp);
	PFILE_OBJECT pFileObj = pIoCurStack->FileObject;

	if (pIoCurStack->Parameters.SetFile.FileInformationClass == FileRenameInformation)
	{
		//检查是否是对保护文件夹中文件的Rename
		if (FZCheckFileName(pDevObj, pFileObj->FileName.Buffer)&& !FZCheckProcessList())
		{
			//直接拒绝
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
			//直接拒绝
			pIrp->IoStatus.Status = STATUS_ACCESS_DENIED;
			pIrp->IoStatus.Information = 0;

			IoCompleteRequest(pIrp, IO_NO_INCREMENT);
			return STATUS_ACCESS_DENIED;
		}
	}

	//否则 向下处理
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
		//直接拒绝
		pIrp->IoStatus.Status = STATUS_ACCESS_DENIED;
		pIrp->IoStatus.Information = 0;

		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_ACCESS_DENIED;
	}

	IoSkipCurrentIrpStackLocation(pIrp);

	PDEVICE_OBJECT pNextDevObj = ((FILE_FILTER_EXT*)pDevObj->DeviceExtension)->AttachToDevice;

	return IoCallDriver(pNextDevObj, pIrp);
}


//该Filter默认的Dispatch函数
NTSTATUS FZFileDefaultFilter(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp )
{
	IoSkipCurrentIrpStackLocation(pIrp);

	return IoCallDriver(((FILE_FILTER_EXT*)pDevObj->DeviceExtension)->AttachToDevice, pIrp);
}

//为了Attach到相应的FileSystemObj上，  进行多次Attach
//注意该FileSystemObj在Mount时由 文件系统驱动创建
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

		//延迟1/2秒 进行绑定
		li.QuadPart = 500 * (-10*1000);
		KeDelayExecutionThread(KernelMode, FALSE, &li);
	}

	return STATUS_UNSUCCESSFUL;
}

//检查pDevObj所在的设备栈中是否有当前Driver创建的DeviceObject
BOOLEAN FZIsAttachedDevice(IN PDEVICE_OBJECT pDevObj)
{
	//获得设备栈顶的Device
	PDEVICE_OBJECT pTopDevObj = IoGetAttachedDeviceReference(pDevObj);

	PDEVICE_OBJECT pCurDevObj = pTopDevObj;
	PDEVICE_OBJECT pLowerDevObj = NULL;

	WCHAR devNameBuf[MAX_DEVICE_NAME];
	UNICODE_STRING devName;

	RtlInitEmptyUnicodeString(&devName, devNameBuf, MAX_DEVICE_NAME);

	//从该Device开始向下遍历 查看是否是当前的Driver创建的
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


//枚举某个DeviceObject(File System的控制设备)的驱动的所有FileSystemDeviceObject 并Attach上
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
		//三种方式:  1.绑定到Ntfs控制设备上？
		//（没用。。因为控制设备是一直存在的，具体IRP是发送给相应的DeviceObject的 而不是控制的DeviceObject 
		// 控制设备在DriverEntry中创建 用来标识该Driver 有名字 无DeviceExtension
		// 控制设备负责在IoAttachDevice时创建一个相应的DeviceObject）
		//2. 绑定到Ntfs相应的uname的设备上 对该设备的通过文件系统发来的请求 都能获得
		//3. 绑定到相应的Volume的设备上 即使是对该设备的绕过文件系统的请求 也能获得
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
				
				//Attach到相应的FileSystemDeviceObject上
				FZAttachFileSystemObj(pDevObj, pDevList[i]);
			}
		}

		ExReleaseFastMutex(&g_FltAttachLock);
	}

	ExFreePoolWithTag(pDevList, FileFltTag);

	return STATUS_SUCCESS;
}