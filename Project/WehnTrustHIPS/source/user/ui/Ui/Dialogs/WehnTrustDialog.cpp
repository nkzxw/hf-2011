/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Precomp.h"

#define TIMER_ID_REFRESH 0x01

WehnTrustDialog::WehnTrustDialog()
: Dialog(IDD_WEHNTRUST),
  DriverHandle(NULL),
  CurrentlyEnabledSubsystems(0),
  ExitOnClose(FALSE)
{
	Status.SetParent(
			this);
}

WehnTrustDialog::~WehnTrustDialog()
{
	DriverClient::Close(
			DriverHandle);
}

////
//
// Notifications
//
////

//
// Load the locale specific values and create the child dialogs
//
BOOLEAN WehnTrustDialog::OnInit()
{
	LPTSTR String;
	TCITEM TabControlItem;
	HICON  Icon;
	HWND   TabControl;

	RefreshTitle();

	//
	// Set the dialog's icon
	//
	if ((Icon = LoadIcon(
			GetInstance(),
			MAKEINTRESOURCE(IDI_WEHNTRUST_ENABLED))))
	{
		SendMessage(
				GetWindow(),
				WM_SETICON,
				(WPARAM)ICON_BIG,
				(LPARAM)Icon);

		SendMessage(
				GetWindow(),
				WM_SETICON,
				(WPARAM)ICON_SMALL,
				(LPARAM)Icon);

		DestroyIcon(
				Icon);
	}

	//
	// Update the individual control's text from the locale DLL if possible
	//
	UpdateControlText(
			WEHNTRUST_BUTTON_OK,
			IDS_OK);
	UpdateControlText(
			WEHNTRUST_BUTTON_APPLY,
			IDS_APPLY);
	UpdateControlText(
			WEHNTRUST_BUTTON_CANCEL,
			IDS_CANCEL);
	UpdateControlText(
			WEHNTRUST_BUTTON_HELP,
			IDS_HELP);

	//
	// Build the tabs
	//
	TabControl = GetControl(
			WEHNTRUST_TAB);

	TabControlItem.mask    = TCIF_TEXT;

	//
	// Status
	//
	if (!(TabControlItem.pszText = String = Locale::LoadString(
			IDS_STATUS)) ||
	    (!String[0]))
		TabControlItem.pszText = TEXT("Status");

	TabCtrl_InsertItem(
			TabControl,
			0,
			&TabControlItem);

	if (String)
		Locale::FreeString(
				String);

	//
	// Exemptions
	//
	
	if (!(TabControlItem.pszText = String = Locale::LoadString(
			IDS_EXEMPTIONS)) ||
	    (!String[0]))
		TabControlItem.pszText = TEXT("Exemptions");

	TabCtrl_InsertItem(
			TabControl,
			1,
			&TabControlItem);

	if (String)
		Locale::FreeString(
				String);

	//
	// About
	//
	if (!(TabControlItem.pszText = String = Locale::LoadString(
			IDS_ABOUT)) ||
	    (!String[0]))
		TabControlItem.pszText = TEXT("About");

	TabCtrl_InsertItem(
			TabControl,
			2,
			&TabControlItem);

	if (String)
		Locale::FreeString(
				String);

	//
	// Initialize the child dialogs
	//
	Status.Create(
			GetWindow());
	Exemptions.Create(
			GetWindow());
	About.Create(
			GetWindow());
				
	//
	// Change the position of the child dialogs
	//
	Status.SetPosition(
			10,
			90);
	Exemptions.SetPosition(
			10,
			90);
	About.SetPosition(
			10,
			90);

	Exemptions.Hide();
	About.Hide();

	//
	// Update status -- if the driver handle is invalid, pop up an alert letting
	// the user know that their driver is not properly installed
	//
	RefreshStatus(
			TRUE);

	if (!DriverHandle)
	{
		LPTSTR String = Locale::LoadString(
				IDS_DRIVER_NOT_FOUND);

		if (String)
		{
			MessageBox(
					GetWindow(),
					String,
					0,
					MB_OK | MB_ICONEXCLAMATION);

			Locale::FreeString(
					String);
		}

		//
		// Disable the check boxes on the status window
		//
		EnableWindow(
				Status.GetControl(STATUS_CHECK_RANDDLL),
				FALSE);
		EnableWindow(
				Status.GetControl(STATUS_CHECK_RANDALLOC),
				FALSE);
	}

	//
	// Disable the apply button at this point
	//
	DisableApply();

	//
	// Set the refresh timer
	//
	if (DriverHandle)
		SetTimer(
				GetWindow(),
				TIMER_ID_REFRESH,
				10000,
				NULL);

	return TRUE;
}

