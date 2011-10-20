//if you use this code in a mfc program:
//add the header stdafx.h or disable precompiled header
//Unless you do it, when compiling  vc will say: Unexpected end 
//of file while looking for precompiled header

#include "stdafx.h"

#include <winsvc.h>

#include <setupapi.h>
#include "Driver.h"
#include "logprintcomm.h"

//Constructor. Initialize variables.
CDriver::CDriver(void)
{
	driverHandle    = NULL;
	   
	removable       = TRUE;

	driverName      = NULL;
	driverPath      = NULL;
	driverDosName   = NULL;

	initialized     = FALSE;
	loaded          = FALSE;
	started         = FALSE;
}

//
// Destructor. Free resources and unload the driver.
//
CDriver::~CDriver(void)
{
	if (NULL != driverHandle)
	{
		CloseHandle(driverHandle); 
		driverHandle = NULL; 
	}
   
    UnloadDriver();
}

//
// If removable = TRUE, the driver isnt unload when exit
//
void CDriver::SetRemovable(BOOL value)
{
	removable = value;
}

//
//is driver initialized?
//
BOOL CDriver::IsInitialized(void)
{
	return initialized;
}

//
// is driver loaded?
BOOL CDriver::IsLoaded(void)
//
{
	return loaded;
}

//
// is driver started?
//
BOOL CDriver::IsStarted(void)
{
	return started;
}

//
// Init the driver class variables
//
DWORD CDriver::InitDriver(LPCTSTR path)
{
	// If already initialized, first unload
	if (initialized)
	{
		if (DRV_SUCCESS != UnloadDriver())
        {
            return DRV_ERROR_ALREADY_INITIALIZED;
        }			
	}

	// If yes, i analized the path to extract driver name
	driverPath = (LPTSTR)malloc(strlen(path) + 1);

	if (NULL == driverPath)
    {	
        return DRV_ERROR_MEMORY;
    }
	
	strcpy(driverPath, path);

	// First I search the last backslash
	LPTSTR sPos1 = strrchr(driverPath, (int)'\\');

	// If null, the string havent any backslash
	if (sPos1 == NULL)
    {
        sPos1 = driverPath;
    }		

	// Now, i search .sys
	LPTSTR sPos2 = strrchr(sPos1, (int)'.');

	if (sPos2 == NULL || sPos1 > sPos2)
	{
		free(driverPath);
		driverPath = NULL;

		return DRV_ERROR_INVALID_PATH_OR_FILE;
	}
	
	// Extract the driver name
	driverName = (LPTSTR)malloc(sPos2 - sPos1);
	
	if (NULL == driverName)
	{
		free(driverPath);
		driverPath = NULL;

		return DRV_ERROR_MEMORY;
	}

	memcpy(driverName, sPos1 + 1, sPos2 - sPos1 - 1);
	
	driverName[sPos2 - sPos1 - 1] = 0;

	//driverDosName = \\.\driverName 
	driverDosName = (LPTSTR)malloc(strlen(driverName) + 5);

	if (NULL == driverDosName)
	{
		free(driverPath);
		driverPath = NULL;

		free(driverName);
		driverName = NULL;

		return DRV_ERROR_MEMORY;
	}

	sprintf(driverDosName, "\\\\.\\%s", driverName);

		
	initialized = TRUE;
	return DRV_SUCCESS;
}

