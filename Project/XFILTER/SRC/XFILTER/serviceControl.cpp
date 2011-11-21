///////////////////////////////////////////////////////////////////////////////
//
//	(C) Copyright 1999 - 2000 Mark Roddy
//	All Rights Reserved
//
//	Hollis Technology Solutions
//	94 Dow Road
//	Hollis, NH 03049
//	info@hollistech.com
//
//	Synopsis: 
// 
//
//	Version Information:
//
//	$Header: /iphook/usr/IpMonitor/serviceControl.cpp 2     1/27/00 10:35p Markr $ 
//
///////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ServiceControl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ServiceControl::ServiceControl()
{
	driverHandle = INVALID_HANDLE_VALUE;
	SchSCManager = (SC_HANDLE) INVALID_HANDLE_VALUE;   
    UnloadInDestructor = TRUE;
    RemoveInDestructor = FALSE;
    lastError = 0;
	ChangeIfExists = FALSE;

}

DWORD ServiceControl::init(LPCTSTR Name, 
					 DWORD flagsAndAttributes, 
					 LPCTSTR Path, 
					 LPCTSTR DosName)
{	

	attributes = flagsAndAttributes;
	DriverName = Name;
	if (Path) {

		BinaryPath = Path;

	} else {
        //
        // allow two choices, first the CWD, second the system root.
        //
        DWORD length = GetCurrentDirectory(0, NULL);

        CString cwd;
        
        LPTSTR buff = cwd.GetBufferSetLength(length);

        DWORD result = GetCurrentDirectory(length, buff);

        cwd.ReleaseBuffer();

        
        if (result != 0xffffffff) {

            BinaryPath = cwd + CString("\\") + DriverName + CString(".sys");

            result = GetFileAttributes(LPCTSTR(BinaryPath));
        }
        if (result == 0xffffffff) {
            
            BinaryPath = CString("%SystemRoot%\\system32\\Drivers\\") + DriverName + CString(".sys");
        }
	}

	if (DosName) {
		DosDevice = DosName;
	} else {
		DosDevice = DriverName;
	}


    //
    // if the current string is not the fully qualified
    // dos device name, make it so.
    //
    if ( CString(DosDevice.Left(2)) != CString("\\\\")) {
	    
        DosDevice = CString("\\\\.\\") + DosDevice;

    }
    try {

        LoadDriver();
    }
    catch (DWORD error)
    {
        //
        // don't throw a simple interface error as
        // an exception or we get a leak (ugh.)
        //
        lastError = error;
    }

    if (lastError != 0) {

        try {
            closeDevice();
        }
        catch (...) {};
    }

	return lastError;    		
}

ServiceControl::~ServiceControl()
{
    try {

        closeDevice();
    }
    catch (...) {};

    if (UnloadInDestructor) {

	    try {

		    UnloadDriver();
	    } 
	    catch (...) {};
    }

}


/****************************************************************************
*
*    FUNCTION: LoadDeviceDriver( const TCHAR, const TCHAR, HANDLE *)
*
*    PURPOSE: Registers a driver with the system configuration manager 
*	 and then loads it.
*
****************************************************************************/
void ServiceControl::LoadDriver()
{
	BOOL		okay;

	SchSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if (SchSCManager == INVALID_HANDLE_VALUE) {
		throw GetLastError();
	}

	// Ignore success of installation: it may already be installed.
	(void) InstallDriver();

	// Ignore success of start: it may already be started.
	(void) StartDriver();

	// Do make sure we can open it.
	okay = OpenDevice();
	if (!okay) {
		throw GetLastError();
	}

 	SchSCManager = closeSVC(SchSCManager);

}


/****************************************************************************
*
*    FUNCTION: InstallDriver( IN SC_HANDLE, IN LPCTSTR, IN LPCTSTR)
*
*    PURPOSE: Creates a driver service.
*
****************************************************************************/
BOOL ServiceControl::InstallDriver( )
{
    SC_HANDLE  schService;

    //
    // NOTE: This creates an entry for a standalone driver. If this
    //       is modified for use with a driver that requires a Tag,
    //       Group, and/or Dependencies, it may be necessary to
    //       query the registry for existing driver information
    //       (in order to determine a unique Tag, etc.).
    //

    schService = CreateService( SchSCManager,          // SCManager database
                                DriverName,           // name of service
                                DriverName,           // name to display
                                SERVICE_ALL_ACCESS,    // desired access
                                SERVICE_KERNEL_DRIVER, // service type
                                SERVICE_DEMAND_START,  // start type
                                SERVICE_ERROR_NORMAL,  // error control type
                                BinaryPath,		           // service's binary
                                NULL,                  // no load ordering group
                                NULL,                  // no tag identifier
                                NULL,                  // no dependencies
                                NULL,                  // LocalSystem account
                                NULL                   // no password
                                );
    if ( schService == NULL ) {
		//
		// Fine, try to Open it and change it
		//
		schService =  OpenService(SchSCManager, DriverName, SERVICE_ALL_ACCESS);
		
		if (schService == NULL) {
			//
			// this is truly hosed
			//
			return FALSE;

		} else if (ChangeIfExists) {

			if (FALSE == ChangeServiceConfig(
									schService,	// handle to service 
									SERVICE_KERNEL_DRIVER,	// type of service 
									SERVICE_DEMAND_START,	// when to start service 
									SERVICE_ERROR_NORMAL,	// severity if service fails to start 
									BinaryPath,	// pointer to service binary file name 
									NULL,	// pointer to load ordering group name 
									NULL,	// pointer to variable to get tag identifier 
									NULL,	// pointer to array of dependency names 
									NULL,	// pointer to account name of service 
									NULL,	// pointer to password for service account  
									DriverName 	// pointer to display name 
					)) {
				//
				// oh mama, we are not going to be happy
				//
				return FALSE;
			}
		}
	}

    schService = closeSVC( schService );

    return TRUE;
}


