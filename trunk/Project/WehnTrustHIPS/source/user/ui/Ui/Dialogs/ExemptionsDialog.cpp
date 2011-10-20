/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#include "Precomp.h"

ExemptionsDialog::ExemptionsDialog()
: Dialog(IDD_EXEMPTIONS),
  Parent(NULL)
{
}

ExemptionsDialog::~ExemptionsDialog()
{
}

//
// Set the pointer to the parent dialog
//
VOID ExemptionsDialog::SetParent(
		IN WehnTrustDialog *InParent)
{
	Parent = InParent;
}

//
//
//
BOOLEAN ExemptionsDialog::OnInit()
{
	LPTSTR SelectOptionApplication;
	LPTSTR SelectOptionImage;
	LPTSTR SelectOptionDirectory;
	HWND   ExemptionTypeWindow;
	RECT   DropDownRect;

	//
	// Initialize the select options
	//
	SelectOptionApplication = Locale::LoadStringDefault(
			IDS_APPLICATION_EXEMPTIONS,
			TEXT("Application Exemptions"));
	SelectOptionImage = Locale::LoadStringDefault(
			IDS_IMAGE_EXEMPTIONS,
			TEXT("Image File Exemptions"));
	SelectOptionDirectory = Locale::LoadStringDefault(
			IDS_DIRECTORY_EXEMPTIONS,
			TEXT("Directory Exemptions"));

	//
	// Get the exemption type drop list
	//
	ExemptionTypeWindow = GetControl(
			EXEMPTIONS_DROPDOWN_SELECTION);

	//
	// Set the drop down size
	//
	GetWindowRect(
			ExemptionTypeWindow,
			&DropDownRect);

	SetWindowPos(
			ExemptionTypeWindow,
			NULL,
			0,
			0,
			DropDownRect.right - DropDownRect.left,
			100,
			SWP_NOZORDER | SWP_NOMOVE | SWP_NOOWNERZORDER);

	//
	// Add the drop down options
	//
	if (SelectOptionApplication)
	{
		SendMessage(
				ExemptionTypeWindow,
				CB_ADDSTRING,
				0,
				(LPARAM)(LPCTSTR)SelectOptionApplication);

		Locale::FreeString(
				SelectOptionApplication);
	}

	if (SelectOptionImage)
	{
		SendMessage(
				ExemptionTypeWindow,
				CB_ADDSTRING,
				0,
				(LPARAM)(LPCTSTR)SelectOptionImage);

		Locale::FreeString(
				SelectOptionImage);
	}

	if (SelectOptionDirectory)
	{
		SendMessage(
				ExemptionTypeWindow,
				CB_ADDSTRING,
				0,
				(LPARAM)(LPTSTR)SelectOptionDirectory);

		Locale::FreeString(
				SelectOptionDirectory);
	}

	//
	// Select the application exemptions
	//
	SendMessage(
			ExemptionTypeWindow,
			CB_SETCURSEL,
			0,
			0);

	//
	// Display application exemptions
	//
	DisplayApplicationExemptions();

	return TRUE;
}

//
// Clean up resources that were allocated while the dialog was displayed
//
BOOLEAN ExemptionsDialog::OnClose()
{
	FlushExemptionList();

	return TRUE;
}

////
//
// Protected Methods
//
////

