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

#pragma once

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#include "../APIError/APIError.h"



class CDriver
{
private:
    SC_HANDLE h_SCManager;
    SC_HANDLE h_SCService;
public:
    CDriver(void);
    ~CDriver(void);

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
    BOOL Install(LPCTSTR lpBinaryPathName,DWORD dwStartType,BOOL* pbAlreadyInstalled);

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
    BOOL Install(LPCTSTR lpBinaryPathName,DWORD dwStartType);

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
    BOOL Open(LPCTSTR lpServiceName);

    //-----------------------------------------------------------------------------
    // Name: Start
    // Object: Start the opened driver
    // Parameters :
    //     in  : 
    //     out : BOOL* pbAlreadyStarted : TRUE if driver was already started
    //     return : TRUE on success, FALSE on error
    //-----------------------------------------------------------------------------
    BOOL Start(BOOL* pbAlreadyStarted);

    //-----------------------------------------------------------------------------
    // Name: Start
    // Object: Start the opened driver
    // Parameters :
    //     in  : 
    //     out : 
    //     return : TRUE on success, FALSE on error
    //-----------------------------------------------------------------------------
    BOOL Start();

    //-----------------------------------------------------------------------------
    // Name: Stop
    // Object: Stop the opened driver
    // Parameters :
    //     in  : 
    //     out : 
    //     return : TRUE on success, FALSE on error
    //-----------------------------------------------------------------------------
    BOOL Stop();

    //-----------------------------------------------------------------------------
    // Name: UnInstall
    // Object: UnInstall the opened driver
    // Parameters :
    //     in  : 
    //     out : 
    //     return : TRUE on success, FALSE on error
    //-----------------------------------------------------------------------------
    BOOL UnInstall();

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
    BOOL GetState(DWORD* dwCurrentState);
};