//
// Init the driver class variables
//
DWORD CDriver::InitDriver(LPCTSTR name, LPCTSTR path, LPCTSTR dosName)
{	
	// If already initialized, first unload
	if (initialized)
	{
		if (DRV_SUCCESS != UnloadDriver())
        {
            return DRV_ERROR_ALREADY_INITIALIZED;
        }			
	}

	LPTSTR dirBuffer;

	// If the user introduced path, first I will ckeck it
	if (NULL != path) 
	{
		// If yes, copy in auxiliar buffer and continue
		DWORD len = (DWORD)(strlen(name) + strlen(path) + 1);
		dirBuffer = (LPTSTR)malloc(len);

		if (NULL == dirBuffer)
        {
			return DRV_ERROR_MEMORY;
        }

		strcpy(dirBuffer, path);
	}
	else 
	{
		// If the user dont introduced path, I search in curren directory
		LPTSTR pathBuffer;
        DWORD len = GetCurrentDirectory(0, NULL);
      
		pathBuffer = (LPTSTR)malloc(len);

		if (NULL == pathBuffer)
        {
            return DRV_ERROR_MEMORY;
        }
					        
        if (0 != GetCurrentDirectory(len, pathBuffer)) 
		{
			len = (DWORD)(strlen(pathBuffer) + strlen(name) + 6);
			dirBuffer = (LPTSTR)malloc(len);

			if (NULL == dirBuffer)
			{
				free(pathBuffer);

				return DRV_ERROR_MEMORY;
			}

			// complete de total path, currentdirectory\driverName.sys
			sprintf(dirBuffer, "%s\\%s.sys", pathBuffer, name);

			// exists this file?
			if (GetFileAttributes(dirBuffer) == 0xFFFFFFFF)
			{
				free(pathBuffer);
				free(dirBuffer);

				// If no, i search in \system32\drivers\ 
				LPCTSTR sysDriver = "\\system32\\Drivers\\";
				LPTSTR  sysPath;
	    	    
				//i have to get the windows directory
				DWORD len = GetWindowsDirectory(NULL, 0);
     			sysPath = (LPTSTR)malloc(len + strlen(sysDriver));

				if (NULL == sysPath)
                {
                    return DRV_ERROR_MEMORY;
                }
					
				if (0 == GetWindowsDirectory(sysPath, len)) 
				{
					free(sysPath);
					
					return DRV_ERROR_UNKNOWN;
				}
	
				//complete the path and check it
				strcat(sysPath, sysDriver);
				len = (DWORD)(strlen(sysPath) + strlen(name) + 5);

				dirBuffer = (LPTSTR)malloc(len);

				if (NULL == dirBuffer)
                {
                    return DRV_ERROR_MEMORY;
                }
					
				sprintf(dirBuffer, "%s%s.sys", sysPath, name);

				free(sysPath);

				// If the file neither exist, I dont know where is it -> I dont initialize
				if (GetFileAttributes(dirBuffer) == 0xFFFFFFFF)
				{
					free(dirBuffer);

					return DRV_ERROR_INVALID_PATH_OR_FILE;
				}
			}
        }
		else
		{
			free(pathBuffer);

			return DRV_ERROR_UNKNOWN;
		}
	}
	
	// Write driver's variables with obtained data
	driverPath = dirBuffer;

	driverName = (LPTSTR)malloc(strlen(name) + 1);

	if (NULL == driverName)
	{
		free(driverPath);
		driverPath = NULL;
		
		return DRV_ERROR_MEMORY;
	}

	strcpy(driverName, name);
	
	LPCTSTR auxBuffer;
	if (NULL != dosName)
    {
        auxBuffer = dosName;
    }
	else
    {
        auxBuffer = name;
    }
		
	//dosName = \\.\driverName
	if (auxBuffer[0] != '\\' && auxBuffer[1] != '\\')
	{
		driverDosName = (LPTSTR)malloc(strlen(auxBuffer) + 5);

		if (NULL == driverDosName)
		{
			free(driverPath);
			driverPath = NULL;

			free(driverName);
			driverName = NULL;

			return DRV_ERROR_MEMORY;
		}

		sprintf(driverDosName, "\\\\.\\%s", auxBuffer);
	}
	else
	{
		driverDosName = (LPTSTR)malloc(strlen(auxBuffer));

		if (NULL == driverDosName)
		{
			free(driverPath);
			driverPath = NULL;

			free(driverName);
			driverName = NULL;

			return DRV_ERROR_MEMORY;
		}

		strcpy(driverDosName, auxBuffer);
	}

	//set the state to initialized
	initialized = TRUE;

	return DRV_SUCCESS;
}

//
// Function to Load the driver.
//
DWORD CDriver::LoadDriver(LPCTSTR name, LPCTSTR path, LPCTSTR dosName, BOOL start)
{
	// First initialized it
	DWORD retCode = InitDriver(name, path, dosName);

	//then load
	if (DRV_SUCCESS == retCode)
    {
        retCode = LoadDriver(start);
    }
		
	return retCode;
}

//
// Function to load the driver
//
DWORD CDriver::LoadDriver(LPCTSTR path, BOOL start)
{
	//first initialized it
	DWORD retCode = InitDriver(path);

	//then load
	if (DRV_SUCCESS == retCode)
    {
        retCode = LoadDriver(start);
    }
		

	return retCode;
}

//
//Function to Load the driver
//
DWORD CDriver::LoadDriver(BOOL start)
{
	// if the driver is already started, i havent to do nothing
	if (loaded)
    {
        return DRV_SUCCESS;
    }
		
	if (! initialized)
    {
        return DRV_ERROR_NO_INITIALIZED;
    }		

	// Open Service manager to create the new "service"
	SC_HANDLE SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	DWORD retCode = DRV_SUCCESS;
	
	if (NULL == SCManager) 
    {
        return DRV_ERROR_SCM;
    }
		
    //Create the driver "service"
    SC_HANDLE  SCService = CreateService(SCManager,			    // SCManager database
									     driverName,            // nombre del servicio
							    		 driverName,            // nombre a mostrar
										 SERVICE_ALL_ACCESS,    // acceso total
										 SERVICE_KERNEL_DRIVER, // driver del kernel
										 SERVICE_DEMAND_START,  // comienzo bajo demanda
										 SERVICE_ERROR_NORMAL,  // control de errores normal
										 driverPath,	        // path del driver
										 NULL,                  // no pertenece a un grupo
										 NULL,                  // sin tag
										 NULL,                  // sin dependencias
										 NULL,                  // cuenta local del sistema
										 NULL                   // sin password
										);
    
	//if i cant create, first i check if the driver already was loaded.
	if (NULL == SCService) 
	{
		SCService = OpenService(SCManager, driverName, SERVICE_ALL_ACCESS);
		
		if (NULL == SCService) 
        {
            retCode = DRV_ERROR_SERVICE;
        }			
	}

    CloseServiceHandle(SCService);
	SCService = NULL;

	CloseServiceHandle(SCManager);
	SCManager = NULL;

	//if all ok, update the state and start if necessary
	if (DRV_SUCCESS == retCode)
	{
		loaded = TRUE;

		if (start)
        {
            retCode = StartDriver();
        }			
	}

	return retCode;
}

