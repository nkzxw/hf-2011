/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#include "Precomp.h"

#define SYSTRAY_ID     1
#define WM_SYSTRAY     WM_USER + 1

#define MENU_ITEM_EXIT           1000
#define MENU_ITEM_STATUS         1001

#define MENU_ITEM_LOCALE_ENGLISH 1100
#define MENU_ITEM_LOCALE_GERMAN  1101
#define MENU_ITEM_LOCALE_SPANISH 1102

SystemTray::SystemTray(
		IN HMODULE InInstance)
: Instance(InInstance),
  Window(NULL)
{
	if (!Instance)
		Instance = GetModuleHandle(0);

	TaskbarCreatedMessage = RegisterWindowMessage(TEXT("TaskbarCreated"));
}

SystemTray::~SystemTray()
{
	Destroy();
}

//
// Creates the system tray icon
//
DWORD SystemTray::Create()
{
	WNDCLASS WindowClass;
	LPCTSTR  ClassName = TEXT("WehnTrustTray");

	ZeroMemory(
			&WindowClass,
			sizeof(WindowClass));

	WindowClass.hInstance     = Instance;
	WindowClass.lpfnWndProc   = (WNDPROC)OnMsgSt;
	WindowClass.lpszClassName = ClassName;
	WindowClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	WindowClass.hCursor       = LoadCursor(
			NULL, 
			IDC_ARROW);

	//
	// Use the enabled icon
	//
	WindowClass.hIcon         = LoadIcon(
			Instance,
			MAKEINTRESOURCE(IDI_WEHNTRUST_ENABLED));

	//
	// Register the window class
	//
	RegisterClass(
			&WindowClass);

	do
	{
		//
		// Create the tray window
		//
		if (!(Window = CreateWindow(
				ClassName,
				TEXT("WehnTrustTray"),
				WS_OVERLAPPEDWINDOW,
				5,
				5,
				5,
				5,
				0,
				0,
				Instance,
				0)))
		{
			Log(LOG_SEV_ERROR, 
					TEXT("Create(): Failed to create system tray window, %lu."),
					GetLastError());
			break;
		}

		//
		// Keep the class pointer associated with the window handle
		//
		SetWindowLong(
				Window,
				GWL_USERDATA,
				(LONG)this);

		SendMessage(
				Window,
				WM_CREATE,
				0,
				0);

		UpdateWindow(
				Window);

		//
		// Update the icon
		//
		DisplayIcon(
				NIM_ADD,
				IDI_WEHNTRUST_ENABLED,
				TEXT("WehnTrust"));

		//
		// Register the base event subscriber to WehnServ so we can receive
		// notifications that might be useful to display to the user.
		//
		RegisterEventSubscriber();

		SetLastError(0);

	} while (0);

	return GetLastError();
}

//
// Closes and destroys the system tray window
//
DWORD SystemTray::Destroy()
{
	if (Window)
	{
		//
		// Deregister the event subscriber we registered.
		//
		DeregisterEventSubscriber();

		DisplayIcon(
				NIM_DELETE,
				0,
				NULL);

		Window = NULL;
	}

	return ERROR_SUCCESS;
}

////
//
// Protected Methods
//
////

//
// Calls the class method for dispatching messages if possible
//
LRESULT SystemTray::OnMsgSt(
		IN HWND Wnd,
		IN UINT Msg,
		IN WPARAM Wp,
		IN LPARAM Lp)
{
	SystemTray *This;
	LRESULT    Result = 0;
	BOOLEAN    Handled = FALSE;
	
	This = (SystemTray *)GetWindowLong(
			Wnd,
			GWL_USERDATA);

	if (This)
		Result = This->OnMsg(
				Msg,
				Wp,
				Lp,
				&Handled);

	return (Handled) 
		? Result
		: DefWindowProc(
			Wnd, 
			Msg, 
			Wp, 
			Lp);
}

