/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_UI_SYSTEMTRAY_H
#define _WEHNTRUST_WEHNTRUST_UI_SYSTEMTRAY_H

class SystemTray :
	public WehnServEventSubscriber
{
	public:
		SystemTray(
				IN HMODULE InInstance = NULL);
		~SystemTray();

		DWORD Create();
		DWORD Destroy();

	protected:

		static LRESULT OnMsgSt(
				IN HWND Wnd,
				IN UINT Msg,
				IN WPARAM Wp,
				IN LPARAM Lp);

		LRESULT OnMsg(
				IN UINT Msg,
				IN WPARAM Wp,
				IN LPARAM Lp,
				OUT PBOOLEAN Handled);

		VOID DisplayIcon(
				IN UINT NotifyAction,
				IN INT ResourceId,
				IN LPCTSTR ToolTip);

		VOID OnExploitationEvent(
				IN ULONG ProcessId,
				IN PEXPLOIT_INFORMATION ExploitInformation);

		//
		// Attributes
		//

		HWND    Window;
		HMODULE Instance;
		UINT    TaskbarCreatedMessage;
};

#endif
