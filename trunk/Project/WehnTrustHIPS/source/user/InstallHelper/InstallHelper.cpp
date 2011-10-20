//#include "Precomp.h"
//#include "../../Common/LicenseValidator.h"
#include "windows.h"
#include <stdio.h>
#include <wchar.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include "msi.h"
#include "msiquery.h"


//
// Installs the device driver
//
__declspec(dllexport) ULONG _stdcall InstallDriver(MSIHANDLE Install)
{
	SC_HANDLE Scm = NULL, Service = NULL;
	BOOLEAN   Success = FALSE;
	TCHAR     InstallPath[1024], DriverPath[1024];
	ULONG     InstallPathSize = sizeof(InstallPath) / sizeof(TCHAR);
	ULONG     Error = 0;

	ZeroMemory(
			InstallPath,
			sizeof(InstallPath));
	
	ZeroMemory(
			DriverPath,
			sizeof(DriverPath));

	//
	// Get the install path string
	//
	Error = MsiGetProperty(
			Install,
			TEXT("CustomActionData"),
			InstallPath,
			&InstallPathSize);

	do
	{
		//
		// Open SCM
		//
		if (!(Scm = OpenSCManager(
				NULL,
				NULL,
				SC_MANAGER_ALL_ACCESS)))
		{
			Error = GetLastError();
			break;
		}

		//
		// Build out the driver path
		//
		ExpandEnvironmentStrings(
				TEXT("%WINDIR%\\System32\\Drivers\\baserand.sys"),
				DriverPath,
				sizeof(DriverPath) / sizeof(TCHAR) - sizeof(TCHAR));

		//
		// Create the service
		//
		if (!(Service = CreateService(
					Scm,
					TEXT("baserand"),
					TEXT("WehnTrust ASLR Driver"),
					SERVICE_ALL_ACCESS,
					SERVICE_KERNEL_DRIVER,
					SERVICE_BOOT_START,
					SERVICE_ERROR_NORMAL,
					DriverPath,
					NULL,
					NULL,
					NULL,
					NULL,
					NULL)))
		{
			Error = GetLastError();
			break;
		}

		Success = TRUE;

	} while (0);

	//
	// Close the service manager handles
	//
	if (Service)
		CloseServiceHandle(
				Service);
	if (Scm)
		CloseServiceHandle(
				Scm);

	//
	// Pop up a message letting the user know that the randomization driver
	// failed
	//
	if (!Success)
	{
		TCHAR ErrorMessage[256];

		ZeroMemory(
				ErrorMessage,
				sizeof(ErrorMessage));

		_sntprintf_s(
				ErrorMessage,
				sizeof(ErrorMessage) / sizeof(TCHAR),
				(sizeof(ErrorMessage) / sizeof(TCHAR)) - 1,
				"The address randomization driver failed to install (%s): %lu.",
				DriverPath, Error);

		MessageBox(
				NULL,
				ErrorMessage,
				TEXT("Driver Installation Failure"),
				MB_OK);
	}

	return ERROR_SUCCESS;
}