/****************************************************************************
*
*    FUNCTION: StartDriver( IN SC_HANDLE, IN LPCTSTR)
*
*    PURPOSE: Starts the driver service.
*
****************************************************************************/
BOOL ServiceControl::StartDriver()
{
    SC_HANDLE  schService;
    BOOL       ret;

    schService = OpenService( SchSCManager,
                              DriverName,
                              SERVICE_ALL_ACCESS
                              );
    
	if ( schService == NULL ) {
        return FALSE;
	}

    ret = StartService( schService, 0, NULL );

    if (!ret) {
        if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) {

            ret = 1; // OK
            //
            // we didn't start it, we don't unload it
            //
            UnloadInDestructor = FALSE;
        }
    }

    //
    // whatever happened close our handle to the SCManager
    //

    schService = closeSVC( schService );

    return ret;
}



/****************************************************************************
*
*    FUNCTION: OpenDevice( IN LPCTSTR, HANDLE *)
*
*    PURPOSE: Opens the device and returns a handle if desired.
*
****************************************************************************/
BOOL ServiceControl::OpenDevice( )
{

   
	if (driverHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(driverHandle);
	}
    driverHandle = CreateFile( DosDevice,
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          attributes,
                          NULL
                          );

    return driverHandle != INVALID_HANDLE_VALUE;
}


/****************************************************************************
*
*    FUNCTION: UnloadDeviceDriver( const TCHAR *)
*
*    PURPOSE: Stops the driver and has the configuration manager unload it.
*
****************************************************************************/
void ServiceControl::UnloadDriver()
{
	if (SchSCManager == INVALID_HANDLE_VALUE) {
		SchSCManager = OpenSCManager(	NULL,                 // machine (NULL == local)
                              			NULL,                 // database (NULL == default)
										SC_MANAGER_ALL_ACCESS // access required
									);
	}

	if (SchSCManager != INVALID_HANDLE_VALUE) {

        if (TRUE == StopDriver()) {

            if (RemoveInDestructor) {
		 
                RemoveDriver();

            }

        }
		 
		SchSCManager = closeSVC(SchSCManager);
	}

}



/****************************************************************************
*
*    FUNCTION: StopDriver( IN SC_HANDLE, IN LPCTSTR)
*
*    PURPOSE: Has the configuration manager stop the driver (unload it)
*
****************************************************************************/
BOOL ServiceControl::StopDriver()
{
    SC_HANDLE       schService;
    BOOL            ret;
    SERVICE_STATUS  serviceStatus;

    schService = OpenService( SchSCManager, DriverName, SERVICE_ALL_ACCESS );
    if ( schService == NULL )
        return FALSE;

	closeDevice();

    ret = ControlService( schService, SERVICE_CONTROL_STOP, &serviceStatus );

    schService = closeSVC( schService );

    return ret;
}


/****************************************************************************
*
*    FUNCTION: RemoveDriver( IN SC_HANDLE, IN LPCTSTR)
*
*    PURPOSE: Deletes the driver service.
*
****************************************************************************/
BOOL ServiceControl::RemoveDriver( )
{
    SC_HANDLE  schService;
    BOOL       ret;

    
    schService = OpenService( SchSCManager,
                              DriverName,
                              SERVICE_ALL_ACCESS
                              );

    if ( schService == NULL ) {
        return FALSE;
    }

    ret = DeleteService( schService );

    schService = closeSVC( schService );

    return ret;
}

void ServiceControl::RemoveDriverOnExit(BOOL val)
{
    RemoveInDestructor = val;

}
///////////////////////////////////////////////////////////////////////////////
// 
// Change History Log
//
// $Log: /iphook/usr/IpMonitor/serviceControl.cpp $
// 
// 2     1/27/00 10:35p Markr
// Prepare to release!
//
///////////////////////////////////////////////////////////////////////////////
#pragma comment( exestr, "B9D3B8FD2A756774786B65676571707674716E2B")
