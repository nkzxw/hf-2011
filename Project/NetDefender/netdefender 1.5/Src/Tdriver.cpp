//if you use this code in a mfc program:
//add the header stdafx.h or disable precompiled header
//Unless you do it, when compiling  vc will say: Unexpected end
//of file while looking for precompiled header
#include "stdafx.h"

#include "TDriver.h"
#include "path.h"

//Constructor. Initialize variables.

/*********************************************
 * Constructor. Initialize variables. 
 ********************************************/
TDriver::TDriver(void)
{
	driverHandle = NULL;

	m_bRemovable = TRUE;
	m_binitialized = FALSE;
	m_bLoaded = FALSE;
	m_bStarted = FALSE;
}

//Destructor. Free resources and unload the driver.

/*********************************************
 * Destructor. Free resources and unload the driver.
 * 
 ********************************************/
TDriver::~TDriver(void)
{
	if(driverHandle != NULL)
	{
		CloseHandle(driverHandle);
		driverHandle = NULL;
	}

	UnloadDriver();
}

//If removable = TRUE, the driver isnt unload when exit

/*********************************************
 * If removable = TRUE, the driver isnt unload when exit
 *
 ********************************************/
void TDriver::SetRemovable(BOOL value)
{
	m_bRemovable = value;
}

//is driver initialized?

/*********************************************
 * is driver initialized?
 ********************************************/
BOOL TDriver::IsInitialized(void)
{
	return(m_binitialized);
}

//is driver loaded?

/*********************************************
 * is driver loaded?
 *
 ********************************************/
BOOL TDriver::IsLoaded(void)
{
	return(m_bLoaded);
}

//is driver started?

/*********************************************
 * is driver started?
 *
 ********************************************/
BOOL TDriver::IsStarted(void)
{
	return(m_bStarted);
}

//Init the driver class variables

/*********************************************
 * Init the driver class variables
 *
 ********************************************/
DWORD TDriver::InitDriver(LPCTSTR path)
{
	//if already initialized, first unload
	if(m_binitialized)
	{
		if(UnloadDriver() != DRV_SUCCESS)
		{
			return(DRV_ERROR_ALREADY_INITIALIZED);
		}
	}
	CPath objPath ;
	objPath = path ;
	m_strDriverName = objPath.GetFileTitle();

	m_strDriverPath = path; 
	 
	m_strDriverDosName.Format( _T("\\\\.\\%s"), m_strDriverName);


	m_binitialized = TRUE;
	return(DRV_SUCCESS);
}

//Init the driver class variables

/*********************************************
 * Init the driver class variables
 ********************************************/
DWORD TDriver::InitDriver(LPCTSTR name, LPCTSTR path, LPCTSTR dosName)
{
	//if already initialized, first unload
	if(m_binitialized)
	{
		if(UnloadDriver() != DRV_SUCCESS)
		{
			return(DRV_ERROR_ALREADY_INITIALIZED);
		}
	}

	CString dirBuffer;

	//if the user introduced path, first i will ckeck it
	if(path != NULL)
	{
		dirBuffer = path;
	}
	else
	{
		//if the user dont introduced path, i search in curren directory
		TCHAR	pathBuffer[MAX_PATH	+1];

		if(GetCurrentDirectory(MAX_PATH+1, pathBuffer) != 0)
		{
			dirBuffer.Format(_T("%s\\%s.sys"), pathBuffer, name);

			//exists this file?
			if(GetFileAttributes(dirBuffer) == 0xFFFFFFFF)
			{
				//if no, i search in \system32\drivers\

				CString sysDriver = _T("\\system32\\Drivers\\");
				//LPTSTR	sysPath;
				TCHAR	sysPath[MAX_PATH	+1];

				if(GetWindowsDirectory(sysPath, MAX_PATH+1) == 0)
				{
					//free(sysPath);

					return(DRV_ERROR_UNKNOWN);
				}

				dirBuffer.Format(_T("%s%s.sys"), sysPath, name);
				//if the file neither exist, i dont know where is it -> i dont initialize
				if(GetFileAttributes(dirBuffer) == 0xFFFFFFFF)
				{
					return(DRV_ERROR_INVALID_PATH_OR_FILE);
				}
			}
		}
		else
		{
			return(DRV_ERROR_UNKNOWN);
		}
	}

	//Write driver's variables with obtained data
	m_strDriverPath = dirBuffer;

	m_strDriverName = name;

	LPCTSTR auxBuffer;
	if(dosName != NULL)
	{
		auxBuffer = dosName;
	}
	else
	{
		auxBuffer = name;
	}

	if(auxBuffer[0] != '\\' && auxBuffer[1] != '\\')
	{
		m_strDriverDosName = auxBuffer; //(LPTSTR) malloc(_tcslen(auxBuffer) + 5);

		m_strDriverDosName.Format(_T("\\\\.\\%s"), auxBuffer);

	}
	else
	{
		m_strDriverDosName = auxBuffer;

	}

	//set the state to initialized
	m_binitialized = TRUE;

	return(DRV_SUCCESS);
}

