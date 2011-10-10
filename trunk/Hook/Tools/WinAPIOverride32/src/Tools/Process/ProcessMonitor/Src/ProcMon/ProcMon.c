/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//-----------------------------------------------------------------------------
// Object: Main part of the ProcMon driver
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "ProcMon.h"

//---------------------------------------------------------------------------
// Global vars
//---------------------------------------------------------------------------
// Global Flag to know monitoring state
BOOLEAN g_bMonotoringStarted=FALSE;

// Global var containing the pending Irp
PIRP    g_PendingIrp=NULL;

// Global Fifo containing the PROCESS_CALLBACK_INFO informations
PKERNEL_FIFO g_pFIFOProcess=NULL;
KEVENT hevt_g_PendingIrpUnlocked={0};

//-----------------------------------------------------------------------------
// Name: DriverEntry
// Object: Entry point of the driver
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT DriverObject, 
    IN PUNICODE_STRING RegistryPath
    )
{
    PDEVICE_OBJECT pDeviceObject = NULL;
    NTSTATUS       ntStatus;
    UNICODE_STRING uszDeviceName;
    UNICODE_STRING uszDeviceLink;

    UNREFERENCED_PARAMETER(RegistryPath);

    // Point uszDriverString at the driver name
    RtlInitUnicodeString(&uszDeviceName, DEVICE_NAME);

    // Create and initialize device object
    ntStatus = IoCreateDevice(
        DriverObject,
        0,// number of bytes to be allocated for the device extension of the device object
        &uszDeviceName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &pDeviceObject
        );
    if(ntStatus != STATUS_SUCCESS)
        return ntStatus;

    // Point uszDeviceString at the device name
    RtlInitUnicodeString(&uszDeviceLink, DEVICE_LINK);

    // Create symbolic link to the user-visible name
    ntStatus = IoCreateSymbolicLink(&uszDeviceLink, &uszDeviceName);

    if(ntStatus != STATUS_SUCCESS)
    {
        // Delete device object if not successful
        IoDeleteDevice(pDeviceObject);
        return ntStatus;
    }

    // Set handler for Major Function IRP dispatching
    DriverObject->DriverUnload                         = UnloadDriver;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;

    // Return success
    return ntStatus;
}

//-----------------------------------------------------------------------------
// Name: UnloadDriver
// Object: Driver unload routine : Free all allocated memory, release pending IRP
//                                 if any more
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
VOID UnloadDriver(IN PDRIVER_OBJECT pDriverObject)
{
    UNICODE_STRING  uszDeviceString;

    // Free resources
    CleanUp();

    // Delete the symbolic link
    RtlInitUnicodeString(&uszDeviceString, DEVICE_LINK);
    IoDeleteSymbolicLink(&uszDeviceString);

    // Delete the device object
    IoDeleteDevice(pDriverObject->DeviceObject);
}

