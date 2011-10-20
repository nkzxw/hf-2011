#include "Precomp.h"

Dialog::Dialog(
		IN INT InResourceId, 
		IN HMODULE InInstance)
: ResourceId(InResourceId),
  Instance(InInstance),
  Window(NULL)
{
	if (!Instance)
		Instance = GetModuleHandle(0);
}

Dialog::~Dialog()
{
	Close();
}

////
//
// Getters/Setters
//
////

//
// Return the dialog's window handle
//
HWND Dialog::GetWindow()
{
	return Window;
}

//
// Return the dialog's resource identifier
//
INT Dialog::GetResourceId()
{
	return ResourceId;
}

//
// Return the dialog's module instance
//
HMODULE Dialog::GetInstance()
{
	return Instance;
}

//
// Change the position of the dialog
//
DWORD Dialog::SetPosition(
		IN INT X,
		IN INT Y)
{
	DWORD Result = ERROR_SUCCESS;

	if (!SetWindowPos(
			GetWindow(),
			HWND_TOPMOST,
			X,
			Y,
			0,
			0,
			SWP_NOZORDER | SWP_NOSIZE | SWP_NOOWNERZORDER))
		Result = GetLastError();

	return Result;
}

////
//
// Notifications
//
////

//
// Notified when the dialog is initialized.  If FALSE is returned, the dialog
// remains hidden, otherwise it is shown.
//
BOOLEAN Dialog::OnInit()
{
	return TRUE;
}

//
// Notified when the dialog is being closed.  If FALSE is returned, the dialog
// does not close, otherwise it closes.
//
BOOLEAN Dialog::OnClose()
{
	return TRUE;
}

////
//
// Builders
//
/////

//
// Create the dialog and return its handle on success
//
DWORD Dialog::Create(
		IN HWND Parent,
		IN BOOLEAN Modal)
{
	DWORD Result = ERROR_SUCCESS;

	if (!Window)
	{
		if (Modal)
		{
			if (DialogBoxParam(
					GetInstance(),
					MAKEINTRESOURCE(GetResourceId()),
					Parent,
					(DLGPROC)OnMsgSt,
					(LONG)this) == -1)
				Result = GetLastError();
		}
		else
		{
			if (!(Window = CreateDialogParam(
					GetInstance(),
					MAKEINTRESOURCE(GetResourceId()),
					Parent,
					(DLGPROC)OnMsgSt,
					(LONG)this)))
				Result = GetLastError();
		}
	}
	else
	{
		//
		// If the window is already created, make it visible and bring it to the
		// foreground
		//
		ShowWindow(
				GetWindow(), 
				SW_SHOW);

		SetForegroundWindow(
				GetWindow());
	}

	//
	// If there is no window handle, log that fact
	//
	if ((!Modal) &&
	    (!Window))
		Log(LOG_SEV_ERROR, 
				TEXT("Create(): Dialog %d failed to create, %lu."),
				GetResourceId(),
				GetLastError());

	return Result;
}

//
// Instruct the dialog to close
//
VOID Dialog::Close()
{
	SendMessage(
			GetWindow(),
			WM_CLOSE,
			0,
			0);
}	

//
// Make the dialog visible
//
VOID Dialog::Show()
{
	ShowWindow(
			GetWindow(),
			SW_SHOW);
}

//
// Hide the dialog from being visible
//
VOID Dialog::Hide()
{
	ShowWindow(
			GetWindow(),
			SW_HIDE);
}

////
//
// Protected Methods
//
////

//
// The entry point for handling messages to the dialog
//
LRESULT Dialog::OnMsgSt(
		IN HWND Wnd,
		IN UINT Msg,
		IN WPARAM Wp,
		IN LPARAM Lp)
{
	LRESULT Result = 0;
	Dialog *This;

	//
	// Get the class pointer if possible
	//
	This = (Dialog *)GetWindowLong(
			Wnd,
			DWL_USER);

	switch (Msg)
	{
		case WM_INITDIALOG:
			if (Lp)
			{
				This = (Dialog *)Lp;

				//
				// Save the class pointer as being associated with the window handle
				//
				SetWindowLong(
						Wnd,
						DWL_USER,
						(LONG)This);

				//
				// Set the class' window handle
				//
				This->Window = Wnd;
			}
			break;
		default:
			break;
	}

	//
	// If the class pointer is valid, pass it onward
	//
	if (This)
		This->OnMsg(
				Msg,
				Wp,
				Lp);

	return Result;
}