//Function to Load the driver.
DWORD TDriver::LoadDriver(LPCTSTR name, LPCTSTR path, LPCTSTR dosName, BOOL start)
{
	//first initialized it
	DWORD	retCode = InitDriver(name, path, dosName);

	//then load
	if(retCode == DRV_SUCCESS)
	{
		retCode = LoadDriver(start);
	}

	return(retCode);
}

//Function to load the driver
DWORD TDriver::LoadDriver(LPCTSTR path, BOOL start)
{
	//first initialized it
	DWORD	retCode = InitDriver(path);

	//then load
	if(retCode == DRV_SUCCESS)
	{
		retCode = LoadDriver(start);
	}

	return(retCode);
}

//Function to Load the driver
DWORD TDriver::LoadDriver(BOOL start)
{
	//if the driver is already started, i havent to do nothing
	if(m_bLoaded)
	{
		return(DRV_SUCCESS);
	}

	if(!m_binitialized)
	{
		return(DRV_ERROR_NO_INITIALIZED);
	}

	//Open Service manager to create the new "service"
	SC_HANDLE	SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	DWORD		retCode = DRV_SUCCESS;

	if(SCManager == NULL)
	{
		return(DRV_ERROR_SCM);
	}

	//Create the driver "service"
	SC_HANDLE	SCService = CreateService(SCManager,				// SCManager database
										  m_strDriverName,				// nombre del servicio
										  m_strDriverName,				// nombre a mostrar
										  SERVICE_ALL_ACCESS,		// acceso total
										  SERVICE_KERNEL_DRIVER,	// driver del kernel
										  SERVICE_DEMAND_START,		// comienzo bajo demanda
										  SERVICE_ERROR_NORMAL,		// control de errores normal
										  m_strDriverPath,				// path del driver
										  NULL,						// no pertenece a un grupo
										  NULL,						// sin tag
										  NULL,						// sin dependencias
										  NULL,						// cuenta local del sistema
										  NULL						// sin password
										  );

	//if i cant create, first i check if the driver already was loaded.
	if(SCService == NULL)
	{
		SCService = OpenService(SCManager, m_strDriverName, SERVICE_ALL_ACCESS);

		if(SCService == NULL)
		{
			retCode = DRV_ERROR_SERVICE;
		}
	}

	CloseServiceHandle(SCService);
	SCService = NULL;

	CloseServiceHandle(SCManager);
	SCManager = NULL;

	//if all ok, update the state and start if necessary
	if(retCode == DRV_SUCCESS)
	{
		m_bLoaded = TRUE;

		if(start)
		{
			retCode = StartDriver();
		}
	}

	return(retCode);
}

//Function to Unload a driver

/*********************************************
 * Function to Unload a driver
 *
 ********************************************/
DWORD TDriver::UnloadDriver(BOOL forceClearData)
{
	DWORD	retCode = DRV_SUCCESS;

	//if the driver is started, first i will stop it
	if(m_bStarted)
	{
		if((retCode = StopDriver()) == DRV_SUCCESS)
		{
			//i only remove it, if it is mark to be removable
			if(m_bRemovable)
			{
				//open service and delete it
				SC_HANDLE	SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

				if(SCManager == NULL)
				{
					return(DRV_ERROR_SCM);
				}

				SC_HANDLE	SCService = OpenService(SCManager, m_strDriverName, SERVICE_ALL_ACCESS);

				if(SCService != NULL)
				{
					if(!DeleteService(SCService))
					{
						retCode = DRV_ERROR_REMOVING;
					}
					else
					{
						retCode = DRV_SUCCESS;
					}
				}
				else
				{
					retCode = DRV_ERROR_SERVICE;
				}

				CloseServiceHandle(SCService);
				SCService = NULL;

				CloseServiceHandle(SCManager);
				SCManager = NULL;

				//if all ok, update the state
				if(retCode == DRV_SUCCESS)
				{
					m_bLoaded = FALSE;
				}
			}
		}
	}

	//if the driver is initialized...
	if(m_binitialized)
	{
		//if there was some problem but i mark foreceClear, i will remove the data
		if(retCode != DRV_SUCCESS && forceClearData == FALSE)
		{
			return(retCode);
		}

		//update the state
		m_binitialized = FALSE;

	}

	return(retCode);
}

