//#include <ntddk.h>
#include <ntifs.h>
#include "ProcessMonitor.h"

UCHAR* PsGetProcessImageFileName(__in PEPROCESS Process); 

void UnloadDriver(
	PDRIVER_OBJECT DriverObject
	);

NTSTATUS DispatchCreateClose(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

NTSTATUS Dispatch(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);

//
// process structure callbacks
//  
VOID ProcessCallback(
	IN HANDLE  hParentId, 
	IN HANDLE  hProcessId, 
	IN BOOLEAN bCreate
	);

//
// Structure for process callback information
//
typedef struct _CallbackInfo
{
    HANDLE  hParentId;
    HANDLE  hProcessId;
    BOOLEAN bCreate;
}CALLBACK_INFO, *PCALLBACK_INFO;

//
// Private storage 
//
typedef struct _DEVICE_EXTENSION 
{
    PDEVICE_OBJECT DeviceObject;
    HANDLE  hProcessHandle;
    PKEVENT ProcessEvent;

    HANDLE  hPParentId;
    HANDLE  hPProcessId;
    BOOLEAN bPCreate;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


PDEVICE_OBJECT g_pDeviceObject;

NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject, 
	IN PUNICODE_STRING RegistryPath
	)
{
    NTSTATUS        ntStatus;
    UNICODE_STRING  uszDriverString;
    UNICODE_STRING  uszDeviceString;
    UNICODE_STRING  uszProcessEventString;

    PDEVICE_OBJECT    pDeviceObject;
    PDEVICE_EXTENSION extension;
    
    RtlInitUnicodeString(&uszDriverString, DEVICE_NAME);

    ntStatus = IoCreateDevice(DriverObject,
										        sizeof(DEVICE_EXTENSION),
										        &uszDriverString,
										        FILE_DEVICE_UNKNOWN,
										        0,
										        FALSE,
										        &pDeviceObject);
	        
    if(ntStatus != STATUS_SUCCESS)
        return ntStatus;
    
    extension = pDeviceObject->DeviceExtension;

    RtlInitUnicodeString(&uszDeviceString, DEVICE_LINK_NAME);    
    ntStatus = IoCreateSymbolicLink(&uszDeviceString, &uszDriverString);
    if(ntStatus != STATUS_SUCCESS)
    {
        IoDeleteDevice(pDeviceObject);
        return ntStatus;
    }

    g_pDeviceObject = pDeviceObject;

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Dispatch;
		DriverObject->DriverUnload                         = UnloadDriver;

    
    RtlInitUnicodeString(&uszProcessEventString, MONITOR_PROCESS_EVENT);
    extension->ProcessEvent = IoCreateNotificationEvent (&uszProcessEventString, &extension->hProcessHandle);

    KeClearEvent(extension->ProcessEvent);

    ntStatus = PsSetCreateProcessNotifyRoutine(ProcessCallback, FALSE);
    
    return ntStatus;
}


NTSTATUS DispatchCreateClose(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

VOID ProcessCallback(
	IN HANDLE  hParentId, 
	IN HANDLE  hProcessId, 
	IN BOOLEAN bCreate
	)
{
		PEPROCESS pEProc;
		CHAR *pName;   //进程名
	
		ANSI_STRING Net1ExeName;
		ANSI_STRING NetExeName;
		ANSI_STRING MmcExeName;
		ANSI_STRING CurExeName;
	
    PDEVICE_EXTENSION extension;
	  extension = g_pDeviceObject->DeviceExtension;

    extension->hPParentId  = hParentId;
    extension->hPProcessId = hProcessId;
    extension->bPCreate    = bCreate;
    
		PsLookupProcessByProcessId(hProcessId, &pEProc);

#if WINVER >= 0x0501
		pName = (CHAR*)PsGetProcessImageFileName(pEProc); //获取进程名
#else
		pName = (CHAR*)pEProc + 0x1FC;
#endif

		ObDereferenceObject(pEProc); 

		DbgPrint ("Create Process Name = %s.\n", pName);

		RtlInitAnsiString(&CurExeName, pName);
		RtlInitAnsiString(&Net1ExeName, "net1.exe");
		RtlInitAnsiString(&NetExeName, "net.exe");
		RtlInitAnsiString(&MmcExeName, "mmc.exe");
		
		if (bCreate && (RtlCompareString(&NetExeName, &CurExeName, TRUE) == 0 ||
				RtlCompareString(&MmcExeName, &CurExeName, TRUE) == 0 ||
				RtlCompareString(&Net1ExeName, &CurExeName, TRUE) == 0)) {

		    KeSetEvent(extension->ProcessEvent, 0, FALSE);
		    KeClearEvent(extension->ProcessEvent);
	  }
}

NTSTATUS Dispatch(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	)
{
    NTSTATUS              ntStatus = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION    irpStack  = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION     extension = DeviceObject->DeviceExtension;
    //PCALLBACK_INFO        pCallbackInfo;
    PHANDLE 								pHandle;	

    switch(irpStack->Parameters.DeviceIoControl.IoControlCode)
    {
      case IOCTL_PROCESSMONITOR_GET_PROCINFO:
        pHandle = Irp->AssociatedIrp.SystemBuffer;
        *pHandle = extension->hPProcessId;
        ntStatus = STATUS_SUCCESS;
        break;
      default:
          break;
    }

    Irp->IoStatus.Status = ntStatus;
   
    if(ntStatus == STATUS_SUCCESS)
        Irp->IoStatus.Information = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    else
        Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return ntStatus;
}

void UnloadDriver(IN PDRIVER_OBJECT DriverObject)
{
		UNICODE_STRING  uszDeviceString;
		NTSTATUS        ntStatus;
		ntStatus = PsSetCreateProcessNotifyRoutine(ProcessCallback, TRUE);
		
		IoDeleteDevice(DriverObject->DeviceObject);
		
		RtlInitUnicodeString(&uszDeviceString, DEVICE_LINK_NAME);
		IoDeleteSymbolicLink(&uszDeviceString);
}