//-----------------------------------------------------------------------------
// Name: DispatchCreate
// Object: Called on IRP_MJ_CREATE
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
NTSTATUS DispatchCreate(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    pIrp->IoStatus.Status      = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    // complete create request
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// Name: DispatchClose
// Object: Called on IRP_MJ_CLOSE : Free all allocated memory, release pending IRP
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
NTSTATUS DispatchClose(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    pIrp->IoStatus.Status      = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    // Free resources
    CleanUp();

    // complete close request
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
// Name: DispatchIoctl
// Object: Dispatch routine of IRP_MJ_DEVICE_CONTROL
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
NTSTATUS DispatchIoctl(IN PDEVICE_OBJECT pDeviceObject,IN PIRP           pIrp)
{
    NTSTATUS               ntStatus = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION     pIrpStack  = IoGetCurrentIrpStackLocation(pIrp);
    PROCESS_CALLBACK_INFO  ProcCallbackInfo={0};
    ULONG                  BytesTransferred=0;

    UNREFERENCED_PARAMETER(pDeviceObject);
    /*

    // Reminder :D
    // Get the pointer to the input/output buffer and it's length
    // IRP_MJ_DEVICE_CONTROL specific
    BYTE  Buffer             = pIrp->AssociatedIrp.SystemBuffer;
    ULONG InputBufferLength  = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG OutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    */

    // switch IoControlCode pass through the DeviceIOControle user mode API
    switch(pIrpStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_START_MONITORING:
            KernelDebugPrint((" ProcMon : START MONITORING"));

            ntStatus = StartMonitoring();
            break;

        case IOCTL_STOP_MONITORING:
            KernelDebugPrint((" ProcMon : STOP MONITORING"));

            ntStatus = StopMonitoring();
            break;

        case IOCTL_GET_PROCINFO:
            KernelDebugPrint((" ProcMon : GET PROCINFO"));
            // if monitoring is not started, don't try to access fifo
            if (!g_bMonotoringStarted)
            {
                KernelDebugPrint((" ProcMon : MONITORING NOT STARTED"));
                // return an invalid device state status
                ntStatus=STATUS_INVALID_DEVICE_STATE;
                pIrp->IoStatus.Status = ntStatus;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                return ntStatus;

            }
            // else

            // if an IOCTL_GET_PROCINFO request is already pending
            KeWaitForSingleObject(&hevt_g_PendingIrpUnlocked,Executive,KernelMode,FALSE,NULL);
            if (g_PendingIrp)
            {
                KernelDebugPrint((" ProcMon : GET PROCINFO DEVICE BUSY"));
                // return a busy status
                ntStatus=STATUS_DEVICE_BUSY;
                pIrp->IoStatus.Status = ntStatus;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                KeSetEvent(&hevt_g_PendingIrpUnlocked,IO_NO_INCREMENT,FALSE);
                return ntStatus;
            }
            // else

            // if there's not enough memory for output buffer
            if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength <sizeof(PROCESS_CALLBACK_INFO))
            {
                KernelDebugPrint((" ProcMon : GET PROCINFO BUFFER TOO SMALL"));

                ntStatus=STATUS_BUFFER_TOO_SMALL;
                pIrp->IoStatus.Status = ntStatus;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                KeSetEvent(&hevt_g_PendingIrpUnlocked,IO_NO_INCREMENT,FALSE);
                return ntStatus;
            }
            // else

            if (!g_pFIFOProcess)
            {
                KernelDebugPrint((" ProcMon : PROCESS FIFO NULL"));

                ntStatus=STATUS_UNSUCCESSFUL;
                pIrp->IoStatus.Status = ntStatus;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                KeSetEvent(&hevt_g_PendingIrpUnlocked,IO_NO_INCREMENT,FALSE);
                return ntStatus;
            }
            // else

            // pop fifo and check if it's empty
            if (KernelFIFOPop(g_pFIFOProcess,&ProcCallbackInfo))
            {
                // fill the output buffer
                RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer,&ProcCallbackInfo,sizeof(PROCESS_CALLBACK_INFO));

                BytesTransferred=sizeof(PROCESS_CALLBACK_INFO);
                ntStatus = STATUS_SUCCESS;

                KeSetEvent(&hevt_g_PendingIrpUnlocked,IO_NO_INCREMENT,FALSE);
            }
            else
            {
                KernelDebugPrint((" ProcMon : IRP PENDING"));

                // save pIrp for a future use
                g_PendingIrp=pIrp;

                // set cancel routine
                IoSetCancelRoutine(pIrp,CancelRoutine);

                // Mark the Irp as pending
                pIrp->PendingReturned=TRUE;
                pIrp->IoStatus.Status = STATUS_PENDING;
                pIrp->IoStatus.Information = 0;
                IoMarkIrpPending(pIrp);

                KeSetEvent(&hevt_g_PendingIrpUnlocked,IO_NO_INCREMENT,FALSE);

                // we have to return status pending
                return STATUS_PENDING;
            }
            break;

        case IOCTL_CANCEL_IO:
            KernelDebugPrint((" ProcMon : IOCTL_CANCEL_IO"));

            KeWaitForSingleObject(&hevt_g_PendingIrpUnlocked,Executive,KernelMode,FALSE,NULL);

            // As CancelPendingIrp() call IoCancelIrp, It can't be called from another thread
            // So we directly call the cancel routine giving the g_PendingIrp as parameter
            CancelRoutine(NULL,g_PendingIrp);

            KeSetEvent(&hevt_g_PendingIrpUnlocked,IO_NO_INCREMENT,FALSE);

            // return success for IOCTL_CANCEL_IO request
            ntStatus = STATUS_SUCCESS;
            break;

        default:
            KernelDebugPrint((" ProcMon : CONTROL CODE NOT IMPLEMENTED"));

            ntStatus=STATUS_NOT_IMPLEMENTED;
            break;
    }

    pIrp->IoStatus.Status = ntStatus;

    if(ntStatus == STATUS_SUCCESS)
        // Set number of bytes to copy back to user-mode
        pIrp->IoStatus.Information = BytesTransferred;
    else
        pIrp->IoStatus.Information = 0;

    // complete request
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return ntStatus;
}

//-----------------------------------------------------------------------------
// Name: CleanUp
// Object: generic clean up : Free all allocated memory and release pending IRP
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
VOID CleanUp()
{
    // stop process spying
    StopMonitoring();

    // cancel Irp
    CancelPendingIrp();
}

//-----------------------------------------------------------------------------
// Name: CancelPendingIrp
// Object: release pending IRP.
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
VOID CancelPendingIrp()
{
    KeWaitForSingleObject(&hevt_g_PendingIrpUnlocked,Executive,KernelMode,FALSE,NULL);
    if (g_PendingIrp)
        IoCancelIrp(g_PendingIrp);

    KeSetEvent(&hevt_g_PendingIrpUnlocked,IO_NO_INCREMENT,FALSE);
}

