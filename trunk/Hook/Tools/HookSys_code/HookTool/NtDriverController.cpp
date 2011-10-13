//---------------------------------------------------------------------------
//
// NtDriverController.cpp
//
// SUBSYSTEM: 
//				API Hooking system
// MODULE:    
//				Provides simple interface for managing device driver 
//              administration
//
// DESCRIPTION:
//
// AUTHOR:		Ivo Ivanov (ivopi@hotmail.com)
//                                                                         
//---------------------------------------------------------------------------
#include "..\Common\Common.h"
#include "NtDriverController.h"
#include "..\Common\SysUtils.h"

//---------------------------------------------------------------------------
//
// class CNtDriverController
//
//---------------------------------------------------------------------------

CNtDriverController::CNtDriverController():
	m_hSCM(NULL),
	m_hDriver(NULL),
	m_bDriverStarted(FALSE),
	m_bErrorOnStart(FALSE)
{
	if (TRUE == Open())
	{
		strcpy(m_szName, "NTProcDrv");
		strcpy(m_szInfo, "Process creation detector for NT.");
		char szFullFileName[MAX_PATH];
		GetProcessHostFullName(szFullFileName);
		if (TRUE == ReplaceFileName(szFullFileName, "NtProcDrv.sys", m_szFullFileName))
			m_bDriverStarted = InstallAndStart();
	} // if
}

CNtDriverController::~CNtDriverController()
{
	StopAndRemove();
	Close();
}

//
// Obtain manager handle
//
BOOL CNtDriverController::Open()
{
	m_hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	return (m_hSCM != NULL);
}

//
// Close handle obtained from Open()
//
void CNtDriverController::Close()
{
	if (m_hDriver != NULL)
	{
		::CloseServiceHandle(m_hDriver);
		m_hDriver = NULL;
	}
	if (m_hSCM != NULL)
	{
		::CloseServiceHandle(m_hSCM);
		m_hSCM = NULL;
	} 
}

//
// Wait until driver reaches desired state or error occurs
//
BOOL CNtDriverController::WaitForState(
	DWORD           dwDesiredState, 
	SERVICE_STATUS* pss
	) 
{
	BOOL bResult = FALSE;
	if (NULL != m_hDriver)
	{
		// Loop until driver reaches desired state or error occurs
		while (1)
		{
			// Get current state of driver
			bResult = ::QueryServiceStatus(m_hDriver, pss);
			// If we can't query the driver, we're done
			if (!bResult) 
				break;
			// If the driver reaches the desired state
			if (pss->dwCurrentState == dwDesiredState) 
				break;
			// We're not done, wait the specified period of time
			DWORD dwWaitHint = pss->dwWaitHint / 10;    // Poll 1/10 of the wait hint
			if (dwWaitHint <  1000) dwWaitHint = 1000;  // At most once a second
			if (dwWaitHint > 10000) dwWaitHint = 10000; // At least every 10 seconds
			::Sleep(dwWaitHint);
		} // while
	} // if

	return bResult;
}


//
// Add the driver to the system and start it up
//
BOOL CNtDriverController::InstallAndStart()
{
	BOOL bResult = FALSE;

	if (NULL != m_hSCM)
	{
		m_hDriver = ::CreateService(
			m_hSCM, 
			m_szName, 
			m_szInfo,
			SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_NORMAL,
			m_szFullFileName, 
			NULL, 
			NULL,
			NULL, 
			NULL, 
			NULL
			);
		if (NULL == m_hDriver)
		{
			if ( (::GetLastError() == ERROR_SERVICE_EXISTS) ||
			     (::GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE) )
				m_hDriver = ::OpenService(
					m_hSCM,
					m_szName,
					SERVICE_ALL_ACCESS
					);
		}
		if (NULL != m_hDriver)
		{
			SERVICE_STATUS serviceStatus = { 0 };
			bResult = ::StartService(m_hDriver, 0, NULL);
			if (bResult)
				bResult = WaitForState(SERVICE_RUNNING, &serviceStatus);	
			else
				bResult = (::GetLastError() == ERROR_SERVICE_ALREADY_RUNNING);
			// We should call DeleteService() if the SCM reports an error
			// on StartService(). Otherwise, the service will remain loaded
			// in an undesired state
			if (!bResult)
			{
				// Mark the service for deletion.
				::DeleteService(m_hDriver);
				if (m_hDriver != NULL)
				{
					::CloseServiceHandle(m_hDriver);
					m_hDriver = NULL;
				}
				m_bErrorOnStart = TRUE;
			}
		} // if
	} // if

	return bResult;
}

//
// Stop the driver and remove it from the system
//
void CNtDriverController::StopAndRemove()
{
	if ((NULL != m_hDriver) && (!m_bErrorOnStart))
	{
		BOOL bResult;
		SERVICE_STATUS serviceStatus = { 0 };
		// Notifies a service that it should stop. 
		bResult = ::ControlService(m_hDriver, SERVICE_CONTROL_STOP, &serviceStatus);
		if (bResult)
			bResult = WaitForState(SERVICE_STOPPED, &serviceStatus);	
		// Mark the service for deletion.
		::DeleteService(m_hDriver);
	} // if
}

//----------------------------End of the file -------------------------------