//
// Simply allow the dialog to close when requested
//
BOOLEAN WehnTrustDialog::OnClose()
{
	//
	// Close the child dialogs
	//
	Status.Close();
	Exemptions.Close();
	About.Close();

	//
	// Close the client
	//
	if (ExitOnClose)
		Ui::Close();

	return TRUE;
}

//
// Save & Close the dialog
//
VOID WehnTrustDialog::OnOK()
{
	OnApply();

	Close();
}

//
// Saves current settings
//
VOID WehnTrustDialog::OnApply()
{
	DWORD Result;
	DWORD Check = 0;
	ULONG DisableSubsystems = 0;
	ULONG EnableSubsystems = 0;
	ULONG CurrentSubsystem;

	DisableApply();

	//
	// Check to see if we need to enable or disable the tray icon
	//
	if (((Check = SendMessage(
			Status.GetControl(STATUS_CHECK_TRAYICON),
			BM_GETCHECK,
			0,
			0)) == BST_CHECKED) &&
	    (Ui::IsTrayDisabled()))
	{
		Ui::EnableTray();
	}
	else if ((Check == BST_UNCHECKED) &&
	         (!Ui::IsTrayDisabled()))
	{
		Ui::DisableTray();
	}

	//
	// If there is no driver handle, do not attempt to update things
	//
	if (!DriverHandle)
		return;

	//
	// Build the mask of subsystems to enable
	//
	CurrentSubsystem = RANDOMIZATION_SUBSYSTEM_DLL;

	if ((Check = SendMessage(
			Status.GetControl(STATUS_CHECK_RANDDLL),
			BM_GETCHECK,
			0,
			0)) == BST_CHECKED)
	{
		if (!(CurrentlyEnabledSubsystems & CurrentSubsystem))
			EnableSubsystems |= CurrentSubsystem;
	}
	else if (CurrentlyEnabledSubsystems & CurrentSubsystem)
		DisableSubsystems |= RANDOMIZATION_SUBSYSTEM_DLL;

	CurrentSubsystem = RANDOMIZATION_SUBSYSTEM_ALLOCATIONS;

	if ((Check = SendMessage(
			Status.GetControl(STATUS_CHECK_RANDALLOC),
			BM_GETCHECK,
			0,
			0)) == BST_CHECKED)
	{
		if (!(CurrentlyEnabledSubsystems & CurrentSubsystem))
			EnableSubsystems |= CurrentSubsystem;
	}
	else if (CurrentlyEnabledSubsystems & CurrentSubsystem)
		DisableSubsystems |= CurrentSubsystem;

	//
	// If there are subsystems to disable...
	//
	if (DisableSubsystems)
	{
		//
		// Disable subsystems first
		//
		if ((Result = DriverClient::Stop(
				DriverHandle,
				DisableSubsystems)) != ERROR_SUCCESS)
		{
			Log(LOG_SEV_ERROR, 
					TEXT("OnApply(): DriverClient::Stop failed, %.8x."),
					Result);
		}
	}

	//
	// If there are subsystems to enable...
	//
	if (EnableSubsystems)
	{
		//
		// Enable subsystems next
		//
		if ((Result = DriverClient::Start(
				DriverHandle,
				EnableSubsystems)) != ERROR_SUCCESS)
		{
			Log(LOG_SEV_ERROR, 
					TEXT("OnApply(): DriverClient:Start failed, %.8x."),
					Result);
		}
	}

	//
	// Synchronize status again with the driver just to make sure we have the
	// right impression of what's up
	//
	RefreshStatus();
}

//
// Close the dialog
//
VOID WehnTrustDialog::OnCancel()
{
	Close();
}

