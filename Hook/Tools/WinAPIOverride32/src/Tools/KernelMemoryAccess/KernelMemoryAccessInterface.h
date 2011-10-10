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
// Object: provide a class helper for using KernelMemoryAccess driver
//         It allows to:
//              - start/stop the driver checking if another application is using it
//-----------------------------------------------------------------------------


#pragma once

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <windows.h>
#include "KernelMemoryAccessExport.h"

#include "../APIError/APIError.h"
#include "../Driver/Driver.h"

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define KERNEL_MEMORY_ACCESS_SYS _T("KernelMemoryAccess.sys")
#define KERNEL_MEMORY_ACCESS_NAME _T("KernelMemoryAccess")
#define DRIVER_NAME _T("\\\\.\\KernelMemoryAccess")

//---------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Class
//---------------------------------------------------------------------------
class CKernelMemoryAccessInterface
{
private:
    BOOL bStarted;
    BOOL bOpen;
    HANDLE hDriverFile;
public:
    CKernelMemoryAccessInterface(void);
    ~CKernelMemoryAccessInterface(void);
    BOOL StartDriver();
    BOOL StopDriver();
    BOOL OpenDriver();
    BOOL CloseDriver();
    BOOL ReadMemory(PVOID Address,ULONG Size,OUT PBYTE Buffer,OUT ULONG* pReadSize);
    BOOL WriteMemory(PVOID Address,PBYTE Buffer,ULONG BufferSize,OUT ULONG* pWriteSize);
    BOOL AllocateMemory(ULONG SizeInByte,OUT PVOID* pAddress);
    BOOL FreeMemory(PVOID Address);
};