//
// Installs the device driver
//
__declspec(dllexport) ULONG _stdcall InstallService(MSIHANDLE Install)
{
	SC_HANDLE Scm = NULL, MonitorService = NULL;
	BOOLEAN   Success = FALSE;
	TCHAR     InstallPath[1024], DriverPath[1024];
	ULONG     InstallPathSize = sizeof(InstallPath) / sizeof(TCHAR);
	ULONG     Error = 0;

	ZeroMemory(
			InstallPath,
			sizeof(InstallPath));
	
	ZeroMemory(
			DriverPath,
			sizeof(DriverPath));

	//
	// Get the install path string
	//
	Error = MsiGetProperty(
			Install,
			TEXT("CustomActionData"),
			InstallPath,
			&InstallPathSize);

	do
	{
		//
		// Open SCM
		//
		if (!(Scm = OpenSCManager(
				NULL,
				NULL,
				SC_MANAGER_ALL_ACCESS)))
		{
			Error = GetLastError();
			break;
		}

		//
		// Create the monitor service
		//
		_sntprintf_s(
				DriverPath,
				sizeof(DriverPath) / sizeof(TCHAR),
				(sizeof(DriverPath) / sizeof(TCHAR)) - 1,
				"%s\\WehnServ.exe",
				InstallPath);

		DriverPath[(sizeof(DriverPath) / sizeof(TCHAR)) - 1] = 0;

		if (!(MonitorService = CreateService(
				Scm,
				TEXT("WehnServ"),
				TEXT("WehnTrust Monitor Service"),
				SERVICE_ALL_ACCESS,
				SERVICE_WIN32_OWN_PROCESS,
				SERVICE_AUTO_START,
				SERVICE_ERROR_NORMAL,
				DriverPath,
				NULL,
				NULL,
				NULL,
				TEXT("LocalSystem"),
				NULL)))
		{
			Error = GetLastError();
			break;
		}

		Success = TRUE;

	} while (0);

	//
	// Close the service manager handles
	//
	if (MonitorService)
		CloseServiceHandle(
				MonitorService);
	if (Scm)
		CloseServiceHandle(
				Scm);

	//
	// Pop up a message letting the user know that the randomization driver
	// failed
	//
	if (!Success)
	{
		TCHAR ErrorMessage[256];

		ZeroMemory(
				ErrorMessage,
				sizeof(ErrorMessage));

		_sntprintf_s(
				ErrorMessage,
				sizeof(ErrorMessage) / sizeof(TCHAR),
				(sizeof(ErrorMessage) / sizeof(TCHAR)) - 1,
				"The address randomization service failed to install (%s): %lu.",
				DriverPath, Error);

		MessageBox(
				NULL,
				ErrorMessage,
				TEXT("Service Installation Failure"),
				MB_OK);
	}

	return ERROR_SUCCESS;
}

//
// Uninstalls the device driver
//
_declspec(dllexport) ULONG _stdcall UninstallDriver(MSIHANDLE Install)
{
	SERVICE_STATUS ss;
	SC_HANDLE      Scm, Service = NULL;
	BOOLEAN        Success = FALSE;

	do
	{
		//
		// Open SCM
		//
		if (!(Scm = OpenSCManager(
				NULL,
				NULL,
				SC_MANAGER_ALL_ACCESS)))
			break;

		//
		// Open the driver service
		//
		if (!(Service = OpenService(
				Scm,
				TEXT("baserand"),
				SERVICE_ALL_ACCESS)))
			break;

		
		//
		// Stop the service even though it cannot be stopped
		//
		ControlService(
				Service,
				SERVICE_CONTROL_STOP,
				&ss);
			
		//
		// Delete the service
		//
		if (!DeleteService(
				Service))
			break;


		Success = TRUE;

	} while (0);


	//
	// Pop up a message letting the user know that the randomization driver
	// uninstall failed
	//
   if (!Success)
	{
		TCHAR ErrorMessage[256];

		ZeroMemory(
				ErrorMessage,
				sizeof(ErrorMessage));

		_sntprintf_s(
				ErrorMessage,
				sizeof(ErrorMessage) / sizeof(TCHAR),
				(sizeof(ErrorMessage) / sizeof(TCHAR)) - 1,
				"The address randomization driver failed to uninstall: %lu.",
				GetLastError());

		MessageBox(
				NULL,
				ErrorMessage,
				TEXT("Driver Uninstallation Failure"),
				MB_OK);
	}

	//
	// Close the service manager handles
	//
	if (Service)
		CloseServiceHandle(
				Service);
	if (Scm)
		CloseServiceHandle(
				Scm);

	return ERROR_SUCCESS;
}