//
// Enables the apply button
//
VOID WehnTrustDialog::EnableApply()
{
	EnableWindow(
			GetControl(WEHNTRUST_BUTTON_APPLY),
			TRUE);
}

//
// Disables the apply button
//
VOID WehnTrustDialog::DisableApply()
{
	EnableWindow(
			GetControl(WEHNTRUST_BUTTON_APPLY),
			FALSE);
}

//
// Set the boolean for whether or not the executable should exit on closure
//
VOID WehnTrustDialog::SetExitOnClose(
		IN BOOLEAN TruthValue)
{
	ExitOnClose = TruthValue;
}

////
//
// Protected methods
//
////

//
// Handles message notifications to some of the controls that are hosted on the
// dialog, such as the tab control.
//
LRESULT WehnTrustDialog::OnNotifyControl(
		IN INT ControlId,
		IN HWND ControlWindow,
		IN UINT Code,
		IN PVOID Notify)
{
	LRESULT Result = 0;

	switch (ControlId)
	{
		//
		// The tab control
		//
		case WEHNTRUST_TAB:
			switch (Code)
			{
				//
				// Tab switch
				//
				case TCN_SELCHANGE:
					{
						INT Index;

						Index = TabCtrl_GetCurSel(
								ControlWindow);

						//
						// Switch tabs accordingly
						//
						if (Index == 0)
						{
							Exemptions.Hide();
							About.Hide();
							Status.Show();
						}
						else if (Index == 1)
						{
							Status.Hide();
							About.Hide();
							Exemptions.Show();
						}
						else
						{
							Status.Hide();
							Exemptions.Hide();
							About.Show();
						}
					}
					break;
			}
			break;
		//
		// The OK button
		//
		case WEHNTRUST_BUTTON_OK:
			switch (Code)
			{
				//
				// Click
				//
				case BN_CLICKED:
					OnOK();
					break;
			}
			break;
		//
		// The Apply button
		//
		case WEHNTRUST_BUTTON_APPLY:
			switch (Code)
			{
				//
				// Click
				//
				case BN_CLICKED:
					OnApply();
					break;
			}
			break;
		//
		// The Cancel button
		//
		case WEHNTRUST_BUTTON_CANCEL:
			switch (Code)
			{
				//
				// Click
				//
				case BN_CLICKED:
					OnCancel();
					break;
			}
			break;
		default:
			break;
	}

	return Result;
}

//
// Callback for WM_TIMER notifications
//
VOID WehnTrustDialog::OnTimer(
		IN UINT TimerId)
{
	if (TimerId == TIMER_ID_REFRESH)
		RefreshStatus();
}