//-----------------------------------------------------------------------------
// Name: CancelRoutine
// Object: CancelRoutine for pending IRP
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
VOID CancelRoutine(IN PDEVICE_OBJECT DeviceObject,IN PIRP pIrp)
{
    // CancelRoutine can be called when hevt_g_PendingIrpUnlocked is locked,
    // so don't call KeWaitForSingleObject(&hevt_g_PendingIrpUnlocked,Executive,KernelMode,FALSE,NULL);
    // else there will be a deadlock

    UNREFERENCED_PARAMETER(DeviceObject);

    if (pIrp==NULL)
        return;

    KernelDebugPrint((" ProcMon : IO CANCELLED"));

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;
    // remove cancel routine
    IoSetCancelRoutine(pIrp,NULL);
    // complete the IO request
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    if (pIrp==g_PendingIrp)// must be the case
        g_PendingIrp=NULL;

}

//-----------------------------------------------------------------------------
// Name: StartMonitoring
// Object: start monitoring process, create fifo
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
NTSTATUS StartMonitoring()
{
    NTSTATUS ntStatus;

    // check if monitoring is already started
    if (g_bMonotoringStarted)
        return STATUS_SUCCESS;

    // initialize FIFO
    if (g_pFIFOProcess)
        // delete FIFO
        KernelFIFODelete(g_pFIFOProcess);

    g_pFIFOProcess=KernelFIFONew(sizeof(PROCESS_CALLBACK_INFO));
    if (!g_pFIFOProcess)
    {
        KernelDebugPrint((" ProcMon : PROCESS FIFO ALLOCATION ERROR"));

        return STATUS_UNSUCCESSFUL;
    }

    // initialize unlock event
    KeInitializeEvent(&hevt_g_PendingIrpUnlocked,SynchronizationEvent,TRUE);

    // call the kernel function to enable process spying
    ntStatus=PsSetCreateProcessNotifyRoutine(ProcessCallback, FALSE);
    if(ntStatus == STATUS_SUCCESS)
        g_bMonotoringStarted=TRUE;

    return ntStatus;
}

//-----------------------------------------------------------------------------
// Name: StopMonitoring
// Object: stop monitoring process, destroy fifo
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
NTSTATUS StopMonitoring()
{
    NTSTATUS ntStatus;

    // check if monitoring is already stopped
    if (!g_bMonotoringStarted)
        return STATUS_SUCCESS;

    if (g_pFIFOProcess)
    {
        // delete FIFO
        KernelFIFODelete(g_pFIFOProcess);
        g_pFIFOProcess=NULL;
    }

    // call the kernel function to disable process spying
    ntStatus=PsSetCreateProcessNotifyRoutine(ProcessCallback, TRUE);
    if(ntStatus == STATUS_SUCCESS)
        g_bMonotoringStarted=FALSE;

    return ntStatus;
}

//-----------------------------------------------------------------------------
// Name: ProcessCallback
// Object: Process creation or destruction callback
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
VOID ProcessCallback(
    IN HANDLE  hParentId, 
    IN HANDLE  hProcessId, 
    IN BOOLEAN bCreate
    )
{
    PPROCESS_CALLBACK_INFO pProcessCallbackInfo;
    PROCESS_CALLBACK_INFO  ProcessCallbackInfo;

    KeWaitForSingleObject(&hevt_g_PendingIrpUnlocked,Executive,KernelMode,FALSE,NULL);

    // if an IRP is pending that's means the FIFO is empty, so we have to complete request
    // with current ProcessCallbackInfo information
    // (Notice the Fifo can be useless if the processing between to IOCTL_GET_PROCINFO is fast)
    if (g_PendingIrp)
    {
        // check of buffer size has already been done in DispatchIoctl callback
        pProcessCallbackInfo = g_PendingIrp->AssociatedIrp.SystemBuffer;
        pProcessCallbackInfo->hParentId  = hParentId;
        pProcessCallbackInfo->hProcessId = hProcessId;
        pProcessCallbackInfo->bCreate    = bCreate;

        g_PendingIrp->IoStatus.Status = STATUS_SUCCESS;
        g_PendingIrp->IoStatus.Information = sizeof(PROCESS_CALLBACK_INFO);

        // remove cancel routine (as we have to clear the cancel routine before doing an IoCompleteRequest)
        IoSetCancelRoutine(g_PendingIrp,NULL);

        // complete the request
        IoCompleteRequest(g_PendingIrp,IO_NO_INCREMENT);

        // there's no more pending IRP
        g_PendingIrp=NULL;

        KeSetEvent(&hevt_g_PendingIrpUnlocked,IO_NO_INCREMENT,FALSE);
        return;
    }
    // else 
    
    // add data to the fifo for the next IOCTL_GET_PROCINFO call
    ProcessCallbackInfo.hParentId  = hParentId;
    ProcessCallbackInfo.hProcessId = hProcessId;
    ProcessCallbackInfo.bCreate    = bCreate;

    if (g_pFIFOProcess)
        // insert into fifo
        KernelFIFOPush(g_pFIFOProcess,&ProcessCallbackInfo);

    KeSetEvent(&hevt_g_PendingIrpUnlocked,IO_NO_INCREMENT,FALSE);
}