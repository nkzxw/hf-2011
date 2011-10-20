/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Precomp.h"

Ui *GlobalUi = NULL;

Ui::Ui()
: Result(UI_RESULT_OK),
  TrayDisabled(FALSE),
  MonitoringMessages(FALSE),
  Closing(FALSE)
{
	//
	// Check to see if the system tray should be disabled
	//
	TrayDisabled = !(Config::IsSystemTrayEnabled());
}

Ui::~Ui()
{
}

//
// Get a pointer to the Ui singleton
//
Ui *Ui::GetInstance()
{
	//
	// If the ui singleton has yet to be created, do so now
	//
	if (!GlobalUi)
	{
		try
		{
			GlobalUi = new Ui;
		} catch (...)
		{
		}
	}

	return GlobalUi;
}

//
// Display the user interface based on whatever policy the user has selected.
//
DWORD Ui::Display(
		IN BOOLEAN ShowDialog, 
		IN BOOLEAN ShowTray,
		IN BOOLEAN FromStartup)
{
	DWORD Result = ERROR_SUCCESS;
	Ui *Instance = Ui::GetInstance();

	//
	// If we're being called from startup and the system tray is disabled then we
	// shall not present a user interface and just exit
	//
	if ((Instance->TrayDisabled) &&
	    (FromStartup))
		return ERROR_SUCCESS;

	do
	{
		//
		// Create the system tray icon.  If the user has opted to not have a
		// system tray icon, this 
		//
		if ((ShowTray) &&
		    (!Instance->TrayDisabled))
		{
			if ((Result = Instance->DisplayTrayIcon()) != ERROR_SUCCESS)
				break;
		}

		//
		// If the caller requested that we also display the dialog, do it.
		//
		if ((ShowDialog) &&
		    (!FromStartup))
		{
			if ((Result = Instance->DisplayDialog()) != ERROR_SUCCESS)
				break;

			if (Instance->TrayDisabled)
				Instance->Status.SetExitOnClose(
						TRUE);
		}

	} while (0);

	//
	// If the result was a success and we aren't monitoring window messages,
	// start doing that now.
	//
	if ((Result == ERROR_SUCCESS) &&
	    (!Instance->MonitoringMessages))
	{
		Instance->MonitoringMessages = TRUE;

		Result = Instance->HandleWindowMessages();
	}

	//
	// If the UI is closing, return the result that was set on the instance
	//
	if (Instance->Closing)
		Result = Instance->Result;

	//
	// If the above operations succeeded, monitor messages
	//
	return Result;
}

//
// Close the status dialog and system tray and post our quit message
//
DWORD Ui::Close(
		IN DWORD Result)
{
	Ui *Instance = Ui::GetInstance();

	if (!Instance->Closing)
	{
		Instance->Closing = TRUE;
		Instance->Result  = Result;

		Instance->Status.Close();
		Instance->Tray.Destroy();
	
		PostQuitMessage(0);
	}

	return ERROR_SUCCESS;
}

//
// Checks to see if the system tray is enabled or disabled
//
BOOLEAN Ui::IsTrayDisabled()
{
	Ui *Instance = Ui::GetInstance();

	return Instance->TrayDisabled;
}

VOID Ui::EnableTray()
{
	if (IsTrayDisabled())
	{
		Ui *Instance = Ui::GetInstance();

		Instance->DisplayTrayIcon();
	}
}

VOID Ui::DisableTray()
{
	if (!IsTrayDisabled())
	{
		Ui *Instance = Ui::GetInstance();

		Instance->DisplayTrayIcon(
				FALSE);
	}
}

////
//
// Protected Methods
//
////

//
// Handles the dispatching of Window messages
//
DWORD Ui::HandleWindowMessages()
{
	INITCOMMONCONTROLSEX ice;
	MSG                  msg;

	//
	// Initialize common controls
	//
	ZeroMemory(&ice, sizeof(ice));

	ice.dwSize = sizeof(INITCOMMONCONTROLSEX);;
	ice.dwICC  = ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES | 
	             ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES | ICC_ANIMATE_CLASS;

	if (!InitCommonControlsEx(&ice))
	{
		Log(LOG_SEV_ERROR, 
				TEXT("HandleWindowMessages(): InitCommonControlsEx failed, %lu."),
				GetLastError());

		return GetLastError();
	}

	//
	// Keep handling window messages until receive the quit message
	//
	while (GetMessage(&msg, NULL, 0, 0))
	{
		//
		// Check to see if it's destined to the status window
		//
		if ((!IsDialogMessage(
				Status.GetWindow(), 
				&msg)))
		{
			TranslateMessage(
					&msg);
			DispatchMessage(
					&msg);
		}
	}

	//
	// Destroy ourselves if we get here as the quit message has been posted
	//
	delete Ui::GetInstance();

	return ERROR_SUCCESS;
}

//
// Create and displays the status dialog
//
DWORD Ui::DisplayDialog()
{
	return Status.Create();
}

//
// Create and displays the system tray icon
//
DWORD Ui::DisplayTrayIcon(
		IN BOOLEAN Show)
{
	DWORD Result;

	if (Show)
	{
		Result = Tray.Create();

		if (Result == ERROR_SUCCESS)
			TrayDisabled = FALSE;
		
		Status.SetExitOnClose(
				FALSE);
	}
	else
	{
		Result = Tray.Destroy();
		
		if (Result == ERROR_SUCCESS)
			TrayDisabled = TRUE;

		Status.SetExitOnClose(
				TRUE);
	}

	if (Result == ERROR_SUCCESS)
		Config::SetSystemTrayEnabled(
				Show);

	return Result;
}
