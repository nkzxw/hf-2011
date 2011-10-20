/*
 * WehnTrust
 *
 * Copyright (c) 2005, Wehnus.
 */
#ifndef _WEHNTRUST_WEHNTRUST_UI_DIALOGS_ADDEDITEXEMPTIONDIALOG_H
#define _WEHNTRUST_WEHNTRUST_UI_DIALOGS_ADDEDITEXEMPTIONDIALOG_H

class WehnTrustDialog;

//
// Provides the user with a dialog that allows them to add or edit an existing
// exemption of a given type
//
class AddEditExemptionDialog : public Dialog
{
	public:
		AddEditExemptionDialog(
				IN EXEMPTION_SCOPE Scope,
				IN EXEMPTION_TYPE Type,
				IN LPTSTR NtFilePath OPTIONAL);
		~AddEditExemptionDialog();

		//
		// Notifications
		//
		BOOLEAN OnInit();
		BOOLEAN OnClose();
	protected:
		LRESULT OnNotifyControl(
				IN INT ControlId,
				IN HWND ControlWindow,
				IN UINT Code,
				IN PVOID Notify);

		VOID ModifyCheck(
				IN INT ControlId,
				IN BOOLEAN Check);
		BOOLEAN IsChecked(
				IN INT ControlId);
		VOID EnableControl(
				IN INT ControlId,
				IN BOOLEAN Enabled);

		//
		// Saving
		//
		BOOLEAN Save();

		//
		// Attributes
		//
		EXEMPTION_SCOPE Scope;
		EXEMPTION_TYPE  Type;
		LPTSTR          NtFilePath;
		DWORD           Flags;
};


#endif


