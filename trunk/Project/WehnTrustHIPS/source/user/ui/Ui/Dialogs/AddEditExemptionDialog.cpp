#include "Precomp.h"

AddEditExemptionDialog::AddEditExemptionDialog(
		IN EXEMPTION_SCOPE InScope,
		IN EXEMPTION_TYPE InType,
		IN LPTSTR InNtFilePath OPTIONAL)
: Dialog(IDD_ADDEDIT_EXEMPTION),
  Scope(InScope),
  Type(InType),
  NtFilePath(InNtFilePath),
  Flags(0)
{
}

AddEditExemptionDialog::~AddEditExemptionDialog()
{
}

//
// Initialize the dialog based on the information the class was initialized with
//
BOOLEAN AddEditExemptionDialog::OnInit()
{
	BOOLEAN UseDefaults = TRUE;
	LPTSTR  PathLabelText = NULL;

	// ADDEDIT_EXEMPTION_LABEL_PATH
	// ADDEDIT_EXEMPTION_EDIT_PATH
	// ADDEDIT_EXEMPTION_BUTTON_FIND
	// ADDEDIT_EXEMPTION_CHECK_RANDALLOC
	// ADDEDIT_EXEMPTION_CHECK_RANDIMAGE
	// ADDEDIT_EXEMPTION_BUTTON_OK
	// ADDEDIT_EXEMPTION_BUTTON_CANCEL

	UpdateControlText(
			ADDEDIT_EXEMPTION_CHECK_RANDALLOC,
			IDS_DISABLE_RANDALLOC);
	UpdateControlText(
			ADDEDIT_EXEMPTION_CHECK_RANDIMAGE,
			IDS_DISABLE_RANDIMAGE);
	UpdateControlText(
			ADDEDIT_EXEMPTION_CHECK_NRE_SEH,
			IDS_DISABLE_NRE_SEH);
	UpdateControlText(
			ADDEDIT_EXEMPTION_CHECK_NRE_STACK,
			IDS_DISABLE_NRE_STACK);
	UpdateControlText(
			ADDEDIT_EXEMPTION_CHECK_NRE_FORMAT,
			IDS_DISABLE_NRE_FORMAT);

	//
	// Set the path label
	//
	switch (Type)
	{
		case ApplicationExemption:
			PathLabelText = Locale::LoadStringDefault(
					IDS_APPLICATION_PATH_TO_EXEMPT,
					TEXT("Application executable path to exempt:"));
			break;
		case DirectoryExemption:
			PathLabelText = Locale::LoadStringDefault(
					IDS_DIRECTORY_PATH_TO_EXEMPT,
					TEXT("Directory path to exempt:"));
			break;
		case ImageFileExemption:
			PathLabelText = Locale::LoadStringDefault(
					IDS_IMAGE_FILE_PATH_TO_EXEMPT,
					TEXT("Image file path to exempt:"));
			break;
		default:
			break;
	}

	if (PathLabelText)
	{
		HWND PathLabelWindow = GetControl(
				ADDEDIT_EXEMPTION_LABEL_PATH);

		SendMessage(
				PathLabelWindow,
				WM_SETTEXT,
				NULL,
				(LPARAM)(LPTSTR)PathLabelText);

		Locale::FreeString(
				PathLabelText);
	}

	//
	// Set the edit path if we were supplied an NT path and get information about
	// it
	//
	if (NtFilePath)
	{
		LPTSTR EditTitleText = NULL;
		DWORD  Result;
		HWND   PathEditWindow = GetControl(
				ADDEDIT_EXEMPTION_EDIT_PATH);

		SendMessage(
				PathEditWindow,
				WM_SETTEXT,
				NULL,
				(LPARAM)(LPTSTR)NtFilePath);

		if ((Result = Config::GetExemptionInfo(
				Scope,
				Type,
				NtFilePath,
				&Flags)) == ERROR_SUCCESS)
		{
			ModifyCheck(
					ADDEDIT_EXEMPTION_CHECK_RANDALLOC,
					(Flags & EXEMPT_MEMORY_ALLOCATIONS)
						? TRUE : FALSE);
			ModifyCheck(
					ADDEDIT_EXEMPTION_CHECK_RANDIMAGE,
					(Flags & EXEMPT_IMAGE_FILES)
						? TRUE : FALSE);

			ModifyCheck(
					ADDEDIT_EXEMPTION_CHECK_NRE_SEH,
					(Flags & EXEMPT_NRE_SEH)
						? TRUE : FALSE);
			ModifyCheck(
					ADDEDIT_EXEMPTION_CHECK_NRE_STACK,
					(Flags & EXEMPT_NRE_STACK)
						? TRUE : FALSE);
			ModifyCheck(
					ADDEDIT_EXEMPTION_CHECK_NRE_FORMAT,
					(Flags & EXEMPT_NRE_FORMAT)
						? TRUE : FALSE);

			UseDefaults = FALSE;
		}
		
		EditTitleText = Locale::LoadStringDefault(
					IDS_EDIT_EXEMPTION,
					TEXT("Edit Exemption"));

		if (EditTitleText)
		{
			SendMessage(
					GetWindow(),
					WM_SETTEXT,
					0,
					(LPARAM)(LPTSTR)EditTitleText);

			Locale::FreeString(
					EditTitleText);
		}

		//
		// Don't allow the editing of the path
		//
		EnableControl(
				ADDEDIT_EXEMPTION_EDIT_PATH,
				FALSE);
		EnableControl(
				ADDEDIT_EXEMPTION_BUTTON_FIND,
				FALSE);
	}
	else
	{
		LPTSTR AddTitleText = Locale::LoadStringDefault(
					IDS_ADD_EXEMPTION,
					TEXT("Add Exemption"));

		if (AddTitleText)
		{
			SendMessage(
					GetWindow(),
					WM_SETTEXT,
					0,
					(LPARAM)(LPTSTR)AddTitleText);

			Locale::FreeString(
					AddTitleText);
		}
	}

	//
	// If we should use the default check state...
	//
	if (UseDefaults)
	{
		//
		// Initialize the check state
		//
		if ((Type == ApplicationExemption) ||
		    (Type == DirectoryExemption))
		{
			ModifyCheck(
					ADDEDIT_EXEMPTION_CHECK_RANDALLOC,
					TRUE);
			ModifyCheck(
					ADDEDIT_EXEMPTION_CHECK_RANDIMAGE,
					TRUE);
		}
		else
		{
			ModifyCheck(
					ADDEDIT_EXEMPTION_CHECK_RANDIMAGE,
					TRUE);
		}
	}

	//
	// If it's not an application exemption, disable the NRE protection
	// mechanisms.
	//
	if (Type != ApplicationExemption)
	{
		EnableControl(
				ADDEDIT_EXEMPTION_CHECK_NRE_SEH,
				FALSE);
		EnableControl(
				ADDEDIT_EXEMPTION_CHECK_NRE_STACK,
				FALSE);
		EnableControl(
				ADDEDIT_EXEMPTION_CHECK_NRE_FORMAT,
				FALSE);
	}
	//
	// Otherwise, if it is an application exemption and we should use defaults,
	// check all three of them.
	//
	else if (UseDefaults)
	{
		ModifyCheck(
				ADDEDIT_EXEMPTION_CHECK_NRE_SEH,
				FALSE);
		ModifyCheck(
				ADDEDIT_EXEMPTION_CHECK_NRE_STACK,
				FALSE);
		ModifyCheck(
				ADDEDIT_EXEMPTION_CHECK_NRE_FORMAT,
				FALSE);
	}

	//
	// If it's an image file exemption, disable the randomized allocations check
	// as it's not applicable
	//
	if (Type == ImageFileExemption)
	{
		EnableControl(
				ADDEDIT_EXEMPTION_CHECK_RANDALLOC,
				FALSE);
	}

	return TRUE;
}