//Function to start the driver "service"
DWORD TDriver::StartDriver(void)
{
	//if already started, all ok
	if(m_bStarted)
	{
		return(DRV_SUCCESS);
	}

	//open the service manager and the service and change driver state
	SC_HANDLE	SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	DWORD		retCode;

	if(SCManager == NULL)
	{
		return(DRV_ERROR_SCM);
	}

	SC_HANDLE	SCService = OpenService(SCManager, m_strDriverName, SERVICE_ALL_ACCESS);

	if(SCService == NULL)
	{
		return(DRV_ERROR_SERVICE);
	}

	if(!StartService(SCService, 0, NULL))
	{
		//if the driver was started before i try to do it,
		//i will not remove, because it was created by other application
		if(GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
		{
			m_bRemovable = FALSE;

			retCode = DRV_SUCCESS;
		}
		else
		{
			retCode = DRV_ERROR_STARTING;
		}
	}
	else
	{
		retCode = DRV_SUCCESS;
	}

	CloseServiceHandle(SCService);
	SCService = NULL;

	CloseServiceHandle(SCManager);
	SCManager = NULL;

	//update the state and open device
	if(retCode == DRV_SUCCESS)
	{
		m_bStarted = TRUE;

		retCode = OpenDevice();
	}

	return(retCode);
}

//Function to stop driver "service"
DWORD TDriver::StopDriver(void)
{
	//if already stopped, all ok
	if(!m_bStarted)
	{
		return(DRV_SUCCESS);
	}

	//open the service manager and the service and change driver state
	SC_HANDLE	SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	DWORD		retCode;

	if(SCManager == NULL)
	{
		return(DRV_ERROR_SCM);
	}

	SERVICE_STATUS	status;

	SC_HANDLE		SCService = OpenService(SCManager, m_strDriverName, SERVICE_ALL_ACCESS);

	if(SCService != NULL)
	{
		//close the driver handle too
		CloseHandle(driverHandle);
		driverHandle = NULL;

		if(!ControlService(SCService, SERVICE_CONTROL_STOP, &status))
		{
			retCode = DRV_ERROR_STOPPING;
		}
		else
		{
			retCode = DRV_SUCCESS;
		}
	}
	else
	{
		retCode = DRV_ERROR_SERVICE;
	}

	CloseServiceHandle(SCService);
	SCService = NULL;

	CloseServiceHandle(SCManager);
	SCManager = NULL;

	//update the state
	if(retCode == DRV_SUCCESS)
	{
		m_bStarted = FALSE;
	}

	return(retCode);
}

//Funtion to open a driver handle

/*********************************************
 * Funtion to open a driver handle
 *
 ********************************************/
DWORD TDriver::OpenDevice(void)
{
	//if i already have a handle, first close it
	if(driverHandle != NULL)
	{
		CloseHandle(driverHandle);
	}

	driverHandle = CreateFile(m_strDriverDosName,
							  GENERIC_READ | GENERIC_WRITE,
							  0,
							  NULL,
							  OPEN_EXISTING,
							  FILE_ATTRIBUTE_NORMAL,
							  NULL);

	if(driverHandle == INVALID_HANDLE_VALUE)
	{
		return(DRV_ERROR_INVALID_HANDLE);
	}

	return(DRV_SUCCESS);
}

//Return the driverHandle obtained
HANDLE TDriver::GetDriverHandle(void)
{
	return(driverHandle);
}

//Funtion to send data to the driver

/*********************************************
 * Funtion to send data to the driver
 *
 ********************************************/
DWORD TDriver::WriteIo(DWORD code, PVOID buffer, DWORD count)
{
	if(driverHandle == NULL)
	{
		return(DRV_ERROR_INVALID_HANDLE);
	}

	DWORD	bytesReturned;

	BOOL	returnCode = DeviceIoControl(driverHandle, code, buffer, count, NULL, 0, &bytesReturned, NULL);

	if(!returnCode)
	{
		return(DRV_ERROR_IO);
	}

	return(DRV_SUCCESS);
}

//Functions to read data from the driver

/*********************************************
 * Functions to read data from the driver
 *
 ********************************************/
DWORD TDriver::ReadIo(DWORD code, PVOID buffer, DWORD count)
{
	if(driverHandle == NULL)
	{
		return(DRV_ERROR_INVALID_HANDLE);
	}

	DWORD	bytesReturned;
	BOOL	retCode = DeviceIoControl(driverHandle, code, NULL, 0, buffer, count, &bytesReturned, NULL);

	if(!retCode)
	{
		return(DRV_ERROR_IO);
	}

	return(bytesReturned);
}

//Function to do IO operation with the driver, read or write or both

/*********************************************
 * Function to do IO operation with the driver, read or write or both
 *
 ********************************************/
DWORD TDriver::RawIo(DWORD code, PVOID inBuffer, DWORD inCount, PVOID outBuffer, DWORD outCount)
{
	if(driverHandle == NULL)
	{
		return(DRV_ERROR_INVALID_HANDLE);
	}

	DWORD	bytesReturned;
	BOOL	retCode = DeviceIoControl(driverHandle, code, inBuffer, inCount, outBuffer, outCount, &bytesReturned, NULL);

	if(!retCode)
	{
		return(DRV_ERROR_IO);
	}

	return(bytesReturned);
}
