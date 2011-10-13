//---------------------------------------------------------------------------
//
// NtProcessMonitor.h
//
// SUBSYSTEM: 
//				API Hooking system
// MODULE:    
//				Implements a thread that uses an NT device driver
//              for monitoring process creation
//
// DESCRIPTION:
//
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
//                                                                         
//---------------------------------------------------------------------------

#include "..\Common\Common.h"
#include "NtProcessMonitor.h"
#include "..\Common\SysUtils.h"
#include <process.h>
#include <winioctl.h>
#include "NtDriverController.h"

//---------------------------------------------------------------------------
//
// File scope consts and typedefs
//
//---------------------------------------------------------------------------

#define FILE_DEVICE_UNKNOWN             0x00000022
#define IOCTL_UNKNOWN_BASE              FILE_DEVICE_UNKNOWN

#define IOCTL_PROCVIEW_GET_PROCINFO     CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)


//---------------------------------------------------------------------------
//
// Thread function prototype
//
//---------------------------------------------------------------------------
typedef unsigned (__stdcall *PTHREAD_START) (void *);
//---------------------------------------------------------------------------
//
// class CNtProcessMonitor
//
//---------------------------------------------------------------------------

CNtProcessMonitor::CNtProcessMonitor():
	m_hShutdownEvent(NULL),
	m_hProcessEvent(NULL),
	m_bThreadActive(FALSE),
	m_dwThreadId(0),
	m_hDriver(INVALID_HANDLE_VALUE),
	m_pDriverCtl(NULL)
{
	if (!IsWindows9x())
		m_pDriverCtl = new CNtDriverController();
}

CNtProcessMonitor::~CNtProcessMonitor()
{
	delete m_pDriverCtl;
}

//
// Accessor method
//
BOOL CNtProcessMonitor::Get_ThreadActive()
{
	CLockMgr<CCSWrapper> lockMgr(m_CritSec, TRUE);	
	return m_bThreadActive;
}

//
// Accessor method
//
void CNtProcessMonitor::Set_ThreadActive(BOOL val)
{
	CLockMgr<CCSWrapper> lockMgr(m_CritSec, TRUE);	
	m_bThreadActive = val;
}


//
// Accessor method
//
HANDLE CNtProcessMonitor::Get_ShutdownEvent() const
{
	return m_hShutdownEvent;
}

//
// Accessor method
//
HANDLE CNtProcessMonitor::Get_ProcessEvent() const
{
	return m_hProcessEvent;
}


//
// Activate / Stop the thread which gets the notification from the 
// device driver
//
void CNtProcessMonitor::SetActive(BOOL bVal)
{
	if (m_pDriverCtl)
	{
		if (bVal)
		{
			if (!Get_ThreadActive())
			{
				// Terminal Services W2K/XP: The name can have a "Global\" 
				// prefix to explicitly create the object in the global 
				// or session name space.
				char szDriverName[MAX_PATH];
				if ( IsWindowsNT4() )
					strcpy(szDriverName, "\\\\.\\NTProcDrv");
				else
					strcpy(szDriverName, "\\\\.\\Global\\NTProcDrv");				

				// Try opening the device driver
				m_hDriver = CreateFile(
					szDriverName,
					GENERIC_READ | GENERIC_WRITE, 
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					0,                     // Default security
					OPEN_EXISTING,
					FILE_FLAG_OVERLAPPED,  // Perform asynchronous I/O
					0);                    // No template
        
				if(INVALID_HANDLE_VALUE == m_hDriver)
					return;
    				
				m_hShutdownEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
				// Attach to KM-created event handle
				m_hProcessEvent = ::OpenEvent(
					SYNCHRONIZE, FALSE, "NTProcDrvProcessEvent");

				_beginthreadex(
					(void *)NULL,
					(unsigned)0,
					(PTHREAD_START)CNtProcessMonitor::ThreadFunc,
					(PVOID)this,
					(unsigned)0,
					(unsigned *)&m_dwThreadId
					);
			} // if
		} 
		else
		{
			if (Get_ThreadActive())
			{
				::SetEvent(m_hShutdownEvent);
				// Give some time to the injector thread to clean up itself
				while (Get_ThreadActive())
				{
				}
				::CloseHandle(m_hShutdownEvent);
				::CloseHandle(m_hProcessEvent);
				m_dwThreadId = 0;
				if (NULL != m_hDriver)
					::CloseHandle(m_hDriver);
			} // if
		} // else
	} // if
}

//
// Retrieves data from the driver after received notification 
//
void CNtProcessMonitor::RetrieveProcessInfo(
	CALLBACK_INFO& callbackInfo,
	CALLBACK_INFO& callbackTemp
	)
{
	OVERLAPPED ov          = { 0 };
	BOOL       bReturnCode = FALSE;
	DWORD      dwBytesReturned;

    // Create an event handle for async notification from the driver
	ov.hEvent = ::CreateEvent(
		NULL,  // Default security
		TRUE,  // Manual reset
		FALSE, // non-signaled state
		NULL
		); 

	// Get the process info
	bReturnCode = ::DeviceIoControl(
		m_hDriver,
		IOCTL_PROCVIEW_GET_PROCINFO,
		0, 
		0,
		&callbackInfo, sizeof(callbackInfo),
		&dwBytesReturned,
		&ov
		);

	// Wait here for the event handle to be set, indicating
	// that the IOCTL processing is completed.
	bReturnCode = ::GetOverlappedResult(
		m_hDriver, 
		&ov,
		&dwBytesReturned, 
		TRUE
		);

	::CloseHandle(ov.hEvent);

	// Pevent getting duplicated events
	if((callbackTemp.ParentId  != callbackInfo.ParentId) ||
	   (callbackTemp.ProcessId != callbackInfo.ProcessId) ||
	   (callbackTemp.bCreate    != callbackInfo.bCreate))
	{
		if(callbackInfo.bCreate)
			// Process creation notification
			OnCreateProcess((DWORD)callbackInfo.ProcessId);
		else
			// Process termination notification
			OnTerminateProcess((DWORD)callbackInfo.ProcessId);
	} // if

	// Store the data for next time
	callbackTemp = callbackInfo;
}

//
// The thread function 
//
unsigned __stdcall CNtProcessMonitor::ThreadFunc(void* pvParam)
{
	CNtProcessMonitor* me = (CNtProcessMonitor*)pvParam;
	CALLBACK_INFO callbackInfo, callbackTemp;
	DWORD dwResult; 
	HANDLE handles[2] = 
	{
		me->Get_ShutdownEvent(),
		me->Get_ProcessEvent()
	};
	me->Set_ThreadActive(TRUE);

	while (TRUE)
	{
		dwResult = ::WaitForMultipleObjects(
			sizeof(handles)/sizeof(handles[0]), // number of handles in array
			&handles[0],                        // object-handle array
			FALSE,                              // wait option
			INFINITE                            // time-out interval
			);
		//	
		// the system shuts down
		//
		if (handles[dwResult - WAIT_OBJECT_0] == me->Get_ShutdownEvent())
			break;
		//
		// A new process has been just created / or terminated
		//
		else
			me->RetrieveProcessInfo(
				callbackInfo, 
				callbackTemp
				);
	} // while
	me->Set_ThreadActive( FALSE );
	_endthreadex(0);
	return 0;
}

//----------------------------End of the file -------------------------------