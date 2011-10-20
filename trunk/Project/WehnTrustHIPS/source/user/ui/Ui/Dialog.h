#ifndef _WEHNTRUST_WEHNTRUST_UI_DIALOG_H
#define _WEHNTRUST_WEHNTRUST_UI_DIALOG_H

//
// Base class for dialogs that implements code common across all dialogs
//
class Dialog
{
	public:
		Dialog(
				IN INT InResourceId, 
				IN HMODULE InInstance = NULL);
		virtual ~Dialog();

		//
		// Getters/Setters
		//
		HWND GetWindow();
		INT GetResourceId();
		HMODULE GetInstance();

		virtual DWORD SetPosition(
				IN INT X,
				IN INT Y);

		HWND GetControl(
				IN INT ControlId);

		VOID UpdateControlText(
				IN INT ControlId,
				IN INT LocaleStringId);
		
		VOID UpdateControlTextFmt(
				IN INT ControlId,
				IN BOOLEAN FreeLocaleString,
				IN LPTSTR LocaleStringId,
				IN ...);

		//
		// Notifications
		//
		virtual BOOLEAN OnInit();
		virtual BOOLEAN OnClose();

		//
		// Builders
		//
		virtual DWORD Create(
				IN HWND Parent = NULL,
				IN BOOLEAN Modal = FALSE);
		virtual VOID Close();
		
		virtual VOID Show();
		virtual VOID Hide();
	protected:
		static LRESULT OnMsgSt(
				IN HWND Wnd, 
				IN UINT Msg,
				IN WPARAM Wp,
				IN LPARAM Lp);
		virtual LRESULT OnMsg(
				IN UINT Msg, 
				IN WPARAM Wp, 
				IN LPARAM Lp);
		
		virtual LRESULT OnNotifyControl(
				IN INT ControlId,
				IN HWND ControlWindow,
				IN UINT Code,
				IN PVOID Notify);
		virtual VOID OnTimer(
				IN UINT TimerId);

		// 
		// Attributes
		//

		INT     ResourceId;
		HMODULE Instance;
		HWND    Window;
};

#endif
