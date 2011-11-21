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
//	$Header: /iphook/usr/IpMonitor/serviceControl.h 2     1/27/00 10:35p Markr $ 
//
///////////////////////////////////////////////////////////////////////////////
#ifndef HTS_SERVICE_CONTROL
#define HTS_SERVICE_CONTROL
///////////////////////////////////////////////////////////////////////////////
//
// ServiceControl.h
//
// This class defines a service control object. The service control object provides a simple tool to
// dynamically load and unload a device driver in windows nt.
//
// The simplest way to use this class is to allocate an instance using new, which will install load and start
// your driver. Conversely delete will stop and unload your driver. 
//
// The public constructor will throw an integer exception if it fails.
// The value of the integer exception is one of the return values from GetLastError().
//
// If the constructor succeeds, the driver is up and running, and the handle() method is a valid handle to the
// device.
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "winsvc.h"

class ServiceControl {
	//
	// private data
	//
private:
	HANDLE driverHandle;
	SC_HANDLE SchSCManager;
	CString DriverName;
	CString BinaryPath;
	CString DosDevice;
    DWORD lastError;
	BOOL RemoveInDestructor;
	DWORD attributes;
    BOOL UnloadInDestructor;
	BOOL ChangeIfExists;

	//
	// constructor/destructor
	//
private:

	ServiceControl& operator=(const ServiceControl& lhs);

	ServiceControl(const ServiceControl& lhs);

public:
	//
	// input: 
	//
	//	Name -- the display name for the driver.
    //
    //  flagsAndAttributes -- how the device will be opened.
	//
	//	Path -- the fully qualified path of the driver executible.
	//			DEFAULT VALUE %SYSTEMROOT%\system32\drivers\"Name".sys,
	//
	//	DosName -- the user space visible device name (dos device name) used to access devices
	//		supported by the driver.
	//				DEFAULT VALUE "Name". (i.e. \\.\Name)
	//
	ServiceControl();
	~ServiceControl();

	DWORD init(LPCTSTR Name, 
			   DWORD flagsAndAttributes=FILE_ATTRIBUTE_NORMAL, 
			   LPCTSTR Path=NULL, 
			   LPCTSTR DosName=NULL);

    DWORD getError() { return lastError; }

	void ChangeConfig(BOOL val) { ChangeIfExists = val; }

	//
	// public methods
	//
public:
	void RemoveDriverOnExit(BOOL val);

	HANDLE handle() { return driverHandle; }

    void UnloadDriverOnExit(BOOL val) { UnloadInDestructor = val; }
	//
	// private methods
	//
private:
	
	void LoadDriver();

	void UnloadDriver();

	BOOL InstallDriver();

	BOOL StartDriver();

	BOOL OpenDevice();

	BOOL StopDriver();

	BOOL RemoveDriver();

	SC_HANDLE closeSVC(SC_HANDLE scHandle) 
	{ 
		CloseServiceHandle(scHandle); 
        return (SC_HANDLE) INVALID_HANDLE_VALUE; 
	}

	void closeDevice() 
	{ 
		CloseHandle(driverHandle); driverHandle = INVALID_HANDLE_VALUE; 
	}

};
#endif
///////////////////////////////////////////////////////////////////////////////
// 
// Change History Log
//
// $Log: /iphook/usr/IpMonitor/serviceControl.h $
// 
// 2     1/27/00 10:35p Markr
// Prepare to release!
//
///////////////////////////////////////////////////////////////////////////////