//
// Cleanup n stuff
//
BOOLEAN AddEditExemptionDialog::OnClose()
{
	return TRUE;
}

////
//
// Protected Methods
//
////

//
// Handles control code notifications
//
LRESULT AddEditExemptionDialog::OnNotifyControl(
		IN INT ControlId,
		IN HWND ControlWindow,
		IN UINT Code,
		IN PVOID Notify)
{
	LRESULT Result = 0;

	switch (ControlId)
	{
		//
		// OK button
		//
		case ADDEDIT_EXEMPTION_BUTTON_OK:
			switch (Code)
			{
				case BN_CLICKED:
					{
						if (Save())
							Close();	
						else
						{
							LPTSTR ErrorString;
							TCHAR  Error[256] = { 0 };
							DWORD  Result = GetLastError();

							ErrorString = Locale::LoadStringDefault(
									IDS_ERROR_COULD_NOT_ADD_EXEMPTION,
									TEXT("The exemption could not be added or updated (error %lu)"));

							if (ErrorString)
							{
								_sntprintf_s(
										Error,
										(sizeof(Error) / sizeof(TCHAR)),
										(sizeof(Error) / sizeof(TCHAR)) - 1,
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
					}
					break;
				default:
					break;
			}
			break;
		//
		// Cancel button
		//
		case ADDEDIT_EXEMPTION_BUTTON_CANCEL:		
			switch (Code)
			{
				case BN_CLICKED:
					Close();
					break;
				default:
					break;
			}
			break;
		//
		// Find button
		//
		case ADDEDIT_EXEMPTION_BUTTON_FIND:
			switch (Code)
			{
				case BN_CLICKED:
					{
						OPENFILENAME OpenFileParameters;
						BOOLEAN      Success = FALSE;
						LPTSTR       FilePath = NULL;
						DWORD        FilePathSize = 1024 * sizeof(TCHAR);
						DWORD        Attempts = 0;

						do
						{
							// 
							// Allocate storage for the file path
							//
							if (FilePath)
								free(
										FilePath);

							FilePathSize += 1024 * sizeof(TCHAR);

							if (!(FilePath = (LPTSTR)malloc(
									FilePathSize)))
								break;

							ZeroMemory(
									FilePath,
									FilePathSize);

							if ((Type == ApplicationExemption) ||
								 (Type == ImageFileExemption))
							{
								//
								// Initialize the open file name parameters buffer
								//
								ZeroMemory(
										&OpenFileParameters,
										sizeof(OPENFILENAME));

								OpenFileParameters.lStructSize  = sizeof(OPENFILENAME);
								OpenFileParameters.hwndOwner    = GetWindow();
								OpenFileParameters.lpstrFile    = FilePath;
								OpenFileParameters.nMaxFile     = FilePathSize;
								OpenFileParameters.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

								if (Type == ApplicationExemption)
								{
									OpenFileParameters.lpstrFilter  = TEXT("All Files\0*.*\0Executable Files\0*.EXE\0");
									OpenFileParameters.nFilterIndex = 2;
									OpenFileParameters.lpstrTitle   = TEXT("Select the Application Executable to Exempt");
								}
								else
								{
									OpenFileParameters.lpstrFilter  = TEXT("All\0*.*\0Images Files\0*.EXE;*.DLL\0");
									OpenFileParameters.nFilterIndex = 2;
									OpenFileParameters.lpstrTitle   = TEXT("Select the Image File to Exempt");
								}

								//
								// Prompt the user for the file name to exempt
								//
								if (GetOpenFileName(
										&OpenFileParameters))
								{
									Success = TRUE;
									break;
								}
								else
								{
									if (CommDlgExtendedError() != FNERR_BUFFERTOOSMALL)
										break;
								}
							}
							else
							{
								LPITEMIDLIST IdList;
								BROWSEINFO   Browse;
								LPMALLOC     Malloc = NULL;

								ZeroMemory(
										&Browse,
										sizeof(Browse));

								CoInitializeEx(
										NULL,
										COINIT_APARTMENTTHREADED);

								Browse.hwndOwner      = GetWindow();
								Browse.pszDisplayName = FilePath;
								Browse.lpszTitle      = TEXT("Select a Directory to Exempt");
								Browse.ulFlags        = BIF_RETURNONLYFSDIRS | 
																BIF_EDITBOX;

								if ((!(IdList = SHBrowseForFolder(
										&Browse))) ||
									 (FAILED(SHGetMalloc(
										&Malloc))))
								{
									Success = FALSE;
									break;
								}

								SHGetPathFromIDList(
										IdList,
										FilePath);

								Malloc->Free(
										IdList);

								Malloc->Release();

								CoUninitialize();

								Success = TRUE;

								break;
							}

						} while (++Attempts < 3);

						//
						// Update the path edit box
						//
						if ((FilePath) &&
						    (FilePath[0]))
						{
							HWND PathEdit = GetControl(
									ADDEDIT_EXEMPTION_EDIT_PATH);

							if (PathEdit)
								SendMessage(
										PathEdit,
										WM_SETTEXT,
										0,
										(LPARAM)(LPTSTR)FilePath);
						}
						
						if (FilePath)
							free(
									FilePath);
					}
					break;
				default:
					break;
			}
			break;
	}

	return Result;
}

//
// Modifies the state of a check box
//
VOID AddEditExemptionDialog::ModifyCheck(
		IN INT ControlId,
		IN BOOLEAN Check)
{
	HWND Window = GetControl(
			ControlId);

	if (Window)
	{
		SendMessage(
				Window,
				BM_SETCHECK,
				(WPARAM)((Check) ? BST_CHECKED : BST_UNCHECKED),
				0);
	}
}

//
// Checks to see if the provided control is checked
//
BOOLEAN AddEditExemptionDialog::IsChecked(
		IN INT ControlId)
{
	BOOLEAN Checked = FALSE;
	HWND    Window = GetControl(
			ControlId);

	if (Window)
	{
		if (SendMessage(
				Window,
				BM_GETCHECK,
				0,
				0) == (LPARAM)BST_CHECKED)
			Checked = TRUE;
	}

	return Checked;
}

//
// Enables or disables a control
//
VOID AddEditExemptionDialog::EnableControl(
		IN INT ControlId,
		IN BOOLEAN Enabled)
{
	HWND Window = GetControl(
			ControlId);

	if (Window)
	{
		EnableWindow(
				Window,
				Enabled);
	}
}

//
// Saves changes by either adding or updating the existing exemption
//
BOOLEAN AddEditExemptionDialog::Save()
{
	BOOLEAN Success = FALSE;
	LPTSTR  NewFilePath = NULL;
	DWORD   NewFilePathSize = 0;
	HWND    PathEdit = GetControl(
			ADDEDIT_EXEMPTION_EDIT_PATH);

	if (PathEdit)
	{
		NewFilePathSize = (DWORD)SendMessage(
				PathEdit,
				WM_GETTEXTLENGTH,
				0,
				0);

		if (!(NewFilePath = (LPTSTR)malloc(
				NewFilePathSize + sizeof(TCHAR))))
			return FALSE;

		ZeroMemory(
				NewFilePath,
				NewFilePathSize + sizeof(TCHAR));

		SendMessage(
				PathEdit,
				WM_GETTEXT,
				(WPARAM)(DWORD)(NewFilePathSize + sizeof(TCHAR)),
				(LPARAM)(LPTSTR)NewFilePath);
	}

	//
	// Check to see the current flag state
	//
	if (IsChecked(
			ADDEDIT_EXEMPTION_CHECK_RANDALLOC))
		Flags |= EXEMPT_MEMORY_ALLOCATIONS;
	else
		Flags &= ~(EXEMPT_MEMORY_ALLOCATIONS);

	if (IsChecked(
			ADDEDIT_EXEMPTION_CHECK_RANDIMAGE))
		Flags |= EXEMPT_IMAGE_FILES;
	else
		Flags &= ~(EXEMPT_IMAGE_FILES);

	if (IsChecked(
			ADDEDIT_EXEMPTION_CHECK_NRE_SEH))
		Flags |= EXEMPT_NRE_SEH;
	else
		Flags &= ~(EXEMPT_NRE_SEH);
	
	if (IsChecked(
			ADDEDIT_EXEMPTION_CHECK_NRE_STACK))
		Flags |= EXEMPT_NRE_STACK;
	else
		Flags &= ~(EXEMPT_NRE_STACK);
	
	if (IsChecked(
			ADDEDIT_EXEMPTION_CHECK_NRE_FORMAT))
		Flags |= EXEMPT_NRE_FORMAT;
	else
		Flags &= ~(EXEMPT_NRE_FORMAT);

	//
	// If we were given back a valid file path, call into the
	// driver and add the exemption.
	//
	if ((NewFilePath) &&
	    (NewFilePath[0]))
	{
		HANDLE Driver = DriverClient::Open();
		DWORD  Result;

		//
		// If we have a driver handle...
		//
		if (Driver)
		{
			//
			// Call the addition routine (which will also update)
			//
			Result = DriverClient::AddExemption(
					Driver,
					Scope,
					Type,
					Flags,
					NewFilePath);

			DriverClient::Close(
					Driver);
		}
		else
			Result = GetLastError();

		if (Result == ERROR_SUCCESS)
			Success = TRUE;
	}

	if (NewFilePath)
		free(
				NewFilePath);

	return Success;
}
