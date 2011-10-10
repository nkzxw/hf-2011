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
// Object: Main part of the KernelMemoryAccess driver
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "KernelMemoryAccess.h"

//---------------------------------------------------------------------------
// Global vars
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Sub functions
//---------------------------------------------------------------------------
BOOLEAN IsValidPtrExWithSize(PVOID *lp,ULONG ucb,ULONG* pValidSize)
{
    unsigned char* lpb=(unsigned char*)lp;
    for (*pValidSize=0;*pValidSize<ucb;(*pValidSize)++)
    {
        if(!MmIsAddressValid(lpb+*pValidSize))
            return FALSE;
    }
    return TRUE;
}

BOOLEAN IsValidPtrEx(PVOID *lp,ULONG ucb)
{
    ULONG ucnt;
    unsigned char* lpb=(unsigned char*)lp;
    for (ucnt=0;ucnt<ucb;ucnt++)
    {
        if(!MmIsAddressValid(lpb+ucnt))
            return FALSE;
    }
    return TRUE;
}
BOOLEAN IsValidPtr(PVOID *lp)
{
    return MmIsAddressValid(lp);
}

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
    MEMORY_ACCESS_INFOS*   pMemoryAccessInfos;
    ULONG                  BytesTransferred=0;
    ULONG               AccessibleByteSize=0;
    unsigned char*         pData=0;
    BOOLEAN                bError=FALSE;

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
        case IOCTL_READ_MEMORY:
            
            KernelDebugPrint((" KernelMemoryAccess : READ MEMORY"));

            if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength<sizeof(MEMORY_ACCESS_INFOS))
            {
                KernelDebugPrint((" KernelMemoryAccess : BUFFER TOO SMALL"));

                ntStatus=STATUS_BUFFER_TOO_SMALL;
                pIrp->IoStatus.Status = ntStatus;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                return ntStatus;
            }

            pMemoryAccessInfos=(MEMORY_ACCESS_INFOS*) pIrp->AssociatedIrp.SystemBuffer;

            // if there's not enough memory for output buffer
            if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength <pMemoryAccessInfos->SizeInByte)
            {
                KernelDebugPrint((" KernelMemoryAccess : BUFFER TOO SMALL"));

                ntStatus=STATUS_BUFFER_TOO_SMALL;
                pIrp->IoStatus.Status = ntStatus;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                return ntStatus;
            }
            // else
            // check pointer and size
            bError=IsValidPtrExWithSize(pMemoryAccessInfos->Address,pMemoryAccessInfos->SizeInByte,&AccessibleByteSize);
            if (bError)
            {
                if (AccessibleByteSize==0)
                {
                    KernelDebugPrint((" KernelMemoryAccess : INVALID MEMORY"));

                    ntStatus=STATUS_ACCESS_VIOLATION;
                    pIrp->IoStatus.Status = ntStatus;
                    pIrp->IoStatus.Information = 0;
                    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                    return ntStatus;
                }
            }

            // fill the output buffer
            BytesTransferred=AccessibleByteSize;
            RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer,pMemoryAccessInfos->Address,AccessibleByteSize);

            // return a success code any way
            ntStatus = STATUS_SUCCESS;

            break;

        case IOCTL_WRITE_MEMORY:
            KernelDebugPrint((" KernelMemoryAccess : WRITE MEMORY"));

            if ((pIrpStack->Parameters.DeviceIoControl.InputBufferLength<sizeof(MEMORY_ACCESS_INFOS))
                ||(pIrpStack->Parameters.DeviceIoControl.OutputBufferLength<sizeof(ULONG)))
            {
                KernelDebugPrint((" KernelMemoryAccess : BUFFER TOO SMALL"));

                ntStatus=STATUS_BUFFER_TOO_SMALL;
                pIrp->IoStatus.Status = ntStatus;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                return ntStatus;
            }

            pMemoryAccessInfos=(MEMORY_ACCESS_INFOS*) pIrp->AssociatedIrp.SystemBuffer;

            // if there's not enough memory in input buffer
            if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength<sizeof(MEMORY_ACCESS_INFOS)+pMemoryAccessInfos->SizeInByte)
            {
                KernelDebugPrint((" KernelMemoryAccess : BUFFER TOO SMALL"));

                ntStatus=STATUS_BUFFER_TOO_SMALL;
                pIrp->IoStatus.Status = ntStatus;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                return ntStatus;
            }
            // else

            // check pointer and size
            bError=IsValidPtrExWithSize(pMemoryAccessInfos->Address,pMemoryAccessInfos->SizeInByte,&AccessibleByteSize);
            if (bError)
            {
                if (AccessibleByteSize==0)
                {
                    KernelDebugPrint((" KernelMemoryAccess : INVALID MEMORY"));

                    ntStatus=STATUS_ACCESS_VIOLATION;
                    pIrp->IoStatus.Status = ntStatus;
                    pIrp->IoStatus.Information = 0;
                    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                    return ntStatus;
                }

            }

            pData=pIrp->AssociatedIrp.SystemBuffer;
            pData+=sizeof(MEMORY_ACCESS_INFOS);

            // write memory at the specified location
            RtlCopyMemory(pMemoryAccessInfos->Address,(PVOID)pData,AccessibleByteSize);
            // copy number of byte
            RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer,&AccessibleByteSize,sizeof(ULONG));
            // nb bytes transfered to user mode
            BytesTransferred=sizeof(ULONG);

            // return a success code any way
            ntStatus = STATUS_SUCCESS;

            break;

        case IOCTL_ALLOCATE_MEMORY:
            KernelDebugPrint((" KernelMemoryAccess : ALLOCATE MEMORY"));

            if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength<sizeof(MEMORY_ACCESS_INFOS))
            {
                KernelDebugPrint((" KernelMemoryAccess : BUFFER TOO SMALL"));

                ntStatus=STATUS_BUFFER_TOO_SMALL;
                pIrp->IoStatus.Status = ntStatus;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                return ntStatus;
            }

            pMemoryAccessInfos=(MEMORY_ACCESS_INFOS*) pIrp->AssociatedIrp.SystemBuffer;

            // if there's not enough memory in output buffer
            if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength<sizeof(PVOID))
            {
                KernelDebugPrint((" KernelMemoryAccess : BUFFER TOO SMALL"));

                ntStatus=STATUS_BUFFER_TOO_SMALL;
                pIrp->IoStatus.Status = ntStatus;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                return ntStatus;
            }
            // else
            pData=ExAllocatePoolWithTag(NonPagedPool,pMemoryAccessInfos->SizeInByte,KERNEL_MEMORY_ACCESS_MEMORY_TAG);
            RtlCopyMemory(pIrp->AssociatedIrp.SystemBuffer,&pData,sizeof(PVOID));

            // nb bytes transfered to user mode
            BytesTransferred=sizeof(PVOID);
            ntStatus = STATUS_SUCCESS;

            break;

        case IOCTL_FREE_MEMORY:
            KernelDebugPrint((" KernelMemoryAccess : FREE MEMORY"));

            if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength<sizeof(MEMORY_ACCESS_INFOS))
            {
                KernelDebugPrint((" KernelMemoryAccess : BUFFER TOO SMALL"));

                ntStatus=STATUS_BUFFER_TOO_SMALL;
                pIrp->IoStatus.Status = ntStatus;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                return ntStatus;
            }

            pMemoryAccessInfos=(MEMORY_ACCESS_INFOS*) pIrp->AssociatedIrp.SystemBuffer;

            // check pointer and size
            if (!IsValidPtr(pMemoryAccessInfos->Address))
            {
                KernelDebugPrint((" KernelMemoryAccess : INVALID MEMORY"));

                ntStatus=STATUS_ACCESS_VIOLATION;
                pIrp->IoStatus.Status = ntStatus;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                return ntStatus;
            }

            ExFreePoolWithTag(pMemoryAccessInfos->Address,KERNEL_MEMORY_ACCESS_MEMORY_TAG);

            // nb bytes transfered to user mode
            BytesTransferred=0;
            ntStatus = STATUS_SUCCESS;

            break;

        default:
            KernelDebugPrint((" KernelMemoryAccess : CONTROL CODE NOT IMPLEMENTED"));

            ntStatus=STATUS_NOT_IMPLEMENTED;
            break;
    }

    pIrp->IoStatus.Status = ntStatus;
    // Set number of bytes to copy back to user-mode
    pIrp->IoStatus.Information = BytesTransferred;

    // complete request
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return ntStatus;
}

//-----------------------------------------------------------------------------
// Name: CleanUp
// Object: generic clean up 
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
VOID CleanUp()
{

}