//
// Handle control notifications
//
LRESULT ExemptionsDialog::OnNotifyControl(
		IN INT ControlId,
		IN HWND ControlWindow,
		IN UINT Code,
		IN PVOID Notify)
{
	LRESULT Result = 0;

	switch (ControlId)
	{
		//
		// Exemption list drop down
		//
		case EXEMPTIONS_DROPDOWN_SELECTION:
			switch (Code)
			{
				//
				// When the user changes selections
				//
				case CBN_SELCHANGE:
					{
						LRESULT Selection = SendMessage(
								GetControl(
									ControlId),
								CB_GETCURSEL,
								0,
								0);

						if (Selection == 0)
							DisplayApplicationExemptions();
						else if (Selection == 1)
							DisplayDirectoryExemptions();
						else
							DisplayImageFileExemptions();
					}
					break;
				default:
					break;
			}
			break;
		//
		// Add button
		//
		case EXEMPTIONS_BUTTON_ADD:
			switch (Code)
			{
				//
				// When the user wants to add a new exemption
				//
				case BN_CLICKED:
					{
						AddEditExemptionDialog AddDialog(
								GlobalScope,
								CurrentExemptionType,
								NULL);

						AddDialog.Create(
								GetWindow(),
								TRUE);

						SetForegroundWindow(
								GetWindow());

						DisplayExemptions(
								CurrentExemptionType);
					}
					break;
				default:
					break;
			}
			break;
		//
		// Edit button
		//
		case EXEMPTIONS_BUTTON_EDIT:
			switch (Code)
			{
				//
				// When the user wants to edit an exemption
				//
				case BN_CLICKED:
					{
						LPTSTR NtFilePath = GetCurrentSelectionNtFilePath();

						//
						// If we get back a valid path
						//
						if (NtFilePath)
						{
							AddEditExemptionDialog EditDialog(
									GlobalScope,
									CurrentExemptionType,
									NtFilePath);

							EditDialog.Create(
									GetWindow(),
									TRUE);

							SetForegroundWindow(
									GetWindow());
						}
						else
						{
							LPTSTR ErrorString = Locale::LoadStringDefault(
									IDS_NO_FILE_SELECTED,
									TEXT("You must select at least one file."));

							if (ErrorString)
							{
								MessageBox(
										GetWindow(),
										ErrorString,
										NULL,
										MB_ICONEXCLAMATION | MB_OK);

								Locale::FreeString(
										ErrorString);
							}
						}
					}
					break;
				default:
					break;
			}
			break;
		//
		// Remove button
		//
		case EXEMPTIONS_BUTTON_REMOVE:
			switch (Code)
			{
				case BN_CLICKED:
					{
						BOOLEAN Refresh = FALSE;
						LRESULT SelCount;
						LPDWORD SelIndexes;
						LONG    Index;
						HWND    ExemptionList;

						ExemptionList = GetControl(
									EXEMPTIONS_LIST_APPLIED);

						//
						// Get the number of selected items
						//
						if ((SelCount = SendMessage(
								ExemptionList,
								LB_GETSELCOUNT,
								0,
								0)) == LB_ERR)
							break;

						if (SelCount == 0)
						{
							LPTSTR ErrorString = Locale::LoadStringDefault(
									IDS_NO_FILE_SELECTED,
									TEXT("You must select at least one file."));

							if (ErrorString)
							{
								MessageBox(
										GetWindow(),
										ErrorString,
										NULL,
										MB_ICONEXCLAMATION | MB_OK);

								Locale::FreeString(
										ErrorString);
							}

							break;
						}

						//
						// Allocate storage for the selected indexes and query each
						// them
						//
						SelIndexes = (LPDWORD)malloc(sizeof(DWORD) * SelCount);

						if ((!SelIndexes) ||
						    (SendMessage(
								ExemptionList,
								LB_GETSELITEMS,
								(WPARAM)(LRESULT)SelCount,
								(WPARAM)(LPDWORD)SelIndexes) == LB_ERR))
						{
							if (SelIndexes)
								free(
										SelIndexes);
							break;
						}

						//
						// Enumerate the returned selected indexes, removing each one
						//
						for (Index = 0;
						     Index < SelCount;
						     Index++)
						{
							LPTSTR NtFilePath = (LPTSTR)SendMessage(
									ExemptionList,
									LB_GETITEMDATA,
									SelIndexes[Index],
									0);

							//
							// If the NT file path is valid...
							//
							if ((NtFilePath) &&
							    (NtFilePath != (LPTSTR)LB_ERR))
							{
								DWORD Result;

								//
								// Remove the path from the current exemption type
								//
								if ((Result = RemoveExemptionFromCurrentType(
										NtFilePath)) != ERROR_SUCCESS)
								{
									LPTSTR ErrorString;
									TCHAR  Error[256] = { 0 };

									ErrorString = Locale::LoadStringDefault(
											IDS_ERROR_COULD_NOT_REMOVE_EXEMPTION,
											TEXT("The exemption could not be removed (error %lu):\n\n%s"));

									if (ErrorString)
									{
										_sntprintf_s(
												Error,
												sizeof(Error) - sizeof(TCHAR),
												ErrorString,
												Result,
												NtFilePath);

										MessageBox(
												GetWindow(),
												Error,
												NULL,
												MB_ICONERROR | MB_OK);

										Locale::FreeString(
												ErrorString);
									}
								}
								else
									Refresh = TRUE;
							}
						}
								
						if (Refresh)
							DisplayExemptions(
									CurrentExemptionType);

						free(
								SelIndexes);
					}
					break;
				default:
					break;
			}
			break;
		//
		// Flush button
		//
		case EXEMPTIONS_BUTTON_FLUSH:
			switch (Code)
			{
				case BN_CLICKED:
					{
						DWORD Result;

						if ((Result = FlushExemptionsFromCurrentType()) != ERROR_SUCCESS)
						{
							LPTSTR ErrorString;
							TCHAR  Error[256] = { 0 };

							ErrorString = Locale::LoadStringDefault(
									IDS_ERROR_COULD_NOT_FLUSH_EXEMPTIONS,
									TEXT("The exemptions could not be flushed (error %lu)."));

							if (ErrorString)
							{
								_sntprintf_s(
										Error,
										sizeof(Error) - sizeof(TCHAR),
										ErrorString,
										Result);

								MessageBox(
										GetWindow(),
										Error,
										NULL,
										MB_ICONERROR | MB_OK);

								Locale::FreeString(
										ErrorString);
							}
						}
						else
							DisplayExemptions(
									CurrentExemptionType);
					}
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}

	return Result;
}

//
// Updates the list control with all of the application exemptions that have been
// defined.
//
VOID ExemptionsDialog::DisplayApplicationExemptions()
{
	DisplayExemptions(
			ApplicationExemption);
}

//
// Updates the list control with all of the image file exemptions that have been
// defined.
//
VOID ExemptionsDialog::DisplayImageFileExemptions()
{
	DisplayExemptions(
			ImageFileExemption);
}

//
// Updates the list control with all of the directory exemptions that have been
// defined.
//
VOID ExemptionsDialog::DisplayDirectoryExemptions()
{
	DisplayExemptions(
			DirectoryExemption);
}

//
// Displays exemptions associated with a given type
//
VOID ExemptionsDialog::DisplayExemptions(
		IN EXEMPTION_TYPE Type)
{
	LRESULT ListIndex;
	LPTSTR  NtFilePath = NULL;
	DWORD   NtFilePathSize = 0;
	DWORD   Index = 0;
	HKEY    ScopeKey = NULL;
	HWND    ExemptionList;
	
	ExemptionList = GetControl(
			EXEMPTIONS_LIST_APPLIED);

	//
	// Flush out all of the currently applied exemptions
	//
	FlushExemptionList();

	//
	// Set the current exemption type to the one supplied
	//
	CurrentExemptionType = Type;

	do
	{
		//
		// Open the arbitrary exemption key
		//
		if (!(ScopeKey = Config::OpenExemptionScopeKey(
				GlobalScope)))
		{
			break;
		}

		//
		// Enumerate all of the exemptions in the key
		//
		while (1)
		{
			LPTSTR FileName;
			DWORD  Result;

			//
			// Allocate storage for the file path
			//
			NtFilePathSize = 1024 * sizeof(TCHAR);

			if (!(NtFilePath = (LPTSTR)malloc(
					NtFilePathSize)))
			{
				break;
			}

			//
			// If the enumeration fails, inspect the reason for the failure
			//
			if ((Result = Config::EnumerateExemptionScopeKey(
					ScopeKey,
					Type,
					&Index,
					NtFilePath,
					&NtFilePathSize,
					NULL)) != ERROR_SUCCESS)
			{
				// 
				// If the reason was because the buffer was too small...grow it
				//
				if (Result == ERROR_MORE_DATA)
				{
					free(
							NtFilePath);

					if (!(NtFilePath = (LPTSTR)malloc(
							NtFilePathSize)))
					{
						break;
					}

					// 
					// Loop around again with the same index in order to obtain the
					// NT path at this index now that the buffer is large enough
					//
					continue;
				}
				else
				{
					free(
							NtFilePath);
					break;
				}
			}

			//
			// Extract the file name from the path name
			//
			FileName = StrRChr(
					NtFilePath,
					NULL,
					'\\');

			if ((FileName) &&
			    (FileName[1] != 0))
				FileName++;
			else
				FileName = NtFilePath;

			//
			// Add this path name to the list control
			//
			ListIndex = SendMessage(
					ExemptionList,
					LB_ADDSTRING,
					0,
					(LPARAM)(LPTSTR)FileName);

			if (ListIndex >= 0)
			{
				SendMessage(
						ExemptionList,
						LB_SETITEMDATA,
						ListIndex,
						(LPARAM)(LPTSTR)NtFilePath);
			}

			//
			// Go to the next index
			//
			Index++;
		}

	} while (0);

	//
	// Close the exemption key
	//
	if (ScopeKey)
		Config::CloseExemptionScopeKey(
				ScopeKey);
}

//
// Flush the list of exemptions and free memory associated with each entry in
// the control
//
VOID ExemptionsDialog::FlushExemptionList()
{
	LPTSTR NtFilePath;
	HWND   ExemptionList;

	ExemptionList = GetControl(
			EXEMPTIONS_LIST_APPLIED);

	while ((NtFilePath = (LPTSTR)SendMessage(
			ExemptionList,
			LB_GETITEMDATA,
			0,
			0)) != (LPTSTR)LB_ERR)
	{
		if (NtFilePath)
			free(
					NtFilePath);

		SendMessage(
				ExemptionList,
				LB_DELETESTRING,
				0,
				0);
	}
}

//
// Removes an exemption from the current exemption type
//
DWORD ExemptionsDialog::RemoveExemptionFromCurrentType(
		IN LPTSTR FilePath)
{
	HANDLE Driver = DriverClient::Open();
	DWORD  Result;

	//
	// If we have a driver handle...
	//
	if (Driver)
	{
		//
		// Call the removal routine
		//
		Result = DriverClient::RemoveExemption(
				Driver,
				GlobalScope,
				CurrentExemptionType,
				EXEMPT_ALL,
				FilePath,
				TRUE);

		DriverClient::Close(
				Driver);
	}
	else
		Result = GetLastError();

	return Result;
}

//
// Flushes all exemptions from the current exemption type
//
DWORD ExemptionsDialog::FlushExemptionsFromCurrentType()
{
	HANDLE Driver = DriverClient::Open();
	DWORD  Result;

	//
	// If we have a driver handle...
	//
	if (Driver)
	{
		Result = DriverClient::FlushExemptions(
				Driver,
				GlobalScope,
				CurrentExemptionType);

		DriverClient::Close(
				Driver);
	}
	else
		Result = GetLastError();

	return Result;
}

//
// Returns the current selections NT file path
//
LPTSTR ExemptionsDialog::GetCurrentSelectionNtFilePath()
{
	LPTSTR NtFilePath = NULL;
	LONG   Index = 0;
	LONG   Count = 0;
	HWND   Control = GetControl(
			EXEMPTIONS_LIST_APPLIED);

	Count = SendMessage(
			Control,
			LB_GETSELCOUNT,
			0,
			0);
	Index = SendMessage(
			Control,
			LB_GETCURSEL,
			0,
			0);

	if ((Count >= 1) &&
	    (Index != LB_ERR))
	{
		NtFilePath = (LPTSTR)SendMessage(
				Control,
				LB_GETITEMDATA,
				(WPARAM)Index,
				0);
	}

	return NtFilePath;
}
