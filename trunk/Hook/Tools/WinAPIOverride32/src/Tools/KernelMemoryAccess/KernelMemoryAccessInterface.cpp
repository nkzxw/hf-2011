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

#include "KernelMemoryAccessinterface.h"

CKernelMemoryAccessInterface::CKernelMemoryAccessInterface(void)
{
    this->bStarted=FALSE;
    this->bOpen=FALSE;
    this->hDriverFile=NULL;
}

CKernelMemoryAccessInterface::~CKernelMemoryAccessInterface(void)
{
    // as this driver can be used by multiple instance, it can/should be let running
    // this->StopDriver();
}


//-----------------------------------------------------------------------------
// Name: StartDriver
// Object: Install and start KernelMemoryAccess driver
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success, FALSE on error or if driver is already running
//-----------------------------------------------------------------------------
BOOL CKernelMemoryAccessInterface::StartDriver()
{
    BOOL bAlreadyLoaded=FALSE;
    BOOL bAlreadyInstalled=FALSE;
/*
    BOOL bForceContinue=FALSE;
    DWORD dwCurrentState=0;
*/
    CDriver Driver;

    TCHAR szDir[MAX_PATH];
    TCHAR* pos;

    // Get current application directory
    GetModuleFileName(GetModuleHandle(NULL),szDir,MAX_PATH);
    pos=_tcsrchr(szDir,'\\');
    if (pos)
        *(pos+1)=0;

    // put full path of KernelMemoryAccess.sys in szDir
    _tcscat(szDir,KERNEL_MEMORY_ACCESS_SYS);

    // install the driver querying if it is already installed
    if (!Driver.Install(szDir,SERVICE_DEMAND_START,&bAlreadyInstalled))
        return FALSE;
    
    // open the driver
    if (!Driver.Open(KERNEL_MEMORY_ACCESS_NAME))
        return FALSE;


/* // driver state checking : useless as multiple app can access it simultaneously
    // if driver is already installed, 2 things may append
    // - another soft is using it
    // - driver hasn't been uninstalled for unknown reason (software/computer crash, bad programming ...)
    if (bAlreadyInstalled)
    {
        // query the state of driver
        if (!Driver.GetState(&dwCurrentState))
            return FALSE;
        // check if another application is using it

        if ((dwCurrentState!=SERVICE_STOPPED)&&(dwCurrentState!=SERVICE_STOP_PENDING))
        {
#if (!defined(TOOLS_NO_MESSAGEBOX))
            if (MessageBox(NULL,MSG_DRIVER_ALREADY_RUNNING,_T("Warning"),MB_YESNO|MB_ICONWARNING|MB_TOPMOST)==IDYES)
                bForceContinue=TRUE;
            else
#endif
                return FALSE;
        }
    }
*/

    // try to start the driver (still check if another app is using it)
    if (!Driver.Start(&bAlreadyLoaded))
        return FALSE;

/* // driver state checking : useless as multiple app can access it simultaneously
    // if used by another application
    if (bAlreadyLoaded&&(!bForceContinue))
    {
#if (!defined(TOOLS_NO_MESSAGEBOX))
        if (MessageBox(NULL,MSG_DRIVER_ALREADY_RUNNING,_T("Warning"),MB_YESNO|MB_ICONWARNING|MB_TOPMOST)!=IDYES)
#endif
            return FALSE;
    }
*/
    this->bStarted=TRUE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: StopDriver
// Object: stop and uninstall KernelMemoryAccess driver
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CKernelMemoryAccessInterface::StopDriver()
{
    CDriver Driver;

    if (!this->bStarted)
        return TRUE;

    // open driver
    Driver.Open(KERNEL_MEMORY_ACCESS_NAME);

    // stop it
    Driver.Stop();

    // uninstall it
    Driver.UnInstall();

    this->bStarted=FALSE;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: OpenDriver
// Object: Connect to driver necessary before read/write/free/allocate operation
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CKernelMemoryAccessInterface::OpenDriver()
{
    // open driver
    this->hDriverFile = CreateFile(
                                    DRIVER_NAME,
                                    GENERIC_READ | GENERIC_WRITE, 
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    0,                     // Default security
                                    OPEN_EXISTING,
                                    FILE_FLAG_OVERLAPPED,  // Perform synchronous I/O
                                    0);                    // No template
    if (hDriverFile==INVALID_HANDLE_VALUE)
    {
        CAPIError::ShowLastError();
        return FALSE;
    }

    this->bOpen=TRUE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CloseDriver
// Object: Disconnect from driver
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CKernelMemoryAccessInterface::CloseDriver()
{
    this->bOpen=FALSE;
    // close driver
    return CloseHandle(this->hDriverFile);
}
//-----------------------------------------------------------------------------
// Name: ReadMemory
// Object: read kernel memory
// Parameters :
//     in  : PVOID Address : start address of reading
//           ULONG Size : size to read (in bytes)
//     out : OUT PBYTE Buffer : ouput buffer containing read memory
//           OUT ULONG* pReadSize : number of read bytes
//     return : TRUE on success, FALSE on error
//              If only a part of Size is readable (but not the all size), pReadSize
//              will contain the size of memory read and Buffer the content of this memory
//              but the return will be FALSE
//-----------------------------------------------------------------------------
BOOL CKernelMemoryAccessInterface::ReadMemory(PVOID Address,ULONG Size,OUT PBYTE Buffer,OUT ULONG* pReadSize)
{
    BOOL bRet;

    if (!this->bOpen)
        return FALSE;

    // check parameters
    if (IsBadWritePtr(Buffer,Size))
        return FALSE;
    if (IsBadWritePtr(pReadSize,sizeof(ULONG)))
        return FALSE;

    MEMORY_ACCESS_INFOS MemAccess;

    MemAccess.Address=Address;
    MemAccess.SizeInByte=Size;
    bRet=DeviceIoControl(this->hDriverFile,IOCTL_READ_MEMORY,
                        &MemAccess,sizeof(MEMORY_ACCESS_INFOS),
                        Buffer,Size,
                        pReadSize,NULL);

    return (bRet&& (Size==*pReadSize));
    
}

//-----------------------------------------------------------------------------
// Name: WriteMemory
// Object: write kernel memory
// Parameters :
//     in  : PVOID Address : start address of write
//           PBYTE Buffer : buffer to write
//           ULONG BufferSize : buffer size to write (in bytes)
//     out : ULONG* pWriteSize : written size in bytes (can differ from BufferSize if some memory is not valid)
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CKernelMemoryAccessInterface::WriteMemory(PVOID Address,PBYTE Buffer,ULONG BufferSize,OUT ULONG* pWriteSize)
{
    BOOL bRet;
    if (!this->bOpen)
        return FALSE;

    if (IsBadReadPtr(Buffer,BufferSize))
        return FALSE;
    if (IsBadWritePtr(pWriteSize,sizeof(ULONG)))
        return FALSE;


    MEMORY_ACCESS_INFOS MemAccess;
    DWORD NbBytesReturned;

    MemAccess.Address=Address;
    MemAccess.SizeInByte=BufferSize;
    DWORD pbBufferSize=sizeof(MEMORY_ACCESS_INFOS)+BufferSize;
    PBYTE pbBuffer=new BYTE[pbBufferSize];
    
    memcpy(pbBuffer,&MemAccess,sizeof(MEMORY_ACCESS_INFOS));
    memcpy(&pbBuffer[sizeof(MEMORY_ACCESS_INFOS)],Buffer,BufferSize);
    bRet=DeviceIoControl(this->hDriverFile,IOCTL_WRITE_MEMORY,
                        pbBuffer,pbBufferSize,
                        pWriteSize,sizeof(ULONG),
                        &NbBytesReturned,NULL);
    delete[] pbBuffer;
    return (bRet&& (BufferSize==*pWriteSize));
}

//-----------------------------------------------------------------------------
// Name: AllocateMemory
// Object: Allocate NonPagedPool kernel memory
// Parameters :
//     in  : ULONG Size : size in bytes of memory to allocate
//     out : PVOID* Address
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CKernelMemoryAccessInterface::AllocateMemory(ULONG Size,OUT PVOID* pAddress)
{
    if (!this->bOpen)
        return FALSE;

    MEMORY_ACCESS_INFOS MemAccess;
    DWORD NbBytesReturned;

    MemAccess.Address=0;
    MemAccess.SizeInByte=Size;
    return DeviceIoControl(this->hDriverFile,IOCTL_ALLOCATE_MEMORY,
                            &MemAccess,sizeof(MEMORY_ACCESS_INFOS),
                            pAddress,sizeof(PVOID),
                            &NbBytesReturned,NULL);
}

//-----------------------------------------------------------------------------
// Name: FreeMemory
// Object: Free kernel memory
// Parameters :
//     in  : PVOID Address : address to free
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CKernelMemoryAccessInterface::FreeMemory(PVOID Address)
{
    if (!this->bOpen)
        return FALSE;

    MEMORY_ACCESS_INFOS MemAccess;
    DWORD NbBytesReturned;

    MemAccess.Address=Address;
    MemAccess.SizeInByte=0;
    return DeviceIoControl(this->hDriverFile,IOCTL_FREE_MEMORY,
                            &MemAccess,sizeof(MEMORY_ACCESS_INFOS),
                            NULL,0,
                            &NbBytesReturned,NULL);
}