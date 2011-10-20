/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Precomp.h"

//
// This routine enumerates all of the images, checking them for changes and
// optionally synchronizing new ones if differences are found.
//
DWORD Env::CheckSynchronize()
{
	BOOLEAN Updated = FALSE;
	DWORD   Result;

	//
	// Perform the check/synchronize operation
	//
	Result = DoCheckSynchronize(
			&Updated);

	//
	// If the jump tables in the registry had to be updated, notify the user that
	// a reboot must occur before the changes can take effect and that while
	// randomization has been disabled until they reboot
	//
	if (Updated)
	{
		LPTSTR BodyString, TitleString;
		DWORD  Answer;

		BodyString = Locale::LoadString(
				IDS_SYSTEM_UPDATED);
		TitleString = Locale::LoadString(
				IDS_SYSTEM_UPDATED_TITLE);

		//
		// Ask the user if they want to reboot
		//
		Answer = MessageBox(
				NULL,
				BodyString,
				TitleString,
				MB_YESNO | MB_ICONEXCLAMATION | MB_SETFOREGROUND);

		if (BodyString)
			Locale::FreeString(
					BodyString);
		if (TitleString)
			Locale::FreeString(
					TitleString);

		//
		// If the user responds with yes, reboot their computer
		//
		if (Answer == IDYES)
		{
			TOKEN_PRIVILEGES Privs;
			HANDLE           Token = NULL;

			OpenProcessToken(
					GetCurrentProcess(),
					TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
					&Token);

			Privs.PrivilegeCount           = 1;
			Privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			LookupPrivilegeValue(
					NULL,
					SE_SHUTDOWN_NAME,
					&Privs.Privileges[0].Luid);

			AdjustTokenPrivileges(
					Token,
					FALSE,
					&Privs,
					0,
					NULL,
					NULL);

			ExitWindowsEx(
					EWX_REBOOT,
					0);
		}
	}

	return Result;
}

//
// This method is called from the installer when the person installs to
// initially obtain environmental information
//
DWORD Env::CheckSynchronizeInstall()
{
	DWORD Result;
	//
	// Do the check synchronize operation but do no attempt to reboot if things
	// are updated
	//
	
	Result = DoCheckSynchronize();

	//
	// Display a message box if synchronization fails
	//
	if (Result != ERROR_SUCCESS)
	{
		LPTSTR TitleString, BodyString;
		
		TitleString = Locale::LoadString(
				IDS_CHECK_SYNC_FAIL_TITLE);
		BodyString = Locale::LoadString(
				IDS_CHECK_SYNC_FAIL_BODY);

		//
		// Display the notification letting the user know that their environment
		// could not be synchronized for the device driver.
		//
		MessageBox(
				NULL,
				BodyString,
				TitleString,
				MB_OK);

		//
		// Release the locale strings
		//
		if (TitleString)
			Locale::FreeString(
					TitleString);
		if (BodyString)
			Locale::FreeString(
					BodyString);
	}

	return Result;
}

////
//
// Protected Methods
//
////

DWORD Env::DoCheckSynchronize(
		OUT PBOOLEAN OutUpdated)
{
	BOOLEAN Updated = FALSE;
	DWORD   Synchronized = 0;
	DWORD   Result;
	HKEY    ImagesKey;

	//
	// If an image implementor returns a failure status, try to synchronize it.
	//
	do
	{
		// NTDLL
		Ntdll ntdll;

		if (ntdll.Check() != ERROR_SUCCESS)
		{
			if ((Result = ntdll.Synchronize()) != ERROR_SUCCESS)
				break;
			else
				Updated = TRUE;
		}

		// KERNEL32
		Kernel32 k32;

		if (k32.Check() != ERROR_SUCCESS)
		{
			if ((Result = k32.Synchronize()) != ERROR_SUCCESS)
				break;
			else
				Updated = TRUE;
		}

		// USER32
		//
		// TODO
		// 
		// This is currently disabled because a means to locate the dispatch table
		// on 2K/2K3 has not been finished.
		//
#if 0
		User32 u32;

		if (u32.Check() != ERROR_SUCCESS)
		{
			if ((Result = u32.Synchronize()) != ERROR_SUCCESS)
				break;
			else
				Updated = TRUE;
		}
#endif

		Synchronized = 1;
		Result       = ERROR_SUCCESS;

	} while (0);

	//
	// Set the overall flag in the registry that lets the driver know whether or
	// not all of the images were successfully synchronized.  This flag is
	// important as if it were not present the driver might assume that a
	// partially initialized address table is valid and thus lead to very ugly
	// crashes.
	//
	if (RegOpenKeyEx(
			WEHNTRUST_ROOT_KEY,
			WEHNTRUST_BASE_KEY_ROOT TEXT("\\Images"),
			0,
			KEY_WRITE,
			&ImagesKey) == ERROR_SUCCESS)
	{
		RegSetValueEx(
				ImagesKey,
				TEXT("Synchronized"),
				0,
				REG_DWORD,
				(LPBYTE)&Synchronized,
				sizeof(Synchronized));

		RegCloseKey(ImagesKey);
	}

	if (OutUpdated)
		*OutUpdated = Updated;

	return Result;
}
