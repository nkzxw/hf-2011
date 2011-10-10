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
// Object: Giving a class helper for installing/starting/stopping/uninstalling
//          drivers
//-----------------------------------------------------------------------------

#include "Driver.h"

CDriver::CDriver(void)
{
    this->h_SCService=NULL;
    this->h_SCManager=OpenSCManager(
                                NULL,//LPCTSTR lpMachineName,
                                NULL,//LPCTSTR lpDatabaseName,
                                SC_MANAGER_ALL_ACCESS//DWORD dwDesiredAccess
                                );
    if (!this->h_SCManager)
        CAPIError::ShowLastError();
}

CDriver::~CDriver(void)
{
    if (this->h_SCService)
        CloseServiceHandle(this->h_SCService);
    if (this->h_SCManager)
        CloseServiceHandle(this->h_SCManager);
}

//-----------------------------------------------------------------------------
// Name: Install
// Object: install driver
// Parameters :
//     in  : LPCTSTR lpBinaryPathName : full path of your driver
//           DWORD dwStartType : one of the following values SERVICE_AUTO_START, 
//                                  SERVICE_BOOT_START, SERVICE_DEMAND_START, 
//                                  SERVICE_DISABLED, SERVICE_SYSTEM_START 
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CDriver::Install(LPCTSTR lpBinaryPathName,DWORD dwStartType)
{
    BOOL bAlreadyInstalled=FALSE;
    return this->Install(lpBinaryPathName,dwStartType,&bAlreadyInstalled);
}