//
// Uninstalls the device service
//
_declspec(dllexport) ULONG _stdcall UninstallService(MSIHANDLE Install)
{
	SERVICE_STATUS ss;
	SC_HANDLE      Scm, MonitorService = NULL;
	BOOLEAN        Success = FALSE;

	do
	{
		//
		// Open SCM
		//
		if (!(Scm = OpenSCManager(
				NULL,
				NULL,
				SC_MANAGER_ALL_ACCESS)))
			break;

		//
		// Open and delete the monitor service.
		//
		if (!(MonitorService = OpenService(
				Scm,
				TEXT("WehnServ"),
				SERVICE_ALL_ACCESS)))
			break;

		ControlService(
				MonitorService,
				SERVICE_CONTROL_STOP,
				&ss);

		if (!DeleteService(
				MonitorService))
			break;

		Success = TRUE;

	} while (0);


	//
	// Pop up a message letting the user know that the randomization driver
	// uninstall failed
	//
   if (!Success)
	{
		TCHAR ErrorMessage[256];

		ZeroMemory(
				ErrorMessage,
				sizeof(ErrorMessage));

		_sntprintf_s(
				ErrorMessage,
				sizeof(ErrorMessage) / sizeof(TCHAR),
				(sizeof(ErrorMessage) / sizeof(TCHAR)) - 1,
				"The address randomization service failed to uninstall: %lu.",
				GetLastError());

		MessageBox(
				NULL,
				ErrorMessage,
				TEXT("Service Uninstallation Failure"),
				MB_OK);
	}

	//
	// Close the service manager handles
	//
	if (MonitorService)
		CloseServiceHandle(
				MonitorService);
	if (Scm)
		CloseServiceHandle(
				Scm);

	return ERROR_SUCCESS;
}

//
// Terminates WehnTrust
//
_declspec(dllexport) ULONG _stdcall TerminateWehnTrust(MSIHANDLE Install)
{
	HMODULE Psapi = NULL;
	DWORD   Pids[1024], PidsSize;
	BOOL    (WINAPI *EnumProcesses)(LPDWORD, DWORD, LPDWORD) = NULL;
	BOOL    (WINAPI *EnumProcessModules)(HANDLE, HMODULE *, DWORD, LPDWORD) = NULL;
	DWORD   (WINAPI *GetModuleBaseName)(HANDLE, HMODULE, LPTSTR, DWORD) = NULL;

	HWND Tray = FindWindow(
			TEXT("WehnTrustTray"),
			NULL);

	//
	// Send EXIT
	//
	if (Tray)
		SendMessage(Tray, WM_COMMAND, 1000, 0);

	//
	// Try to find the process in the task list
	//
	Psapi = LoadLibrary("psapi");

	EnumProcesses      = (BOOL (WINAPI *)(LPDWORD, DWORD, LPDWORD))GetProcAddress(Psapi, "EnumProcesses");
	EnumProcessModules = (BOOL (WINAPI *)(HANDLE, HMODULE *, DWORD, LPDWORD))GetProcAddress(Psapi, "EnumProcessModules");
	GetModuleBaseName  = (DWORD (WINAPI *)(HANDLE, HMODULE, LPTSTR, DWORD))GetProcAddress(Psapi, "GetModuleBaseName");

	if ((EnumProcesses) &&
	    (EnumProcesses(Pids, sizeof(Pids), &PidsSize)))
	{
		HANDLE ProcessHandle;
		ULONG  Index;
		CHAR   ProcessName[MAX_PATH] = "unknown";

		for (Index = 0; 
		     Index < PidsSize / sizeof(DWORD); 
		     Index++)
		{
			HMODULE Module;
			DWORD   Needed;

			if ((ProcessHandle = OpenProcess(
					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE,
					FALSE,
					Pids[Index])) &&
			    (EnumProcessModules) &&
			    (EnumProcessModules(
					ProcessHandle,
					&Module,
					sizeof(Module),
					&Needed)) &&
			    (GetModuleBaseName))
			{
				GetModuleBaseName(
						ProcessHandle,
						Module,
						ProcessName,
						sizeof(ProcessName));
			}
			
			if (strstr(ProcessName, "WehnTrust"))
			{
				TerminateProcess(
						ProcessHandle, 
						0);
			}
		}
	}
	return ERROR_SUCCESS;
}