//
// Class method for dispatching window messages
//
LRESULT SystemTray::OnMsg(
		IN UINT Msg,
		IN WPARAM Wp,
		IN LPARAM Lp,
		OUT PBOOLEAN Handled)
{
	LRESULT Result = 0;

	switch (Msg)
	{
		case WM_SYSTRAY:
			switch (Lp)
			{
				//
				// Left double click
				//
				case WM_LBUTTONDBLCLK:
					Ui::Display(
							TRUE, 
							FALSE);
					break;
				//
				// Right button down, display the system tray menu
				//
				case WM_RBUTTONDOWN:
					{
						LPTSTR String;
						POINT  Point;
						HMENU  Menu, LocaleMenu;

						SetForegroundWindow(
								Window);

						Menu = CreatePopupMenu();

						//
						// Create the locale sub-menu
						//
						if ((LocaleMenu = CreatePopupMenu()))
						{
							MENUITEMINFO mii;

							ZeroMemory(
									&mii,
									sizeof(mii));

							AppendMenu(
									LocaleMenu,
									MF_STRING,
									MENU_ITEM_LOCALE_ENGLISH,
									TEXT("English"));
							AppendMenu(
									LocaleMenu,
									MF_STRING,
									MENU_ITEM_LOCALE_GERMAN,
									TEXT("German"));
							AppendMenu(
									LocaleMenu,
									MF_STRING,
									MENU_ITEM_LOCALE_SPANISH,
									TEXT("Spanish"));

							mii.cbSize     = sizeof(mii);
							mii.fMask      = MIIM_TYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU;
							mii.fType      = MFT_STRING;
							mii.fState     = MFS_ENABLED;
							mii.wID        = 0;
							mii.dwTypeData = TEXT("Language");
							mii.hSubMenu   = LocaleMenu;

							InsertMenuItem(
									Menu,
									0,
									FALSE,
									&mii);
						}

						//
						// Append Status
						//
						if ((String = Locale::LoadString(
								IDS_STATUS)))
						{
							AppendMenu(
									Menu,
									MF_STRING,
									MENU_ITEM_STATUS,
									String);

							Locale::FreeString(
									String);
						}
						else
							AppendMenu(
									Menu,
									MF_STRING,
									MENU_ITEM_STATUS,
									TEXT("Status"));

						//
						// Append Exit
						//
						if ((String = Locale::LoadString(
								IDS_EXIT)))
						{
							AppendMenu(
									Menu,
									MF_STRING,
									MENU_ITEM_EXIT,
									String);

							Locale::FreeString(
									String);
						}
						else
							AppendMenu(
									Menu,
									MF_STRING,
									MENU_ITEM_EXIT,
									TEXT("Exit"));

						GetCursorPos(
								&Point);

						//
						// Display the menu
						//
						TrackPopupMenu(
								Menu,
								TPM_LEFTALIGN | TPM_RIGHTBUTTON,
								Point.x,
								Point.y,
								0,
								Window,
								0);

						DestroyMenu(
								Menu);
					}
					break;
				default:
					break;
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(Wp))
			{
				case MENU_ITEM_STATUS:
					Ui::Display(
							TRUE, 
							FALSE);
					break;
				case MENU_ITEM_EXIT:
					Ui::Close();
					break;
				//
				// Locales
				//
				case MENU_ITEM_LOCALE_ENGLISH:
					Config::SetLocale(
							"en_US");

					Ui::Close(
							UI_RESULT_RESTART);
					break;
				case MENU_ITEM_LOCALE_GERMAN:
					Config::SetLocale(
							"de_DE");

					Ui::Close(
							UI_RESULT_RESTART);
					break;
				case MENU_ITEM_LOCALE_SPANISH:
					Config::SetLocale(
							"es_ES");

					Ui::Close(
							UI_RESULT_RESTART);
				default:
					break;
			}
			break;
		default:
			if (Msg == TaskbarCreatedMessage)
			{
				//
				// If the shell started, then we need to revive our icon.
				//
				// XXX if the icon image/text ever becomes multi-state, then
				// we need to remember the previous state to use here.
				//

				DisplayIcon(
					NIM_ADD,
					IDI_WEHNTRUST_ENABLED,
					TEXT("WehnTrust"));
			}
			break;
	}

	return Result;
}

//
// Display the icon in the system tray
//
VOID SystemTray::DisplayIcon(
		IN UINT NotifyAction,
		IN INT ResourceId,
		IN LPCTSTR ToolTip)
{
	NOTIFYICONDATA NotifyIconData;

	NotifyIconData.cbSize           = sizeof(NotifyIconData);
	NotifyIconData.hWnd             = Window;
	NotifyIconData.uID              = WM_SYSTRAY;
	NotifyIconData.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	NotifyIconData.uCallbackMessage = WM_SYSTRAY;
	NotifyIconData.hIcon            = LoadIcon(
			Instance,
			MAKEINTRESOURCE(ResourceId));

	if (ToolTip)
		lstrcpyn(
				NotifyIconData.szTip,
				ToolTip,
				sizeof(NotifyIconData.szTip) - sizeof(TCHAR));
	else
		NotifyIconData.szTip[0] = 0;

	Shell_NotifyIcon(
			NotifyAction,
			&NotifyIconData);

	if (NotifyIconData.hIcon)
		DestroyIcon(NotifyIconData.hIcon);

}

VOID SystemTray::OnExploitationEvent(
		IN ULONG ProcessId,
		IN PEXPLOIT_INFORMATION ExploitInformation)
{
	NOTIFYICONDATA NotifyIconData = { 0 };
	WCHAR          ProcessFileName[32] = { 0 };

	Native::GetProcessImageFileName(
			ProcessId,
			ProcessFileName,
			sizeof(ProcessFileName),
			NULL);

	NotifyIconData.cbSize           = sizeof(NotifyIconData);
	NotifyIconData.hWnd             = Window;
	NotifyIconData.uFlags           = NIF_INFO;
	NotifyIconData.uTimeout         = 10000;
	NotifyIconData.dwInfoFlags      = NIIF_WARNING;
	NotifyIconData.uID              = WM_SYSTRAY;
	NotifyIconData.uCallbackMessage = WM_SYSTRAY;

	if (ProcessFileName[0])
		_sntprintf_s(
				NotifyIconData.szInfo,
				sizeof(NotifyIconData.szInfo) / sizeof(TCHAR),
				TEXT("WehnTrust has blocked an exploit (%S)"),
				ProcessFileName);
	else
		_sntprintf_s(
				NotifyIconData.szInfo,
				sizeof(NotifyIconData.szInfo) / sizeof(TCHAR),
				TEXT("WehnTrust has blocked an exploit (%lu)"),
				ProcessId);

	_tcscpy_s(
			NotifyIconData.szInfoTitle,
			TEXT("Exploit Prevented"));

	Shell_NotifyIcon(
			NIM_MODIFY,
			&NotifyIconData);
}