//-----------------------------------------------------------------------------
// Name: Install
// Object: install driver
// Parameters :
//     in  : LPCTSTR lpBinaryPathName : full path of your driver
//           DWORD dwStartType : one of the following values SERVICE_AUTO_START, 
//                                  SERVICE_BOOT_START, SERVICE_DEMAND_START, 
//                                  SERVICE_DISABLED, SERVICE_SYSTEM_START 
//     out : BOOL* pbAlreadyInstalled return TRUE if driver was already installed
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CDriver::Install(LPCTSTR lpBinaryPathName,DWORD dwStartType,BOOL* pbAlreadyInstalled)
{
    DWORD dwLastError;
    BOOL bRet=TRUE;
    LPTSTR lpServiceName;
    TCHAR* ptc;

    if (IsBadWritePtr(pbAlreadyInstalled,sizeof(BOOL)))
        return FALSE;
    *pbAlreadyInstalled=FALSE;

    // remove path and .sys
    ptc=(TCHAR*)_tcsrchr(lpBinaryPathName,'\\');
    if (ptc)
        ptc++;
    else
        ptc=(TCHAR*)lpBinaryPathName;
    lpServiceName=_tcsdup(ptc);
    ptc=_tcsrchr(lpServiceName,'.');
    if (ptc)
        *ptc=0;

    // create service
    this->h_SCService=CreateService(
                                h_SCManager,//SC_HANDLE hSCManager,
                                lpServiceName,//LPCTSTR lpServiceName,
                                lpServiceName,//LPCTSTR lpDisplayName,
                                SERVICE_ALL_ACCESS,//DWORD dwDesiredAccess,
                                SERVICE_KERNEL_DRIVER,//DWORD dwServiceType,
                                dwStartType,//DWORD dwStartType,
                                SERVICE_ERROR_NORMAL,//DWORD dwErrorControl,
                                lpBinaryPathName,//LPCTSTR lpBinaryPathName,
                                NULL,//LPCTSTR lpLoadOrderGroup,
                                NULL,//LPDWORD lpdwTagId,
                                NULL,//LPCTSTR lpDependencies,
                                NULL,//LPCTSTR lpServiceStartName,
                                NULL//LPCTSTR lpPassword
                                );

    if (!this->h_SCService)
    {
        bRet=FALSE;
        dwLastError=GetLastError();
        if (dwLastError==ERROR_SERVICE_EXISTS)// ERROR_DUPLICATE_SERVICE_NAME
        {
            bRet=TRUE;
            *pbAlreadyInstalled=TRUE;
        }
        else
            CAPIError::ShowError(dwLastError);
    }

    // free resource
    free(lpServiceName);

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: Open
// Object: open a driver. A driver must be opened before try to start/stop/GetState or UnInstall it
//         Notice : You don't need to close your driver: it's done automatically
// Parameters :
//     in  : LPCTSTR lpServiceName : the name of the service (name of your driver without .sys if you use
//                                   the install method of this class)
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CDriver::Open(LPCTSTR lpServiceName)
{
    BOOL bRet=TRUE;

    // close previous service if any
    if (this->h_SCService)
        CloseServiceHandle(this->h_SCService);

    this->h_SCService = OpenService(this->h_SCManager,lpServiceName,SERVICE_ALL_ACCESS);
    if (!this->h_SCService)
    {
        CAPIError::ShowLastError();
        bRet=FALSE;
    }

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: Start
// Object: Start the opened driver
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CDriver::Start()
{
    BOOL bAlreadyStarted=FALSE;
    return this->Start(&bAlreadyStarted);
}

//-----------------------------------------------------------------------------
// Name: Start
// Object: Start the opened driver
// Parameters :
//     in  : 
//     out : BOOL* pbAlreadyStarted : TRUE if driver was already started
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CDriver::Start(BOOL* pbAlreadyStarted)
{
    DWORD dwLastError;
    BOOL bRet=FALSE;
    if (IsBadWritePtr(pbAlreadyStarted,sizeof(BOOL)))
        return FALSE;
    *pbAlreadyStarted=FALSE;

    if (!this->h_SCService)
        return FALSE;

    bRet=StartService(this->h_SCService,   //SC_HANDLE hService,
                            0,             //DWORD dwNumServiceArgs,
                            NULL           //LPCTSTR* lpServiceArgVectors
                            );
    if (!bRet)
    {
        dwLastError=GetLastError();
        if (dwLastError==ERROR_SERVICE_ALREADY_RUNNING)
        {
            bRet=TRUE;
            *pbAlreadyStarted=TRUE;
        }
        else
            CAPIError::ShowError(dwLastError);
    }
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: Stop
// Object: Stop the opened driver
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CDriver::Stop()
{
    DWORD dwLastError;
    DWORD dwCurrentState=0;
    SERVICE_STATUS ServiceStatus;
    BOOL bRet;

    if (!this->h_SCService)
        return FALSE;
    if (this->GetState(&dwCurrentState))
    {
        if ((dwCurrentState==SERVICE_STOPPED)
            ||(dwCurrentState==SERVICE_STOP_PENDING))
            return TRUE;
    }
    bRet=ControlService(this->h_SCService,//SC_HANDLE hService,
                             SERVICE_CONTROL_STOP,//DWORD dwControl,
                             &ServiceStatus//LPSERVICE_STATUS lpServiceStatus
                             );
    if (!bRet)
    {
        dwLastError=GetLastError();
        if (dwLastError==ERROR_SERVICE_CANNOT_ACCEPT_CTRL)
        {
            // The requested control code cannot be sent to the service because the state 
            // of the service is SERVICE_STOPPED, SERVICE_START_PENDING, or SERVICE_STOP_PENDING.
            this->GetState(&dwCurrentState);
        }
        if ((dwLastError==ERROR_SERVICE_NOT_ACTIVE)
            ||(dwCurrentState==SERVICE_STOPPED)
            ||(dwCurrentState==SERVICE_STOP_PENDING))
            bRet=TRUE;
        else
            CAPIError::ShowError(dwLastError);
    }
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: UnInstall
// Object: UnInstall the opened driver
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CDriver::UnInstall()
{
    BOOL bRet;

    if (!this->h_SCService)
        return FALSE;

    if (!this->Stop())
        return FALSE;

    bRet=DeleteService(this->h_SCService);
    if (!bRet)
        CAPIError::ShowLastError();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: GetState
// Object: get state of a driver
// Parameters :
//     in  : 
//     out : DWORD* dwCurrentState : one of the following value :
//                                  SERVICE_CONTINUE_PENDING The service continue is pending. 
//                                  SERVICE_PAUSE_PENDING The service pause is pending. 
//                                  SERVICE_PAUSED The service is paused. 
//                                  SERVICE_RUNNING The service is running. 
//                                  SERVICE_START_PENDING The service is starting. 
//                                  SERVICE_STOP_PENDING The service is stopping. 
//                                  SERVICE_STOPPED The service is not running. 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CDriver::GetState(DWORD* dwCurrentState)
{
    BOOL bRet;
    SERVICE_STATUS ServiceStatus;
    if (!this->h_SCService)
        return FALSE;

    if (IsBadWritePtr(&dwCurrentState,sizeof(DWORD)))
        return FALSE;

    bRet=QueryServiceStatus(this->h_SCService,&ServiceStatus);
    if (!bRet)
        CAPIError::ShowLastError();
    else
        *dwCurrentState=ServiceStatus.dwCurrentState;

    return bRet;
}