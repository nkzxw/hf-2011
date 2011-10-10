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
// Object: header of the main part of the KernelMemoryAccess driver
//-----------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------

#include "DriversCommon.h" // place it before <ntddk.h> to avoid warnings
#include <ntddk.h>
#include "../KernelMemoryAccessExport.h"

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define DEVICE_NAME L"\\Device\\KernelMemoryAccess"
#define DEVICE_LINK L"\\DosDevices\\KernelMemoryAccess"

#define KERNEL_MEMORY_ACCESS_MEMORY_TAG 'AMeK'

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

VOID CleanUp();
BOOLEAN IsValidPtrEx(PVOID *lp,ULONG ucb);
BOOLEAN IsValidPtr(PVOID *lp);