/*
//
// Adds default exemptions for files or applications that are known to cause
// problems
//
static const LPTSTR ImageFileGlobalExemptions[] =
{
	TEXT("%WINDIR%\\system32\\CmdLineExt03.dll"),
	NULL
};

//
// Registry based exemptions
//
#define REG_EXEMPTION_TYPE_FOLDER 1
#define EXEMPT_IMAGE_FILES 1
#define EXEMPT_MEMORY_ALLOCATIONS 2

// Norton Internet Security 2004
//
// 	HKLM\Software\Symantec\InstalledApps
// 		Norton Internet Security 
// 		Symantec Shared Directory
//
// Norton 2004
//
// 	HKLM\Software\Symantec\InstalledApps
// 		NAV
// 		Common Client
//
// Cygwin
//
// 	HKLM\Software\Cygnus Solutions\Cygwin\mounts v2\/
//

static struct
{
	HKEY    RootKey;
	LPCTSTR BaseKey;
	LPCTSTR ValueName;
	ULONG   Type;
	ULONG   Flags;
} RegistryBasedExemptions[] = 
{
	// 
	// Norton
	//
	{ HKEY_LOCAL_MACHINE, 
	  TEXT("Software\\Symantec\\InstalledApps"), 
	  TEXT("Norton Internet Security"), 
	  REG_EXEMPTION_TYPE_FOLDER,
	  EXEMPT_IMAGE_FILES
	},
	{ HKEY_LOCAL_MACHINE, 
	  TEXT("Software\\Symantec\\InstalledApps"), 
	  TEXT("Symantec Shared Directory"),
	  REG_EXEMPTION_TYPE_FOLDER,
	  EXEMPT_IMAGE_FILES
	},
	{ HKEY_LOCAL_MACHINE, 
	  TEXT("Software\\Symantec\\InstalledApps"), 
	  TEXT("NAV"),
	  REG_EXEMPTION_TYPE_FOLDER,
	  EXEMPT_IMAGE_FILES
	},
	{ HKEY_LOCAL_MACHINE, 
	  TEXT("Software\\Symantec\\InstalledApps"), 
	  TEXT("Common Client"),
	  REG_EXEMPTION_TYPE_FOLDER,
	  EXEMPT_IMAGE_FILES
	},

	//
	// Cygwin
	//
	{ HKEY_LOCAL_MACHINE, 
	  TEXT("Software\\Cygnus Solutions\\Cygwin\\mounts v2\\/"),
	  TEXT("native"),
	  REG_EXEMPTION_TYPE_FOLDER,
	  EXEMPT_MEMORY_ALLOCATIONS
	},

	{ 0,
	  NULL,
	  NULL,
	  0
	},
};

BOOLEAN AddExemption(
		LPCTSTR FilePath, 
		DWORD Type, 
		DWORD Flags);
BOOLEAN AddExemptionFromRegistryValue(
		HKEY RootKey,
		LPCTSTR BaseKey,
		LPCTSTR ValueName,
		ULONG Type,
		ULONG Flags);

_declspec(dllexport) ULONG _stdcall AddDefaultExemptions(MSIHANDLE Install)
{
	DWORD Index;
	HKEY  Key;

	//
	// Ensure the proper keys exist
	//
	if (RegCreateKey(
			HKEY_LOCAL_MACHINE,
			TEXT("System\\CurrentControlSet\\Services\\baserand\\Exemptions"),
			&Key) == ERROR_SUCCESS)
		RegCloseKey(
				Key);
	if (RegCreateKey(
			HKEY_LOCAL_MACHINE,
			TEXT("System\\CurrentControlSet\\Services\\baserand\\Exemptions\\Global"),
			&Key) == ERROR_SUCCESS)
		RegCloseKey(
				Key);

	//
	// Static path exemptions
	//
	for (Index = 0;
	     ImageFileGlobalExemptions[Index];
	     Index++)
		AddExemption(ImageFileGlobalExemptions[Index],
				(DWORD)"2", EXEMPT_IMAGE_FILES);

	//
	// Registry Based Exemptions
	//
	for (Index = 0;
	     RegistryBasedExemptions[Index].BaseKey;
	     Index++)
		AddExemptionFromRegistryValue(
	   		RegistryBasedExemptions[Index].RootKey,
	   		RegistryBasedExemptions[Index].BaseKey,
	   		RegistryBasedExemptions[Index].ValueName,
	   		RegistryBasedExemptions[Index].Type,
				RegistryBasedExemptions[Index].Flags);

	return ERROR_SUCCESS;
}

//
// Adds an exemption to a given file
//
BOOLEAN AddExemption(LPCTSTR FilePath, DWORD Type, DWORD Flags)
{
	BOOLEAN Result = FALSE;
	TCHAR   TempBuffer[8192];
	HKEY    Key;

	//
	// Add global image file exemptions
	//
	if (RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			TEXT("System\\CurrentControlSet\\Services\\baserand\\Exemptions\\Global"),
			0,
			KEY_WRITE,
			&Key) == ERROR_SUCCESS)
	{
		UNICODE_STRING NtPath;
		SHA1_CTX       ShaContext;
		WCHAR          RegistryKeyName[64];
		UCHAR          ShaDigest[SHA1_HASH_SIZE];
		ULONG          Index = 0;
		HKEY           ExemptionKey;

		ZeroMemory(
				TempBuffer,
				sizeof(TempBuffer));

		//
		// Expand the path to the file that is to be exempted
		//
		ExpandEnvironmentStrings(
				FilePath,
				TempBuffer,
				sizeof(TempBuffer) - sizeof(TCHAR));

		//
		// Get the file's NT path
		//
		if (!DriverClient::GetNtPath(
				TempBuffer,
				&NtPath,
				FALSE))
		{
			RegCloseKey(
					Key);

			return FALSE;
		}

		SHA1_Init(
				&ShaContext);
		SHA1_Update(
				&ShaContext,
				(PUCHAR)NtPath.Buffer,
				NtPath.Length);
		SHA1_Final(
				&ShaContext,
				ShaDigest);

		//
		// Initialize the registry value name for this exemption
		//
		ZeroMemory(
				RegistryKeyName,
				sizeof(RegistryKeyName));

		_snwprintf_s(
				RegistryKeyName,
				sizeof(RegistryKeyName) / sizeof(WCHAR),
				(sizeof(RegistryKeyName) / sizeof(WCHAR)) - 1,
				L"%.8x%.8x%.8x%.8x%.8x",
				ShaContext.A,
				ShaContext.B,
				ShaContext.C,
				ShaContext.D,
				ShaContext.E);

		//
		// Create the exemption key
		//
		if (RegCreateKeyW(
				Key,
				RegistryKeyName,
				&ExemptionKey) == ERROR_SUCCESS)
		{
			RegSetValueExW(
					ExemptionKey,
					L"Path",
					0,
					REG_SZ,
					(PUCHAR)NtPath.Buffer,
					NtPath.Length);
			RegSetValueEx(
					ExemptionKey,
					TEXT("Type"),
					0,
					REG_DWORD,
					(LPBYTE)&Type,
					sizeof(Type));
			RegSetValueEx(
					ExemptionKey,
					TEXT("Flags"),
					0,
					REG_DWORD,
					(LPBYTE)&Flags,
					sizeof(Flags));

			RegCloseKey(
					ExemptionKey);

			Result = TRUE;
		}

		RegCloseKey(
				Key);
	}

	return Result;
}
	
//
// Gets a path from the registry and exemptions it
//
BOOLEAN AddExemptionFromRegistryValue(
		HKEY RootKey,
		LPCTSTR BaseKey,
		LPCTSTR ValueName,
		ULONG Type,
		ULONG Flags)
{
	BOOLEAN Result = FALSE;
	HKEY    Key;

	if (RegOpenKeyEx(
			RootKey,
			BaseKey,
			0,
			KEY_READ,
			&Key) == ERROR_SUCCESS)
	{
		switch (Type)
		{
			case REG_EXEMPTION_TYPE_FOLDER:
				{
					TCHAR Directory[2048];
					DWORD DirectorySize = sizeof(Directory);
					DWORD Type = REG_SZ;

					ZeroMemory(
							Directory,
							sizeof(Directory));

					if (RegQueryValueEx(
							Key,
							ValueName,
							0,
							&Type,
							(LPBYTE)Directory,
							&DirectorySize) == ERROR_SUCCESS)
					{
						Result = AddExemption(Directory,
								DirectoryExemption, Flags);
					}
				}
				break;
			default:
				break;
		}

		RegCloseKey(
				Key);
	}

	return Result;
}
*/
