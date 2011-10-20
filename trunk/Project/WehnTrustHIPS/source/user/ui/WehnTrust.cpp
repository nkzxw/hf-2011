/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Precomp.h"

//
// This user-mode executable is responsible for managing the environment
// synchronization from user-mode to the driver and for providing the user with
// an interface to communicating with the driver.
//
BOOLEAN CheckRunning();

//
// Restarts the user interface
//
VOID RestartUserInterface(
		IN HINSTANCE Inst);
VOID RedirectUninstall(
		IN LPCSTR Cmd);

struct {
	LPCSTR Name;
	DWORD  (*Function)();
} ArgumentDispatchTable[] = {
	{ "CheckSynchronize",        Env::CheckSynchronize        },
	{ "CheckSynchronizeInstall", Env::CheckSynchronizeInstall },
	{ NULL,                      NULL                         },
};

int WINAPI WinMain(
		HINSTANCE Inst, 
		HINSTANCE PrevInst,
		LPSTR CmdLine,
		int Show)
{
	DWORD Result = ERROR_NOT_FOUND;
	DWORD Index;
	BOOL  FromStartup = FALSE;
	BOOL  Found = FALSE;

	//
	// Always load the language library here
	//
	Locale::LoadLanguageLibrary();

	//
	// An internal check to see if we're being called from startup
	//
	if (!_stricmp(CmdLine, "Startup"))
		FromStartup = TRUE;
	else if (!strncmp(CmdLine, "Uninstall ", 10))
		RedirectUninstall(CmdLine + 10);

	//
	// Walk the dispatch table comparing the passed in command line argument with
	// the symbolic name for the dispatch handler to be called.
	//
	for (Index = 0;
	     ArgumentDispatchTable[Index].Name;
	     Index++)
	{
		if (!_stricmp(CmdLine, ArgumentDispatchTable[Index].Name))
		{
			Result = ArgumentDispatchTable[Index].Function();

			Log(LOG_SEV_DEBUG, "'%s' returned %lu.",
					ArgumentDispatchTable[Index].Name,
					Result);

			Found = TRUE;

			break;
		}
	}

	//
	// If no match was found, display the user interface
	//
	if (!Found)
	{
		//
		// Check to see if another instance of WehnTrust is running, if so,
		// return success
		//
		if (CheckRunning())
			return ERROR_SUCCESS;

		Result = Ui::Display(
				TRUE,
				TRUE,
				FromStartup);

		//
		// If the user interface needs to restart, such as due to a locale change,
		// do that now.
		//
		if (Result == UI_RESULT_RESTART)
		{
			RestartUserInterface(
					Inst);
		}
	}

	return Result;
}

BOOLEAN CheckRunning()
{
	BOOLEAN Running = FALSE;
	HANDLE  Event = NULL;
	TCHAR   EventName[512], UserName[256];
	DWORD   UserNameSize = sizeof(UserName) / sizeof(TCHAR) - sizeof(TCHAR);

	ZeroMemory(
			UserName,
			sizeof(UserName));

	GetUserName(
			UserName,
			&UserNameSize);

	_sntprintf_s(EventName, 
			sizeof(EventName) / sizeof(TCHAR) - sizeof(TCHAR),
			TEXT("WehnTrust%s"),
			UserName);

	//
	// Check to see if another instance is already running 
	//
	Event = OpenEvent(
			EVENT_ALL_ACCESS, 
			FALSE, 
			EventName);

	//
	// If an instance is not running, create a named event to serve as a marker
	// that an instance is running for this user.  We should probably make this
	// session dependent.
	//
	if (!Event)
		Event = CreateEvent(
				NULL,
				FALSE,
				FALSE,
				EventName);
	else
	{
		Running = TRUE;

		CloseHandle(Event);
	}

	return Running;
}

VOID RestartUserInterface(
		IN HINSTANCE Inst)
{
	PROCESS_INFORMATION ProcessInformation;
	STARTUPINFO         Startup;
	TCHAR               CommandPath[1024];

	ZeroMemory(
			CommandPath,
			sizeof(CommandPath));
	ZeroMemory(
			&Startup,
			sizeof(Startup));

	GetModuleFileName(
			Inst,
			CommandPath,
			sizeof(CommandPath) / sizeof(TCHAR));

	Startup.cb = sizeof(Startup);

	if (CreateProcess(
			NULL,
			CommandPath,
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			NULL,
			&Startup,
			&ProcessInformation))
	{
		CloseHandle(
				ProcessInformation.hProcess);
		CloseHandle(
				ProcessInformation.hThread);
	}
}

//
// This routine redirects the uninstall process based on whether or not the
// system has been booted into safe mode
//
VOID RedirectUninstall(
		IN LPCSTR Cmd)
{
	//
	// If we're in safe mode...
	//
	if (GetSystemMetrics(SM_CLEANBOOT) > 0)
	{
		BOOLEAN Success = FALSE;
		DWORD   Value = 3;
		HKEY    Key = NULL;

		do
		{
			if (RegOpenKeyEx(
					HKEY_LOCAL_MACHINE,
					TEXT("System\\CurrentControlSet\\Services\\baserand"),
					0,
					KEY_WRITE,
					&Key) != ERROR_SUCCESS)
				break;

			if (RegSetValueEx(
					Key,
					TEXT("Start"),
					0,
					REG_DWORD,
					(LPBYTE)&Value,
					sizeof(Value)) != ERROR_SUCCESS)
				break;

			Success = TRUE;

		} while (0);

		if (Key)
			RegCloseKey(
					Key);

		//
		// Display a message based on whether or not it succeeds or fails
		//
		if (!Success)
			MessageBox(
					NULL,
					TEXT("The WehnTrust software could not be disabled.  Please contact technical support."),
					TEXT("WehnTrust Driver Could Not Be Disabled"),
					MB_OK | MB_ICONEXCLAMATION);
		else
			MessageBox(
					NULL,
					TEXT("The WehnTrust software has been disabled.  Please reboot to remove the product."),
					TEXT("WehnTrust Driver Disabled"),
					MB_OK | MB_ICONEXCLAMATION);

	}
	//
	// Otherwise, pass the uninstall on to MSI
	//
	else
	{
		PROCESS_INFORMATION pi;
		STARTUPINFO si;

		memset(&si, 0, sizeof(si));

		si.cb = sizeof(si);

		CreateProcess(
				NULL,
				(char *)Cmd,
				NULL,
				NULL,
				FALSE,
				0,
				NULL,
				NULL,
				&si,
				&pi);
	}

	ExitProcess(0);
}
