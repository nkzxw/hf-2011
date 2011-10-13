//---------------------------------------------------------------------------
//
// NTProcDrv.c
//
// SUBSYSTEM: 
//				System monitor
// MODULE:    
//				Driver for monitoring NT process and DLLs mapping
//              monitoring. 
//
// DESCRIPTION:
//              This code is based on the James Finnegan’s sample 
//              (MSJ January 1999).
//
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com), January 2002
//                                                                         
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
// Includes
//  
//---------------------------------------------------------------------------
#include <ntddk.h>

//---------------------------------------------------------------------------
//
// Defines
//  
//---------------------------------------------------------------------------
#define FILE_DEVICE_UNKNOWN             0x00000022
#define IOCTL_UNKNOWN_BASE              FILE_DEVICE_UNKNOWN
#define IOCTL_NTPROCDRV_GET_PROCINFO    CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//---------------------------------------------------------------------------
//
// Forward declaration
//  
//---------------------------------------------------------------------------

void UnloadDriver(
	PDRIVER_OBJECT DriverObject
	);
NTSTATUS DispatchCreateClose(
	IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp
	);
NTSTATUS DispatchIoctl(
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

//
// The main entry point of the driver module
//
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
    
	// Point uszDriverString at the driver name
    RtlInitUnicodeString(&uszDriverString, L"\\Device\\NTProcDrv");

    // Create and initialize device object
    ntStatus = IoCreateDevice(
		DriverObject,
        sizeof(DEVICE_EXTENSION),
        &uszDriverString,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &pDeviceObject
		);
    if(ntStatus != STATUS_SUCCESS)
        return ntStatus;
    
	// Assign extension variable
    extension = pDeviceObject->DeviceExtension;
    
	// Point uszDeviceString at the device name
    RtlInitUnicodeString(&uszDeviceString, L"\\DosDevices\\NTProcDrv");
    
	// Create symbolic link to the user-visible name
    ntStatus = IoCreateSymbolicLink(&uszDeviceString, &uszDriverString);

    if(ntStatus != STATUS_SUCCESS)
    {
        // Delete device object if not successful
        IoDeleteDevice(pDeviceObject);
        return ntStatus;
    }

    // Assign global pointer to the device object for use by the callback functions
    g_pDeviceObject = pDeviceObject;

    // Load structure to point to IRP handlers
    DriverObject->DriverUnload                         = UnloadDriver;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;

    // Create event for user-mode processes to monitor
    RtlInitUnicodeString(&uszProcessEventString, L"\\BaseNamedObjects\\NTProcDrvProcessEvent");
    extension->ProcessEvent = IoCreateNotificationEvent (&uszProcessEventString, &extension->hProcessHandle);

    // Clear it out
    KeClearEvent(extension->ProcessEvent);

    // Set up callback routines
    ntStatus = PsSetCreateProcessNotifyRoutine(ProcessCallback, FALSE);

    // Return success
    return ntStatus;
}


//
// Create and close routine
//
NTSTATUS DispatchCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


//
// Process function callback
//
VOID ProcessCallback(
	IN HANDLE  hParentId, 
	IN HANDLE  hProcessId, 
	IN BOOLEAN bCreate
	)
{
    PDEVICE_EXTENSION extension;

    // Assign extension variable
    extension = g_pDeviceObject->DeviceExtension;

    // Assign current values into device extension.  
	// User-mode apps will pick it up using DeviceIoControl calls.
    extension->hPParentId  = hParentId;
    extension->hPProcessId = hProcessId;
    extension->bPCreate    = bCreate;

    // Pulse the event so any user-mode apps listening will know something
    // interesting has happened.  Sadly, user-mode apps can't reset a KM
    // event, which is why we're pulsing it here...
    KeSetEvent(extension->ProcessEvent, 0, FALSE);
    KeClearEvent(extension->ProcessEvent);
}



NTSTATUS DispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS              ntStatus = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION    irpStack  = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION     extension = DeviceObject->DeviceExtension;
    PCALLBACK_INFO        pCallbackInfo;
	//
    // These IOCTL handlers get the current data out of the device
    // extension structure.  
	//
    switch(irpStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_NTPROCDRV_GET_PROCINFO:
            if(irpStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(CALLBACK_INFO))
            {
                pCallbackInfo = Irp->AssociatedIrp.SystemBuffer;
                pCallbackInfo->hParentId  = extension->hPParentId;
                pCallbackInfo->hProcessId = extension->hPProcessId;
                pCallbackInfo->bCreate    = extension->bPCreate;
    
                ntStatus = STATUS_SUCCESS;
            }
            break;

        default:
            break;
    }

    Irp->IoStatus.Status = ntStatus;
   
    // Set number of bytes to copy back to user-mode
    if(ntStatus == STATUS_SUCCESS)
        Irp->IoStatus.Information = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    else
        Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return ntStatus;
}


//
// Driver unload routine
//
void UnloadDriver(IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING  uszDeviceString;
	NTSTATUS        ntStatus;

    // restore the call back routine, thus givinig chance to the 
	// user mode application to unload dynamically the driver
    ntStatus = PsSetCreateProcessNotifyRoutine(ProcessCallback, TRUE);

    IoDeleteDevice(DriverObject->DeviceObject);

    RtlInitUnicodeString(&uszDeviceString, L"\\DosDevices\\NTProcDrv");
    IoDeleteSymbolicLink(&uszDeviceString);
}
//----------------------End of file -----------------------------------------
