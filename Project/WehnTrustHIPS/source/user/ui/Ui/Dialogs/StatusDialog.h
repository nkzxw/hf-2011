/*
 * WehnTrust
 *
 * Copyright (c) 2004, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_UI_DIALOGS_STATUSDIALOG_H
#define _WEHNTRUST_WEHNTRUST_UI_DIALOGS_STATUSDIALOG_H

class WehnTrustDialog;

//
// Displays status information and provides the user with the ability to
// start/stop randomization
//
class StatusDialog : public Dialog
{
	public:
		StatusDialog();
		~StatusDialog();

		//
		// Getters/Setters
		// 
		VOID SetParent(
				IN WehnTrustDialog *Parent);

		//
		// Notifications
		//
		
		BOOLEAN OnInit();
	protected:

		LRESULT OnNotifyControl(
				IN INT ControlId,
				IN HWND ControlWindow,
				IN UINT Code,
				IN PVOID Notify);

		//
		// Attributes
		//
		WehnTrustDialog *Parent;
};


#endif
