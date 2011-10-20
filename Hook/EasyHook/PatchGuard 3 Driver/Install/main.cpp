/*
Released under MIT License

Copyright (c) 2008 by Christoph Husse, SecurityRevolutions e.K.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial 
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Visit http://www.codeplex.com/easyhook for more information.
*/

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include "DriverShared.h"

#define THROW(msg)		{ printf("\r\n" "ERROR: %s (Code: %d)\r\n", (msg), GetLastError()); return GetLastError(); }

BOOLEAN FileExists(wchar_t* InPath)
{
	HANDLE			hFile;

	if((hFile = CreateFileW(InPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
		return FALSE;

	CloseHandle(hFile);

	return TRUE;
}

/************************************************************************
*************************************** PgInstallDriver
*************************************************************************

Description:

	Installs the specified driver. Please note that since
	Windows Vista, all driver must be signed!

*/
DWORD PgInstallDriver(wchar_t* InDriverName)
{
	wchar_t				DriverPath[MAX_PATH + 1];
	SC_HANDLE			hSCManager = NULL;
	SC_HANDLE			hService = NULL;	
	HANDLE				hFile = INVALID_HANDLE_VALUE;

	__try
	{	
		GetFullPathNameW(InDriverName, MAX_PATH, DriverPath, NULL);

		if(!FileExists(DriverPath))
			THROW("Driver file does not exist.");

		CloseHandle(hFile);

		if((hSCManager = OpenSCManagerW(
				NULL, 
				NULL, 
				SC_MANAGER_ALL_ACCESS)) == NULL)
			THROW("Unable to open service control manager. Are you running as administrator?");

		// does service exist?
		if((hService = OpenService(
				hSCManager, 
				InDriverName, 
				SERVICE_ALL_ACCESS)) == NULL)
		{
			if(GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
				THROW("An unknown error has occurred during driver installation.");

			// Create the service
			if((hService = CreateServiceW(
					hSCManager,              
					InDriverName,            
					InDriverName,           
					SERVICE_ALL_ACCESS,        
					SERVICE_KERNEL_DRIVER,
					SERVICE_DEMAND_START,    
					SERVICE_ERROR_NORMAL,     
					DriverPath,            
					NULL, NULL, NULL, NULL, NULL)) == NULL)
				THROW("Unable to install driver.");
		}
		else
		{
			printf("WARNING: The driver has already been installed!");
		}	

		// start and connect service...
		if(!StartServiceW(hService, 0, NULL) && (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING))
			THROW("Unable to start driver. This may also indicate that PatchGuard could not be detected!");

		return ERROR_SUCCESS;
	}
	__finally
	{
		if(hService != NULL)
			CloseServiceHandle(hService);

		if(hSCManager != NULL)
			CloseServiceHandle(hSCManager);
	}
}

/************************************************************************
*************************************** PgDriverCommand
*************************************************************************

Description:

	The driver exports three commands:

	IOCTL_PATCHGUARD_DISABLE

		This command will disable PatchGuard. Multiple calls
		are ignored for PG2Disable.sys and will fail for
		PG3Disable.sys!
		
		No parameters, no return value.

	IOCTL_PATCHGUARD_DUMP

		Will dump the timer table in case of PG2Disable.sys
		and the fingerprints in case of PG3Disable.sys!

		No parameters, no return value.

	IOCTL_PATCHGUARD_PROBE

		Installs a test hook to probe whether PatchGuard is disabled.
		PG3Disable will ignore this command. Please note that this
		command may lead to DoS attachs. Remove it for any non-testing
		purposes...

		No parameters, no return value.

*/
DWORD PgDriverCommand(
		HANDLE hInDriver,
		DWORD InCommand)
{
	DWORD			BytesWritten;

	if(!DeviceIoControl(
			hInDriver,
			InCommand,
			NULL,
			0,
			NULL,
			0,
			&BytesWritten,
			NULL))
		THROW("Driver reported an error.");

	return ERROR_SUCCESS;
}

int main(int argc, wchar_t* argv[])
{
	__try
	{
		HANDLE							hDriver = INVALID_HANDLE_VALUE;

		if(PgInstallDriver(L"PG3Disable.sys") != ERROR_SUCCESS)
			return -1;

		printf("\r\n" "INFO: Operation in progress...\r\n");

		 if((hDriver = CreateFileW(
				EASYHOOK_WIN32_DEVICE_NAME,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				0,
				NULL)) == INVALID_HANDLE_VALUE)
			THROW("Unable to open driver interface");

		if(PgDriverCommand(hDriver, IOCTL_PATCHGUARD_DUMP) != ERROR_SUCCESS)
			return -1;

		if(PgDriverCommand(hDriver, IOCTL_PATCHGUARD_DISABLE) != ERROR_SUCCESS)
			return -1;

		printf("\r\n" "INFO: PatchGuard has been successfully disabled.");

		return 0;
	}
	__finally
	{
		printf("\r\n\r\n" "Press any key to exit.\r\n");
	
		_getch();
	}
}