//
// Class method for doing notification callbacks
//
LRESULT Dialog::OnMsg(
		IN UINT Msg,
		IN WPARAM Wp,
		IN LPARAM Lp)
{
	LRESULT Result = 0;

	switch (Msg)
	{
		case WM_INITDIALOG:
			if (OnInit())
				ShowWindow(Window, SW_SHOW);
			break;
		case WM_CLOSE:
			if (OnClose())
				DestroyWindow(Window);
			break;
		case WM_TIMER:
			OnTimer((UINT)Wp);
			break;
		case WM_NOTIFY:
			{
				LPNMHDR Notify = (LPNMHDR)Lp;
				HWND ControlWindow = GetControl(
						Notify->idFrom);

				if (ControlWindow)
					Result = OnNotifyControl(
							Notify->idFrom,
							ControlWindow,
							Notify->code,
							(PVOID)Lp);
			}
			break;
		case WM_COMMAND:
			{
				LONG NotifyCode, Wid;
				INT ControlId;

				ControlId  = LOWORD(Wp);
				NotifyCode = HIWORD(Wp);
				Wid        = LOWORD(Wp);

				//
				// If the escape button was clicked...
				//
				if ((NotifyCode == 0) &&
				    (Wid == 2))
					Close();
				else
				{
					HWND ControlWindow = GetControl(ControlId);

					if (ControlWindow)
						Result = OnNotifyControl(
								ControlId,
								ControlWindow,
								NotifyCode,
								(PVOID)Lp);
				}
			}
			break;
		case WM_DESTROY:
			EndDialog(Window, 0);

			Window = NULL;
			break;
	}

	return Result;
}

//
// Control notification message handler
//
LRESULT Dialog::OnNotifyControl(
		IN INT ControlId,
		IN HWND ControlWindow,
		IN UINT Code,
		IN PVOID Notify)
{
	return 0;
}

//
// Stub for timer notifications
//
VOID Dialog::OnTimer(
		IN UINT TimerId)
{
}

//
// Get a the window handle of a control on a dialog
//
HWND Dialog::GetControl(
		IN INT ControlId)
{
	return GetDlgItem(
			GetWindow(),
			ControlId);
}

//
// Update the string of a control item from the locale DLL
//
VOID Dialog::UpdateControlText(
		IN INT ControlId,
		IN INT LocaleStringId)
{
	LPTSTR String;

	if ((String = Locale::LoadString(
			LocaleStringId)))
	{
		SendMessage(
				GetControl(ControlId),
				WM_SETTEXT,
				0,
				(LPARAM)String);

		Locale::FreeString(
				String);
	}
}

//
// Updates the text of a control item with a format string that is obtained from
// the language library
//
VOID Dialog::UpdateControlTextFmt(
		IN INT ControlId,
		IN BOOLEAN FreeLocaleString,
		IN LPTSTR LocaleString,
		IN ...)
{
	TCHAR Buffer[4096];
	va_list ap;

	if (!LocaleString)
		return;

	//
	// Build the full string from the locale string
	//
	va_start(
			ap,
			LocaleString);
	_vsntprintf_s(
			Buffer,
			sizeof(Buffer) / sizeof(TCHAR),
			(sizeof(Buffer) / sizeof(TCHAR)) - sizeof(TCHAR),
			LocaleString,
			ap);
	va_end(
			ap);

	//
	// Update the control's text
	//
	SendMessage(
			GetControl(ControlId),
			WM_SETTEXT,
			0,
			(LPARAM)Buffer);

	//
	// Free the locale string that was passed
	//
	if (FreeLocaleString)
		Locale::FreeString(
				LocaleString);
}