//
// Synchronize the current status with the user interface
//
VOID WehnTrustDialog::RefreshStatus(
		IN BOOLEAN UpdateEnabledStatus)
{
	WEHNTRUST_STATISTICS Statistics;
	DWORD                Result;

	//
	// Update the tray icon check
	//
	SendMessage(
			Status.GetControl(STATUS_CHECK_TRAYICON),
			BM_SETCHECK,
			Ui::IsTrayDisabled() 
				? BST_UNCHECKED
				: BST_CHECKED,
			0);

	//
	// Open a handle to the driver if possible
	//
	if (!DriverHandle)
		DriverHandle = DriverClient::Open();

	if (!DriverHandle)
	{
		Log(LOG_SEV_ERROR, 
				TEXT("RefreshStatus(): The driver could not be opened, %lu."),
				GetLastError());
		//
		// Update statistics fromt he buffer regardless of success
		//
		Status.UpdateControlTextFmt(
				STATUS_LABEL_STAT_NUMRANDDLL,
				TRUE,
				Locale::LoadString(IDS_STAT_NUMRANDDLL),
				0);
		
		Status.UpdateControlTextFmt(
				STATUS_LABEL_STAT_NUMEXEMPTDLL,
				TRUE,
				Locale::LoadString(IDS_STAT_NUMEXEMPTDLL),
				0);
		
		Status.UpdateControlTextFmt(
				STATUS_LABEL_STAT_NUMIMAGESET,
				TRUE,
				Locale::LoadString(IDS_STAT_NUMIMAGESET),
				0);				

		Status.UpdateControlTextFmt(
				STATUS_LABEL_STAT_NUMRANDALLOC,
				TRUE,
				Locale::LoadString(IDS_STAT_NUMRANDALLOC),
				0);

		Status.UpdateControlTextFmt(
				STATUS_LABEL_STAT_EXPPREV,
				TRUE,
				Locale::LoadString(IDS_STAT_EXPPREV),
				0);

		return;
	}

	//
	// Initialize the structure's meta information
	//
	ZeroMemory(
			&Statistics,
			sizeof(Statistics));

	InitializeStructure(
			&Statistics,
			sizeof(Statistics));

	//
	// Get statistic information from the driver
	//
	if ((Result = DriverClient::GetStatistics(
			DriverHandle,
			&Statistics)) != ERROR_SUCCESS)
	{
		Log(LOG_SEV_ERROR, 
				TEXT("RefreshStatus(): GetStatistics failed, %lu."), 
				Result);
	}

	//
	// Update statistics fromt he buffer regardless of success
	//
	Status.UpdateControlTextFmt(
			STATUS_LABEL_STAT_NUMRANDDLL,
			TRUE,
			Locale::LoadString(IDS_STAT_NUMRANDDLL),
			Statistics.NumberOfRandomizedDlls);
	
	Status.UpdateControlTextFmt(
			STATUS_LABEL_STAT_NUMEXEMPTDLL,
			TRUE,
			Locale::LoadString(IDS_STAT_NUMEXEMPTDLL),
			Statistics.NumberOfRandomizationsExempted);
	
	Status.UpdateControlTextFmt(
			STATUS_LABEL_STAT_NUMIMAGESET,
			TRUE,
			Locale::LoadString(IDS_STAT_NUMIMAGESET),
			Statistics.NumberOfImageSets);

	Status.UpdateControlTextFmt(
			STATUS_LABEL_STAT_NUMRANDALLOC,
			TRUE,
			Locale::LoadString(IDS_STAT_NUMRANDALLOC),
			Statistics.NumberOfRandomizedAllocations);

	Status.UpdateControlTextFmt(
			STATUS_LABEL_STAT_EXPPREV,
			TRUE,
			Locale::LoadString(IDS_STAT_EXPPREV),
			Config::GetExploitsPreventedCount());

	//
	// Cache the current value of the enabled subsystems
	//
	CurrentlyEnabledSubsystems = Statistics.Enabled;

	//
	// If the caller wants us to update the status of the checkboxes, do that
	// now.
	//
	if (UpdateEnabledStatus)
	{
		SendMessage(
				Status.GetControl(STATUS_CHECK_RANDDLL),
				BM_SETCHECK,
				(Statistics.Enabled & RANDOMIZATION_SUBSYSTEM_DLL) 
					? BST_CHECKED
					: BST_UNCHECKED,
				0);
		
		SendMessage(
				Status.GetControl(STATUS_CHECK_RANDALLOC),
				BM_SETCHECK,
				(Statistics.Enabled & RANDOMIZATION_SUBSYSTEM_ALLOCATIONS) 
					? BST_CHECKED
					: BST_UNCHECKED,
				0);
	}

}	

//
// Updates the dialog's title, including whether or not the free or commercial
// version is being used.
//
VOID WehnTrustDialog::RefreshTitle()
{
	LPTSTR Title;
	TCHAR  TitleExt[1024];

	if ((Title = Locale::LoadString(
			IDS_WEHNTRUST_DIALOG_TITLE)))
	{
		_sntprintf_s(
				TitleExt,
				sizeof(TitleExt) / sizeof(TCHAR),
				(sizeof(TitleExt) / sizeof(TCHAR)) - 1,
				"%s",
				Title);

		Locale::FreeString(Title);
	}
	else
		_sntprintf_s(
				TitleExt,
				sizeof(TitleExt) / sizeof(TCHAR),
				(sizeof(TitleExt) / sizeof(TCHAR)) - 1,
				"WehnTrust");

	SendMessage(
			GetWindow(),
			WM_SETTEXT,
			0,
			(LPARAM)TitleExt);
}