//
// Function to Unload a driver
//
DWORD CDriver::UnloadDriver(BOOL forceClearData)
{
	DWORD retCode = DRV_SUCCESS;

	//if the driver is started, first i will stop it
	if (started)
	{
		if ((retCode = StopDriver()) == DRV_SUCCESS) 
		{
			// I only remove it, if it is mark to be removable
			if (removable)
			{
				// open service and delete it
				SC_HANDLE SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
				
				if (NULL == SCManager) 
                {
                    return DRV_ERROR_SCM;
                }
					
				SC_HANDLE SCService = OpenService(SCManager, driverName, SERVICE_ALL_ACCESS);
				
				if (NULL != SCService)
				{
					if (! DeleteService(SCService))
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

				// if all ok, update the state
				if (retCode == DRV_SUCCESS)
                {
                    loaded = FALSE;
                }
					
			}
		}
	}

	// if the driver is initialized...
	if (initialized) 
	{
		//if there was some problem but i mark foreceClear, i will remove the data
		if (DRV_SUCCESS != retCode && FALSE == forceClearData)
        {
            return retCode;
        }
					
		// update the state
		initialized = FALSE;
				
		//free memory
		if (NULL != driverPath)
		{
			free(driverPath);
			driverPath = NULL;
		}

		if (NULL != driverDosName)
		{
			free(driverDosName);
			driverDosName = NULL;
		}

		if (NULL != driverName)
		{
			free(driverName);
			driverName = NULL;
		}

	}

	return retCode;
}

//
// Function to start the driver "service"
//
DWORD CDriver::StartDriver(void)
{
	//if already started, all ok
	if (started)
    {
        return DRV_SUCCESS;
    }
		
	//open the service manager and the service and change driver state
	SC_HANDLE SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	DWORD retCode;
	
	if (NULL == SCManager) 
    {
        return DRV_ERROR_SCM;
    }
		
    SC_HANDLE SCService = OpenService(SCManager,
		                              driverName,
				                      SERVICE_ALL_ACCESS);
    
	if (NULL == SCService) 
    {
        return DRV_ERROR_SERVICE;
    }

    if (! StartService(SCService, 0, NULL)) 
	{
		//if the driver was started before i try to do it,
		//i will not remove, because it was created by other application
        if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) 
		{
			removable = FALSE;
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
	if (DRV_SUCCESS == retCode)
	{
		started = TRUE;
		retCode = OpenDevice();
	}

    return retCode;
}

//
// Function to stop driver "service"
//
DWORD CDriver::StopDriver(void)
{
	//if already stopped, all ok
	if (! started)
    {
        return DRV_SUCCESS;
    }
		
	//open the service manager and the service and change driver state
	SC_HANDLE SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	DWORD retCode;
	
	if (NULL == SCManager)
    {
		return DRV_ERROR_SCM;
    }
   
    SERVICE_STATUS  status;

    SC_HANDLE SCService = OpenService(SCManager, driverName, SERVICE_ALL_ACCESS);
    
	if (NULL != SCService)
	{
		//close the driver handle too
		CloseHandle(driverHandle); 
		driverHandle = NULL; 

		if (! ControlService(SCService, SERVICE_CONTROL_STOP, &status))
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
	if(DRV_SUCCESS == retCode)
    {
        started = FALSE;
    }
	
    return retCode;
}

//
// Funtion to open a driver handle
//
DWORD CDriver::OpenDevice(void)
{
	//if i already have a handle, first close it
	if (NULL != driverHandle) 
    {
        CloseHandle(driverHandle);
    }
		
    driverHandle = CreateFile(driverDosName,
							  GENERIC_READ | GENERIC_WRITE,
							  0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                             );


    if (INVALID_HANDLE_VALUE == driverHandle)
    {
        return DRV_ERROR_INVALID_HANDLE;
    }
		
	return DRV_SUCCESS;
}

//
// Return the driverHandle obtained
//
HANDLE CDriver::GetDriverHandle(void)
{
	return driverHandle;
}

//
// Funtion to send data to the driver
//
DWORD CDriver::WriteIo(DWORD code, PVOID buffer, DWORD count)
{
	if (NULL == driverHandle)
    {
        return DRV_ERROR_INVALID_HANDLE;
    }

	DWORD bytesReturned;

	BOOL retCode = DeviceIoControl(driverHandle,
								   code,
								   buffer,
								   count,
								   NULL,
								   0,
								   &bytesReturned,
								   NULL
                                  );

	if (! retCode)
    {
        return DRV_ERROR_IO;
    }
		
	return DRV_SUCCESS;
}

//
//Functions to read data from the driver
//
DWORD CDriver::ReadIo(DWORD code, PVOID buffer, DWORD count)
{
    if (NULL == driverHandle)
    {
        return DRV_ERROR_INVALID_HANDLE;
    }

	DWORD bytesReturned;
	BOOL retCode = DeviceIoControl(driverHandle,
								   code,
								   NULL,
								   0,
								   buffer,
								   count,
								   &bytesReturned,
								   NULL
                                  );

    if (! retCode)
    {
        return DRV_ERROR_IO;
    }

	return bytesReturned;
}

//
// Function to do IO operation with the driver, read or write or both
//
DWORD CDriver::RawIo(DWORD code, PVOID inBuffer, DWORD inCount, PVOID outBuffer, DWORD outCount)
{
    if (NULL == driverHandle)
    {
        return DRV_ERROR_INVALID_HANDLE;
    }

	DWORD bytesReturned;
	BOOL retCode = DeviceIoControl(driverHandle,
								   code,
								   inBuffer,
								   inCount,
								   outBuffer,
								   outCount,
								   &bytesReturned,
								   NULL
                                  );

    if (! retCode)
    {
        return DRV_ERROR_IO;
    }

	return bytesReturned;
}

/************************************************************************/
/*   N T   D E V I C E   D R I V E R   R O U T I N E                    */
/************************************************************************/


/****************************************************************************
*
*    FUNCTION: UnloadDeviceDriver( const TCHAR *)
*
*    PURPOSE: Stops the driver and has the configuration manager unload it.
*
****************************************************************************/
BOOL CDriver::UnloadDeviceDriver(LPCTSTR lpszDriverName, LPTSTR lpszError)
{
    //
    // Open service control manager
    //
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (NULL == hSCManager)
    {
        _stprintf(lpszError, _T("%s"), _T("Could not open Service Control Manager."));
        return FALSE;
    }
    
    //
    // Driver is there
    //
    SC_HANDLE hDriver = OpenService(hSCManager, lpszDriverName, SERVICE_ALL_ACCESS);
    if (NULL == hDriver)
    {
        _stprintf(lpszError, _T("%s"), _T("Could not open driver service"));
        CloseServiceHandle(hSCManager);
        return FALSE;
    }
    
    SERVICE_STATUS ss;
    if (ControlService(hDriver, SERVICE_CONTROL_INTERROGATE, &ss))
    {
        _stprintf(lpszError, _T("%s"), _T("Could not interrogate driver service"));                
        CloseServiceHandle(hSCManager);
        CloseServiceHandle(hDriver);
        return FALSE;
    }
    
    if (SERVICE_STOPPED == ss.dwCurrentState)
    {
        BOOL bRet = DeleteService(hDriver);
        if (! bRet)
        {
            _stprintf(lpszError, _T("%s"), _T("Could not delete driver service")); 
        }
        CloseServiceHandle(hSCManager);
        CloseServiceHandle(hDriver);
        return bRet;
    }
    
    if (! ControlService(hDriver, SERVICE_CONTROL_STOP, &ss))
    {
        _stprintf(lpszError, _T("%s"), _T("Could not stop driver"));
        CloseServiceHandle(hSCManager);
        CloseServiceHandle(hDriver);
        return FALSE;
    }
    
    //
    // Give it 10 seconds to stop
    //
    BOOL Stopped = FALSE;
    for (int seconds = 0; seconds < 10; seconds++)
    {
        Sleep(1000);
        if (ControlService(hDriver, SERVICE_CONTROL_INTERROGATE, &ss) 
            && SERVICE_STOPPED == ss.dwCurrentState)
        {
            Stopped = TRUE;
            break;
        }
    }
    
    if (! Stopped)
    {
        _stprintf(lpszError, _T("%s"), _T("Could not stop driver"));
        CloseServiceHandle(hSCManager);
        CloseServiceHandle(hDriver);
        return FALSE;
    }
                   
    BOOL bRet = DeleteService(hDriver);
    if (! bRet)
    {
        _stprintf(lpszError, _T("%s"), _T("Could not delete driver service")); 
    }
    
    CloseServiceHandle(hSCManager);
    CloseServiceHandle(hDriver);
    
    return bRet;    
}

/****************************************************************************
*
*    FUNCTION: LoadDeviceDriver( const TCHAR, const TCHAR, HANDLE *)
*
*    PURPOSE: Registers a driver with the system configuration manager 
*	 and then loads it.
*
****************************************************************************/
BOOL CDriver::LoadDeviceDriver(LPCTSTR  lpszDriverName, 
                                   LPCTSTR  lpszDriverFromPath, 
                                   LPTSTR   lpszError
                                  )
{    
    assert(lpszDriverName && lpszDriverFromPath);
    assert(lpszError);
    
    //
	// Get System32 directory
    //
    _asm int 3;
	_TCHAR System32Directory[_MAX_PATH];
	if (0 == GetSystemDirectory(System32Directory, _MAX_PATH)) // C:\WINNT\System32
	{
		_stprintf(lpszError, _T("%s"), _T("Could not find Windows system directory."));
		return FALSE;
	}

    //
	// Copy driver .sys file across
    //
    _TCHAR szDriverFullPath[_MAX_PATH];
    _stprintf(szDriverFullPath, _T("%s\\drivers\\%s.sys"), System32Directory, lpszDriverName);	
	if (FALSE == CopyFile(lpszDriverFromPath, szDriverFullPath, FALSE)) // Overwrite OK
	{
        _stprintf(lpszError, _T("Unable to copy %s to %s\n\nMake sure that logprint.sys is in the current directory."), lpszDriverName, szDriverFullPath);		
		return FALSE;
	}

	//
	// Create driver (or stop existing driver)
    //
	if (! CreateDriver(lpszDriverName, szDriverFullPath, lpszError))
    {
        return FALSE;
    }
		

	//
	// Create/Open driver registry key and set its values
	//	Overwrite registry values written in driver creation
    //
	HKEY hKey;
	DWORD disposition;
    TCHAR szDriverpath[_MAX_PATH];
    _stprintf(szDriverpath, _T("SYSTEM\\CurrentControlSet\\Services\\%s"), lpszDriverName);
	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
                                       szDriverpath,
					                   0, 
                                       _T(""), 
                                       0, 
                                       KEY_ALL_ACCESS, 
                                       NULL, 
                                       &hKey, 
                                       &disposition
                                      ))
	{
        _stprintf(lpszError, _T("%s"), _T("Could not create driver registry key."));
		return FALSE;
	}
    /*
	// Delete ImagePath    
	RegDeleteValue(hKey, _T("ImagePath"));

	// Delete DisplayName
	RegDeleteValue(hKey, _T("DisplayName"));
    
	// ErrorControl
	DWORD dwRegValue = SERVICE_ERROR_NORMAL;
	if (ERROR_SUCCESS != RegSetValueEx(hKey, _T("ErrorControl"), 0, REG_DWORD, (CONST BYTE*)&dwRegValue, sizeof(DWORD)))
	{
        _stprintf(lpszError, _T("%s"), _T("Could not create driver registry value ErrorControl."));
		return FALSE;
	}
    
	// Start
	dwRegValue = SERVICE_DEMAND_START;//SERVICE_AUTO_START;
	if (ERROR_SUCCESS != RegSetValueEx(hKey, _T("Start"), 0, REG_DWORD, (CONST BYTE*)&dwRegValue, sizeof(DWORD)))
	{
        _stprintf(lpszError, _T("%s"), _T("Could not create driver registry value Start."));
		return FALSE;
	}
    
	// Type
	dwRegValue = SERVICE_KERNEL_DRIVER;
	if (ERROR_SUCCESS != RegSetValueEx(hKey, _T("Type"), 0, REG_DWORD, (CONST BYTE*)&dwRegValue, sizeof(DWORD)))
	{
        _stprintf(lpszError, _T("%s"), _T("Could not create driver registry value Type."));
		return FALSE;
	}
    
	RegCloseKey(hKey);

	//
	// Create/Open driver\Parameters registry key and set its values
    //
    TCHAR szParapath[_MAX_PATH];
    _stprintf(szParapath, _T("SYSTEM\\CurrentControlSet\\Services\\%s\\Parameters"), lpszDriverName);
	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
                                        szParapath,
						                0, 
                                        _T(""), 
                                        0, 
                                        KEY_ALL_ACCESS, 
                                        NULL, 
                                        &hKey, 
                                        &disposition
                                       ))
	{
        _stprintf(lpszError, _T("%s"), _T("Could not create driver\\Parameters registry key."));
		return FALSE;
	}
    
	// EventLogLevel
	dwRegValue = 1;
	if (ERROR_SUCCESS != RegSetValueEx(hKey, _T("EventLogLevel"), 0, REG_DWORD, (CONST BYTE*)&dwRegValue, sizeof(DWORD)))
	{
        _stprintf(lpszError, _T("%s"), _T("Could not create driver\\Parameters registry value EventLogLevel."));
		return FALSE;
	}

	// Default or No Name
	int DeviceNameLen = _tcslen(lpszDriverName) + 1;
	
	if (ERROR_SUCCESS != RegSetValueEx(hKey, _T(""), 0, REG_SZ, (CONST BYTE*)lpszDriverName, DeviceNameLen))
	{
        _stprintf(lpszError, _T("%s"), _T("Could not create driver\\Parameters default registry value."));
		return FALSE;
	}
	RegCloseKey(hKey);

	//
	// Open EventLog\System registry key and set its values
    //
	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
                                        _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\System"),
						                0, 
                                        _T(""), 
                                        0, 
                                        KEY_ALL_ACCESS, 
                                        NULL, 
                                        &hKey, 
                                        &disposition
                                       ))
	{
        _stprintf(lpszError, _T("%s"), _T("Could not open EventLog\\System registry key."));
		return FALSE;
	}
    
	// get Sources size
	DWORD DataSize = 0;
	DWORD Type;
	if (ERROR_SUCCESS != RegQueryValueEx(hKey, _T("Sources"), NULL, &Type, NULL, &DataSize))
	{
        _stprintf(lpszError, _T("%s"), _T("Could not read size of EventLog\\System registry value Sources."));
		return FALSE;
	}
    
	// read Sources
	int DriverNameLen = _tcslen(lpszDriverName);
	DataSize += DriverNameLen + 1;
	LPTSTR Sources = new _TCHAR[DataSize];
    
	if (NULL == Sources || ERROR_SUCCESS != RegQueryValueEx(hKey, _T("Sources"), NULL, &Type, (LPBYTE)Sources, &DataSize))
	{
        _stprintf(lpszError, _T("%s"), _T("Could not read EventLog\\System registry value Sources."));
        if (NULL != Sources)
        {
            delete []Sources;
            Sources = NULL;
        }
		return FALSE;
	}
    delete []Sources;
    
    //
	// If driver not there, add and write
    //
	if (-1 == FindInMultiSz(Sources, DataSize, lpszDriverName))
	{
		_tcscpy(Sources + DataSize - 1, lpszDriverName);
		DataSize += DriverNameLen;
		*(Sources + DataSize) = '\0';

		if (ERROR_SUCCESS != RegSetValueEx(hKey, _T("Sources"), 0, REG_MULTI_SZ, (CONST BYTE*)Sources, DataSize))
		{
            _stprintf(lpszError, _T("%s"), _T("Could not create driver registry value Sources."));
			return FALSE;
		}
	}

    RegCloseKey(hKey);

	//
	// Create/Open EventLog\System\driver registry key and set its values
    //
    _stprintf(szParapath, _T("SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\%s"), lpszDriverName);
	if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                        szParapath,
						                0, 
                                        _T(""), 
                                        0, 
                                        KEY_ALL_ACCESS, 
                                        NULL, 
                                        &hKey, 
                                        &disposition
                                       ))
	{
        _stprintf(lpszError, _T("%s"), _T("Could not create EventLog\\System\\driver registry key."));
		return FALSE;
	}
    
	// TypesSupported
	dwRegValue = 7;
	if (ERROR_SUCCESS != RegSetValueEx(hKey, _T("TypesSupported"), 0, REG_DWORD, (CONST BYTE*)&dwRegValue, sizeof(DWORD)))
	{
        _stprintf(lpszError, _T("%s"), _T("Could not create EventLog\\System\\driver registry value TypesSupported."));
		return FALSE;
	}
    
	// EventMessageFile
    TCHAR szEventMessageFile[_MAX_PATH];
    _stprintf(szEventMessageFile, _T(%s%s.sys"), _T("%SystemRoot%\\System32\\IoLogMsg.dll;%SystemRoot%\\System32\\Drivers\\"), lpszDriverName);	
	if (ERROR_SUCCESS != RegSetValueEx(hKey, _T("EventMessageFile"), 0, REG_EXPAND_SZ, (CONST BYTE*)szEventMessageFile, _tcslen(szEventMessageFile) + 1))
	{
        _stprintf(lpszError, _T("%s"), _T("Could not create EventLog\\System\\driver registry value EventMessageFile."));
		return FALSE;
	}
	RegCloseKey(hKey);
*/
	/////////////////////////////////////////////////////////////////////////
	// Start driver service

	if (! StartDriver(lpszDriverName, lpszError))
    {
        return FALSE;
    }
		
    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////

BOOL CDriver::CreateDriver(LPCTSTR lpszDriverName, LPCTSTR lpszFullDriver, LPTSTR lpszError)
{
    //
	// Open service control manager
    //
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hSCManager)
	{
		_stprintf(lpszError, _T("%s"), _T("Could not open Service Control Manager"));
		return FALSE;
	}

    //
	// If driver is running, stop it
    //
	SC_HANDLE hDriver = OpenService(hSCManager, lpszDriverName, SERVICE_ALL_ACCESS);
	if (NULL != hDriver)
	{
        SERVICE_STATUS ss;
		if (ControlService(hDriver, SERVICE_CONTROL_INTERROGATE, &ss))
		{
			if (SERVICE_STOPPED != ss.dwCurrentState)
			{
				if (! ControlService(hDriver, SERVICE_CONTROL_STOP, &ss))
				{
                    _stprintf(lpszError, _T("%s"), _T("Could not stop driver"));
					CloseServiceHandle(hSCManager);
					CloseServiceHandle(hDriver);
					return FALSE;
				}
                
                //
				// Give it 10 seconds to stop
                //
				BOOL Stopped = FALSE;
				for (int seconds = 0; seconds < 10; seconds++)
				{
					Sleep(1000);
					if (ControlService(hDriver, SERVICE_CONTROL_INTERROGATE, &ss) 
                        && SERVICE_STOPPED == ss.dwCurrentState)
					{
						Stopped = TRUE;
						break;
					}
				}

				if (! Stopped)
				{
					_stprintf(lpszError, _T("%s"), _T("Could not stop driver"));
					CloseServiceHandle(hSCManager);
					CloseServiceHandle(hDriver);
					return FALSE;
				}
			}
            
			CloseServiceHandle(hDriver);
		}

		return TRUE;
	}

    //
	// Create driver service
    //
    hDriver = CreateService(hSCManager,            // SCManager database
                            lpszDriverName,        // name of service
                            lpszDriverName,        // name to display
                            SERVICE_ALL_ACCESS,    // desired access
                            SERVICE_KERNEL_DRIVER, // service type
                            SERVICE_DEMAND_START,  // start type
                            SERVICE_ERROR_NORMAL,  // error control type
                            lpszFullDriver,        // service's binary
                            NULL,                  // no load ordering group
                            NULL,                  // no tag identifier
                            NULL,                  // no dependencies
                            NULL,                  // LocalSystem account
                            NULL                   // no password
                           );
/*
ERROR_ACCESS_DENIED         5L
ERROR_CIRCULAR_DEPENDENCY   1059L
ERROR_DUP_NAME              52L
ERROR_INVALID_HANDLE        6L
ERROR_INVALID_NAME          123L
ERROR_INVALID_PARAMETER     87L
ERROR_INVALID_SERVICE_ACCOUNT 1057L
ERROR_SERVICE_EXISTS        1073L
*/
	if (NULL == hDriver)
	{
        DWORD error = GetLastError();
        _stprintf(lpszError, _T("%s"), _T("Could not install driver with Service Control Manager"));
		CloseServiceHandle(hSCManager);
		return FALSE;
	}

	CloseServiceHandle(hSCManager);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CDriver::StartDriver(LPCTSTR lpszDriverName, LPTSTR lpszError)
{
	//
	// Open service control manager
    //
	SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hSCManager)
	{
        _stprintf(lpszError, _T("%s"), _T("Could not open Service Control Manager."));
        return FALSE;
	}

	//
	// Driver isn't there
    //
	SC_HANDLE hDriver = OpenService(hSCManager, lpszDriverName, SERVICE_ALL_ACCESS);
	if (NULL == hDriver)
	{
		_stprintf(lpszError, _T("%s"), _T("Could not open driver service"));
		CloseServiceHandle(hSCManager);
		return FALSE;
	}
    /*
ERROR_ACCESS_DENIED 5
ERROR_DEPENDENT_SERVICES_RUNNING 1051
ERROR_INVALID_HANDLE 6
ERROR_INVALID_PARAMETER 87
ERROR_INVALID_SERVICE_CONTROL 1052
ERROR_SERVICE_CANNOT_ACCEPT_CTRL 1061
ERROR_SERVICE_NOT_ACTIVE 1062
ERROR_SERVICE_REQUEST_TIMEOUT 1053
ERROR_SHUTDOWN_IN_PROGRESS 1115L
*/
	SERVICE_STATUS ss;
	if (! ControlService(hDriver, SERVICE_CONTROL_INTERROGATE, &ss))
	{
        DWORD error = GetLastError();
		_stprintf(lpszError, _T("%s"), _T("Could not interrogate driver service"));
		CloseServiceHandle(hSCManager);
		CloseServiceHandle(hDriver);
		return FALSE;
	}

    if (SERVICE_STOPPED != ss.dwCurrentState)
    {
        _stprintf(lpszError, _T("%s"), _T("Driver service already running."));
        CloseServiceHandle(hSCManager);
        CloseServiceHandle(hDriver);
        return FALSE;
    }


/*ERROR_ACCESS_DENIED 5L
ERROR_INVALID_HANDLE 6L
ERROR_PATH_NOT_FOUND 3L
ERROR_SERVICE_ALREADY_RUNNING 1056L
ERROR_SERVICE_DATABASE_LOCKED 1055L
ERROR_SERVICE_DEPENDENCY_DELETED 1075L
ERROR_SERVICE_DEPENDENCY_FAIL 1068L
ERROR_SERVICE_DISABLED 1058L
ERROR_SERVICE_LOGON_FAILED 1069L
ERROR_SERVICE_MARKED_FOR_DELETE 1072L
ERROR_SERVICE_NO_THREAD 1054L
ERROR_SERVICE_REQUEST_TIMEOUT 1053L
*/
	if (! StartService(hDriver, 0, NULL))
	{
        DWORD error = GetLastError();
		_stprintf(lpszError, _T("%s"), _T("Could not start driver"));
		CloseServiceHandle(hSCManager);
		CloseServiceHandle(hDriver);
		return FALSE;
	}

    //
	// Give it 10 seconds to start
    //
	BOOL Started = FALSE;
	for (int seconds = 0; seconds < 10; seconds++)
	{
		Sleep(1000);
		if (ControlService(hDriver, SERVICE_CONTROL_INTERROGATE, &ss) 
            && SERVICE_RUNNING == ss.dwCurrentState)
		{
			Started = TRUE;
			break;
		}
	}

	if (! Started)
	{
		_stprintf(lpszError, _T("%s"), _T("Could not start driver"));
		CloseServiceHandle(hSCManager);
		CloseServiceHandle(hDriver);
		return FALSE;
	}
    
	CloseServiceHandle(hDriver);
	CloseServiceHandle(hSCManager);
	return TRUE;
}

/****************************************************************************
*
*    FUNCTION: OpenDevice( IN LPCTSTR, HANDLE *)
*
*    PURPOSE: Opens the device and returns a handle if desired.
*
****************************************************************************/
BOOL CDriver::OpenDevice(DWORD dwInst, HANDLE * lphDevice, LPTSTR lpszError, IN GUID * pGuid)
{
    BOOL        bRet = FALSE;
    HDEVINFO    info = INVALID_HANDLE_VALUE;

    do
    {    
        // Get handle to relevant device information set
        if (NULL == pGuid)
        {
            pGuid = (LPGUID)&LOGPRINT_GUID;
        }
        assert(NULL != pGuid);
        info = SetupDiGetClassDevs(pGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
        if (INVALID_HANDLE_VALUE == info)
        {
            _stprintf(lpszError, _T("%s"), _T("Could not get device info set."));
            break;
        }
    
        // Get interface data for the requested instance
        SP_INTERFACE_DEVICE_DATA ifdata;
        ifdata.cbSize = sizeof(ifdata);
        if (! SetupDiEnumDeviceInterfaces(info, NULL, pGuid, dwInst, &ifdata))
        {
            _stprintf(lpszError, _T("%s"), _T("Could not get interface data for the requested instance."));
            break;
        }
    
        // Get size of symbolic link name
        DWORD ReqLen;
        SetupDiGetDeviceInterfaceDetail(info, &ifdata, NULL, 0, &ReqLen, NULL);
        PSP_INTERFACE_DEVICE_DETAIL_DATA ifDetail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)(new char[ReqLen]);
        if (NULL == ifDetail)
        {
            _stprintf(lpszError, _T("%s"), _T("Could not get size of symbolic link name."));
            break;
        }
    
        // Get symbolic link name
        ifDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
        if( !SetupDiGetDeviceInterfaceDetail(info, &ifdata, ifDetail, ReqLen, NULL, NULL))
        {
            delete ifDetail;
            _stprintf(lpszError, _T("%s"), _T("Could not get symbolic link name."));
            break;
        }
    
        // Open file
        HANDLE hDevice = CreateFile(ifDetail->DevicePath, 
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL, 
                                    OPEN_EXISTING, 
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 
                                    NULL
                                   );
        if (INVALID_HANDLE_VALUE == hDevice)             
        {
            _stprintf(lpszError, _T("%s"), _T("Could not open LogPrint device."));            
        }
    
        // Tidy up
        delete ifDetail;
            
        // If user wants handle, give it to them.  Otherwise, just close it.
        *lphDevice = hDevice;
        bRet = TRUE;
        
    } while (FALSE);
    
    if (INVALID_HANDLE_VALUE != info)
    {
        SetupDiDestroyDeviceInfoList(info);
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
//	Try to find Match in MultiSz, including Match's terminating \0

int CDriver::FindInMultiSz(LPTSTR MultiSz, int MultiSzLen, LPCTSTR Match)
{
	int MatchLen = _tcslen(Match);
	_TCHAR FirstChar = *Match;

	for (int i = 0; i < MultiSzLen - MatchLen; i++)
	{
		if (*MultiSz++ == FirstChar)
		{
			BOOL Found = TRUE;
			LPTSTR Try = MultiSz;
			for (int j = 1; j <= MatchLen; j++)
            {            
				if (*Try++ != Match[j])
				{
					Found = FALSE;
					break;
				}
            }

			if (Found)
            {
                return i;
            }				
		}
	}
    
	return -1;
}

/*
extern CDriver * pDriver;

BOOL CMainFrame::TestLoadDriver()
{
#define	    SYS_FILE    _T("logprint.sys")
#define     SYS_NAME    _T("LogPrint")

    static TCHAR	szLogFile[_MAX_PATH];
    
    // Driver's registry key
    TCHAR	DriverRegistryKey[] = _T("System\\CurrentControlSet\\Services\\LogPrint");

    CWaitCursor wait;
    
    TCHAR  szDriverPath[_MAX_PATH];
	GetCurrentDirectory(sizeof(szDriverPath), szDriverPath);
    _stprintf(szDriverPath + _tcslen(szDriverPath), _T("\\%s"), SYS_FILE);
                
    WIN32_FIND_DATA  findData;
    HANDLE findHandle = FindFirstFile(szDriverPath, &findData);
    if (INVALID_HANDLE_VALUE == findHandle) 
    {                    
        TCHAR   * pFile;

        if (! SearchPath(NULL, SYS_FILE, NULL, sizeof(szDriverPath), szDriverPath, &pFile))
        {                        
            CString strTmp;
            strTmp.Format(_T("%s was not found."), SYS_FILE);
            AfxMessageBox(strTmp);
            return;
        }                    
    } 
    else 
    {
        FindClose(findHandle);
    }

    // read driver start type to see if boot-logging is enabled
    HKEY  hDriverKey;
    DWORD type, driverStart = SERVICE_DEMAND_START;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, DriverRegistryKey, &hDriverKey)) 
    {                    
        DWORD length = sizeof(driverStart);
        RegQueryValueEx(hDriverKey, _T("Start"), NULL, &type, (PBYTE)&driverStart, &length);
        RegCloseKey(hDriverKey);
    } 
    BOOL bBootLog = (driverStart != SERVICE_DEMAND_START);

    //
    // Load LogPrint Driver
    //
    TCHAR lpszError[MAX_PATH];
    RtlZeroMemory(lpszError, MAX_PATH);
    if (! (pDriver->LoadDeviceDriver(SYS_NAME, szDriverPath, lpszError))  
    {
        AfxMessageBox((LPCTSTR)lpszError);
        return;
    }

    //
    // Open LogPrint Device
    //
    TCHAR lpszError[MAX_PATH];
    RtlZeroMemory(lpszError, MAX_PATH);
    if (! (pDriver->OpenDevice(0, &m_hLogPrint, lpszError))  
    {
        AfxMessageBox((LPCTSTR)lpszError);
        return;
    }

    ASSERT(INVALID_HANDLE_VALUE != m_hLogPrint);
    
    //
    // Correct driver version?
    //
    DWORD nb, versionNumber;

    if (! DeviceIoControl(m_hLogPrint, IOCTL_LOGPRINT_GET_VERSION,
                          NULL, 0, &versionNumber, sizeof(DWORD), &nb, NULL) 
          || versionNumber != LOGPRINT_VERSION)
    {
        AfxMessageBox(_T("LogPrint located a driver with the wrong version.\n")
            _T("\nIf you just installed a new version you must reboot before you are")
            _T("able to use it."));
        return;
    }   
}
*/