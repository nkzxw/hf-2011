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
// Object: header of the main part of the ProcMon driver
//-----------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------

#include "DriversCommon.h" // place it before <ntddk.h> to avoid warnings
#include <ntddk.h>
#include "../../ProcMonExport.h"
#include "KernelFIFO.h"

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define DEVICE_NAME L"\\Device\\ProcMon"
#define DEVICE_LINK L"\\DosDevices\\ProcMon"


//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------
VOID UnloadDriver(
    PDRIVER_OBJECT pDriverObject
    );
NTSTATUS DispatchCreate(
    IN PDEVICE_OBJECT pDeviceObject, 
    IN PIRP pIrp
    );
NTSTATUS DispatchClose(
    IN PDEVICE_OBJECT pDeviceObject, 
    IN PIRP pIrp
    );
NTSTATUS DispatchIoctl(
    IN PDEVICE_OBJECT pDeviceObject, 
    IN PIRP pIrp
    );

// Process function callback
VOID ProcessCallback(
    IN HANDLE  hParentId, 
    IN HANDLE  hProcessId, 
    IN BOOLEAN bCreate
    );

NTSTATUS StartMonitoring();
NTSTATUS StopMonitoring();
VOID CleanUp();
VOID CancelPendingIrp();
VOID CancelRoutine(IN PDEVICE_OBJECT DeviceObject,IN PIRP pIrp);
